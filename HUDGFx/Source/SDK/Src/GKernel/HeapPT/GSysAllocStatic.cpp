/**********************************************************************

Filename    :   GSysAllocStatic.cpp
Content     :   Static memory System Allocator
Created     :   2009
Authors     :   Maxim Shemanarev

Notes       :   System allocator that works entirely in a single 
                block of memory.

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GHeapTypes.h"
#include "GHeapAllocLite.h"
#include "GSysAlloc.h"


//------------------------------------------------------------------------
GSysAllocStatic::GSysAllocStatic(void* mem1, UPInt size1,
                                 void* mem2, UPInt size2,
                                 void* mem3, UPInt size3,
                                 void* mem4, UPInt size4) :
    MinSize(GHeap_PageSize),
    NumSegments(0), 
    pAllocator(0),
    TotalSpace(0)
{
    pAllocator = ::new(PrivateData) GHeapAllocLite(MinSize);
    if (mem1) AddMemSegment(mem1, size1);
    if (mem2) AddMemSegment(mem2, size2);
    if (mem3) AddMemSegment(mem3, size3);
    if (mem4) AddMemSegment(mem4, size4);
    GCOMPILER_ASSERT(sizeof(PrivateData) >= sizeof(GHeapAllocLite));
    GCOMPILER_ASSERT(sizeof(Segments[0]) >= sizeof(GHeapTreeSeg));
}

//------------------------------------------------------------------------
GSysAllocStatic::GSysAllocStatic(UPInt minSize) :
    MinSize(minSize),
    NumSegments(0), 
    pAllocator(0),
    TotalSpace(0)
{
    pAllocator = ::new(PrivateData) GHeapAllocLite(MinSize);
    GCOMPILER_ASSERT(sizeof(PrivateData) >= sizeof(GHeapAllocLite));
    GCOMPILER_ASSERT(sizeof(Segments[0]) >= sizeof(GHeapTreeSeg));
}


//------------------------------------------------------------------------
void GSysAllocStatic::AddMemSegment(void* mem, UPInt size)
{
    UByte* ptr = (UByte*)((UPInt(mem) + MinSize-1) & ~(MinSize-1));
    UByte* end = (UByte*)((UPInt(mem) + size)      & ~(MinSize-1));
    if (NumSegments < MaxSegments)
    {
        GHeapTreeSeg* seg = (GHeapTreeSeg*)Segments[NumSegments];
        seg->Buffer   = ptr;
        seg->Size     = end - ptr;
        seg->UseCount = 0;
        TotalSpace   += seg->Size;
        pAllocator->InitSegment(seg);
        ++NumSegments;
    }
}

//------------------------------------------------------------------------
void GSysAllocStatic::GetInfo(Info* i) const
{
    i->MinAlign     = pAllocator->GetMinSize();
    i->MaxAlign     = 0; // Any alignment
    i->Granularity  = 0; // No granularity
    i->HasRealloc   = true;
}


//------------------------------------------------------------------------
void* GSysAllocStatic::Alloc(UPInt size, UPInt alignment)
{
    GHeapTreeSeg* seg;
    return pAllocator->Alloc(size, alignment, &seg);
}


//------------------------------------------------------------------------
bool GSysAllocStatic::ReallocInPlace(void* oldPtr, UPInt oldSize, 
                                     UPInt newSize, UPInt alignment)
{
    UPInt i;
    for (i = 0; i < NumSegments; ++i)
    {
        GHeapTreeSeg* seg = (GHeapTreeSeg*)Segments[i];
        if ((UByte*)oldPtr >= seg->Buffer && 
            (UByte*)oldPtr <  seg->Buffer + seg->Size)
        {
            GHeapAllocLite::ReallocResult ret = 
                pAllocator->ReallocInPlace(seg, oldPtr, oldSize, 
                                           newSize, alignment);
            return ret < GHeapAllocLite::ReallocFailed;
        }
    }
    return false;
}


//------------------------------------------------------------------------
bool GSysAllocStatic::Free(void* ptr, UPInt size, UPInt alignment)
{
    UPInt i;
    for (i = 0; i < NumSegments; ++i)
    {
        GHeapTreeSeg* seg = (GHeapTreeSeg*)Segments[i];
        if ((UByte*)ptr >= seg->Buffer && 
            (UByte*)ptr <  seg->Buffer + seg->Size)
        {
            pAllocator->Free(seg, ptr, size, alignment);
            return true;
        }
    }
    return false;
}


//------------------------------------------------------------------------
void GSysAllocStatic::VisitMem(GHeapMemVisitor* visitor) const
{ 
    pAllocator->VisitMem(visitor, GHeapMemVisitor::Cat_SysAllocFree);
}



//------------------------------------------------------------------------
UPInt GSysAllocStatic::GetBase() const 
{
    UPInt i;
    UPInt m = ~UPInt(0);
    for (i = 0; i < NumSegments; ++i)
    {
        GHeapTreeSeg* seg = (GHeapTreeSeg*)Segments[i];
        if (UPInt(seg->Buffer) < m)
        {
            m = UPInt(seg->Buffer);
        }
    }
    return m;
} 

//------------------------------------------------------------------------
UPInt GSysAllocStatic::GetSize() const 
{ 
    UPInt i;
    UPInt s = 0;
    for (i = 0; i < NumSegments; ++i)
    {
        s += ((GHeapTreeSeg*)Segments[i])->Size;
    }
    return s;
}

//------------------------------------------------------------------------
UPInt GSysAllocStatic::GetFootprint() const
{
    return TotalSpace;
}

//------------------------------------------------------------------------
UPInt GSysAllocStatic::GetUsedSpace() const
{
    return TotalSpace - pAllocator->GetFreeBytes();
}
