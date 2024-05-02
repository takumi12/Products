/**********************************************************************

Filename    :   GHeapAllocBitSet2.cpp
Content     :   Bit-set based allocator, 2 bits per block.

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

#include "GHeapAllocBitSet2.h"


//------------------------------------------------------------------------
GHeapAllocBitSet2::GHeapAllocBitSet2(UPInt minAlignShift) :
    MinAlignShift(minAlignShift),
    MinAlignMask((UPInt(1) << minAlignShift) - 1)
{
}


//------------------------------------------------------------------------
void GHeapAllocBitSet2::InitSegment(GHeapSegment* seg)
{
    GHeapBitSet2::MarkFree(GetBitSet(seg), 0, seg->DataSize >> MinAlignShift);
    Bin.Push(seg, seg->pData, seg->DataSize >> MinAlignShift, MinAlignShift);
}


//------------------------------------------------------------------------
void GHeapAllocBitSet2::ReleaseSegment(GHeapSegment* seg)
{
    Bin.Pull(seg->pData);
}

//------------------------------------------------------------------------
void* GHeapAllocBitSet2::Alloc(UPInt bytes, GHeapSegment** allocSeg)
{
    // Size must be greater or equal than GHeap_MinSize.
    // The conditions must be verified externally.
    GHEAP_ASSERT(bytes >= GHeap_MinSize && 
                (bytes & MinAlignMask) == 0);

    UByte* best = (UByte*)Bin.PullBest(bytes >> MinAlignShift);
    if (best)
    {
#ifdef GHEAP_CHECK_CORRUPTION
        Bin.CheckNode((GHeapLNode*)best, MinAlignShift);
#endif
        GHeapSegment* seg  = ((GHeapLNode*)best)->pSegment;
        UPInt   shift      = MinAlignShift;
        UPInt   blocks     = bytes >> shift;
        UInt32* bitSet     = GetBitSet(seg);
        UByte*  base       = seg->pData;
        UPInt   tailBlocks = Bin.GetSize(best) - blocks;
        UPInt   tailBytes  = tailBlocks << shift;

        // Free blocks with size less than GHeap_MinSize are not allowed.
        // If it happens, just merge this "tail" with allocated block.
        //--------------------------
        if (tailBytes >= GHeap_MinSize)
        {
            Bin.Push(seg, best + bytes, tailBlocks, shift);
            GHeapBitSet2::MarkFree(bitSet,
                                   (best + bytes - base) >> shift, 
                                   tailBlocks);
        }
        else
        {
            bytes += tailBytes;
        }
        GHeapBitSet2::MarkBusy(bitSet,
                               (best - base) >> shift, 
                               bytes >> shift, 0);
        *allocSeg = seg;
    }
    return best;
}



// Alloc with the given alignment.
//------------------------------------------------------------------------
void* GHeapAllocBitSet2::Alloc(UPInt bytes, UPInt alignSize, GHeapSegment** allocSeg)
{
    // Size and alignSize must meet the requirements below. 
    // The conditions must be verified externally.
    GHEAP_ASSERT(bytes     >= alignSize && 
                 bytes     >= GHeap_MinSize && 
                 alignSize >= MinAlignMask+1 &&
                 (bytes & (alignSize-1)) == 0);

    UPInt  shift     = MinAlignShift;
    UPInt  blocks    = bytes >> shift;
    UPInt  alignMask = alignSize-1;
    UByte* best      = (UByte*)Bin.PullBest(blocks, shift, alignMask);
    if (best)
    {
#ifdef GHEAP_CHECK_CORRUPTION
        Bin.CheckNode((GHeapLNode*)best, shift);
#endif
        GHeapSegment* seg = ((GHeapLNode*)best)->pSegment;
        UInt32* bitSet    = GetBitSet(seg);
        UByte*  base      = seg->pData;
        UByte*  aligned   = GHeapListBin::GetAlignedPtr(best, alignMask);
        UPInt   headBytes = aligned - best;
        UPInt   tailBytes = (best + (Bin.GetSize(best) << shift)) - (aligned + bytes);
        if (headBytes)
        {
            // PullBest with the given alignment guarantees that the 
            // "empty head" cannot be less than GHeap_MinSize, so that, 
            // we can add this block for sure.
            //--------------------------
            GHEAP_ASSERT(headBytes >= GHeap_MinSize);
            Bin.Push(seg, best, headBytes >> shift, shift);
            GHeapBitSet2::MarkFree(bitSet, (best - base) >> shift, 
                                   headBytes >> shift);
        }

        // Free blocks with size less than GHeap_MinSize are not allowed.
        // If it happens, just merge this "tail" with allocated block.
        //--------------------------
        if (tailBytes >= GHeap_MinSize)
        {
            Bin.Push(seg, aligned + bytes, tailBytes >> shift, shift);
            GHeapBitSet2::MarkFree(bitSet, (aligned + bytes - base) >> shift, 
                                   tailBytes >> shift);
        }
        else
        {
            bytes += tailBytes;
        }
        GHeapBitSet2::MarkBusy(bitSet,
                               (aligned - base) >> shift, 
                               bytes >> shift, 
                               GHeapGetUpperBit(alignSize) - shift);
        *allocSeg = seg;
        return aligned;
    }
    return 0;
}


//------------------------------------------------------------------------
void GHeapAllocBitSet2::Free(GHeapSegment* seg, void* ptr)
{
    bool    left, right, tail;
    UPInt   blocks;
    UPInt   bytes;
    UByte*  base   = seg->pData;
    UByte*  end    = base + seg->DataSize;
    UInt32* bitSet = GetBitSet(seg);
    UPInt   shift  = MinAlignShift;
    UPInt   start  = ((UByte*)ptr - base) >> shift;

    blocks = GHeapBitSet2::GetBlockSize(bitSet, start);
    GHEAP_ASSERT(blocks);
    bytes  = blocks << shift;

    GHeapBitSet2::MarkFree(bitSet, start, blocks);
    tail  = (UByte*)ptr + bytes < end;
    left  = start && GHeapBitSet2::GetValue(bitSet, start - 1     ) == 0;
    right = tail  && GHeapBitSet2::GetValue(bitSet, start + blocks) == 0;
    Bin.MakeNode(seg, (UByte*)ptr, blocks, shift);
    if (left | right)
    {
        Bin.Merge((UByte*)ptr, shift, left, right);
    }
    else
    {
        Bin.Push((UByte*)ptr);
    }
}

//------------------------------------------------------------------------
void* GHeapAllocBitSet2::ReallocInPlace(GHeapSegment* seg, void* oldPtr, 
                                        UPInt newSize, UPInt* oldSize)
{
    GHEAP_ASSERT(oldPtr && newSize && 
                 newSize >= GHeap_MinSize &&
                (newSize & MinAlignMask) == 0);

    UPInt   newBytes = newSize;
    UByte*  base     = seg->pData;
    UByte*  end      = base + seg->DataSize;
    UInt32* bitSet   = GetBitSet(seg);
    UPInt   shift    = MinAlignShift;
    UPInt   start    = ((UByte*)oldPtr - base) >> shift;
    UPInt   blocks   = GHeapBitSet2::GetBlockSize(bitSet, start);
    UInt    alignSh  = GHeapBitSet2::GetAlignShift(bitSet, start, blocks);
    UPInt   alignMsk = (UPInt(1) << (alignSh + MinAlignShift)) - 1;
    UPInt   oldBytes = blocks << shift;
    UByte*  nextAdj;

    *oldSize = oldBytes;
    newBytes = (newBytes + alignMsk) & ~alignMsk;

    if (newBytes > oldBytes)                // Grow
    {
        nextAdj = (UByte*)oldPtr + oldBytes;
        if (nextAdj < end && GHeapBitSet2::GetValue(bitSet, start + blocks) == 0)
        {
            UPInt nextBytes = Bin.GetSize(nextAdj) << shift;
            GHEAP_ASSERT(nextBytes);
            if (newBytes <= oldBytes + nextBytes)
            {
                Bin.Pull(nextAdj);
                UPInt tailBytes = oldBytes + nextBytes - newBytes;
                if (tailBytes >= GHeap_MinSize)
                {
                    UByte* tailBlock  = (UByte*)oldPtr + newBytes;
                    Bin.Push(seg, tailBlock, tailBytes >> shift, shift);

                    GHeapBitSet2::MarkFree(bitSet, 
                                           (tailBlock - base) >> shift, 
                                           tailBytes >> shift);
                }
                else
                {
                    newBytes += tailBytes;
                }
                GHeapBitSet2::MarkBusy(bitSet, start, newBytes >> shift, alignSh);
                return oldPtr;
            }
        }
        return 0;   // Can't grow
    }

    if (newBytes < oldBytes)                // Shrink
    {
        UPInt thisTail = oldBytes - newBytes;
        UPInt nextTail = 0;

        nextAdj = (UByte*)oldPtr + oldBytes;
        if (nextAdj < end && GHeapBitSet2::GetValue(bitSet, start + blocks) == 0)
        {
            nextTail = Bin.GetSize(nextAdj) << shift;
            GHEAP_ASSERT(nextTail);
        }
        if (thisTail + nextTail >= GHeap_MinSize)
        {
            if (nextTail)
            {
                Bin.Pull(nextAdj);
            }
            nextAdj   = (UByte*)oldPtr + newBytes;
            thisTail += nextTail;

            Bin.Push(seg, nextAdj, thisTail >> shift, shift);
            GHeapBitSet2::MarkBusy(bitSet, start, newBytes >> shift, alignSh);
            GHeapBitSet2::MarkFree(bitSet, (nextAdj - base) >> shift, 
                                   thisTail >> shift);
        }
    }
    return oldPtr;
}


//------------------------------------------------------------------------
UPInt GHeapAllocBitSet2::GetUsableSize(const GHeapSegment* seg, const void* ptr) const
{
    UPInt start = ((UByte*)ptr - seg->pData) >> MinAlignShift;
    return GHeapBitSet2::GetBlockSize(GetBitSet(seg), start) << MinAlignShift;
}

//------------------------------------------------------------------------
UPInt GHeapAllocBitSet2::GetAlignShift(const GHeapSegment* seg, const void* ptr, UPInt size) const
{
    UPInt start = ((UByte*)ptr - seg->pData) >> MinAlignShift;
    UInt  al = GHeapBitSet2::GetAlignShift(GetBitSet(seg), start, size >> MinAlignShift);
    return MinAlignShift + al;
}
