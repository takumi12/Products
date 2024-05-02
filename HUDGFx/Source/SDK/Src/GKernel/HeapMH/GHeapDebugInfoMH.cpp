/**********************************************************************

Filename    :   DebugInfo.cpp
Content     :   Debug and statistics implementation.
Created     :   July 14, 2008
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GDebug.h"
#include "GStats.h"
#include "GHeapDebugInfoMH.h"
#include "GHeapAllocEngineMH.h"

#ifdef GHEAP_DEBUG_INFO

//------------------------------------------------------------------------
GHeapDebugStorageMH::GHeapDebugStorageMH(GSysAlloc* alloc, GLockSafe* rootLocker) : 
    pAllocator(alloc), 
    pRootLocker(rootLocker)
{}


//------------------------------------------------------------------------
bool GHeapDebugStorageMH::allocDataPool()
{
    void* poolBuf = 0; 
    {
        GLockSafe::Locker rl(pRootLocker);
        poolBuf = pAllocator->Alloc(PoolSize, sizeof(void*));
    }

    if (poolBuf)
    {
        //memset(poolBuf, 0xCC, PoolSize);
        DataPoolType* pool = ::new (poolBuf) DataPoolType;
        DataPools.PushFront(pool);

        UPInt size = pool->GetSize(PoolSize);
        for(UPInt i = 0; i < size; ++i)
        {
            GHeapDebugDataMH* data = pool->GetElement(i);
            data->pDataPool = pool;
            FreeDataList.PushBack(data);
        }
        return true;
    }
    return false;
}


//------------------------------------------------------------------------
void GHeapDebugStorageMH::freeDataPool(DataPoolType* pool)
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
        pAllocator->Free(pool, PoolSize, sizeof(void*));
    }
}

//------------------------------------------------------------------------
GHeapDebugDataMH* GHeapDebugStorageMH::allocDebugData()
{
    if (FreeDataList.IsEmpty())
    {
        if (!allocDataPool())
            return 0;
    }
    GASSERT(!FreeDataList.IsEmpty());

    GHeapDebugDataMH* data = FreeDataList.GetFirst();
    FreeDataList.Remove(data);
    data->Clear();
    UsedDataList.PushBack(data);
    data->pDataPool->UseCount++;
    return data;
}


//------------------------------------------------------------------------
void GHeapDebugStorageMH::freeDebugData(GHeapDebugDataMH* data)
{
    //data->Clear();
    FreeDataList.PushFront(data);
    if (--data->pDataPool->UseCount == 0)
    {
        freeDataPool(data->pDataPool);
    }
}


//------------------------------------------------------------------------
GHeapDebugDataMH* GHeapDebugStorageMH::getDebugData(GHeapPageInfoMH* page)
{
    for (int i = 0; i < 3; ++i)
    {
        if (page->DebugHeaders[i])
        {
            return *(page->DebugHeaders[i]);
        }
    }
    return 0;
}

//------------------------------------------------------------------------
void GHeapDebugStorageMH::setDebugData(GHeapPageInfoMH* page, GHeapDebugDataMH* data)
{
    for (int i = 0; i < 3; ++i)
    {
        if (page->DebugHeaders[i])
        {
            *(page->DebugHeaders[i]) = data;
        }
    }
}


//------------------------------------------------------------------------
void GHeapDebugStorageMH::unlinkDebugData(GHeapPageInfoMH* page, DebugDataPtr* ptr)
{
    ptr->pData->DataCount--;
    if (ptr->pPrev)
    {
        ptr->pPrev->pNextData = ptr->pSelf->pNextData;
    }
    else
    {
        if (ptr->pSelf->pNextData)
        {
            ptr->pSelf->pNextData->DataCount = ptr->pData->DataCount;
        }
        setDebugData(page, ptr->pSelf->pNextData);
    }
}



//------------------------------------------------------------------------
void GHeapDebugStorageMH::linkDebugData(GHeapPageInfoMH* page, GHeapDebugDataMH* data)
{
    GHeapDebugDataMH* node = getDebugData(page);
    if (node == 0)
    {
        data->pNextData = 0;
        node = data;
    }

    if (data != node)
    {
        data->pNextData = (GHeapDebugDataMH*)node;
        data->DataCount = node->DataCount;
    }
    setDebugData(page, data);
}



//------------------------------------------------------------------------
void GHeapDebugStorageMH::findInChainWithin(GHeapDebugDataMH* chain, UPInt addr, DebugDataPtr* ptr)
{
    GHeapDebugDataMH* prev = 0;
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
void GHeapDebugStorageMH::findInChainExact(GHeapDebugDataMH* chain, UPInt addr, DebugDataPtr* ptr)
{
    GHeapDebugDataMH* prev = 0;
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
void GHeapDebugStorageMH::findDebugData(GHeapPageInfoMH* page, UPInt addr, DebugDataPtr* ptr)
{
    GHeapDebugDataMH* node = getDebugData(page);
    ptr->pData = node;
    findInChainWithin(node, addr, ptr);
}


//------------------------------------------------------------------------
void GHeapDebugStorageMH::fillDataTail(GHeapDebugDataMH* data, UPInt usable)
{
    UByte* p = (UByte*)data->Address;
    for (UPInt i = data->Size; i < usable; ++i)
    {
        p[i] = UByte(129 + i);
    }
}

//------------------------------------------------------------------------
void GHeapDebugStorageMH::CheckDataTail(const DebugDataPtr* ptr, UPInt usable)
{
    const UByte* p = (const UByte*)ptr->pSelf->Address;
    for (UPInt i = ptr->pSelf->Size; i < usable; ++i)
    {
        if (p[i] != UByte(129 + i))
        {
            reportViolation(ptr->pSelf, " Corruption");
            GASSERT(0);
        }
    }
}

//------------------------------------------------------------------------
unsigned GHeapDebugStorageMH::GetStatId(GHeapPageInfoMH* page, UPInt parentAddr, const GAllocDebugInfo* info)
{
    GAllocDebugInfo i2 = info ? *info : GAllocDebugInfo();
    if (i2.StatId != GStat_Default_Mem)
    {
        return i2.StatId;
    }
    DebugDataPtr parent;
    findDebugData(page, parentAddr, &parent);
    if (parent.pSelf == 0)
    {
        return GStat_Default_Mem;
    }
    return parent.pSelf->Info.StatId;
}


//------------------------------------------------------------------------
bool GHeapDebugStorageMH::AddAlloc(UPInt thisAddr, UPInt size, GHeapPageInfoMH* page, const GAllocDebugInfo* info)
{
    GASSERT(thisAddr);

    GHeapDebugDataMH* data = allocDebugData();
    if (data)
    {
        data->RefCount  = 1;
        data->Address   = thisAddr;
        data->Size      = size;
        data->Info      = info ? *info : GAllocDebugInfo();

        linkDebugData(page, data);
        fillDataTail(data, page->UsableSize);
        return true;
    }
    return false;
}


//------------------------------------------------------------------------
bool GHeapDebugStorageMH::AddAlloc(GHeapPageInfoMH* parentInfo, UPInt parentAddr, UPInt thisAddr, 
                                   UPInt size, GHeapPageInfoMH* page, const GAllocDebugInfo* info)
{
    GASSERT(thisAddr);

    GHeapDebugDataMH* data = allocDebugData();
    if (data)
    {
        data->RefCount  = 1;
        data->Address   = thisAddr;
        data->Size      = size;
        data->Info      = info ? *info : GAllocDebugInfo();

        linkDebugData(page, data);
        fillDataTail(data, page->UsableSize);

//if (usable > size)      // DBG Simulate memory corruption
//    ((char*)data->Address)[size] = 0;

        DebugDataPtr parent;
        findDebugData(parentInfo, parentAddr, &parent);
        GASSERT(parent.pSelf != 0);
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
        // The condition "DebugNode::ChainLimit+1" means that 
        // we allow one more element in the chain to be able to detect 
        // whether or not the chain was interrupted: 
        // Interruption has occurred if ChainCount > ChainLimit.
        //-----------------------------
        if (parent.pSelf->ChainCount > GHeapDebugDataMH::ChainLimit+1)
        {
            parent.pSelf = parent.pSelf->pParent;
            GASSERT(parent.pSelf);
        }
        data->pParent    = parent.pSelf;
        data->ChainCount = parent.pSelf->ChainCount + 1;
        parent.pSelf->RefCount++;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------
void GHeapDebugStorageMH::RemoveAlloc(UPInt addr, GHeapPageInfoMH* page)
{
    GASSERT(addr);
    DebugDataPtr ptr;
    GetDebugData(addr, page, &ptr);
    CheckDataTail(&ptr, page->UsableSize);
    GASSERT(ptr.pSelf->Address == addr);  // Must be exact address.

    UsedDataList.Remove(ptr.pSelf);
    unlinkDebugData(page, &ptr);

    GHeapDebugDataMH* data = ptr.pSelf;
    while (data && --data->RefCount == 0)
    {
        GHeapDebugDataMH* parent = data->pParent;
        freeDebugData(data);
        data = parent;
    }
}

//------------------------------------------------------------------------
void GHeapDebugStorageMH::UnlinkAlloc(UPInt addr, GHeapPageInfoMH* page, DebugDataPtr* ptr)
{
    GetDebugData(addr, page, ptr);
    unlinkDebugData(page, ptr);
}

//------------------------------------------------------------------------
void GHeapDebugStorageMH::RelinkAlloc(DebugDataPtr* ptr, UPInt oldAddr, 
                                      UPInt newAddr, UPInt newSize,
                                      GHeapPageInfoMH* newInfo)
{
    GASSERT(ptr->pSelf && oldAddr && newAddr);
    
    ptr->pSelf->Size = newSize;
    ptr->pSelf->Address   = newAddr;
    ptr->pSelf->DataCount = 0;
    linkDebugData(newInfo, ptr->pSelf);
    fillDataTail(ptr->pSelf, newInfo->UsableSize);

#ifndef GFC_BUILD_DEBUG
    GUNUSED(oldAddr);
#endif
}



//------------------------------------------------------------------------
void GHeapDebugStorageMH::GetDebugData(UPInt addr, GHeapPageInfoMH* page, DebugDataPtr* ptr)
{
    findDebugData(page, addr, ptr);
    GASSERT(ptr->pSelf);
}

//------------------------------------------------------------------------
void GHeapDebugStorageMH::FreeAll()
{
    while (!DataPools.IsEmpty())
    {
        DataPoolType* pool = DataPools.GetFirst();
        DataPools.Remove(pool);
        pAllocator->Free(pool, PoolSize, 0);
    }

    UsedDataList.Clear();
    FreeDataList.Clear();
}

//------------------------------------------------------------------------
void GHeapDebugStorageMH::GetStats(GHeapAllocEngineMH* allocator, GStatBag* bag) const
{
#ifndef GFC_NO_STAT
    // Must be locked in MemoryHeap.
    const GHeapDebugDataMH* data = UsedDataList.GetFirst();
    while (!UsedDataList.IsNull(data))
    {
        bag->IncrementMemoryStat(data->Info.StatId,
                                 data->Size,
                                 allocator->GetUsableSize((void*)data->Address));
        data = (const GHeapDebugDataMH*)data->pNext;
    }
#else
    GUNUSED2(allocator, bag);
#endif
}

//------------------------------------------------------------------------
const GHeapDebugDataMH* GHeapDebugStorageMH::GetFirstEntry() const
{
    return UsedDataList.IsEmpty() ? 0 : UsedDataList.GetFirst();
}

//------------------------------------------------------------------------
const GHeapDebugDataMH* GHeapDebugStorageMH::GetNextEntry(const GHeapDebugDataMH* entry) const
{
    const GHeapDebugDataMH* next = (const GHeapDebugDataMH*)entry->pNext;
    return UsedDataList.IsNull(next) ? 0 : next;
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
void GHeapDebugStorageMH::reportViolation(GHeapDebugDataMH* data, const char* msg)
{
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
    
    const GHeapDebugDataMH* parent = data->pParent;

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

#ifndef GFC_BUILD_DEBUG
    GUNUSED(msg);
#endif
}

//------------------------------------------------------------------------
bool GHeapDebugStorageMH::DumpMemoryLeaks(const char* heapName)
{
    bool ret = false;
    UPInt leakedBytes = 0;
    UPInt leakedAllocs = 0;

#ifndef GFC_BUILD_DEBUG
    GUNUSED(heapName);
#endif

    GHeapDebugDataMH* data = UsedDataList.GetFirst();
    GFC_DEBUG_ERROR1(!UsedDataList.IsNull(data), 
                    "Memory leaks detected in heap '%s'!", heapName);
    GUNUSED(heapName);

    while (!UsedDataList.IsNull(data))
    {
        ret = true;
        reportViolation(data, " Leak");
        leakedBytes += data->Size;
        leakedAllocs++;
        data = (GHeapDebugDataMH*)data->pNext;
    }
    GFC_DEBUG_ERROR2(ret, "Total memory leaked: %d bytes in %d allocations", 
                    leakedBytes, 
                    leakedAllocs);
    return ret;
}


//------------------------------------------------------------------------
void GHeapDebugStorageMH::UltimateCheck()
{
}


//------------------------------------------------------------------------
UPInt GHeapDebugStorageMH::GetUsedSpace() const
{
    UPInt usedSpace = 0;
    const DataPoolType* data = DataPools.GetFirst();
    while(!DataPools.IsNull(data))
    {
        usedSpace += PoolSize;
        data = data->pNext;
    }
    return usedSpace;
}


#endif
