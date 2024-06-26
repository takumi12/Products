/**********************************************************************

Filename    :   GFxLoadProcess.ccp
Content     :   GFxLoadProcess - tracks loading and binding state.
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

#include "GFxLoadProcess.h"

#include "GJPEGUtil.h"
#include "GFile.h"
#include "GFxAudio.h"

// For GFxMovieRoot::IsPathAbsolute
//#include "GFxPlayerImpl.h"



// ***** GFxLoadStates


GFxLoadStates::GFxLoadStates() : ThreadedLoading(false) 
{ 
    //printf("GFxLoadStates::GFxLoadStates : %x, thread : %d\n", this, GetCurrentThreadId());    
}

GFxLoadStates::GFxLoadStates(GFxLoaderImpl* pimpl, GFxStateBag* pstates,
                             GFxMovieDefBindStates *pbindStates)
                             : ThreadedLoading(false)
{
    //printf("GFxLoadStates::GFxLoadStates : %x, thread : %d\n", this, GetCurrentThreadId());

    pLoaderImpl         = pimpl;
    pWeakResourceLib    = pimpl->GetWeakLib();

    // If pstates is null, states comde from loader.
    if (!pstates)
        pstates = pimpl;

    // Capture states.
    // We may override bind states when loading fonts from FontLib.
    pBindStates         = pbindStates ? *new GFxMovieDefBindStates(pbindStates) :
                                        *new GFxMovieDefBindStates(pstates);
    pLog                = pstates->GetLog();
    pParseControl       = pstates->GetParseControl();
    pProgressHandler    = pstates->GetProgressHandler();
    pFontCacheManager   = pstates->GetFontCacheManager();
    pTaskManager        = pstates->GetTaskManager();
    // Record render config so that it can be used for image creation during loading.
    pRenderConfig       = pstates->GetRenderConfig();
    pJpegSupport        = pstates->GetJpegSupport();
    pZlibSupport        = pstates->GetZlibSupport();
    pPNGSupport         = pstates->GetPNGSupport();
#ifndef GFC_NO_VIDEO
    pVideoPlayerState   = pstates->GetVideo();
#endif
#ifndef GFC_NO_SOUND
    pAudioState         = pstates->GetAudio();
#endif
}

GFxLoadStates::~GFxLoadStates()
{
    //printf("GFxLoadStates::~GFxLoadStates : %x, thread : %d\n", this, GetCurrentThreadId());
}

// Obtains an image creator, but only if image data is not supposed to
// be preserved; considers GFxLoader::LoadKeepBindData flag from arguments.
GFxImageCreator* GFxLoadStates::GetLoadTimeImageCreator(UInt loadConstants) const
{
    // Image creator is only used as a key if it is bound to, as specified
    // by flags and IsKeepingImageData member.
    GFxImageCreator* pkeyImageCreator = 0;

    if (!(loadConstants & GFxLoader::LoadKeepBindData) &&
        pBindStates->pImageCreator &&
        !pBindStates->pImageCreator->IsKeepingImageData())
    {
        pkeyImageCreator = pBindStates->pImageCreator.GetPtr();
    }

    return pkeyImageCreator;
}

// Initializes the relative path.
void        GFxLoadStates::SetRelativePathForDataDef(GFxMovieDataDef* pdef)
{    
    GASSERT(pdef != 0);

    // Extract and store relative path from parent def.
    RelativePath = pdef->GetFileURL();    
    if (!GFxURLBuilder::ExtractFilePath(&RelativePath))
        RelativePath.Clear();
}

// Implementation that allows us to override the log.
GFile*      GFxLoadStates::OpenFile(const char *pfilename, UInt loadConstants)
{    
    if (!pBindStates->pFileOpener)
    {             
        // Don't even have a way to open the file.
        if (pLog && !(loadConstants & GFxLoader::LoadQuietOpen))
            pLog->LogError(
            "Error: GFxLoader failed to open '%s', GFxFileOpener not installed\n",
            pfilename);
        return 0;
    }

    return pBindStates->pFileOpener->OpenFileEx(pfilename, (loadConstants & GFxLoader::LoadQuietOpen)? 0 : pLog);
}

// Perform filename translation and/or copy by relying on translator.
void    GFxLoadStates::BuildURL(GString *pdest, const GFxURLBuilder::LocationInfo &loc) const
{
    GASSERT(pdest);
    GFxURLBuilder* pbuilder = pBindStates->pURLBulider;

    if (!pbuilder)
        GFxURLBuilder::DefaultBuildURL(pdest, loc);
    else
        pbuilder->BuildURL(pdest, loc);
}

// Helper that clones load states, pBindSates.
// The only thing left un-copied is GFxMovieDefBindStates::pDataDef
GFxLoadStates*  GFxLoadStates::CloneForImport() const
{    
    GPtr<GFxMovieDefBindStates> pnewBindStates = *new GFxMovieDefBindStates(pBindStates);
    GFxLoadStates*              pnewStates = new GFxLoadStates;

    if (pnewStates)
    {
        pnewStates->pBindStates       = pnewBindStates;
        pnewStates->pLoaderImpl       = pLoaderImpl;
        pnewStates->pLog              = pLog;
        pnewStates->pProgressHandler  = pProgressHandler;
        pnewStates->pTaskManager      = pTaskManager;
        pnewStates->pFontCacheManager = pFontCacheManager;
        pnewStates->pParseControl     = pParseControl;
        pnewStates->pWeakResourceLib  = pWeakResourceLib;
        pnewStates->pJpegSupport      = pJpegSupport;
        pnewStates->pZlibSupport      = pZlibSupport;
        pnewStates->pPNGSupport       = pPNGSupport;
        // Must copy RenderConfig, even though it's not a binding state.
        pnewStates->pRenderConfig     = pRenderConfig;
#ifndef GFC_NO_SOUND
        pnewStates->pAudioState       = pAudioState;
#endif
#ifndef GFC_NO_VIDEO
        pnewStates->pVideoPlayerState = pVideoPlayerState;
#endif
    }
    return pnewStates;
}

bool GFxLoadStates::SubmitBackgroundTask(GFxLoaderTask* ptask)
{
    if (!pTaskManager)
        return false;
    //    pLoaderImpl->RegisterLoadProcess(ptask);
    if (!pTaskManager->AddTask(ptask))
    {
        //        pLoaderImpl->UnRegisterLoadProcess(ptask);
        return false;
    }
    return true;
}



// ***** GFxLoadProcess


GFxLoadProcess::GFxLoadProcess(GFxMovieDataDef* pdataDef,
                               GFxLoadStates *pstates, UInt loadFlags)
    : GFxLoaderTask(pstates, Id_MovieDataLoad), ProcessInfo(pdataDef->GetHeap())
{
#ifdef GFC_DEBUG_COUNT_TAGS
    TagCounts.Add(GFxTag_PlaceObject,           TagCountType("PlaceObject", sizeof(GFxPlaceObject)));
    TagCounts.Add(GFxTag_PlaceObject2,          TagCountType("PlaceObject2", sizeof(GFxPlaceObject2)));
    TagCounts.Add(GFxTag_PlaceObject3,          TagCountType("PlaceObject3", sizeof(GFxPlaceObject3)));
    TagCounts.Add(GFxTag_SetBackgroundColor,    TagCountType("SetBackgroundColor", sizeof(GFxSetBackgroundColor)));
    TagCounts.Add(GFxTag_RemoveObject,          TagCountType("RemoveObject", sizeof(GFxRemoveObject)));
    TagCounts.Add(GFxTag_RemoveObject2,         TagCountType("RemoveObject2", sizeof(GFxRemoveObject2)));
    TagCounts.Add(GFxTag_DoAction,              TagCountType("DoAction", 0));
    TagCounts.Add(GFxTag_DoInitAction,          TagCountType("DoInitAction", 0));
#endif

    // Save states.
    //pStates             = pstates;
    // Cache parse flags for quick accesss.
    ParseFlags          = pstates->pParseControl.GetPtr() ?
                          pstates->pParseControl->GetParseFlags() : 0;

    // Initialize other data used during loading.    
    pLoadData           = pdataDef->pData;
    pDataDef_Unsafe     = pdataDef;  // For GFxSpriteDef argument only!
    pTimelineDef        = 0;

    pJpegIn             = 0;
    LoadFlags           = loadFlags;
    LoadState           = LS_LoadingRoot;

    // Avoid resizing frame arrays to 0 (extra allocations).     // Anachronism
    //FrameTags[0].SetSizePolicy(GArrayPolicy::Buffer_NoShrink);
    //FrameTags[1].SetSizePolicy(GArrayPolicy::Buffer_NoShrink);
    //InitActionTags.SetSizePolicy(GArrayPolicy::Buffer_NoShrink);

    ImportIndex         = 0;
    ImportDataCount     = 0;
    ResourceDataCount   = 0;
    FontDataCount       = 0;
    // Clear linked lists.
    pImportData   = pImportDataLast   = 0;
    pResourceData = pResourceDataLast = 0;
    pFontData     = pFontDataLast     = 0;

    pAltStream          = NULL;
    pTempBindData       = 0;
}

GFxLoadProcess::~GFxLoadProcess()
{
#ifdef GFC_DEBUG_COUNT_TAGS
    GFC_DEBUG_MESSAGE(1,    ">");
    GFC_DEBUG_MESSAGE1(1,   "> Loading %s..",FilePath.ToCStr());
    GHashIdentity<GFxTagType, TagCountType>::ConstIterator iter = TagCounts.Begin();
    for (; iter != TagCounts.End(); ++iter)
    {
        const TagCountType& count = iter->Second;
        GFC_DEBUG_MESSAGE3(1,   "> %s loaded: %d  (CSZ: %db)", count.Name, count.Count, count.ClassSize);
    }
    GFC_DEBUG_MESSAGE(1,    ">");
#endif

    // Clear JPEG loader if it was used.
    if (pJpegIn)
        delete pJpegIn;    

    // Notify any waiters that our task thread is done with
    // all of its references to loaded data.
#ifndef GFC_NO_THREADSUPPORT
    GPtr<GFxLoadUpdateSync> ploadSync = pLoadData->GetFrameUpdateSync();
#endif
    ProcessInfo.ShutDown();
    pLoadData.Clear();
    pBindProcess.Clear();   
#ifndef GFC_NO_THREADSUPPORT 
    ploadSync->NotifyLoadFinished();
#endif
}


bool    GFxLoadProcess::BeginSWFLoading(GFile *pfile)
{
#ifdef GFC_DEBUG_COUNT_TAGS
    FilePath = pfile->GetFilePath();
#endif
    // Read header while logging messages as appropriate, fail on wrong file format.
    if (!ProcessInfo.Initialize(pfile, GetLog(), pLoadStates->GetZlibSupport(), pLoadStates->pParseControl, true))
        return false;
    pLoadData->BeginSWFLoading(ProcessInfo.Header);
    return true;
}


void    GFxLoadProcess::ReadRgbaTag(GColor *pc, GFxTagType tagType)
{
    if (tagType <= GFxTag_DefineShape2)
        GetStream()->ReadRgb(pc);
    else
        GetStream()->ReadRgba(pc);
}


GFxTimelineDef::Frame GFxLoadProcess::TagArrayToFrame(ExecuteTagArrayType &tagArray)
{
    GFxTimelineDef::Frame frame;

    if (tagArray.GetSize())
    {
        UPInt memSize = sizeof(GASExecuteTag*) * tagArray.GetSize();

        if ((frame.pTagPtrList = (GASExecuteTag**)AllocTagMemory(memSize)) != 0)
        {
            memcpy(frame.pTagPtrList, &tagArray[0], memSize);
            frame.TagCount = (UInt)tagArray.GetSize();
        }
        tagArray.Clear();
    }

    return frame;
}

// Apply frame tags that have been accumulated to the MovieDef.
void    GFxLoadProcess::CommitFrameTags()
{
    // Add frame to time-line. Timeline can be either MovieDataDef or SpriteDef.
    if (LoadState == LS_LoadingSprite)
    {
        GASSERT(pTimelineDef);
        pTimelineDef->SetLoadingPlaylistFrame(TagArrayToFrame(FrameTags[LoadState]));
    }
    else
    {
        pLoadData->SetLoadingPlaylistFrame(TagArrayToFrame(FrameTags[LoadState]));
        pLoadData->SetLoadingInitActionFrame(TagArrayToFrame(InitActionTags));
    }
}

// Cleans up frame tags; only used if loading was canceled.
void  GFxLoadProcess::CleanupFrameTags()
{    
    UPInt i;
    for(i=0; i<FrameTags[LS_LoadingSprite].GetSize(); i++)
        FrameTags[LS_LoadingSprite][i]->~GASExecuteTag();
    for(i=0; i<FrameTags[LS_LoadingRoot].GetSize(); i++)
        FrameTags[LS_LoadingRoot][i]->~GASExecuteTag();
    for(i=0; i<InitActionTags.GetSize(); i++)
        InitActionTags[i]->~GASExecuteTag();

    FrameTags[LS_LoadingSprite].Clear();
    FrameTags[LS_LoadingRoot].Clear();
    InitActionTags.Clear();
}


// Adds a new resource and generates a handle for it.
GFxResourceHandle   GFxLoadProcess::AddDataResource(GFxResourceId rid, const GFxResourceData &resData)
{
    // We get back a handle with a new bind index.
    GFxResourceHandle               rh = pLoadData->AddNewResourceHandle(rid);
    GFxMovieDataDef::DefBindingData& bd = pLoadData->BindData;

    GFxResourceDataNode* pnode =  pLoadData->AllocMovieDefClass<GFxResourceDataNode>();
    if (pnode)
    {
        pnode->Data      = resData;
        pnode->BindIndex = rh.GetBindIndex();

        // Initialize beginnings of local and global lists.
        if (!pResourceData)
            pResourceData = pnode;

        // Insert data in the end of the global list.
        if (!bd.pResourceNodes)
            bd.pResourceNodes = pnode;
        else        
            bd.pResourceNodesLast->pNext = pnode;
        bd.pResourceNodesLast = pnode;

        ResourceDataCount++;
    }
    return rh;
}

static GFxFontResourceCreator static_inst;
GFxResourceHandle   GFxLoadProcess::AddFontDataResource(GFxResourceId rid, GFxFont* pfontData)
{   
    // We get back a handler with bind index.
    GFxResourceData     resData(&static_inst, pfontData);
    GFxResourceHandle   rh = AddDataResource(rid, resData);    

    GFxFontDataUseNode* pfnode = pLoadData->AllocMovieDefClass<GFxFontDataUseNode>();
    if (pfnode)
    {
        GFxMovieDataDef::DefBindingData& bd = pLoadData->BindData;

        pfnode->Id        = rid;
        pfnode->pFontData = pfontData;
        pfnode->BindIndex = rh.GetBindIndex();

        // Initialize this node as the beginning of the list.
        if (!pFontData)
            pFontData = pfnode;

        // Insert data in the end of the global list.
        if (!bd.pFonts)
            bd.pFonts = pfnode;
        else        
            bd.pFontsLast->pNext = pfnode;
        bd.pFontsLast = pfnode;

        FontDataCount++;
    }

    return rh;
}

void    GFxLoadProcess::AddImportData(GFxImportData* pnode)
{
    pnode->ImportIndex = ImportIndex;
    ImportIndex++;

    // Insert data node in a list.
    if (!pImportData)
        pImportData = pnode;

    GFxMovieDataDef::DefBindingData& bd = pLoadData->BindData;

    if (!bd.pImports)
        bd.pImports = pnode;
    else
        bd.pImportsLast->pNext = pnode;
    bd.pImportsLast = pnode;

    ImportDataCount++;
}


// Add a dynamically-loaded image resource, with unique key.
// This is normally used for SWF embedded images.
void    GFxLoadProcess::AddImageResource(GFxResourceId rid, GImage *pimage)
{    
    GFxImageCreator* pkeyImageCreator = pLoadStates->GetLoadTimeImageCreator(LoadFlags);

    // Image creator is only used as a key if it is bound to, based on flags.
    if (pkeyImageCreator)
    {
        GFxImageCreateInfo   icreateInfo;
        GPtr<GImageInfoBase> pimageInfo;

        icreateInfo.pHeap = GetLoadHeap();

        icreateInfo.SetStates(0, pLoadStates->GetRenderConfig(),
                              pLoadStates->GetLog(),pLoadStates->GetJpegSupport(), pLoadStates->GetPNGSupport());
        icreateInfo.ThreadedLoading = pLoadStates->IsThreadedLoading();

        if (pimage)
            icreateInfo.SetImage(pimage);
        if (GetBindStates()->pImageCreator)
            pimageInfo= *GetBindStates()->pImageCreator->CreateImage(icreateInfo);

        GPtr<GFxImageResource> pch = *GHEAP_NEW(icreateInfo.pHeap) GFxImageResource(pimageInfo.GetPtr());
        AddResource(rid, pch);
    }
    else
    {
        // Creation of image resource is delayed due to late binding.
        // Create image resource data instead. This data will be used during binding.
        GFxResourceData rdata = GFxImageResourceCreator::CreateImageResourceData(pimage);
        AddDataResource(rid, rdata);
    }
}


// Creates a frame binding object; should be called only when
// a root frame is finished. Clears the internal import/font/resource lists.
GFxFrameBindData* GFxLoadProcess::CreateFrameBindData()
{
    GFxFrameBindData* pbdata = pLoadData->AllocMovieDefClass<GFxFrameBindData>();
    if (pbdata)
    {
        // Frame and Bytes loaded will be assigned by the caller.
        // pbdata->Frame           = ?;
        // pbdata->FrameBytesLoaded= ?;
        pbdata->ImportCount     = ImportDataCount;
        pbdata->pImportData     = pImportData;
        pbdata->FontCount       = FontDataCount;
        pbdata->pFontData       = pFontData;
        pbdata->ResourceCount   = ResourceDataCount;
        pbdata->pResourceData   = pResourceData;

        ImportDataCount         = 0;
        ResourceDataCount       = 0;
        FontDataCount           = 0;

        // Clear data pointers. We do not clear the data Last pointers,
        // allowing a full list of imports to be built up in the MovieDef.
        pImportData     = 0;
        pResourceData   = 0;
        pFontData       = 0;
    }
    return pbdata;
}



// ***** GFxImageResourceCreator - Image creator from files

// Creates/Loads resource based on data and loading process
bool    GFxImageFileResourceCreator::CreateResource(DataHandle hdata, GFxResourceBindData *pbindData,
                                                    GFxLoadStates *pls, GMemoryHeap *pbindHeap) const
{
    GFxImageFileInfo *prfi = (GFxImageFileInfo*) hdata;
    GASSERT(prfi);   
    GUNUSED(pbindHeap); // Don't use bindHeap for shared resources.


    // Make a new fileInfo so that it can have a modified filename.   
    GPtr<GFxImageFileInfo> pimageFileInfo = *GNEW GFxImageFileInfo(*prfi);

    // Ensure format
    // TBD: If we always save the format for texture tags in gfxexport we
    // would not need to do this. Verify.
    if ((pimageFileInfo->Format == GFxLoader::File_Unknown) && pimageFileInfo->pExporterInfo)
        pimageFileInfo->Format =
        (GFxFileConstants::FileFormatType) pimageFileInfo->pExporterInfo->Format;

    // Translate filename.
    GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_ImageImport,
                                    prfi->FileName, pls->GetRelativePath());
    pls->BuildURL(&pimageFileInfo->FileName, loc);


    // Now that we have a correct file object we need to match it against
    // the library so as to check if is is already loaded
    GMemoryHeap*   pimageHeap = pls->GetLib()->GetImageHeap();
    GFxResourceKey imageKey =
        GFxImageResource::CreateImageFileKey(pimageFileInfo, 
                                             pls->GetFileOpener(),
                                             pls->GetBindStates()->pImageCreator,
                                             pimageHeap);

    GString                   errorMessage;
    GFxResourceLib::BindHandle  bh;
    GPtr<GFxImageResource>      pimageRes = 0;

    if (pls->GetLib()->BindResourceKey(&bh, imageKey) == GFxResourceLib::RS_NeedsResolve)
    {
        // If not found, create an object of the right type.
        GFxImageCreateInfo ico;
        ico.SetType(GFxImageCreateInfo::Input_File);
        ico.Use         = prfi->Use;
        if (prfi->Flags & GFxImageFileInfo::Wrappable)
            ico.SetTextureUsage(GTexture::Usage_Wrap);
        else
            ico.SetTextureUsage (0);
        ico.pHeap       = pimageHeap;
        ico.pFileInfo   = pimageFileInfo.GetPtr();
        ico.SetStates(pls->GetFileOpener(), pls->GetRenderConfig(),
                      pls->GetLog(), pls->GetJpegSupport(),pls->GetPNGSupport());
        ico.ThreadedLoading = pls->IsThreadedLoading();
        ico.pExportName = prfi->ExportName.ToCStr();

        GPtr<GImageInfoBase> pimage;
        GFxImageCreator*     pcreator = pls->GetBindStates()->pImageCreator;

        if (pcreator)
            pimage = *pcreator->CreateImage(ico);
        else
            GFC_DEBUG_WARNING(1, "Image resource creation failed - GFxImageCreator not installed");        

        if (pimage)
            pimageRes = *GHEAP_NEW(ico.pHeap) GFxImageResource(pimage.GetPtr(), imageKey, ico.Use);
        // Need to read header first.
        if (pimageRes)
            bh.ResolveResource(pimageRes);
        else
        {
            errorMessage = "Failed to load image '";
            errorMessage += pimageFileInfo->FileName;
            errorMessage += "'";

            bh.CancelResolve(errorMessage.ToCStr());
        }
    }
    else
    {
        // If Available and Waiting resources will be resolved here.
        if ((pimageRes = *(GFxImageResource*)bh.WaitForResolve()).GetPtr() == 0)
        {
            errorMessage = bh.GetResolveError();
        }
    }

    // If there was an error, display it
    if (!pimageRes)
    {
        pls->pLog->LogError("Error: %s\n", errorMessage.ToCStr());
        return 0;
    }

    // Pass resource ownership to BindData.
    pbindData->pResource = pimageRes;
    return 1;    
}


GFxResourceData GFxImageFileResourceCreator::CreateImageFileResourceData(GFxImageFileInfo * prfi)
{
    static GFxImageFileResourceCreator inst;
    return GFxResourceData(&inst, prfi);
}



// Creates/Loads resource based on GImage during binding process
bool    GFxImageResourceCreator::CreateResource(DataHandle hdata, GFxResourceBindData *pbindData,
                                                GFxLoadStates *pls, GMemoryHeap *pbindHeap) const
{
    GImage *pimage = (GImage*) hdata;
    GASSERT(pimage);

    // Create resource from image.
    GFxImageCreateInfo   icreateInfo(pimage, GFxResource::Use_None);
    icreateInfo.SetStates(0, pls->GetRenderConfig(), pls->GetLog(),
                          pls->GetJpegSupport(),pls->GetPNGSupport());
    icreateInfo.ThreadedLoading = pls->IsThreadedLoading();
    icreateInfo.pHeap           = pbindHeap;

    GPtr<GImageInfoBase> pimageInfo;
    GFxImageCreator*     pcreator = pls->GetBindStates()->pImageCreator;
    if (!pcreator)
    {
        GFC_DEBUG_WARNING(1, "Image resource creation failed - GFxImageCreator not installed");
        return 0;
    }
    if (pcreator)    
        pimageInfo= *pcreator->CreateImage(icreateInfo);
    if (!pimageInfo)
        return 0;

    GPtr<GFxImageResource> pimageRes = *GHEAP_NEW(pbindHeap) GFxImageResource(pimageInfo.GetPtr());
    if (!pimageRes)
        return 0;

    // Pass resource ownership to BindData.
    pbindData->pResource = pimageRes;
    return 1;    
}

GFxResourceData GFxImageResourceCreator::CreateImageResourceData(GImage* pimage)
{
    static GFxImageResourceCreator inst;
    //!AB: pimage might be NULL if ZLIB is not linked and zlib-compresed image is found.
    if (pimage)
        return GFxResourceData(&inst, pimage);
    else
        return GFxResourceData();
}


bool    GFxSubImageResourceCreator::CreateResource(DataHandle hdata, GFxResourceBindData *pbindData,
                                                   GFxLoadStates *pls, GMemoryHeap *pbindHeap) const
{
    GFxSubImageResourceInfo* pinfo = (GFxSubImageResourceInfo*)hdata;
    GASSERT(pinfo); 
    GUNUSED(pls);

    GFxImageResource* pimageRes = pinfo->Image;

    if (!pimageRes)
    {
        GFxResourceHandle rh;
        pbindData->pBinding->GetOwnerDefImpl()->GetDataDef()->GetResourceHandle(&rh, pinfo->ImageId);
        GFxResource*      pres = rh.GetResource(pbindData->pBinding);
        if (pres && pres->GetResourceType() == GFxResource::RT_Image)
            pimageRes = (GFxImageResource*)pres;
    }

    if (pimageRes)
    {
        GImageInfoBase* pbaseimage = pimageRes->GetImageInfo();
        if (pbaseimage->GetImageInfoType() == GImageInfoBase::IIT_ImageInfo)
            ((GImageInfo*)pbaseimage)->SetTextureUsage(0);
        pbindData->pResource = 
            *GHEAP_NEW(pbindHeap) GFxSubImageResource(pimageRes, GFxResourceId(0),
                                                      pinfo->Rect, pbindHeap);
        return 1;
    }

    return 0;
}

GFxResourceData GFxSubImageResourceCreator::CreateSubImageResourceData(GFxSubImageResourceInfo* pinfo)
{
    static GFxSubImageResourceCreator inst;
    return GFxResourceData(&inst, pinfo);
}


// ***** GFxFontResourceCreator - Font resource creator to use during binding


// Creates/Loads resource based on data and loading process.
bool   GFxFontResourceCreator::CreateResource(DataHandle hdata, GFxResourceBindData *pbindData,
                                              GFxLoadStates *pls, GMemoryHeap *pbindHeap) const
{

    GFxFont *pfd = (GFxFont*) hdata;
    GASSERT(pfd);

    // 1) Traverse through potential substitute fonts to see if substitution
    //    needs to take place.

    // FontResourceCreator may be responsible for substituting glyphs from
    // other files with '_glyphs' in their name.
    GArray<GPtr<GFxMovieDefImpl> >& fontDefs = pls->SubstituteFontMovieDefs;

    for(UInt ifontDef=0; ifontDef<fontDefs.GetSize(); ifontDef++)
    {
        GFxMovieDefImpl*    pdefImpl = fontDefs[ifontDef];
        GFxFontDataUseNode* psourceFont = 
            pdefImpl->GetDataDef()->GetFirstFont();

        // Search imports for the font with the same name which has glyphs.
        for(; psourceFont != 0; psourceFont = psourceFont->pNext)
        {
            GFxFont *psourceFontData = psourceFont->pFontData;

            if (psourceFontData->GetGlyphShapeCount() > 0)
            {
                if (pfd->MatchSubstituteFont(psourceFontData))
                {                    
                    // Set our binding.
                    // Note: Unlike us, the source is guaranteed to be fully loaded when
                    // this takes place, so we can just look up its binding.
                    // Set binding together with its internal table.
                    pdefImpl->GetResourceBinding().
                        GetResourceData(pbindData, psourceFont->BindIndex);
                    return 1;
                }
            }
        }
    }

    // 2) Not substitution. Create a font based on our data.


    // First, try to create a system font if the shape has no glyphs.      
    if (!pfd->HasVectorOrRasterGlyphs() && pfd->GetName())
    {
        // Just set not resolved flag for the font. This will instruct 
        // FontManager to search this font through FontLib or FontProvider
        pfd->SetNotResolvedFlag();
    }

    // If this is not a system font, create one based on out FontData.
    if (!pbindData->pResource)
    {
        // Our GFxMovieDefImpl's pBinding should have been provided by caller.
        pbindData->pResource = *GHEAP_NEW(pbindHeap) GFxFontResource(pfd, pbindData->pBinding);
    }


    // We could do font glyph rendering here, however it is better to delay it
    // till the end of frame binding so that textures from several fonts
    // can be potentially combined.

    return pbindData->pResource ? 1 : 0;
}

