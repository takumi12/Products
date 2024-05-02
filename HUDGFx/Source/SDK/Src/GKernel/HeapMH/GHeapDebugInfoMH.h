/**********************************************************************

Filename    :   DebugInfoMH.h
Content     :   Debug and statistics implementation.
Created     :   July 14, 2008
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GHeapDebugInfoMH_H
#define INC_GHeapDebugInfoMH_H

#include "GHeapTypes.h"

#ifdef GHEAP_DEBUG_INFO

#include "GList.h"
#include "GAtomic.h"
#include "GHeapRootMH.h"

class GHeapAllocEngineMH;

//------------------------------------------------------------------------
template<class T>
struct GHeapDebugDataPoolMH : GListNode<GHeapDebugDataPoolMH<T> >
{
    typedef GHeapDebugDataPoolMH<T>   SelfType;

    GHeapDebugDataPoolMH() : UseCount(0) {}

    static UPInt GetSize(UPInt bytes)
    { 
        return (bytes - sizeof(SelfType)) / sizeof(T); 
    }

    T* GetElement(UPInt i) 
    {
        return (T*)((UByte*)this + sizeof(SelfType) + i*sizeof(T));
    }
    UPInt   UseCount;
};



//------------------------------------------------------------------------
struct GHeapDebugDataMH : GListNode<GHeapDebugDataMH>
{
    enum { ChainLimit = 10 };

    typedef GHeapDebugDataPoolMH<GHeapDebugDataMH> DataPool;

    void Clear()
    {
        DataCount   = 0;
        ChainCount  = 0;
        pParent     = 0;
        pNextData   = 0;
        RefCount    = 0;
        Address     = 0;
        Size        = 0;
        memset(&Info, 0, sizeof(Info));
    }

    void MessUp()
    {
        memset(&DataCount,  0xFE, sizeof(DataCount));
        memset(&ChainCount, 0xFE, sizeof(ChainCount));
        memset(&pParent,    0xFE, sizeof(pParent));
        memset(&pNextData,  0xFE, sizeof(pNextData));
        memset(&RefCount,   0xFE, sizeof(RefCount));
        memset(&Address,    0xFE, sizeof(Address));
        memset(&Size,       0xFE, sizeof(Size));
        memset(&Info,       0xFE, sizeof(Info));
    }


    UPIntHalf       DataCount;
    UPIntHalf       ChainCount;
    DataPool*       pDataPool;
    GHeapDebugDataMH* pParent;
    GHeapDebugDataMH* pNextData;      // Next data entry in chain
    UPInt           RefCount;
    UPInt           Address;
    UPInt           Size;
    GAllocDebugInfo Info;
};




//------------------------------------------------------------------------
class GHeapDebugStorageMH
{
    enum 
    { 
        PoolSize = GHeap_DebugAllocPoolSize,
    };

    typedef GHeapDebugDataPoolMH<GHeapDebugDataMH> DataPoolType;

public:
    struct DebugDataPtr
    {
        DebugDataPtr() : pData(0), Index(~UPInt(0)), pSelf(0), pPrev(0) {}
        GHeapDebugDataMH*  pData;
        UPInt              Index;
        GHeapDebugDataMH*  pSelf;
        GHeapDebugDataMH*  pPrev;
    };

    GHeapDebugStorageMH(GSysAlloc* alloc, GLockSafe* rootLocker);

    unsigned GetStatId(GHeapPageInfoMH* page, UPInt parentAddr, const GAllocDebugInfo* info);

    bool AddAlloc(UPInt addr, UPInt size, GHeapPageInfoMH* pageInfo, const GAllocDebugInfo* info);

    bool AddAlloc(GHeapPageInfoMH* parentInfo, UPInt parentAddr, UPInt thisAddr, 
                  UPInt size, GHeapPageInfoMH* pageInfo, const GAllocDebugInfo* info);

    void RemoveAlloc(UPInt addr, GHeapPageInfoMH* pageInfo);

    void RelinkAlloc(DebugDataPtr* ptr, UPInt oldAddr, 
                     UPInt newAddr, UPInt newSize, GHeapPageInfoMH* newInfo);

    void CheckDataTail(const DebugDataPtr* ptr, UPInt usable);

    void FreeAll();

    void GetDebugData(UPInt addr, GHeapPageInfoMH* pageInfo, DebugDataPtr* ptr);

    void UnlinkAlloc(UPInt addr, GHeapPageInfoMH* pageInfo, DebugDataPtr* ptr);

    void GetStats(GHeapAllocEngineMH* allocator, GStatBag* bag) const;

    const GHeapDebugDataMH* GetFirstEntry() const;
    const GHeapDebugDataMH* GetNextEntry(const GHeapDebugDataMH* entry) const;

    void VisitMem(GHeapMemVisitor* visitor, unsigned flags) const;

    bool DumpMemoryLeaks(const char* heapName);

    void UltimateCheck();

    UPInt GetUsedSpace() const;

private:
    bool        allocDataPool();
    void        freeDataPool(DataPoolType* pool);

    GHeapDebugDataMH* getDebugData(GHeapPageInfoMH* page);
    void              setDebugData(GHeapPageInfoMH* page, GHeapDebugDataMH* data);
    GHeapDebugDataMH* allocDebugData();
    void        freeDebugData(GHeapDebugDataMH* data);
    void        unlinkDebugData(GHeapPageInfoMH* page, DebugDataPtr* ptr);
    void        linkDebugData(GHeapPageInfoMH* page, GHeapDebugDataMH* data);
    void        findInChainWithin(GHeapDebugDataMH* chain, UPInt addr, DebugDataPtr* ptr);
    void        findInChainExact(GHeapDebugDataMH* chain, UPInt addr, DebugDataPtr* ptr);
    void        findDebugData(GHeapPageInfoMH* page, UPInt addr, DebugDataPtr* ret);
    void        fillDataTail(GHeapDebugDataMH* data, UPInt usable);
    void        reportViolation(GHeapDebugDataMH* data, const char* msg);

    GSysAlloc*            pAllocator;
    GList<DataPoolType>     DataPools;
    GList<GHeapDebugDataMH> UsedDataList;
    GList<GHeapDebugDataMH> FreeDataList;
    GLockSafe*              pRootLocker;
};

#endif // GHEAP_DEBUG_INFO

#endif

