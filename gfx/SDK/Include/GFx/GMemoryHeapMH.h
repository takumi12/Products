/**********************************************************************

Access      :   Public
Filename    :   MemoryHeapMH.h
Content     :   Public memory heap class declaration, MagicHeader engine.
Created     :   October 1, 2008
Authors     :   Michael Antonov, Maxim Shemanarev

Copyright   :   (c) 2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GMemoryHeapMH_H
#define INC_GMemoryHeapMH_H

#include "GMemoryHeap.h"


// *** Predeclarations
//------------------------------------------------------------------------
class  GHeapRootMH;
class  GHeapAllocEngineMH;
class  GHeapDebugStorageMH;
struct GHeapPageMH;
struct GHeapNodeMH;
struct GHeapPageInfoMH;

// ***** MemoryHeap
//------------------------------------------------------------------------
class GMemoryHeapMH : public GMemoryHeap
{
private:
    friend class GHeapRootMH;

    GMemoryHeapMH();  // Explicit creation and destruction is prohibited
    virtual ~GMemoryHeapMH() {}

public:

    // *** Operations with memory arenas
    //
    //--------------------------------------------------------------------
    virtual void CreateArena(UPInt arena, GSysAllocPaged* sysAlloc);
    virtual void DestroyArena(UPInt arena);
    virtual bool ArenaIsEmpty(UPInt arena);

    // *** Initialization
    //
    // Creates a nested child heap; The heap should be destroyed 
    // by calling release. If child heap creation failed due to 
    // out-of-memory condition, returns 0. If child heap creation 
    // is not supported a pointer to the same parent heap will be returned.
    //--------------------------------------------------------------------
    virtual GMemoryHeap* CreateHeap(const char* name, 
                                    const HeapDesc& desc);

    // *** Service functions
    //--------------------------------------------------------------------

    virtual void    SetLimitHandler(LimitHandler* handler);
    virtual void    SetLimit(UPInt newLimit);
    virtual void    AddRef();
    virtual void    Release();

    // *** Allocation API
    //--------------------------------------------------------------------
    virtual void*   Alloc(UPInt size, const GAllocDebugInfo* info = 0);
    virtual void*   Alloc(UPInt size, UPInt align, const GAllocDebugInfo* info = 0);
    virtual void*   Realloc(void* oldPtr, UPInt newSize); 
    virtual void    Free(void* ptr);
    virtual void*   AllocAutoHeap(const void *thisPtr, UPInt size,
                                  const GAllocDebugInfo* info = 0);

    virtual void*   AllocAutoHeap(const void *thisPtr, UPInt size, UPInt align,
                                  const GAllocDebugInfo* info = 0);

    virtual GMemoryHeap* GetAllocHeap(const void *thisPtr);

    virtual UPInt   GetUsableSize(const void* ptr);


    virtual void*   AllocSysDirect(UPInt size);
    virtual void    FreeSysDirect(void* ptr, UPInt size);

    // *** Statistics
    //--------------------------------------------------------------------
    virtual bool    GetStats(GStatBag* bag);
    virtual UPInt   GetFootprint() const;
    virtual UPInt   GetTotalFootprint() const;
    virtual UPInt   GetUsedSpace() const;
    virtual UPInt   GetTotalUsedSpace() const;
    virtual void    GetRootStats(RootStats* stats);
    virtual void    VisitMem(GHeapMemVisitor* visitor, unsigned flags);

    virtual void    VisitRootSegments(GHeapSegVisitor* visitor);
    virtual void    VisitHeapSegments(GHeapSegVisitor* visitor) const;
    virtual void    SetTracer(HeapTracer* tracer);

private:
    //--------------------------------------------------------------------
    virtual void  destroyItself();
    virtual void  ultimateCheck();
    virtual void  releaseCachedMem();
    virtual bool  dumpMemoryLeaks();
    virtual void  checkIntegrity() const;
    virtual void  getUserDebugStats(RootStats* stats) const;

    void* allocMem(UPInt size, const GAllocDebugInfo* info);
    void* allocMem(UPInt size, UPInt align, const GAllocDebugInfo* info);
    void* allocMem(GHeapPageInfoMH* parentInfo, const void *thisPtr, UPInt size, const GAllocDebugInfo* info);
    void* allocMem(GHeapPageInfoMH* parentInfo, const void *thisPtr, UPInt size, UPInt align, const GAllocDebugInfo* info);
    void* reallocMem(GHeapPageMH* page, void* oldPtr, UPInt newSize);
    void* reallocMem(GHeapNodeMH* node, void* oldPtr, UPInt newSize);

    void freeLocked(void* ptr);
    void freeMem(GHeapPageMH* page, void* ptr);
    void freeMem(GHeapNodeMH* node, void* ptr);

    //--------------------------------------------------------------------
    GHeapAllocEngineMH*  pEngine;
    GHeapDebugStorageMH* pDebugStorage;
};

#endif