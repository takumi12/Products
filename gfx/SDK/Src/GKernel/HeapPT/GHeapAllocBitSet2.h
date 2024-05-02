/**********************************************************************

Filename    :   GHeapAllocBitSet2.h
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

#ifndef INC_GHeapAllocBitSet2_H
#define INC_GHeapAllocBitSet2_H

#include "GHeapFreeBin.h"
#include "GHeapBitSet2.h"




//------------------------------------------------------------------------
class GHeapAllocBitSet2
{
public:
    GHeapAllocBitSet2(UPInt minAlignShift);

    void Reset() { Bin.Reset(); }

    static UInt32* GetBitSet(const GHeapSegment* seg)
    {
        return (UInt32*)(((UByte*)seg) + sizeof(GHeapSegment));
    }

    UPInt GetBitSetWords(UPInt dataSize) const
    {
        return GHeapBitSet2::GetBitSetSize(dataSize, MinAlignShift);
    }

    UPInt GetBitSetBytes(UPInt dataSize) const
    {
        return GetBitSetWords(dataSize) * sizeof(UInt32);
    }

    void  InitSegment(GHeapSegment* seg);
    void  ReleaseSegment(GHeapSegment* seg);

    void* Alloc(UPInt size, GHeapSegment** allocSeg);
    void* Alloc(UPInt size, UPInt alignSize, GHeapSegment** allocSeg);

    void  Free(GHeapSegment* seg, void* ptr);
    void* ReallocInPlace(GHeapSegment* seg, void* oldPtr, 
                         UPInt newSize, UPInt* oldSize);

    UPInt GetUsableSize(const GHeapSegment* seg, const void* ptr) const;
    UPInt GetAlignShift(const GHeapSegment* seg, const void* ptr, UPInt size) const;

    UPInt GetTotalFreeSpace() const
    {
        return Bin.GetTotalFreeSpace(MinAlignShift);
    }

    void VisitMem(GHeapMemVisitor* visitor, 
                  GHeapMemVisitor::Category cat) const
    {
        Bin.VisitMem(visitor, MinAlignShift, cat);
    }

    void VisitUnused(GHeapSegVisitor* visitor, UInt cat) const
    {
        Bin.VisitUnused(visitor, MinAlignShift, cat);
    }

    void CheckIntegrity() const
    {
        Bin.CheckIntegrity(MinAlignShift);
    }

private:
    UPInt        MinAlignShift;
    UPInt        MinAlignMask;
    GHeapFreeBin Bin;
};




#endif
