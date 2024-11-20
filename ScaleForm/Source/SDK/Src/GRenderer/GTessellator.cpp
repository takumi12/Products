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

#ifndef INC_GTESSELLATOR_H
#define INC_GTESSELLATOR_H

#include "GTessellator.h"


#ifdef GDEBUGDRAW
#include "AggDraw.h"
extern AggDraw* DrawPtr;
#endif


//-----------------------------------------------------------------------
inline bool GTessellator::monoChainLess(const MonoChainType* a, 
                                        const MonoChainType* b)  
{
    return a->ySort < b->ySort;
}

//-----------------------------------------------------------------------
inline bool GTessellator::styleLess(const MonotoneType& a, 
                                    const MonotoneType& b)
{
    return a.style < b.style;
}

//-----------------------------------------------------------------------
inline bool GTessellator::activeChainLess(const MonoChainType* a, 
                                          const MonoChainType* b)
{ 
    if(a->xb != b->xb) return a->xb < b->xb;
    return a->xt < b->xt;
}

//-----------------------------------------------------------------------
inline bool GTessellator::intersectionLess(const IntersectionType& a, 
                                           const IntersectionType& b)
{
    return a.y < b.y;
}

//-----------------------------------------------------------------------
inline bool GTessellator::forwardMin(int idx, int start) const
{
    CoordType y = Vertices[idx].y;
    if(idx <= start) return Vertices[idx+1].y > y;
    return Vertices[idx-1].y >= y && Vertices[idx+1].y > y;
}

//-----------------------------------------------------------------------
inline bool GTessellator::reverseMin(int idx, int end) const
{
    CoordType y = Vertices[idx].y;
    if(idx >= end) return Vertices[idx-1].y > y;
    return Vertices[idx-1].y > y && Vertices[idx+1].y >= y;
}

//-----------------------------------------------------------------------
inline bool GTessellator::forwardMax(int idx, int end) const
{
    if(idx >= end) return true;
    return Vertices[idx+1].y <= Vertices[idx].y;
}

//-----------------------------------------------------------------------
inline bool GTessellator::reverseMax(int idx, int start) const
{
    if(idx <= start) return true;
    return Vertices[idx-1].y <= Vertices[idx].y;
}

//-----------------------------------------------------------------------
void GTessellator::SetStyleVisibility(unsigned style, bool visible)
{
    UPInt s = VisibleStyles.GetSize();
    if(style >= VisibleStyles.GetSize())
    {
        UPInt i;
        if(style >= VisibleStyles.GetCapacity())
        {
            GArrayUnsafePOD<bool, SID> t;
            t.Reserve(VisibleStyles.GetSize());
            G_AppendArray(t, VisibleStyles);
            VisibleStyles.Reserve(style + 1, 32);
            for(i = 0; i < t.GetSize(); i++) VisibleStyles.PushBack(t[i]);
        }
        for(i = VisibleStyles.GetSize(); i <= style; i++)
        {
            VisibleStyles.PushBack(true);
        }
    }
    VisibleStyles[style] = visible;
    if(s == 0 && style > 0) VisibleStyles[0] = false;
}

//-----------------------------------------------------------------------
bool GTessellator::GetStyleVisibility(unsigned style) const
{
    if(style < VisibleStyles.GetSize()) return VisibleStyles[style];
    return style != 0;
}

//-----------------------------------------------------------------------
void GTessellator::buildEdgeList(unsigned start, 
                                 unsigned numEdges, 
                                 int step, 
                                 unsigned leftStyle, 
                                 unsigned rightStyle)
{
    EdgeType e;
    unsigned i;
    unsigned startEdgeIdx = (unsigned)Edges.GetSize();
    for(i = 0; i < numEdges; ++i)
    {
        e.lower = start;
        start  += step;
        e.upper = start;
        const VertexType& v1 = Vertices[e.lower];
        const VertexType& v2 = Vertices[e.upper];
        e.slope = (v2.x - v1.x) / (v2.y - v1.y);
        e.next  = 0;
        Edges.PushBack(e);
        if(i) 
        {
            Edges[Edges.GetSize()-2].next = &Edges[Edges.GetSize()-1];
        }
    }
    MonoChainType mc;
    mc.edge        = &Edges[startEdgeIdx];//mc.lower;
    mc.ySort       = 0;//Vertices[mc.edge->lower].y;
    mc.xb = mc.xt  = 0;//Vertices[mc.edge->lower].x;
    mc.dir         = step;
    mc.leftStyle   = leftStyle;
    mc.rightStyle  = rightStyle;
    mc.leftBelow   = 0;
    mc.leftAbove   = 0;
    mc.rightBelow  = 0;
    mc.rightAbove  = 0;
    mc.flags       = 0;
    mc.posScan     = 0;
    mc.posIntr     = ~0U;
    MonoChains.PushBack(mc);
}

//-----------------------------------------------------------------------
void GTessellator::addPath(const GCompoundShape::SPath& path, int addStyle)
{
    if(path.GetNumVertices() < 2) return;

    unsigned i;
    PathInfoType p;

    p.start = (unsigned)Vertices.GetSize();

    for(i = 0; i < path.GetNumVertices(); ++i)
    {
        VertexType v = path.GetVertex(i);
        Scanbeams.PushBack((unsigned)Vertices.GetSize());
        Vertices.PushBack(v);
        if(v.x < MinX) MinX = v.x;
        if(v.y < MinY) MinY = v.y;
        if(v.x > MaxX) MaxX = v.x;
        if(v.y > MaxY) MaxY = v.y;
    }

    p.end = (unsigned)Vertices.GetSize() - 1;
    p.leftStyle  = path.GetLeftStyle()  + addStyle;
    p.rightStyle = path.GetRightStyle() + addStyle;
    Paths.PushBack(p);
}

//-----------------------------------------------------------------------
void GTessellator::decomposePath(const PathInfoType& path)
{
    int min, max;
    int numEdges;

    // Do the path forward pass
    for(min = path.start; min < path.end; ++min)
    {
        // If a forward local minimum...
        if(forwardMin(min, path.start))
        {
            // Search for the next local maximum...
            numEdges = 1;
            max = min + 1;
            while(!forwardMax(max, path.end))
            {
                ++numEdges;
                ++max;
            }

            // Build the next edge list 
            buildEdgeList(min, numEdges, +1, path.leftStyle, path.rightStyle);
            min += numEdges - 1;
        }
    }

    // Do the path reverse pass
    for(min = path.end; min > path.start; --min)
    {
        // If a reverse local minimum...
        if(reverseMin(min, path.end))
        {
            // Search for the previous local maximum...
            numEdges = 1;
            max = min - 1;
            while(!reverseMax(max, path.start))
            {
                ++numEdges;
                --max;
            }

            // Build the previous edge list
            buildEdgeList(min, numEdges, -1, path.leftStyle, path.rightStyle);
            min -= numEdges - 1;
        }
    }
}

//-----------------------------------------------------------------------
inline void GTessellator::addStyles(const MonoChainType* mc)
{
    unsigned ls = mc->leftStyle;
    unsigned rs = mc->rightStyle;
    if(FillRule == FillNonZero)
    {
        StyleCounts[ls] += mc->dir;
        StyleCounts[rs] -= mc->dir;
    }
    else
    {
        StyleCounts[ls] ^= 1;
        StyleCounts[rs] ^= 1;
    }
}

//-----------------------------------------------------------------------
inline int GTessellator::findElderStyle() const
{
    unsigned i;
    for(i = MaxStyle; i >= MinStyle; --i)
    {
        if(StyleCounts[i]) return i;
    }
    return 0;
}

//-----------------------------------------------------------------------
void GTessellator::perceiveStyles(const ChainPtrStorage& aet)
{
    unsigned i, j;

    StyleCounts[0] = 0;
    for(j = MinStyle; j <= MaxStyle; j++) StyleCounts[j] = 0;

    unsigned leftAbove = 0;
    for(i = 0; i < aet.GetSize(); ++i)
    {
        MonoChainType* mc = aet[i];
        mc->flags &= ~VisibleChain;
        if((mc->flags & EndChainFlag) == 0)
        {
            addStyles(mc);
            mc->rightAbove = findElderStyle();
            mc->leftAbove  = leftAbove;
            if(leftAbove != mc->rightAbove) 
            {
                mc->flags |= VisibleChain;
            }
            leftAbove = mc->rightAbove;
        }
    }
}

//-----------------------------------------------------------------------
void GTessellator::setupIntersections()
{
    InteriorChains = ActiveChains;
    unsigned i;
    for(i = 0; i < ActiveChains.GetSize(); i++) 
    {
        ActiveChains[i]->posIntr = i;
        InteriorOrder[i] = i;
    }
}

//-----------------------------------------------------------------------
inline void GTessellator::setStyleBounds(const MonoChainType* mc)
{
    unsigned ls = mc->leftStyle;
    unsigned rs = mc->rightStyle;
    if(ls)
    {
        if(ls < MinStyle) MinStyle = ls;
        if(ls > MaxStyle) MaxStyle = ls;
    }
    if(rs)
    {
        if(rs < MinStyle) MinStyle = rs;
        if(rs > MaxStyle) MaxStyle = rs;
    }
}

//-----------------------------------------------------------------------
inline GTessellator::CoordType GTessellator::calcX(const EdgeType* edge, 
                                                   CoordType y) const
{
    const VertexType& lower = Vertices[edge->lower];
    return lower.x + (y - lower.y) * edge->slope;
}

//-----------------------------------------------------------------------
unsigned GTessellator::nextScanbeam(CoordType yb, CoordType yt, 
                                    unsigned startMc, unsigned numMc)
{
    unsigned i, pos;
    MonoChainType* mc;
    const VertexType* lower;
    const VertexType* upper;
    IntersectionType in;
    unsigned retFlags = 0;

    if(numMc) retFlags = InsertEdgesFlag;

    MinStyle = 0x3FFFFFFF;
    MaxStyle = 0;
    ValidChains.Clear();
    for(i = 0; i < ActiveChains.GetSize(); ++i)
    {
        mc = ActiveChains[i];

        // Clear EventFlag, that is, assume the chain won't have 
        // an event point.
        //------------------------
        mc->flags &= ~EventFlag;

        if(Vertices[mc->edge->upper].y == yb)
        {
            // There is an event point, that is, the yb scan-beam 
            // goes through the upper vertex of the edge
            //-----------------------
            if(mc->edge->next)
            {
                // Next edge exists, the chain continues.
                // Switch to the next edge, initialize xb, calculate xt, 
                // and mark the chain as having an event point.
                //----------------
                mc->edge = mc->edge->next;
                lower    = &Vertices[mc->edge->lower];
                upper    = &Vertices[mc->edge->upper];
                mc->xb   = lower->x;
                mc->xt   = (upper->y == yt) ?
                            upper->x :
                            calcX(mc->edge, yt);
                setStyleBounds(mc);
                ValidChains.PushBack(i);
            }
            else
            {
                // Chain ends at this scan-beam. Move xb to xt
                // and mark the chain as ending and having an event point.
                //------------------
                mc->xb     = mc->xt;
                mc->flags |= EndChainFlag;
                retFlags  |= RemoveEdgesFlag;
            }
            mc->flags |= EventFlag;
        }
        else
        {
            // The scan-beam yb crosses the edge somewhere in the middle.
            // No event points, just calculate new xb and xt.
            //------------------
            lower  = &Vertices[mc->edge->lower];
            upper  = &Vertices[mc->edge->upper];
            mc->xb = mc->xt;
            mc->xt = (upper->y == yt) ?
                      upper->x :
                      calcX(mc->edge, yt);
            setStyleBounds(mc);
            ValidChains.PushBack(i);
        }
    }

    // Add new monotone chains
    while(numMc)
    {
        mc        = MonoChainsSorted[startMc++];
        lower     = &Vertices[mc->edge->lower];
        upper     = &Vertices[mc->edge->upper];
        mc->xb    = lower->x;
        mc->flags = EventFlag;
        mc->xt    = (upper->y == yt) ?
                     upper->x :
                     calcX(mc->edge, yt);
        pos = (unsigned)G_UpperBound(ActiveChains, mc, activeChainLess);
        ActiveChains.InsertAt(pos, mc);
        setStyleBounds(mc);
        --numMc;
    }

    Intersections.Clear();

    if(retFlags & InsertEdgesFlag)
    {
        // Re-create a list of valid chains to be sorted
        ValidChains.Clear();
        for(i = 0; i < ActiveChains.GetSize(); ++i)
        {
            if((ActiveChains[i]->flags & EndChainFlag) == 0) 
            {
                ValidChains.PushBack(i);
            }
        }
    }

    // Perceive intersections.
    // To do that we just sort the edges by top X, using 
    // a O(N^2) sort method (insertion sort)
    // The necessity to swap the edges means they have an intersection
    // point within the yb...yt slab
    //---------------------
    CoordType height = yt - yb;
    for(i = 1; i < ValidChains.GetSize(); ++i)
    {
        int j;
        for(j = i - 1; j >= 0; --j)
        {
            MonoChainType* mc1 = ActiveChains[ValidChains[j  ]];
            MonoChainType* mc2 = ActiveChains[ValidChains[j+1]];

            if(mc1->xt <= mc2->xt) break;

            if(Intersections.GetSize() == 0)
            {
                setupIntersections();
            }

            CoordType nom = mc1->xb - mc2->xb;
            CoordType den = mc2->xt - mc2->xb - mc1->xt + mc1->xb;

            in.pos1 = mc1->posIntr;
            in.pos2 = mc2->posIntr;
            in.y    = (den == 0) ? yb : yb + height * nom / den;

            if(in.y < yb)
            {
                in.y = yb;
            }
            if(in.y > yt)
            {
                in.y = yt;
            }
            Intersections.PushBack(in);
            G_Swap(ActiveChains[ValidChains[j  ]], ActiveChains[ValidChains[j+1]]);
        }
    }

    // If there are two or more intersections sort them
    //-------------------------
    if(Intersections.GetSize() > 1)
    {
        //G_QuickSort(Intersections, intersectionLess);

        // Insertion sort works better because it does not
        // perform unnecessary swaps. In case there are
        // two or more exactly coinciding intersection 
        // points it's highly desirable to keep the original order, 
        // while quick sort may swap equal elements.
        // Since it's extremelly unlikely that there are many
        // intersections within one slab, the performance of  
        // insertion sort is perfectly appropriate.
        //---------------------
        G_InsertionSort(Intersections, intersectionLess);
    }

    return retFlags;
}

//-----------------------------------------------------------------------
inline unsigned GTessellator::addEventVertex(const VertexType& v)
{
    if(v.y > LastVertex.y || v.x > LastVertex.x)
    {
        EventVertices.PushBack(LastVertex = v);
    }
    return (unsigned)EventVertices.GetSize() - 1;
}

//-----------------------------------------------------------------------
inline unsigned GTessellator::addEventVertex(const MonoChainType* mc, 
                                             CoordType yb, 
                                             bool enforceFlag)
{
    // (1) Check the condition if the left and right styles remain 
    //     the same and producing the vertex is not enforced. 
    //------------
    if(!enforceFlag && 
        mc->leftBelow == mc->leftAbove && 
        mc->rightBelow == mc->rightAbove)
    {
        // (2) If the styles are the same, add the event vertex only 
        //     in case the current edge has changed (EventFlag) and its 
        //     lower vertex lies exactly on the scan-beam. The last 
        //     condition is necessary to process self-intersections 
        //     correctly. When the horizontal band is divided into a 
        //     number of sub-bands in function processInterior(), 
        //     there should be a check for adding the same vertex 
        //     only once. The next sub-band will have a different 
        //     Y coordinate and the same vertex will be skipped.
        //     This branch is executed most frequently. Also only in
        //     this branch the function may return -1 ("No Event").
        //---------------
        if((mc->flags & EventFlag) != 0 &&
           Vertices[mc->edge->lower].y == yb)
        {
            return addEventVertex(Vertices[mc->edge->lower]);
        }
        else
        {
            return ~0U;
        }
    }
    else
    {
        // (3) The styles on the left and right are different, or 
        //     the vertex is enforced. It may also mean the edge
        //     intersects with another one. The algorithm does not 
        //     distinguish the cases when the edges do intersect, or
        //     the styles change for another reason (integrity violation).
        //     This generalization makes the algorithm extremely robust.
        //     Before calculating the intersection it's a good idea to 
        //     check for trivial cases, when the current scan-beam 
        //     coincides with the lower or upper edge vertex.
        //     This branch always returns a valid vertex, whether 
        //     a new or existing one.
        //--------------
        if(Vertices[mc->edge->lower].y == yb) 
        {
            return addEventVertex(Vertices[mc->edge->lower]);
        }

        if((mc->flags & EndChainFlag) != 0 && 
            Vertices[mc->edge->upper].y == yb)
        {
            return addEventVertex(Vertices[mc->edge->upper]);
        }

        // (4) The scan-bean differs from the lower or upper vertex,
        //     calculate the intersection point with function calcX().
        //     This code also checks for the same X coordinate with 
        //     certain Epsilon value. If the difference between the 
        //     last X and the calculated X is within this epsilon, 
        //     the vertices merge.
        //--------------
        float x = calcX(mc->edge, yb);
        if(yb > LastVertex.y)
        {
            LastVertex = VertexType(x, yb);
            EventVertices.PushBack(LastVertex);
        }
        else
        if(x - LastVertex.x > EpsilonX)
        {
            LastVertex = VertexType(x, yb);
            EventVertices.PushBack(LastVertex);
        }
    }
    return (unsigned)EventVertices.GetSize() - 1;
}

//-----------------------------------------------------------------------
inline void GTessellator::resetMonotone(MonotoneType* m, unsigned style)
{
    m->start       =  0;
    m->lastVertex  = ~0U;
    m->prevVertex1 = ~0U;
    m->prevVertex2 = ~0U;
    m->style       =  style;
    m->lowerBase   =  0;
}

//-----------------------------------------------------------------------
inline void GTessellator::growMonotone(MonotoneType* m, 
                                       unsigned vertex)
{
    MonoVertexType mv;
    mv.next   = 0;
    mv.vertex = vertex;
    if(m->start == 0)
    {
        MonoVertices.PushBack(mv);
        m->start       = &MonoVertices.Back();
        m->prevVertex2 = ~0U;
        m->prevVertex1 = ~0U;
        m->lastVertex  = (unsigned)MonoVertices.GetSize() - 1;
    }
    else
    {
        MonoVertexType& last = MonoVertices[m->lastVertex];
        if(last.vertex != vertex)
        {
            MonoVertices.PushBack(mv);
            last.next      = &MonoVertices.Back();
            m->prevVertex2 = m->prevVertex1;
            m->prevVertex1 = m->lastVertex;
            m->lastVertex  = (unsigned)MonoVertices.GetSize() - 1;
        }
    }
}

//-----------------------------------------------------------------------
inline void GTessellator::growMonotone(MonotoneType* m, 
                                       unsigned left,
                                       unsigned right)
{
    if(left  != ~0U) growMonotone(m, left  |  LeftMask);
    if(right != ~0U) growMonotone(m, right & ~LeftMask);
}

//-----------------------------------------------------------------------
inline void GTessellator::growMonotoneAndConnect(ScanChainType* scan, 
                                                 unsigned vertex,
                                                 bool)// enforceConnection)
                                                 // Parameter enforceConnection 
                                                 // will be used later
{
    if(scan && scan->monotone)
    {
        MonotoneType* m = scan->monotone;
        if(m->lowerBase)
        {
            if(m->lowerBase->y == EventVertices[vertex & ~LeftMask].y)
            {
                m->lowerBase->vertexRight = vertex & ~LeftMask;
            }
            else
            {
                if(vertex & LeftMask) connectPendingToLeft (scan, vertex);
                else                  connectPendingToRight(scan, vertex);
            }
        }
        else
        {
            growMonotone(m, vertex);
        }
    }
}

//-----------------------------------------------------------------------
unsigned GTessellator::lastMonoVertex(const MonotoneType* m) const
{
    if(m->lastVertex != ~0U) return MonoVertices[m->lastVertex].vertex;
    return ~0U;
}

//-----------------------------------------------------------------------
void GTessellator::removeLastMonoVertex(MonotoneType* m)
{
    if(m->lastVertex != ~0U)
    {
        if(m->lastVertex == MonoVertices.GetSize() - 1) 
        {
            MonoVertices.PopBack();
        }
        m->lastVertex  = m->prevVertex1;
        m->prevVertex1 = m->prevVertex2;
        m->prevVertex2 = ~0U;
        if(m->lastVertex == ~0U)
        {
            m->start = 0;
        }
        else
        {
            MonoVertices[m->lastVertex].next = 0;
        }
    }
}

//-----------------------------------------------------------------------
GTessellator::MonotoneType* GTessellator::startMonotone(unsigned style)
{
    MonotoneType m;
    resetMonotone(&m, style);
    Monotones.PushBack(m);
    return &Monotones.Back();
}

//-----------------------------------------------------------------------
void GTessellator::startMonotone(ScanChainType* scan, unsigned vertex)
{
    scan->monotone = 0;
    if(VisibleStyles[scan->chain->rightAbove])
    {
        scan->monotone = startMonotone(scan->chain->rightAbove);
        growMonotoneAndConnect(scan, vertex, true);
    }
}

//-----------------------------------------------------------------------
void GTessellator::replaceMonotone(ScanChainType* scan, unsigned style)
{
    if(VisibleStyles[style])
    {
        if(scan->monotone == 0)
        {
            scan->monotone = startMonotone(style);
            return;
        }
        if(scan->monotone->style == style || 
           scan->monotone->start == 0)
        {
            scan->monotone->style = style;
            return;
        }
        MonotoneType* monotone = startMonotone(style);
        *monotone = *scan->monotone;
        resetMonotone(scan->monotone, style);
    }
}

//-----------------------------------------------------------------------
void GTessellator::replaceMonotone(PendingEndType* pe, unsigned style)
{
    if(VisibleStyles[style])
    {
        if(pe->monotone == 0)
        {
            pe->monotone = startMonotone(style);
            return;
        }
        if(pe->monotone->style == style || 
           pe->monotone->start == 0)
        {
            pe->monotone->style = style;
            return;
        }
        MonotoneType* monotone = startMonotone(style);
        *monotone = *pe->monotone;
        resetMonotone(pe->monotone, style);
    }
}

//-----------------------------------------------------------------------
void GTessellator::addPendingEnd(ScanChainType* dst, 
                                 ScanChainType* pending, 
                                 CoordType  y)
{
    if(dst && dst->monotone && VisibleStyles[dst->monotone->style])
    {
        MonotoneType* m = dst->monotone;
        if(m->lowerBase == 0)
        {
            BaseLineType lowerBase;
            lowerBase.y           =  y;
            lowerBase.styleLeft   =  pending->chain->leftBelow;
            lowerBase.vertexLeft  =  dst->vertex;
            lowerBase.vertexRight = ~0U;
            lowerBase.firstChain  =  (unsigned)PendingEnds.GetSize();
            lowerBase.numChains   =  0;
            lowerBase.leftAbove   = ~0U;
            BaseLines.PushBack(lowerBase);
            m->lowerBase = &BaseLines.Back();
        }
        PendingEndType pe;
        pe.vertex   = pending->vertex;
        pe.monotone = pending->monotone;
        PendingEnds.PushBack(pe);
        ++m->lowerBase->numChains;
    }
}

//-----------------------------------------------------------------------
template<class ElementsArray> struct GTessBaseLineIterator
{
    typedef typename ElementsArray::ValueType ScanChainType;

    template<class BaseLineType> 
    GTessBaseLineIterator(ElementsArray& e,
                          BaseLineType* bl,
                          ScanChainType* scanLeft) :
        Elements(e),
        Index(bl->firstChain),
        Num(bl->numChains),
        VertexRightmost(bl->vertexRight),
        Chain(scanLeft),
        VertexLeft(bl->vertexLeft),
        VertexRight(e[bl->firstChain].vertex),
        Style(bl->styleLeft),
        FlagFirst(true)
    {}

    template<class StyleFunction> bool Next(StyleFunction style)
    {
        FlagFirst = false;
        if(Num)
        {
            --Num;
            VertexLeft  = VertexRight;
            Chain       = &Elements[Index++];
            VertexRight = Num ? Elements[Index].vertex : VertexRightmost;
            Style       = style(Chain);
            return true;
        }
        return false;
    }

    bool IsFirst() const { return FlagFirst; }
    bool IsLast()  const { return Num == 0; }

    ElementsArray& Elements;
    unsigned       Index;
    unsigned       Num;
    unsigned       VertexRightmost;
    ScanChainType* Chain;
    unsigned       VertexLeft;
    unsigned       VertexRight;
    unsigned       Style;
    bool           FlagFirst;

private:
    void operator = (const GTessBaseLineIterator<ElementsArray>&);
};

//-----------------------------------------------------------------------
void GTessellator::connectPendingToLeft(ScanChainType* scan, 
                                        unsigned targetVertex)
{
    BaseLineType* lowerBase = scan->monotone->lowerBase;
    unsigned styleAbove = scan->monotone->style;

    scan->monotone->lowerBase = 0;

    PendingEndType scanLeft;
    scanLeft.vertex   = lowerBase->vertexLeft;
    scanLeft.monotone = scan->monotone;

    GTessBaseLineIterator<PendingEndArrayType> it(PendingEnds,
                                                  lowerBase,
                                                  &scanLeft);
    do
    {
        if(it.VertexLeft != it.VertexRight)
        {
            if(it.IsFirst())
            {
                growMonotone(scan->monotone, it.VertexRight);
                growMonotone(scan->monotone, targetVertex, targetVertex);
            }
            else
            {
                if(it.Style != styleAbove || it.Chain->monotone == 0)
                {
                    it.Chain->monotone = startMonotone(styleAbove);
                    growMonotone(it.Chain->monotone, it.VertexLeft, it.VertexRight);
                }
                if(!it.IsLast())
                {
                    growMonotone(it.Chain->monotone, targetVertex, targetVertex);
                }
                else
                {
                    scan->monotone = it.Chain->monotone;
                    growMonotone(scan->monotone, targetVertex | LeftMask);
                }
            }
        }
    }
    while(it.Next(pendingMonotoneStyle));

    if(lowerBase == &BaseLines.Back())
    {
        PendingEnds.CutAt(lowerBase->firstChain);
        BaseLines.PopBack();
    }
}




//-----------------------------------------------------------------------
void GTessellator::connectPendingToRight(ScanChainType* scan, 
                                         unsigned targetVertex)
{
    BaseLineType* lowerBase = scan->monotone->lowerBase;
    unsigned styleAbove = scan->monotone->style;

    scan->monotone->lowerBase = 0;

    PendingEndType scanLeft;
    scanLeft.vertex   = lowerBase->vertexLeft;
    scanLeft.monotone = scan->monotone;

    GTessBaseLineIterator<PendingEndArrayType> it(PendingEnds,
                                                  lowerBase,
                                                  &scanLeft);
    growMonotone(scan->monotone, it.VertexRight);
    growMonotone(scan->monotone, targetVertex);
    while(it.Next(pendingMonotoneStyle))
    {
        if(it.VertexLeft != it.VertexRight)
        {
            if(it.Style != styleAbove || it.Chain->monotone == 0)
            {
                it.Chain->monotone = startMonotone(styleAbove);
                growMonotone(it.Chain->monotone, it.VertexLeft, it.VertexRight);
            }
            growMonotone(it.Chain->monotone, targetVertex, targetVertex);
        }
    }
    if(lowerBase == &BaseLines.Back())
    {
        PendingEnds.CutAt(lowerBase->firstChain);
        BaseLines.PopBack();
    }
}

//-----------------------------------------------------------------------
void GTessellator::connectStartingToLeft(ScanChainType* scan, 
                                         BaseLineType* upperBase, 
                                         unsigned targetVertex)
{
    ScanChainType* leftAbove = (upperBase->leftAbove == ~0U) ? 
                                scan :
                               &ChainsAbove[upperBase->leftAbove];

    GTessBaseLineIterator<ScanChainArrayType> it(ChainsAbove,
                                              upperBase,
                                              scan);
    unsigned styleBelow = scan->monotone->style;
    MonotoneType* monotone = startMonotone(0);
    *monotone = *scan->monotone;
    resetMonotone(scan->monotone, styleBelow);

    do
    {
        if(!it.IsLast())
        {
            if(it.VertexLeft != it.VertexRight)
            {
                replaceMonotone(it.Chain, styleBelow);
                growMonotone(it.Chain->monotone, targetVertex, targetVertex);
                growMonotone(it.Chain->monotone, it.VertexLeft, it.VertexRight);
            }
        }
        else
        {
            it.Chain->monotone = monotone;
            growMonotone(it.Chain->monotone, it.VertexLeft, it.VertexRight);
        }
        if(it.Style != styleBelow || it.Chain->monotone == 0)
        {
            if(!VisibleStyles[it.Style])
            {
                it.Chain->monotone = 0;
            }
            else
            {
                if(it.IsFirst())
                {
                    it.Chain = leftAbove;
                }
                replaceMonotone(it.Chain, it.Style);
                growMonotone(it.Chain->monotone, it.VertexLeft, it.VertexRight);
            }
        }
    }
    while(it.Next(startingMonotoneStyle));
    upperBase->numChains = 0;
}

//-----------------------------------------------------------------------
void GTessellator::connectStartingToRight(ScanChainType* scan, 
                                          BaseLineType* upperBase, 
                                          unsigned targetVertex)
{
    ScanChainType* leftAbove = (upperBase->leftAbove == ~0U) ? 
                                scan :
                               &ChainsAbove[upperBase->leftAbove];

    GTessBaseLineIterator<ScanChainArrayType> it(ChainsAbove,
                                              upperBase,
                                              scan);
    unsigned styleBelow = scan->monotone->style;

    do
    {
        if(it.IsFirst())
        {
            growMonotone(it.Chain->monotone, it.VertexLeft, it.VertexRight);
        }
        else
        {
            if(it.VertexLeft != it.VertexRight)
            {
                replaceMonotone(it.Chain, styleBelow);
                growMonotone(it.Chain->monotone, targetVertex, targetVertex);
                growMonotone(it.Chain->monotone, it.VertexLeft, it.VertexRight);
            }
        }
        if(it.Style != styleBelow || it.Chain->monotone == 0)
        {
            if(!VisibleStyles[it.Style])
            {
                it.Chain->monotone = 0;
            }
            else
            {
                if(it.IsFirst())
                {
                    it.Chain = leftAbove;
                }
                replaceMonotone(it.Chain, it.Style);
                growMonotone(it.Chain->monotone, it.VertexLeft, it.VertexRight);
            }
        }
    }
    while(it.Next(startingMonotoneStyle));
    upperBase->numChains = 0;
}

//-----------------------------------------------------------------------
void GTessellator::connectStartingToPending(ScanChainType* scan, 
                                            BaseLineType* upperBase)
{
    BaseLineType* lowerBase = scan->monotone->lowerBase;
    unsigned styleBetween = scan->monotone->style;

    scan->monotone->lowerBase = 0;

    PendingEndType scanLeft;
    scanLeft.vertex   = lowerBase->vertexLeft;
    scanLeft.monotone = scan->monotone;

    GTessBaseLineIterator<PendingEndArrayType> itLower(PendingEnds,
                                                       lowerBase,
                                                       &scanLeft);

    GTessBaseLineIterator<ScanChainArrayType>  itUpper(ChainsAbove,
                                                       upperBase,
                                                       scan);
    unsigned lowerTarget = ~0U;
    unsigned stopNum = itLower.Num < itUpper.Num;

    for(;;)
    {
        bool invalid = 
            (itLower.VertexLeft == itLower.VertexRight &&
            (itUpper.VertexLeft == ~0U || itUpper.VertexRight == ~0U)) ||
            (itUpper.VertexLeft == itUpper.VertexRight &&
            (itLower.VertexLeft == ~0U || itLower.VertexRight == ~0U));

        if(!invalid && 
           (itLower.VertexLeft != itLower.VertexRight ||
            itUpper.VertexLeft != itUpper.VertexRight))
        {
            if(itLower.Style != styleBetween)
            {
                replaceMonotone(itLower.Chain, styleBetween);
                growMonotone(itLower.Chain->monotone, 
                             itLower.VertexLeft, 
                             itLower.VertexRight);
            }
            growMonotone(itLower.Chain->monotone, 
                         itUpper.VertexLeft, 
                         itUpper.VertexRight);
            itUpper.Chain->monotone = itLower.Chain->monotone;
        }
        if(itUpper.Style != styleBetween || itUpper.Chain->monotone == 0)
        {
            if(!VisibleStyles[itUpper.Style])
            {
                itUpper.Chain->monotone = 0;
            }
            else
            {
                replaceMonotone(itUpper.Chain, itUpper.Style);
                growMonotone(itUpper.Chain->monotone, 
                             itUpper.VertexLeft, 
                             itUpper.VertexRight);
            }
        }
        if(itLower.Num == stopNum) 
        {
            lowerTarget = itLower.VertexLeft;
            if(itLower.VertexRight != ~0U) 
            {
                lowerTarget = itLower.VertexRight;
            }
            break;
        }
        itLower.Next(pendingMonotoneStyle);
        itUpper.Next(startingMonotoneStyle);
    }

    if(itUpper.Num && lowerTarget != ~0U)
    {
        MonotoneType* monotone = 0;
        itLower.Next(pendingMonotoneStyle);
        itUpper.Next(startingMonotoneStyle);

        if(itLower.Chain->monotone && 
           itLower.Chain->monotone->style == styleBetween)
        {
            monotone = startMonotone(styleBetween);
            *monotone = *itLower.Chain->monotone;
            resetMonotone(itLower.Chain->monotone, styleBetween);
        }

        do
        {
            if(itUpper.IsLast())
            {
                itUpper.Chain->monotone = monotone;
                if(itUpper.Chain->monotone == 0)
                {
                    itUpper.Chain->monotone = startMonotone(styleBetween);
                    growMonotone(itUpper.Chain->monotone, 
                                 itLower.VertexLeft, 
                                 itLower.VertexRight);
                }
                growMonotone(itUpper.Chain->monotone, 
                             itUpper.VertexLeft, 
                             itUpper.VertexRight);
            }
            else
            {
                if(itUpper.VertexLeft != itUpper.VertexRight)
                {
                    replaceMonotone(itUpper.Chain, styleBetween);
                    growMonotone(itUpper.Chain->monotone, lowerTarget, lowerTarget);
                    growMonotone(itUpper.Chain->monotone, 
                                 itUpper.VertexLeft, 
                                 itUpper.VertexRight);
                }
            }
            if(itUpper.Style != styleBetween || itUpper.Chain->monotone == 0)
            {
                if(!VisibleStyles[itUpper.Style])
                {
                    itUpper.Chain->monotone = 0;
                }
                else
                {
                    replaceMonotone(itUpper.Chain, itUpper.Style);
                    growMonotone(itUpper.Chain->monotone, 
                                 itUpper.VertexLeft, 
                                 itUpper.VertexRight);
                }
            }
        }
        while(itUpper.Next(startingMonotoneStyle));
    }

    if(lowerBase == &BaseLines.Back())
    {
        PendingEnds.CutAt(lowerBase->firstChain);
        BaseLines.PopBack();
    }
    upperBase->numChains = 0;
}

//-----------------------------------------------------------------------
void GTessellator::connectStarting(ScanChainType* scan, 
                                   BaseLineType* upperBase)
{
    if(scan && scan->monotone)
    {
        unsigned i;
        unsigned targetVertex = lastMonoVertex(scan->monotone);

        // Retrieve the latest vertex below yb, removing vertices 
        // from the monotone vertex list. There can be at most 
        // two vertices that lie on the current scan line.
        //--------------------
        upperBase->vertexLeft  = ~0U;
        upperBase->vertexRight = ~0U;
        for(i = 0; i < 2; i++)
        {
            if(targetVertex == ~0U ||
               EventVertices[targetVertex & ~LeftMask].y < upperBase->y)
            {
                break;
            }
            if(targetVertex & LeftMask)
            {
                upperBase->vertexLeft  = targetVertex & ~LeftMask;
            }
            else 
            {
                upperBase->vertexRight = targetVertex;
            }
            removeLastMonoVertex(scan->monotone);
            targetVertex = lastMonoVertex(scan->monotone);
        }

        if(scan->monotone->lowerBase)
        {
            connectStartingToPending(scan, upperBase);
            return;
        }

        if(targetVertex == ~0U)
        {
            // This should never happen but may, in some cases of broken 
            // data integrity. It means that there is no vertex to connect 
            // with the starting chains. In this case we just restore 
            // one of the retrieved vertices and continue processing.
            // Most probably degenerate monotones will appear (no harm).
            //--------------------
            if(upperBase->vertexRight != ~0U)
            {
                targetVertex = upperBase->vertexRight;
                upperBase->vertexRight = ~0U;
                growMonotone(scan->monotone, targetVertex);
            }
            else
            if(upperBase->vertexLeft != ~0U)
            {
                targetVertex = upperBase->vertexLeft;
                upperBase->vertexLeft = ~0U;
                growMonotone(scan->monotone, targetVertex);
            }
        }

        if(targetVertex & LeftMask)
        {
            connectStartingToLeft(scan, upperBase, targetVertex & ~LeftMask);
        }
        else
        {
            connectStartingToRight(scan, upperBase, targetVertex);
        }
    }
    upperBase->numChains = 0;
}

//-----------------------------------------------------------------------
unsigned GTessellator::nextChainInBundle(unsigned below, 
                                         unsigned above, 
                                         unsigned vertex) const
{
    const ScanChainType* thisBelow = 0;
    const ScanChainType* thisAbove = 0;
    if(below < ChainsBelow.GetSize()) thisBelow = &ChainsBelow[below];
    if(above < ChainsAbove.GetSize()) thisAbove = &ChainsAbove[above];
    if(thisBelow && 
       thisAbove && 
       thisBelow->vertex == thisAbove->vertex)
    {
        return 0;
    }
    if(thisAbove && thisAbove->vertex == vertex)
    {
        return ChainStartsAtScanline;
    }
    if(thisBelow && thisBelow->vertex == vertex)
    {
        return ChainEndsAtScanline;
    }
    return 0;
}

//-----------------------------------------------------------------------
void GTessellator::sweepScanbeam(const ChainPtrStorage& aet, CoordType yb)
{
    unsigned i;
    MonoChainType* mc;

    ChainsAbove.Clear();
    for(i = 0; i < aet.GetSize(); ++i)
    {
        mc = aet[i];
        mc->posScan = i; 
        if(mc->flags & VisibleChain) 
        {
            ScanChainType scan;
            scan.chain    = mc;
            scan.monotone = 0;
            scan.vertex   = ~0U;
            ChainsAbove.PushBack(scan);
        }
    }

    unsigned below = 0;
    unsigned above = 0;
    unsigned prevAbove = ~0U;
    unsigned vertex;

    ScanChainType* thisBelow;
    ScanChainType* thisAbove;
    ScanChainType* leftBelow = 0;
    ScanChainType* leftAbove = 0;
    BaseLineType   upperBase;

    upperBase.y         = yb;
    upperBase.numChains =  0;

    for(;;)
    {
        if(below < ChainsBelow.GetSize() && above < ChainsAbove.GetSize())
        {
            thisBelow = &ChainsBelow[below];
            thisAbove = &ChainsAbove[above];
            if(thisAbove->chain->posScan == thisBelow->chain->posScan)
            {
                thisBelow->vertex = 
                thisAbove->vertex = addEventVertex(thisAbove->chain, yb, false);
                ++below;
                ++above;
            }
            else
            if(thisAbove->chain->posScan < thisBelow->chain->posScan)
            {
                thisAbove->vertex = addEventVertex(thisAbove->chain, yb, true);
                ++above;
            }
            else
            {
                thisBelow->vertex = addEventVertex(thisBelow->chain, yb, true);
                ++below;
            }
        }
        else
        if(above < ChainsAbove.GetSize())
        {
            thisAbove = &ChainsAbove[above];
            thisAbove->vertex = addEventVertex(thisAbove->chain, yb, true);
            ++above;
        }
        else
        if(below < ChainsBelow.GetSize())
        {
            thisBelow = &ChainsBelow[below];
            thisBelow->vertex = addEventVertex(thisBelow->chain, yb, true);
            ++below;
        }
        else
        {
            break;
        }
    }

    below = 0;
    above = 0;
    for(;;)
    {
        if(below < ChainsBelow.GetSize() && above < ChainsAbove.GetSize())
        {
            thisBelow = &ChainsBelow[below];
            thisAbove = &ChainsAbove[above];

            // Define the processing branch
            //----------------
            unsigned chainFlag = ChainContinuesAtScanline;
            if(thisBelow->vertex != thisAbove->vertex)
            {
                chainFlag = 
                   (thisAbove->chain->posScan  < 
                    thisBelow->chain->posScan) ?
                        ChainStartsAtScanline : ChainEndsAtScanline;
            }

            switch(chainFlag)
            {
            case ChainContinuesAtScanline:              // Above  |
                                                        // Below  | continue chain
                vertex = thisBelow->vertex;
                if(vertex != ~0U)
                {
                    // Vertex at the continuing chain exists. 
                    // Grow the monotone polygon on the left of the chain.
                    // Most probably it is the same monotone, but may be
                    // a different one in case of intersecting with a 
                    // horizontal edge. In case the monotone polygon is the 
                    // same, function growMonotone() will filter out the latest 
                    // vertex.
                    //------------------
                    growMonotoneAndConnect(leftBelow, vertex, false);
                    growMonotoneAndConnect(leftAbove, vertex, false);

                    // If the upper base line contains starting chains,
                    // connect them with the latest visible vertex.
                    //------------------
                    if(upperBase.numChains)
                    {
                        connectStarting(leftBelow, &upperBase);
                    }

                    // Process a bundle, eliminating all degenerate cases.
                    //------------------
                    while((chainFlag = 
                           nextChainInBundle(below+1, above+1, vertex)) != 0)
                    {
                        if(chainFlag == ChainStartsAtScanline)
                        {
                            leftAbove = thisAbove;
                            thisAbove = &ChainsAbove[++above];
                            startMonotone(leftAbove, vertex | LeftMask);
                            growMonotoneAndConnect(leftAbove, vertex, true);
                        }
                        else
                        {
                            leftBelow = thisBelow;
                            thisBelow = &ChainsBelow[++below];
                            growMonotoneAndConnect(leftBelow, vertex | LeftMask, true);
                            growMonotoneAndConnect(leftBelow, vertex, true);
                        }
                    }

                    // Grow the monotone polygon on the right of the chain
                    // and if the style does not change, grow the polygon
                    // on the left and transfer it from thisBelow to thisAbove.
                    // In case the monotone polygon is the same function 
                    // growMonotone() will filter out the latest vertex,
                    // but it is necessary for correct processing of some 
                    // degenerate cases.
                    //---------------------
                    growMonotoneAndConnect(thisBelow, vertex | LeftMask, false);
                    if(thisBelow->chain->rightBelow == thisAbove->chain->rightAbove)
                    {
                        growMonotoneAndConnect(leftAbove, vertex, false);
                        thisAbove->monotone = thisBelow->monotone;
                    }
                    else
                    {
                        // If the style changes start new monotone polygon.
                        //---------------------
                        startMonotone(thisAbove, vertex | LeftMask);
                    }
                }
                else
                {
                    // The chain does not have a vertex at this scan-beam,
                    // but it is still necessary to check for the starting chains
                    // in this trapezoid. If there are any, connect them.
                    // Also, transfer the monotone polygon from thisBelow to 
                    // thisAbove.
                    //--------------------
                    if(upperBase.numChains)
                    {
                        connectStarting(leftBelow, &upperBase);
                    }
                    thisAbove->monotone = thisBelow->monotone;
                }

                prevAbove = above;
                leftBelow = thisBelow;
                leftAbove = thisAbove;
                ++below;
                ++above;
                break;
            
            case ChainStartsAtScanline:                 // Above  |  
                                                        // Below    | start chain (split)
                if(leftBelow && 
                   leftBelow->monotone && 
                   VisibleStyles[leftBelow->monotone->style])
                {
                    if(upperBase.numChains)
                    {
                        ++upperBase.numChains;
                    }
                    else
                    {
                        upperBase.styleLeft  = thisAbove->chain->leftAbove;
                        upperBase.firstChain = above;
                        upperBase.numChains  = 1;
                        upperBase.leftAbove  = prevAbove;
                    }
                    if(leftAbove &&
                       leftAbove->monotone &&
                       leftAbove->monotone->lowerBase &&
                       leftAbove->monotone->lowerBase->y == yb)
                    {
                        leftAbove->monotone->lowerBase->vertexRight = thisAbove->vertex;
                    }
                }
                else
                {
                    growMonotoneAndConnect(leftAbove, thisAbove->vertex, true);
                    startMonotone(thisAbove, thisAbove->vertex | LeftMask);
                }
                leftAbove = thisAbove;
                ++above;
                break;

            case ChainEndsAtScanline:                   // Above    |
                                                        // Below  |   end chain (merge)
                growMonotoneAndConnect(leftBelow, thisBelow->vertex, true);
                growMonotoneAndConnect(thisBelow, thisBelow->vertex | LeftMask, true);
                if(upperBase.numChains)
                {
                    connectStarting(leftBelow, &upperBase);
                }
                addPendingEnd(leftAbove, thisBelow, yb);
                prevAbove = ~0U;
                leftBelow = thisBelow;
                ++below;
                break;
            }
        }
        else
        if(above < ChainsAbove.GetSize())               // Above    | 
        {                                               // Below  |   start chain
            thisAbove = &ChainsAbove[above];
            growMonotoneAndConnect(leftAbove, thisAbove->vertex, true);
            startMonotone(thisAbove, thisAbove->vertex | LeftMask);
            leftAbove = thisAbove;
            ++above;
        }
        else
        if(below < ChainsBelow.GetSize())               // Above  | 
        {                                               // Below    | end chain
            thisBelow = &ChainsBelow[below];
            growMonotoneAndConnect(leftBelow, thisBelow->vertex, true);
            growMonotoneAndConnect(thisBelow, thisBelow->vertex | LeftMask, true);
            if(upperBase.numChains)
            {
                connectStarting(leftBelow, &upperBase);
            }
            addPendingEnd(leftAbove, thisBelow, yb);
            prevAbove = ~0U;
            leftBelow = thisBelow;
            ++below;
        }
        else
        {
            break;
        }
    }

    ChainsBelow = ChainsAbove;
    for(i = 0; i < aet.GetSize(); ++i)
    {
        mc = aet[i];
        mc->leftBelow  = mc->leftAbove;
        mc->rightBelow = mc->rightAbove;
    }
}

//-----------------------------------------------------------------------
void GTessellator::swapChains(unsigned startIn, unsigned endIn)
{
    unsigned i;
    for(i = startIn; i < endIn; i++)
    {
        const IntersectionType& in = Intersections[i];
        G_Swap(InteriorChains[InteriorOrder[in.pos1]],
               InteriorChains[InteriorOrder[in.pos2]]);
        G_Swap(InteriorOrder[in.pos1],
               InteriorOrder[in.pos2]);
    }
}

//-----------------------------------------------------------------------
void GTessellator::processInterior(CoordType yb, CoordType yTop, 
                                   unsigned perceiveFlag)
{

    unsigned startIn = 0;
    unsigned endIn   = 0;
    const IntersectionType* in;
    CoordType yt = yb;
    
    // Skip all intersections at yb
    while(endIn < Intersections.GetSize())
    {
        in = &Intersections[endIn];
        yt = in->y;
        if(yt > yb) break;
        perceiveFlag = 1;
        ++endIn;
    }
    swapChains(startIn, endIn);

    if(perceiveFlag) 
    {
        perceiveStyles(InteriorChains);
    }

    while(endIn < Intersections.GetSize())
    {
        CoordType yt2 = yt;
        startIn = endIn;
        while(endIn < Intersections.GetSize())
        {
            in = &Intersections[endIn];
            yt2 = in->y;
            if(yt2 > yt) break;
            ++endIn;
        }
        perceiveStyles(InteriorChains);
        sweepScanbeam(InteriorChains, yb);
        swapChains(startIn, endIn);
        yb = yt;
        yt = yt2;
    }

    perceiveStyles(ActiveChains);

    if(yt < yTop)
    {
        sweepScanbeam(ActiveChains, yt);
    }
}


//-----------------------------------------------------------------------
UPInt GTessellator::GetNumBytes() const
{
    return 
        HistHor.GetNumBytes() +
        HistVer.GetNumBytes() +
        VisibleStyles.GetNumBytes() +
        Vertices.GetNumBytes() +
        Paths.GetNumBytes() +
        Edges.GetNumBytes() +
        MonoChains.GetNumBytes() +
        MonoChainsSorted.GetNumBytes() +
        Scanbeams.GetNumBytes() +
        ActiveChains.GetNumBytes() +
        InteriorChains.GetNumBytes() +
        ValidChains.GetNumBytes() +
        InteriorOrder.GetNumBytes() +
        Intersections.GetNumBytes() +
        StyleCounts.GetNumBytes() +
        SwapEdgesTable.GetNumBytes() +
        EventVertices.GetNumBytes() +
        ChainsBelow.GetNumBytes() +
        ChainsAbove.GetNumBytes() +
        BaseLines.GetNumBytes() +
        PendingEnds.GetNumBytes() +
        Monotones.GetNumBytes() +
        MonoVertices.GetNumBytes() +
        MonoStack.GetNumBytes() +
        Triangles.GetNumBytes() +
        LeftVertices.GetNumBytes() +
        RightVertices.GetNumBytes() +
        ConvexShape.GetNumBytes();
}

//-----------------------------------------------------------------------
void GTessellator::ClearAndRelease()
{
    Clear();
    HistHor.ClearAndRelease();
    HistVer.ClearAndRelease();
    VisibleStyles.ClearAndRelease();
    Vertices.ClearAndRelease();
    Paths.ClearAndRelease();
    Edges.ClearAndRelease();
    MonoChains.ClearAndRelease();
    MonoChainsSorted.ClearAndRelease();
    Scanbeams.ClearAndRelease();
    ActiveChains.ClearAndRelease();
    InteriorChains.ClearAndRelease();
    ValidChains.ClearAndRelease();
    InteriorOrder.ClearAndRelease();
    Intersections.ClearAndRelease();
    StyleCounts.ClearAndRelease();
    SwapEdgesTable.ClearAndRelease();
    EventVertices.ClearAndRelease();
    ChainsBelow.ClearAndRelease();
    ChainsAbove.ClearAndRelease();
    BaseLines.ClearAndRelease();
    PendingEnds.ClearAndRelease();
    Monotones.ClearAndRelease();
    MonoVertices.ClearAndRelease();
    MonoStack.ClearAndRelease();
    Triangles.ClearAndRelease();
    LeftVertices.ClearAndRelease();
    RightVertices.ClearAndRelease();
    ConvexShape.ClearAndRelease();
}

//-----------------------------------------------------------------------
void GTessellator::Clear()
{
    Vertices.Clear();
    Paths.Clear();
    Edges.Clear();
    MonoChains.Clear();
    MonoChainsSorted.Clear();
    Scanbeams.Clear();
    EventVertices.Clear();
    Monotones.Clear();
    MonoVertices.Clear();
    MonoStack.Clear();
    Triangles.Clear();
    MinStyle = 0;
    MaxStyle = 0;
    MinX = G_TessellatorMaxCoord;
    MinY = G_TessellatorMaxCoord;
    MaxX = G_TessellatorMinCoord;
    MaxY = G_TessellatorMinCoord;
    SwapCoordinates = false;
}

//-----------------------------------------------------------------------
void GTessellator::AddShape(const GCompoundShape& shape, int addStyle)
{
    unsigned i;
    for(i = 0; i < shape.GetNumPaths(); ++i)
    {
        const GCompoundShape::SPath& path = shape.GetPath(i);
        unsigned ls = path.GetLeftStyle()  + addStyle;
        unsigned rs = path.GetRightStyle() + addStyle;
        if(ls != rs)
        {
            addPath(path, addStyle);
            if(ls > MaxStyle) MaxStyle = ls;
            if(rs > MaxStyle) MaxStyle = rs;
        }
    }
}


//-----------------------------------------------------------------------
void GTessellator::optimizeDirection()
{
    // These constants may be adjustable variables 
    //-------------------
    //const CoordType narrowness = (CoordType)1.75;
    enum 
    { 
        histSize = 64,
        minVerticesToCheckNarrowness = 2000
    };

    CoordType dx = MaxX - MinX;
    CoordType dy = MaxY - MinY;
    SwapCoordinates = false;

    if(dx > 0 && dy > 0)
    {
        //bool optimize = false;
        //if(dx > dy)
        //{
        //    if(dy * narrowness < dx) optimize = true;
        //}
        //else
        //{
        //    if(dx * narrowness < dy) optimize = true;
        //}

        //if(Vertices.GetSize() > histSize && 
        //  (Vertices.GetSize() > minVerticesToCheckNarrowness || optimize))
        if(Vertices.GetSize() > histSize)
        {
            unsigned i, j, pathIdx;
            HistVer.Resize(histSize + 2);
            HistHor.Resize(histSize + 2);
            HistVer.Zero();
            HistHor.Zero();
            CoordType kx = histSize / dx;
            CoordType ky = histSize / dy;
            for(pathIdx = 0; pathIdx < Paths.GetSize(); ++pathIdx)
            {
                const PathInfoType& path = Paths[pathIdx];
                for(i = path.start + 1; (int)i <= path.end; ++i)
                {
                    const VertexType& v0 = Vertices[i - 1];
                    const VertexType& v1 = Vertices[i];
                    unsigned i1, i2;

                    i1 = GMath2D::URound((v0.x - MinX) * kx);
                    i2 = GMath2D::URound((v1.x - MinX) * kx);
                    if(i1 > i2) G_Swap(i1, i2);
                    for(j = i1; j < i2; ++j) ++HistHor[j];

                    i1 = GMath2D::URound((v0.y - MinY) * ky);
                    i2 = GMath2D::URound((v1.y - MinY) * ky);
                    if(i1 > i2) G_Swap(i1, i2);
                    for(j = i1; j < i2; ++j) ++HistVer[j];
                }
            }

            unsigned ratioHor = 0;
            unsigned ratioVer = 0;
            for(i = 0; i < histSize; i++)
            {
                ratioHor += HistHor[i];
                ratioVer += HistVer[i];
            }
            if(ratioHor < ratioVer)
            {
                CoordType t;
                for(i = 0; i < Vertices.GetSize(); ++i)
                {
                    VertexType& v = Vertices[i];
                    t = v.x; v.x = -v.y; v.y = t;
                }
                t = MinX; MinX = -MinY; MinY = t;
                t = MaxX; MaxX = -MaxY; MaxY = t;
                t = MinX; MinX =  MaxX; MaxX = t;
                SwapCoordinates = true;
            }
        }
    }
}


//-----------------------------------------------------------------------
void GTessellator::prepareChainsAndScanbeams()
{
    // Sort the Scanbeams by Y
    //-----------------
    CmpScanbeams cmp(Vertices);
    G_QuickSort(Scanbeams, cmp);

    // Calculate Epsilons as the maximal absolute 
    // value of the bounding box.
    //-----------------
    CoordType c1;
    CoordType c2;
    c1 = (MinX < 0) ? -MinX : MinX;
    c2 = (MaxX < 0) ? -MaxX : MaxX;
    EpsilonX = G_TessellatorEpsilon * ((c2 > c1) ? c2 : c1);

    c1 = (MinY < 0) ? -MinY : MinY;
    c2 = (MaxY < 0) ? -MaxY : MaxY;
    EpsilonY = G_TessellatorEpsilon * ((c2 > c1) ? c2 : c1);

    // Clean up the coordinates. 
    // When two Y values are too close just make them exactly equal 
    // and remove the respective elements from Scanbeams.
    //-----------------
    CoordType y1 = G_TessellatorMinCoord;

    unsigned i, j;
    for(i = j = 0; i < Scanbeams.GetSize(); ++i)
    {
        VertexType& v2 = Vertices[Scanbeams[i]];
        if(v2.y - y1 > EpsilonY)
        {
            Scanbeams[j++] = Scanbeams[i];
            y1 = v2.y;
        }
        else
        {
            v2.y = y1;
        }
    }
    Scanbeams.CutAt(j);

    // Create MonoChains and Edges arrays
    //------------------
    for(i = 0; i < Paths.GetSize(); ++i)
    {
        decomposePath(Paths[i]);
    }

    // Fill MonoChainsSorted array and assign new, possibly 
    // modified ySort values.
    //------------------
    MonoChainsSorted.Resize(MonoChains.GetSize(), 32);
    for(i = 0; i < MonoChains.GetSize(); ++i)
    {
        MonoChainType* mc = &MonoChains[i];
        mc->ySort = Vertices[mc->edge->lower].y;
        MonoChainsSorted[i] = mc;
    }
    G_QuickSort(MonoChainsSorted, monoChainLess);
}

//-----------------------------------------------------------------------
void GTessellator::Monotonize()
{
    if(Scanbeams.GetSize() == 0) return;

    StyleCounts.Resize(MaxStyle + 1, 32);
    SetStyleVisibility(MaxStyle + 1, true);

    LastVertex.x = G_TessellatorMinCoord;
    LastVertex.y = G_TessellatorMinCoord;

    if(OptimizeDirection)
    {
        optimizeDirection();
    }
    prepareChainsAndScanbeams();

    unsigned sb = 0; // Scan beam index
    unsigned mc = 0; // Monotone Chain index
    CoordType yb, yt;

    ActiveChains.Reserve(MonoChains.GetSize(), 32);
    InteriorChains.Reserve(MonoChains.GetSize(), 32);
    ValidChains.Reserve(MonoChains.GetSize(), 32);
    InteriorOrder.Resize(MonoChains.GetSize(), 32);
    ChainsBelow.Reserve(MonoChains.GetSize(), 32);
    ChainsAbove.Reserve(MonoChains.GetSize(), 32);

    BaseLines.Clear();
    PendingEnds.Clear();

    yb = yt = Vertices[Scanbeams[0]].y;

    // Perform scanbeam processing loop
    //----------------
    while(sb < Scanbeams.GetSize())
    {
        if(++sb < Scanbeams.GetSize())
        {
            yt = Vertices[Scanbeams[sb]].y;
        }

        // Old code; removing duplicates on-the-fly.
        //while(++sb < Scanbeams.GetSize())
        //{
        //    yt = Vertices[Scanbeams[sb]].y;
        //    if(yt > yb) break;
        //}

        // While monotone chains corresponding to yb exist
        unsigned startMc = mc;
        while(mc < MonoChainsSorted.GetSize() && 
              MonoChainsSorted[mc]->ySort <= yb) ++mc;

        unsigned flags = nextScanbeam(yb, yt, startMc, mc - startMc);

        if(Intersections.GetSize())
        {
            processInterior(yb, yt, flags);
        }
        else
        {
            if(flags) 
            {
                perceiveStyles(ActiveChains);
            }
            sweepScanbeam(ActiveChains, yb);
        }

        if(flags & RemoveEdgesFlag)
        {
            unsigned i, pos;
            for(i = pos = 0; i < ActiveChains.GetSize(); ++i)
            {
                MonoChainType* mc = ActiveChains[i];
                if((mc->flags & EndChainFlag) == 0)
                {
                    ActiveChains[pos++] = mc;
                }
            }
            ActiveChains.CutAt(pos);
        }
        yb = yt; // Advance yb
    }

    if(SwapCoordinates)
    {
        unsigned i;
        for(i = 0; i < EventVertices.GetSize(); ++i)
        {
            VertexType& v = EventVertices[i];
            CoordType t = v.y; v.y = -v.x; v.x = t;
        }
    }

}


/*
#include <stdio.h>
static void DumpCompoundShape(const GCompoundShape& shape, 
                              const char* file, 
                              const char* mode)
{
    FILE* fd = fopen(file, mode);
    fprintf(fd, "=======BeginShape\n");
    unsigned i;
    for(i = 0; i < shape.GetNumPaths(); i++)
    {
        const GCompoundShape::SPath& path = shape.GetPath(i);
        GPointType v = path.GetVertex(0);
        fprintf(fd, "Path %d %d %d %.16f %.16f\n",
                     path.GetLeftStyle(), 
                     path.GetRightStyle(), 
                     path.GetLineStyle(), 
                     v.x, v.y);
        unsigned j;
        for(j = 1; j < path.GetNumVertices(); j++)
        {
            v = path.GetVertex(j);
            fprintf(fd, "Line %.16f %.16f\n", v.x, v.y);
        }
        fprintf(fd, "<-------EndPath\n");
    }
    fprintf(fd, "!======EndShape\n");
    fclose(fd);
}
*/

//-----------------------------------------------------------------------
void GTessellator::Monotonize(const GCompoundShape& shape, int addStyle)
{
//DumpCompoundShape(shape, "compound_shape", "at");
    Clear();
    AddShape(shape, addStyle);
    Monotonize();
}

//-----------------------------------------------------------------------
void GTessellator::SortMonotonesByStyle()
{
    G_QuickSort(Monotones, styleLess);
}

//-----------------------------------------------------------------------
inline GTessellator::CoordType 
GTessellator::triangleCrossProduct(unsigned v1, 
                                   unsigned v2, 
                                   unsigned v3) const
{
    return GMath2D::CrossProduct(EventVertices[v1 & ~LeftMask],
                                 EventVertices[v2 & ~LeftMask], 
                                 EventVertices[v3 & ~LeftMask]);
}


//-----------------------------------------------------------------------
bool GTessellator::shapeCoherent(unsigned v)
{
//return false;
    CoordType cp = 0;
    bool left = isLeft(v);

    const VertexType* l1  = 0;
    const VertexType* l2  = 0;
    const VertexType* r1  = 0;
    const VertexType* r2  = 0;
    const VertexType& ver = EventVertices[v & ~LeftMask];

    if(left)
    {
        if(LeftVertices.GetSize() < 2) return true;
        l1 = &EventVertices[LeftVertices[LeftVertices.GetSize() - 2]];
        l2 = &EventVertices[LeftVertices[LeftVertices.GetSize() - 1]];
        cp = GMath2D::CrossProduct(*l1, *l2, ver);
        if(cp == 0) return true;
        if(LeftCoherence == CoherenceUndefined) 
        {
            LeftCoherence = (cp < 0) ? CoherenceLeftCurve : CoherenceRightCurve;
        }
        else
        {
            if((LeftCoherence == CoherenceLeftCurve) != (cp < 0))
            {
                return false;
            }
        }
    }
    else
    {
        if(RightVertices.GetSize() < 2) return true;
        r1 = &EventVertices[RightVertices[RightVertices.GetSize() - 2]];
        r2 = &EventVertices[RightVertices[RightVertices.GetSize() - 1]];
        cp = GMath2D::CrossProduct(*r1, *r2, ver);
        if(cp == 0) return true;
        if(RightCoherence == CoherenceUndefined) 
        {
            RightCoherence = (cp < 0) ? CoherenceLeftCurve : CoherenceRightCurve;
        }
        else
        {
            if((RightCoherence == CoherenceLeftCurve) != (cp < 0))
            {
                return false;
            }
        }
    }

    if(ShapeCoherence == CoherenceUndefined)
    {
        if(LeftCoherence != CoherenceUndefined && 
           RightCoherence != CoherenceUndefined)
        {
            Coherence_e coherences[4] = 
            {
                CoherenceLeftCurve, 
                CoherencePincushion, 
                CoherenceConvex, 
                CoherenceRightCurve
            };
            unsigned cohCode = ((LeftCoherence == CoherenceRightCurve) << 1) | 
                                (RightCoherence == CoherenceRightCurve);
            ShapeCoherence = coherences[cohCode];
        }
    }

    switch(ShapeCoherence)
    {
    case CoherenceConvex:
        if(left)
        {
            r1 = &EventVertices[RightVertices[RightVertices.GetSize() - 2]];
            r2 = &EventVertices[RightVertices[RightVertices.GetSize() - 1]];
            if(GMath2D::CrossProduct(*r1, *r2, ver) > 0)
            {
                return false;
            }
        }
        else
        {
            l1 = &EventVertices[LeftVertices[LeftVertices.GetSize() - 2]];
            l2 = &EventVertices[LeftVertices[LeftVertices.GetSize() - 1]];
            if(GMath2D::CrossProduct(*l1, *l2, ver) < 0)
            {
                return false;
            }
        }
        break;

    case CoherenceLeftCurve:
        r1 = &EventVertices[RightVertices[RightVertices.GetSize() - 2]];
        r2 = &EventVertices[RightVertices[RightVertices.GetSize() - 1]];
        if(left && GMath2D::CrossProduct(*r1, *r2, ver) > 0)
        {
            return false;
        }
        break;

    case CoherenceRightCurve:
        l1 = &EventVertices[LeftVertices[LeftVertices.GetSize() - 2]];
        l2 = &EventVertices[LeftVertices[LeftVertices.GetSize() - 1]];
        if(!left && GMath2D::CrossProduct(*l1, *l2, ver) < 0)
        {
            return false;
        }
        break;

    default:
        break;
    }
    return true;
}


////-----------------------------------------------------------------------
//GTessellator::CoordType GTessellator::calcTriangleRatio(const VertexType& v1,
//                                                        const VertexType& v2,
//                                                        const VertexType& v3) const
//{
//    CoordType dx, dy;
//    CoordType perimeter2 = 0;
//    dx = v2.x - v1.x; dy = v2.y - v1.y; perimeter2 += dx*dx + dy*dy;
//    dx = v3.x - v2.x; dy = v3.y - v2.y; perimeter2 += dx*dx + dy*dy;
//    dx = v1.x - v3.x; dy = v1.y - v3.y; perimeter2 += dx*dx + dy*dy;
//    CoordType area = (v1.x*v2.y - v2.x*v1.y + v2.x*v3.y - 
//                      v3.x*v2.y + v3.x*v1.y - v1.x*v3.y);
//    return (area * area) / perimeter2;
//}



////-----------------------------------------------------------------------
//GTessellator::CoordType GTessellator::triangleArea(unsigned p, 
//                                                   unsigned q,
//                                                   unsigned r) const
//{
//    const VertexType& v1 = EventVertices[ConvexShape[p]];
//    const VertexType& v2 = EventVertices[ConvexShape[q]];
//    const VertexType& v3 = EventVertices[ConvexShape[r]];
//    return -((v2.x - v1.x) * (v3.y - v1.y) - (v3.x - v1.x) * (v2.y - v1.y));
//}



//-----------------------------------------------------------------------
inline GTessellator::CoordType 
GTessellator::distanceSquare(const VertexType& v1, const VertexType& v2)
{
    GCoordType dx = v2.x - v1.x;
    GCoordType dy = v2.y - v1.y;
    return dx*dx + dy*dy;
}


//-----------------------------------------------------------------------
void GTessellator::findMaxDiameter(unsigned* p, unsigned* q) const
{
    unsigned i;
    unsigned p0 = *p;
    unsigned q0 = *q;
    unsigned p1 = *p;
    unsigned q1 = *q;
    unsigned nv = (unsigned)ConvexShape.GetSize();
    CoordType dMax = distanceSquare(EventVertices[ConvexShape[p0]], 
                                    EventVertices[ConvexShape[q0]]);

    for(i = 0; i < nv; ++i)
    {
        unsigned p2 = p1 + 1;
        unsigned q2 = q1 + 1;

        if(p2 >= nv) p2 -= nv;
        if(q2 >= nv) q2 -= nv;

        const VertexType& vp1 = EventVertices[ConvexShape[p1]];
        const VertexType& vq1 = EventVertices[ConvexShape[q1]];
        const VertexType& vp2 = EventVertices[ConvexShape[p2]];
        const VertexType& vq2 = EventVertices[ConvexShape[q2]];

        CoordType d1 = distanceSquare(vp2, vq1);
        CoordType d2 = distanceSquare(vp1, vq2);
        CoordType d3 = distanceSquare(vp2, vq2);

        CoordType d = d1;
        unsigned  c = 0;
        if(d2 > d) { d = d2; c = 1; }
        if(d3 > d) { d = d3; c = 2; }

        switch(c)
        {
            case 0: p1 = p2; break;
            case 1: q1 = q2; break;
            case 2: p1 = p2; q1 = q2; ++i; break;
        }

        if(d > dMax)
        {
            dMax = d;
            *p = p1;
            *q = q1;
        }
    }
}


//-----------------------------------------------------------------------
inline void GTessellator::addTriangle(const TriangleType& t)
{
    if(t.v1 != t.v2 && t.v2 != t.v3 && t.v3 != t.v1) 
    {
        Triangles.PushBack(t);
    }
}


//-----------------------------------------------------------------------
void GTessellator::triangulateCoherentCurve()
{
    TriangleType t1, t2;
    unsigned lastLeft  = 1;
    unsigned lastRight = 1;

    t1.v1 = LeftVertices[0];
    t1.v2 = RightVertices[1];
    t1.v3 = LeftVertices[1];
    addTriangle(t1);

    unsigned topLeft  =  (unsigned)LeftVertices.GetSize() - 1;
    unsigned topRight =  (unsigned)RightVertices.GetSize() - 1;

    while(lastLeft < topLeft || lastRight < topRight)
    {
        if(lastLeft < topLeft && lastRight < topRight)
        {
            t1.v1 = LeftVertices [lastLeft];
            t1.v2 = RightVertices[lastRight];
            t1.v3 = LeftVertices [lastLeft + 1];

            t2.v1 = LeftVertices [lastLeft];
            t2.v2 = RightVertices[lastRight];
            t2.v3 = RightVertices[lastRight + 1];

            const VertexType& v1 = EventVertices[t1.v1];
            const VertexType& v2 = EventVertices[t1.v2];
            const VertexType& v3 = EventVertices[t1.v3];
            const VertexType& v4 = EventVertices[t2.v3];

            bool t1Valid = GMath2D::CrossProduct(v1, v2, v3) <= 0; 
            bool t2Valid = GMath2D::CrossProduct(v1, v2, v4) <= 0; 

            if(t1Valid && t2Valid)
            {
                t1Valid = GMath2D::CrossProduct(v2, v4, v3) <= 0; 
                t2Valid = GMath2D::CrossProduct(v1, v3, v4) >= 0; 
            }

            if(t1Valid && t2Valid)
            {
                if(distanceSquare(v2, v3) < distanceSquare(v1, v4))
                {
                    addTriangle(t1);
                    ++lastLeft;
                }
                else
                {
                    addTriangle(t2);
                    ++lastRight;
                }
            }
            else
            if(t1Valid)
            {
                addTriangle(t1);
                ++lastLeft;
            }
            else
            {
                addTriangle(t2);
                ++lastRight;
            }
        }
        else
        if(lastLeft < topLeft)
        {
            t1.v1 = LeftVertices[lastLeft];
            t1.v2 = RightVertices[lastRight];
            t1.v3 = LeftVertices[++lastLeft];
            addTriangle(t1);
        }
        else
        {
            t1.v1 = LeftVertices[lastLeft];
            t1.v2 = RightVertices[lastRight];
            t1.v3 = RightVertices[++lastRight];
            addTriangle(t1);
        }
    }
}

//-----------------------------------------------------------------------
void GTessellator::triangulateConvexShape()
{
    TriangleType t1, t2;
    unsigned lastLeft;
    unsigned lastRight;

    ConvexShape.Clear();
    unsigned i;
    unsigned p = 0;
    for(i = 0; i < RightVertices.GetSize(); ++i)
    {
        ConvexShape.PushBack(RightVertices[i]);
    }

    unsigned q = (unsigned)ConvexShape.GetSize() - 1;
    if(EventVertices[LeftVertices.Back()].y > EventVertices[ConvexShape[q]].y)
    {
        ++q;
    }

    for(i = (unsigned)LeftVertices.GetSize() - 1; i; --i)
    {
        ConvexShape.PushBack(LeftVertices[i]);
    }

    if(LeftVertices[0] != RightVertices[0])
    {
        ConvexShape.PushBack(LeftVertices[0]);
    }

    findMaxDiameter(&p, &q);

    unsigned nv = (unsigned)ConvexShape.GetSize();

    lastLeft  = (p + nv - 1) % nv;
    lastRight = (p + 1)      % nv;

    t1.v1 = ConvexShape[p];
    t1.v2 = ConvexShape[lastRight];
    t1.v3 = ConvexShape[lastLeft];
    addTriangle(t1);

    unsigned nextLeft;
    unsigned nextRight;
    while(lastLeft != q || lastRight != q)
    {
        if(lastLeft != q && lastRight != q)
        {
            nextLeft  = (lastLeft + nv - 1) % nv;
            nextRight = (lastRight + 1)     % nv;

            t1.v1 = ConvexShape[lastLeft];
            t1.v2 = ConvexShape[lastRight];
            t1.v3 = ConvexShape[nextLeft];

            t2.v1 = ConvexShape[lastLeft];
            t2.v2 = ConvexShape[lastRight];
            t2.v3 = ConvexShape[nextRight];

            const VertexType& v1 = EventVertices[t1.v1];
            const VertexType& v2 = EventVertices[t1.v2];
            const VertexType& v3 = EventVertices[t1.v3];
            const VertexType& v4 = EventVertices[t2.v3];

            if(distanceSquare(v2, v3) < distanceSquare(v1, v4))
            {
                addTriangle(t1);
                lastLeft = nextLeft;
            }
            else
            {
                addTriangle(t2);
                lastRight = nextRight;
            }
        }
        else
        if(lastLeft != q)
        {
            nextLeft = (lastLeft + nv - 1) % nv;
            t1.v1 = ConvexShape[lastLeft];
            t1.v2 = ConvexShape[lastRight];
            t1.v3 = ConvexShape[nextLeft];
            addTriangle(t1);
            lastLeft = nextLeft;
        }
        else
        {
            nextRight = (lastRight + 1) % nv;
            t1.v1 = ConvexShape[lastLeft];
            t1.v2 = ConvexShape[lastRight];
            t1.v3 = ConvexShape[nextRight];
            addTriangle(t1);
            lastRight = nextRight;
        }
    }
}


//-----------------------------------------------------------------------
void GTessellator::processCoherentShape()
{
    if(ShapeCoherence == CoherenceLeftCurve || 
       ShapeCoherence == CoherenceRightCurve)
    {
        triangulateCoherentCurve();
    }
    else
    if(ShapeCoherence == CoherenceConvex)
    {
        triangulateConvexShape();
    }
/*
// TO DO: Remove
CoherentPartType cp;
cp.size = 0;
cp.start = CoherentVertices.GetSize();
cp.type = ShapeCoherence;
unsigned i;
for(i = 0; i < LeftVertices.GetSize(); ++i)
{
    CoherentVertices.PushBack(LeftVertices[i]);
    ++cp.size;
}
for(i = RightVertices.GetSize() - 1; i; --i)
{
    CoherentVertices.PushBack(RightVertices[i]);
    ++cp.size;
}
CoherentParts.PushBack(cp);
*/
}


//-----------------------------------------------------------------------
void GTessellator::createCoherentNucleus(unsigned v1, unsigned v2, unsigned v3)
{
    bool left1 = isLeft(v1);
    bool left2 = isLeft(v2);

    v1 &= ~LeftMask;
    v2 &= ~LeftMask;
    v3 &= ~LeftMask;

    if(left1 == left2)
    {
        LeftVertices.PushBack(v2);
        RightVertices.PushBack(v2);
        if(left1)
        {
            LeftVertices.PushBack(v1);
            RightVertices.PushBack(v3);
        }
        else 
        {
            RightVertices.PushBack(v1);
            LeftVertices.PushBack(v3);
        }
    }
    else
    {
        LeftVertices.PushBack(v3);
        RightVertices.PushBack(v3);
        if(left1)
        {
            LeftVertices.PushBack(v1);
            RightVertices.PushBack(v2);
        }
        else 
        {
            RightVertices.PushBack(v1);
            LeftVertices.PushBack(v2);
        }
    }
}






//-----------------------------------------------------------------------
void GTessellator::addTriangle(unsigned v1, unsigned v2, unsigned v3)
{
    TriangleType t2;
    t2.v1 = v1 & ~LeftMask;
    t2.v2 = v2 & ~LeftMask;
    t2.v3 = v3 & ~LeftMask;
    if(t2.v1 != t2.v2 && t2.v2 != t2.v3 && t2.v3 != t2.v1) 
    {
/*
CoordType cp = triangleCrossProduct(v1, v2, v3);
if(cp > 0) 
{
//    G_Swap(v2, v3);
    t2.v1 = v1 & ~LeftMask;
    t2.v2 = v2 & ~LeftMask;
    t2.v3 = v3 & ~LeftMask;
}
*/


        if(OptimizeCoherence)
        {
            if(Triangles.GetSize() == TrianglesCutOff)
            {
                createCoherentNucleus(v1, v2, v3);
            }
            else
            {
                const TriangleType& t1 = Triangles.Back();
                bool triangleCoherent = (t1.v2 == t2.v2 && t1.v1 == t2.v3) || 
                                        (t1.v3 == t2.v3 && t1.v1 == t2.v2);
                if(triangleCoherent && shapeCoherent(v1))
                {
                    if(isLeft(v1)) LeftVertices.PushBack(t2.v1);
                    else           RightVertices.PushBack(t2.v1);
                }
                else
                {
                    if(LeftVertices.GetSize() > 3 && 
                       RightVertices.GetSize() > 3 && 
                       ShapeCoherence > CoherencePincushion)
                    {
                        Triangles.CutAt(TrianglesCutOff);
                        processCoherentShape();
                    }
                    LeftVertices.Clear();
                    RightVertices.Clear();
                    TrianglesCutOff = (unsigned)Triangles.GetSize();
                    ShapeCoherence  = CoherenceUndefined;
                    LeftCoherence   = CoherenceUndefined;
                    RightCoherence  = CoherenceUndefined;
                    createCoherentNucleus(v1, v2, v3);
                }
            }
        }
        Triangles.PushBack(t2);
    }
}

//-----------------------------------------------------------------------
inline bool GTessellator::addTrianglePilot(unsigned v1, unsigned v2, unsigned v3)
{
    TriangleType t;
    t.v1 = v1 & ~LeftMask;
    t.v2 = v2 & ~LeftMask;
    t.v3 = v3 & ~LeftMask;
    CoordType cp = triangleCrossProduct(v1, v2, v3);
    if(cp != 0)
    {
        if(cp > 0) return false;
        Triangles.PushBack(t);
    }
    return true;
}

//-----------------------------------------------------------------------
inline void GTessellator::addTrianglePilot(unsigned v1, unsigned v2, unsigned v3,
                                           CoordType cp)
{
    if(cp != 0)
    {
        TriangleType t;
        t.v1 = v1 & ~LeftMask;
        t.v2 = v2 & ~LeftMask;
        t.v3 = v3 & ~LeftMask;
        Triangles.PushBack(t);
    }
}

//-----------------------------------------------------------------------
bool GTessellator::triangulateMonotonePilot(unsigned idx)
{
    Triangles.Clear();
    LeftVertices.Clear();
    RightVertices.Clear();
    TrianglesCutOff = 0;
    ShapeCoherence  = CoherenceUndefined;
    LeftCoherence   = CoherenceUndefined;
    RightCoherence  = CoherenceUndefined;

    const MonotoneType& monotone = Monotones[idx];
    const MonoVertexType* vertex = monotone.start;

    if(vertex == 0) return true;

    bool convex = true;
    const VertexType* vl1 = &EventVertices[vertex->vertex & ~LeftMask];
    const VertexType* vl2 = vl1;
    const VertexType* vr1 = vl1;
    const VertexType* vr2 = vl1;
    const VertexType* v3;
    while(vertex)
    {
        if(isLeft(vertex->vertex)) 
        {
            LeftVertices.PushBack(vertex->vertex & ~LeftMask);
            if(convex)
            {
                v3 = &EventVertices[vertex->vertex & ~LeftMask];
                if(GMath2D::CrossProduct(*vl1, *vl2, *v3) < 0)
                {
                    convex = false;
                }
                vl1 = vl2;
                vl2 = v3;
            }
        }
        else 
        {
            RightVertices.PushBack(vertex->vertex);
            if(convex)
            {
                v3 = &EventVertices[vertex->vertex];
                if(GMath2D::CrossProduct(*vr1, *vr2, *v3) > 0)
                {
                    convex = false;
                }
                vr1 = vr2;
                vr2 = v3;
            }
        }
        vertex = vertex->next;
    }


    if(LeftVertices.GetSize() < 2 || RightVertices.GetSize() < 2)
    {
        return false;
    }

    if(convex)
    {
        triangulateConvexShape();
        return true;
    }

    unsigned iLeft  = 1;
    unsigned iRight = LeftVertices[0] == RightVertices[0];

    MonoStack.Clear();
    MonoStack.PushBack(LeftVertices[0] | LeftMask);

    if(LeftVertices[iLeft] < RightVertices[iRight])
    {
        MonoStack.PushBack(LeftVertices[iLeft++] | LeftMask);
    }
    else
    {
        MonoStack.PushBack(RightVertices[iRight++]);
    }

    unsigned p1, p2;
    unsigned topLeft  =  (unsigned)LeftVertices.GetSize();
    unsigned topRight =  (unsigned)RightVertices.GetSize();
    TriangleType t1, t2;

    while(iLeft < topLeft || iRight < topRight)
    {
        unsigned topStrip = ~0U;
        if(iLeft < topLeft && iRight < topRight)
        {
            if(iRight == 0)
            {
                if(LeftVertices[iLeft] < RightVertices[iRight])
                {
                    topStrip = LeftVertices[iLeft++] | LeftMask;
                }
                else
                {
                    topStrip = RightVertices[iRight++];
                }
            }
            else
            {
                t1.v1 = LeftVertices [iLeft  - 1];
                t1.v2 = RightVertices[iRight - 1];
                t1.v3 = LeftVertices [iLeft];

                t2.v1 = LeftVertices [iLeft  - 1];
                t2.v2 = RightVertices[iRight - 1];
                t2.v3 = RightVertices[iRight];

                const VertexType& v1 = EventVertices[t1.v1];
                const VertexType& v2 = EventVertices[t1.v2];
                const VertexType& v3 = EventVertices[t1.v3];
                const VertexType& v4 = EventVertices[t2.v3];

                bool t1Valid = GMath2D::CrossProduct(v1, v2, v3) <= 0; 
                bool t2Valid = GMath2D::CrossProduct(v1, v2, v4) <= 0; 

                if(t1Valid && t2Valid)
                {
                    t1Valid = GMath2D::CrossProduct(v2, v4, v3) <= 0; 
                    t2Valid = GMath2D::CrossProduct(v1, v3, v4) >= 0; 
                }

                if(t1Valid && t2Valid)
                {
                    if(distanceSquare(v2, v3) < distanceSquare(v1, v4))
                    {
                        topStrip = LeftVertices[iLeft++] | LeftMask;
                    }
                    else
                    {
                        topStrip = RightVertices[iRight++];
                    }
                }
                else
                if(t1Valid)
                {
                    topStrip = LeftVertices[iLeft++] | LeftMask;
                }
                else
                if(t2Valid)
                {
                    topStrip = RightVertices[iRight++];
                }
                else
                {
                    return false;
                }
            }
        }
        else
        if(iLeft < topLeft)
        {
            topStrip = LeftVertices[iLeft++] | LeftMask;
        }
        else
        {
            topStrip = RightVertices[iRight++];
        }

        unsigned topStack = MonoStack.Back();
        if(isLeft(topStrip) != isLeft(topStack))
        {
            while(MonoStack.GetSize() > 1)
            {
                p1 = MonoStack.Back();
                MonoStack.PopBack();
                if(isLeft(topStrip)) 
                {
                    if(!addTrianglePilot(topStrip, MonoStack.Back(), p1)) return false;
                }
                else                 
                {
                    if(!addTrianglePilot(topStrip, p1, MonoStack.Back())) return false;
                }
            }
            MonoStack.PopBack();
            MonoStack.PushBack(topStack);
            MonoStack.PushBack(topStrip);
        }
        else
        {
            while(MonoStack.GetSize() > 1)
            {       
                p1 = MonoStack[MonoStack.GetSize() - 1];
                p2 = MonoStack[MonoStack.GetSize() - 2];
                CoordType cp = triangleCrossProduct(topStrip, p1, p2);
                if((cp < 0) != isLeft(p1)) break;
                if(isLeft(p1)) 
                {
                    if(!addTrianglePilot(topStrip, p1, p2)) return false;
                }
                else 
                {
                    if(!addTrianglePilot(topStrip, p2, p1)) return false;
                }
                MonoStack.PopBack();
            }
            MonoStack.PushBack(topStrip); 
        }
    }

    unsigned lastQueuePoint = LeftVertices.Back();
    while(MonoStack.GetSize() > 1)
    {
        p1 = MonoStack.Back();
        MonoStack.PopBack();
        if(isLeft(p1))
        {
            if(!addTrianglePilot(lastQueuePoint, p1, MonoStack.Back())) return false;
        }
        else
        {
            if(!addTrianglePilot(lastQueuePoint, MonoStack.Back(), p1)) return false;
        }
    }
    return true;
}


//-----------------------------------------------------------------------
void GTessellator::triangulateMonotoneRegular(unsigned idx)
{
////TO DO: Remove
//CoherentParts.Clear();
//CoherentVertices.Clear();

    Triangles.Clear();
    LeftVertices.Clear();
    RightVertices.Clear();
    TrianglesCutOff = 0;
    ShapeCoherence  = CoherenceUndefined;
    LeftCoherence   = CoherenceUndefined;
    RightCoherence  = CoherenceUndefined;

    const MonotoneType& monotone = Monotones[idx];
    const MonoVertexType* vertex = monotone.start;

    if(vertex == 0 ||
       vertex->next == 0 ||
       vertex->next->next == 0) return;

    MonoStack.Clear();
    MonoStack.PushBack(vertex->vertex); vertex = vertex->next;
    MonoStack.PushBack(vertex->vertex); vertex = vertex->next;

    unsigned p1, p2;
    while(vertex->next)
    {
        unsigned topStrip = vertex->vertex;
        unsigned topStack = MonoStack.Back();

        if(isLeft(topStrip) != isLeft(topStack))
        {
            while(MonoStack.GetSize() > 1)
            {
                p1 = MonoStack.Back();
                MonoStack.PopBack();
                if(isLeft(topStrip)) addTriangle(topStrip, MonoStack.Back(), p1);
                else                 addTriangle(topStrip, p1, MonoStack.Back());
            }
            MonoStack.PopBack();
            MonoStack.PushBack(topStack);
            MonoStack.PushBack(topStrip);
        }
        else
        {
            while(MonoStack.GetSize() > 1)
            {       
                p1 = MonoStack[MonoStack.GetSize() - 1];
                p2 = MonoStack[MonoStack.GetSize() - 2];
                CoordType cp = triangleCrossProduct(topStrip, p1, p2);
                if((cp < 0) != isLeft(p1)) break;
                if(isLeft(p1)) addTriangle(topStrip, p1, p2);
                else           addTriangle(topStrip, p2, p1);
                MonoStack.PopBack();
            }
            MonoStack.PushBack(topStrip); 
        }
        vertex = vertex->next;
    }

    unsigned lastQueuePoint = vertex->vertex;
    while(MonoStack.GetSize() > 1)
    {
        p1 = MonoStack.Back();
        MonoStack.PopBack();
        if(isLeft(p1)) addTriangle(lastQueuePoint, p1, MonoStack.Back());
        else           addTriangle(lastQueuePoint, MonoStack.Back(), p1);
    }

    if(LeftVertices.GetSize() > 3 && 
       RightVertices.GetSize() > 3 && 
       ShapeCoherence > CoherencePincushion)
    {
        Triangles.CutAt(TrianglesCutOff);
        processCoherentShape();
    }
}


//-----------------------------------------------------------------------
void GTessellator::TriangulateMonotone(unsigned idx)
{
    if(!PilotTriangulation || !triangulateMonotonePilot(idx))
    {
        triangulateMonotoneRegular(idx);
    }
}











#endif

