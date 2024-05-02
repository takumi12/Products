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

#include "GTypes.h"
#include "GHeapAllocEngine.h"
#include "GHeapRoot.h"
#include "GMemoryHeapPT.h"


extern GHeapRoot* GHeapGlobalRoot;


//------------------------------------------------------------------------
void GHeapAllocEngine::compilerAsserts()
{
    // Must be at least 4 pointers
    GCOMPILER_ASSERT(GHeap_MinSize >= 4*sizeof(void*));
}


//------------------------------------------------------------------------
GHeapAllocEngine::GHeapAllocEngine(GSysAllocPaged*   sysAlloc,
                                   GMemoryHeapPT* heap,
                                   UInt         allocFlags, 
                                   UPInt        minAlignSize,
                                   UPInt        granularity, 
                                   UPInt        reserve,
                                   UPInt        threshold,
                                   UPInt        limit) :
    pHeap(heap),
    pSysAlloc(sysAlloc),
    pBookkeeper(GHeapGlobalRoot->GetBookkeeper()),
    MinAlignShift(GHeapGetUpperBit(minAlignSize)),
    MinAlignMask((UPInt(1) << MinAlignShift) - 1),
    Allocator(MinAlignShift),
    AllowTinyBlocks((allocFlags & GHeap_AllowTinyBlocks) != 0),
    AllowDynaSize((allocFlags & GHeap_AllowDynaSize) != 0),
    Valid(false),
    HasRealloc(false),
    SysGranularity(GHeap_PageSize),
    Granularity((granularity + GHeap_PageMask) & ~UPInt(GHeap_PageMask)),
    Reserve((reserve + Granularity-1) / Granularity * Granularity),
    Threshold(threshold),
    SysDirectThreshold(0),
    Footprint(0),
    TinyFreeSpace(0),
    SysDirectSpace(0),
    pCachedBSeg(0),
    pCachedTSeg(0),
    Limit(limit),
    pLimHandler(0)
{
    // Must be locked by Root::Lock externally
    GHEAP_ASSERT(minAlignSize >= sizeof(void*) &&
                ((minAlignSize-1) & minAlignSize) == 0); // Must be pow2

    GSysAllocPaged::Info i;
    memset(&i, 0, sizeof(GSysAllocPaged::Info));
    pSysAlloc->GetInfo(&i);

    HasRealloc = i.HasRealloc;

    if (i.Granularity < GHeap_PageSize)
        i.Granularity = GHeap_PageSize;
    GHEAP_ASSERT((i.Granularity & GHeap_PageMask) == 0);

    SysGranularity     = i.Granularity;
    SysDirectThreshold = i.SysDirectThreshold;

    Granularity = (Granularity + i.Granularity-1) / i.Granularity * i.Granularity;

    if (i.MaxHeapGranularity)
    {
        i.MaxHeapGranularity = (i.MaxHeapGranularity + GHeap_PageMask) & ~GHeap_PageMask;
        if (Granularity > i.MaxHeapGranularity)
        {
            Granularity     = i.MaxHeapGranularity;
            AllowTinyBlocks = false;
            AllowDynaSize   = false;
          //Reserve         = 0; // Questionable
        }
    }

    // Restrict the minimal threshold. If direct system requests are too small
    // compared with the granularity there will be significant memory losses.
    // Empirically it's good enough to have at least 32x system granularity.
    // If threshold is 0 all the allocation will be redirected to the system
    // consuming a huge amount of memory. May be useful only for debugging.
    //------------------
    if (Threshold < 32*SysGranularity && Threshold)
        Threshold = 32*SysGranularity;

    if (SysDirectThreshold)
    {
        if (Threshold > SysDirectThreshold)
            Threshold = SysDirectThreshold;

        if (Threshold < GHeap_PageSize && Threshold)
            Threshold = GHeap_PageSize;

        if (Granularity > SysDirectThreshold)
        {
            Granularity = SysDirectThreshold;
            Granularity = (Granularity + GHeap_PageMask) & ~GHeap_PageMask;
        }

        if (SysGranularity > SysDirectThreshold)
        {
            SysGranularity = SysDirectThreshold;
            SysGranularity = (SysGranularity + GHeap_PageMask) & ~GHeap_PageMask;
        }
        if (Reserve > SysDirectThreshold)
            Reserve = SysDirectThreshold;
        AllowDynaSize = false;
    }

#ifdef GHEAP_FORCE_SYSALLOC
    SysGranularity  = GHeap_PageSize;
    Granularity     = GHeap_PageSize;
    Threshold       = 0;//GHeap_PageSize;
    Reserve         = 0;
    AllowTinyBlocks = false;
#endif

    bool t;
    Valid = Reserve ? allocSegmentBitSet(Reserve, minAlignSize, Granularity, &t) != 0 : true;
}


//------------------------------------------------------------------------
GHeapAllocEngine::~GHeapAllocEngine()
{
    FreeAll();
}


// Function FreeAll() is used when the heap is destroyed.
// After calling this function the heap remains usable, but it loses 
// all the allocations. 
//------------------------------------------------------------------------
void GHeapAllocEngine::FreeAll()
{
    // Already locked in Root::DestroyHeap()
    GHeapSegment* seg;
    while(!SegmentList.IsNull(seg = SegmentList.GetFirst()))
    {
        freeSegment(seg);
    }
    Allocator.Reset();
}


//------------------------------------------------------------------------
UPInt GHeapAllocEngine::SetLimit(UPInt lim)
{
    UPInt segSize = calcDynaSize();
    return Limit = (lim + segSize-1) / segSize * segSize;
}

//------------------------------------------------------------------------
GHeapSegment* GHeapAllocEngine::allocSegment(UInt  segType, UPInt dataSize, 
                                             UPInt alignSize, UPInt bookkeepingSize,
                                             bool *limHandlerOK)
{
    // Already locked by the caller

    if (Limit && Footprint + dataSize > Limit && pLimHandler)
    {
        GLockSafe::TmpUnlocker unlocker(GHeapGlobalRoot->GetLock());
        *limHandlerOK = 
            ((GMemoryHeap::LimitHandler*)pLimHandler)->
                OnExceedLimit(pHeap, Footprint + dataSize - Limit);
        return 0;
    }

    *limHandlerOK = false;

    UPInt totalSegSize = 
        (sizeof(GHeapSegment) + bookkeepingSize + GHeap_MinMask) & ~UPInt(GHeap_MinMask);

    UByte* segMem = (UByte*)pBookkeeper->Alloc(totalSegSize);
    GHeapSegment* seg = (GHeapSegment*)segMem;
    if (seg)
    {
        seg->SelfSize   = totalSegSize;
        seg->SegType    = UPIntHalf(segType);
        seg->Alignment  = GHeapGetUpperBit(UInt32(alignSize));
        seg->UseCount   = 0;
        seg->pHeap      = pHeap;
        seg->DataSize   = dataSize;
        seg->pData      = 0;
        if (dataSize)
        {
            // Allocate with the alignment of at least GHeap_PageSize.
            // However, the original alignment must be stored as is.
            //---------------
            if (alignSize < GHeap_PageSize)
                alignSize = GHeap_PageSize;

            seg->pData = (UByte*)pSysAlloc->Alloc(dataSize, alignSize);
            if (seg->pData == 0)
            {
                pBookkeeper->Free(seg, totalSegSize);
                return 0;
            }
            if (!GHeapGlobalPageTable->MapRange(seg->pData, seg->DataSize))
            {
                pSysAlloc->Free(seg->pData, dataSize, alignSize);
                pBookkeeper->Free(seg, totalSegSize);
                return 0;
            }
            GHeapGlobalPageTable->SetSegmentInRange(UPInt(seg->pData), seg->DataSize, seg);
        }
        SegmentList.PushFront(seg);
        Footprint += seg->DataSize;
        *limHandlerOK = true;
        return seg;
    }
    return 0;
}


//------------------------------------------------------------------------
GHeapSegment* GHeapAllocEngine::allocSegmentNoGranulator(UPInt dataSize, 
                                                         UPInt alignSize, 
                                                         bool* limHandlerOK)
{
    if (Limit && Footprint + dataSize > Limit && pLimHandler)
    {
        *limHandlerOK = 
            ((GMemoryHeap::LimitHandler*)pLimHandler)->
                OnExceedLimit(pHeap, Footprint + dataSize - Limit);
        return 0;
    }

    *limHandlerOK = false;

    GLockSafe::Locker rl(GHeapGlobalRoot->GetLock()); // Lock is necessary here!

    UByte* segMem = (UByte*)pBookkeeper->Alloc(sizeof(GHeapSegment));
    GHeapSegment* seg = (GHeapSegment*)segMem;
    if (seg)
    {
        seg->SelfSize   = sizeof(GHeapSegment);
        seg->SegType    = UPIntHalf(GHeap_SegmentSystem);
        seg->Alignment  = 0;
        seg->UseCount   = 0;
        seg->pHeap      = pHeap;
        seg->DataSize   = 0;
        seg->pData      = 0;
        if (dataSize)
        {
            UPInt actualSize, actualAlign;
            seg->pData = (UByte*)pSysAlloc->AllocSysDirect(dataSize, alignSize, &actualSize, &actualAlign);
            if (seg->pData == 0)
            {
                pBookkeeper->Free(seg, seg->SelfSize);
                return 0;
            }

            UPInt pageAlign = (alignSize > GHeap_PageSize) ? alignSize : GHeap_PageSize;
            UPInt headBytes = ((UPInt(seg->pData) + pageAlign-1) & ~(pageAlign-1)) - UPInt(seg->pData);
            seg->Alignment  = GHeapGetUpperBit(UInt32(actualAlign));
            seg->UseCount   = 0x80000000 | headBytes;
            seg->DataSize   = actualSize - headBytes;
            seg->pData     += headBytes;

            if (!GHeapGlobalPageTable->MapRange(seg->pData, seg->DataSize))
            {
                pSysAlloc->FreeSysDirect(seg->pData    - headBytes,
                                         seg->DataSize + headBytes,
                                         UPInt(1) << seg->Alignment);

                pSysAlloc->FreeSysDirect(seg->pData, dataSize, alignSize);
                pBookkeeper->Free(seg, seg->SelfSize);
                return 0;
            }
            GHeapGlobalPageTable->SetSegmentInRange(UPInt(seg->pData), seg->DataSize, seg);
        }
        SegmentList.PushFront(seg);
        Footprint += seg->DataSize + (seg->UseCount & 0x7FFFFFFF);
        *limHandlerOK = true;
        return seg;
    }
    return 0;
}


//------------------------------------------------------------------------
GHeapSegment* GHeapAllocEngine::allocSegmentLocked(UInt segType, UPInt dataSize, 
                                                   UPInt alignSize, UPInt bookkeepingSize,
                                                   bool *limHandlerOK)
{
    GLockSafe::Locker rl(GHeapGlobalRoot->GetLock());
    return allocSegment(segType, dataSize, alignSize, bookkeepingSize, limHandlerOK);
}

//------------------------------------------------------------------------
GHeapSegment* GHeapAllocEngine::allocSegmentBitSet(UPInt size, UPInt alignSize, 
                                                   UPInt granularity,
                                                   bool *limHandlerOK)
{
    GLockSafe::Locker rl(GHeapGlobalRoot->GetLock());

    UPInt alignment = (alignSize > MinAlignMask+1) ? alignSize : MinAlignMask+1;
    UPInt extraTail = (alignment > GHeap_PageSize) ? alignment : 0;
    UPInt segSize   = (size + extraTail + alignment-1) & ~(alignment-1);
          segSize   = (segSize + granularity-1) / granularity * granularity;

    GHeapSegment* seg = 
        allocSegment(GHeap_SegmentBitSet, 
                     segSize, 
                     GHeap_PageSize,
                     Allocator.GetBitSetBytes(segSize),
                     limHandlerOK);
    if (seg)
    {
        Allocator.InitSegment(seg);
    }
    return seg;
}


//------------------------------------------------------------------------
void GHeapAllocEngine::freeSegment(GHeapSegment* seg)
{
    // Already locked by the caller

    UPInt freeingSize = seg->DataSize;
    if (pLimHandler)
    {
        ((GMemoryHeap::LimitHandler*)pLimHandler)->OnFreeSegment(pHeap, freeingSize);
    }
    Footprint -= freeingSize;
    GHeapGlobalPageTable->UnmapRange(seg->pData, freeingSize);
    UPInt alignSize = UPInt(1) << seg->Alignment;
    if (seg->UseCount & 0x80000000)
    {
        Footprint -= (seg->UseCount & 0x7FFFFFFF);
        pSysAlloc->FreeSysDirect(seg->pData  - (seg->UseCount & 0x7FFFFFFF),
                                 freeingSize + (seg->UseCount & 0x7FFFFFFF),
                                 alignSize);
    }
    else
    {
        pSysAlloc->Free(seg->pData, freeingSize, 
                        (alignSize > GHeap_PageSize) ? 
                         alignSize : GHeap_PageSize);
    }
    SegmentListType::Remove(seg);
    pBookkeeper->Free(seg, seg->SelfSize);
}

//------------------------------------------------------------------------
void GHeapAllocEngine::freeSegmentLocked(GHeapSegment* seg)
{
    GLockSafe::Locker rl(GHeapGlobalRoot->GetLock());
    freeSegment(seg);
}


//------------------------------------------------------------------------
void GHeapAllocEngine::freeSegmentBitSet(GHeapSegment* seg)
{
    GLockSafe::Locker rl(GHeapGlobalRoot->GetLock());

    if (pCachedBSeg && pCachedBSeg != seg && pCachedBSeg->UseCount == 0)
    {
        Allocator.ReleaseSegment(pCachedBSeg);
        freeSegment(pCachedBSeg);
    }
    pCachedBSeg = 0;

#ifndef GHEAP_FORCE_SYSALLOC
    // Option 1.
    if (GetUsedSpace() && seg->DataSize <= 4*Granularity)
    {
        pCachedBSeg = seg;
    }
    else
#endif
    {
        if (Footprint >= Reserve + seg->DataSize)
        {
            Allocator.ReleaseSegment(seg);
            freeSegment(seg);
        }
    }
}

//------------------------------------------------------------------------
void* GHeapAllocEngine::allocSysDirect(UPInt dataSize, UPInt alignSize)
{
    bool limHandlerOK = false;
    GHeapSegment* seg;

    if (SysDirectThreshold && dataSize >= SysDirectThreshold)
    {
        do
        {
            seg = allocSegmentNoGranulator(dataSize, alignSize, &limHandlerOK);
            if (seg)
            {
                SysDirectSpace += seg->DataSize;
                return seg->pData;
            }
        }
        while(limHandlerOK);
    }

    // If allocSegmentNoGranulator() didn't work, try regular allocation.
    dataSize = (dataSize + (alignSize-1)) & ~(alignSize-1);
    dataSize = (dataSize + SysGranularity-1) / SysGranularity * SysGranularity;
    limHandlerOK = false;
    do
    {
        seg = allocSegmentLocked(GHeap_SegmentSystem, dataSize, alignSize, 
                                 0, &limHandlerOK);
        if (seg)
        {
            SysDirectSpace += seg->DataSize;
            return seg->pData;
        }
    }
    while(limHandlerOK);
    return 0;
}


//------------------------------------------------------------------------
UPInt GHeapAllocEngine::calcDynaSize() const
{
#ifdef GHEAP_FORCE_SYSALLOC
    return Granularity;
#else
    if (AllowDynaSize)
    {
        // Dynamic granularity works as follows:
        // 1. It takes a 1/16th of the totally allocated size and rounds it up 
        //    to the nearest granularity value.
        // 2. If not a power-of-two it rounds the size down to the nearest 
        //    power-of-two.
        // 3. Sets the size in range of [1...16] * Granularity
        UPInt dynaSize = (((GetUsedSpace() + 16) / 16) + Granularity-1) / Granularity * Granularity;

        dynaSize = UPInt(1) << GHeapGetUpperBit(dynaSize);

        if (dynaSize < Granularity)
            dynaSize = Granularity;

        if (dynaSize > 4*Granularity)
            dynaSize = 4*Granularity;

        return dynaSize;
    }
    return Granularity;
#endif
}

//------------------------------------------------------------------------
GHeapSegment* GHeapAllocEngine::allocSegmentBitSet(UPInt size, 
                                                   UPInt alignSize,
                                                   bool* limHandlerOK)
{
    GHeapSegment* seg = allocSegmentBitSet(size, alignSize, 
                                           calcDynaSize(), limHandlerOK);
    if (seg)
    {
        return seg;
    }
    return *limHandlerOK ? 
        allocSegmentBitSet(size, alignSize, Granularity, limHandlerOK) : 0;
}


//------------------------------------------------------------------------
void* GHeapAllocEngine::allocBitSet(UPInt size)
{
    void* ptr;
    GHeapSegment* seg;
    bool limHandlerOK = false;
    do
    {
        ptr = Allocator.Alloc(size, &seg);
        if (ptr)
        {
            ++seg->UseCount;
            return ptr;
        }
        allocSegmentBitSet(size, MinAlignMask+1, &limHandlerOK);
    }
    while(limHandlerOK);
    return 0;
}


//------------------------------------------------------------------------
void* GHeapAllocEngine::allocBitSet(UPInt size, UPInt alignSize)
{
    void* ptr;
    GHeapSegment* seg;
    bool limHandlerOK = false;
    do
    {
        ptr = Allocator.Alloc(size, alignSize, &seg);
        if (ptr)
        {
            ++seg->UseCount;
            return ptr;
        }
        allocSegmentBitSet(size, alignSize, &limHandlerOK);
    }
    while(limHandlerOK);
    return 0;
}


//------------------------------------------------------------------------
void* GHeapAllocEngine::allocSegmentTiny(UInt sizeIdx)
{
    GLockSafe::Locker rl(GHeapGlobalRoot->GetLock());

    bool limHandlerOK;
    UPInt blockSize = (sizeIdx + 1) << MinAlignShift;

    // The segment must be able to keep at least 4 tiny blocks,
    // aligned to GHeap_PageSize.
    //--------------------------
    UPInt segSize;
    segSize = (blockSize * 4 > GHeap_PageSize) ? blockSize * 4 : GHeap_PageSize;
    segSize = (segSize + GHeap_PageMask) & ~UPInt(GHeap_PageMask);

    GHeapSegment* seg = allocSegment(sizeIdx, segSize, GHeap_PageSize, 0, &limHandlerOK);
    if (seg)
    {
        UPInt numBlocks = seg->DataSize / blockSize;
        UByte* ptr = seg->pData;
        TinyListType& lst = TinyBlocks[sizeIdx];
        UPInt i;
        for (i = 0; i < numBlocks; ++i, ptr += blockSize)
        {
            ((TinyBlock*)ptr)->pSegment = seg;
            lst.PushBack((TinyBlock*)ptr);
        }
        TinyFreeSpace += seg->DataSize;
        return seg->pData;
    }
    return 0;
}



//------------------------------------------------------------------------
void GHeapAllocEngine::releaseSegmentTiny(GHeapSegment* seg)
{
    UPInt i;
    UPInt blockSize = (seg->SegType + 1) << MinAlignShift;
    UPInt numBlocks = seg->DataSize / blockSize;
    UByte* ptr = seg->pData;
    for (i = 0; i < numBlocks; ++i, ptr += blockSize)
    {
        TinyListType::Remove((TinyBlock*)ptr);
    }
    TinyFreeSpace -= seg->DataSize;
    freeSegment(seg);
}


//------------------------------------------------------------------------
void GHeapAllocEngine::freeSegmentTiny(GHeapSegment* seg)
{
    GLockSafe::Locker rl(GHeapGlobalRoot->GetLock());

    if (pCachedTSeg && pCachedTSeg != seg && pCachedTSeg->UseCount == 0)
    {
        releaseSegmentTiny(pCachedTSeg);
    }
    pCachedTSeg = 0;

    if (GetUsedSpace())
    {
        pCachedTSeg = seg;
    }
    else
    {
        if (Footprint >= Reserve + seg->DataSize)
        {
            releaseSegmentTiny(seg);
        }
    }
}


//------------------------------------------------------------------------
GINLINE void* GHeapAllocEngine::allocTiny(UInt sizeIdx)
{
    TinyBlock* ptr = TinyBlocks[sizeIdx].GetFirst();
    if (TinyBlocks[sizeIdx].IsNull(ptr))
    {
        ptr = (TinyBlock*)allocSegmentTiny(sizeIdx);
        if (ptr == 0)
        {
            return 0;
        }
    }
    TinyListType::Remove(ptr);
    ++ptr->pSegment->UseCount;
    TinyFreeSpace -= (sizeIdx + 1) << MinAlignShift;
    return ptr;
}


//------------------------------------------------------------------------
GINLINE void GHeapAllocEngine::freeTiny(GHeapSegment* seg, TinyBlock* ptr)
{
    ptr->pSegment = seg;
    TinyBlocks[seg->SegType].PushFront(ptr);
    TinyFreeSpace += (seg->SegType + 1) << MinAlignShift;
    if (--seg->UseCount == 0)
    {
        freeSegmentTiny(seg);
    }
}

//------------------------------------------------------------------------
GINLINE void* 
GHeapAllocEngine::reallocGeneral(GHeapSegment* seg, void* oldPtr, 
                                 UPInt oldSize, UPInt newSize,
                                 UPInt alignShift)
{
    void* newPtr = Alloc(newSize, UPInt(1) << alignShift);
    if (newPtr)
    {
        memcpy(newPtr, oldPtr, (oldSize < newSize) ? oldSize : newSize);
        Free(seg, oldPtr);
    }
    return newPtr;
}




//------------------------------------------------------------------------
void* GHeapAllocEngine::reallocSysDirect(GHeapSegment* seg, 
                                         void* oldPtr, UPInt newSize)
{
    if (seg->UseCount & 0x80000000)
    {
        // Not-null UseCound means we have a system direct allocation, without
        // the granulator. In this case just use general realloc.
        //--------------------
        return reallocGeneral(seg, oldPtr, seg->DataSize, newSize, seg->Alignment);
    }

    UPInt alignSize = UPInt(1) << seg->Alignment;

    newSize = (newSize + (alignSize-1)) & ~(alignSize-1);
    newSize = (newSize + SysGranularity-1) / SysGranularity * SysGranularity;
    UPInt oldSize = seg->DataSize;
    if (newSize == oldSize)
    {
        return seg->pData; // May happen often
    }

    GHEAP_ASSERT((UByte*)oldPtr == seg->pData);
    if (newSize < seg->DataSize && 2*newSize < Threshold)
    {
        // When shrinking, the size can become too small to continue
        // with the direct system allocations. Switch to general realloc.
        //--------------------
        return reallocGeneral(seg, oldPtr, seg->DataSize, newSize, seg->Alignment);
    }

    bool ok = false;
    GUNUSED(ok);

    if (newSize > oldSize)
    {
        if (Limit && Footprint + newSize - oldSize > Limit && pLimHandler)
        {
            bool limHandlerOK = 
                ((GMemoryHeap::LimitHandler*)pLimHandler)->
                    OnExceedLimit(pHeap, Footprint + newSize - oldSize - Limit);
            if (!limHandlerOK || Footprint + newSize - oldSize > Limit)
                return reallocGeneral(seg, oldPtr, oldSize, newSize, seg->Alignment);
        }
    }

    {
        GLockSafe::Locker rl(GHeapGlobalRoot->GetLock());

        if (HasRealloc &&
            pSysAlloc->ReallocInPlace(seg->pData, oldSize, newSize, alignSize))
        {
            // Remap
            //--------------------
            if (newSize > oldSize)
            {
                // Grow
                //--------------------
                if (!GHeapGlobalPageTable->RemapRange(seg->pData, newSize, oldSize))
                {
                    // Could not map, revert to the old state (shrink back)
                    //--------------------
                    ok = pSysAlloc->ReallocInPlace(seg->pData, newSize, oldSize, alignSize);
                    GHEAP_ASSERT(ok);
                    return NULL;
                }
            }
            else
            {
                // Shrink
                //--------------------
                ok = GHeapGlobalPageTable->RemapRange(seg->pData, newSize, oldSize);
                GHEAP_ASSERT(ok); 
            }

            Footprint      -= oldSize;
            Footprint      += newSize;
            SysDirectSpace -= oldSize;
            SysDirectSpace += newSize;

            seg->DataSize = newSize;
            return seg->pData;
        }
    }

    return reallocGeneral(seg, oldPtr, oldSize, newSize, seg->Alignment);
}





//------------------------------------------------------------------------
void* GHeapAllocEngine::Alloc(UPInt size)
{
    // The malloc() policy is to allocate the smallest possible 
    // block if size=0, or, in general, less than AC_MinSize.
    //------------------------
    if (size < GHeap_MinSize)
        size = GHeap_MinSize;

    size = (size + MinAlignMask) & ~MinAlignMask;

    if (AllowTinyBlocks && size <= (UPInt(GHeap_TinyBinSize) << MinAlignShift))
    {
        void* ptr;

        // The following empirical logic is necessary to correctly handle 
        // the situation with exceeding the footprint limit. It isn't necessary 
        // to make the allocation namely in the tiny segment. However, if the limit 
        // is exceeded after the first attempt it makes sense to try again 
        // considering that the application algorithm could free some memory.
        // If the second attempt fails just do a regular allocation.
        //-----------------
        ptr = allocTiny(UInt((size - 1) >> MinAlignShift));
        if (ptr) return ptr;

        ptr = allocTiny(UInt((size - 1) >> MinAlignShift));
        if (ptr) return ptr;
    }

    if (size < Threshold)
    {
        return allocBitSet(size);
    }

    return allocSysDirect(size, GHeap_PageSize);
}


//------------------------------------------------------------------------
static const UInt GHeapTinyPow2AllocType[GHeap_TinyBinSize] =
{
//  For 16-byte MinAlign:
//  16->16  32->32  48->64  64->64  etc->128
      0,      1,      3,      3,    7, 7, 7, 7
};


// Alloc with the given alignment
//------------------------------------------------------------------------
void* GHeapAllocEngine::Alloc(UPInt size, UPInt alignSize)
{
    GHEAP_ASSERT(((alignSize-1) & alignSize) == 0); // Must be a power of two

    if ((alignSize-1) <= MinAlignMask)
    {
        return Alloc(size); // Redirect to regular alloc 
    }

    // The malloc() policy is to allocate the smallest possible 
    // block if size=0, or, in general, less than AC_MinSize.
    //------------------------
    if (size < GHeap_MinSize)
        size = GHeap_MinSize;

    // alignSize cannot be less than MinAlignSize
    //------------------------
    if (alignSize < MinAlignMask+1)
        alignSize = MinAlignMask+1;

    // The malloc() policy is to allocate the smallest possible 
    // block if size=0, or, in general, if less than alignSize.
    //------------------------
    if (size < alignSize)
        size = alignSize;

    size = (size + alignSize-1) & ~(alignSize-1);

    if (AllowTinyBlocks && size <= (UPInt(GHeap_TinyBinSize) << MinAlignShift))
    {
        void* ptr;

        // The following empirical logic is necessary to correctly handle 
        // the situation with exceeding the footprint limit. It isn't necessary 
        // to make the allocation namely in the tiny segment. However, if the limit 
        // is exceeded after the first attempt it makes sense to try again 
        // considering that the application algorithm could free some memory.
        // If the second attempt fails just do a regular allocation.
        //-----------------
        ptr = allocTiny(GHeapTinyPow2AllocType[(size - 1) >> MinAlignShift]);
        if (ptr) return ptr;

        ptr = allocTiny(GHeapTinyPow2AllocType[(size - 1) >> MinAlignShift]);
        if (ptr) return ptr;
    }

    if (size < Threshold)
    {
        return allocBitSet(size, alignSize);
    }

    return allocSysDirect(size, alignSize);
}


//------------------------------------------------------------------------
void* GHeapAllocEngine::Realloc(GHeapSegment* seg, void* oldPtr, UPInt newSize)
{
    void* newPtr  = 0;
    UPInt oldSize = 0;
    GHEAP_ASSERT(seg != 0);

    // The realloc() policy is to shrink to the smallest possible 
    // block if size=0, or, in general, less than AC_MinSize.
    //------------------------
    if (newSize < GHeap_MinSize)
        newSize = GHeap_MinSize;

    newSize = (newSize + MinAlignMask) & ~MinAlignMask;

    if (seg->SegType <= GHeap_SegmentTiny8)
    {
        oldSize = (seg->SegType + 1) << MinAlignShift;
        if (newSize <= oldSize)
        {
            return oldPtr;      // Less or equal size.
        }
        return reallocGeneral(seg, oldPtr, oldSize, newSize, 
                              MinAlignShift + GHeapTinyPow2AllocType[seg->SegType]);
    }

    if (seg->SegType == GHeap_SegmentBitSet)
    {
        newPtr = Allocator.ReallocInPlace(seg, oldPtr, newSize, &oldSize);
        if (newPtr)
        {
            return newPtr;
        }
        return reallocGeneral(seg, oldPtr, oldSize, newSize, 
                              Allocator.GetAlignShift(seg, oldPtr, oldSize));
    }

    GHEAP_ASSERT(seg->SegType == GHeap_SegmentSystem);
    return reallocSysDirect(seg, oldPtr, newSize);
}


//------------------------------------------------------------------------
void GHeapAllocEngine::Free(GHeapSegment* seg, void* ptr)
{
    GHEAP_ASSERT(seg != 0);
    if (seg->SegType <= GHeap_SegmentTiny8)
    {
        freeTiny(seg, (TinyBlock*)ptr);
        return;
    }

    if (seg->SegType == GHeap_SegmentBitSet)
    {
        Allocator.Free(seg, ptr);
        if (--seg->UseCount == 0)
        {
            freeSegmentBitSet(seg);
        }
        return;
    }

    GHEAP_ASSERT(seg->SegType == GHeap_SegmentSystem);
    SysDirectSpace -= seg->DataSize;
    freeSegmentLocked(seg);
}


//------------------------------------------------------------------------
void* GHeapAllocEngine::Realloc(void* oldPtr, UPInt newSize)
{
    return Realloc(GHeapGlobalPageTable->GetSegment(UPInt(oldPtr)), oldPtr, newSize);
}


//------------------------------------------------------------------------
void GHeapAllocEngine::Free(void* ptr)
{
    Free(GHeapGlobalPageTable->GetSegment(UPInt(ptr)), ptr);
}


//------------------------------------------------------------------------
UPInt GHeapAllocEngine::GetUsableSize(GHeapSegment* seg, const void* ptr)
{
    GHEAP_ASSERT(seg != 0);
    if (seg->SegType <= GHeap_SegmentTiny8)
    {
        return UPInt(seg->SegType + 1) << MinAlignShift;
    }

    if (seg->SegType == GHeap_SegmentBitSet)
    {
        return Allocator.GetUsableSize(seg, ptr);
    }

    GHEAP_ASSERT(seg->SegType == GHeap_SegmentSystem);
    return seg->DataSize;
}


//------------------------------------------------------------------------
UPInt GHeapAllocEngine::GetUsableSize(const void* ptr)
{
    return GetUsableSize(GHeapGlobalPageTable->GetSegment(UPInt(ptr)), ptr);
}


//------------------------------------------------------------------------
UPInt GHeapAllocEngine::GetFootprint() const
{
    return Footprint;
}

//------------------------------------------------------------------------
UPInt GHeapAllocEngine::GetUsedSpace() const
{
    return Footprint - Allocator.GetTotalFreeSpace() - TinyFreeSpace;
}

//------------------------------------------------------------------------
void GHeapAllocEngine::ReleaseCachedMem()
{
    // Already locked by the caller with Root::Lock.

    if (pCachedBSeg && pCachedBSeg->UseCount == 0)
    {
        Allocator.ReleaseSegment(pCachedBSeg);
        freeSegment(pCachedBSeg);
    }
    if (pCachedTSeg && pCachedTSeg->UseCount == 0)
    {
        freeSegmentTiny(pCachedTSeg);
    }
    pCachedBSeg = 0;
    pCachedTSeg = 0;
}


//------------------------------------------------------------------------
void GHeapAllocEngine::VisitMem(GHeapMemVisitor* visitor, UInt flags) const
{
    if (flags & GHeapMemVisitor::VF_SysAlloc)
        pSysAlloc->VisitMem(visitor);

    if (flags & GHeapMemVisitor::VF_Starter)
        GHeapGlobalPageTable->VisitMem(visitor);

    if (flags & GHeapMemVisitor::VF_Bookkeeper)
        pBookkeeper->VisitMem(visitor, flags);

    if (flags & GHeapMemVisitor::VF_Heap)
    {
        const GHeapSegment* seg = SegmentList.GetFirst();
        while(!SegmentList.IsNull(seg))
        {
            switch(seg->SegType)
            {
            case GHeap_SegmentTiny1:
            case GHeap_SegmentTiny2:
            case GHeap_SegmentTiny3:
            case GHeap_SegmentTiny4:
            case GHeap_SegmentTiny5:
            case GHeap_SegmentTiny6:
            case GHeap_SegmentTiny7:
            case GHeap_SegmentTiny8:
                visitor->Visit(seg, UPInt(seg->pData), seg->DataSize, 
                               GHeapMemVisitor::Cat_AllocTiny);
                {
                    UPInt tinySize = (seg->SegType + 1) << MinAlignShift;
                    UPInt tailSize = seg->DataSize % tinySize;
                    if (tailSize)
                    {
                        // Visit the tail in the tiny block that always remains free
                        UPInt tail = UPInt(seg->pData) + seg->DataSize / tinySize * tinySize;
                        visitor->Visit(seg, tail, tailSize, GHeapMemVisitor::Cat_AllocTinyFree);
                    }
                }
                break;

            case GHeap_SegmentBookkeeper: // Reserved for the bookkeeper
                GHEAP_ASSERT(0);          // This event must never happen
                break;

            case GHeap_SegmentSystem:
                visitor->Visit(seg, UPInt(seg->pData), seg->DataSize, 
                               GHeapMemVisitor::Cat_SystemDirect);
                break;

            case GHeap_SegmentBitSet:
                visitor->Visit(seg, UPInt(seg->pData), seg->DataSize, 
                               GHeapMemVisitor::Cat_AllocBitSet);
                break;
            }
            seg = seg->pNext;
        }

        if (flags & GHeapMemVisitor::VF_HeapFree)
        {
            Allocator.VisitMem(visitor, GHeapMemVisitor::Cat_AllocBitSetFree);

            // Visit free tiny blocks
            UPInt i;
            for (i = 0; i < GHeap_TinyBinSize; ++i)
            {
                const TinyListType& lst = TinyBlocks[i];
                const TinyBlock* block = lst.GetFirst();
                while(!lst.IsNull(block))
                {
                    visitor->Visit(block->pSegment,
                                   UPInt(block), (i+1) << MinAlignShift, 
                                   GHeapMemVisitor::Cat_AllocTinyFree);
                    block = block->pNext;
                }
            }
        }
    }
}


//------------------------------------------------------------------------
void GHeapAllocEngine::VisitSegments(GHeapSegVisitor* visitor) const
{
    const GHeapSegment* seg = SegmentList.GetFirst();
    while(!SegmentList.IsNull(seg))
    {
        visitor->Visit(GHeapSegVisitor::Seg_Heap, seg->pHeap, 
                       UPInt(seg->pData), 
                       (seg->DataSize + GHeap_PageMask) & ~GHeap_PageMask);
        seg = seg->pNext;
    }
    Allocator.VisitUnused(visitor, GHeapSegVisitor::Seg_HeapUnused);
}


//------------------------------------------------------------------------
void GHeapAllocEngine::GetHeapOtherStats(GHeapOtherStats* otherStats) const
{
    otherStats->Segments = 0;
    otherStats->Bookkeeping = 0;
    otherStats->DynamicGranularity = calcDynaSize();
    otherStats->SysDirectSpace = SysDirectSpace;
    const GHeapSegment* seg = SegmentList.GetFirst();
    while(!SegmentList.IsNull(seg))
    {
        otherStats->Segments++;
        otherStats->Bookkeeping += seg->SelfSize;
        seg = seg->pNext;
    }
}
