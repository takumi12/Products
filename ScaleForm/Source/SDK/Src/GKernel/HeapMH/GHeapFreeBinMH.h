/**********************************************************************

Filename    :   GHeapFreeBinMH.h
Content     :   
Created     :   2010
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

Notes       :   Containers to store information about free memory areas

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

For information regarding Commercial License Agreements go to:
online - http://www.scaleform.com/licensing.html or
email  - sales@scaleform.com 

**********************************************************************/

#ifndef INC_GHeapFreeBinMH_H
#define INC_GHeapFreeBinMH_H

#include "GHeapRootMH.h"

struct GHeapBinNodeMH
{
#ifdef GFC_64BIT_POINTERS
    enum { MinBlocks = 2 };
#else
    enum { MinBlocks = 1 };
#endif

    UPInt         Prev;
    UPInt        Next;
    GHeapPageMH* Page;  

    UPInt GetBlocks() const
    { 
        return (Prev & 0xF) | ((Next & 0xF) << 4);
    }

    UPInt GetBytes() const 
    { 
        return GetBlocks() << GHeapPageMH::UnitShift; 
    }

    UPInt GetPrevBlocks() const
    { 
        const UPInt* tail  = ((const UPInt*)this) - 2;
        return (tail[0] & 0xF) | ((tail[1] & 0xF) << 4);
    }

    UPInt GetPrevBytes() const
    { 
        return GetPrevBlocks() << GHeapPageMH::UnitShift; 
    }

    void SetBlocks(UPInt blocks)
    { 
        UPInt bytes = blocks << GHeapPageMH::UnitShift;
        UPInt* tail  = ((UPInt*)this) + bytes / sizeof(UPInt) - 2;
        tail[0] = Prev = (Prev & ~UPInt(0xF)) | (blocks & 0xF);
        tail[1] = Next = (Next & ~UPInt(0xF)) | (blocks >> 4);
    }

    void SetBytes(UPInt bytes)
    { 
        UPInt blocks = bytes >> GHeapPageMH::UnitShift;
        UPInt* tail  = ((UPInt*)this) + bytes / sizeof(UPInt) - 2;
        tail[0] = Prev = (Prev & ~UPInt(0xF)) | (blocks & 0xF);
        tail[1] = Next = (Next & ~UPInt(0xF)) | (blocks >> 4);
    }

    GHeapPageMH* GetPage()
    {
        return (GetBlocks() > MinBlocks) ? Page : 0;
    }

    void SetPage(GHeapPageMH* page)
    {
        if (GetBlocks() > MinBlocks)
            Page = page;
    }

    GHeapBinNodeMH*  GetPrev() const { return (GHeapBinNodeMH*)(Prev & ~UPInt(0xF)); }
    GHeapBinNodeMH*  GetNext() const { return (GHeapBinNodeMH*)(Next & ~UPInt(0xF)); }

    void        SetPrev(GHeapBinNodeMH* prev) { Prev = UPInt(prev) | (Prev & 0xF); }
    void        SetNext(GHeapBinNodeMH* next) { Next = UPInt(next) | (Next & 0xF); }

    static GHeapBinNodeMH* MakeNode(UByte* start, UPInt bytes, GHeapPageMH* page)
    {
        GHeapBinNodeMH* node = (GHeapBinNodeMH*)start;
        node->SetBytes(bytes);
        node->SetPage(page);
        return node;
    }
};


//------------------------------------------------------------------------
struct GHeapListBinMH
{
    enum { BinSize = 8*sizeof(UPInt) }; // Assume Byte is 8 bits.

    GHeapListBinMH();
    void Reset();

    //--------------------------------------------------------------------
    void        Push(UByte* node);
    void        Pull(UByte* node);

    GHeapBinNodeMH*  PullBest(UPInt blocks);
    GHeapBinNodeMH*  PullBest(UPInt blocks, UPInt alignMask);

    void        Merge(UByte* node, UPInt bytes, bool left, bool right, GHeapPageMH* page);

    static UByte* GetAlignedPtr(UByte* start, UPInt alignMask);
    static bool   AlignmentIsOK(const GHeapBinNodeMH* node, UPInt blocks, UPInt alignMask);

private:
    void        pushNode(UPInt idx, GHeapBinNodeMH* node);
    void        pullNode(UPInt idx, GHeapBinNodeMH* node);

    GHeapBinNodeMH*  findAligned(GHeapBinNodeMH* root, UPInt blocks, UPInt alignMask);

    GHeapBinNodeMH*  getPrevAdjacent(UByte* node) const;
    GHeapBinNodeMH*  getNextAdjacent(UByte* node) const;

    //--------------------------------------------------------------------
    UPInt           Mask;
    GHeapBinNodeMH* Roots[BinSize];
};



#endif
