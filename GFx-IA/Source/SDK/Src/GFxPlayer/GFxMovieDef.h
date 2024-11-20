/**********************************************************************

Filename    :   GFxMovieDef.h
Content     :   GFxMovieDataDef and GFxMovieDefImpl classes used to
                represent loaded and bound movie data, respectively.
Created     :   
Authors     :   Michael Antonov

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   Starting with GFx 2.0, loaded and bound data is stored
                separately in GFxMovieDataDef and GFxMovieDefImpl
                classes. GFxMovieDataDef represents data loaded from
                a file, shared by all bindings. GFxMovieDefImpl binds
                resources referenced by the loaded data, by providing
                a custom ResourceBinding table. Bindings can vary
                depending on GFxState objects specified during loading.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXMOVIEDEF_H
#define INC_GFXMOVIEDEF_H

#include "GArray.h"

#include "GFxCharacterDef.h"
#include "GFxResourceHandle.h"
#include "GFxLoaderImpl.h"
#include "GFxFontResource.h"
#include "GFxFontLib.h"
#include "GFxStringHash.h"
#include "GFxActionTypes.h"
#include "GFxFilterDesc.h"
#include "GFxVideoBase.h"
#include "GFxAudio.h"

// For GFxTask base.
#include "GFxTaskManager.h"

#include "GHeapNew.h"


#ifdef GFC_BUILD_DEBUG
//#define GFC_DEBUG_COUNT_TAGS
#endif

// ***** Declared Classes

class GFxMovieDataDef;
class GFxMovieDefImpl;
class GFxSpriteDef;
class GFxSprite;
// Helpers
class GFxImportInfo;
class GFxSwfEvent;

// Tag classes
class GFxPlaceObject2;
class GFxRemoveObject2;
class GFxSetBackgroundColor;
// class GASDoAction;           - in GFxAction.cpp
// class GFxStartSoundTag;      - in GFxSound.cpp


class GFxMovieBindProcess;

// ***** External Classes

class GFxLoadStates;
class GFxTextAllocator;
class GFxFontPackParams;
class GFxImagePackParamsBase;
class GFxImagePacker;

class GFxMovieRoot;
class GFxLoadQueueEntry;

class GJPEGInput;
class GFxSoundStreamDef;

#ifndef GFC_NO_SOUND
class GFxSoundStreamDef;
#endif

struct GFxTempBindData;

// ***** Movie classes

    
// Helper for GFxMovieDefImpl
class GFxImportData
{  
public:

    struct Symbol
    {
        GString     SymbolName;
        int         CharacterId;
        UInt        BindIndex;
    };

    GArrayLH<Symbol>    Imports;
    GStringLH           SourceUrl;
    UInt                Frame;
    
    // Index of this import in the file, used to index
    // us within GFxMovieDefImpl::ImportSourceMovies array.
    UInt            ImportIndex;

    // Pointer to next import node.
    GAtomicPtr<GFxImportData>  pNext;

    GFxImportData()
        : Frame(0), ImportIndex(0)
    { }

    GFxImportData(const char* source, UInt frame)
        : SourceUrl(source), Frame(frame), ImportIndex(0)
    { }

    void    AddSymbol(const char* psymbolName, int characterId, UInt bindIndex)
    {
        Symbol s;
        s.SymbolName  = psymbolName;
        s.CharacterId = characterId;
        s.BindIndex   = bindIndex;
        Imports.PushBack(s);
    }

};

struct GFxResourceDataNode
{
    // Resource binding data used here.
    GFxResourceData      Data;
    UInt                 BindIndex;
    GAtomicPtr<GFxResourceDataNode> pNext;    

    GFxResourceDataNode()        
    {  }
};

// Font data reference. We need to keep these in case we need to substitute fonts.
// We can scan through this list to figure out which fonts to substitute.
struct GFxFontDataUseNode
{
    GFxResourceId       Id;
    // pointer is technically redundant
    GPtr<GFxFont>   pFontData;          
    // This font data is used at specified font index.
    UInt                BindIndex;

    GAtomicPtr<GFxFontDataUseNode> pNext;

    GFxFontDataUseNode()
        : BindIndex(0)
    { }
};


// Frame bind data consists of 
// - imports, fonts, resource data for creation

struct GFxFrameBindData
{
    // Frame that this structure represents. Stroring this field
    // allows us to skip frames that don't require binding.
    UInt                Frame;
    // Number of bytes loaded by this frame.
    UInt                BytesLoaded;

    // Number of Imports, Resources, and Fonts for this frame.
    // We use these cumbers instead of traversing the entire linked
    // lists, which can include elements from other frames.
    UInt                FontCount;
    UInt                ImportCount;
    UInt                ResourceCount;
   
    // Singly linked lists of imports for this frame.   
    GFxImportData*      pImportData;
    // Fonts referenced in this frame.
    GFxFontDataUseNode* pFontData;

    // A list of ResourceData objects associated with each bindIndex in GFxResourceHandle.
    // Except for imports, which will have an invalid data object, appropriate resources can
    // be created by calling GFxResourceData::CreateResource
    GFxResourceDataNode* pResourceData;
       
    // Pointer to next binding frame. It will be 0 if there are
    // either no more frames of they haven't been loaded yet (this
    // pointer is updated asynchronously while FrameUpdateMutex
    // is locked).    
    GAtomicPtr<GFxFrameBindData> pNextFrame;

    GFxFrameBindData()
        : Frame(0), BytesLoaded(0),
          FontCount(0), ImportCount(0), ResourceCount(0),
          pImportData(0), pFontData(0), pResourceData(0)
    { }
};



// ***** GFxDataAllocator

// GFxDataAllocator is a simple allocator used for tag data in GFxMovieDef.
// Memory is allocated consecutively and can only be freed all at once in a
// destructor. This scheme is used to save on allocation overhead, to reduce
// the number of small allocated chunks and to improve loading performance.

class GFxDataAllocator
{   
    struct Block
    {
        Block*  pNext;
    };

    enum AllocConstants
    {
        // Leave 4 byte space to be efficient with buddy allocator.
        BlockSize = 8192 - sizeof(Block*) - 8
    };

    // Current pointer and number of bytes left there.
    UByte*       pCurrent;
    UPInt        BytesLeft;
    // All allocated blocks, including current.
    Block*       pAllocations;

    GMemoryHeap* pHeap;

    void*   OverflowAlloc(UPInt bytes);

public:

    GFxDataAllocator(GMemoryHeap* pheap);
    ~GFxDataAllocator();

    inline void*   Alloc(UPInt bytes)
    {
        // Round up to pointer size.
        bytes = (bytes + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1);

        if (bytes <= BytesLeft)
        {
            void* pmem = pCurrent;
            pCurrent += bytes;
            BytesLeft -= bytes;
            return pmem;
        }
        return OverflowAlloc(bytes);
    }

    // Allocates an individual chunk of memory, without using the pool.
    // The chunk will be freed later together with the pool.
    void*    AllocIndividual(UPInt bytes);

};


// GFxLoadUpdateSync constrains Mutex and WaitCondition used to notify of loading
// progress when it takes place on a different thread. It is separated into a
// globally allocated object to allow notification as when loading task completes,
// *AFTER* loading task has released all other references to loaded memory. Once
// such full release tooks place, waiting thread can destroy loaded heaps and
// memory arenas.
#ifndef GFC_NO_THREADSUPPORT

class GFxLoadUpdateSync : public GRefCountBase<GFxLoadUpdateSync, GFxStatMD_Other_Mem>
{
    GMutex          Mutex;
    GWaitCondition  WC;
    bool            LoadFinished;

public:
    GFxLoadUpdateSync() : LoadFinished(false)
    {
        GASSERT(GMemory::GetHeapByAddress(this) == GMemory::GetGlobalHeap());
    }

    GMutex&         GetMutex()      { return Mutex; }
    void            UpdateNotify()  { WC.NotifyAll(); }
    void            WaitForNotify() { WC.Wait(&Mutex); }

    // Called by loading thread to notify resource owner that it is done
    // and has released all of its memory pointers to the loaded data
    // (that is besides the pointer to this object).
    void            NotifyLoadFinished()
    {
        GMutex::Locker l(&Mutex);
        LoadFinished = true;
        WC.NotifyAll();
    }

    bool            IsLoadFinished() const { return LoadFinished; }

    void            WaitForLoadFinished()
    {
        GMutex::Locker lock(&Mutex);
        while (!IsLoadFinished())
            WaitForNotify();
    }
};

#endif


// ***** GFxMovieDataDef - Stores file loaded data.

class GFxMovieDataDef : public GFxTimelineDef, public GFxResourceReport
{
public:

    // Store the resource key used to create us.
    // Note: There is a bit of redundancy here with respect to filename;
    // could resolve it with shared strings later on...
    GFxResourceKey          ResourceKey;    

    // MovieDataDefType - Describe the type of data that is stored in this DataDef.    
    enum MovieDataType
    {
        MT_Empty,   // No data, empty clip - useful for LoadVars into new level.
        MT_Flash,   // SWF or GFX file.
        MT_Image    // An image file.
    };

    MovieDataType            MovieType;


    // *** Binding and enumeration lists

    // DefBindingData contains a number of linked lists which are updated during
    // by the LoadProcess and used later on by BindProcess. Linked lists are used 
    // here because they can be updated without disturbing the reader thread; all
    // list data nodes are allocated from tag allocator. Containing DefBindingData
    // to a single object allows it to be easily passed to LoadProcess, which can
    // add items to appropriate lists. If load process fails, these entries are
    // correctly cleaned up by the virtue of being here.
    // 
    // The following points are significant:
    //  - pFrameData / pFrameDataLast writes are protected by the FrameUpdateMutex
    //    and can be waited on through FrameUpdated WaitCondition.
    //  - pImports and pFonts lists are maintained to allow independent traversal
    //    from other non-bind process functions. Parts of these lists are also
    //    referenced from GFxFrameBindData, limited by a count.
    //  - pResourceNodes is kept to allow destructors to be called.
    //  - GAtomicPtr<> is used to have proper Acquire/Release semantics on
    //    variables updated across multiple threads. 'pLast' pointers do not need
    //    sync because they are for loading thread housekeeping only.

    struct DefBindingData
    {
        // A linked list of binding data for each frame that requires it.
        // This may be empty if no frames required binding data.
        GAtomicPtr<GFxFrameBindData>    pFrameData;
        GFxFrameBindData*               pFrameDataLast;

        // A linked list of all import nodes.
        GAtomicPtr<GFxImportData>       pImports;
        GFxImportData*                  pImportsLast;
        // A linked list of all fonts; we keep these in case we need to substitute
        // fonts. Scan through this list to figure out which fonts to substitute.
        GAtomicPtr<GFxFontDataUseNode>  pFonts;
        GFxFontDataUseNode*             pFontsLast;

        // A list of resource data nodes; used for cleanup.
        GAtomicPtr<GFxResourceDataNode> pResourceNodes;
        GFxResourceDataNode*            pResourceNodesLast;

        DefBindingData()
            : pFrameDataLast(0),
            pImportsLast(0), pFontsLast(0), pResourceNodesLast(0)
        { }

        // Clean up data instanced created from tag allocator.
        ~DefBindingData();
    };


    // GFxMovieDataDef starts out in LS_Uninitialized and switches to
    // the LS_LoadingFrames state as the frames start loading. During this
    // process the LoadingFrames counter will be incremented, with playback
    // being possible after it is greater then 0. 
    // For image files, state can be set immediately to LS_LoadFinished
    // while LoadingFrame is set to 1.
    enum MovieLoadState
    {
        LS_Uninitialized,
        LS_LoadingFrames,
        LS_LoadFinished,
        LS_LoadCanceled, // Canceled by user.
        LS_LoadError
    };


    // *** LoadTaskData

    // Most of the GFxMovieDataDef data is contained in LoadTaskData class,
    // which is AddRef-ed separately. Such setup is necessary to allow
    // the loading thread to be canceled when all user reference count
    // for GFxMovieDataDef are released. The loading thread only references
    // the LoadData object and thus can be notified when GFxMovieDataDef dies.

    // Use a base class for LoadTaskData to make sure that TagMemAllocator
    // is destroyed last. That is necessary because LoadTaskData contains
    // multiple references to sprites and objects that use that allocator
    class LoadTaskDataBase : public GRefCountBase<LoadTaskDataBase, GFxStatMD_Other_Mem>
    {
    protected:
        // Tag/Frame Array Memory allocator.
        // Memory is allocated permanently until GFxMovieDataDef/LoadTaskData dies.
        GFxDataAllocator        TagMemAllocator;

        // Path allocator used for shape data. Use a pointer
        // to avoid including GFxShape.h.
        class GFxPathAllocator* pPathAllocator;

        LoadTaskDataBase(GMemoryHeap* pheap)
            : TagMemAllocator(pheap), pPathAllocator(0) { }
    };

    // Resource hash table used in LoadTaskData.
    typedef GHashUncachedLH<
        GFxResourceId, GFxResourceHandle, GFxResourceId::HashOp>  ResourceHash;


    class LoadTaskData : public LoadTaskDataBase
    {
        friend class GFxMovieDataDef;
        friend class GFxLoadProcess;
        friend class GFxMovieDefImpl;
        friend class GFxMovieBindProcess;

    public:
        // ***** Initialization / Cleanup
        LoadTaskData(GFxMovieDataDef* pdataDef, const char *purl,
            GMemoryHeap* pheap);
        ~LoadTaskData();

        // Notifies LoadTaskData that MovieDataDef is being destroyed. This may be a
        // premature destruction if we are in the process of loading (in that case it
        // will lead to loading being canceled).
        void                OnMovieDataDefRelease();

        // Create an empty usable definition with no content.        
        void                InitEmptyMovieDef();
        // Create a definition describing an image file.
        void                InitImageFileMovieDef(UInt fileLength,
            GFxImageResource *pimageResource, bool bilinear = 1);


        // *** Accessors

        GMemoryHeap*        GetHeap() const             { return pHeap; }      
        // Return heap used for image data of this movie; creates the heap if it doesn't exist.
        GMemoryHeap*        GetImageHeap();

        // Get allocator used for path shape storage.
        GFxPathAllocator*   GetPathAllocator()          { return pPathAllocator; }

        const char* GetFileURL() const                  { return FileURL.ToCStr(); }
        UInt                GetTotalFrames() const      { return Header.FrameCount; }
        UInt                GetVersion() const          { return Header.Version; }
        const GFxExporterInfo*  GetExporterInfo() const { return Header.ExporterInfo.GetExporterInfo(); }

#ifndef GFC_NO_THREADSUPPORT
        GFxLoadUpdateSync*  GetFrameUpdateSync() const  { return pFrameUpdate; }
#endif

        // ** Loading

        // Initializes SWF/GFX header for loading.
        void                BeginSWFLoading(const GFxMovieHeaderData &header);

        // Read a .SWF/GFX pMovie. Binding will take place interleaved with
        // loading if passed; no binding is done otherwise.
        void                Read(GFxLoadProcess *plp, GFxMovieBindProcess* pbp = 0);

        // Updates bind data and increments LoadingFrame. 
        // It is expected that it will also release any threads waiting on it in the future.
        // Returns false if there was an error during loading,
        // in which case LoadState is set to LS_LoadError can loading finished.
        bool                FinishLoadingFrame(GFxLoadProcess* plp, bool finished = 0);

        // Update frame and loading state with proper notification, for use image loading, etc.
        void                UpdateLoadState(UInt loadingFrame, MovieLoadState loadState);
        // Signals frame updated; can be used to wake up threads waiting on it
        // if another condition checked externally was met (such as cancel binding).
        void                NotifyFrameUpdated();

        // Waits until loading is completed, used by GFxFontLib.
        void                WaitForLoadFinish();

        // Waits until a specified frame is loaded
        void                WaitForFrame(UInt frame);

        // *** Init / Access IO related data

        UInt                GetMetadata(char *pbuff, UInt buffSize) const;
        void                SetMetadata(UByte *pdata, UInt size); 
        void                SetFileAttributes(UInt attrs)   { FileAttributes = attrs; }

        // Allocate MovieData local memory.
        inline void*        AllocTagMemory(UPInt bytes)     { return TagMemAllocator.Alloc(bytes);  }
        // Allocate a tag directly through method above.
        template<class T>
        inline T*           AllocMovieDefClass()            { return G_Construct<T>(AllocTagMemory(sizeof(T))); }

        GFxResourceId       GetNextGradientId()             { return GradientIdGenerator.GenerateNextId(); }

        // Creates a new resource handle with a binding index for the resourceId; used 
        // to register ResourceData objects that need to be bound later.
        GFxResourceHandle   AddNewResourceHandle(GFxResourceId rid);
        // Add a resource during loading.
        void                AddResource(GFxResourceId rid, GFxResource* pres);   
        // A character is different from other resources because it tracks its creation Id.
        void                AddCharacter(GFxResourceId rid, GFxCharacterDef* c)
        {
            c->SetId(rid);
            AddResource(rid, c);
        }

        // Expose one of our resources under the given symbol, for export.  Other movies can import it.        
        void                ExportResource(const GString& symbol, GFxResourceId rid, const GFxResourceHandle &hres);

        bool                GetResourceHandle(GFxResourceHandle* phandle, GFxResourceId rid) const;

        GFxFont*            GetFontData(GFxResourceId rid);

        // Gets binding data for the first frame, if any.
        GFxFrameBindData*    GetFirstFrameBindData() const   { return BindData.pFrameData; }
        GFxFontDataUseNode* GetFirstFont() const            { return BindData.pFonts; }
        GFxImportData*      GetFirstImport() const          { return BindData.pImports; }


        // Labels the frame currently being loaded with the given name.
        // A copy of the name string is made and kept in this object.    
        virtual void        AddFrameName(const GString& name, GFxLog *plog);
        virtual void        SetLoadingPlaylistFrame(const Frame& frame);
        virtual void        SetLoadingInitActionFrame(const Frame& frame);  

        bool                GetLabeledFrame(const char* label, UInt* frameNumber, bool translateNumbers = 1);

        // *** Playlist access
        const Frame         GetPlaylist(int frameNumber) const;
        bool                GetInitActions(Frame* pframe, int frameNumber) const;
        UInt                GetInitActionListSize() const;
        inline bool         HasInitActions() const   { return InitActionsCnt > 0; }

        GFxSoundStreamDef*  GetSoundStream() const;
        void                SetSoundStream(GFxSoundStreamDef* psoundStream);

        // Locker class used for LoadTaskData::Resources, Exports and InvExports.
        struct ResourceLocker
        {
            const LoadTaskData *pLoadData;

            ResourceLocker(const LoadTaskData* ploadData)
            {
                pLoadData = 0;
                if (ploadData->LoadState < LS_LoadFinished)
                {
                    pLoadData = ploadData;
                    pLoadData->ResourceLock.Lock();
                }            
            }
            ~ResourceLocker()
            {
                if (pLoadData)
                    pLoadData->ResourceLock.Unlock();
            }
        };

        void SetSwdHandle(UInt32 swdHandle) { SwdHandle = swdHandle; }
        UInt32 GetSwdHandle() const { return SwdHandle; }

private:
        // Memory heap used for us and containing parent GFxMovieDataDef.
        // This heap is destroyed on LoadTaskData deallocation.
        GMemoryHeap*            pHeap;

        // This heap is used to allocate images in MovieData; these images will come
        // from this heap if KeepBindData flag has been specified. This heap is NOT
        // created until the first request, since it is optional.
        GPtr<GMemoryHeap>       pImageHeap;


        // *** Header / Info Tag data.

        // General info about an SWF/GFX file

        // File path passed to FileOpenCallback for loading this object, un-escaped.
        // For Flash, this should always include the protocol; however, we do not require that.
        // For convenience of users we also support this being relative to the cwd.
        GStringLH               FileURL;
        // Export header.
        GFxMovieHeaderData      Header;
        // These attributes should become available any time after loading has started.
        // File Attributes, described by FileAttrType.
        UInt                    FileAttributes;
        // String storing meta-data. May have embedded 0 chars ?
        UByte*                  pMetadata;
        UInt                    MetadataSize;

        // *** Current Loading State        

        // These values are updated based the I/O progress.
        MovieLoadState          LoadState;
        // Current frame being loaded; playback/rendering is only possible
        // when this value is greater then 0.
        UInt                    LoadingFrame;
        // This flag is set to 'true' if loading is being canceled;
        // this flag will be set if the loading process should terminate.
        // This value is different from LoadState == BS_Canceled
        volatile bool           LoadingCanceled;

        // Tag Count, used for reporting movie info; also advanced during loading.
        // Serves as a general information 
        UInt                    TagCount;


#ifndef GFC_NO_THREADSUPPORT
        // Mutex locked when incrementing a frame and
        // thus adding data to pFrameBindData.
        GPtr<GFxLoadUpdateSync> pFrameUpdate;
#endif

        // Read-time updating linked lists of binding data, as 
        // described by comments above for DefBindingData.
        DefBindingData          BindData;


        // *** Resource Data

        // Counter used to assign resource handle indices during loading.
        UInt                    ResIndexCounter;

        // Resource data is progressively loaded as new tags become available; during
        // loading new entries are added to the data structures below.
       
        // Lock applies to all resource data; only used during loading,
        // i.e. when (pdataDef->LoadState < LS_LoadFinished).
        // Locks: Resources, Exports and InvExports.
        mutable GLock           ResourceLock;

        // {resourceCharId -> resourceHandle} 
        // A library of all resources defined in this SWF/GFX file.
        ResourceHash            Resources;

        // A resource can also be exported
        // (it is possible to both import AND export a resource in the same file).
        // {export SymbolName -> Resource}   
        GFxStringHashLH<GFxResourceHandle>                  Exports;
        // Inverse exports map from GFxResourceId, since mapping from GFxResourceHandle
        // would not be useful (we don't know how to create a handle from a GFxResource*).
        // This is used primarily in GetNameOfExportedResource.
        GHashLH<GFxResourceId, GStringLH, GFixedSizeHash<GFxResourceId> >   InvExports;


        // *** Playlist Frames

        // Playlist lock, only applied when LoadState < LoadingFrames.
        // Locks: Playlist, InitActionList, NamedFrames.
        // TBD: Can become a bottleneck in FindPreviousPlaceObject2 during loading
        //      if a movie has a lot of frames and seek-back occurs.
        mutable GLock           PlaylistLock;

        GArrayLH<Frame>         Playlist;          // A list of movie control events for each frame.
        GArrayLH<Frame>         InitActionList;    // Init actions for each frame.
        int                     InitActionsCnt;    // Total number of init action buffers in InitActionList

        GFxStringHashLH<UInt>   NamedFrames;        // 0-based frame #'s

        // Incremented progressively as gradients are loaded to assign ids
        // to their data, and match them with loaded files.
        GFxResourceId           GradientIdGenerator;

        UInt32 SwdHandle;

#ifndef GFC_NO_SOUND
        GFxSoundStreamDef*      pSoundStream;
#endif
};
   


    // Data for all of our content.
    GPtr<LoadTaskData>  pData;



    // *** Initialization / Cleanup

    GFxMovieDataDef(const GFxResourceKey &creationKey,
                    MovieDataType mtype, const char *purl,
                    GMemoryHeap* pheap, bool debugHeap = false,
                    UPInt memoryArena = 0);
    ~GFxMovieDataDef();

    // Creates GFxMovieDataDef and also initializes its heap. If parent heap is not
    // specified, the new heap lifetime is ties to the internal LoadTaskData object,
    // since its reference goes away last.
    static GFxMovieDataDef* Create(const GFxResourceKey &creationKey,
                                   MovieDataType mtype, const char *purl,
                                   GMemoryHeap *pargHeap = 0, bool debugHeap = 0, UPInt memoryArena = 0)
    {        
        GFxMovieDataDef* proot = GHEAP_NEW(pargHeap ? pargHeap : GMemory::GetGlobalHeap())
                                    GFxMovieDataDef(creationKey, mtype, purl, pargHeap, debugHeap, memoryArena);
        return proot;
    }

    GMemoryHeap*        GetHeap() const { return pData->GetHeap(); }
    GMemoryHeap*        GetImageHeap()  { return pData->GetImageHeap(); }


    // Create an empty MovieDef with no content; Used for LoadVars into a new level.
    void    InitEmptyMovieDef()
    {
        pData->InitEmptyMovieDef();
    }

    // Create a MovieDef describing an image file.
    void    InitImageFileMovieDef(UInt fileLength,
                                  GFxImageResource *pimageResource, bool bilinear = 1)
    {
        pData->InitImageFileMovieDef(fileLength, pimageResource, bilinear);
    }

    // Waits until loading is completed, used by GFxFontLib.
    void                WaitForLoadFinish(bool cancel = false) const
    {
        if (cancel)
            pData->OnMovieDataDefRelease();
        pData->WaitForLoadFinish();
    }

    // Waits until a specified frame is loaded
    void                WaitForFrame(UInt frame) const { pData->WaitForFrame(frame); }

    
    // *** Information query methods.

    // All of the accessor methods just delegate to LoadTaskData; there
    // are no loading methods exposed here since loading happens only through GFxLoadProcess.    
    Float               GetFrameRate() const        { return pData->Header.FPS; }
    const GRectF&       GetFrameRectInTwips() const { return pData->Header.FrameRect; }
    GRectF              GetFrameRect() const        { return TwipsToPixels(pData->Header.FrameRect); }
    Float               GetWidth() const            { return ceilf(TwipsToPixels(pData->Header.FrameRect.Width())); }
    Float               GetHeight() const           { return ceilf(TwipsToPixels(pData->Header.FrameRect.Height())); }
    virtual UInt        GetVersion() const          { return pData->GetVersion(); }
    virtual UInt        GetLoadingFrame() const     { return pData->LoadingFrame; }
    virtual UInt        GetSWFFlags() const         { return pData->Header.SWFFlags; }

    MovieLoadState      GetLoadState() const        { return pData->LoadState; }

    UInt32              GetFileBytes() const        { return pData->Header.FileLength; }
    UInt                GetTagCount() const         { return pData->TagCount;  }
    const char*         GetFileURL() const          { return pData->GetFileURL(); }
    UInt                GetFrameCount() const       { return pData->Header.FrameCount; }

    const GFxExporterInfo*  GetExporterInfo() const { return pData->GetExporterInfo(); }    
    void                GetMovieInfo(GFxMovieInfo *pinfo) const { pData->Header.GetMovieInfo(pinfo); }
    
    // Sets MetaData of desired size.
    UInt                GetFileAttributes() const   { return pData->FileAttributes; }
    UInt                GetMetadata(char *pbuff, UInt buffSize) const { return pData->GetMetadata(pbuff, buffSize); }
           

    // Fills in 0-based frame number.
    bool                GetLabeledFrame(const char* label, UInt* frameNumber, bool translateNumbers = 1)
    {
        return pData->GetLabeledFrame(label, frameNumber, translateNumbers);
    }
    
    // Get font data by ResourceId.   
    bool                GetResourceHandle(GFxResourceHandle* phandle, GFxResourceId rid) const
    {
        return pData->GetResourceHandle(phandle, rid);
    }

    GFxFontDataUseNode* GetFirstFont() const     { return pData->GetFirstFont(); }
    GFxImportData*      GetFirstImport() const   { return pData->GetFirstImport(); }


    // Helper used to look up labeled frames and/or translate frame numbers from a string.
    static bool         TranslateFrameString(
                            const  GFxStringHashLH<UInt> &namedFrames,
                            const char* label, UInt* frameNumber, bool translateNumbers);
    
         
    // *** GFxTimelineDef implementation.

    // Frame access from GFxTimelineDef.
    virtual const Frame GetPlaylist(int frame) const        { return pData->GetPlaylist(frame); }
    UInt                GetInitActionListSize() const       { return pData->GetInitActionListSize(); }
    inline  bool        HasInitActions() const              { return pData->HasInitActions(); }
    virtual bool        GetInitActions(Frame* pframe, int frame) const { return pData->GetInitActions(pframe, frame); }

#ifndef GFC_NO_SOUND
    virtual GFxSoundStreamDef*  GetSoundStream() const      { return pData->GetSoundStream(); }
    virtual void                SetSoundStream(GFxSoundStreamDef* pdef) { pData->SetSoundStream(pdef); }
#endif

    // *** Creating MovieDefData file keys

    // Create a key for an SWF file corresponding to GFxMovieDef.
    // Note that GFxImageCreator is only used as a key if is not null.
    static  GFxResourceKey  CreateMovieFileKey(const char* pfilename,
                                               SInt64 modifyTime,
                                               GFxFileOpener* pfileOpener,
                                               GFxImageCreator* pimageCreator,
                                               GFxPreprocessParams* ppreprocessParams);

    // *** GFxResource implementation
    
    virtual GFxResourceKey  GetKey()                        { return ResourceKey; }
    virtual UInt            GetResourceTypeCode() const     { return MakeTypeCode(RT_MovieDataDef); }

    // Resource report.
    virtual GFxResourceReport* GetResourceReport()          { return this; }
    virtual GString         GetResourceName() const;
    virtual GMemoryHeap*    GetResourceHeap() const         { return GetHeap(); }
    virtual void            GetStats(GStatBag* pbag, bool reset = true) { GUNUSED2(pbag, reset); }

};


// State values that cause generation of a different binding
class  GFxMovieDefBindStates : public GRefCountBase<GFxMovieDefBindStates, GStat_Default_Mem>
{
public:
    // NOTE: We no longer store pDataDef here, instead it is now a part of
    // a separate GFxMovieDefImplKey object. Such separation is necessary
    // to allow release of GFxMovieDefImpl to cancel the loading process, which
    // is thus not allowed to AddRef to pDataDef. 

    // GFxStates that would cause generation of a new DefImpl binding.
    // These are required for the following reasons:
    //
    //  Class               Binding Change Cause
    //  ------------------  ----------------------------------------------------
    //  GFxFileOpener       Substitute GFile can produce different data.
    //  GFxURLBuilder   Can cause different import file substitutions.
    //  GFxImageCreator     Different image representation = different binding.
    //  GFxImportVisitor     (?) Import visitors can substitute import files/data.
    //  GFxImageVisitor      (?) Image visitors  can substitute image files/data.
    //  GFxGradientParams   Different size of gradient = different binding.
    //  GFxFontParams       Different font texture sizes = different binding.
    //  GFxFontCompactorParams


    GPtr<GFxFileOpener>          pFileOpener;
    GPtr<GFxURLBuilder>          pURLBulider;
    GPtr<GFxImageCreator>        pImageCreator;       
    GPtr<GFxImportVisitor>       pImportVisitor;
  //  GPtr<GFxImageVisitor>       pImageVisitor;
    GPtr<GFxGradientParams>      pGradientParams;
    GPtr<GFxFontPackParams>      pFontPackParams;
    GPtr<GFxPreprocessParams>    pPreprocessParams;
    GPtr<GFxFontCompactorParams> pFontCompactorParams;
    GPtr<GFxImagePackParamsBase> pImagePackParams;

    GFxMovieDefBindStates(GFxStateBag* psharedState)
    {        
        // Get multiple states at once to avoid extra locking.
        GFxState*                        pstates[9]    = {0,0,0,0,0,0,0,0,0};
        const static GFxState::StateType stateQuery[9] =
          { GFxState::State_FileOpener,     GFxState::State_URLBuilder,
            GFxState::State_ImageCreator,   GFxState::State_ImportVisitor,
            GFxState::State_GradientParams, GFxState::State_FontPackParams,
            GFxState::State_PreprocessParams, GFxState::State_FontCompactorParams,
            GFxState::State_ImagePackerParams
          };

        // Get states and assign them locally.
        psharedState->GetStatesAddRef(pstates, stateQuery, 9);
        pFileOpener          = *(GFxFileOpener*)          pstates[0];
        pURLBulider          = *(GFxURLBuilder*)          pstates[1];
        pImageCreator        = *(GFxImageCreator*)        pstates[2];
        pImportVisitor       = *(GFxImportVisitor*)       pstates[3];
        pGradientParams      = *(GFxGradientParams*)      pstates[4];
        pFontPackParams      = *(GFxFontPackParams*)      pstates[5];
        pPreprocessParams    = *(GFxPreprocessParams*)    pstates[6];
        pFontCompactorParams = *(GFxFontCompactorParams*) pstates[7];
        pImagePackParams     = *(GFxImagePackParamsBase*) pstates[8];
    }

    GFxMovieDefBindStates(GFxMovieDefBindStates *pother)
    {        
        pFileOpener         = pother->pFileOpener;
        pURLBulider         = pother->pURLBulider;
        pImageCreator       = pother->pImageCreator;
        pImportVisitor      = pother->pImportVisitor;
      //  pImageVisitor       = pother->pImageVisitor;
        pFontPackParams     = pother->pFontPackParams;
        pGradientParams     = pother->pGradientParams;
        pPreprocessParams   = pother->pPreprocessParams;
        pFontCompactorParams = pother->pFontCompactorParams;
        pImagePackParams  = pother->pImagePackParams;
        // Leave pDataDef uninitialized since this is used
        // from GFxLoadStates::CloneForImport, and imports
        // have their own DataDefs.
    }

    // Functionality necessary for this GFxMovieDefBindStates to be used
    // as a key object for GFxMovieDefImpl.
    UPInt  GetHashCode() const;
    bool operator == (GFxMovieDefBindStates& other) const;  
    bool operator != (GFxMovieDefBindStates& other) const { return !operator == (other); }
};


// ***** Loading state collection


// Load states are used for both loading and binding, they 
// are collected atomically at the beginning of CreateMovie call,
// and apply for the duration of load/bind time.

class GFxLoadStates : public GRefCountBase<GFxLoadStates, GStat_Default_Mem>
{
public:

    // *** These states are captured from Loader in constructor.

    // States used for binding of GFxMovieDefImpl.
    GPtr<GFxMovieDefBindStates> pBindStates;

    // Other states we might need
    // Load-affecting GFxState(s).
    GPtr<GFxLog>                pLog;     
    GPtr<GFxParseControl>       pParseControl;
    GPtr<GFxProgressHandler>    pProgressHandler;
    GPtr<GFxTaskManager>        pTaskManager;
    // Store cache manager so that we can issue font params warning if
    // it is not available.
    GPtr<GFxFontCacheManager>   pFontCacheManager;
    // Store render config so that it can be passed for image creation.
    // Note that this state is non-binding.
    GPtr<GFxRenderConfig>       pRenderConfig;

    GPtr<GFxJpegSupportBase>    pJpegSupport;
    GPtr<GFxZlibSupportBase>    pZlibSupport;
    GPtr<GFxPNGSupportBase>     pPNGSupport;
#ifndef GFC_NO_VIDEO
    GPtr<GFxVideoBase> pVideoPlayerState;
#endif
#ifndef GFC_NO_SOUND
    GPtr<GFxAudioBase>          pAudioState;
#endif
    // Weak States
    GPtr<GFxResourceWeakLib>    pWeakResourceLib;    


    // *** Loader

    // Loader back-pointer (contents may change).
    // We do NOT use this to access states because that may result 
    // in inconsistent state use when loading is threaded.    
    GPtr<GFxLoaderImpl>         pLoaderImpl;

    // Cached relativePath taken from DataDef URL, stored
    // so that it can be easily passed for imports.
    GString                     RelativePath;

    // Set to true if multi-threaded loading is using
    bool                        ThreadedLoading; 

    // *** Substitute fonts

    // These are 'substitute' fonts provider files that need to be considered when
    // binding creating a GFxFontResource from ResourceData. Substitute fonts come from
    // files that have '_glyphs' in filename and replace fonts with matching names
    // in the file being bound.
    // Technically, this array should be a part of 'GFxBindingProcess'. If more
    // of such types are created, we may need to institute one.
    GArray<GPtr<GFxMovieDefImpl> > SubstituteFontMovieDefs;


    GFxLoadStates();
    ~GFxLoadStates();

    // Creates GFxLoadStates by capturing states from pstates. If pstates
    // is null, all states come from loader. If binds states are null,
    // bind states come from pstates.
    GFxLoadStates(GFxLoaderImpl* ploader, GFxStateBag* pstates = 0,
                  GFxMovieDefBindStates *pbindStates = 0);

    // Helper that clones load states, pBindSates.
    // The only thing left un-copied is GFxMovieDefBindStates::pDataDef
    GFxLoadStates*          CloneForImport() const;


    GFxResourceWeakLib*     GetLib() const              { return pWeakResourceLib.GetPtr();  }
    GFxMovieDefBindStates*  GetBindStates() const       { return pBindStates.GetPtr(); }
    GFxLog*                 GetLog() const              { return pLog; }
    GFxTaskManager*         GetTaskManager() const      { return pTaskManager; }
    GFxRenderConfig*        GetRenderConfig() const     { return pRenderConfig; }
    GFxFontCacheManager*    GetFontCacheManager() const { return pFontCacheManager; }
    GFxProgressHandler*     GetProgressHandler() const  { return pProgressHandler; }
    GFxJpegSupportBase*     GetJpegSupport() const      { return pJpegSupport; }
    GFxZlibSupportBase*     GetZlibSupport() const      { return pZlibSupport; }
    GFxFontCompactorParams* GetFontCompactorParams() const { return pBindStates->pFontCompactorParams; }
    GFxPNGSupportBase*      GetPNGSupport() const       { return pPNGSupport; }

    GFxFileOpener*          GetFileOpener() const       { return pBindStates->pFileOpener;  }
    GFxFontPackParams*      GetFontPackParams() const   { return pBindStates->pFontPackParams; }
    GFxPreprocessParams*    GetPreprocessParams() const { return pBindStates->pPreprocessParams; }

#ifndef GFC_NO_VIDEO
    GFxVideoBase* GetVideoPlayerState() const { return pVideoPlayerState; }
#endif
#ifndef GFC_NO_SOUND
    GFxAudioBase*           GetAudio() const            { return pAudioState; }
#endif
    // Initializes the relative path.
    void                    SetRelativePathForDataDef(GFxMovieDataDef* pdef);
    const GString&          GetRelativePath() const     { return RelativePath;  }


    // Obtains an image creator, but only if image data is not supposed to
    // be preserved; considers GFxLoader::LoadKeepBindData flag from arguments.
    GFxImageCreator*        GetLoadTimeImageCreator(UInt loadConstants) const; 

    // Delegated state access helpers. If loader flag LoadQuietOpen is set in loadConstants
    // and requested file can not be opened all error messages will be suppressed.
    // The 'pfilename' should be encoded as UTF-8 to support international names.
    GFile*      OpenFile(const char *pfilename, UInt loadConstants = GFxLoader::LoadAll);

    // Perform filename translation and/or copy by relying on translator.
    void        BuildURL(GString *pdest, const GFxURLBuilder::LocationInfo& loc) const;

    // Submit a background task. Returns false if TaskManager is not set or it couldn't 
    // add the task
    bool        SubmitBackgroundTask(GFxLoaderTask* ptask);

    bool        IsThreadedLoading() const { return ThreadedLoading || pTaskManager; }
};



// ***  GFxCharacterCreateInfo

// Simple structure containing information necessary to create a character.
// This is returned bu GFxMovieDefImpl::GetCharacterCreateInfo().

// Since characters can come from an imported file, we also contain a 
// GFxMovieDefImpl that should be used for a context of any characters
// created from this character def.
struct GFxCharacterCreateInfo
{
    GFxCharacterDef* pCharDef;
    GFxMovieDefImpl* pBindDefImpl;
};



// ***** GFxMovieDefImpl

// This class holds the immutable definition of a GFxMovieSub's
// contents.  It cannot be played directly, and does not hold
// current state; for that you need to call CreateInstance()
// to get a MovieInstance.


class GFxMovieDefImpl : public GFxMovieDef
{
public:

    // Shared state, for loading, images, logs, etc.
    // Note that we actually own our own copy here.
    GPtr<GFxStateBagImpl>       pStateBag;

    // We store the loader here; however, we do not take our
    // states from it -> instead, the states come from pStateBag.
    // The loader is here to give access to pWeakLib. This loader is also used
    // from GFxMovieRoot to load child movies (but with a different state bag).
    // TBD: Do we actually need Loader then? Or just Weak Lib?
    // May need task manger too...
    // NOTE: This instead could be stored in MovieRoot. What's a more logical place?    
    GPtr<GFxLoaderImpl>         pLoaderImpl;


    // States form OUR Binding Key.

    // Bind States contain the DataDef.    
    // It could be 'const GFxMovieDefBindStates*' but we can't do that with GPtr.
    GPtr<GFxMovieDefBindStates> pBindStates;



    // *** BindTaskData

    // Most of the GFxMovieDefImpl data is contained in BindTaskData class,
    // which is AddRef-ed separately. Such setup is necessary to allow
    // the loading thread to be canceled when all user reference count
    // for GFxMovieDefImpl are released. The loading thread only references
    // the LoadData object and thus can be notified when GFxMovieDefImpl dies.

    // BindStateType is used by BindTaskData::BindState.
    enum BindStateType
    {
        BS_NotStarted       = 0,
        BS_InProgress       = 1,
        BS_Finished         = 2,
        BS_Canceled         = 3, // Canceled due to a user request.
        BS_Error            = 4,
        // Mask for above states.
        BS_StateMask        = 0xF,

        // These bits store the status of what was
        // actually loaded; we can wait based on them.
        BSF_Frame1Loaded    = 0x100,
        BSF_LastFrameLoaded = 0x200,
    };


    class BindTaskData : public GRefCountBase<BindTaskData, GStat_Default_Mem>
    {
        friend class GFxMovieDefImpl;
        friend class GFxMovieBindProcess;

        // Our own heap.
        GMemoryHeap*               pHeap;

        // AddRef to DataDef because as long as binding process
        // is alive we must exist.
        GPtr<GFxMovieDataDef>       pDataDef;

        // A shadow pointer to pDefImpl that can 'turn' bad unless
        // accessed through a lock in GetDefImplAddRef(). Can be null
        // if GFxMovieDefImpl has been deleted by another thread.
        GFxMovieDefImpl*            pDefImpl_Unsafe;

        // Save load flags so that they can be propagated
        // into the nested LoadMovie calls.
        UInt                        LoadFlags;
              

        // Binding table for handles in pMovieDataDef.
        GFxResourceBinding          ResourceBinding;

        // Movies we import from; hold a ref on these, to keep them alive.
        // This array directly corresponds to GFxMovieDataDef::ImportData.
        GArrayLH<GPtr<GFxMovieDefImpl>, GFxStatMD_Other_Mem>    ImportSourceMovies;
        // Lock for accessing above.
        //  - We also use this lock to protect pDefImpl
        GLock                                  ImportSourceLock;

        // Other imports such as through FontLib. These do not need
        // to be locked because it is never accessed outside binding
        // thread and destructor.
        GArrayLH<GPtr<GFxMovieDefImpl>, GFxStatMD_Other_Mem>    ResourceImports;


        // *** Binding Progress State

        // Binding state variables are modified as binding progresses; there is only
        // one writer - the binding thread. The binding variables modified progressively
        // are BindState, BindingFrame and BytesLoaded. BindingFrame and BytesLoaded
        // are polled in the beginning of the next frame Advance, thus no extra sync
        // is necessary for them. BindState; however, can be waited on with BindStateMutex.

        // Current binding state modified after locking BindStateMutex.
        volatile UInt       BindState;

#ifndef GFC_NO_THREADSUPPORT
        // This mutex/WC pair is used to wait on the BindState, modified when a
        // critical event such as binding completion, frame 1 bound or error
        // take place. We have to wait on these when/if GFxLoader::LoadWaitFrame1
        // or LoadWaitCompletion load flags are passed.
        GPtr<GFxLoadUpdateSync> pBindUpdate;        
#endif

        // Bound Frame - the frame we are binding now. This frame corresponds
        // to the Loading Frame, as only bound frames (with index < BindingFrame)
        // can actually be used.
        volatile UInt       BindingFrame;

        // Bound amount of bytes loaded for the Binding frame,
        // must be assigned before BindingFrame.
        volatile UInt32     BytesLoaded;

        // This flag is set to 'true' if binding is being canceled;
        // this flag will be set if the binding process should terminate.
        // This value is different from BinsState == BS_Canceled
        volatile bool       BindingCanceled;


    public:
        
        // Technically BindTaskData should not store a pointer to GFxMovieDefImpl,
        // however, we just pass it to initialize regular ptr in GFxResourceBinding.
        BindTaskData(GMemoryHeap *pheap,
                     GFxMovieDataDef *pdataDef,
                     GFxMovieDefImpl *pdefImpl,
                     UInt loadFlags, bool fullyLoaded);
        ~BindTaskData();

        GMemoryHeap*        GetBindDataHeap() const { return pHeap; }

        // Notifies BindData that MovieDefImpl is being destroyed. This may be a premature
        // destruction if we are in the process of loading (in that case it will lead to
        // loading being canceled).
        void                OnMovieDefRelease();


        GFxMovieDataDef*    GetDataDef() const { return pDataDef; }

        // Grabs OUR GFxMovieDefImpl through a lock; can return null.
        GFxMovieDefImpl*    GetMovieDefImplAddRef();

        
        // Bind state accessors; calling SetBindState notifies BindStateUpdated.
        void                SetBindState(UInt newState);
        UInt                GetBindState() const { return BindState; }    
        BindStateType       GetBindStateType() const { return (BindStateType) (BindState & BS_StateMask); }
        UInt                GetBindStateFlags() const { return BindState & ~BS_StateMask; }

#ifndef GFC_NO_THREADSUPPORT
        GFxLoadUpdateSync*  GetBindUpdateSync() const  { return pBindUpdate; }
#endif

        // Wait for for bind state flag or error. Return true for success,
        // false if bind state was changed to error without setting the flags.
        bool                WaitForBindStateFlags(UInt flags);

        // Query progress.
        UInt                GetLoadingFrame() const     { return GAtomicOps<UInt>::Load_Acquire(&BindingFrame); }
        UInt32              GetBytesLoaded() const      { return BytesLoaded; }
        
        // Updates binding state Frame and BytesLoaded (called from image loading task).
        void                UpdateBindingFrame(UInt frame, UInt32 bytesLoaded);


        // Access import source movie based on import index (uses a lock).
        GFxMovieDefImpl*    GetImportSourceMovie(UInt importIndex);
        // Adds a movie reference to ResourceImports array.
        void                AddResourceImportMovie(GFxMovieDefImpl *pdefImpl);


        // *** Import binding support.

        // After GFxMovieDefImpl constructor, the most important part of GFxMovieDefImpl 
        // is initialization binding, i.e. resolving all of dependencies based on the binding states.
        // This is a step where imports and images are loaded, gradient images are
        // generated and fonts are pre-processed.
        // Binding is done by calls to GFxMovieBindProcess::BindNextFrame.

        // Resolves and import during binding.
        void                    ResolveImport(GFxImportData* pimport, GFxMovieDefImpl* pdefImpl,
                                              GFxLoadStates* pls, bool recursive);

        // Resolves an import of 'gfxfontlib.swf' through the GFxFontLib object.
        // Returns 1 if ALL mappings succeeded, otherwise 0.
        bool                    ResolveImportThroughFontLib(GFxImportData* pimport);
                                                            //GFxLoadStates* pls,
                                                            //GFxMovieDefImpl* pourDefImpl);

        // Internal helper for import updates.
        bool                    SetResourceBindData(GFxResourceId rid, GFxResourceBindData& bindData,
                                                    const char* pimportSymbolName);

        bool                    SetResourceBindData(GFxResourceDataNode *presnode, GFxResource* pres);
    };

    GPtr<BindTaskData>  pBindData;


    
    // *** Constructor / Destructor

    GFxMovieDefImpl(GFxMovieDataDef* pdataDef,
                    GFxMovieDefBindStates* pstates,
                    GFxLoaderImpl* ploaderImpl,
                    UInt loadConstantFlags,                    
                    GFxStateBagImpl *pdelegateState = 0,
                    GMemoryHeap* pargHeap = 0,
                    bool fullyLoaded = 0,
                    UPInt memoryArena = 0);
    ~GFxMovieDefImpl();

    MemoryContext*  CreateMemoryContext(const char* heapName, const MemoryParams& memParams, bool debugHeap);
    GFxMovieView*   CreateInstance(const MemoryParams& memParams, bool initFirstFrame = true);
    GFxMovieView*   CreateInstance(MemoryContext* memContext, bool initFirstFrame = true); 

    //  TBD: Should rename for Memory API cleanup
    virtual GMemoryHeap*    GetLoadDataHeap() const { return pBindData->pDataDef->GetHeap(); }
    virtual GMemoryHeap*    GetBindDataHeap() const { return pBindData->GetBindDataHeap(); }
    virtual GFxResource*    GetMovieDataResource() const { return pBindData->pDataDef; }
    virtual GMemoryHeap*    GetImageHeap() const { return pBindData->pDataDef->GetImageHeap(); }


    // *** Creating MovieDefImpl keys

    // GFxMovieDefImpl key depends (1) pMovieDefData, plus (2) all of the states
    // used for its resource bindings, such as file opener, file translator, image creator,
    // visitors, etc. A snapshot of these states is stored in GFxMovieDefBindStates.
    // Movies that share the same bind states are shared through GFxResourceLib.

    // Create a key for an SWF file corresponding to GFxMovieDef.
    static  GFxResourceKey  CreateMovieKey(GFxMovieDataDef *pdataDef,
                                           GFxMovieDefBindStates* pbindStates);
    
    
    // *** Property access

    GFxMovieDataDef*    GetDataDef() const          { return pBindData->GetDataDef(); }

    // ...
    UInt                    GetFrameCount() const       { return GetDataDef()->GetFrameCount(); }
    Float                   GetFrameRate() const        { return GetDataDef()->GetFrameRate(); }
    GRectF                  GetFrameRect() const        { return GetDataDef()->GetFrameRect(); }
    Float                   GetWidth() const            { return GetDataDef()->GetWidth(); }
    Float                   GetHeight() const           { return GetDataDef()->GetHeight(); }
    virtual UInt            GetVersion() const          { return GetDataDef()->GetVersion(); }
    virtual UInt            GetSWFFlags() const         { return GetDataDef()->GetSWFFlags(); }    
    virtual const char*     GetFileURL() const          { return GetDataDef()->GetFileURL(); }

    UInt32              GetFileBytes() const        { return GetDataDef()->GetFileBytes(); }
    virtual UInt        GetLoadingFrame() const     { return pBindData->GetLoadingFrame(); }
    UInt32              GetBytesLoaded() const      { return pBindData->GetBytesLoaded(); }
    GFxMovieDataDef::MovieLoadState      GetLoadState() const      { return GetDataDef()->GetLoadState(); }
    UInt                GetTagCount() const         { return GetDataDef()->GetTagCount();  }
    
    inline UInt         GetLoadFlags() const        { return pBindData->LoadFlags; }

    void                WaitForLoadFinish(bool cancel = false) const
    {
        GetDataDef()->WaitForLoadFinish(cancel);
#ifndef GFC_NO_THREADSUPPORT
        // If using binding thread, make sure that binding thread has finished
        // and released its references to us as well.
        pBindData->GetBindUpdateSync()->WaitForLoadFinished();
#endif
    }

    void                WaitForFrame(UInt frame) const { GetDataDef()->WaitForFrame(frame); }

    // Stripper info query.
    virtual const GFxExporterInfo*  GetExporterInfo() const  { return GetDataDef()->GetExporterInfo(); }

    GRectF                      GetFrameRectInTwips() const { return GetDataDef()->GetFrameRectInTwips(); }
    GFxResourceWeakLib*         GetWeakLib() const          { return pLoaderImpl->GetWeakLib(); }

    // Shared state implementation.
    virtual GFxStateBag*        GetStateBagImpl() const       { return pStateBag.GetPtr(); }    

    // Overrides for users
    virtual UInt                GetMetadata(char *pbuff, UInt buffSize) const
        { return GetDataDef()->GetMetadata(pbuff, buffSize); }
    virtual UInt                GetFileAttributes() const { return GetDataDef()->GetFileAttributes(); }


    // GFxLog Error delegation
    void    LogError(const char* pfmt, ...)
    { 
        va_list argList; va_start(argList, pfmt);
        GFxLog *plog = GetLog();
        if (plog) plog->LogMessageVarg(GFxLog::Log_Error, pfmt, argList);
        va_end(argList); 
    }


    // Wait for for bind state flag or error. Return true for success,
    // false if bind state was changed to error without setting the flags.
    bool                        WaitForBindStateFlags(UInt flags) { return pBindData->WaitForBindStateFlags(flags); }
       

    // *** Resource Lookup

    // Obtains a resource based on its id. If resource is not yet resolved,
    // NULL is returned. Should be used only before creating an instance.
    // Type checks the resource based on specified type.
   // GFxResource*                GetResource(GFxResourceId rid, GFxResource::ResourceType rtype);
    // Obtains full character creation information, including GFxCharacterDef.
    GFxCharacterCreateInfo      GetCharacterCreateInfo(GFxResourceId rid);

    // Get a binding table reference.
    const GFxResourceBinding&   GetResourceBinding() const { return pBindData->ResourceBinding; }
    GFxResourceBinding&         GetResourceBinding()       { return pBindData->ResourceBinding; }
 
   
    // *** GFxMovieDef implementation

    virtual void                VisitImportedMovies(ImportVisitor* visitor);           
    virtual void                VisitResources(ResourceVisitor* pvisitor, UInt visitMask = ResVisit_AllImages);
    virtual GFxResource*        GetResource(const char *pexportName) const;

    // Locate a font resource by name and style.
    // It's ok to return GFxFontResource* without the binding because pBinding
    // is embedded into font resource allowing imports to be handled properly.
    struct SearchInfo
    {
        enum SearchStatus
        {
            NotFound = 0,
            FoundInResources,
            FoundInResourcesNoGlyphs,
            FoundInResourcesNeedFaux,
            FoundInImports,
            FoundInImportsFontLib,
            FoundInExports
        } Status;
        typedef GHashSet<GString, GString::NoCaseHashFunctor> StringSet;
        StringSet ImportSearchUrls;
        GString   ImportFoundUrl;
    };
    virtual GFxFontResource*    GetFontResource(const char* pfontName, UInt styleFlags, SearchInfo* psearchInfo = NULL);

    // Fill in the binding resource information together with its binding.
    // 'ignoreDef' is the def that will be ignored (incl all sub-defs), used to search globally when
    // local search was unsuccessful, to exclude the already searched branch.
    // Return 0 if Get failed and no bind data was returned.
    bool                        GetExportedResource(GFxResourceBindData *pdata, const GString& symbol, GFxMovieDefImpl* ignoreDef = NULL);   
    const GString*              GetNameOfExportedResource(GFxResourceId rid) const;
    GFxResourceId               GetExportedResourceIdByName(const GString& name) const;

    // *** GFxResource implementation

    virtual GFxResourceKey  GetKey()                        { return CreateMovieKey(GetDataDef(), pBindStates); }
    virtual UInt            GetResourceTypeCode() const     { return MakeTypeCode(RT_MovieDef); }

    // checks if this moviedefimpl imports the "import" one directly
    bool                    DoesDirectlyImport(const GFxMovieDefImpl* import);
};




// *** GFxMovieDefBindProcess

// GFxMovieDefBindProcess stores the states necessary for binding. The actual
// binding is implemented by calling BindFrame for every frame that needs to be bound.
//
// Binding is separated into a separate object so that it can be reused independent
// of whether it takes place in a separate thread (which just calls BindFrame until
// its done) or is interleaved together with the loading process, which can call
// it after each frame.

class GFxMovieBindProcess : public GFxLoaderTask
{
    typedef GFxMovieDefImpl::BindTaskData BindTaskData;


    // This is either current (if in BindNextFrame) or previous frame bind data.
    GFxFrameBindData*    pFrameBindData;

    // Perform binding of resources.
    GFxResourceId       GlyphTextureIdGen;
  
    GPtr<GFxImagePacker> pImagePacker;
  
    // We keep a smart pointer to GFxMovieDefImpl::BindTaskData and not
    // the GFxMovieDefImpl itself; this allows us to cancel loading
    // gracefully if user's GFxMovieDef is released.
    GPtr<BindTaskData>  pBindData;

    // Ok to store weak ptr since BindTaskData AddRefs to DataDef. 
    GFxMovieDataDef*    pDataDef;   

    bool                Stripped;

    // We need to keep load import stack so we can detect recursion.
    GFxLoaderImpl::LoadStackItem* pLoadStack;

    // Shared with binding
    GFxTempBindData*        pTempBindData;

public:
   
    GFxMovieBindProcess(GFxLoadStates *pls,
                        GFxMovieDefImpl* pdefImpl,
                        GFxLoaderImpl::LoadStackItem* ploadStack = NULL);

    ~GFxMovieBindProcess();


    typedef GFxMovieDefImpl::BindStateType BindStateType;

    // Bind a next frame.
    // If binding failed, then BS_Error will be returned.
    BindStateType       BindNextFrame();


    // BindState delegates to GFxMovieDefImpl.
    void                SetBindState(UInt newState) { if (pBindData) pBindData->SetBindState(newState); }
    UInt                GetBindState() const        { return pBindData->GetBindState(); }
    BindStateType       GetBindStateType() const    { return pBindData->GetBindStateType(); }
    UInt                GetBindStateFlags() const   { return pBindData->GetBindStateFlags(); }

    // Gets binding data for the next frame, if any.
    GFxFrameBindData*    GetNextFrameBindData()
    {
        if (pFrameBindData)
            return pFrameBindData->pNextFrame;
        return pDataDef->pData->GetFirstFrameBindData();
    }

    void                FinishBinding();

    void                    SetTempBindData(GFxTempBindData* ptempdata)     { pTempBindData = ptempdata; }
    GFxTempBindData*        GetTempBindData() const                         { return pTempBindData; }


    // *** GFxTask implementation
    
    virtual void    Execute()
    {
        // Do the binding.
        while(BindNextFrame() == GFxMovieDefImpl::BS_InProgress)
        { }     
    }

    virtual void    OnAbandon(bool started) 
    { 
        if (pBindData)
        {
            if (started)
                pBindData->BindingCanceled = true;
            else
                SetBindState(GFxMovieDefImpl::BS_Canceled); 
        }
    }
};


class GFxTimelineSnapshot;


//
// GFxSwfEvent
//
// For embedding event handlers in GFxPlaceObject2

class GFxSwfEvent : public GNewOverrideBase<GFxStatMD_Other_Mem>
{
public:
    // NOTE: DO NOT USE THESE AS VALUE TYPES IN AN
    // GArray<>!  They cannot be moved!  The private
    // operator=(const GFxSwfEvent&) should help guard
    // against that.

    GFxEventId                      Event;
    GPtr<GASActionBufferData>       pActionOpData;    

    GFxSwfEvent()
    { }
        
    void    Read(GFxStreamContext* psc, UInt32 flags);

    void    AttachTo(GFxASCharacter* ch) const;
    
private:
    // DON'T USE THESE
    GFxSwfEvent(const GFxSwfEvent& ) { GASSERT(0); }
    void    operator=(const GFxSwfEvent& ) { GASSERT(0); }
};


// ***** Execute Tags

class GFxActionPriority
{
public:
    enum Priority
    {
        AP_Highest      = 0, // initclips for imported source movies
        AP_Initialize      , // onClipEvent(initialize)
        AP_InitClip        , // local initclips
        AP_Construct       , // onClipEvent(construct)/ctor
        AP_Frame           , // frame code
        AP_Normal = AP_Frame,
        AP_Load            , // onLoad-only

        AP_Count
    };
};


struct GFxCharPosInfoFlags
{
    enum
    {
        Flags_HasDepth          = 0x01,
        Flags_HasCharacterId    = 0x02,
        Flags_HasMatrix         = 0x04,
        Flags_HasCxform         = 0x08,
        Flags_HasRatio          = 0x10,
        Flags_HasFilters        = 0x20,
        Flags_HasClipDepth      = 0x40,
        Flags_HasBlendMode      = 0x80
    };

    UInt8               Flags;
    
    GFxCharPosInfoFlags() : Flags(0) {}
    GFxCharPosInfoFlags(UInt8 flags) : Flags(flags) {}

    GINLINE void SetMatrixFlag()        { Flags |= Flags_HasMatrix; }
    GINLINE void SetCxFormFlag()        { Flags |= Flags_HasCxform; }
    GINLINE void SetBlendModeFlag()     { Flags |= Flags_HasBlendMode; }
    GINLINE void SetFiltersFlag()       { Flags |= Flags_HasFilters; }
    GINLINE void SetDepthFlag()         { Flags |= Flags_HasDepth; }
    GINLINE void SetClipDepthFlag()     { Flags |= Flags_HasClipDepth; }
    GINLINE void SetRatioFlag()         { Flags |= Flags_HasRatio; }
    GINLINE void SetCharacterIdFlag()   { Flags |= Flags_HasCharacterId; }

    GINLINE bool HasMatrix() const      { return (Flags & Flags_HasMatrix) != 0; }
    GINLINE bool HasCxform() const      { return (Flags & Flags_HasCxform) != 0; }
    GINLINE bool HasBlendMode() const   { return (Flags & Flags_HasBlendMode) != 0; }
    GINLINE bool HasFilters() const     { return (Flags & Flags_HasFilters) != 0; }
    GINLINE bool HasDepth() const       { return (Flags & Flags_HasDepth) != 0; }
    GINLINE bool HasClipDepth() const   { return (Flags & Flags_HasClipDepth) != 0; }
    GINLINE bool HasRatio() const       { return (Flags & Flags_HasRatio) != 0; }
    GINLINE bool HasCharacterId() const { return (Flags & Flags_HasCharacterId) != 0; }
};


// A data structure that describes common positioning state.
class GFxCharPosInfo 
{
public:
    GArray<GFxFilterDesc> Filters;    // MOOSE change to pointer to array

    // -- CXFORM HEADER -----------------
    //     HasAddTerms      UB[1]                               Has color addition values if equal to 1
    //     HasMultTerms     UB[1]                               Has color multiply values if equal to 1
    //     Nbits            UB[4]                               Bits in each value field
    //     RedMultTerm      If HasMultTerms = 1, SB[Nbits]      Red multiply value
    //     GreenMultTerm    If HasMultTerms = 1, SB[Nbits]      Green multiply value
    //     BlueMultTerm     If HasMultTerms = 1, SB[Nbits]      Blue multiply value
    //     RedAddTerm       If HasAddTerms = 1, SB[Nbits]       Red addition value
    //     GreenAddTerm     If HasAddTerms = 1, SB[Nbits]       Green addition value
    //     BlueAddTerm      If HasAddTerms = 1, SB[Nbits]       Blue addition value
    GRenderer::Cxform   ColorTransform;

    // -- MATRIX HEADER -----------------
    //     HasScale         UB[1]                               Has scale values if equal to 1
    //     NScaleBits       If HasScale = 1, UB[5]              Bits in each scale value field
    //     ScaleX           If HasScale = 1, FB[NScaleBits]     x scale value
    //     ScaleY           If HasScale = 1, FB[NScaleBits]     y scale value
    //     HasRotate        UB[1]                               Has rotate and skew values if equal to 1
    //     NRotateBits      If HasRotate = 1, UB[5]             Bits in each rotate value field
    //     RotateSkew0      If HasRotate = 1,FB[NRotateBits]    First rotate and skew value
    //     RotateSkew1      If HasRotate = 1,FB[NRotateBits]    Second rotate and skew value
    //     NTranslateBits   UB[5]                               Bits in each translate value field
    //     TranslateX       SB[NTranslateBits]                  x translate value in twips
    //     TranslateY       SB[NTranslateBits]                  y translate value in twips
    GRenderer::Matrix   Matrix_1;

    Float               Ratio;              // [PPS] Should be U16
    SInt                Depth;              // [PPS] Should be U16
    GFxResourceId       CharacterId;        // [PPS] Should be U16
    UInt16              ClipDepth;
    UInt8               BlendMode;          // [PPS] Only requires 4bits for storage
    GFxCharPosInfoFlags Flags;
   
    GFxCharPosInfo()
    {
        Ratio       = 0.0f;
        Depth       = 0;        
        ClipDepth   = 0;
        BlendMode   = GRenderer::Blend_None;
    }

    GFxCharPosInfo(GFxResourceId chId, SInt depth,
        bool hasCxform, const GRenderer::Cxform &cxform,
        bool hasMatrix, const GRenderer::Matrix &matrix,
        Float ratio = 0.0f, UInt16 clipDepth = 0,
        bool hasBlendMode = false, GRenderer::BlendType blend = GRenderer::Blend_None)
        : ColorTransform(cxform), Matrix_1(matrix), CharacterId(chId)
    {
        Ratio       = ratio;
        if (hasMatrix)
            Flags.SetMatrixFlag();
        if (hasCxform)
            Flags.SetCxFormFlag();
        if (hasBlendMode)
            Flags.SetBlendModeFlag();
        Depth       = depth;
        CharacterId = chId;
        ClipDepth   = clipDepth;
        BlendMode   = (UInt8)blend;
        //Filters  = 0;
    }

    GINLINE void SetMatrixFlag()        { Flags.SetMatrixFlag(); }
    GINLINE void SetCxFormFlag()        { Flags.SetCxFormFlag(); }
    GINLINE void SetBlendModeFlag()     { Flags.SetBlendModeFlag(); }
    GINLINE void SetFiltersFlag()       { Flags.SetFiltersFlag(); }
    GINLINE void SetDepthFlag()         { Flags.SetDepthFlag(); }
    GINLINE void SetClipDepthFlag()     { Flags.SetClipDepthFlag(); }
    GINLINE void SetRatioFlag()         { Flags.SetRatioFlag(); }
    GINLINE void SetCharacterIdFlag()   { Flags.SetCharacterIdFlag(); }

    GINLINE bool HasMatrix() const      { return Flags.HasMatrix(); }
    GINLINE bool HasCxform() const      { return Flags.HasCxform(); }
    GINLINE bool HasBlendMode() const   { return Flags.HasBlendMode(); }
    GINLINE bool HasFilters() const     { return Flags.HasFilters(); }
    GINLINE bool HasDepth() const       { return Flags.HasDepth(); }
    GINLINE bool HasClipDepth() const   { return Flags.HasClipDepth(); }
    GINLINE bool HasRatio() const       { return Flags.HasRatio(); }
    GINLINE bool HasCharacterId() const { return Flags.HasCharacterId(); }
};


// Execute tags include things that control the operation of
// the GFxSprite.  Essentially, these are the events associated with a frame.
class GASExecuteTag //: public GNewOverrideBase
{
public:

#ifdef GFC_BUILD_DEBUG    
    GASExecuteTag();
    virtual ~GASExecuteTag();
#else
    virtual ~GASExecuteTag() {}
#endif    
    
    static void     LoadData(GFxStream*  pin, UByte* pdata, UPInt datalen, UPInt offsetInBytes = 0) 
    {  
        pin->ReadToBuffer(pdata + offsetInBytes, (UInt)datalen); 
    }

    virtual void    Execute(GFxSprite* m)                           { GUNUSED(m); }
    virtual void    ExecuteWithPriority(GFxSprite* m, GFxActionPriority::Priority prio) { GUNUSED(prio);  Execute(m); }

    virtual bool    IsRemoveTag() const { return false; }
    virtual bool    IsActionTag() const { return false; }
    virtual bool    IsInitImportActionsTag() const { return false;  }

    // A combination of ResourceId and depth - used to identify a tag created
    // character in a display list.
    struct DepthResId
    {
        SInt                Depth;
        GFxResourceId       Id;

        DepthResId() { Depth = 0; }
        DepthResId(GFxResourceId rid, SInt depth) : Depth(depth), Id(rid) { }
        DepthResId(const DepthResId& src) : Depth(src.Depth), Id(src.Id) { }
        inline DepthResId& operator = (const DepthResId& src) { Id = src.Id; Depth = src.Depth; return *this; }
        // Comparison - used during searches.
        inline bool operator == (const DepthResId& other) const { return (Id == other.Id) && (Depth == other.Depth); }
        inline bool operator != (const DepthResId& other) const { return !operator == (other); }
    };

    virtual void AddToTimelineSnapshot(GFxTimelineSnapshot*, UInt) {}
    virtual void Trace(const char*) {}
};


//
// GFxPlaceObjectBase
//
// Base interface for place object execute tags
//
class GFxPlaceObjectBase : public GASExecuteTag
{
public:
    typedef GArrayLH<GFxSwfEvent*, GFxStatMD_Tags_Mem> EventArrayType;

    enum PlaceActionType
    {
        Place_Add       = 0x00,
        Place_Move      = 0x01,
        Place_Replace   = 0x02,
    };

    // Matrices, depth and character index.
    struct UnpackedData
    {
        GFxCharPosInfo      Pos;
        EventArrayType*     pEventHandlers;
        const char*         Name;
        PlaceActionType     PlaceType;
    };

    virtual void                Unpack(UnpackedData& data) = 0;
    virtual EventArrayType*     UnpackEventHandlers() { return NULL; }
    virtual GFxCharPosInfoFlags GetFlags() const = 0;
};


//
// GFxPlaceObjectUnpacked
//
// Unpacked version of GFxPlaceObject. Used for placing images on stage
// (loadMovie for images)
//
class GFxPlaceObjectUnpacked : public GFxPlaceObjectBase
{
private:
    GFxPlaceObjectUnpacked(const GFxPlaceObjectUnpacked& o);
    GFxPlaceObjectUnpacked& operator=(const GFxPlaceObjectUnpacked& o);

public: 
    // Unpacked matrices, depth and character index.
    GFxCharPosInfo      Pos;

    // Constructors
    GFxPlaceObjectUnpacked() {};
    ~GFxPlaceObjectUnpacked() {};

    // *** GFxPlaceObjectBase implementation
    void        Unpack(GFxPlaceObjectBase::UnpackedData& data) 
    { 
        data.Name = NULL;
        data.pEventHandlers = NULL;
        data.PlaceType = GFxPlaceObjectBase::Place_Add;
        data.Pos = Pos; 
    }
    GFxCharPosInfoFlags GetFlags() const
    {
        return Pos.Flags;
    }

    // *** GASExecuteTag implementation
    // Place/move/whatever our object in the given pMovie.
    void        Execute(GFxSprite* m);
    void        AddToTimelineSnapshot(GFxTimelineSnapshot*, UInt frame);
    void        Trace(const char* str);

    // *** Custom
    // Initialize to a specified value (used when generating MovieDataDef for images).
    void        InitializeToAdd(const GFxCharPosInfo& posInfo)  { Pos     = posInfo; }

};

//
// GFxPlaceObject
//
// SWF1 PlaceObject record in packed form.
//
class GFxPlaceObject : public GFxPlaceObjectBase
{
private:
    GFxPlaceObject(const GFxPlaceObject& o);
    GFxPlaceObject& operator=(const GFxPlaceObject& o);

public: 
    bool            HasCxForm;   // Cxform is optional
    UByte           pData[1];

    // Constructors
    GFxPlaceObject();
    ~GFxPlaceObject();

    static UPInt GSTDCALL ComputeDataSize(GFxStream* pin);
    void            CheckForCxForm(UPInt dataSz);

    // *** GFxPlaceObjectBase implementation
    void            Unpack(GFxPlaceObjectBase::UnpackedData& data);
    GFxCharPosInfoFlags GetFlags() const;

    // *** GASExecuteTag implementation
    // Place/move/whatever our object in the given pMovie.
    void            Execute(GFxSprite* m);
    void            AddToTimelineSnapshot(GFxTimelineSnapshot*, UInt frame);
    void            Trace(const char* str);    

protected:
    UInt16          GetDepth() const;
};


//
// GFxPlaceObject2
//
// >=SWF6 PlaceObject2 record in packed form.
//
class GFxPlaceObject2 : public GFxPlaceObjectBase
{
private:
    GFxPlaceObject2(const GFxPlaceObject2& o);
    GFxPlaceObject2& operator=(const GFxPlaceObject2& o);
public: 
   
    // Bit packings for tag bytes.
    enum PlaceObject2Flags
    {
        PO2_HasActions      = 0x80,
        PO2_HasClipBracket  = 0x40,
        PO2_HasName         = 0x20,
        PO2_HasRatio        = 0x10,
        PO2_HasCxform       = 0x08,
        PO2_HasMatrix       = 0x04,
        PO2_HasChar         = 0x02,
        PO2_FlagMove        = 0x01
    };

    UByte               pData[1];
   
    // Constructors
    GFxPlaceObject2() {}
    ~GFxPlaceObject2();

    // *** Helper methods for packed event handler pointer
    static bool GSTDCALL HasEventHandlers(GFxStream* pin);
    // Move flags to the beginning and initialize packed event handler pointer
    static void GSTDCALL RestructureForEventHandlers(UByte* pdata);
    // Get/Set the packed event handler pointer
    static EventArrayType* GSTDCALL GetEventHandlersPtr(UByte* pdata);
    static void GSTDCALL SetEventHandlersPtr(UByte* pdata, EventArrayType* peh);

    static UPInt GSTDCALL ComputeDataSize(GFxStream* pin, UInt movieVersion);

    // *** GFxPlaceObjectBase implementation
    void                Unpack(GFxPlaceObjectBase::UnpackedData& data) { UnpackBase(data, 6); }
    EventArrayType*     UnpackEventHandlers();
    GFxCharPosInfoFlags GetFlags() const;

    // *** GASExecuteTag implementation
    // Place/move/whatever our object in the given pMovie.
    void                Execute(GFxSprite* m) { ExecuteBase(m, 6); }
    void                AddToTimelineSnapshot(GFxTimelineSnapshot* snapshot, UInt frame);
    void                Trace(const char* str) { TraceBase(str, 6); }

protected:
    UInt16              GetDepth() const;
    PlaceActionType     GetPlaceType() const;

    void                ExecuteBase(GFxSprite* m, UInt8 version);
    void                AddToTimelineSnapshotBase(GFxTimelineSnapshot*, UInt frame, UInt8 version);
    void                TraceBase(const char* str, UInt8 version);

    void                UnpackBase(GFxPlaceObjectBase::UnpackedData& data, UInt8 version);
};

//
// < SWF6 PlaceObject2 record in packed form.
//
// Some fields are 16-bit instead of 32 in the regular version
//
class GFxPlaceObject2a : public GFxPlaceObject2
{
private:
    GFxPlaceObject2a(const GFxPlaceObject2a& o);
    GFxPlaceObject2a& operator=(const GFxPlaceObject2a& o);
public:
    GFxPlaceObject2a() {}

    // *** GFxPlaceObjectBase implementation
    void                Unpack(GFxPlaceObjectBase::UnpackedData& data) { UnpackBase(data, 3); }

    // *** GASExecuteTag implementation
    // Place/move/whatever our object in the given pMovie.
    void        Execute(GFxSprite* m) { ExecuteBase(m, 3); }
    void        Trace(const char* str) { TraceBase(str, 3); }
};

//
// GFxPlaceObject3
//
// SWF8 PlaceObject3 record in packed form.
//
class GFxPlaceObject3 : public GFxPlaceObjectBase
{
private:
    GFxPlaceObject3(const GFxPlaceObject3& o);
    GFxPlaceObject3& operator=(const GFxPlaceObject3& o);
public: 

    enum PlaceObject3Flags
    {
        PO3_BitmapCaching   = 0x04,
        PO3_HasBlendMode    = 0x02,
        PO3_HasFilters      = 0x01,
    };

    UByte               pData[1];
    
    // Constructors
    GFxPlaceObject3() {}
    ~GFxPlaceObject3();

    static UPInt GSTDCALL ComputeDataSize(GFxStream* pin);

    // *** GFxPlaceObjectBase implementation
    void                Unpack(GFxPlaceObjectBase::UnpackedData& data);
    EventArrayType*     UnpackEventHandlers();
    GFxCharPosInfoFlags GetFlags() const;

    // *** GASExecuteTag implementation
    // Place/move/whatever our object in the given pMovie.
    void                Execute(GFxSprite* m);
    void                AddToTimelineSnapshot(GFxTimelineSnapshot*, UInt frame);
    void                Trace(const char* str);

protected:
    UInt16              GetDepth() const;
    PlaceActionType     GetPlaceType() const;
};


class GFxRemoveObject : public GASExecuteTag
{
public:
    UInt16          Id;
    UInt16          Depth;

    // *** GASExecuteTag implementation

    void            Read(GFxLoadProcess* p);

    virtual void    Execute(GFxSprite* m);

    virtual bool    IsRemoveTag() const { return true; }

    virtual void    AddToTimelineSnapshot(GFxTimelineSnapshot*, UInt frame);
    void            Trace(const char* str);
};

class GFxRemoveObject2 : public GASExecuteTag
{
public:
    UInt16          Depth;

    // *** GASExecuteTag implementation

    void            Read(GFxLoadProcess* p);

    virtual void    Execute(GFxSprite* m);

    virtual bool    IsRemoveTag() const{ return true; }

    virtual void    AddToTimelineSnapshot(GFxTimelineSnapshot*, UInt frame);
    void            Trace(const char* str);
};


class GFxSetBackgroundColor : public GASExecuteTag
{
public:
    GColor  Color;

    void    Execute(GFxSprite* m);

    void    ExecuteState(GFxSprite* m)
        { Execute(m); }

    void    Read(GFxLoadProcess* p);   
};



//
// GFxInitImportActions
//

// GFxInitImportActions - created for import tags.
class GFxInitImportActions : public GASExecuteTag
{
    UInt        ImportIndex;
public:
   
    GFxInitImportActions()
    {
        ImportIndex = 0;
    }
    void            SetImportIndex(UInt importIndex)
    {
        ImportIndex = importIndex;
    }

    // Queues up logic to execute InitClip actions from the import,
    // using Highest priority by default.
    virtual void    Execute(GFxSprite* m);  

    // InitImportActions that come from imported files need to be executed
    // in the MovieDefImpl binding context (otherwise we would index parent's
    // source movies incorrectly, resulting in recursive loop).
    void            ExecuteInContext(GFxSprite* m, GFxMovieDefImpl *pbindDef, bool recursiveCheck = 1);  

    virtual bool    IsInitImportActionsTag() const { return true; }
};



// ** Inline Implementation

// ** End Inline Implementation


#endif // INC_GFXIMPL_H
