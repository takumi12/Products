/**********************************************************************

Filename    :   GFxShape.h
Content     :   SWF (Shockwave Flash) player library
Created     :   July 7, 2005
Authors     :

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXSHAPE_H
#define INC_GFXSHAPE_H

#include "GFxStreamContext.h"
#include "GFxStyles.h"
#include "GFxScale9Grid.h"


// ***** Declared Classes
class GFxConstShapeNoStyles;
class GFxShapeNoStyles;
class GFxShapeBase;
class GFxShapeCharacterDef;

// ***** External Classes
class GCompoundShape;
class GFxMeshSet;
class GFxCharacter;
class GFxStream;

typedef GArrayLH<GFxFillStyle, GFxStatMD_ShapeData_Mem> GFxFillStyleArray;
typedef GArrayLH<GFxLineStyle, GFxStatMD_ShapeData_Mem> GFxLineStyleArray;


typedef GArray<GFxFillStyle, GFxStatMD_ShapeData_Mem> GFxFillStyleArrayTemp;
typedef GArray<GFxLineStyle, GFxStatMD_ShapeData_Mem> GFxLineStyleArrayTemp;



// Struct to pass parameters to shape's and mesh's Display
//
struct GFxDisplayParams
{
    GFxDisplayContext&          Context;
    const GFxFillStyle*         pFillStyles;
    const GFxLineStyle*         pLineStyles;
    GRenderer::Matrix           Mat;
    GRenderer::Cxform           Cx;
    GRenderer::BlendType        Blend;
    UInt                        FillStylesNum;
    UInt                        LineStylesNum;

    GFxDisplayParams(GFxDisplayContext& ctxt) : Context(ctxt), pFillStyles(NULL), pLineStyles(NULL)
    {
        FillStylesNum = LineStylesNum = 0;
    }
    GFxDisplayParams(GFxDisplayContext& ctxt, 
                     const GRenderer::Matrix& mat, 
                     const GRenderer::Cxform& cx,
                     GRenderer::BlendType blend, 
                     const GFxFillStyle* pfillStyles = NULL, UPInt fillStylesNum = 0, 
                     const GFxLineStyle* plineStyles = NULL, UPInt lineStylesNum = 0) : 
    Context(ctxt), pFillStyles(pfillStyles), pLineStyles(plineStyles), Mat(mat), 
    Cx(cx), Blend(blend), FillStylesNum((UInt)fillStylesNum), LineStylesNum((UInt)lineStylesNum)
    {}

    GFxDisplayParams(GFxDisplayContext& ctxt, 
        const GRenderer::Matrix& mat, 
        const GRenderer::Cxform& cx,
        GRenderer::BlendType blend, 
        const GFxFillStyleArray& fillStyles,
        const GFxLineStyleArray& lineStyles) : 
    Context(ctxt), pFillStyles(fillStyles.GetSize() ? &fillStyles[0] : NULL), 
    pLineStyles(lineStyles.GetSize() ? &lineStyles[0] : NULL), Mat(mat), 
    Cx(cx), Blend(blend), FillStylesNum((UInt)fillStyles.GetSize()), LineStylesNum((UInt)lineStyles.GetSize())
    {}

    GFxDisplayParams& operator=(const GFxDisplayParams&) { GASSERT(0); return *this; }
};


// ***** Path Representation

struct GFxPathConsts
{
    enum StyleFlag
    {
        Style_None  = GFC_MAX_UINT,
    };
    enum 
    {
        PathTypeMask = 0x1 // 0 - complex, 1 - path8 + 16bit edges
    };
    enum
    {
        Path8EdgesCountMask     = 0x6,
        Path8EdgesCountShift    = 1,
        Path8EdgeTypesMask      = 0x78,
        Path8EdgeTypesShift     = 3,

        PathSizeMask            = 0x6,         // 0 - new shape, 1 - path 8, 2 - path 16, 3 - path 32
        PathSizeShift           = 1,
        PathEdgeSizeMask        = 0x8,     // 0 - 16 bit, 1 - 32 - bit
        PathEdgeSizeShift       = 3,
        PathEdgesCountMask      = 0xF0,
        PathEdgesCountShift     = 4
    };
};

// A path allocator class. Used to allocate memory for shapes to avoid
// fragmentation.
//
class GFxPathAllocator : public GNewOverrideBase<GFxStatMD_CharDefs_Mem>
{
private:
    struct Page
    {
        Page*       pNext;
        UInt32      PageSize;
        //UByte Buffer[];

        UByte* GetBufferPtr()                            { return ((UByte*)(&PageSize + 1)); }
        UByte* GetBufferPtr(UInt freeBytes)              { return ((UByte*)(&PageSize + 1)) + PageSize - freeBytes; }
        const UByte* GetBufferPtr() const                { return ((UByte*)(&PageSize + 1)); }
        const UByte* GetBufferPtr(UInt freeBytes) const  { return ((UByte*)(&PageSize + 1)) + PageSize - freeBytes; }
    };
    Page        *pFirstPage, *pLastPage;
    UInt16      FreeBytes;
    UInt16      DefaultPageSize;

public:
    enum { Default_PageBufferSize = 8192-sizeof(Page) };

    GFxPathAllocator(UInt pageSize = Default_PageBufferSize);
    ~GFxPathAllocator();

    void   Clear();

    UByte* AllocPath(UInt edgesDataSize, UInt pathSize, UInt edgeSize);
    UByte* AllocRawPath(UInt32 sizeInBytes);
    UByte* AllocMemoryBlock(UInt32 sizeForCurrentPage, UInt32 sizeInNewPage);
    bool   ReallocLastBlock(UByte* ptr, UInt32 oldSize, UInt32 newSize);
    void   SetDefaultPageSize(UInt dps);
    UInt16 GetPageOffset(const UByte* ptr) const
    {
        GASSERT(ptr - (const UByte*)pLastPage >= 0 && ptr - (const UByte*)pLastPage < 65536);
        return (UInt16)(ptr - (const UByte*)pLastPage);
    }

    static UInt16 GetPageOffset(const void* ppage, const UByte* ptr)
    {
        const Page* p = (const Page*)ppage;
        GASSERT(ptr - (const UByte*)p >= 0 && ptr - (const UByte*)p < 65536);
        return (UInt16)(ptr - (const UByte*)p);
    }
    static const void* GetPagePtr(const UByte* pinitialPtr, UInt16 offtopage) { return pinitialPtr - offtopage; }
    static bool  IsInPage(const void* ppage, const UByte* ptr) 
    { 
        GASSERT(ppage && ptr);
        const Page* p = (const Page*)ppage;
        return (ptr - (const UByte*)(&p->PageSize + 1)) < (SInt)p->PageSize;
    }
    static const void* GetNextPage(const void* ppage, const UByte** pdata)
    {
        GASSERT(ppage && pdata);
        const Page* p = (const Page*)ppage;
        p = p->pNext;
        if (p)
            *pdata = p->GetBufferPtr();
        else
            *pdata = NULL;
        return p;
    }
};

// Class that provides iterators to work with GFx path data (created by GFxPathPacker)
//
class GFxPathData
{
public:
    enum { Signature = 0x12345678 };

    // Structure that stores path info
    //
    struct PathsInfo
    {
        const UByte* pPaths;
        UInt         PathsCount;
        UInt         ShapesCount;
        UInt16       PathsPageOffset;

        PathsInfo():pPaths(NULL), PathsCount(0), ShapesCount(0), PathsPageOffset(0) {}
    };

    class PathsIterator;
    class EdgesIterator
    {
        friend class PathsIterator;
        const UByte*    pPath;

        UInt            EdgeIndex;
        UInt            EdgesCount;
        SInt            MoveX, MoveY;
        UInt            CurEdgesInfo;
        UInt            CurEdgesInfoMask;

        const SInt16*   pVertices16;
        const SInt32*   pVertices32;

        Float           Sfactor;

    public:
        EdgesIterator():pPath(0), EdgesCount(0) {}
        EdgesIterator(const PathsIterator&);

        bool IsFinished() const { return EdgeIndex >= EdgesCount; }

        void GetMoveXY(Float* px, Float* py)
        {
            *px = MoveX * Sfactor;
            *py = MoveY * Sfactor;
        }

        struct Edge
        {
            Float Cx, Cy, Ax, Ay;
            bool  Curve;
        };
        struct PlainEdge
        {
            SInt32 Data[4];
            UInt   Size;
        };
        void GetEdge(Edge*, bool doLines2CurveConv = false);
        void GetPlainEdge(PlainEdge*);
        UInt GetEdgesCount() const { return EdgesCount; }
    };
    class PathsIterator
    {
        friend class EdgesIterator;
        friend class GFxShapeBase;
        friend class GFxShapeNoStyles;

        const GFxShapeNoStyles* pShapeDef;
        const UByte*    CurPathsPtr;
        GFxPathData::PathsInfo Paths;
        UInt            CurIndex;
        UInt            Count;
        Float           Sfactor;

        void            SkipComplex();
        void            CheckPage();

        PathsIterator(const GFxShapeBase* pdef);
    public:

        // adds current path to tessellation and advances the iterator
        void AddForTessellation(GCompoundShape *cs);
        // skip current path
        void Skip()
        {
            if (!IsFinished())
            {
                GASSERT(CurIndex < Count);
                if (IsNewShape())
                {
                    ++CurPathsPtr;
                }
                else
                    SkipComplex();
                ++CurIndex;
                CheckPage();
            }
        }
        // is current path a new shape?
        bool IsNewShape() const 
        { 
            GASSERT(CurPathsPtr);
            return ((*CurPathsPtr & (GFxPathConsts::PathTypeMask | GFxPathConsts::PathSizeMask)) == 0); 
        }
        bool IsFinished() const { return CurIndex >= Count; }

        void GetStyles(UInt* pfill0, UInt* pfill1, UInt* pline) const;

        EdgesIterator GetEdgesIterator() const { return EdgesIterator(*this); }
        void AdvanceBy(const EdgesIterator& edgeIt);

        UInt GetEdgesCount() const { return EdgesIterator(*this).GetEdgesCount(); }
    };
};

// Class that provides iterators to work with native flash path data
//
class GFxSwfPathData
{
public:
    enum { Signature = 0x07654321 };

    enum Flags
    {
        Flags_HasFillStyles         = 0x1,
        Flags_HasLineStyles         = 0x2,
        Flags_HasExtendedFillNum    = 0x4,

        Mask_NumBytesInMemCnt       = 0x18, // mask for number of bytes in memory counter
        Shift_NumBytesInMemCnt      = 3,

        Mask_NumBytesInGeomCnt      = 0x60,  // mask for number of bytes in shapes & paths counter
        Shift_NumBytesInGeomCnt     = 5,

        Flags_HasScaleFactor        = 0x80
    };
    class PathsIterator;
    class EdgesIterator
    {
        friend class PathsIterator;
        PathsIterator*  pPathIter;

        UInt            EdgeIndex;
    
        UInt CalcEdgesCount();
    public:
        EdgesIterator(): pPathIter(NULL), EdgeIndex(0) {}
        EdgesIterator(PathsIterator&);

        GINLINE bool IsFinished() const;

        void GetMoveXY(Float* px, Float* py);

        struct Edge
        {
            Float Cx, Cy, Ax, Ay;
            bool  Curve;
        };
        struct PlainEdge
        {
            SInt32 Data[4];
            UInt   Size;
        };
        void GetEdge(Edge*, bool doLines2CurveConv = false);
        void GetPlainEdge(PlainEdge*);
        UInt GetEdgesCount() const;
    };
    class PathsIterator
    {
        friend class GFxShapeBase;
        friend class GFxConstShapeNoStyles;
        friend class EdgesIterator;

        const GFxConstShapeNoStyles* pShapeDef;
        GFxStreamContext   Stream;
        UInt            Fill0, Fill1, Line;
        enum RecordType
        {
            Rec_None            = 0,
            Rec_EndOfShape      = 1,
            Rec_NewShape        = 2,
            Rec_NonEdge         = 3,
            Rec_EdgeMask        = 0x80,
            Rec_StraightEdge    = 4 | Rec_EdgeMask,
            Rec_CurveEdge       = 5 | Rec_EdgeMask
        };               
        UByte           Record;
        UInt            PathsCount;
        UInt            ShapesCount;
        SInt            MoveX, MoveY;
        UInt            FillBase;
        UInt            LineBase;
        UInt            CurFillStylesNum;
        UInt            CurLineStylesNum;
        UInt            NumFillBits;
        UInt            NumLineBits;
        UInt            CurEdgesCount;
        UInt            CurPathsCount;
        SInt            Ax, Ay, Cx, Cy, Dx, Dy;
        Float           Sfactor;
        UByte           PathFlags; // bits, see enum Flags_

        void            SkipComplex();
        void            ReadNext();
        void            ReadNextEdge();

        PathsIterator(const GFxShapeBase* pdef);
    public:
        PathsIterator(const PathsIterator& p);

        // adds current path to tessellation and advances the iterator
        void AddForTessellation(GCompoundShape *cs);
        // skip current path
        void Skip()
        {
            if (!IsFinished())
            {
                if (IsNewShape())
                {
                    ReadNextEdge();
                }
                else
                    SkipComplex();
            }
        }
        // is current path a new shape?
        bool IsNewShape() const 
        { 
            return Record == Rec_NewShape; 
        }
        bool IsFinished() const { return Record == Rec_EndOfShape; }

        void GetStyles(UInt* pfill0, UInt* pfill1, UInt* pline) const
        {
            *pfill0 = Fill0; *pfill1 = Fill1; *pline = Line;
        }

        EdgesIterator GetEdgesIterator() { return EdgesIterator(*this); }
        void AdvanceBy(const EdgesIterator& ) {}

        UInt GetEdgesCount() const;
    };

    static UInt GetMemorySize(const UByte* p);
    static void GetShapeAndPathCounts(const UByte* p, UInt* pshapesCnt, UInt* ppathsCnt);
};

// A path packer class. Used for shapes' creation other than loaded from SWF files, 
// for example for drawing API or for system font glyphs.
//
class GFxPathPacker
{
public:
    GFxPathPacker();

    void Pack(GFxPathAllocator* pallocator, GFxPathData::PathsInfo* ppathsInfo);
    void SetFill0(UInt fill0) { Fill0 = fill0; }
    void SetFill1(UInt fill1) { Fill1 = fill1; }
    void SetLine (UInt line)  { Line  = line; }

    UInt GetFill0() const { return Fill0; }
    UInt GetFill1() const { return Fill1; }
    UInt GetLine () const { return Line; }

    void SetMoveTo(SInt x, SInt y, UInt numBits = 0);
    void SetMoveToLastVertex() { SetMoveTo(Ex, Ey); }
    void AddLineTo(SInt x, SInt y, UInt numBits = 0);
    void AddCurve (SInt cx, SInt cy, SInt ax, SInt ay, UInt numBits = 0);
    void LineToAbs(SInt x, SInt y);
    void CurveToAbs(SInt cx, SInt cy, SInt ax, SInt ay);
    void ClosePath();
    void GetLastVertex(SInt* x, SInt *y) const { *x = Ex; *y = Ey; }

    void Reset();
    void ResetEdges()    { EdgesIndex = LinesNum = CurvesNum = 0; NewShape = 0; }
    bool IsEmpty() const { return EdgesIndex == 0; }
    void SetNewShape()   { NewShape = 1; }

protected:
    //private:
    UInt                    Fill0, Fill1, Line;

    SInt                    Ax, Ay; // moveTo
    SInt                    Ex, Ey;

    struct Edge
    {
        // *quadratic* bezier: point = p0 * t^2 + p1 * 2t(1-t) + p2 * (1-t)^2
        SInt   Cx, Cy;     // "control" point
        SInt   Ax, Ay;     // "anchor" point
        enum
        {
            Curve,
            Line
        };
        UByte  Type;

        Edge() { Cx = Cy = Ax = Ay = 0; Type = Line; }
        Edge(SInt x, SInt y): Cx(x), Cy(y), Ax(x), Ay(y), Type(Line) {}
        Edge(SInt cx, SInt cy, SInt ax, SInt ay): Cx(cx), Cy(cy), Ax(ax), Ay(ay), Type(Curve) {}

        bool IsLine()  { return Type == Line; }
        bool IsCurve() { return Type == Curve; }
    };
    GArray<Edge>    Edges;
    UInt            EdgesIndex;
    UInt            CurvesNum;
    UInt            LinesNum;
    UByte           EdgesNumBits;
    bool            NewShape;
};


GINLINE UInt GFx_GetFloatExponent(Float scale)
{
    union { Float f; UInt32 i; } u;
    u.f = scale;
    return ((u.i >> 23) & 0xFF) - 64;
}


GINLINE UInt GFx_ComputeShapeScaleKey(Float scale, bool useEdgeAA)
{
    // Compute the scaleKey. It is calculated empirically, as an approximate
    // binary logarithm of the overall scale (world-to-pixels). 
    // The direct use of the scale produces a new key for approximately 
    // every 2x scale value. scale*0.707 is a bias, which means the use
    // of the same scale key from 0.707 to 1.41:
    //
    // return GFx_GetFloatExponent(scale*0.707f);              // Scales 0.707...1.41
    //
    // For more accurate EdgeAA use:
    // ts2 = scale * 0.841; GFx_GetFloatExponent(ts2*ts2);     // Scales 0.841...1.189
    // ts2 = scale * 0.891; GFx_GetFloatExponent(ts2*ts2*ts2); // Scales 0.891...1.122
    //-------------------
    if (useEdgeAA)
    {
        Float ts2 = scale * 0.841f;
        return GFx_GetFloatExponent(ts2*ts2);
//        return GFx_GetFloatExponent(scale*0.707f);
    }

    // When EdgeAA is not used, the accuracy requirements are less strict. 
    // It is good enough to have a mesh for every x4 scaling value.
    //-------------------
    return  GFx_GetFloatExponent(scale) / 2;
}

class  GFxMeshCache;
struct GFxCachedMeshSetBag;

// This is the base shape definition class that provides basic
// shape functionality. This is an abstract class that should be
// inherited and pure virtual functions should be implemented.
class GFxShapeBase : public GNewOverrideBase<GFxStatMD_CharDefs_Mem>
{
public:
    // Thread-Safe ref-count implementation.
    void        AddRef();  
    void        Release();
    SInt32      GetRefCount() const { return RefCount; }

public:
    enum
    {
        Flags_TexturedFill       = 1,
        Flags_Sfactor20          = 2,
        Flags_LocallyAllocated   = 4,
        Flags_NonZeroFill        = 8,
        Flags_ValidBounds        = 0x10,

        Flags_StylesSupport      = 0x40,
        Flags_S9GSupport         = 0x80
    };
    enum PathDataFormatType
    {
        PathData_Swf,
        PathData_PathPacker
    };

    GFxShapeBase();
    virtual         ~GFxShapeBase();

    void            ResetCache();

    void            Display(GFxDisplayContext &context, GFxCharacter* inst);
    void            Display(GFxDisplayParams &params,
                            bool  edgeAADisabled,
                            GFxCharacter* inst);

    GRectF          GetBoundsLocal() const { return Bound; }
    
    // These methods are implemented only in shapes with styles, i.e.
    // it is not needed for glyph shapes.
    virtual GRectF  GetRectBoundsLocal() const { return GFxShapeBase::GetBoundsLocal(); }
    virtual void    SetRectBoundsLocal(const GRectF&) {}

    //bool            PointInShape(const GCompoundShape& shape, Float x, Float y) const;

    template<class PathData, class PathIterator>
    bool            PointInShape(PathIterator& it, const GFxScale9GridInfo* s9g, Float x, Float y) const;


    // Determine texture 9-grid style if exists
    int             GetTexture9GridStyle(const GCompoundShape& cs) const;

    void            AddShapeToMesh(GFxMeshSet *meshSet, GCompoundShape* cs, 
                                   GFxDisplayContext &context, 
                                   GFxScale9GridInfo* s9g) const;

    const GRectF&   GetBound() const { return Bound; }

    virtual bool    DefPointTestLocal(const GPointF &pt, bool testShape = 0, const GFxCharacter *pinst = 0) const = 0;

    // Push our shape data through the tessellator to mesh.
    virtual void    Tessellate(GFxMeshSet *p, Float tolerance,
                               GFxDisplayContext &renderInfo,
                               GFxScale9GridInfo* s9g) const = 0;

    // calculate exact bounds, since Bounds may contain null or inexact info
    virtual void    ComputeBound(GRectF* r) const = 0;

    // Convert the paths to GCompoundShape, flattening the curves.
    virtual void    MakeCompoundShape(GCompoundShape *cs, Float tolerance) const = 0;

    virtual UInt32  ComputeGeometryHash() const = 0;
    virtual bool    IsEqualGeometry(const GFxShapeBase& cmpWith) const = 0;

    // Stubs for work with fill & line styles
    virtual const GFxFillStyle* GetFillStyles(UInt* pstylesNum) const;
    virtual const GFxLineStyle* GetLineStyles(UInt* pstylesNum) const;
    virtual void                GetFillAndLineStyles(const GFxFillStyle**, UInt* pfillStylesNum, 
        const GFxLineStyle**, UInt* plineStylesNum) const;
    virtual void                GetFillAndLineStyles(GFxDisplayParams*) const;

    // Maximum computed distance from bounds due to stroke.
    virtual void    SetMaxStrokeExtent(Float) {}    
    virtual Float   GetMaxStrokeExtent() const { return 100.f; }

    // Morph uses this
    void            SetBound(const GRectF& r)       { Bound = r; } // should do some verifying?
    void            SetValidBoundsFlag(bool flag)   { (flag) ? Flags |= Flags_ValidBounds : Flags &= ~Flags_ValidBounds; }
    bool            HasValidBounds() const          { return (Flags & Flags_ValidBounds) != 0; }

    void            SetNonZeroFill(bool flag) 
    { 
        if (flag) Flags |= Flags_NonZeroFill; else Flags &= ~Flags_NonZeroFill;
    }
    bool            IsNonZeroFill() const   { return (Flags & Flags_NonZeroFill) != 0; }

    bool            HasTexturedFill() const { return Flags & Flags_TexturedFill; }

    static void     ApplyScale9Grid(GCompoundShape* shape, const GFxScale9GridInfo& sg);

    void    SetHintedGlyphSize(UInt s) { HintedGlyphSize = UByte(s); }
    UInt    GetHintedGlyphSize() const { return HintedGlyphSize; }

    virtual const UByte* GetPathData()     const = 0;
    virtual UInt         GetPathDataType() const = 0; // see PathDataFormatType enum
    virtual void         GetShapeAndPathCounts(UInt* pshapesCnt, UInt* ppathsCnt) const = 0;
    UInt                 GetFlags() const        { return Flags; }

    GFxCachedMeshSetBag* GetMeshSetBag() const   { return pMeshSetBag; }
    void                 SetMeshSetBag(GFxCachedMeshSetBag* bag, GFxMeshCache* cache)
    { 
        pMeshSetBag = bag; 
        pMeshCache = cache; 
    }

public:
    // implementation of public virtual iterators. This implementation is
    // necessary if the path data format is unknown or may be mixed. 
    // Works slower than native iterators (GFxPathDats::PathsIterator & 
    // GFxSwfPathData::PathsIterator) because of virtual calls.
    class PathsIterator : public GRefCountBaseNTS<PathsIterator, GStat_Default_Mem>
    {
    public:
        struct StateInfo
        {
            enum StateT
            {
                St_Setup,
                St_NewShape,
                St_NewPath,
                St_Edge,
                St_Finished
            } State;
            union
            {
                struct // State == St_Setup
                {
                    Float MoveX, MoveY;
                    UInt  Fill0, Fill1, Line;
                } Setup;
                struct  // State == St_Edge 
                {
                    Float Cx, Cy, Ax, Ay;
                    bool  Curve;
                } Edge;
            };
        };
        virtual bool GetNext(StateInfo* pcontext) = 0;
    };
    virtual PathsIterator* GetPathsIterator() const = 0;
protected:
    template<class PathData>
    bool            DefPointTestLocalImpl(const GPointF &pt, bool testShape = 0, const GFxCharacter *pinst = 0) const;

    template<class PathData>
    void            TessellateImpl(GFxMeshSet *p, Float tolerance,
                                   GFxDisplayContext &renderInfo,
                                   GFxScale9GridInfo* s9g,
                                   Float maxStrokeExtent = 100.f) const;

    template<class PathData>
    void            PreTessellateImpl(Float masterScale, 
                                      const GFxRenderConfig& config,
                                      Float maxStrokeExtent = 100.f);

    template<class PathData>
    void            ComputeBoundImpl(GRectF* r) const;

    // Convert the paths to GCompoundShape, flattening the curves.
    template<class PathData>
    void            MakeCompoundShapeImpl(GCompoundShape *cs, Float tolerance) const;

    template<class PathData>
    UInt32          ComputeGeometryHashImpl() const;

    template<class PathData>
    bool            IsEqualGeometryImpl(const GFxShapeBase& cmpWith) const;

protected:
    GRectF                      Bound;
    GAtomicInt<SInt32>          RefCount;

    GFxMeshCache*               pMeshCache;
    GFxCachedMeshSetBag*        pMeshSetBag;
    GFxMeshSet*                 pPreTessMesh;

    mutable UByte               Flags; // See enum Flags_...
    // If the shape represents a natively hinted glyph, this size can 
    // be used to indicate the actual glyph size in pixels.
    // In all other cases it must be zero.
    UByte                       HintedGlyphSize;
};

// An implementation of shape without styles and without
// ability to be dynamically created. Useful for pre-loaded
// font glyph shapes. This shape doesn't have path packer and
// uses Flash native shape format.

class GFxConstShapeNoStyles : public GFxShapeBase
{
public:
    GFxConstShapeNoStyles() : pPaths(NULL) {}

    void            Read(GFxLoadProcess* p, GFxTagType tagType, UInt lenInBytes, bool withStyle, GFxPathAllocator* pAllocator = NULL);

    virtual void    ComputeBound(GRectF* r) const;
    virtual bool    DefPointTestLocal(const GPointF &pt, bool testShape = 0, const GFxCharacter *pinst = 0) const;

    // Convert the paths to GCompoundShape, flattening the curves.
    virtual void    MakeCompoundShape(GCompoundShape *cs, Float tolerance) const;

    virtual UInt32  ComputeGeometryHash() const;
    virtual bool    IsEqualGeometry(const GFxShapeBase& cmpWith) const;

    // Push our shape data through the tessellator to mesh.
    virtual void    Tessellate(GFxMeshSet *p, Float tolerance,
                               GFxDisplayContext &renderInfo,
                               GFxScale9GridInfo* s9g) const;

    virtual PathsIterator* GetPathsIterator() const;

    GFxSwfPathData::PathsIterator GetNativePathsIterator() const
    { 
        return GFxSwfPathData::PathsIterator(this); 
    }

    virtual const UByte* GetPathData() const { return pPaths; }
    virtual UInt         GetPathDataType() const { return PathData_Swf; }
    const UByte*         GetNativePathData() const { return pPaths; }
    virtual void         GetShapeAndPathCounts(UInt* pshapesCnt, UInt* ppathsCnt) const;
protected:
    void                 Read(GFxLoadProcess* p, GFxTagType tagType, UInt lenInBytes, 
                              bool withStyle, GFxFillStyleArrayTemp*, GFxLineStyleArrayTemp*,
                              GFxPathAllocator* pAllocator = NULL);
protected:
    UByte*          pPaths;
};

// An implementation of shape without styles but the one that could 
// be dynamically created. Useful for system font glyph shapes which
// are created by the system font provider.

class GFxShapeNoStyles : public GFxShapeBase
{
public:
    GFxShapeNoStyles(UInt pageSize = GFxPathAllocator::Default_PageBufferSize);

    // calculate exact bounds, since Bounds may contain null or inexact info
    virtual void    ComputeBound(GRectF* r) const;
    virtual bool    DefPointTestLocal(const GPointF &pt, bool testShape = 0, const GFxCharacter *pinst = 0) const;

    // Convert the paths to GCompoundShape, flattening the curves.
    virtual void    MakeCompoundShape(GCompoundShape *cs, Float tolerance) const;

    virtual UInt32  ComputeGeometryHash() const;
    virtual bool    IsEqualGeometry(const GFxShapeBase& cmpWith) const;

    // Push our shape data through the tessellator to mesh.
    virtual void    Tessellate(GFxMeshSet *p, Float tolerance,
                               GFxDisplayContext &renderInfo,
                               GFxScale9GridInfo* s9g) const;

    // methods for dynamic shape creation
    void            StartNewShape()
    {
        GFxPathPacker p;
        p.SetNewShape();
        p.Pack(&PathAllocator, &Paths);
    }

    void            AddPath(GFxPathPacker* path)
    {
        if (!path->IsEmpty())
        {
            path->Pack(&PathAllocator, &Paths);
        }
    }

    GFxPathData::PathsIterator GetNativePathsIterator() const
    { 
        return GFxPathData::PathsIterator(this); 
    }

    virtual PathsIterator* GetPathsIterator() const;

    virtual const UByte*   GetPathData() const { return (const UByte*)&Paths; }
    virtual UInt           GetPathDataType() const { return PathData_PathPacker; }
    const GFxPathData::PathsInfo* GetNativePathData() const { return &Paths; }
    virtual void           GetShapeAndPathCounts(UInt* pshapesCnt, UInt* ppathsCnt) const;
protected:
    GFxPathAllocator       PathAllocator;
    GFxPathData::PathsInfo Paths;
};

// An implementation of shape with styles but without
// ability to be dynamically created. Useful for pre-loaded
// Flash shapes. This shape doesn't have path packer and
// uses Flash native shape format.

class GFxConstShapeWithStyles : public GFxConstShapeNoStyles
{
    friend class GFxConstShapeCharacterDef;
public:
    GFxConstShapeWithStyles();
    ~GFxConstShapeWithStyles();

    void          Read(GFxLoadProcess* p, GFxTagType tagType, UInt lenInBytes, bool withStyle);

    virtual GRectF GetRectBoundsLocal() const           { return RectBound; }
    virtual void   SetRectBoundsLocal(const GRectF& r)  { RectBound = r; }

    // Push our shape data through the tessellator to mesh.
    virtual void    Tessellate(GFxMeshSet *p, Float tolerance,
                               GFxDisplayContext &renderInfo,
                               GFxScale9GridInfo* s9g) const;

    virtual const GFxFillStyle* GetFillStyles(UInt* pstylesNum) const
    {
        *pstylesNum = FillStylesNum;
        return pFillStyles;
    }
    virtual const GFxLineStyle* GetLineStyles(UInt* pstylesNum) const
    {
        *pstylesNum = LineStylesNum;
        return pLineStyles;
    }
    virtual void GetFillAndLineStyles(const GFxFillStyle** ppfillStyles, UInt* pfillStylesNum, 
        const GFxLineStyle** pplineStyles, UInt* plineStylesNum) const
    {
        *ppfillStyles   = pFillStyles;
        *pfillStylesNum = FillStylesNum;
        *pplineStyles   = pLineStyles;
        *plineStylesNum = LineStylesNum;
    }
    virtual void GetFillAndLineStyles(GFxDisplayParams* pparams) const
    {
        pparams->pFillStyles = pFillStyles;
        pparams->pLineStyles = pLineStyles;
        pparams->FillStylesNum = FillStylesNum;
        pparams->LineStylesNum = LineStylesNum;
    }

    bool IsEmpty() const 
    { 
        return FillStylesNum == 0 && LineStylesNum == 0;
    }

    // Maximum computed distance from bounds due to stroke.
    virtual void  SetMaxStrokeExtent(Float m) { MaxStrokeExtent = m; }    
    virtual Float GetMaxStrokeExtent() const { return MaxStrokeExtent; }

protected:
    const GFxFillStyle*         pFillStyles;
    const GFxLineStyle*         pLineStyles;
    UInt32                      FillStylesNum; 
    UInt32                      LineStylesNum;
    Float                       MaxStrokeExtent;    // Maximum computed distance from bounds due to stroke.
    GRectF                      RectBound; // Smaller bounds without stroke, SWF 8 (!AB never used!)
};

// An implementation of shape with styles and with
// ability to be dynamically created. Useful for DrawingAPI, 
// uses our own path packer.

class GFxShapeWithStyles : public GFxShapeNoStyles
{
    friend class GFxShapeCharacterDef;
public:
    GFxShapeWithStyles(UInt pageSize = GFxPathAllocator::Default_PageBufferSize);
    ~GFxShapeWithStyles();

    virtual GRectF GetRectBoundsLocal() const           { return RectBound; }
    virtual void   SetRectBoundsLocal(const GRectF& r)  { RectBound = r; }

    // Push our shape data through the tessellator to mesh.
    virtual void    Tessellate(GFxMeshSet *p, Float tolerance,
                               GFxDisplayContext &renderInfo,
                               GFxScale9GridInfo* s9g) const;

    virtual const GFxFillStyle* GetFillStyles(UInt* pstylesNum) const
    {
        *pstylesNum = (UInt)FillStyles.GetSize();
        return ((UInt)FillStyles.GetSize() > 0) ? &FillStyles[0] : NULL;
    }
    virtual const GFxLineStyle* GetLineStyles(UInt* pstylesNum) const
    {
        *pstylesNum = (UInt)LineStyles.GetSize();
        return ((UInt)LineStyles.GetSize() > 0) ? &LineStyles[0] : NULL;
    }
    virtual void GetFillAndLineStyles(const GFxFillStyle** ppfillStyles, UInt* pfillStylesNum, 
        const GFxLineStyle** pplineStyles, UInt* plineStylesNum) const
    {
        *pfillStylesNum = (UInt)FillStyles.GetSize();
        *plineStylesNum = (UInt)LineStyles.GetSize();
        *ppfillStyles   = ((UInt)FillStyles.GetSize() > 0) ? &FillStyles[0] : NULL;
        *pplineStyles   = ((UInt)LineStyles.GetSize() > 0) ? &LineStyles[0] : NULL;
    }
    virtual void GetFillAndLineStyles(GFxDisplayParams* pparams) const
    {
        pparams->FillStylesNum = (UInt)FillStyles.GetSize();
        pparams->LineStylesNum = (UInt)LineStyles.GetSize();
        pparams->pFillStyles = (pparams->FillStylesNum > 0) ? &FillStyles[0] : NULL;
        pparams->pLineStyles = (pparams->LineStylesNum > 0) ? &LineStyles[0] : NULL;
    }

    // methods for dynamic shape creation
    UInt AddFillStyle(const GFxFillStyle& style)
    {
        FillStyles.PushBack(style);
        return (UInt)FillStyles.GetSize();
    }

    GFxFillStyle&       GetFillStyleAt(UPInt idx)       { return FillStyles[idx]; }
    const GFxFillStyle& GetFillStyleAt(UPInt idx) const { return FillStyles[idx]; }
    UInt                GetNumFillStyles() const { return (UInt)FillStyles.GetSize(); }
    const GFxFillStyle& GetLastFillStyle() const { return FillStyles[FillStyles.GetSize() - 1]; }
    GFxFillStyle&       GetLastFillStyle()       { return FillStyles[FillStyles.GetSize() - 1]; }
    GFxFillStyleArray&  GetFillStyleArray()      { return FillStyles; }
    const GFxFillStyleArray&  GetFillStyleArray() const { return FillStyles; }

    // Creates a definition relying on in-memory image.
    void                SetToImage(GFxImageResource *pimage, bool bilinear);

    UInt                AddLineStyle(const GFxLineStyle& style)
    {
        LineStyles.PushBack(style);
        return (UInt)LineStyles.GetSize();
    }

    GFxLineStyle&       GetLineStyleAt(UPInt idx)       { return LineStyles[idx]; }
    const GFxLineStyle& GetLineStyleAt(UPInt idx) const { return LineStyles[idx]; }
    UInt                GetNumLineStyles() const { return (UInt)LineStyles.GetSize(); }
    const GFxLineStyle& GetLastLineStyle() const { return LineStyles[LineStyles.GetSize() - 1]; }
    GFxLineStyle&       GetLastLineStyle()       { return LineStyles[LineStyles.GetSize() - 1]; }
    GFxLineStyleArray&  GetLineStyleArray()      { return LineStyles; }
    const GFxLineStyleArray&  GetLineStyleArray() const { return LineStyles; }

    bool                IsEmpty() const 
    { 
        return FillStyles.GetSize() == 0 && LineStyles.GetSize() == 0;
    }

    // Maximum computed distance from bounds due to stroke.
    virtual void        SetMaxStrokeExtent(Float m) { MaxStrokeExtent = m; }    
    virtual Float       GetMaxStrokeExtent() const { return MaxStrokeExtent; }
protected:
    GFxPathAllocator            PathAllocator;
    GFxFillStyleArray           FillStyles;
    GFxLineStyleArray           LineStyles;
    Float                       MaxStrokeExtent;    // Maximum computed distance from bounds due to stroke.
    GRectF                      RectBound; // Smaller bounds without stroke, SWF 8 (!AB never used!)
};

//////////////////////////////// GFxCharacterDef implementations ///////////////////////////////////
// A base abstract class for shape character definition. 
// Implemented by GFxShapeCharacterDef and GFxConstShapeCharacterDef

class GFxShapeBaseCharacterDef : public GFxCharacterDef
{
public:
    GFxShapeBaseCharacterDef() {}
    virtual ~GFxShapeBaseCharacterDef() {}

    typedef GFxShapeBase::PathsIterator PathsIterator;

    virtual void    Display(GFxDisplayContext &context, GFxCharacter* inst) = 0;
    //virtual void    Display(GFxDisplayParams &params,
    //                        bool  edgeAADisabled,
    //                        GFxCharacter* inst) = 0;
    virtual GRectF  GetBoundsLocal() const = 0;

    // These methods are implemented only in shapes with styles, i.e.
    // it is not needed for glyph shapes.
    virtual GRectF  GetRectBoundsLocal() const = 0;
    virtual void    SetRectBoundsLocal(const GRectF&) = 0;

    virtual void    PreTessellate(Float masterScale, const GFxRenderConfig& config) = 0;
    // calculate exact bounds, since Bounds may contain null or inexact info
    virtual void    ComputeBound(GRectF* r) const = 0;

    // Convert the paths to GCompoundShape, flattening the curves.
    virtual void    MakeCompoundShape(GCompoundShape *cs, Float tolerance) const = 0;

    virtual UInt32  ComputeGeometryHash() const = 0;
    virtual bool    IsEqualGeometry(const GFxShapeBaseCharacterDef& cmpWith) const = 0;

    // Query Resource type code, which is a combination of ResourceType and ResourceUse.
    virtual UInt    GetResourceTypeCode() const     { return MakeTypeCode(RT_ShapeDef); }

    virtual GFxShapeBase*       GetShape() = 0;
    virtual const GFxShapeBase* GetShape() const = 0;
};

// An implementation of non-const shape character definiton.
// Non-const shape is used with DrawingAPI

class GFxShapeCharacterDef : public GFxShapeBaseCharacterDef
{
public:
    typedef GFxShapeBase::PathsIterator PathsIterator;

    virtual void    Display(GFxDisplayContext &context, GFxCharacter* inst);

    virtual GRectF  GetBoundsLocal() const { return Shape.GetBoundsLocal(); }

    // These methods are implemented only in shapes with styles, i.e.
    // it is not needed for glyph shapes.
    virtual GRectF  GetRectBoundsLocal() const { return Shape.GetRectBoundsLocal(); }
    virtual void    SetRectBoundsLocal(const GRectF& r) { Shape.SetRectBoundsLocal(r); }

    virtual void    ComputeBound(GRectF* r) const;
    virtual bool    DefPointTestLocal(const GPointF &pt, bool testShape = 0, 
                                      const GFxCharacter *pinst = 0) const;

    // Convert the paths to GCompoundShape, flattening the curves.
    virtual void    MakeCompoundShape(GCompoundShape *cs, Float tolerance) const;

    virtual UInt32  ComputeGeometryHash() const;
    virtual bool    IsEqualGeometry(const GFxShapeBaseCharacterDef& cmpWith) const;

    virtual void    PreTessellate(Float masterScale, const GFxRenderConfig& config);

    virtual PathsIterator* GetPathsIterator() const;

    virtual const UByte* GetPathData() const        { return Shape.GetPathData(); }
    virtual UInt         GetPathDataType() const    { return Shape.GetPathDataType(); }
    virtual void         GetShapeAndPathCounts(UInt* pshapesCnt, UInt* ppathsCnt) const
    {
        Shape.GetShapeAndPathCounts(pshapesCnt, ppathsCnt);
    }
    virtual GFxShapeBase*       GetShape() { return &Shape; }
    virtual const GFxShapeBase* GetShape() const { return &Shape; }

    const GRectF&   GetBound() const { return Shape.GetBound(); }

    // Creates a definition relying on in-memory image.
    void    SetToImage(GFxImageResource *pimage, bool bilinear);

    void    SetNonZeroFill(bool flag) 
    { 
        Shape.SetNonZeroFill(flag);
    }
    bool    IsNonZeroFill() const { return Shape.IsNonZeroFill(); }
protected:
    GFxShapeWithStyles Shape;
};

// An implementation of constant shape character definition. 
// Constant shape is coming from swf files.

class GFxConstShapeCharacterDef : public GFxShapeBaseCharacterDef
{
public:
    typedef GFxShapeBase::PathsIterator PathsIterator;

    virtual void    Display(GFxDisplayContext &context, GFxCharacter* inst);

    virtual GRectF  GetBoundsLocal() const { return Shape.GetBoundsLocal(); }

    // These methods are implemented only in shapes with styles, i.e.
    // it is not needed for glyph shapes.
    virtual GRectF  GetRectBoundsLocal() const { return Shape.GetRectBoundsLocal(); }
    virtual void    SetRectBoundsLocal(const GRectF& r) { Shape.SetRectBoundsLocal(r); }

    virtual void    ComputeBound(GRectF* r) const;
    virtual bool    DefPointTestLocal(const GPointF &pt, bool testShape = 0, 
                                      const GFxCharacter *pinst = 0) const;

    // Convert the paths to GCompoundShape, flattening the curves.
    virtual void    MakeCompoundShape(GCompoundShape *cs, Float tolerance) const;

    virtual UInt32  ComputeGeometryHash() const;
    virtual bool    IsEqualGeometry(const GFxShapeBaseCharacterDef& cmpWith) const;

    virtual void    PreTessellate(Float masterScale, const GFxRenderConfig& config);

    virtual PathsIterator* GetPathsIterator() const;

    virtual const UByte* GetPathData() const        { return Shape.GetPathData(); }
    virtual UInt         GetPathDataType() const    { return Shape.GetPathDataType(); }
    virtual void         GetShapeAndPathCounts(UInt* pshapesCnt, UInt* ppathsCnt) const
    {
        Shape.GetShapeAndPathCounts(pshapesCnt, ppathsCnt);
    }
    virtual GFxShapeBase*       GetShape() { return &Shape; }
    virtual const GFxShapeBase* GetShape() const { return &Shape; }

    const GRectF&       GetBound() const { return Shape.GetBound(); }
    void                Read(GFxLoadProcess* p, GFxTagType tagType, 
                             UInt lenInBytes, bool withStyle)
    {
        Shape.Read(p, tagType, lenInBytes, withStyle);
    }
protected:
    GFxConstShapeWithStyles Shape;
};

GINLINE bool GFxSwfPathData::EdgesIterator::IsFinished() const
{
    return pPathIter->IsFinished () || !(pPathIter->Record & PathsIterator::Rec_EdgeMask); 
}
#endif // INC_GFXSHAPE_H

