/**********************************************************************

Filename    :   GFxLoaderImpl.cpp
Content     :   GFxPlayer loader implementation
Created     :   June 30, 2005
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFile.h"
#include "GImage.h"

#include "GFxLoaderImpl.h"

#include "GFxLoadProcess.h"
#include "GFxImageResource.h"

#include "GFxLog.h"
#include "GFxTaskManager.h"

#include <string.h> // for memset
#include <float.h>

#include "GFxAmpServer.h"

int StateAccessCount = 0;

/*********** KNOWN ISSUE WITH LOADING ********************************************************
There is a known issue with our loading process: reloading a swf file does not detect 
changes made in imported files. The only workaround around this problem is to release 
all references to the main movie and the reload it again completely with all it 
dependencies instead of reloading only modified files. This is not a very efficient way. 
The solution for this problem could be to hold a list of {MovieDefImpl,timestamp} objects
for each MovieDataDef in the Resource lib. When this list is retrieved from Resource Lib
we would take its last item and then go over all imports in a MovieDataDef find theirs
MovieDefImpl in that item’s ImportedMovieSource vector and check if any of imported movies
is changed. If no changes are found we return this last MovieDefImpl to the user. If we 
find a change we will need to create a new MovieDefImpl for a given MovieDataDef add it
to the list that we got from the Resource Lib and return this new object to the user. 
(Keep in mind that the search should be done recursively because there could be a chain 
of imports-exports).  
The solution is not completely thought through because there could be a loading inconsistence. 
Let say Movie1 imports MovieA and MovieB and MovieB also imports MovieA. Then Movie1 loads 
its imports first it loads MovieA and then MovieB, but while MovieB is being load MovieA could
be changed. When MovieB starts loading MovieA it will detect the change and load a new version 
of MovieA (MovieA.1). After loading is complete Movie1 will use MovieA but MovieB will use MovieA.1. 
We decided not to invest more time in fixing this issue for now, because it can take a lot time
but will benefit a couple of our user.
**************************************************************************************************/


// ***** GFxExporterInfoImpl Loading

// Assigns data
void    GFxExporterInfoImpl::SetData(UInt16 version, GFxLoader::FileFormatType format,
                                     const char* pname, const char* pprefix, UInt flags, 
                                     const GArray<UInt32>* codeOffsets)
{
    SI.Version  = version;
    SI.Format   = format;
    Prefix      = (pprefix) ? pprefix : "";
    SWFName     = (pname) ? pname : "";
    SI.pSWFName = SWFName.ToCStr(); // Update SI pointers.
    SI.pPrefix  = Prefix.ToCStr();
    SI.ExportFlags = flags;
    if (codeOffsets != NULL)
    {
        CodeOffsets = *codeOffsets;
    }
    else
    {
        CodeOffsets.Clear();
    }
}

void    GFxExporterInfoImpl::ReadExporterInfoTag(GFxStream *pin, GFxTagType tagType)
{
    GUNUSED(tagType);
    GASSERT(tagType == GFxTag_ExporterInfo);

    // Utilizes the tag 1000 (unused in normal SWF): the format is as follows:
    // Header           RECORDHEADER    1000
    // Version          UI16            Version (1.10 will be encoded as 0x10A)
    // Flags            UI32            Version 1.10 (0x10A) and above - flags
    //      Bit0 - Contains glyphs' textures info (tags 1002)
    //      Bit1 - Glyphs are stripped from DefineFont tags
    //      Bit2 - Indicates gradients' images were exported
    // BitmapsFormat    UI16            1 - TGA
    //                                  2 - DDS
    // PrefixLen        UI8
    // Prefix           UI8[PrefixLen]
    // SwfNameLen       UI8
    // SwfName          UI8[SwfNameLen]

    UInt32  flags = 0;
    UInt16  version  = pin->ReadU16(); // read version
    if (version >= 0x10A)
        flags = pin->ReadU32();
    UInt16  bitmapFormat = pin->ReadU16();

    GString fxstr, swfstr;
    pin->ReadStringWithLength(&fxstr);
    pin->ReadStringWithLength(&swfstr);

    GArray<UInt32> codeOffsets;
    if (version >= 0x36F)
    {
        UInt16 numCodeOffsets = pin->ReadU16();
        for (UInt16 i = 0; i < numCodeOffsets; ++i)
        {
            codeOffsets.PushBack(pin->ReadU32());
        }
    }

    pin->LogParse("  ExportInfo: tagType = %d, tool ver = %d.%d, imgfmt = %d, prefix = '%s', swfname = '%s', flags = 0x%X\n",
        tagType,
        (version>>8), (version&0xFF),
        bitmapFormat,
        fxstr.ToCStr(),
        swfstr.ToCStr(),
        (int)flags);

    SetData(version, (GFxLoader::FileFormatType)bitmapFormat, swfstr.ToCStr(), fxstr.ToCStr(), flags, &codeOffsets);
}


// Processes and reads in a SWF file header and opens the GFxStream
// If 0 is returned, there was an error and error message is already displayed
bool    GFxSWFProcessInfo::Initialize(GFile *pin, GFxLog *plog, GFxZlibSupportBase* zlib,
                                      GFxParseControl* pparseControl, bool parseMsg)
{
    UInt32  header;
    bool    compressed;

    FileStartPos        = pin->Tell();
    header              = pin->ReadUInt32();
    Header.FileLength   = pin->ReadUInt32();
    FileEndPos          = FileStartPos + Header.FileLength;
    NextActionBlock     = 0;
    Header.Version      = (header >> 24) & 255;
    Header.SWFFlags     = 0;
    compressed          = (header & 255) == 'C';

    // Verify header
    if ( ((header & 0x0FFFFFF) != 0x00535746) && // FWS
        ((header & 0x0FFFFFF) != 0x00535743) && // CWS
        ((header & 0x0FFFFFF) != 0x00584647) && // GFX
        ((header & 0x0FFFFFF) != 0x00584643) )  // CFX
    {
        // ERROR
        if (plog)
            plog->LogError("Error: GFxLoader read failed - file does not start with a SWF header\n");
        return 0;
    }
    if (((header >> 16) & 0xFF) == 'X')
        Header.SWFFlags |= GFxMovieInfo::SWF_Stripped;
    if (compressed)
        Header.SWFFlags |= GFxMovieInfo::SWF_Compressed;

    // Parse messages will not be generated if they are disabled by GFxParseControl.
    if (!plog || !pparseControl || !pparseControl->IsVerboseParse())
        parseMsg = false;

    if (parseMsg)
        plog->LogMessageByType(GFxLog::Log_Parse,
        "SWF File version = %d, File length = %d\n",
        Header.Version, Header.FileLength);

    // AddRef to file
    GPtr<GFile> pfileIn = pin;  
    if (compressed)
    {
#ifndef GFC_USE_ZLIB
        GUNUSED(zlib);
        if (plog)
            plog->LogError("Error: GFxLoader - unable to read compressed SWF data; GFC_USE_ZLIB not defined\n");
        return 0;
#else
        if (!zlib)
        {
            if (plog)
                plog->LogError("Error: GFxLoader - unable to read compressed SWF data; GFxZlibState is not set.\n");
            return 0;
        }
        if (parseMsg)
            plog->LogMessageByType(GFxLog::Log_Parse, "SWF file is compressed.\n");

        // Uncompress the input as we read it.
        pfileIn = *zlib->CreateZlibFile(pfileIn);

        // Subtract the size of the 8-byte header, since
        // it's not included pin the compressed
        // GFxStream length.
        FileEndPos = Header.FileLength - 8;
#endif
    }

    // Initialize stream, this AddRefs to file
    Stream.Initialize(pfileIn, plog, pparseControl);

    // Read final data
    Stream.ReadRect(& Header.FrameRect);
    Header.FPS         = Stream.ReadU16() / 256.0f;
    Header.FrameCount  = Stream.ReadU16();

    // Read the exporter tag, which must be the first tag in the GFX file.
    // We require this tag to be included in the very beginning of file because:
    //  1. Some of its content is reported by GFxMovieInfo.
    //  2. Reporting it from cached MovieDataDef would require a
    //     'wait-for-load' in cases when most of the loading is done
    //     on another thread.

    if (Header.SWFFlags & GFxMovieInfo::SWF_Stripped)
    {
        if ((UInt32) Stream.Tell() < FileEndPos)
        {
            if (Stream.OpenTag() == GFxTag_ExporterInfo)
            {
                Header.ExporterInfo.ReadExporterInfoTag(&Stream, GFxTag_ExporterInfo);
                if ((Header.ExporterInfo.GetExporterInfo()->Version & (~0xFF)) < 0x300 ||
                    (Header.ExporterInfo.GetExporterInfo()->Version & (~0xFF)) > 0x400)
                {
                    // Only gfx from exporter 3.0 are supported
                    if (plog)
                        plog->LogError(
                        "Error: GFxLoader read failed - incompatible GFX file, version 3-4.x expected\n");
                    return 0;
                }
            }
            else
            {
                if (plog)
                    plog->LogError(
                    "Error: GFxLoader read failed - no ExporterInfo tag in GFX file header\n");
                return 0;
            }
            Stream.CloseTag();
        }
        // Do not seek back; we advance the tags by one, as appropriate.
    }

    return 1;
}

// ***** GFxLoaderTask - implementation

GFxLoaderTask::GFxLoaderTask(GFxLoadStates* pls, TaskId id)
: GFxTask(id), pLoadStates(pls)
{
    //printf("GFxLoaderTask::GFxLoaderTask : %x, thread : %d\n", this, GetCurrentThreadId());
    pLoadStates->pLoaderImpl->RegisterLoadProcess(this);
}
GFxLoaderTask::~GFxLoaderTask()
{
    //printf("GFxLoaderTask::~GFxLoaderTask : %x, thread : %d\n", this, GetCurrentThreadId());
    pLoadStates->pLoaderImpl->UnRegisterLoadProcess(this);
}

// ***** GFxLoaderImpl - loader implementation


GFxLoaderImpl::GFxLoaderImpl(GFxResourceLib* plib, bool debugHeap)
: DebugHeap(debugHeap)
{    
    if (plib)
        pWeakResourceLib = plib->GetWeakLib();

    if (pStateBag = *new GFxStateBagImpl(0))
    {
        pStateBag->SetLog(GPtr<GFxLog>(*new GFxLog));
        pStateBag->SetImageCreator(GPtr<GFxImageCreator>(*new GFxImageCreator));

        // By default there should be no glyph packer
        pStateBag->SetFontPackParams(0);
        //pStateBag->SetFontPackParams(GPtr<GFxFontPackParams>(*new GFxFontPackParams));

        // It's mandatory to have the cache manager for text rendering to work,
        // even if the dynamic cache isn't used. 
        pStateBag->SetFontCacheManager(
            GPtr<GFxFontCacheManager>(*new GFxFontCacheManager(true, DebugHeap)));

        pStateBag->SetTextClipboard(GPtr<GFxTextClipboard>(*new GFxTextClipboard));
        pStateBag->SetTextKeyMap(GPtr<GFxTextKeyMap>(*(new GFxTextKeyMap)->InitWindowsKeyMap()));
    }
}

GFxLoaderImpl::GFxLoaderImpl(GFxLoaderImpl* psource)
: pWeakResourceLib(psource->pWeakResourceLib), DebugHeap(psource->DebugHeap)
{   
    if (pStateBag = *new GFxStateBagImpl(0))
    {
        if (psource->pStateBag)
            pStateBag->CopyStatesFrom(psource->pStateBag);
        else
        {
            pStateBag->SetLog(GPtr<GFxLog>(*new GFxLog));

            // By default there should be no glyph packer
            pStateBag->SetFontPackParams(0);
            //pStateBag->SetFontPackParams(GPtr<GFxFontPackParams>(*new GFxFontPackParams));

            // It's mandatory to have the cache manager for text rendering to work,
            // even if the dynamic cache isn't used. 
            pStateBag->SetFontCacheManager(
                GPtr<GFxFontCacheManager>(*new GFxFontCacheManager(true, DebugHeap)));
        }      
    }
}

GFxLoaderImpl::~GFxLoaderImpl()
{
    CancelLoading();
}



// Obtains information about SWF file and checks for its availability.
// Return 1 if the info was obtained successfully (or was null, but SWF file existed),
// or 0 if it did not exist. Pass LoadCheckLibrary if the library should be checked before loading the file.
// Specifying LoadFromLibraryOnly can be used to check for presence of the file in the library.
bool    GFxLoaderImpl::GetMovieInfo(const char *pfilename, GFxMovieInfo *pinfo,
                                    bool getTagCount, UInt loadConstants)
{    
    if (!pinfo)
    {
        GFC_DEBUG_WARNING(1, "GFxLoader::GetMovieInfo failed, pinfo argument is null");
        return 0;
    }
    pinfo->Clear();

    // LOCK
    // Capture loading states/variables used during loading.
    GPtr<GFxLoadStates>  pls = *new GFxLoadStates(this);
    // UNLOCK

    if (!pls->GetLib())
    {
        GFC_DEBUG_WARNING(1, "GFxLoader::GetMovieInfo failed, ResourceLibrary does not exist");
        return 0;
    }


    // Translate the filename.
    GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_Regular, pfilename);
    GString                   fileName;
    pls->BuildURL(&fileName, loc);

    // Use the MovieDataDef version already in the library if necessary.
    GPtr<GFxResource>    pmovieDataResource;

    //    if (loadConstants & GFxLoader::LoadCheckLibrary)
    {
        // Image creator is only used as a key if it is bound to, based on flags.    
        GFxImageCreator* pkeyImageCreator = pls->GetLoadTimeImageCreator(loadConstants);

        GFxFileOpener *pfileOpener = pls->GetFileOpener();
        SInt64         modifyTime  = pfileOpener ?
            pfileOpener->GetFileModifyTime(fileName.ToCStr()) : 0;

        GFxResourceKey fileDataKey = 
            GFxMovieDataDef::CreateMovieFileKey(fileName.ToCStr(), modifyTime,
            pfileOpener, pkeyImageCreator,
            pls->GetPreprocessParams());
        pmovieDataResource = *pls->GetLib()->GetResource(fileDataKey);
    }

    if (pmovieDataResource)
    {
        // Fetch the data from GFxMovieDataDef.
        GFxMovieDataDef* pmd = (GFxMovieDataDef*)pmovieDataResource.GetPtr();
        pmd->GetMovieInfo(pinfo);

        if (getTagCount)
        {
            // TBD: This may have to block for MovieDef to load.
            pinfo->TagCount = pmd->GetTagCount();
        }
    }
    else
    {
        // Open the file; this will automatically do the logging on error.
        GPtr<GFile> pin = *pls->OpenFile(fileName.ToCStr());
        if (!pin)
            return 0;

        // Open and real file header, failing if it doesn't match.
        GFxSWFProcessInfo pi(GMemory::GetGlobalHeap());
        if (!pi.Initialize(pin, pls->GetLog(), pls->GetZlibSupport(), pls->pParseControl))
            return 0;

        // Store header data.
        pi.Header.GetMovieInfo(pinfo);

        if (getTagCount)
        {
            // Count tags.
            // pinfo->TagCount starts out at 0 after Clear
            while ((UInt32) pi.Stream.Tell() < pi.FileEndPos)
            {
                pi.Stream.OpenTag();
                pi.Stream.CloseTag();
                pinfo->TagCount++;
            }
        }

        // Done; file will be closed by destructor.
    }   

    return 1;
}



GFxMovieDef* GFxLoaderImpl::CreateMovie(const char* pfilename, UInt loadConstants, UPInt memoryArena)
{
    // LOCK

    // Capture loading states/variables used during loading.
    GPtr<GFxLoadStates>  pls = *new GFxLoadStates(this);
    // If CreateMovie is started on a thread (not main thread) we need to set
    // threaded loading flag regardless of whether task manage is set or not
    if (loadConstants & GFxLoader::LoadOnThread)
        pls->ThreadedLoading = true;

    // UNLOCK

    if (!pls->GetLib())
    {
        GFC_DEBUG_WARNING(1, "GFxLoader::CreateMovie failed, ResourceLibrary does not exist");
        return 0;
    }

    GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_Regular, pfilename);
    return CreateMovie_LoadState(pls, loc, loadConstants, NULL, memoryArena);
}


void GFxLoaderImpl::RegisterLoadProcess(GFxLoaderTask* ptask)
{

    //GASSERT(pStateBag->GetTaskManager());
    GLock::Locker guard(&LoadProcessesLock);
    LoadProcesses.PushBack(new GFxLoadProcessNode(ptask));
}

void GFxLoaderImpl::UnRegisterLoadProcess(GFxLoaderTask* ptask)
{
    //GASSERT(pStateBag->GetTaskManager());
    GLock::Locker guard(&LoadProcessesLock);
    GFxLoadProcessNode* pnode = LoadProcesses.GetFirst();
    while (!LoadProcesses.IsNull(pnode))
    {
        if (pnode->pTask == ptask) 
        {
            LoadProcesses.Remove(pnode);
            //pStateBag->GetTaskManager()->AbandonTask(ptask);
            delete pnode;
            break;
        }
        pnode = pnode->pNext;
    }
}
void GFxLoaderImpl::CancelLoading()
{
    //printf("GFxLoaderTask::~CancelLoading ---  : %x, thread : %d\n", this, GetCurrentThreadId());
    GPtr<GFxTaskManager> ptm = pStateBag->GetTaskManager();
    if (!ptm)
        return;
    GLock::Locker guard(&LoadProcessesLock);
    GFxLoadProcessNode* pnode = LoadProcesses.GetFirst();
    while (!LoadProcesses.IsNull(pnode))
    {
        LoadProcesses.Remove(pnode);
        ptm->AbandonTask(pnode->pTask);
        delete pnode;
        pnode = LoadProcesses.GetFirst();
    }
}

// *** Loading tasks.

// Loading of image into GFxMovieDataDef.
class GFxMovieImageLoadTask : public GFxLoaderTask
{
    // TODO: Replace this with equivalent of a weak pointer,
    // same as in GFxMovieDataLoadTask above.
    GPtr<GFxMovieDataDef>       pDef;
    GPtr<GFxMovieDefImpl>       pDefImpl;

    // Data required for image loading.    
    GPtr<GFile>                 pImageFile;
    GFxLoader::FileFormatType   ImageFormat;

    //GPtr<GFxLoadStates>         pLoadStates;
    //GPtr<GFxImageCreator>       pImageCreator;
    //GPtr<GFxRenderConfig>       pRenderConfig;

    // This stores the result of loading: Image Resource.
    GPtr<GFxImageResource>      pImageRes;

public:
    GFxMovieImageLoadTask(
        GFxMovieDataDef *pdef, GFxMovieDefImpl *pdefImpl,
        GFile *pin, GFxLoader::FileFormatType format, GFxLoadStates* pls)
        : GFxLoaderTask(pls, Id_MovieImageLoad),
        pDef(pdef), pDefImpl(pdefImpl), pImageFile(pin), ImageFormat(format)//, pLoadStates(pls)
    { }

    virtual void    Execute()
    {
        // Image data loading: could be separated into a different task later on.
        // For now, read the data.
        GPtr<GImage>           pimage;
        GPtr<GImageInfoBase>   pimageBase;

        // MA: Using resource lib's heap seems ok here, although the image is not
        // shared directly through GFxResourceLib. Instead, the image is shared indirectly
        // based in its contained GFxMovieDataDef. This will work; however, is not ideal
        // because it means that if you both call 'loadMovie' on an image from ActionScript
        // AND have it loaded based on 'gxexport' extraction, you will get two copies in
        // memory. TBD: Perhaps we could use a MovideDef heap here... or improve image
        // sharing so that it is done for the *image itself* in the resource lib.
        GMemoryHeap*           pimageHeap = pLoadStates->pWeakResourceLib->GetImageHeap();

        pimage = *GFxImageCreator::LoadBuiltinImage(pImageFile, ImageFormat,
                        GFxResource::Use_Bitmap, 
                        pLoadStates->GetLog(), pLoadStates->GetJpegSupport(),
                        pLoadStates->GetPNGSupport(),
                        pimageHeap);

        if (pimage)
        {
            // Use creator for image
            GFxImageCreateInfo ico(pimage, GFxResource::Use_None, pLoadStates->GetRenderConfig());
            ico.ThreadedLoading = pLoadStates->IsThreadedLoading();
            ico.pHeap           = pimageHeap;
            pimageBase = *pLoadStates->GetBindStates()->pImageCreator->CreateImage(ico);

            if (pimageBase)            
                pImageRes = 
                    *GHEAP_NEW(pimageHeap) GFxImageResource(pimageBase, GFxResource::Use_Bitmap);
        }

        if (pImageRes)
        {
            pDef->InitImageFileMovieDef(pImageFile->GetLength(), pImageRes);

            // Notify GFxMovieDefImpl that binding is finished,
            // so that any threaded waiters can be released.
            UInt fileBytes = pDef->GetFileBytes();
            pDefImpl->pBindData->UpdateBindingFrame(pDef->GetLoadingFrame(), fileBytes);
            pDefImpl->pBindData->SetBindState(
                GFxMovieDefImpl::BS_Finished |
                GFxMovieDefImpl::BSF_Frame1Loaded | GFxMovieDefImpl::BSF_LastFrameLoaded);
        }
        else
        {   // Error            
            pDefImpl->pBindData->SetBindState(GFxMovieDefImpl::BS_Error);
        }
    }

    virtual void    OnAbandon(bool)
    {        
        // TODO: Mark movie as canceled, so that it knows that
        // it will not proceed here.
    }

    bool    LoadingSucceeded() const   { return pImageRes.GetPtr() != 0; }
};  



static GFile* s_OpenAndDetectFile(const char* filename, UInt loadConstants, GFxLoadStates* pls, 
                                  GFxLoaderImpl::FileFormatType* format, GFxMovieDataDef::MovieDataType* mtype,
                                  GString* err_msg)
{
    GPtr<GFile> pin = *pls->OpenFile(filename, loadConstants);
    if (!pin) 
    {
        // TBD: Shouldn't we transfer this OpenFile's error message to our 
        // waiters, if any? That way both threads would report the same message.
        // For now, just create a string.
        *err_msg = GString("GFxLoader failed to open \"", filename, "\"\n");
        return 0;
    }


    // Detect file format so that we can determine whether we can
    // and/or allowed to support it. Images can be loaded directly
    // into GFxMovieDef files, but their loading logic is custom.        
    *format = GFxLoaderImpl::DetectFileFormat(pin);

    switch(*format)
    {      
    case GFxLoader::File_SWF:
        if (loadConstants & GFxLoader::LoadDisableSWF)
        {                
            *err_msg = GString("Error loading SWF file \"", filename, "\" - GFX file format expected\n");
            return 0;
        }
        // Fall through to Flash file loading
    case GFxLoader::File_GFX:
        *mtype = GFxMovieDataDef::MT_Flash;
        break;

        // Image file formats support.
    case GFxLoader::File_JPEG:
    case GFxLoader::File_DDS:
    case GFxLoader::File_TGA:
    case GFxLoader::File_PNG:
        // If image file format loading is enabled proceed to do so.
        if (loadConstants & GFxLoader::LoadImageFiles)
        {
            *mtype = GFxMovieDataDef::MT_Image;
            break;
        }

    case GFxLoader::File_Unopened:
        // Unopened should not occur due to the check above.
    case GFxLoader::File_Unknown:
    default:
        *err_msg = GString("Unknown file format at URL \"", filename, "\"\n");
        return 0;
    };
    pin->AddRef();
    return pin.GetPtr();
}

// Static: The actual creation function; called from CreateMovie.
GFxMovieDefImpl* GFxLoaderImpl::CreateMovie_LoadState(GFxLoadStates* pls,
                                                      const GFxURLBuilder::LocationInfo& loc,
                                                      UInt loadConstants, LoadStackItem* ploadStack,
                                                      UPInt memoryArena)
{
    // Translate the filename.
    GString fileName;
    pls->BuildURL(&fileName, loc);

    // *** Check Library and Initiate Loading

    GFxResourceLib::BindHandle      bh;    
    GPtr<GFxMovieDataDef>           pmd;
    GFxMovieDefImpl*                pm = 0;
    GFxLog*                         plog  = pls->pLog;

    bool                            movieNeedsLoading = 0;    
    GFxMovieDataDef::MovieDataType  mtype  = GFxMovieDataDef::MT_Empty;
    GPtr<GFxMovieBindProcess>       pbp;
    GPtr<GFxLoadProcess>            plp;
    GPtr<GFile>                     pin;
    FileFormatType                  format = GFxLoader::File_Unopened;

    GFxImagePackParamsBase* pimagePacker = pls->GetBindStates()->pImagePackParams;
    if (pimagePacker)
        loadConstants |= GFxLoader::LoadOrdered|GFxLoader::LoadWaitCompletion;

    // Ordered loading means that all binding-dependent files (imports and images) will be
    // loaded after the main file. Technically, this disagrees with progressive loading,
    // although we could devise a better scheme in the future.
    // If 'Ordered' is not specified, loading is interleaved, meaning that imports
    // and dependencies get resolved while parent file hasn't yet finished loading.
    // ThreadedBinding implies interleaved loading, since the binding thread can
    // issue a dependency load request at any time.    
    bool    interleavedLoading = (loadConstants & GFxLoader::LoadThreadedBinding) ||
        !(loadConstants & GFxLoader::LoadOrdered);

    // Since Ordered loading prevents threaded binding from starting on time, warn.
    GFC_DEBUG_WARNING((loadConstants & (GFxLoader::LoadOrdered|GFxLoader::LoadThreadedBinding)) ==
        (GFxLoader::LoadOrdered|GFxLoader::LoadThreadedBinding),
        "GFxLoader::CreateMovie - LoadOrdered flag conflicts with GFxLoader::LoadThreadedBinding");

    // We integrate optional ImageCreator for loading, with hash matching
    // dependent on GFxImageLoader::IsKeepingImageData and LoadKeepBindData flag.
    // Image creator is only used as a key if it is bound to, based on flags.    
    GFxImageCreator*                 pkeyImageCreator = pls->GetLoadTimeImageCreator(loadConstants);
    GFxFileOpener*                   pfileOpener = pls->GetFileOpener();
    UInt64                           modifyTime  = pfileOpener ?
                                            pfileOpener->GetFileModifyTime(fileName.ToCStr()) : 0;

    GFxResourceKey fileDataKey = 
        GFxMovieDataDef::CreateMovieFileKey(fileName.ToCStr(), modifyTime,
                                            pfileOpener, pkeyImageCreator, pls->GetPreprocessParams());

    GFxResourceLib::ResolveState rs;

    if ((rs = pls->GetLib()->BindResourceKey(&bh, fileDataKey)) == GFxResourceLib::RS_NeedsResolve)
    {        
        // Open the file; this will automatically do the logging on error.
        GString err_msg;
        pin = *s_OpenAndDetectFile(fileName.ToCStr(), loadConstants, pls, &format, &mtype, &err_msg);
        if (!pin) 
        {
            if (plog)
                plog->LogError("%s", err_msg.ToCStr());
            bh.CancelResolve(err_msg.ToCStr());
            return 0;
        }

        // Create GFxMovieDataDef of appropriate type (Image or Flash)
        pmd = *GFxMovieDataDef::Create(fileDataKey, mtype, fileName.ToCStr(), 0,
                                       (loadConstants & GFxLoader::LoadDebugHeap) ? true : false, 
                                       memoryArena);

        //printf("Thr %4d, %8x : CreateMovie - constructed GFxMovieDataDef for '%s'\n", 
        //       GetCurrentThreadId(), pmd.GetPtr(), fileName.ToCStr());

        if (pmd)
        {
            // Assign movieDef's file path to LoadStates.
            pls->SetRelativePathForDataDef(pmd);

            // Create a loading process for Flash files and verify header.
            // For images, this is done later on.
            if (mtype == GFxMovieDataDef::MT_Flash)
            {  
                plp = *GNEW GFxLoadProcess(pmd, pls, loadConstants);

                // Read in and verify header, initializing loading.
                // Note that this also reads the export tags,
                // so no extra pre-loading will be necessary in GFxMovieDef.
                if (!plp || !plp->BeginSWFLoading(pin))
                {
                    // Clear pmd, causing an error message and CancelResolve below.
                    plp = 0;
                    pmd = 0;
                }
            }
        }

        if (pmd)
        {    
            // For images we always create DefImpl before ResolveResource, so that
            // other threads don't try to bind us (no separate binding from images now).

            if  ((mtype != GFxMovieDataDef::MT_Flash) || interleavedLoading )
            {
                // If we are doing interleaved loading, create the bound movie entry immediately,
                // to ensure that we don't have another thread start binding before us.     
                pm = CreateMovieDefImpl(pls, pmd, loadConstants,
                    (mtype == GFxMovieDataDef::MT_Flash) ? &pbp.GetRawRef() : 0, true, ploadStack, memoryArena);
            }

            bh.ResolveResource(pmd.GetPtr());
        }
        else
        {
            GString s("Failed to load SWF file \"", fileName.ToCStr(), "\"\n");
            bh.CancelResolve(s.ToCStr());
            return 0;
        }

        movieNeedsLoading = 1;
    }
    else
    {
        // If Available and Waiting resources will be resolved here.        
        /*
        if (rs == GFxResourceLib::RS_Available)        
        printf("Thr %4d, ________ : CreateMovie - '%s' is in library\n", GetCurrentThreadId(), fileName.ToCStr());        
        else        
        printf("Thr %4d, ________ : CreateMovie - waiting on '%s'\n", GetCurrentThreadId(), fileName.ToCStr());        
        */
        GUNUSED(rs);

        if ((pmd = *(GFxMovieDataDef*)bh.WaitForResolve()).GetPtr() == 0)
        {
            // Error occurred during loading.
            if (plog)
                plog->LogError("Error: %s", bh.GetResolveError());
            return 0;
        }

        mtype = pmd->MovieType;

        // SetDataDef to load states so that GFxMovieDefImpl::Bind can proceed.
        pls->SetRelativePathForDataDef(pmd);
        // May need to wait for movieDefData to become available.
    }


    // *** Check the library for MovieDefImpl and Initiate Binding    

    // Do a check because for Ordered loading this might have been
    // done above to avoid data race.
    if (!movieNeedsLoading || !interleavedLoading)
    {
        if (!pm)
        {            
            // For images this can grab an existing MovieDefImpl, but it will never
            // create one since it's taken care of before DataDef ResolveResource.

            bool justCreated = false;
            pm = CreateMovieDefImpl(pls, pmd, loadConstants,
                (mtype == GFxMovieDataDef::MT_Flash) ? &pbp.GetRawRef() : 0, false, ploadStack, memoryArena, &justCreated);
            if (mtype == GFxMovieDataDef::MT_Image && justCreated)
            {
                // if we get here that means that this movie clip (MovieDef) is in process of unloading and its
                // MovieDefImpl has already been removed from the resource lib, but MovieDef is still in 
                // the resource lib and then a request from another thread comes to load this movie clip again.
                // In this case we need to reload the image file again. It only apples to image files because 
                // flash files will be handled by MovieBindProcess object which is created inside CreateMovieDefImpl call
                movieNeedsLoading = true;
            }
        }
    }
    if (!pm)
        return 0;


    // *** Do Loading

    if (movieNeedsLoading)
    {
        if (mtype == GFxMovieDataDef::MT_Flash)
        { 
            // Set 'ploadBind' if we are going to do interleaved binding
            // simultaneously with loading. LoadOrdered means that binding
            // will be done separately - in that case Read is don with no binding.
            GFxMovieBindProcess* ploadBind =
                (loadConstants & (GFxLoader::LoadOrdered|GFxLoader::LoadThreadedBinding)) ? 0 : pbp.GetPtr();
            if (ploadBind)
                plp->SetBindProcess(ploadBind);

            // bind process will allocate the temporary data if necessary
            if (pbp) // used for packer; incompatible with packer if binding is occurring on diff thread (AR)
                plp->SetTempBindData(pbp->GetTempBindData());

            // If we have task manager, queue up loading task for execution,
            // otherwise just run it immediately.
            if (loadConstants & GFxLoader::LoadWaitCompletion || !pls->SubmitBackgroundTask(plp))
                plp->Execute();

            if (ploadBind)
            {
                // If bind process was performed as part of the load task,
                // we no longer need it.
                pbp = 0;
            }

            plp = 0;
            pin = 0;
        }
        else
        {
            if (!pin)
            {
                // we are here because MovieDevImpl for this movieclip has already been remove from the
                // resource lib, but MovieDev is still in. We need to reload an image for this movieclip
                GString err_msg;
                pin = *s_OpenAndDetectFile(fileName.ToCStr(), loadConstants, pls, &format, &mtype, &err_msg);
                if (!pin) 
                {
                    if (plog)
                        plog->LogError("%s", err_msg.ToCStr());
                    pm->Release();
                    return 0;
                }
            }
            GPtr<GFxMovieImageLoadTask> ptask = 
                *GNEW GFxMovieImageLoadTask(pmd, pm, pin, format, pls);

            if ((loadConstants & (GFxLoader::LoadWaitCompletion|GFxLoader::LoadOrdered)) 
                || !pls->SubmitBackgroundTask(ptask) )
            {
                ptask->Execute();

                if (!ptask->LoadingSucceeded())
                {
                    if (pm) pm->Release();
                    return 0;
                }

                // NOTE: A similar check is done by the use of 'waitSuceeded'
                // flag below for threaded tasks.
            }
        }
    }


    // Run bind task on a MovieDefImpl and waits for completion, based on flags. 
    return BindMovieAndWait(pm, pbp, pls, loadConstants, ploadStack);
}



GFxMovieDefImpl* GFxLoaderImpl::BindMovieAndWait(GFxMovieDefImpl* pm, GFxMovieBindProcess* pbp,
                                                 GFxLoadStates* pls, UInt loadConstants, LoadStackItem* ploadStack)
{
    // It we still need binding, perform it.
    if (pbp)
    {   
        if (loadConstants & GFxLoader::LoadWaitCompletion || !pls->SubmitBackgroundTask(pbp))
            pbp->Execute();
    }

    // Note that if loading failed in the middle we may return a partially loaded object.
    // This is normal because (a) loading can technically take place in a different thread
    // so it is not yet known if it will finish successfully and (2) Flash can actually
    // play unfinished files, even if the error-ed in a middle.

    // The exception to above are wait flags, however.
    bool waitSuceeded = true;
    bool needWait = true;

    // Checking for recursion in the loading process.
    LoadStackItem* pstack = ploadStack;
    while(pstack)
    {
        if (pstack->pDefImpl == pm)
        {
            // Recursion is detected. 
            // Check if this is a self recursion 
            if(pstack->pNext)
            {
                // This is not a self recursion. We don't support this recursion type yet.
                // Stop loading and return error.
                waitSuceeded = false;
                if (pls->GetLog())
                {
                    GStringBuffer buffer;

                    while(ploadStack)
                    {
                        buffer += ploadStack->pDefImpl->GetFileURL();
                        buffer += '\n';
                        ploadStack = ploadStack->pNext;
                    }

                    buffer += pm->GetFileURL();
                    buffer += '\n';
                    pls->GetLog()->LogError("Error: Recursive import detected. Import stack:\n%s", buffer.ToCStr());
                }
            }
            // We must not wait on a waitcondition which will never be set.
            needWait = false;
            break;
        }
        pstack = pstack->pNext;
    }
    if (needWait && (loadConstants & GFxLoader::LoadWaitCompletion))
    {
        // TBD: Under threaded situation the semantic of LoadWaitCompletion might
        // actually be to do loading on 'this' thread without actually queuing
        // a task.

        // We might also want to have a flag that would control whether WaitCompletion
        // fails the load on partial load, or returns an object partially loaded
        // similar to the standard behavior above.
        waitSuceeded = pm->WaitForBindStateFlags(GFxMovieDefImpl::BSF_LastFrameLoaded);
    }
    else if (needWait && (loadConstants & GFxLoader::LoadWaitFrame1))
    {
        waitSuceeded = pm->WaitForBindStateFlags(GFxMovieDefImpl::BSF_Frame1Loaded);
    }

    // waitSuceeded would only be 'false' in case of error.
    if (!waitSuceeded)
    {
        pm->Release();
        pm = 0;
    }
    return pm;
}


// Looks up or registers GFxMovieDefImpl, separated so that both versions 
// of loading can share implementation. Fills is pbindProcess pointer if
// the later is provided (not necessary for image file MovieDefImpl objects).
GFxMovieDefImpl* GFxLoaderImpl::CreateMovieDefImpl(GFxLoadStates* pls,
                                                   GFxMovieDataDef* pmd,
                                                   UInt loadConstants,
                                                   GFxMovieBindProcess** ppbindProcess,
                                                   bool checkCreate, LoadStackItem* ploadStack,
                                                   UPInt memoryArena,
                                                   bool* justCreated)
{        
    GFxResourceLib::BindHandle  bh;    
    GFxMovieDefImpl*            pm = 0;

    // Create an Impl key and see if it can be resolved.
    GFxMovieDefBindStates*      pbindStates   = pls->GetBindStates();
    GFxResourceKey              movieImplKey  = GFxMovieDefImpl::CreateMovieKey(pmd, pbindStates);
    GPtr<GFxMovieBindProcess>   pbp;
    GFxResourceLib::ResolveState rs;

    if ((rs = pls->GetLib()->BindResourceKey(&bh, movieImplKey)) ==
        GFxResourceLib::RS_NeedsResolve)
    {
        // Create a new MovieDefImpl
        // We pass GetStateBagImpl() from loader so that it is used for delegation 
        // when accessing non-binding states such as log and renderer.
        pm = GNEW GFxMovieDefImpl(pmd, pbindStates, pls->pLoaderImpl, loadConstants,
                                  pls->pLoaderImpl->pStateBag, GMemory::pGlobalHeap, 0, memoryArena);

        if (justCreated)
            *justCreated = true;

        //printf("Thr %4d, %8x :  CreateMovieDefImpl - GFxMovieDefImpl constructed for %8x\n",
        //       GetCurrentThreadId(), pm, pmd);

        if (ppbindProcess)
        {
            // Only create bind process for Flash movies, not images.
            *ppbindProcess = GNEW GFxMovieBindProcess(pls, pm, ploadStack);
            if (!*ppbindProcess && pm)
            {
                pm->Release();
                pm = 0;
            }
        }

        // Need to read header first.
        if (pm)
            bh.ResolveResource(pm);
        else
        {
            GString s("Failed to bind SWF file \"", pmd->GetFileURL(), "\"\n");
            bh.CancelResolve(s.ToCStr());
            return 0;
        }

    }
    else
    {
        GASSERT(!checkCreate);
        GUNUSED(checkCreate);

        /*
        if (rs == GFxResourceLib::RS_Available)
        {
        printf("Thr %4d, ________ :  CreateMovieDefImpl - Impl for %8x is in library\n",
        GetCurrentThreadId(), pmd);
        }
        else
        {
        printf("Thr %4d, ________ :  CreateMovieDefImpl - waiting GFxMovieDefImpl for %8x\n",
        GetCurrentThreadId(), pmd);
        }
        */
        GUNUSED(rs);


        // If Available and Waiting resources will be resolved here.
        // Note: Returned value is AddRefed for us, so we don't need to do so.
        if ((pm = (GFxMovieDefImpl*)bh.WaitForResolve()) == 0)
        {
            // Error occurred during loading.
            if (pls->pLog)
                pls->pLog->LogError("Error: %s", bh.GetResolveError());
            return 0;
        }
        if (justCreated)
            *justCreated = false;
    }

    return pm;
}


// Loading version used for look up / bind GFxMovieDataDef based on provided states.
// Used to look up movies serving fonts from GFxFontProviderSWF.
GFxMovieDefImpl*  GFxLoaderImpl::CreateMovie_LoadState(GFxLoadStates* pls,
                                                       GFxMovieDataDef* pmd,
                                                       UInt loadConstants,
                                                       UPInt memoryArena)
{
    if (pmd)
        pls->SetRelativePathForDataDef(pmd);

    GFxResourceLib::BindHandle      bh;
    GPtr<GFxMovieBindProcess>       pbp;
    GFxMovieDefImpl*                pm = CreateMovieDefImpl(pls, pmd, loadConstants,
        &pbp.GetRawRef(), false, NULL, memoryArena);

    if (!pm) return 0;

    // It we need binding, perform it.
    return BindMovieAndWait(pm, pbp, pls, loadConstants);
}




// *** File format detection logic.

GFxLoader::FileFormatType GFxLoaderImpl::DetectFileFormat(GFile *pfile)
{   
    if (!pfile)
        return GFxLoader::File_Unopened;

    SInt            pos     = pfile->Tell();
    FileFormatType  format  = GFxLoader::File_Unknown;
    UByte           buffer[4] = {0,0,0,0};

    if (pfile->Read(buffer, 4) <= 0)
        return GFxLoader::File_Unknown;

    switch(buffer[0])
    {
    case 0x43:
    case 0x46:    
        if ((buffer[1] == 0x57) && (buffer[2] == 0x53))
            format = GFxLoader::File_SWF;
        else if ((buffer[1] == 0x46) && (buffer[2] == 0x58))
            format = GFxLoader::File_GFX;
        break;    

    case 0xFF:
        if (buffer[1] == 0xD8)
            format = GFxLoader::File_JPEG;
        break;

    case 0x89:
        if ((buffer[1] == 'P') && (buffer[2] == 'N') && (buffer[3] == 'G'))
            format = GFxLoader::File_PNG;
        break;

    case 'G':
        if ((buffer[1] == 'I') && (buffer[2] == 'F') && (buffer[3] == '8'))
            format = GFxLoader::File_GIF;
        // 'GFX' also starts with a G.
        if ((buffer[1] == 0x46) && (buffer[2] == 0x58))
            format = GFxLoader::File_GFX;
        break;
    case 'D': // check is it DDS
        if ((buffer[1] == 'D') && (buffer[2] == 'S'))
            format = GFxLoader::File_DDS;
        break;
    }

    pfile->Seek(pos);
    if (format == GFxLoader::File_Unknown)
    {
        // check for extension. TGA format is hard to detect, that is why
        // we use extension test.
        const char* ppath = pfile->GetFilePath();
        if (ppath)
        {
            // look for the last '.'
            const char* pstr = strrchr(ppath, '.');
            if (pstr && GString::CompareNoCase(pstr, ".tga") == 0)
                format = GFxLoader::File_TGA;
        }
    }
    return format;
}




GFxImageResource* GFxLoaderImpl::LoadMovieImage(const char *purl,
                                                GFxImageLoader *ploader, GFxLog *plog,
                                                GMemoryHeap* pheap)
{
    // LoadMovieImage function is only used to load images with 'img://' prefix.
    // This means that we don't cache it in the resourceLib and instead just
    // pass it along to the user (allowing them to refresh the data if necessary).

    GPtr<GImageInfoBase> pimage;
    if (ploader)
        pimage = *ploader->LoadImage(purl);

    if (!pimage)
    {
        if (plog)
            plog->LogScriptWarning(
            "Could not load user image \"%s\" - GFxImageLoader failed or not specified\n", purl);
        pimage = *CreateStaticUserImage();
    }

    // With respect to image keys, we just use a unique key here.
    return pimage? GHEAP_NEW(pheap) GFxImageResource(pimage) : 0;
}

// Create a filler image that will be displayed in place of loadMovie() user images.
GImageInfoBase* GFxLoaderImpl::CreateStaticUserImage()
{
    enum {
        StaticImgWidth      = 19,
        StaticImgHeight     = 12,
        StaticImageScale    = 3,
    };

    // Encodes 'img:' picture with color palette int the back.  
    static char pstaticImage[StaticImgWidth * StaticImgHeight + 1] =
        "aaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaa"
        "rr#rrrrrrrrrrrrrrrr"
        "rrrrrrrrrrrrrrrr##r" 
        "g##gg##g#ggg###g##g"
        "gg#gg#g#g#g#gg#gggg"
        "bb#bb#b#b#b#bb#b##b"
        "bb#bb#b#b#bb###b##b"
        "y###y#y#y#yyyy#yyyy"
        "yyyyyyyyyyy###yyyyy"
        "ccccccccccccccccccc"
        "ccccccccccccccccccc"
        ;

    GPtr<GImage> pimage = *new GImage(GImage::Image_ARGB_8888,
        StaticImgWidth * StaticImageScale,
        StaticImgHeight * StaticImageScale);
    if (pimage)
    {
        for (int y=0; y<StaticImgHeight; y++)
            for (int x=0; x<StaticImgWidth; x++)
            {
                UInt32 color = 0;

                switch(pstaticImage[y * StaticImgWidth + x])
                {
                case '#': color = 0xFF000000; break;
                case 'r': color = 0x80FF2020; break;
                case 'g': color = 0x8020FF20; break;
                case 'b': color = 0x802020FF; break;
                case 'y': color = 0x80FFFF00; break;
                case 'a': color = 0x80FF00FF; break;
                case 'c': color = 0x8000FFFF; break;
                }

                for (int iy = 0; iy < StaticImageScale; iy++)
                    for (int ix = 0; ix < StaticImageScale; ix++)
                    {
                        pimage->SetPixelRGBA(x * StaticImageScale + ix,
                            y * StaticImageScale + iy, color);
                    }
            }
    }

    // Use GImageInfo directly without callback, since this is an alternative
    // to pImageLoadFunc, which is not the same as pImageCreateFunc.    
    return new GImageInfo(pimage);
}

//////////////////////////////////////////////////////////////////////////
// Default implementation of Asian (Japanese, Korean, Chinese) word-wrapping
//
bool GFxTranslator::OnWordWrapping(LineFormatDesc* pdesc)
{
    if (WWMode == WWT_Default)
        return false;
    if ((WWMode & (WWT_Asian | WWT_NoHangulWrap | WWT_Prohibition)) && pdesc->NumCharsInLine > 0)
    {
        UPInt wordWrapPos = GFxWWHelper::FindWordWrapPos
            (WWMode, 
            pdesc->ProposedWordWrapPoint, 
            pdesc->pParaText, pdesc->ParaTextLen, 
            pdesc->LineStartPos, 
            pdesc->NumCharsInLine);
        if (wordWrapPos != GFC_MAX_UPINT)
        {
            pdesc->ProposedWordWrapPoint = wordWrapPos;
            return true;
        }
        return false;
    }
    else if ((WWMode & WWT_Hyphenation))
    {
        if (pdesc->ProposedWordWrapPoint == 0)
            return false;
        const wchar_t* pstr = pdesc->pParaText + pdesc->LineStartPos;
        // determine if we need hyphenation or not. For simplicity,
        // we just will put dash only after vowels.
        UPInt hyphenPos = pdesc->NumCharsInLine;
        // check if the proposed word wrapping position is at the space.
        // if so, this will be the ending point in hyphenation position search.
        // Otherwise, will look for the position till the beginning of the line.
        // If we couldn't find appropriate position - just leave the proposed word
        // wrap point unmodified.
        UPInt endingHyphenPos = (G_iswspace(pstr[pdesc->ProposedWordWrapPoint - 1])) ? 
            pdesc->ProposedWordWrapPoint : 0;
        for (; hyphenPos > endingHyphenPos; --hyphenPos)
        {
            if (GFxWWHelper::IsVowel(pstr[hyphenPos - 1]))
            {
                // check if we have enough space for putting dash symbol
                // we need to summarize all widths up to hyphenPos + pdesc->DashSymbolWidth
                // and this should be less than view rect width
                Float lineW = pdesc->pWidths[hyphenPos - 1];
                lineW += pdesc->DashSymbolWidth;
                if (lineW < pdesc->VisibleRectWidth)
                {
                    // ok, looks like we can do hyphenation
                    pdesc->ProposedWordWrapPoint = hyphenPos;
                    pdesc->UseHyphenation = true;
                    return true;
                }
                else
                {
                    // oops, we have no space for hyphenation mark
                    continue;
                }
                break;
            }
        }
    }
    return false;
}

void GFxTranslator::TranslateInfo::SetResult(const wchar_t* presultText, UPInt resultLen)
{
    GASSERT(pResult);
    if (!presultText)
        return;

    if (resultLen == GFC_MAX_UPINT)
        resultLen = G_wcslen(presultText);
    pResult->Resize(resultLen + 1);
    G_wcsncpy(pResult->GetBuffer(), resultLen + 1, presultText, resultLen);

    Flags |= TranslateInfo::Flag_Translated;
}

void GFxTranslator::TranslateInfo::SetResult(const char* presultTextUTF8, UPInt resultLen)
{
    GASSERT(pResult);
    if (!presultTextUTF8)
        return;

    if (resultLen == GFC_MAX_UPINT)
        resultLen = G_strlen(presultTextUTF8);

    int nchars = (int)GUTF8Util::GetLength(presultTextUTF8);
    pResult->Resize(nchars + 1);
    GUTF8Util::DecodeString(pResult->GetBuffer(), presultTextUTF8, resultLen);

    Flags |= TranslateInfo::Flag_Translated;
}

