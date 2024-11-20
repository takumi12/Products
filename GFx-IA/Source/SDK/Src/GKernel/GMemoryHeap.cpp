/**********************************************************************

Filename    :   GMemoryHeap.cpp
Content     :   The main allocator interface. Receive data from heap 
                management system and allocate/free memory from the
                current heap. Manipulates with heaps - creation of
                heap chunks, deletes heap chunks, if free.
Created     :   July 14, 2008
Authors     :   Michael Antonov, Boris Rayskiy, Maxim Shemanarev

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GDebug.h"
#include "GMemoryHeap.h"
#include "GMemory.h" 

#include <string.h>

#undef new

#ifdef SN_TARGET_PS3
#include <stdlib.h>	// for reallocalign()
#endif

// *** Heap-Specific Statistics identifiers
GDECLARE_STAT_GROUP(GStatHeap_Summary,              "Heap Summary",     GStatGroup_Default)
GDECLARE_COUNTER_STAT(GStatHeap_TotalFootprint,     "Total Footprint",  GStatHeap_Summary)
GDECLARE_COUNTER_STAT(GStatHeap_LocalFootprint,     "Local Footprint",  GStatHeap_Summary)
GDECLARE_COUNTER_STAT(GStatHeap_ChildFootprint,     "Child Footprint",  GStatHeap_Summary)
GDECLARE_COUNTER_STAT(GStatHeap_ChildHeaps,         "Child Heaps",      GStatHeap_Summary)
GDECLARE_COUNTER_STAT(GStatHeap_LocalUsedSpace,     "Local Used Space", GStatHeap_Summary)
GDECLARE_COUNTER_STAT(GStatHeap_SysDirectSpace,     "Sys Direct Space", GStatHeap_Summary)
GDECLARE_COUNTER_STAT(GStatHeap_Bookkeeping,        "Bookkeeping",      GStatHeap_Summary)
GDECLARE_COUNTER_STAT(GStatHeap_DebugInfo,          "DebugInfo",        GStatHeap_Summary)
GDECLARE_COUNTER_STAT(GStatHeap_Segments,           "Segments",         GStatHeap_Summary)
GDECLARE_COUNTER_STAT(GStatHeap_Granularity,        "Granularity",      GStatHeap_Summary)
GDECLARE_COUNTER_STAT(GStatHeap_DynamicGranularity, "Dynamic Granularity",GStatHeap_Summary)
GDECLARE_COUNTER_STAT(GStatHeap_Reserve,            "Reserve",          GStatHeap_Summary)


#if defined(GFC_OS_WIN32) && defined(GFC_BUILD_DEBUG)
#define GMEMORYHEAP_CHECK_CURRENT_THREAD(h) GASSERT((h->OwnerThreadId == 0) || (h->OwnerThreadId == ::GetCurrentThreadId()))
#else
#define GMEMORYHEAP_CHECK_CURRENT_THREAD(h)
#endif

GMemoryHeap* GMemory::pGlobalHeap = 0;

//------------------------------------------------------------------------
GMemoryHeap::GMemoryHeap() :
    SelfSize(0),
    RefCount(1),
    OwnerThreadId(0),
    pAutoRelease(0),
    //Info(),
    ChildHeaps(),
    HeapLock(),
    UseLocks(true),
    TrackDebugInfo(true)
{
    memset(&Info, 0, sizeof(Info));
}

//------------------------------------------------------------------------
void GMemoryHeap::GetHeapInfo(HeapInfo* infoPtr) const
{
    memcpy(infoPtr, &Info, sizeof(HeapInfo));
}

//------------------------------------------------------------------------
void GMemoryHeap::ReleaseOnFree(void *ptr)
{
    pAutoRelease = ptr;
}

//------------------------------------------------------------------------
void GMemoryHeap::AssignToCurrentThread()
{
#ifdef GFC_OS_WIN32
    OwnerThreadId = (UPInt) ::GetCurrentThreadId();
#endif
}

//------------------------------------------------------------------------
void GMemoryHeap::VisitChildHeaps(HeapVisitor* visitor)
{
    GLock::Locker locker(&HeapLock);

    GMemoryHeap* heap = ChildHeaps.GetFirst();
    while(!ChildHeaps.IsNull(heap))
    {
        // AddRef() and Release() are locked with a Global Lock here,
        // to avoid the possibility of heap being released during this iteration.
        // If necessary, this could re rewritten with AddRef_NotZero for efficiency.
        visitor->Visit(this, heap);
        heap = heap->pNext;
    }
}

//------------------------------------------------------------------------
bool GMemoryHeap::DumpMemoryLeaks()
{
    // This lock is safe because at this point there should be no more
    // external references to this heap.
    GLock::Locker locker(&HeapLock);
    return dumpMemoryLeaks();
}

//------------------------------------------------------------------------
void GMemoryHeap::UltimateCheck()
{
    GMemory::pGlobalHeap->ultimateCheck();
}

//------------------------------------------------------------------------
void GMemoryHeap::CheckIntegrity()
{
    GMemory::pGlobalHeap->checkIntegrity();
}

//------------------------------------------------------------------------
void GMemoryHeap::ReleaseCachedMem()
{
    GMemory::pGlobalHeap->releaseCachedMem();
}



