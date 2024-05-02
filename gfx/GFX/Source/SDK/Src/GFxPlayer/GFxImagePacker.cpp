/**********************************************************************

Filename    :   GFxImagePacker.cpp
Content     :   
Created     :   
Authors     :   Andrew Reisse

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.


Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxImagePacker.h"
#include "GHeapNew.h"

GFxImagePacker* GFxImagePackParams::Begin(GFxResourceId* pIdGen, GFxImageCreator* pic,
                                          GFxImageCreateInfo* pici) const
{
    return new GFxImagePackerImpl(this, pIdGen, pic, pici);
}

void GFxImagePackerImpl::AddResource( GFxResourceDataNode* presNode, GFxImageResource* presource )
{
    InputResources.Set(presource, presNode);
}

void GFxImagePackerImpl::AddImageFromResource( GFxImageResource* presource, const char *pexportname )
{
    if (pexportname && strstr(pexportname, "-nopack"))
        return;
    GFxResourceDataNode** ppresNode = InputResources.Get(presource);
    if (ppresNode)
    {
        GImageInfoBase *pimageinfo = presource->GetImageInfo();
        if (pimageinfo->GetImageInfoType() == GImageInfoBase::IIT_ImageInfo)
        {
            GFxImageSubstProvider* pisp = pImpl->GetImageSubstProvider(); 
            GPtr<GImage> pimage = 0;
            GImage *peimage = ((GImageInfo*) pimageinfo)->GetImage();
            GASSERT(peimage);
            if (pisp && pexportname)
            {
                pimage = *pisp->CreateImage(pexportname);
                if (!pimage)
                {
                    fprintf(stderr, "\nWarning: Can't find substitution image for '%s'.\n ", pexportname);
                }
                else if ((pimage->Width != peimage->Width) || (pimage->Height != peimage->Height) )
                {
                    fprintf(stderr, "\nWarning: Substitution image for '%s' has different resolution.\n " 
                        "Embedded image will be used.\n", pexportname);
                    pimage = peimage;
                }
            }
            if (!pimage)
                pimage = peimage;
            if (pimage && (pimage->Format == GImage::Image_ARGB_8888 ||
                pimage->Format == GImage::Image_RGB_888))
            {
                InputImage in;
                in.pImage = pimage;
                in.pResNode = *ppresNode;
                InputImages.PushBack(in);
            }
            else
                GFC_DEBUG_WARNING(1, "GFxImagePacker: ImageCreator did not provide uncompressed GImage, not packing");
        }
        else
            GFC_DEBUG_WARNING(1, "GFxImagePacker: ImageCreator did not provide GImageInfo, not packing");
    }
}

void GFxImagePackerImpl::CopyImage(GImage* pdest, GImageBase* psrc, GRectPacker::RectType rect)
{
    const UByte* psrcrow = psrc->GetScanline(0);
    UInt destpitch = pdest->GetPitch();
    UInt srcpitch = psrc->GetPitch();
    bool startx = (rect.x > 0);
    bool starty = (rect.y > 0);
    if (startx)
        rect.x--;
    if (starty)
        rect.y--;
    bool endx = (rect.x + psrc->Width + (rect.x ? 0 : -1) >= pdest->Width-1) ? 0 : 1;
    bool endy = (rect.y + psrc->Height + (rect.y ? 0 : -1) >= pdest->Height-1) ? 0 : 1;
    UByte* pdestrow = pdest->GetScanline(rect.y) + 4*rect.x;

    GASSERT(rect.x + psrc->Width + endx <= pdest->Width);
    GASSERT(rect.y + psrc->Height + endy <= pdest->Height);

    switch(psrc->Format)
    {
    case GImage::Image_ARGB_8888:
        if (!startx)
            pdestrow -= 4;
        if (starty)
        {
            if (startx) *((UInt32*) pdestrow) = *((const UInt32*) psrcrow);
            memcpy(pdestrow+4, psrcrow, srcpitch);
            if (endx) *((UInt32*) (pdestrow+4+psrc->Width*4)) = *((const UInt32*) (psrcrow+psrc->Width*4-4));
            pdestrow += destpitch;
        }
        for (UInt j = 0; j < psrc->Height; j++, pdestrow += destpitch, psrcrow += srcpitch)
        {
            if (startx) *((UInt32*) pdestrow) = *((const UInt32*) psrcrow);
            memcpy(pdestrow+4, psrcrow, psrc->Width*4);
            if (endx) *((UInt32*) (pdestrow+4+psrc->Width*4)) = *((const UInt32*) (psrcrow+psrc->Width*4-4));
        }
        if (endy)
        {
            psrcrow -= srcpitch;
            if (startx) *((UInt32*) pdestrow) = *((const UInt32*) psrcrow);
            memcpy(pdestrow+4, psrcrow, srcpitch);
            if (endx) *((UInt32*) (pdestrow+4+psrc->Width*4)) = *((const UInt32*) (psrcrow+psrc->Width*4-4));
            pdestrow += destpitch;
        }
        break;

    case GImage::Image_RGB_888:
        destpitch -= psrc->Width*4 + (startx ? 4 : 0);
        srcpitch -= psrc->Width*3;
        if (starty)
        {
            if (startx)
            {
                pdestrow[0] = psrcrow[0]; pdestrow[1] = psrcrow[1]; pdestrow[2] = psrcrow[2];  pdestrow[3] = 255;
                pdestrow += 4;
            }
            for (UInt i = 0; i < psrc->Width; i++, psrcrow+=3, pdestrow+=4)
            {
                pdestrow[0] = psrcrow[0]; pdestrow[1] = psrcrow[1]; pdestrow[2] = psrcrow[2];  pdestrow[3] = 255;
            }
            if (endx)
            {
                pdestrow[0] = psrcrow[-3]; pdestrow[1] = psrcrow[-2]; pdestrow[2] = psrcrow[-1];  pdestrow[3] = 255;
            }
            pdestrow += destpitch;
            psrcrow -= psrc->Width*3;
        }
        for (UInt j = 0; j < psrc->Height; j++, pdestrow += destpitch, psrcrow += srcpitch)
        {
            if (startx)
            {
                pdestrow[0] = psrcrow[0]; pdestrow[1] = psrcrow[1]; pdestrow[2] = psrcrow[2];  pdestrow[3] = 255;
                pdestrow += 4;
            }
            for (UInt i = 0; i < psrc->Width; i++, psrcrow+=3, pdestrow+=4)
            {
                pdestrow[0] = psrcrow[0]; pdestrow[1] = psrcrow[1]; pdestrow[2] = psrcrow[2];  pdestrow[3] = 255;
            }
            if (endx)
            {
                pdestrow[0] = psrcrow[-3]; pdestrow[1] = psrcrow[-2]; pdestrow[2] = psrcrow[-1];  pdestrow[3] = 255;
            }
        }
        if (endy)
        {
            psrcrow -= psrc->GetPitch();
            if (startx)
            {
                pdestrow[0] = psrcrow[0]; pdestrow[1] = psrcrow[1]; pdestrow[2] = psrcrow[2];  pdestrow[3] = 255;
                pdestrow += 4;
            }
            for (UInt i = 0; i < psrc->Width; i++, psrcrow+=3, pdestrow+=4)
            {
                pdestrow[0] = psrcrow[0]; pdestrow[1] = psrcrow[1]; pdestrow[2] = psrcrow[2];  pdestrow[3] = 255;
            }
            if (endx)
            {
                pdestrow[0] = psrcrow[-3]; pdestrow[1] = psrcrow[-2]; pdestrow[2] = psrcrow[-1];  pdestrow[3] = 255;
            }
        }
        break;

    default:
        break;
    }
}

void GFxImagePackerImpl::Finish()
{
    GFxImagePackParams::TextureConfig PackTextureConfig;
    pImpl->GetTextureConfig(&PackTextureConfig);
    Packer.SetWidth (2+PackTextureConfig.TextureWidth);
    Packer.SetHeight(2+PackTextureConfig.TextureHeight);
    Packer.Clear();

    for (UInt i = 0; i < InputImages.GetSize(); i++)
        Packer.AddRect(2+InputImages[i].pImage->Width, 2+InputImages[i].pImage->Height, i);

    Packer.Pack();

    if (ImageCreateInfo.pRenderConfig)
        PackTextureConfig.SizeOptions = (ImageCreateInfo.pRenderConfig->GetRendererCapBits() & GRenderer::Cap_TexNonPower2) ? 
        GFxImagePackParams::PackSize_1 : GFxImagePackParams::PackSize_PowerOf2;

    for (UInt i = 0; i < Packer.GetNumPacks(); i++)
    {
        UInt imgWidth = 0, imgHeight = 0;
        const GRectPacker::PackType& pack = Packer.GetPack(i);
        for (UInt j = 0; j < pack.NumRects; j++)
        {
            const GRectPacker::RectType& rect = Packer.GetRect(pack, j);
            GImageBase* pimagein = InputImages[rect.Id].pImage;
            GRect<UInt> irect (rect.x, rect.y, GSize<UInt>(pimagein->Width, pimagein->Height));

            if (irect.Right >= imgWidth) imgWidth = irect.Right;
            if (irect.Bottom >= imgHeight) imgHeight = irect.Bottom;
        }
        if (PackTextureConfig.SizeOptions == GFxImagePackParams::PackSize_PowerOf2)
        {
            UInt w = 1; while (w < imgWidth) { w <<= 1; }
            UInt h = 1; while (h < imgHeight) { h <<= 1; }
            imgWidth = w;
            imgHeight = h;
        }
        else if (PackTextureConfig.SizeOptions == GFxImagePackParams::PackSize_4)
        {
            imgHeight = (imgHeight + 3) & ~3;
            imgWidth = (imgWidth + 3) & ~3;
        }

        GPtr<GImage> pPackImage = *GHEAP_NEW(ImageCreateInfo.pHeap) GImage (GImage::Image_ARGB_8888, imgWidth, imgHeight);

        for (UInt j = 0; j < pack.NumRects; j++)
        {
            const GRectPacker::RectType& rect = Packer.GetRect(pack, j);
            CopyImage(pPackImage, InputImages[rect.Id].pImage, rect);
        }

        GFxResourceId          textureId = pIdGen->GenerateNextId();

        ImageCreateInfo.SetImage(pPackImage);
        ImageCreateInfo.SetTextureUsage(0);
        GPtr<GImageInfoBase>   pimageInfo = *pImageCreator->CreateImage(ImageCreateInfo);
        GPtr<GFxImageResource> pimageRes  = 
            *GHEAP_NEW(ImageCreateInfo.pHeap) GFxImageResource(pimageInfo.GetPtr(), GFxResource::Use_Bitmap);

        for (UInt j = 0; j < pack.NumRects; j++)
        {
            const GRectPacker::RectType& rect = Packer.GetRect(pack, j);
            GImageBase* pimagein = InputImages[rect.Id].pImage;
            GRect<SInt> irect (rect.x, rect.y,
                               GSize<SInt>(pimagein->Width, pimagein->Height));
            GFxResourceDataNode *presNode = InputImages[rect.Id].pResNode;

            GPtr<GFxResource> pres = 
                *GHEAP_NEW(ImageCreateInfo.pHeap) GFxSubImageResource(pimageRes, textureId,                                                                      
                                                                      irect, ImageCreateInfo.pHeap);
            pBindData->SetResourceBindData(presNode, pres);
        }
    }
	for (UInt i = 0; i < Packer.GetNumFailed(); i++)
	{
		GImage* psrcImage = InputImages[Packer.GetFailed(i)].pImage;
		
		UInt imgWidth = psrcImage->Width, imgHeight = psrcImage->Height;
		if (PackTextureConfig.SizeOptions == GFxImagePackParams::PackSize_PowerOf2)
		{
			UInt w = 1; while (w < imgWidth) { w <<= 1; }
			UInt h = 1; while (h < imgHeight) { h <<= 1; }
			imgWidth = w;
			imgHeight = h;
		}
		else if (PackTextureConfig.SizeOptions == GFxImagePackParams::PackSize_4)
		{
			imgHeight = (imgHeight + 3) & ~3;
			imgWidth = (imgWidth + 3) & ~3;
		}
		GPtr<GImage> pPackImage = *GHEAP_NEW(ImageCreateInfo.pHeap) GImage (GImage::Image_ARGB_8888, imgWidth, imgHeight);
		GRectPacker::RectType rect;
		rect.Id = rect.x = rect.y = 0;
		CopyImage(pPackImage, psrcImage, rect);
		GFxResourceId textureId = pIdGen->GenerateNextId();
		ImageCreateInfo.SetImage(pPackImage);
		ImageCreateInfo.SetTextureUsage(0);
		GPtr<GImageInfoBase>   pimageInfo = *pImageCreator->CreateImage(ImageCreateInfo);
		GPtr<GFxImageResource> pimageRes  = 
			*GHEAP_NEW(ImageCreateInfo.pHeap) GFxImageResource(pimageInfo.GetPtr(), GFxResource::Use_Bitmap);
		GFxResourceDataNode *presNode = InputImages[Packer.GetFailed(i)].pResNode;
		GRect<SInt> irect(0,0, psrcImage->Width, psrcImage->Height);
		GPtr<GFxResource> pres = 
			*GHEAP_NEW(ImageCreateInfo.pHeap) GFxSubImageResource(pimageRes, textureId,                                                                      
			irect, ImageCreateInfo.pHeap);
		pBindData->SetResourceBindData(presNode, pres);
	}
}

