/**********************************************************************

Filename    :   GFxLoadProcess.h
Content     :   GFxLoadProcess - tracks loading and binding state.
Created     :   
Authors     :   Michael Antonov

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   This file contains class declarations used in
                GFxPlayerImpl.cpp only. Declarations that need to be
                visible by other player files should be placed
                in GFxCharacter.h.


Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXLOADPROCESS_H
#define INC_GFXLOADPROCESS_H

#include "GArray.h"

#include "GFxStream.h"
#include "GFxLoaderImpl.h"

//#include "GFxTaskManager.h"

// LoadProcess relies no many MovieDef function and APIs.
#include "GFxMovieDef.h"


// ***** Declared Classes

class GFxLoadProcess;

// ***** External Classes

class GFxLoader;
class GFxFontPackParams;
class GFxMovieRoot;
class GFxLoadQueueEntry;

class GJPEGInput;
#ifndef GFC_NO_SOUND
class GSoundData;
class GFxSoundStreamDef;
struct GFxSoundFileInfo;
#endif

// Not accessed by binding thread until FinishBinding (when loading is complete)
// GFxLoadProcess and GFxBindProcess both have a reference. Bind process releases it.

struct GFxTempBindData : public GNewOverrideBase<GStat_Default_Mem>
{
    GHashSetLH<UInt>       FillStyleImageWrap;
};

// ***** Loading Process

class GFxLoadProcess : public GFxLoaderTask, public GFxLogBase<GFxLoadProcess>
{
public:
    typedef GFxMovieDataDef::LoadTaskData LoadTaskData;

private:
    // Load states used during load processing.
    //GPtr<GFxLoadStates>         pStates;
    // Cached GFxParseControl flags from the states for more efficient checks.
    UInt                        ParseFlags;
    
    // Optional binding process. This pointer will be assigned if
    // we are doing frame-by-frame interleaved load/bind on one thread.    
    GPtr<GFxMovieBindProcess>   pBindProcess;


    // We keep a smart pointer to GFxMovieDataDef::LoadTaskData and not
    // the GFxMovieDataDef itself; this allows us to cancel loading
    // gracefully if user's GFxMovieDataDef is released.
    GPtr<LoadTaskData>          pLoadData;

    // A pointer to DataDef used to pass to GFxSpriteDef constructor.
    // This pointer is usuable from the sprite instance point of view,
    // but it can go bad if pMovieDef is released in playback thread.
    // Therefore, we can't really use it during loading.
    GFxMovieDataDef*            pDataDef_Unsafe;


    // *** Temporary load process state

    // NOTE: It is important ProcessInfo is placed AFTER pLoadData, as it must
    // be destroyed earlier due to do contained GFxStream movie heap references.
    GFxSWFProcessInfo       ProcessInfo;

    GJPEGInput*             pJpegIn;
    // Load flags, from GfxLoader::LoadConstants
    UInt                    LoadFlags;

    // Running total index of import in file, used to
    // initialize ImportIndex in ImportData objects
    UInt                    ImportIndex;

    // Shared with binding
    GFxTempBindData*        pTempBindData;

    
    // *** Frame Bind Data content allocated

    // This data is used to initialize GFxFrameBindData object for
    // each frame, committed at the end of each root frame.   
    UInt                    ImportDataCount;
    UInt                    ResourceDataCount;
    UInt                    FontDataCount;

    // Singly linked lists of imports for this frame.   
    GFxImportData*          pImportDataLast;
    GFxImportData*          pImportData;
    // A linked list of resource data that will be used for binding.
    GFxResourceDataNode*    pResourceData;
    GFxResourceDataNode*    pResourceDataLast;
    // Fonts referenced in this frame.
    GFxFontDataUseNode*     pFontData;
    GFxFontDataUseNode*     pFontDataLast;


    // *** Sprite/Root time-line loading stack

    enum LoadStateType
    {
        LS_LoadingRoot      = 0,
        LS_LoadingSprite    = 1,
        LS_LoadState_Count 
    };

    LoadStateType           LoadState;

    // Sprite definition into which data is being loaded. This will either
    // be GFxSpriteDef for loading a sprite or null if we should load into pLoadData.
    GFxTimelineIODef*           pTimelineDef;

    typedef GArrayConstPolicy<32, 16, false> SizePolicyType; // MinCapacity=32, Granularity=16, Shrink
    typedef GArray<GASExecuteTag*, GStat_Default_Mem, SizePolicyType> ExecuteTagArrayType;

    // Array of loaded frame pointers. Once the frame is done, this 
    // array is re-allocated as tag data and saved in GFxMovieDataDef.
    // Note that tags themselves also come from Def's allocator.    
    ExecuteTagArrayType     FrameTags[LS_LoadState_Count];
    ExecuteTagArrayType     InitActionTags;

    GFxStream*              pAltStream;
public:

    GFxLoadProcess(GFxMovieDataDef* pdataDef, GFxLoadStates *pstates, UInt loadFlags);    
    ~GFxLoadProcess();

    // Initializes SWF/GFX header for loading; returns 0 if there was
    // an error with loading header. Messages are properly logged.
    bool                BeginSWFLoading(GFile *pfile);

    // Assign a bind process, if we are to do simultaneous loading and binding.
    void                SetBindProcess(GFxMovieBindProcess *pbindProcess)
    {
        pBindProcess = pbindProcess;
    }
    

    // Save/access an input object used for later loading DefineBits
    // Images (JPEG images without the table info).
    void                    SetJpegLoader(GJPEGInput* pjin) { GASSERT(pJpegIn == NULL); pJpegIn = pjin; }
    GJPEGInput*             GetJpegLoader() const           { return pJpegIn; }


    // Return pDataDef pointer; can turn invalid if GFxMovieDef is relased asynchronously.
    GFxMovieDataDef*        GetDataDef_Unsafe() const   { return pDataDef_Unsafe; }
    const GFxSWFProcessInfo& GetProcessInfo() const     { return ProcessInfo; }
    LoadTaskData*           GetLoadTaskData() const     { return pLoadData; }

    const char*             GetFileURL() const          { return pLoadData->GetFileURL(); }
    UInt                    GetVersion() const          { return pLoadData->GetVersion(); }
    UInt                    GetLoadingFrame() const     { return pLoadData->LoadingFrame; }
    UInt                    GetTotalFrames() const      { return pLoadData->GetTotalFrames(); }
    const GFxExporterInfo*  GetExporterInfo() const     { return pLoadData->GetExporterInfo(); }
    
    GFxResourceWeakLib*     GetWeakLib() const          { return pLoadStates->pWeakResourceLib; }
    GFxLoadStates*          GetLoadStates() const       { return pLoadStates; }
    GFxMovieDefBindStates*  GetBindStates() const       { return pLoadStates->GetBindStates(); }


    // *** GFxStream Support

    inline  GFxStream* GetStream()          { return !pAltStream ? &ProcessInfo.Stream : pAltStream; }

    // Sets alternative stream. Use with care, since pAltStream is not refcounted
    // and GFxLoadProcess class does not own it.
    inline  void       SetAltStream(GFxStream* ns) { pAltStream = ns; }
    inline  bool       HasAltStream() const { return pAltStream != NULL; }

    // Stream inlines
    inline UInt8    ReadU8()                { return GetStream()->ReadU8(); }
    inline UInt16   ReadU16()               { return GetStream()->ReadU16(); }
    inline UInt32   ReadU32()               { return GetStream()->ReadU32(); }
    inline SInt8    ReadS8()                { return GetStream()->ReadS8(); }
    inline SInt16   ReadS16()               { return GetStream()->ReadS16(); }
    inline SInt32   ReadS32()               { return GetStream()->ReadS32(); }

    inline void     AlignStream()           { return GetStream()->Align(); }

    inline GFile*   GetUnderlyingFile ()    { return GetStream()->GetUnderlyingFile();  }
    inline int      GetTagEndPosition()     { return GetStream()->GetTagEndPosition(); }
    inline int      Tell()                  { return GetStream()->Tell();  }
    inline void     SetPosition(int pos)    { GetStream()->SetPosition(pos);  }

    void            ReadRgbaTag(GColor *pc, GFxTagType tagType);


    // *** Delegated Logging Support 

    // GFxLogBase will output log messages to the appropriate logging stream,
    // but only if the enable option in context is set. 
    GFxLog*         GetLog() const              { return pLoadStates->pLog; }

    // These work of captured parse control flags for efficiency. This is ok because
    // user updates that take place in the middle of loading are not guaranteed to take effect.
    inline bool     IsVerboseParse() const              { return (ParseFlags & GFxParseControl::VerboseParse) != 0; }
    inline bool     IsVerboseParseShape() const         { return (ParseFlags & GFxParseControl::VerboseParseShape) != 0; }
    inline bool     IsVerboseParseMorphShape() const    { return (ParseFlags & GFxParseControl::VerboseParseMorphShape) != 0; }
    inline bool     IsVerboseParseAction() const        { return (ParseFlags & GFxParseControl::VerboseParseAction) != 0; }


    inline bool     IsLoadingImageData() const          { return 1; }
    inline bool     IsLoadingFontShapes() const         { return 1; }


    // *** Resource Loading / Access

    // There function are used to add resource data to LoadTaskData during loading,
    // or query resources loaded so far. Most of I/O data is assigned through here,
    // GFxMovieDef only implements accessor methods.    

    GMemoryHeap*        GetLoadHeap() const             { return pLoadData->GetHeap(); }

    // If we are keeping image data, we load it into the MovieData's image heap; if not,
    // that means it is a temporary so we place it into the global instead.
    GMemoryHeap*        GetLoadImageHeap() const
    {
        // The LoadTimeImageCreator is used only if we are NOT keeping the image data.
        // This may be counter-intuitive but is logical:
        //   - If we are NOT keeping the image data, we would like to create a texture
        //     out of it ASAP, so that the temporary image is discarded. Since LoadTimeImageCreator
        //     is responsible for creating GImageInfo and GTexture from renderer, it is used
        //     during loading in that case.
        //   - If we ARE keeping the image data, we don't need a texture immediately since
        //     we can create it later on anyway. Thus, we'll have no image creator. But we will
        //     want to put such images into a dedicated heap for efficient storage and unloading
        //     (avoid fragmenting/growing the global heap).
#ifdef GFX_USE_IMAGE_HEAPS
        return (pLoadStates->GetLoadTimeImageCreator(LoadFlags)) ?
            GMemory::GetGlobalHeap() : pLoadData->GetImageHeap();
#else
        return GMemory::GetGlobalHeap();
#endif
    }    


    void                SetMetadata(UByte *pdata, UInt size) { pLoadData->SetMetadata(pdata, size); }
    void                SetFileAttributes(UInt attrs)   { pLoadData->SetFileAttributes(attrs); }

    // Allocate MovieData local memory.
    inline void*        AllocTagMemory(UPInt bytes)     { return pLoadData->AllocTagMemory(bytes);  }
    // Allocate a tag directly through method above.
    template<class T>
    inline T*           AllocTag()                      { return G_Construct<T>(AllocTagMemory(sizeof(T))); }
    template<class T>
    inline T*           AllocTag(UPInt len)
    {
        return G_Construct<T>(AllocTagMemory(sizeof(T) + len - 1));
    }
    template<class T>
    inline T*           AllocMovieDefClass()            { return G_Construct<T>(AllocTagMemory(sizeof(T))); }

    GFxResourceId       GetNextGradientId()             { return pLoadData->GetNextGradientId(); }

#ifdef GFX_AMP_SERVER
    void                SetSwdHandle(UInt32 iSwdHandle)        { pLoadData->SetSwdHandle(iSwdHandle); }
#endif  // GFX_AMP_SERVER

    GFxResourceHandle   AddNewResourceHandle(GFxResourceId rid)
    {
        return pLoadData->AddNewResourceHandle(rid);
    }
    // Add a resource during loading.
    inline void         AddResource(GFxResourceId rid, GFxResource* pres)
    {
        GASSERT(LoadState == LS_LoadingRoot);
        if (LoadState == LS_LoadingRoot)
            pLoadData->AddResource(rid, pres);
    }    
    inline void         AddCharacter(GFxResourceId rid, GFxCharacterDef* pdef)
    {
        return pLoadData->AddCharacter(rid, pdef);
    }

    // Adds a new resource and generates a handle for it.
    GFxResourceHandle   AddDataResource(GFxResourceId rid, const GFxResourceData &resData);
    GFxResourceHandle   AddFontDataResource(GFxResourceId rid, GFxFont* pfontData);
    // Add a dynamically-loaded image resource, with unique key.
    // This is normally used for SWF embedded images.
    // Based on image creator flags, either the resource itself or
    // its resource data will be added.
    void                AddImageResource(GFxResourceId rid, GImage *pimage);

    void                AddImportData(GFxImportData* pimportData);

    void                ExportResource(const GString& symbol, GFxResourceId rid, const GFxResourceHandle &hres)
    {
        pLoadData->ExportResource(symbol, rid, hres);
    }

    inline bool         GetResourceHandle(GFxResourceHandle* phandle, GFxResourceId rid) const
    {
        return pLoadData->GetResourceHandle(phandle, rid);
    }

    inline GFxFont*     GetFontData(GFxResourceId rid) { return pLoadData->GetFontData(rid); }
    
#ifndef GFC_NO_SOUND

    void SetSoundStream( GFxSoundStreamDef* psoundDef)
    {
        if (LoadState == LS_LoadingSprite)
        {
            GASSERT(pTimelineDef);
            pTimelineDef->SetSoundStream(psoundDef);
        }
        else
            pLoadData->SetSoundStream(psoundDef);
    }
    GFxSoundStreamDef* GetSoundStream()
    {
        if (LoadState == LS_LoadingSprite)
        {
            GASSERT(pTimelineDef);
            return pTimelineDef->GetSoundStream();
        }
        else
            return pLoadData->GetSoundStream();
    }

    UInt GetLoadingFrameForStreamSound()
    {
        if (LoadState == LS_LoadingSprite)
        {
            GASSERT(pTimelineDef);
            return pTimelineDef->GetLoadingFrame();
        }
        return GetLoadingFrame();
    }

    UInt GetFrameCountForStreamSound()
    {
        if (LoadState == LS_LoadingSprite)
        {
            GASSERT(pTimelineDef);
            return pTimelineDef->GetFrameCount();
        }
        return GetProcessInfo().Header.FrameCount;
    }
#endif // GFC_NO_SOUND

    // *** Frame Loading

    // Creates a frame binding object; should be called only when
    // a root frame is finished. Clears the internal import/font/resource lists.
    GFxFrameBindData* CreateFrameBindData();


    // Enter and leave Sprite. 
    // Only one level of nesting is possible in SWF files.
    void                EnterSpriteDef(GFxTimelineIODef *psprite)
    {
        GASSERT(LoadState == LS_LoadingRoot);
        GASSERT(psprite != 0);
        LoadState    = LS_LoadingSprite;
        pTimelineDef = psprite;
    }

    void                LeaveSpriteDef()
    {
        GASSERT(LoadState == LS_LoadingSprite);
        LoadState    = LS_LoadingRoot;
        pTimelineDef = 0;
    }


    // Get allocator used for path shape storage.
    GFxPathAllocator*   GetPathAllocator() const        { return pLoadData->GetPathAllocator(); }

    // Labels the frame currently being loaded with the given name.
    // A copy of the name string is made and kept in this object.    
    inline void     AddFrameName(const GString& name, GFxLog *plog)
    {
        if (LoadState == LS_LoadingSprite)
            pTimelineDef->AddFrameName(name, plog);
        else
            pLoadData->AddFrameName(name, plog);
    }

#ifdef GFC_DEBUG_COUNT_TAGS
    GString             FilePath;
    struct TagCountType
    {
        const char*     Name;
        UInt            Count;
        UInt            ClassSize;
        TagCountType(const char* name, UInt csz) : Name(name), Count(0), ClassSize(csz) {}
    };
    GHashIdentity<GFxTagType, TagCountType>     TagCounts;

    inline void     CountTag(GFxTagType tagType)
    {
        TagCountType* tagCount = TagCounts.Get(tagType);
        if (tagCount)
            tagCount->Count++;
    }
#endif

    inline void     AddExecuteTag(GASExecuteTag* ptag)
    {
        FrameTags[LoadState].PushBack(ptag);
    }
    // Need to execute the given tag before entering the
    // currently-loading frame for the first time.    
    inline void     AddInitAction(GFxResourceId spriteId, GASExecuteTag* ptag)
    {
        GUNUSED(spriteId);
        GASSERT(LoadState == LS_LoadingRoot);
        InitActionTags.PushBack(ptag);
    }
    
    // Helper method used to convert accumulated tags into a frame.
    GFxTimelineDef::Frame TagArrayToFrame(ExecuteTagArrayType &tagArray);

    // Apply frame tags that have been accumulated to the MovieDef/SpriteDef.
    void            CommitFrameTags();
    // Cleans up frame tags; only used if loading was canceled.
    void            CleanupFrameTags();
    

    inline bool     FrameTagsAvailable() const
    {
        return (FrameTags[LoadState].GetSize() > 0) ||
               ((LoadState == LS_LoadingRoot) && (InitActionTags.GetSize() > 0));
    }

    void                    SetTempBindData(GFxTempBindData* ptempdata)     { pTempBindData = ptempdata; }
    GFxTempBindData*        GetTempBindData() const                         { return pTempBindData; }


    void ReportProgress(const GString& fileURL, const GFxTagInfo& tagInfo, bool calledFromDefSprite)
    { 
        if (pLoadStates->pProgressHandler)
            pLoadStates->pProgressHandler->LoadTagUpdate(
                                              GFxProgressHandler::TagInfo(fileURL,tagInfo.TagType,tagInfo.TagOffset,
                                                                          tagInfo.TagLength, tagInfo.TagDataOffset),
                                              calledFromDefSprite); 
    }    // *** GFxTask implementation

    virtual void    Execute()
    {   
        //GFxMovieDefImpl::BindTaskData* pbd = NULL;
        //if (pBindProcess)
        //    pbd = pBindProcess->pBindData.GetPtr();
        //printf("GFxLoadProcess::Execute: this: %x, BindData: %x, thread: %d --->>\n",this,pbd,GetCurrentThreadId());
        // Do the loading.
        pLoadData->Read(this, pBindProcess);
        //printf("GFxLoadProcess::Execute: this: %x, BindData: %x, thread: %d ---<<\n",this,pbd,GetCurrentThreadId());
    }

    virtual void    OnAbandon(bool started)
    {        
        //GFxMovieDefImpl::BindTaskData* pbd = NULL;
        //if (pBindProcess)
        //    pbd = pBindProcess->pBindData.GetPtr();
        //printf("GFxLoadProcess::OnAbandon: this: %x, BindData: %x, thread: %d, started: %d\n",this,pbd,GetCurrentThreadId(),started);

        if (started)
            pLoadData->OnMovieDataDefRelease();
        if (pBindProcess && !started)
            pBindProcess->SetBindState(GFxMovieDefImpl::BS_Canceled);
    }

    bool GetNextCodeOffset(int* offset)
    {
        GArray<UInt32>& offsets = ProcessInfo.Header.ExporterInfo.CodeOffsets;

        if (offsets.GetSize() <= ProcessInfo.NextActionBlock)
        {
            return false;
        }
        *offset = offsets[ProcessInfo.NextActionBlock++];
        return true;
    }
};



// ***** Image creator from files

class GFxImageFileResourceCreator : public GFxResourceData::DataInterface
{
    typedef GFxResourceData::DataHandle DataHandle;

    // Creates/Loads resource based on data and loading process
    virtual bool    CreateResource(DataHandle hdata, GFxResourceBindData *pbindData,
                                   GFxLoadStates *plp, GMemoryHeap* pbindHeap) const;
public:

    static  GFxResourceData CreateImageFileResourceData(GFxImageFileInfo * prfi);    
};

class GFxImageResourceCreator : public GFxResourceData::DataInterface
{
    typedef GFxResourceData::DataHandle DataHandle;

    // Creates/Loads resource based on data and loading process
    virtual bool    CreateResource(DataHandle hdata, GFxResourceBindData *pbindData,
                                   GFxLoadStates *plp, GMemoryHeap* pbindHeap) const;
public:

    static  GFxResourceData CreateImageResourceData(GImage *pimage);    
};

class GFxSubImageResourceCreator : public GFxResourceData::DataInterface
{
    typedef GFxResourceData::DataHandle DataHandle;

    // Creates/Loads resource based on data and loading process
    virtual bool    CreateResource(DataHandle hdata, GFxResourceBindData *pbindData,
        GFxLoadStates *plp, GMemoryHeap* pbindHeap) const;
public:

    static  GFxResourceData CreateSubImageResourceData(GFxSubImageResourceInfo* pinfo);    
};

// ***** Font resource creators from embedded font data

class GFxFontResourceCreator : public GFxResourceData::DataInterface
{
    typedef GFxResourceData::DataHandle DataHandle;

    // Creates/Loads resource based on data and loading process
    virtual bool    CreateResource(DataHandle hdata, GFxResourceBindData *pbindData,
                                   GFxLoadStates *plp, GMemoryHeap* pbindHeap) const;
};

#endif // INC_GFXLOADPROCESS_H
