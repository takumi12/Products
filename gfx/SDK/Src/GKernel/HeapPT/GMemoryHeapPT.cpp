/**********************************************************************

Filename    :   GMemoryHeapPT.cpp
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

#include "GHeapRoot.h"
#include "GHeapAllocEngine.h"
#include "GHeapDebugInfo.h"
#include "GDebug.h"
#include "GMemoryHeapPT.h"
#include "GMemory.h"

#undef new

#ifdef SN_TARGET_PS3
#include <stdlib.h>	// for reallocalign()
#endif

#if defined(GFC_OS_WIN32) && defined(GFC_BUILD_DEBUG)
#define GMEMORYHEAP_CHECK_CURRENT_THREAD(h) GASSERT((h->OwnerThreadId == 0) || (h->OwnerThreadId == ::GetCurrentThreadId()))
#else
#define GMEMORYHEAP_CHECK_CURRENT_THREAD(h)
#endif

//------------------------------------------------------------------------
static UPInt GHeapRootMem[(sizeof(GHeapRoot) + sizeof(UPInt) - 1) / sizeof(UPInt)];
GHeapRoot*   GHeapGlobalRoot = 0;

//------------------------------------------------------------------------
GSysAllocWrapper::GSysAllocWrapper(GSysAllocPaged* sysAlloc) :
    Granulator(),
    pSrcAlloc(sysAlloc),
    pSysAlloc(sysAlloc),
    SysGranularity(GHeap_PageSize),
    MinAlign(1),
    UsedSpace(0)
{
    Info i;
    memset(&i, 0, sizeof(Info));
    sysAlloc->GetInfo(&i);
    if (i.Granularity)
    {
        Granulator.Init(sysAlloc);
        pSysAlloc = &Granulator;
        SysGranularity = i.Granularity;
    }
    if (MinAlign < i.MinAlign)
        MinAlign = i.MinAlign;
}

//------------------------------------------------------------------------
void GSysAllocWrapper::GetInfo(Info* i) const
{
    memset(i, 0, sizeof(Info));
    pSysAlloc->GetInfo(i);
}

void* GSysAllocWrapper::Alloc(UPInt size, UPInt align)
{
    UsedSpace += size;
    return pSysAlloc->Alloc(size, align);
}

bool GSysAllocWrapper::Free(void* ptr, UPInt size, UPInt align)
{
    UsedSpace -= size;
    return pSysAlloc->Free(ptr, size, align);
}

bool GSysAllocWrapper::ReallocInPlace(void* oldPtr, UPInt oldSize, UPInt newSize, UPInt align)
{
    UsedSpace -= oldSize;
    UsedSpace += newSize;
    return pSysAlloc->ReallocInPlace(oldPtr, oldSize, newSize, align);
}

void* GSysAllocWrapper::AllocSysDirect(UPInt size, UPInt alignment, UPInt* actualSize, UPInt* actualAlign)
{
    UsedSpace += size;
    return pSysAlloc->AllocSysDirect(size, alignment, actualSize, actualAlign);
}

bool  GSysAllocWrapper::FreeSysDirect(void* ptr, UPInt size, UPInt alignment)
{
    UsedSpace -= size;
    return pSysAlloc->FreeSysDirect(ptr, size, alignment);
}

void GSysAllocWrapper::VisitMem(class GHeapMemVisitor* visitor) const
{
    pSysAlloc->VisitMem(visitor);
}

void GSysAllocWrapper::VisitSegments(class GHeapSegVisitor* visitor, 
                                     UInt catSeg, UInt catUnused) const
{
    pSysAlloc->VisitSegments(visitor, catSeg, catUnused);
}

//------------------------------------------------------------------------
void* GSysAllocWrapper::AllocSysDirect(UPInt size)
{
    size = (size + SysGranularity - 1) / SysGranularity * SysGranularity;
    UsedSpace += size;
    return pSrcAlloc->Alloc(size, MinAlign);
}

//------------------------------------------------------------------------
void GSysAllocWrapper::FreeSysDirect(void* ptr, UPInt size)
{
    size = (size + SysGranularity - 1) / SysGranularity * SysGranularity;
    UsedSpace -= size;
    pSrcAlloc->Free(ptr, size, MinAlign);
}

//------------------------------------------------------------------------
UPInt GSysAllocWrapper::GetFootprint() const { return pSysAlloc->GetFootprint(); }
UPInt GSysAllocWrapper::GetUsedSpace() const { return UsedSpace; }






//------------------------------------------------------------------------
#ifdef GHEAP_TRACE_ALL
GMemoryHeap::HeapTracer* GHeapRoot::pTracer = 0;
#endif


//------------------------------------------------------------------------
GHeapRoot::GHeapRoot(GSysAllocPaged* sysAlloc) :
    SysAllocWrapper(sysAlloc),
    Starter(&SysAllocWrapper, GHeap_StarterGranularity, GHeap_StarterHeaderPageSize),
    Bookkeeper(&SysAllocWrapper, GHeap_BookkeeperGranularity),
#ifdef GHEAP_DEBUG_INFO
    DebugAlloc(&SysAllocWrapper, 
               256,
               GHeap_DebugAllocGranularity, 
               GHeap_DebugAllocHeaderPageSize),
#endif
    RootLock(),
    pArenas(0),
    NumArenas(0)
{
    GHeapGlobalPageTable->SetStarter(&Starter);
    GHeapGlobalRoot = this;
}

GHeapRoot::~GHeapRoot()
{
}

// *** Memory Arenas
//
//------------------------------------------------------------------------
void GHeapRoot::CreateArena(UPInt arena, GSysAllocPaged* sysAlloc)
{
    GHEAP_ASSERT(arena);
    GLockSafe::Locker lock(&RootLock);
    if (NumArenas < arena)
    {
        UPInt newNumArenas = (arena + 15) & ~15;
        GSysAllocPaged** newArenas = (GSysAllocPaged**)Bookkeeper.Alloc(newNumArenas * sizeof(void*));
        GHEAP_ASSERT(newArenas);
        memset(newArenas, 0, newNumArenas * sizeof(void*));
        if (pArenas) 
        {
            memcpy(newArenas, pArenas, NumArenas * sizeof(void*));
            Bookkeeper.Free(pArenas, NumArenas * sizeof(void*));
        }
        pArenas = newArenas;
        NumArenas = newNumArenas;
    }
    GHEAP_ASSERT(arena <= NumArenas);
    GHEAP_ASSERT(pArenas[arena-1] == 0);
    pArenas[arena-1] = (GSysAllocPaged*)Bookkeeper.Alloc(sizeof(GSysAllocWrapper));
    GHEAP_ASSERT(pArenas[arena-1]);
    pArenas[arena-1] = ::new(pArenas[arena-1]) GSysAllocWrapper(sysAlloc);
}

//------------------------------------------------------------------------
int GHeapRoot::DestroyArena(UPInt arena)
{
    GHEAP_ASSERT(arena);
    GHEAP_ASSERT(pArenas[arena-1]);
    GLockSafe::Locker lock(&RootLock);
    bool empty = ArenaIsEmpty(arena);
    if (empty)
    {
        pArenas[arena-1]->~GSysAllocPaged();
        Bookkeeper.Free(pArenas[arena-1], sizeof(GSysAllocWrapper));
        pArenas[arena-1] = 0;
    }
    else
    {
        GFC_DEBUG_MESSAGE1(!empty, "GMemoryHeap Arena %d is not empty!", arena);
#ifdef GFC_BUILD_DEBUG
        GHEAP_ASSERT(empty);
        return 0;
#else
        return *(int*)0; // "Safe crash" in release mode
#endif
    }
    return 0;
}

//------------------------------------------------------------------------
void GHeapRoot::DestroyAllArenas()
{
    GLockSafe::Locker lock(&RootLock);
    if (pArenas)
    {
        for (UPInt i = NumArenas; i; --i)
        {
            if (pArenas[i-1])
                DestroyArena(i);
        }
        Bookkeeper.Free(pArenas, NumArenas * sizeof(void*));
        pArenas = 0;
        NumArenas = 0;
    }
}

//------------------------------------------------------------------------
bool GHeapRoot::ArenaIsEmpty(UPInt arena)
{
    GLockSafe::Locker lock(&RootLock);
    return GetSysAlloc(arena)->GetUsedSpace() == 0;
}

//------------------------------------------------------------------------
GSysAllocPaged* GHeapRoot::GetSysAlloc(UPInt arena) 
{
    GLockSafe::Locker lock(&RootLock);
    if(arena)
    {
        GHEAP_ASSERT(arena <= NumArenas);
        GHEAP_ASSERT(pArenas[arena-1]);
        return pArenas[arena-1];
    }
    return &SysAllocWrapper; 
}


//------------------------------------------------------------------------
GMemoryHeapPT* GHeapRoot::CreateHeap(const char*  name, 
                                     GMemoryHeapPT* parent,
                                     const GMemoryHeap::HeapDesc& desc)
{
    // Locked by Root::Lock in GMemoryHeap::CreateHeap
    if (GetSysAlloc(0) == 0)
    {
        GFC_DEBUG_MESSAGE(1, "CreateHeap: Memory System is not initialized");
        GHEAP_ASSERT(0);
    }

#ifdef GHEAP_DEBUG_INFO
    UPInt debugStorageSize = sizeof(GHeapDebugStorage);
#else
    UPInt debugStorageSize = 0;
#endif

    UPInt selfSize = sizeof(GMemoryHeapPT) + 
                     sizeof(GHeapAllocEngine) + 
                     debugStorageSize +
                     strlen(name) + 1;

    selfSize = (selfSize + GHeap_MinMask) & ~UPInt(GHeap_MinMask);

    UByte* heapBuf = (UByte*)Bookkeeper.Alloc(selfSize);
    if (heapBuf == 0)
    {
        return 0;
    }

    UInt engineFlags = 0;

    if (desc.Flags & GMemoryHeap::Heap_FastTinyBlocks) 
        engineFlags |= GHeap_AllowTinyBlocks;

    if ((desc.Flags & GMemoryHeap::Heap_FixedGranularity) == 0) 
        engineFlags |= GHeap_AllowDynaSize;

    GMemoryHeapPT* heap = ::new(heapBuf) GMemoryHeapPT;

    GHeapAllocEngine* engine = ::new (heapBuf + sizeof(GMemoryHeapPT))
        GHeapAllocEngine(GetSysAlloc(desc.Arena),
                         heap,
                         engineFlags, 
                         desc.MinAlign, 
                         desc.Granularity, 
#ifdef GHEAP_FORCE_MALLOC
                         0,
#else
                         desc.Reserve, 
#endif
                         desc.Threshold,
                         desc.Limit);

    if (!engine->IsValid())
    {
        Bookkeeper.Free(heapBuf, selfSize);
        return 0;
    }

    heap->SelfSize          = selfSize;
    heap->RefCount          = 1;
    heap->pAutoRelease      = 0;
    heap->Info.Desc         = desc;
    heap->Info.pParent      = parent;
    heap->Info.pName        = (char*)heapBuf + 
                                     sizeof(GMemoryHeapPT) + 
                                     sizeof(GHeapAllocEngine) + 
                                     debugStorageSize;
    heap->UseLocks          = (desc.Flags & GMemoryHeap::Heap_ThreadUnsafe) == 0;
    heap->TrackDebugInfo    = (desc.Flags & GMemoryHeap::Heap_NoDebugInfo)  == 0;
    heap->pEngine           = engine;

#ifdef GHEAP_DEBUG_INFO
    heap->pDebugStorage = ::new(heapBuf + sizeof(GMemoryHeapPT) + sizeof(GHeapAllocEngine))
        GHeapDebugStorage(GHeapGlobalRoot->GetDebugAlloc(), 
                          GHeapGlobalRoot->GetLock()),
#endif
    memcpy(heap->Info.pName, name, strlen(name) + 1);

#ifdef GHEAP_TRACE_ALL
    pTracer->OnCreateHeap(heap);
#endif

    return heap;
}


//------------------------------------------------------------------------
void GHeapRoot::DestroyHeap(GMemoryHeapPT* heap)
{
    // Locked by Root::Lock in GMemoryHeap::Release
#ifdef GHEAP_TRACE_ALL
    pTracer->OnDestroyHeap(heap);
#endif

#ifdef GHEAP_DEBUG_INFO
    heap->pDebugStorage->FreeAll();
#endif
    heap->pEngine->FreeAll();
    heap->~GMemoryHeapPT();
    Bookkeeper.Free(heap, heap->SelfSize);
}


//------------------------------------------------------------------------
#ifdef GHEAP_TRACE_ALL
void GHeapRoot::OnAlloc(const GMemoryHeapPT* heap, UPInt size, UPInt align, UInt sid, const void* ptr)
{
    GLock::Locker lock(&RootLock);
    pTracer->OnAlloc(heap, size, align, sid, ptr);
}

void GHeapRoot::OnRealloc(const GMemoryHeapPT* heap, const void* oldPtr, UPInt newSize, const void* newPtr)
{
    GLock::Locker lock(&RootLock);
    pTracer->OnRealloc(heap, oldPtr, newSize, newPtr);
}

void GHeapRoot::OnFree(const GMemoryHeapPT* heap, const void* ptr)
{
    GLock::Locker lock(&RootLock);
    pTracer->OnFree(heap, ptr);
}
#endif


//------------------------------------------------------------------------
void GHeapRoot::VisitSegments(GHeapSegVisitor* visitor) const
{
    GLockSafe::Locker lock(&RootLock);
    SysAllocWrapper.VisitSegments(visitor, 
                                  GHeapSegVisitor::Seg_SysMem, 
                                  GHeapSegVisitor::Seg_SysMemUnused);
    Starter.VisitSegments(visitor);
    Bookkeeper.VisitSegments(visitor);
#ifdef GHEAP_DEBUG_INFO
    DebugAlloc.VisitSegments(visitor, 
                             GHeapSegVisitor::Seg_DebugInfo,
                             GHeapSegVisitor::Seg_DebugInfoUnused);
#endif
}


//------------------------------------------------------------------------
void GHeapRoot::GetStats(GMemoryHeap::RootStats* stats) const
{
    GLockSafe::Locker lock(&RootLock);
    stats->SysMemFootprint      = SysAllocWrapper.GetFootprint();
    stats->SysMemUsedSpace      = SysAllocWrapper.GetUsedSpace();
    stats->PageMapFootprint     = Starter.GetFootprint();
    stats->PageMapUsedSpace     = Starter.GetUsedSpace();
    stats->BookkeepingFootprint = Bookkeeper.GetFootprint();
    stats->BookkeepingUsedSpace = Bookkeeper.GetUsedSpace();
#ifdef GHEAP_DEBUG_INFO
    stats->DebugInfoFootprint   = DebugAlloc.GetFootprint();
    stats->DebugInfoUsedSpace   = DebugAlloc.GetUsedSpace();
#else
    stats->DebugInfoFootprint   = 0;
    stats->DebugInfoUsedSpace   = 0;
#endif
}


//------------------------------------------------------------------------
void* GHeapRoot::AllocSysDirect(UPInt size)
{
    GLockSafe::Locker lock(&RootLock);
    return SysAllocWrapper.AllocSysDirect(size);
}
void GHeapRoot::FreeSysDirect(void* ptr, UPInt size)
{
    GLockSafe::Locker lock(&RootLock);
    SysAllocWrapper.FreeSysDirect(ptr, size);
}



//------------------------------------------------------------------------
void GMemoryHeap::InitPT(GSysAllocPaged* sysAlloc) // static
{
    if (GHeapGlobalPageTable || GHeapGlobalRoot)
    {
        GFC_DEBUG_MESSAGE(1, "GMemoryHeap::Init: Memory system has been already initialized");
        GHEAP_ASSERT(0); return;
    }

    // Function HeapPageTable::Init() does not allocate any memory;
    // it just initializes the root page table with zeros. So, the 
    // sequence of calls must remain as below: First init the page table,
    // then create the root.
    GHeapPageTable::Init();
    ::new(GHeapRootMem) GHeapRoot(sysAlloc);
}


//------------------------------------------------------------------------
void GMemoryHeap::CleanUpPT() // static
{
    if (GHeapGlobalPageTable == 0 || GHeapGlobalRoot == 0)
    {
        GFC_DEBUG_MESSAGE(1, "GMemoryHeap::Finalize: Memory system was not initialized");
        GHEAP_ASSERT(0); return;
    }

    GHeapGlobalRoot->DestroyAllArenas();

    GHeapGlobalPageTable->~GHeapPageTable();
    GHeapGlobalPageTable = 0;

    GHeapGlobalRoot->~GHeapRoot();
    GHeapGlobalRoot = 0;
}


//------------------------------------------------------------------------
GMemoryHeap* GMemoryHeap::CreateRootHeapPT(const HeapDesc& desc) // static
{
    if (GHeapGlobalRoot == 0)
    {
        GFC_DEBUG_MESSAGE(1, "CreateRootHeap: Memory System is not initialized");
        GHEAP_ASSERT(0); return 0;
    }
    {
        GLockSafe::Locker locker(GHeapGlobalRoot->GetLock());
        if (GMemory::pGlobalHeap)
        {
            GFC_DEBUG_MESSAGE(1, "CreateRootHeap: Root Memory Heap is already created");
            GHEAP_ASSERT(0); return 0;
        }
        HeapDesc d2 = desc;
        d2.HeapId = GHeapId_Global;
        GMemory::pGlobalHeap = GHeapGlobalRoot->CreateHeap("Global", 0, d2);
        GHEAP_ASSERT(GMemory::pGlobalHeap);
    }
    return GMemory::pGlobalHeap;
}

//------------------------------------------------------------------------
GMemoryHeap* GMemoryHeap::CreateRootHeapPT() // static
{
    HeapDesc desc;
    desc.Flags         = Heap_Root;
    desc.MinAlign      = RootHeap_MinAlign;
    desc.Granularity   = RootHeap_Granularity;
    desc.Reserve       = RootHeap_Reserve;
    desc.Threshold     = RootHeap_Threshold;
    desc.Limit         = RootHeap_Limit;
    desc.HeapId        = GHeapId_Global;
    return CreateRootHeapPT(desc);
}

//------------------------------------------------------------------------
void GMemoryHeap::ReleaseRootHeapPT() // static
{
    // No locks needed because this function can be called
    // only after all the threads have shut down.
    if (GHeapGlobalRoot == 0)
    {
        GFC_DEBUG_MESSAGE(1, "ReleaseRootHeap: Memory System is not initialized");
        GHEAP_ASSERT(0); return;
    }
    GLockSafe::Locker locker(GHeapGlobalRoot->GetLock());
    if (GMemory::pGlobalHeap == 0)
    {
        GFC_DEBUG_MESSAGE(1, "ReleaseRootHeap:: Root Memory Heap was not created");
        GHEAP_ASSERT(0); return;
    }
    GMemory::pGlobalHeap->dumpMemoryLeaks();
    GMemory::pGlobalHeap->destroyItself();
    GMemory::pGlobalHeap = 0;
}


//------------------------------------------------------------------------
GMemoryHeapPT::GMemoryHeapPT() : GMemoryHeap(), pEngine(0), pDebugStorage(0)
{
}



// *** Operations with memory arenas
//
//------------------------------------------------------------------------
void GMemoryHeapPT::CreateArena(UPInt arena, GSysAllocPaged* sysAlloc)
{
    GHEAP_ASSERT(GHeapGlobalRoot);
    GHeapGlobalRoot->CreateArena(arena, sysAlloc);
}

void GMemoryHeapPT::DestroyArena(UPInt arena)
{
    GHEAP_ASSERT(GHeapGlobalRoot);
    GHeapGlobalRoot->DestroyArena(arena);
}

bool GMemoryHeapPT::ArenaIsEmpty(UPInt arena)
{
    GHEAP_ASSERT(GHeapGlobalRoot);
    return GHeapGlobalRoot->ArenaIsEmpty(arena);
}


//------------------------------------------------------------------------
GMemoryHeap* GMemoryHeapPT::CreateHeap(const char* name, 
                                       const HeapDesc& desc)
{
#ifdef GHEAP_FORCE_MALLOC
    GUNUSED3(name, desc, sysAlloc);
    return this;
#else
    GMemoryHeap* heap = 0;
    {
        GLockSafe::Locker lock(GHeapGlobalRoot->GetLock());
        heap = GHeapGlobalRoot->CreateHeap(name, this, desc);
        if (heap)
        {
            RefCount++;
            // printf("Created heap %p: %s\n", heap, name); // DBG
        }
    }
    if (heap)
    {
        GLock::Locker lock(&HeapLock);
        ChildHeaps.PushBack(heap);
    }
    return heap;
#endif
}

//------------------------------------------------------------------------
UPInt GMemoryHeapPT::GetFootprint() const
{
    GLock::Locker lock(&HeapLock);
    return pEngine->GetFootprint();
}

//------------------------------------------------------------------------
UPInt GMemoryHeapPT::GetTotalFootprint() const
{
    GLock::Locker lock(&HeapLock);
    UPInt footprint = 0;
    if (!(Info.Desc.Flags & GMemoryHeap::Heap_UserDebug))
    {
        footprint = pEngine->GetFootprint();
    }
    const GMemoryHeap* heap = ChildHeaps.GetFirst();
    while(!ChildHeaps.IsNull(heap))
    {
        footprint += heap->GetTotalFootprint();
        heap = heap->pNext;
    }
    return footprint;
}

//------------------------------------------------------------------------
UPInt GMemoryHeapPT::GetUsedSpace() const
{
    GLock::Locker lock(&HeapLock);
    return pEngine->GetUsedSpace();
}

//------------------------------------------------------------------------
UPInt GMemoryHeapPT::GetTotalUsedSpace() const
{
    GLock::Locker lock(&HeapLock);
    UPInt space = 0;
    if (!(Info.Desc.Flags & GMemoryHeap::Heap_UserDebug))
    {
        space = pEngine->GetUsedSpace();
    }
    const GMemoryHeap* heap = ChildHeaps.GetFirst();
    while(!ChildHeaps.IsNull(heap))
    {
        space += heap->GetTotalUsedSpace();
        heap = heap->pNext;
    }
    return space;
}

//------------------------------------------------------------------------
void GMemoryHeapPT::SetLimitHandler(LimitHandler* handler)
{
    GLock::Locker locker(&HeapLock);
    pEngine->SetLimitHandler(handler);
}

//------------------------------------------------------------------------
void GMemoryHeapPT::SetLimit(UPInt newLimit)
{
    GLock::Locker locker(&HeapLock);
    if (newLimit < Info.Desc.Limit)
    {
        if (newLimit < pEngine->GetFootprint())
            newLimit = pEngine->GetFootprint();
    }
    Info.Desc.Limit = pEngine->SetLimit(newLimit);
}

//------------------------------------------------------------------------
void GMemoryHeapPT::AddRef()
{
#ifndef GHEAP_FORCE_MALLOC
    // We use lock instead of atomic ops here to avoid accidental
    // heap release during VisitChildHeaps iteration.
    GLockSafe::Locker lock(GHeapGlobalRoot->GetLock());
    RefCount++;
#endif
}

//------------------------------------------------------------------------
void GMemoryHeapPT::Release()
{
#ifndef GHEAP_FORCE_MALLOC
    GMemoryHeap* parent = Info.pParent;
    if (parent)
    {
        // Lock scope.
        GLock::Locker localLock(&parent->HeapLock);
        GLockSafe::Locker globalLock(GHeapGlobalRoot->GetLock());

        if (--RefCount == 0)
        {
            // printf("Releasing heap %p: %s\n", this, GetName()); // DBG

            GHEAP_ASSERT((Info.Desc.Flags & Heap_Root) == 0); // Cannot free root.

            dumpMemoryLeaks();
            GHEAP_ASSERT(ChildHeaps.IsEmpty());

            parent->ChildHeaps.Remove(this);
            GHeapGlobalRoot->DestroyHeap(this);
        }
        else
        {
            parent = 0;
        }
    }
    else
    {
        GLockSafe::Locker lock(GHeapGlobalRoot->GetLock());
        RefCount--;

        // The global heap can be destroyed only from ReleaseRootHeap().
        GHEAP_ASSERT(RefCount);
    }

    // This Release() must be performed out of the lock scopes.
    if (parent)
        parent->Release();
#endif
}

//------------------------------------------------------------------------
bool GMemoryHeapPT::dumpMemoryLeaks()
{
    bool ret = false;

#ifdef GHEAP_DEBUG_INFO
    ret = pDebugStorage->DumpMemoryLeaks(Info.pName);
#endif

    // TO DO: May use something else instead of GFC_DEBUG_MESSAGE 
    // to report unreleased heaps. Or turn on GFC_DEBUG_MESSAGE in
    // Release mode.
#ifdef GFC_BUILD_DEBUG
    GMemoryHeap* heap = ChildHeaps.GetFirst();
    while(!ChildHeaps.IsNull(heap))
    {
        GFC_DEBUG_MESSAGE2(1, "Heap '%s' has unreleased child '%s'", 
                           Info.pName, 
                           heap->Info.pName);
        heap->dumpMemoryLeaks();
        heap = heap->pNext;
        ret  = true;
    }
#endif

    return ret;
}


//------------------------------------------------------------------------
void GMemoryHeapPT::destroyItself()
{
    GMemoryHeap* heap = ChildHeaps.GetFirst();
    while(!ChildHeaps.IsNull(heap))
    {
        GMemoryHeap* next = heap->pNext;
        heap->destroyItself();
        heap = next;
    }
    GHeapGlobalRoot->DestroyHeap(this);
}


//------------------------------------------------------------------------
inline void* GMemoryHeapPT::allocMem(UPInt size, const GAllocDebugInfo* info)
{
    void* ptr = pEngine->Alloc(size);

#ifdef GHEAP_TRACE_ALL
    GHeapGlobalRoot->OnAlloc(this, size, 0, 0, ptr);
#endif

#ifdef GHEAP_DEBUG_INFO

    UPInt usable = 0;
    if (ptr)
    {
        usable = pEngine->GetUsableSize(ptr);
#ifdef GHEAP_MEMSET_ALL
        memset(ptr, 0xCC, usable);
#endif
    }

    if (ptr && TrackDebugInfo)
    {
        if (!pDebugStorage->AddAlloc(0, false, UPInt(ptr), size, usable, info))
        {
            pEngine->Free(GHeapGlobalPageTable->GetSegment(UPInt(ptr)), ptr);
            ptr = 0;
        }
    }
#else
    GUNUSED(info);
#endif
    return ptr;
}


//------------------------------------------------------------------------
inline void* GMemoryHeapPT::allocMem(UPInt size, UPInt align, const GAllocDebugInfo* info)
{
    void* ptr = pEngine->Alloc(size, align);

#ifdef GHEAP_TRACE_ALL
    GHeapGlobalRoot->OnAlloc(this, size, align, 0, ptr);
#endif

#ifdef GHEAP_DEBUG_INFO
    UPInt usable = 0;
    if (ptr)
    {
        usable = pEngine->GetUsableSize(ptr);
#ifdef GHEAP_MEMSET_ALL
        memset(ptr, 0xCC, usable);
#endif
    }

    if (ptr && TrackDebugInfo)
    {
        if (!pDebugStorage->AddAlloc(0, false, UPInt(ptr), size, usable, info))
        {
            pEngine->Free(GHeapGlobalPageTable->GetSegment(UPInt(ptr)), ptr);
            ptr = 0;
        }
    }
#else
    GUNUSED(info);
#endif
    return ptr;
}

//------------------------------------------------------------------------
inline void* GMemoryHeapPT::allocMem(const void *thisPtr, UPInt size, const GAllocDebugInfo* info)
{
    void* ptr = pEngine->Alloc(size);

#ifdef GHEAP_TRACE_ALL
    unsigned sid = 0;
#ifdef GHEAP_DEBUG_INFO
    if (ptr && TrackDebugInfo)
    {
        sid = pDebugStorage->GetStatId(UPInt(thisPtr), info);
    }
#endif
    GHeapGlobalRoot->OnAlloc(this, size, 0, sid, ptr);
#endif

#ifdef GHEAP_DEBUG_INFO
    UPInt usable = 0;
    if (ptr)
    {
        usable = pEngine->GetUsableSize(ptr);
#ifdef GHEAP_MEMSET_ALL
        memset(ptr, 0xCC, usable);
#endif
    }

    if (ptr && TrackDebugInfo)
    {
        if (!pDebugStorage->AddAlloc(UPInt(thisPtr), true, UPInt(ptr), size, usable, info))
        {
            pEngine->Free(ptr);
            ptr = 0;
        }
    }
#else
    GUNUSED2(thisPtr, info);
#endif
    return ptr;
}

//------------------------------------------------------------------------
inline void* GMemoryHeapPT::allocMem(const void *thisPtr, UPInt size, UPInt align,
                                    const GAllocDebugInfo* info)
{
    void* ptr = pEngine->Alloc(size, align);

#ifdef GHEAP_TRACE_ALL
    unsigned sid = 0;
#ifdef GHEAP_DEBUG_INFO
    if (ptr && TrackDebugInfo)
    {
        sid = pDebugStorage->GetStatId(UPInt(thisPtr), info);
    }
#endif
    GHeapGlobalRoot->OnAlloc(this, size, align, sid, ptr);
#endif

#ifdef GHEAP_DEBUG_INFO
    UPInt usable = 0;
    if (ptr)
    {
        usable = pEngine->GetUsableSize(ptr);
#ifdef GHEAP_MEMSET_ALL
        memset(ptr, 0xCC, usable);
#endif
    }

    if (ptr && TrackDebugInfo)
    {
        if (!pDebugStorage->AddAlloc(UPInt(thisPtr), true, UPInt(ptr), size, usable, info))
        {
            pEngine->Free(ptr);
            ptr = 0;
        }
    }
#else
    GUNUSED2(thisPtr, info);
#endif
    return ptr;
}

//------------------------------------------------------------------------
inline void* GMemoryHeapPT::reallocMem(GHeapSegment* seg, void* oldPtr, UPInt newSize)
{
//// DBG
//UPInt oldSize = GetUsableSize(oldPtr);
//void* ptr = Alloc(newSize, (GAllocDebugInfo*)0);
//      //newSize = GetUsableSize(ptr);
//memcpy(ptr, oldPtr, (oldSize < newSize) ? oldSize : newSize);
//Free(oldPtr);
//return ptr;

    // Realloc cannot work as Alloc in case oldPtr is 0. It's done in 
    // order not to pass a lot of parameters, such as DebugInfo, alignment, 
    // parent allocation, etc.
    //------------------------
    GHEAP_ASSERT(oldPtr != 0);

#ifdef GHEAP_DEBUG_INFO
    UPInt usable = pEngine->GetUsableSize(oldPtr);
    GHeapDebugStorage::DebugDataPtr debugData;
    if (TrackDebugInfo)
    {
        pDebugStorage->UnlinkAlloc(UPInt(oldPtr), &debugData);
        pDebugStorage->CheckDataTail(&debugData, usable);
    }
#ifdef GHEAP_MEMSET_ALL
    if (newSize < usable)
    {
        memset((UByte*)oldPtr + newSize, 0xFE, usable - newSize);
    }
#endif //GHEAP_MEMSET_ALL
#endif //GHEAP_DEBUG_INFO

    void* newPtr = pEngine->Realloc(seg, oldPtr, newSize);
    if (newPtr)
    {
#ifdef GHEAP_DEBUG_INFO
        usable = pEngine->GetUsableSize(newPtr);
#ifdef GHEAP_MEMSET_ALL
        if (newPtr && newSize > usable)
        {
            memset((UByte*)newPtr + usable, 0xCC, newSize - usable);
        }
#endif //GHEAP_MEMSET_ALL
        if (TrackDebugInfo && newPtr)
        {
            pDebugStorage->RelinkAlloc(&debugData, UPInt(oldPtr), 
                                       UPInt(newPtr), newSize, usable);
        }
#endif //GHEAP_DEBUG_INFO
    }

#ifdef GHEAP_TRACE_ALL
    GHeapGlobalRoot->OnRealloc(this, oldPtr, newSize, newPtr);
#endif

    return newPtr;
}


//------------------------------------------------------------------------
void* GMemoryHeapPT::Alloc(UPInt size, const GAllocDebugInfo* info)
{
#ifdef GHEAP_FORCE_MALLOC
    GUNUSED(info);
    return _aligned_malloc(size, 8);
#else
    if (UseLocks)
    {
        GLock::Locker locker(&HeapLock);
        return allocMem(size, info);
    }
    GMEMORYHEAP_CHECK_CURRENT_THREAD(this);
    return allocMem(size, info);
#endif
}

//------------------------------------------------------------------------
void* GMemoryHeapPT::Alloc(UPInt size, UPInt align, const GAllocDebugInfo* info)
{
#ifdef GHEAP_FORCE_MALLOC
    GUNUSED(info);
    return _aligned_malloc(size, align);
#else
    if (UseLocks)
    {
        GLock::Locker locker(&HeapLock);
        return allocMem(size, align, info);
    }
    GMEMORYHEAP_CHECK_CURRENT_THREAD(this);
    return allocMem(size, align, info);
#endif
}

//------------------------------------------------------------------------
void* GMemoryHeapPT::Realloc(void* oldPtr, UPInt newSize)
{
#ifdef GHEAP_FORCE_MALLOC
    return _aligned_realloc(oldPtr, newSize, 8);
#else
    // Realloc cannot work as Alloc in case oldPtr is 0. It's done in 
    // order not to pass a lot of parameters, such as DebugInfo, alignment, 
    // parent allocation, etc.
    //------------------------
    GHEAP_ASSERT(oldPtr != 0);

    GHeapSegment* seg = GHeapGlobalPageTable->GetSegment(UPInt(oldPtr));
    GHEAP_ASSERT(seg && seg->pHeap);
    GMemoryHeapPT* heap = seg->pHeap;
    if (heap->UseLocks)
    {
        GLock::Locker locker(&heap->HeapLock);
        return heap->reallocMem(seg, oldPtr, newSize);
    }
    GMEMORYHEAP_CHECK_CURRENT_THREAD(heap);
    return heap->reallocMem(seg, oldPtr, newSize);
#endif
}

//------------------------------------------------------------------------
void  GMemoryHeapPT::Free(void* ptr)
{
//UltimateCheck(); // DBG
#ifdef GHEAP_FORCE_MALLOC
    _aligned_free(ptr);
#else
//static int c; if(++c % 500 == 0) return; // DBG Simulate memory leaks
    if (ptr)
    {
        GHeapSegment* seg = GHeapGlobalPageTable->GetSegment(UPInt(ptr));
        GHEAP_ASSERT(seg && seg->pHeap);
        GMemoryHeapPT* heap = seg->pHeap;

        if (heap->UseLocks)
        {
            {
                GLock::Locker locker(&heap->HeapLock);
#ifdef GHEAP_DEBUG_INFO
                UPInt usable = heap->pEngine->GetUsableSize(ptr);
                heap->pDebugStorage->RemoveAlloc(UPInt(ptr), usable);
#ifdef GHEAP_MEMSET_ALL
                memset(ptr, 0xFE, usable);
#endif //GHEAP_MEMSET_ALL
#endif //GHEAP_DEBUG_INFO
                heap->pEngine->Free(seg, ptr);
#ifdef GHEAP_TRACE_ALL
                GHeapGlobalRoot->OnFree(heap, ptr);
#endif
            }
            // Release is performed outside of the locker scope to 
            // avoid accessing released heap and the lock respectively.
            // This check is OK with threading because "ReleaseOnFree"
            // always takes place with the last Free request.
            if (ptr == heap->pAutoRelease)
            {
                heap->Release();
            }
        }
        else
        {
            GMEMORYHEAP_CHECK_CURRENT_THREAD(heap);
#ifdef GHEAP_DEBUG_INFO
            UPInt usable = heap->pEngine->GetUsableSize(ptr);
            heap->pDebugStorage->RemoveAlloc(UPInt(ptr), usable);
#ifdef GHEAP_MEMSET_ALL
            memset(ptr, 0xFE, usable);
#endif //GHEAP_MEMSET_ALL
#endif //GHEAP_DEBUG_INFO
            heap->pEngine->Free(seg, ptr);
#ifdef GHEAP_TRACE_ALL
            GHeapGlobalRoot->OnFree(heap, ptr);
#endif
            if (ptr == heap->pAutoRelease)
            {
                heap->Release();
            }
        }
    }
#endif // GHEAP_FORCE_MALLOC
}


//------------------------------------------------------------------------
void* GMemoryHeapPT::AllocAutoHeap(const void *thisPtr, UPInt size,
                                   const GAllocDebugInfo* info)
{
#ifdef GHEAP_FORCE_MALLOC
    GUNUSED2(thisPtr, info);
    return _aligned_malloc(size, 8);
#else
    GHeapSegment* seg = GHeapGlobalPageTable->GetSegment(UPInt(thisPtr));
    GHEAP_ASSERT(seg && seg->pHeap);
    GMemoryHeapPT* heap = seg->pHeap;
    if (heap->UseLocks)
    {
        GLock::Locker locker(&heap->HeapLock);
        return heap->allocMem(thisPtr, size, info);
    }
    GMEMORYHEAP_CHECK_CURRENT_THREAD(heap);
    return heap->allocMem(thisPtr, size, info);
#endif // GHEAP_FORCE_MALLOC
}


//------------------------------------------------------------------------
void* GMemoryHeapPT::AllocAutoHeap(const void *thisPtr, UPInt size, UPInt align,
                                   const GAllocDebugInfo* info)
{
#ifdef GHEAP_FORCE_MALLOC
    GUNUSED2(thisPtr, info);
    return _aligned_malloc(size, align);
#else
    GHeapSegment* seg = GHeapGlobalPageTable->GetSegment(UPInt(thisPtr));
    GHEAP_ASSERT(seg && seg->pHeap);
    GMemoryHeapPT* heap = seg->pHeap;
    if (heap->UseLocks)
    {
        GLock::Locker locker(&heap->HeapLock);
        return heap->allocMem(thisPtr, size, align, info);
    }
    GMEMORYHEAP_CHECK_CURRENT_THREAD(heap);
    return heap->allocMem(thisPtr, size, align, info);
#endif // GHEAP_FORCE_MALLOC
}


//------------------------------------------------------------------------
GMemoryHeap* GMemoryHeapPT::GetAllocHeap(const void *thisPtr)
{
#ifdef GHEAP_FORCE_MALLOC
    GUNUSED(thisPtr);
    return Memory::pGlobalHeap;
#else
    GHeapSegment* seg = GHeapGlobalPageTable->GetSegment(UPInt(thisPtr));
    GHEAP_ASSERT(seg && seg->pHeap);
    return seg->pHeap;
#endif // GHEAP_FORCE_MALLOC
}

//------------------------------------------------------------------------
UPInt GMemoryHeapPT::GetUsableSize(const void* ptr)
{
#ifdef GHEAP_FORCE_MALLOC
    GUNUSED(ptr);
    return 0;
#else
    GHeapSegment* seg = GHeapGlobalPageTable->GetSegment(UPInt(ptr));
    GHEAP_ASSERT(seg && seg->pHeap);
    GMemoryHeapPT* heap = seg->pHeap;
    if (heap->UseLocks)
    {
        GLock::Locker locker(&heap->HeapLock);
        return heap->pEngine->GetUsableSize(seg, ptr);
    }
    return heap->pEngine->GetUsableSize(seg, ptr);
#endif // GHEAP_FORCE_MALLOC
}


//------------------------------------------------------------------------
void GMemoryHeapPT::releaseCachedMem()
{
    GLock::Locker locker(&HeapLock);
    GMemoryHeap* heap = ChildHeaps.GetFirst();
    while(!ChildHeaps.IsNull(heap))
    {
        heap->releaseCachedMem();
        heap = heap->pNext;
    }
    pEngine->ReleaseCachedMem();
}

//------------------------------------------------------------------------
void* GMemoryHeapPT::AllocSysDirect(UPInt size)
{
    return GHeapGlobalRoot->AllocSysDirect(size);
}
void GMemoryHeapPT::FreeSysDirect(void* ptr, UPInt size)
{
    GHeapGlobalRoot->FreeSysDirect(ptr, size);
}

//------------------------------------------------------------------------
bool GMemoryHeapPT::GetStats(GStatBag* bag)
{
#ifdef GHEAP_FORCE_MALLOC
    GUNUSED(bag);
    return true;
#else
    GLock::Locker locker(&HeapLock);

    UPInt footprint = pEngine->GetFootprint();
    
    bag->AddStat(GStatHeap_LocalFootprint,   GCounterStat(footprint));
    bag->AddStat(GStatHeap_LocalUsedSpace,   GCounterStat(pEngine->GetUsedSpace()));
    bag->AddStat(GStatHeap_Granularity,      GCounterStat(Info.Desc.Granularity));
    bag->AddStat(GStatHeap_Reserve,          GCounterStat(Info.Desc.Reserve));

    GHeapOtherStats otherStats;
    pEngine->GetHeapOtherStats(&otherStats);

    otherStats.Bookkeeping += SelfSize;

    bag->AddStat(GStatHeap_Bookkeeping,        GCounterStat(otherStats.Bookkeeping));
#ifdef GHEAP_DEBUG_INFO
    bag->AddStat(GStatHeap_DebugInfo,          GCounterStat(pDebugStorage->GetUsedSpace()));
#endif
    bag->AddStat(GStatHeap_Segments,           GCounterStat(otherStats.Segments));
    bag->AddStat(GStatHeap_DynamicGranularity, GCounterStat(otherStats.DynamicGranularity));
    bag->AddStat(GStatHeap_SysDirectSpace,     GCounterStat(otherStats.SysDirectSpace));

    // Collect child footprint.
    UPInt childHeapCount = 0;
    UPInt childFootprint = 0;

    GMemoryHeap* heap = ChildHeaps.GetFirst();
    while(!ChildHeaps.IsNull(heap))
    {
        // TBD: Should we have more detailed logic here?
        if (!(heap->Info.Desc.Flags & GMemoryHeap::Heap_UserDebug))
        {
            childHeapCount++;
            // Thread TBD: Need to make sure that child heap locks are not obtained before parent.
            childFootprint += heap->GetTotalFootprint();
        }       
        heap = heap->pNext;
    }

    if (childHeapCount)
    {
        bag->AddStat(GStatHeap_ChildHeaps,    GCounterStat(childHeapCount));
        bag->AddStat(GStatHeap_ChildFootprint,GCounterStat(childFootprint));
    }
    bag->AddStat(GStatHeap_TotalFootprint,   GCounterStat(childFootprint + footprint));

    // Collect detailed memory summary information in debug builds.
#ifdef GHEAP_DEBUG_INFO
    pDebugStorage->GetStats(pEngine, bag);
#endif    
    return true;
#endif // GHEAP_FORCE_MALLOC
}

//------------------------------------------------------------------------
void GMemoryHeapPT::ultimateCheck()
{
#ifdef GHEAP_DEBUG_INFO
    GLock::Locker locker(&HeapLock);
    pDebugStorage->UltimateCheck();
    GMemoryHeap* heap = ChildHeaps.GetFirst();
    while(!ChildHeaps.IsNull(heap))
    {
        heap->ultimateCheck();
        heap = heap->pNext;
    }
#endif
}

//------------------------------------------------------------------------
void GMemoryHeapPT::VisitMem(GHeapMemVisitor* visitor, unsigned flags)
{
    GLock::Locker locker(&HeapLock);
    pEngine->VisitMem(visitor, flags);
#ifdef GHEAP_DEBUG_INFO
    pDebugStorage->VisitMem(visitor, flags);
#endif
}

//------------------------------------------------------------------------
void GMemoryHeapPT::VisitHeapSegments(GHeapSegVisitor* visitor) const
{
    GLock::Locker locker(&HeapLock);
    pEngine->VisitSegments(visitor);
}

//------------------------------------------------------------------------
void GMemoryHeapPT::VisitRootSegments(GHeapSegVisitor* visitor)
{
    GHeapGlobalRoot->VisitSegments(visitor);
}

//------------------------------------------------------------------------
void GMemoryHeapPT::getUserDebugStats(RootStats* stats) const
{
    GLock::Locker lock(&HeapLock);
    if (Info.Desc.Flags & GMemoryHeap::Heap_UserDebug)
    {
        stats->UserDebugFootprint += pEngine->GetFootprint();
        stats->UserDebugUsedSpace += pEngine->GetUsedSpace();
    }
    const GMemoryHeap* heap = ChildHeaps.GetFirst();
    while(!ChildHeaps.IsNull(heap))
    {
        heap->getUserDebugStats(stats);
        heap = heap->pNext;
    }
}

//------------------------------------------------------------------------
void GMemoryHeapPT::GetRootStats(RootStats* stats)
{
    GHeapGlobalRoot->GetStats(stats);
    stats->UserDebugFootprint = 0;
    stats->UserDebugUsedSpace = 0;
    GMemory::pGlobalHeap->getUserDebugStats(stats);
}

//------------------------------------------------------------------------
void GMemoryHeapPT::checkIntegrity() const
{
    GLock::Locker locker(&HeapLock);

    pEngine->CheckIntegrity();
    const GMemoryHeap* heap = ChildHeaps.GetFirst();
    while(!ChildHeaps.IsNull(heap))
    {
        // AddRef() and Release() are locked with a Global Lock here,
        // to avoid the possibility of heap being released during this iteration.
        // If necessary, this could re rewritten with AddRef_NotZero for efficiency.
        heap->checkIntegrity();
        heap = heap->pNext;
    }
}

//------------------------------------------------------------------------
void GMemoryHeapPT::SetTracer(HeapTracer* tracer) 
{
#ifdef GHEAP_TRACE_ALL
    GHeapGlobalRoot->pTracer = tracer;
#else
    GUNUSED(tracer);
#endif
}

//------------------------------------------------------------------------

// Implementation of GSysAllocBase based on MemoryHeapPT.
// Initializes heap system, creating and initializing GlobalHeap.
bool GSysAllocPaged::initHeapEngine(const void* heapDesc)
{
    GMemoryHeap::InitPT(this);
    const GMemoryHeap::HeapDesc& rootHeapDesc = *(const GMemoryHeap::HeapDesc*)heapDesc;
    GMemoryHeap* p = GMemoryHeap::CreateRootHeapPT(rootHeapDesc);
    return p != 0;
}

void GSysAllocPaged::shutdownHeapEngine()
{
    GMemoryHeap::ReleaseRootHeapPT();
    GMemoryHeap::CleanUpPT();
}
