/**********************************************************************

Filename    :   GHeapAllocLite.h
Content     :   Allocator based on binary search trees
Created     :   2009
Authors     :   Maxim Shemanarev

Notes       :   The granulator requests the system allocator for 
                large memory blocks and granulates them as necessary,
                providing smaller granularity to the consumer.

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GHeapAllocLite_H
#define INC_GHeapAllocLite_H

#include "GList.h"
#include "GRadixTree.h"
#include "GHeapTypes.h"

//------------------------------------------------------------------------
struct GHeapHdrPage;
struct GHeapTreeSeg
{
    GHeapTreeSeg*   AddrParent;
    GHeapTreeSeg*   AddrChild[2];
    GHeapHdrPage*   HdrPage;
    UByte*          Buffer;
    UPInt           Size;
    UPInt           UseCount;
    UPIntHalf       AlignShift;
    UPIntHalf       HeadBytes;
};


//------------------------------------------------------------------------
struct GHeapDualTNode : GListNode<GHeapDualTNode>
{
    GHeapDualTNode* Parent;
    GHeapDualTNode* Child[2];
    GHeapDualTNode* AddrParent;
    GHeapDualTNode* AddrChild[2];
    GHeapTreeSeg*   ParentSeg;
    UPInt           Size;

    //--------------------------------------------------------------------
    static GINLINE UByte* GetAlignedPtr(UByte* start, UPInt alignMask)
    {
        UPInt aligned = (UPInt(start) + alignMask) & ~alignMask;
        UPInt head = aligned - UPInt(start);
        while (head && head < 16*sizeof(void*))
        {
            aligned += alignMask+1;
            head    += alignMask+1;
        }
        return (UByte*)aligned;
    }

    GINLINE bool AlignmentIsOK(UPInt blocks, UPInt shift, UPInt alignMask) const
    {
        UPInt  bytes   = blocks << shift;
        UPInt  aligned = UPInt(GetAlignedPtr((UByte*)this, alignMask));
        return aligned + bytes <= UPInt(this) + (Size << shift);
    }
};

class GHeapSegVisitor;

//------------------------------------------------------------------------
class GHeapAllocLite
{
    typedef GHeapDualTNode FreeNode;

public:
    //--------------------------------------------------------------------
    struct SizeAccessor
    {
        static       UPInt      GetKey     (const FreeNode* n)          { return  n->Size;     }
        static       FreeNode*  GetChild   (      FreeNode* n, UPInt i) { return  n->Child[i]; }
        static const FreeNode*  GetChild   (const FreeNode* n, UPInt i) { return  n->Child[i]; }
        static       FreeNode** GetChildPtr(      FreeNode* n, UPInt i) { return &n->Child[i]; }

        static       FreeNode*  GetParent  (      FreeNode* n)          { return n->Parent; }
        static const FreeNode*  GetParent  (const FreeNode* n)          { return n->Parent; }

        static void SetParent (FreeNode* n, FreeNode* p)                { n->Parent   = p; }
        static void SetChild  (FreeNode* n, UPInt i, FreeNode* c)       { n->Child[i] = c; }
        static void ClearLinks(FreeNode* n) { n->Parent = n->Child[0] = n->Child[1] = 0; }
    };

    //--------------------------------------------------------------------
    struct AddrAccessor
    {
        static       UPInt      GetKey     (const FreeNode* n)          { return  UPInt(n); }
        static       FreeNode*  GetChild   (      FreeNode* n, UPInt i) { return  n->AddrChild[i]; }
        static const FreeNode*  GetChild   (const FreeNode* n, UPInt i) { return  n->AddrChild[i]; }
        static       FreeNode** GetChildPtr(      FreeNode* n, UPInt i) { return &n->AddrChild[i]; }

        static       FreeNode*  GetParent  (      FreeNode* n)          { return n->AddrParent; }
        static const FreeNode*  GetParent  (const FreeNode* n)          { return n->AddrParent; }

        static void SetParent (FreeNode* n, FreeNode* p)                { n->AddrParent   = p; }
        static void SetChild  (FreeNode* n, UPInt i, FreeNode* c)       { n->AddrChild[i] = c; }
        static void ClearLinks(FreeNode* n) { n->AddrParent = n->AddrChild[0] = n->AddrChild[1] = 0; }
    };

public:
    //--------------------------------------------------------------------
    enum ReallocResult
    {
        // Don't change these values because the check for successful
        // operation looks like (res < ReallocFailed).
        ReallocSucceeded        = 0,
        ReallocShrinkedAtTail   = 1,
        ReallocFailed           = 2,
        ReallocFailedAtTail     = 3
    };

    GHeapAllocLite(UPInt minSize);

    void  InitSegment(GHeapTreeSeg* seg);
    void  ReleaseSegment(GHeapTreeSeg* seg);

    void* Alloc(UPInt size, UPInt alignSize, GHeapTreeSeg** allocSeg);
    void  Free(GHeapTreeSeg* seg, void* ptr, UPInt size, UPInt alignSize);

    ReallocResult   ReallocInPlace(GHeapTreeSeg* seg, void* ptr, UPInt oldSize, 
                                   UPInt newSize, UPInt alignSize);

    void    Extend(GHeapTreeSeg* seg, UPInt incSize);
    bool    TrimAt(GHeapTreeSeg* seg, UByte* ptrAt);

    UPInt GetMinSize()   const { return MinSize; }
    UPInt GetFreeBytes() const { return FreeBlocks << MinShift; }
    void  VisitMem(GHeapMemVisitor* visitor, GHeapMemVisitor::Category cat) const;
    void  VisitUnused(GHeapSegVisitor* visitor, UInt cat) const;

private:
    //--------------------------------------------------------------------
    typedef GRadixTreeMulti<FreeNode, SizeAccessor>   SizeTreeType;
    typedef GRadixTree     <FreeNode, AddrAccessor>   AddrTreeType;

    //--------------------------------------------------------------------
    void        pushNode(FreeNode* node, GHeapTreeSeg* seg, UPInt blocks);
    void        pullNode(FreeNode* node);
    FreeNode*   pullBest(UPInt blocks);
    FreeNode*   pullBest(UPInt blocks, UPInt alignMask);
    void        splitNode(FreeNode* node, UByte* start, UPInt size);

    void        visitTree(const FreeNode* root, 
                          GHeapSegment* seg,
                          GHeapMemVisitor* visitor,
                          GHeapMemVisitor::Category cat) const;

    void        visitUnusedNode(const FreeNode* node, 
                                GHeapSegVisitor* visitor, 
                                UInt cat) const;

    void        visitUnusedInTree(const FreeNode* root, 
                                  GHeapSegVisitor* visitor, 
                                  UInt cat) const;

    //--------------------------------------------------------------------
    UPInt           MinShift;
    UPInt           MinSize;
    UPInt           MinMask;
    SizeTreeType    SizeTree;
    AddrTreeType    AddrTree;
    UPInt           FreeBlocks;
};

#endif
