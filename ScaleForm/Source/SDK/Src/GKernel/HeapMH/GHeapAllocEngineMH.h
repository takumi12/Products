/**********************************************************************

Filename    :   GHeapAllocEngineMH.h
Content     :   The main allocation engine
            :   
Created     :   2009
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GHeapAllocEngineMH_H
#define INC_GHeapAllocEngineMH_H

#include "GAtomic.h"
#include "GHeapRootMH.h"
#include "GHeapAllocBitSet2MH.h"

//------------------------------------------------------------------------
class GHeapAllocEngineMH
{
    typedef GList<GHeapPageMH>    PageListType;
public:
    GHeapAllocEngineMH(GSysAlloc*   sysAlloc,
                       GMemoryHeapMH* heap,
                       UPInt          minAlignSize=16,
                       UPInt          limit=0);
    ~GHeapAllocEngineMH();

    bool IsValid() const { return true; }

    void FreeAll();

    UPInt SetLimit(UPInt lim)            { return Limit = lim; }
    void  SetLimitHandler(void* handler) { pLimHandler = handler; }

    void*   Alloc(UPInt size, GHeapPageInfoMH* info);
    void*   Alloc(UPInt size, UPInt alignSize, GHeapPageInfoMH* info);

    void*   ReallocInPage(GHeapPageMH* page, void* oldPtr, UPInt newSize, GHeapPageInfoMH* newInfo);
    void*   ReallocInNode(GHeapNodeMH* node, void* oldPtr, UPInt newSize, GHeapPageInfoMH* newInfo);
    void*   ReallocGeneral(GHeapPageMH* page, void* oldPtr, UPInt newSize, GHeapPageInfoMH* newInfo);
    void*   Realloc(void* oldPtr, UPInt newSize);
    void    Free(GHeapPageMH* page, void* ptr);
    void    Free(GHeapNodeMH* node, void* ptr);
    void    Free(void* ptr);

    void    GetPageInfo(GHeapPageMH* page, GHeapPageInfoMH* info) const;
    void    GetPageInfo(GHeapNodeMH* node, GHeapPageInfoMH* info) const;
    void    GetPageInfoWithSize(GHeapPageMH* page, const void* ptr, GHeapPageInfoMH* info) const;
    void    GetPageInfoWithSize(GHeapNodeMH* node, const void* ptr, GHeapPageInfoMH* info) const;

    UPInt   GetFootprint() const { return Footprint; }
    UPInt   GetUsedSpace() const { return UsedSpace; }

    UPInt   GetUsableSize(void* ptr);

private:
    GHeapPageMH* allocPage(bool* limHandlerOK);
    void*   allocDirect(UPInt size, UPInt alignSize, bool* limHandlerOK, GHeapPageInfoMH* info);
    void    freePage(GHeapPageMH* page);

    void*   allocFromPage(UPInt size, GHeapPageInfoMH* info);
    void*   allocFromPage(UPInt size, UPInt alignSize, GHeapPageInfoMH* info);
    //void    freeToPage(PageMH* page, void* ptr);
    //UPInt   getAllocSize(void* ptr) const;


    GSysAlloc*        pSysAlloc;
    GMemoryHeapMH*      pHeap;
    UPInt               MinAlignSize;
    GHeapAllocBitSet2MH Allocator;
    PageListType        Pages;
    UPInt               Footprint;
    UPInt               UsedSpace;
    UPInt               Limit;
    void*               pLimHandler;
    UPInt               UseCount;
};


#endif

