/**********************************************************************

Filename    :   HeapRoot.cpp
Content     :   Heap root used for bootstrapping and bookkeeping.
            :   
Created     :   2009
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GDebug.h"
#include "GMemoryHeapMH.h"
#include "GHeapRootMH.h"
#include "GHeapAllocEngineMH.h"
#include "GHeapDebugInfoMH.h"


GHeapRootMH*  GHeapGlobalRootMH = 0;
GPageTableMH  GHeapGlobalPageTableMH;
GHeapPageMH   GHeapGlobalEmptyPageMH;

//------------------------------------------------------------------------
#ifdef GHEAP_TRACE_ALL
GMemoryHeap::HeapTracer* GHeapRootMH::pTracer = 0;
#endif

//------------------------------------------------------------------------
void GHeapGetMagicHeaders(UPInt pageStart, GHeapMagicHeadersInfo* headers)
{
    headers->Header1 = 0;
    headers->Header2 = 0;
    UPInt start = (pageStart + GHeapPageMH::UnitMask) & ~UPInt(GHeapPageMH::UnitMask);
    UPInt end   = (pageStart + GHeapPageMH::PageSize) & ~UPInt(GHeapPageMH::UnitMask);
    UPInt bound = (pageStart + GHeapPageMH::PageMask) & ~UPInt(GHeapPageMH::PageMask);
    if (bound-start > GHeapPageMH::UnitSize)
    {
        headers->Header1 = (GHeapMagicHeader*)(bound - sizeof(GHeapMagicHeader));
    }
    if (end-bound > GHeapPageMH::UnitSize)
    {
        headers->Header2 = (GHeapMagicHeader*)bound;
    }
    headers->BitSet = (UInt32*)((bound-start > end-bound)? 
                                (bound - sizeof(GHeapMagicHeader) - GHeapPageMH::BitSetBytes): 
                                (bound + sizeof(GHeapMagicHeader)));
    headers->AlignedStart = (UByte*)start;
    headers->AlignedEnd   = (UByte*)end;
    headers->Bound = (UByte*)bound;
    headers->Page  = 0;
}


//------------------------------------------------------------------------
GHeapRootMH::GHeapRootMH(GSysAlloc* sysAlloc) :
    pSysAlloc(sysAlloc),
//#ifdef SF_MEMORY_ENABLE_DEBUG_INFO
//    DebugAlloc(&AllocWrapper, 
//               256,
//               Heap_DebugAllocGranularity, 
//               Heap_DebugAllocHeaderPageSize),
//#endif
    RootLock(),
    FreePages(),
    TableCount(0)
{
    GHeapGlobalEmptyPageMH.pHeap  = 0;
    GHeapGlobalEmptyPageMH.Start = 0; // Theoretically there must be a 4K static block,
                                 // but 0...4K is never available on any system

    for(int i = 0; i < GPageTableMH::TableSize; ++i)
    {
        GHeapGlobalPageTableMH.Entries[i].FirstPage = &GHeapGlobalEmptyPageMH;
        GHeapGlobalPageTableMH.Entries[i].SizeMask  = 0;
    }
    GHeapGlobalRootMH = this;
}

//------------------------------------------------------------------------
GHeapRootMH::~GHeapRootMH()
{
    FreeTables();
    GHeapGlobalRootMH = 0;
}

//------------------------------------------------------------------------
void GHeapRootMH::FreeTables()
{
    for(int i = 0; i < GPageTableMH::TableSize; ++i)
    {
        GHeapPageMH* pool = GHeapGlobalPageTableMH.Entries[i].FirstPage;
        if (pool != &GHeapGlobalEmptyPageMH)
        {
// Took out ASSERT to avoid it happening on shutdown after memory leak report.
//#ifdef GFC_BUILD_DEBUG
//            UPInt size = GHeapGlobalPageTableMH.Entries[i].SizeMask;
//            GASSERT(size > 1);
//            for (UPInt j = 0; j < size; ++j)
//            {
//                // This assert indicates that the respective PageMH is still use.
//                // Expect memory leaks.
//                //-----------------------
//                //GASSERT(pool[j].pHeap == 0 && pool[j].Start == 0);
//            }
//#endif
            pSysAlloc->Free(GHeapGlobalPageTableMH.Entries[i].FirstPage, 
                           (GHeapGlobalPageTableMH.Entries[i].SizeMask+1) * sizeof(GHeapPageMH),
                            sizeof(void*));
        }
        GHeapGlobalPageTableMH.Entries[i].FirstPage = &GHeapGlobalEmptyPageMH;
        GHeapGlobalPageTableMH.Entries[i].SizeMask  = 0;
    }
}

//------------------------------------------------------------------------
bool GHeapRootMH::allocPagePool()
{
    if (TableCount >= GPageTableMH::TableSize)
    {
        // TO DO: Report table overflow
        return false;
    }

    UPInt tableSize = 128 << (TableCount / 16); //  0...15: 128
                                                // 16...31: 256
                                                // 32...48: 512 ...etc
    GHeapPageMH* pool = (GHeapPageMH*)pSysAlloc->Alloc(tableSize * sizeof(GHeapPageMH), sizeof(void*));
    if (pool == 0)
        return false;

    GHeapGlobalPageTableMH.Entries[TableCount].FirstPage = pool;
    GHeapGlobalPageTableMH.Entries[TableCount].SizeMask  = tableSize-1;
    for (UPInt i = 0; i < tableSize; ++i)
    {
        pool->pHeap = 0;
        pool->Start = 0;
        FreePages.PushFront(pool);
        ++pool;
    }
    ++TableCount;
    return true;
}

//------------------------------------------------------------------------
GHeapPageMH* GHeapRootMH::AllocPage(GMemoryHeapMH* heap)
{
    if (FreePages.IsEmpty())
        if (!allocPagePool())
            return 0;

    GASSERT(!FreePages.IsEmpty());
    GHeapPageMH* page = FreePages.GetFirst();
    page->Start = (UByte*)pSysAlloc->Alloc(GHeapPageMH::PageSize, sizeof(void*));
    if (page->Start == 0)
    {
        page->pHeap = 0;
        return 0;
    }
    GList<GHeapPageMH>::Remove(page);
    page->pHeap = heap;
    setMagic(page->Start, GHeapMagicHeader::MagicValue);
    return page;
}

//------------------------------------------------------------------------
void GHeapRootMH::FreePage(GHeapPageMH* page)
{
    setMagic(page->Start, 0); // To reduce the number of collisions
    UByte* p = page->Start;
    page->Start = 0;
    page->pHeap = 0;
    pSysAlloc->Free(p, GHeapPageMH::PageSize, sizeof(void*));
    FreePages.PushFront(page);
}

//------------------------------------------------------------------------
UInt32 GHeapRootMH::GetPageIndex(const GHeapPageMH* page) const
{
    for (unsigned i = 0; i < TableCount; ++i)
    {
        const GPageTableMH::Level0Entry& l1Table = GHeapGlobalPageTableMH.Entries[i];
        GASSERT(l1Table.FirstPage != &GHeapGlobalEmptyPageMH);
        UPInt idx1 = page - l1Table.FirstPage;
        if (idx1 <= UPInt(l1Table.SizeMask))
        {
            return UInt32((idx1 << GPageTableMH::Index1Shift) | i);
        }
    }
    return UInt32(~0U);
}

//------------------------------------------------------------------------
void GHeapRootMH::setMagic(UByte* pageStart, unsigned magicValue)
{
    GHeapMagicHeadersInfo headers;
    GHeapGetMagicHeaders(UPInt(pageStart), &headers);
    if (headers.Header1) headers.Header1->Magic = (UInt16)magicValue;
    if (headers.Header2) headers.Header2->Magic = (UInt16)magicValue;
}

//------------------------------------------------------------------------
GHeapPageMH* GHeapRootMH::ResolveAddress(UPInt addr) const
{
    const GHeapMagicHeader* header;

    header = (const GHeapMagicHeader*)(addr & ~UPInt(GHeapPageMH::PageMask));
    if (header->Magic == GHeapMagicHeader::MagicValue)
    {      
        const GPageTableMH::Level0Entry& l0e = GHeapGlobalPageTableMH.Entries[header->GetIndex0()];
        GHeapPageMH* page = l0e.FirstPage + (header->GetIndex1() & l0e.SizeMask);
        if ((addr - UPInt(page->Start)) < GHeapPageMH::PageSize)
        {
            return page;
        }
    }

    // No match on left
    header = (const GHeapMagicHeader*)((addr & ~UPInt(GHeapPageMH::PageMask)) + GHeapPageMH::PageSize - sizeof(GHeapMagicHeader));
    if (header->Magic == GHeapMagicHeader::MagicValue)
    {
        const GPageTableMH::Level0Entry& l0e = GHeapGlobalPageTableMH.Entries[header->GetIndex0()];
        GHeapPageMH* page = l0e.FirstPage + (header->GetIndex1() & l0e.SizeMask);
        if ((addr - UPInt(page->Start)) < GHeapPageMH::PageSize)
        {
            return page;
        }
    }
    return 0;
}


//------------------------------------------------------------------------
GMemoryHeapMH* GHeapRootMH::CreateHeap(const char*  name, 
                                       GMemoryHeapMH* parent,
                                       const GMemoryHeap::HeapDesc& desc)
{
    // Locked by Root::Lock in MemoryHeap::CreateHeap
    if (GetSysAlloc() == 0)
    {
        GFC_DEBUG_MESSAGE(1, "CreateHeap: Memory System is not initialized");
        GASSERT(0);
    }

#ifdef GHEAP_DEBUG_INFO
    UPInt debugStorageSize = sizeof(GHeapDebugStorageMH);
#else
    UPInt debugStorageSize = 0;
#endif

    UPInt selfSize = sizeof(GMemoryHeapMH) + 
                     sizeof(GHeapAllocEngineMH) + 
                     debugStorageSize +
                     strlen(name) + 1;

    selfSize = (selfSize + GHeapPageMH::UnitMask) & ~UPInt(GHeapPageMH::UnitMask);

    UByte* heapBuf = (UByte*)pSysAlloc->Alloc(selfSize, sizeof(void*));
    if (heapBuf == 0)
    {
        return 0;
    }

    GMemoryHeapMH* heap = ::new(heapBuf) GMemoryHeapMH;

    GHeapAllocEngineMH* engine = ::new (heapBuf + sizeof(GMemoryHeapMH))
        GHeapAllocEngineMH(pSysAlloc,
                           heap,
                           desc.MinAlign, 
                           desc.Limit);

    if (!engine->IsValid())
    {
        pSysAlloc->Free(heapBuf, selfSize, sizeof(void*));
        return 0;
    }

    heap->SelfSize          = selfSize;
    heap->RefCount          = 1;
    heap->pAutoRelease      = 0;
    heap->Info.Desc         = desc;
    heap->Info.pParent      = parent;
    heap->Info.pName        = (char*)heapBuf + 
                                     sizeof(GMemoryHeapMH) + 
                                     sizeof(GHeapAllocEngineMH) + 
                                     debugStorageSize;
    heap->UseLocks          = (desc.Flags & GMemoryHeap::Heap_ThreadUnsafe) == 0;
    heap->TrackDebugInfo    = (desc.Flags & GMemoryHeap::Heap_NoDebugInfo)  == 0;
    heap->pEngine           = engine;

#ifdef GHEAP_DEBUG_INFO
    heap->pDebugStorage = ::new(heapBuf + sizeof(GMemoryHeapMH) + sizeof(GHeapAllocEngineMH))
        GHeapDebugStorageMH(GHeapGlobalRootMH->GetSysAlloc(), GHeapGlobalRootMH->GetLock());
#endif
    memcpy(heap->Info.pName, name, strlen(name) + 1);

#ifdef GHEAP_TRACE_ALL
    pTracer->OnCreateHeap(heap);
#endif

    return heap;
}

//------------------------------------------------------------------------
void GHeapRootMH::DestroyHeap(GMemoryHeapMH* heap)
{
    // Locked by Root::Lock in MemoryHeap::Release
#ifdef GHEAP_TRACE_ALL
    pTracer->OnDestroyHeap(heap);
#endif

    UPInt heapSize = heap->SelfSize;
#ifdef GHEAP_DEBUG_INFO
    heap->pDebugStorage->FreeAll();
#endif
    heap->pEngine->FreeAll();
    heap->~GMemoryHeapMH();
    pSysAlloc->Free(heap, heapSize, sizeof(void*));
}


//------------------------------------------------------------------------
#ifdef GHEAP_TRACE_ALL
void GHeapRootMH::OnAlloc(const GMemoryHeapMH* heap, UPInt size, UPInt align, unsigned sid, const void* ptr)
{
    GLock::Locker lock(&RootLock);
    pTracer->OnAlloc(heap, size, align, sid, ptr);
}

void GHeapRoot::OnRealloc(const GMemoryHeapMH* heap, const void* oldPtr, UPInt newSize, const void* newPtr)
{
    GLock::Locker lock(&RootLock);
    pTracer->OnRealloc(heap, oldPtr, newSize, newPtr);
}

void GHeapRoot::OnFree(const GMemoryHeapMH* heap, const void* ptr)
{
    GLock::Locker lock(&RootLock);
    pTracer->OnFree(heap, ptr);
}
#endif



