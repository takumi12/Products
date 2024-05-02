/**********************************************************************

Filename    :   GHeapAllocEngine.cpp
Content     :   The main allocation engine
            :   
Created     :   2009
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2008-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GHeapTypes.h"
#include "GHeapAllocEngineMH.h"


//------------------------------------------------------------------------
GHeapAllocEngineMH::GHeapAllocEngineMH(GSysAlloc*   sysAlloc,
                                       GMemoryHeapMH* heap,
                                       UPInt          minAlignSize,
                                       UPInt          limit) : 
    pSysAlloc(sysAlloc),
    pHeap(heap),
    MinAlignSize((minAlignSize < sizeof(void*)) ? sizeof(void*) : minAlignSize),
    Allocator(),
    Pages(),
    Footprint(0),
    UsedSpace(0),
    Limit(limit),
    pLimHandler(0),
    UseCount(0)
{
}

//------------------------------------------------------------------------
GHeapAllocEngineMH::~GHeapAllocEngineMH()
{
    FreeAll();
}

void GHeapAllocEngineMH::FreeAll()
{
    // TO DO: Implement
}

//------------------------------------------------------------------------
GHeapPageMH* GHeapAllocEngineMH::allocPage(bool* limHandlerOK)
{
    if (Limit && Footprint + GHeapPageMH::PageSize > Limit && pLimHandler)
    {
        *limHandlerOK = 
            ((GMemoryHeap::LimitHandler*)pLimHandler)->
                OnExceedLimit(pHeap, Footprint + GHeapPageMH::PageSize - Limit);
        return 0;
    }

    *limHandlerOK = false;

    GLockSafe::Locker rl(GHeapGlobalRootMH->GetLock());
    GHeapPageMH* page = GHeapGlobalRootMH->AllocPage(pHeap);
    if (page)
    {
        UInt32 index = GHeapGlobalRootMH->GetPageIndex(page);
        Allocator.InitPage(page, index);
        Footprint += GHeapPageMH::PageSize;
        Pages.PushFront(page);
        *limHandlerOK = true;
    }
    return page;
}

//------------------------------------------------------------------------
void GHeapAllocEngineMH::freePage(GHeapPageMH* page)
{
    GLockSafe::Locker rl(GHeapGlobalRootMH->GetLock());
    Allocator.ReleasePage(page->Start);
    PageListType::Remove(page);
    GHeapGlobalRootMH->FreePage(page);
    Footprint -= GHeapPageMH::PageSize;
}

//------------------------------------------------------------------------
void* GHeapAllocEngineMH::allocFromPage(UPInt size, GHeapPageInfoMH* info)
{
    void* ptr;
    GHeapMagicHeadersInfo headers;
    bool limHandlerOK = false;
    do
    {
        ptr = Allocator.Alloc(size, &headers);
        if (ptr)
        {
#ifdef GHEAP_DEBUG_INFO
            info->DebugHeaders[0] = info->DebugHeaders[1] = info->DebugHeaders[2] = 0;
#endif
            if (headers.Header1) 
            {
                headers.Header1->UseCount++;
#ifdef GHEAP_DEBUG_INFO
                info->DebugHeaders[0] = &(headers.Header1->DebugHeader);
#endif
            }
            if (headers.Header2) 
            {
                headers.Header2->UseCount++;
#ifdef GHEAP_DEBUG_INFO
                info->DebugHeaders[1] = &(headers.Header2->DebugHeader);
#endif
            }
            info->UsableSize = size;
            info->Page = headers.Page;
            info->Node = 0;
            ++UseCount;
            UsedSpace += size;
            return ptr;
        }
        allocPage(&limHandlerOK);
    }
    while(limHandlerOK);
    return 0;
}

//------------------------------------------------------------------------
void* GHeapAllocEngineMH::allocFromPage(UPInt size, UPInt alignSize, GHeapPageInfoMH* info)
{
    void* ptr;
    GHeapMagicHeadersInfo headers;
    bool limHandlerOK = false;
    do
    {
        ptr = Allocator.Alloc(size, alignSize, &headers);
        if (ptr)
        {
#ifdef GHEAP_DEBUG_INFO
            info->DebugHeaders[0] = info->DebugHeaders[1] = info->DebugHeaders[2] = 0;
#endif
            if (headers.Header1) 
            {
                headers.Header1->UseCount++;
#ifdef GHEAP_DEBUG_INFO
                info->DebugHeaders[0] = &(headers.Header1->DebugHeader);
#endif
            }
            if (headers.Header2) 
            {
                headers.Header2->UseCount++;
#ifdef GHEAP_DEBUG_INFO
                info->DebugHeaders[1] = &(headers.Header2->DebugHeader);
#endif
            }
            info->UsableSize = size;
            info->Page = headers.Page;
            info->Node = 0;
            ++UseCount;
            UsedSpace += size;
            return ptr;
        }
        allocPage(&limHandlerOK);
    }
    while(limHandlerOK);
    return 0;
}

//------------------------------------------------------------------------
void GHeapAllocEngineMH::Free(GHeapPageMH* page, void* ptr)
{
    GASSERT(page && page->pHeap == pHeap);
    GHeapMagicHeadersInfo headers;
    UPInt oldSize;
    Allocator.Free(page, ptr, &headers, &oldSize);
    unsigned useCount = 0;
    GASSERT(UsedSpace >= oldSize);
    UsedSpace -= oldSize;
    if (headers.Header1) useCount = --headers.Header1->UseCount;
    if (headers.Header2) useCount = --headers.Header2->UseCount;
    if (useCount == 0)
    {
        freePage(page);
    }
    --UseCount;
}


//------------------------------------------------------------------------
void GHeapAllocEngineMH::Free(GHeapNodeMH* node, void* ptr)
{
    GASSERT(node && node->GetHeap() == pHeap);
    GHeapGlobalRootMH->RemoveFromGlobalTree(node);
    UPInt align = node->GetAlign();
    UPInt nodeSize = GHeapNodeMH::GetNodeSize(align);
    UPInt size = (UByte*)node - (UByte*)ptr + nodeSize;
    --UseCount;
    Footprint -= size;
    GASSERT(UsedSpace >= size - nodeSize);
    UsedSpace -= size - nodeSize;
    pSysAlloc->Free(ptr, size, align);
}


//------------------------------------------------------------------------
// For easy use only. Not called from MemoryHeap
void GHeapAllocEngineMH::Free(void* ptr)
{
    GHeapPageMH* page = GHeapGlobalRootMH->ResolveAddress(UPInt(ptr));
    if (page)
    {
        Free(page, ptr);
    }
    else
    {
        GLockSafe::Locker rl(GHeapGlobalRootMH->GetLock());
        GHeapNodeMH* node = GHeapGlobalRootMH->FindNodeInGlobalTree((UByte*)ptr);
        Free(node, ptr);
    }
}

//------------------------------------------------------------------------
void GHeapAllocEngineMH::GetPageInfo(GHeapPageMH* page, GHeapPageInfoMH* info) const
{
    memset(info, 0, sizeof(GHeapPageInfoMH));
#ifdef GHEAP_DEBUG_INFO
    GHeapMagicHeadersInfo headers;
    GHeapGetMagicHeaders(UPInt(page->Start), &headers);
    if(headers.Header1) info->DebugHeaders[0] = &(headers.Header1->DebugHeader);
    if(headers.Header2) info->DebugHeaders[1] = &(headers.Header2->DebugHeader);
#endif
    info->UsableSize = 0;
    info->Page = page;
}

//------------------------------------------------------------------------
void GHeapAllocEngineMH::GetPageInfo(GHeapNodeMH* node, GHeapPageInfoMH* info) const
{
    memset(info, 0, sizeof(GHeapPageInfoMH));
#ifdef GHEAP_DEBUG_INFO
    info->DebugHeaders[2] = &(node->DebugHeader);
#endif
    info->UsableSize = 0;
    info->Node = node;
}

//------------------------------------------------------------------------
void GHeapAllocEngineMH::GetPageInfoWithSize(GHeapPageMH* page, const void* ptr, GHeapPageInfoMH* info) const
{
    memset(info, 0, sizeof(GHeapPageInfoMH));
#ifdef GHEAP_DEBUG_INFO
    GHeapMagicHeadersInfo headers;
    GHeapGetMagicHeaders(UPInt(page->Start), &headers);
    if(headers.Header1) info->DebugHeaders[0] = &(headers.Header1->DebugHeader);
    if(headers.Header2) info->DebugHeaders[1] = &(headers.Header2->DebugHeader);
#endif
    info->UsableSize = Allocator.GetUsableSize(page, ptr);
    info->Page = page;
}

//------------------------------------------------------------------------
void GHeapAllocEngineMH::GetPageInfoWithSize(GHeapNodeMH* node, const void* ptr, GHeapPageInfoMH* info) const
{
    memset(info, 0, sizeof(GHeapPageInfoMH));
#ifdef GHEAP_DEBUG_INFO
    info->DebugHeaders[2] = &(node->DebugHeader);
#endif
    info->UsableSize = (UByte*)node - (UByte*)ptr;
    info->Node = node;
}

//------------------------------------------------------------------------
void* GHeapAllocEngineMH::allocDirect(UPInt size, UPInt alignSize, bool* limHandlerOK, GHeapPageInfoMH* info)
{
    size = (size + sizeof(void*) - 1) & ~UPInt(sizeof(void*) - 1);
    UPInt nodeSize = GHeapNodeMH::GetNodeSize(alignSize);

    if (Limit && Footprint + size + nodeSize > Limit && pLimHandler)
    {
        GLockSafe::TmpUnlocker unlocker(GHeapGlobalRootMH->GetLock());
        *limHandlerOK = 
            ((GMemoryHeap::LimitHandler*)pLimHandler)->
                OnExceedLimit(pHeap, Footprint + size + nodeSize - Limit);
        return 0;
    }

    *limHandlerOK = false;
    void* ptr = pSysAlloc->Alloc(size + nodeSize, alignSize);
    if (ptr)
    {
        GHeapNodeMH* node = GHeapGlobalRootMH->AddToGlobalTree((UByte*)ptr, size, alignSize, pHeap);
#ifdef GHEAP_DEBUG_INFO
        info->DebugHeaders[0] = info->DebugHeaders[1] = 0;
        info->DebugHeaders[2] = &(node->DebugHeader);
#endif
        info->UsableSize = size;
        info->Page = 0;
        info->Node = node;
        ++UseCount;
        Footprint += size + nodeSize;
        UsedSpace += size;
        *limHandlerOK = true;
    }
    return ptr;
}


//------------------------------------------------------------------------
void* GHeapAllocEngineMH::Alloc(UPInt size, GHeapPageInfoMH* info)
{
    if (MinAlignSize > GHeapPageMH::UnitSize)
        return Alloc(size, MinAlignSize, info);

    if (size <= GHeapPageMH::MaxBytes)
    {
        size = (size + GHeapPageMH::UnitMask) & ~UPInt(GHeapPageMH::UnitMask);
        return allocFromPage(size, info);
    }

    void* ptr = 0;
    {
        GLockSafe::Locker rl(GHeapGlobalRootMH->GetLock());
        bool limHandlerOK = false;
        do
        {
            ptr = allocDirect(size, MinAlignSize, &limHandlerOK, info);
            if (ptr)
                return ptr;
        }
        while(limHandlerOK);
    }
    return ptr;
}


//------------------------------------------------------------------------
void* GHeapAllocEngineMH::Alloc(UPInt size, UPInt alignSize, GHeapPageInfoMH* info)
{
    GASSERT(((alignSize-1) & alignSize) == 0); // Must be a power of two

    if (size <= GHeapPageMH::MaxBytes)
    {
        if (alignSize < GHeapPageMH::UnitSize)
            alignSize = GHeapPageMH::UnitSize;

        size = (size + GHeapPageMH::UnitMask) & ~UPInt(GHeapPageMH::UnitMask);
        return allocFromPage(size, alignSize, info);
    }

    void* ptr = 0;
    {
        if (alignSize < sizeof(void*))
            alignSize = sizeof(void*);

        if (size < alignSize)
            size = alignSize;

        size = (size + sizeof(void*)-1) & ~UPInt(sizeof(void*)-1);
        void* ptr = 0;
        {
            GLockSafe::Locker rl(GHeapGlobalRootMH->GetLock());
            bool limHandlerOK = false;
            do
            {
                ptr = allocDirect(size, alignSize, &limHandlerOK, info);
                if (ptr)
                    return ptr;
            }
            while(limHandlerOK);
        }
    }
    return ptr;
}


//------------------------------------------------------------------------
void* GHeapAllocEngineMH::ReallocInPage(GHeapPageMH* page, void* oldPtr, UPInt newSize, GHeapPageInfoMH* newInfo)
{
    GASSERT(page && page->pHeap == pHeap);
    void* newPtr = 0;
    if (newSize < GHeapPageMH::PageSize/2)
    {
        newSize = (newSize + GHeapPageMH::UnitMask) & ~UPInt(GHeapPageMH::UnitMask);
        GHeapMagicHeadersInfo headers;
        UPInt oldSize;
        newPtr = Allocator.ReallocInPlace(page, oldPtr, newSize, &oldSize, &headers);
        if (newPtr)
        {
#ifdef GHEAP_DEBUG_INFO
            newInfo->DebugHeaders[0] = newInfo->DebugHeaders[1] = newInfo->DebugHeaders[2] = 0;
            if (headers.Header1) newInfo->DebugHeaders[0] = &(headers.Header1->DebugHeader);
            if (headers.Header2) newInfo->DebugHeaders[1] = &(headers.Header2->DebugHeader);
#endif
            newInfo->UsableSize = newSize;
            newInfo->Page = headers.Page;
            newInfo->Node = 0;
            GASSERT(UsedSpace >= oldSize);
            UsedSpace -= oldSize;
            UsedSpace += newSize;
        }
    }
    return newPtr;
}

//------------------------------------------------------------------------
void* GHeapAllocEngineMH::ReallocInNode(GHeapNodeMH* node, void* oldPtr, UPInt newSize, GHeapPageInfoMH* newInfo)
{
    GASSERT(node && node->GetHeap() == pHeap);
    newSize = (newSize + sizeof(void*) - 1) & ~UPInt(sizeof(void*) - 1);

    void* newPtr = 0;
    UPInt oldAlign = node->GetAlign();
    UPInt nodeSize = GHeapNodeMH::GetNodeSize(oldAlign);
    UPInt oldSize  = (UByte*)node - (UByte*)oldPtr + nodeSize;
    newSize = (newSize + sizeof(void*)-1) & ~UPInt(sizeof(void*)-1);
    newSize += nodeSize;
    if (newSize > oldSize)
    {
        while (Limit && Footprint + newSize - oldSize > Limit && pLimHandler)
        {
            GLockSafe::TmpUnlocker unlocker(GHeapGlobalRootMH->GetLock());
            bool limHandlerOK = 
                ((GMemoryHeap::LimitHandler*)pLimHandler)->
                    OnExceedLimit(pHeap, Footprint + newSize - oldSize - Limit);
            if (!limHandlerOK)
                return 0;
        }
    }
    GHeapGlobalRootMH->RemoveFromGlobalTree(node);
    newPtr = pSysAlloc->Realloc(oldPtr, oldSize, newSize, oldAlign);
    if (newPtr)
    {
        GASSERT((UPInt(newPtr) & (oldAlign-1)) == 0);
        GHeapNodeMH* node = 
            GHeapGlobalRootMH->AddToGlobalTree((UByte*)newPtr, newSize-nodeSize, oldAlign, pHeap);
#ifdef GHEAP_DEBUG_INFO
        newInfo->DebugHeaders[0] = newInfo->DebugHeaders[1] = 0;
        newInfo->DebugHeaders[2] = &(node->DebugHeader);
#endif
        newInfo->UsableSize = newSize-nodeSize;
        newInfo->Page = 0;
        newInfo->Node = node;

        Footprint -= oldSize;
        Footprint += newSize;
        GASSERT(UsedSpace >= oldSize-nodeSize);
        UsedSpace -= oldSize-nodeSize;
        UsedSpace += newSize-nodeSize;
    }
    else
    {
        GHeapGlobalRootMH->AddToGlobalTree((UByte*)oldPtr, oldSize-nodeSize, oldAlign, pHeap);
    }
    return newPtr;
}

//------------------------------------------------------------------------
void* GHeapAllocEngineMH::ReallocGeneral(GHeapPageMH* page, void* oldPtr, UPInt newSize, GHeapPageInfoMH* newInfo)
{
    GHeapPageInfoMH oldInfo;
    void* newPtr = Alloc(newSize, newInfo);
    if (newPtr)
    {
        GetPageInfoWithSize(page, oldPtr, &oldInfo);
        memcpy(newPtr, oldPtr, (oldInfo.UsableSize < newInfo->UsableSize) ? 
                                oldInfo.UsableSize : newInfo->UsableSize);
        Free(page, oldPtr);
    }
    return newPtr;
}

//------------------------------------------------------------------------
void* GHeapAllocEngineMH::Realloc(void* oldPtr, UPInt newSize)
{
    GHeapPageInfoMH newInfo;
    GHeapPageMH* page = GHeapGlobalRootMH->ResolveAddress(UPInt(oldPtr));
    if (page)
    {
        return ReallocGeneral(page, oldPtr, newSize, &newInfo);
    }
    GLockSafe::Locker rl(GHeapGlobalRootMH->GetLock());
    GHeapNodeMH* node = GHeapGlobalRootMH->FindNodeInGlobalTree((UByte*)oldPtr);
    return ReallocInNode(node, oldPtr, newSize, &newInfo);
}

//------------------------------------------------------------------------
UPInt GHeapAllocEngineMH::GetUsableSize(void* ptr)
{
    GHeapPageMH* page = GHeapGlobalRootMH->ResolveAddress(UPInt(ptr));
    if (page)
    {
        return Allocator.GetUsableSize(page, ptr);
    }
    GLockSafe::Locker rl(GHeapGlobalRootMH->GetLock());
    GHeapNodeMH* node = GHeapGlobalRootMH->FindNodeInGlobalTree((UByte*)ptr);
    GASSERT(node);
    return (UByte*)node - (UByte*)ptr;
}

