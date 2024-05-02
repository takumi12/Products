/**********************************************************************

Filename    :   GTessellator.h
Content     :   An optimal Flash compound shape tessellator
Created     :   2005-2006
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

For information regarding Commercial License Agreements go to:
online - http://www.scaleform.com/licensing.html or
email  - sales@scaleform.com 

**********************************************************************/

#ifndef INC_GTESSELLATORDECL_H
#define INC_GTESSELLATORDECL_H

#include "GArrayUnsafe.h"
#include "GArrayPaged.h"
#include "GAlgorithm.h"
#include "GCompoundShape.h"

const GCoordType G_TessellatorEpsilon  =  1e-5f;
const GCoordType G_TessellatorMinCoord = -1e30f;
const GCoordType G_TessellatorMaxCoord =  1e30f;

//-----------------------------------------------------------------------
class GTessellator
{
public:
    enum { SID = GStatRG_Tessellator_Mem };

    typedef GCoordType CoordType;
    typedef GPointType VertexType;

    enum FillRuleType
    {
        FillNonZero,
        FillEvenOdd
    };

private:
    enum EdgeFlags_e
    {
        EndChainFlag   = 1 << 1,
        VisibleChain   = 1 << 2,
        EventFlag      = 1 << 3
    };

    enum ScanbeamFlags_e
    {
        InsertEdgesFlag = 1,
        RemoveEdgesFlag = 2
    };

    enum ChainFlagAtScanline_e
    {
        ChainContinuesAtScanline = 1,
        ChainStartsAtScanline    = 2,
        ChainEndsAtScanline      = 3
    };

    struct PathInfoType
    {
        int      start;       // Starting index in Vertices[]
        int      end;         // Ending index in Vertices[]
        unsigned leftStyle;   // Original left style
        unsigned rightStyle;  // Original right style
    };

    struct EdgeType
    {
        unsigned  lower;  // Edge lower point (global index)
        unsigned  upper;  // Edge upper point (global index)
        CoordType slope;  // Change in X for a unit Y increase
        EdgeType* next;   // Edge connected at the upper end
    };

    struct MonoChainType
    {
        EdgeType*  edge;       // Current edge during monotonization
        CoordType  ySort;      // Y coordinate to sort
        CoordType  xb;         // Scanbean bottom X during monotonization
        CoordType  xt;         // Scanbeam top X during monotonization

        int        dir;        // Edge direction, +1=up, -1=down
        unsigned   leftStyle;  // Original left style
        unsigned   rightStyle; // Original right style
        unsigned   leftBelow;  // Left style below scan line
        unsigned   leftAbove;  // Left style above scan line
        unsigned   rightBelow; // Right style below scan line
        unsigned   rightAbove; // Right style above scan line

        unsigned   flags;
        unsigned   posIntr;    // Position in the ActiveChains table at 
                               // the bottom of the slab, before perceiving 
                               // the intersections

        unsigned   posScan;    // Position in the scan-beam
    };

    struct IntersectionType
    {
        unsigned  pos1;
        unsigned  pos2;
        CoordType y;
    };

    struct BaseLineType
    {
        CoordType y;
        unsigned  styleLeft;    // Style on the left of the trapezoid
        unsigned  vertexLeft;   // Opposite left vertex of the trapezoid (if any)
        unsigned  vertexRight;  // Opposite right vertex of the trapezoid (if any)
        unsigned  firstChain;   // Index of the first chain in the array
        unsigned  numChains;    // Number of chains in the array
        unsigned  leftAbove;    // Index of the chain on the left in the array (if any)
    };

public:
    struct MonoVertexType
    {
        unsigned        vertex;
        MonoVertexType* next;
    };

    struct MonotoneType
    {
        MonoVertexType* start;
        unsigned        lastVertex;
        unsigned        prevVertex1;
        unsigned        prevVertex2;
        unsigned        style;
        BaseLineType*   lowerBase;
    };

    struct TriangleType
    {
        unsigned v1, v2, v3;
    };

private:
    struct PendingEndType
    {
        unsigned      vertex;
        MonotoneType* monotone;
    };

    struct ScanChainType
    {
        MonoChainType* chain;
        MonotoneType*  monotone;
        unsigned       vertex;
    };

    enum Coherence_e
    {
        CoherenceUndefined  = 0,
        CoherencePincushion = 1,
        CoherenceConvex     = 2,
        CoherenceLeftCurve  = 3,
        CoherenceRightCurve = 4
    };

public:
    //-----------------------------------
    GTessellator() : 
        FillRule(FillNonZero),
        OptimizeDirection(true),
        OptimizeCoherence(true),
        OptimizeMonotones(true),
        PilotTriangulation(true),
        SwapCoordinates(false),
        HistHor(),
        HistVer(),
        VisibleStyles(),
        Vertices(),
        Paths(),
        Edges(),
        MonoChains(),
        MonoChainsSorted(),
        Scanbeams(),
        ActiveChains(),
        InteriorChains(),
        ValidChains(),
        InteriorOrder(),
        Intersections(),
        StyleCounts(),
        MinStyle(0x3FFFFFFF),
        MaxStyle(0),
        MinX(G_TessellatorMaxCoord),
        MinY(G_TessellatorMaxCoord),
        MaxX(G_TessellatorMinCoord),
        MaxY(G_TessellatorMinCoord),
        EpsilonX(0),
        EpsilonY(0),
        SwapEdgesTable(),
        EventVertices(),
        ChainsBelow(),
        ChainsAbove(),
        BaseLines(),
        PendingEnds(),
        Monotones(),
        MonoVertices(),
        MonoStack(),
        Triangles(),

        LeftVertices(),
        RightVertices(),
        ConvexShape(),
        TrianglesCutOff(0),
        ShapeCoherence(CoherenceUndefined),
        LeftCoherence(CoherenceUndefined),
        RightCoherence(CoherenceUndefined)
    {}

    // Setup (optional)
    //-----------------------------------

    // FillRule = FillNonZero by default
    void            SetFillRule(FillRuleType f)     { FillRule = f; }
    FillRuleType    GetFillRule() const             { return FillRule; }

    // DirOptimization = true by default
    void            SetDirOptimization(bool v)      { OptimizeDirection = v; }
    bool            GetDirOptimization() const      { return OptimizeDirection; }

    void            SetStyleVisibility(unsigned style, bool visible);
    bool            GetStyleVisibility(unsigned style) const;

    void            SetMonotoneOptimization(bool v) { OptimizeMonotones = v; }
    bool            GetMonotoneOptimization() const { return OptimizeMonotones; }

    void            SetCoherenceOptimization(bool v) { OptimizeCoherence = v; }
    bool            GetCoherenceOptimization() const { return OptimizeCoherence; }

    void            SetPilotTriangulation(bool v)   { PilotTriangulation = v; }
    bool            GetPilotTriangulation() const   { return PilotTriangulation; }

    // Tessellation
    //-----------------------------------
    UPInt GetNumBytes() const;
    void  Clear();
    void  ClearAndRelease();
    void  AddShape(const GCompoundShape& shape, int addStyle=0);
    void  Monotonize();

    void  Monotonize(const GCompoundShape& shape, int addStyle=0);
    void  SortMonotonesByStyle();
    void  TriangulateMonotone(unsigned idx);

    UInt                GetNumVertices() const          { return (UInt)EventVertices.GetSize(); }
    const VertexType&   GetVertex(UPInt idx) const      { return EventVertices[idx]; }

    UInt                GetNumMonotones() const         { return (UInt)Monotones.GetSize(); }
    const MonotoneType& GetMonotone(UPInt i) const      { return Monotones[i]; }

    UInt                GetNumTriangles() const         { return (UInt)Triangles.GetSize(); }
    const TriangleType& GetTriangle(UPInt i) const      { return Triangles[i]; }

    // Auxiliary
    //-----------------------------------
    UInt               GetNumMonoChains() const         { return (UInt)MonoChains.GetSize(); }
    const MonoChainType& GetMonoChain(UInt i) const     { return MonoChains[i]; }

    UInt                GetNumEdges() const             { return (UInt)Edges.GetSize(); }
    const EdgeType&     GetEdge(UInt i) const           { return Edges[i]; }

    enum { LeftMask = 0x40000000U };
    static bool isLeft(unsigned v) { return (v & LeftMask) != 0; }

private:
    typedef GArrayPagedLH_POD<VertexType, 10, 64, SID>      VertexStorageType;
    typedef GArrayUnsafeLH_POD<MonoChainType*, SID>         ChainPtrStorage;

    typedef GArrayUnsafeLH_POD<ScanChainType, SID>          ScanChainArrayType;
    typedef GArrayPagedLH_POD<PendingEndType, 4, 16, SID>   PendingEndArrayType;


    static bool monoChainLess(const MonoChainType* a, const MonoChainType* b);
    static bool styleLess(const MonotoneType& a, const MonotoneType& b);
    static bool activeChainLess(const MonoChainType* a, const MonoChainType* b);
    static bool intersectionLess(const IntersectionType& a, const IntersectionType& b);

    struct CmpScanbeams
    {
        const VertexStorageType& Vertices;
        CmpScanbeams(const VertexStorageType& v) : Vertices(v) {}
        bool operator () (unsigned a, unsigned b)
        {
            return Vertices[a].y < Vertices[b].y;
        }
    private: 
        const CmpScanbeams& operator = (const CmpScanbeams&);
    };

    bool forwardMin(int idx, int start) const;
    bool reverseMin(int idx, int end) const;
    bool forwardMax(int idx, int end) const;
    bool reverseMax(int idx, int start) const;

    void buildEdgeList(unsigned start, 
                       unsigned numEdges, 
                       int step,
                       unsigned leftStyle, 
                       unsigned rightStyle);

    void addPath(const GCompoundShape::SPath& path, int addStyle);
    void decomposePath(const PathInfoType& path);
    void optimizeDirection();
    void prepareChainsAndScanbeams();
    
    void setStyleBounds(const MonoChainType* mc);
    void addStyles(const MonoChainType* mc);
    int  findElderStyle() const;
    void perceiveStyles(const ChainPtrStorage& aet);

    CoordType calcX(const EdgeType* edge, CoordType yt) const;

    unsigned nextScanbeam(CoordType yb, CoordType yt, 
                          unsigned startMc, unsigned numMc);

    void setupIntersections();

    unsigned addEventVertex(const VertexType& v);
    unsigned addEventVertex(const MonoChainType* mc, CoordType yb, bool enforceFlag);

    void addPendingEnd(ScanChainType* dst, ScanChainType* pending, CoordType y);

    unsigned lastMonoVertex(const MonotoneType* m) const;
    void     removeLastMonoVertex(MonotoneType* m);

    static void resetMonotone(MonotoneType* m, unsigned style);
    MonotoneType* startMonotone(unsigned style);
    void startMonotone(ScanChainType* chain, unsigned vertex);
    void replaceMonotone(ScanChainType* chain, unsigned style);
    void replaceMonotone(PendingEndType* pe, unsigned style);

    static int pendingMonotoneStyle(const PendingEndType* pe)
    {
        return pe->monotone ? pe->monotone->style : 0;
    }

    static int startingMonotoneStyle(const ScanChainType* chain)
    {
        return chain->chain->rightAbove;
    }

    void connectPendingToLeft    (ScanChainType* chain, unsigned targetVertex);
    void connectPendingToRight   (ScanChainType* chain, unsigned targetVertex);
    void connectStartingToPending(ScanChainType* chain, BaseLineType* upperBase);
    void connectStartingToLeft   (ScanChainType* chain, BaseLineType* upperBase, unsigned targetVertex);
    void connectStartingToRight  (ScanChainType* chain, BaseLineType* upperBase, unsigned targetVertex);
    void connectStarting         (ScanChainType* chain, BaseLineType* upperBase);

    void growMonotone(MonotoneType* m, unsigned vertex);
    void growMonotone(MonotoneType* m, unsigned left, unsigned right);
    void growMonotoneAndConnect(ScanChainType* chain, unsigned vertex, bool enforceConnection);

    unsigned nextChainInBundle(unsigned below, unsigned above, unsigned vertex) const;
    void sweepScanbeam(const ChainPtrStorage& aet, CoordType yb);

    void swapChains(unsigned startIn, unsigned endIn);
    void processInterior(CoordType yb, CoordType yTop, unsigned perceiveFlag);

    CoordType triangleCrossProduct(unsigned ia, unsigned ib, unsigned ic) const;
    void addTriangle(unsigned ia, unsigned ib, unsigned ic);

    CoordType calcTriangleRatio(const VertexType& v1,
                                const VertexType& v2,
                                const VertexType& v3) const;
    void createCoherentNucleus(unsigned v1, unsigned v2, unsigned v3);
    bool shapeCoherent(unsigned v);

    //CoordType triangleArea(unsigned p, unsigned q, unsigned r) const;
    static CoordType distanceSquare(const VertexType& v1, const VertexType& v2);
    void findMaxDiameter(unsigned* v1, unsigned* v2) const;
    void triangulateCoherentCurve();
    void triangulateConvexShape();
    void processCoherentShape();

    void addTriangle(const TriangleType& t);
    bool addTrianglePilot(unsigned ia, unsigned ib, unsigned ic);
    void addTrianglePilot(unsigned ia, unsigned ib, unsigned ic, CoordType cp);

    bool triangulateMonotonePilot(unsigned idx);
    void triangulateMonotoneRegular(unsigned idx);


    //-------------------------------------------------------------------
    FillRuleType                                    FillRule;

    bool                                            OptimizeDirection;
    bool                                            OptimizeCoherence;
    bool                                            OptimizeMonotones;
    bool                                            PilotTriangulation;
    bool                                            SwapCoordinates;
    GArrayUnsafeLH_POD<unsigned, SID>               HistHor;
    GArrayUnsafeLH_POD<unsigned, SID>               HistVer;

    GArrayUnsafeLH_POD<bool, SID>                   VisibleStyles;
    VertexStorageType                               Vertices;
    GArrayPagedLH_POD<PathInfoType, 6, 64, SID>     Paths;
    GArrayPagedLH_POD<EdgeType, 10, 64, SID>        Edges;
    GArrayPagedLH_POD<MonoChainType, 6, 64, SID>    MonoChains;
    ChainPtrStorage                                 MonoChainsSorted;
    GArrayPagedLH_POD<unsigned, 10, 64, SID>        Scanbeams;

    ChainPtrStorage                                 ActiveChains;
    ChainPtrStorage                                 InteriorChains;
    GArrayUnsafeLH_POD<unsigned, SID>               ValidChains;
    GArrayUnsafeLH_POD<unsigned, SID>               InteriorOrder;
    GArrayPagedLH_POD<IntersectionType, 6, 64, SID> Intersections;

    GArrayUnsafeLH_POD<int, SID>                    StyleCounts;
    unsigned                                        MinStyle;
    unsigned                                        MaxStyle;
    CoordType                                       MinX;
    CoordType                                       MinY;
    CoordType                                       MaxX;
    CoordType                                       MaxY;
    CoordType                                       EpsilonX;
    CoordType                                       EpsilonY;
                                 
    GArrayUnsafeLH_POD<unsigned, SID>               SwapEdgesTable;
    VertexStorageType                               EventVertices;
    VertexType                                      LastVertex;

    ScanChainArrayType                              ChainsBelow;
    ScanChainArrayType                              ChainsAbove;
    GArrayPagedLH_POD<BaseLineType, 4, 16, SID>     BaseLines;   // Base lines of the pending ends
    PendingEndArrayType                             PendingEnds; // An array of all pending monotones

    GArrayPagedLH_POD<MonotoneType, 6, 64, SID>     Monotones;
    GArrayPagedLH_POD<MonoVertexType, 10, 64, SID>  MonoVertices;

    GArrayPagedLH_POD<unsigned, 8, 64, SID>         MonoStack;
    GArrayPagedLH_POD<TriangleType, 10, 64, SID>    Triangles;

    GArrayPagedLH_POD<unsigned, 8, 64, SID>         LeftVertices;
    GArrayPagedLH_POD<unsigned, 8, 64, SID>         RightVertices;
    GArrayPagedLH_POD<unsigned, 8, 64, SID>         ConvexShape;
    unsigned                                        TrianglesCutOff;
    Coherence_e                                     ShapeCoherence;
    Coherence_e                                     LeftCoherence;
    Coherence_e                                     RightCoherence;



//// TO DO: Remove
//public:
//struct CoherentPartType
//{
//    unsigned start;
//    unsigned size;
//    unsigned type;
//};
//GPodBVector<CoherentPartType> CoherentParts;
//GPodBVector<unsigned>         CoherentVertices;



};

#endif
