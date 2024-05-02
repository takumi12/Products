/**********************************************************************

Filename    :   GHeapFreeBin.h
Content     :   
Created     :   2009
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

#ifndef INC_GHeapFreeBin_H
#define INC_GHeapFreeBin_H

#include "GHeapTypes.h"


// The structure of free zones.
//
// If the size is less than Limit (see below) it's stored in "ShortSize". 
// Otherwise, ShortSize equals to Limit, and the size is stored in UPInt, 
// right after sizeof(GHeapLNode). If the size is more than GHeap_MinSize, 
// the Filler is not used. In the smallest possible free block it's used 
// as the size at the end of the block. So that, for sizes < Limit:
// +----------+----------+----------+-----+-----+- - -+-----+
// |   pPrev  |   pNext  | pSegment |Size |Filler ... |Size |
// +-One-UPInt+----------+----------+-----+-----+- - -+-----+
//
// For sizes >= Limit:
// +----------+----------+----------+-----+-----+----------+- - -+----------+-----+-----+
// |   pPrev  |   pNext  | pSegment |Limit|Filler   Size   | ... |   Size   Filler|Limit|
// +-One-UPInt+----------+----------+-----+-----+----------+- - -+----------+-----+-----+
//
// (pPrev and pNext belong to GListNode<GHeapLNode>).
//------------------------------------------------------------------------
struct GHeapLNode
{
    GHeapLNode*     pPrev;
    GHeapLNode*     pNext;
    GHeapSegment*   pSegment;
    UPIntHalf       ShortSize;
    UPIntHalf       Filler;
};

//------------------------------------------------------------------------
struct GHeapTNode : GHeapLNode
{
    UPInt           Size;
    GHeapTNode*     Parent;
    GHeapTNode*     Child[2];
    UPInt           Index;
};


//------------------------------------------------------------------------
struct GHeapListBin
{
    enum { BinSize = 8*sizeof(UPInt) }; // Assume Byte is 8 bits.

    GHeapListBin();
    void Reset();

    //--------------------------------------------------------------------
    void        PushNode(UPInt idx, GHeapLNode* node);
    void        PullNode(UPInt idx, GHeapLNode* node);

    GHeapLNode* PullBest(UPInt idx);
    GHeapLNode* PullBest(UPInt idx, UPInt blocks, UPInt shift, UPInt alignMask);

    GHeapLNode* FindAligned(GHeapLNode* root, UPInt blocks, UPInt shift, UPInt alignMask);

    static UByte* GetAlignedPtr(UByte* start, UPInt alignMask);
    static bool   AlignmentIsOK(const GHeapLNode* node, UPInt blocks,
                                UPInt shift, UPInt alignMask);

    //--------------------------------------------------------------------
    UPInt       Mask;
    GHeapLNode* Roots[BinSize];
};


//------------------------------------------------------------------------
struct GHeapTreeBin
{
    enum 
    {
        BinSize   = GHeapListBin::BinSize,
        SizeShift = (sizeof(void*) > 4) ? 6 : 5, // Must correspond to sizeof(void*)
        SizeLimit = (sizeof(void*) > 4) ? 0xFFFFFFFFU : 0xFFFFU
    };

    GHeapTreeBin();
    void Reset();

    //--------------------------------------------------------------------
    static UPInt ShiftForTreeIndex(UPInt i);
    static UPInt ComputeTreeIndex(UPInt blocks);

    void        PushNode(GHeapTNode* node);
    void        PullNode(GHeapTNode* node);
    GHeapTNode* FindBest(UPInt size);
    GHeapTNode* PullBest(UPInt size);

    const GHeapTNode* FindExactSize(UPInt size) const;

    //--------------------------------------------------------------------
    UPInt       Mask;
    GHeapTNode* Roots[BinSize];
};



//------------------------------------------------------------------------
class GHeapSegVisitor;
class GHeapFreeBin
{
public:
    enum { BinSize = GHeapListBin::BinSize };

    GHeapFreeBin();
    void Reset();

    //--------------------------------------------------------------------
    GINLINE static
    UInt32* GetBitSet(const GHeapSegment* seg)
    {
        return (UInt32*)(((UByte*)seg) + sizeof(GHeapSegment));
    }

    //--------------------------------------------------------------------
    GINLINE static 
    UPInt GetSize(const void* node)
    {
        UPInt size = ((const GHeapLNode*)node)->ShortSize;
        if (size < BinSize+1)
        {
            return size;
        }
        return *(const UPInt*)((const UByte*)node + sizeof(GHeapLNode));
    }

    //--------------------------------------------------------------------
    GINLINE static 
    UPInt GetBytes(const void* node, UPInt shift)
    {
        return GetSize(node) << shift;
    }

    //--------------------------------------------------------------------
    GINLINE static 
    void SetSize(UByte* node, UPInt blocks, UPInt shift) 
    { 
        UPInt bytes = blocks << shift;
        GHeapLNode* listNode = (GHeapLNode*)node;
        if (blocks < BinSize+1)
        {
            listNode->ShortSize = 
            *(UPIntHalf*)(node + bytes - sizeof(UPIntHalf)) = UPIntHalf(blocks);
        }
        else
        {
            listNode->ShortSize = 
            *(UPIntHalf*)(node + bytes - sizeof(UPIntHalf)) = UPIntHalf(BinSize+1);

            *(UPInt*)(node + sizeof(GHeapLNode)) = 
            *(UPInt*)(node + bytes - 2*sizeof(UPInt)) = blocks;
        }
    }

    //--------------------------------------------------------------------
    GINLINE static void 
    MakeNode(GHeapSegment* seg, UByte* data, UPInt blocks, UPInt shift)
    {
        SetSize(data, blocks, shift);
        ((GHeapLNode*)data)->pSegment = seg;
#ifdef GHEAP_CHECK_CORRUPTION
        MemsetNode((GHeapLNode*)data, shift);
#endif
    }

    //--------------------------------------------------------------------
    UByte*  PullBest(UPInt blocks);
    UByte*  PullBest(UPInt blocks, UPInt shift, UPInt alignMask);

    //--------------------------------------------------------------------
            void Push(UByte* node); // Node must be initialized!
    GINLINE void Push(GHeapSegment* seg, UByte* node, 
                      UPInt blocks, UPInt shift)
    {
        MakeNode(seg, node, blocks, shift);
        Push(node);
    }

    //--------------------------------------------------------------------
    void    Pull(UByte* node);
    void    Merge(UByte* node, UPInt shift, bool left, bool right);

    UPInt   GetTotalFreeSpace(UPInt shift) const
    {
        return FreeBlocks << shift;
    }

    //--------------------------------------------------------------------
    void    VisitMem(GHeapMemVisitor* visitor, UPInt shift,
                     GHeapMemVisitor::Category cat) const;

    void    VisitUnused(GHeapSegVisitor* visitor, UPInt shift, UInt cat) const;

    void    CheckIntegrity(UPInt shift) const;

#ifdef GHEAP_CHECK_CORRUPTION
    static void CheckArea(const void* area, UPInt bytes);
    static void MemsetNode(GHeapLNode* node, UPInt shift);
    static void CheckNode(const GHeapLNode* node, UPInt shift);
#endif


private:
    static void compilerAsserts(); // A fake function used for GCOMPILER_ASSERTS only

    static GHeapLNode* getPrevAdjacent(UByte* node, UPInt shift);
    static GHeapLNode* getNextAdjacent(UByte* node, UPInt shift);

    void visitTree(const GHeapTNode* root, GHeapMemVisitor* visitor,
                   UPInt shift, GHeapMemVisitor::Category cat) const;

    void visitUnusedNode(const GHeapLNode* node, 
                         GHeapSegVisitor* visitor, 
                         UPInt shift, UInt cat) const;

    void visitUnusedInTree(const GHeapTNode* root, 
                           GHeapSegVisitor* visitor, 
                           UPInt shift, UInt cat) const;

    void checkBlockIntegrity(const GHeapLNode* node, UPInt shift) const;
    void checkTreeIntegrity(const GHeapTNode* root, UPInt shift) const;

    GHeapListBin    ListBin1;
    GHeapListBin    ListBin2;
    GHeapTreeBin    TreeBin;
    UPInt           FreeBlocks;
};



#endif
