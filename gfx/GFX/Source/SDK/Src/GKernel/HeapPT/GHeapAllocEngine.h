/**********************************************************************

Filename    :   GHeapAllocEngine.h
Content     :   The main allocation engine
            :   
Created     :   2009
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GHeapAllocEngine_H
#define INC_GHeapAllocEngine_H

#include "GAtomic.h"
#include "GHeapPageTable.h"
#include "GHeapBookkeeper.h"
#include "GHeapAllocBitSet2.h"


//------------------------------------------------------------------------
struct GHeapOtherStats
{
    UPInt Segments;
    UPInt Bookkeeping;
    UPInt DynamicGranularity;
    UPInt SysDirectSpace;
};

//------------------------------------------------------------------------
class GHeapAllocEngine
{
public:
    //-------------------------------------------------------------------
    struct TinyBlock : GListNode<TinyBlock>
    {
        GHeapSegment*   pSegment;
    };

private:
    typedef GList<GHeapSegment> SegmentListType;
    typedef GList<TinyBlock>    TinyListType;

public:
    //--------------------------------------------------------------------
    GHeapAllocEngine(GSysAllocPaged*     sysAlloc,
                     GMemoryHeapPT* heap,
                     UInt           allocFlags, 
                     UPInt          minAlignSize=16,
                     UPInt          granularity=8*1024, 
                     UPInt          reserve=8*1024,
                     UPInt          internalThreshold=128*1024,
                     UPInt          limit=0);

    ~GHeapAllocEngine();

    bool IsValid() const { return Valid; }

    UPInt SetLimit(UPInt lim);
    void  SetLimitHandler(void* handler) { pLimHandler = handler; }

    // Calling FreeAll() releases all memory the heap holds, 
    // but the heap remains functional.
    void FreeAll();

    // External functions
    //--------------------------------------------------------------------
    void*   Alloc(UPInt size);
    void*   Alloc(UPInt size, UPInt alignSize);
    void*   Realloc(GHeapSegment* seg, void* oldPtr, UPInt newSize);
    void    Free(GHeapSegment* seg, void* ptr);

    void*   Realloc(void* oldPtr, UPInt newSize);
    void    Free(void* ptr);

    UPInt   GetUsableSize(const void* ptr);
    UPInt   GetUsableSize(GHeapSegment* seg, const void* ptr);

    UPInt   GetFootprint() const;
    UPInt   GetUsedSpace() const;

    void    ReleaseCachedMem();

    // See GHeapMemVisitor::VisitingFlags for "flags" argument.
    void    VisitMem(GHeapMemVisitor* visitor, 
                     UInt flags = GHeapMemVisitor::VF_Heap) const;

    void    VisitSegments(GHeapSegVisitor* visitor) const;

    void    GetHeapOtherStats(GHeapOtherStats* otherStats) const;

    void    CheckIntegrity() const
    {
        Allocator.CheckIntegrity();
    }

private:
    //--------------------------------------------------------------------
    static void compilerAsserts(); // A fake function used for GCOMPILER_ASSERTS only

    GHeapSegment*   allocSegment(UInt  segType, UPInt size, UPInt sysAlignment, 
                                 UPInt bookkeepingSize, bool* limHandlerOK);

    GHeapSegment*   allocSegmentNoGranulator(UPInt dataSize, UPInt alignment, bool* limHandlerOK);

    GHeapSegment*   allocSegmentLocked(UInt  segType, UPInt size, UPInt sysAlignment, 
                                       UPInt bookkeepingSize, bool* limHandlerOK);

    void            freeSegment(GHeapSegment* seg);
    void            freeSegmentLocked(GHeapSegment* seg);

    UPInt           calcDynaSize() const;
    GHeapSegment*   allocSegmentBitSet(UPInt size, UPInt alignSize, 
                                       UPInt granularity, 
                                       bool* limHandlerOK);

    GHeapSegment*   allocSegmentBitSet(UPInt size, UPInt alignSize,
                                       bool* limHandlerOK);

    void            freeSegmentBitSet(GHeapSegment* seg);

    void* allocSysDirect(UPInt dataSize, UPInt alignSize);
    void* reallocSysDirect(GHeapSegment* seg, void* oldPtr, UPInt newSize);
    void* reallocGeneral(GHeapSegment* seg, void* oldPtr, 
                         UPInt oldSize, UPInt newSize, 
                         UPInt alignShift);

    void* allocBitSet(UPInt size);
    void* allocBitSet(UPInt size, UPInt alignSize);

    void* allocSegmentTiny(UInt idx);
    void  releaseSegmentTiny(GHeapSegment* seg);
    void  freeSegmentTiny(GHeapSegment* seg);
    void* allocTiny(UInt sizeIdx);
    void  freeTiny(GHeapSegment* seg, TinyBlock* ptr);

private:
    //--------------------------------------------------------------------
    GMemoryHeapPT*      pHeap;
    GSysAllocPaged*          pSysAlloc;
    GHeapBookkeeper*    pBookkeeper;
    UPInt               MinAlignShift;
    UPInt               MinAlignMask;
    GHeapAllocBitSet2   Allocator;
    SegmentListType     SegmentList;
    TinyListType        TinyBlocks[GHeap_TinyBinSize];
    bool                AllowTinyBlocks;
    bool                AllowDynaSize;
    bool                Valid;
    bool                HasRealloc;
    UPInt               SysGranularity;
    UPInt               Granularity;
    UPInt               Reserve;
    UPInt               Threshold;
    UPInt               SysDirectThreshold;
    UPInt               Footprint;
    UPInt               TinyFreeSpace;
    UPInt               SysDirectSpace;
    GHeapSegment*       pCachedBSeg; // Cached bit-set segment
    GHeapSegment*       pCachedTSeg; // Cached tiny blocks segment
    UPInt               Limit;
    void*               pLimHandler;
};


#endif

