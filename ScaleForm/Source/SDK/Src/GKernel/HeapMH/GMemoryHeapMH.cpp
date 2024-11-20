/**********************************************************************

Filename    :   MemoryHeapMH.cpp
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

#include "GHeapRootMH.h"
#include "GHeapAllocEngineMH.h"
#include "GHeapDebugInfoMH.h"
#include "GDebug.h"
#include "GMemoryHeapMH.h"
#include "GMemory.h"

#undef new

#ifdef SN_TARGET_PS3
#include <stdlib.h>	// for reallocalign()
#endif

#if defined(GFC_OS_WIN32) && defined(GFC_BUILD_DEBUG)
#define GMEMORY_CHECK_CURRENT_THREAD(h) GASSERT((h->OwnerThreadId == 0) || (h->OwnerThreadId == ::GetCurrentThreadId()))
#else
#define GMEMORY_CHECK_CURRENT_THREAD(h)
#endif

static UPInt GHeapRootMemMH[(sizeof(GHeapRootMH) + sizeof(UPInt) - 1) / sizeof(UPInt)];

//------------------------------------------------------------------------
void GMemoryHeap::InitMH(GSysAlloc* sysAlloc) // static
{
    if (GHeapGlobalRootMH)
    {
        GFC_DEBUG_MESSAGE(1, "GMemoryHeap::Init: Memory system has been already initialized");
        GASSERT(0); return;
    }

    // Function HeapPageTable::Init() does not allocate any memory;
    // it just initializes the root page table with zeros. So, the 
    // sequence of calls must remain as below: First init the page table,
    // then create the root.
    ::new(GHeapRootMemMH) GHeapRootMH(sysAlloc);
}


//------------------------------------------------------------------------
void GMemoryHeap::CleanUpMH() // static
{
    if (GHeapGlobalRootMH == 0)
    {
        GFC_DEBUG_MESSAGE(1, "MemoryHeap::Finalize: Memory system was not initialized");
        GASSERT(0); return;
    }
    GHeapGlobalRootMH->~GHeapRootMH();
    GHeapGlobalRootMH = 0;
}


//------------------------------------------------------------------------
GMemoryHeap* GMemoryHeap::CreateRootHeapMH(const HeapDesc& desc) // static
{
    if (GHeapGlobalRootMH == 0)
    {
        GFC_DEBUG_MESSAGE(1, "CreateRootHeap: Memory System is not initialized");
        GASSERT(0); return 0;
    }
    {
        GLockSafe::Locker locker(GHeapGlobalRootMH->GetLock());
        if (GMemory::pGlobalHeap)
        {
            GFC_DEBUG_MESSAGE(1, "CreateRootHeap: Root Memory Heap is already created");
            GASSERT(0); return 0;
        }
        HeapDesc d2 = desc;
        d2.HeapId = GHeapId_Global;
        GMemory::pGlobalHeap = GHeapGlobalRootMH->CreateHeap("Global", 0, d2);
        GASSERT(GMemory::pGlobalHeap);
    }
    return GMemory::pGlobalHeap;
}

//------------------------------------------------------------------------
GMemoryHeap* GMemoryHeap::CreateRootHeapMH() // static
{
    HeapDesc desc;
    desc.Flags         = Heap_Root;
    desc.MinAlign      = RootHeap_MinAlign;
    desc.Granularity   = RootHeap_Granularity;
    desc.Reserve       = RootHeap_Reserve;
    desc.Threshold     = RootHeap_Threshold;
    desc.Limit         = RootHeap_Limit;
    desc.HeapId        = GHeapId_Global;
    return CreateRootHeapMH(desc);
}

//------------------------------------------------------------------------
void GMemoryHeap::ReleaseRootHeapMH() // static
{
    // No locks needed because this function can be called
    // only after all the threads have shut down.
    if (GHeapGlobalRootMH == 0)
    {
        GFC_DEBUG_MESSAGE(1, "ReleaseRootHeap: Memory System is not initialized");
        GASSERT(0); return;
    }
    GLockSafe::Locker locker(GHeapGlobalRootMH->GetLock());
    if (GMemory::pGlobalHeap == 0)
    {
        GFC_DEBUG_MESSAGE(1, "ReleaseRootHeap:: Root Memory Heap was not created");
        GASSERT(0); return;
    }
    GMemory::pGlobalHeap->dumpMemoryLeaks();
    GMemory::pGlobalHeap->destroyItself();
    GMemory::pGlobalHeap = 0;
}


//------------------------------------------------------------------------
GMemoryHeapMH::GMemoryHeapMH() : GMemoryHeap(), pEngine(0), pDebugStorage(0)
{
}



// *** Operations with memory arenas
//
//------------------------------------------------------------------------
void GMemoryHeapMH::CreateArena(UPInt, GSysAllocPaged*)
{
}

void GMemoryHeapMH::DestroyArena(UPInt)
{
}

bool GMemoryHeapMH::ArenaIsEmpty(UPInt)
{
    return true;
}


//------------------------------------------------------------------------
GMemoryHeap* GMemoryHeapMH::CreateHeap(const char* name, 
                                       const HeapDesc& desc)
{
    GMemoryHeap* heap = 0;
    {
        GLockSafe::Locker lock(GHeapGlobalRootMH->GetLock());
        heap = GHeapGlobalRootMH->CreateHeap(name, this, desc);
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
}

//------------------------------------------------------------------------
UPInt GMemoryHeapMH::GetFootprint() const
{
    GLock::Locker lock(&HeapLock);
    return pEngine->GetFootprint();
}

//------------------------------------------------------------------------
UPInt GMemoryHeapMH::GetTotalFootprint() const
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
UPInt GMemoryHeapMH::GetUsedSpace() const
{
    GLock::Locker lock(&HeapLock);
    return pEngine->GetUsedSpace();
}

//------------------------------------------------------------------------
UPInt GMemoryHeapMH::GetTotalUsedSpace() const
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
void GMemoryHeapMH::SetLimitHandler(LimitHandler* handler)
{
    GLock::Locker locker(&HeapLock);
    pEngine->SetLimitHandler(handler);
}

//------------------------------------------------------------------------
void GMemoryHeapMH::SetLimit(UPInt newLimit)
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
void GMemoryHeapMH::AddRef()
{
    // We use lock instead of atomic ops here to avoid accidental
    // heap release during VisitChildHeaps iteration.
    GLockSafe::Locker lock(GHeapGlobalRootMH->GetLock());
    RefCount++;
}

//------------------------------------------------------------------------
void GMemoryHeapMH::Release()
{
    GMemoryHeap* parent = Info.pParent;
    if (parent)
    {
        // Lock scope.
        GLock::Locker localLock(&parent->HeapLock);
        GLockSafe::Locker globalLock(GHeapGlobalRootMH->GetLock());

        if (--RefCount == 0)
        {
            // printf("Releasing heap %p: %s\n", this, GetName()); // DBG

            GASSERT((Info.Desc.Flags & Heap_Root) == 0); // Cannot free root.

            dumpMemoryLeaks();
            GASSERT(ChildHeaps.IsEmpty());

            parent->ChildHeaps.Remove(this);
            GHeapGlobalRootMH->DestroyHeap(this);
        }
        else
        {
            parent = 0;
        }
    }
    else
    {
        GLockSafe::Locker lock(GHeapGlobalRootMH->GetLock());
        RefCount--;

        // The global heap can be destroyed only from ReleaseRootHeap().
        GASSERT(RefCount);
    }

    // This Release() must be performed out of the lock scopes.
    if (parent)
        parent->Release();
}

//------------------------------------------------------------------------
bool GMemoryHeapMH::dumpMemoryLeaks()
{
    bool ret = false;

#ifdef GHEAP_DEBUG_INFO
    ret = pDebugStorage->DumpMemoryLeaks(Info.pName);
#endif

    // TO DO: May use something else instead of SF_DEBUG_MESSAGE 
    // to report unreleased heaps. Or turn on SF_DEBUG_MESSAGE in
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
void GMemoryHeapMH::destroyItself()
{
    GMemoryHeap* heap = ChildHeaps.GetFirst();
    while(!ChildHeaps.IsNull(heap))
    {
        GMemoryHeap* next = heap->pNext;
        heap->destroyItself();
        heap = next;
    }
    GHeapGlobalRootMH->DestroyHeap(this);
}

//------------------------------------------------------------------------
void GMemoryHeapMH::freeLocked(void* ptr)
{
    GLock::Locker heapLocker(&HeapLock);
    GLockSafe::Locker rootLocker(GHeapGlobalRootMH->GetLock());
    pEngine->Free(ptr);
}

//------------------------------------------------------------------------
inline void* GMemoryHeapMH::allocMem(UPInt size, const GAllocDebugInfo* info)
{
    GHeapPageInfoMH pageInfo;
    void* ptr = pEngine->Alloc(size, &pageInfo);

#ifdef GHEAP_TRACE_ALL
    GHeapGlobalRootMH->OnAlloc(this, size, 0, 0, ptr);
#endif

#ifdef GHEAP_DEBUG_INFO

#ifdef GHEAP_MEMSET_ALL
    if (ptr)
        memset(ptr, 0xCC, pageInfo.UsableSize);
#endif

    if (ptr && TrackDebugInfo)
    {
        if (!pDebugStorage->AddAlloc(UPInt(ptr), size, &pageInfo, info))
        {
            freeLocked(ptr);
            ptr = 0;
        }
    }
#else
    GUNUSED(info);
#endif
    return ptr;
}


//------------------------------------------------------------------------
inline void* GMemoryHeapMH::allocMem(UPInt size, UPInt align, const GAllocDebugInfo* info)
{
    GHeapPageInfoMH pageInfo;
    void* ptr = pEngine->Alloc(size, align, &pageInfo);

#ifdef GHEAP_TRACE_ALL
    GHeapGlobalRootMH->OnAlloc(this, size, align, 0, ptr);
#endif

#ifdef GHEAP_DEBUG_INFO
#ifdef GHEAP_MEMSET_ALL
    if (ptr)
        memset(ptr, 0xCC, pageInfo.UsableSize);
#endif

    if (ptr && TrackDebugInfo)
    {
        if (!pDebugStorage->AddAlloc(UPInt(ptr), size, &pageInfo, info))
        {
            freeLocked(ptr);
            ptr = 0;
        }
    }
#else
    GUNUSED(info);
#endif
    return ptr;
}

//------------------------------------------------------------------------
inline void* GMemoryHeapMH::allocMem(GHeapPageInfoMH* parentInfo, const void *thisPtr, 
                                     UPInt size, const GAllocDebugInfo* info)
{
    GHeapPageInfoMH pageInfo;
    void* ptr = pEngine->Alloc(size, &pageInfo);

#ifdef GHEAP_TRACE_ALL
    unsigned sid = 0;
#ifdef GHEAP_ENABLE_DEBUG_INFO
    if (ptr && TrackDebugInfo)
    {
        sid = pDebugStorage->GetStatId(&pageInfo, UPInt(thisPtr), info);
    }
#endif
    GHeapGlobalRootMH->OnAlloc(this, size, 0, sid, ptr);
#endif

#ifdef GHEAP_DEBUG_INFO
#ifdef GHEAP_MEMSET_ALL
    if (ptr)
        memset(ptr, 0xCC, pageInfo.UsableSize);
#endif

    if (ptr && TrackDebugInfo)
    {
        if (!pDebugStorage->AddAlloc(parentInfo, UPInt(thisPtr), 
                                     UPInt(ptr), size, 
                                     &pageInfo, info))
        {
            freeLocked(ptr);
            ptr = 0;
        }
    }
#else
    GUNUSED3(parentInfo, thisPtr, info);
#endif
    return ptr;
}

//------------------------------------------------------------------------
inline void* GMemoryHeapMH::allocMem(GHeapPageInfoMH* parentInfo, 
                                     const void *thisPtr, UPInt size, UPInt align,
                                     const GAllocDebugInfo* info)
{
    GHeapPageInfoMH pageInfo;
    void* ptr = pEngine->Alloc(size, align, &pageInfo);

#ifdef GHEAP_TRACE_ALL
    unsigned sid = 0;
#ifdef GHEAP_DEBUG_INFO
    if (ptr && TrackDebugInfo)
    {
        sid = pDebugStorage->GetStatId(&pageInfo, UPInt(thisPtr), info);
    }
#endif
    GHeapGlobalRootMH->OnAlloc(this, size, align, sid, ptr);
#endif

#ifdef GHEAP_DEBUG_INFO
#ifdef GHEAP_MEMSET_ALL
    if (ptr)
        memset(ptr, 0xCC, pageInfo.UsableSize);
#endif

    if (ptr && TrackDebugInfo)
    {
        if (!pDebugStorage->AddAlloc(parentInfo, UPInt(thisPtr), 
                                     UPInt(ptr), size, 
                                     &pageInfo, info))
        {
            freeLocked(ptr);
            ptr = 0;
        }
    }
#else
    GUNUSED3(parentInfo, thisPtr, info);
#endif
    return ptr;
}


//------------------------------------------------------------------------
void* GMemoryHeapMH::Alloc(UPInt size, const GAllocDebugInfo* info)
{
    if (UseLocks)
    {
        GLock::Locker locker(&HeapLock);
        return allocMem(size, info);
    }
    GMEMORY_CHECK_CURRENT_THREAD(this);
    return allocMem(size, info);
}

//------------------------------------------------------------------------
void* GMemoryHeapMH::Alloc(UPInt size, UPInt align, const GAllocDebugInfo* info)
{
    if (UseLocks)
    {
        GLock::Locker locker(&HeapLock);
        return allocMem(size, align, info);
    }
    GMEMORY_CHECK_CURRENT_THREAD(this);
    return allocMem(size, align, info);
}


//------------------------------------------------------------------------
void* GMemoryHeapMH::AllocAutoHeap(const void *thisPtr, UPInt size,
                                   const GAllocDebugInfo* info)
{
    GMemoryHeapMH* heap;
    GHeapPageMH* page = GHeapGlobalRootMH->ResolveAddress(UPInt(thisPtr));
#ifdef GHEAP_DEBUG_INFO
    GHeapPageInfoMH parentInfo;
#endif
    void* ptr = 0;
    if (page)
    {
        heap = page->pHeap;
        if (heap->UseLocks)
        {
            GLock::Locker locker(&heap->HeapLock);
#ifdef GHEAP_DEBUG_INFO
            heap->pEngine->GetPageInfo(page, &parentInfo);
            return heap->allocMem(&parentInfo, thisPtr, size, info);
#else
            return heap->allocMem(0, thisPtr, size, info);
#endif
        }
        else
        {
            GMEMORY_CHECK_CURRENT_THREAD(this);
#ifdef GHEAP_DEBUG_INFO
            heap->pEngine->GetPageInfo(page, &parentInfo);
            return heap->allocMem(&parentInfo, thisPtr, size, info);
#else
            return heap->allocMem(0, thisPtr, size, info);
#endif
        }
    }
    else
    {
        GHeapNodeMH* node = 0;
        {
            GLockSafe::Locker locker(GHeapGlobalRootMH->GetLock());
            node = GHeapGlobalRootMH->FindNodeInGlobalTree((UByte*)thisPtr);
            GASSERT(node);
            heap = node->GetHeap();
        }
        if (heap->UseLocks)
        {
            GLock::Locker locker1(&heap->HeapLock);
            GLockSafe::Locker locker2(GHeapGlobalRootMH->GetLock());
#ifdef GHEAP_DEBUG_INFO
            heap->pEngine->GetPageInfo(node, &parentInfo);
            ptr = heap->allocMem(&parentInfo, thisPtr, size, info);
#else
            ptr = heap->allocMem(0, thisPtr, size, info);
#endif
        }
        else
        {
            GLockSafe::Locker locker2(GHeapGlobalRootMH->GetLock());
#ifdef GHEAP_DEBUG_INFO
            heap->pEngine->GetPageInfo(node, &parentInfo);
            ptr = heap->allocMem(&parentInfo, thisPtr, size, info);
#else
            ptr = heap->allocMem(0, thisPtr, size, info);
#endif
        }
    }
    return ptr;
}

//------------------------------------------------------------------------
void* GMemoryHeapMH::AllocAutoHeap(const void *thisPtr, UPInt size, UPInt align,
                                   const GAllocDebugInfo* info)
{
    GMemoryHeapMH* heap;
    GHeapPageMH* page = GHeapGlobalRootMH->ResolveAddress(UPInt(thisPtr));
#ifdef GHEAP_DEBUG_INFO
    GHeapPageInfoMH parentInfo;
#endif
    void* ptr = 0;
    if (page)
    {
        heap = page->pHeap;
        if (heap->UseLocks)
        {
            GLock::Locker locker(&heap->HeapLock);
#ifdef GHEAP_DEBUG_INFO
            heap->pEngine->GetPageInfo(page, &parentInfo);
            return heap->allocMem(&parentInfo, thisPtr, size, align, info);
#else
            return heap->allocMem(0, thisPtr, size, align, info);
#endif
        }
        else
        {
            GMEMORY_CHECK_CURRENT_THREAD(this);
#ifdef GHEAP_DEBUG_INFO
            heap->pEngine->GetPageInfo(page, &parentInfo);
            return heap->allocMem(&parentInfo, thisPtr, size, align, info);
#else
            return heap->allocMem(0, thisPtr, size, align, info);
#endif
        }
    }
    else
    {
        GHeapNodeMH* node = 0;
        {
            GLockSafe::Locker locker(GHeapGlobalRootMH->GetLock());
            node = GHeapGlobalRootMH->FindNodeInGlobalTree((UByte*)thisPtr);
            GASSERT(node);
            heap = node->GetHeap();
        }
        if (heap->UseLocks)
        {
            GLock::Locker locker1(&heap->HeapLock);
            GLockSafe::Locker locker2(GHeapGlobalRootMH->GetLock());
#ifdef GHEAP_DEBUG_INFO
            heap->pEngine->GetPageInfo(page, &parentInfo);
            ptr = heap->allocMem(&parentInfo, thisPtr, size, align, info);
#else
            ptr = heap->allocMem(0, thisPtr, size, align, info);
#endif
        }
        else
        {
            GLockSafe::Locker locker2(GHeapGlobalRootMH->GetLock());
#ifdef GHEAP_DEBUG_INFO
            heap->pEngine->GetPageInfo(page, &parentInfo);
            ptr = heap->allocMem(&parentInfo, thisPtr, size, align, info);
#else
            ptr = heap->allocMem(0, thisPtr, size, align, info);
#endif
        }
    }
    return ptr;
}



//------------------------------------------------------------------------
void  GMemoryHeapMH::freeMem(GHeapPageMH* page, void* ptr)
{
#ifdef GHEAP_DEBUG_INFO
    GHeapPageInfoMH pageInfo;
    pEngine->GetPageInfoWithSize(page, ptr, &pageInfo);
    pDebugStorage->RemoveAlloc(UPInt(ptr), &pageInfo);
#ifdef GHEAP_MEMSET_ALL
    memset(ptr, 0xFE, pageInfo.UsableSize);
#endif //GHEAP_MEMSET_ALL
#endif //GHEAP_DEBUG_INFO
    pEngine->Free(page, ptr);
#ifdef GHEAP_TRACE_ALL
    GHeapGlobalRootMH->OnFree(this, ptr);
#endif
}

//------------------------------------------------------------------------
void  GMemoryHeapMH::freeMem(GHeapNodeMH* node, void* ptr)
{
#ifdef GHEAP_DEBUG_INFO
    GHeapPageInfoMH pageInfo;
    pEngine->GetPageInfoWithSize(node, ptr, &pageInfo);
    pDebugStorage->RemoveAlloc(UPInt(ptr), &pageInfo);
#ifdef GHEAP_MEMSET_ALL
    memset(ptr, 0xFE, pageInfo.UsableSize);
#endif //GHEAP_MEMSET_ALL
#endif //GHEAP_DEBUG_INFO
    pEngine->Free(node, ptr);
#ifdef GHEAP_TRACE_ALL
    GHeapGlobalRootMH->OnFree(this, ptr);
#endif
}

//------------------------------------------------------------------------
void  GMemoryHeapMH::Free(void* ptr)
{
//UltimateCheck(); // DBG
//static int c; if(++c % 500 == 0) return; // DBG Simulate memory leaks
    if (ptr)
    {
        GMemoryHeapMH* heap = 0;
        GHeapPageMH* page = GHeapGlobalRootMH->ResolveAddress(UPInt(ptr));
        if (page)
        {
            heap = page->pHeap;
            if (heap->UseLocks)
            {
                GLock::Locker locker(&heap->HeapLock);
                heap->freeMem(page, ptr);
            }
            else
            {
                GMEMORY_CHECK_CURRENT_THREAD(this);
                heap->freeMem(page, ptr);
            }
        }
        else
        {
            GHeapNodeMH* node = 0;
            {
                GLockSafe::Locker locker(GHeapGlobalRootMH->GetLock());
                node = GHeapGlobalRootMH->FindNodeInGlobalTree((UByte*)ptr);
                GASSERT(node);
                heap = node->GetHeap();
            }
            if (heap->UseLocks)
            {
                GLock::Locker locker1(&heap->HeapLock);
                GLockSafe::Locker locker2(GHeapGlobalRootMH->GetLock());
                heap->freeMem(node, ptr);
            }
            else
            {
                GLockSafe::Locker locker2(GHeapGlobalRootMH->GetLock());
                heap->freeMem(node, ptr);
            }
        }
        GASSERT(heap);
        if (ptr == heap->pAutoRelease)
            heap->Release();
    }
}


//------------------------------------------------------------------------
void* GMemoryHeapMH::reallocMem(GHeapPageMH* page, void* oldPtr, UPInt newSize)
{
#ifdef GHEAP_DEBUG_INFO
    GHeapPageInfoMH oldPageInfo;
    pEngine->GetPageInfoWithSize(page, oldPtr, &oldPageInfo);
    GHeapDebugStorageMH::DebugDataPtr debugData;
    if (TrackDebugInfo)
    {
        pDebugStorage->UnlinkAlloc(UPInt(oldPtr), &oldPageInfo, &debugData);
        pDebugStorage->CheckDataTail(&debugData, oldPageInfo.UsableSize);
    }
#ifdef GHEAP_MEMSET_ALL
    if (newSize < oldPageInfo.UsableSize)
    {
        memset((UByte*)oldPtr + newSize, 0xFE, oldPageInfo.UsableSize - newSize);
    }
#endif //GHEAP_MEMSET_ALL
#endif //GHEAP_DEBUG_INFO

    GHeapPageInfoMH newPageInfo;
    void* newPtr = pEngine->ReallocInPage(page, oldPtr, newSize, &newPageInfo);
    if (newPtr == 0)
    {
        GLockSafe::Locker locker(GHeapGlobalRootMH->GetLock());
        newPtr = pEngine->ReallocGeneral(page, oldPtr, newSize, &newPageInfo);
    }

#ifdef GHEAP_DEBUG_INFO
    if (newPtr)
    {
#ifdef GHEAP_MEMSET_ALL
        if (newPtr && newSize > newPageInfo.UsableSize)
        {
            memset((UByte*)newPtr + newPageInfo.UsableSize, 0xCC, newSize - newPageInfo.UsableSize);
        }
#endif //GHEAP_MEMSET_ALL
        if (TrackDebugInfo && newPtr)
        {
            pDebugStorage->RelinkAlloc(&debugData, UPInt(oldPtr), 
                                       UPInt(newPtr), newSize,
                                       &newPageInfo);
        }
    }
#endif //GHEAP_DEBUG_INFO

#ifdef GHEAP_TRACE_ALL
    GHeapGlobalRootMH->OnRealloc(this, oldPtr, newSize, newPtr);
#endif

    return newPtr;
}


//------------------------------------------------------------------------
void* GMemoryHeapMH::reallocMem(GHeapNodeMH* node, void* oldPtr, UPInt newSize)
{
#ifdef GHEAP_DEBUG_INFO
    GHeapPageInfoMH oldPageInfo;
    pEngine->GetPageInfoWithSize(node, oldPtr, &oldPageInfo);
    GHeapDebugStorageMH::DebugDataPtr debugData;
    if (TrackDebugInfo)
    {
        pDebugStorage->UnlinkAlloc(UPInt(oldPtr), &oldPageInfo, &debugData);
        pDebugStorage->CheckDataTail(&debugData, oldPageInfo.UsableSize);
    }
#ifdef GHEAP_MEMSET_ALL
    if (newSize < oldPageInfo.UsableSize)
    {
        memset((UByte*)oldPtr + newSize, 0xFE, oldPageInfo.UsableSize - newSize);
    }
#endif //GHEAP_MEMSET_ALL
#endif //GHEAP_DEBUG_INFO

    GHeapPageInfoMH newPageInfo;
    void* newPtr = pEngine->ReallocInNode(node, oldPtr, newSize, &newPageInfo);
#ifdef GHEAP_DEBUG_INFO
    if (newPtr)
    {
#ifdef GHEAP_MEMSET_ALL
        if (newPtr && newSize > newPageInfo.UsableSize)
        {
            memset((UByte*)newPtr + newPageInfo.UsableSize, 0xCC, newSize - newPageInfo.UsableSize);
        }
#endif //GHEAP_MEMSET_ALL
        if (TrackDebugInfo && newPtr)
        {
            pDebugStorage->RelinkAlloc(&debugData, UPInt(oldPtr), 
                                       UPInt(newPtr), newSize, 
                                       &newPageInfo);
        }
    }
#endif //GHEAP_DEBUG_INFO

#ifdef GHEAP_TRACE_ALL
    GHeapGlobalRootMH->OnRealloc(this, oldPtr, newSize, newPtr);
#endif

    return newPtr;
}


//------------------------------------------------------------------------
void* GMemoryHeapMH::Realloc(void* oldPtr, UPInt newSize)
{
    // Realloc cannot work as Alloc in case oldPtr is 0. It's done in 
    // order not to pass a lot of parameters, such as DebugInfo, alignment, 
    // parent allocation, etc.
    //------------------------
    GASSERT(oldPtr != 0);

    GMemoryHeapMH* heap;
    GHeapPageMH* page = GHeapGlobalRootMH->ResolveAddress(UPInt(oldPtr));
    void* newPtr = 0;
    if (page)
    {
        heap = page->pHeap;
        if (heap->UseLocks)
        {
            GLock::Locker locker(&heap->HeapLock);
            newPtr = heap->reallocMem(page, oldPtr, newSize);
        }
        else
        {
            GMEMORY_CHECK_CURRENT_THREAD(this);
            newPtr = heap->reallocMem(page, oldPtr, newSize);
        }
    }
    else
    {
        GHeapNodeMH* node = 0;
        {
            GLockSafe::Locker locker(GHeapGlobalRootMH->GetLock());
            node = GHeapGlobalRootMH->FindNodeInGlobalTree((UByte*)oldPtr);
            GASSERT(node);
            heap = node->GetHeap();
        }
        if (heap->UseLocks)
        {
            GLock::Locker locker1(&heap->HeapLock);
            GLockSafe::Locker locker2(GHeapGlobalRootMH->GetLock());
            newPtr = heap->reallocMem(node, oldPtr, newSize);
        }
        else
        {
            GLockSafe::Locker locker2(GHeapGlobalRootMH->GetLock());
            newPtr = heap->reallocMem(node, oldPtr, newSize);
        }
    }
    return newPtr;
}

//------------------------------------------------------------------------
GMemoryHeap* GMemoryHeapMH::GetAllocHeap(const void *thisPtr)
{
    GHeapPageMH* page = GHeapGlobalRootMH->ResolveAddress(UPInt(thisPtr));
    if (page) return page->pHeap;
    GLockSafe::Locker locker(GHeapGlobalRootMH->GetLock());
    GHeapNodeMH* node = GHeapGlobalRootMH->FindNodeInGlobalTree((UByte*)thisPtr);
    GASSERT(node);
    return node->GetHeap();
}

//------------------------------------------------------------------------
UPInt GMemoryHeapMH::GetUsableSize(const void* ptr)
{
    GHeapPageMH* page = GHeapGlobalRootMH->ResolveAddress(UPInt(ptr));
    GHeapPageInfoMH pageInfo;
    if (page) 
    {
        page->pHeap->pEngine->GetPageInfoWithSize(page, ptr, &pageInfo);
        return pageInfo.UsableSize;
    }
    GLockSafe::Locker locker(GHeapGlobalRootMH->GetLock());
    GHeapNodeMH* node = GHeapGlobalRootMH->FindNodeInGlobalTree((UByte*)ptr);
    GASSERT(node);
    node->GetHeap()->pEngine->GetPageInfoWithSize(node, ptr, &pageInfo);
    return pageInfo.UsableSize;
}

//------------------------------------------------------------------------
void* GMemoryHeapMH::AllocSysDirect(UPInt size)
{
    GLockSafe::Locker locker(GHeapGlobalRootMH->GetLock());
    return GHeapGlobalRootMH->GetSysAlloc()->Alloc(size, sizeof(void*));
}

void GMemoryHeapMH::FreeSysDirect(void* ptr, UPInt size)
{
    GLockSafe::Locker locker(GHeapGlobalRootMH->GetLock());
    return GHeapGlobalRootMH->GetSysAlloc()->Free(ptr, size, sizeof(void*));
}

//------------------------------------------------------------------------
bool GMemoryHeapMH::GetStats(GStatBag* bag)
{
    GLock::Locker locker(&HeapLock);

    UPInt footprint = pEngine->GetFootprint();
    
    bag->AddStat(GStatHeap_LocalFootprint,   GCounterStat(footprint));
    bag->AddStat(GStatHeap_LocalUsedSpace,   GCounterStat(pEngine->GetUsedSpace()));
    bag->AddStat(GStatHeap_Granularity,      GCounterStat(0));
    bag->AddStat(GStatHeap_Reserve,          GCounterStat(0));

    bag->AddStat(GStatHeap_Bookkeeping,         GCounterStat(0));
#ifdef GHEAP_DEBUG_INFO
    bag->AddStat(GStatHeap_DebugInfo,           GCounterStat(pDebugStorage->GetUsedSpace()));
#endif
    bag->AddStat(GStatHeap_Segments,            GCounterStat(0));
    bag->AddStat(GStatHeap_DynamicGranularity,  GCounterStat(0));
    bag->AddStat(GStatHeap_SysDirectSpace,      GCounterStat(0));

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
}


//------------------------------------------------------------------------
void GMemoryHeapMH::VisitMem(GHeapMemVisitor*, unsigned)
{
}

//------------------------------------------------------------------------
void GMemoryHeapMH::VisitHeapSegments(GHeapSegVisitor*) const
{
}

//------------------------------------------------------------------------
void GMemoryHeapMH::VisitRootSegments(GHeapSegVisitor*)
{
}

//------------------------------------------------------------------------
void GMemoryHeapMH::getUserDebugStats(RootStats* stats) const
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
void GMemoryHeapMH::GetRootStats(RootStats* stats)
{
    stats->UserDebugFootprint = 0;
    stats->UserDebugUsedSpace = 0;
    GMemory::pGlobalHeap->getUserDebugStats(stats);

    stats->SysMemFootprint      = GMemory::pGlobalHeap->GetTotalFootprint() + stats->UserDebugFootprint;
    stats->SysMemUsedSpace      = GMemory::pGlobalHeap->GetTotalUsedSpace() + stats->UserDebugFootprint;
    stats->PageMapFootprint     = 0;
    stats->PageMapUsedSpace     = 0;
    stats->BookkeepingFootprint = 0;
    stats->BookkeepingUsedSpace = 0;
    stats->DebugInfoFootprint   = 0;
    stats->DebugInfoUsedSpace   = 0;
}

//------------------------------------------------------------------------
void GMemoryHeapMH::SetTracer(HeapTracer* tracer) 
{
#ifdef GHEAP_TRACE_ALL
    GHeapGlobalRootMH->pTracer = tracer;
#else
    GUNUSED(tracer);
#endif
}

//------------------------------------------------------------------------
void GMemoryHeapMH::ultimateCheck()
{
}

//------------------------------------------------------------------------
void GMemoryHeapMH::checkIntegrity() const
{
}

//------------------------------------------------------------------------
void GMemoryHeapMH::releaseCachedMem()
{
}
    

//------------------------------------------------------------------------
// Implementation of GSysAllocBase based on MemoryHeapMH.
// Initializes heap system, creating and initializing GlobalHeap.
bool GSysAlloc::initHeapEngine(const void* heapDesc)
{
    GMemoryHeap::InitMH(this);
    const GMemoryHeap::HeapDesc& rootHeapDesc = *(const GMemoryHeap::HeapDesc*)heapDesc;
    GMemoryHeap* p = GMemoryHeap::CreateRootHeapMH(rootHeapDesc);
    return p != 0;
}

void GSysAlloc::shutdownHeapEngine()
{
    GMemoryHeap::ReleaseRootHeapMH();
    GMemoryHeap::CleanUpMH();
}


