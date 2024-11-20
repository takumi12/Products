/**********************************************************************

Filename    :   GFxShape.cpp
Content     :   Shape character definition with paths and edges
Created     :
Authors     :   Michael Antonov, Artem Bolgar

Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.
                Patent Pending. Contact Scaleform for more information.

Notes       :

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFile.h"
#include "GFxLog.h"
#include "GFxStream.h"
#include "GFxShape.h"
#include "GFxMesh.h"
#include "GFxCharacter.h"
#include "GFxPlayerImpl.h"
#include "GRenderer.h"
#include "GFxRenderGen.h"

#include "GFxLoadProcess.h"

#include "GFxImageResource.h"
#include "GFxDisplayContext.h"
#include "GFxMeshCacheManager.h"
#include "AMP/GFxAmpViewStats.h"

#include <float.h>
#include <stdlib.h>

//#if defined(GFC_OS_WIN32)
  //  #include <windows.h>
//#endif

#ifdef GFC_BUILD_DEBUG
//#define GFX_SHAPE_MEM_TRACKING
#endif  //GFC_BUILD_DEBUG

#ifdef GFX_SHAPE_MEM_TRACKING
UInt GFx_MT_SwfGeomMem = 0;
UInt GFx_MT_PathsCount = 0;
UInt GFx_MT_GeomMem = 0;
UInt GFx_MT_ShapesCount = 0;
#endif //GFX_SHAPE_MEM_TRACKING

// Debugging macro: Define this to generate "shapes" file, used for
// shape extraction and external tessellator testing.

//
// Shape I/O helper functions.
//

// Read fill styles, and push them onto the given style GArray.
static int GFx_ReadFillStyles(GFxFillStyleArrayTemp* styles, GFxLoadProcess* p, GFxTagType tagType)
{
    //GASSERT(styles);

    // Get the count.
    UInt fillStyleCount = p->ReadU8();
    if (tagType > GFxTag_DefineShape)
    {
        if (fillStyleCount == 0xFF)
            fillStyleCount = p->ReadU16();
    }
    int off = p->Tell();

    if (styles)
    {
        p->LogParse("  GFx_ReadFillStyles: count = %d\n", fillStyleCount);

        // Read the styles.
        UInt baseIndex = (UInt)styles->GetSize();
        if (fillStyleCount)
            styles->Resize(baseIndex + fillStyleCount);
        for (UInt i = 0; i < fillStyleCount; i++)
        {
            p->AlignStream(); //!AB
            (*styles)[baseIndex + i].Read(p, tagType);
        }
    }
    else if (fillStyleCount > 0)
    {
        p->LogError("Error: GFx_ReadFillStyles, trying to read %d fillstyles into no-style shape\n", fillStyleCount);
    }
    return off;
}

// Read line styles and push them onto the back of the given GArray.
static int GFx_ReadLineStyles(GFxLineStyleArrayTemp* styles, GFxLoadProcess* p, GFxTagType tagType)
{
    //GASSERT(styles);

    // Get the count.
    UInt lineStyleCount = p->ReadU8();

    p->LogParse("  GFx_ReadLineStyles: count = %d\n", lineStyleCount);

    // @@ does the 0xFF flag apply to all tag types?
    // if (TagType > GFxTag_DefineShape)
    // {
    if (lineStyleCount == 0xFF)
    {
        lineStyleCount = p->ReadU16();
        p->LogParse("  GFx_ReadLineStyles: count2 = %d\n", lineStyleCount);
    }
    // }
    int off = p->Tell();

    if (styles)
    {
        // Read the styles.
        UInt baseIndex = (UInt)styles->GetSize();
        styles->Resize(baseIndex + lineStyleCount);

        for (UInt i = 0; i < lineStyleCount; i++)
        {
            p->AlignStream(); //!AB
            (*styles)[baseIndex + i].Read(p, tagType);
        }
    }
    else if (lineStyleCount > 0)
    {
        p->LogError("Error: GFx_ReadLineStyles, trying to read %d linestyles into no-style shape\n", lineStyleCount);
    }
    return off;
}

// Implementation of virtual path Iterator, that may be used for shapes
// with unknown or mixed path data formats.
//
template<class ShapeDef, class PathData>
class GFxVirtualPathIterator : public GFxShapeBase::PathsIterator
{
    typedef typename PathData::PathsIterator NativePathsIterator;
    typedef typename PathData::EdgesIterator NativeEdgesIterator;
    NativePathsIterator PathIter;
    NativeEdgesIterator EdgeIter;
    StateInfo State;
public:
    GFxVirtualPathIterator(const ShapeDef* pdef) :
    PathIter(pdef->GetNativePathsIterator())
    { 
        if(PathIter.IsFinished())
        {
            State.State = StateInfo::St_Finished;
        }
        else
        {
            State.State = StateInfo::St_Setup;
            PathIter.GetStyles(&State.Setup.Fill0, &State.Setup.Fill1, &State.Setup.Line);
            EdgeIter = PathIter.GetEdgesIterator();
            EdgeIter.GetMoveXY(&State.Setup.MoveX, &State.Setup.MoveY);
        }
    }
    virtual bool GetNext(StateInfo* pcontext)
    {
        *pcontext = State;
        switch(State.State)
        {
        case StateInfo::St_Setup:
        case StateInfo::St_Edge:
            if (EdgeIter.IsFinished())
            {
                State.State = StateInfo::St_NewPath;
                PathIter.AdvanceBy(EdgeIter);
            }
            else
            {
                typename NativeEdgesIterator::Edge edge;
                EdgeIter.GetEdge(&edge);
                State.State = StateInfo::St_Edge;
                State.Edge.Cx = edge.Cx;
                State.Edge.Cy = edge.Cy;
                State.Edge.Ax = edge.Ax;
                State.Edge.Ay = edge.Ay;
                State.Edge.Curve = edge.Curve;
            }
            break;
        case StateInfo::St_NewPath:
            if (PathIter.IsFinished())
                State.State = StateInfo::St_Finished;
            else
            {
                if (PathIter.IsNewShape())
                    PathIter.Skip();
                State.State = StateInfo::St_Setup;
                PathIter.GetStyles(&State.Setup.Fill0, &State.Setup.Fill1, &State.Setup.Line);
                EdgeIter = NativeEdgesIterator(PathIter);
                EdgeIter.GetMoveXY(&State.Setup.MoveX, &State.Setup.MoveY);
            }
            break;
        default:;
        }
        return pcontext->State != StateInfo::St_Finished;
    }
};

//////////////////////////////////////////////////////////////////////////
// GFxShapeBase
//
GFxShapeBase::GFxShapeBase() : 
    RefCount(1), pMeshCache(0), pMeshSetBag(0), pPreTessMesh(0)
{
    Bound = GRectF(0,0,0,0);
    Flags = 0;
    HintedGlyphSize = 0;
}

GFxShapeBase::~GFxShapeBase()
{
    ResetCache();
}

// Thread-Safe ref-count implementation.
void    GFxShapeBase::AddRef()
{
    RefCount.Increment_NoSync();
    //  SInt32 val = RefCount.ExchangeAdd_NoSync(1);
    //  if (ResType == 100)
    //       printf("Thr %4d, %8x : GFxResource::AddRef (%d -> %d)\n", GetCurrentThreadId(), this, val, val+1);  
}
void    GFxShapeBase::Release()
{
    if ((RefCount.ExchangeAdd_Acquire(-1) - 1) == 0)
//    if (--RefCount == 0)
    {  
        delete this;
    }
}

void GFxShapeBase::ResetCache()
{
    if (pMeshCache)
        pMeshCache->AddShapeToKillList(this);
    pMeshCache = 0;
    delete pPreTessMesh;
    pPreTessMesh = 0;
}

const GFxFillStyle* GFxShapeBase::GetFillStyles(UInt* pstylesNum) const
{
    *pstylesNum = 0;
    return NULL;
}

const GFxLineStyle* GFxShapeBase::GetLineStyles(UInt* pstylesNum) const
{
    *pstylesNum = 0;
    return NULL;
}

void GFxShapeBase::GetFillAndLineStyles
    (const GFxFillStyle** ppfillStyles, UInt* pfillStylesNum, 
     const GFxLineStyle** pplineStyles, UInt* plineStylesNum) const
{
    *ppfillStyles   = NULL;
    *pfillStylesNum = 0;
    *pplineStyles   = NULL;
    *plineStylesNum = 0;
}

void GFxShapeBase::GetFillAndLineStyles(GFxDisplayParams* pparams) const
{
    pparams->pFillStyles = NULL;
    pparams->pLineStyles = NULL;
    pparams->FillStylesNum = 0;
    pparams->LineStylesNum = 0;
}

void    GFxShapeBase::ApplyScale9Grid(GCompoundShape* shape,
                                         const GFxScale9GridInfo& sg)
{
    UInt i;
    for (i = 0; i < shape->GetNumVertices(); ++i)
    {
        GPointType& v = shape->GetVertex(i);
        sg.Transform(&v.x, &v.y);
    }
}

// Draw the shape using our own inherent styles.
void    GFxShapeBase::Display(GFxDisplayContext &context, GFxCharacter* inst)
{
    GMatrix2D matrix = *context.pParentMatrix;
    matrix.Prepend(inst->GetMatrix());
    GRenderer::Cxform cxform = *context.pParentCxform;
    cxform.Concatenate(inst->GetCxform());

    GFxDisplayParams params(context, matrix,
                            cxform, inst->GetActiveBlendMode());

    // Do viewport culling if bounds are available (and not 3D).
    if (
#ifndef GFC_NO_3D
        context.pParentMatrix3D == NULL && 
#endif
        HasValidBounds())
    {
        GRectF           tbounds;

        params.Mat.EncloseTransform(&tbounds, Bound);
        if (!inst->GetMovieRoot()->GetVisibleFrameRectInTwips().Intersects(tbounds))
        {
            if (!(context.GetRenderFlags() & GFxRenderConfig::RF_NoViewCull))
                return;
        }
    }

    if (Flags & Flags_StylesSupport)
    {
        // fill fill and line styles
        GetFillAndLineStyles(&params.pFillStyles, &params.FillStylesNum, 
                             &params.pLineStyles, &params.LineStylesNum);
    }
    Display(params, (inst->GetClipDepth() > 0), inst);
}





// Display our shape.  Use the FillStyles arg to
// override our default set of fill Styles (e.G. when
// rendering text).
void    GFxShapeBase::Display (GFxDisplayParams& params,
                               bool edgeAADisabled,
                               GFxCharacter* inst)
{
    const GFxRenderConfig &rconfig = *params.Context.GetRenderConfig();
    GFxMeshCache* cache = params.Context.pMeshCacheManager->GetMeshCache();

#ifdef GFC_NO_FXPLAYER_EDGEAA
    GUNUSED(edgeAADisabled);
    bool    useEdgeAA   = 0;
#else
    bool    useEdgeAA   =   rconfig.IsUsingEdgeAA() &&
        (rconfig.IsEdgeAATextured() || !HasTexturedFill()) &&
        !edgeAADisabled && params.Context.MaskRenderCount == 0;
#endif

    GPtr<GFxScale9GridInfo> s9g;

    if ((Flags & Flags_S9GSupport) && inst && inst->DoesScale9GridExist())
        s9g = *inst->CreateScale9Grid(params.Context.GetPixelScale() * rconfig.GetMaxCurvePixelError());

    // Check to see if it's possible to use the pre-tessellated mesh. 
    // The pre-tessellated mesh must exist, plus there should not be 
    // scale9grid, plus, if the shape is used as a mask, there should not 
    // be EdgeAA.
    if (pPreTessMesh && s9g.GetPtr() == 0)
    {
        if (useEdgeAA == pPreTessMesh->IsUsingEdgeAA())
        {
            pPreTessMesh->Display(params, false);
            return;
        }
    }

    GFxMeshSet* meshSet = 0;

    // Compute the error tolerance in object-space.
    Float   maxScale = params.Mat.GetMaxScale();

    //Float sx = mat.M_[0][0] + mat.M_[0][1];
    //Float sy = mat.M_[1][0] + mat.M_[1][1];
    //Float maxScale = sqrtf(sx*sx + sy*sy) * 0.707106781f;

    if (fabsf(maxScale) < 1e-6f)
    {
        // Scale is essentially zero.
        return;
    }

    bool  cxformHasAddAlpha = (fabs(params.Cx.M_[3][1]) >= 1.0f);

    // If Alpha is Zero, don't draw.
    // Though, if the mask is being drawn, don't consider the alpha. Tested with MovieClip.setMask (!AB)
    if ((fabs(params.Cx.M_[3][0]) < 0.001f) && !cxformHasAddAlpha && params.Context.MaskRenderCount == 0)
        return;

    Float masterScale = params.Context.GetPixelScale() * maxScale;

    GFxMeshSet::KeyCategoryType meshKeyCat;
    UInt  meshKeyLen;
    UInt  meshKeyFlags =  
        (useEdgeAA != false) | 
        ((cxformHasAddAlpha != false) << 1) | 
        ((rconfig.IsOptimizingTriangles() != false) << 2);
    UInt8 meshKey[GFxScale9GridInfo::KeySize];

    // Make the meshKey. If scale9grid is in use we look for the 
    // meshSet whose key fits the given scale9grid. 
    // Otherwise just use the mesh that corresponds to the scaleKey. 
    //-------------------
    if (s9g.GetPtr() == 0)
    {
        meshKey[0] = UInt8(meshKeyFlags);
        meshKey[1] = UInt8(GFx_ComputeShapeScaleKey(masterScale, useEdgeAA));
        meshKey[2] = UInt8(GFx_ComputeShapeScaleKey(rconfig.GetMaxCurvePixelError(), false));
        meshKeyLen = 3;
        meshKeyCat = GFxMeshSet::KeyCategory_Regular;
    }
    else
    {
        s9g->MakeKey(meshKey, masterScale, 0, (UInt8)meshKeyFlags);
        meshKeyLen = GFxScale9GridInfo::KeySize;
        meshKeyCat = GFxMeshSet::KeyCategory_Scale9Grid;
    }

    meshSet = cache->GetMeshSet(this, inst, meshKeyCat, meshKey, meshKeyLen);
    if (meshSet)
    {
        meshSet->DoLock();
        meshSet->Display(params, s9g.GetPtr() != 0);
        meshSet->Unlock();
        return;
    }

    // Construct A new GFxMesh to handle this pixel size and the 
    // other visual parameters. Then tessellate it and store in
    // the proper cache, depending on whether or not Scale9grid is in use.
    //----------------
    {
        Float screenPixelSize = 20.0f / masterScale;
        Float curveError      = screenPixelSize * 0.75f * rconfig.GetMaxCurvePixelError();
        meshSet = GHEAP_NEW(cache->GetHeap())
                    GFxMeshSet(screenPixelSize, 
                               curveError, !useEdgeAA, rconfig.IsOptimizingTriangles());
    #ifndef GFC_NO_FXPLAYER_EDGEAA
        meshSet->SetAllowCxformAddAlpha(cxformHasAddAlpha);
    #endif
        Tessellate(meshSet, curveError, params.Context, s9g);
    }

    meshSet->SetMeshKey(meshKeyCat, meshKey, meshKeyLen, inst);
    meshSet->Display(params, s9g.GetPtr() != 0);
    cache->AddMeshSet(this, meshSet);

//printf("T");// DBG
}

// Find the bounds of this shape, and store them in
// the given rectangle.
template <class PathData>
void    GFxShapeBase::ComputeBoundImpl(GRectF* r) const
{
    r->Left = 1e10f;
    r->Top = 1e10f;
    r->Right = -1e10f;
    r->Bottom = -1e10f;

    typename PathData::PathsIterator it = typename PathData::PathsIterator(this);
    while(!it.IsFinished())
    {
        if (it.IsNewShape())
        {
            it.Skip();
            continue;
        }

        typename PathData::EdgesIterator edgesIt = it.GetEdgesIterator();
        Float ax, ay;
        edgesIt.GetMoveXY(&ax, &ay);
        r->ExpandToPoint(ax, ay);

        while (!edgesIt.IsFinished())
        {
            typename PathData::EdgesIterator::Edge edge;
            edgesIt.GetEdge(&edge);
            if (edge.Curve)
            {
                Float t, x, y;
                t = GMath2D::CalcQuadCurveExtremum(ax, edge.Cx, edge.Ax);
                if (t > 0 && t < 1)
                {
                    GMath2D::CalcPointOnQuadCurve(
                        ax, ay, 
                        edge.Cx, edge.Cy, 
                        edge.Ax, edge.Ay,
                        t, &x, &y);
                    r->ExpandToPoint(x, y);
                }
                t = GMath2D::CalcQuadCurveExtremum(ay, edge.Cy, edge.Ay);
                if (t > 0 && t < 1)
                {
                    GMath2D::CalcPointOnQuadCurve(
                        ax, ay, 
                        edge.Cx, edge.Cy, 
                        edge.Ax, edge.Ay,
                        t, &x, &y);
                    r->ExpandToPoint(x, y);
                }
            }
            r->ExpandToPoint(edge.Ax, edge.Ay);
            ax = edge.Ax;
            ay = edge.Ay;
        }
        it.AdvanceBy(edgesIt);
    }
}


namespace GMath2D
{
    //--------------------------------------------------------------------
    struct QuadCoordType
    {
        Float x1, y1, x2, y2, x3, y3;
    };

    //--------------------------------------------------------------------
    template<class CurveType>
    void SubdivideQuadCurve(GCoordType x1, GCoordType y1, 
                            GCoordType x2, GCoordType y2, 
                            GCoordType x3, GCoordType y3, 
                            GCoordType t, 
                            CurveType* c1, CurveType* c2)
    {
        GCoordType x12  = x1  + t*(x2  - x1);
        GCoordType y12  = y1  + t*(y2  - y1);
        GCoordType x23  = x2  + t*(x3  - x2);
        GCoordType y23  = y2  + t*(y3  - y2);
        GCoordType x123 = x12 + t*(x23 - x12);
        GCoordType y123 = y12 + t*(y23 - y12);
        c1->x1 = x1; 
        c1->y1 = y1;
        c1->x2 = x12; 
        c1->y2 = y12; 
        c1->x3 = x123;
        c1->y3 = y123;
        c2->x1 = x123;
        c2->y1 = y123; 
        c2->x2 = x23; 
        c2->y2 = y23;
        c2->x3 = x3;
        c2->y3 = y3;
    }

    //--------------------------------------------------------------------
    inline GCoordType CalcPointOnQuadCurve1D(GCoordType x1, 
                                             GCoordType x2, 
                                             GCoordType x3,
                                             GCoordType t)
    {
        x1 += t*(x2 - x1);
        x2 += t*(x3 - x2);
        return x1 + t*(x2 - x1);
    }

    //--------------------------------------------------------------------
    inline bool CheckMonoCurveIntersection(GCoordType x1, GCoordType y1, 
                                           GCoordType x2, GCoordType y2,
                                           GCoordType x3, GCoordType y3,
                                           GCoordType x,  GCoordType y)
    {
        // Check the Y-monotone quadratic curve for the intersection 
        // with a horizontal ray from (x,y) to the left. First check Y.
        //----------------------
        if (y >= y1 && y < y3) // Conditions >= && < are IMPORTANT!
        {
            // Early out. Check if all tree edges (triangle) lie on the left 
            // or on the right. First means "definitely no intersection", 
            // second - "definitely there is an intersection".
            // It's OK to use bitwise expressions rather than logical 
            // as potentially more efficient.
            //-----------------------
            unsigned cp1 = GMath2D::CrossProduct(x1, y1, x2, y2, x, y) > 0;
            unsigned cp2 = GMath2D::CrossProduct(x2, y2, x3, y3, x, y) > 0;
            unsigned cp3 = GMath2D::CrossProduct(x1, y1, x3, y3, x, y) > 0;

            if (cp1 & cp2 & cp3)             // cp1>0  && cp2>0  && cp3>0, on the right
                return true;

            if ((cp1^1) & (cp2^1) & (cp3^1)) // cp1<=0 && cp2<=0 && cp3<=0, on the left
                return false;

            // Intermediate case, the point is inside the triangle. 
            // Calculate the real intersection point between the curve and 
            // the horizontal line at Y. Then check if the intersection point 
            // lies on the left of the given x.
            //-----------------------
            Float den = y1 - 2*y2 + y3;
            Float t = -1;
            if(den == 0)
            {
                den = y3 - y1;
                if (den != 0)
                    t = (y - y1) / den;
            }
            else
            {
                // The initial expression is: 
                // t = (sqrtf(-y1 * (y3 - y) + y2*y2 - 2*y*y2 + y*y3) + y1 - y2) / den;
                // Theoretically the expression under the sqrtf 
                // (-y1 * (y3 - y) + y2*y2 - 2*y*y2 + y*y3) must never be negative. 
                // However, it may occur because of finite precision of 
                // float/double. So that, it's a good idea to clamp the value to ]0...oo]
                //------------------------
                t = -y1 * (y3 - y) + y2*y2 - 2*y*y2 + y*y3;
                t = (t > 0) ? sqrtf(t) : 0;
                t = (t + y1 - y2) / den;
            }
            return GMath2D::CalcPointOnQuadCurve1D(x1, x2, x3, t) < x;
        }
        return false;
    }

    //--------------------------------------------------------------------
    bool CheckCurveIntersection(GCoordType x1, GCoordType y1, 
                                GCoordType x2, GCoordType y2,
                                GCoordType x3, GCoordType y3,
                                GCoordType x,  GCoordType y) 
    {
        if(y2 >= y1 && y2 <= y3)
        {
            // A simple (Y-monotone) case
            //-----------------
            return CheckMonoCurveIntersection(x1, y1, x2, y2, x3, y3, x, y);
        }

        // The curve has a Y-extreme. Subdivide it at the 
        // extreme point and process separately.
        //--------------------
        Float dy = (2*y2 - y1 - y3);
        Float ty = (dy == 0) ? -1 : (y2 - y1) / dy;

        // Subdivide the curves they needs to be normalized 
        // (y1 must be less than y2). To do so it's theoretically
        // enough to check only one of (c1.y1, c1.y3) or (c2.y1, c2.y3),
        // but in practice it's better to check them both for the sake
        // of numerical stability.
        //--------------------
        QuadCoordType c1, c2;
        SubdivideQuadCurve(x1, y1, x2, y2, x3, y3, ty, &c1, &c2);
        if(c1.y1 > c1.y3)
        {
            G_Swap(c1.x1, c1.x3);
            G_Swap(c1.y1, c1.y3);
        }
        if(c2.y1 > c2.y3)
        {
            G_Swap(c2.x1, c2.x3);
            G_Swap(c2.y1, c2.y3);
        }

        // The results of these calls must differ. That is, if the hit-test point 
        // lies on the left or on the right of both sub-curves, it definitely
        // means that the curve is not contributing (0 or 2 intersections).
        // Different results mean that the point lies between the sub-curves
        // ("inside" the original curve).
        //-------------------
        return CheckMonoCurveIntersection(c1.x1, c1.y1, c1.x2, c1.y2, c1.x3, c1.y3, x, y) != 
               CheckMonoCurveIntersection(c2.x1, c2.y1, c2.x2, c2.y2, c2.x3, c2.y3, x, y);
    }

}






template<class PathData, class PathIterator>
bool GFxShapeBase::PointInShape(PathIterator& it, const GFxScale9GridInfo* s9g, Float x, Float y) const
{
#ifndef GFC_NO_FXPLAYER_STROKER
    UInt                lineStylesNum = 0;
    const GFxLineStyle* lineStyles = GetLineStyles(&lineStylesNum);
    bool strokeFlag = (lineStylesNum && (Flags & Flags_StylesSupport) != 0);
    GPtr<GFxRenderGenStroker> str;

#endif

    int styleCount = 0;
    while(!it.IsFinished())
    {
        if (it.IsNewShape())
        {
            if (styleCount)
                return true;
            it.Skip();
            styleCount = 0;
        }
        else
        {
            UInt fill0, fill1, line;
            it.GetStyles(&fill0, &fill1, &line);
            if (line > 0 || (fill0 == 0) != (fill1 == 0))
            {
                typedef typename PathData::EdgesIterator EdgesIteratorType;
                EdgesIteratorType ei = it.GetEdgesIterator();
                Float ax, ay;
                ei.GetMoveXY(&ax, &ay);
                if (s9g)
                    s9g->Transform(&ax, &ay);

#ifndef GFC_NO_FXPLAYER_STROKER
                if (strokeFlag && line > 0)
                {
                    if(str.GetPtr() == 0)
                    {
                        str = *GNEW GFxRenderGenStroker;
                        str->Shape.SetCurveTolerance(20.0f);
                        str->Stroke.SetCurveTolerance(str->Shape.GetCurveTolerance());
                    }
                    str->Shape.BeginPath(-1, -1, line - 1, ax, ay);
                }
#endif
                typename EdgesIteratorType::Edge edge;
                while(!ei.IsFinished())
                {
                    ei.GetEdge(&edge);
                    if (s9g)
                        s9g->Transform(&edge.Ax, &edge.Ay);
                    Float x1 = ax;
                    Float y1 = ay;
                    Float x3 = edge.Ax;
                    Float y3 = edge.Ay;
                    if(y1 > y3)
                    {
                        G_Swap(x1, x3);
                        G_Swap(y1, y3);
                    }
                    if (edge.Curve)
                    {
                        if (s9g)
                            s9g->Transform(&edge.Cx, &edge.Cy);

                        if ((fill0 == 0) != (fill1 == 0) &&
                            GMath2D::CheckCurveIntersection(x1, y1, edge.Cx, edge.Cy, x3, y3, x, y))
                        {
                            styleCount ^= 1;
                        }
                    }
                    else
                    if ((fill0 == 0) != (fill1 == 0) &&
                        y >= y1 && y < y3 && 
                        GMath2D::CrossProduct(x1, y1, x3, y3, x, y) > 0)
                    {
                        styleCount ^= 1;
                    }

#ifndef GFC_NO_FXPLAYER_STROKER
                    if (strokeFlag && line > 0)
                    {
                        if (edge.Curve)
                            str->Shape.AddCurve(edge.Cx, edge.Cy, edge.Ax, edge.Ay);
                        else
                            str->Shape.AddVertex(edge.Ax, edge.Ay);
                    }
#endif
                    ax = edge.Ax;
                    ay = edge.Ay;
                }
                it.AdvanceBy(ei);
            }
            else
            {
                it.SkipComplex();
            }
        }
    }
    if (styleCount)
        return true;

#ifndef GFC_NO_FXPLAYER_STROKER
    if(str.GetPtr())
    {
        for(UInt i = 0; i < str->Shape.GetNumPaths(); ++i)
        {
            const GCompoundShape::SPath& path = str->Shape.GetPath(i);
            const GFxLineStyle& style = lineStyles[path.GetLineStyle()];
            str->Stroker.SetWidth((style.GetWidth() < 20.0f) ? 20.0f : style.GetWidth());
            str->Stroker.SetLineJoin(GFx_SWFToFxStroke_LineJoin(style.GetJoin()));
            str->Stroker.SetStartLineCap(GFx_SWFToFxStroke_LineCap(style.GetStartCap()));
            str->Stroker.SetEndLineCap(GFx_SWFToFxStroke_LineCap(style.GetEndCap()));
            if (style.GetJoin() == GFxLineStyle::LineJoin_Miter) 
                str->Stroker.SetMiterLimit(style.GetMiterSize());

            str->Stroke.Clear();
            str->Stroker.GenerateStroke(path, str->Stroke);
            if (str->Stroke.PointInShape(x, y, true))
                return true;
        }
    }
#endif
    return false;
}

/*
struct GFxPathBounds
{
    float x1,y1,x2,y2;
};

bool GFxCmpPathBounds(const GFxPathBounds& a, const GFxPathBounds& b)
{
    if (a.y1 != b.y1) return a.y1 < b.y1;
    return a.x1 < b.x1;
}
*/

// Return true if the specified GRenderer::Point is on the interior of our shape.
// Incoming coords are local coords.
template <class PathData>
bool    GFxShapeBase::DefPointTestLocalImpl(const GPointF &pt, 
                                            bool testShape, 
                                            const GFxCharacter* pinst) const
{
//printf("%f %f\n", pt.x, pt.y);// DBG
    GPtr<GFxScale9GridInfo> s9g;

    if (pinst && pinst->DoesScale9GridExist())
    {
        s9g = *pinst->CreateScale9Grid(1.0f);
        if (s9g.GetPtr())
            s9g->Compute();
    }

    GRectF b2 = Bound;
    if (!HasValidBounds())
        ComputeBound(&b2);

    if (s9g.GetPtr())
        b2 = s9g->AdjustBounds(b2);

    // Early out.
    if (!b2.Contains(pt))
        return false;

    // If we are within bounds and not testing shape, return true.
    if (!testShape)
        return true;

    if (pinst && s9g.GetPtr() == 0 && pinst->CheckLastHitResult(pt.x, pt.y))
    {
//printf("-");// DBG
        return pinst->WasLastHitPositive();
    }

    typedef typename PathData::PathsIterator PathIteratorType;
    PathIteratorType it = typename PathData::PathsIterator(this);

    bool result = PointInShape<PathData, PathIteratorType>(it, s9g, pt.x, pt.y);

    if (pinst)
        pinst->SetLastHitResult(pt.x, pt.y, result);

//printf("*"); // DBG
    return result;
}



// Push our shape data through the tessellator.
template<class PathData>
void GFxShapeBase::TessellateImpl(GFxMeshSet *meshSet, Float tolerance,
                                  GFxDisplayContext &context,
                                  GFxScale9GridInfo* s9g,
                                  Float maxStrokeExtent) const
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer tessTimer(context.pStats, NativeCodeSwdHandle, Func_GFxShapeBase_TessellateImpl, Amp_Profile_Level_Low);
#endif
    
    GFxRenderGen* rg = context.pMeshCacheManager->GetRenderGen();

    rg->Shape1.Clear(); //    GCompoundShape cs;
    rg->Shape1.SetCurveTolerance(tolerance);
    rg->Shape1.SetNonZeroFill(IsNonZeroFill());

    // Character shapes may have invalid bounds.
    if (HasValidBounds())
        meshSet->SetShapeBounds(Bound, maxStrokeExtent);

    if (s9g)
    {
        s9g->Compute();
        if (HasValidBounds())
            meshSet->SetShapeBounds(s9g->AdjustBounds(Bound), maxStrokeExtent);

        // Merging compatible image scale9grid generated by  Grant Skinner's 
        // JSFL command. See http://www.gskinner.com/blog/archives/2010/04/bitmapslice9_sc.html
        // The plug-in generates 9 rectangles in one shape, separated by NewShape flag.
        // The conditions are:
        // - the shape must have exactly 9 image fill styles
        // - the shape must have exactly 9 subshapes
        // - the sum area of all 9 rectangles must be equal to the area 
        //   of the bounding box, with 2x2 pixels tolerance.
        //--------------------------------------
        unsigned imgStyleCount = 0;
        if (Flags & Flags_StylesSupport)
        {
            unsigned numFillStyles = 0;
            const GFxFillStyle* fillStyles = GetFillStyles(&numFillStyles);
            if (numFillStyles == 9)
            {
                for(unsigned i = 0; i < 9; ++i)
                {
                    // TO DO: revise and possibly check for the same image/texture
                    if (fillStyles[i].IsImageFill())
                        imgStyleCount++;
                }
            }
        }
 
        if (imgStyleCount == 9)
        {
            typename PathData::PathsIterator it = typename PathData::PathsIterator(this);
            while(!it.IsFinished())
            {
                it.AddForTessellation(&rg->Shape1);
                if (it.IsNewShape())
                {
                    it.Skip();
                }
            }
            if (rg->Shape1.GetNumPaths() == 9)
            {
                float x1, y1, x2, y2;
                float area1 = 0;
                for(unsigned i = 0; i < rg->Shape1.GetNumPaths(); ++i)
                {
                    x1 =  1e30f;
                    y1 =  1e30f;
                    x2 = -1e30f;
                    y2 = -1e30f;
                    rg->Shape1.ExpandPathBounds(rg->Shape1.GetPath(i), &x1, &y1, &x2, &y2);
                    area1 += (x2-x1)*(y2-y1);
                }
                rg->Shape1.PerceiveBounds(&x1, &y1, &x2, &y2);
                float area2 = (x2-x1)*(y2-y1);
                const float areaTolerance = 20*2*20*2; // 2x2 pixels tolerance

                if (fabsf(area1-area2) <= areaTolerance)
                {
                    const GCompoundShape::SPath& path = rg->Shape1.GetPath(0);
                    if ((path.GetLeftStyle() < 0) != (path.GetRightStyle() < 0) && path.GetLineStyle() < 0)
                    {
                        int styleIdx = (path.GetLeftStyle() >= 0) ? path.GetLeftStyle() : path.GetRightStyle();
                        rg->Shape1.Clear();
                        rg->Shape1.BeginPath(styleIdx, -1, -1);
                        rg->Shape1.AddVertex(x1, y1);
                        rg->Shape1.AddVertex(x2, y1);
                        rg->Shape1.AddVertex(x2, y2);
                        rg->Shape1.AddVertex(x1, y2);
                        rg->Shape1.ClosePath();
                        unsigned numStyles;
                        GFxFillStyle* fillStyles = (GFxFillStyle*)GetFillStyles(&numStyles);

                        GRenderer::FillTexture  texture;
                        fillStyles[styleIdx].GetFillTexture(&texture, context, 1, 0);
                        if (!texture.TextureMatrix.IsFreeRotation())
                        {
                            fillStyles[styleIdx].SetImageMatrix(GMatrix2D().AppendScaling(0.01f));

                            // The automatic slicing only works for clipped images, 
                            // while in this case they are tiled. So, replace the type.
                            unsigned type = fillStyles[styleIdx].GetType();
                            if (type == GFxFill_TiledImage)
                                type = GFxFill_ClippedImage;

                            if (type == GFxFill_TiledSmoothImage)
                                type = GFxFill_ClippedSmoothImage;

                            fillStyles[styleIdx].SetFillType((GFxFillType)type);

                            // Done, store last set of meshes.
                            AddShapeToMesh(meshSet, &rg->Shape1, context, s9g);
                            rg->Shape1.Clear();
                            return;
                        }
                    }
                }
            }
        }
    }

    rg->Shape1.Clear();
    typename PathData::PathsIterator it = typename PathData::PathsIterator(this);
    while(!it.IsFinished())
    {
        if (it.IsNewShape())
        {
            // Process the compound shape and prepare it for next set of paths.
            AddShapeToMesh(meshSet, &rg->Shape1, context, s9g);
            rg->Shape1.Clear();
            it.Skip();
        }
        else
        {
            it.AddForTessellation(&rg->Shape1);
        }
    }

    // Done, store last set of meshes.
    AddShapeToMesh(meshSet, &rg->Shape1, context, s9g);
    rg->Shape1.Clear();

    if (s9g && s9g->ImgAdjustments.GetSize())
    {
        s9g->ComputeImgAdjustMatrices();
        meshSet->SetImgAdjustMatrices(s9g->ImgAdjustments);
    }
}


template<class PathData>
void GFxShapeBase::PreTessellateImpl(Float masterScale, 
                                     const GFxRenderConfig& config, 
                                     Float maxStrokeExtent)
{
    if (pPreTessMesh)
        return;

#ifdef GFC_NO_FXPLAYER_EDGEAA
    bool    useEdgeAA        = 0;
#else
    bool    useEdgeAA        =  config.IsUsingEdgeAA() &&
        (config.IsEdgeAATextured() || !HasTexturedFill());
#endif

    GPtr<GFxRenderGen> rg = *GNEW GFxRenderGen(GMemory::GetGlobalHeap());

    Float screenPixelSize = 20.0f / masterScale;
    Float curveError      = screenPixelSize * 0.75f * config.GetMaxCurvePixelError();

    // Construct A new GFxMesh to handle this error tolerance.
    pPreTessMesh = GHEAP_AUTO_NEW(this) GFxMeshSet(screenPixelSize, 
                                                   curveError, !useEdgeAA, false);

    rg->Shape1.Clear();
    rg->Shape1.SetCurveTolerance(curveError);
    rg->Shape1.SetNonZeroFill(IsNonZeroFill());

    // Character shapes may have invalid bounds.
    if (HasValidBounds())
        pPreTessMesh->SetShapeBounds(Bound, maxStrokeExtent);

    //PathsIterator it = GetPathsIterator();
    typename PathData::PathsIterator it = typename PathData::PathsIterator(this);
    UInt fillStylesNum = 0;
    const GFxFillStyle* pfillStyles = GetFillStyles(&fillStylesNum);
    while(!it.IsFinished())
    {
        if (it.IsNewShape())
        {
            // Add and Tessellate shape, including styles and line strips.
            rg->Shape1.RemoveShortSegments(curveError / 4);
            pPreTessMesh->AddTessellatedShape(rg->Shape1, 
                                              pfillStyles, 
                                              fillStylesNum, 
                                              (GFxDisplayContext*)0, 
                                              config);
            // Prepare compound shape for nest set of paths.
            rg->Shape1.Clear();
            it.Skip();
        }
        else
        {
            it.AddForTessellation(&rg->Shape1);
        }
    }
    rg->Shape1.RemoveShortSegments(curveError / 4);
    pPreTessMesh->AddTessellatedShape(rg->Shape1, 
                                      pfillStyles, 
                                      fillStylesNum, 
                                      (GFxDisplayContext*)0, 
                                      config);

//printf("P"); // DBG
}

// Convert the paths to GCompoundShape, flattening the curves.
// The function ignores the NewShape flag and adds all paths
// GFxShapeBase contains.
template<class PathData>
void    GFxShapeBase::MakeCompoundShapeImpl(GCompoundShape *cs, Float tolerance) const
{
    cs->Clear();
    cs->SetCurveTolerance(tolerance);
    cs->SetNonZeroFill(IsNonZeroFill());
    typename PathData::PathsIterator it = typename PathData::PathsIterator(this);
    while(!it.IsFinished())
    {
        it.AddForTessellation(cs);
    }
}


//------------------------------------------------------------------------------
static UInt32 GFx_ComputeBernsteinHash(const void* buf, UInt size, UInt32 seed = 5381)
{
    const UByte*    pdata   = (const UByte*) buf;
    UInt32          hash    = seed;
    while (size)
    {
        size--;
        hash = ((hash << 5) + hash) ^ (UInt)pdata[size];
    }
    return hash;
}

template<class PathData>
UInt32 GFxShapeBase::ComputeGeometryHashImpl() const
{
    UInt32 hash = 5381;
    UInt shapesCnt = 0, pathsCnt = 0;
    GetShapeAndPathCounts(&shapesCnt, &pathsCnt);
    hash = GFx_ComputeBernsteinHash(&shapesCnt, sizeof(shapesCnt), hash);
    hash = GFx_ComputeBernsteinHash(&pathsCnt,  sizeof(pathsCnt),  hash);
    typename PathData::PathsIterator pit = typename PathData::PathsIterator(this);

    while(!pit.IsFinished())
    {
        UInt styles[3];
        pit.GetStyles(&styles[0], &styles[1], &styles[2]);
        hash = GFx_ComputeBernsteinHash(&styles,  sizeof(styles),  hash);

        typename PathData::EdgesIterator eit = pit.GetEdgesIterator();

        Float mx[2];
        eit.GetMoveXY(&mx[0], &mx[1]);
        hash = GFx_ComputeBernsteinHash(&mx,  sizeof(mx),  hash);

        // Counting num of edges might be expensive! Especially for Swf paths.
        //UInt numEdges = eit.GetEdgesCount();
        //hash = GFx_ComputeBernsteinHash(&numEdges, sizeof(numEdges), hash);
        while (!eit.IsFinished())
        {
            typename PathData::EdgesIterator::PlainEdge edge;
            eit.GetPlainEdge(&edge);
            hash = GFx_ComputeBernsteinHash(&edge.Data, edge.Size * sizeof(SInt32), hash);
        }
        pit.AdvanceBy(eit);
    }
    return hash;
}

template<class PathData>
bool    GFxShapeBase::IsEqualGeometryImpl(const GFxShapeBase& cmpWith) const
{
    UInt shapesCnt = 0, pathsCnt = 0;
    GetShapeAndPathCounts(&shapesCnt, &pathsCnt);
    UInt cmpWithShapesCnt = 0, cmpWithPathsCnt = 0;
    cmpWith.GetShapeAndPathCounts(&cmpWithShapesCnt, &cmpWithPathsCnt);
    if (shapesCnt != cmpWithShapesCnt || pathsCnt  != cmpWithPathsCnt)
        return false;

    typename PathData::PathsIterator pit1 = typename PathData::PathsIterator(this);
    typename PathData::PathsIterator pit2 = typename PathData::PathsIterator(&cmpWith);
    while(!pit1.IsFinished())
    {
        if (pit2.IsFinished()) 
            return false;

        UInt styles1[3], styles2[3];
        pit1.GetStyles(&styles1[0], &styles1[1], &styles1[2]);
        pit2.GetStyles(&styles2[0], &styles2[1], &styles2[2]);
        if (memcmp(styles1, styles2, sizeof(styles1)) != 0)
            return false;

        // !AB: theoretically, we need to compare fill styles too, but
        // for font glyphs this is not critical.

        typename PathData::EdgesIterator eit1 = pit1.GetEdgesIterator();
        typename PathData::EdgesIterator eit2 = pit2.GetEdgesIterator();

        Float ax1, ay1, ax2, ay2;
        eit1.GetMoveXY(&ax1, &ay1);
        eit2.GetMoveXY(&ax2, &ay2);
        if (ax1 != ax2 || ay1 != ay2)
            return false;

        if (eit1.GetEdgesCount() != eit2.GetEdgesCount())
            return false;

        typename PathData::EdgesIterator::PlainEdge edge1;
        typename PathData::EdgesIterator::PlainEdge edge2;
        while (!eit1.IsFinished())
        {
            if (eit2.IsFinished()) 
                return false;

            eit1.GetPlainEdge(&edge1);
            eit2.GetPlainEdge(&edge2);

            if (edge1.Size != edge2.Size ||
                memcmp(edge1.Data, edge2.Data, edge1.Size * sizeof(SInt)) != 0)
                return false;
        }
        pit1.AdvanceBy(eit1);
        pit2.AdvanceBy(eit2);
    }
    return true;
}

void GFxShapeBase::AddShapeToMesh(GFxMeshSet *meshSet, 
                                  GCompoundShape* cs, 
                                  GFxDisplayContext &context,
                                  GFxScale9GridInfo* s9g) const
{
    GRenderer::FillTexture  texture;
    GFxTexture9Grid         t9g;
    GRectF                  t9gClip;
    bool                    useTiling = false;

    const GFxFillStyle* pfillStyles = NULL;
    UInt fillStylesNum = 0;
    if (Flags & Flags_StylesSupport)
    {
        pfillStyles = GetFillStyles(&fillStylesNum);
    }

    // Add and tessellate shape, including styles and line strips.

    if ((Flags & (Flags_S9GSupport | Flags_StylesSupport)) && s9g && s9g->CanUseTiling)
    {
        // The images can be drawn with automatic tiling according to the 
        // scale9grid. It is possible when: 
        // 1. The image is on the level of the scale9grid movie clip 
        //    (s9g->CanUseTiling == true), and
        // 2. The shape has only one path, one fill style, and no line style 
        //    (which basically represents a Flash image as opposed to a shape 
        //    with image fill style), and
        // 3. The shape fill is ClippedImageFill (not tiled), and
        // 4. The texture matrix has no free rotation.
        //----------------------
        int texture9GridStyle = GetTexture9GridStyle(*cs);
        if (texture9GridStyle >= 0)
        {
            pfillStyles[texture9GridStyle].GetFillTexture(&texture, context, 1, 0);
            if (!texture.TextureMatrix.IsFreeRotation())
            {
                // The clipping bounds for the texture9grid must be calculated for this 
                // particular part of the shape. It's incorrect to take just RectBound.
                //--------------------------
                cs->PerceiveBounds(&t9gClip.Left, &t9gClip.Top, &t9gClip.Right, &t9gClip.Bottom);
                t9g.Compute(*s9g, t9gClip, meshSet->GetScaleMultiplier(), texture9GridStyle);
                meshSet->AddTexture9Grid(t9g);
                useTiling = true;
            }
        }
    }

    if (!useTiling)
    {
        if (s9g)
        {
            s9g->ComputeImgAdjustRects(*cs, pfillStyles, fillStylesNum);
            ApplyScale9Grid(cs, *s9g);
        }
        cs->RemoveShortSegments(meshSet->GetCurveError() / 4);
        meshSet->AddTessellatedShape(*cs, pfillStyles, fillStylesNum, context);
    }
}

int    GFxShapeBase::GetTexture9GridStyle(const GCompoundShape& cs) const
{
    int styleIdx = -1;
    if ((Flags & Flags_StylesSupport) && cs.GetNumPaths() == 1)
    {
        const GCompoundShape::SPath& path = cs.GetPath(0);
        if ((path.GetLeftStyle() < 0) != (path.GetRightStyle() < 0) && path.GetLineStyle() < 0)
        {
            styleIdx = (path.GetLeftStyle() >= 0) ? path.GetLeftStyle() : path.GetRightStyle();

            UInt fillStylesNum = 0;
            const GFxFillStyle* pfillStyles = GetFillStyles(&fillStylesNum);
            if (styleIdx < (int)fillStylesNum && !pfillStyles[styleIdx].IsClippedImageFill())
                styleIdx = -1;
        }
    }
    return styleIdx;
}


GFxPathAllocator::GFxPathAllocator(UInt pageSize) : 
    pFirstPage(0), pLastPage(0), FreeBytes(0), DefaultPageSize((UInt16)pageSize)
{
}

GFxPathAllocator::~GFxPathAllocator()
{
    Clear();
}

void   GFxPathAllocator::Clear()
{
    for(Page* pcurPage = pFirstPage; pcurPage; )
    {
        Page* pnextPage = pcurPage->pNext;
        GFREE(pcurPage);
        pcurPage = pnextPage;
    }

     pFirstPage = 0;
     pLastPage  = 0;
     FreeBytes  = 0;
}

// edgesDataSize - size of geometrical data including the optional edge count, edge infos and vertices.
// Can be 0 if new shape.
// pathSize - 0 - new shape, 1 - 8-bit, 2 - 16-bit (aligned), 4 - 32-bit (aligned) 
// edgeSize - 2 - 16-bit, 4 - 32-bit
UByte* GFxPathAllocator::AllocPath(UInt edgesDataSize, UInt pathSize, UInt edgeSize)
{
    UInt freeBytes = FreeBytes;
    UInt size = 1 + pathSize*3 + edgesDataSize;
    UInt sizeForCurrentPage = size;

    if (edgesDataSize > 0)
    {
        // calculate the actual size of path taking into account both alignments (for path and for edges)
        UInt delta = 0;
        if (pLastPage)
        {
            UByte* ptr = pLastPage->GetBufferPtr(freeBytes) + 1; // +1 is for first byte (bit flags)

            // The first delta represents alignment for path
            delta = (UInt)(((UPInt)ptr) & (pathSize - 1));
            delta = ((delta + pathSize - 1) & (~(pathSize - 1))) - delta; // aligned delta

            // The second delta (delta2) represents the alignment for edges, taking into account
            // the previous alignment.
            UInt delta2 = (UInt)((UPInt)ptr + delta + pathSize*3) & (edgeSize - 1);
            delta2 = ((delta2 + edgeSize - 1) & (~(edgeSize - 1))) - delta2;

            delta += delta2;
        }
        sizeForCurrentPage += delta;
        if (!pLastPage || FreeBytes < sizeForCurrentPage)
        {
            // new page will be allocated, calculate new delta size
            delta = (1 + (pathSize - 1)) & (~(pathSize - 1));
            delta = ((delta + pathSize*3 + (edgeSize - 1)) & (~(edgeSize - 1))) + edgesDataSize - size;
        }
        size += delta;
    }
    UByte* ptr = AllocMemoryBlock(sizeForCurrentPage, size);
    return ptr;
}

UByte* GFxPathAllocator::AllocRawPath(UInt32 sizeInBytes)
{
    return AllocMemoryBlock(sizeInBytes, sizeInBytes);
}

UByte* GFxPathAllocator::AllocMemoryBlock(UInt32 sizeForCurrentPage, UInt32 sizeInNewPage)
{
    UInt size = sizeInNewPage;
    UInt freeBytes = FreeBytes;
    if (pLastPage == NULL || FreeBytes < sizeForCurrentPage)
    {
        UInt pageSize = DefaultPageSize;
        Page* pnewPage;
        if (size > pageSize)
        {
            pageSize = size;
        }
        freeBytes           = pageSize;
        pnewPage            = (Page*)GHEAP_AUTO_ALLOC(this, sizeof(Page) + pageSize);
                                                      // Was: GFxStatMD_ShapeData_Mem);
#ifdef GFC_BUILD_DEBUG
        memset(pnewPage, 0xba, sizeof(Page) + pageSize);
#endif
        pnewPage->pNext     = NULL;
        pnewPage->PageSize  = pageSize;
        if (pLastPage)
        {
            pLastPage->pNext    = pnewPage;
            // correct page size
            pLastPage->PageSize = pLastPage->PageSize - FreeBytes;
        }
        pLastPage = pnewPage;
        if (!pFirstPage)
            pFirstPage = pnewPage;
    }
    else
    {
        size = sizeForCurrentPage;
    }

    UByte* ptr  = pLastPage->GetBufferPtr(freeBytes);
#ifdef GFC_BUILD_DEBUG
    memset(ptr, 0xcc, size);
#endif
    FreeBytes   = (UInt16)(freeBytes - size);

    return ptr;
}

bool   GFxPathAllocator::ReallocLastBlock(UByte* ptr, UInt32 oldSize, UInt32 newSize)
{
    GASSERT(newSize <= oldSize);
    if (newSize < oldSize && pLastPage && IsInPage(pLastPage, ptr))
    {
        UPInt off = ptr - pLastPage->GetBufferPtr();
        if (pLastPage->PageSize - (off + oldSize) == FreeBytes)
        {
            UPInt newFreeBytes = pLastPage->PageSize - (off + newSize);
            if (newFreeBytes < 65536)
                FreeBytes = UInt16(newFreeBytes);
        }
    }
    return false;
}

void GFxPathAllocator::SetDefaultPageSize(UInt dps)
{
    if (pFirstPage == NULL)
    {
        GASSERT(dps < 65536);
        DefaultPageSize = (UInt16)dps;
    }
}

GFxPathPacker::GFxPathPacker()
{
    Reset();
}

void GFxPathPacker::Reset()
{
    Fill0 = Fill1 = Line = 0;
    Ax = Ay = Ex = Ey = 0; // moveTo
    EdgesIndex = 0;
    CurvesNum = 0;
    LinesNum = 0;
    EdgesNumBits = 0;
    NewShape = 0;
}

static GINLINE UInt GFx_BitsToUInt(SInt value)
{
    //return (value >= 0) ? (UInt)value : (((UInt)G_Abs(value))<<1);
    // always need to add one bit for a sign, even if value is positive,
    // since this value will be assigned to signed int (8, 16 or 32 bits)
    return (value == 0) ? 0 : (((UInt)G_Abs(value))<<1);
}

static GINLINE UByte GFx_BitCountSInt(SInt value)
{
    return GBitsUtil::BitCount32(GFx_BitsToUInt(value));
}

static GINLINE UByte GFx_BitCountSInt(SInt value1, SInt value2)
{
    return GBitsUtil::BitCount32(GFx_BitsToUInt(value1) | GFx_BitsToUInt(value2));
}

static GINLINE UByte GFx_BitCountSInt(SInt value1, SInt value2, SInt value3, SInt value4)
{
    return GBitsUtil::BitCount32(GFx_BitsToUInt(value1) | GFx_BitsToUInt(value2) | 
                                 GFx_BitsToUInt(value3) | GFx_BitsToUInt(value4));
}

template <UPInt alignSize, typename T>
GINLINE static T GFx_AlignedPtr(T ptr)
{
    return (T)(UPInt(ptr + alignSize - 1) & (~(alignSize - 1)));
}

template <typename T>
GINLINE static T GFx_AlignedPtr(T ptr, UPInt alignSize)
{
    return (T)(UPInt(ptr + alignSize - 1) & (~(alignSize - 1)));
}

void GFxPathPacker::Pack(GFxPathAllocator* pallocator, GFxPathData::PathsInfo* ppathsInfo)
{
    /* There are several types of packed paths:
       1) Path8
       2) Path16
       3) Path32
       4) NewShape
       5) Path8 + up to 4 16-bit edges.

       Octet 0 represents bit flags:
            Bit 0:      0 - complex type (path types 1 - 4)
                        1 - path type 5 (Path8 + up to 4 16-bit edges)
         For complex type (if bit 0 is 0):
            Bits 1-2:   type:
                        0 - NewShape
                        1 - path 8
                        2 - path 16
                        3 - path 32
            Bit 3:      edge size:
                        0 - 16 bit
                        1 - 32 bit
            Bits 4-7:   edge count:
                        0 - edge count is stored separately
                        1 - 15 - actual edge count (no additional counter is stored)
        For path type 5:
            Bits 1-2:   edge count
                        0 - 1
                        1 - 2
                        2 - 3
                        3 - 4 edges
            Bits 3-6:   edge types mask, bit set to 1 indicates curve, otherwise - line.
            Bit 7:      spare.
        Flags' octet is not aligned.
        The following part is aligned accrodingly to path type: path8 - 8-bit alignment, path16 - 16-bit, path32 - 32-bit 
        (aligned accordingly);
        NewShape consists of only one octet.
        Octet 1/Word 1/Dword 1 - Fill0
        Octet 2/Word 2/Dword 2 - Fill1
        Octet 3/Word 3/Dword 3 - Line
        
        Edges aligned accordingly to bit 3 ("edge size").
        Edges are encoded as follows: optional edge count, "moveto" starting point, edge info, edge coordinates, edge info, edge coords, etc.
        Edge count is encoded for path types 1 - 3, if it is greater than 15 (see bits 4-7 for complex type path).
        The size of count depends on "edge size" bit - 16 or 32-bit.

        "moveto" starting point is encoded as set of 2 16 or 32-bit integers.

        Edge info represented by bit array encoded as 16 or 32-bit integer (depending on "edge size" bit).
        Each bit indicates the type of edge - 1 - curve, 0 - line.

        Line edges are encoded as set of 2 16- or 32-bit integers (depending on "edge size" bit).
        Curve edges are encoded as set of 4 16- or 32-bit integers (depending on "edge size" bit).

        If there are more than 16 (32) edges then another edge info bit array is encoded for the next 16 (32) edges 
        followed by encoded edges.
    */
    GASSERT((Fill0 & 0x80000000) == 0); // prevent from "negative" styles
    GASSERT((Fill1 & 0x80000000) == 0); // prevent from "negative" styles
    GASSERT((Line  & 0x80000000) == 0); // prevent from "negative" styles

    UInt fillNumBits = GBitsUtil::BitCount32(Fill0 | Fill1 | Line);
    UByte* pbuf = 0;

    if (NewShape)
    {
        pbuf = pallocator->AllocPath(0, 0, 0);
        *pbuf = 0;
    }
    else if (fillNumBits <= 8 && EdgesNumBits <= 16 && EdgesIndex <= 4)
    {
        // special case, Path8 and up to 4 16-bits edges
        // encode flags
        UByte flags = 0;
        flags |= GFxPathConsts::PathTypeMask;
        flags |= ((EdgesIndex - 1) << GFxPathConsts::Path8EdgesCountShift) & GFxPathConsts::Path8EdgesCountMask;
        UByte edgesTypes = 0;
        pbuf = pallocator->AllocPath((LinesNum + 1)*2*2 + CurvesNum*4*2, 1, 2);
        pbuf[1] = (UByte)Fill0;
        pbuf[2] = (UByte)Fill1;
        pbuf[3] = (UByte)Line;
        
        SInt16* pbuf16 = (SInt16*)(GFx_AlignedPtr<2>(pbuf + 4));

        *pbuf16++ = (SInt16)Ax; // move to
        *pbuf16++ = (SInt16)Ay;

        UByte i, mask = 0x01;
        for(i = 0; i < EdgesIndex; ++i, mask <<= 1)
        {
            if (Edges[i].IsCurve())
            {
                *pbuf16++ = (SInt16)Edges[i].Cx;
                *pbuf16++ = (SInt16)Edges[i].Cy;
                edgesTypes |= mask;
            }
            *pbuf16++ = (SInt16)Edges[i].Ax;
            *pbuf16++ = (SInt16)Edges[i].Ay;
        }
        flags |= (edgesTypes << GFxPathConsts::Path8EdgeTypesShift) & GFxPathConsts::Path8EdgeTypesMask;
        pbuf[0] = flags;
    }
    else
    {  
        UByte flags = 0;
        UInt pathSizeFactor = 1;
        if (fillNumBits <= 8)
            flags |= (1 << GFxPathConsts::PathSizeShift);
        else if (fillNumBits <= 16)
        {
            flags |= (2 << GFxPathConsts::PathSizeShift);
            pathSizeFactor = 2;
        }
        else
        {   // 32-bit path
            flags |= (3 << GFxPathConsts::PathSizeShift);
            pathSizeFactor = 4;
        }
        UInt bytesToAllocate = 0;
        // encode edge count
        UInt edgesSizeFactor;
        if (EdgesNumBits > 16 || EdgesIndex > 65535)
        {
            flags |= GFxPathConsts::PathEdgeSizeMask; // 32-bit edges
            edgesSizeFactor = 4;
        }
        else
            edgesSizeFactor = 2;
        if (EdgesIndex < 16)
            flags |= (EdgesIndex & 0xF) << GFxPathConsts::PathEdgesCountShift;
        else
            bytesToAllocate += edgesSizeFactor; // space for size

        // calculate space needed for edgeinfo
        if (EdgesNumBits <= 16)
            bytesToAllocate += (EdgesIndex + 15)/16*2;
        else
            bytesToAllocate += (EdgesIndex + 31)/32*4;
        //
        pbuf = pallocator->AllocPath(bytesToAllocate + (LinesNum + 1)*2*edgesSizeFactor + CurvesNum * 4 *edgesSizeFactor, pathSizeFactor, edgesSizeFactor);
        pbuf[0] = flags;
        UByte* palignedBuf = GFx_AlignedPtr(pbuf + 1, pathSizeFactor);
        switch (pathSizeFactor)
        {
        case 1:  
            palignedBuf[0] = (UByte)Fill0;
            palignedBuf[1] = (UByte)Fill1;
            palignedBuf[2] = (UByte)Line;
            break;
        case 2:
            {
                SInt16* pbuf16 = (SInt16*)palignedBuf;
                pbuf16[0] = (SInt16)Fill0;
                pbuf16[1] = (SInt16)Fill1;
                pbuf16[2] = (SInt16)Line;
                break;
            }
        case 4:
            {
                SInt32* pbuf32 = (SInt32*)palignedBuf;
                pbuf32[0] = (SInt32)Fill0;
                pbuf32[1] = (SInt32)Fill1;
                pbuf32[2] = (SInt32)Line;
                break;
            }
        }
        UInt bufIndex = 3 * pathSizeFactor;
        if (edgesSizeFactor == 2)
        {
            SInt16* pbuf16 = (SInt16*)(GFx_AlignedPtr(palignedBuf + bufIndex, edgesSizeFactor));
            if (EdgesIndex >= 16)
                *pbuf16++ = (SInt16)EdgesIndex; // edge count

            *pbuf16++ = (SInt16)Ax; // moveto
            *pbuf16++ = (SInt16)Ay;

            SInt16* pedgeInfo = 0;
            UInt i, mask = 0;
            for(i = 0; i < EdgesIndex; ++i, mask <<= 1)
            {
                if (i % 16 == 0)
                {
                    // edge info
                    pedgeInfo = pbuf16++;
                    *pedgeInfo = 0;
                    mask = 1;
                }
                if (Edges[i].IsCurve())
                {
                    *pbuf16++ = (SInt16)Edges[i].Cx;
                    *pbuf16++ = (SInt16)Edges[i].Cy;
                    *pedgeInfo |= mask;
                }
                *pbuf16++ = (SInt16)Edges[i].Ax;
                *pbuf16++ = (SInt16)Edges[i].Ay;
            }
        }
        else
        {
            SInt32* pbuf32 = (SInt32*)(GFx_AlignedPtr(palignedBuf + bufIndex, edgesSizeFactor));
            if (EdgesIndex >= 16)
                *pbuf32++ = (SInt32)EdgesIndex;

            *pbuf32++ = (SInt32)Ax; // moveto
            *pbuf32++ = (SInt32)Ay;

            SInt32* pedgeInfo = 0;
            UInt i, mask = 0;
            for(i = 0; i < EdgesIndex; ++i, mask <<= 1)
            {
                if (i % 32 == 0)
                {
                    // edge info
                    pedgeInfo = pbuf32++;
                    *pedgeInfo = 0;
                    mask = 1;
                }
                if (Edges[i].IsCurve())
                {
                    *pbuf32++ = (SInt32)Edges[i].Cx;
                    *pbuf32++ = (SInt32)Edges[i].Cy;
                    *pedgeInfo |= mask;
                }
                *pbuf32++ = (SInt32)Edges[i].Ax;
                *pbuf32++ = (SInt32)Edges[i].Ay;
            }
        }
    }
    if (pbuf && ppathsInfo)
    {
        if (ppathsInfo->pPaths == NULL)
        {
            ppathsInfo->PathsPageOffset = pallocator->GetPageOffset(pbuf);
            ppathsInfo->pPaths = pbuf;
        }
        if (NewShape)
            ++ppathsInfo->ShapesCount;
        ++ppathsInfo->PathsCount;
    }
    ResetEdges();
}

void GFxPathPacker::SetMoveTo(SInt x, SInt y, UInt numBits)
{
    Ax = Ex = x;
    Ay = Ey = y;
    if (numBits > EdgesNumBits)
        EdgesNumBits = (UByte)numBits;
    else if (numBits == 0)
        EdgesNumBits = G_Max(GFx_BitCountSInt(x, y), EdgesNumBits);
}

void GFxPathPacker::AddLineTo(SInt x, SInt y, UInt numBits)
{
    Edge e(x, y);
    if (EdgesIndex < Edges.GetSize())
        Edges[EdgesIndex] = e;
    else
        Edges.PushBack(e);
    ++EdgesIndex;
    ++LinesNum;
    if (numBits > EdgesNumBits)
        EdgesNumBits = (UByte)numBits;
    else if (numBits == 0)
        EdgesNumBits = G_Max(GFx_BitCountSInt(x, y), EdgesNumBits);
    Ex += x;
    Ey += y;
}

void GFxPathPacker::AddCurve (SInt cx, SInt cy, SInt ax, SInt ay, UInt numBits)
{
    Edge e(cx, cy, ax, ay);
    if (EdgesIndex < Edges.GetSize())
        Edges[EdgesIndex] = e;
    else
        Edges.PushBack(e);
    ++EdgesIndex;
    ++CurvesNum;
    if (numBits > EdgesNumBits)
        EdgesNumBits = (UByte)numBits;
    else if (numBits == 0)
        EdgesNumBits = G_Max(GFx_BitCountSInt(cx, cy, ax, ay), EdgesNumBits);
    Ex += cx + ax;
    Ey += cy + ay;
}



void GFxPathPacker::LineToAbs(SInt x, SInt y)
{
    AddLineTo(x - Ex, y - Ey, 0);
}

void GFxPathPacker::CurveToAbs(SInt cx, SInt cy, SInt ax, SInt ay)
{
    AddCurve(cx - Ex, cy - Ey, ax - cx, ay - cy, 0);
}

void GFxPathPacker::ClosePath() 
{ 
    if(Ex != Ax || Ey != Ay)
    {
        LineToAbs(Ax, Ay); 
    }
}

//////////////////////////////////////////////////////////////////////////
// GFxConstShapeNoStyles
//
bool    GFxConstShapeNoStyles::DefPointTestLocal(const GPointF &pt, 
                                                 bool testShape, 
                                                 const GFxCharacter* pinst) const
{
    return DefPointTestLocalImpl<GFxSwfPathData>(pt, testShape, pinst);
}

// Push our shape data through the tessellator.
void    GFxConstShapeNoStyles::Tessellate(GFxMeshSet *meshSet, Float tolerance,
                                          GFxDisplayContext &context,
                                          GFxScale9GridInfo* s9g) const
{
    TessellateImpl<GFxSwfPathData>(meshSet, tolerance, context, s9g);
}


// Convert the paths to GCompoundShape, flattening the curves.
// The function ignores the NewShape flag and adds all paths
// GFxShapeBase contains.
void    GFxConstShapeNoStyles::MakeCompoundShape(GCompoundShape *cs, Float tolerance) const
{
    MakeCompoundShapeImpl<GFxSwfPathData>(cs, tolerance);
}


UInt32 GFxConstShapeNoStyles::ComputeGeometryHash() const
{
    // just compute hash of piece of memory
    UInt memsz = GFxSwfPathData::GetMemorySize(pPaths);
    return GFx_ComputeBernsteinHash(pPaths, memsz, 0);
}

bool    GFxConstShapeNoStyles::IsEqualGeometry(const GFxShapeBase& cmpWith) const
{
    if (cmpWith.GetPathDataType() == GFxConstShapeNoStyles::GetPathDataType())
    {
        // just compare two memory blocks
        const GFxConstShapeNoStyles* cmpWithShape = 
            static_cast<const GFxConstShapeNoStyles*>(&cmpWith);
        UInt memsz          = GFxSwfPathData::GetMemorySize(pPaths);
        UInt cmpWithMemsz   = GFxSwfPathData::GetMemorySize(cmpWithShape->pPaths);
        if (memsz != cmpWithMemsz)
            return false;
        bool res = (memcmp(pPaths, cmpWithShape->pPaths, memsz) == 0);
        GASSERT(res == IsEqualGeometryImpl<GFxSwfPathData>(cmpWith));
        return res;
    }
    return IsEqualGeometryImpl<GFxSwfPathData>(cmpWith);
}



// Find the bounds of this shape, and store them in
// the given rectangle.
void    GFxConstShapeNoStyles::ComputeBound(GRectF* r) const
{
    ComputeBoundImpl<GFxSwfPathData>(r);
}

void    GFxConstShapeNoStyles::Read(GFxLoadProcess* p, GFxTagType tagType, UInt lenInBytes, bool withStyle, GFxPathAllocator* pAllocator)
{
    Read(p, tagType, lenInBytes, withStyle, NULL, NULL,pAllocator);
}

static UByte* GFx_WriteUInt(UByte* p, UInt value, UInt sz)
{
    UByte* dp = p;
    for (UInt i = 0, shift = 0; i < sz; ++i, shift += 8)
    {
        *dp++ = UByte((value >> shift) & 0xFF);
    }
    return dp;
}

static UInt GFx_ReadUInt(const UByte* p, UInt sz)
{
    UInt res = 0;
    for (UInt i = 0, shift = 0; i < sz; ++i, shift += 8)
    {
        res |= ((UInt)p[i]) << shift;
    }
    return res;
}

void    GFxConstShapeNoStyles::Read(GFxLoadProcess* p, GFxTagType tagType, UInt lenInBytes, bool withStyle, 
                                    GFxFillStyleArrayTemp* pfillStyles, GFxLineStyleArrayTemp* plineStyles, GFxPathAllocator* pAllocator)
{
    GFxPathAllocator* ppathAllocator = pAllocator? pAllocator : p->GetPathAllocator();
    GFxStream*  in = p->GetStream();
    GASSERT(ppathAllocator);

    int stylesLen;
    if (withStyle)
    {
        int curOffset = in->Tell();

        SetValidBoundsFlag(true);
        in->ReadRect(&Bound);

        // SWF 8 contains bounds without a stroke: MovieClip.getRect()
        if ((tagType == GFxTag_DefineShape4) || (tagType == GFxTag_DefineFont3))
        {
            GRectF rectBound;
            in->ReadRect(&rectBound);
            SetRectBoundsLocal(rectBound);
            // MA: What does this byte do?
            // I've seen it take on values of 0x01 and 0x00
            //UByte mysteryByte =
            in->ReadU8();
        }
        else
            SetRectBoundsLocal(Bound);

        GFx_ReadFillStyles(pfillStyles, p, tagType);
        GFx_ReadLineStyles(plineStyles, p, tagType);
        stylesLen = in->Tell() - curOffset;
        GASSERT((int)stylesLen >= 0);
        GASSERT((int)(lenInBytes - stylesLen) >= 0);
    }
    else
        stylesLen = 0;

#ifndef GFX_USE_CUSTOM_PACKER
    // Format of shape data is as follows:
    // 1 byte - PathFlags (see GFxSwfPathData::Flags_<>)
    // 4 bytes - signature (in DEBUG build only)
    // N bytes [1..4] - size of whole shape data memory block (N depends on Mask_NumBytesInMemCnt)
    // X bytes - original SWF shape data
    // Y bytes [1..4] - number of shapes in swf shape data (Y depends on Mask_NumBytesInGeomCnt)
    // Z bytes [1..4] - number of paths in swf shape data (Z depends on Mask_NumBytesInGeomCnt)

    UInt shapeLen = (UInt)lenInBytes - stylesLen;
    UInt memBlockSize = shapeLen;
    UInt originalShapeLen = shapeLen;
    memBlockSize += 1; // mandatory 1 extra byte with flags (see GFxSwfPathData::Flags enum)
    UByte pathFlags = (UByte)
        ((tagType > GFxTag_DefineShape) ? GFxSwfPathData::Flags_HasExtendedFillNum : 0);
    if (withStyle)
    {
        if (pfillStyles && pfillStyles->GetSize() > 0)
        {
            pathFlags |= GFxSwfPathData::Flags_HasFillStyles;
            memBlockSize += 2;
        }
        if (plineStyles && plineStyles->GetSize() > 0)
        {
            pathFlags |= GFxSwfPathData::Flags_HasLineStyles;
            memBlockSize += 2;
        }
    }
#ifdef GFC_BUILD_DEBUG
    // put a 4-bytes signature at the beginning for debugging purposes
    // to catch improper iterator usage.
    memBlockSize += 4;
#endif

    // we need to reserve space for shapes & paths counter
    // these counter might be 1, 2, 3 or 4 bytes length
    // depending on their size. PathFlags should contain 
    // appropriate bits set (see Mask_NumBytesInGeomCnt).
    // At the beginning we will reserve maximum - 4 bytes per each
    // and will correct the final size at the end of read.
    // These counters will be stored at the end of whole memory block.
    // To access them it would be necessary to decode memory counter
    // first and add it the pointer and subtract two sizes of geometry
    // counters.
    UInt geomCntInBytes = 4;
    memBlockSize += geomCntInBytes * 2;

    // we need to reserve space for memory counter
    // memory counter might be 1, 2, 3 or 4 bytes length
    // depending on its size. PathFlags should contain 
    // appropriate bits set (see Mask_NumBytesInMemCnt).
    UInt memBlockSizeInBytes;
    if (memBlockSize < (1U<<8))
        memBlockSizeInBytes = 1;
    else if (memBlockSize < (1U<<16))
        memBlockSizeInBytes = 2;
    else if (memBlockSize < (1U<<24))
        memBlockSizeInBytes = 3;
    else
        memBlockSizeInBytes = 4;
    memBlockSize += memBlockSizeInBytes;
    pathFlags |= ((memBlockSizeInBytes - 1) << GFxSwfPathData::Shift_NumBytesInMemCnt) & 
        GFxSwfPathData::Mask_NumBytesInMemCnt;

    UByte *pmemBlock = ppathAllocator->AllocRawPath(memBlockSize);
    UByte *ptr = pmemBlock;

#ifdef GFC_BUILD_DEBUG
    // put a 4-bytes signature at the beginning for debugging purposes
    // to catch improper iterator usage. Put it in little endian format
    // like all swf's integers.
    *ptr++ = GFxSwfPathData::Signature & 0xFF;
    *ptr++ = (GFxSwfPathData::Signature >> 8) & 0xFF;
    *ptr++ = (GFxSwfPathData::Signature >> 16) & 0xFF;
    *ptr++ = (GFxSwfPathData::Signature >> 24) & 0xFF;
#endif

    *ptr++ = pathFlags;
    
    // actual value of memory size will be written at the end of read
    // after all corrections are done.
    ptr += memBlockSizeInBytes;

    if (pathFlags & GFxSwfPathData::Flags_HasFillStyles)
    {
        UPInt fn = pfillStyles->GetSize();
        GASSERT(fn <= 65535);
        *ptr++ = UInt8(fn & 0xFF);
        *ptr++ = UInt8((fn >> 8) & 0x7F);
    }
    if (pathFlags & GFxSwfPathData::Flags_HasLineStyles)
    {
        UPInt fn = plineStyles->GetSize();
        GASSERT(fn <= 65535);
        *ptr++ = UInt8(fn & 0xFF);
        *ptr++ = UInt8((fn >> 8) & 0x7F);
    }

    in->Align();
    in->ReadToBuffer(ptr, shapeLen);

    // Multiplier used for scaling.
    // In SWF 8, font shape resolution (tag 75) was increased by 20.
    Float sfactor = 1.0f;
    if (tagType == GFxTag_DefineFont3)
    {
        sfactor = 1.0f / 20.0f;
        Flags |= Flags_Sfactor20;
    }

    GFxStream* poriginalStream = in;
    GFxStream ss(ptr, shapeLen, p->GetLoadHeap(), in->GetLog(), in->GetParseControl());
    in = &ss;
    p->SetAltStream(in);

    // need to preprocess shape to parse and remove all additional fill & line styles.
    //
    // SHAPE
    //
    in->Align(); //!AB
    int numFillBits = in->ReadUInt(4);
    int numLineBits = in->ReadUInt(4);

    if (withStyle) // do not trace, if !withStyle (for example, for font glyphs)
        in->LogParse("  ShapeCharacter read: nfillbits = %d, nlinebits = %d\n", numFillBits, numLineBits);

    // These are state variables that keep the
    // current position & style of the shape
    // outline, and vary as we read the GFxEdge data.
    //
    // At the moment we just store each GFxEdge with
    // the full necessary info to render it, which
    // is simple but not optimally efficient.
    int fillBase = 0;
    int lineBase = 0;
    int moveX = 0, moveY = 0;
    SInt numMoveBits = 0;
    UInt totalPathsCount = 0;
    UInt totalShapesCount = 0;
    int curEdgesCount = 0;
    int curPathsCount = 0;
    bool shapeIsCorrupted = false;

#define SHAPE_LOG 0
#ifdef GFX_SHAPE_MEM_TRACKING
    int __startingFileOffset = in->Tell();
#endif //GFX_SHAPE_MEM_TRACKING
    // SHAPERECORDS
    while(!shapeIsCorrupted) 
    {
        int typeFlag = in->ReadUInt1();
        if (typeFlag == 0)
        {
            // Parse the record.
            int flags = in->ReadUInt(5);
            if (flags == 0) {
                // End of shape records.
                if (curEdgesCount > 0)
                {
                    ++totalPathsCount;
                    curEdgesCount = 0;
                    ++curPathsCount;
                }
                break;
            }
            if (flags & 0x01)
            {
                // MoveTo = 1;
                if (curEdgesCount > 0)
                {
                    ++totalPathsCount;
                    curEdgesCount = 0;
                    ++curPathsCount;
                }

                numMoveBits = in->ReadUInt(5);
                moveX = in->ReadSInt(numMoveBits);
                moveY = in->ReadSInt(numMoveBits);

                if (in->IsVerboseParseShape())
                    in->LogParseShape("  ShapeCharacter read: moveto %4g %4g\n", (Float) moveX * sfactor, (Float) moveY * sfactor);
            }
            if ((flags & 0x02) && numFillBits > 0)
            {
                // FillStyle0_change = 1;
                if (curEdgesCount > 0)
                {
                    ++totalPathsCount;
                    curEdgesCount = 0;
                    ++curPathsCount;
                }

                int style = in->ReadUInt(numFillBits);
                if (in->IsVerboseParseShape())
                {
                    if (style > 0)
                    {
                        style += fillBase;
                    }
                    in->LogParseShape("  ShapeCharacter read: fill0 = %d\n", style);
                }
            }
            if ((flags & 0x04) && numFillBits > 0)
            {
                // FillStyle1_change = 1;
                if (curEdgesCount > 0)
                {
                    ++totalPathsCount;
                    curEdgesCount = 0;
                    ++curPathsCount;
                }

                int style = in->ReadUInt(numFillBits);
                if (in->IsVerboseParseShape())
                {
                    if (style > 0)
                    {
                        style += fillBase;
                    }
                    in->LogParseShape("  ShapeCharacter read: fill1 = %d\n", style);
                }
            }
            if ((flags & 0x08) && numLineBits > 0)
            {
                // LineStyleChange = 1;
                if (curEdgesCount > 0)
                {
                    ++totalPathsCount;
                    curEdgesCount = 0;
                    ++curPathsCount;
                }

                int style = in->ReadUInt(numLineBits);
                if (style > 0)
                {
                    style += lineBase;
                }

                if (in->IsVerboseParseShape())
                    in->LogParseShape("  ShapeCharacter read: line = %d\n", style);
            }
            if (flags & 0x10)
            {
                GASSERT(tagType >= 22);

                in->LogParse("  ShapeCharacter read: more fill styles\n");

                if (curPathsCount > 0)
                {
                    ++totalShapesCount;
                    curPathsCount = 0;
                }
                if (curEdgesCount > 0)
                {
                    ++totalPathsCount;
                    curEdgesCount = 0;
                    ++curPathsCount;
                }

#ifdef GFX_SHAPE_MEM_TRACKING
                int __offset = in->Tell();
#endif
                fillBase = pfillStyles ? (int)pfillStyles->GetSize() : 0;
                lineBase = plineStyles ? (int)plineStyles->GetSize() : 0;

                int foff = GFx_ReadFillStyles(pfillStyles, p, tagType);
                int moff = in->Tell();
                int loff = GFx_ReadLineStyles(plineStyles, p, tagType);
                int eoff = in->Tell();

                if (moff != foff)
                {
                    if (foff > moff || UInt(moff) > originalShapeLen)
                    {
                        shapeIsCorrupted = true;
                        break;
                    }
                    else
                    {
                        // collapse array of fill styles and correct all offsets
                        memmove(ptr + foff, ptr + moff, loff - moff);
                        loff -= moff - foff;
                        moff = foff;
                    }
                }
                if (loff != eoff)
                {
                    if (loff > eoff || UInt(eoff) > originalShapeLen)
                    {
                        shapeIsCorrupted = true;
                        break;
                    }
                    else
                    {
                        // collapse array of lines
                        memmove(ptr + loff, ptr + eoff, shapeLen - eoff);
                        shapeLen -= eoff - loff;
                    }
                }
                in->SetPosition(loff); // do we need to set new DataSize in the stream? (!AB)

                numFillBits = in->ReadUInt(4);
                numLineBits = in->ReadUInt(4);
#ifdef GFX_SHAPE_MEM_TRACKING
                __startingFileOffset -= (in->Tell() - __offset);
#endif
            }
        }
        else
        {
            // EDGERECORD
            int edgeFlag = in->ReadUInt1();
            ++curEdgesCount;
            if (edgeFlag == 0)
            {
                // curved GFxEdge
                UInt numbits = 2 + in->ReadUInt(4);
                SInt cx = in->ReadSInt(numbits);
                SInt cy = in->ReadSInt(numbits);
                SInt ax = in->ReadSInt(numbits);
                SInt ay = in->ReadSInt(numbits);

                if (in->IsVerboseParseShape())
                {
                    in->LogParseShape("  ShapeCharacter read: curved edge   = %4g %4g - %4g %4g - %4g %4g\n", 
                        (Float) moveX * sfactor, (Float) moveY * sfactor, 
                        (Float) (moveX + cx) * sfactor, (Float) (moveY + cy) * sfactor, 
                        (Float) (moveX + cx + ax ) * sfactor, (Float) (moveY + cy + ay ) * sfactor);
                }
                moveX += ax + cx;
                moveY += ay + cy;
            }
            else
            {
                // straight GFxEdge
                UInt numbits = 2 + in->ReadUInt(4);
                int lineFlag = in->ReadUInt1();
                SInt dx = 0, dy = 0;
                if (lineFlag)
                {
                    // General line.
                    dx = in->ReadSInt(numbits);
                    dy = in->ReadSInt(numbits);
                }
                else
                {
                    int vertFlag = in->ReadUInt1();
                    if (vertFlag == 0) {
                        // Horizontal line.
                        dx = in->ReadSInt(numbits);
                    } else {
                        // Vertical line.
                        dy = in->ReadSInt(numbits);
                    }
                }

                if (in->IsVerboseParseShape())
                {
                    in->LogParseShape("  ShapeCharacter read: straight edge = %4g %4g - %4g %4g\n", 
                        (Float)moveX * sfactor, (Float)moveY * sfactor, 
                        (Float)(moveX + dx) * sfactor, (Float)(moveY + dy) * sfactor);
                }
                moveX += dx;
                moveY += dy;
            }
        }
        // detect if shape record is corrupted
        shapeIsCorrupted = ((UInt)in->Tell() > originalShapeLen);
    }
    if (!shapeIsCorrupted)
    {
        if (curPathsCount > 0)
            ++totalShapesCount;

        // make correction for new shape length (if built-in fill & line styles
        // were removed).
        UInt newMemBlockSize = memBlockSize - (originalShapeLen - shapeLen);
        
        // now we need to calculate how much memory would be necessary
        // to store totalShapeCount & totalPathCount
        UInt maxGeomCnt = G_Max(totalShapesCount, totalPathsCount);
        if (maxGeomCnt < (1U<<8))
            geomCntInBytes = 1;
        else if (maxGeomCnt < (1U<<16))
            geomCntInBytes = 2;
        else if (maxGeomCnt < (1U<<24))
            geomCntInBytes = 3;
        else
            geomCntInBytes = 4;

        // correct allocated memory once again
        newMemBlockSize = newMemBlockSize - 2*4 + 2*geomCntInBytes;
        UByte* pgeomCnt = pmemBlock + newMemBlockSize - 2*geomCntInBytes;
        pgeomCnt = GFx_WriteUInt(pgeomCnt, totalShapesCount, geomCntInBytes);
        pgeomCnt = GFx_WriteUInt(pgeomCnt, totalPathsCount, geomCntInBytes);

        if (memBlockSize > newMemBlockSize)
        {
            ppathAllocator->ReallocLastBlock(pmemBlock, memBlockSize, newMemBlockSize); 
        }
        // now we can put correct memory size
        UInt offset = 0; // path flags
#ifdef GFC_BUILD_DEBUG
        // erase tail of reallocated memory block to make sure data is not used
        if (memBlockSize > newMemBlockSize)
            memset(pmemBlock + newMemBlockSize, 0xFB, memBlockSize - newMemBlockSize);
        offset += 4; // signature
#endif
        // update PathFlags to reflect correct size of shape&path counters
        pmemBlock[offset++] |= ((geomCntInBytes - 1) << GFxSwfPathData::Shift_NumBytesInGeomCnt) & 
                                GFxSwfPathData::Mask_NumBytesInGeomCnt;

        GASSERT(newMemBlockSize != 0);
        GFx_WriteUInt(pmemBlock + offset, newMemBlockSize, memBlockSizeInBytes);

        // just make sure all values are stored correctly
        GASSERT(newMemBlockSize == GFxSwfPathData::GetMemorySize(pmemBlock));
#ifdef GFC_BUILD_DEBUG
        { 
        UInt sc = 0, pc = 0;
        GFxSwfPathData::GetShapeAndPathCounts(pmemBlock, &sc, &pc);
        GASSERT(totalShapesCount == sc && totalPathsCount == pc);
        }
#endif
    }
    else
    {
        // corrupted shape; just make an empty shape and display warning message.
        in->LogWarning("Error: Corrupted shape detected in file %s\n", 
            poriginalStream->GetFileName().ToCStr());

        UInt offset = 0;

        // pathFlags, memsz, numbits fill and line (1 b), end-of-shape record (1 b), shapes cnt, paths cnt
        UInt newMemBlockSize = 1 + 1 + 1 + 1 + 1 + 1; 
#ifdef GFC_BUILD_DEBUG
        newMemBlockSize += 4; // signature
        offset += 4;
#endif
        pmemBlock[offset++] = 0; // pathFlags, just set to 0 
        pmemBlock[offset++] = 0; // mem size
        pmemBlock[offset++] = 0; // num bits in fill & line styles
        pmemBlock[offset++] = 0; // end-of-shape record
        pmemBlock[offset++] = 0; // shapes cnt
        pmemBlock[offset++] = 0; // paths cnt
        if (memBlockSize > newMemBlockSize)
        {
            ppathAllocator->ReallocLastBlock(pmemBlock, memBlockSize, newMemBlockSize); 
        }
    }
    pPaths = pmemBlock;
#ifdef GFC_BUILD_DEBUG
    {
    // Make sure signature is still correct
    // Read a 4-bytes signature to ensure we use correct path format.
    UInt32 v = (UInt32(pmemBlock[0])      | (UInt32(pmemBlock[1])<<8) |
               (UInt32(pmemBlock[2])<<16) | (UInt32(pmemBlock[3])<<24));
    GUNUSED(v);
    GASSERT(v == GFxSwfPathData::Signature);
    }
#endif

    p->SetAltStream(NULL);
#else // GFX_USE_CUSTOM_PACKER
    //
    // SHAPE
    //
    in->Align(); //!AB
    int NumFillBits = in->ReadUInt(4);
    int NumLineBits = in->ReadUInt(4);

    if (withStyle) // do not trace, if !withStyle (for example, for font glyphs)
        in->LogParse("  ShapeCharacter read: nfillbits = %d, nlinebits = %d\n", NumFillBits, NumLineBits);

    // These are state variables that keep the
    // current position & style of the shape
    // outline, and vary as we read the GFxEdge data.
    //
    // At the moment we just store each GFxEdge with
    // the full necessary info to render it, which
    // is simple but not optimally efficient.
    int FillBase = 0;
    int LineBase = 0;
    int moveX = 0, moveY = 0;
    SInt numMoveBits = 0;
    GFxPathPacker CurrentPath;

#define SHAPE_LOG 0
#ifdef GFX_SHAPE_MEM_TRACKING
    int __startingFileOffset = in->Tell();
#endif //GFX_SHAPE_MEM_TRACKING
    // SHAPERECORDS
    for (;;) {
        int TypeFlag = in->ReadUInt1();
        if (TypeFlag == 0)
        {
            // Parse the record.
            int flags = in->ReadUInt(5);
            if (flags == 0) {
                // End of shape records.

                // Store the current GFxPath if any.
                if (! CurrentPath.IsEmpty())
                {
                    CurrentPath.Pack(ppathAllocator, &Paths);
                }

                break;
            }
            if (flags & 0x01)
            {
                // MoveTo = 1;

                // Store the current GFxPath if any, and prepare a fresh one.
                if (! CurrentPath.IsEmpty())
                {
                    CurrentPath.Pack(ppathAllocator, &Paths);
                }

                numMoveBits = in->ReadUInt(5);
                moveX = in->ReadSInt(numMoveBits);
                moveY = in->ReadSInt(numMoveBits);

                // Set the beginning of the GFxPath.
                CurrentPath.SetMoveTo(moveX, moveY, numMoveBits);

                in->LogParseShape("  ShapeCharacter read: moveto %4g %4g\n", (Float) moveX * sfactor, (Float) moveY * sfactor);
            }
            if ((flags & 0x02)
                && NumFillBits > 0)
            {
                // FillStyle0_change = 1;
                if (! CurrentPath.IsEmpty())
                {
                    CurrentPath.Pack(ppathAllocator, &Paths);
                    CurrentPath.SetMoveTo(moveX, moveY, numMoveBits);
                }
                int style = in->ReadUInt(NumFillBits);
                if (style > 0)
                {
                    style += FillBase;
                }
                CurrentPath.SetFill0(style);

                in->LogParseShape("  ShapeCharacter read: fill0 = %d\n", style);
            }
            if ((flags & 0x04)
                && NumFillBits > 0)
            {
                // FillStyle1_change = 1;
                if (! CurrentPath.IsEmpty())
                {
                    CurrentPath.Pack(ppathAllocator, &Paths);
                    CurrentPath.SetMoveTo(moveX, moveY, numMoveBits);
                }
                int style = in->ReadUInt(NumFillBits);
                if (style > 0)
                {
                    style += FillBase;
                }
                CurrentPath.SetFill1(style);

                in->LogParseShape("  ShapeCharacter read: fill1 = %d\n", style);
            }
            if ((flags & 0x08)
                && NumLineBits > 0)
            {
                // LineStyleChange = 1;
                if (! CurrentPath.IsEmpty())
                {
                    CurrentPath.Pack(ppathAllocator, &Paths);
                    CurrentPath.SetMoveTo(moveX, moveY, numMoveBits);
                }
                int style = in->ReadUInt(NumLineBits);
                if (style > 0)
                {
                    style += LineBase;
                }
                CurrentPath.SetLine(style);

                in->LogParseShape("  ShapeCharacter read: line = %d\n", style);
            }
            if (flags & 0x10)
            {
                GASSERT(tagType >= 22);

                in->LogParse("  ShapeCharacter read: more fill styles\n");

                // Store the current GFxPath if any.
                if (! CurrentPath.IsEmpty())
                {
                    CurrentPath.Pack(ppathAllocator, &Paths);
                    // Clear styles.
                    CurrentPath.SetFill0(0);
                    CurrentPath.SetFill1(0);
                    CurrentPath.SetLine(0);
                }
                // Tack on an empty GFxPath signaling a new shape.
                // @@ need better understanding of whether this is correct??!?!!
                // @@ i.E., we should just start a whole new shape here, right?

                CurrentPath.SetNewShape();
                CurrentPath.Pack(ppathAllocator, &Paths);
#ifdef GFX_SHAPE_MEM_TRACKING
                int __offset = in->Tell();
#endif
                FillBase = pfillStyles ? (int)pfillStyles->GetSize() : 0;
                LineBase = plineStyles ? (int)plineStyles->GetSize() : 0;
                GFx_ReadFillStyles(pfillStyles, p, tagType);
                GFx_ReadLineStyles(plineStyles, p, tagType);
                NumFillBits = in->ReadUInt(4);
                NumLineBits = in->ReadUInt(4);
#ifdef GFX_SHAPE_MEM_TRACKING
                __startingFileOffset -= (in->Tell() - __offset);
#endif
            }
        }
        else
        {
            // EDGERECORD
            int EdgeFlag = in->ReadUInt1();
            if (EdgeFlag == 0)
            {
                // curved GFxEdge
                UInt numbits = 2 + in->ReadUInt(4);
                SInt cx = in->ReadSInt(numbits);
                SInt cy = in->ReadSInt(numbits);
                SInt ax = in->ReadSInt(numbits);
                SInt ay = in->ReadSInt(numbits);

                CurrentPath.AddCurve(cx, cy, ax, ay, numbits);

                in->LogParseShape("  ShapeCharacter read: curved edge   = %4g %4g - %4g %4g - %4g %4g\n", 
                    (Float) moveX * sfactor, (Float) moveY * sfactor, 
                    (Float) (moveX + cx) * sfactor, (Float) (moveY + cy) * sfactor, 
                    (Float) (moveX + cx + ax ) * sfactor, (Float) (moveY + cy + ay ) * sfactor);

                moveX += ax + cx;
                moveY += ay + cy;
            }
            else
            {
                // straight GFxEdge
                UInt numbits = 2 + in->ReadUInt(4);
                int LineFlag = in->ReadUInt1();
                //Float   dx = 0, dy = 0;
                SInt dx = 0, dy = 0;
                if (LineFlag)
                {
                    // General line.
                    dx = in->ReadSInt(numbits);
                    dy = in->ReadSInt(numbits);
                }
                else
                {
                    int VertFlag = in->ReadUInt1();
                    if (VertFlag == 0) {
                        // Horizontal line.
                        dx = in->ReadSInt(numbits);
                    } else {
                        // Vertical line.
                        dy = in->ReadSInt(numbits);
                    }
                }

                in->LogParseShape("  ShapeCharacter read: straight edge = %4g %4g - %4g %4g\n", 
                    (Float)moveX * sfactor, (Float)moveY * sfactor, 
                    (Float)(moveX + dx) * sfactor, (Float)(moveY + dy) * sfactor);

                // Must have a half of dx,dy for the curve control point
                // to perform shape tween (morph) correctly.
                CurrentPath.AddLineTo(dx, dy, numbits);

                moveX += dx;
                moveY += dy;
            }
        }
    }
#endif // GFX_USE_CUSTOM_PACKER

#ifdef GFX_SHAPE_MEM_TRACKING
    GFx_MT_SwfGeomMem += (in->Tell() - __startingFileOffset);
    if(withStyle)
    {
        printf("Shapes memory statistics:\n"
            "   ShapesCount = %d, PathsCount = %d, SwfGeomMem = %d, AllocatedMem = %d\n", 
            GFx_MT_ShapesCount, GFx_MT_PathsCount, GFx_MT_SwfGeomMem, GFx_MT_GeomMem);
    }
#endif

    UPInt i, n;
    // Determine if we have any non-solid color fills.
    Flags &= (~Flags_TexturedFill);
    if (pfillStyles)
    {
        for (i = 0, n = pfillStyles->GetSize(); i < n; i++)
        {
            if ((*pfillStyles)[i].GetType() != 0)
            {
                Flags |= Flags_TexturedFill;
                break;
            }
        }
    }
    // Compete maximum stroke extent
    if (Flags & Flags_StylesSupport)
    {
        Float maxStrokeExtent = 1.0f;
        if (plineStyles)
        {
            for(i = 0, n = plineStyles->GetSize(); i < n; i++)
                maxStrokeExtent = G_Max(maxStrokeExtent, (*plineStyles)[i].GetStrokeExtent());
        }
        SetMaxStrokeExtent(maxStrokeExtent);
    }
}

void GFxConstShapeNoStyles::GetShapeAndPathCounts(UInt* pshapesCnt, UInt* ppathsCnt) const
{
    return GFxSwfPathData::GetShapeAndPathCounts(pPaths, pshapesCnt, ppathsCnt);
}

GFxShapeBase::PathsIterator* GFxConstShapeNoStyles::GetPathsIterator() const
{
    GFxVirtualPathIterator<GFxConstShapeNoStyles, GFxSwfPathData>* pswfIter = 
        GHEAP_AUTO_NEW(this) GFxVirtualPathIterator<GFxConstShapeNoStyles, GFxSwfPathData>(this);
    return pswfIter;
}

//////////////////////////////////////////////////////////////////////////
//
// GFxSwfPathData
//
GFxSwfPathData::PathsIterator::PathsIterator(const GFxShapeBase* pdef) :
    pShapeDef(static_cast<const GFxConstShapeNoStyles*>(pdef)),
    Stream(pShapeDef->GetNativePathData()), 
    Record(Rec_None)
{
    if (Stream.pData)
    {
    #ifdef GFC_BUILD_DEBUG
        // Read a 4-bytes signature to ensure we use correct path format.
        UInt32 sig;
        sig = Stream.ReadU32();
        GUNUSED(sig);
        GASSERT(sig == Signature);
    #endif

        PathFlags = Stream.ReadU8();
        if (pdef->GetFlags() & GFxShapeBase::Flags_Sfactor20)
        {
            Sfactor    = 1.0f/20.0f;
            PathFlags |= Flags_HasScaleFactor;
        }
        else
        {
            Sfactor    = 1.0f;
        }

        // need to skip memory counter
        UInt memSizeInBytes = ((PathFlags & GFxSwfPathData::Mask_NumBytesInMemCnt) >> 
                                GFxSwfPathData::Shift_NumBytesInMemCnt) + 1;
        Stream.Skip(memSizeInBytes); // just skip for now

        if (PathFlags & Flags_HasFillStyles)
            CurFillStylesNum = Stream.ReadU16();
        else
            CurFillStylesNum = 0;
        if (PathFlags & Flags_HasLineStyles)
            CurLineStylesNum = Stream.ReadU16();
        else
            CurLineStylesNum = 0;
        FillBase = LineBase = 0;
        NumFillBits = Stream.ReadUInt4();
        NumLineBits = Stream.ReadUInt4();
        MoveX = MoveY = 0;
        CurEdgesCount = 0;
        CurPathsCount = 0;
        Fill0 = Fill1 = Line     = 0;
        PathsCount = ShapesCount = 0;
        ReadNextEdge();
    }
    else
    {
        // empty shape? init by default values
        Record                      = Rec_EndOfShape; // forces IsFinished to ret true
        PathFlags                   = 0;
        Sfactor                     = 1.0f;
        CurFillStylesNum = CurLineStylesNum = 0;
        FillBase = LineBase         = 0;
        NumFillBits = NumLineBits   = 0;
        MoveX = MoveY               = 0;
        CurEdgesCount               = 0;
        CurPathsCount               = 0;
        Fill0 = Fill1 = Line        = 0;
        PathsCount = ShapesCount    = 0;
    }
}

GFxSwfPathData::PathsIterator::PathsIterator(const GFxSwfPathData::PathsIterator& p) :
    Stream(p.Stream), Fill0(p.Fill0), Fill1(p.Fill1), Line(p.Line), Record(p.Record),
    PathsCount(p.PathsCount), ShapesCount(p.ShapesCount), MoveX(p.MoveX), MoveY(p.MoveY),
    FillBase(p.FillBase), LineBase(p.LineBase), CurFillStylesNum(p.CurFillStylesNum),
    CurLineStylesNum(p.CurLineStylesNum), NumFillBits(p.NumFillBits), 
    NumLineBits(p.NumLineBits), CurEdgesCount(p.CurEdgesCount), CurPathsCount(p.CurPathsCount),
    Ax(p.Ax), Ay(p.Ay), Cx(p.Cx), Cy(p.Cy), Dx(p.Dx), Dy(p.Dy),
    Sfactor(p.Sfactor), PathFlags(p.PathFlags)
{}

void GFxSwfPathData::PathsIterator::AddForTessellation(GCompoundShape *cs)
{
    EdgesIterator it = GetEdgesIterator();

    Float ax, ay;
    it.GetMoveXY(&ax, &ay);
    UInt fill0, fill1, line;
    GetStyles(&fill0, &fill1, &line);
    cs->BeginPath(fill0 - 1, fill1 - 1, line - 1, ax, ay);

    EdgesIterator::Edge edge;
    while(!it.IsFinished())
    {
        it.GetEdge(&edge);
        if (edge.Curve)
            cs->AddCurve(edge.Cx, edge.Cy, edge.Ax, edge.Ay);
        else
            cs->AddVertex(edge.Ax, edge.Ay);
    }
    AdvanceBy(it);
}

void GFxSwfPathData::PathsIterator::SkipComplex()
{
    // this call will skip whole path
    EdgesIterator ei(*this); 
    while(!ei.IsFinished())
    {
        GASSERT(Record == PathsIterator::Rec_StraightEdge || 
                Record == PathsIterator::Rec_CurveEdge);
        switch(Record)
        {
        case PathsIterator::Rec_CurveEdge:
            MoveX += Cx + Ax;
            MoveY += Cy + Ay;
            break;
        case PathsIterator::Rec_StraightEdge:
            MoveX += Dx;
            MoveY += Dy;
            break;
        default:;
        }
        ++ei.EdgeIndex;
        ReadNext();
    }
    AdvanceBy(ei);
}

void GFxSwfPathData::PathsIterator::ReadNextEdge()
{
    while(!IsFinished())
    {
        ReadNext();
        if (Record != Rec_NonEdge)
            break;
    }
}

#if 0//defined(GFC_BUILD_DEBUG)
#define LOG_SHAPE0(s)                   printf(s)
#define LOG_SHAPE1(s,p1)                printf(s,p1)
#define LOG_SHAPE2(s,p1,p2)             printf(s,p1,p2)
#define LOG_SHAPE3(s,p1,p2,p3)          printf(s,p1,p2,p3)
#define LOG_SHAPE4(s,p1,p2,p3,p4)       printf(s,p1,p2,p3,p4)
#define LOG_SHAPE5(s,p1,p2,p3,p4,p5)    printf(s,p1,p2,p3,p4,p5)
#define LOG_SHAPE6(s,p1,p2,p3,p4,p5,p6) printf(s,p1,p2,p3,p4,p5,p6)
#else
#define LOG_SHAPE0(s)                   
#define LOG_SHAPE1(s,p1)                
#define LOG_SHAPE2(s,p1,p2)             
#define LOG_SHAPE3(s,p1,p2,p3)          
#define LOG_SHAPE4(s,p1,p2,p3,p4)       
#define LOG_SHAPE5(s,p1,p2,p3,p4,p5)    
#define LOG_SHAPE6(s,p1,p2,p3,p4,p5,p6) 
#endif

void GFxSwfPathData::PathsIterator::ReadNext()
{
   
//    GFxStream* in = &Stream;

    LOG_SHAPE0("SHAPE decode: start\n");

    // These are state variables that keep the
    // current position & style of the shape
    // outline, and vary as we read the GFxEdge data.
    //
    // At the moment we just store each GFxEdge with
    // the full necessary info to render it, which
    // is simple but not optimally efficient.

    // SHAPERECORDS
    do
    {
        bool typeFlag = Stream.ReadUInt1();
        if (!typeFlag)
        {
            // Parse the record.
            int flags = Stream.ReadUInt5();
            if (flags == 0) 
            {
                // End of shape records.
                Record = Rec_EndOfShape;
                LOG_SHAPE0("  SHAPE decode: end-of-shape\n");
                if (CurEdgesCount > 0)
                {
                    ++PathsCount;
                    CurEdgesCount = 0;
                    ++CurPathsCount;
                }
                break;
            }
            if (flags & 0x01)
            {
                // MoveTo = 1;
                Record = Rec_NonEdge;
                if (CurEdgesCount > 0)
                {
                    ++PathsCount;
                    CurEdgesCount = 0;
                    ++CurPathsCount;
                }

                UInt numMoveBits = Stream.ReadUInt5();
                MoveX = Stream.ReadSInt(numMoveBits);
                MoveY = Stream.ReadSInt(numMoveBits);

                LOG_SHAPE3("  SHAPE decode: numbits = %d, moveto %d %d\n", 
                    numMoveBits, MoveX, MoveY);
            }
            if ((flags & 0x02) && NumFillBits > 0)
            {
                Record = Rec_NonEdge;
                // FillStyle0_change = 1;
                if (CurEdgesCount > 0)
                {
                    ++PathsCount;
                    CurEdgesCount = 0;
                    ++CurPathsCount;
                }

                UInt style = Stream.ReadUInt(NumFillBits);
                if (style > 0)
                {
                    style += FillBase;
                }
                Fill0 = style;

                LOG_SHAPE1("  SHAPE decode: fill0 = %d\n", style);
            }
            if ((flags & 0x04) && NumFillBits > 0)
            {
                Record = Rec_NonEdge;
                // FillStyle1_change = 1;
                if (CurEdgesCount > 0)
                {
                    ++PathsCount;
                    CurEdgesCount = 0;
                    ++CurPathsCount;
                }

                int style = Stream.ReadUInt(NumFillBits);
                if (style > 0)
                {
                    style += FillBase;
                }
                Fill1 = style;

                LOG_SHAPE1("  SHAPE decode: fill1 = %d\n", style);
            }
            if ((flags & 0x08) && NumLineBits > 0)
            {
                Record = Rec_NonEdge;
                // LineStyleChange = 1;
                if (CurEdgesCount > 0)
                {
                    ++PathsCount;
                    CurEdgesCount = 0;
                    ++CurPathsCount;
                }

                int style = Stream.ReadUInt(NumLineBits);
                if (style > 0)
                {
                    style += LineBase;
                }
                Line = style;

                LOG_SHAPE1("  SHAPE decode: line = %d\n", style);
            }
            if (flags & 0x10)
            {
                Record = Rec_NewShape;

                if (CurPathsCount > 0)
                {
                    ++ShapesCount;
                    CurPathsCount = 0;
                }
                if (CurEdgesCount > 0)
                {
                    ++PathsCount;
                    CurEdgesCount = 0;
                    ++CurPathsCount;
                }

                Fill0 = Fill1 = Line = 0;

                Stream.Align();
                UInt fsn = Stream.ReadU8();
                if (fsn == 0xFF && PathFlags & Flags_HasExtendedFillNum)
                    fsn = Stream.ReadU16();
                UInt lsn = Stream.ReadU8();
                if (lsn == 0xFF) // ? PathFlags & Flags_HasExtendedFillNum ?
                    lsn = Stream.ReadU16();
                FillBase = CurFillStylesNum;
                LineBase = CurLineStylesNum;
                CurFillStylesNum += fsn;
                CurLineStylesNum += lsn;

                NumFillBits = Stream.ReadUInt4();
                NumLineBits = Stream.ReadUInt4();

                LOG_SHAPE2("  SHAPE decode: more fill styles, numfillbits = %d, numlinebint = %d\n", 
                    NumFillBits, NumLineBits);
            }
        }
        else
        {
            // EDGERECORD
            int edgeFlag = Stream.ReadUInt1();
            ++CurEdgesCount;
            if (edgeFlag == 0)
            {
                // curved GFxEdge
                UInt numbits = 2 + Stream.ReadUInt4();
                Cx = Stream.ReadSInt(numbits);
                Cy = Stream.ReadSInt(numbits);
                Ax = Stream.ReadSInt(numbits);
                Ay = Stream.ReadSInt(numbits);

                Record = Rec_CurveEdge;

                LOG_SHAPE5("  SHAPE decode: curved edge, numbits = %d,  %d %d - %d %d\n", 
                    numbits, Cx, Cy, Ax, Ay);
            }
            else
            {
                // straight GFxEdge
                UInt numbits = 2 + Stream.ReadUInt4();
                int lineFlag = Stream.ReadUInt1();
                //Float   dx = 0, dy = 0;
                Dx = 0, Dy = 0;
                if (lineFlag)
                {
                    // General line.
                    Dx = Stream.ReadSInt(numbits);
                    Dy = Stream.ReadSInt(numbits);
                }
                else
                {
                    int vertFlag = Stream.ReadUInt1();
                    if (vertFlag == 0) {
                        // Horizontal line.
                        Dx = Stream.ReadSInt(numbits);
                    } else {
                        // Vertical line.
                        Dy = Stream.ReadSInt(numbits);
                    }
                }

                LOG_SHAPE4("  SHAPE decode: straight edge, numbits = %d, lineFl = %d, Dx = %d, Dy = %d\n", 
                    numbits, lineFlag, Dx, Dy);
                Record = Rec_StraightEdge;
            }
        }
    } while(0);        
    LOG_SHAPE0("SHAPE decode: end\n");
}

UInt GFxSwfPathData::PathsIterator::GetEdgesCount() const 
{ 
    PathsIterator pit(*this);
    return EdgesIterator(pit).CalcEdgesCount(); 
}

GFxSwfPathData::EdgesIterator::EdgesIterator(GFxSwfPathData::PathsIterator& pathIter) :
    pPathIter(&pathIter)
{
    EdgeIndex = 0;
    if (!(pPathIter->Record & PathsIterator::Rec_EdgeMask) && !pPathIter->IsFinished())
        pPathIter->ReadNextEdge();
}

void GFxSwfPathData::EdgesIterator::GetMoveXY(Float* px, Float* py)
{
    *px = Float(pPathIter->MoveX);
    *py = Float(pPathIter->MoveY);
    if (pPathIter->PathFlags & Flags_HasScaleFactor)
    {
        *px *= pPathIter->Sfactor;
        *py *= pPathIter->Sfactor;
    }
}

UInt GFxSwfPathData::EdgesIterator::GetEdgesCount() const
{
    PathsIterator pit(*pPathIter);
    EdgesIterator eit = pit.GetEdgesIterator();
    return eit.CalcEdgesCount();
}

UInt GFxSwfPathData::EdgesIterator::CalcEdgesCount()
{
    while(!IsFinished())
    {
        PlainEdge edge;
        GetPlainEdge(&edge);
    }
    return EdgeIndex;
}

void GFxSwfPathData::EdgesIterator::GetEdge(Edge* pedge, bool doLines2CurveConv)
{
    GASSERT(pPathIter->Record == PathsIterator::Rec_StraightEdge || 
            pPathIter->Record == PathsIterator::Rec_CurveEdge);
    switch(pPathIter->Record)
    {
    case PathsIterator::Rec_CurveEdge:
        {
            pedge->Cx = Float(pPathIter->Cx + pPathIter->MoveX);
            pedge->Cy = Float(pPathIter->Cy + pPathIter->MoveY);
            pPathIter->MoveX += pPathIter->Cx + pPathIter->Ax;
            pPathIter->MoveY += pPathIter->Cy + pPathIter->Ay;
            pedge->Ax = Float(pPathIter->MoveX);
            pedge->Ay = Float(pPathIter->MoveY);
            if (pPathIter->PathFlags & Flags_HasScaleFactor)
            {
                pedge->Cx *= pPathIter->Sfactor;
                pedge->Cy *= pPathIter->Sfactor;
                pedge->Ax *= pPathIter->Sfactor;
                pedge->Ay *= pPathIter->Sfactor;
            }
            pedge->Curve = 1;
        }
        break;
    case PathsIterator::Rec_StraightEdge:
        {
            pedge->Ax = Float(pPathIter->Dx + pPathIter->MoveX);
            pedge->Ay = Float(pPathIter->Dy + pPathIter->MoveY);
            if (doLines2CurveConv)
            {
                pedge->Cx = (pPathIter->Dx * 0.5f + pPathIter->MoveX);
                pedge->Cy = (pPathIter->Dy * 0.5f + pPathIter->MoveY);
                if (pPathIter->PathFlags & Flags_HasScaleFactor)
                {
                    pedge->Cx *= pPathIter->Sfactor;
                    pedge->Cy *= pPathIter->Sfactor;
                    pedge->Ax *= pPathIter->Sfactor;
                    pedge->Ay *= pPathIter->Sfactor;
                }
                pedge->Curve = 1;
            }
            else
            {
                if (pPathIter->PathFlags & Flags_HasScaleFactor)
                {
                    pedge->Ax *= pPathIter->Sfactor;
                    pedge->Ay *= pPathIter->Sfactor;
                }
                pedge->Curve = 0;
            }
            pPathIter->MoveX += pPathIter->Dx;
            pPathIter->MoveY += pPathIter->Dy;
        }
        break;
    default:;
    }
    ++EdgeIndex;
    pPathIter->ReadNext();
}


void GFxSwfPathData::EdgesIterator::GetPlainEdge(PlainEdge* pedge)
{
    GASSERT(pPathIter->Record == PathsIterator::Rec_StraightEdge || 
        pPathIter->Record == PathsIterator::Rec_CurveEdge);
    SInt32* p = pedge->Data;
    switch(pPathIter->Record)
    {
    case PathsIterator::Rec_CurveEdge:
        *p++ = pPathIter->Cx;
        *p++ = pPathIter->Cy;
        *p++ = pPathIter->Ax;
        *p++ = pPathIter->Ay;
        pPathIter->MoveX += pPathIter->Cx + pPathIter->Ax;
        pPathIter->MoveY += pPathIter->Cy + pPathIter->Ay;
        break;
    case PathsIterator::Rec_StraightEdge:
        *p++ = pPathIter->Dx;
        *p++ = pPathIter->Dy;
        pPathIter->MoveX += pPathIter->Dx;
        pPathIter->MoveY += pPathIter->Dy;
        break;
    default:;
    }
    pedge->Size = (UInt)(p - pedge->Data);
    ++EdgeIndex;
    pPathIter->ReadNext();
}

UInt GFxSwfPathData::GetMemorySize(const UByte* ppaths)
{
    UInt offset = 0;
#ifdef GFC_BUILD_DEBUG
    offset += 4; // signature
#endif
    UByte pathFlags = ppaths[offset];
    ++offset; // path flags
    UInt memSizeInBytes = ((pathFlags & GFxSwfPathData::Mask_NumBytesInMemCnt) >> 
                            GFxSwfPathData::Shift_NumBytesInMemCnt) + 1;
    const UByte* pmemSize = ppaths + offset;
    UInt memSize = GFx_ReadUInt(pmemSize, memSizeInBytes);
    return memSize;
}

void GFxSwfPathData::GetShapeAndPathCounts(const UByte* ppaths, UInt* pshapesCnt, UInt* ppathsCnt)
{
    UInt offset = 0;
#ifdef GFC_BUILD_DEBUG
    offset += 4; // signature
#endif
    UByte pathFlags = ppaths[offset];
    ++offset; // path flags
    UInt memSizeInBytes = ((pathFlags & GFxSwfPathData::Mask_NumBytesInMemCnt) >> 
                            GFxSwfPathData::Shift_NumBytesInMemCnt) + 1;
    UInt geomCntInBytes = ((pathFlags & GFxSwfPathData::Mask_NumBytesInGeomCnt) >> 
                            GFxSwfPathData::Shift_NumBytesInGeomCnt) + 1;
    const UByte* pmemSize = ppaths + offset;
    UInt memSize = GFx_ReadUInt(pmemSize, memSizeInBytes);
    const UByte* pcnts = ppaths + memSize - 2 * geomCntInBytes;
    if (pshapesCnt)
        *pshapesCnt = GFx_ReadUInt(pcnts, geomCntInBytes);
    pcnts += geomCntInBytes;
    if (ppathsCnt)
        *ppathsCnt = GFx_ReadUInt(pcnts, geomCntInBytes);
}

///////////////////////////////////
// GFxShapeNoStyles
//
GFxShapeNoStyles::GFxShapeNoStyles(UInt pageSize) :
    PathAllocator(pageSize)
{

}

// Push our shape data through the tessellator.
void    GFxShapeNoStyles::Tessellate(GFxMeshSet *meshSet, Float tolerance,
                                     GFxDisplayContext &context,
                                     GFxScale9GridInfo* s9g) const
{
    TessellateImpl<GFxPathData>(meshSet, tolerance, context, s9g);
}

bool    GFxShapeNoStyles::DefPointTestLocal(const GPointF &pt, 
                                            bool testShape, 
                                            const GFxCharacter* pinst) const
{
    return DefPointTestLocalImpl<GFxPathData>(pt, testShape, pinst);
}



// Convert the paths to GCompoundShape, flattening the curves.
// The function ignores the NewShape flag and adds all paths
// GFxShapeBase contains.
void    GFxShapeNoStyles::MakeCompoundShape(GCompoundShape *cs, Float tolerance) const
{
    MakeCompoundShapeImpl<GFxPathData>(cs, tolerance);
}


UInt32 GFxShapeNoStyles::ComputeGeometryHash() const
{
    return ComputeGeometryHashImpl<GFxPathData>();
}


bool    GFxShapeNoStyles::IsEqualGeometry(const GFxShapeBase& cmpWith) const
{
    return IsEqualGeometryImpl<GFxPathData>(cmpWith);
}



// Find the bounds of this shape, and store them in
// the given rectangle.
void    GFxShapeNoStyles::ComputeBound(GRectF* r) const
{
    ComputeBoundImpl<GFxPathData>(r);
}

GFxShapeBase::PathsIterator* GFxShapeNoStyles::GetPathsIterator() const
{
    GFxVirtualPathIterator<GFxShapeNoStyles, GFxPathData>* pswfIter = 
        GHEAP_AUTO_NEW(this) GFxVirtualPathIterator<GFxShapeNoStyles, GFxPathData>(this);
    return pswfIter;
}

void GFxShapeNoStyles::GetShapeAndPathCounts(UInt* pshapesCnt, UInt* ppathsCnt) const
{
    if (pshapesCnt)
        *pshapesCnt = Paths.ShapesCount;
    if (ppathsCnt)
        *ppathsCnt = Paths.PathsCount;
}

//////////////////////////////////////////////////////////////////////////
// GFxConstShapeWithStyles
//
GFxConstShapeWithStyles::GFxConstShapeWithStyles()
{
    MaxStrokeExtent = 100.0f; // Max Flash width = 200; so divide by 2.
    Flags |= Flags_StylesSupport | Flags_S9GSupport;
    pFillStyles = NULL;
    pLineStyles = NULL;
    FillStylesNum = LineStylesNum = 0;
    RectBound = GRectF(0,0,0,0);
}

GFxConstShapeWithStyles::~GFxConstShapeWithStyles()
{
    for (UInt i = 0; i < (UInt)FillStylesNum; ++i)
        pFillStyles[i].~GFxFillStyle();
    GFREE(const_cast<GFxFillStyle*>(pFillStyles));

    for (UInt i = 0; i < (UInt)LineStylesNum; ++i)
        pLineStyles[i].~GFxLineStyle();
    GFREE(const_cast<GFxLineStyle*>(pLineStyles));
}

void GFxConstShapeWithStyles::Tessellate(GFxMeshSet *meshSet, Float tolerance,
                                         GFxDisplayContext &context,
                                         GFxScale9GridInfo* s9g) const
{
    TessellateImpl<GFxSwfPathData>(meshSet, tolerance, context, s9g, MaxStrokeExtent);
}

void    GFxConstShapeWithStyles::Read(GFxLoadProcess* p, GFxTagType tagType, UInt lenInBytes, bool withStyle)
{
    GFxFillStyleArrayTemp fillStyles;
    GFxLineStyleArrayTemp lineStyles;
    GFxConstShapeNoStyles::Read(p, tagType, lenInBytes, withStyle, &fillStyles, &lineStyles);

    // convert fillStyles & lineStyles to regular arrays
    GMemoryHeap* pheap = p->GetLoadHeap();

    if (fillStyles.GetSize())
    {
        GASSERT(fillStyles.GetSize() < 65536);
        GASSERT(FillStylesNum == 0 && pFillStyles == NULL);
        FillStylesNum = (UInt32)fillStyles.GetSize();
        GFxFillStyle* pfillStyles = (GFxFillStyle*)
            GHEAP_ALLOC(pheap, sizeof(GFxFillStyle)*FillStylesNum, GFxStatMD_ShapeData_Mem);
        pFillStyles = pfillStyles;
        for(UInt i = 0; i < (UInt)FillStylesNum; ++i)
        {
            G_Construct<GFxFillStyle>(&pfillStyles[i], fillStyles[i]);
        }
    }
    if (lineStyles.GetSize())
    {
        GASSERT(lineStyles.GetSize() < 65536);
        GASSERT(LineStylesNum == 0 && pLineStyles == NULL);
        LineStylesNum = (UInt32)lineStyles.GetSize();
        GFxLineStyle* plineStyles = (GFxLineStyle*)
            GHEAP_ALLOC(pheap, sizeof(GFxLineStyle)*LineStylesNum, GFxStatMD_ShapeData_Mem);
        pLineStyles = plineStyles;
        for(UInt i = 0; i < (UInt)LineStylesNum; ++i)
        {
            G_Construct<GFxLineStyle>(&plineStyles[i], lineStyles[i]);
        }
    }
}

////////////////////////////////
// GFxShapeWithStyles

GFxShapeWithStyles::GFxShapeWithStyles(UInt pageSize) : GFxShapeNoStyles(pageSize)
{
    MaxStrokeExtent = 100.0f; // Max Flash width = 200; so divide by 2.
    Flags |= Flags_StylesSupport | Flags_S9GSupport;
    RectBound = GRectF(0,0,0,0);
}

GFxShapeWithStyles::~GFxShapeWithStyles()
{
}

// Creates a definition relying on in-memory image.
void    GFxShapeWithStyles::SetToImage(GFxImageResource *pimage, bool bilinear)
{
    GRect<SInt> r = pimage->GetImageInfo()->GetRect();
    SInt iw = r.Width();
    SInt ih = r.Height();
    Float width  = (Float)PixelsToTwips(iw);
    Float height = (Float)PixelsToTwips(ih);

    // Create fill style
    FillStyles.Resize(1);
    GMatrix2D m;
    m.Translation((Float)PixelsToTwips(r.Left), (Float)PixelsToTwips(r.Top));
    FillStyles[0].SetImageFill(bilinear ? GFxFill_ClippedSmoothImage : GFxFill_ClippedImage,
        pimage, m);
    Flags |= GFxShapeBase::Flags_TexturedFill;

    // Set bounds
    Bound.SetRect(width, height);
    SetRectBoundsLocal(Bound);
    SetValidBoundsFlag(true);
    MaxStrokeExtent = 0;

    // Path with fill 0 on one side; clockwise.
    PathAllocator.SetDefaultPageSize(32);
    Flags |= GFxShapeBase::Flags_LocallyAllocated;

    GFxPathPacker currentPath;
    currentPath.SetFill0(0);
    currentPath.SetFill1(1);
    currentPath.SetLine(0);

    currentPath.SetMoveTo(0, 0);
    currentPath.AddLineTo(PixelsToTwips(iw), 0);
    currentPath.AddLineTo(0, PixelsToTwips(ih));
    currentPath.AddLineTo(-PixelsToTwips(iw), 0);
    currentPath.AddLineTo(0, -PixelsToTwips(ih));

    currentPath.Pack(&PathAllocator, &Paths);
}

void GFxShapeWithStyles::Tessellate(GFxMeshSet *meshSet, Float tolerance,
                                    GFxDisplayContext &context,
                                    GFxScale9GridInfo* s9g) const
{
    TessellateImpl<GFxPathData>(meshSet, tolerance, context, s9g, MaxStrokeExtent);
}


//
// ***** GFxPathData::EdgesIterator
//

GFxPathData::EdgesIterator::EdgesIterator(const GFxPathData::PathsIterator& pathIter)
{
    // pre-parse the path
    pPath = pathIter.CurPathsPtr;

    EdgeIndex = 0;
    Sfactor = pathIter.Sfactor;
    CurEdgesInfoMask = 1;
    if (!pPath) 
    {
        EdgesCount   = 0;
        CurEdgesInfo = 0;
        pVertices16  = 0;
        pVertices32  = 0;
        return;
    }

    UByte flags = *pPath;
    
    if (flags & GFxPathConsts::PathTypeMask)
    {
        EdgesCount = ((flags & GFxPathConsts::Path8EdgesCountMask) >> GFxPathConsts::Path8EdgesCountShift) + 1;
        CurEdgesInfo = (flags & GFxPathConsts::Path8EdgeTypesMask) >> GFxPathConsts::Path8EdgeTypesShift;
        pVertices16 = (const SInt16*)(GFx_AlignedPtr<2>(pPath + 4));
        MoveX = *pVertices16++;
        MoveY = *pVertices16++;
    }
    else
    {
        UInt vertexSFactor = (flags & GFxPathConsts::PathEdgeSizeMask) ? 4 : 2;
        UByte pathSizeType = (UByte)((flags & GFxPathConsts::PathSizeMask) >> GFxPathConsts::PathSizeShift);
        if (pathSizeType == 0) // newShape
        {
            EdgesCount = 0;
            return;
        }
        static const UInt pathSFactorTable[] = { 0, 1, 2, 4 };
        const UByte* paligned = GFx_AlignedPtr(pPath + 1, pathSizeType);
        paligned              = GFx_AlignedPtr(paligned + pathSFactorTable[pathSizeType] * 3, vertexSFactor);
        EdgesCount            = (flags & GFxPathConsts::PathEdgesCountMask) >> GFxPathConsts::PathEdgesCountShift;
        if (vertexSFactor == 2)
        {
            pVertices16 = (const SInt16*)(paligned);
            if (EdgesCount == 0)
                EdgesCount = (UInt16)(*pVertices16++);
            GASSERT(EdgesCount > 0);
            MoveX = *pVertices16++;
            MoveY = *pVertices16++;
            CurEdgesInfo = (UInt16)(*pVertices16++);

            pVertices32 = 0;
        }
        else
        {
            pVertices32 = (const SInt32*)(paligned);
            if (EdgesCount == 0)
                EdgesCount = (UInt32)(*pVertices32++);
            GASSERT(EdgesCount > 0);
            MoveX = *pVertices32++;
            MoveY = *pVertices32++;
            CurEdgesInfo = (UInt32)(*pVertices32++);

            pVertices16 = 0;
        }
    }
    GASSERT(EdgesCount < (1UL<<31));
}

void GFxPathData::EdgesIterator::GetEdge(Edge* pedge, bool doLines2CurveConv)
{
    if (EdgeIndex < EdgesCount)
    {
        SInt32 cx, cy, ax, ay;
        bool curve = false;
        if (pVertices16)
        {
            if (CurEdgesInfoMask > 0x8000)
            {
                CurEdgesInfo = *pVertices16++;
                CurEdgesInfoMask = 1;
            }
            if (CurEdgesInfo & CurEdgesInfoMask) // curve
            {
                cx = *pVertices16++;
                cy = *pVertices16++;
                ax = *pVertices16++;
                ay = *pVertices16++;
                curve = true;
            }
            else
            {
                cx = ax = *pVertices16++;
                cy = ay = *pVertices16++;
            }
        }
        else
        {
            GASSERT(pVertices32);

            if ((CurEdgesInfoMask & 0xFFFFFFFFu) == 0) // care just about 32-bits
            {
                CurEdgesInfo = *pVertices32++;
                CurEdgesInfoMask = 1;
            }
            if (CurEdgesInfo & CurEdgesInfoMask) // curve
            {
                cx = *pVertices32++;
                cy = *pVertices32++;
                ax = *pVertices32++;
                ay = *pVertices32++;
                curve = true;
            }
            else
            {
                cx = ax = *pVertices32++;
                cy = ay = *pVertices32++;
            }
        }
        if (curve)
        {
            pedge->Cx = (cx + MoveX) * Sfactor;
            pedge->Cy = (cy + MoveY) * Sfactor;
            pedge->Ax = (ax + cx + MoveX) * Sfactor;
            pedge->Ay = (ay + cy + MoveY) * Sfactor;
            pedge->Curve = 1;
            MoveX += cx + ax;
            MoveY += cy + ay;
        }
        else
        {
            pedge->Ax = (ax + MoveX) * Sfactor;
            pedge->Ay = (ay + MoveY) * Sfactor;
            if (doLines2CurveConv)
            {
                pedge->Cx = (ax * 0.5f + MoveX) * Sfactor;
                pedge->Cy = (ay * 0.5f + MoveY) * Sfactor;
                pedge->Curve = 1;
            }
            else
                pedge->Curve = 0;
            MoveX += ax;
            MoveY += ay;
        }
        ++EdgeIndex;
        CurEdgesInfoMask <<= 1;
    }
}


void GFxPathData::EdgesIterator::GetPlainEdge(PlainEdge* pedge)
{
    if (EdgeIndex < EdgesCount)
    {
        SInt32* p = pedge->Data;
        if (pVertices16)
        {
            if (CurEdgesInfoMask > 0x8000)
            {
                CurEdgesInfo = *pVertices16++;
                CurEdgesInfoMask = 1;
            }
            if (CurEdgesInfo & CurEdgesInfoMask) // curve
            {
                *p++ = *pVertices16++;
                *p++ = *pVertices16++;
                *p++ = *pVertices16++;
                *p++ = *pVertices16++;
            }
            else
            {
                *p++ = *pVertices16++;
                *p++ = *pVertices16++;
            }
        }
        else
        {
            GASSERT(pVertices32);

            if ((CurEdgesInfoMask & 0xFFFFFFFFu) == 0) // care just about 32-bits
            {
                CurEdgesInfo = *pVertices32++;
                CurEdgesInfoMask = 1;
            }
            if (CurEdgesInfo & CurEdgesInfoMask) // curve
            {
                *p++ = *pVertices32++;
                *p++ = *pVertices32++;
                *p++ = *pVertices32++;
                *p++ = *pVertices32++;
            }
            else
            {
                *p++ = *pVertices32++;
                *p++ = *pVertices32++;
            }
        }
        ++EdgeIndex;
        CurEdgesInfoMask <<= 1;
        pedge->Size = (UInt)(p - pedge->Data);
    }
}



//
// ***** GFxPathData::PathsIterator
//

GFxPathData::PathsIterator::PathsIterator(const GFxShapeBase* pdef) : 
    pShapeDef(static_cast<const GFxShapeNoStyles*>(pdef)),
    CurPathsPtr(pShapeDef->GetNativePathData()->pPaths), Paths(*pShapeDef->GetNativePathData()), 
    CurIndex(0), Count(pShapeDef->GetNativePathData()->PathsCount),
    Sfactor((pdef->GetFlags() & GFxShapeBase::Flags_Sfactor20) ? 1.0f/20.0f : 1.0f)
{
}

void GFxPathData::PathsIterator::AddForTessellation(GCompoundShape *cs)
{
    EdgesIterator it = GetEdgesIterator();

    Float ax, ay;
    it.GetMoveXY(&ax, &ay);
    UInt fill0, fill1, line;
    GetStyles(&fill0, &fill1, &line);
    cs->BeginPath(fill0 - 1, fill1 - 1, line - 1, ax, ay);

    EdgesIterator::Edge edge;
    while(!it.IsFinished())
    {
        it.GetEdge(&edge);
        if (edge.Curve)
            cs->AddCurve(edge.Cx, edge.Cy, edge.Ax, edge.Ay);
        else
            cs->AddVertex(edge.Ax, edge.Ay);
    }
    AdvanceBy(it);
}

void GFxPathData::PathsIterator::GetStyles(UInt* pfill0, UInt* pfill1, UInt* pline) const
{
    UByte flags = *CurPathsPtr;
    if (flags & GFxPathConsts::PathTypeMask || 
        (flags & GFxPathConsts::PathSizeMask) == (1 << GFxPathConsts::PathSizeShift))
    {
        const UByte* ptr = CurPathsPtr + 1;
        *pfill0 = *ptr++;
        *pfill1 = *ptr++;
        *pline  = *ptr;
    }
    else if (!IsNewShape()) 
    {
        if ((flags & GFxPathConsts::PathSizeMask) == (2 << GFxPathConsts::PathSizeShift))
        {
            const UInt16* ptr16 = (const UInt16*)GFx_AlignedPtr<2>(CurPathsPtr + 1);
            *pfill0 = *ptr16++;
            *pfill1 = *ptr16++;
            *pline  = *ptr16;
        }
        else
        {
            const UInt32* ptr32 = (const UInt32*)GFx_AlignedPtr<4>(CurPathsPtr + 1);
            *pfill0 = *ptr32++;
            *pfill1 = *ptr32++;
            *pline  = *ptr32;
        }
    }
    else
        GASSERT(0);
}

void GFxPathData::PathsIterator::AdvanceBy(const EdgesIterator& edgeIt)
{
    if (edgeIt.IsFinished())
    {
        CurPathsPtr = (edgeIt.pVertices16) ? (const UByte*)edgeIt.pVertices16 : (const UByte*)edgeIt.pVertices32;
        ++CurIndex;
        CheckPage();
    }
}

void GFxPathData::PathsIterator::CheckPage()
{
    const void* ppage = GFxPathAllocator::GetPagePtr(Paths.pPaths, Paths.PathsPageOffset);
    if (!GFxPathAllocator::IsInPage(ppage, CurPathsPtr))
    {
        const void* pnextPage;
        if ((pnextPage = GFxPathAllocator::GetNextPage(ppage, &CurPathsPtr)) != NULL)
        {
            Paths.pPaths = CurPathsPtr;
            Paths.PathsPageOffset = GFxPathAllocator::GetPageOffset(pnextPage, CurPathsPtr);
        }
    }
}

void GFxPathData::PathsIterator::SkipComplex()
{
    UInt32 edgesCount, curEdgesInfo = 0;
    UInt32 curEdgesInfoMask = 1;
    UByte flags = *CurPathsPtr;
    const SInt16* pvertices16 = 0;
    const SInt32* pvertices32 = 0;

    if (flags & GFxPathConsts::PathTypeMask)
    {
        edgesCount = ((flags & GFxPathConsts::Path8EdgesCountMask) >> GFxPathConsts::Path8EdgesCountShift) + 1;
        curEdgesInfo = (flags & GFxPathConsts::Path8EdgeTypesMask) >> GFxPathConsts::Path8EdgeTypesShift;
        pvertices16 = (const SInt16*)(GFx_AlignedPtr<2>(CurPathsPtr + 4));
        // skip moveX, moveY
        pvertices16 += 2;
    }
    else
    {
        UInt vertexSFactor = (flags & GFxPathConsts::PathEdgeSizeMask) ? 4 : 2;
        UByte pathSizeType = (UByte)((flags & GFxPathConsts::PathSizeMask) >> GFxPathConsts::PathSizeShift);
        if (pathSizeType == 0) // newShape
        {
            edgesCount = 0;
            ++CurPathsPtr;
            return;
        }
        static const UInt pathSFactorTable[] = { 0, 1, 2, 4 };
        const UByte* paligned = GFx_AlignedPtr(CurPathsPtr + 1, pathSizeType);
        paligned              = GFx_AlignedPtr(paligned + pathSFactorTable[pathSizeType] * 3, vertexSFactor);
        edgesCount            = (flags & GFxPathConsts::PathEdgesCountMask) >> GFxPathConsts::PathEdgesCountShift;
        if (vertexSFactor == 2)
        {
            pvertices16 = (const SInt16*)(paligned);
            if (edgesCount == 0)
                edgesCount = (UInt16)(*pvertices16++);
            GASSERT(edgesCount > 0);
            // skip moveX, Y
            pvertices16 += 2;
            curEdgesInfo = (UInt16)(*pvertices16++);
        }
        else
        {
            pvertices32 = (const SInt32*)(paligned);
            if (edgesCount == 0)
                edgesCount = (UInt32)(*pvertices32++);
            GASSERT(edgesCount > 0);
            // skip moveX, Y
            pvertices32 += 2;
            curEdgesInfo = (UInt32)(*pvertices32++);
        }
    }
    // skip vertices, edgeinfos
    if (pvertices16)
    {
        for (UInt i = 0; i < edgesCount; ++i)
        {
            if (curEdgesInfoMask > 0x8000)
            {
                curEdgesInfo = *pvertices16++;
                curEdgesInfoMask = 1;
            }
            if (curEdgesInfo & curEdgesInfoMask) // curve
            {
                // skip curve
                pvertices16 += 4;
            }
            else
            {
                // skip line
                pvertices16 += 2;
            }
            curEdgesInfoMask <<= 1;
        }
        CurPathsPtr = (const UByte*)pvertices16;
    }
    else
    {
        GASSERT(pvertices32);
        for (UInt i = 0; i < edgesCount; ++i)
        {
            //GASSERT(*pvertices32 != 0xbabababa);

            if ((curEdgesInfoMask & 0xFFFFFFFFu) == 0) // care just about 32-bits
            {
                curEdgesInfo = *pvertices32++;
                curEdgesInfoMask = 1;
            }
            if (curEdgesInfo & curEdgesInfoMask) // curve
            {
                // skip curve
                pvertices32 += 4;
            }
            else
            {
                // skip line
                pvertices32 += 2;
            }
            curEdgesInfoMask <<= 1;
        }
        CurPathsPtr = (const UByte*)pvertices32;
    }
    ++CurIndex;
    CheckPage();
}

//////////////////////////////////////////////////////////////////////////
// GFxShapeCharacterDef
//
// Creates a definition relying on in-memory image.
void    GFxShapeCharacterDef::SetToImage(GFxImageResource *pimage, bool bilinear)
{
    Shape.SetToImage(pimage, bilinear);
}

void    GFxShapeCharacterDef::Display(GFxDisplayContext &context, GFxCharacter* inst)
{
    Shape.Display(context, inst);
}

void    GFxShapeCharacterDef::ComputeBound(GRectF* r) const
{
    Shape.ComputeBoundImpl<GFxPathData>(r);
}

bool    GFxShapeCharacterDef::DefPointTestLocal(const GPointF &pt, bool testShape, const GFxCharacter *pinst) const
{
    return Shape.DefPointTestLocalImpl<GFxPathData>(pt, testShape, pinst);
}

// Convert the paths to GCompoundShape, flattening the curves.
void    GFxShapeCharacterDef::MakeCompoundShape(GCompoundShape *cs, Float tolerance) const
{
    Shape.MakeCompoundShapeImpl<GFxPathData>(cs, tolerance);

}

UInt32  GFxShapeCharacterDef::ComputeGeometryHash() const
{
    return Shape.ComputeGeometryHashImpl<GFxPathData>();
}

bool    GFxShapeCharacterDef::IsEqualGeometry(const GFxShapeBaseCharacterDef& cmpWith) const
{
    return Shape.IsEqualGeometryImpl<GFxPathData>(*cmpWith.GetShape());
}

void    GFxShapeCharacterDef::PreTessellate(Float masterScale, const GFxRenderConfig& config)
{
    Shape.PreTessellateImpl<GFxPathData>(masterScale, config);
}

GFxShapeCharacterDef::PathsIterator* GFxShapeCharacterDef::GetPathsIterator() const
{
    return Shape.GetPathsIterator();
}


//////////////////////////////////////////////////////////////////////////
// GFxConstShapeCharacterDef
//
void    GFxConstShapeCharacterDef::Display(GFxDisplayContext &context, GFxCharacter* inst)
{
    Shape.Display(context, inst);
}

void    GFxConstShapeCharacterDef::ComputeBound(GRectF* r) const
{
    Shape.ComputeBoundImpl<GFxSwfPathData>(r);
}

bool    GFxConstShapeCharacterDef::DefPointTestLocal(const GPointF &pt, bool testShape, const GFxCharacter *pinst) const
{
    return Shape.DefPointTestLocalImpl<GFxSwfPathData>(pt, testShape, pinst);
}

// Convert the paths to GCompoundShape, flattening the curves.
void    GFxConstShapeCharacterDef::MakeCompoundShape(GCompoundShape *cs, Float tolerance) const
{
    Shape.MakeCompoundShapeImpl<GFxSwfPathData>(cs, tolerance);

}

UInt32  GFxConstShapeCharacterDef::ComputeGeometryHash() const
{
    return Shape.ComputeGeometryHashImpl<GFxSwfPathData>();
}

bool    GFxConstShapeCharacterDef::IsEqualGeometry(const GFxShapeBaseCharacterDef& cmpWith) const
{
    return Shape.IsEqualGeometryImpl<GFxSwfPathData>(*cmpWith.GetShape());
}

void    GFxConstShapeCharacterDef::PreTessellate(Float masterScale, const GFxRenderConfig& config)
{
    Shape.PreTessellateImpl<GFxSwfPathData>(masterScale, config);
}

GFxConstShapeCharacterDef::PathsIterator* GFxConstShapeCharacterDef::GetPathsIterator() const
{
    return Shape.GetPathsIterator();
}



#if 0
// DBG
//static void GFX_DrawQuad(Float x1, Float y1, Float x2, Float y2,
//                         Float x3, Float y3, Float x4, Float y4,
//                         UInt32 rgba, 
//                         GRenderer* prenderer)
//{
//    const SInt16  icoords[18] = 
//    {
//        // Strip (fill in)
//        (SInt16) x1, (SInt16) y1,
//        (SInt16) x2, (SInt16) y2,
//        (SInt16) x3, (SInt16) y3,
//        (SInt16) x4, (SInt16) y4
//    };
//    static const UInt16     indices[6] = { 0, 1, 2, 0, 2, 3 };
//
//    prenderer->FillStyleColor(rgba);
//    prenderer->SetVertexData(icoords, 9, GRenderer::Vertex_XY16i);
//
//    // Fill the inside
//    prenderer->SetIndexData(indices, 6, GRenderer::Index_16);
//    prenderer->DrawIndexedTriList(0, 0, 6, 0, 2);
//
//    // Done
//    prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
//    prenderer->SetIndexData(0, 0, GRenderer::Index_None);
//}
//static void GFX_DrawParl(Float x1, Float y1, Float x2, Float y2,
//                         Float x3, Float y3, 
//                         UInt32 rgba, 
//                         GRenderer* prenderer)
//{
//    GFX_DrawQuad(x1, y1, x2, y2, x3, y3, x1 + (x3-x2), y1 + (y3-y2), rgba, prenderer);
//}






template<class Container>
static void GFx_DumpFillStyles(FILE* fd, const Container& s)
{
    unsigned i;
    for(i = 0; i < s.GetSize(); i++)
    {
        fprintf(fd, "Fill %d %d %d %d %d\n",
                i,
                s[i].GetColor().GetRed(),
                s[i].GetColor().GetGreen(),
                s[i].GetColor().GetBlue(),
                s[i].GetColor().GetAlpha());
    }
}

template<class Container>
static void GFx_DumpLineStyles(FILE* fd, const Container& s)
{
    unsigned i;
    for(i = 0; i < s.GetSize(); i++)
    {
        fprintf(fd, "Stroke %d %d.0 %u %d %d %d %d\n",
                i,
                s[i].GetWidth(),
                s[i].GetStyle(),
                s[i].GetColor().GetRed(),
                s[i].GetColor().GetGreen(),
                s[i].GetColor().GetBlue(),
                s[i].GetColor().GetAlpha());
    }
}

/*
// Dump our precomputed GFxMesh data to the given GFxStream.
void    GFxShapeBase::OutputCachedData(GFile* out, const GFxCacheOptions& options)
{
    GUNUSED(options);

    UInt    n = CachedMeshes.GetSize();
    out->WriteSInt32(n);

    for (UInt i = 0; i < n; i++)
    {
        CachedMeshes[i]->OutputCachedData(out);
    }
}


// Initialize our GFxMesh data from the given GFxStream.
void    GFxShapeBase::InputCachedData(GFile* in)
{
    int n = in->ReadSInt32();

    CachedMeshes.Resize(n);

    for (int i = 0; i < n; i++)
    {
        GFxMeshSet* ms = GHEAP_AUTO_NEW(this) GFxMeshSet();
        ms->InputCachedData(in);
        CachedMeshes[i] = ms;
    }
}
*/

#endif
