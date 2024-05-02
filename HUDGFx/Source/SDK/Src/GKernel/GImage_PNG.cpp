/**********************************************************************

Filename    :   GImage_PNG.cpp
Content     :   GImage PNG support
Created     :   
Authors     :   Artem Bolgar

Copyright   :   (c) 2001-2008 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

///////////// WARNINNG ///////////////////////////////////////////////
// The implementation of GImage::ReadPng method is moved into a separate 
// file to avoid linking PNG functionality into the application if 
// it does't need PNG support

#include "GImage.h"
#include "GSysFile.h"
#include "GStd.h"

#ifdef GFC_USE_LIBPNG
#include "png.h"

#ifdef GFC_CC_MSVC
// disable warning "warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable"
#pragma warning(disable:4611)
#endif

struct GFxPngContext
{
    png_structp         png_ptr;
    png_infop           info_ptr;
    png_uint_32         width, height; 
    int                 bitDepth;
    int                 colorType;
    png_uint_32         ulRowBytes;
    char                errorMessage[100];
    char                filePath[256];
};

static void
png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    GFile* pfile = reinterpret_cast<GFile*>(png_ptr->io_ptr);
    SInt check = pfile->Read(data, (SInt)length);

    if (check < 0 || ((png_size_t)check) != length)
    {
        png_error(png_ptr, "Read Error.");
    }
}

static void
png_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    GFile* pfile = reinterpret_cast<GFile*>(png_ptr->io_ptr);
    SInt check = pfile->Write(data, (SInt)length);

    if (check < 0 || ((png_size_t)check) != length)
    {
        png_error(png_ptr, "Write Error.");
    }
}

static void 
png_error_handler(png_structp png_ptr, png_const_charp msg)
{
    UPInt mlen = G_strlen(msg);

    GFxPngContext *pcontext = (GFxPngContext*)png_get_error_ptr(png_ptr);
    if (mlen < sizeof(pcontext->errorMessage))
        G_strcpy(pcontext->errorMessage, sizeof(pcontext->errorMessage), msg);
    else
    {
        G_strncpy(pcontext->errorMessage, sizeof(pcontext->errorMessage), msg, sizeof(pcontext->errorMessage) - 1);
        pcontext->errorMessage[sizeof(pcontext->errorMessage) - 1] = 0;
    }
    longjmp(png_ptr->jmpbuf, 1);
}

static
int GFxPngReadInfo(GFxPngContext* context)
{
    double              dGamma;

    if (setjmp(png_jmpbuf(context->png_ptr)))
    {
        GFC_DEBUG_WARNING2(1, "GImage::ReadPng failed - Can't read info from file %s, error - %s\n", 
            context->filePath, context->errorMessage);
        return 0;
    }

    // initialize the png structure

    png_set_sig_bytes(context->png_ptr, 8);

    // read all PNG info up to image data

    png_read_info(context->png_ptr, context->info_ptr);

    // get width, height, bit-depth and color-type

    png_get_IHDR(context->png_ptr, context->info_ptr, &context->width, &context->height, &context->bitDepth,
        &context->colorType, NULL, NULL, NULL);

    // expand images of all color-type and bit-depth to 3x8 bit RGB images
    // let the library process things like alpha, transparency, background

    // convert to 8-bit per color component
    if (context->bitDepth == 16)
        png_set_strip_16(context->png_ptr);

    // convert palette  (8-bit per pixel) to RGB
    if (context->colorType == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(context->png_ptr);

    if (context->bitDepth < 8)
#if (PNG_LIBPNG_VER_SONUM >= 14)
        png_set_expand_gray_1_2_4_to_8(context->png_ptr);
#else
        png_set_gray_1_2_4_to_8(context->png_ptr);
#endif

    if (png_get_valid(context->png_ptr, context->info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(context->png_ptr);

    if (context->colorType == PNG_COLOR_TYPE_GRAY ||
        context->colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(context->png_ptr);


    //!AB: we don't need to set background since this will replace transparent pixels
    // by the background colored ones.
    // set the background color to draw transparent and alpha images over.
    //png_color_16 *pBackground;
    //if (png_get_bKGD(context->png_ptr, context->info_ptr, &pBackground))
    //{
    //    png_set_background(context->png_ptr, pBackground, PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
    //}

    // if required set gamma conversion
    if (png_get_gAMA(context->png_ptr, context->info_ptr, &dGamma))
        png_set_gamma(context->png_ptr, (double) 2.2, dGamma);

    // after the transformations have been registered update info_ptr data

    png_read_update_info(context->png_ptr, context->info_ptr);

    // get again width, height and the new bit-depth and color-type

    png_get_IHDR(context->png_ptr, context->info_ptr, &context->width, &context->height, &context->bitDepth,
        &context->colorType, NULL, NULL, NULL);

    // row_bytes is the width x number of channels

    context->ulRowBytes = png_get_rowbytes(context->png_ptr, context->info_ptr);

    return 1;
}

static
int GFxPngReadData(GFxPngContext* context, png_byte **ppbRowPointers)
{
    if (setjmp(png_jmpbuf(context->png_ptr)))
    {
        GFC_DEBUG_WARNING2(1, "GImage::ReadPng failed - Can't read data from file %s, error - %s\n", 
            context->filePath, context->errorMessage);
        return 0;
    }

    // now we can go ahead and just read the whole image

    png_read_image(context->png_ptr, ppbRowPointers);

    // read the additional chunks in the PNG file (not really needed)

    png_read_end(context->png_ptr, NULL);

    return 1;
}

// Create and read a new image from the stream.
GImage* GImage::ReadPng(GFile* pin, GMemoryHeap* pimageHeap)
{
    png_byte            pbSig[8];
    png_byte           *pbImageData; // = *ppbImageData;
    png_byte            **ppbRowPointers = NULL;
    ImageFormat         destFormat;
    UInt                srcScanLineSize = 0;

    if (!pin || !pin->IsValid()) 
        return NULL;

    GFxPngContext context;
    memset(&context, 0, sizeof(context));

    const char* ppath = pin->GetFilePath();

    G_strcpy(context.filePath, sizeof(context.filePath), ppath);

    // first check the eight byte PNG signature

    if (pin->Read(pbSig, 8) != 8 || !png_check_sig(pbSig, 8))
    {
        GFC_DEBUG_WARNING1(1, "GImage::ReadPng failed - Can't read signature from file %s\n", 
            context.filePath);
        return NULL;
    }

    // create the two png(-info) structures

    context.png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)&context,  
                                                                    (png_error_ptr)png_error_handler, NULL);
    if (!context.png_ptr)
    {
        GFC_DEBUG_WARNING1(1, "GImage::ReadPng failed - Can't create read struct for file %s\n", 
            context.filePath);
        return NULL;
    }

    context.info_ptr = png_create_info_struct(context.png_ptr);
    if (!context.info_ptr)
    {
        GFC_DEBUG_WARNING1(1, "GImage::ReadPng failed - Can't create info struct for file %s\n", 
            context.filePath);
        png_destroy_read_struct(&context.png_ptr, NULL, NULL);
        return NULL;
    }

    png_set_read_fn(context.png_ptr, (png_voidp)pin, png_read_data);

    if (!GFxPngReadInfo(&context))
    {
        // context.errorMessage contains an error message
        png_destroy_read_struct(&context.png_ptr, &context.info_ptr, NULL);
        return NULL;
    }

    switch(context.colorType)
    {
        case PNG_COLOR_TYPE_RGB:        destFormat = Image_RGB_888;   srcScanLineSize = context.width*3; break;
        case PNG_COLOR_TYPE_RGB_ALPHA:  destFormat = Image_ARGB_8888; srcScanLineSize = context.width*4; break;
        default: destFormat = Image_None; break;
    }
    if (context.ulRowBytes != 0) // can context.ulRowBytes be zero? 
        srcScanLineSize = context.ulRowBytes;
    
    // align srcScanLineSize on boundary of 4
    srcScanLineSize = (srcScanLineSize + 3) & (~3u);

    if (destFormat != Image_None)
    {
        // now we can allocate memory to store the image
        GPtr<GImage> pimage = *CreateImage(destFormat, context.width, context.height, pimageHeap);
        if (pimage)
        {
            pbImageData = (png_byte*)pimage->GetScanline(0);

            // and allocate memory for an array of row-pointers

            if ((ppbRowPointers = (png_bytepp) GALLOC((context.height)
                * sizeof(png_bytep), GStat_Default_Mem)) == NULL)
            {
                GFC_DEBUG_WARNING1(1, "GImage::ReadPng failed - Out of memory, file %s\n", 
                    context.filePath);
                png_destroy_read_struct(&context.png_ptr, &context.info_ptr, NULL);
                return NULL;
            }

            // set the individual row-pointers to point at the correct offsets

            for (UInt i = 0; i < context.height; i++)
                ppbRowPointers[i] = pbImageData + i * srcScanLineSize;

            if (!GFxPngReadData(&context, ppbRowPointers))
            {
                png_destroy_read_struct(&context.png_ptr, &context.info_ptr, NULL);
                GFREE(ppbRowPointers);
                return NULL;
            }
            png_destroy_read_struct(&context.png_ptr, &context.info_ptr, NULL);

            GFREE(ppbRowPointers);

            pimage->AddRef();
            return pimage;
        }
    }
    png_destroy_read_struct(&context.png_ptr, &context.info_ptr, NULL);
    return NULL;   
}

GImage* GImage::ReadPng(const char* filename, GMemoryHeap* pimageHeap)
{
    GSysFile in(filename);
    if (in.IsValid())
        return ReadPng(&in, pimageHeap);
    return 0;
}

bool GImage::WritePng(GFile* pout)
{
    GFxPngContext context;
    png_bytep* ppbRowPointers = 0;

    const char* ppath = pout->GetFilePath();
    G_strcpy(context.filePath, sizeof(context.filePath), ppath);
    context.width = Width;
    context.height = Height;

    GPtr<GImage> pdstImage; 
    switch (Format)
    {
    case Image_ARGB_8888:
        context.colorType = PNG_COLOR_TYPE_RGB_ALPHA;
        pdstImage = this;
        break;
    case Image_RGB_888:
        context.colorType = PNG_COLOR_TYPE_RGB;
        pdstImage = this;
        break;
    case Image_L_8:
        context.colorType = PNG_COLOR_TYPE_GRAY;
        pdstImage = this;
        break;
    default:
        context.colorType = PNG_COLOR_TYPE_RGB_ALPHA;
        pdstImage = *ConvertImage(Image_ARGB_8888);
        if (!pdstImage)
        {
            GFC_DEBUG_WARNING(1, "WritePng: ConvertImage(Image_ARGB_8888) failed.\n");
            return false;
        }
    }
    context.bitDepth = 8; //Always 8 in GImage (?)
    context.png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)&context,  
        (png_error_ptr)png_error_handler, NULL);

    if (!context.png_ptr)
    {
        GFC_DEBUG_WARNING1(1, "WritePng(%s): png_create_write_struct failed.\n", context.filePath);
        return false;
    }

    context.info_ptr = png_create_info_struct(context.png_ptr);

    if (!context.info_ptr)
    {
        GFC_DEBUG_WARNING1(1, "WritePng(%s): png_create_info_struct failed.\n", context.filePath);
        return false;
    }

    png_set_write_fn(context.png_ptr, (png_voidp)pout, png_write_data, NULL);

    if (setjmp(png_jmpbuf(context.png_ptr)))
    {
        GFC_DEBUG_WARNING2(1, "WritePng(%s): Writing header failed. Error: %s\n",
            context.filePath, context.errorMessage);
        return false;
    }

    png_set_IHDR(context.png_ptr, context.info_ptr, Width, Height,
        context.bitDepth, context.colorType, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(context.png_ptr, context.info_ptr);


    if (setjmp(png_jmpbuf(context.png_ptr)))
    {
        GFC_DEBUG_WARNING2(1, "WritePng(%s): Writing image data failed. Error: %s\n",
            context.filePath, context.errorMessage);
        if (ppbRowPointers)
            GFREE(ppbRowPointers);
        return false;
    }

    ppbRowPointers = (png_bytep*) GALLOC(sizeof(png_bytep) * Height, GStat_Default_Mem);
    for (UInt i = 0; i < context.height; i++)
        ppbRowPointers[i] = pdstImage->GetScanline(i); //(png_byte*) malloc(info_ptr->rowbytes);

    png_write_image(context.png_ptr, ppbRowPointers);
    GFREE(ppbRowPointers);

    if (setjmp(png_jmpbuf(context.png_ptr)))
    {
        GFC_DEBUG_WARNING(1, "WritePng: Error during end of write");
        return false;
    }

    png_write_end(context.png_ptr, NULL);

    return true;
}

#endif // GFC_USE_LIBPNG
