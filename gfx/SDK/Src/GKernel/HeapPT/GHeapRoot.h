/**********************************************************************

Filename    :   GHeapRoot.h
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

#ifndef INC_GHeapRoot_H
#define INC_GHeapRoot_H

#include "GAtomic.h"
#include "GHeapStarter.h"
#include "GHeapBookkeeper.h"
#include "GMemoryHeap.h"
#include "GSysAllocGranulator.h"


//------------------------------------------------------------------------
class GSysAllocWrapper : public GSysAllocPaged
{
public:
    GSysAllocWrapper(GSysAllocPaged* sysAlloc);

    virtual void    GetInfo(Info* i) const;
    virtual void*   Alloc(UPInt size, UPInt align);
    virtual bool    Free(void* ptr, UPInt size, UPInt align);
    virtual bool    ReallocInPlace(void* oldPtr, UPInt oldSize, UPInt newSize, UPInt align);

    virtual void*   AllocSysDirect(UPInt size, UPInt alignment, UPInt* actualSize, UPInt* actualAlign);
    virtual bool    FreeSysDirect(void* ptr, UPInt size, UPInt alignment);

    virtual void    VisitMem(class GHeapMemVisitor*) const;
    virtual void    VisitSegments(class GHeapSegVisitor* visitor, UInt catSeg, UInt catUnused) const;

    virtual UPInt   GetFootprint() const;
    virtual UPInt   GetUsedSpace() const;

    void*           AllocSysDirect(UPInt size);
    void            FreeSysDirect(void* ptr, UPInt size);

private:
    GSysAllocGranulator Granulator;
    GSysAllocPaged*          pSrcAlloc;
    GSysAllocPaged*          pSysAlloc;
    UPInt               SysGranularity; // For AllocSysDirect
    UPInt               MinAlign;       // For AllocSysDirect
    UPInt               UsedSpace;
};

//------------------------------------------------------------------------
class GHeapRoot
{
public:
    GHeapRoot(GSysAllocPaged* sysAlloc);
    ~GHeapRoot();

    void                CreateArena(UPInt arena, GSysAllocPaged* sysAlloc);
    int                 DestroyArena(UPInt arena);
    void                DestroyAllArenas();
    bool                ArenaIsEmpty(UPInt arena);
    GSysAllocPaged*          GetSysAlloc(UPInt arena);

    GHeapStarter*       GetStarter()    { return &Starter;    }
    GHeapBookkeeper*    GetBookkeeper() { return &Bookkeeper; }

    GMemoryHeapPT*      CreateHeap(const char*  name, 
                                   GMemoryHeapPT* parent,
                                   const GMemoryHeap::HeapDesc& desc);

    void                DestroyHeap(GMemoryHeapPT* heap);

    GLockSafe*          GetLock()     { return &RootLock; }

    void                VisitSegments(GHeapSegVisitor* visitor) const;

    void*               AllocSysDirect(UPInt size);
    void                FreeSysDirect(void* ptr, UPInt size);

    void                GetStats(GMemoryHeap::RootStats* stats) const;

#ifdef GHEAP_DEBUG_INFO
    GHeapGranulator*    GetDebugAlloc() { return &DebugAlloc; }
#endif

#ifdef GHEAP_TRACE_ALL
    void OnAlloc(const GMemoryHeapPT* heap, UPInt size, UPInt align, UInt sid, const void* ptr);
    void OnRealloc(const GMemoryHeapPT* heap, const void* oldPtr, UPInt newSize, const void* newPtr);
    void OnFree(const GMemoryHeapPT* heap, const void* ptr);

    static GMemoryHeap::HeapTracer* pTracer;
#endif


private:
    GSysAllocWrapper    SysAllocWrapper;
    GHeapStarter        Starter;
    GHeapBookkeeper     Bookkeeper;
#ifdef GHEAP_DEBUG_INFO
    GHeapGranulator     DebugAlloc;
#endif
    mutable GLockSafe   RootLock;
    GSysAllocPaged**         pArenas;
    UPInt               NumArenas;
};


#endif
