/**********************************************************************

Filename    :   GFxJpegSupport.cpp
Content     :   
Created     :   April 15, 2008
Authors     :   

Notes       :   Redesigned to use inherited state objects for GFx 2.2.

Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

///////////// WARNINNG ///////////////////////////////////////////////
// The implementation of GFxJpegSupport class is moved into a separate 
// file to avoid linking of JPEG functionality into the application if 
// it does't need JPED support and GFxJpegSupport state is not set to 
// the loader

#include "GArray.h"
#include "GAutoPtr.h"
#include "GFxLoader.h"
#include "GJPEGUtil.h"
#include "GImage.h"
#include "GFile.h"

GFxJpegSupport::GFxJpegSupport()
{
    pSystem = *GJPEGSystem::CreateDefaultSystem();
}
// Create and read a new image from the stream.
GImage* GFxJpegSupport::ReadJpeg(GFile* pin, GMemoryHeap* pimageHeap)
{
    if (!pSystem)
    {
        GFC_DEBUG_WARNING1(1, "jpeglib is not installed - can't process jpeg image data \"%s\"\n", pin->GetFilePath());
        return NULL;
    }
    return GImage::ReadJpeg(pin, pSystem,pimageHeap);
}

// Write image to the stream.
bool GFxJpegSupport::WriteJpeg(GFile* pout, int quality, GImage* image)
{
    if (image == NULL)
    {
        return false;
    }

    if (!pSystem)
    {
        GFC_DEBUG_WARNING1(1, "jpeglib is not installed - can't process jpeg image data \"%s\"\n", pout->GetFilePath());
        return false;
    }

    return image->WriteJpeg(pout, quality, pSystem);
}


// Read SWF JPEG2-style Header (separate encoding
// table followed by image data), and create jpeg
// input object.
GJPEGInput* GFxJpegSupport::CreateSwfJpeg2HeaderOnly(GFile* pin)
{
    if (!pSystem)
    {
        GFC_DEBUG_WARNING1(1, "jpeglib is not installed - can't process jpeg image data \"%s\"\n", pin->GetFilePath());
        return NULL;
    }
    return pSystem->CreateSwfJpeg2HeaderOnly(pin);
}

// Create and read a new image from the stream.  Image is pin
// SWF JPEG2-style Format (the encoding tables come first pin a
// separate "stream" -- otherwise it's just normal JPEG).  The
// IJG documentation describes this as "abbreviated" format.
GImage* GFxJpegSupport::ReadSwfJpeg2(GFile* pin, GMemoryHeap* pimageHeap)
{
    GScopePtr<GJPEGInput> pjin (CreateSwfJpeg2HeaderOnly(pin));
    if (!pjin) return 0;

    GImage* pimage;
    pimage = pjin->IsErrorOccurred() ? NULL : ReadSwfJpeg2WithTables(pjin.GetPtr(), pimageHeap);
    return pimage;
}

// Create and read a new image, using a input object that
// already has tables loaded.
GImage* GFxJpegSupport::ReadSwfJpeg2WithTables(GJPEGInput* pjin, GMemoryHeap* pimageHeap)
{
    if (!pjin || pjin->IsErrorOccurred())
        return NULL;

    pjin->StartImage();

    GImage* pimage = GImage::CreateImage(GImage::Image_RGB_888,
                                         pjin->GetWidth(), pjin->GetHeight(),
                                         pimageHeap);
    if (pimage)
    {
        for (UInt y = 0; y < pjin->GetHeight(); y++)
        {
            if (!pjin->ReadScanline(pimage->GetScanline(y)))
            {
                pimage->Release();
                pimage = NULL;
                break;
            }
        }
    }

    pjin->FinishImage();
    return pimage;
}

// For reading SWF JPEG3-style image data, like ordinary JPEG, 
// but stores the data pin GImage format.
GImage* GFxJpegSupport::ReadSwfJpeg3(GFile* pin, GMemoryHeap* pimageHeap)
{
    GScopePtr<GJPEGInput> pjin (CreateSwfJpeg2HeaderOnly(pin));
    if (!pjin) return NULL;

    GImage* pimage = NULL;
    if (!pjin->IsErrorOccurred())
    {
        if (pjin->StartImage())
        {
            pimage = GImage::CreateImage(GImage::Image_ARGB_8888,
                                         pjin->GetWidth(), pjin->GetHeight(),
                                         pimageHeap);
            if (pimage)
            {
                UInt    width = pimage->Width; // Cache width
                UByte*  line = (UByte*)GALLOC(3*width, GStat_Default_Mem);

                for (UInt y = 0; y < pjin->GetHeight(); y++) 
                {
                    if (!pjin->ReadScanline(line))
                    {
                        pimage->Release();
                        pimage = NULL;
                        break;
                    }

                    UByte*  data = pimage->GetScanline(y);
                    for (UInt x = 0; x < width; x++) 
                    {
                        data[4*x+0] = line[3*x+0];
                        data[4*x+1] = line[3*x+1];
                        data[4*x+2] = line[3*x+2];
                        data[4*x+3] = 255;
                    }
                }

                GFREE(line);
            }

            pjin->FinishImage();
        }
    }
    return pimage;
}


