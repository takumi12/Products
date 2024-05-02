/**********************************************************************

Filename    :   GHeapAllocBitSet2MH.cpp
Content     :   Bit-set based allocator, 2 bits per block.

Created     :   2009
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


#include "GHeapRootMH.h"
#include "GHeapAllocBitSet2MH.h"

//------------------------------------------------------------------------
GHeapAllocBitSet2MH::GHeapAllocBitSet2MH()
{
}


//------------------------------------------------------------------------
void GHeapAllocBitSet2MH::InitPage(GHeapPageMH* page, UInt32 index)
{
    GHeapMagicHeadersInfo headers;
    GHeapGetMagicHeaders(UPInt(page->Start), &headers);
    memset(headers.BitSet, 0x55, GHeapPageMH::BitSetBytes); // set "all busy"
    if (headers.Header1)
    {
        headers.Header1->Magic = (UInt16)GHeapMagicHeader::MagicValue;
        headers.Header1->UseCount = 0;
        headers.Header1->Index = index;
        headers.Header1->DebugHeader = 0;
    }
    if (headers.Header2)
    {
        headers.Header2->Magic = (UInt16)GHeapMagicHeader::MagicValue;
        headers.Header2->UseCount = 0;
        headers.Header2->Index = index;
        headers.Header2->DebugHeader = 0;
    }
    UByte* start1 = 0;
    UByte* end1   = 0;
    UByte* start2 = 0;
    UByte* end2   = 0;
    if (headers.Header1)
    {
        start1 = headers.AlignedStart;
        end1   = (UByte*)headers.Header1;
    }
    if (headers.Header2)
    {
        start2 = ((UByte*)headers.Header2) + sizeof(GHeapMagicHeader);
        end2   = headers.AlignedEnd;
    }
    if ((UByte*)headers.BitSet < headers.Bound)
    {
        GASSERT(end1);
        end1 -= GHeapPageMH::BitSetBytes;
    }
    else
    {
        GASSERT(start2);
        start2 += GHeapPageMH::BitSetBytes;
    }
    if (start1)
    {
        GHeapBinNodeMH::MakeNode(start1, end1-start1, page);
        Bin.Push(start1);
        GHeapBitSet2::MarkFree(headers.BitSet, (start1-headers.AlignedStart) >> GHeapPageMH::UnitShift, 
                                               (end1-start1) >> GHeapPageMH::UnitShift);
    }
    if (start2)
    {
        GHeapBinNodeMH::MakeNode(start2, end2-start2, page);
        Bin.Push(start2);
        GHeapBitSet2::MarkFree(headers.BitSet, (start2-headers.AlignedStart) >> GHeapPageMH::UnitShift, 
                                               (end2-start2) >> GHeapPageMH::UnitShift);
    }
}


//------------------------------------------------------------------------
void GHeapAllocBitSet2MH::ReleasePage(UByte* start)
{
    GHeapMagicHeadersInfo headers;
    GHeapGetMagicHeaders(UPInt(start), &headers);
    UByte* start1 = 0;
    UByte* start2 = 0;
    if (headers.Header1)
    {
        GASSERT(headers.Header1->UseCount == 0);
        start1 = headers.AlignedStart;
        Bin.Pull(start1);
    }
    if (headers.Header2)
    {
        GASSERT(headers.Header2->UseCount == 0);
        start2 = ((UByte*)headers.Header2) + sizeof(GHeapMagicHeader);
        if ((UByte*)headers.BitSet > headers.Bound)
        {
            start2 += GHeapPageMH::BitSetBytes;
        }
        Bin.Pull(start2);
    }
}



//------------------------------------------------------------------------
void* GHeapAllocBitSet2MH::Alloc(UPInt bytes, GHeapMagicHeadersInfo* headers)
{
    // Size must be greater or equal than UnitSize.
    // The conditions must be verified externally.
    GASSERT(bytes >= GHeapPageMH::UnitSize && 
           (bytes &  GHeapPageMH::UnitMask) == 0 &&
            bytes <= GHeapPageMH::MaxBytes);

    UPInt           blocks = bytes >> GHeapPageMH::UnitShift;
    GHeapBinNodeMH* node  = Bin.PullBest(blocks);
    UByte*          best   = 0;
    if (node)
    {
        GHeapPageMH* page = node->GetPage();
        if (page == 0)
            page = GHeapGlobalRootMH->ResolveAddress(UPInt(node));
        GASSERT(page);
        best = (UByte*)node;
        GHeapGetMagicHeaders(UPInt(page->Start), headers);
        headers->Page = page;
        UPInt tailBytes = node->GetBytes() - bytes;
        if (tailBytes)
        {
            GHeapBinNodeMH::MakeNode(best + bytes, tailBytes, page);
            Bin.Push(best + bytes);
            GHeapBitSet2::MarkFree(headers->BitSet, 
                                  (best + bytes - headers->AlignedStart) >> GHeapPageMH::UnitShift, 
                                  tailBytes >> GHeapPageMH::UnitShift);
        }
        GHeapBitSet2::MarkBusy(headers->BitSet, 
                              (best - headers->AlignedStart) >> GHeapPageMH::UnitShift, 
                              bytes >> GHeapPageMH::UnitShift);
    }
    return best;
}

//------------------------------------------------------------------------
void GHeapAllocBitSet2MH::Free(GHeapPageMH* page, void* ptr, 
                               GHeapMagicHeadersInfo* headers,
                               UPInt* oldSize)
{
    GHeapGetMagicHeaders(UPInt(page->Start), headers);
    headers->Page = page;
    bool  left, right, tail;
    UPInt blocks, bytes, start;

    start  = ((UByte*)ptr - headers->AlignedStart) >> GHeapPageMH::UnitShift;
    blocks = GHeapBitSet2::GetBlockSize(headers->BitSet, start);
    GASSERT(blocks);
    bytes  = *oldSize = blocks << GHeapPageMH::UnitShift;
    GHeapBitSet2::MarkFree(headers->BitSet, start, blocks);
    tail  = (UByte*)ptr + bytes < headers->AlignedEnd;
    left  = start && GHeapBitSet2::GetValue(headers->BitSet, start - 1     ) == 0;
    right = tail  && GHeapBitSet2::GetValue(headers->BitSet, start + blocks) == 0;
    if (left | right)
    {
        Bin.Merge((UByte*)ptr, bytes, left, right, page);
    }
    else
    {
        GHeapBinNodeMH::MakeNode((UByte*)ptr, bytes, page);
        Bin.Push((UByte*)ptr);
    }
}


//------------------------------------------------------------------------
UPInt GHeapAllocBitSet2MH::GetUsableSize(const GHeapPageMH* page, const void* ptr) const
{
    GHeapMagicHeadersInfo headers;
    GHeapGetMagicHeaders(UPInt(page->Start), &headers);
    UPInt start, blocks;
    start  = ((const UByte*)ptr - headers.AlignedStart) >> GHeapPageMH::UnitShift;
    blocks = GHeapBitSet2::GetBlockSize(headers.BitSet, start);
    GASSERT(blocks);
    return blocks << GHeapPageMH::UnitShift;
}

//------------------------------------------------------------------------
void* GHeapAllocBitSet2MH::Alloc(UPInt bytes, UPInt alignSize, 
                                 GHeapMagicHeadersInfo* headers)
{
    // Size must be greater or equal than UnitSize.
    // The conditions must be verified externally.
    GASSERT(bytes >= GHeapPageMH::UnitSize && 
           (bytes &  GHeapPageMH::UnitMask) == 0 &&
            bytes <= GHeapPageMH::MaxBytes &&
            alignSize >= GHeapPageMH::UnitSize);

    UPInt      blocks    = bytes >> GHeapPageMH::UnitShift;
    UPInt      alignMask = alignSize-1;
    GHeapBinNodeMH* node = Bin.PullBest(blocks, alignMask);
    UByte*     best      = 0;
    if (node)
    {
        GHeapPageMH* page = node->GetPage();
        if (page == 0)
            page = GHeapGlobalRootMH->ResolveAddress(UPInt(node));
        GASSERT(page);

        best = (UByte*)node;
        UByte* aligned = GHeapListBinMH::GetAlignedPtr(best, alignMask);

        GHeapGetMagicHeaders(UPInt(page->Start), headers);
        headers->Page = page;
        UPInt headBytes = aligned - best;
        UPInt tailBytes = (best + node->GetBytes()) - (aligned + bytes);
        if (headBytes)
        {
            GHeapBinNodeMH::MakeNode(best, headBytes, page);
            Bin.Push(best);
            GHeapBitSet2::MarkFree(headers->BitSet, 
                                  (best - headers->AlignedStart) >> GHeapPageMH::UnitShift, 
                                   headBytes >> GHeapPageMH::UnitShift);
        }
        if (tailBytes)
        {
            GHeapBinNodeMH::MakeNode(aligned + bytes, tailBytes, page);
            Bin.Push(aligned + bytes);
            GHeapBitSet2::MarkFree(headers->BitSet, 
                                  (aligned + bytes - headers->AlignedStart) >> GHeapPageMH::UnitShift, 
                                   tailBytes >> GHeapPageMH::UnitShift);
        }
        GHeapBitSet2::MarkBusy(headers->BitSet, 
                              (aligned - headers->AlignedStart) >> GHeapPageMH::UnitShift, 
                               bytes >> GHeapPageMH::UnitShift);
        return aligned;
    }
    return 0;
}

//------------------------------------------------------------------------
void* GHeapAllocBitSet2MH::ReallocInPlace(GHeapPageMH* page, void* oldPtr, 
                                          UPInt newSize, UPInt* oldSize, 
                                          GHeapMagicHeadersInfo* headers)
{
    GASSERT(oldPtr && newSize && 
            newSize >= GHeapPageMH::UnitSize &&
           (newSize &  GHeapPageMH::UnitMask) == 0);

    GHeapGetMagicHeaders(UPInt(page->Start), headers);
    headers->Page = page;

    UPInt    newBytes = newSize;
    UByte*   base     = headers->AlignedStart;
    UByte*   end      = headers->AlignedEnd;
    if ((UByte*)oldPtr < headers->Bound)
    {
        end = headers->Bound - sizeof(GHeapMagicHeader);
        if ((UByte*)headers->BitSet < headers->Bound)
            end -= GHeapPageMH::BitSetBytes;
    }

    UInt32*  bitSet   = headers->BitSet;
    UPInt    start    = ((UByte*)oldPtr - base) >> GHeapPageMH::UnitShift;
    UPInt    blocks   = GHeapBitSet2::GetBlockSize(bitSet, start);
    UPInt    oldBytes = blocks << GHeapPageMH::UnitShift;
    UByte*   nextAdj;
    GHeapBinNodeMH* nextNode;
    UPInt    nextBytes;
    
    *oldSize = oldBytes;

    if (newBytes > oldBytes)                // Grow
    {
        nextAdj = (UByte*)oldPtr + oldBytes;
        if (nextAdj < end && GHeapBitSet2::GetValue(bitSet, start + blocks) == 0)
        {
            nextNode = (GHeapBinNodeMH*)nextAdj;
            nextBytes = nextNode->GetBytes();

            GASSERT(nextBytes);
            if (newBytes <= oldBytes + nextBytes)
            {
                Bin.Pull(nextAdj);
                UPInt tailBytes = oldBytes + nextBytes - newBytes;

                if (tailBytes)
                {
                    UByte* tailBlock = (UByte*)oldPtr + newBytes;
                    GHeapBinNodeMH::MakeNode(tailBlock, tailBytes, page);
                    Bin.Push(tailBlock);
                    GHeapBitSet2::MarkFree(bitSet, (tailBlock - base) >> GHeapPageMH::UnitShift, 
                                                    tailBytes >> GHeapPageMH::UnitShift);
                }
                GHeapBitSet2::MarkBusy(bitSet, start, newBytes >> GHeapPageMH::UnitShift);
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
            nextNode = (GHeapBinNodeMH*)nextAdj;
            nextTail = nextNode->GetBytes();
            GASSERT(nextTail);
        }
        if (thisTail + nextTail)
        {
            if (nextTail)
            {
                Bin.Pull(nextAdj);
            }
            nextAdj   = (UByte*)oldPtr + newBytes;
            thisTail += nextTail;

            GHeapBinNodeMH::MakeNode(nextAdj, thisTail, page);
            Bin.Push(nextAdj);
            GHeapBitSet2::MarkBusy(bitSet, start, newBytes >> GHeapPageMH::UnitShift);
            GHeapBitSet2::MarkFree(bitSet, (nextAdj - base) >> GHeapPageMH::UnitShift, 
                                   thisTail >> GHeapPageMH::UnitShift);
        }
    }
    return oldPtr;
}

