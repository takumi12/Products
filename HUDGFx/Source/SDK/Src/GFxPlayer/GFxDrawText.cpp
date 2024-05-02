/**********************************************************************

Filename    :   GFxDrawText.cpp
Content     :   External text interface
Created     :   May 23, 2008
Authors     :   Artem Bolgar

Notes       :   

Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#include "GFxDrawText.h"
#include "Text/GFxTextDocView.h"
#include "GAutoPtr.h"
#include "AMP/GFxAmpViewStats.h"
#include "GFxRenderGen.h"

#ifndef GFC_NO_DRAWTEXT_SUPPORT
class GFxDrawTextManagerImpl : public GNewOverrideBase<GFxStatMV_Text_Mem>
{
public:
    GPtr<GFxStateBagImpl>           pStateBag;
    GPtr<GFxMovieDef>               pMovieDef;
    GPtr<GFxTextAllocator>          pTextAllocator;
    GPtr<GFxFontManager>            pFontManager;
    GPtr<GFxFontManagerStates>      pFontStates;
    GPtr<GFxResourceWeakLib>        pWeakLib;
    GFxDisplayContext               DisplayContext;
    GPtr<GFxAmpViewStats>           DisplayContextStats;
    GFxDrawTextManager::TextParams  DefaultTextParams;
    GPtr<GFxLoaderImpl>             pLoaderImpl;
    enum
    {
        RTFlags_BeginDisplayInvoked = 1,
    };
    UInt8                           RTFlags; // run-time flags

    GFxDrawTextManagerImpl():RTFlags(0) 
    {
        DisplayContextStats = *GNEW GFxAmpViewStats();
        DisplayContext.pStats = DisplayContextStats;
    }
};

class GFxDrawTextImpl : public GFxDrawText
{
public:
    GPtr<GFxDrawTextManager>    pDrawTextCtxt;
    GPtr<GFxTextDocView>        pDocView;
    Matrix                      Matrix_1;
#ifndef GFC_NO_3D
    GMatrix3DNewable           *pMatrix3D_1;
#endif
    Cxform                      Cxform_1;
    GMemoryHeap*                pHeap;
    struct UserRendererData : public GNewOverrideBase<GFxStatMV_Text_Mem>
    {           
        GString                 StringVal;
        float                   FloatVal;
        float                   MatrixVal[16];
        GRenderer::UserData     UserData;
        UserRendererData() : FloatVal(0.0f) { }
    };
    GAutoPtr<UserRendererData>  pUserData;
    GColor                      BorderColor;
    GColor                      BackgroundColor;
public:
    GFxDrawTextImpl(GFxDrawTextManager* pdtMgr);
    ~GFxDrawTextImpl()
    {
#ifndef GFC_NO_3D
        if (pMatrix3D_1)
        {
            delete pMatrix3D_1;
            pMatrix3D_1 = NULL;
        }
#endif
    }
    void SetText(const char* putf8Str, UPInt lengthInBytes = UPInt(-1))
    {
        pDrawTextCtxt->CheckFontStatesChange();
        pDocView->SetText(putf8Str, lengthInBytes);
    }
    void SetText(const wchar_t* pstr,  UPInt lengthInChars = UPInt(-1))
    {
        pDrawTextCtxt->CheckFontStatesChange();
        pDocView->SetText(pstr, lengthInChars);
    }
    void SetText(const GString& str)
    {
        pDrawTextCtxt->CheckFontStatesChange();
        pDocView->SetText(str.ToCStr(), str.GetSize());
    }
    GString GetText() const
    {
        return pDocView->GetText();
    }

    void SetHtmlText(const char* putf8Str, UPInt lengthInBytes = UPInt(-1))
    {
        pDrawTextCtxt->CheckFontStatesChange();
        GFxStyledText::HTMLImageTagInfoArray imageInfoArray(pDrawTextCtxt->pHeap);		
        pDocView->ParseHtml(putf8Str, lengthInBytes, false, &imageInfoArray);
        if (imageInfoArray.GetSize() > 0)
        {
			GFxDrawTextImpl::ProcessImageTags(pDocView, pDrawTextCtxt, imageInfoArray);
        }
    }
    void SetHtmlText(const wchar_t* pstr,  UPInt lengthInChars = UPInt(-1))
    {
        pDrawTextCtxt->CheckFontStatesChange();        
        GFxStyledText::HTMLImageTagInfoArray imageInfoArray(pDrawTextCtxt->pHeap);	
        pDocView->ParseHtml(pstr, lengthInChars, false, &imageInfoArray);
        if (imageInfoArray.GetSize() > 0)
        {
            GFxDrawTextImpl::ProcessImageTags(pDocView, pDrawTextCtxt, imageInfoArray);
        }
    }
    void SetHtmlText(const GString& str)
    {
        pDrawTextCtxt->CheckFontStatesChange();
		GFxStyledText::HTMLImageTagInfoArray imageInfoArray(pDrawTextCtxt->pHeap);		
		pDocView->ParseHtml(str.ToCStr(), str.GetLength(), false, &imageInfoArray);	
		if (imageInfoArray.GetSize() > 0)
		{
			GFxDrawTextImpl::ProcessImageTags(pDocView, pDrawTextCtxt, imageInfoArray);
		}
    }
    GString GetHtmlText() const
    {
        return pDocView->GetHtml();
    }

    void SetRect(const GRectF& viewRect)
    {
        pDocView->SetViewRect(PixelsToTwips(viewRect));
    }
    GRectF GetRect() const
    {
        return TwipsToPixels(pDocView->GetViewRect());
    }

    void SetMatrix(const Matrix& matrix)
    {
        Matrix_1 = matrix;
        Matrix_1.M_[0][2] = PixelsToTwips(Matrix_1.M_[0][2]);
        Matrix_1.M_[1][2] = PixelsToTwips(Matrix_1.M_[1][2]);
    }
    Matrix GetMatrix() const 
    { 
        Matrix m(Matrix_1);
        m.M_[0][2] = TwipsToPixels(Matrix_1.M_[0][2]);
        m.M_[1][2] = TwipsToPixels(Matrix_1.M_[1][2]);
        return m; 
    }

    bool Is3D() const
    { 
#ifndef GFC_NO_3D
        return (pMatrix3D_1 != NULL);
#else
        return false;
#endif
    }
    void CreateMatrix3D()
    {
#ifndef GFC_NO_3D
        if (!pMatrix3D_1)
        {
            pMatrix3D_1 = GHEAP_NEW(pHeap)  GMatrix3DNewable;
        }
#endif
    }

#ifndef GFC_NO_3D
    void SetMatrix3D(const GMatrix3D& matrix3D)
    {
        CreateMatrix3D();
        *pMatrix3D_1 = matrix3D;
        pMatrix3D_1->SetX(PixelsToTwips(pMatrix3D_1->GetX()));
        pMatrix3D_1->SetY(PixelsToTwips(pMatrix3D_1->GetY()));
    }

    GMatrix3D *GetMatrix3D() const 
    {
        return pMatrix3D_1;
    }
#endif

    void SetCxform(const Cxform& cx)
    {
        Cxform_1 = cx;
      }
    const Cxform& GetCxform() const
    {
        return Cxform_1;
    }

    void UpdateDefaultTextFormat(const GFxTextFormat& fmt)
    {
        const GFxTextFormat* ptextFmt = pDocView->GetDefaultTextFormat();
        pDocView->SetDefaultTextFormat(ptextFmt->Merge(fmt));
    }

    void UpdateDefaultParagraphFormat(const GFxTextParagraphFormat& fmt)
    {
        const GFxTextParagraphFormat* pparaFmt = pDocView->GetDefaultParagraphFormat();
        pDocView->SetDefaultParagraphFormat(pparaFmt->Merge(fmt));
    }

    void SetColor(GColor c, UPInt startPos = 0, UPInt endPos = UPInt(-1))
    {
        GFxTextFormat fmt(pHeap);
        fmt.SetColor(c);
        pDocView->SetTextFormat(fmt, startPos, endPos);
        UpdateDefaultTextFormat(fmt);
    }
    void SetFont (const char* pfontName, UPInt startPos = 0, UPInt endPos = UPInt(-1))
    {
        GFxTextFormat fmt(pHeap);
        fmt.SetFontName(pfontName);
        pDocView->SetTextFormat(fmt, startPos, endPos);
        UpdateDefaultTextFormat(fmt);
    }
    void SetFontSize(Float fontSize, UPInt startPos = 0, UPInt endPos = UPInt(-1))
    {
        GFxTextFormat fmt(pHeap);
        fmt.SetFontSize(fontSize);
        pDocView->SetTextFormat(fmt, startPos, endPos);
        UpdateDefaultTextFormat(fmt);
    }
    void SetFontStyle(FontStyle fontStyle, UPInt startPos = 0, UPInt endPos = UPInt(-1))
    {
        GFxTextFormat fmt(pHeap);
        switch(fontStyle)
        {
        case Normal: 
            fmt.SetBold(false);
            fmt.SetItalic(false);
            break;
        case Bold:
            fmt.SetBold(true);
            fmt.SetItalic(false);
            break;
        case Italic:
            fmt.SetBold(false);
            fmt.SetItalic(true);
            break;
        case BoldItalic:
            fmt.SetBold(true);
            fmt.SetItalic(true);
            break;
        }
        pDocView->SetTextFormat(fmt, startPos, endPos);
        UpdateDefaultTextFormat(fmt);
    }
    void SetUnderline(bool underline, UPInt startPos = 0, UPInt endPos = UPInt(-1))
    {
        GFxTextFormat fmt(pHeap);
        fmt.SetUnderline(underline);
        pDocView->SetTextFormat(fmt, startPos, endPos);
        UpdateDefaultTextFormat(fmt);
    }
    void SetMultiline(bool multiline)
    {
        if (multiline)
            pDocView->SetMultiline();
        else
            pDocView->ClearMultiline();
    }
    bool IsMultiline() const
    {
        return pDocView->IsMultiline();
    }

    static void ProcessImageTags(GFxTextDocView* ptextDocView, const GFxDrawTextManager* pmgr, const GFxStyledText::HTMLImageTagInfoArray& imageInfoArray);

    // Turns wordwrapping on/off
    void SetWordWrap(bool wordWrap)
    {
        if (wordWrap)
            pDocView->SetWordWrap();
        else
            pDocView->ClearWordWrap();
    }
    // Returns state of wordwrapping.
    bool IsWordWrap() const
    {
        return pDocView->IsWordWrap();
    }

    void SetAlignment(Alignment a)
    {
        GFxTextParagraphFormat::AlignType pa;
        switch(a)
        {
        case Align_Right:   pa = GFxTextParagraphFormat::Align_Right; break;
        case Align_Center:  pa = GFxTextParagraphFormat::Align_Center; break;
        case Align_Justify: pa = GFxTextParagraphFormat::Align_Justify; break;
        default:            pa = GFxTextParagraphFormat::Align_Left; break;
        }
        GFxTextParagraphFormat parafmt;
        parafmt.SetAlignment(pa);
        pDocView->SetParagraphFormat(parafmt);
        UpdateDefaultParagraphFormat(parafmt);
    }
    Alignment  GetAlignment() const
    {
        GFxTextParagraphFormat parafmt;
        pDocView->GetTextAndParagraphFormat(NULL, &parafmt, 0);
        if (parafmt.IsAlignmentSet())
        {
            switch(parafmt.GetAlignment())
            {
            case GFxTextParagraphFormat::Align_Right: return Align_Right;
            case GFxTextParagraphFormat::Align_Center: return Align_Center;
            case GFxTextParagraphFormat::Align_Justify: return Align_Justify;
            default:;
            }
        }
        return Align_Left;
    }

    void SetVAlignment(VAlignment a)
    {
        GFxTextDocView::ViewVAlignment pa;
        switch(a)
        {
        case VAlign_Bottom:  pa = GFxTextDocView::VAlign_Bottom; break;
        case VAlign_Center:  pa = GFxTextDocView::VAlign_Center; break;
        default:             pa = GFxTextDocView::VAlign_Top; break;
        }
        pDocView->SetVAlignment(pa);
    }
    VAlignment  GetVAlignment() const
    {
        VAlignment pa;
        switch(pDocView->GetVAlignment())
        {
        case GFxTextDocView::VAlign_Bottom: pa = VAlign_Bottom; break;
        case GFxTextDocView::VAlign_Center: pa = VAlign_Center; break;
        default: pa = VAlign_Top;
        }
        return pa;
    }

    void   SetBorderColor(const GColor& borderColor)
    {
        BorderColor = borderColor;
    }
    GColor GetBorderColor() const { return BorderColor; }
    void   SetBackgroundColor(const GColor& bkgColor)
    {
        BackgroundColor = bkgColor;
    }
    GColor GetBackgroundColor() const { return BackgroundColor; }

    void Display();

    void SetRendererString(const GString& str)
    {
        if (!pUserData)
            pUserData = GHEAP_NEW(pHeap) UserRendererData();
        pUserData->StringVal = str;
        pUserData->UserData.PropFlags |= GRenderer::UD_HasString;
        pUserData->UserData.pString = pUserData->StringVal.ToCStr();
    }
    GString GetRendererString() const
    {
        if (!pUserData)
            return GString();
        return pUserData->StringVal;
    }

    void SetRendererFloat(float num)
    {
        if (!pUserData)
            pUserData = GHEAP_NEW(pHeap) UserRendererData();
        pUserData->FloatVal = num;
        pUserData->UserData.PropFlags |= GRenderer::UD_HasFloat;
        pUserData->UserData.pFloat = &pUserData->FloatVal;
    }

    float GetRendererFloat() const
    {
        if (!pUserData)
            return 0;
        return pUserData->FloatVal;
    }

    void SetRendererMatrix(float *m, UInt count = 16)
    {
        if (!pUserData)
            pUserData = GHEAP_NEW(pHeap) UserRendererData();
        memcpy(pUserData->MatrixVal, m, count*sizeof(float));
        pUserData->UserData.PropFlags |= GRenderer::UD_HasMatrix;
        pUserData->UserData.pMatrix = pUserData->MatrixVal;
        pUserData->UserData.MatrixSize = count;
    }
    
    void        SetAAMode(AAMode aa)
    {
        if (aa == AA_Readability)
            pDocView->SetAAForReadability();
        else
            pDocView->ClearAAForReadability();
    }

    AAMode      GetAAMode() const 
    { 
        return (pDocView->IsAAForReadability()) ? AA_Readability : AA_Animation; 
    }

    void SetFilters(const Filter* filters, UPInt filtersCnt);
    void ClearFilters();

    GFxTextDocView* GetDocView() const { return pDocView; }
};

GFxDrawTextImpl::GFxDrawTextImpl(GFxDrawTextManager* pdtMgr) : 
    pDrawTextCtxt(pdtMgr)
#ifndef GFC_NO_3D
  , pMatrix3D_1(NULL)
#endif
{
    pHeap = GMemory::GetHeapByAddress(this);

    pDocView = *GHEAP_NEW(pHeap) GFxTextDocView(pDrawTextCtxt->GetTextAllocator(), 
        pDrawTextCtxt->GetFontManager(), pDrawTextCtxt->GetLog());
    // initialize pDocView
    GFxTextFormat tfmt(pHeap);
    tfmt.InitByDefaultValues();
    pDocView->SetDefaultTextFormat(tfmt);

    GFxTextParagraphFormat pfmt;
    pfmt.InitByDefaultValues();
    pDocView->SetDefaultParagraphFormat(pfmt);

    BorderColor.SetAlpha(0);
    BackgroundColor.SetAlpha(0);

    SetAAMode(AA_Animation);
}

void GFxDrawTextImpl::ProcessImageTags(GFxTextDocView* ptextDocView, const GFxDrawTextManager* pmgr, const GFxStyledText::HTMLImageTagInfoArray& imageInfoArray)
{
	UPInt numTags = imageInfoArray.GetSize();
    
    GFxMovieDefImpl*        pmdImpl = NULL;
    GPtr<GFxImageCreator>   pimageCreator;
    GPtr<GFxImageLoader>    pimageLoader;

    // If the DrawTextManager was created with a MovieDef, it can load 
    // images from the MovieDef or disk. If it was created with a Loader, 
    // it will load only be able to load images from disk.
    bool bMovieExists = false;
    if (pmgr->pImpl->pMovieDef)
    {
        pmdImpl = (GFxMovieDefImpl*)pmgr->pImpl->pMovieDef.GetPtr();
        bMovieExists = true;
    }
    
    if (pmgr->GetImageCreator())
        pimageCreator = pmgr->GetImageCreator();
    if (pmgr->GetImageLoader())
        pimageLoader = pmgr->GetImageLoader();

	for (UPInt i = 0; i < numTags; ++i)
	{
        const GFxStyledText::HTMLImageTagInfo& imgTagInfo = imageInfoArray[i];		
        GASSERT(imgTagInfo.pTextImageDesc);

        GPtr<GFxImageResource> pimageRes;
        bool bImageFound = false;
       
        bool userImageProtocol = false;
        if (imgTagInfo.Url.GetLength() > 0 && (imgTagInfo.Url[0] == 'i' || imgTagInfo.Url[0] == 'I'))
        {
            GString urlLowerCase = imgTagInfo.Url.ToLower();
            if (urlLowerCase.Substring(0, 6) == "img://")
                userImageProtocol = true;
            else if (urlLowerCase.Substring(0, 8) == "imgps://")
                userImageProtocol = true;
        }
       
        if (userImageProtocol) // If the user image protocol was used, disregard the other cases.
        {            
            if (pimageLoader)
            {
                GPtr<GImageInfoBase> pimageInfo = *pimageLoader->LoadImage(imgTagInfo.Url);
                pimageRes  = *GHEAP_NEW(pmgr->pHeap) GFxImageResource(pimageInfo.GetPtr());
                bImageFound = true;
            }
            else
            {
                pmgr->GetLog()->LogScriptWarning(
                    "GFxDrawText::ProcessImageTags - No GFxImageLoader found for loading '%s'\n", 
                    imgTagInfo.Url.ToCStr());
            }  
        }        
        else if (bMovieExists) // Attempt to load the SWF from the MovieDef
        {
            GFxResourceBindData             resBindData;
            GPtr<GFxResource>               pexportedResource;

            if (pmdImpl->GetExportedResource(&resBindData, imgTagInfo.Url))
            {
                GASSERT(resBindData.pResource.GetPtr() != 0);
                if (resBindData.pResource->GetResourceType() == GFxResource::RT_Image)
                {
                    pimageRes = (GFxImageResource*)resBindData.pResource.GetPtr();
                    bImageFound = true;
                }
            }   
        }

		if (bImageFound)
		{			
			GPtr<GFxShapeWithStyles> pimgShape = 
                    *GHEAP_NEW(pmgr->pHeap) GFxShapeWithStyles();
			GASSERT(pimgShape);
			pimgShape->SetToImage(pimageRes, true);
			imgTagInfo.pTextImageDesc->pImageShape = pimgShape;

			Float origWidth = 0, origHeight = 0;
			Float screenWidth = 0, screenHeight = 0;
			GRect<SInt> dimr = pimageRes->GetImageInfo()->GetRect();
			screenWidth  = origWidth  = (Float)PixelsToTwips(dimr.Width());
			screenHeight = origHeight = (Float)PixelsToTwips(dimr.Height());
			if (imgTagInfo.Width)
				screenWidth = Float(imgTagInfo.Width);
			if (imgTagInfo.Height)
				screenHeight = Float(imgTagInfo.Height);

			Float baseLineY = origHeight - PixelsToTwips(1.0f);
			baseLineY += imgTagInfo.VSpace;
			imgTagInfo.pTextImageDesc->ScreenWidth  = (UInt)screenWidth;
			imgTagInfo.pTextImageDesc->ScreenHeight = (UInt)screenHeight;
			imgTagInfo.pTextImageDesc->Matrix.AppendTranslation(0, -baseLineY);
			imgTagInfo.pTextImageDesc->Matrix.AppendScaling(screenWidth/origWidth, screenHeight/origHeight);

			ptextDocView->SetCompleteReformatReq();
		}
        else 
        {
            pmgr->GetLog()->LogScriptWarning("GFxDrawText::ProcessImageTags - Unable to load '%s'\n", imgTagInfo.Url.ToCStr());
        }
	}
}

void GFxDrawTextImpl::Display()
{
    if (pDrawTextCtxt)
    {
        if (!pDrawTextCtxt->IsBeginDisplayInvokedFlagSet())
        {
            GPtr<GFxLog> plog = pDrawTextCtxt->pImpl->pStateBag->GetLog();
            if (plog)
                plog->LogWarning("GFxDrawText::Display is called w/o calling GFxDrawTextManager::BeginDisplay().\n");
            return;
        }

        GRenderer* prenderer = pDrawTextCtxt->pImpl->DisplayContext.GetRenderer();

        // check if mesh cache has registered its event handler in renderer
        GPtr<GFxMeshCacheManager> meshCache = pDrawTextCtxt->GetMeshCacheManager();
        if (meshCache)
        {
            GRendererEventHandler* evt = meshCache->GetEventHandler();
            if (evt->GetRenderer() != prenderer)
            {
                if (evt->GetRenderer())
                    prenderer->RemoveEventHandler(evt);
                prenderer->AddEventHandler(evt);
            }
        }

#ifndef GFC_NO_3D
        GFxDisplayContext &context = pDrawTextCtxt->pImpl->DisplayContext;
        GMatrix3D *poldmatrix3D = context.pParentMatrix3D;
#endif
        // Draw background and/or border.
        if (BorderColor.GetAlpha() > 0 || BackgroundColor.GetAlpha() > 0)
        {        
            static const UInt16   indices[6] = { 0, 1, 2, 2, 1, 3 };

            prenderer->SetCxform(Cxform_1);
            Matrix m(Matrix_1);

            GRectF newRect;
            GFx_RecalculateRectToFit16Bit(m, pDocView->GetViewRect(), &newRect);
            prenderer->SetMatrix(m);

#ifndef GFC_NO_3D
            GMatrix3D m3d;
            if (Is3D())
            {
                m3d = *pMatrix3D_1;
                prenderer->SetWorld3D(&m3d);
            }
#endif

            GPointF         coords[4];
            coords[0] = newRect.TopLeft();
            coords[1] = newRect.TopRight();
            coords[2] = newRect.BottomLeft();
            coords[3] = newRect.BottomRight();

            GRenderer::VertexXY16i icoords[4];
            icoords[0].x = (SInt16) coords[0].x;
            icoords[0].y = (SInt16) coords[0].y;
            icoords[1].x = (SInt16) coords[1].x;
            icoords[1].y = (SInt16) coords[1].y;
            icoords[2].x = (SInt16) coords[2].x;
            icoords[2].y = (SInt16) coords[2].y;
            icoords[3].x = (SInt16) coords[3].x;
            icoords[3].y = (SInt16) coords[3].y;

            const SInt16  linecoords[10] = 
            {
                // outline
                (SInt16) coords[0].x, (SInt16) coords[0].y,
                (SInt16) coords[1].x, (SInt16) coords[1].y,
                (SInt16) coords[3].x, (SInt16) coords[3].y,
                (SInt16) coords[2].x, (SInt16) coords[2].y,
                (SInt16) coords[0].x, (SInt16) coords[0].y,
            };

            prenderer->FillStyleColor(BackgroundColor);
            prenderer->SetVertexData(icoords, 4, GRenderer::Vertex_XY16i);

            // Fill the inside
            prenderer->SetIndexData(indices, 6, GRenderer::Index_16);
            prenderer->DrawIndexedTriList(0, 0, 6, 0, 2);
            prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
            // And draw outline
            prenderer->SetVertexData(linecoords, 5, GRenderer::Vertex_XY16i);
            prenderer->LineStyleColor(BorderColor);
            prenderer->DrawLineStrip(0, 4);
            // Done
            prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
            prenderer->SetIndexData(0, 0, GRenderer::Index_None);
        }

        bool bPushed = (pUserData) ? pDrawTextCtxt->pImpl->DisplayContext.GetRenderer()->PushUserData(&pUserData->UserData) : false;
        pDocView->Display(pDrawTextCtxt->pImpl->DisplayContext, Matrix_1, Cxform_1, false);
        if (bPushed)
            pDrawTextCtxt->pImpl->DisplayContext.GetRenderer()->PopUserData();

#ifndef GFC_NO_3D
        context.GetRenderer()->SetWorld3D(poldmatrix3D == &GMatrix3D::Identity ? NULL : poldmatrix3D);
#endif
    }
}

void GFxDrawText::Filter::InitByDefaultValues() 
{
    // fill out by default values
    GFxTextFilter tf;
    Blur.BlurX = tf.Fixed44toFloat(tf.BlurX);
    Blur.BlurY = tf.Fixed44toFloat(tf.BlurY);
    Blur.Strength = tf.Fixed44toFloat(tf.BlurStrength) * 100.f;
    DropShadow.Flags = (UInt8)(tf.ShadowFlags | GFxDrawText::FilterFlag_FineBlur);
    DropShadow.Color = ((tf.ShadowColor.ToColor32() & 0xFFFFFF) | ((UInt32)tf.ShadowAlpha << 24));
    DropShadow.Angle = (Float)tf.ShadowAngle / 10.f;
    DropShadow.Distance = TwipsToPixels((Float)tf.ShadowDistance);
}


void GFxDrawTextImpl::SetFilters(const Filter* filters, UPInt filtersCnt)
{
    GFxTextFilter tf;
    for (UPInt i = 0; i < filtersCnt; ++i)
    {
        switch(filters[i].Type)
        {
        case Filter_Blur:
            tf.BlurX        = tf.FloatToFixed44(filters[i].Blur.BlurX);
            tf.BlurY        = tf.FloatToFixed44(filters[i].Blur.BlurY);
            tf.BlurStrength = tf.FloatToFixed44(filters[i].Blur.Strength/100.f);
            break;
        case Filter_DropShadow:
            tf.ShadowFlags    = filters[i].DropShadow.Flags;
            tf.ShadowBlurX    = tf.FloatToFixed44(filters[i].DropShadow.BlurX);
            tf.ShadowBlurY    = tf.FloatToFixed44(filters[i].DropShadow.BlurY);
            tf.ShadowStrength = tf.FloatToFixed44(filters[i].DropShadow.Strength/100.f);
            tf.ShadowAlpha    = GColor(filters[i].DropShadow.Color).GetAlpha();
            tf.ShadowAngle    = (SInt16)(filters[i].DropShadow.Angle * 10);
            tf.ShadowDistance = (SInt16)PixelsToTwips(filters[i].DropShadow.Distance);
            tf.ShadowOffsetX  = 0;
            tf.ShadowOffsetY  = 0;
            tf.ShadowColor    = filters[i].DropShadow.Color;
            tf.UpdateShadowOffset();
            break;
        case Filter_Glow:
            tf.ShadowFlags    = filters[i].Glow.Flags;
            tf.ShadowBlurX    = tf.FloatToFixed44(filters[i].Glow.BlurX);
            tf.ShadowBlurY    = tf.FloatToFixed44(filters[i].Glow.BlurY);
            tf.ShadowStrength = tf.FloatToFixed44(filters[i].Glow.Strength/100.f);
            tf.ShadowAlpha    = GColor(filters[i].Glow.Color).GetAlpha();
            tf.ShadowAngle    = 0;
            tf.ShadowDistance = 0;
            tf.ShadowOffsetX  = 0;
            tf.ShadowOffsetY  = 0;
            tf.ShadowColor    = filters[i].Glow.Color;
            tf.UpdateShadowOffset();
            break;
		default:
			break;
        }
    }
    pDocView->SetFilters(tf);
}

void GFxDrawTextImpl::ClearFilters()
{
    GFxDrawText::Filter filter;
    SetFilters(&filter, 1);
}

//////////////////////////////////////////////////////////////////////////
// GFxDrawTextManager
GFxDrawTextManager::GFxDrawTextManager(GFxMovieDef* pmovieDef, GMemoryHeap* pheap)
{
	if (pheap == NULL)
	{
		pHeap = GMemory::GetGlobalHeap()->CreateHeap("DrawText Manager");
		HeapOwned = true;
	}
	else
	{
		pHeap = pheap;
		HeapOwned = false;
	}

    pImpl = GHEAP_NEW(pHeap) GFxDrawTextManagerImpl();
    pImpl->pMovieDef = pmovieDef;
    if (pmovieDef)
    {
        pmovieDef->WaitForLoadFinish();
        GFxMovieDefImpl* pdefImpl = static_cast<GFxMovieDefImpl*>(pmovieDef);
        pImpl->pStateBag   = *new GFxStateBagImpl(pdefImpl->pStateBag);
    }
    else
    {
        pImpl->pStateBag   = *new GFxStateBagImpl(NULL);
        pImpl->pStateBag->SetLog(GPtr<GFxLog>(*new GFxLog));
    }
    if (pImpl->pStateBag)
    {
        // By default there should be no glyph packer
        pImpl->pStateBag->SetFontPackParams(0);

        pImpl->pTextAllocator  = *GHEAP_NEW(pHeap) GFxTextAllocator(pHeap);
        pImpl->pFontStates     = *new GFxFontManagerStates(pImpl->pStateBag);

        if (pImpl->pMovieDef)
        {
            //pStateBag->SetFontCacheManager(pMovieDef->GetFontCacheManager());
            //pStateBag->SetFontLib(pMovieDef->GetFontLib());
            //pStateBag->SetFontMap(pMovieDef->GetFontMap());

            GFxMovieDefImpl* pdefImpl = static_cast<GFxMovieDefImpl*>(pmovieDef);
            pImpl->pFontManager    = *GHEAP_NEW(pHeap) GFxFontManager(pdefImpl, pImpl->pFontStates);
            pImpl->pWeakLib        = pdefImpl->GetWeakLib();

            if (pdefImpl->GetRenderConfig())
                pImpl->pStateBag->SetRenderConfig(pdefImpl->GetRenderConfig());

            if (pdefImpl->GetImageCreator())
                pImpl->pStateBag->SetImageCreator(pdefImpl->GetImageCreator());
            if (pdefImpl->GetImageLoader())
                pImpl->pStateBag->SetImageLoader(pdefImpl->GetImageLoader());

            if (pdefImpl->GetJpegSupport())
                pImpl->pStateBag->SetJpegSupport(pdefImpl->GetJpegSupport());
        }
        else
        {
            // It's mandatory to have the cache manager for text rendering to work,
            // even if the dynamic cache isn't used. 
            pImpl->pStateBag->SetFontCacheManager(GPtr<GFxFontCacheManager>(*new GFxFontCacheManager(true)));
            pImpl->pWeakLib        = *new GFxResourceWeakLib(NULL);
            pImpl->pFontManager    = *GHEAP_NEW(pHeap) GFxFontManager(pImpl->pWeakLib, pImpl->pFontStates);
        }
        if (!pImpl->pStateBag->GetMeshCacheManager())
            pImpl->pStateBag->SetMeshCacheManager(GPtr<GFxMeshCacheManager>(*GNEW GFxMeshCacheManager(false)));
    }
}

GFxDrawTextManager::GFxDrawTextManager(GFxLoader* ploader, GMemoryHeap* pheap)
{
	if (pheap == NULL)
	{
		pHeap = GMemory::GetGlobalHeap()->CreateHeap("DrawText Manager");
		HeapOwned = true;
	}
	else
	{
		pHeap = pheap;
		HeapOwned = false;
	}

    pImpl = GHEAP_NEW(pHeap) GFxDrawTextManagerImpl();
    pImpl->pMovieDef = 0;
    pImpl->pStateBag    = *new GFxStateBagImpl(NULL);
    if (ploader->GetLog())
        pImpl->pStateBag->SetLog(ploader->GetLog());
    else
        pImpl->pStateBag->SetLog(GPtr<GFxLog>(*new GFxLog));

    // By default there should be no glyph packer
    pImpl->pStateBag->SetFontPackParams(0);

    pImpl->pTextAllocator  = *GHEAP_NEW(pHeap) GFxTextAllocator(pHeap);
    pImpl->pFontStates     = *new GFxFontManagerStates(pImpl->pStateBag);

    // It's mandatory to have the cache manager for text rendering to work,
    // even if the dynamic cache isn't used.
    if (ploader->GetFontCacheManager())
        pImpl->pStateBag->SetFontCacheManager(ploader->GetFontCacheManager());
    else
        pImpl->pStateBag->SetFontCacheManager(GPtr<GFxFontCacheManager>(*new GFxFontCacheManager(true)));

    if (ploader->GetRenderConfig())
        pImpl->pStateBag->SetRenderConfig(ploader->GetRenderConfig());

    if (ploader->GetFontLib())
        pImpl->pStateBag->SetFontLib(ploader->GetFontLib());
    if (ploader->GetFontMap())
        pImpl->pStateBag->SetFontMap(ploader->GetFontMap());
    if (ploader->GetFontProvider())
        pImpl->pStateBag->SetFontProvider(ploader->GetFontProvider());

	if (ploader->GetImageCreator())
		pImpl->pStateBag->SetImageCreator(ploader->GetImageCreator());
	if (ploader->GetImageLoader())
		pImpl->pStateBag->SetImageLoader(ploader->GetImageLoader());

	if (ploader->GetJpegSupport())
		pImpl->pStateBag->SetJpegSupport(ploader->GetJpegSupport());

    GPtr<GFxResourceLib> pstrongLib = ploader->GetResourceLib();
    if (pstrongLib)
        pImpl->pWeakLib    = pstrongLib->GetWeakLib();
    else
        pImpl->pWeakLib    = *new GFxResourceWeakLib(NULL);
    pImpl->pFontManager    = *GHEAP_NEW(pHeap) GFxFontManager(pImpl->pWeakLib, pImpl->pFontStates);
    pImpl->pStateBag->SetMeshCacheManager(ploader->GetMeshCacheManager());
}

GFxDrawTextManager::~GFxDrawTextManager()
{
    delete pImpl;
	if (HeapOwned)
		pHeap->Release();
}

void GFxDrawTextManager::SetBeginDisplayInvokedFlag(bool v) 
{ 
    (v) ? pImpl->RTFlags |= GFxDrawTextManagerImpl::RTFlags_BeginDisplayInvoked : 
          pImpl->RTFlags &= (~GFxDrawTextManagerImpl::RTFlags_BeginDisplayInvoked); 
}
void GFxDrawTextManager::ClearBeginDisplayInvokedFlag()            
{ 
    SetBeginDisplayInvokedFlag(false); 
}

bool GFxDrawTextManager::IsBeginDisplayInvokedFlagSet() const      
{ 
    return (pImpl->RTFlags & GFxDrawTextManagerImpl::RTFlags_BeginDisplayInvoked) != 0; 
}

GFxDrawTextManager::TextParams::TextParams()
{
    TextColor = GColor(0, 0, 0, 255);
    HAlignment = GFxDrawText::Align_Left;
    VAlignment = GFxDrawText::VAlign_Top;
    FontStyle  = GFxDrawText::Normal;
    FontSize   = 12;
    FontName   = "Times New Roman";
    Underline  = false;
    Multiline  = true;
    WordWrap   = true;
	Indent     = 0;
	Leading    = 0;
	RightMargin = 0;
	LeftMargin  = 0;
}

void GFxDrawTextManager::SetDefaultTextParams(const GFxDrawTextManager::TextParams& params) 
{ 
    pImpl->DefaultTextParams = params; 
}

// Returns currently set default text parameters.
const GFxDrawTextManager::TextParams& GFxDrawTextManager::GetDefaultTextParams() const 
{ 
    return pImpl->DefaultTextParams; 
}

void GFxDrawTextManager::SetTextParams(GFxTextDocView* pdoc, const TextParams& txtParams,
                                       const GFxTextFormat* tfmt, const GFxTextParagraphFormat* pfmt)
{
    GFxTextFormat textFmt(pHeap);
    GFxTextParagraphFormat paraFmt;
    if (tfmt)
        textFmt = *tfmt;
    if (pfmt)
        paraFmt = *pfmt;
    textFmt.SetColor(txtParams.TextColor);
    switch(txtParams.FontStyle)
    {
    case GFxDrawText::Normal: 
        textFmt.SetBold(false);
        textFmt.SetItalic(false);
        break;
    case GFxDrawText::Bold:
        textFmt.SetBold(true);
        textFmt.SetItalic(false);
        break;
    case GFxDrawText::Italic:
        textFmt.SetBold(false);
        textFmt.SetItalic(true);
        break;
    case GFxDrawText::BoldItalic:
        textFmt.SetBold(true);
        textFmt.SetItalic(true);
        break;
    }
    textFmt.SetFontName(txtParams.FontName);
    textFmt.SetFontSize(txtParams.FontSize);
    textFmt.SetUnderline(txtParams.Underline);

    GFxTextParagraphFormat::AlignType pa;
    switch(txtParams.HAlignment)
    {
    case GFxDrawText::Align_Right:   pa = GFxTextParagraphFormat::Align_Right; break;
    case GFxDrawText::Align_Center:  pa = GFxTextParagraphFormat::Align_Center; break;
    case GFxDrawText::Align_Justify: pa = GFxTextParagraphFormat::Align_Justify; break;
    default:            pa = GFxTextParagraphFormat::Align_Left; break;
    }
    paraFmt.SetAlignment(pa);
	
	paraFmt.SetIndent(txtParams.Indent);
	paraFmt.SetLeading(txtParams.Leading);
	paraFmt.SetLeftMargin(txtParams.LeftMargin);
	paraFmt.SetRightMargin(txtParams.RightMargin);

    GFxTextDocView::ViewVAlignment va;
    switch(txtParams.VAlignment)
    {
    case GFxDrawText::VAlign_Bottom:  va = GFxTextDocView::VAlign_Bottom; break;
    case GFxDrawText::VAlign_Center:  va = GFxTextDocView::VAlign_Center; break;
    default:                          va = GFxTextDocView::VAlign_Top; break;
    }
    pdoc->SetVAlignment(va);

    if (txtParams.Multiline)
    {
        pdoc->SetMultiline();
        if (txtParams.WordWrap)
            pdoc->SetWordWrap();
    }
    pdoc->SetTextFormat(textFmt);
    pdoc->SetParagraphFormat(paraFmt);
    pdoc->SetDefaultTextFormat(textFmt);
    pdoc->SetDefaultParagraphFormat(paraFmt);
}

GFxDrawText* GFxDrawTextManager::CreateText()
{
    return GHEAP_NEW(pHeap) GFxDrawTextImpl(this);
}

GFxDrawText* GFxDrawTextManager::CreateText(const char* putf8Str, const GRectF& viewRect, const TextParams* ptxtParams)
{
    GFxDrawTextImpl* ptext = GHEAP_NEW(pHeap) GFxDrawTextImpl(this);
    ptext->SetRect(viewRect);
    ptext->SetText(putf8Str);
    SetTextParams(ptext->GetDocView(), (ptxtParams) ? *ptxtParams : pImpl->DefaultTextParams);
    return ptext;
}

GFxDrawText* GFxDrawTextManager::CreateText(const wchar_t* pwstr, const GRectF& viewRect, const TextParams* ptxtParams)
{
    GFxDrawTextImpl* ptext = GHEAP_NEW(pHeap) GFxDrawTextImpl(this);
    ptext->SetRect(viewRect);
    ptext->SetText(pwstr);
    SetTextParams(ptext->GetDocView(), (ptxtParams) ? *ptxtParams : pImpl->DefaultTextParams);
    return ptext;
}

GFxDrawText* GFxDrawTextManager::CreateText(const GString& str, const GRectF& viewRect, const TextParams* ptxtParams)
{
    GFxDrawTextImpl* ptext = GHEAP_NEW(pHeap) GFxDrawTextImpl(this);
    ptext->SetRect(viewRect);
    ptext->SetText(str);
    SetTextParams(ptext->GetDocView(), (ptxtParams) ? *ptxtParams : pImpl->DefaultTextParams);
    //ptext->GetDocView()->Dump();
    return ptext;
}

GFxDrawText* GFxDrawTextManager::CreateHtmlText(const char* putf8Str, const GRectF& viewRect, const TextParams* ptxtParams)
{
    GFxDrawTextImpl* ptext = GHEAP_NEW(pHeap) GFxDrawTextImpl(this);
    ptext->SetRect(viewRect);
    SetTextParams(ptext->GetDocView(), (ptxtParams) ? *ptxtParams : pImpl->DefaultTextParams);
    ptext->SetHtmlText(putf8Str);
    return ptext;
}

GFxDrawText* GFxDrawTextManager::CreateHtmlText(const wchar_t* pwstr, const GRectF& viewRect, const TextParams* ptxtParams)
{
    GFxDrawTextImpl* ptext = GHEAP_NEW(pHeap) GFxDrawTextImpl(this);
    ptext->SetRect(viewRect);
    SetTextParams(ptext->GetDocView(), (ptxtParams) ? *ptxtParams : pImpl->DefaultTextParams);
    ptext->SetHtmlText(pwstr);
    return ptext;
}

GFxDrawText* GFxDrawTextManager::CreateHtmlText(const GString& str, const GRectF& viewRect, const TextParams* ptxtParams)
{
    GFxDrawTextImpl* ptext = GHEAP_NEW(pHeap) GFxDrawTextImpl(this);
    ptext->SetRect(viewRect);
    SetTextParams(ptext->GetDocView(), (ptxtParams) ? *ptxtParams : pImpl->DefaultTextParams);
    ptext->SetHtmlText(str);
    return ptext;
}

GFxTextDocView* GFxDrawTextManager::CreateTempDoc(const TextParams& txtParams, 
                                                  GFxTextFormat* tfmt, GFxTextParagraphFormat *pfmt,
                                                  Float width, Float height)
{
    GFxTextDocView* ptempText = GHEAP_NEW(pHeap) GFxTextDocView(pImpl->pTextAllocator, pImpl->pFontManager, GetLog());
    tfmt->InitByDefaultValues();
    pfmt->InitByDefaultValues();

    GSizeF sz(width, height);
    ptempText->SetViewRect(GRectF(0, 0, sz));
    if (txtParams.Multiline)
        ptempText->SetMultiline();
    else      
        ptempText->ClearMultiline();

    if (txtParams.WordWrap && width > 0)
    {
        ptempText->SetWordWrap();
        if (txtParams.Multiline)
            ptempText->SetAutoSizeY();
    }
    else
    {
        ptempText->SetAutoSizeX();
        ptempText->ClearWordWrap();
    }

    return ptempText;
}

GSizeF GFxDrawTextManager::GetTextExtent(const char* putf8Str, Float width, const TextParams* ptxtParams)
{
    CheckFontStatesChange();
    GFxTextFormat tfmt(pHeap);
    GFxTextParagraphFormat pfmt;
    TextParams txtParams = (ptxtParams) ? *ptxtParams : pImpl->DefaultTextParams;
    GPtr<GFxTextDocView> ptempText = *CreateTempDoc(txtParams, &tfmt, &pfmt, PixelsToTwips(width), 0);
    // need to reset Multiline and WordWrap, since they are already set up
    // correctly inside the CreateTempDoc.
    txtParams.Multiline = txtParams.WordWrap = false; 
    SetTextParams(ptempText, txtParams, &tfmt, &pfmt);
    ptempText->SetText(putf8Str);

    GSizeF sz(TwipsToPixels(ptempText->GetTextWidth()), TwipsToPixels(ptempText->GetTextHeight()));
    sz.Expand(TwipsToPixels(GFX_TEXT_GUTTER*2));
    return sz;
}
GSizeF GFxDrawTextManager::GetTextExtent(const wchar_t* pwstr, Float width, const TextParams* ptxtParams)
{
    CheckFontStatesChange();
    GFxTextFormat tfmt(pHeap);
    GFxTextParagraphFormat pfmt;
    TextParams txtParams = (ptxtParams) ? *ptxtParams : pImpl->DefaultTextParams;
    GPtr<GFxTextDocView> ptempText = *CreateTempDoc(txtParams, &tfmt, &pfmt, PixelsToTwips(width), 0);
    // need to reset Multiline and WordWrap, since they are already set up
    // correctly inside the CreateTempDoc.
    txtParams.Multiline = txtParams.WordWrap = false; 
    SetTextParams(ptempText, txtParams, &tfmt, &pfmt);
    ptempText->SetText(pwstr);

    GSizeF sz(TwipsToPixels(ptempText->GetTextWidth()), TwipsToPixels(ptempText->GetTextHeight()));
    sz.Expand(TwipsToPixels(GFX_TEXT_GUTTER*2));
    return sz;
}
GSizeF GFxDrawTextManager::GetTextExtent(const GString& str, Float width, const TextParams* ptxtParams)
{
    CheckFontStatesChange();
    GFxTextFormat tfmt(pHeap);
    GFxTextParagraphFormat pfmt;
    TextParams txtParams = (ptxtParams) ? *ptxtParams : pImpl->DefaultTextParams;
    GPtr<GFxTextDocView> ptempText = *CreateTempDoc(txtParams, &tfmt, &pfmt, PixelsToTwips(width), 0);
    // need to reset Multiline and WordWrap, since they are already set up
    // correctly inside the CreateTempDoc.
    txtParams.Multiline = txtParams.WordWrap = false; 
    SetTextParams(ptempText, txtParams, &tfmt, &pfmt);
    ptempText->SetText(str);

    //ptempText->Dump();
    GSizeF sz(TwipsToPixels(ptempText->GetTextWidth()), TwipsToPixels(ptempText->GetTextHeight()));
    sz.Expand(TwipsToPixels(GFX_TEXT_GUTTER*2));
    return sz;
}

GSizeF GFxDrawTextManager::GetHtmlTextExtent(const char* putf8Str, Float width, const TextParams* ptxtParams)
{
    CheckFontStatesChange();
    GFxTextFormat tfmt(pHeap);
    GFxTextParagraphFormat pfmt;
    TextParams txtParams = (ptxtParams) ? *ptxtParams : pImpl->DefaultTextParams;
    GPtr<GFxTextDocView> ptempText = *CreateTempDoc(txtParams, &tfmt, &pfmt, PixelsToTwips(width), 0);
    // need to reset Multiline and WordWrap, since they are already set up
    // correctly inside the CreateTempDoc.
    txtParams.Multiline = txtParams.WordWrap = false;
    SetTextParams(ptempText, txtParams, &tfmt, &pfmt);
    GFxStyledText::HTMLImageTagInfoArray imageInfoArray(this->pHeap);	
    ptempText->ParseHtml(putf8Str, GUTF8Util::GetLength(putf8Str), false, &imageInfoArray);	
    if (imageInfoArray.GetSize() > 0)
    {
        GFxDrawTextImpl::ProcessImageTags(ptempText, this, imageInfoArray);
    }

    GSizeF sz(TwipsToPixels(ptempText->GetTextWidth()), TwipsToPixels(ptempText->GetTextHeight()));
    sz.Expand(TwipsToPixels(GFX_TEXT_GUTTER*2));
    return sz;
}
GSizeF GFxDrawTextManager::GetHtmlTextExtent(const wchar_t* pwstr, Float width, const TextParams* ptxtParams)
{
    CheckFontStatesChange();
    GFxTextFormat tfmt(pHeap);
    GFxTextParagraphFormat pfmt;
    TextParams txtParams = (ptxtParams) ? *ptxtParams : pImpl->DefaultTextParams;
    GPtr<GFxTextDocView> ptempText = *CreateTempDoc(txtParams, &tfmt, &pfmt, PixelsToTwips(width), 0);
    // need to reset Multiline and WordWrap, since they are already set up
    // correctly inside the CreateTempDoc.
    txtParams.Multiline = txtParams.WordWrap = false; 
    SetTextParams(ptempText, txtParams, &tfmt, &pfmt);
    GFxStyledText::HTMLImageTagInfoArray imageInfoArray(this->pHeap);
    ptempText->ParseHtml(pwstr, G_wcslen(pwstr), false, &imageInfoArray);
    if (imageInfoArray.GetSize() > 0)
    {
        GFxDrawTextImpl::ProcessImageTags(ptempText, this, imageInfoArray);
    }

    GSizeF sz(TwipsToPixels(ptempText->GetTextWidth()), TwipsToPixels(ptempText->GetTextHeight()));
    sz.Expand(TwipsToPixels(GFX_TEXT_GUTTER*2));
    return sz;
}
GSizeF GFxDrawTextManager::GetHtmlTextExtent(const GString& str, Float width, const TextParams* ptxtParams)
{
    CheckFontStatesChange();
    GFxTextFormat tfmt(pHeap);
    GFxTextParagraphFormat pfmt;
    TextParams txtParams = (ptxtParams) ? *ptxtParams : pImpl->DefaultTextParams;
    GPtr<GFxTextDocView> ptempText = *CreateTempDoc(txtParams, &tfmt, &pfmt, PixelsToTwips(width), 0);
    // need to reset Multiline and WordWrap, since they are already set up
    // correctly inside the CreateTempDoc.
    txtParams.Multiline = txtParams.WordWrap = false; 
    SetTextParams(ptempText, txtParams, &tfmt, &pfmt);
	GFxStyledText::HTMLImageTagInfoArray imageInfoArray(this->pHeap);		
	ptempText->ParseHtml(str, str.GetLength(), false, &imageInfoArray);	
	if (imageInfoArray.GetSize() > 0)
	{
        GFxDrawTextImpl::ProcessImageTags(ptempText, this, imageInfoArray);
	}

    GSizeF sz(TwipsToPixels(ptempText->GetTextWidth()), TwipsToPixels(ptempText->GetTextHeight()));
    sz.Expand(TwipsToPixels(GFX_TEXT_GUTTER*2));
    return sz;
}

GFxStateBag* GFxDrawTextManager::GetStateBagImpl() const 
{ 
    return pImpl->pStateBag; 
}

GFxTextAllocator*       GFxDrawTextManager::GetTextAllocator() 
{ 
    return pImpl->pTextAllocator; 
}

GFxFontManager*         GFxDrawTextManager::GetFontManager()   
{ 
    return pImpl->pFontManager; 
}

GFxFontManagerStates*   GFxDrawTextManager::GetFontManagerStates() 
{ 
    return pImpl->pFontStates; 
}

void GFxDrawTextManager::CheckFontStatesChange()
{
    pImpl->pFontStates->CheckStateChange(GetFontLib(), GetFontMap(), GetFontProvider(), GetTranslator());
}

void GFxDrawTextManager::BeginDisplay(const GViewport& vp)
{
    if (IsBeginDisplayInvokedFlagSet())
    {
        GPtr<GFxLog> plog = pImpl->pStateBag->GetLog();
        if (plog)
            plog->LogWarning("Nested GFxDrawTextManager::BeginDisplay() call detected\n");
        return;
    }
    GFxDrawText::Matrix viewportMat;
    viewportMat.AppendScaling(1.f/20*vp.Scale*vp.AspectRatio, 1.f/20*vp.Scale);

    pImpl->DisplayContext.Init(this, NULL, NULL, 1, viewportMat);
    if (!pImpl->DisplayContext.GetRenderConfig() ||
        !pImpl->DisplayContext.GetRenderer())
    {
        GPtr<GFxLog> plog = pImpl->pStateBag->GetLog();
        if (plog)
            plog->LogWarning("GFxDrawTextManager: Renderer is not set! Nothing will be rendered!\n");
        return;
    }
    SetBeginDisplayInvokedFlag();
    pImpl->DisplayContext.GetRenderer()->BeginDisplay(GColor::White, vp, 0, 
        vp.Width*20.f/(vp.Scale*vp.AspectRatio), 0, vp.Height*20.f/vp.Scale);
}

void GFxDrawTextManager::EndDisplay()
{
    if (!IsBeginDisplayInvokedFlagSet())
    {
        GPtr<GFxLog> plog = pImpl->pStateBag->GetLog();
        if (plog)
            plog->LogWarning("GFxDrawTextManager::EndDisplay() is called w/o BeginDisplay() call\n");
        return;
    }
    ClearBeginDisplayInvokedFlag();
    pImpl->DisplayContext.GetRenderer()->EndDisplay();
}

#endif //GFC_NO_DRAWTEXT_SUPPORT
