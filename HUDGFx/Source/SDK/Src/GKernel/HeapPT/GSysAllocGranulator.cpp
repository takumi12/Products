/**********************************************************************

Filename    :   GSysAllocGranulator.cpp
Content     :   An intermediate granulator for System Allocator
Created     :   2009
Authors     :   Maxim Shemanarev

Notes       :   The granulator requests the system allocator for 
                large memory blocks and granulates them as necessary,
                providing smaller granularity to the allocation engine.

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GHeapGranulator.h"
#include "GSysAllocGranulator.h"

//#include <stdio.h>   // DBG
//#include "GMemory.h" // DBG
//#undef new // DBG


//------------------------------------------------------------------------
GSysAllocGranulator::GSysAllocGranulator() : 
    pGranulator(0),
    SysDirectThreshold(0),
    MinAlign(0),
    MaxAlign(0),
    SysDirectFootprint(0)
{}

GSysAllocGranulator::GSysAllocGranulator(GSysAllocPaged* sysAlloc) : 
    pGranulator(0),
    SysDirectThreshold(0),
    MaxHeapGranularity(0),
    MinAlign(0),
    MaxAlign(0),
    SysDirectFootprint(0)
{
    GCOMPILER_ASSERT(sizeof(PrivateData) >= sizeof(GHeapGranulator));
    Init(sysAlloc);
}

//------------------------------------------------------------------------
void GSysAllocGranulator::Init(GSysAllocPaged* sysAlloc)
{
    pGranulator = ::new(PrivateData) 
                GHeapGranulator(sysAlloc, 
                                GHeap_PageSize,
                                GHeap_PageSize, 
                                GHeap_GranulatorHeaderPageSize);
    Info i;
    memset(&i, 0, sizeof(Info));
    pGranulator->GetSysAlloc()->GetInfo(&i); // For the sake of SysDirectThreshold
    SysDirectThreshold = i.SysDirectThreshold;
    MaxHeapGranularity = i.MaxHeapGranularity;
    MinAlign = i.MinAlign;
    MaxAlign = i.MaxAlign;

    // Allow MinAlign and MaxAlign to have zero values, which means
    // "any alignment is supported".
    if (MinAlign == 0)
        MinAlign = 1;

    if (MaxAlign == 0)
        MaxAlign = UPInt(1) << (8*sizeof(UPInt)-1);
}

//------------------------------------------------------------------------
void GSysAllocGranulator::GetInfo(Info* i) const
{
    i->MinAlign             = 0; // Any alignment
    i->MaxAlign             = 0; // Any alignment
    i->Granularity          = 0; // No Granularity
    i->SysDirectThreshold   = SysDirectThreshold;
    i->MaxHeapGranularity   = MaxHeapGranularity;
    i->HasRealloc           = true;
}


//------------------------------------------------------------------------
void* GSysAllocGranulator::Alloc(UPInt size, UPInt alignment)
{
    void* ptr = pGranulator->Alloc(size, alignment);

    //if (GMemory::pGlobalHeap)             // DBG
    //    printf("+%uK fp:%uK used:%f%%\n",
    //        size/1024, pGranulator->GetUsedSpace()/1024, 
    //        GMemory::pGlobalHeap->GetTotalUsedSpace()/double(pGranulator->GetUsedSpace())*100.0);

    return ptr;
}


//------------------------------------------------------------------------
bool GSysAllocGranulator::ReallocInPlace(void* oldPtr, UPInt oldSize, 
                                        UPInt newSize, UPInt alignment)
{
    return pGranulator->ReallocInPlace((UByte*)oldPtr, oldSize, 
                                       newSize, alignment);
}

//------------------------------------------------------------------------
bool GSysAllocGranulator::Free(void* ptr, UPInt size, UPInt alignment)
{
    bool ret = pGranulator->Free(ptr, size, alignment);

    //// With this DBG GFx crashes at exit. The crash is expected and normal.
    //if (GMemory::pGlobalHeap)             // DBG
    //    printf("-%uK fp:%uK used:%f%%\n",
    //        size/1024, pGranulator->GetUsedSpace()/1024, 
    //        GMemory::pGlobalHeap->GetTotalUsedSpace()/double(pGranulator->GetUsedSpace())*100.0);

    return ret;
}


//------------------------------------------------------------------------
void* GSysAllocGranulator::AllocSysDirect(UPInt size, UPInt alignment, 
                                          UPInt* actualSize, UPInt* actualAlign)
{
    UPInt pageAlignment = alignment;
    if (pageAlignment < GHeap_PageSize)
        pageAlignment = GHeap_PageSize;

    if (alignment < MinAlign)
        alignment = MinAlign;

    if (alignment > MaxAlign)
        alignment = MaxAlign;

    if (alignment < pageAlignment)
        size += pageAlignment;

    *actualSize  = size;
    *actualAlign = alignment;
    SysDirectFootprint += size;
    void* ptr = pGranulator->GetSysAlloc()->Alloc(size, alignment);
    return ptr;
}

//------------------------------------------------------------------------
bool GSysAllocGranulator::FreeSysDirect(void* ptr, UPInt size, UPInt alignment)
{
    SysDirectFootprint -= size;
    return pGranulator->GetSysAlloc()->Free(ptr, size, alignment);
}


//------------------------------------------------------------------------
void GSysAllocGranulator::VisitMem(GHeapMemVisitor* visitor) const
{ 
    pGranulator->VisitMem(visitor, 
                          GHeapMemVisitor::Cat_SysAlloc, 
                          GHeapMemVisitor::Cat_SysAllocFree);
}

//------------------------------------------------------------------------
void GSysAllocGranulator::VisitSegments(GHeapSegVisitor* visitor, 
                                        UInt catSeg, 
                                        UInt catUnused) const
{
    pGranulator->VisitSegments(visitor, catSeg, catUnused);
}

//------------------------------------------------------------------------
UPInt GSysAllocGranulator::GetFootprint() const { return pGranulator->GetFootprint() + SysDirectFootprint; }
UPInt GSysAllocGranulator::GetUsedSpace() const { return pGranulator->GetUsedSpace() + SysDirectFootprint; }

//------------------------------------------------------------------------
UPInt GSysAllocGranulator::GetBase() const { return pGranulator->GetBase(); } // DBG
UPInt GSysAllocGranulator::GetSize() const { return pGranulator->GetSize(); } // DBG
