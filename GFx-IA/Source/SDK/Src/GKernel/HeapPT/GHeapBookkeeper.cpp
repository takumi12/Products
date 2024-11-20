/**********************************************************************

Filename    :   GHeapBookkeeper.cpp
Content     :   Bookkeeping allocator
Created     :   2009
Authors     :   Maxim Shemanarev

Notes       :   Internal allocator used to store bookkeeping information.

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GHeapBookkeeper.h"


//------------------------------------------------------------------------
GHeapBookkeeper::GHeapBookkeeper(GSysAllocPaged* sysAlloc, 
                                 UPInt granularity) :
    pSysAlloc(sysAlloc), 
    Granularity(granularity),
    Allocator(GHeap_MinShift),
    Footprint(0)
{
    GSysAllocPaged::Info i;
    memset(&i, 0, sizeof(GSysAllocPaged::Info));
    pSysAlloc->GetInfo(&i);
    if (i.Granularity < GHeap_PageSize)
        i.Granularity = GHeap_PageSize;
    GHEAP_ASSERT((i.Granularity & GHeap_PageMask) == 0);
    Granularity = (Granularity + i.Granularity-1) / i.Granularity * i.Granularity;
}

//------------------------------------------------------------------------
UPInt GHeapBookkeeper::getHeaderSize(UPInt dataSize) const
{
    UPInt headerSize;
    headerSize = sizeof(GHeapSegment) + Allocator.GetBitSetBytes(dataSize);
    headerSize = (headerSize + GHeap_MinMask) & ~UPInt(GHeap_MinMask);
    return headerSize;
}

//------------------------------------------------------------------------
GHeapSegment* GHeapBookkeeper::allocSegment(UPInt dataSize)
{
    UByte* segMem = (UByte*)pSysAlloc->Alloc(dataSize, GHeap_PageSize);
    GHeapSegment* seg = (GHeapSegment*)segMem;
    UPInt headerSize;
    if (seg)
    {
        seg->SelfSize   = dataSize;
        seg->SegType    = GHeap_SegmentBookkeeper;
        seg->Alignment  = GHeap_PageShift;
        seg->UseCount   = 0;
        seg->pHeap      = 0;
        seg->DataSize   = 0;
        seg->pData      = 0;
        if (!GHeapGlobalPageTable->MapRange(segMem, dataSize))
        {
            pSysAlloc->Free(segMem, dataSize, GHeap_PageSize);
            return 0;
        }
        GHeapGlobalPageTable->SetSegmentInRange(UPInt(segMem), dataSize, seg);
        headerSize    = getHeaderSize(dataSize);
        seg->DataSize = dataSize - headerSize;
        seg->pData    = segMem + headerSize;
        SegmentList.PushFront(seg);
        Allocator.InitSegment(seg);
        Footprint += seg->SelfSize;
        return seg;
    }
    return 0;
}


//------------------------------------------------------------------------
void GHeapBookkeeper::freeSegment(GHeapSegment* seg)
{
    Allocator.ReleaseSegment(seg);
    SegmentListType::Remove(seg);
    GHeapGlobalPageTable->UnmapRange(seg, seg->SelfSize);
    Footprint -= seg->SelfSize;
    pSysAlloc->Free(seg, seg->SelfSize, GHeap_PageSize);
}


//------------------------------------------------------------------------
void* GHeapBookkeeper::Alloc(UPInt size)
{
    size = Allocator.AlignSize((size < GHeap_MinSize) ? GHeap_MinSize : size);

    GHeapSegment* seg;
    void* ptr = Allocator.Alloc(size, &seg);
    if (ptr)
    {
        seg->UseCount++;
        return ptr;
    }

    UPInt segSize = (size + Granularity - 1) / Granularity * Granularity;
    while (segSize < size + getHeaderSize(segSize))
    {
        // This is an extreme condition, but still, may occur. 
        // Since the the bit-set takes some space in the beginning,
        // the rest of the payload may be not enough for the allocation, 
        // while the allocation must be guaranteed if allocSegment()
        // succeeded.
        //-----------------------
        segSize += Granularity;
    }

    seg = allocSegment(segSize);
    if (seg == 0)
    {
        return 0;
    }

    GHeapSegment* t;
    ptr = Allocator.Alloc(size, &t);
    GHEAP_ASSERT(ptr && t == seg); // Must succeed and allocate in the new segment!
    seg->UseCount++;
    return ptr;
}


//------------------------------------------------------------------------
void GHeapBookkeeper::Free(void* ptr, UPInt size)
{
    GHeapSegment* seg = GHeapGlobalPageTable->GetSegmentSafe(UPInt(ptr));
    GHEAP_ASSERT(seg != 0 && seg->SegType == GHeap_SegmentBookkeeper);

    size = Allocator.AlignSize((size < GHeap_MinSize) ? GHeap_MinSize : size);

    Allocator.Free(seg, ptr, size);
    if (--seg->UseCount == 0)
    {
        freeSegment(seg);
    }
}


//------------------------------------------------------------------------
void GHeapBookkeeper::VisitMem(GHeapMemVisitor* visitor, UInt flags) const
{
    if (flags & GHeapMemVisitor::VF_Bookkeeper)
    {
        const GHeapSegment* seg = SegmentList.GetFirst();
        while(!SegmentList.IsNull(seg))
        {
            visitor->Visit(seg, UPInt(seg->pData), 
                           seg->DataSize, GHeapMemVisitor::Cat_Bookkeeper);
            seg = seg->pNext;
        }
        if (flags & GHeapMemVisitor::VF_BookkeeperFree)
        {
            Allocator.VisitMem(visitor, GHeapMemVisitor::Cat_BookkeeperFree);
        }
    }
}

//------------------------------------------------------------------------
void GHeapBookkeeper::VisitSegments(GHeapSegVisitor* visitor) const
{
    const GHeapSegment* seg = SegmentList.GetFirst();
    while(!SegmentList.IsNull(seg))
    {
        visitor->Visit(GHeapSegVisitor::Seg_Bookkeeper, 0, 
                       UPInt(seg), seg->SelfSize);
        seg = seg->pNext;
    }
    Allocator.VisitUnused(visitor, GHeapSegVisitor::Seg_BookkeeperUnused);
}

