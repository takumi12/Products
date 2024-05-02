/**********************************************************************

Filename    :   GFxDisplayContext.h
Content     :   Defines GFxDisplayContext class, used to pass state
                and binding information during rendering.
Created     :   
Authors     :   Michael Antonov

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   More implementation-specific classes are
                defined in the GFxPlayerImpl.h

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXDISPLAYCONTEXT_H
#define INC_GFXDISPLAYCONTEXT_H

#include "GRefCount.h"
#include "GFxResource.h"
#include "GFxResourceHandle.h"

#include "GStats.h"

#include "GFxRenderConfig.h"
// Loader: For Gradient/Image states.
#include "GFxLoader.h"
#include "GFxFilterDesc.h"

// ***** Declared Classes

class GFxDisplayContext;

// External Classes
class GFxCharacter;
class GFxAmpViewStats;

//
// *** transform info that is saved and restored during display object traversal
// This class pulls the transform related members (matrices) out of display content 
// so that they can be easily saved, restored and initialized during display list traversal.
// This class is meant to be a base class of GFxDisplayContext.
//
class GFxDisplayContextTransforms
{

public:
    typedef GRenderer::Matrix Matrix;

    void ResetTransforms()
    {       
        pParentMatrix     = &GMatrix2D::Identity;
        pParentCxform     = &GRenderer::Cxform::Identity;
#ifndef GFC_NO_3D
        pParentMatrix3D   = NULL;
        Is3D              = false;
        pViewMatrix3D     = NULL;
        pPerspectiveMatrix3D = NULL;
#endif
    }

    void CopyTransformsTo(GFxDisplayContextTransforms *pXFormInfo) const
    {
        pXFormInfo->pParentCxform   = pParentCxform;
        pXFormInfo->pParentMatrix   = pParentMatrix;
#ifndef GFC_NO_3D
        pXFormInfo->pParentMatrix3D = pParentMatrix3D;
        pXFormInfo->pPerspectiveMatrix3D = pPerspectiveMatrix3D;
        pXFormInfo->pViewMatrix3D   = pViewMatrix3D;
        pXFormInfo->Is3D            = Is3D;
#endif
    }

    void CopyTransformsFrom(const GFxDisplayContextTransforms &xformInfo)
    {
        pParentCxform   = xformInfo.pParentCxform;
        pParentMatrix   = xformInfo.pParentMatrix;
#ifndef GFC_NO_3D
        pParentMatrix3D = xformInfo.pParentMatrix3D;
        pPerspectiveMatrix3D = xformInfo.pPerspectiveMatrix3D;
        pViewMatrix3D   = xformInfo.pViewMatrix3D;
        Is3D            = xformInfo.Is3D;
#endif
    }

    GFxDisplayContextTransforms() { ResetTransforms(); }

public:

    GRenderer::Cxform          *pParentCxform;
    Matrix                     *pParentMatrix;
#ifndef GFC_NO_3D
    GMatrix3D                  *pParentMatrix3D;        // world 3d matrix (2D and 3D combined)
    const GMatrix3D            *pPerspectiveMatrix3D;   
    const GMatrix3D            *pViewMatrix3D;    
    bool                        Is3D;
#endif
};

class GFxDisplayContextFilters
{
public:
    bool                        IsInPrePass;
    SInt                        FilterStack;
    GRenderer::Cxform           *pRealParentCxform;
    GRectF                      mcRect;
    UInt                        mcWidth, mcHeight;
    Float                       scrXScale, scrYScale;
    GPtr<GTexture>              pFilterRTT;

    GFxDisplayContextFilters(bool prepass = 0) : IsInPrePass(prepass) { pRealParentCxform = 0; FilterStack = 0; }

    void CopyFilterStateFrom(const GFxDisplayContextFilters& state)
    {
        IsInPrePass = state.IsInPrePass;
        pRealParentCxform = state.pRealParentCxform;
        pFilterRTT = state.pFilterRTT;
        mcWidth = state.mcWidth;
        mcHeight = state.mcHeight;
        mcRect = state.mcRect;
        scrXScale = state.scrXScale;
        scrYScale = state.scrYScale;
    }
};


// *****  GFxDisplayContext - used by GFxCharacter::Display

// Display context - passes renderer/masking state.
// This class is here largely to support creation of nested masks;
// i.e. it uses a stack to track mask layers that were applied so far.

class GFxDisplayContext : public GFxDisplayContextTransforms, public GFxDisplayContextFilters
{
public:    
    typedef GRenderer::Matrix Matrix;

    GPtr<GFxRenderConfig> pRenderConfig;
    GFxRenderConfig*    GetRenderConfig() const   { return pRenderConfig; }
    GRenderer*          GetRenderer() const       { return pRenderConfig->GetRenderer(); }
    UInt32              GetRenderFlags() const    { return pRenderConfig->GetRenderFlags(); }        

    // Statistics
    GFxRenderStats*     pRenderStats;
    const GFxCharacter* pChar;		// last character using this context 

#ifndef GFC_NO_3D
    GMatrix3D                  *pRealParentMatrix3D;
#endif
    // *** Image generation states

    // States useful during rendering
    // (used to generate gradients and dynamic images).
    GPtr<GFxGradientParams>     pGradientParams;
    GPtr<GFxImageCreator>       pImageCreator;
    GPtr<GFxFontCacheManager>   pFontCacheManager;
    GPtr<GFxLog>                pLog;               // Log used during rendering 
    GPtr<GFxMeshCacheManager>   pMeshCacheManager;

    // Library
    GPtr<GFxResourceWeakLib>    pWeakLib;

    // Resource binding: need this to look up.
    // Note that this variable changes as it is re-assigned during display traversal.
    // Can be null (initially, but not during shape rendering).
    GFxResourceBinding*         pResourceBinding;    

    // Tessellator Data

    // Pixel scale used for rendering
    Float                       PixelScale;

    Matrix                      ViewportMatrix;

    GFxAmpViewStats*            pStats;    

    // *** Mask stack support - necessary to ensure proper clipping.
    
    enum ContextConstants {
        Mask_MaxStackSize = 64,
    };    
    // Indicates how deep inside of the mask rendering we are (if > 0 we are rendering mask).
    // Technically masking of masks is impossible (?); but this is a safeguard.
    UInt            MaskRenderCount;

    // A stack layer of currently active masks.
    // Entries in this stack need to be re-applied if a layer is popped and
    // partially non-masked rendering must take place after it.
    UInt            MaskStackIndexMax;
    UInt            MaskStackIndex;
    GFxCharacter*   MaskStack[Mask_MaxStackSize];    


    GFxDisplayContext();
    GFxDisplayContext(const GFxStateBag *pstate,
                      GFxResourceWeakLib *plib,
                      GFxResourceBinding *pbinding,
                      Float pixelScale,
                      const Matrix& viewportMatrix,
                      GFxAmpViewStats *pstat,
                      bool isprepass = 0);  
    ~GFxDisplayContext();
   
    void    PreDisplay(GFxDisplayContextTransforms &oldXform, const GFxCharacter *pchar, 
                            GMatrix2D *pmatrix, GMatrix3D *pmatrix3D, GRenderer::Cxform *pcxform);
    void    PostDisplay(GFxDisplayContextTransforms &oldXform);
    void    PreDrawMask(GFxCharacter* pmask, GMatrix2D *pmatrix, GMatrix3D *pmatrix3D);

    // Mask stack helpers.
    void    PushAndDrawMask(GFxCharacter* pmask);
    void    PopMask();

    Float   GetPixelScale() const   { return PixelScale; }

    // Statistics update
    void    AddTessTriangles(UInt count)
    {
        if (pRenderStats)
            pRenderStats->AddTessTriangles(count);
    }
    void Init(const GFxStateBag *pstate,
        GFxResourceWeakLib *plib,
        GFxResourceBinding *pbinding,
        Float pixelScale,
        const Matrix& viewportMatrix);

    void SetParentMatrix(Matrix *pmatrix)
    {
        pParentMatrix = pmatrix;
    }

    // Filter helpers.

    // call after PreDisplay
    bool        BeginFilters(GFxDisplayContextFilters& oldFilters, GFxCharacter* ch, const GArray<GFxFilterDesc>& Filters);
    void        EndFilters(const GFxDisplayContextFilters& oldFilters, GFxCharacter* ch, const GArray<GFxFilterDesc>& Filters,
        bool finishPrePass = 0);

    void        DisplayFilterPrePass(GFxCharacter* ch, const GArray<GFxFilterDesc>& Filters,
                                     GTexture* pFilterResult, GRectF mcr, UInt mcw, UInt mch);
};



#endif // INC_GFXDISPLAYCONTEXT_H
