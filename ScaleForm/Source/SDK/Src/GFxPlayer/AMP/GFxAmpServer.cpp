/**********************************************************************

Filename    :   GFxAmpServer.cpp
Content     :   Class encapsulating communication with AMP
                Embedded in application to be profiled
Created     :   December 2009
Authors     :   Alex Mantzaris

Copyright   :   (c) 2005-2009 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GConfig.h"

#ifdef GFX_AMP_SERVER

#include "GHeapTypes.h"
#include "GFxAmpThreadMgr.h"
#include "GFxAmpServer.h"
#include "GFxAmpMessage.h"
#include "GMsgFormat.h"
#include "GFxLog.h"
#include "GFxMeshCacheManager.h"
#include "GFxPlayerImpl.h"
#include "GFxAmpViewStats.h"
#include "GFxAmpProfileFrame.h"
#include "GAmpRenderer.h"
#include "GFxAmpMemoryImage.h"
#include "GFxAmpStream.h"
#include "GFxAmpServerCallbacks.h"

#if defined(GFC_OS_WIN32)
#define AMP_HEAP_RESERVE (256 * 1024)
#else
#define AMP_HEAP_RESERVE (16 * 1024)
#endif


// Static
GFxAmpServer* GFxAmpServer::AmpServerSingleton = NULL;

// Static
// Singleton accessor
GFxAmpServer& GFxAmpServer::GetInstance()
{
    return *AmpServerSingleton;
}

// Static
// AMP Server initialization
void GFxAmpServer::Init(UPInt arena, GSysAllocPaged* arenaAllocator)
{
    if (arena != 0)
    {
        GMemory::CreateArena(arena, arenaAllocator);
    }

    GMemoryHeap::HeapDesc desc;
    desc.Flags = GMemoryHeap::Heap_UserDebug;
    desc.Granularity = 16 * 1024;
    desc.Reserve = AMP_HEAP_RESERVE;
    desc.Limit = 1024 * 1024;
    desc.Arena = arena;

    GMemoryHeap* heap = GMemory::GetGlobalHeap()->CreateHeap("AMP", desc);
    AmpServerSingleton = GHEAP_NEW(heap) GFxAmpServer();
    heap->ReleaseOnFree(AmpServerSingleton);
}

// Static
// AMP Server uninitialization
void GFxAmpServer::Uninit()
{
    AmpServerSingleton->Release();
}

struct GFxAmpImageInfo
{
    GPtr<GFxImageResource>  ImageResource;
    GString                 HeapName;

    // for sorting images by size
    bool operator<(const GFxAmpImageInfo& rhs) const
    {
        const GImageInfoBase* lhsInfo = ImageResource->GetImageInfo();
        const GImageInfoBase* rhsInfo = rhs.ImageResource->GetImageInfo();
        const GImageInfoBase* lhsBaseInfo = lhsInfo;
        const GImageInfoBase* rhsBaseInfo = rhsInfo;

        if (rhsInfo->GetImageInfoType() == GImageInfoBase::IIT_SubImageInfo)
        {
            rhsBaseInfo = static_cast<const GSubImageInfo*>(rhsInfo)->GetBaseImage();
        }
        if (lhsInfo->GetImageInfoType() == GImageInfoBase::IIT_SubImageInfo)
        {
            lhsBaseInfo = static_cast<const GSubImageInfo*>(lhsInfo)->GetBaseImage();
        }

        if (lhsBaseInfo->GetImageId() == rhsBaseInfo->GetImageId())
        {
            if (lhsInfo->GetRect().Area() == rhsInfo->GetRect().Area())
            {
                return lhsInfo->GetImageId() < rhsInfo->GetImageId();
            }
            return lhsInfo->GetRect().Area() > rhsInfo->GetRect().Area();
        }

        if (lhsBaseInfo->GetBytes() == rhsBaseInfo->GetBytes())
        {
            return lhsBaseInfo->GetImageId() < rhsBaseInfo->GetImageId();
        }
        return lhsBaseInfo->GetBytes() > rhsBaseInfo->GetBytes();
    }
};


////////////////////////////////////////////////

// Returns true if there is a valid socket connection to the AMP client
bool GFxAmpServer::IsValidConnection() const
{
    return (ThreadMgr && ThreadMgr->IsValidConnection());
}

// State accessor
// Checks for paused or disabled state
bool GFxAmpServer::IsState(AmpServerStateType state) const
{
    GLock::Locker locker(&CurrentStateLock);
    return ((CurrentState.StateFlags & state) != 0);
}

// State accessor
// Sets the specified state (paused, disabled, etc)
void GFxAmpServer::SetState(AmpServerStateType state, bool stateValue)
{
    if (stateValue != IsState(state))
    {
        ToggleAmpState(state);
    }
}

// State accessor
// Checks for level of function profiling
SInt32 GFxAmpServer::GetProfileLevel() const
{
    GLock::Locker locker(&CurrentStateLock);
    return CurrentState.ProfileLevel;
}

// Toggles the specified state, allowing multiple states to be set or unset at once
void GFxAmpServer::ToggleAmpState(UInt32 toggleState)
{
    GLock::Locker locker(&CurrentStateLock);
    CurrentState.StateFlags ^= toggleState;

    if (toggleState != 0)
    {
        if ((Amp_Disabled & toggleState) != 0)
        {
            if (IsState(Amp_Disabled))
            {
                CloseConnection();
            }
            else
            {
                OpenConnection();
            }            
        }

        if ((Amp_RenderOverdraw & toggleState) != 0)
        {
            if (IsState(Amp_RenderOverdraw))
            {
                for (UPInt i = 0; i < AmpRenderers.GetSize(); ++i)
                {
                    AmpRenderers[i]->ProfileCounters(
                        ((GAmpRenderer::Profile_Fill | 16) << GAmpRenderer::Channel_Green) |
                        ((GAmpRenderer::Profile_Mask | 32) << GAmpRenderer::Channel_Red));
                }
            }
            else
            {
                for (UPInt i = 0; i < AmpRenderers.GetSize(); ++i)
                {
                    AmpRenderers[i]->DisableProfile();
                }
            }            
        }

        SendCurrentState();
    }
}

// Sets the function profiling level. The higher the level, the more detail you get
void GFxAmpServer::SetProfileLevel(SInt32 profileLevel)
{
    GLock::Locker locker(&CurrentStateLock);
    CurrentState.ProfileLevel = profileLevel;
    SetState(Amp_NoFunctionAggregation, profileLevel >= Amp_Profile_Level_High);
    SendCurrentState();
}

UInt32 GFxAmpServer::GetCurrentState() const
{
    GLock::Locker locker(&CurrentStateLock);
    return CurrentState.StateFlags;
}

void GFxAmpServer::SetConnectedApp(const char* playerTitle)
{
    GLock::Locker locker(&CurrentStateLock);
    if (CurrentState.ConnectedApp != playerTitle)
    {
        CurrentState.ConnectedApp = playerTitle;
        SendCurrentState();
    }
}

void GFxAmpServer::SetAaMode(const char* aaMode)
{
    GLock::Locker locker(&CurrentStateLock);
    if (CurrentState.AaMode != aaMode)
    {
        CurrentState.AaMode = aaMode;
        SendCurrentState();
    }
}

void GFxAmpServer::SetStrokeType(const char* strokeType)
{
    GLock::Locker locker(&CurrentStateLock);
    if (CurrentState.StrokeType != strokeType)
    {
        CurrentState.StrokeType = strokeType;
        SendCurrentState();
    }
}

void GFxAmpServer::SetCurrentLocale(const char* locale)
{
    GLock::Locker locker(&CurrentStateLock);
    if (CurrentState.CurrentLocale != locale)
    {
        CurrentState.CurrentLocale = locale;
        SendCurrentState();
    }
}

void GFxAmpServer::SetCurveTolerance(float tolerance)
{
    GLock::Locker locker(&CurrentStateLock);
    if (G_Abs(tolerance - CurrentState.CurveTolerance) > 0.001f)
    {
        CurrentState.CurveTolerance = tolerance;
        SendCurrentState();
    }
}

// Updates the state and sends it to AMP
// Useful because not all state components are exposed through individual accessors
// Also, allows changing many components at once, sending only one update message, 
// for example during initialization
void GFxAmpServer::UpdateState(const GFxAmpCurrentState* state)
{
    GLock::Locker locker(&CurrentStateLock);
    if (CurrentState != *state)
    {
        CurrentState = *state;
        SendCurrentState();
    }
}


// Can change port so that AMP can distinguish between instances of GFx running on the same address
void GFxAmpServer::SetListeningPort(UInt32 port)
{
    if (Port != port)
    {
        Port = port;
        if (IsSocketCreated())
        {
            CloseConnection();
            OpenConnection();
        }
    }
}

// Set the port where UDP broadcast messages are sent and received
// Has no effect after server had already started
void GFxAmpServer::SetBroadcastPort(UInt32 port)
{
    BroadcastPort = port;
}

// Disable broadcasting
void GFxAmpServer::SetNoBroadcast()
{
    GASSERT(!IsSocketCreated());  //  no effect - call before connection is opened
    SetBroadcastPort(0);
}

// Returns true if currently connected to AMP and sending stats
bool GFxAmpServer::IsProfiling() const
{ 
    if (!IsEnabled())
    {
        return false;
    }

    if (IsPaused())
    {
        return false;
    }

    if (!ThreadMgr || !ThreadMgr->IsValidSocket())
    {
        return false;
    }

    if (!IsValidConnection())
    {
        return false;
    }

    return true;
}

bool GFxAmpServer::IsInstructionProfiling() const
{
    return (GetProfileLevel() >= Amp_Profile_Level_High);
}

// Wait until connected, or until the specified time interval has passed
// Used so that an application can start being profiled from the first frame
// since establishing connection can take a little while to establish
void GFxAmpServer::WaitForAmpConnection(UInt maxDelayMilliseconds)
{
    ConnectedEvent.Wait(maxDelayMilliseconds);
}

// Sets the AMP capabilities supported by the current GFx application
// These capabilities, such as wireframe mode, are implemented outside of GFx
// and are application-specific
void GFxAmpServer::SetAppControlCaps(const GFxAmpMessageAppControl* caps)
{
    AppControlCaps = *GHEAP_AUTO_NEW(this) GFxAmpMessageAppControl(caps->GetValue());
    AppControlCaps->SetLoadMovieFile(caps->GetLoadMovieFile());
}

// The server can wait for a connection before proceeding
// This is useful when you want to profile the startup
void GFxAmpServer::SetConnectionWaitTime(UInt waitTimeMilliseconds)
{
    ConnectionWaitDelay = waitTimeMilliseconds;
}

// If AMP is the only system using sockets, it will initialize the library 
// and release it when it's done.
// SetInitSocketLib(false) means that AMP is going to be using a previously-initialized socket library
void GFxAmpServer::SetInitSocketLib(bool initSocketLib)
{
    InitSocketLib = initSocketLib;
}

// Set the AMP socket implementation factory
// Must be called before a connection has been opened
void GFxAmpServer::SetSocketImplFactory(GFxSocketImplFactory* socketImplFactory)
{
    SocketImplFactory = socketImplFactory;
}

// Sets the AMP heap limit in bytes
// When this limit is exceeded, GFx is paused until any pending messages have been sent to AMP,
// thus reducing the heap memory
void GFxAmpServer::SetHeapLimit(UPInt memLimit)
{
    GMemory::GetHeapByAddress(this)->SetLimit(memLimit);
}


// Called once per frame by the app
// It is called from GRenderer::EndFrame
// Updates all statistics
void GFxAmpServer::AdvanceFrame()
{
    // Don't bother updating if we have no client connected
    if (IsProfiling())
    {
        SendFrameStats();
    }
    else
    {
        // Timing stats need to be cleared every frame when not recording
        // Otherwise times will accumulate

        ClearMovieData();
        ClearRendererData();
    }

    GLock::Locker locker(&ToggleStateLock);
    if (ToggleState != 0)
    {
        ToggleAmpState(ToggleState);
        ToggleState = 0;
    }

    if (PendingProfileLevel != Amp_Profile_Level_Null)
    {
        SetProfileLevel(PendingProfileLevel);
        PendingProfileLevel = Amp_Profile_Level_Null;
    }
}

// Optional callback to handle app control messages
void GFxAmpServer::SetAppControlCallback(GFxAmpAppControlInterface* callback)
{
    AppControlCallback = callback;
}

// Called from the app to give a chance to handle new socket messages
// Returns true if a message was found and handled
bool GFxAmpServer::HandleNextMessage()
{
    if (ThreadMgr)
    {
        GPtr<GFxAmpMessage> msg = ThreadMgr->GetNextReceivedMessage();
        if (msg)
        {
            return msg->AcceptHandler(this);
        }
    }

    return false;
}

// Sends a log message to AMP client
// called directly from the log object
void GFxAmpServer::SendLog(const char* message, int msgLength, GFxLogConstants::LogMessageType msgType)
{
    if (ThreadMgr && ThreadMgr->IsValidConnection())
    {
        ThreadMgr->SendLog(GString(message, msgLength), msgType);
    }
}

// Sends the current state to the AMP client so the UI can be properly updated
void GFxAmpServer::SendCurrentState()
{
    if (ThreadMgr)
    {
        GLock::Locker locker(&CurrentStateLock);
        ThreadMgr->SetBroadcastInfo(CurrentState.ConnectedApp, CurrentState.ConnectedFile);
        GFxAmpMessageCurrentState* state = GHEAP_AUTO_NEW(this) GFxAmpMessageCurrentState(&CurrentState);
        ThreadMgr->SendAmpMessage(state);
    }
}

// Sends the supported AMP capabilities (for example, wireframe mode) to the AMP client
void GFxAmpServer::SendAppControlCaps()
{
    ThreadMgr->SendAmpMessage(GHEAP_AUTO_NEW(this) GFxAmpMessageAppControl(AppControlCaps->GetValue()));
}

// Handles an app control request message by forwarding it to the callback object
bool GFxAmpServer::HandleAppControl(const GFxAmpMessageAppControl* msg)
{
    GLock::Locker locker(&ToggleStateLock);
    ToggleState = 0;
    if (msg->IsToggleAmpRecording())
    {
        ToggleState |= Amp_Paused;
    }
    if (msg->IsToggleInstructionProfile())
    {
        if (IsInstructionProfiling())
        {
            PendingProfileLevel = Amp_Profile_Level_Low;
        }
        else
        {
            PendingProfileLevel = Amp_Profile_Level_High;
        }
    }
    if (msg->IsToggleMemReport())
    {
        ToggleState |= Amp_MemoryReports;
    }

    if (msg->IsForceInstructionProfile() && !IsInstructionProfiling())
    {
        PendingProfileLevel = Amp_Profile_Level_High;
    }
    if (msg->GetProfileLevel() != Amp_Profile_Level_Null)
    {
        PendingProfileLevel = msg->GetProfileLevel();
    }
 
    if (AppControlCallback != NULL)
    {
        // Forward to custom handler
        AppControlCallback->HandleAmpRequest(msg);
    }

    return true;
}

// The server has no knowledge of the SWD format
// but the SWD may be located on the server computer and not the client
// Therefore the server first tries to load the SWD locally 
// If it succeeds, it sends the data, without parsing it, to the client
// If no SWD is found, then it sends the file name for the requested SWD handle
bool GFxAmpServer::HandleSwdRequest(const GFxAmpMessageSwdRequest* msg)
{
    // Get the first file opener found
    GPtr<GFxFileOpenerBase> fileOpener;
    LoaderLock.Lock();
    for (UPInt i = 0; i < Loaders.GetSize(); ++i)
    {
        fileOpener = Loaders[i]->GetFileOpener();
        if (fileOpener)
        {
            break;
        }
    }
    LoaderLock.Unlock();

    if (fileOpener)
    {
        GLock::Locker locker(&SwfLock);

        UInt32 handle = msg->GetValue();
        SwdMap::Iterator it = HandleToSwdIdMap.Find(handle);
        if (it != HandleToSwdIdMap.End())
        {
            GString filename = GetSwdFilename(handle);
            if (!filename.IsEmpty() && msg->IsRequestContents())
            {
                // Convert .swf into .swd
                GString swdFilename = filename;
                UPInt iLength = swdFilename.GetLength();
                if (iLength > 4)
                {
                    GString extension = swdFilename.Substring(iLength - 4, iLength);
                    if (extension == ".swf" || extension == ".gfx")
                    {
                        swdFilename = swdFilename.Substring(0, iLength - 4);
                    }
                }
                swdFilename += ".swd";

                // Try to read the SWD
                bool sent = false;

                GPtr<GFile> swdFile = *fileOpener->OpenFile(swdFilename);
                if (swdFile && swdFile->GetLength() > 0)
                {
                    GArray<UByte> fileData(swdFile->GetLength());
                    if (swdFile->Read(&fileData[0], (SInt)fileData.GetSize()) == swdFile->GetLength())
                    {
                        // File read. Send the data to the client
                        ThreadMgr->SendAmpMessage(GHEAP_AUTO_NEW(this) GFxAmpMessageSwdFile(handle, 
                            &fileData[0], (UInt32)fileData.GetSize(), swdFilename));
                        sent = true;
                    }
                    else
                    {
                        GASSERT(false); // corrupt SWD
                    }
                    swdFile->Close();
                }

                if (!sent)
                {
                    // Send just the SWD file name so the client can try to load it
                    ThreadMgr->SendAmpMessage(GHEAP_AUTO_NEW(this) GFxAmpMessageSwdFile(handle, NULL, 0, swdFilename));
                }
            }
        }
    }

    return true;
}


bool GFxAmpServer::HandleSourceRequest(const GFxAmpMessageSourceRequest* msg)
{
    // Get the first file opener found
    GPtr<GFxFileOpenerBase> fileOpener;
    LoaderLock.Lock();
    for (UPInt i = 0; i < Loaders.GetSize(); ++i)
    {
        fileOpener = Loaders[i]->GetFileOpener();
        if (fileOpener)
        {
            break;
        }
    }
    LoaderLock.Unlock();

    if (fileOpener)
    {
        GLock::Locker locker(&SourceFileLock);

        UInt64 handle = msg->GetFileHandle();
        SourceFileMap::Iterator it = HandleToSourceFileMap.Find(handle);
        if (it != HandleToSourceFileMap.End())
        {
            GString filename = GetSourceFilename(handle);
            if (!filename.IsEmpty() && msg->IsRequestContents())
            {
                // Try to read the file
                bool sent = false;

                GPtr<GFile> sourceFile = *fileOpener->OpenFile(filename);
                if (sourceFile && sourceFile->GetLength() > 0)
                {
                    GArray<UByte> fileData(sourceFile->GetLength());
                    if (sourceFile->Read(&fileData[0], (SInt)fileData.GetSize()) == sourceFile->GetLength())
                    {
                        // File read. Send the data to the client
                        ThreadMgr->SendAmpMessage(GHEAP_AUTO_NEW(this) GFxAmpMessageSourceFile(handle, 
                            &fileData[0], (UInt32)fileData.GetSize(), filename));
                        sent = true;
                    }
                    else
                    {
                        GASSERT(false); // corrupt file
                    }
                    sourceFile->Close();
                }

                if (!sent)
                {
                    // Send just the file name so the client can try to load it
                    ThreadMgr->SendAmpMessage(GHEAP_AUTO_NEW(this) GFxAmpMessageSourceFile(handle, NULL, 0, filename));
                }
            }
        }
    }

    return true;
}

bool GFxAmpServer::HandleImageRequest(const GFxAmpMessageImageRequest* msg)
{
    HeapResourceMap imageMap;
    CollectImageData(&imageMap, NULL);

    HeapResourceMap::Iterator it;
    for (it = imageMap.Begin(); it != imageMap.End(); ++it)
    {
        for (ResourceSet::Iterator jt = it->Second.Begin(); jt != it->Second.End(); ++jt)
        {
            GFxImageResource* imgRes = jt->GetPtr();
            if (imgRes->GetImageInfo()->GetImageId() == msg->GetValue())
            {
                GPtr<GImage> srcImage = imgRes->GetImageInfo()->GetImage();
                if (srcImage)
                {    
#ifdef GFC_USE_LIBPNG
                    GFxAmpMessageImageData* dataMsg;
                    dataMsg = GHEAP_AUTO_NEW(this) GFxAmpMessageImageData(msg->GetValue());
                    GPtr<GFxAmpStream> imageFile = *GHEAP_AUTO_NEW(this) GFxAmpStream();
                    if (srcImage->WritePng(imageFile))
                    {
                        dataMsg->SetImageData(imageFile);
                        ThreadMgr->SendAmpMessage(dataMsg);
                    }
                    else
                    {
                        dataMsg->Release();
                    }
#endif
                    return true;
                }
            }
        }
    }

    return true;
}

bool GFxAmpServer::HandleFontRequest(const GFxAmpMessageFontRequest* msg)
{
    ThreadMgr->SendAmpMessage(GHEAP_AUTO_NEW(this) GFxAmpMessageFontData(msg->GetValue()));

    return true;
}

// Adds a movie to the list of movies to be profiled
// Called from GFxMovieRoot constructor
void GFxAmpServer::AddMovie(GFxMovieRoot* movie)
{
    // Don't profile movies on the debug heaps (such as the AMP HUD)
    if (((movie->GetHeap()->GetFlags() & GMemoryHeap::Heap_UserDebug) == 0))
    {
        GLock::Locker locker(&MovieLock);

        // Open the socket connection, if needed
        if (!IsSocketCreated())
        {
            OpenConnection();
        }
        else if (Movies.GetSize() == 0)
        {
            CloseConnectionOnUnloadMovies = false;
        }

        Movies.PushBack(movie);

        // Add the stats immediately
        MovieStats.PushBack(*GHEAP_AUTO_NEW(this) ViewStats(movie));
    }
}

// Removes a movie from the list of movies to be profiled
// Called from GFxMovieRoot destructor
void GFxAmpServer::RemoveMovie(GFxMovieRoot* movie)
{
    if (((movie->GetHeap()->GetFlags() & GMemoryHeap::Heap_UserDebug) == 0))
    {
        GLock::Locker locker(&MovieLock);
        for (UPInt i = 0; i < Movies.GetSize(); ++i)
        {
            if (Movies[i] == movie)
            {
                Movies.RemoveAt(i);
                // Don't remove the stats yet
                // wait until they have reported
                break;
            }
        }

        // If there are no more movies left to profile, close the socket connection
        if (Movies.GetSize() == 0 && CloseConnectionOnUnloadMovies)
        {
            CloseConnection();        
        }
    }
}

// When a movie is unloaded, the stats are kept alive by AMP
// until they can be reported to the client
// RefreshMovieStats re-syncs the movie stats with the loaded movies
// after the statistics have been reported
void GFxAmpServer::RefreshMovieStats()
{
    GLock::Locker locker(&MovieLock);
    MovieStats.Clear();
    for (UPInt i = 0; i < Movies.GetSize(); ++i)
    {
        MovieStats.PushBack(*GHEAP_AUTO_NEW(this) ViewStats(Movies[i]));
    }
}


// Adds a loader to the list of active loaders
// Called from the GFxLoader constructor
void GFxAmpServer::AddLoader(GFxLoader* loader)
{
    loader->GetFontCacheManager()->SetEventHandler(FontCacheHandler);
    GLock::Locker locker(&LoaderLock);
    Loaders.PushBack(loader);
}

// Removes a loader from the list of active loaders 
// Called from the GFxLoader destructor
void GFxAmpServer::RemoveLoader(GFxLoader* loader)
{
    GLock::Locker locker(&LoaderLock);
    for (UPInt i = 0; i < Loaders.GetSize(); ++i)
    {
        if (Loaders[i] == loader)
        {
            Loaders.RemoveAt(i);
            break;
        }
    }
}

void GFxAmpServer::AddSound(UPInt soundMem)
{
    SoundMemory += soundMem;
}

void GFxAmpServer::RemoveSound(UPInt soundMem)
{
    SoundMemory -= soundMem;
}

// AMP renderer is used to render overdraw
void GFxAmpServer::AddAmpRenderer(GAmpRenderer* renderer)
{
    GLock::Locker locker(&LoaderLock);
    if (renderer != NULL)
    {
        AmpRenderers.PushBack(renderer);
    }
}


// AMP server generates unique handles for each SWF
// The 16-byte SWD debug ID is not used because it is not appropriate as a hash key
// AMP keeps a map of SWD handles to actual SWD ID and filename
UInt32 GFxAmpServer::GetNextSwdHandle() const
{
    static UInt32 lastSwdHandle = NativeCodeSwdHandle;
    GLock::Locker locker(&SwfLock);
    ++lastSwdHandle;
    return lastSwdHandle;
}

// Called whenever a Debug ID tag is found when reading a SWF
// Creates a new entry in the SWD map
UInt32 GFxAmpServer::AddSwf(const char* swdId, const char* filename)
{
    SwdInfo* pInfo = GHEAP_AUTO_NEW(this) SwdInfo();
    pInfo->SwdId = swdId;
    pInfo->Filename = filename;
    UInt32 swdHandle = GetNextSwdHandle();

    GLock::Locker locker(&SwfLock);
    HandleToSwdIdMap.Set(swdHandle, *pInfo);
    return swdHandle;
}

// Retrieves the SWD debug ID for a given handle
GString GFxAmpServer::GetSwdId(UInt32 handle) const
{
    GLock::Locker locker(&SwfLock);
    SwdMap::ConstIterator it = HandleToSwdIdMap.Find(handle);
    if (it == HandleToSwdIdMap.End())
    {
        return "";
    }
    return it->Second->SwdId;
}

// Retrieves the SWF filename for a given handle
GString GFxAmpServer::GetSwdFilename(UInt32 handle) const
{
    GLock::Locker locker(&SwfLock);
    SwdMap::ConstIterator it = HandleToSwdIdMap.Find(handle);
    if (it == HandleToSwdIdMap.End())
    {
        return "";
    }
    return it->Second->Filename;
}

// Retrieves the source filename for a given handle
GString GFxAmpServer::GetSourceFilename(UInt64 handle) const
{
    GLock::Locker locker(&SourceFileLock);
    SourceFileMap::ConstIterator it = HandleToSourceFileMap.Find(handle);
    if (it == HandleToSourceFileMap.End())
    {
        return "";
    }
    return it->Second->Filename;
}

void GFxAmpServer::AddSourceFile(UInt64 fileHandle, const char* fileName)
{
    GLock::Locker locker(&SourceFileLock);
    GPtr<SourceFileInfo> fileInfo = *GHEAP_AUTO_NEW(this) SourceFileInfo();
    fileInfo->Filename = fileName;
    HandleToSourceFileMap.Set(fileHandle, fileInfo);
}



// Constructor
// Called only from GFxAmpServer::Init
GFxAmpServer::GFxAmpServer() :
    ToggleState(0),
    PendingProfileLevel(Amp_Profile_Level_Null),
    Port(7534),
    BroadcastPort(GFX_AMP_BROADCAST_PORT),
    ThreadMgr(NULL),
    CloseConnectionOnUnloadMovies(true),
    NumFontCacheTextureUpdates(0),
    ConnectionWaitDelay(0),
    InitSocketLib(true),
    SoundMemory(0),
    SocketImplFactory(NULL),
    AppControlCallback(NULL)
{
    SendThreadCallback = *GHEAP_AUTO_NEW(this) GFxAmpSendThreadCallback();
    StatusChangedCallback = *GHEAP_AUTO_NEW(this) GFxAmpStatusChangedCallback(&ConnectedEvent);
    AppControlCaps = *GHEAP_AUTO_NEW(this) GFxAmpMessageAppControl();
    FontCacheHandler = *GHEAP_AUTO_NEW(this) GFxAmpGlyphCacheEventHandler();
}

// Destructor
GFxAmpServer::~GFxAmpServer()
{
    ThreadMgr = NULL;
}

// Start the socket threads
// Returns true if successful
bool GFxAmpServer::OpenConnection()
{
    if (!IsState(Amp_Disabled))
    {
        ConnectionLock.Lock();
        if (!ThreadMgr)
        {
            ThreadMgr = *GHEAP_AUTO_NEW(this) GFxAmpThreadMgr(this, SendThreadCallback, 
                StatusChangedCallback, &SendingEvent, SocketImplFactory);
        }
        ConnectionLock.Unlock();

        if (!ThreadMgr || !ThreadMgr->InitAmp(NULL, Port, BroadcastPort))
        {
            return false;
        }

        WaitForAmpConnection(ConnectionWaitDelay);
    }

    return true;
}

// Stops the socket threads
void GFxAmpServer::CloseConnection()
{
    GLock::Locker locker(&ConnectionLock);
    ThreadMgr = NULL;
}

// Returns true if we are listening for a connection
bool GFxAmpServer::IsSocketCreated() const
{
    if (!ThreadMgr)
    {
        return false;
    }
    return true;
}

// Collects all the memory data per frame
// Populates the ProfileFrame object, which is then sent to the client
void GFxAmpServer::CollectMemoryData(GFxAmpProfileFrame* frameProfile)
{
    UInt32 MemoryComponents = 0;

    HeapResourceMap imageMap;
    FontResourceMap fontMap;
    CollectImageData(&imageMap, &fontMap);

    // Images
    UInt32 imageTotal = 0;
    UInt32 imageGFx = 0;
    for (HeapResourceMap::Iterator it = imageMap.Begin(); it != imageMap.End(); ++it)
    {
        for (ResourceSet::Iterator jt = it->Second.Begin(); jt != it->Second.End(); ++jt)
        {
            GFxImageResource* imgRes = *jt;

            UInt32 imgSize = static_cast<UInt32>(imgRes->GetBytes());
            if (!imgRes->IsExternal())
            {
                imageGFx += imgSize;
            }
            imageTotal += imgSize;

            GString imageName;
            GString fileURL(imgRes->GetFileURL());
            if (!fileURL.IsEmpty())
            {
                imageName += fileURL + " ";
            }
            if (imgRes->GetResourceUse() == GFxResource::Use_Gradient)
            {
                imageName += "Gradient";
            }
            else if (imgRes->GetResourceUse() == GFxResource::Use_Bitmap)
            {
                imageName += "Bitmap";
            }

            GString imgFormat;
            switch (imgRes->GetImageFormat())
            {
            case GImageBase::Image_ARGB_8888:
                imgFormat = "R8G8B8A8";
                break;
            case GImageBase::Image_RGB_888:
                imgFormat = "R8G8B8";
                break;
            case GImageBase::Image_A_8:
                imgFormat = "A8";
                break;
            case GImageBase::Image_DXT1:
                imgFormat = "DXT1";
                break;
            case GImageBase::Image_DXT3:
                imgFormat = "DXT3";
                break;
            case GImageBase::Image_DXT5:
                imgFormat = "DXT5";
                break;
            default:
                break;
            }

            UInt32 imageWidth = imgRes->GetWidth();
            UInt32 imageHeight = imgRes->GetHeight();

            GPtr<ImageInfo> imgInfo = *GHEAP_AUTO_NEW(this) ImageInfo();
            imgInfo->Bytes = imgSize;
            imgInfo->Id = imgRes->GetImageInfo()->GetImageId();
            imgInfo->Bytes = static_cast<UInt32>(imgRes->GetBytes());
            imgInfo->HeapName = it->First;
            imgInfo->External = imgRes->IsExternal();

            if (imgRes->GetImageInfo()->GetImageInfoType() == GImageInfoBase::IIT_SubImageInfo)
            {
                GSubImageInfo* subImgInfo = static_cast<GSubImageInfo*>(imgRes->GetImageInfo());
                imgInfo->AtlasTop = subImgInfo->GetRect().Top;
                imgInfo->AtlasBottom = subImgInfo->GetRect().Bottom;
                imgInfo->AtlasLeft = subImgInfo->GetRect().Left;
                imgInfo->AtlasRight = subImgInfo->GetRect().Right;
                imgInfo->AtlasId = subImgInfo->GetBaseImage()->GetImageId();
                imageWidth = imgInfo->AtlasRight - imgInfo->AtlasLeft;
                imageHeight = imgInfo->AtlasBottom - imgInfo->AtlasTop;
            }

            G_Format(imgInfo->Name, "{1}x{2} {0} ({3})", 
                imageName.ToCStr(), 
                imageWidth, 
                imageHeight,
                imgFormat, imgInfo->Id, imgInfo->AtlasId);

            frameProfile->ImageList.PushBack(imgInfo);
        }
    }

    frameProfile->Images->StartExpanded = true;
    if (IsMemReports())
    {
        GMemory::pGlobalHeap->MemReport(frameProfile->MemoryByStatId, GMemoryHeap::MemReportHeapDetailed);
        frameProfile->TotalMemory = frameProfile->MemoryByStatId->GetValue("Total Footprint") - frameProfile->MemoryByStatId->GetValue("Debug Data");
        frameProfile->VideoMemory = frameProfile->MemoryByStatId->SumValues("Video");
        frameProfile->MovieDataMemory = frameProfile->MemoryByStatId->GetValue("Movie Data Heaps");
        // Assume image memory counted in the MovieData heap
        frameProfile->MovieDataMemory -= imageGFx;
        frameProfile->MovieViewMemory = frameProfile->MemoryByStatId->GetValue("Movie View Heaps");
        frameProfile->FontCacheMemory = frameProfile->MemoryByStatId->SumValues("_Font_Cache");
        frameProfile->MeshCacheMemory = frameProfile->MemoryByStatId->SumValues("_Mesh_Cache");
#ifdef GHEAP_DEBUG_INFO
        frameProfile->SoundMemory = frameProfile->MemoryByStatId->SumValues("Sound");
#else
        frameProfile->SoundMemory = SoundMemory;
        if (frameProfile->SoundMemory != 0)
        {
            UInt32 lastMemId = frameProfile->MemoryByStatId->GetMaxId();
            GFxAmpMemItem* globalHeapItem = frameProfile->MemoryByStatId->SearchForName("Global Heap");
            if (globalHeapItem != NULL)
            {
                globalHeapItem->AddChild(++lastMemId, "Sound", frameProfile->SoundMemory);
            }
        }
#endif
    }
    else
    {
        GMemoryHeap::RootStats stats;
        GMemory::pGlobalHeap->GetRootStats(&stats);
        GString toggleMessage("Toggle the <img src='instr' width='13' height='13' align='baseline' vspace='-4'> button to enable detailed memory reports");
        GPtr<GFxAmpMemItem> memSummary = *GHEAP_AUTO_NEW(this) GFxAmpMemItem(0);
        GMemory::pGlobalHeap->MemReport(memSummary, GMemoryHeap::MemReportBrief);
        frameProfile->TotalMemory = (UInt32)(stats.SysMemFootprint - stats.DebugInfoFootprint - stats.UserDebugFootprint);
        UInt32 lastMemId = memSummary->GetMaxId();
        memSummary->ID = ++lastMemId;
        frameProfile->MemoryByStatId->AddChild(++lastMemId, toggleMessage);
        frameProfile->MemoryByStatId->Children.PushBack(memSummary);

        for (UPInt i = 0; i < memSummary->Children.GetSize(); ++i)
        {
            GFxAmpMemItem* childItem = memSummary->Children[i];
            if (childItem->Name == "Video")
            {
                frameProfile->VideoMemory = childItem->Value;
            }
            else if (childItem->Name == "Sound")
            {
                frameProfile->SoundMemory = childItem->Value;
            }
            else if (childItem->Name == "Movie Data")
            {
                frameProfile->MovieDataMemory = childItem->Value;
            }
            else if (childItem->Name == "Movie View")
            {
                frameProfile->MovieViewMemory = childItem->Value;
            }
            else if (childItem->Name == "Mesh Cache")
            {
                frameProfile->MeshCacheMemory = childItem->Value;
            }
            else if (childItem->Name == "Font Cache")
            {
                frameProfile->FontCacheMemory = childItem->Value;
            }
        }

#ifndef GHEAP_DEBUG_INFO
        frameProfile->SoundMemory = SoundMemory;
#endif
        memSummary->AddChild(++lastMemId, "Sound", frameProfile->SoundMemory);

        // Don't send too many images
        if (frameProfile->ImageList.GetSize() > 10)
        {
            frameProfile->Images->Name = "Total Image Memory";
            frameProfile->Images->SetValue(imageTotal);
            GString imagesString;
            G_Format(imagesString, "{0} Images", frameProfile->ImageList.GetSize());
            frameProfile->Images->AddChild(++lastMemId, imagesString);
            frameProfile->Images->AddChild(++lastMemId, "Toggle the <img src='instr' width='13' height='13' align='baseline' vspace='-4'> button to display the full image list");
            frameProfile->ImageList.Clear();
        }   
    }

    frameProfile->ImageMemory = imageGFx;

    MemoryComponents += frameProfile->ImageMemory;
    MemoryComponents += frameProfile->VideoMemory;
    MemoryComponents += frameProfile->SoundMemory;
    MemoryComponents += frameProfile->MovieDataMemory;
    MemoryComponents += frameProfile->MovieViewMemory;
    MemoryComponents += frameProfile->MeshCacheMemory;
    MemoryComponents += frameProfile->FontCacheMemory;

    GFxAmpMemItem* unused = frameProfile->MemoryByStatId->SearchForName("Unused Memory");
    if (unused != NULL)
    {
        MemoryComponents += unused->Value;
    }

    frameProfile->OtherMemory = G_Max(frameProfile->TotalMemory, MemoryComponents) - MemoryComponents;

    UInt32 lastItemId = frameProfile->MemoryByStatId->GetMaxId();

    // Fonts
    FontResourceMap::Iterator itFont;
    for (itFont = fontMap.Begin(); itFont != fontMap.End(); ++itFont)
    {
        GPtr<GFxAmpMemItem> fontMovie = *GHEAP_AUTO_NEW(this) GFxAmpMemItem(++lastItemId);
        fontMovie->Name = itFont->First;
        fontMovie->StartExpanded = true;
        for (UPInt j = 0; j < itFont->Second.GetSize(); ++j)
        {
            fontMovie->AddChild(++lastItemId, itFont->Second[j]);
        }
        frameProfile->Fonts->Children.PushBack(fontMovie);
    }

    frameProfile->Fonts->StartExpanded = true;

    /*
    GFxMemSegVisitor segVisitor;
    GMemoryHeap::VisitRootSegments(&segVisitor);
    GMemory::pGlobalHeap->VisitHeapSegments(&segVisitor);
    GFxAmpHeapVisitor heapVisitor(&segVisitor);
    GMemory::pGlobalHeap->VisitChildHeaps(&heapVisitor);
    G_QuickSort(segVisitor.MemorySegments);

    for (UPInt i = 0; i < segVisitor.MemorySegments.GetSize(); ++i)
    {
        const GMemoryHeap* heap = segVisitor.MemorySegments[i].Heap;
        GFxAmpMemSegment seg;
        seg.Addr = segVisitor.MemorySegments[i].Addr;
        seg.Type = segVisitor.MemorySegments[i].Type;
        seg.Size = segVisitor.MemorySegments[i].Size;
        seg.HeapId = reinterpret_cast<UInt64>(heap);
        frameProfile->MemFragReport->MemorySegments.PushBack(seg);

        if (frameProfile->MemFragReport->HeapInformation.Get(seg.HeapId) == NULL)
        {
            GFxAmpHeapInfo* heapInfo = GHEAP_AUTO_NEW(this) GFxAmpHeapInfo(heap);
            frameProfile->MemFragReport->HeapInformation.Set(seg.HeapId, *heapInfo);
        }
    }
    */
}

// Collects the image information from the ResourceLib and MovieDefs
// Output is a map of heap name to an array of images
void GFxAmpServer::CollectImageData(HeapResourceMap* resourceMap, FontResourceMap* fontMap)
{
    {
        GHashSet<UInt32> resLibImages;

        if (resourceMap != NULL)
        {
            GLock::Locker lock(&LoaderLock);

            // Get all the unique resourceLibs from the registered loaders
            GArray<GFxResourceLib*> resourceLibs;
            for (UPInt i = 0; i < Loaders.GetSize(); ++i)
            {
                GFxResourceLib* resourceLib = Loaders[i]->GetResourceLib();
                if (resourceLib != NULL)
                {
                    bool resLibFound = false;
                    for (UPInt j  = 0; j < resourceLibs.GetSize(); ++j)
                    {
                        if (resourceLibs[j] == resourceLib)
                        {
                            resLibFound = true;
                            break;
                        }
                    }
                    if (!resLibFound)
                    {
                        resourceLibs.PushBack(resourceLib);
                    }
                }
            }

            // Get the images for each resourceLib
            for (UPInt i = 0; i < resourceLibs.GetSize(); ++i)
            {
                GMemoryHeap* heap = resourceLibs[i]->GetImageHeap();
                GString heapName("[Heap] ");
                heapName += heap->GetName();
                HeapResourceMap::Iterator it = resourceMap->FindCaseInsensitive(heapName);
                if (it == resourceMap->End())
                {
                    resourceMap->SetCaseInsensitive(heapName, ResourceSet());
                    it = resourceMap->FindCaseInsensitive(heapName);
                }

                GArray< GPtr<GFxResource> > resources;
                resourceLibs[i]->GetResourceArray(&resources);
                for (UPInt i = 0; i < resources.GetSize(); ++i)
                {
                    if (resources[i]->GetResourceType() == GFxResource::RT_Image)
                    {
                        GFxImageResource* imgRes = static_cast<GFxImageResource*>(resources[i].GetPtr());
                        resLibImages.Set(imgRes->GetImageInfo()->GetImageId());
                        it->Second.Set(imgRes);
                    }
                }
            }
        }


        GLock::Locker lock(&MovieLock);

        // Get all the unique movieDefs from the registered Movies
        GArray<GFxMovieDef*> movieDefs;
        for (UPInt i = 0; i < Movies.GetSize(); ++i)
        {
            GFxMovieDef* movieDef = Movies[i]->GetMovieDef();
            if (movieDef != NULL)
            {
                bool movieDefFound = false;
                for (UPInt j  = 0; j < movieDefs.GetSize(); ++j)
                {
                    if (movieDefs[j] == movieDef)
                    {
                        movieDefFound = true;
                        break;
                    }
                }
                if (!movieDefFound)
                {
                    movieDefs.PushBack(movieDef);
                }
            }
        }

        // Get the images for each movieDef
        for (UPInt i = 0; i < movieDefs.GetSize(); ++i)
        {
            if (resourceMap != NULL)
            {
                GFxAmpImageVisitor imgVisitor;
                movieDefs[i]->VisitResources(&imgVisitor, GFxMovieDef::ResVisit_AllImages);       

                GMemoryHeap* heap = movieDefs[i]->GetImageHeap();

                GMemoryHeap::HeapInfo kInfo;
                heap->GetHeapInfo(&kInfo);
                GString heapName("[Heap] ");
                heapName += heap->GetName();
                if (kInfo.pParent != NULL && kInfo.pParent != GMemory::GetGlobalHeap())
                {
                    heapName += kInfo.pParent->GetName();
                }

                HeapResourceMap::Iterator it = resourceMap->FindCaseInsensitive(heapName);
                if (it == resourceMap->End())
                {
                    resourceMap->SetCaseInsensitive(heapName, ResourceSet());
                    it = resourceMap->FindCaseInsensitive(heapName);
                }
                for (UPInt j = 0; j < imgVisitor.Images.GetSize(); ++j)
                {
                    if (resLibImages.Find(imgVisitor.Images[j]->GetImageInfo()->GetImageId()) == resLibImages.End())
                    {
                        it->Second.Set(imgVisitor.Images[j]);
                    }
                }
            }

            if (fontMap != NULL)
            {
                GFxAmpFontVisitor fontVisitor;
                movieDefs[i]->VisitResources(&fontVisitor, GFxMovieDef::ResVisit_Fonts);

                GString movieName(movieDefs[i]->GetFileURL());
                FontResourceMap::Iterator itFont = fontMap->FindCaseInsensitive(movieName);
                if (itFont == fontMap->End())
                {
                    fontMap->SetCaseInsensitive(movieName, fontVisitor.Fonts);
                }
                else
                {
                    itFont->Second.Append(fontVisitor.Fonts);
                }
            }
        }
    }
}


// Collects all the movie data per frame
// Populates the ProfileFrame object, which is then sent to the client
void GFxAmpServer::CollectMovieData(GFxAmpProfileFrame* frameProfile)
{

    GArray< GPtr<ViewStats> > movieCopy;
    {
        // Make a copy before collecting stats to avoid deadlocks (in case ViewStats is locked)
        GLock::Locker lock(&MovieLock);
        for (UPInt i = 0; i < MovieStats.GetSize(); ++i)
        {
            movieCopy.PushBack(MovieStats[i]);
        }
    }

    frameProfile->MovieStats.Resize(movieCopy.GetSize());
    for (UPInt i = 0; i < movieCopy.GetSize(); ++i)
    {
        movieCopy[i]->CollectStats(frameProfile, i);
    }

    ClearMovieData();
}

// Clears all the movie data
// Clearing needs to happen even when stats are not being reported
// to avoid them accumulating and therefore being wrong for the first frame
void GFxAmpServer::ClearMovieData()
{
    RefreshMovieStats();  // free stats that belong to unloaded movies


    GArray< GPtr<ViewStats> > movieCopy;
    {
        // Make a copy before collecting stats to avoid deadlocks (in case ViewStats is locked)
        GLock::Locker lock(&MovieLock);
        for (UPInt i = 0; i < MovieStats.GetSize(); ++i)
        {
            movieCopy.PushBack(MovieStats[i]);
        }
    }

    for (UPInt i = 0; i < movieCopy.GetSize(); ++i)
    {
        movieCopy[i]->ClearStats();
    }
}

// Populates the ProfileFrame with renderer stats
void GFxAmpServer::CollectRendererData(GFxAmpProfileFrame* frameProfile)
{
    frameProfile->NumFontCacheTextureUpdates = NumFontCacheTextureUpdates;
    frameProfile->FontThrashing = FontCacheHandler->GetThrashing();
    frameProfile->FontFail = FontCacheHandler->GetFailures();

    GArray<GRenderer*> renderers;
    GArray<GAmpRenderer*> ampRenderers;
    GArray<GFxMeshCacheManager*> meshCaches;
    GArray<GFxFontCacheManager*> fontCaches;

    GLock::Locker locker(&LoaderLock);

    // Update active list of Renderers, Mesh Caches, and Font Caches
    // This is inefficient O(n^2) but number of loaders is going to be small
    for (UPInt i = 0; i < Loaders.GetSize(); ++i)
    {
        if (!IsState(Amp_RenderOverdraw))
        {
            // Unique renderers
            GRenderer* renderer = Loaders[i]->GetRenderer();
            if (renderer != NULL)
            {
                bool rendererFound = false;
                for (UPInt j  = 0; j < renderers.GetSize(); ++j)
                {
                    if (renderers[j] == renderer)
                    {
                        rendererFound = true;
                        break;
                    }
                }
                if (!rendererFound)
                {
                    renderers.PushBack(renderer);
                }
            }
        }

        // Unique mesh caches
        GFxMeshCacheManager* meshCache = Loaders[i]->GetMeshCacheManager();
        if (meshCache != NULL)
        {
            bool meshCacheFound = false;
            for (UPInt j  = 0; j < meshCaches.GetSize(); ++j)
            {
                if (meshCaches[j] == meshCache)
                {
                    meshCacheFound = true;
                    break;
                }
            }
            if (!meshCacheFound)
            {
                meshCaches.PushBack(meshCache);
            }
        }

        // Unique font caches
        GFxFontCacheManager* fontCache = Loaders[i]->GetFontCacheManager();
        if (fontCache != NULL)
        {
            bool fontCacheFound = false;
            for (UPInt j  = 0; j < fontCaches.GetSize(); ++j)
            {
                if (fontCaches[j] == fontCache)
                {
                    fontCacheFound = true;
                    break;
                }
            }
            if (!fontCacheFound)
            {
                fontCaches.PushBack(fontCache);
            }
        }
    }

    if (IsState(Amp_RenderOverdraw))
    {
        for (UPInt i = 0; i < AmpRenderers.GetSize(); ++i)
        {
            bool rendererFound = false;
            for (UPInt j  = 0; j < ampRenderers.GetSize(); ++j)
            {
                if (ampRenderers[j] == AmpRenderers[i])
                {
                    rendererFound = true;
                    break;
                }
            }
            if (!rendererFound)
            {
                ampRenderers.PushBack(AmpRenderers[i]);
            }
        }
    }

    for (UPInt i = 0; i < renderers.GetSize(); ++i)
    {
        GRenderer::Stats stats;
        renderers[i]->GetRenderStats(&stats, false);
        CollectRendererStats(frameProfile, stats);
    }

    for (UPInt i = 0; i < ampRenderers.GetSize(); ++i)
    {
        GRenderer::Stats stats;
        ampRenderers[i]->GetAmpRenderStats(&stats, false);
        CollectRendererStats(frameProfile, stats);
    }

    for (UPInt i = 0; i < meshCaches.GetSize(); ++i)
    {
        frameProfile->StrokeCount += meshCaches[i]->GetNumStrokes();
        frameProfile->MeshThrashing += meshCaches[i]->GetMeshThrashing();
    }

    UInt32 totalArea = 0;
    UInt32 usedArea = 0;
    for (UPInt i = 0; i < fontCaches.GetSize(); ++i)
    {
        frameProfile->RasterizedGlyphCount += fontCaches[i]->GetNumRasterizedGlyphs();
        frameProfile->FontTextureCount += fontCaches[i]->GetNumTextures();
        totalArea += fontCaches[i]->ComputeTotalArea();
        usedArea += fontCaches[i]->ComputeUsedArea();
    }
    if (totalArea != 0)
    {
        frameProfile->FontFill = 100 * usedArea / totalArea;
    }

    ClearRendererData();
}

// Renderer stats obtained either from a GRenderer or a GAmpRenderer
void GFxAmpServer::CollectRendererStats(GFxAmpProfileFrame* frameProfile, const GRenderer::Stats& stats)
{
    frameProfile->TriangleCount += stats.Triangles;
    frameProfile->MaskCount += stats.Masks;
    frameProfile->FilterCount += stats.Filters;
    frameProfile->LineCount += stats.Lines;
    frameProfile->DrawPrimitiveCount += stats.Primitives;
}

// Waits until AMP thread catches up
// This prevents the buffered data from accumulating without limit
void GFxAmpServer::WaitForAmpThread(GFxAmpViewStats* viewStats, UInt32 waitMillisecs)
{
    ScopeFunctionTimer timer(viewStats, NativeCodeSwdHandle, Func_GFxAmpServer_WaitForAmpThread, Amp_Profile_Level_Low);
    SendingEvent.Wait(waitMillisecs);
}


// Clears all the renderer statistics
// Clearing needs to happen even when stats are not being reported
// to avoid them accumulating and therefore being wrong for the first frame
void GFxAmpServer::ClearRendererData()
{
    NumFontCacheTextureUpdates = 0;

    GLock::Locker locker(&LoaderLock);

    for (UPInt i = 0; i < Loaders.GetSize(); ++i)
    {
        GRenderer* renderer = Loaders[i]->GetRenderer();
        if (renderer != NULL)
        {
            renderer->GetRenderStats(NULL, true);
        }
    }

    for (UPInt i = 0; i < AmpRenderers.GetSize(); ++i)
    {
        AmpRenderers[i]->GetAmpRenderStats(NULL, true);
    }

    FontCacheHandler->ResetStats();
}

// Sends all the statistics for the current frame to the AMP client
void GFxAmpServer::SendFrameStats()
{
    GLock::Locker locker(&FrameDataLock);

    GFxAmpProfileFrame* frameProfile = GHEAP_AUTO_NEW(this) GFxAmpProfileFrame();

    static UInt64 lastTick = 0;
    frameProfile->TimeStamp = GTimer::GetProfileTicks();
    if (lastTick != 0)
    {
        UInt64 iDeltaTicks = frameProfile->TimeStamp - lastTick;
        if (iDeltaTicks != 0)
        {
            frameProfile->FramesPerSecond = static_cast<UInt32>(1000000 / iDeltaTicks);
        }
    }
    lastTick = frameProfile->TimeStamp;

    // Send the active SWD handles so the client knows to load the appropriate SWD
    SwfLock.Lock();
    for (SwdMap::Iterator it = HandleToSwdIdMap.Begin(); it != HandleToSwdIdMap.End(); ++it)
    {
        frameProfile->SwdHandles.PushBack(it->First);
    }
    SwfLock.Unlock();

    SourceFileLock.Lock();
    for (SourceFileMap::Iterator it = HandleToSourceFileMap.Begin(); it != HandleToSourceFileMap.End(); ++it)
    {
        frameProfile->FileHandles.PushBack(it->First);
    }
    SourceFileLock.Unlock();

//     Uncomment for marker testing
//     if (frameProfile->TimeStamp % 10 == 0 && MovieStats.GetSize() > 0)
//     {
//         MovieStats[0]->AdvanceStats->AddMarker("test");
//     }

    CollectMovieData(frameProfile);
    CollectRendererData(frameProfile);
    CollectMemoryData(frameProfile);
    
    ThreadMgr->SendAmpMessage(GHEAP_AUTO_NEW(this) GFxAmpMessageProfileFrame(*frameProfile));

    // Wait for AMP thread to catch up
    if (MovieStats.GetSize() > 0)
    {
        WaitForAmpThread(MovieStats[0]->DisplayStats, 1000);
    }
}

//////////////////////////////////////////////////////////////////////////////

GFxAmpServer::ViewStats::ViewStats(GFxMovieRoot* movie) : 
    DisplayStats(movie->DisplayStats), 
    AdvanceStats(movie->AdvanceStats)
{

}

void GFxAmpServer::ViewStats::CollectStats(GFxAmpProfileFrame* frameProfile, UPInt index)
{
    GFxMovieStats* stats = GHEAP_AUTO_NEW(this) GFxMovieStats(); 

    AdvanceStats->CollectTimingStats(frameProfile);
    AdvanceStats->CollectAmpInstructionStats(stats);
    AdvanceStats->CollectAmpFunctionStats(stats);
    AdvanceStats->CollectAmpSourceLineStats(stats);
    AdvanceStats->CollectMarkers(stats);
    DisplayStats->CollectTimingStats(frameProfile);
    DisplayStats->CollectAmpFunctionStats(stats);

    stats->ViewHandle = AdvanceStats->GetViewHandle();
    stats->MinFrame = stats->MaxFrame = AdvanceStats->GetCurrentFrame();
    stats->ViewName = AdvanceStats->GetName();
    stats->Version = AdvanceStats->GetVersion();
    stats->Width = AdvanceStats->GetWidth();
    stats->Height = AdvanceStats->GetHeight();
    stats->FrameRate = AdvanceStats->GetFrameRate();
    stats->FrameCount = AdvanceStats->GetFrameCount();

    frameProfile->MovieStats[index] = *stats;        

    // Update gradient fill by counting how mant times GFxFillStyle::GetGradientFillTexture was called
    for (UPInt j = 0; j < stats->FunctionStats->FunctionTimings.GetSize(); ++j)
    {
        GFxAmpMovieFunctionStats::FuncStats& funcStats = stats->FunctionStats->FunctionTimings[j];
        UInt64 key = NativeCodeSwdHandle;
        key <<= 32;
        key += Func_GFxFillStyle_GetGradientFillTexture;
        if (funcStats.FunctionId == key)
        {
            frameProfile->GradientFillCount += funcStats.TimesCalled;
        }
    }
}

void GFxAmpServer::ViewStats::ClearStats()
{
    AdvanceStats->ClearAmpFunctionStats();
    AdvanceStats->ClearAmpInstructionStats();
    AdvanceStats->ClearAmpSourceLineStats();
    AdvanceStats->ClearMarkers();
    DisplayStats->ClearAmpFunctionStats();
    DisplayStats->ClearAmpInstructionStats();
}


#endif  // GFX_AMP_SERVER


