/**********************************************************************

Filename    :   GFxDisplayContext.cpp
Content     :   Implements GFxDisplayContext class, used to pass state
                and binding information during rendering.
Created     :   
Authors     :   Michael Antonov

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   More implementation-specific classes are
                defined in the GFxPlayerImpl.h

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxDisplayContext.h"

#include "GFxCharacter.h"
#include "GFxPlayerImpl.h"

// Necessary to avoid template related issued wiuth GASEnvironment.
#include "GFxAction.h"


// ***** GFxDisplayContext

GFxDisplayContext::GFxDisplayContext()
{
    pRenderStats      = 0;
    pChar             = 0;
#ifndef GFC_NO_3D
    pRealParentMatrix3D = 0;
#endif
    pResourceBinding  = 0;
    MaskRenderCount   = 0;
    MaskStackIndex    = 0;
    MaskStackIndexMax = 0;
    PixelScale        = 1;
    pStats            = 0;
    pRealParentCxform = 0;
}

GFxDisplayContext::GFxDisplayContext(const GFxStateBag *pstate,
                                     GFxResourceWeakLib *plib,
                                     GFxResourceBinding *pbinding,
                                     Float pixelScale,
                                     const Matrix& viewportMatrix,
                                     GFxAmpViewStats* pstats,
                                     bool prepass)
    : GFxDisplayContextFilters(prepass), pStats(pstats)
{
    Init(pstate, plib, pbinding, pixelScale, viewportMatrix);
}

GFxDisplayContext::~GFxDisplayContext()
{
    GASSERT(MaskStackIndex == 0);
    GASSERT(MaskStackIndexMax == 0);
    GASSERT(MaskRenderCount == 0);
}

void GFxDisplayContext::Init(const GFxStateBag *pstate,
                             GFxResourceWeakLib *plib,
                             GFxResourceBinding *pbinding,
                             Float pixelScale,
                             const Matrix& viewportMatrix)
{
    // Capture the state for efficient access
    // (Usually called in the beginning for Advance)

    const static GFxState::StateType stateQuery[6] =
    { 
        GFxState::State_RenderConfig, GFxState::State_GradientParams,
        GFxState::State_ImageCreator, GFxState::State_FontCacheManager,
        GFxState::State_Log, GFxState::State_MeshCacheManager
    };

    GFxState* pstates[6] = {0,0,0,0,0,0};

    // Get multiple states at once to avoid extra locking.
    pstate->GetStatesAddRef(pstates, stateQuery, 6);
    pRenderConfig     = *(GFxRenderConfig*)     pstates[0];
    pGradientParams   = *(GFxGradientParams*)   pstates[1];
    pImageCreator     = *(GFxImageCreator*)     pstates[2];
    pFontCacheManager = *(GFxFontCacheManager*) pstates[3];
    pLog              = *(GFxLog*)              pstates[4];
    pMeshCacheManager = *(GFxMeshCacheManager*) pstates[5];

    pWeakLib          = plib;
    pResourceBinding  = pbinding;

    pRenderStats      = 0;

    MaskRenderCount   = 0;
    MaskStackIndex    = 0;
    MaskStackIndexMax = 0;
    pRealParentCxform = 0;
#ifndef GFC_NO_3D
    pRealParentMatrix3D = 0;
#endif
    PixelScale        = pixelScale;
    ViewportMatrix    = viewportMatrix;
    pChar             = 0;

    GFxDisplayContextTransforms::ResetTransforms();
}

// Mask Stack management
void GFxDisplayContext::PushAndDrawMask(GFxCharacter* pmask)
{
    GRenderer*  prenderer = GetRenderer();
    // Only clear stencil if no masks were applied before us; otherwise
    // no clear is necessary because we are building a cumulative mask.
    prenderer->BeginSubmitMask((MaskStackIndex == 0) ?
                               GRenderer::Mask_Clear : GRenderer::Mask_Increment);

    // Stack bounds check. We increment anyway, but access only lower stack.
    if (MaskStackIndex < GFxDisplayContext::Mask_MaxStackSize)
        MaskStack[MaskStackIndex] = pmask;
    MaskStackIndex++;
    MaskStackIndexMax++;
    MaskRenderCount++;

    // Draw mask.
    prenderer->PushBlendMode(pmask->GetBlendMode());
    pmask->Display(*this);
    prenderer->PopBlendMode();

    // Done rendering mask.
    MaskRenderCount--;
    prenderer->EndSubmitMask();
}

void GFxDisplayContext::PopMask()
{
    GASSERT(MaskStackIndex > 0);
    MaskStackIndex--;
    // If we reached the bottom of stack, disable mask checks.        
    if (MaskStackIndex == 0)
    {
        GetRenderer()->DisableMask();
        MaskStackIndexMax = 0;
    }
}

void GFxDisplayContext::PreDrawMask(GFxCharacter* pmask, GMatrix2D *pmatrix, GMatrix3D *pmatrix3D)
{
#ifndef GFC_NO_3D
    if (pmask->Is3D(true))
    {
        pmask->GetParent()->GetWorldMatrix3D(pmatrix3D);
        pParentMatrix3D = pmatrix3D;
        pParentMatrix = &GMatrix2D::Identity;
    }
    else
    {
        // 2d case
        pmask->GetParent()->GetWorldMatrix(pmatrix);
        pParentMatrix = pmatrix;
        pParentMatrix3D = NULL;
    }
#else
    // 2d case
    pmask->GetParent()->GetWorldMatrix(pmatrix);
    pParentMatrix = pmatrix;
    GUNUSED(pmatrix3D);
#endif
}

void GFxDisplayContext::PreDisplay(GFxDisplayContextTransforms &oldXform, const GFxCharacter *pchar, 
                                   GMatrix2D *pmatrix, GMatrix3D *pmatrix3D, GRenderer::Cxform *pcxform) 
{
    *pmatrix = *oldXform.pParentMatrix;
    pChar = pchar;

#ifndef GFC_NO_3D
    if (Is3D == false)
#endif
    {
        pmatrix->Prepend(pchar->GetMatrix()); 
        pParentMatrix = pmatrix;
    }

    *pcxform = *pParentCxform;
    pcxform->Concatenate(pchar->GetCxform());
    pParentCxform = pcxform;

#ifndef GFC_NO_3D
    if (Is3D || pchar->Is3D())
    {
        *pmatrix3D = oldXform.pParentMatrix3D ? *oldXform.pParentMatrix3D : GMatrix3D::Identity;
        GMatrix3D local3DMat = (pchar->GetMatrix3D() ? *pchar->GetMatrix3D() : GMatrix3D::Identity);
        if (pchar->Is3D() && Is3D == false)
        {
            local3DMat.Append(GMatrix3D(*pParentMatrix));                           // add 2d world mat
            Is3D = true;                                                            // enter 3d mode
        }
        else if (Is3D == true)
            local3DMat.Append(GMatrix3D(pchar->GetMatrix()));                                 // add 2d local mat

        if (Is3D)
        {
            pmatrix3D->Prepend(local3DMat);
            pParentMatrix3D = pmatrix3D;
            GetRenderer()->SetWorld3D(pmatrix3D);
            pParentMatrix = &GMatrix2D::Identity;                                   // reset 2d matrix

            pViewMatrix3D = pchar->GetView3D() ? pchar->GetView3D() : oldXform.pViewMatrix3D;
            pPerspectiveMatrix3D = pchar->GetPerspective3D() ? pchar->GetPerspective3D() : oldXform.pPerspectiveMatrix3D;
            if (!pRealParentMatrix3D)	// don't change view/persp if rendering to a filter texture
            {
                if (pViewMatrix3D)
                    GetRenderer()->SetView3D(*pViewMatrix3D);
                if (pPerspectiveMatrix3D)
                    GetRenderer()->SetPerspective3D(*pPerspectiveMatrix3D);
            }
        }
    }
#else
    GUNUSED(pmatrix3D);
#endif
}

// restore old transforms
void GFxDisplayContext::PostDisplay(GFxDisplayContextTransforms &oldXform)
{
#ifndef GFC_NO_3D
    if (Is3D)
        GetRenderer()->SetWorld3D(oldXform.pParentMatrix3D);
if (!pRealParentMatrix3D)   	// don't change view/persp if rendering to a filter texture
{
    if (oldXform.pViewMatrix3D)
        GetRenderer()->SetView3D(*oldXform.pViewMatrix3D);
    if (oldXform.pPerspectiveMatrix3D)
        GetRenderer()->SetPerspective3D(*oldXform.pPerspectiveMatrix3D);
}
#endif
    CopyTransformsFrom(oldXform);       // restore context transform info
}

bool GFxDisplayContext::BeginFilters(GFxDisplayContextFilters& oldFilters, GFxCharacter* ch, const GArray<GFxFilterDesc>& Filters)
{
    if (Filters.GetSize())
    {
        bool   HasFilter = 0;
        Float  FilterSizeX = 0;
        Float  FilterSizeY = 0;

        for (UInt i = 0; i < Filters.GetSize(); i++)
        {
            const GFxFilterDesc &filter = Filters[i];
            bool bBlurFilter = ((filter.Blur.BlurX > 1 || filter.Blur.BlurY > 1) && ((filter.Flags & 0xF) == GFxFilterDesc::Filter_Blur));
            bool bShadowFilter = (filter.Blur.Color.GetAlpha() > 0 && ((filter.Flags & 0xF) == GFxFilterDesc::Filter_DropShadow));
            bool bGlowFilter = (filter.Blur.Color.GetAlpha() > 0 && ((filter.Flags & 0xF) == GFxFilterDesc::Filter_Glow));
            bool bBevelFilter = (filter.Blur.Color.GetAlpha() > 0 && ((filter.Flags & 0xF) == GFxFilterDesc::Filter_Bevel));
            if (bBlurFilter)
            {
                HasFilter = 1;
                FilterSizeX += 20 * ceilf(filter.Blur.BlurX * filter.Blur.Passes);
                FilterSizeY += 20 * ceilf(filter.Blur.BlurY * filter.Blur.Passes);
            }
            else if (bShadowFilter || bGlowFilter || bBevelFilter)
            {
                HasFilter = 1;

                // the filter area must be extended even for inner shadows as the blurred area outside is still read
                Float count = bBevelFilter ? 2.f : 1.f;
                FilterSizeX += count * 20 * ceilf(filter.Blur.BlurX * filter.Blur.Passes);
                FilterSizeY += count * 20 * ceilf(filter.Blur.BlurY * filter.Blur.Passes);

                if (bShadowFilter)
                {
                    GPointF offset = filter.GetShadowOffset();
                    FilterSizeX += ceilf(count * 20 * fabsf(offset.x));
                    FilterSizeY += ceilf(count * 20 * fabsf(offset.y));
                }
            }
            else if ((filter.Flags & 0xf) == GFxFilterDesc::Filter_AdjustColor)
                HasFilter = 1;
        }

        if (!HasFilter)
            return 0;

        oldFilters.CopyFilterStateFrom(*this);

        scrXScale = (Float)ViewportMatrix.GetXScale();
        scrYScale = (Float)ViewportMatrix.GetYScale();

        mcRect = ch->GetBounds(*pParentMatrix); 
        mcRect.Expand(FilterSizeX, FilterSizeY);
        mcWidth = (UInt)ceilf(mcRect.Width() * scrXScale);
        mcHeight = (UInt)ceilf(mcRect.Height() * scrYScale);

        // Before setting the new render target, give the app a chance to save off the previous contents
        GetRenderer()->SaveCurrentRenderTargetContents();

        pFilterRTT = GetRenderer()->PushTempRenderTarget(mcRect, mcWidth, mcHeight, true);
        if (pFilterRTT)
        {
            pRealParentCxform = pParentCxform;
            pParentCxform = &GRenderer::Cxform::Identity;            

#ifndef GFC_NO_3D
            // Draw object without 3D
            if (Is3D)
            {
                pRealParentMatrix3D = pParentMatrix3D;
                pParentMatrix3D = &GMatrix3D::Identity;
                GetRenderer()->SetWorld3D(NULL);
                Is3D = false;
            }
#endif

            // Use normal blending for the filtered clip, and all the filters except the last one.
            // Overlay is used because it is not actually supported (treated as Normal), but isn't ignored if there are "real"
            // blend modes below it on stack.
            GetRenderer()->PushBlendMode(GRenderer::Blend_Overlay);

            FilterStack++;
            return 1;
        }
        else
            CopyFilterStateFrom(oldFilters);
    }

    return 0;
}

void GFxDisplayContext::EndFilters(const GFxDisplayContextFilters& oldFilters, GFxCharacter* ch,
                                   const GArray<GFxFilterDesc>& Filters, bool finishPrePass)
{
	GUNUSED(ch);
    GASSERT(pFilterRTT);
    GRenderer* prenderer = GetRenderer();

    UInt filtercount = (UInt)Filters.GetSize();
    UInt lastfilter = filtercount - 1;

#ifndef GFC_NO_3D
    // restore correct 3d matrix when drawing RT
    if (pRealParentMatrix3D)
    {
        GetRenderer()->SetWorld3D(pRealParentMatrix3D);
//        pRealParentMatrix3D = 0;
    }
#endif
#ifndef GFC_NO_FILTERS_PREPASS
    if (!finishPrePass)
        prenderer->PopRenderTarget();

    if (IsInPrePass)
    {
        // Leave the temp last render target, don't draw anything to the primary framebuffer
        lastfilter++;
        // The last filter is drawn in the main pass
        filtercount--;
    }
#endif

    for (UInt i = 0; i < filtercount; i++)
    {
        const GFxFilterDesc &filter = Filters[i];
        bool bBlurFilter = ((filter.Blur.BlurX > 1 || filter.Blur.BlurY > 1) && ((filter.Flags & 0xF) == GFxFilterDesc::Filter_Blur));
        bool bShadowFilter = (filter.Blur.Color.GetAlpha() > 0 && ((filter.Flags & 0xF) == GFxFilterDesc::Filter_DropShadow));
        bool bBevelFilter = (filter.Blur.Color.GetAlpha() > 0 && ((filter.Flags & 0xF) == GFxFilterDesc::Filter_Bevel));
        bool bGlowFilter = (filter.Blur.Color.GetAlpha() > 0 && ((filter.Flags & 0xF) == GFxFilterDesc::Filter_Glow));
        bool bColorFilter = ((filter.Flags & 0xF) == GFxFilterDesc::Filter_AdjustColor);

        if (bBlurFilter || bShadowFilter || bGlowFilter || bColorFilter || bBevelFilter)
        {
            prenderer->SetMatrix(GMatrix2D::Identity);

            GPtr<GTexture> pNext;
            if (i != lastfilter)
            {
                pNext = prenderer->PushTempRenderTarget(mcRect, mcWidth, mcHeight);
                if (!pNext)
                {
                    // out of resources, this will be the last filter processed in this stack
                    lastfilter = i;
                    filtercount = i+1;
                }
            }

            if (i == lastfilter && !finishPrePass)
                prenderer->PopBlendMode();

            if (bColorFilter)
            {
                if (i == lastfilter)
                {
                    Float cxmatrix[20];
                    for (UInt i = 0; i < 5; i++)
                    {
                        cxmatrix[i*4+0] = filter.ColorMatrix[i*4+0] * pRealParentCxform->M_[0][0] * pRealParentCxform->M_[3][0];
                        cxmatrix[i*4+1] = filter.ColorMatrix[i*4+1] * pRealParentCxform->M_[1][0] * pRealParentCxform->M_[3][0];
                        cxmatrix[i*4+2] = filter.ColorMatrix[i*4+2] * pRealParentCxform->M_[2][0] * pRealParentCxform->M_[3][0];
                        cxmatrix[i*4+3] = filter.ColorMatrix[i*4+3] * pRealParentCxform->M_[3][0];
                    }
                    cxmatrix[16] = (filter.ColorMatrix[16] + pRealParentCxform->M_[0][1] * 1.f/255.f) * pRealParentCxform->M_[3][0];
                    cxmatrix[17] = (filter.ColorMatrix[17] + pRealParentCxform->M_[1][1] * 1.f/255.f) * pRealParentCxform->M_[3][0];
                    cxmatrix[18] = (filter.ColorMatrix[18] + pRealParentCxform->M_[2][1] * 1.f/255.f) * pRealParentCxform->M_[3][0];
                    cxmatrix[19] = (filter.ColorMatrix[19] + pRealParentCxform->M_[3][1] * 1.f/255.f) * pRealParentCxform->M_[3][0];

                    // This is the last filter. Ensure we restore the previous contents of the render target
                    prenderer->DrawColorMatrixRect(pFilterRTT, GRectF(0,0,Float(mcWidth),Float(mcHeight)), mcRect, cxmatrix, true);
                }
                else {
                    // Not the last filter. No need to restore the previous contents of the render target
                    prenderer->DrawColorMatrixRect(pFilterRTT, GRectF(0,0,Float(mcWidth),Float(mcHeight)), mcRect, filter.ColorMatrix, false);
                }
            }
            else
            {
                GRenderer::BlurFilterParams params (filter.Blur);
                if (i == lastfilter)
                    params.cxform = *pRealParentCxform;

                prenderer->DrawBlurRect(pFilterRTT, GRectF(0,0,Float(mcWidth),Float(mcHeight)), mcRect,
					GRenderer::BlurFilterParams(params).Scale(scrXScale*20.f,scrYScale*20.f), i == lastfilter);
            }

            if (i != lastfilter)
            {
                prenderer->PopRenderTarget();
                pFilterRTT = pNext;
            }
        }
    }

    pParentCxform = pRealParentCxform;
#ifndef GFC_NO_3D
    if (pRealParentMatrix3D)
    {
        pParentMatrix3D = pRealParentMatrix3D;
        pRealParentMatrix3D = 0;
    }
#endif
    FilterStack--;
    if (!IsInPrePass)
    {
        pFilterRTT = 0;
        CopyFilterStateFrom(oldFilters);
    }
    else
    {
        prenderer->PopBlendMode();
        if (FilterStack > 0)
            CopyFilterStateFrom(oldFilters);
    }
}

void GFxDisplayContext::DisplayFilterPrePass(GFxCharacter* ch, const GArray<GFxFilterDesc>& Filters,
                                             GTexture* pFilterResult, GRectF mcr, UInt mcw, UInt mch)
{
    GFxDisplayContextFilters oldFilters;
    oldFilters.CopyFilterStateFrom(*this);

    // Even if in a prepass, display nested filters normally
    IsInPrePass = 0;
    pRealParentCxform = pParentCxform;
    pFilterRTT = pFilterResult;
    mcRect = mcr;
    mcWidth = mcw;
    mcHeight = mch;
    scrXScale = (Float)ViewportMatrix.GetXScale();
    scrYScale = (Float)ViewportMatrix.GetYScale();

    EndFilters(oldFilters, ch, Filters, true);
    CopyFilterStateFrom(oldFilters);
}
