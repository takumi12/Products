/**********************************************************************

Filename    :   GHeapBookkeeper.h
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

#ifndef INC_GHeapBookkeeper_H
#define INC_GHeapBookkeeper_H

#include "GHeapPageTable.h"
#include "GSysAlloc.h"
#include "GHeapAllocBitSet1.h"

// ***** GHeapBookkeeper
//
//  Internal allocator used to store bookkeeping information.
//------------------------------------------------------------------------
class GHeapBookkeeper
{
public:
    typedef GList<GHeapSegment> SegmentListType;

    GHeapBookkeeper(GSysAllocPaged* sysAlloc, 
                    UPInt granularity = GHeap_PageSize);

    GSysAllocPaged* GetSysAlloc() { return pSysAlloc; }

    void*  Alloc(UPInt size);
    void   Free(void* ptr, UPInt size);

    void   VisitMem(GHeapMemVisitor* visitor, UInt flags) const;
    void   VisitSegments(GHeapSegVisitor* visitor) const;

    UPInt  GetFootprint() const { return Footprint; }
    UPInt  GetUsedSpace() const { return Footprint - Allocator.GetTotalFreeSpace(); }

private:
    UPInt           getHeaderSize(UPInt dataSize) const;
    GHeapSegment*   allocSegment(UPInt size);
    void            freeSegment(GHeapSegment* seg);

    GSysAllocPaged*          pSysAlloc;
    UPInt               Granularity;
    SegmentListType     SegmentList;
    GHeapAllocBitSet1   Allocator;
    UPInt               Footprint;
};


#endif
