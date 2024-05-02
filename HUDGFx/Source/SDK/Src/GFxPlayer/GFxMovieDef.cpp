/**********************************************************************

Filename    :   GFxMovieDef.cpp
Content     :   SWF Player Core movie data structures.
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   This file contains class declarations used in
                GFxPlayerImpl.cpp only. Declarations that need to be
                visible by other player files should be placed
                in GFxCharacter.h.


Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxMovieDef.h"
#include "GFxLoadProcess.h"
#include "GFile.h"

// For GFxSprite and GFxMovieRoot
#include "GFxSprite.h"
#include "GFxPlayerImpl.h"
#include "GFxText.h"

#include "GFxFontGlyphPacker.h"
#include "GFxFontManager.h"

#include "GFxImagePacker.h"

#include "GFxShape.h"
#include "GFxSound.h"

#include "GASAsFunctionObject.h"
#include "GFxAmpServer.h"
#include "AMP/GFxAmpViewStats.h"


// This is the key string that will cause import substitution
// to use GFxFontLib instead in order to support internationalization.
#define GFX_FONTLIB_IMPORT_KEY  "gfxfontlib.swf"


// ***** GFxDataAllocator

// Allocation class used for tag data in GFxMovieDataDef.
GFxDataAllocator::GFxDataAllocator(GMemoryHeap* pheap)
    : pHeap(pheap)
{
    pAllocations = 0;
    pCurrent     = 0;
    BytesLeft    = 0;
}

GFxDataAllocator::~GFxDataAllocator()
{
    while(pAllocations)
    {
        Block*  pnext = pAllocations->pNext;
        GHEAP_FREE(pHeap, pAllocations);
        pAllocations = pnext;
    }
}

void*   GFxDataAllocator::OverflowAlloc(UPInt bytes)
{       
    // If the request is big, give it an individual chunk.
    if (bytes > BlockSize / 2)    
        return AllocIndividual(bytes);    

    if (bytes > BytesLeft)
    {
        Block* pblock = (Block*) GHEAP_ALLOC(pHeap, BlockSize + sizeof(Block), GFxStatMD_Tags_Mem);
        if (!pblock)
            return 0;
        // Insert to allocated list.
        pblock->pNext = pAllocations;
        pAllocations = pblock;
        // Assign free space.
        pCurrent = (UByte*) (pblock + 1);
        BytesLeft= BlockSize;
    }

    void* pmem = pCurrent;
    pCurrent += bytes;
    BytesLeft -= bytes;

    return pmem;
}

void*    GFxDataAllocator::AllocIndividual(UPInt bytes)
{
    Block* pblock = (Block*) GHEAP_ALLOC(pHeap, bytes + sizeof(Block), GFxStatMD_Tags_Mem);
    if (!pblock)
        return 0;
    // Insert to allocated list.
    pblock->pNext = pAllocations;
    pAllocations = pblock;
    return (pblock + 1);
}


// ***** GFxMovieDataDef


GFxMovieDataDef::GFxMovieDataDef(const GFxResourceKey &creationKey,
                                 MovieDataType mtype, const char *purl,
                                 GMemoryHeap* pargHeap, bool debugHeap, UPInt memoryArena)
    : ResourceKey(creationKey), MovieType(mtype)    
{
    // printf("Thr %4d, %8x : GFxMovieDataDef()\n", GetCurrentThreadId(), this);

    // LoadTaskData gets its own heap, if it loaded separately. It is not possible
    // to put GFxMovieDataDef into the same heap since destruction order is not known,
    // so we don't know to which object we should attach heap destroy.
    GMemoryHeap*  pheap = pargHeap;
    if (!pheap)
    {
        GString heapName("MovieData \"", G_GetShortFilename(purl), "\"");
        UInt    heapFlags = debugHeap ? GMemoryHeap::Heap_UserDebug : 0;
        GMemoryHeap::HeapDesc desc(heapFlags);
        desc.Flags      |= GMemoryHeap::Heap_FixedGranularity;
        desc.Granularity = 8*1024;
        desc.Reserve     = 0;
        desc.HeapId      = GHeapId_MovieData;
        desc.Arena       = memoryArena;
        pheap = GMemory::GetGlobalHeap()->CreateHeap(heapName.ToCStr(), desc);
    }     
    pData = *GHEAP_NEW(pheap) LoadTaskData(this, purl, pheap);   

    if (!pargHeap)
        pheap->ReleaseOnFree(pData.GetPtr());
}

GFxMovieDataDef::~GFxMovieDataDef()
{
    //printf("Thr %4d, %8x : ~GFxMovieDataDef()\n", GetCurrentThreadId(), this);
    // Notify LoadDataTask about our death.
    pData->OnMovieDataDefRelease();
}


// ~DefBindingData is executed as a part of ~GFxMovieDataDef, it is
// responsible for calling destructors on BindData linked list content
// that was allocated from tag allocator.
GFxMovieDataDef::DefBindingData::~DefBindingData()
{
    // Destroy bind frames.
    GFxFrameBindData* pframeData = pFrameData;
    pFrameData = 0;
    while(pframeData)
    {
        GFxFrameBindData* pframeNext = pframeData->pNextFrame;
        pframeData->~GFxFrameBindData();
        pframeData = pframeNext;
    }

    // Destroy imports.
    while(pImports)
    {
        GFxImportData *pimport = pImports;
        pImports = pImports->pNext;
        pimport->~GFxImportData();
    }

    // Destroy bind data.
    while(pResourceNodes)
    {
        GFxResourceDataNode* pnode = pResourceNodes;
        pResourceNodes = pResourceNodes->pNext;
        pnode->~GFxResourceDataNode();
    }

    // Destroy fonts data nodes.
    while(pFonts)
    {
        GFxFontDataUseNode* pnode = pFonts;
        pFonts = pFonts->pNext;
        pnode->~GFxFontDataUseNode();
    }
}



// Helper used to look up labeled frames and/or translate frame numbers from a string.
bool    GFxMovieDataDef::TranslateFrameString(
            const GFxStringHashLH<UInt> &namedFrames,
            const char* label, UInt* frameNumber, bool translateNumbers)
{
    if (!label || !label[0])
        return 0;

    // First, determines if a string is a frame number. Generally,
    // if it is NOT a frame number, the string is treated as a label.
    // However, there are some cases when a string number check is
    // not done (i.e. op 0x8C), so that is treated as an option.
    if (translateNumbers)
    {
        // The string must evaluate to an integer to be converted to
        // frame number; frames like "8.5" are treated as labels. Whitespace,
        // '+' or '-' characters, however, are allowed in front of a number string.
        // However, any other characters in the string would cause it to be a label.
        // TBD: May be that matches ECMA ActionScript number conversion ?

        bool        digitFound  = 0;            
        int         i;

        // Check whether to interpret as a number or a label.
        for (i=0; label[i] != 0; i++)
        {
            UByte ch = label[i];
            if ((ch>='0') && (ch <= '9'))
            {
                digitFound = 1;
                continue;
            }
            if (ch == '+' || ch == '-')
            {
                if (!digitFound)
                    continue;
            }
            if (ch == ' ' || ch == '\t')
                continue;
            // Any other char? It's a label.
            goto translate_frame_label;
        }

        // This must be  a frame number: do conversion.
        char*   tail   = 0;
        UInt    number = (UInt) G_strtod(label, &tail);
        // Check for conversion failure.
        if (tail == label || *tail != 0)
            return 0;

        // Frames number arguments start with 1, so make it 0-based.
        *frameNumber = number - 1;
        return 1;
    }

translate_frame_label:

    // We have found a label, do lookup.
    return namedFrames.GetCaseInsensitive(GString(label), frameNumber);
}

// *** GFxMovieDataDef::LoadTaskData


GFxMovieDataDef::LoadTaskData::LoadTaskData(GFxMovieDataDef* pdataDef, const char *purl,
                                            GMemoryHeap* pheap)
    : LoadTaskDataBase(pheap), pHeap(pheap),
      FileURL(purl), GradientIdGenerator(GFxResourceId::IdType_GradientImage), SwdHandle(0)
#ifndef GFC_NO_SOUND
      , pSoundStream(NULL)
#endif
{    
    // We don't store the pDataDef pointer because it is unsafe if
    // GFxMovieDataDef dies in main thread. Just use it for GFxSpriteDef.

    FileAttributes  = 0;
    pMetadata       = 0;
    MetadataSize    = 0;

    pPathAllocator  = GHEAP_NEW(pHeap) GFxPathAllocator();

    LoadingCanceled = false;
    LoadState       = LS_Uninitialized;
    LoadingFrame    = 0;
    TagCount        = 0; // Should be based on export?

    ResIndexCounter = 0;
    InitActionsCnt  = 0;

#ifndef GFC_NO_THREADSUPPORT
    pFrameUpdate    = *GNEW GFxLoadUpdateSync();
#endif

    // Add an empty character id, for createEmptyMovieClip.
    GPtr<GFxSpriteDef> pdef = *GHEAP_NEW(pHeap) GFxSpriteDef(pdataDef);
    pdef->InitEmptyClipDef();
    AddResource(GFxResourceId(GFxCharacterDef::CharId_EmptyMovieClip), pdef);

    GPtr<GFxEditTextCharacterDef> ptextDef = *GHEAP_NEW(pHeap) GFxEditTextCharacterDef();
    ptextDef->InitEmptyTextDef();
    AddResource(GFxResourceId(GFxCharacterDef::CharId_EmptyTextField), ptextDef);
}

GFxMovieDataDef::LoadTaskData::~LoadTaskData()
{
    // Wait for the lock in case if anther object is still accessing this
    ResourceLocker lock(this);
    // Destroy frame data.   
    // Should not need to lock playlist here since when destroying MovieDef /
    // LoadTaskData loading must have completed and playback is finished.    
    UInt i;
    for(i=0; i<Playlist.GetSize(); i++)
        Playlist[i].DestroyTags();
    for(i=0; i<InitActionList.GetSize(); i++)
        InitActionList[i].DestroyTags();  

    if (pPathAllocator)
        delete pPathAllocator;

    if (pMetadata)
    {
        GHEAP_FREE(pHeap, pMetadata);
        pMetadata = 0;
    }   

#ifndef GFC_NO_SOUND
    if (pSoundStream)
        pSoundStream->Release();
#endif // GFC_NO_SOUND

}

// Notifies LoadTaskData that MovieDataDef is being destroyed. This may be a
// premature destruction if we are in the process of loading (in that case it
// will lead to loading being canceled).
void    GFxMovieDataDef::LoadTaskData::OnMovieDataDefRelease()
{
    if (LoadState <= LS_LoadingFrames)
        LoadingCanceled = true;
}


// Initialize an empty movie definition.
void    GFxMovieDataDef::LoadTaskData::InitEmptyMovieDef()
{
    GASSERT(LoadState == LS_Uninitialized);

    // FrameCount of 1 is the default for an empty clip.
    GASSERT(Header.FrameCount == 1);

    {   // Not critical if GFxMovieDataDef is never used for playback while
        // InitEmptyMovieDef hasn't been called; but lock for consistency.
        GLock::Locker lock(&PlaylistLock);
        Playlist.Resize(Header.FrameCount);
        InitActionList.Resize(Header.FrameCount);
        InitActionsCnt  = 0;
    }

    UpdateLoadState(Header.FrameCount, LS_LoadFinished);
}

// Create a definition describing an image file.
void    GFxMovieDataDef::LoadTaskData::InitImageFileMovieDef(
                UInt fileLength, GFxImageResource *pimageResource, bool bilinear)
{
    //GASSERT(LoadState == LS_Uninitialized);

    // Original FrameCount of 1 is critical for proper image load waiting.
    GASSERT(Header.FrameCount == 1);

    Header.FileLength = fileLength;

    // Create image resource and shape character using it.    
    AddResource(GFxResourceId(CharId_ImageMovieDef_ImageResource), pimageResource);

    GPtr<GFxShapeCharacterDef> pshapeDef = *GHEAP_NEW(pHeap) GFxShapeCharacterDef();
    pshapeDef->SetToImage(pimageResource, bilinear);
    AddCharacter(GFxResourceId(CharId_ImageMovieDef_ShapeDef), pshapeDef); 

    // Configure us to have one frame - same as empty initialization.
    {
        GLock::Locker lock(&PlaylistLock);
        Playlist.Resize(Header.FrameCount);
        InitActionList.Resize(Header.FrameCount);
        InitActionsCnt  = 0;
        GASSERT(Playlist.GetSize() == 1);

        // Add a PlaceObject command, so that it creates a shape.
        // We use an individual allocator since we don't want a block of memory to be wasted.
        void* ptagMem = TagMemAllocator.AllocIndividual(
            sizeof(GFxPlaceObjectUnpacked) + sizeof(GASExecuteTag*));
        if (ptagMem)
        {
            GASExecuteTag**  pptagArray = (GASExecuteTag**) ptagMem;
            GFxPlaceObjectUnpacked* ptag       = (GFxPlaceObjectUnpacked*) (pptagArray + 1);
            G_Construct<GFxPlaceObjectUnpacked>(ptag);

            // Verified: Flash assigns depth -16383 to image shapes (depth value = 1 in our list).
            SInt            depth = 1;
            GFxCharPosInfo  pos(GFxResourceId(GFxCharacterDef::CharId_ImageMovieDef_ShapeDef),
                                depth, 0, GRenderer::Cxform(), 1, GRenderer::Matrix());
            ptag->InitializeToAdd(pos);

            // Add playlist frame.
            GASSERT(Playlist.GetSize() == 1);
            pptagArray[0] = ptag;
            Playlist[0].pTagPtrList = pptagArray;
            Playlist[0].TagCount = 1;
        }
        else
        {
            // Not enough memory for tags.
            GASSERT(0);
        }        
    }

    // Notify of load completion.
    UpdateLoadState(Header.FrameCount, LS_LoadFinished);
}

// Return heap used for image data of this movie; creates the heap if it doesn't exist.
GMemoryHeap* GFxMovieDataDef::LoadTaskData::GetImageHeap()
{
    if (!pImageHeap)
    {
        GMemoryHeap::HeapDesc desc;

        desc.Flags |= GMemoryHeap::Heap_FixedGranularity;
        desc.MinAlign    = 32;
        desc.Granularity = 4 * 1024;
        desc.Reserve     = 0;
        desc.Threshold   = ~UPInt(0);
        desc.Limit       = 0;
        desc.HeapId      = GHeapId_Images;

        pImageHeap = *pHeap->CreateHeap("_Images", desc);
    }
    return pImageHeap.GetPtr();
}

// Initializes SWF/GFX header for loading.
void    GFxMovieDataDef::LoadTaskData::BeginSWFLoading(const GFxMovieHeaderData &header)
{
     Header = header;
     UpdateLoadState(LoadingFrame, LS_LoadingFrames);
}


// Read a .SWF pMovie.
void    GFxMovieDataDef::LoadTaskData::Read(GFxLoadProcess *plp, GFxMovieBindProcess* pbp)
{
    GASSERT(LoadState == LS_LoadingFrames); 

    // Reference stream for easy access.
    GFxStream   &stream = *plp->GetStream();       

    // Create list and process tags
    {
        GLock::Locker lock(&PlaylistLock);
        Playlist.Resize(Header.FrameCount);
        InitActionList.Resize(Header.FrameCount);
    }

    stream.LogParseClass(Header.FrameRect); 
    stream.LogParse("Note: SWF Frame Rate = %f, Frames = %d\n",
        Header.FPS, Header.FrameCount);

    // Initialize LoadProcess lists(?)
    //  -> they should be already empty, since each process is only used once
    TagCount = 0;

    // Accumulate tag bytes to know when to send notify to binding thread.
    // We compute update increment based on total file size so that we
    // don't get too few (bad progress) or too many notifications (slow
    // due to expensive thread switching on OS).
    int  tagLoadedBytes = 0;
    bool notifyNeeded   = false;
    int  loadUpdateIncrement = (Header.FileLength / 30);
    if (loadUpdateIncrement < 1024*8)
        loadUpdateIncrement = 1024*8;

    const GFxSWFProcessInfo &pi = plp->GetProcessInfo();
    
    while ((UInt32) stream.Tell() < pi.FileEndPos)
    {
        // Cancel loading if requested externally.
        if (LoadingCanceled)
        {
            // There may be frame tags left; clean them up.
            plp->CleanupFrameTags();
            if (pbp) 
                pbp->SetBindState(GFxMovieDefImpl::BS_Canceled);
            UpdateLoadState(LoadingFrame, LS_LoadCanceled);
            return;
        }

        GFxTagInfo tagInfo;
        GFxTagType tagType = stream.OpenTag(&tagInfo);      

#ifdef GFC_DEBUG_COUNT_TAGS
        plp->CountTag(tagType);
#endif

        tagLoadedBytes += tagInfo.TagLength;

        // Report previous progress to sleeping/waiting binding threads
        // once a while (more efficient then doing it every frame).
        // Update waiters if a lot of data was loaded or if the next
        // tag is big (i.e. could cause slow load).
        #ifndef GFC_NO_THREADSUPPORT
        if (notifyNeeded && 
            ((LoadingFrame == 1) ||
             (tagLoadedBytes > loadUpdateIncrement) ||
             (tagInfo.TagLength > 1024*8)))
        {
            pFrameUpdate->UpdateNotify();
            notifyNeeded     = false;
            tagLoadedBytes   = 0;
        }
        #endif

        plp->ReportProgress(FileURL, tagInfo, false);

        GFxLoaderImpl::LoaderFunction   lf = NULL;
        //in->LogParse("tagType = %d\n", tagType);

        if (tagType == GFxTag_EndFrame)
        {            
            // We handle EndFrame logic after the tag is closed below.
            // That is helpful to ensure that reported bytesLoaded position is correct.
        }
        else if (GFxLoaderImpl::GetTagLoader(tagType, &lf))
        {
            /* Tag decoding helper code
            //  if ((tagType == 83) || (tagType == 2) || (tagType == 32) ||
            // Morph
            //      (tagType == 46) || (tagType == 84) )
            {
            int start = stream.Tell();
            stream.LogParse("*** Tag data for tag %d:\n", tagType);
            stream.LogTagBytes();
            stream.SetPosition(start);
            }
            */

            // call the tag loader.  The tag loader should add
            // characters or tags to the GFxMovieSub data structure.
            (*lf)(plp, tagInfo);
        }
        else
        {
            // no tag loader for this tag type.
            stream.LogParse("*** no tag loader for type %d\n", tagType);
            stream.LogTagBytes();
        }

        stream.CloseTag();
        TagCount++;

        // Handle EndFrame after it's tag is closed.
        if (tagType == GFxTag_EndFrame)
        {
            // Post bound frame and update count.
            if (!FinishLoadingFrame(plp, false))
            {
                // Error occurred; LS_LoadError should already be set internally.
                return;
            }

            notifyNeeded = true;

            // Show frame tag -- advance to the next frame.
            stream.LogParse("  ShowFrame\n");

            // Transfer loaded resources to frame
            // If binding was requested, do binding.
            if (pbp)
            {
                pbp->BindNextFrame();
                // Do we need to do anything about bind errors? Technically
                // they will be sticky in the bind process.
            }
        }

        else if (tagType == 0)
        {
            if ((unsigned)stream.Tell() != pi.FileEndPos)
            {
                // Safety break, so we don't read past the end of the pMovie.               
                stream.LogWarning("Warning: GFxLoader - GFxStream-end tag hit, but not at the "
                                  "end of the file yet; stopping for safety\n");
                break;
            }
        }
    }

    // TBD: Shouldn't we verify the number of frames loaded
    // and set a different state otherwise?

    if (plp->FrameTagsAvailable())
    {
        FinishLoadingFrame(plp, true);

        if (pbp)            
            pbp->BindNextFrame();
        // Do we need to do anything about bind errors? Technically
        // they will be sticky in the bind process.
    }
    else
    {
        // Change state to LS_LoadFinished with notification.
        UpdateLoadState(LoadingFrame, LS_LoadFinished);
    }
}


// Updates bind data and increments LoadingFrame. 
// It is expected that it will also release any threads waiting on it in the future.
bool    GFxMovieDataDef::LoadTaskData::FinishLoadingFrame(GFxLoadProcess *plp, bool finished)
{
    plp->CommitFrameTags();

    // TBD: We could record a pending error status in GFxLoadProcess
    // and check it here...
    bool    success = true;

    GFxFrameBindData* pfdata = plp->CreateFrameBindData();
    if (pfdata)
    {
        // Record the number of resources in frame so that they can be bound.
        pfdata->Frame       = LoadingFrame;
        pfdata->BytesLoaded = plp->GetStream()->Tell() -
                              plp->GetProcessInfo().FileStartPos;
    }

#ifndef GFC_NO_THREADSUPPORT
    GMutex::Locker lock(&pFrameUpdate->GetMutex());
#endif

    if (pfdata)
    {
        // Update the front if necessary and insert item at the end.
        if (!BindData.pFrameData)
            BindData.pFrameData = pfdata;
        else
            BindData.pFrameDataLast->pNextFrame = pfdata;
        BindData.pFrameDataLast = pfdata;

        LoadingFrame++;    
        if (finished)
            LoadState = LS_LoadFinished;
    }
    else
    {
        LoadState = LS_LoadError;
        success   = false;
    }

    // Notify bind thread(s) that file has finished loading.
    // More detailed per-frame data notifications take place in the main
    // loop but only once substantial enough chunk is read.
#ifndef GFC_NO_THREADSUPPORT
    //printf("Next frame is ready %d\n", LoadingFrame);
    if (finished || !success)
        pFrameUpdate->UpdateNotify();        
#endif   

    return success;
}

// Update frame and loading state with proper notification, for use image loading, etc.
void    GFxMovieDataDef::LoadTaskData::UpdateLoadState(UInt loadingFrame, MovieLoadState loadState)
{
#ifndef GFC_NO_THREADSUPPORT
    GMutex::Locker lock(&pFrameUpdate->GetMutex());
    LoadingFrame = loadingFrame;    
    LoadState    = loadState;
    pFrameUpdate->UpdateNotify();
#else
    LoadingFrame = loadingFrame;    
    LoadState    = loadState;
#endif
}

void  GFxMovieDataDef::LoadTaskData::NotifyFrameUpdated()
{
#ifndef GFC_NO_THREADSUPPORT
    GMutex::Locker lock(&pFrameUpdate->GetMutex());
    pFrameUpdate->UpdateNotify();    
#endif
}

// Waits until loading is completed, used by GFxFontLib.
void GFxMovieDataDef::LoadTaskData::WaitForLoadFinish()
{
#ifndef GFC_NO_THREADSUPPORT
    if (LoadState <= LS_LoadingFrames)
    {
        pFrameUpdate->WaitForLoadFinished();
        // LoadState must've been modified before LoadFinished flag is set.
        GASSERT(LoadState > LS_LoadingFrames);
    }
#endif
}

// Waits until loading of a given frame is completed
void GFxMovieDataDef::LoadTaskData::WaitForFrame(UInt frame)
{
#ifndef GFC_NO_THREADSUPPORT
    if (LoadState <= LS_LoadingFrames && LoadingFrame <= frame)
    {
        GMutex::Locker lock(&pFrameUpdate->GetMutex());
        while (LoadState <= LS_LoadingFrames && LoadingFrame <= frame)
            pFrameUpdate->WaitForNotify();
    }
#else
    GUNUSED(frame);
#endif
}


UInt    GFxMovieDataDef::LoadTaskData::GetMetadata(char *pbuff, UInt buffSize) const
{
    if (!pbuff)
        return MetadataSize;
    buffSize = G_Min<UInt>(buffSize, MetadataSize);
    if (pMetadata)
        memcpy(pbuff, pMetadata, buffSize);
    return buffSize;
}


// Sets MetaData of desired size.
void    GFxMovieDataDef::LoadTaskData::SetMetadata(UByte *pdata, UInt size)
{
    // Should only set metadata once.
    GASSERT(pMetadata == 0);
    if ((pMetadata = (UByte*)GHEAP_ALLOC(pHeap, size, GFxStatMD_Other_Mem))!=0)
    {
        MetadataSize = size;
        memcpy(pMetadata, pdata, size);
    }
}

// Add a resource during loading.
void    GFxMovieDataDef::LoadTaskData::AddResource(GFxResourceId rid, GFxResource* pres)
{
    ResourceLocker lock(this);
    Resources.Add(rid, GFxResourceHandle(pres));
}

// Creates a new resource handle with a binding index for the resourceId; used 
// to register ResourceData objects that need to be bound later.
GFxResourceHandle   GFxMovieDataDef::LoadTaskData::AddNewResourceHandle(GFxResourceId rid)
{
    GFxResourceHandle rh(GFxResourceHandle::RH_Index, ResIndexCounter);
    ResIndexCounter++;

    ResourceLocker lock(this);
    Resources.Add(rid, rh);
    return rh;
}

bool    GFxMovieDataDef::LoadTaskData::GetResourceHandle(GFxResourceHandle* phandle, GFxResourceId rid) const
{
    ResourceLocker lock(this);

    ResourceHash::ConstIterator ir = Resources.Find(rid);
    if (ir != Resources.End())
    {
        *phandle = ir->Second;
        return 1;
    }
    return 0;
}



// Get font data bi ResourceId.
GFxFont* GFxMovieDataDef::LoadTaskData::GetFontData(GFxResourceId rid)
{
    GFxFontDataUseNode* pfonts = GetFirstFont();

    // Thread-safe traversal since only completed fonts are added to
    // list. TBD: We might want to check that loading has completed.
    while(pfonts)
    {
        if (pfonts->Id == rid)
            return pfonts->pFontData.GetPtr();
        pfonts = pfonts->pNext;
    }
    return 0;
}



// Labels the frame currently being loaded with the given name.
// A copy of the name string is made and kept in this object.    
void    GFxMovieDataDef::LoadTaskData::AddFrameName(const GString& name, GFxLog *plog)
{
    GUNUSED(plog);
    GASSERT(LoadingFrame < Header.FrameCount);
    
    GLock::Locker lock(&PlaylistLock);    
    NamedFrames.SetCaseInsensitive(name, LoadingFrame);   // stores 0-based frame #
}


void    GFxMovieDataDef::LoadTaskData::SetLoadingPlaylistFrame(const Frame& frame)
{
    // We should not call SetLoadingPlaylistFrame() for the same frame multiple times.
    GLock::Locker lock(&PlaylistLock);
    GASSERT(Playlist[LoadingFrame].TagCount == 0);
    Playlist[LoadingFrame] = frame;
}

void    GFxMovieDataDef::LoadTaskData::SetLoadingInitActionFrame(const Frame& frame)
{
    GLock::Locker lock(&PlaylistLock);
    GASSERT(InitActionList[LoadingFrame].TagCount == 0);
    InitActionList[LoadingFrame] = frame;
    ++InitActionsCnt;
}


// Expose one of our resources under the given symbol,
// for export.  Other movies can import it.    
void    GFxMovieDataDef::LoadTaskData::ExportResource(const GString& symbol, GFxResourceId rid, const GFxResourceHandle &hres)
{
    ResourceLocker lock(this);
    // SWF sometimes exports the same thing more than once!
    // Export names are case-insensitive.
    Exports.SetCaseInsensitive(symbol, hres);
    GASSERT(Exports.FindCaseInsensitive(symbol) != Exports.End());
    InvExports.Set(rid, Exports.FindCaseInsensitive(symbol)->First);
}    


// Returns 0-based frame #
bool    GFxMovieDataDef::LoadTaskData::GetLabeledFrame(const char* label, UInt* frameNumber, bool translateNumbers)
{
    if (LoadState >= LS_LoadFinished)
        return TranslateFrameString(NamedFrames, label, frameNumber, translateNumbers);
    // NamedFrames access is synchronized if loading is not finished.
    GLock::Locker lock(&PlaylistLock);
    return TranslateFrameString(NamedFrames, label, frameNumber, translateNumbers);
}


const GFxTimelineDef::Frame   GFxMovieDataDef::LoadTaskData::GetPlaylist(int frameNumber) const
{
    if (LoadState >= LS_LoadFinished)
        return Playlist[frameNumber];
    
    // Only lock access if we are still loading.
    // This lock shouldn't be a big problem because most playlists
    // will come from GFxSprite where they don't need to be locked.
    // However, we could still get a bottleneck here due to FindPreviousPlaceObject2.
    // If this happens, we might have to come up with a buffered multi-frame copy solution.
    GLock::Locker lock(&PlaylistLock);
    return Playlist[frameNumber];    
}

bool    GFxMovieDataDef::LoadTaskData::GetInitActions(Frame* pframe, int frameNumber) const
{
    GLock::Locker lock(&PlaylistLock);

    if (((UInt)frameNumber) >= InitActionList.GetSize())
        return false;
    *pframe = InitActionList[frameNumber];
    return true;
}

UInt   GFxMovieDataDef::LoadTaskData::GetInitActionListSize() const
{
    GLock::Locker lock(&PlaylistLock);
    return (UInt)InitActionList.GetSize();
}


#ifndef GFC_NO_SOUND
GFxSoundStreamDef*  GFxMovieDataDef::LoadTaskData::GetSoundStream() const   
{ 
    return pSoundStream; 
}
void GFxMovieDataDef::LoadTaskData::SetSoundStream(GFxSoundStreamDef* psoundStream) 
{ 
    if (pSoundStream)
        pSoundStream->Release();
    if (psoundStream)
        psoundStream->AddRef();
    pSoundStream = psoundStream; 
}
#endif

// *** Creating MovieDefData file keys


// GFxMovieDataDef key: {FileName, pFileOpener, optional pImageCreator}
class GFxMovieDataDefFileKeyData : public GRefCountBase<GFxMovieDataDefFileKeyData, GStat_Default_Mem>
{
    friend class GFxMovieDataDefFileKeyInterface;

    GString                 FileName;
    GPtr<GFxFileOpener>     pFileOpener;    
    SInt64                  ModifyTime;
    
    // pImageCreator is unique in that it is only used for GFxMovieDataDef binding
    // if GFxImageCreator::IsKeepingImageData returns false and GFxLoader::LoadKeepBindData
    // flag is not specified. If one of those flags are set, GFxImageCreator is only
    // used as an argument for GFxMovieDefImpl and not MovieDataDef. In that case,
    // its value here will be null and image resources are not created until binding time.
    GPtr<GFxImageCreator>       pImageCreator;    

    GPtr<GFxPreprocessParams>   pPreprocessParams;

public:

    GFxMovieDataDefFileKeyData(const char* pfilename, SInt64 modifyTime,
                               GFxFileOpener* pfileOpener, GFxImageCreator *pimageCreator,
                               GFxPreprocessParams* ppreprocessParams)
    {        
        FileName            = pfilename;
        ModifyTime          = modifyTime;
        pFileOpener         = pfileOpener;
        pImageCreator       = pimageCreator;
        pPreprocessParams   = ppreprocessParams;
    }

    bool operator == (GFxMovieDataDefFileKeyData& other) const
    {
        return  pFileOpener == other.pFileOpener &&
                pImageCreator == other.pImageCreator &&
                ModifyTime == other.ModifyTime &&
                FileName == other.FileName &&
                pPreprocessParams == other.pPreprocessParams;
    }
    bool operator != (GFxMovieDataDefFileKeyData& other) const
    {
        return !operator == (other);
    }

    UPInt  GetHashCode() const
    {
        UPInt fileHashCode = GString::BernsteinHashFunction(FileName.ToCStr(),
                                                              FileName.GetSize());
        return fileHashCode ^
            ((UPInt)ModifyTime) ^
            ((UPInt)pFileOpener.GetPtr()) ^ (((UPInt)pFileOpener.GetPtr()) >> 7) ^
            ((UPInt)pImageCreator.GetPtr()) ^ (((UPInt)pImageCreator.GetPtr()) >> 7) ^
            ((UPInt)pPreprocessParams.GetPtr()) ^ (((UPInt)pPreprocessParams.GetPtr()) >> 7);
    }
};


class GFxMovieDataDefFileKeyInterface : public GFxResourceKey::KeyInterface
{
public:
    typedef GFxResourceKey::KeyHandle KeyHandle;

    virtual void    AddRef(KeyHandle hdata)
    {
        GASSERT(hdata); ((GFxMovieDataDefFileKeyData*) hdata)->AddRef();
    }
    virtual void    Release(KeyHandle hdata)
    {        
        GASSERT(hdata); ((GFxMovieDataDefFileKeyData*) hdata)->Release();
    }

    // Key/Hash code implementation.
    virtual GFxResourceKey::KeyType GetKeyType(KeyHandle hdata) const
    {
        GUNUSED(hdata);
        return GFxResourceKey::Key_File;
    }

    virtual UPInt  GetHashCode(KeyHandle hdata) const
    {
        GASSERT(hdata);        
        return ((GFxMovieDataDefFileKeyData*) hdata)->GetHashCode();
    }

    virtual bool    KeyEquals(KeyHandle hdata, const GFxResourceKey& other)
    {
        if (this != other.GetKeyInterface())
            return 0;        
        return *((GFxMovieDataDefFileKeyData*) hdata) ==
               *((GFxMovieDataDefFileKeyData*) other.GetKeyData());
    }

    const char* GetFileURL(KeyHandle hdata) const
    {
        GFxMovieDataDefFileKeyData* pdata = (GFxMovieDataDefFileKeyData*) hdata;
        GASSERT(pdata);
        return pdata->FileName.ToCStr();
    }
};


static GFxMovieDataDefFileKeyInterface GFxMovieDataDefFileKeyInterface_Instance;

// Create a key for an SWF file corresponding to GFxMovieDef.
GFxResourceKey  GFxMovieDataDef::CreateMovieFileKey(const char* pfilename,
                                                    SInt64 modifyTime,
                                                    GFxFileOpener* pfileOpener,
                                                    GFxImageCreator* pimageCreator,
                                                    GFxPreprocessParams* ppreprocessParams)
{
    GPtr<GFxMovieDataDefFileKeyData> pdata =
        *GNEW GFxMovieDataDefFileKeyData(pfilename, modifyTime, pfileOpener, 
                                        pimageCreator, ppreprocessParams);

    return GFxResourceKey(&GFxMovieDataDefFileKeyInterface_Instance,
                          (GFxResourceKey::KeyHandle)pdata.GetPtr() );
}

GString GFxMovieDataDef::GetResourceName() const
{
    //return GString("MovieData \"", G_GetShortFilename(GetFileURL()), "\"");
    return GString(G_GetShortFilename(GetFileURL()));
}



//
// ***** GFxMovieDefImpl
//

GFxMovieDefImpl::GFxMovieDefImpl(GFxMovieDataDef* pdataDef,
                                 GFxMovieDefBindStates* pstates,
                                 GFxLoaderImpl* ploaderImpl,
                                 UInt loadConstantFlags,
                                 GFxStateBagImpl *pdelegateState,
                                 GMemoryHeap* pargHeap,
                                 bool fullyLoaded,
                                 UPInt memoryArena)    
{    
    //printf("GFxMovieDefImpl::GFxMovieDefImpl: %x, thread : %d\n", this, GetCurrentThreadId());
    GMemoryHeap*  pheap = pargHeap;
    if (!pheap)
    {
        // Heap name above includes extra space to align with "Movie Data".
        GString heapName("MovieDef  \"", G_GetShortFilename(pdataDef->GetFileURL()), "\"");
        UInt    heapFlags = (loadConstantFlags & GFxLoader::LoadDebugHeap) ? 
                            GMemoryHeap::Heap_UserDebug : 0;
        // Binding heap gets small granularity since its initially empty.
        GMemoryHeap::HeapDesc desc(heapFlags, 16, 4*1024, 4*1024);
        desc.HeapId = GHeapId_MovieDef;
        desc.Arena  = memoryArena;
        pheap = GMemory::GetGlobalHeap()->CreateHeap(heapName.ToCStr(), desc);
    }
    

    pBindData = *GHEAP_NEW(pheap) BindTaskData(pheap, pdataDef, this,
                                               loadConstantFlags, fullyLoaded);
    if (!pargHeap)
        pheap->ReleaseOnFree(pBindData.GetPtr());

    pLoaderImpl = ploaderImpl;

    // We MUST have states and DataDef
    GASSERT(pstates);
    pBindStates  = pstates;
    
    // Create a delegated shared state.
    pStateBag = *GNEW GFxStateBagImpl(pdelegateState);   
}

GFxMovieDefImpl::~GFxMovieDefImpl()
{
    pBindData->OnMovieDefRelease();
}




// *** GFxMovieDefBindProcess - used for GFxMovieDefImpl Binding
    
GFxMovieBindProcess::GFxMovieBindProcess(GFxLoadStates *pls,
                                         GFxMovieDefImpl* pdefImpl, 
                                         GFxLoaderImpl::LoadStackItem* ploadStack)
    : GFxLoaderTask(pls, Id_MovieBind),
      pFrameBindData(0), GlyphTextureIdGen(GFxResourceId::IdType_DynFontImage),
      /*pLoadStates(pls), */pBindData(pdefImpl->pBindData), pLoadStack(ploadStack)
{
    // RefCountMode is already thread safe for GFxTask.
    
    pDataDef    = pBindData->GetDataDef();
    Stripped    = ((pDataDef->GetSWFFlags() & GFxMovieInfo::SWF_Stripped) != 0);

    GFxImagePackParamsBase* pimagePacker = pls->GetBindStates()->pImagePackParams;
    if (pimagePacker && !Stripped)
    {
        GFxImageCreateInfo     icreateInfo(NULL, GFxResource::Use_Bitmap);
        icreateInfo.SetStates(0, pls->GetRenderConfig(), pls->GetLog(), NULL,NULL);
        icreateInfo.ThreadedLoading = pls->IsThreadedLoading();
        icreateInfo.pHeap           = pdefImpl->GetBindDataHeap();

        pImagePacker = *pimagePacker->Begin(&GlyphTextureIdGen, pls->GetBindStates()->pImageCreator, &icreateInfo);
        pImagePacker->SetBindData(pBindData);

        pTempBindData = GNEW GFxTempBindData;
    }
    else
        pTempBindData = 0;
}

GFxMovieBindProcess::~GFxMovieBindProcess()
{
#ifndef GFC_NO_THREADSUPPORT
    GPtr<GFxLoadUpdateSync> ploadSync = pBindData ? pBindData->GetBindUpdateSync() : 0;    
#endif

    if (pBindData) 
    {
        if (pBindData->GetBindState() == GFxMovieDefImpl::BS_InProgress)
            pBindData->SetBindState(GFxMovieDefImpl::BS_Canceled);
        pBindData = 0;
    }

    if (pTempBindData)
        delete pTempBindData;
    pImagePacker = 0;

#ifndef GFC_NO_THREADSUPPORT
    // Notify any waiters that our task thread has released
    // all of its references to loaded data.
    if (ploadSync)
        ploadSync->NotifyLoadFinished();
#endif
}

GFxMovieBindProcess::BindStateType GFxMovieBindProcess::BindNextFrame()
{
    // For some reason this check seems to be required, as we
    // may com in here with null pointer after cancel. This check
    // prevents GetBindStateType() from crashing.
    if (!pBindData)
        return GFxMovieDefImpl::BS_Canceled;

    // Check binding state and return correct value if appropriate.
    BindStateType startBindStateType = GetBindStateType();
    
    if (startBindStateType != GFxMovieDefImpl::BS_InProgress)
    {
        if (startBindStateType == GFxMovieDefImpl::BS_NotStarted)
        {
            SetBindState(GFxMovieDefImpl::BS_InProgress|GetBindStateFlags());
            // Binding can only take place once.
            GASSERT(pBindData->BindingFrame == 0);
        }
        else
        {
            return startBindStateType;
        }
    }

    
    // Get the next frame data if possible, waiting for it if necessary.
    // The nature of frames linked list allows us to avoid mutex locks
    // on every access. This is ok as long as updates happen within the
    // lock on the producer (DataDef) thread side.
    GFxFrameBindData* pframeData = GetNextFrameBindData();

#ifndef GFC_NO_THREADSUPPORT
    if (!pframeData)
    {
        GFxMovieDataDef::LoadTaskData* ploadData = pDataDef->pData;

        GMutex::Locker lock(&ploadData->pFrameUpdate->GetMutex());
        pframeData = GetNextFrameBindData();
        // Wait until next frame data is available or loading fails.
        //printf("Waiting for the next frame: %p\n", this);
        while ((pframeData == 0) &&
               (ploadData->LoadState == GFxMovieDataDef::LS_LoadingFrames) &&
               !pBindData->BindingCanceled)
        {            
            ploadData->pFrameUpdate->WaitForNotify();
            pframeData = GetNextFrameBindData();
        };
        //printf("Done waiting for the next frame: %p\n", this);
        if (ploadData->LoadState == GFxMovieDataDef::LS_LoadCanceled)
            pBindData->BindingCanceled = true;
    }
#endif

    if (!pframeData || pBindData->BindingCanceled)
    {                
        // Error occurred in load, turn it into error in processing.
        FinishBinding();
        BindStateType bindState = pBindData->BindingCanceled ? 
                        GFxMovieDefImpl::BS_Canceled : GFxMovieDefImpl::BS_Error;
        SetBindState(bindState | GetBindStateFlags());
        // If binding was canceled, release pBindData so that the load process
        // can terminate as well (necessary if we are on the same thread).
#ifndef GFC_NO_THREADSUPPORT
        GPtr<GFxLoadUpdateSync> psync = pBindData->GetBindUpdateSync();
        pBindData = 0;
        psync->NotifyLoadFinished();
#else
        pBindData = 0;
#endif
        return bindState;
    }

    // At this point we must be processing binding frame (unless we
    // allow empty frame entries in the future). Not that allowing
    // such entries would cause problem with waits & BindingFrame
    // updates, unless done intelligently.
    GASSERT(pBindData->BindingFrame == pframeData->Frame);
    pFrameBindData = pframeData;


    // Perform the main tasks of binding:
    //  1. Resolve all imports and store imported movie references
    //     in ImportSourceMovies.
    //  2. Populate the ResourceBinding table to resolve Handles by using
    //     the resource binding data.
    //  3. Process/pack fonts included in this frame.
    //  4. Update frame data and bytes.

    
    GFxLoadStates *pls        = pLoadStates;
    UInt           loadFlags  = pBindData->LoadFlags;
    bool           fontImportsSucceeded = true;

    // Resolve all imports for this frame.
    if (!(loadFlags & GFxLoader::LoadDisableImports))
    {
        UInt            iimport;
        GFxImportData*  pimportData = pframeData->pImportData;
        
        // Iterate imports for this frame. We must limit the number of
        // imports iterated and not go till the end of the list because
        // the list contains ALL imports in the file.
        for (iimport = 0; iimport < pframeData->ImportCount;
                          iimport++, pimportData = pimportData->pNext)
        {
            // These must hold at this point.
            GASSERT(pimportData != 0);
            GASSERT(pimportData->Frame <= pBindData->BindingFrame);
            
            const GString& sourceURL   = pimportData->SourceUrl;           

            // See if the source URL ends in 'gfxfontlib.swf', in which
            // case we do import font substitution instead.
            static const char fontlibKey[]   = GFX_FONTLIB_IMPORT_KEY;
            const UPInt       fontlibKeySize = sizeof(fontlibKey) - 1;

            if ((sourceURL.GetSize() >= fontlibKeySize) &&
                (GString::CompareNoCase(sourceURL.ToCStr() +
                    (sourceURL.GetSize() - fontlibKeySize), fontlibKey) == 0))
            {
                // This if a substituted font import, handle it in a custom manner.
                bool allSucceed = false;

                // Grab temporary reference to GFxMovieDefImpl to access pSharedSate
                GPtr<GFxMovieDefImpl> pdefImpl = *pBindData->GetMovieDefImplAddRef();
                                
                if (pdefImpl)
                {
                    // Check if FontLib is installed. if it is not or import name is not "$IMECandidateListFont" 
                    // we will treat 'gfxfontlib.swf' file like a usual import file.
                    GPtr<GFxFontLib> pfontLib = pdefImpl->GetFontLib();
                    if (pfontLib.GetPtr() 
#if defined(GFC_USE_IME)
                        ||(pimportData->Imports.GetSize() > 0 &&  pimportData->Imports[0].SymbolName == "$IMECandidateListFont" )
#endif
                        )
                    {
                        allSucceed = pBindData->ResolveImportThroughFontLib(pimportData);
                        if (!allSucceed)
                            fontImportsSucceeded = 0;
                        // TO DO: Font import substitution could cause font resource to be assigned 
                        // to resource data slots of different type (import, etc.) in pathological
                        // cases. We should check for that in the future perhaps by verifying the
                        // type during binding load time...                
                        continue;
                    }
                }
                else
                    continue;
            }


            GFxMovieDefImpl*    pdef = 0;

            // LoadStates are separate in import because import has a separate MovieDef.
            GPtr<GFxLoadStates> pimportLoadStates = *pls->CloneForImport();
            // Load flags must be the same, but we wait for load completion with imports.
            UInt                importLoadFlags = loadFlags | GFxLoader::LoadWaitCompletion;

            // Create import stack. We need it for detecting recursion int loading process.
            // It is safe to create the item on stack because we set LoadWaitCompletion 
            // load state and call to CreateMovie_LoadState will not return until the whole movie
            // with all its dependencies if any is loaded.
            GFxLoaderImpl::LoadStackItem loadStack(pBindData->pDefImpl_Unsafe);
            if (!pLoadStack)
                pLoadStack = &loadStack;
            else
            {
                GFxLoaderImpl::LoadStackItem* ptail = pLoadStack;
                while(ptail->pNext)
                    ptail = ptail->pNext;
                ptail->pNext = &loadStack;
            }

            // Try both SWF and GFX files.
            // If the file is stripped, its imports may be stripped too,
            // so try to load '.gfx' import file first.
        
            // We don't use GetLength() because ".swf" is represented as 4 bytes in UTF8
            if (Stripped && (sourceURL.GetSize() > 4) &&
                    (GString::CompareNoCase(sourceURL.ToCStr() +
                    (sourceURL.GetSize() - 4), ".swf") == 0) )
            {           
                GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_Import,
                                                sourceURL, pls->GetRelativePath());
                loc.FileName.Clear();
                loc.FileName.AppendString(sourceURL.ToCStr(), sourceURL.GetSize() - 4);
                loc.FileName += ".gfx";

                // Use our captured load state to load the imported movie. This
                // guarantees that the states are consistent.
                pdef = GFxLoaderImpl::CreateMovie_LoadState(pimportLoadStates,
                                                            loc, importLoadFlags, pLoadStack);
            }

            if (!pdef)
            {
                GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_Import,
                                                sourceURL, pls->GetRelativePath());
                pdef = GFxLoaderImpl::CreateMovie_LoadState(pimportLoadStates,
                                                            loc, importLoadFlags, pLoadStack);
            }

            // Remove stack item from the stack
            if (pLoadStack == &loadStack)
                pLoadStack = NULL;
            else
            {
                GFxLoaderImpl::LoadStackItem* ptail = pLoadStack;
                while(ptail->pNext)
                {
                    if (ptail->pNext == &loadStack)
                    {
                        ptail->pNext = ptail->pNext->pNext;
                        break;
                    }
                    ptail = ptail->pNext;
                }
                GASSERT(!ptail->pNext);
            }

            if (!pdef)
            {
                // Failed loading; leave us at correct bind frame.
                FinishBinding();
                SetBindState(GFxMovieDefImpl::BS_Error | GetBindStateFlags());
                return GFxMovieDefImpl::BS_Error;
            }
            else
            {
                // It is possible for SWF files to import themselves, which Flash
                // seems to handle gracefully. What happens with indirect recursiveness?
                // TBD: For now we the pass the recursive flag to avoid leaks,
                // but we will need to investigate this in detail in the future.
                bool recursive = (pdef->pBindData == pBindData);
                if (recursive && pls->GetLog())
                    pls->GetLog()->LogWarning("Warning: Self recursive import detected in '%s'\n",
                                              sourceURL.ToCStr());

                pBindData->ResolveImport(pimportData, pdef, pls, recursive);
                // Invoke default import visitor to notify it of resolve.
                GFxImportVisitor* pimportVistor = pls->GetBindStates()->pImportVisitor;
                if (pimportVistor)
                {
                    // Do temporary locked grab of our pDefImpl pointer.
                    GPtr<GFxMovieDefImpl> pdefImpl = *pBindData->GetMovieDefImplAddRef();
                    if (pdefImpl)
                        pimportVistor->Visit(pdefImpl, pdef, sourceURL.ToCStr());
                }

                pdef->Release();
            }
        } // for (iimport...)
    }



    // Create & Resolve all resources in the frame.
    GFxResourceDataNode* presData = pframeData->pResourceData;
    UInt                 ires;
    GFxResourceBinding   &resourceBinding = pBindData->ResourceBinding;
    GMemoryHeap*         pheap = pBindData->GetBindDataHeap();
    
    
    for (ires = 0; ires < pframeData->ResourceCount; ires++, presData = presData->pNext)
    {
        GASSERT(presData != 0);        

        if (!presData->Data.IsValid())
        {
            // We can get here for several reasons:
            //  1) This presData->BindIndex corresponds to an imported resource that failed
            //     to load. In that case we would've already emitted a warning inside of ResolveImport.
            //  2) An image loading JPEG/ZLib Support was not installed causing image
            //     resource data to be null.
            // For now, leave the slot as null.

            // MA NOTE: There used to be a non-null import slot with ASSERT here that was triggered
            // only if fontImportsSucceeded was true, but is seems to serve no purpose.
            GUNUSED(fontImportsSucceeded);
        }
        else
        {
            // Create a resource based on its loaded data in DataDef.
            GFxResourceBindData bd;
            bd.pBinding = &resourceBinding;
            bd.pResource= 0;

            if (presData->Data.CreateResource(&bd, pls, pheap))
            {
                if (pImagePacker && bd.pResource->GetResourceType() == GFxResource::RT_Image &&
                    pTempBindData->FillStyleImageWrap.Get(presData->BindIndex) == 0 &&
                    bd.pResource->GetResourceUse() == GFxResource::Use_Bitmap)
                {
                    GPtr<GFxMovieDefImpl> pdefImpl = *pBindData->GetMovieDefImplAddRef();
                    GFxImageResource* pimageRes = (GFxImageResource*) (GFxResource*) bd.pResource;
                    pImagePacker->AddResource(presData, pimageRes);
                }

                // SetBindData AddRefs to the resource.
                resourceBinding.SetBindData(presData->BindIndex, bd);
            }
            else
            {
                if (pBindData->BindingCanceled)
                {
                    // Handle cancel. We may and up here if resource creating
                    // failed due to cancel (if it tired to access MovieDefImpl).
                    FinishBinding();                    
                    SetBindState(GFxMovieDefImpl::BS_Canceled | GetBindStateFlags());
            #ifndef GFC_NO_THREADSUPPORT
                    GPtr<GFxLoadUpdateSync> psync = pBindData->GetBindUpdateSync();
                    pBindData = 0;
                    psync->NotifyLoadFinished();
            #else
                    pBindData = 0;
            #endif
                    return GFxMovieDefImpl::BS_Canceled;
                }

                // Set empty binding otherwise: this allows for indexing
                // without problems.
                resourceBinding.SetBindData(presData->BindIndex, bd);

                // Fail the rest of loading/binding?
                // Do we block on a certain frame?
                // Who emits the error message?
            }
        }
    }


    // Make a list of fonts generated in this frame.
    if (pframeData->FontCount)
    {
        GArray<GFxFontResource*>   fonts;

        GFxFontDataUseNode* pfontData = pframeData->pFontData;
        UInt                ifont;

        for (ifont = 0; ifont < pframeData->FontCount;
                        ifont++, pfontData = pfontData->pNext)
        {
            GASSERT(pfontData != 0);

            GFxResourceBindData rbd;
            resourceBinding.GetResourceData(&rbd, pfontData->BindIndex);

            if (rbd.pResource)
            {
                GASSERT(rbd.pResource->GetResourceType() == GFxResource::RT_Font);
                GFxFontResource* pfont = (GFxFontResource*) rbd.pResource.GetPtr();
                fonts.PushBack(pfont);           
            }
        }
       
        // Generate font bitmaps for the frame.
        if (fonts.GetSize())
        {
            // NOTE: Some fonts may have already been rasterized if they were imported 
            // through '_glyphs' substitution, which can happen during CreateResource above.

            if (pls->GetFontPackParams() != 0)
            {            
                GFx_GenerateFontBitmaps(pls->GetFontPackParams(), fonts,
                    pls->GetBindStates()->pImageCreator, 
                    pls->GetRenderConfig(),
                    pls->GetLog(),
                    &GlyphTextureIdGen,
                    pheap,
                    pls->IsThreadedLoading());
            }
            else
            {
                // Warn if there is no pack params and cache;
                // this would cause glyphs to be rendered as shapes.
                GFC_DEBUG_WARNING(!pls->GetFontCacheManager() ||
                    !pls->GetFontCacheManager()->IsDynamicCacheEnabled(),
                    "Both font packing and dynamic cache disabled - text will be vectorized");
            }
        }
    }


    // This frame done, advance and update state.
    pBindData->BytesLoaded = pframeData->BytesLoaded;
    GAtomicOps<UInt>::Store_Release(&pBindData->BindingFrame, pBindData->BindingFrame + 1);
    
    if (pBindData->BindingFrame == 1)
    {
        // Frame 1 is special because clients can wait for it, so alter state.
        SetBindState(GetBindState() | GFxMovieDefImpl::BSF_Frame1Loaded);
    }    
    
    if (pBindData->BindingFrame == pDataDef->GetFrameCount())
    {  
        // Update bytes loaded for the end of the file. This is necessary
        // because we don't count the size of final tag 0 during loading.
        pBindData->BytesLoaded = pDataDef->GetFileBytes();
        if (pImagePacker)
        {
            struct ImagePackVisitor : public GFxMovieDef::ResourceVisitor
            {
                GFxImagePacker*       pImagePacker;
                    
                ImagePackVisitor(GFxImagePacker* pImagePacker):pImagePacker(pImagePacker) { }

                virtual void    Visit(GFxMovieDef* pmovieDef, GFxResource* presource,
                    GFxResourceId rid, const char* pexportName) 
                {
                    GUNUSED2(rid, pmovieDef);
                    pImagePacker->AddImageFromResource((GFxImageResource*)presource, pexportName);
  
                }
            } visitor(pImagePacker);
            GPtr<GFxMovieDefImpl> pdefImpl = *pBindData->GetMovieDefImplAddRef();
            pdefImpl->VisitResources(&visitor, GFxMovieDef::ResVisit_Bitmaps);
        }
        
        FinishBinding();
        // Update state and we are done.
        SetBindState(GFxMovieDefImpl::BS_Finished |
                     GetBindStateFlags() | GFxMovieDefImpl::BSF_LastFrameLoaded);
    }

    GFxProgressHandler* ph = pls->GetProgressHandler();
    if (ph)
    {
        ph->ProgressUpdate(GFxProgressHandler::Info(pBindData->GetDataDef()->pData->FileURL,
            pBindData->BytesLoaded, pBindData->GetDataDef()->GetFileBytes(), 
            pBindData->BindingFrame, pBindData->GetDataDef()->GetFrameCount()));
    }

    return GetBindStateType();
}


void    GFxMovieBindProcess::FinishBinding()
{
    if (pImagePacker)
        pImagePacker->Finish();

    pBindData->ResourceBinding.Freeze();
}

GFxMovieDef::MemoryContext* GFxMovieDefImpl::CreateMemoryContext(const char* heapName, 
                                                                 const MemoryParams& memParams,
                                                                 bool debugHeap)
{
    UInt heapFlags = debugHeap ? GMemoryHeap::Heap_UserDebug : 0;
    GMemoryHeap::HeapDesc desc = memParams.Desc;
    desc.Flags |= heapFlags;
    desc.Flags |= GMemoryHeap::Heap_FastTinyBlocks;
    desc.Flags |= GMemoryHeap::Heap_ThreadUnsafe;
#ifdef GFX_AMP_SERVER
    // disable Heap_ThreadUnsafe for AMP, 
    // since we get all memory reports from display thread
    desc.Flags &= ~GMemoryHeap::Heap_ThreadUnsafe;
#endif

    desc.HeapId = GHeapId_MovieView;
    desc.Limit  = GFxMovieRoot::MemoryContextImpl::HeapLimit::INITIAL_DYNAMIC_LIMIT; // 128K is the initial dynamic limit

    GMemoryHeap* heap = GMemory::GetGlobalHeap()->CreateHeap(heapName, desc);
    GFxMovieRoot::MemoryContextImpl* memContext = GHEAP_NEW(heap) GFxMovieRoot::MemoryContextImpl();
    memContext->Heap = heap;
#ifndef GFC_NO_GC
    memContext->ASGC = *GHEAP_NEW(heap) GASRefCountCollector();
    memContext->ASGC->SetParams(memParams.FramesBetweenCollections, memParams.MaxCollectionRoots);
#endif
    memContext->StringManager = *GHEAP_NEW(heap) GASStringManager(heap);

    memContext->LimHandler.UserLevelLimit        = memParams.Desc.Limit;
    memContext->LimHandler.HeapLimitMultiplier   = memParams.HeapLimitMultiplier;

    heap->SetLimitHandler(&memContext->LimHandler);
    heap->ReleaseOnFree(memContext);

    return memContext;
}


// Create a playable GFxMovieSub instance from a def.
GFxMovieView* GFxMovieDefImpl::CreateInstance(const MemoryParams& memParams, bool initFirstFrame)    
{
    GString heapName("MovieView \"", G_GetShortFilename(GetFileURL()), "\"");

    GFxMovieDef::MemoryContext* memContext = CreateMemoryContext(heapName.ToCStr(), memParams, 
                                                (GetLoadFlags() & GFxLoader::LoadDebugHeap) != 0);

    GFxMovieView* movieView = CreateInstance(memContext, initFirstFrame);
    memContext->Release();  // the MovieView owns the context now
    return movieView;
}

GFxMovieView* GFxMovieDefImpl::CreateInstance(MemoryContext* memContext, bool initFirstFrame)
{
    GFxMovieRoot* proot = GFxMovieRoot::Create(memContext);
    if (proot == NULL)
    {
        return NULL;
    }

    // Note: Log is inherited dynamically from Def, so we don't set it here
    GPtr<GFxSprite> prootMovie = 
        *GHEAP_NEW(proot->GetMovieHeap()) GFxSprite(GetDataDef(), this, proot,
        NULL, GFxResourceId());

    if (!prootMovie)
    {
        proot->Release();
        return NULL;
    }

    // Assign level and _level0 name.
    prootMovie->SetLevel(0);
    proot->SetLevelMovie(0, prootMovie);

    // Register aux AS classes defined in external libraries
    proot->RegisterAuxASClasses();

    if (proot->AdvanceStats)
    {
        proot->AdvanceStats->SetMovieDef(proot->GetMovieDef());
    }

    if (initFirstFrame)
    {
        proot->Advance(0.0f, 0);
    }

    // AddRef unnecessary, refCount == 1 on new
    return proot;
}

// *** GFxMovieDefImpl::LoadData


GFxMovieDefImpl::BindTaskData::BindTaskData(GMemoryHeap* pheap,
                                            GFxMovieDataDef *pdataDef,
                                            GFxMovieDefImpl *pdefImpl,
                                            UInt loadFlags, bool fullyLoaded)
    : pHeap(pheap), pDataDef(pdataDef), pDefImpl_Unsafe(pdefImpl), ResourceBinding(pheap)
{    
    // Technically BindTaskData should not store a pointer to GFxMovieDefImpl,
    // however, we just pass it to initialize regular ptr in GFxResourceBinding.
    // This should not be a problem since it is only used where DefImpl reference
    // is guaranteed to exist.
    // (We should reset it to 0 on DefImpl death for consistency).
    ResourceBinding.SetOwnerDefImpl(pdefImpl);

    LoadFlags   = loadFlags;    

    // Initialize binding progress state.
    BindingCanceled = 0;
    BindingFrame    = 0;
    BytesLoaded     = 0; 
    BindState       = BS_NotStarted;    

#ifndef GFC_NO_THREADSUPPORT
    pBindUpdate = *GNEW GFxLoadUpdateSync();
#endif

    if (fullyLoaded)
    {
        BindingFrame = GetDataDef()->GetLoadingFrame();
        BytesLoaded  = GetDataDef()->GetFileBytes();
    }
}

GFxMovieDefImpl::BindTaskData::~BindTaskData()
{
#ifndef GFC_NO_THREADSUPPORT
    // Wait for the lock if some other objects is still accessing the object
    GMutex::Locker lock(&pBindUpdate->GetMutex());
#endif
    // Release all binding references before ImportSourceMovies is
    // cleared. This is required because GFxSpriteDef references contain
    // GASExecuteTag tags that must be destroyed before MovieDefImpl is
    // released; thus all imported binding references must be cleared first.    
    // If this is not done tag destructors may crash if MovieDefImpl was
    // released first.
    ResourceBinding.Destroy();
}


void    GFxMovieDefImpl::BindTaskData::OnMovieDefRelease()
{
    // Clear pointers to our pDataDef.
    ResourceBinding.SetOwnerDefImpl(0);
    {   // lock scope.
        GLock::Locker lock(&ImportSourceLock);
        pDefImpl_Unsafe = 0;
    }
    
    // Set flag to cancel binding process.
    if (GetBindStateType() <= BS_InProgress)
        BindingCanceled = true;
    // Signal the frame update mutex, since we could be waiting on it.
    pDataDef->pData->NotifyFrameUpdated();
}


// Grabs OUR GFxMovieDefImpl through a lock; can return null.
GFxMovieDefImpl*    GFxMovieDefImpl::BindTaskData::GetMovieDefImplAddRef()
{
    GLock::Locker lock(&ImportSourceLock);
    if (pDefImpl_Unsafe && pDefImpl_Unsafe->AddRef_NotZero())        
        return pDefImpl_Unsafe;    
    return 0;
}


// Bind State Updates / Waiting.

void    GFxMovieDefImpl::BindTaskData::SetBindState(UInt newState)
{
#ifndef GFC_NO_THREADSUPPORT
    GMutex::Locker lock(&pBindUpdate->GetMutex());
    BindState = newState;
    pBindUpdate->UpdateNotify();    
#else
    BindState = newState;
#endif
//    if (GetBindStateType() == BS_Canceled)
//        printf("GFxMovieDefImpl::BindTaskData::SetBindState, thread: %d\n", GetCurrentThreadId());
}

bool   GFxMovieDefImpl::BindTaskData::WaitForBindStateFlags(UInt flags)
{
    //printf("GFxMovieDefImpl::BindTaskData::WaitForBindStateFlags: %x, thread: %d\n", this, GetCurrentThreadId());
#ifndef GFC_NO_THREADSUPPORT
    // Wait for for bind state flag or error. Return true for success,
    // false if bind state was changed to error without setting the flags.
    GMutex::Locker lock(&pBindUpdate->GetMutex());

    while((GetBindStateType() < BS_Canceled) &&
        !(GetBindState() & flags))
    {
        pBindUpdate->WaitForNotify();
    }
#endif

    return (GetBindState() & flags) != 0;
}

// Updates binding state Frame and BytesLoaded (called from image loading task).
void    GFxMovieDefImpl::BindTaskData::UpdateBindingFrame(UInt frame, UInt32 bytesLoaded)
{
    // Assign BytesLoaded first so that Advance gets them correctly for frame.
    BytesLoaded  = bytesLoaded;
    GAtomicOps<UInt>::Store_Release(&BindingFrame, frame);
}



// Access import source movie based on import index with a lock.
GFxMovieDefImpl*    GFxMovieDefImpl::BindTaskData::GetImportSourceMovie(UInt importIndex)
{
    // Access ImportSourceMovies in a lock.
    GLock::Locker loc(&ImportSourceLock);
    if (importIndex >= ImportSourceMovies.GetSize())
        return 0;
    return ImportSourceMovies[importIndex];
}

// Adds a movie reference to ResourceImports array.
void    GFxMovieDefImpl::BindTaskData::AddResourceImportMovie(GFxMovieDefImpl *pdefImpl)
{
    ResourceImports.PushBack(pdefImpl);
}


// Grabs the stuff we want from the source pMovie.
void    GFxMovieDefImpl::BindTaskData::ResolveImport(
                    GFxImportData* pimport, GFxMovieDefImpl* pdefImpl,
                    GFxLoadStates* pls, bool recursive)
{    
    bool imported = false;

    // Go through all character ids and resolve their handles.
    for (UInt i=0; i< pimport->Imports.GetSize(); i++)
    {
        GFxImportData::Symbol &symbol = pimport->Imports[i];

        // Look for exports and substitute.

        // Do the import.
        GFxResourceBindData bindData;

        if (!pdefImpl->GetExportedResource(&bindData, symbol.SymbolName))
        {               
            if (pls->GetLog())
                pls->GetLog()->LogError(
                    "Import error: GFxResource '%s' is not exported from movie '%s'\n",
                    symbol.SymbolName.ToCStr(), pimport->SourceUrl.ToCStr());
        }
        else
        {
            // Add it to binding for the character.
            // Set imported flag so that we can AddRef to movieDef of an import,
            // allowing resources such as fonts to work right.
            imported = SetResourceBindData(GFxResourceId(symbol.CharacterId),
                bindData, symbol.SymbolName.ToCStr());
        }
    }

    // Hold a ref, to keep this source GFxMovieDef alive.
    // TBD: Why only add if imported ??
    if (imported)
    {
        if (!recursive)
        {
            GLock::Locker loc(&ImportSourceLock);
            ImportSourceMovies.PushBack(pdefImpl);
        }
    }

    // No font substitution for recursive imports.
    if (recursive)
        return;


    // Name-based font import substitution:
    // The Font EXPORT feature in Flash Studio ignores the embedded glyphs and instead,
    // exports a fixed number of glyphs (243 or larger, depending on studio version), 
    // which makes it unstable for font imports. 
    // This name-based substitution can be used instead, replacing "Device Fonts" with
    // identically named font from the shared font.swf file.

    // To make exported fonts match up using export name we need glyphs to come
    // from a different file. Font substitution occurs if source import file name
    // has '_glyphs' in it.
    bool        forceFontSubstitution = 0;    
    GString   lowerURL = pimport->SourceUrl.ToLower();

    if (strstr(lowerURL.ToCStr(), "_glyphs") != 0)
    {
        forceFontSubstitution = 1;

        // Save substitution so that it can be potentially checked.
        pls->SubstituteFontMovieDefs.PushBack(pdefImpl);
    }


    /*  Font substitution difficulties and how they are addressed:

    Full font substitution can not take place here because some of the
    FontUse entries (for future frames) may not yet be created. Similarly
    there might be some thread considerations having to do with overwriting
    fonts bound in previous frames.

    Specifically, there are two substitution problems:

    1) Previously bound fonts could have already been used by text field
    instances; substituting them may be too late.

    For now: Go through list, substitute anyway. This would not be
    a problem if we could force all imports to take place before
    font definitions. It is not clear if this is practical and can be
    controlled in Flash studio to the extent we need - TBD (need
    to research this).

    For now, we can have text field cached texture glyph batches store
    original font pointers and compare them to indexed font handle lookup
    result. Text fields would then be updated in the event of change.
    TBD - any thread issues with this?


    2) Future fonts may not have FontUse array entries and thus need to 
    be substituted later on.

    Solution: we add all forced substitutes sources to the
    pls->SubstituteFontMovieDefs array. This array will need to be checked
    when GFxFontResourceCreator::CreateResource is called to create
    GFxFontResource from GFxFontData.

    */


    // Iterate through fonts and resolve fonts with no glyphs from the def.
    GFxFontDataUseNode* pfont       = GetDataDef()->GetFirstFont();
    GFxFontDataUseNode* psourceFont = pdefImpl->GetDataDef()->GetFirstFont();

    for(; pfont != 0; pfont = pfont->pNext)
    {        
        GFxFont* pfontData =pfont->pFontData;

        if ((pfontData->GetGlyphShapeCount() == 0) || forceFontSubstitution)
        {
            // Search imports for the font with the same name which has glyphs.       
            GFxFontDataUseNode* psfont = psourceFont;

            for(; psfont != 0; psfont = psfont->pNext)
            {
                GFxFont *psourceFontData = psfont->pFontData;

                if (psourceFontData->GetGlyphShapeCount() > 0)
                {
                    if (pfontData->MatchSubstituteFont(psourceFontData))
                    {
                        // With DefData separation, we do not need to ReplaceExport
                        // any more; updating the binding should be good enough.

                        // Set our binding.
                        // Note: Unlike us, the source is guaranteed to be fully loaded when
                        // this takes place, so we can just look up its binding.
                        GFxResourceBindData sourceBindData;
                        pdefImpl->pBindData->ResourceBinding.GetResourceData(
                                                        &sourceBindData,psfont->BindIndex);

                        if (sourceBindData.pResource)
                        {
                            ResourceBinding.SetBindData(pfont->BindIndex, sourceBindData);
                        }

                        break;
                    }
                }
            }
        }
    }

}


// Resolves an import of 'gfxfontlib.swf' through the GFxFontLib object.
bool    GFxMovieDefImpl::BindTaskData::ResolveImportThroughFontLib(GFxImportData* pimport)
{    
    // Go through all character ids and resolve their handles.
    for (UInt i=0; i< pimport->Imports.GetSize(); i++)
    {
        
        GFxImportData::Symbol &symbol = pimport->Imports[i];
        GFxResourceBindData bindData;
        bindData.pBinding = &ResourceBinding;
        // Create a new font with FF_NotResolved flag set. This will instruct FontManager
        // to search this font through FontLib and FontProvider
        GPtr<GFxFont> font = *new GFxFontData(symbol.SymbolName.ToCStr(), 0);
        font->SetNotResolvedFlag();
        bindData.pResource= *new GFxFontResource(font, &ResourceBinding);

        SetResourceBindData(GFxResourceId(symbol.CharacterId), bindData,
                                          symbol.SymbolName.ToCStr());
    }
    
    // Add an empty import so that ImportSourceMovies still matches ImportData.
    GLock::Locker loc(&ImportSourceLock);
    ImportSourceMovies.PushBack(0);
    return true;
}


// Internal helper for import updates.
bool    GFxMovieDefImpl::BindTaskData::SetResourceBindData(
                    GFxResourceId rid, GFxResourceBindData& bindData,
                    const char* pimportSymbolName)
{
    GFxResourceHandle rh;
    if (GetDataDef()->GetResourceHandle(&rh, rid))
    {
        GASSERT(rh.IsIndex());

        // Establish an association for BindIndex; note that
        // ResourceBindData is not used here. For imported characters
        // ResourceData elements will be empty.
        ResourceBinding.SetBindData(rh.GetBindIndex(), bindData);

        // Return imported flag so that the caller knows when to AddRef to the MovieDef
        return true;
    }

    // The handle for import MUST exists in DataDef since it is created 
    // during the Read of import tags. If it doesn't, there is an error or
    // perhaps we are out of memory.   
    GFC_DEBUG_WARNING1(1,
        "Internal import bind error: bind handle not found for resource '%s'",
        pimportSymbolName);
    GUNUSED(pimportSymbolName); // For release build.

    return false;
}

bool    GFxMovieDefImpl::BindTaskData::SetResourceBindData(GFxResourceDataNode *presnode, GFxResource* pres)
{
    GFxResourceBindData bd;
    bd.pBinding = &ResourceBinding;
    bd.pResource = pres;
    ResourceBinding.SetBindData(presnode->BindIndex, bd);
    return true;
}

// ***** GFxMovieDefBindStates

// GFxMovieDefBindStates is used as a part of the key object in GFxMovieDefImpl.
UPInt  GFxMovieDefBindStates::GetHashCode() const
{
    return     
        ((UPInt)pFileOpener.GetPtr()) ^ (((UPInt)pFileOpener.GetPtr()) >> 7) ^
        ((UPInt)pURLBulider.GetPtr()) ^ (((UPInt)pURLBulider.GetPtr()) >> 7) ^
        ((UPInt)pImageCreator.GetPtr()) ^ (((UPInt)pImageCreator.GetPtr()) >> 7) ^
        ((UPInt)pImportVisitor.GetPtr()) ^ (((UPInt)pImportVisitor.GetPtr()) >> 7) ^
        // ((UPInt)pImageVisitor.GetPtr()) ^ (((UPInt)pImageVisitor.GetPtr()) >> 7) ^
        ((UPInt)pGradientParams.GetPtr()) ^ (((UPInt)pGradientParams.GetPtr()) >> 7) ^
        ((UPInt)pFontPackParams.GetPtr()) ^ (((UPInt)pFontPackParams.GetPtr()) >> 7) ^
        ((UPInt)pPreprocessParams.GetPtr()) ^ (((UPInt)pPreprocessParams.GetPtr()) >> 7) ^
        ((UPInt)pFontCompactorParams.GetPtr()) ^ (((UPInt)pFontCompactorParams.GetPtr()) >> 7) ^
        ((UPInt)pImagePackParams.GetPtr()) ^ (((UPInt)pImagePackParams.GetPtr()) >> 7);
}

bool GFxMovieDefBindStates::operator == (GFxMovieDefBindStates& other) const
{
    // For bind states to be identical all of the binding
    // states must match.
    return pFileOpener == other.pFileOpener &&
        pURLBulider == other.pURLBulider &&
        pImageCreator == other.pImageCreator &&
        pImportVisitor == other.pImportVisitor &&
        //      pImageVisitor == other.pImageVisitor &&
        pGradientParams == other.pGradientParams &&
        pFontPackParams == other.pFontPackParams &&           
        pPreprocessParams == other.pPreprocessParams &&
        pFontCompactorParams == other.pFontCompactorParams &&
        pImagePackParams == other.pImagePackParams;
}


// ***** GFxMovieDefImpl Key 

class  GFxMovieDefImplKey : public GRefCountBase<GFxMovieDefImplKey, GStat_Default_Mem>
{
    // MovieDefImpl key consists of DataDef and bind states pointers.
    // pDataDef can not be inside of GFxMovieDefBindStates because
    // the later is used in GFxLoadStates and thus referenced by the
    // loading process.
    GPtr<GFxMovieDataDef>       pDataDef;
    GPtr<GFxMovieDefBindStates> pBindStates;
public:

    GFxMovieDefImplKey(GFxMovieDataDef* pdataDef, GFxMovieDefBindStates* pbindStates)
        : pDataDef(pdataDef), pBindStates(pbindStates)
    {        
    }

    // Functionality necessary for this GFxMovieDefBindStates to be used
    // as a key object for GFxMovieDefImpl.
    UPInt   GetHashCode() const
    {
        return  ((UPInt)pDataDef.GetPtr()) ^ (((UPInt)pDataDef.GetPtr()) >> 7) ^
                pBindStates->GetHashCode();
    }
    bool operator == (GFxMovieDefImplKey& other) const
    {
        return pDataDef == other.pDataDef && *pBindStates == *other.pBindStates;
    }
    bool operator != (GFxMovieDefImplKey& other) const { return !operator == (other); }    
};


class GFxMovieDefImplKeyInterface : public GFxResourceKey::KeyInterface
{
public:
    typedef GFxResourceKey::KeyHandle KeyHandle;

    virtual void    AddRef(KeyHandle hdata)
    {
        GASSERT(hdata); ((GFxMovieDefImplKey*) hdata)->AddRef();
    }
    virtual void    Release(KeyHandle hdata)
    {        
        GASSERT(hdata); ((GFxMovieDefImplKey*) hdata)->Release();
    }
    virtual GFxResourceKey::KeyType GetKeyType(KeyHandle hdata) const
    {
        GUNUSED(hdata); return GFxResourceKey::Key_Unique;
    }

    virtual UPInt   GetHashCode(KeyHandle hdata) const
    {
        GASSERT(hdata);        
        return ((GFxMovieDefImplKey*) hdata)->GetHashCode();
    }
    virtual bool    KeyEquals(KeyHandle hdata, const GFxResourceKey& other)
    {
        if (this != other.GetKeyInterface())
            return 0;        
        return *((GFxMovieDefImplKey*) hdata) ==
               *((GFxMovieDefImplKey*) other.GetKeyData());
    }
};

static GFxMovieDefImplKeyInterface GFxMovieDefImplKeyInterface_Instance;

// Create a key for an SWF file corresponding to GFxMovieDef.
GFxResourceKey  GFxMovieDefImpl::CreateMovieKey(GFxMovieDataDef *pdataDef,
                                                GFxMovieDefBindStates* pbindStates)
{
    GPtr<GFxMovieDefImplKey> pkey = *new GFxMovieDefImplKey(pdataDef, pbindStates);

    return GFxResourceKey(&GFxMovieDefImplKeyInterface_Instance,
                          (GFxResourceKey::KeyHandle)pkey.GetPtr());
}




// Fill in the binding resource information together with its binding.
bool GFxMovieDefImpl::GetExportedResource(GFxResourceBindData *pdata, const GString& symbol, GFxMovieDefImpl* ignoreDef)
{
    // Thread TBD: Should we block for availability depending on whether
    // the source has finished loading?

    GFxResourceHandle hres;
    bool              exportFound = 0;
    
    {   // Lock Exports.
        GFxMovieDataDef::LoadTaskData::ResourceLocker lock(GetDataDef()->pData);
        exportFound = GetDataDef()->pData->Exports.GetCaseInsensitive(symbol, &hres);
    }

    if (exportFound)
    {
        // Determine the exact binding
        if (hres.IsIndex())
        {
            pBindData->ResourceBinding.GetResourceData(pdata, hres.GetBindIndex());
        }
        else
        {
            pdata->pBinding = &pBindData->ResourceBinding;
            pdata->pResource= hres.GetResource(&pBindData->ResourceBinding);
        }
        return (pdata->pResource.GetPtr() != 0);
    }

    else
    {
        // Nested import symbols are also visible in parent, although
        // our own Exports table has precedence (checked for above).
      
        GArray<GPtr<GFxMovieDefImpl>, GFxStatMD_Other_Mem> importsCopy;
        
        { // Access ImportSourceMovies in a lock.
            GLock::Locker loc(&pBindData->ImportSourceLock);

            importsCopy.Reserve(pBindData->ImportSourceMovies.GetSize());
            for (UPInt i = 0; i< pBindData->ImportSourceMovies.GetSize(); i++)
            {
                GFxMovieDefImpl* pdef = pBindData->ImportSourceMovies[i];
                if (pdef != ignoreDef)
                    importsCopy.PushBack(pdef);
            }
        }
              
        // TBD: Any particular traversal order ?
        for (UInt i = 0; i<importsCopy.GetSize(); i++)
        {
            GFxMovieDefImpl* pdef = importsCopy[i].GetPtr();
            if (pdef && pdef->GetExportedResource(pdata, symbol))
                return 1;
        }
    }

    return 0;
}

bool GFxMovieDefImpl::DoesDirectlyImport(const GFxMovieDefImpl* import)
{
    GLock::Locker loc(&pBindData->ImportSourceLock);

    for (UPInt i = 0; i < pBindData->ImportSourceMovies.GetSize(); i++)
    {
        GFxMovieDefImpl* pdef = pBindData->ImportSourceMovies[i];
        if (pdef == import)
            return true;
    }
    return false;   
}

const GString*   GFxMovieDefImpl::GetNameOfExportedResource(GFxResourceId rid) const
{
    GFxMovieDataDef::LoadTaskData::ResourceLocker lock(GetDataDef()->pData);
    return GetDataDef()->pData->InvExports.Get(rid);
}
GFxResourceId GFxMovieDefImpl::GetExportedResourceIdByName(const GString& name) const
{
    typedef GHashLH<GFxResourceId, GStringLH, GFixedSizeHash<GFxResourceId> > THASH;
    GFxMovieDataDef::LoadTaskData::ResourceLocker lock(GetDataDef()->pData);
    THASH::ConstIterator it = GetDataDef()->pData->InvExports.Begin();
    for(; it != GetDataDef()->pData->InvExports.End(); ++it)
    {
        if (it->Second == name)
            return it->First;
    }
    return GFxResourceId();
}


// *** Resource Lookup


// Obtains full character creation information, including GFxCharacterDef.
GFxCharacterCreateInfo     GFxMovieDefImpl::GetCharacterCreateInfo(GFxResourceId rid)
{
    GFxResourceHandle      rh;
    
    GFxCharacterCreateInfo ccinfo;
    ccinfo.pCharDef     = 0;
    ccinfo.pBindDefImpl = 0;

    if (GetDataDef()->GetResourceHandle(&rh, rid))
    {        
        GFxResourceBinding* pbinding;
        GFxResource*        pres = 
            rh.GetResourceAndBinding(&pBindData->ResourceBinding, &pbinding);
        if (pres)
        {
            if (pres->GetResourceType() & GFxResource::RT_CharacterDef_Bit)
            {
                ccinfo.pBindDefImpl = pbinding->GetOwnerDefImpl();
                ccinfo.pCharDef     = (GFxCharacterDef*) pres;                
            }
        }
    }
    
    return ccinfo;
}




// Calls back the visitor for each GFxMovieSub that we
// import symbols from.
void    GFxMovieDefImpl::VisitImportedMovies(GFxMovieDefImpl::ImportVisitor* visitor)
{

    // Thread TBD: Ensure that all imports are loaded (?)
    GFxImportData* pimportData = GetDataDef()->GetFirstImport();
    
    if (pimportData)
    {
        // TBD: Visited may no longer be necessary in 2.0. However,
        // we need to verify that it's not possible to produce to identical import
        // file references in the Flash studio.
        GFxStringHash<bool>        visited;  
    
        while (pimportData)
        {   
            GFxMovieDefImpl* pindexedMovie = 0;

            { // Access ImportSourceMovies in a lock.
                GLock::Locker loc(&pBindData->ImportSourceLock);

                if (pimportData->ImportIndex >= pBindData->ImportSourceMovies.GetSize())
                {
                    // We may not want to allow this. The only time this could happen is
                    // if file is still being bound by another thread that has not caught
                    // up to this point, or binding error has occurred.
                    GASSERT(0);
                    break;
                }

                pindexedMovie = pBindData->ImportSourceMovies[pimportData->ImportIndex];
            }
          
            if (visited.FindCaseInsensitive(pimportData->SourceUrl) == visited.End())
            {                           
                // Call back the visitor.
                if (pindexedMovie)
                    visitor->Visit(this, pindexedMovie,
                                         pimportData->SourceUrl.ToCStr());
                visited.SetCaseInsensitive(pimportData->SourceUrl, true);
            }

            pimportData = pimportData->pNext;
        }
    }
}


void GFxMovieDefImpl::VisitResources(ResourceVisitor* pvisitor, UInt visitMask)
{
    
    if (visitMask & (GFxMovieDef::ResVisit_AllImages |
                     GFxMovieDef::ResVisit_Fonts | 
                     GFxMovieDef::ResVisit_EditTextFields | 
                     GFxMovieDef::ResVisit_Sounds | 
                     GFxMovieDef::ResVisit_Sprite ) )
    {
        // Lock DataDef's Resources and Exports.
        GFxMovieDataDef::LoadTaskData::ResourceLocker lock( GetDataDef()->pData );

        GFxMovieDataDef::ResourceHash& resources            = GetDataDef()->pData->Resources;
        GFxMovieDataDef::ResourceHash::ConstIterator  ihash = resources.Begin();

        for(; ihash != resources.End(); ++ihash)
        {
            GFxResource *pres          = ihash->Second.GetResource(&pBindData->ResourceBinding);
            bool         resourceMatch = 0;

            if (pres)
            {
                GFxResource::ResourceUse use = pres->GetResourceUse();

                switch(pres->GetResourceType())
                {
                case GFxResource::RT_Image:
                    {                       
                        if (use == GFxResource::Use_Bitmap)
                        {
                            if (visitMask & GFxMovieDef::ResVisit_Bitmaps)
                                resourceMatch = 1;
                        }
                        else if (use == GFxResource::Use_Gradient)
                        {
                            if (visitMask & GFxMovieDef::ResVisit_GradientImages)
                                resourceMatch = 1;
                        }
                    }                    
                    break;

                case GFxResource::RT_Font:
                    if (visitMask & GFxMovieDef::ResVisit_Fonts)
                        resourceMatch = 1;
                    break;

                case GFxResource::RT_EditTextDef:
                    if (visitMask & GFxMovieDef::ResVisit_EditTextFields)
                        resourceMatch = 1;
                    break;

                case GFxResource::RT_SoundSample:
                    if (visitMask & GFxMovieDef::ResVisit_Sounds)
                        resourceMatch = 1;
                    break;

                case GFxResource::RT_SpriteDef:
                    if (visitMask & GFxMovieDef::ResVisit_Sprite)
                        resourceMatch = 1;
                    break;

                default: break;
                }
            }

            if (resourceMatch)
            {
                // Determine export name by doing a search.
                const char* pexportName = 0;

                GFxStringHashLH<GFxResourceHandle> &exports = GetDataDef()->pData->Exports;
                GFxStringHashLH<GFxResourceHandle>::Iterator iexport = exports.Begin();

                while (iexport != exports.End())
                {
                    if (iexport->Second.Equals(ihash->Second))
                    {
                        pexportName = iexport->First.ToCStr();
                        break;
                    }
                    ++iexport;
                }                            

                pvisitor->Visit(this, pres, ihash->First, pexportName);
            }
        }
    }

    // Check to see if we need to traverse nested files.
    if (visitMask & GFxMovieDef::ResVisit_NestedMovies)
    {
        // visit imported movies
        GArray<GPtr<GFxMovieDefImpl>, GFxStatMD_Other_Mem> importsCopy;

        { // Access ImportSourceMovies in a lock.
            GLock::Locker loc(&pBindData->ImportSourceLock);
            
            //importsCopy = pBindData->ImportSourceMovies;
            importsCopy.Reserve(pBindData->ImportSourceMovies.GetSize());
            for (UPInt i = 0; i< pBindData->ImportSourceMovies.GetSize(); i++)
                importsCopy.PushBack(pBindData->ImportSourceMovies[i]);
        }

        for(UPInt i = 0, n = importsCopy.GetSize(); i < n; ++i)
        {
            if (importsCopy[i])
                importsCopy[i]->VisitResources(pvisitor, visitMask);
        }
    }
}


GFxResource*     GFxMovieDefImpl::GetResource(const char *pexportName) const
{
    if (!pexportName)
        return 0;

    // Find the export string
    GString str(pexportName);
    
    GFxMovieDataDef::LoadTaskData::ResourceLocker lock( GetDataDef()->pData );

    GFxStringHashLH<GFxResourceHandle>& exports = GetDataDef()->pData->Exports;
    GFxStringHashLH<GFxResourceHandle>::ConstIterator iexport = exports.FindCaseInsensitive(str);
    if ( iexport == exports.End())
        return 0;

    // TBD: Thread issues?
    // Check if the resource has been resolved yet. Not quite right since
    // pointer-based handles are possible.
    //if (iexport->Second.GetBindIndex() >= ResourceBinding.GetResourceCount())    
    //    return 0;

    GFxResource *pres = iexport->Second.GetResource(&pBindData->ResourceBinding);
    return pres;    
}


// Locate a font resource by name and style.
// It's ok to return GFxFontResource* without the binding because pBinding
// is embedded into font resource allowing imports to be handled properly.
GFxFontResource*    GFxMovieDefImpl::GetFontResource(const char* pfontName, UInt styleFlags, SearchInfo* psearchInfo)
{
    GFxMovieDataDef* pdataDef = GetDataDef();

    // TBD: We will need to do something about threading here, since it is legit
    // to call GetFontResource while loading still hasn't completed.

    GFxFontDataUseNode *pfont = pdataDef->GetFirstFont();

    for(; pfont != 0; pfont = pfont->pNext)
    {
        if (pfont->pFontData->MatchFont(pfontName, styleFlags))
        {
            // TBD: Wait for resource binding?
            GFxResourceBindData rbd;
            pBindData->ResourceBinding.GetResourceData(&rbd, pfont->BindIndex);
            GFC_DEBUG_WARNING1(!rbd.pResource, "rbd.pResource = 0 for font '%s' ", pfontName);
            if (rbd.pResource)
            {
                GASSERT(rbd.pResource->GetResourceType() == GFxResource::RT_Font);
                GFxFontResource* pfont = (GFxFontResource*)rbd.pResource.GetPtr();
                if (psearchInfo)
                {
                    if (!pfont->GetFont()->IsResolved())
                        psearchInfo->Status = SearchInfo::FoundInResourcesNoGlyphs;
                    else if ((styleFlags & GFxFont::FF_BoldItalic) && !pfont->GetFont()->HasVectorOrRasterGlyphs())
                        psearchInfo->Status = SearchInfo::FoundInResourcesNeedFaux;
                    else
                        psearchInfo->Status = SearchInfo::FoundInResources;
                }
                return pfont;
            }
        }
    }
    
    // Import names also look like font names for lookup purposes.
    // In particular, we need to do this to support imported shared font,
    // where export names are used during lookup.
    // We should look not only for import names but also for real font names. 
    // Imported font should be locatable either by import name (like $TitleFont)
    // or by the real font name (like Times New Roman).
    GFxImportData* pimport = pdataDef->GetFirstImport();

    for(; pimport != 0; pimport = pimport->pNext)
    {
        for (UInt j = 0; j<pimport->Imports.GetSize(); j++)
        {
            // Locate the font and match style flags.
            // We will also have to report the new name so that it can be
            // detected in the font manager.
            if (psearchInfo)
                psearchInfo->ImportSearchUrls.Set(pimport->SourceUrl);

            GFxResourceHandle rh;
            if (pdataDef->GetResourceHandle(&rh, GFxResourceId(pimport->Imports[j].CharacterId)))
            {
                GFxResource* pres = rh.GetResource(&pBindData->ResourceBinding);
                // The resource must be a font.
                if (pres && (pres->GetResourceType() == GFxResource::RT_Font))
                {
                    GFxFontResource* pfontRes = (GFxFontResource*)pres;
                    if (pfontRes->GetFont()->MatchFontFlags(styleFlags))
                    {
                        // Compare the import font name and the real font name.
                        if (!pimport->Imports[j].SymbolName.CompareNoCase(pfontName) ||
                            !GString::CompareNoCase(pfontRes->GetName(), pfontName))
                        {
                            // We found it!
                            // Fill in name, since font will not have its export name.
                            if (psearchInfo)
                            {
                                static const char fontlibKey[]   = GFX_FONTLIB_IMPORT_KEY;
                                const UPInt       fontlibKeySize = sizeof(fontlibKey) - 1;

                                if ((pimport->SourceUrl.GetSize() >= fontlibKeySize) &&
                                    (GString::CompareNoCase(pimport->SourceUrl.ToCStr() +
                                    (pimport->SourceUrl.GetSize() - fontlibKeySize), fontlibKey) == 0))
                                    psearchInfo->Status = SearchInfo::FoundInImportsFontLib;
                                else
                                    psearchInfo->Status = SearchInfo::FoundInImports;
                                psearchInfo->ImportFoundUrl = pimport->SourceUrl;
                            }
                            return pfontRes;
                        }
                    }
                }
            }
        }
    }

    // Look for the names of EXPORTED fonts too (!AB)
    GFxResource* pres = GetResource(pfontName);
    if (pres && (pres->GetResourceType() == GFxResource::RT_Font))
    {
        GFxFontResource* pfontRes = (GFxFontResource*)pres;
        if (pfontRes->GetFont()->MatchFontFlags(styleFlags))
        {
            // We found it!
            // Fill in name, since font will not have its export name.
            if (psearchInfo)
                psearchInfo->Status = SearchInfo::FoundInExports;
            return pfontRes;
        }
    }
    if (psearchInfo)
        psearchInfo->Status = SearchInfo::NotFound;
    return 0;   
}


// *** GASExecuteTag - debug counters

#ifdef GFC_BUILD_DEBUG

class ASClassCounter
{
public:
    GAtomicInt<UInt> Count;

    ASClassCounter()  { Count = 0; }
    // If this assertion is hit, it means that some allocated
    // GASExecuteTag(s) weren't properly cleaned up.
    ~ASClassCounter() {}//{ GASSERT(Count == 0); }
};

ASClassCounter AS_Counter_Instance;

GASExecuteTag::GASExecuteTag()
{
    AS_Counter_Instance.Count++;
    //Sentinel = 0xE465F1A9;
}
GASExecuteTag::~GASExecuteTag()
{
    //GASSERT(Sentinel == 0xE465F1A9);
    //Sentinel = 0;
    AS_Counter_Instance.Count--;
}

#endif



// ***** GFxInitImportActions - created for import tags.

// Queues up logic to execute InitClip actions from the import,
// using Highest priority by default.
void    GFxInitImportActions::Execute(GFxSprite* m)
{
    // For regular execute our context is the same as of target sprite,
    // but it gets overriden if called from GFxSprite::ExecuteImportedInitActions.
    ExecuteInContext(m, m->GetResourceMovieDef(), 0);
}

// InitImportActions that come from imported files need to be executed
// in the MovieDefImpl binding context (otherwise we would index parent's
// source movies incorrectly, resulting in recursive loop).
void    GFxInitImportActions::ExecuteInContext(GFxSprite* m, GFxMovieDefImpl *pbindDef, bool recursiveCheck)
{
    GFxMovieDefImpl* psourceImportMovie = 
        pbindDef->pBindData->GetImportSourceMovie(ImportIndex);
    
    if (psourceImportMovie &&
        psourceImportMovie->GetDataDef()->HasInitActions())
    {
        // It is possible that SWF files will import themselves or other files recursively.
        // Check for that and don't run sprite's ResourceMovieDef's actions again.
        if (recursiveCheck && (psourceImportMovie == m->GetResourceMovieDef()))
            return;

        m->ExecuteImportedInitActions(psourceImportMovie);
    }    
}

