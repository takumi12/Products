/**********************************************************************

Filename    :   GFxLoader.cpp
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
#include "GDebug.h"
#include "GUTF8Util.h"

#include "GFxLoaderImpl.h"

#include "GImageInfo.h"
#include "GFxImageResource.h"
#include "GImage.h"

// Include GRenderConfig to support GFxImageCreator creating textures.
#include "GFxRenderConfig.h"

#include "GFxAmpServer.h"

#include "GHeapNew.h"



// ***** GFxURLBuilder implementation

// Default TranslateFilename implementation.
void    GFxURLBuilder::BuildURL(GString *ppath, const LocationInfo& loc)
{
    DefaultBuildURL(ppath, loc);  
}  

void    GFxURLBuilder::DefaultBuildURL(GString *ppath, const LocationInfo& loc)
{
    // Determine the file name we should use base on a relative path.
    if (IsPathAbsolute(loc.FileName))
        *ppath = loc.FileName;
    else
    {  
        // If not absolute, concatenate image path to the relative parent path.

        UPInt length = loc.ParentPath.GetSize();
        if (length > 0)
        {
            *ppath = loc.ParentPath;
            UInt32 cend = loc.ParentPath[length - 1];

            if ((cend != '\\') && (cend != '/'))
            {
                *ppath += "/";
            }
            *ppath += loc.FileName;
        }
        else
        {
            *ppath = loc.FileName;
        }
    }
}

// Static helper function used to determine if the path is absolute.
bool    GFxURLBuilder::IsPathAbsolute(const char *putf8str)
{

    // Absolute paths can star with:
    //  - protocols:        'file://', 'http://'
    //  - windows drive:    'c:\'
    //  - UNC share name:   '\\share'
    //  - unix root         '/'

    // On the other hand, relative paths are:
    //  - directory:        'directory/file'
    //  - this directory:   './file'
    //  - parent directory: '../file'
    // 
    // For now, we don't parse '.' or '..' out, but instead let it be concatenated
    // to string and let the OS figure it out. This, however, is not good for file
    // name matching in library/etc, so it should be improved.

    if (!putf8str || !*putf8str)
        return true; // Treat empty strings as absolute.    

    UInt32 charVal = GUTF8Util::DecodeNextChar(&putf8str);

    // Fist character of '/' or '\\' means absolute path.
    if ((charVal == '/') || (charVal == '\\'))
        return true;

    while (charVal != 0)
    {
        // Treat a colon followed by a slash as absolute.
        if (charVal == ':')
        {
            charVal = GUTF8Util::DecodeNextChar(&putf8str);
            // Protocol or windows drive. Absolute.
            if ((charVal == '/') || (charVal == '\\'))
                return true;
        }
        else if ((charVal == '/') || (charVal == '\\'))
        {
            // Not a first character (else 'if' above the loop would have caught it).
            // Must be a relative path.
            break;
        }

        charVal = GUTF8Util::DecodeNextChar(&putf8str);
    }

    // We get here for relative paths.
    return false;
}

// Modifies path to not include the filename, leaves trailing '/'.
bool    GFxURLBuilder::ExtractFilePath(GString *ppath)
{
    // And strip off the actual file name.
    SPInt length  = (SPInt)ppath->GetLength();
    SPInt i       = length-1;
    for (; i>=0; i--)
    {
        UInt32 charVal = ppath->GetCharAt(i);

        // The trailing name will end in either '/' or '\\',
        // so just clip it off at that point.
        if ((charVal == '/') || (charVal == '\\'))
        {
            *ppath = ppath->Substring(0, i+1);
            break;
        }
    }

    // Technically we can have extra logic somewhere for paths,
    // such as enforcing protocol and '/' only based on flags,
    // but we keep it simple for now.
    return (i >= 0);
}




// ***** Default Image Creator implementation

// Default implementation reads images from DDS/TGA files
// creating GImageInfo objects.
GImageInfoBase* GFxImageCreator::CreateImage(const GFxImageCreateInfo &info)
{   
    GPtr<GImage> pimage;
    UInt         twidth = 0, theight = 0;

    GMemoryHeap* pheap = info.pHeap ? info.pHeap : GMemory::GetGlobalHeap();

    switch(info.Type)
    {
    case GFxImageCreateInfo::Input_Image:
        pimage  = info.pImage;
        // We have to pass correct size; it is required at least
        // when we are initializing image info with a texture.
        twidth  = pimage->Width;
        theight = pimage->Height;
        break;

    case GFxImageCreateInfo::Input_File:
        {
            // If we got here, we are responsible for loading an image file.
            GPtr<GFile> pfile  = *info.pFileOpener->OpenFile(info.pFileInfo->FileName.ToCStr());
            if (!pfile)
                return 0; // Log ??

            // Detecting the file format may be better!?! But then, how do we handle extensions?
            // GFxLoader::FileFormatType format = GFxMovieRoot::DetectFileFormat(pfile);

            // Load an image into GImage object.
            pimage = *LoadBuiltinImage(pfile, info.pFileInfo->Format, info.Use,
                                       info.pLog, info.pJpegSupport, info.pPNGSupport, info.pHeap);
            if (!pimage)
                return 0;

            // info.pFileInfo->TargetWidth is 0 for gradient images; use real width instead
            if (info.pFileInfo->TargetWidth)
                twidth  = info.pFileInfo->TargetWidth;
            else
                twidth = pimage->Width;
            // info.pFileInfo->TargetHeight is 0 for gradient images; use real height instead
            if (info.pFileInfo->TargetHeight)
                theight = info.pFileInfo->TargetHeight;
            else
                theight = pimage->Height;
        }        
        break;

        // No input - empty image info.
    case GFxImageCreateInfo::Input_None:
    default:
        return GHEAP_NEW(pheap) GImageInfo();
    }

    // Make a distinction whether to keep the data or not based on
    // the KeepBindingData flag in GFxImageInfo.
    if (IsKeepingImageData())    
        return GHEAP_NEW(pheap) GImageInfo(pimage, twidth, theight, false);

    // Else, create a texture.
    if (!info.pRenderConfig || !info.pRenderConfig->GetRenderer())
    {
        // We need to either provide a renderer, or use KeepImageBindData flag.
        GFC_DEBUG_ERROR(1, "GFxImageCreator failed to create texture; no renderer installed");
        return 0;
    }

    // If renderer can generate Event_DataLost for textures (D3D9), store GImage in GFxImageInfo 
    // anyway. This is helpful because otherwise images can be lost and wiped out. Users can
    // override this behavior with custom creator if desired, handling loss in alternative manner.
    if (info.pRenderConfig->GetRendererCapBits() & GRenderer::Cap_CanLoseData)
        return GHEAP_NEW(pheap) GImageInfo(pimage, twidth, theight, false);

    // If we are working with multi-threaded loading and the renderer does not support texture creation on
    // separate threads we have to postpone the actual texture creation until the main renderer thread needs it
    // and instruct image info to release image as soon as texture is created
    if (info.ThreadedLoading && !(info.pRenderConfig->GetRendererCapBits() & GRenderer::Cap_ThreadedTextureCreation))
        return GHEAP_NEW(pheap) GImageInfo(pimage, twidth, theight, true);    

    // Renderer ok, create a texture-based GFxImageInfo instead. This means that
    // the source image memory will be released by our caller.
    GPtr<GTexture> ptexture = *info.pRenderConfig->GetRenderer()->CreateTexture();
    if (!ptexture || !ptexture->InitTexture(pimage))
        return 0;

    return GHEAP_NEW(pheap) GImageInfo(ptexture, twidth, theight);
}


// Loads a GImage from an open file, assuming the specified file format.
GImage* GFxImageCreator::LoadBuiltinImage(GFile* pfile,
                                          GFxFileConstants::FileFormatType format,
                                          GFxResource::ResourceUse use,
                                          GFxLog* plog,
                                          GFxJpegSupportBase* pjpegState, GFxPNGSupportBase* ppngState,
                                          GMemoryHeap* pimageHeap)
{
    // Open the file and create image.
    GImage    * pimage = 0;
    const char* pfilePath = pfile->GetFilePath();

    switch(format)
    {
    case GFxLoader::File_TGA:
        pimage = GImage::ReadTga(pfile, 
                                (use == GFxResource::Use_FontTexture) ?
                                    GImage::Image_A_8 : GImage::Image_None, pimageHeap);
        break;

    case GFxLoader::File_DDS:
        pimage = GImage::ReadDDS(pfile, pimageHeap);
        break;
    case GFxLoader::File_JPEG:

        if (!pjpegState)
        {
            if(plog)
                plog->LogMessage(
                    "Unable to load JPEG at URL \"%s\" - libjpeg not included\n", pfilePath);  
            else
            {
                GFC_DEBUG_WARNING1(1, "Unable to load JPEG at URL \"%s\" - libjpeg not included\n",
                                      pfilePath);
            }
        }
        else
            pimage = pjpegState->ReadJpeg(pfile, pimageHeap);
        break;

    case GFxLoader::File_PNG:

        if (!ppngState)
        {
            if (plog)
                plog->LogMessage("Unable to load PNG at URL \"%s\" - GFxPNGState not set\n",
                                 pfilePath);
            else
            {
                GFC_DEBUG_WARNING1(1, "Unable to load PNG at URL \"%s\" - GFxPNGState not included\n",
                                      pfilePath);
            }
        }
        else 
            pimage = ppngState->CreateImage(pfile, pimageHeap);
        break;

    default:
        // Unknown format!
        if (plog)
            plog->LogMessage("Default image loader failed to load '%s'", pfilePath);
        else 
        {
            GFC_DEBUG_WARNING1(1, "Default image loader failed to load '%s'",
                                  pfilePath);
        }
        break;
    }

    return pimage;
}


// ***** GFxStateBag implementation


// Implementation that allows us to override the log.
GFile*      GFxStateBag::OpenFileEx(const char *pfilename, GFxLog *plog)
{
    GPtr<GFxFileOpenerBase> popener = GetFileOpener();
    if (!popener)
    {             
        // Don't even have a way to open the file.
        if (plog)
            plog->LogError(
            "Error: GFxLoader failed to open '%s', GFxFileOpener not installed\n",
            pfilename);
        return 0;
    }

    return popener->OpenFileEx(pfilename, plog);
}

// Opens a file using the specified callback.
GFile*      GFxStateBag::OpenFile(const char *pfilename)
{
    return OpenFileEx(pfilename, GetLog());
}



GImageInfoBase* GFxStateBag::CreateImageInfo(const GFxImageCreateInfo& info)
{
    GPtr<GFxImageCreator> pcreator = GetImageCreator();
    if (pcreator)
        return pcreator->CreateImage(info);

    return new GImageInfo(
        (info.Type == GFxImageCreateInfo::Input_Image) ? info.pImage : 0,
        (info.Type == GFxImageCreateInfo::Input_File) ? info.pFileInfo->TargetWidth : 0,
        (info.Type == GFxImageCreateInfo::Input_File) ? info.pFileInfo->TargetHeight : 0);     
}



// ***** GFxLoader implementation

// Internal GFC Evaluation License Reader
#ifdef GFC_BUILD_EVAL
#define GFC_EVAL(x) GFx_##x
#define GFC_LIB_NAME_S "GFx"
#include "GFCValidateEval.cpp"
#else
void    GFx_ValidateEvaluation() { }
#endif


GFxLoader::GFxLoader(const GFxLoader::LoaderConfig& config)
{
    InitLoader(config);
}

GFxLoader::GFxLoader(const GPtr<GFxFileOpenerBase>& pfileOpener, 
                     const GPtr<GFxZlibSupportBase>& pzlib,
                     const GPtr<GFxJpegSupportBase>& pjpeg)
{
    LoaderConfig config(0, pfileOpener, pzlib, pjpeg);
    InitLoader(config);
}

// Create a new loader, copying it's library and states.
GFxLoader::GFxLoader(const GFxLoader& src)
{
    GFx_ValidateEvaluation();

    // Create new LoaderImpl with copied states.
    pImpl = GNEW GFxLoaderImpl(src.pImpl);
    // Copy strong resource lib reference.
    pStrongResourceLib = src.pStrongResourceLib;
    if (pStrongResourceLib)
        pStrongResourceLib->AddRef();

    GFC_DEBUG_ERROR(!pImpl, "GFxLoader::GFxLoader failed to initialize properly, low memory");
}


GFxLoader::~GFxLoader()
{

#ifdef GFX_AMP_SERVER
    GFxAmpServer::GetInstance().RemoveLoader(this);
#endif

    if (pImpl)
        pImpl->Release();
    if (pStrongResourceLib)
        pStrongResourceLib->Release();
}

void GFxLoader::InitLoader(const LoaderConfig& cfg)
{
    GFx_ValidateEvaluation();
       
    bool debugHeap = (cfg.DefLoadFlags & LoadDebugHeap) ? true : false;

    DefLoadFlags       = cfg.DefLoadFlags;
    pStrongResourceLib = GNEW GFxResourceLib(debugHeap);

    if ((pImpl = GNEW GFxLoaderImpl(pStrongResourceLib, debugHeap))!=0)
    {
        SetFileOpener(cfg.pFileOpener);
        SetParseControl(GPtr<GFxParseControl>(*GNEW GFxParseControl(GFxParseControl::VerboseParseNone)));
        SetZlibSupport(cfg.pZLibSupport);
        SetJpegSupport(cfg.pJpegSupport);
        SetMeshCacheManager(GPtr<GFxMeshCacheManager>(*GNEW GFxMeshCacheManager(debugHeap)));
    }

#ifdef GFX_AMP_SERVER
    if (!debugHeap)
    {
        GFxAmpServer::GetInstance().AddLoader(this);
    }
#endif

    GFC_DEBUG_ERROR(!pImpl || !pStrongResourceLib,
        "GFxLoader::GFxLoader failed to initialize properly, low memory");
}


GFxStateBag* GFxLoader::GetStateBagImpl() const
{
    return pImpl;
}

bool GFxLoader::CheckTagLoader(int tagType) const 
{ 
    return (pImpl) ? pImpl->CheckTagLoader(tagType) : 0;        
};

// Resource library management.
void               GFxLoader::SetResourceLib(GFxResourceLib *plib)
{
    if (pImpl)
    {
        if (plib)
            plib->AddRef();
        if (pStrongResourceLib)
            pStrongResourceLib->Release();
        pStrongResourceLib      = plib;
        pImpl->SetWeakResourceLib(plib->GetWeakLib());
    }
}

GFxResourceLib*    GFxLoader::GetResourceLib() const
{
    return pStrongResourceLib;
}


// *** Movie Loading

// Movie Loading APIs just delegate to GFxLoaderImpl after some error checking.

bool    GFxLoader::GetMovieInfo(const char *pfilename, GFxMovieInfo *pinfo,
                                bool getTagCount, UInt loadConstants)
{
    if (!pfilename || pfilename[0]==0)
    {
        GFC_DEBUG_WARNING(1, "GFxLoader::GetMovieInfo - passed filename is empty, no file to load");
        return 0;
    }
    if (!pinfo)
    {
        GFC_DEBUG_WARNING(1, "GFxLoader::GetMovieInfo - passed GFxMovieInfo pointer is NULL");
        return 0;
    }

    return pImpl ? pImpl->GetMovieInfo(pfilename, pinfo, getTagCount, loadConstants | DefLoadFlags) : 0;
}


GFxMovieDef*    GFxLoader::CreateMovie(const char *pfilename, UInt loadConstants, UPInt memoryArena)
{
    if (!pfilename || pfilename[0]==0)
    {
        GFC_DEBUG_WARNING(1, "GFxLoader::CreateMovie - passed filename is empty, no file to load");
        return 0;
    }
    return pImpl ? pImpl->CreateMovie(pfilename, loadConstants | DefLoadFlags, memoryArena) : 0;
}

/*
GFxMovieDef*    GFxLoader::CreateMovie(GFile *pfile, UInt loadConstants)
{
if (!pfile || !pfile->IsValid())
{
GFC_DEBUG_WARNING(1, "GFxLoader::CreateMovie - passed file is not valid, no file to load");
return 0;
}
return pImpl ? pImpl->CreateMovie(pfile, 0, loadConstants) : 0;
}
*/


// Unpins all resources held in the library
void GFxLoader::UnpinAll()
{
    if (pStrongResourceLib)    
        pStrongResourceLib->UnpinAll();    
}

void GFxLoader::CancelLoading()
{
    pImpl->CancelLoading();
}


void GFxImagePackParamsBase::SetTextureConfig(const TextureConfig& config)
{
     PackTextureConfig = config;
}

//////////////////////////////////////////////////////////////////////////


#ifdef GFC_BUILD_DEBUG
int GFx_Compile_without_GFC_BUILD_DEBUG;
#else
int GFx_Compile_with_GFC_BUILD_DEBUG;
#endif
