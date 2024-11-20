/**********************************************************************

Filename    :   GSysAllocGranulator.h
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

#ifndef INC_GSysAllocGranulator_H
#define INC_GSysAllocGranulator_H

#include "GSysAlloc.h"


// ***** GSysAllocGranulator
//
// An intermediate system allocator that allows to have as large system
// granularity as wanted. The granulator requests the system allocator for 
// large memory blocks and granulates them as necessary, providing smaller 
// granularity to the allocation engine.
//
// If the system provides an advanced allocation mechanism, the granulator 
// typically isn't necessary. However, it may speed up some cases, when 
// the system calls are expensive (like VirtualAlloc on Win platforms).
// 
// The granulator is really useful when the allocation system calls are 
// "alignment unfriendly". A typical case of it is regular malloc/free
// (or dlmalloc as well).
//
// The allocator may request the system for small blocks of memory, but not
// less than GHeap_PageSize, which is 4KB on 32-bit systems. The allocation
// data must be aligned to the "alignment" parameter, which is in most
// cases equals to GHeap_PageSize. The alignment requirement can be less 
// in some rare cases, when the allocator requests for small blocks of 
// for the initial bootstrapping.
// But for the user payload the alignment is at least GHeap_PageSize. It 
// can be bigger than that only in cases of direct system allocations with
// bigger than 4K alignment requirements.
//
// If you create a heap with a very small granularity, like 4K, it's very
// likely that there will be many requests for those 4KB, aligned to 4KB 
// too. With regular malloc/free it may result in up to 2x memory overhead.
// GSysAllocGranulator can smooth this "injustice of nature." It 
// requests the system for large blocks, (64K by default), aligned to 4K, 
// and handles all the smaller requests from the allocator. Obviously, in 
// case of 64K granularity and GHeap_PageSize = 4K, the memory overhead 
// for the alignment will be in the worst case 1/16, or 6.25%. In average 
// it will be about two times less.
//------------------------------------------------------------------------
class GHeapGranulator;
class GHeapMemVisitor;
class GSysAllocGranulator : public GSysAllocPaged
{
public:
    GSysAllocGranulator();
    GSysAllocGranulator(GSysAllocPaged* sysAlloc);

    void Init(GSysAllocPaged* sysAlloc);

    virtual void    GetInfo(Info* i) const;
    virtual void*   Alloc(UPInt size, UPInt alignment);
    virtual bool    ReallocInPlace(void* oldPtr, UPInt oldSize, UPInt newSize, UPInt alignment);
    virtual bool    Free(void* ptr, UPInt size, UPInt alignment);

    virtual void*   AllocSysDirect(UPInt size, UPInt alignment, UPInt* actualSize, UPInt* actualAlign);
    virtual bool    FreeSysDirect(void* ptr, UPInt size, UPInt alignment);

    virtual UPInt   GetFootprint() const;
    virtual UPInt   GetUsedSpace() const;

    virtual void    VisitMem(GHeapMemVisitor* visitor) const;
    virtual void    VisitSegments(GHeapSegVisitor* visitor, UInt catSeg, UInt catUnused) const;

    virtual UPInt   GetBase() const; // DBG
    virtual UPInt   GetSize() const; // DBG

private:
    GHeapGranulator* pGranulator;
    UPInt            SysDirectThreshold;
    UPInt            MaxHeapGranularity;
    UPInt            MinAlign;
    UPInt            MaxAlign;
    UPInt            SysDirectFootprint;
    UPInt            PrivateData[32];
};

#endif
