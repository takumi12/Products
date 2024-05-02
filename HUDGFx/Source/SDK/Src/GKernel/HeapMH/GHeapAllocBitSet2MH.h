/**********************************************************************

Filename    :   GHeapAllocBitSet2MH.h
Content     :   "Magic-header based" Bit-set based allocator, 2 bits 
                per block.

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

#ifndef INC_GHeapAllocBitSet2MH_H
#define INC_GHeapAllocBitSet2MH_H

#include "GHeapFreeBinMH.h"
#include "GHeapBitSet2.h"

//------------------------------------------------------------------------
class GHeapAllocBitSet2MH
{
public:
    GHeapAllocBitSet2MH();

    void Reset() { Bin.Reset(); }

    void  InitPage(GHeapPageMH* page, UInt32 index);
    void  ReleasePage(UByte* start);

    void* Alloc(UPInt size, GHeapMagicHeadersInfo* headers);
    void* Alloc(UPInt size, UPInt alignSize, GHeapMagicHeadersInfo* headers);
    void* ReallocInPlace(GHeapPageMH* page, void* oldPtr, UPInt newSize, UPInt* oldSize, GHeapMagicHeadersInfo* headers);
    void  Free(GHeapPageMH* page, void* ptr, GHeapMagicHeadersInfo* headers, UPInt* oldBytes);

    UPInt GetUsableSize(const GHeapPageMH* page, const void* ptr) const;

private:
    GHeapListBinMH Bin;
};

#endif
