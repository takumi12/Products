/**********************************************************************

Filename    :   GHeapDebugInfo.h
Content     :   Debug and statistics implementation.
Created     :   July 14, 2008
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GHeapDebugInfo_H
#define INC_GHeapDebugInfo_H

#include "GHeapTypes.h"
#include "GMemoryHeap.h"

#ifdef GHEAP_DEBUG_INFO

#include "GList.h"
#include "GAtomic.h"
#include "GHeapGranulator.h"
#include "GHeapPageTable.h"


//------------------------------------------------------------------------
template<class T>
struct GHeapDebugDataPool : GListNode<GHeapDebugDataPool<T> >
{
    typedef GHeapDebugDataPool<T>   SelfType;

    GHeapDebugDataPool() : UseCount(0) {}

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
struct GHeapDebugNode : GListNode<GHeapDebugNode>
{
    enum
    {
        // Initially GHeapDebugData is created, as a single linked list
        // of nodes. After exceeding PageUpperLimit it is converted into
        // GHeapDebugPage. When DataCount gets less then PageLowerLimit
        // the GHeapDebugPage is converted back to GHeapDebugData.
        // It works like a classical Schmitt trigger with a hysteresis 
        // loop:
        //           LowerLimit
        //               /-------<-------+-----Page----
        //               |               | 
        //               |   Hysteresis  |
        //               |      Loop     |
        //               |               |
        //  ----Chain----+------->-------/
        //                           UpperLimit
        //----------------------------------------------------------------
        PageUpperLimit = 12,
        PageLowerLimit = 6,
        NodePageFlag   = 0x8000U,
        NodePageMask   = 0x7FFFU,
        ChainLimit     = 10
    };

    UPInt IsPage() const 
    { 
        return DataCount & UPInt(NodePageFlag); 
    }

    UPInt GetDataCount() const 
    {
        return DataCount & UPInt(NodePageMask);
    }

    UPInt Increment()
    {
        return (++DataCount) & UPInt(NodePageMask);
    }

    UPInt Decrement()
    {
        return (--DataCount) & UPInt(NodePageMask);
    }

    UPIntHalf DataCount;
    UPIntHalf ChainCount;
};


//------------------------------------------------------------------------
struct GHeapDebugData : GHeapDebugNode
{
    typedef GHeapDebugDataPool<GHeapDebugData> DataPool;

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


    DataPool*       pDataPool;
    GHeapDebugData* pParent;
    GHeapDebugData* pNextData;      // Next data entry in chain
    UPInt           RefCount;
    UPInt           Address;
    UPInt           Size;
    GAllocDebugInfo Info;
};



//------------------------------------------------------------------------
struct GHeapDebugPage : GHeapDebugNode
{
    typedef GHeapDebugDataPool<GHeapDebugPage> DataPool;

    enum 
    {
        PageShift   = 5,
        PageSize    = 1 << PageShift, // Must not exceed the number of bits in UPInt

        AddrShift   = GHeap_PageShift - PageShift,
        AddrSize    = 1 << AddrShift,
        AddrMask    = AddrSize-1
    }; 

    void Clear()
    {
        DataCount   = UPInt(NodePageFlag);
        ChainCount  = 0;
        Mask        = 0;
        memset(pData, 0, sizeof(pData));
    }

    void MessUp()
    {
        memset(&DataCount,  0xFE, sizeof(DataCount));
        memset(&ChainCount, 0xFE, sizeof(ChainCount));
        memset(&Mask,       0xFE, sizeof(Mask));
        memset(pData,       0xFE, sizeof(pData));
    }

    DataPool*       pDataPool;
    UPInt           Mask;
    GHeapDebugData* pData[PageSize];
};




//------------------------------------------------------------------------
class GStatBag;
class GHeapAllocEngine;
class GHeapDebugStorage
{
    enum 
    { 
        PoolSize        = GHeap_DebugAllocPoolSize,
        AddrShift       = GHeapDebugPage::AddrShift,
        AddrSize        = GHeapDebugPage::AddrSize,
        AddrMask        = GHeapDebugPage::AddrMask,
        PageUpperLimit  = GHeapDebugNode::PageUpperLimit,
        PageLowerLimit  = GHeapDebugNode::PageLowerLimit,
    };

    typedef GHeapDebugDataPool<GHeapDebugData> DebugDataPool;
    typedef GHeapDebugDataPool<GHeapDebugPage> DebugPagePool;

public:
    struct DebugDataPtr
    {
        DebugDataPtr() : pNode(0), Index(~UPInt(0)), pSelf(0), pPrev(0) {}
        GHeapDebugNode* pNode;
        UPInt           Index;
        GHeapDebugData* pSelf;
        GHeapDebugData* pPrev;
    };

    GHeapDebugStorage(GHeapGranulator* alloc, GLockSafe* rootLocker);

    UInt GetStatId(UPInt parentAddr, const GAllocDebugInfo* info);

    bool AddAlloc(UPInt parentAddr, bool autoHeap, UPInt thisAddr, 
                  UPInt size, UPInt usable, const GAllocDebugInfo* info);

    void RemoveAlloc(UPInt addr, UPInt usable);

    void RelinkAlloc(DebugDataPtr* ptr, UPInt oldAddr, 
                     UPInt newAddr, UPInt newSize, UPInt usable);

    void CheckDataTail(const DebugDataPtr* ptr, UPInt usable);

    void FreeAll();

    void GetDebugData(UPInt addr, DebugDataPtr* ptr);

    void UnlinkAlloc(UPInt addr, DebugDataPtr* ptr);

    void GetStats(GHeapAllocEngine* allocator, GStatBag* bag) const;

    const GHeapDebugData* GetFirstEntry() const;
    const GHeapDebugData* GetNextEntry(const GHeapDebugData* entry) const;

    void VisitMem(GHeapMemVisitor* visitor, UInt flags) const;

    bool DumpMemoryLeaks(const char* heapName);

    void UltimateCheck();

    UPInt GetUsedSpace() const;

private:
    bool            allocDataPool();
    void            freeDataPool(DebugDataPool* pool);

    bool            allocPagePool();
    void            freePagePool(DebugPagePool* pool);

    GHeapDebugPage* allocDebugPage();
    void            freeDebugPage(GHeapDebugPage* page);

    GHeapDebugData* allocDebugData();
    void            freeDebugData(GHeapDebugData* data);
    void            unlinkDebugData(DebugDataPtr* ptr);
    void            linkDebugData(GHeapDebugData* data);

    void            findInChainWithin(GHeapDebugData* chain, UPInt addr, 
                                      DebugDataPtr* ptr);

    void            findInChainExact(GHeapDebugData* chain, UPInt addr, 
                                     DebugDataPtr* ptr);

    void            findInNodeWithin(GHeapDebugNode* node, UPInt idx, UPInt addr, 
                                     DebugDataPtr* ptr);

    void            findDebugData(UPInt addr, bool autoHeap, DebugDataPtr* ret);

    void            linkToPage(GHeapDebugPage* page, GHeapDebugData* data);

    void            convertToChain(GHeapDebugPage* page, GHeapDebugData* oldData);
    bool            convertToPage(GHeapDebugData* chain, GHeapDebugData* newData);

    void            fillDataTail(GHeapDebugData* data, UPInt usable);

    void            reportViolation(GHeapDebugData* data, const char* msg);

    template<class T>
    void visitMem(const GList<T>& lst, UPInt dataSize, 
                  GHeapMemVisitor* visitor,
                  GHeapMemVisitor::Category cat, 
                  GHeapSegment* seg) const
    {
        const T* ptr = lst.GetFirst();
        while(!lst.IsNull(ptr))
        {
            const GHeapTreeSeg* allocSeg = pAllocator->GetAllocSegment(ptr);
            seg->pData    = allocSeg->Buffer;
            seg->DataSize = allocSeg->Size;
            visitor->Visit(seg, UPInt(ptr), dataSize, cat);
            ptr = (const T*)(ptr->pNext);
        }
    }


    GHeapGranulator*        pAllocator;
    GList<DebugDataPool>    DataPools;
    GList<DebugPagePool>    PagePools;
    GList<GHeapDebugData>   UsedDataList;
    GList<GHeapDebugData>   FreeDataList;
    GList<GHeapDebugPage>   FreePageList;
    GLockSafe*              pRootLocker;
};


#else //ifndef GHEAP_DEBUG_INFO

// inline stub for GHeapDebugStorage
class GHeapDebugStorage 
{
public:
    GHeapDebugStorage();
};

#endif //ifndef GHEAP_DEBUG_INFO

#endif // GHEAP_DEBUG_INFO

