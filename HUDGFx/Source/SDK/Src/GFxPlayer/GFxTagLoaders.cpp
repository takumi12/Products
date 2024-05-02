/**********************************************************************

Filename    :   GFxTagLoaders.cpp
Content     :   GFxPlayer tag loaders implementation
Created     :   June 30, 2005
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxTagLoaders.h"

#include "GFile.h"
#include "GImage.h"
#include "GFxImageResource.h"

#include "GJPEGUtil.h"

#include "GRenderer.h"

#include "GFxCharacter.h"

#include "GFxLoaderImpl.h"
#include "GFxPlayerImpl.h"
#include "GFxSprite.h"

#include "GFxAction.h"
#include "GFxButton.h"

#include "GFxFontResource.h"
#include "GFxLog.h"
#include "GFxMorphCharacter.h"

#include "GFxShape.h"
#include "GFxStream.h"
#include "GFxStyles.h"
#include "GFxDlist.h"
#include "AS/GASTimers.h"
#include "GConfig.h"

#include "GFxLoadProcess.h"
#include "GFxAudio.h"
#include "GFxSoundTagsReader.h"
#include "GFxAmpServer.h"

#include <string.h> // for memset
#include <float.h>

#ifdef GFC_MATH_H
#include GFC_MATH_H
#else
#include <math.h>
#endif

// ***** Tag Loader Tables

GFx_TagLoaderFunction GFx_SWF_TagLoaderTable[GFxTag_SWF_TagTableEnd] =
{
    // [0]
    GFx_EndLoader,
    0,
    GFx_DefineShapeLoader,
    0,
    GFx_PlaceObjectLoader,
    GFx_RemoveObjectLoader,
    GFx_DefineBitsJpegLoader,
    GFx_ButtonCharacterLoader,
    GFx_JpegTablesLoader,
    GFx_SetBackgroundColorLoader,

    // [10]
    GFx_DefineFontLoader,
    GFx_DefineTextLoader,
    GFx_DoActionLoader,
    GFx_DefineFontInfoLoader,
#ifndef GFC_NO_SOUND
    GFx_DefineSoundLoader,
    GFx_StartSoundLoader,
    0,
    GFx_ButtonSoundLoader,
    GFx_SoundStreamHeadLoader,
    GFx_SoundStreamBlockLoader,
#else
    0,
    0,
    0,
    0,
    0,
    0,
#endif

    // [20]
    GFx_DefineBitsLossless2Loader,
    GFx_DefineBitsJpeg2Loader,
    GFx_DefineShapeLoader,
    0,
    GFx_NullLoader,   // "protect" tag; we're not an authoring tool so we don't care.
    0,
    GFx_PlaceObject2Loader,
    0,
    GFx_RemoveObject2Loader,
    0,

    // [30]
    0,
    0,
    GFx_DefineShapeLoader,
    GFx_DefineTextLoader,
    GFx_ButtonCharacterLoader,
    GFx_DefineBitsJpeg3Loader,
    GFx_DefineBitsLossless2Loader,
    GFx_DefineEditTextLoader,
    0,    
    GFx_SpriteLoader,

    // [40]
    0,
    0,
    0,
    GFx_FrameLabelLoader,
    0,
#ifndef GFC_NO_SOUND
    GFx_SoundStreamHeadLoader,
#else
    0,
#endif
    GFx_DefineShapeMorphLoader,
    0,
    GFx_DefineFontLoader,
    0,

    // [50]
    0,
    0,
    0,
    0,
    0,
    0,
    GFx_ExportLoader,
    GFx_ImportLoader,
    0,
    GFx_DoInitActionLoader,

    // [60]
#ifndef GFC_NO_VIDEO
    GFx_DefineVideoStream,
#else
    0,
#endif
    0,
    GFx_DefineFontInfoLoader,
    GFx_DebugIDLoader,
    0,
    0,
    GFx_SetTabIndexLoader,
    0,
    0,
    GFx_FileAttributesLoader,

    // [70]
    GFx_PlaceObject3Loader,
    GFx_ImportLoader,
    0,
    0,
    GFx_CSMTextSettings,
    GFx_DefineFontLoader,
    0,
    GFx_MetadataLoader,
    GFx_Scale9GridLoader,
    0,

    // [80]
    0,
    0,
    0,
    GFx_DefineShapeLoader,
    GFx_DefineShapeMorphLoader    
};

GFx_TagLoaderFunction GFx_GFX_TagLoaderTable[GFxTag_GFX_TagTableEnd - GFxTag_GFX_TagTableBegin] =
{
    // [1000]
    GFx_ExporterInfoLoader,
    GFx_DefineExternalImageLoader,
    GFx_FontTextureInfoLoader,
    GFx_DefineExternalGradientImageLoader,
    GFx_DefineGradientMapLoader,
    GFx_DefineFontLoader,
#ifndef GFC_NO_SOUND
    GFx_DefineExternalSoundLoader,
    GFx_DefineExternalStreamSoundLoader,
#else
    0,
    0,
#endif
    GFx_DefineSubImageLoader,
    GFx_DefineExternalImageLoader2
};

//
// Tag implementations
//

// Silently ignore the contents of this tag.
void    GSTDCALL GFx_NullLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED2(p, tagInfo);
}

// Label the current frame of m with the name from the GFxStream.
void    GSTDCALL GFx_FrameLabelLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    
    GStringDH name(p->GetLoadHeap());
    p->GetStream()->ReadString(&name);
    p->AddFrameName(name, p->GetLog());
}


void    GSTDCALL GFx_SetBackgroundColorLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_SetBackgroundColor);
    GASSERT(p);

    GFxSetBackgroundColor* pt = p->AllocTag<GFxSetBackgroundColor>();
    pt->Read(p);
    p->AddExecuteTag(pt);
}


// Load JPEG compression tables that can be used to load
// images further along in the GFxStream.
void    GSTDCALL GFx_JpegTablesLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_JpegTables);

    GFxJpegSupportBase* pjpegSupport = p->GetLoadStates()->GetJpegSupport();
    if (!pjpegSupport)
        p->LogError("Error: Jpeg System is not installed - can't load jpeg image data\n");
    else
    {
        // When Flash CS3 saves SWF8-, it puts this tag with zero length (no data in it).
        // In this case we should do nothing here, and create the JPEG image differently
        // in GFx_DefineBitsJpegLoader (AB)
        if (tagInfo.TagLength > 0)
        {
            GJPEGInput* pjin = pjpegSupport->CreateSwfJpeg2HeaderOnly(p->GetStream()->GetUnderlyingFile());
            GASSERT(pjin);
            p->SetJpegLoader(pjin);
        }
    }
}


// A JPEG image without included tables; those should be in an
// existing GJPEGInput object stored in the pMovie.
void    GSTDCALL GFx_DefineBitsJpegLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DefineBitsJpeg);

    UInt16  bitmapResourceId = p->GetStream()->ReadU16();

    //
    // Read the image data.
    //
    GPtr<GImage>         pimage;

    if (p->IsLoadingImageData())
    {
        GFxJpegSupportBase* pjpegSupport = p->GetLoadStates()->GetJpegSupport();
        if (!pjpegSupport)
            p->GetStream()->LogError("Error: Jpeg System is not installed - can't load jpeg image data\n");
        else
        {
            p->GetStream()->SyncFileStream();
            GJPEGInput* pjin = p->GetJpegLoader();
            if (!pjin)
            {
                // if tag 8 was not loaded or has zero length - just read
                // jpeg as usually.
                pimage = *pjpegSupport->ReadJpeg(p->GetStream()->GetUnderlyingFile(),
                                                 p->GetLoadImageHeap());
            }
            else
            {
                pjin->DiscardPartialBuffer();

                pimage = *pjpegSupport->ReadSwfJpeg2WithTables(
                                            pjin, p->GetLoadImageHeap());
            }
            // MA: We don't have renderer during loading? -> what do we do
        }
    }
    else
    {
        // Empty image.
    }

    // Create a unique resource for the image and add it.
    p->AddImageResource(GFxResourceId(bitmapResourceId), pimage);
}


void    GSTDCALL GFx_DefineBitsJpeg2Loader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DefineBitsJpeg2);
    
    UInt16  bitmapResourceId = p->ReadU16();

    p->LogParse("  GFx_DefineBitsJpeg2Loader: charid = %d pos = 0x%x\n",
                bitmapResourceId, p->Tell());

    //
    // Read the image data.
    //
    GPtr<GImage>         pimage;

    if (p->IsLoadingImageData())
    {
        GFxJpegSupportBase* pjpegSupport = p->GetLoadStates()->GetJpegSupport();
        if (!pjpegSupport)
            p->LogError("Error: Jpeg system is not installed - can't load jpeg image data\n");
        else
            pimage = *pjpegSupport->ReadSwfJpeg2(p->GetUnderlyingFile(),
                                                 p->GetLoadImageHeap());
    }
    else
    {
        // Empty image.
    }

    // Create a unique resource for the image and add it.
    p->AddImageResource(GFxResourceId(bitmapResourceId), pimage);
}


// Redundant code to compute GFx_UndoPremultiplyTable.
//#if defined(GFC_USE_ZLIB)
//// Helpers used for un-premultiplication.
//// SFW RGB+A data is stored with RGB channels pre-multiplied by alpha.
//// This lookup table is used to convert those efficiently.
//static SInt*    GFx_GetPremultiplyAlphaUndoTable()
//{
//    static  SInt    PremultiplyTable[256];
//    static  bool    TableValid = 0;
//
//    if (!TableValid)
//    {
//        PremultiplyTable[0] = 255 * 256;
//        for(UInt i=1; i<=255; i++)
//        {
//            PremultiplyTable[i] = (255 * 256) / i;
//        }
//        TableValid = 1;
//
//FILE* fd = fopen("dt", "wt");
//for(UInt i=0; i<=255; i++)
//{
//if(i)
//{
//    if(((i * PremultiplyTable[i]) >> 8) > 255)
//    {
//fprintf(fd, "\n========", PremultiplyTable[i]);
//    }
//}
//fprintf(fd, "%5d,", PremultiplyTable[i]);
//}
//fclose(fd);
//
//    }
//    return PremultiplyTable;
//}
//#endif


static UInt16 GFx_UndoPremultiplyTable[256] = 
{
    65280,65280,32640,21760,16320,13056,10880, 9325, 8160, 7253, 6528, 5934, 5440, 5021, 4662, 4352, 
     4080, 3840, 3626, 3435, 3264, 3108, 2967, 2838, 2720, 2611, 2510, 2417, 2331, 2251, 2176, 2105, 
     2040, 1978, 1920, 1865, 1813, 1764, 1717, 1673, 1632, 1592, 1554, 1518, 1483, 1450, 1419, 1388, 
     1360, 1332, 1305, 1280, 1255, 1231, 1208, 1186, 1165, 1145, 1125, 1106, 1088, 1070, 1052, 1036, 
     1020, 1004,  989,  974,  960,  946,  932,  919,  906,  894,  882,  870,  858,  847,  836,  826,  
      816,  805,  796,  786,  777,  768,  759,  750,  741,  733,  725,  717,  709,  701,  694,  687,  
      680,  672,  666,  659,  652,  646,  640,  633,  627,  621,  615,  610,  604,  598,  593,  588,  
      582,  577,  572,  567,  562,  557,  553,  548,  544,  539,  535,  530,  526,  522,  518,  514,  
      510,  506,  502,  498,  494,  490,  487,  483,  480,  476,  473,  469,  466,  462,  459,  456,  
      453,  450,  447,  444,  441,  438,  435,  432,  429,  426,  423,  421,  418,  415,  413,  410,  
      408,  405,  402,  400,  398,  395,  393,  390,  388,  386,  384,  381,  379,  377,  375,  373,  
      370,  368,  366,  364,  362,  360,  358,  356,  354,  352,  350,  349,  347,  345,  343,  341,  
      340,  338,  336,  334,  333,  331,  329,  328,  326,  324,  323,  321,  320,  318,  316,  315,  
      313,  312,  310,  309,  307,  306,  305,  303,  302,  300,  299,  298,  296,  295,  294,  292,  
      291,  290,  288,  287,  286,  285,  283,  282,  281,  280,  278,  277,  276,  275,  274,  273,  
      272,  270,  269,  268,  267,  266,  265,  264,  263,  262,  261,  260,  259,  258,  257,  256
};


// Helper routine to undo pre-multiply by alpha, stored in SWF files.
GINLINE void    UndoPremultiplyAlpha(UByte *prgb, UByte a)
{
    //SInt undoVal = GFx_PremultiplyTable[a]; //GFx_GetPremultiplyAlphaUndoTable()[a];
    //// This can probably be optimized... (with binary ops and/or SSE/MMX)
    //prgb[0] =  UByte( G_Clamp<SInt>((undoVal * (SInt)prgb[0]) >> 8, 0, 255) );
    //prgb[1] =  UByte( G_Clamp<SInt>((undoVal * (SInt)prgb[1]) >> 8, 0, 255) );
    //prgb[2] =  UByte( G_Clamp<SInt>((undoVal * (SInt)prgb[2]) >> 8, 0, 255) );

    UInt undoVal = GFx_UndoPremultiplyTable[a];
    prgb[0] = UByte((undoVal * ((prgb[0] <= a) ? prgb[0] : a)) >> 8);
    prgb[1] = UByte((undoVal * ((prgb[1] <= a) ? prgb[1] : a)) >> 8);
    prgb[2] = UByte((undoVal * ((prgb[2] <= a) ? prgb[2] : a)) >> 8);
}


// A special filter to restore lost colors after the alpha-premultiplication operation
void UndoAndFilterPremultiplied(GImage* pimage)
{
    if (pimage->Format != GImage::Image_ARGB_8888) return;
    UInt x,y;
    UInt pitch = pimage->Width * 4 + 8;

    GArray<UByte> buf;
    buf.Resize((pimage->Height + 2) * pitch);

    if (buf.GetSize() == 0) return;

    memset(&buf[0],                            0, pitch);
    memset(&buf[(pimage->Height + 1) * pitch], 0, pitch);

    for (y = 0; y < pimage->Height; ++y)
    {
        UByte* sl = &buf[(y + 1) * pitch];
        memcpy(sl + 4, pimage->GetScanline(y), pimage->Width * 4);
        sl[0] = sl[1] = sl[2] = sl[3] = 0;
        sl += pitch - 4;
        sl[0] = sl[1] = sl[2] = sl[3] = 0;
    }

    for (y = 0; y < pimage->Height; ++y)
    {
              UByte* imgSl = pimage->GetScanline(y);
        const UByte* bufSl = &buf[(y + 1) * pitch];
        for (x = 0; x < pimage->Width; ++x)
        {
            const UByte* p5 = bufSl + x * 4 + 4;
                  UByte* pi = imgSl + x * 4;

            if (pi[3] < 16)
            {
                const UByte* p1 = p5 - pitch - 4;
                const UByte* p2 = p5 - pitch;
                const UByte* p3 = p5 - pitch + 4;
                const UByte* p4 = p5 - 4;
                const UByte* p6 = p5 + 4;
                const UByte* p7 = p5 + pitch - 4;
                const UByte* p8 = p5 + pitch;
                const UByte* p9 = p5 + pitch + 4;

                // Calculate the weighted mean of 9 pixels. Sum(x[i] * w[i]) / Sum(w[i]);
                // Here x[i] is the color component, w[i] is alpha. So that, we calculate 
                // the denomonator once and the numerators for each channel. Note that we 
                // do not need to multiply by alpha because the components are already premultiplied.
                // We only need to scale the color values to the appropriate range (multiply by 256).
                UInt den = UInt(p1[3]) + p2[3] + p3[3] + p4[3] + p5[3] + p6[3] + p7[3] + p8[3] + p9[3];
                if (den)
                {
                    UInt r = ((p1[0] + p2[0] + p3[0] + p4[0] + p5[0] + p6[0] + p7[0] + p8[0] + p9[0]) << 8) / den;
                    UInt g = ((p1[1] + p2[1] + p3[1] + p4[1] + p5[1] + p6[1] + p7[1] + p8[1] + p9[1]) << 8) / den;
                    UInt b = ((p1[2] + p2[2] + p3[2] + p4[2] + p5[2] + p6[2] + p7[2] + p8[2] + p9[2]) << 8) / den;
                    pi[0] = UByte((r <= 255) ? r : 255);
                    pi[1] = UByte((g <= 255) ? g : 255);
                    pi[2] = UByte((b <= 255) ? b : 255);
                }

                // An alternative method. Just fing the neighbor pixel with the maximal alpha
                //UInt  a = p5[3];
                //if(p1[3] > a) { pi[0] = p1[0]; pi[1] = p1[1]; pi[2] = p1[2]; a = p1[3]; }
                //if(p2[3] > a) { pi[0] = p2[0]; pi[1] = p2[1]; pi[2] = p2[2]; a = p2[3]; }
                //if(p3[3] > a) { pi[0] = p3[0]; pi[1] = p3[1]; pi[2] = p3[2]; a = p3[3]; }
                //if(p4[3] > a) { pi[0] = p4[0]; pi[1] = p4[1]; pi[2] = p4[2]; a = p4[3]; }
                //if(p6[3] > a) { pi[0] = p6[0]; pi[1] = p6[1]; pi[2] = p6[2]; a = p6[3]; }
                //if(p7[3] > a) { pi[0] = p7[0]; pi[1] = p7[1]; pi[2] = p7[2]; a = p7[3]; }
                //if(p8[3] > a) { pi[0] = p8[0]; pi[1] = p8[1]; pi[2] = p8[2]; a = p8[3]; }
                //if(p9[3] > a) { pi[0] = p9[0]; pi[1] = p9[1]; pi[2] = p9[2]; a = p9[3]; }
            }
            else
            {
                UndoPremultiplyAlpha(pi, pi[3]);
            }
        }
    }
}



// loads a DefineBitsJpeg3 tag. This is a jpeg file with an alpha
// channel using zlib compression.
void    GSTDCALL GFx_DefineBitsJpeg3Loader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DefineBitsJpeg3);

    UInt16  bitmapResourceId = p->ReadU16();
    p->LogParse("  GFx_DefineBitsJpeg3Loader: charid = %d pos = %d\n",
                bitmapResourceId, p->Tell());

    UInt32  jpegSize      = p->ReadU32();
    UInt32  alphaPosition = p->Tell() + jpegSize;

    // MA Debug
    //GFC_DEBUG_MESSAGE1(1, "  GFx_DefineBitsJpeg3Loader: Begin jpeg at %d", alphaPosition - jpegSize);

    GPtr<GImage>         pimage;
 
    if (p->IsLoadingImageData())
    {
#if !defined(GFC_USE_ZLIB)
        p->LogError("Error: zlib is not linked - can't load zipped image data!\n");
        GUNUSED(alphaPosition);
#else
        GFxZlibSupportBase* pzlib = p->GetLoadStates()->GetZlibSupport();
        if (!pzlib)
            p->LogError("Error: GFxZlibState is not set - can't load zipped image data\n");
        else 
        {
        GFxJpegSupportBase* pjpegState = p->GetLoadStates()->GetJpegSupport();
        if (!pjpegState)
            p->LogError("Error: Jpeg System is not installed - can't load jpeg image data\n");
        else
        {
            // MA Debug
            //GFC_DEBUG_MESSAGE1(1, "  GFx_DefineBitsJpeg3Loader: At %d, string to read JPeg", p->Tell());

            //
            // Read the image data.
            //
            pimage = *pjpegState->ReadSwfJpeg3(p->GetUnderlyingFile(),
                                               p->GetLoadImageHeap());

            // Read alpha channel.
            p->SetPosition(alphaPosition);

            int     bufferBytes = pimage->Width * pimage->Height;
            UByte*  buffer      = (UByte*)GALLOC(bufferBytes, GStat_Default_Mem);

            pzlib->InflateWrapper(p->GetStream(), buffer, bufferBytes);

            for (int i = 0; i < bufferBytes; i++)
            {
                pimage->pData[4*i+3] = buffer[i];
            }
            // RGB comes in pre-multiplied by alpha. Undo and filter pre-multiplication.
            UndoAndFilterPremultiplied(pimage);
            GFREE(buffer);
        }
        }
#endif
    }
    else
    {
        // Empty image data.
    }


    // Create a unique resource for the image and add it.
    p->AddImageResource(GFxResourceId(bitmapResourceId), pimage);
}


void    GSTDCALL GFx_DefineBitsLossless2Loader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT(tagInfo.TagType == GFxTag_DefineBitsLossless || tagInfo.TagType == GFxTag_DefineBitsLossless2);

    UInt16  bitmapResourceId = p->ReadU16();
    UByte   BitmapFormat     = p->ReadU8();    // 3 == 8 bit, 4 == 16 bit, 5 == 32 bit
    UInt16  width            = p->ReadU16();
    UInt16  height           = p->ReadU16();

    p->LogParse("  DefBitsLossless2: tagInfo.TagType = %d, id = %d, fmt = %d, w = %d, h = %d\n",
            tagInfo.TagType,
            bitmapResourceId,
            BitmapFormat,
            width,
            height);


    GPtr<GImage> pimage;

    if (p->IsLoadingImageData())
    {
#ifndef GFC_USE_ZLIB
        p->LogError("Error: zlib is not linked - can't load zipped image data\n");
#else
        GFxZlibSupportBase* pzlib = p->GetLoadStates()->GetZlibSupport();
        if (!pzlib)
            p->LogError("Error: GFxZlibState is not set - can't load zipped image data\n");
        else 
        {
        GMemoryHeap* pimageHeap = p->GetLoadImageHeap();

        if (tagInfo.TagType == GFxTag_DefineBitsLossless)
        {
            // RGB image data.
            pimage = *GImage::CreateImage(GImage::Image_RGB_888, width, height,
                                          pimageHeap);

            if (BitmapFormat == 3)
            {
                // 8-bit data, preceded by a palette.

                const int   BytesPerPixel = 1;
                int ColorTableSize = p->ReadU8();
                ColorTableSize += 1;    // !! SWF stores one less than the actual size

                int pitch = (width * BytesPerPixel + 3) & ~3;

                int BufferBytes = ColorTableSize * 3 + pitch * height;
                UByte*  buffer = (UByte*)GALLOC(BufferBytes, GStat_Default_Mem);

                pzlib->InflateWrapper(p->GetStream(), buffer, BufferBytes);
                GASSERT(p->Tell() <= p->GetTagEndPosition());

                UByte*  ColorTable = buffer;

                for (int j = 0; j < height; j++)
                {
                    UByte*  ImageInRow = buffer + ColorTableSize * 3 + j * pitch;
                    UByte*  ImageOutRow = pimage->GetScanline(j);
                    for (int i = 0; i < width; i++)
                    {
                        UByte   pixel = ImageInRow[i * BytesPerPixel];
                        ImageOutRow[i * 3 + 0] = ColorTable[pixel * 3 + 0];
                        ImageOutRow[i * 3 + 1] = ColorTable[pixel * 3 + 1];
                        ImageOutRow[i * 3 + 2] = ColorTable[pixel * 3 + 2];
                    }
                }

                GFREE(buffer);
            }
            else if (BitmapFormat == 4)
            {
                // 16 bits / pixel
                const int   BytesPerPixel = 2;
                int pitch = (width * BytesPerPixel + 3) & ~3;

                int BufferBytes = pitch * height;
                UByte*  buffer = (UByte*)GALLOC(BufferBytes, GStat_Default_Mem);

                pzlib->InflateWrapper(p->GetStream(), buffer, BufferBytes);
                GASSERT(p->Tell() <= p->GetTagEndPosition());
        
                for (int j = 0; j < height; j++)
                {
                    UByte*  ImageInRow = buffer + j * pitch;
                    UByte*  ImageOutRow = pimage->GetScanline(j);
                    for (int i = 0; i < width; i++)
                    {
                        UInt16  pixel = ImageInRow[i * 2] | (ImageInRow[i * 2 + 1] << 8);
                
                        // Format must be RGB 555.
                        ImageOutRow[i * 3 + 0] = UByte( (pixel >> 7) & 0xF8 );  // red
                        ImageOutRow[i * 3 + 1] = UByte( (pixel >> 2) & 0xF8 );  // green
                        ImageOutRow[i * 3 + 2] = UByte( (pixel << 3) & 0xF8 );  // blue
                    }
                }
        
                GFREE(buffer);
            }
            else if (BitmapFormat == 5)
            {
                // 32 bits / pixel, input is ARGB Format (???)
                const int   BytesPerPixel = 4;
                int pitch = width * BytesPerPixel;

                int BufferBytes = pitch * height;
                UByte*  buffer = (UByte*)GALLOC(BufferBytes, GStat_Default_Mem);

                pzlib->InflateWrapper(p->GetStream(), buffer, BufferBytes);
                GASSERT(p->Tell() <= p->GetTagEndPosition());
        
                // Need to re-arrange ARGB into RGB.
                for (int j = 0; j < height; j++)
                {
                    UByte*  ImageInRow = buffer + j * pitch;
                    UByte*  ImageOutRow = pimage->GetScanline(j);
                    for (int i = 0; i < width; i++)
                    {
                        UByte   a = ImageInRow[i * 4 + 0];
                        UByte   r = ImageInRow[i * 4 + 1];
                        UByte   g = ImageInRow[i * 4 + 2];
                        UByte   b = ImageInRow[i * 4 + 3];
                        ImageOutRow[i * 3 + 0] = r;
                        ImageOutRow[i * 3 + 1] = g;
                        ImageOutRow[i * 3 + 2] = b;
                        a = a;  // Inhibit warning.
                    }
                }

                GFREE(buffer);
                }

                //icreateInfo.SetImage(pimage);
            }
            else
            {
                // RGBA image data.
                GASSERT(tagInfo.TagType == GFxTag_DefineBitsLossless2);

                pimage = *GImage::CreateImage(GImage::Image_ARGB_8888, width, height,
                                              pimageHeap);

                if (BitmapFormat == 3)
                {
                    // 8-bit data, preceded by a palette.

                    const int   BytesPerPixel = 1;
                    int ColorTableSize = p->ReadU8();
                    ColorTableSize += 1;    // !! SWF stores one less than the actual size

                    int pitch = (width * BytesPerPixel + 3) & ~3;

                    int BufferBytes = ColorTableSize * 4 + pitch * height;
                    UByte*  buffer = (UByte*)GALLOC(BufferBytes, GStat_Default_Mem);

                    pzlib->InflateWrapper(p->GetStream(), buffer, BufferBytes);
                    GASSERT(p->Tell() <= p->GetTagEndPosition());

                    UByte*  ColorTable = buffer;

                    for (int j = 0; j < height; j++)
                    {
                        UByte*  ImageInRow = buffer + ColorTableSize * 4 + j * pitch;
                        UByte*  ImageOutRow = pimage->GetScanline(j);
                        for (int i = 0; i < width; i++)
                        {
                            UByte   pixel = ImageInRow[i * BytesPerPixel];
                            ImageOutRow[i * 4 + 0] = ColorTable[pixel * 4 + 0];
                            ImageOutRow[i * 4 + 1] = ColorTable[pixel * 4 + 1];
                            ImageOutRow[i * 4 + 2] = ColorTable[pixel * 4 + 2];
                            ImageOutRow[i * 4 + 3] = ColorTable[pixel * 4 + 3];
                            
                            // Should we do this for color-mapped table?
                        //  UndoPremultiplyAlpha(ImageOutRow + i * 4, ImageOutRow[i * 4 + 3], undoAlphaTable);
                        }
                    }

                    GFREE(buffer);
                }
                else if (BitmapFormat == 4)
                {
                    // should be 555.
                    // Is this combination not supported?

                    // 16 bits / pixel
                    const int   BytesPerPixel = 2;
                    int pitch = (width * BytesPerPixel + 3) & ~3;

                    int BufferBytes = pitch * height;
                    UByte*  buffer = (UByte*)GALLOC(BufferBytes, GStat_Default_Mem);

                    pzlib->InflateWrapper(p->GetStream(), buffer, BufferBytes);
                    GASSERT(p->Tell() <= p->GetTagEndPosition());
            
                    for (int j = 0; j < height; j++)
                    {
                        UByte*  ImageInRow = buffer + j * pitch;
                        UByte*  ImageOutRow = pimage->GetScanline(j);
                        for (int i = 0; i < width; i++)
                        {
                            UInt16  pixel = ImageInRow[i * 2] | (ImageInRow[i * 2 + 1] << 8);
                    
                            // Format is RGB 555.
                            ImageOutRow[i * 4 + 0] = UByte( 255 );                  // alpha
                            ImageOutRow[i * 4 + 1] = UByte( (pixel >> 7) & 0xF8 );  // red
                            ImageOutRow[i * 4 + 2] = UByte( (pixel >> 2) & 0xF8 );  // green
                            ImageOutRow[i * 4 + 3] = UByte( (pixel << 3) & 0xF8 );  // blue
                        }
                    }
            
                    GFREE(buffer);
                }
                else if (BitmapFormat == 5)
                {
                    // 32 bits / pixel, input is ARGB format

                    pzlib->InflateWrapper(p->GetStream(), pimage->pData, width * height * 4);
                    GASSERT(p->Tell() <= p->GetTagEndPosition());
            
                    // Need to re-arrange ARGB into RGBA.
                    for (int j = 0; j < height; j++)
                    {
                        UByte*  ImageRow = pimage->GetScanline(j);
                        for (int i = 0; i < width; i++)
                        {
                            UByte   a = ImageRow[i * 4 + 0];
                            UByte   r = ImageRow[i * 4 + 1];
                            UByte   g = ImageRow[i * 4 + 2];
                            UByte   b = ImageRow[i * 4 + 3];
                            ImageRow[i * 4 + 0] = r;
                            ImageRow[i * 4 + 1] = g;
                            ImageRow[i * 4 + 2] = b;
                            ImageRow[i * 4 + 3] = a;
                        }
                    }
                    // Undo and filter pre-multiplied format result.
                    UndoAndFilterPremultiplied(pimage);
                }

                // RGBA
                //icreateInfo.SetImage(pimage);
            }
            }
#endif // GFC_USE_ZLIB
    }
    else
    {
        // Empty image in icreateInfo.
    }
    

    // Create a unique resource for the image and add it.
    p->AddImageResource(GFxResourceId(bitmapResourceId), pimage);    
}


void    GSTDCALL GFx_DefineShapeLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT( tagInfo.TagType == GFxTag_DefineShape  ||
             tagInfo.TagType == GFxTag_DefineShape2 ||
             tagInfo.TagType == GFxTag_DefineShape3 ||
             tagInfo.TagType == GFxTag_DefineShape4 );

    UInt16  characterId = p->ReadU16();
    p->LogParse("  ShapeLoader: id = %d\n", characterId);

    GPtr<GFxConstShapeCharacterDef>  ch = 
        *GHEAP_NEW_ID(p->GetLoadHeap(), GFxStatMD_CharDefs_Mem) GFxConstShapeCharacterDef;
    int shapeOffset = p->GetStream()->Tell();
    ch->Read(p, tagInfo.TagType, tagInfo.TagLength - (shapeOffset - tagInfo.TagDataOffset), true);

    const GFxPreprocessParams* pp = p->GetBindStates()->pPreprocessParams;
    if (pp)
    {
        GFxRenderConfig cfg(pp->GetTessellateEdgeAA() ? 
                                    GFxRenderConfig::RF_EdgeAA : 0, 
                               ~0U, ~0U);
        ch->PreTessellate(pp->GetTessellateScale(), cfg);
    }

    p->LogParse("  bound rect:");      
    p->GetStream()->LogParseClass(ch->GetBound());

    p->AddResource(GFxResourceId(characterId), ch.GetPtr());
}

void    GSTDCALL GFx_DefineShapeMorphLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT( tagInfo.TagType == GFxTag_DefineShapeMorph ||
             tagInfo.TagType == GFxTag_DefineShapeMorph2 );
#ifndef GFC_NO_MORPHING_SUPPORT
    UInt16 characterId = p->ReadU16();
    p->LogParse("  ShapeMorphLoader: id = %d\n", characterId);

    GPtr<GFxMorphCharacterDef> morph = 
        *GHEAP_NEW_ID(p->GetLoadHeap(), GFxStatMD_CharDefs_Mem) GFxMorphCharacterDef;
    morph->Read(p, tagInfo, true);
    p->AddResource(GFxResourceId(characterId), morph);
#else
    p->LogError("Error: ShapeMorphLoader failed due to GFC_NO_MORPHING_SUPPORT macro\n");
#endif
}


//
// GFxFontResource loaders
//

// Load a DefineFont or DefineFont2 tag.
void    GSTDCALL GFx_DefineFontLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT( tagInfo.TagType == GFxTag_DefineFont  ||
             tagInfo.TagType == GFxTag_DefineFont2 ||
             tagInfo.TagType == GFxTag_DefineFont3 ||
             tagInfo.TagType == GFxTag_DefineCompactedFont );

    UInt16  fontId = p->ReadU16();

    p->LogParse("  Font: id = %d\n", fontId);
    // Note that we always create FontData separately from font. In addition to
    // allowing for different font texture bindings, this lets us substitute
    // fonts based on names even when they are not imported.

    // FontComptactor is not compatible with stripped fonts that come from gfx files.
    bool stripped_fonts = false;
    const GFxExporterInfo* ei = p->GetExporterInfo();
    if (ei && ei->ExportFlags & GFxExporterInfo::EXF_GlyphsStripped)
        stripped_fonts = true;

    GPtr<GFxFont> pf;
    if (tagInfo.TagType == GFxTag_DefineCompactedFont)
    {
#ifndef GFC_NO_COMPACTED_FONT_SUPPORT
        GFxFontDataCompactedGfx* pfd = GHEAP_NEW(p->GetLoadHeap()) GFxFontDataCompactedGfx;
        pf = *pfd;
        pfd->Read(p, tagInfo);
#else
        p->LogError("Error: Trying to load compacted font with GFC_NO_COMPACTED_FONT_SUPPORT defined.\n");
        return;
#endif //#ifndef GFC_NO_COMPACTED_FONT_SUPPORT
    }
#ifndef GFC_NO_FONTCOMPACTOR_SUPPORT
    else if ((tagInfo.TagType == GFxTag_DefineFont2 || tagInfo.TagType == GFxTag_DefineFont3) 
             && !stripped_fonts
             && p->GetLoadStates()->GetFontCompactorParams())
    {
        GFxFontDataCompactedSwf* pfd = GHEAP_NEW(p->GetLoadHeap()) GFxFontDataCompactedSwf;
        pf = *pfd;
        pfd->Read(p, tagInfo);
    }
#endif //#ifndef GFC_NO_FONTCOMPACTOR_SUPPORT
    else
    {
        GFxFontData* pfd = GHEAP_NEW(p->GetLoadHeap()) GFxFontData;
        pf = *pfd;
        pfd->Read(p, tagInfo);
    }
   
    p->AddFontDataResource(GFxResourceId(fontId), pf);
}

// Load a SwdId tag, a 16 byte unique hash that matches up with the hash stored in a .swd file.
void    GSTDCALL GFx_DebugIDLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DebugID);

    GString strSwdId;
    char acHex[3];
    for (UPInt i = 0; i < 16; ++i)
    {
        strSwdId += G_itoa(p->ReadU8(), acHex, 3, 16);
    }
#ifdef GFX_AMP_SERVER
    p->SetSwdHandle(GFxAmpServer::GetInstance().AddSwf(strSwdId, p->GetFileURL()));
#endif  // GFX_AMP_SERVER
}

// Load a DefineFontInfo tag.  This adds information to an
// existing GFxFontResource.
void    GSTDCALL GFx_DefineFontInfoLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DefineFontInfo || tagInfo.TagType == GFxTag_DefineFontInfo2);

    UInt16  fontId = p->ReadU16();

    // We need to get at the font data somehow!!!

    // Here we assume that the real font type is GFxFontData. 
    // DefineFontInfo and DefineFontInfo2 tags only go with DefineFont tag which we 
    // don't convert to compacted form.
    GFxFontData* pfd = (GFxFontData*)p->GetFontData(GFxResourceId(fontId));

    if (pfd)
    {
        pfd->ReadFontInfo(p->GetStream(), tagInfo.TagType);
    }
    else
    {
        p->LogError("GFx_DefineFontInfoLoader: can't find GFxFontResource w/ id %d\n", fontId);
    }
}

void    GSTDCALL GFx_PlaceObjectLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT( tagInfo.TagType == GFxTag_PlaceObject );
    p->LogParse("  PlaceObject\n");
    GFxStream* pin = p->GetStream();
    UPInt dataSz = GFxPlaceObject::ComputeDataSize(pin);
    GFxPlaceObject* ptag = p->AllocTag<GFxPlaceObject>(dataSz);
    GASExecuteTag::LoadData(pin, ptag->pData, dataSz);   
    ptag->CheckForCxForm(dataSz);
    p->AddExecuteTag(ptag);
}

void    GSTDCALL GFx_PlaceObject2Loader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT( tagInfo.TagType == GFxTag_PlaceObject2 );
    p->LogParse("  PlaceObject2\n");
    GFxStream* pin = p->GetStream();
    UPInt dataSz = GFxPlaceObject2::ComputeDataSize(pin, p->GetVersion());
    bool hasEventHandlers = GFxPlaceObject2::HasEventHandlers(pin);
    if (hasEventHandlers)
        dataSz += sizeof(GFxPlaceObject2::EventArrayType*);
    GFxPlaceObject2* ptag = NULL;
    if (p->GetVersion() >= 6)
        ptag = p->AllocTag<GFxPlaceObject2>(dataSz);
    else
        ptag = p->AllocTag<GFxPlaceObject2a>(dataSz);
    if (hasEventHandlers)
    {
        GASExecuteTag::LoadData(pin, ptag->pData, dataSz-sizeof(GFxPlaceObject2::EventArrayType*), sizeof(GFxPlaceObjectBase::EventArrayType*));
        GFxPlaceObject2::RestructureForEventHandlers(ptag->pData);
    }
    else
        GASExecuteTag::LoadData(pin, ptag->pData, dataSz);
    p->AddExecuteTag(ptag);
}

void    GSTDCALL GFx_PlaceObject3Loader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT( tagInfo.TagType == GFxTag_PlaceObject3 );
    p->LogParse("  PlaceObject3\n");
    GFxStream* pin = p->GetStream();
    UPInt dataSz = GFxPlaceObject3::ComputeDataSize(pin);
    bool hasEventHandlers = GFxPlaceObject2::HasEventHandlers(pin);
    if (hasEventHandlers)
        dataSz += sizeof(GFxPlaceObject3::EventArrayType*);
    GFxPlaceObject3* ptag = p->AllocTag<GFxPlaceObject3>(dataSz);
    if (hasEventHandlers)
    {
        GASExecuteTag::LoadData(pin, ptag->pData, dataSz-sizeof(GFxPlaceObject3::EventArrayType*), sizeof(GFxPlaceObject3::EventArrayType*));
        GFxPlaceObject2::RestructureForEventHandlers(ptag->pData);
    }
    else
        GASExecuteTag::LoadData(pin, ptag->pData, dataSz);
    p->AddExecuteTag(ptag);
}


// Create and initialize a sprite, and add it to the pMovie.
void    GSTDCALL GFx_SpriteLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_Sprite);
            
    GFxResourceId characterId = GFxResourceId(p->ReadU16());
    p->LogParse("  sprite\n  char id = %d\n", characterId.GetIdIndex());

    // @@ combine GFxSpriteDef with GFxMovieDefImpl
    GPtr<GFxSpriteDef> ch = 
        *GHEAP_NEW_ID(p->GetLoadHeap(), GFxStatMD_CharDefs_Mem) GFxSpriteDef(p->GetDataDef_Unsafe());
    ch->Read(p, characterId);
    p->AddCharacter(characterId, ch);
}





//
// EndTag
//

// EndTag doesn't actually need to exist.

void    GSTDCALL GFx_EndLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED2(p, tagInfo.TagType);
    GASSERT(tagInfo.TagType == GFxTag_End);
    GASSERT(p->Tell() == p->GetTagEndPosition());
}

void    GSTDCALL GFx_RemoveObjectLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT( tagInfo.TagType == GFxTag_RemoveObject );

    GFxRemoveObject*   ptag = p->AllocTag<GFxRemoveObject>();
    ptag->Read(p);
    p->LogParse("  RemoveObject(%d, %d)\n", ptag->Id, ptag->Depth);
    p->AddExecuteTag(ptag);
}

void    GSTDCALL GFx_RemoveObject2Loader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT( tagInfo.TagType == GFxTag_RemoveObject2 );

    GFxRemoveObject2*   ptag = p->AllocTag<GFxRemoveObject2>();
    ptag->Read(p);
    p->LogParse("  RemoveObject2(%d)\n", ptag->Depth);
    p->AddExecuteTag(ptag);
}


void    GSTDCALL GFx_ButtonCharacterLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT( tagInfo.TagType == GFxTag_ButtonCharacter ||
             tagInfo.TagType == GFxTag_ButtonCharacter2 );

    int characterId = p->ReadU16();
    p->LogParse("  button GFxCharacter loader: CharId = %d\n", characterId);

    GPtr<GFxButtonCharacterDef> ch = 
        *GHEAP_NEW_ID(p->GetLoadHeap(), GFxStatMD_CharDefs_Mem) GFxButtonCharacterDef;
    ch->Read(p, tagInfo.TagType);
    p->AddResource(GFxResourceId(characterId), ch);
}

#ifndef GFC_NO_VIDEO

void    GSTDCALL GFx_DefineVideoStream(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT(tagInfo.TagType == GFxTag_DefineVideoStream);
    GFxLoadStates* pstates = p->GetLoadStates();
    GFxVideoBase* pvideoState = pstates->GetVideoPlayerState();
    if (pvideoState)
        pvideoState->ReadDefineVideoStreamTag(p, tagInfo);
    else
    {
        p->GetStream()->LogParse("GFx_DefineVideoStream: Video libarary is not set.\n");
        p->GetStream()->LogTagBytes();
    }
}

#endif

// Handling this tag is necessary for files produced by FlashPaper. This tag may be inside
// DefineSprite tag, and removing SetTabIndex by gfxeport causes 
// incorrect length of DefineSprite tag 
void    GSTDCALL GFx_SetTabIndexLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_SetTabIndex);
    GASSERT (4 == p->GetTagEndPosition() - p->Tell());    
    UInt16 depth = p->ReadU16();
    UInt16 tabIndex = p->ReadU16();
    GUNUSED2(depth, tabIndex);
    p->LogParse("SetTabIndex (unused) \n");
}

//
// *** Export and Import Tags
//

// Load an export Tag (for exposing internal resources of m)
void    GSTDCALL GFx_ExportLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_Export);

    UInt count = p->ReadU16();
    p->LogParse("  export: count = %d\n", count);

    // Read the exports.
    for (UInt i = 0; i < count; i++)
    {
        UInt16      id          = p->ReadU16();
        GStringDH symbolName(p->GetLoadHeap());

        p->GetStream()->ReadString(&symbolName);
        
        p->LogParse("  export: id = %d, name = %s\n", id, symbolName.ToCStr());

        GFxResourceId     rid(id);
        GFxResourceHandle hres;
        if (p->GetResourceHandle(&hres, rid))
        {
            // Add export to the list.
            p->ExportResource(symbolName, rid, hres);

            // Should we check export types?
            // This may be impossible with handles.
        }
        else         
        {
            // This is really a debug error, since we expect Flash files to
            // be consistent and include their exported characters.
           // GFC_DEBUG_WARNING1(1, "Export loader failed to export resource '%s'",
           //                       psymbolName );
            p->LogError("Export error: don't know how to export GFxResource '%s'\n",
                        symbolName.ToCStr());
        }        
    }
}


// Load an import Tag (for pulling in external resources)
void    GSTDCALL GFx_ImportLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT(tagInfo.TagType == GFxTag_Import || tagInfo.TagType == GFxTag_Import2);

    GFxStream * pin = p->GetStream();    

    GString sourceUrl;
    pin->ReadString(&sourceUrl);

    int       count = pin->ReadU16();

    p->LogParse( ((tagInfo.TagType != GFxTag_Import2) ?
                    "  importAssets: SourceUrl = %s, count = %d\n" :
                    "  importAssets2: SourceUrl = %s, count = %d\n" ),
                    sourceUrl.ToCStr(), count);

    if (tagInfo.TagType == GFxTag_Import2)
    {
        UInt val = p->ReadU16();
        GUNUSED(val);
        GFC_DEBUG_WARNING1(val != 1, "Unexpected attribute in ImportAssets2 - 0x%X instead of 1", val);
    }
    
    GFxImportData* pimportData = p->AllocMovieDefClass<GFxImportData>();
    pimportData->Frame     = p->GetLoadingFrame();
    pimportData->SourceUrl = sourceUrl;

    //GFxImportData* pimportData = new GFxImportData(psourceUrl,
    //                                               pdataDef->GetLoadingFrame());   

    // Get the imports.
    for (int i = 0; i < count; i++)
    {
        GString   symbolName;
        UInt16      id = pin->ReadU16();

        pin->ReadString(&symbolName);
        p->LogParse("  import: id = %d, name = %s\n", id, symbolName.ToCStr());

        // And add ResourceId so that it's handle can be referenced by new load operations.
        GFxResourceHandle rh = p->AddNewResourceHandle(GFxResourceId(id));
        // Add import symbol to data.
        pimportData->AddSymbol(symbolName.ToCStr(), id, rh.GetBindIndex());
    }

    // Pass completed GFxImportData to GFxMovieDataDef.
    p->AddImportData(pimportData);


    // Add InitAction import tag, so that init actions are processed correctly.
    GFxInitImportActions*  ptag = p->AllocTag<GFxInitImportActions>();
    ptag->SetImportIndex(pimportData->ImportIndex);
    p->AddInitAction(GFxResourceId(), ptag);  
}


//
// *** SWF 8
//

void    GSTDCALL GFx_FileAttributesLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_FileAttributes);

    UInt16 attrVal = p->ReadU16();
    p->SetFileAttributes(attrVal);

    // Log values for the user.
#ifndef GFC_NO_FXPLAYER_VERBOSE_PARSE
    if (attrVal)
    {
        p->LogParse("  fileAttr:");

        char separator = ' ';
        if (attrVal & GFxMovieDef::FileAttr_UseNetwork)
        {
            p->LogParse("%cUseNetwork", separator);
            separator = ',';
        }
        if (attrVal & GFxMovieDef::FileAttr_HasMetadata)
        {
            p->LogParse("%cHasMetadata", separator);
        }
        p->LogParse("\n");
    }
#endif

}

void    GSTDCALL GFx_MetadataLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_Metadata);

    // Determine the size of meta-data and read it into the buffer.
    int     size = p->GetTagEndPosition() - p->Tell();
    UByte*  pbuff = (UByte*)GALLOC(size + 1, GStat_Default_Mem);
    if (pbuff)
    {           
        for(int i=0; i<size; i++)
            pbuff[i] = p->ReadU8();
        p->SetMetadata(pbuff, (UInt)size);

        pbuff[G_Min<int>(size,255)] = 0;        
        p->LogParse("  metadata: %s\n", pbuff);
        GFREE(pbuff);       
    }
}

void    GSTDCALL GFx_ExporterInfoLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{   
    GASSERT(tagInfo.TagType == GFxTag_ExporterInfo);    

    // This tag should no longer be called because it is assumed to always be processed 
    // by the file-header loader through the GFxExporterInfoImpl::ReadExporterInfoTag
    // function.
    GASSERT(0);    
    GUNUSED2(p, tagInfo.TagType);

    // Utilizes the tag 1000 (unused in normal SWF):
    // See GFxExporterInfoImpl::ReadExporterInfoTag for implementation.

    //GFxExporterInfoImpl sinfo;
    // sinfo.ReadExporterInfoTag(p, tagInfo.TagType);
    // sinfo.SetData(version, (GFxLoader::FileFormatType)bitmapFormat, pswf, pfx, flags);
    // m->SetExporterInfo(sinfo);
}


// A helper method that generates image file info and resource data for the
// image based on specified arguments, returning a handle.
static GFxResourceHandle GFx_CreateImageFileResourceHandle(
                            GFxLoadProcess* p, GFxResourceId rid,
                            const char* pimageFileName, const char* pimageExportName, 
                            UInt16 bitmapFormat,
                            UInt16 targetWidth, UInt16 targetHeight, UInt8 flags = GFxImageFileInfo::Wrappable)
{
    GFxResourceHandle rh;
    
    // Create a concatenated file name.
    /* // WE no longer translate the filename here, as it is done through
       // a local translator interface during image creation -
       // see (GFxImageResourceCreator::CreateResource).

    GString         relativePath;

    // Determine the file name we should use base on a relative path.
    if (GFxURLBuilder::IsPathAbsolute(pimageFileName))
        relativePath = pimageFileName;
    else
    {
        relativePath = p->GetFileURL();
        if (!GFxURLBuilder::ExtractFilePath(&relativePath))
            relativePath = "";
        relativePath += pimageFileName;
    }
    */

    // Fill in file information.
    GPtr<GFxImageFileInfo> pfi = *new GFxImageFileInfo;
    if (pfi)
    {
        pfi->FileName       = pimageFileName;
        pfi->ExportName     = pimageExportName;
        pfi->pExporterInfo  = p->GetExporterInfo();
        pfi->Format         = (GFxFileConstants::FileFormatType) bitmapFormat;
        pfi->TargetWidth    = targetWidth;
        pfi->TargetHeight   = targetHeight;
        pfi->Flags          = flags;

        // !AB: need to set Use_FontTextures for images used as
        // font textures to provide correct conversion to A8 format,
        // if necessary.
        if (rid.GetIdType() == GFxResourceId::IdType_FontImage)
            pfi->Use = GFxResource::Use_FontTexture;

        // Add resource id and data.      
        rh = p->AddDataResource(rid,
            GFxImageFileResourceCreator::CreateImageFileResourceData(pfi));
    }
    return rh;
}

void    GSTDCALL GFx_DefineExternalImageLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DefineExternalImage);

    // Utilizes the tag 1001 (unused in normal SWF): the format is as follows:
    // Header           RECORDHEADER    1001
    // CharacterID      UI16
    // BitmapsFormat    UI16            0 - Default, as in 1001 tag
    //                                  1 - TGA
    //                                  2 - DDS
    // TargetWidth      UI16
    // TargetHeight     UI16
    // FileNameLen      UI8             without extension, only name
    // FileName         UI8[FileNameLen]

    GFxStream* pin = p->GetStream();

    UInt    bitmapResourceId    = p->ReadU16();
    UInt16  bitmapFormat        = p->ReadU16();
    UInt16  origWidth           = p->ReadU16();
    UInt16  origHeight          = p->ReadU16();

    GString imageExportName, imageFileName;
    pin->ReadStringWithLength(&imageExportName);
    pin->ReadStringWithLength(&imageFileName);

    pin->LogParse("  DefineExternalImage: tagInfo.TagType = %d, id = 0x%X, fmt = %d, name = '%s', exp = '%s', w = %d, h = %d\n",
        tagInfo.TagType,
        bitmapResourceId,
        bitmapFormat,
        imageFileName.ToCStr(),
        imageExportName.ToCStr(),
        origWidth,
        origHeight);

    // Register the image file info.
    GFx_CreateImageFileResourceHandle(p, GFxResourceId(bitmapResourceId),
        imageFileName.ToCStr(), imageExportName.ToCStr(), bitmapFormat,
        origWidth, origHeight);
}

void    GSTDCALL GFx_DefineExternalImageLoader2(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DefineExternalImage2);

    // Utilizes the tag 1009 (unused in normal SWF): the format is as follows:
    // Header           RECORDHEADER    1009
    // CharacterID      UI32
    // BitmapsFormat    UI16            0 - Default, as in 1001 tag
    //                                  1 - TGA
    //                                  2 - DDS
    // TargetWidth      UI16
    // TargetHeight     UI16
    // FileNameLen      UI8             without extension, only name
    // FileName         UI8[FileNameLen]

    UInt16 version = p->GetExporterInfo()->Version;
    GFxStream* pin = p->GetStream();

    UInt    bitmapResourceId    = p->ReadU32();
    UInt16  bitmapFormat        = p->ReadU16();
    UInt16  origWidth           = p->ReadU16();
    UInt16  origHeight          = p->ReadU16();

    GString imageExportName, imageFileName;
    pin->ReadStringWithLength(&imageExportName);
    pin->ReadStringWithLength(&imageFileName);

    UInt8 flags;
    if (version >= 0x036e) //for compatibility with gfx file generated by earlier 3.1 
        flags = p->ReadU8();
    else
        flags =  GFxImageFileInfo::Wrappable;


    pin->LogParse("  DefineExternalImage2: tagInfo.TagType = %d, id = 0x%X, fmt = %d, name = '%s', exp = '%s', w = %d, h = %d\n",
        tagInfo.TagType,
        bitmapResourceId,
        bitmapFormat,
        imageFileName.ToCStr(),
        imageExportName.ToCStr(),
        origWidth,
        origHeight);

    // Register the image file info.
    GFx_CreateImageFileResourceHandle(p, GFxResourceId(bitmapResourceId & (0xFFFF | GFxResourceId::IdType_DynFontImage)),
                                      imageFileName.ToCStr(), imageExportName.ToCStr(), bitmapFormat,
                                      origWidth, origHeight, flags);
}

void    GSTDCALL GFx_DefineSubImageLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DefineSubImage);

    // utilizes the tag 1008 (unused in normal SWF): the format is as follows:
    // Header           RECORDHEADER    1008
    // CharacterID      UI16
    // ImageCharacterID UI16
    // Left             UI16
    // Top              UI16
    // Right            UI16
    // Bottom           UI16

    UInt    bitmapResourceId    = p->ReadU16();
    UInt    baseImageId         = p->ReadU16();
    UInt16  left                = p->ReadU16();
    UInt16  top                 = p->ReadU16();
    UInt16  right               = p->ReadU16();
    UInt16  bottom              = p->ReadU16();

    GPtr<GFxSubImageResourceInfo> psubimage = *new GFxSubImageResourceInfo;
    psubimage->ImageId = GFxResourceId(baseImageId | GFxResourceId::IdType_DynFontImage);
    psubimage->Rect = GRect<SInt>(left, top, right, bottom);

    GFxResourceData rdata = GFxSubImageResourceCreator::CreateSubImageResourceData(psubimage);
    p->AddDataResource(GFxResourceId(bitmapResourceId), rdata);
}

#ifndef GFC_NO_SOUND

void  GSTDCALL GFx_StartSoundLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
// Load a StartSound tag.
{
    GUNUSED(tagInfo);    
    GASSERT(tagInfo.TagType == GFxTag_StartSound);
    GFxAudioBase* paudio = p->GetLoadStates()->GetAudio();
    if (paudio)
    {
        GASSERT(paudio->GetSoundTagsReader());
        paudio->GetSoundTagsReader()->ReadStartSoundTag(p,tagInfo);
    }
    else
    {
        p->GetStream()->LogParse("GFx_StartSoundLoader: Audio library is not set.\n");
        p->GetStream()->LogTagBytes();
    }
}

void  GSTDCALL GFx_DefineSoundLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
// Load a DefineSound tag.
{
    GUNUSED(tagInfo); 
    GASSERT(tagInfo.TagType == GFxTag_DefineSound);
    GFxAudioBase* paudio = p->GetLoadStates()->GetAudio();
    if (paudio)
    {
        GASSERT(paudio->GetSoundTagsReader());
        paudio->GetSoundTagsReader()->ReadDefineSoundTag(p,tagInfo);
    }
    else
    {
        p->GetStream()->LogParse("GFx_DefineSoundLoader: Audio library is not set.\n");
        p->GetStream()->LogTagBytes();
    }
}

void    GSTDCALL GFx_ButtonSoundLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo); 
    GASSERT(tagInfo.TagType == GFxTag_ButtonSound);
    GFxAudioBase* paudio = p->GetLoadStates()->GetAudio();
    if (paudio)
    {
        GASSERT(paudio->GetSoundTagsReader());
        paudio->GetSoundTagsReader()->ReadButtonSoundTag(p,tagInfo);
    }
    else
    {
        p->GetStream()->LogParse("GFx_ButtonSoundLoader: Audio library is not set.\n");
        p->GetStream()->LogTagBytes();
    }

}
void GSTDCALL GFx_SoundStreamHeadLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);    
    GASSERT(tagInfo.TagType == GFxTag_SoundStreamHead || tagInfo.TagType == GFxTag_SoundStreamHead2);

    GFxAudioBase* paudio = p->GetLoadStates()->GetAudio();
    if (paudio)
    {
        GASSERT(paudio->GetSoundTagsReader());
        paudio->GetSoundTagsReader()->ReadSoundStreamHeadTag(p,tagInfo);
    }
    else
    {
        p->GetStream()->LogParse("GFx_SoundStreamHeadLoader: Audio library is not set.\n");
        p->GetStream()->LogTagBytes();
    }
}

void GSTDCALL GFx_SoundStreamBlockLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);    
    GASSERT(tagInfo.TagType == GFxTag_SoundStreamBlock);

    GFxAudioBase* paudio = p->GetLoadStates()->GetAudio();
    if (paudio)
    {
        GASSERT(paudio->GetSoundTagsReader());
        paudio->GetSoundTagsReader()->ReadSoundStreamBlockTag(p,tagInfo);
    }
    else
    {
        p->GetStream()->LogParse("GFx_SoundStreamBlockLoader: Audio library is not set.\n");
        p->GetStream()->LogTagBytes();
    }
}

void    GSTDCALL GFx_DefineExternalSoundLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DefineExternalSound);

    GFxAudioBase* paudio = p->GetLoadStates()->GetAudio();
    if (paudio)
    {
        GASSERT(paudio->GetSoundTagsReader());
        paudio->GetSoundTagsReader()->ReadDefineExternalSoundTag(p,tagInfo);
    }
    else
    {
        p->GetStream()->LogParse("GFx_DefineExternalSoundLoader: Audio library is not set.\n");
        p->GetStream()->LogTagBytes();
    }
}

void    GSTDCALL GFx_DefineExternalStreamSoundLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DefineExternalStreamSound);
    GFxAudioBase* paudio = p->GetLoadStates()->GetAudio();
    if (paudio)
    {
        GASSERT(paudio->GetSoundTagsReader());
        paudio->GetSoundTagsReader()->ReadDefineExternalStreamSoundTag(p,tagInfo);
    }
    else
    {
        p->GetStream()->LogParse("GFx_DefineExternalStreamSoundLoader: Audio library is not set.\n");
        p->GetStream()->LogTagBytes();
    }
}
#endif // GFC_NO_SOUND

void    GSTDCALL GFx_FontTextureInfoLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_FontTextureInfo);

    // Glyphs' texture info tags
    // utilizes the tag 1002 (unused in normal SWF): the format is as follows:
    // Header           RECORDHEADER    1002
    // TextureID        UI32            Texture ID
    // TextureFormat    UI16            0 - Default, as in 1001 tag
    //                                  1 - TGA
    //                                  2 - DDS
    // FileNameLen      UI8             name of file with texture's image (without extension)
    // FileName         UI8[FileNameLen]
    // TextureWidth     UI16
    // TextureHeight    UI16
    // PadPixels        UI8             
    // NominalGlyphSz   UI16            Nominal height of glyphs
    // NumTexGlyphs     UI16            Number of texture glyphs
    // TexGlyphs        TEXGLYPH[NumTexGlyphs]
    // NumFonts         UI16            Number of fonts using this texture
    // Fonts            FONTINFO[NumFonts]  Font infos
    //
    // FONTINFO
    // FontId           UI16
    // NumGlyphs        UI16            Number of texture glyphs in the font from the current texture
    // GlyphIndices     GLYPHIDX[NumGlyphs] Mapping of font glyph's indices to textures' ones (TexGlyphs)
    //
    // GLYPHIDX
    // IndexInFont      UI16            Index in font
    // IndexInTexture   UI16            Index in texture
    //
    // TEXGLYPH:
    // UvBoundsLeft     FLOAT
    // UvBoundsTop      FLOAT
    // UvBoundsRight    FLOAT
    // UvBoundsBottom   FLOAT
    // UvOriginX        FLOAT
    // UvOriginY        FLOAT

    GFxStream*      pin = p->GetStream();    
    GMemoryHeap*    pheap = p->GetLoadHeap();
    
    UInt    textureId       = pin->ReadU32();
    UInt16  bitmapFormat    = pin->ReadU16();    
    GString imageFileName;
    pin->ReadStringWithLength(&imageFileName);

    // Load texture glyph configuration.
    GFxFontPackParams::TextureConfig  tgc;
    tgc.TextureWidth   = pin->ReadU16();
    tgc.TextureHeight  = pin->ReadU16();
    tgc.PadPixels      = pin->ReadU8();
    tgc.NominalSize    = pin->ReadU16();

    pin->LogParse("  FontTextureInfo: tagInfo.TagType = %d, id = 0x%X, fmt = %d, name = '%s', w = %d, h = %d\n",
                  tagInfo.TagType, textureId,
                  bitmapFormat, imageFileName.ToCStr(),
                  tgc.TextureWidth, tgc.TextureHeight);


    // Fill in file information.
    GASSERT((textureId & GFxResourceId::IdType_FontImage) == GFxResourceId::IdType_FontImage);
    GFxResourceHandle rh = GFx_CreateImageFileResourceHandle(
                                p, GFxResourceId(textureId),
                                imageFileName.ToCStr(), "", bitmapFormat,
                                (UInt16) tgc.TextureWidth, (UInt16) tgc.TextureHeight);   

    // Load texture glyphs info first
    GArray<GPtr<GFxTextureGlyph> > texGlyphsInTexture;
    UInt numTexGlyphs = pin->ReadU16();
    UInt i;

    pin->LogParse("  PadPixels = %d, nominal glyph size = %d, numTexGlyphs = %d\n",
                  tgc.PadPixels, tgc.NominalSize, numTexGlyphs);

    for (i = 0; i < numTexGlyphs; ++i)
    {
        // load TEXGLYPH
        Float uvBoundsLeft      = pin->ReadFloat();
        Float uvBoundsTop       = pin->ReadFloat();
        Float uvBoundsRight     = pin->ReadFloat();
        Float uvBoundsBottom    = pin->ReadFloat();
        Float uvOriginX         = pin->ReadFloat();
        Float uvOriginY         = pin->ReadFloat();

        pin->LogParse("  TEXGLYPH[%d]: uvBnd.Left = %f, uvBnd.Top = %f,"
                      " uvBnd.Right = %f, uvBnd.Bottom = %f\n",
                      i, uvBoundsLeft, uvBoundsTop, uvBoundsRight, uvBoundsBottom);
        pin->LogParse("                uvOrigin.x = %f, uvOrigin.y = %f\n",
                      uvOriginX, uvOriginY);

        // create GFxTextureGlyph
        GPtr<GFxTextureGlyph> ptexGlyph = *GHEAP_NEW(pheap) GFxTextureGlyph();
        ptexGlyph->SetImageResource(rh);
        ptexGlyph->UvBounds = GRenderer::Rect(uvBoundsLeft, uvBoundsTop,
                                              uvBoundsRight, uvBoundsBottom);
        ptexGlyph->UvOrigin = GRenderer::Point(uvOriginX, uvOriginY);
        texGlyphsInTexture.PushBack(ptexGlyph);
    }

    // load fonts' info
    UInt numFonts = pin->ReadU16();
    pin->LogParse("  NumFonts = %d\n", numFonts);
    
    for (i = 0; i < numFonts; ++i)
    {
        // Load FONTINFO
        UInt16       fontId = pin->ReadU16();
        GFxFont* pfd    = p->GetFontData(GFxResourceId(fontId));
        if (!pfd)
        {
            GFC_DEBUG_WARNING1(1, "FontTextureInfoLoader: can't find font, fontId = (%d)\n",
                                  fontId);
            continue;
        }

        // Get/Create font texture data.
        // When fonts are spread among multiple textures, texture glyphs would
        // already be created by the previous tag.
        GPtr<GFxTextureGlyphData> ptextureGlyphData = pfd->GetTextureGlyphData();
        if (!ptextureGlyphData)
        {
            if (ptextureGlyphData = *GHEAP_NEW(pheap) GFxTextureGlyphData(pfd->GetGlyphShapeCount(), true))
            {
                ptextureGlyphData->SetTextureConfig(tgc);
                pfd->SetTextureGlyphData(ptextureGlyphData);
            }
        }
        
        if (ptextureGlyphData)
            ptextureGlyphData->AddTexture(GFxResourceId(textureId), rh);
        
        UInt numGlyphsInFont = pin->ReadU16();
        for (UInt i = 0; i < numGlyphsInFont; ++i)
        {
            // load GLYPHIDX
            UInt indexInFont    = pin->ReadU16();
            UInt indexInTexture = pin->ReadU16();
            
            if (ptextureGlyphData)
                ptextureGlyphData->AddTextureGlyph(
                    indexInFont, *texGlyphsInTexture[indexInTexture].GetPtr());
        }
    }

    pin->LogParse("\n");
}


void    GSTDCALL GFx_DefineExternalGradientImageLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DefineExternalGradient);

    // Utilizes the tag 1003 (unused in normal SWF): the format is as follows:
    // Header           RECORDHEADER    1001
    // GradientID       UI16
    // BitmapsFormat    UI16            0 - Default, as in 1001 tag
    //                                  1 - TGA
    //                                  2 - DDS
    // GradientSize     UI16
    // FileNameLen      UI8             without extension, only name
    // FileName         UI8[FileNameLen]

    GFxStream*  pin              = p->GetStream();

    UInt        gradientId       = pin->ReadU16();
    UInt        bitmapResourceId = gradientId | GFxResourceId::IdType_GradientImage;
    UInt16      bitmapFormat     = pin->ReadU16();
    UInt16      gradSize         = pin->ReadU16();
    
    GString   imageFileName;
    pin->ReadStringWithLength(&imageFileName);

    pin->LogParse("  DefineExternalGradientImage: tagInfo.TagType = %d, id = 0x%X, fmt = %d, name = '%s', size = %d\n",
        tagInfo.TagType,
        bitmapResourceId,
        bitmapFormat,
        imageFileName.ToCStr(),
        gradSize);

    // Register image resource data.
    GFx_CreateImageFileResourceHandle(p, GFxResourceId(bitmapResourceId),
                                      imageFileName.ToCStr(), "", bitmapFormat,
                                      0, 0);
    // GFxImageCreator::CreateImage will substute targetWidth/targetHeight with real
    // values (we pass 0,0 since we don't know either this is a linear or radial gradient)
}


void    GSTDCALL GFx_DefineGradientMapLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);    
    GASSERT(tagInfo.TagType == GFxTag_DefineGradientMap);

    p->LogWarning("Deprecated tag 1004 - DefineGradientMapLoader encountered, ignored\n");

    /*

    // Utilizes the tag 1004 (unused in normal SWF): the format is as follows:
    // Header           RECORDHEADER    1004
    // NumGradients     UI16            
    // Indices          UI16[NumGradients]

    UInt    numGradients = pin->ReadU16();

    pin->LogParse("  DefineGradientMap: tagInfo.TagType = %d, num = %d\n",
        tagInfo.TagType,
        numGradients);
    GFxGradientParams* pgrParams = pin->GetLoader()->GetGradientParams();
    if (pgrParams == 0)
    {
        GFxGradientParams grParams;
        pin->GetLoader()->SetGradientParams(&grParams);
        pgrParams = pin->GetLoader()->GetGradientParams();
    }
    GASSERT(pgrParams);
    pgrParams->GradientMap.SetSize(numGradients);

    for (UInt i = 0; i < numGradients; ++i)
    {
        UInt16 v = pin->ReadU16();
        pgrParams->GradientMap.pData[i] = v;
    }
    */
}


void GSTDCALL GFx_Scale9GridLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DefineScale9Grid);

    GFxStream*  pin = p->GetStream();    

    GRectF r;
    UInt  refId = pin->ReadU16();
                  pin->ReadRect(&r);

    if (pin->IsVerboseParse())
    {
        p->LogParse("Scale9GridLoader, id=%d, x=%d, y=%d, w=%d, h=%d\n", 
            refId, int(r.Left), int(r.Top), int(r.Right), int(r.Bottom));
    }

    if (r.Left >= r.Right)
    {
        p->LogWarning("Scale9Grid has negative width %f\n", (r.Right - r.Left) / 20);
        return;
    }

    if (r.Top >= r.Bottom)
    {
        p->LogWarning("Scale9Grid has negative height %f\n", (r.Bottom - r.Top) / 20);
        return;
    }

    GFxResourceHandle handle;
    if (p->GetResourceHandle(&handle, GFxResourceId(refId)))
    {
        GFxResource* rptr = handle.GetResourcePtr();
        if (rptr)
        { 
            if (rptr->GetResourceType() == GFxResource::RT_SpriteDef)
            {
                GFxSpriteDef* sp = static_cast<GFxSpriteDef*>(rptr);
                sp->SetScale9Grid(r);
            }
            else if (rptr->GetResourceType() == GFxResource::RT_ButtonDef)
            {
                GFxButtonCharacterDef* b = static_cast<GFxButtonCharacterDef*>(rptr);
                b->SetScale9Grid(r);
            }
        }
    }
    
}
