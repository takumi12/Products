/**********************************************************************

Filename    :   GHeapStarter.cpp
Content     :   Starter allocator
Created     :   2009
Authors     :   Maxim Shemanarev

Notes       :   Internal allocator used for the initial bootstrapping.
                It allocates/frees the page mapping arrays.

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GHeapStarter.h"
#include "GMemoryHeap.h"


//------------------------------------------------------------------------
GHeapStarter::GHeapStarter(GSysAllocPaged* sysAlloc, 
                           UPInt granularity,
                           UPInt headerPageSize) : 
    Granulator(sysAlloc, 256, granularity, headerPageSize)
{}

//------------------------------------------------------------------------
void* GHeapStarter::Alloc(UPInt size, UPInt alignSize)
{
    return Granulator.Alloc(size, alignSize);
}

//------------------------------------------------------------------------
void GHeapStarter::Free(void* ptr, UPInt size, UPInt alignSize)
{
    Granulator.Free(ptr, size, alignSize);
}

//------------------------------------------------------------------------
void GHeapStarter::VisitMem(GHeapMemVisitor* visitor) const
{
    Granulator.VisitMem(visitor, 
                        GHeapMemVisitor::Cat_Starter, 
                        GHeapMemVisitor::Cat_StarterFree);
}

void GHeapStarter::VisitSegments(class GHeapSegVisitor* visitor) const
{
    Granulator.VisitSegments(visitor, GHeapSegVisitor::Seg_PageMap,
                                      GHeapSegVisitor::Seg_PageMapUnused);
}
