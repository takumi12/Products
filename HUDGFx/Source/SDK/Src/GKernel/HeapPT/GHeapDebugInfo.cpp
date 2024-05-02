/**********************************************************************

Filename    :   GHeapDebugInfo.cpp
Content     :   Debug and statistics implementation.
Created     :   July 14, 2008
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GHeapDebugInfo.h"
#include "GHeapAllocEngine.h"
#include "GStats.h"

#ifdef GHEAP_DEBUG_INFO

//------------------------------------------------------------------------
GHeapDebugStorage::GHeapDebugStorage(GHeapGranulator* alloc, GLockSafe* rootLocker) : 
    pAllocator(alloc), 
    pRootLocker(rootLocker)
{}


//------------------------------------------------------------------------
bool GHeapDebugStorage::allocDataPool()
{
    void* poolBuf = 0; 
    {
        GLockSafe::Locker rl(pRootLocker);
        poolBuf = pAllocator->Alloc(PoolSize, 0);
    }

    if (poolBuf)
    {
        //memset(poolBuf, 0xCC, PoolSize);
        DebugDataPool* pool = ::new (poolBuf) DebugDataPool;
        DataPools.PushFront(pool);

        UPInt size = pool->GetSize(PoolSize);
        for(UPInt i = 0; i < size; ++i)
        {
            GHeapDebugData* data = pool->GetElement(i);
            data->pDataPool = pool;
            FreeDataList.PushBack(data);
        }
        return true;
    }
    return false;
}


//------------------------------------------------------------------------
void GHeapDebugStorage::freeDataPool(DebugDataPool* pool)
{
    UPInt size = pool->GetSize(PoolSize);
    for(UPInt i = 0; i < size; ++i)
    {
        FreeDataList.Remove(pool->GetElement(i));
    }
    DataPools.Remove(pool);
    {
        GLockSafe::Locker rl(pRootLocker);
        //memset(pool, 0xFE, PoolSize);
        pAllocator->Free(pool, PoolSize, 0);
    }
}


//------------------------------------------------------------------------
bool GHeapDebugStorage::allocPagePool()
{
    void* poolBuf = 0; 
    {
        GLockSafe::Locker rl(pRootLocker);
        poolBuf = pAllocator->Alloc(PoolSize, 0);
    }

    if (poolBuf)
    {
        //memset(poolBuf, 0xCC, PoolSize);
        DebugPagePool* pool = ::new (poolBuf) DebugPagePool;
        PagePools.PushFront(pool);

        UPInt size = pool->GetSize(PoolSize);
        for(UPInt i = 0; i < size; ++i)
        {
            GHeapDebugPage* page = pool->GetElement(i);
            page->pDataPool = pool;
            FreePageList.PushBack(page);
        }
        return true;
    }
    return false;
}


//------------------------------------------------------------------------
void GHeapDebugStorage::freePagePool(DebugPagePool* pool)
{
    UPInt size = pool->GetSize(PoolSize);
    for(UPInt i = 0; i < size; ++i)
    {
        FreePageList.Remove(pool->GetElement(i));
    }
    PagePools.Remove(pool);
    {
        GLockSafe::Locker rl(pRootLocker);
        //memset(pool, 0xFE, PoolSize);
        pAllocator->Free(pool, PoolSize, 0);
    }
}




//------------------------------------------------------------------------
GHeapDebugPage* GHeapDebugStorage::allocDebugPage()
{
    if (FreePageList.IsEmpty())
    {
        if (!allocPagePool())
            return 0;
    }
    GHEAP_ASSERT(!FreePageList.IsEmpty());

    GHeapDebugPage* page = FreePageList.GetFirst();
    FreePageList.Remove(page);
    page->Clear();
    page->pDataPool->UseCount++;

    return page;
}


//------------------------------------------------------------------------
void GHeapDebugStorage::freeDebugPage(GHeapDebugPage* page)
{
    //page->Clear();
    FreePageList.PushFront(page);
    if (--page->pDataPool->UseCount == 0)
    {
        freePagePool(page->pDataPool);
    }
}


//------------------------------------------------------------------------
GHeapDebugData* GHeapDebugStorage::allocDebugData()
{
    if (FreeDataList.IsEmpty())
    {
        if (!allocDataPool())
            return 0;
    }
    GHEAP_ASSERT(!FreeDataList.IsEmpty());

    GHeapDebugData* data = FreeDataList.GetFirst();
    FreeDataList.Remove(data);
    data->Clear();
    UsedDataList.PushBack(data);
    data->pDataPool->UseCount++;
    return data;
}


//------------------------------------------------------------------------
void GHeapDebugStorage::freeDebugData(GHeapDebugData* data)
{
    //data->Clear();
    FreeDataList.PushFront(data);
    if (--data->pDataPool->UseCount == 0)
    {
        freeDataPool(data->pDataPool);
    }
}




//------------------------------------------------------------------------
void GHeapDebugStorage::linkToPage(GHeapDebugPage* page, GHeapDebugData* data)
{
    UPInt idx = (data->Address & GHeap_PageMask) >> AddrShift;
    GHEAP_ASSERT(idx < GHeapDebugPage::PageSize);
    data->pNextData = page->pData[idx];
    page->pData[idx] = data;
    page->Mask |= UPInt(1) << idx;
    page->DataCount++;
}


//------------------------------------------------------------------------
bool GHeapDebugStorage::convertToPage(GHeapDebugData* chain, 
                                      GHeapDebugData* newData)
{
    GHeapDebugPage* page = allocDebugPage();
    if (page == 0)
    {
        return false;
    }

    GHeapDebugData* node = chain;
    while (node)
    {
        GHeapDebugData* next = node->pNextData;
        linkToPage(page, node);
        node = next;
    }
    linkToPage(page, newData);
    GHeapGlobalPageTable->SetDebugNode(newData->Address, page);
    return true;
}

//------------------------------------------------------------------------
void GHeapDebugStorage::convertToChain(GHeapDebugPage* page, GHeapDebugData* oldData)
{
    GHeapDebugData* chain = 0;
    UPInt mask  = page->Mask;
    UPInt idx   = 0;
    UPInt count = 0;
    while(mask)
    {
        UPInt shift = GHeapGetLowerBit(mask);
        idx += shift;
        GHeapDebugData* data = page->pData[idx];
        while(data)
        {
            GHeapDebugData* next = data->pNextData;
            if (data != oldData)
            {
                data->pNextData = chain;
                chain = data;
                ++count;
            }
            data = next;
        }
        ++idx;
        mask >>= shift;
        mask >>= 1; // DON'T DO "mask >>= shift+1"
    }
    GHEAP_ASSERT(chain != 0);
    chain->DataCount = UPIntHalf(count);
    GHeapGlobalPageTable->SetDebugNode(oldData->Address, chain);
    freeDebugPage(page);
}

//------------------------------------------------------------------------
void GHeapDebugStorage::unlinkDebugData(DebugDataPtr* ptr)
{
    if (ptr->pNode->IsPage())
    {
        GHeapDebugPage* page = (GHeapDebugPage*)ptr->pNode;
        if (page->Decrement() <= PageLowerLimit)
        {
            convertToChain(page, ptr->pSelf);
        }
        else
        {
            if (ptr->pPrev)
            {
                ptr->pPrev->pNextData = ptr->pSelf->pNextData;
            }
            else
            {
                page->pData[ptr->Index] = ptr->pSelf->pNextData;
                if (page->pData[ptr->Index] == 0)
                {
                    page->Mask &= ~(UPInt(1) << ptr->Index);
                    if (page->Mask == 0)
                    {
                        GHeapGlobalPageTable->SetDebugNode(ptr->pSelf->Address, 0);
                        freeDebugPage(page);
                    }
                }
            }
        }
    }
    else
    {
        ptr->pNode->DataCount--;
        if (ptr->pPrev)
        {
            ptr->pPrev->pNextData = ptr->pSelf->pNextData;
        }
        else
        {
            if (ptr->pSelf->pNextData)
            {
                ptr->pSelf->pNextData->DataCount = ptr->pNode->DataCount;
            }
#ifdef GHEAP_FORCE_SYSALLOC
            GHEAP_ASSERT(ptr->pSelf->pNextData == 0);
#endif
            GHeapGlobalPageTable->SetDebugNode(ptr->pSelf->Address, ptr->pSelf->pNextData);
        }
    }
}

//------------------------------------------------------------------------
void GHeapDebugStorage::linkDebugData(GHeapDebugData* data)
{
    GHeapDebugNode* node = GHeapGlobalPageTable->GetDebugNode(data->Address);

#ifdef GHEAP_FORCE_SYSALLOC
    GHEAP_ASSERT(node == 0);
#endif

    if (node == 0)
    {
        data->pNextData = 0;
        node = data;
    }

    if (node->IsPage())
    {
        linkToPage((GHeapDebugPage*)node, data);
    }
    else
    {
        // The following logic guarantees successful linking.
        // Even if it fails to convert the data chain to a page,
        // it continues the chain. This guarantee is necessary
        // for realloc. If realloc worked successfully, we must 
        // guarantee successful relink.
        //-------------------------
        if (node->Increment() >= PageUpperLimit && 
            convertToPage((GHeapDebugData*)node, data))
        {
            return;
        }
        if (data != node)
        {
            data->pNextData = (GHeapDebugData*)node;
            data->DataCount = node->DataCount;
        }
        GHeapGlobalPageTable->SetDebugNode(data->Address, data);
    }
}



//------------------------------------------------------------------------
void GHeapDebugStorage::findInChainWithin(GHeapDebugData* chain, UPInt addr, 
                                          DebugDataPtr* ptr)
{
    GHeapDebugData* prev = 0;
    while (chain)
    {
        UPInt end = chain->Address + chain->Size;
        if (addr >= chain->Address && addr < end)
        {
            ptr->pSelf = chain;
            ptr->pPrev = prev;
            return;
        }
        prev  = chain;
        chain = chain->pNextData;
    }
}


//------------------------------------------------------------------------
void GHeapDebugStorage::findInChainExact(GHeapDebugData* chain, UPInt addr, 
                                         DebugDataPtr* ptr)
{
    GHeapDebugData* prev = 0;
    while (chain)
    {
        if (addr == chain->Address)
        {
            ptr->pSelf = chain;
            ptr->pPrev = prev;
            return;
        }
        prev  = chain;
        chain = chain->pNextData;
    }
}


//------------------------------------------------------------------------
void GHeapDebugStorage::findInNodeWithin(GHeapDebugNode* node, 
                                         UPInt idx, UPInt addr, 
                                         DebugDataPtr* ptr)
{
    if (node->IsPage())
    {
        GHeapDebugPage* page = (GHeapDebugPage*)node;
        UPInt mask = (((UPInt(1) << idx) - 1) << 1) | 1;
        idx = GHeapGetUpperBit(page->Mask & mask);
        ptr->pNode = page;
        ptr->Index = idx;
        findInChainWithin(page->pData[idx], addr, ptr);
    }
    else
    {
        findInChainWithin((GHeapDebugData*)node, addr, ptr);
    }
}

//------------------------------------------------------------------------
void GHeapDebugStorage::findDebugData(UPInt addr, bool autoHeap, DebugDataPtr* ptr)
{
    GHeapDebugNode* node = GHeapGlobalPageTable->FindDebugNode(addr);

    GHEAP_ASSERT(!(autoHeap && node == 0));

    UPInt idx = (addr & GHeap_PageMask) >> AddrShift;
    GHEAP_ASSERT(idx < GHeapDebugPage::PageSize);

    findInNodeWithin(node, idx, addr, ptr);
    if (ptr->pSelf)
    {
        return;
    }

    if (idx)
    {
        findInNodeWithin(node, idx-1, addr, ptr);
        if (ptr->pSelf)
        {
            return;
        }
    }

    node = GHeapGlobalPageTable->FindDebugNode(addr-GHeap_PageSize);
    if (!autoHeap && node == 0)
    {
        return;
    }
    GHEAP_ASSERT(node != 0);
    findInNodeWithin(node, GHeapDebugPage::PageSize-1, addr, ptr);
    GHEAP_ASSERT(!(autoHeap && ptr->pSelf == 0));
}


//------------------------------------------------------------------------
void GHeapDebugStorage::fillDataTail(GHeapDebugData* data, UPInt usable)
{
    UByte* p = (UByte*)data->Address;
    for (UPInt i = data->Size; i < usable; ++i)
    {
        p[i] = UByte(129 + i);
    }
}

//------------------------------------------------------------------------
void GHeapDebugStorage::CheckDataTail(const DebugDataPtr* ptr, UPInt usable)
{
    const UByte* p = (const UByte*)ptr->pSelf->Address;
    for (UPInt i = ptr->pSelf->Size; i < usable; ++i)
    {
        if (p[i] != UByte(129 + i))
        {
            reportViolation(ptr->pSelf, " Corruption");
            GHEAP_ASSERT(0);
        }
    }
}

//------------------------------------------------------------------------
UInt GHeapDebugStorage::GetStatId(UPInt parentAddr, const GAllocDebugInfo* info)
{
    GAllocDebugInfo i2 = info ? *info : GAllocDebugInfo();
    if (i2.StatId != GStat_Default_Mem)
    {
        return i2.StatId;
    }
    DebugDataPtr parent;
    findDebugData(parentAddr, true, &parent);
    if (parent.pSelf == 0)
    {
        return GStat_Default_Mem;
    }
    return parent.pSelf->Info.StatId;
}

//------------------------------------------------------------------------
bool GHeapDebugStorage::AddAlloc(UPInt parentAddr, bool autoHeap, UPInt thisAddr, 
                                 UPInt size, UPInt usable, const GAllocDebugInfo* info)
{
    GHEAP_ASSERT(thisAddr);

    GHeapDebugData* data = allocDebugData();
    if (data)
    {
        data->RefCount  = 1;
        data->Address   = thisAddr;
        data->Size      = size;
        data->Info      = info ? *info : GAllocDebugInfo();

        linkDebugData(data);
        fillDataTail(data, usable);

//if (usable > size)      // DBG Simulate memory corruption
//    ((char*)data->Address)[size] = 0;

        if (parentAddr)
        {
            DebugDataPtr parent;
            findDebugData(parentAddr, autoHeap, &parent);
            if (!autoHeap && parent.pSelf == 0)
            {
                return true;
            }
            GHEAP_ASSERT(parent.pSelf != 0);
            if (data->Info.StatId == GStat_Default_Mem)
            {
                data->Info.StatId = parent.pSelf->Info.StatId;
            }

            // Prevent from creating too long parent chains in 
            // the following scenario:
            //
            //    p1 = heap.Alloc(. . .);
            //    for(. . .)
            //    {
            //        p2 = AllocAutoHeap(p1, . . .);
            //        Free(p1);
            //        p1 = p2;
            //    }
            //
            // When the chain gets longer than ChainLimit, just 
            // connect it to the grandparent, interrupting the chain. 
            // The condition "GHeapDebugNode::ChainLimit+1" means that 
            // we allow one more element in the chain to be able to detect 
            // whether or not the chain was interrupted: 
            // Interruption has occurred if ChainCount > ChainLimit.
            //-----------------------------
            if (parent.pSelf->ChainCount > GHeapDebugNode::ChainLimit+1)
            {
                parent.pSelf = parent.pSelf->pParent;
                GHEAP_ASSERT(parent.pSelf);
            }
            data->pParent    = parent.pSelf;
            data->ChainCount = parent.pSelf->ChainCount + 1;
            parent.pSelf->RefCount++;
        }
        return true;
    }
    return false;
}

//------------------------------------------------------------------------
void GHeapDebugStorage::RelinkAlloc(DebugDataPtr* ptr, UPInt oldAddr, 
                                    UPInt newAddr, UPInt newSize, UPInt usable)
{
    GUNUSED(oldAddr);
    GHEAP_ASSERT(ptr->pSelf && oldAddr && newAddr);
    ptr->pSelf->Size = newSize;
    ptr->pSelf->Address   = newAddr;
    ptr->pSelf->DataCount = 0;
    linkDebugData(ptr->pSelf);
    fillDataTail(ptr->pSelf, usable);
}


//------------------------------------------------------------------------
void GHeapDebugStorage::UnlinkAlloc(UPInt addr, DebugDataPtr* ptr)
{
    GetDebugData(addr, ptr);
    unlinkDebugData(ptr);
}


//------------------------------------------------------------------------
void GHeapDebugStorage::GetDebugData(UPInt addr, DebugDataPtr* ptr)
{
    GHeapDebugNode* node = GHeapGlobalPageTable->GetDebugNode(addr);
//if(node == 0)
//{
//printf("0 Addr=%08x\n", addr); // DBG
//}
    GHEAP_ASSERT(node);
    ptr->pNode = node;

#ifdef GHEAP_FORCE_SYSALLOC
    GHeapDebugData* data = (GHeapDebugData*)node;
    GHEAP_ASSERT(data->pNextData == 0 && data->Address == addr);
    ptr->pSelf = data;
    ptr->pPrev = 0;
#else
    if (node->IsPage())
    {
        GHeapDebugPage* page = (GHeapDebugPage*)node;
        if (page->pData)
        {
            ptr->Index = (addr & GHeap_PageMask) >> AddrShift;
            findInChainExact(page->pData[ptr->Index], addr, ptr);
        }
    }
    else
    {
        findInChainExact((GHeapDebugData*)node, addr, ptr);
    }
#endif
    GHEAP_ASSERT(ptr->pSelf);
}

//------------------------------------------------------------------------
void GHeapDebugStorage::RemoveAlloc(UPInt addr, UPInt usable)
{
    GHEAP_ASSERT(addr);
    DebugDataPtr ptr;
    GetDebugData(addr, &ptr);
    CheckDataTail(&ptr, usable);
    GHEAP_ASSERT(ptr.pSelf->Address == addr);  // Must be exact address.

    UsedDataList.Remove(ptr.pSelf);
    unlinkDebugData(&ptr);

    GHeapDebugData* data = ptr.pSelf;
    while (data && --data->RefCount == 0)
    {
        GHeapDebugData* parent = data->pParent;
        freeDebugData(data);
        data = parent;
    }
}

//------------------------------------------------------------------------
void GHeapDebugStorage::FreeAll()
{
    const GHeapDebugData* data = UsedDataList.GetFirst();
    while (!UsedDataList.IsNull(data))
    {
        GHeapGlobalPageTable->SetDebugNode(data->Address, 0);
        data = (const GHeapDebugData*)data->pNext;
    }

    while (!DataPools.IsEmpty())
    {
        DebugDataPool* pool = DataPools.GetFirst();
        DataPools.Remove(pool);
        pAllocator->Free(pool, PoolSize, 0);
    }

    while (!PagePools.IsEmpty())
    {
        DebugPagePool* pool = PagePools.GetFirst();
        PagePools.Remove(pool);
        pAllocator->Free(pool, PoolSize, 0);
    }
    UsedDataList.Clear();
    FreeDataList.Clear();
    FreePageList.Clear();
}

//------------------------------------------------------------------------
void GHeapDebugStorage::GetStats(GHeapAllocEngine* allocator, GStatBag* bag) const
{
#ifndef GFC_NO_STAT
    // Must be locked in GMemoryHeap.
    const GHeapDebugData* data = UsedDataList.GetFirst();
    while (!UsedDataList.IsNull(data))
    {
        bag->IncrementMemoryStat(data->Info.StatId,
                                 data->Size,
                                 allocator->GetUsableSize((void*)data->Address));
        data = (const GHeapDebugData*)data->pNext;
    }
#else
    GUNUSED2(allocator, bag);
#endif
}

//------------------------------------------------------------------------
const GHeapDebugData* GHeapDebugStorage::GetFirstEntry() const
{
    return UsedDataList.IsEmpty() ? 0 : UsedDataList.GetFirst();
}


//------------------------------------------------------------------------
const GHeapDebugData* GHeapDebugStorage::GetNextEntry(const GHeapDebugData* entry) const
{
    const GHeapDebugData* next = (const GHeapDebugData*)entry->pNext;
    return UsedDataList.IsNull(next) ? 0 : next;
}


//------------------------------------------------------------------------
void GHeapDebugStorage::VisitMem(GHeapMemVisitor* visitor, UInt flags) const
{
    pAllocator->VisitMem(visitor, 
                         GHeapMemVisitor::Cat_DebugInfo,
                         GHeapMemVisitor::Cat_DebugInfoFree);

    if (flags & GHeapMemVisitor::VF_DebugInfoDetailed)
    {
        GHeapSegment seg; // Fake segment
        seg.SelfSize  = 0;
        seg.SegType   = GHeap_SegmentUndefined;
        seg.Alignment = 0;
        seg.UseCount  = 0;
        seg.pHeap     = 0;
        seg.DataSize  = 0;
        seg.pData     = 0;

        visitMem(DataPools, PoolSize, visitor, GHeapMemVisitor::Cat_DebugDataPool, &seg);
        visitMem(PagePools, PoolSize, visitor, GHeapMemVisitor::Cat_DebugPagePool, &seg);

        visitMem(UsedDataList, sizeof(GHeapDebugData), visitor, GHeapMemVisitor::Cat_DebugDataBusy, &seg);
        visitMem(FreeDataList, sizeof(GHeapDebugData), visitor, GHeapMemVisitor::Cat_DebugDataFree, &seg);
        visitMem(FreePageList, sizeof(GHeapDebugPage), visitor, GHeapMemVisitor::Cat_DebugPageFree, &seg);
    }
}


//------------------------------------------------------------------------
static void MemCpyClean(char* dest, const char* src, int sz)
{
    int i = 0;
    for (; i < sz; i++)
    {
        char c = src[i];
        if (c < 32 || c > 126)
            c = '.';
        dest[i] = c;
    }
    dest[i] = 0;
}


//------------------------------------------------------------------------
void GHeapDebugStorage::reportViolation(GHeapDebugData* data, const char* msg)
{
    GUNUSED(msg);
    char memdump[33];

    MemCpyClean(memdump, 
               (const char*)(data->Address), 
               (data->Size < 32) ? int(data->Size) : 32);

    GFC_DEBUG_MESSAGE6(data->Info.pFileName,
                       "%s(%d) :%s %d bytes @0x%8.8X (contents:'%s')", 
                       data->Info.pFileName, 
                       data->Info.Line,
                       msg,
                       data->Size, 
                       data->Address, 
                       memdump);

    GFC_DEBUG_MESSAGE2(!data->Info.pFileName,
                       "%s in unknown file: %d bytes ",
                       msg,
                       data->Size);

    const GHeapDebugData* parent = data->pParent;

    unsigned int i2 = 1;
    char indentBuf[256];
    while(parent)
    {
        if (i2 > sizeof(indentBuf) - 1)
            i2 = sizeof(indentBuf) - 1;
        memset(indentBuf, '+', i2);
        indentBuf[i2] = 0;

        GFC_DEBUG_MESSAGE6(parent->Info.pFileName,
                           "%s(%d) : %s Parent of alloc @0x%8.8X is (%d bytes @0x%8.8X)",
                           parent->Info.pFileName, 
                           parent->Info.Line,
                           indentBuf,
                           data->Address,
                           parent->Size,
                           parent->Address);

        GFC_DEBUG_MESSAGE4(!parent->Info.pFileName,
                           "%s Unknown file: Parent of alloc @0x%8.8X is (%d bytes @0x%8.8X)", 
                           indentBuf,
                           data->Address,
                           parent->Size,
                           parent->Address);

        parent = parent->pParent;
        ++i2;
    }
}


//------------------------------------------------------------------------
bool GHeapDebugStorage::DumpMemoryLeaks(const char* heapName)
{
    GUNUSED(heapName);

    bool ret = false;
    UPInt leakedBytes = 0;
    UPInt leakedAllocs = 0;

    GHeapDebugData* data = UsedDataList.GetFirst();
    GFC_DEBUG_ERROR1(!UsedDataList.IsNull(data), 
                    "Memory leaks detected in heap '%s'!", heapName);

    while (!UsedDataList.IsNull(data))
    {
        ret = true;
        reportViolation(data, " Leak");
        leakedBytes += data->Size;
        leakedAllocs++;
        data = (GHeapDebugData*)data->pNext;
    }
    GFC_DEBUG_ERROR2(ret, "Total memory leaked: %d bytes in %d allocations", 
                     leakedBytes, 
                     leakedAllocs);
    return ret;
}


//------------------------------------------------------------------------
void GHeapDebugStorage::UltimateCheck()
{
    const GHeapDebugData* data = UsedDataList.GetFirst();
    while (!UsedDataList.IsNull(data))
    {
        DebugDataPtr ptr;
        GetDebugData(data->Address, &ptr);
        data = (const GHeapDebugData*)data->pNext;
    }
}


//------------------------------------------------------------------------
UPInt GHeapDebugStorage::GetUsedSpace() const
{
    UPInt usedSpace = 0;
    const DebugDataPool* data = DataPools.GetFirst();
    while(!DataPools.IsNull(data))
    {
        usedSpace += PoolSize;
        data = data->pNext;
    }
    const DebugPagePool* page = PagePools.GetFirst();
    while(!PagePools.IsNull(page))
    {
        usedSpace += PoolSize;
        page = page->pNext;
    }
    return usedSpace;
}

#else //#ifndef GHEAP_DEBUG_INFO

// just to avoid warning "LNK4221: no public symbols found; archive member will be inaccessible" 
GHeapDebugStorage::GHeapDebugStorage() { }

#endif //#ifndef GHEAP_DEBUG_INFO
