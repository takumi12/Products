/**********************************************************************

Filename    :   GHeapAllocBitSet1.cpp
Content     :   Bit-set based allocator, 1 bit per block.

Created     :   2009
Authors     :   Maxim Shemanarev

Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

For information regarding Commercial License Agreements go to:
online - http://www.scaleform.com/licensing.html or
email  - sales@scaleform.com 

**********************************************************************/

#include "GHeapAllocBitSet1.h"


//------------------------------------------------------------------------
GHeapAllocBitSet1::GHeapAllocBitSet1(UPInt minAlignShift) :
    MinAlignShift(minAlignShift),
    MinAlignMask((UPInt(1) << minAlignShift) - 1)
{
}

//------------------------------------------------------------------------
void GHeapAllocBitSet1::InitSegment(GHeapSegment* seg)
{
    UPInt blocks = seg->DataSize >> MinAlignShift;
    markFree(GetBitSet(seg), 0, blocks);
    Bin.Push(seg, seg->pData, blocks, MinAlignShift);
}

//------------------------------------------------------------------------
void GHeapAllocBitSet1::ReleaseSegment(GHeapSegment* seg)
{
    Bin.Pull(seg->pData);
}

//------------------------------------------------------------------------
void* GHeapAllocBitSet1::Alloc(UPInt bytes, GHeapSegment** allocSeg)
{
    GHEAP_ASSERT(bytes >= GHeap_MinSize && (bytes & MinAlignMask) == 0);

    UPInt  minAlignShift = MinAlignShift;
    UPInt  blocks = bytes >> minAlignShift;
    UByte* best   = (UByte*)Bin.PullBest(blocks);
    if (best)
    {
#ifdef GHEAP_CHECK_CORRUPTION
        Bin.CheckNode((GHeapLNode*)best, minAlignShift);
#endif
        GHeapSegment* seg = ((GHeapLNode*)best)->pSegment;
        UPInt tailBlocks = Bin.GetSize(best) - blocks;
        UPInt tailBytes  = tailBlocks << minAlignShift;

        if (tailBytes >= GHeap_MinSize)
        {
            Bin.Push(seg, best + bytes, tailBlocks, minAlignShift);
            markFree(GetBitSet(seg),
                     (best - seg->pData + bytes) >> minAlignShift, 
                     tailBlocks);
        }
        else
        {
            bytes += tailBytes;
        }
        markBusy(GetBitSet(seg),
                 (best - seg->pData) >> minAlignShift, 
                 bytes >> minAlignShift);
        *allocSeg = seg;
    }
    return best;
}

//------------------------------------------------------------------------
void GHeapAllocBitSet1::Free(GHeapSegment* seg, void* ptr, UPInt bytes)
{
    GHEAP_ASSERT(bytes >= GHeap_MinSize && (bytes & MinAlignMask) == 0);

    bool    left, right, tail;
    UPInt   blocks;
    UByte*  base   = seg->pData;
    UByte*  end    = base + seg->DataSize;
    UInt32* bitSet = GetBitSet(seg);
    UPInt   start  = ((UByte*)ptr - base) >> MinAlignShift;

    blocks = bytes >> MinAlignShift;
    markFree(bitSet, start, blocks);
    tail  = (UByte*)ptr + bytes < end;
    left  = start && isZero(bitSet, start - 1     );
    right = tail  && isZero(bitSet, start + blocks);
    Bin.MakeNode(seg, (UByte*)ptr, blocks, MinAlignShift);
    if (left | right)
    {
        Bin.Merge((UByte*)ptr, MinAlignShift, left, right);
    }
    else
    {
        Bin.Push((UByte*)ptr);
    }
}


