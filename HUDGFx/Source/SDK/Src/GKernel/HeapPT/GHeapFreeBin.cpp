/**********************************************************************

Filename    :   GHeapFreeBin.h
Content     :   
Created     :   2009
Authors     :   Maxim Shemanarev, Boris Rayskiy

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

#include "GHeapFreeBin.h"
#include "GHeapBitSet2.h"
#include "GMemoryHeapPT.h"



//------------------------------------------------------------------------
GHeapListBin::GHeapListBin()
{
    Reset();
}
void GHeapListBin::Reset()
{
    Mask = 0;
    memset(Roots, 0, sizeof(Roots));
}


//------------------------------------------------------------------------
GHeapTreeBin::GHeapTreeBin()
{
    Reset();
}
void GHeapTreeBin::Reset()
{
    Mask = 0;
    memset(Roots, 0, sizeof(Roots));
}

//--------------------------------------------------------------------
UByte* GHeapListBin::GetAlignedPtr(UByte* start, UPInt alignMask)
{
    UPInt aligned = (UPInt(start) + alignMask) & ~alignMask;
    UPInt head = aligned - UPInt(start);
    while (head && head < GHeap_MinSize)
    {
        aligned += alignMask+1;
        head    += alignMask+1;
    }
    return (UByte*)aligned;
}

//--------------------------------------------------------------------
bool GHeapListBin::AlignmentIsOK(const GHeapLNode* node, UPInt blocks,
                                 UPInt shift, UPInt alignMask)
{
    UPInt  bytes   = blocks << shift;
    UPInt  aligned = UPInt(GetAlignedPtr((UByte*)node, alignMask));
    return aligned + bytes <= UPInt(node) + GHeapFreeBin::GetBytes(node, shift);
}


//--------------------------------------------------------------------
GINLINE void GHeapListBin::PushNode(UPInt idx, GHeapLNode* node)
{
    GHeapLNode* root = Roots[idx];
    if (root)
    {
        node->pPrev = root;
        node->pNext = root->pNext;
        root->pNext->pPrev = node;
        root->pNext = node;
    }
    else
    {
        node->pPrev = node->pNext = node;
    }
    Roots[idx] = node;
    Mask |= UPInt(1) << idx;
}

//--------------------------------------------------------------------
GINLINE void GHeapListBin::PullNode(UPInt idx, GHeapLNode* node)
{
    GHeapLNode* root = Roots[idx];
    if (node != root)
    {
        node->pPrev->pNext = node->pNext;
        node->pNext->pPrev = node->pPrev;
        return;
    }
    if (root != root->pNext)
    {
        Roots[idx] = root->pNext;
        node->pPrev->pNext = node->pNext;
        node->pNext->pPrev = node->pPrev;
        return;
    }
    Roots[idx] = 0;
    Mask &= ~(UPInt(1) << idx);
}

//--------------------------------------------------------------------
GINLINE GHeapLNode* GHeapListBin::PullBest(UPInt idx)
{
    GHeapLNode* best = 0;
    UPInt bits = Mask >> idx;
    if (bits)
    {
        // Go straight to the first not empty list. No searching loops.
        //----------------------
        UPInt i = idx + GHeapGetLowerBit(bits);
        best = Roots[i];
        GHEAP_ASSERT(best);
        PullNode(i, best);
    }
    return best;
}

//--------------------------------------------------------------------
GHeapLNode* GHeapListBin::FindAligned(GHeapLNode* root, UPInt blocks, 
                                      UPInt shift, UPInt alignMask)
{
    GHeapLNode* node = root;
    if (node) do
    {
        if (AlignmentIsOK(node, blocks, shift, alignMask))
        {
            return node;
        }
        node = node->pNext;
    }
    while(node != root);
    return 0;
}

//--------------------------------------------------------------------
GHeapLNode* GHeapListBin::PullBest(UPInt idx, UPInt blocks, 
                                   UPInt shift, UPInt alignMask)
{
    GHeapLNode* best = 0;
    UPInt bits = Mask >> idx;
    if (bits)
    {
        // Go straight to the first not empty list. No searching loops.
        //----------------------
        UPInt i = idx + GHeapGetLowerBit(bits);
        GHEAP_ASSERT(Roots[i]);

        // TO DO: Can be optimized by skipping empty roots 
        // according to the Mask.
        do
        {
            best = FindAligned(Roots[i], blocks, shift, alignMask);
            if (best)
            {
                PullNode(i, best);
                break;
            }
        }
        while(++i < BinSize);
    }
    return best;
}




// The TreeBin uses ideas from Doug Lea malloc (aka dlmalloc). 
// dlmalloc has been released to the public domain, as explained at
// http://creativecommons.org/licenses/publicdomain. The tree used 
// there is well proven and tested during years. It's compact and fast.
// Dlmalloc is great masterpiece of public domain software, so, there 
// is the acknowlwgwment: http://g.oswego.edu/ 
// Or just google for "Doug Lea malloc".
//--------------------------------------------------------------------




//--------------------------------------------------------------------
GINLINE UPInt GHeapTreeBin::ShiftForTreeIndex(UPInt i)
{
    // Computes a left shift for the tree.
    return (i >= BinSize-1) ? 0 : (BinSize-1) - ((i >> 1) + SizeShift - 2);
}


//--------------------------------------------------------------------
GINLINE UPInt GHeapTreeBin::ComputeTreeIndex(UPInt blocks)
{
    UPInt piles = blocks >> SizeShift;

    if (piles == 0)
        return 0;

    if (piles > SizeLimit)
        return BinSize-1;

    // This is a Doug Lea voodoo that most probably computes the index
    // according to geometric progression with the sqrt(2) coefficient.
    //------------------------
    UPInt   pow2 = GHeapGetUpperBit(piles);
    return (pow2 << 1) + ((blocks >> (pow2 + (SizeShift - 1))) & 1);
}

//------------------------------------------------------------------------
void GHeapTreeBin::PushNode(GHeapTNode* node)
{
    UPInt size = node->Size;
    UPInt idx  = ComputeTreeIndex(size);

    GHeapTNode** root = Roots + idx;

    node->Index = idx;
    node->Child[0] = node->Child[1] = 0;

    UPInt mask = UPInt(1) << idx;
    if ((Mask & mask) == 0) 
    {
        Mask |= mask;
        *root = node;
        node->Parent = (GHeapTNode*)root;
        node->pNext  = node->pPrev = node;
    }
    else 
    {
        GHeapTNode* head = *root;
        UPInt bits = size << ShiftForTreeIndex(idx);

        for (;;) 
        {
            if (head->Size != size) 
            {
                GHeapTNode** link = &(head->Child[(bits >> (BinSize-1)) & 1]);
                bits <<= 1;

                if (*link != 0)
                {
                    head = *link;
                }
                else
                {
                    *link = node;
                    node->Parent = head;
                    node->pNext  = node->pPrev = node;
                    break;
                }
            }
            else 
            {
                GHeapTNode* next = (GHeapTNode*)head->pNext;
                head->pNext  = next->pPrev = node;
                node->pNext  = next;
                node->pPrev  = head;
                node->Parent = 0;
                break;
            }
        }
    }
}


//------------------------------------------------------------------------
void GHeapTreeBin::PullNode(GHeapTNode* node)
{
    GHeapTNode* parent = node->Parent;
    GHeapTNode* rotor;

    if (node->pPrev != node) 
    {
        GHeapTNode* f = (GHeapTNode*)node->pNext;
        rotor = (GHeapTNode*)node->pPrev;

        f->pPrev = rotor;
        rotor->pNext = f;
    }
    else 
    {
        GHeapTNode** rp;

        if (((rotor = *(rp = &(node->Child[1]))) != 0) || 
            ((rotor = *(rp = &(node->Child[0]))) != 0)) 
        {
            GHeapTNode** cp;
            while ((*(cp = &(rotor->Child[1])) != 0) || 
                   (*(cp = &(rotor->Child[0])) != 0)) 
            {  
                rotor = *(rp = cp);
            }
            *rp = 0;
        }
    }

    if (parent != 0) 
    {
        GHeapTNode** root = Roots + node->Index;

        if (node == *root) 
        {
            if ((*root = rotor) == 0)
            {
                Mask &= ~(UPInt(1) << node->Index);
            }
        }    
        else
        {
            parent->Child[parent->Child[0] != node] = rotor;
        }

        if (rotor != 0)
        {
            GHeapTNode* child;

            rotor->Parent = parent;

            if ((child = node->Child[0]) != 0)
            {
                rotor->Child[0] = child;
                child->Parent   = rotor;
            }

            if ((child = node->Child[1]) != 0)
            {
                rotor->Child[1] = child;
                child->Parent   = rotor;
            }
        }
    }
}


//--------------------------------------------------------------------
GHeapTNode* GHeapTreeBin::FindBest(UPInt size)
{
    GHeapTNode* best = 0;

    UPInt rsize = -SPInt(size); // Unsigned negation
    UPInt idx   = ComputeTreeIndex(size);
    UPInt tail;

    GHeapTNode* node = Roots[idx];

    if (node != 0)
    {
        // Traverse tree for this bin looking for node with node->Size == size
        //--------------------------
        GHeapTNode* rst = 0;  // The deepest untaken right subtree
        UPInt bits = size << ShiftForTreeIndex(idx);
        for (;;)
        {
            GHeapTNode* rt;
            tail = node->Size - size;
            if (tail < rsize)
            {
                best = node;
                if ((rsize = tail) == 0)
                {
                    break;
                }
            }
            rt   = node->Child[1];
            node = node->Child[(bits >> (BinSize-1)) & 1];
            if (rt != 0 && rt != node)
            {
                rst = rt;
            }
            if (node == 0)
            {
                node = rst; // set node to least subtree holding sizes > size
                break;
            }
            bits <<= 1;
        }
    }

    if (node == 0 && best == 0)
    { 
        // Set node to root of next non-empty bin
        //------------------------
        UPInt x = UPInt(1) << (idx + 1);

        // Mask with all bits to left of least bit of x on,
        // and then mask it with "Mask" (Doug Lea voodoo magic).
        //------------------------
        x = (x | -SPInt(x)) & Mask;
        if (x)
        {
            node = Roots[GHeapGetLowerBit(x)];
        }
    }

    while (node)
    { 
        // Find smallest of tree or subtree.
        //------------------------
        tail = node->Size - size;
        if (tail < rsize)
        {
            rsize = tail;
            best  = node;
        }
        node = node->Child[node->Child[0] == 0]; // leftmost child
    }

    GHEAP_ASSERT(best == 0 || best->Size == rsize + size);
    return best;
}


//--------------------------------------------------------------------
GINLINE GHeapTNode* GHeapTreeBin::PullBest(UPInt size)
{
    GHeapTNode* best = FindBest(size);
    if (best)
    {
        best = (GHeapTNode*)best->pNext; // May potentially speed-up slightly.
        PullNode(best);
    }
    return best;
}


//--------------------------------------------------------------------
const GHeapTNode* GHeapTreeBin::FindExactSize(UPInt size) const
{
    UPInt idx = ComputeTreeIndex(size);
    const GHeapTNode* node = Roots[idx];

    // Traverse tree for this bin looking for node with node->Size == size
    //--------------------------
    UPInt bits = size << ShiftForTreeIndex(idx);
    while (node && node->Size != size)
    {
        node = node->Child[(bits >> (BinSize-1)) & 1];
        bits <<= 1;
    }
    return node;
}





//------------------------------------------------------------------------
GHeapFreeBin::GHeapFreeBin() : FreeBlocks(0) {}

void GHeapFreeBin::Reset()
{
    ListBin1.Reset();
    ListBin2.Reset();
    TreeBin.Reset();
    FreeBlocks = 0;
}


//------------------------------------------------------------------------
void GHeapFreeBin::compilerAsserts()
{
    GCOMPILER_ASSERT(GHeap_MinSize >= 4*sizeof(void*));
}

//--------------------------------------------------------------------
GINLINE GHeapLNode* GHeapFreeBin::getPrevAdjacent(UByte* node, UPInt shift)
{
    // Get the previous adjacent free block. If there's 
    // an empty space in the bit-set it means that the previous
    // adjacent block is empty and valid. This statement simply
    // reads the length from the end of the previous adjacent 
    // block and subtracts the respective size from the "node" pointer.
    //------------------------
    UPInt size = *(((const UPIntHalf*)node) - 1);
    if (size >= BinSize+1)
    {
        size = *(((const UPInt*)node) - 2);
    }
    return (GHeapLNode*)(node - (size << shift));
}

//--------------------------------------------------------------------
GINLINE GHeapLNode* GHeapFreeBin::getNextAdjacent(UByte* node, UPInt shift)
{ 
    // Get the next adjacent free block. If there's 
    // an empty space in the bit-set it means that the next
    // adjacent block is empty and valid. It just takes
    // the size of the node and adds the respective size to 
    // the "node" pointer.
    //------------------------
    return (GHeapLNode*)(node + (GetSize(node) << shift));
}









// The node must be a fully constructed GHeapLNode
//------------------------------------------------------------------------
void GHeapFreeBin::Push(UByte* node)
{
    UPInt blocks = GetSize(node);
    FreeBlocks += blocks;
    if (blocks <= 1*BinSize)
    {
        ListBin1.PushNode(blocks-1, (GHeapLNode*)node);
        return;
    }
    if (blocks <= 2*BinSize)
    {
        ListBin2.PushNode(blocks-BinSize-1, (GHeapLNode*)node);
        return;
    }
    TreeBin.PushNode((GHeapTNode*)node);
}


//------------------------------------------------------------------------
void GHeapFreeBin::Pull(UByte* node)
{
    UPInt blocks = GetSize(node);
    FreeBlocks -= blocks;
    if (blocks <= 1*BinSize)
    {
        ListBin1.PullNode(blocks-1, (GHeapLNode*)node);
        return;
    }
    if (blocks <= 2*BinSize)
    {
        ListBin2.PullNode(blocks-BinSize-1, (GHeapLNode*)node);
        return;
    }
    TreeBin.PullNode((GHeapTNode*)node);
}


//------------------------------------------------------------------------
UByte* GHeapFreeBin::PullBest(UPInt blocks)
{
    GHeapLNode* best = 0;
    if (blocks <= 2*BinSize)
    {
        if (blocks <= 1*BinSize && 
           (best = ListBin1.PullBest(blocks-1)) != 0)
        {
            FreeBlocks -= UPInt(best->ShortSize);
            return (UByte*)best;
        }
        UPInt idx = (blocks <= BinSize) ? 0 : blocks-BinSize-1;
        if ((best = ListBin2.PullBest(idx)) != 0)
        {
            FreeBlocks -= ((GHeapTNode*)best)->Size;
            return (UByte*)best;
        }
    }
    best = TreeBin.PullBest(blocks);
    if (best)
    {
        FreeBlocks -= ((GHeapTNode*)best)->Size;
    }
    return (UByte*)best;
}



// Best fit with the given alignment
//------------------------------------------------------------------------
UByte* GHeapFreeBin::PullBest(UPInt blocks, UPInt shift, UPInt alignMask)
{
    if (blocks <= 2*BinSize)
    {
        GHeapLNode* best = 0;
        if (blocks <= 1*BinSize && 
           (best = ListBin1.PullBest(blocks-1, blocks, shift, alignMask)) != 0)
        {
            FreeBlocks -= UPInt(best->ShortSize);
            return (UByte*)best;
        }

        UPInt idx = (blocks <= BinSize) ? 0 : blocks-BinSize-1;
        if ((best = ListBin2.PullBest(idx, blocks, shift, alignMask)) != 0)
        {
            FreeBlocks -= ((GHeapTNode*)best)->Size;
            return (UByte*)best;
        }
    }

    // Slightly slower but more precise version
    GHeapTNode* head;
    GHeapTNode* best;
    UPInt search = blocks;
    while((head = best = TreeBin.FindBest(search)) != 0)
    {
        do
        {
            if (GHeapListBin::AlignmentIsOK(best, blocks, shift, alignMask))
            {
                TreeBin.PullNode(best);
                FreeBlocks -= best->Size;
                return (UByte*)best;
            }
            best = (GHeapTNode*)best->pNext;
        }
        while(best != head);
        search = best->Size + 1;
    }
    return 0;

    //// Slightly faster but less precise version
    //GHeapTNode* root = TreeBin.FindBest(blocks);
    //GHeapTNode* node = root;
    //if (node) do
    //{
    //    if (GHeapListBin::AlignmentIsOK(node, blocks, shift, alignMask))
    //    {
    //        TreeBin.PullNode(node);
    //        FreeBlocks -= node->Size;
    //        return (UByte*)node;
    //    }
    //    node = (GHeapTNode*)node->pNext;
    //}
    //while(node != root);
    //
    //UPInt extra = (alignMask + 1) >> shift;
    //node = TreeBin.FindBest(blocks + extra);
    //
    //// At this point the alignment might be not OK because 
    //// of a small heading block before the aligned memory starts. 
    //// If this block is smaller GHeap_MinSize, we cannot return it.
    ////-------------------------
    //if (node && GHeapListBin::AlignmentIsOK(node, blocks, shift, alignMask))
    //{
    //    TreeBin.PullNode(node);
    //    FreeBlocks -= node->Size;
    //    return (UByte*)node;
    //}
    //
    //// 2*extra must satisfy the alignment requirements for sure. 
    //// Code below has practically zero probability of execution.
    //// Still, theoretically the situation may occur.
    ////-------------------------
    //node = TreeBin.FindBest(blocks + 2*extra);
    //if (node)
    //{
    //    GHEAP_ASSERT(GHeapListBin::AlignmentIsOK(node, blocks, shift, alignMask));
    //    TreeBin.PullNode(node);
    //    FreeBlocks -= node->Size;
    //    return (UByte*)node;
    //}
    //return 0;
}



// The "node" must be a fully constructed GHeapLNode. Use MakeNode().
//------------------------------------------------------------------------
void GHeapFreeBin::Merge(UByte* node, UPInt shift, bool left, bool right)
{
    UPInt       mergeSize = GetSize(node);
    GHeapLNode* mergeNode = (GHeapLNode*)node;
    if (left)
    {
        mergeNode  = getPrevAdjacent(node, shift);
        mergeSize += GetSize(mergeNode);
        Pull((UByte*)mergeNode);
#ifdef GHEAP_CHECK_CORRUPTION
        CheckNode(mergeNode, shift);
#endif
    }
    if (right)
    {
        GHeapLNode* nextAdjacent = getNextAdjacent(node, shift);
        mergeSize += GetSize(nextAdjacent);
        Pull((UByte*)nextAdjacent);
#ifdef GHEAP_CHECK_CORRUPTION
        CheckNode(nextAdjacent, shift);
#endif
    }
    SetSize((UByte*)mergeNode, mergeSize, shift);
#ifdef GHEAP_CHECK_CORRUPTION
    MemsetNode(mergeNode, shift);
#endif
    Push((UByte*)mergeNode);
}


//------------------------------------------------------------------------
void GHeapFreeBin::visitUnusedNode(const GHeapLNode* node, 
                                   GHeapSegVisitor* visitor, 
                                   UPInt shift, UInt cat) const
{
    UPInt start = (UPInt(node) + GHeap_PageMask)        & ~GHeap_PageMask;
    UPInt end   = (UPInt(node) + GetBytes(node, shift)) & ~GHeap_PageMask;
    if (start + GHeap_PageSize <= end)
    {
        visitor->Visit(cat, node->pSegment->pHeap, start, end - start);
    }
}

//------------------------------------------------------------------------
void GHeapFreeBin::visitUnusedInTree(const GHeapTNode* root, 
                                     GHeapSegVisitor* visitor, 
                                     UPInt shift, UInt cat) const
{
    if (root)
    {
        visitUnusedInTree(root->Child[0], visitor, shift, cat);
        const GHeapLNode* node = root;
        do
        {
            visitUnusedNode(node, visitor, shift, cat);
            node = node->pNext;
        }
        while(node != root);
        visitUnusedInTree(root->Child[1], visitor, shift, cat);
    }
}

//------------------------------------------------------------------------
void GHeapFreeBin::VisitUnused(GHeapSegVisitor* visitor, UPInt shift, UInt cat) const
{
    const GHeapLNode* node;
    const GHeapLNode* root;
    for(UPInt i = 0; i < BinSize; ++i)
    {
        node = root = ListBin1.Roots[i];
        if (root && GetBytes(root, shift) >= GHeap_PageSize) do
        {
            visitUnusedNode(node, visitor, shift, cat);
            node = node->pNext;
        }
        while(node != root);

        node = root = ListBin2.Roots[i];
        if (root && GetBytes(root, shift) >= GHeap_PageSize) do
        {
            visitUnusedNode(node, visitor, shift, cat);
            node = node->pNext;
        }
        while(node != root);

        visitUnusedInTree(TreeBin.Roots[i], visitor, shift, cat);
    }
}


//------------------------------------------------------------------------
void GHeapFreeBin::visitTree(const GHeapTNode* root, 
                             GHeapMemVisitor* visitor,
                             UPInt shift, 
                             GHeapMemVisitor::Category cat) const
{
    if (root)
    {
        visitTree(root->Child[0], visitor, shift, cat);
        const GHeapTNode* node = root;
        do
        {
            visitor->Visit(node->pSegment, UPInt(node), 
                           node->Size << shift, cat);
            node = (GHeapTNode*)node->pNext;
        }
        while(node != root);
        visitTree(root->Child[1], visitor, shift, cat);
    }
}


//------------------------------------------------------------------------
void GHeapFreeBin::VisitMem(GHeapMemVisitor* visitor, UPInt shift,
                            GHeapMemVisitor::Category cat) const
{
    UPInt i;
    const GHeapLNode* node;
    const GHeapLNode* root;
    for(i = 0; i < BinSize; ++i)
    {
        node = root = ListBin1.Roots[i];
        if (root) do
        {
            visitor->Visit(node->pSegment, UPInt(node), 
                           UPInt(node->ShortSize) << shift, cat);
            node = node->pNext;
        }
        while(node != root);

        node = root = ListBin2.Roots[i];
        if (root) do
        {
            visitor->Visit(node->pSegment, UPInt(node), 
                           GetSize(node) << shift, cat);
            node = node->pNext;
        }
        while(node != root);

        visitTree(TreeBin.Roots[i], visitor, shift, cat);
    }
}


#ifdef GFC_BUILD_DEBUG

//------------------------------------------------------------------------
void GHeapFreeBin::checkBlockIntegrity(const GHeapLNode* node, UPInt shift) const
{
#ifdef GHEAP_CHECK_CORRUPTION
    CheckNode(node, shift);
#endif
    const GHeapSegment* seg = node->pSegment;
    UPInt   blocks = GetSize(node);
    GHEAP_ASSERT(blocks);
    UInt32* bitSet = GetBitSet(seg);
    UPInt   start  = (((UByte*)node) - seg->pData) >> shift;
    GHEAP_ASSERT(GHeapBitSet2::GetValue(bitSet, start) == 0);
    GHEAP_ASSERT(GHeapBitSet2::GetValue(bitSet, start + blocks - 1) == 0);
}


//------------------------------------------------------------------------
void GHeapFreeBin::checkTreeIntegrity(const GHeapTNode* root, UPInt shift) const
{
    if (root)
    {
        checkTreeIntegrity(root->Child[0], shift);
        const GHeapTNode* node = root;
        do
        {
            checkBlockIntegrity(node, shift);
            node = (GHeapTNode*)node->pNext;
        }
        while(node != root);
        checkTreeIntegrity(root->Child[1], shift);
    }
}


// The function is very slow, but in some extreme cases may be handy. 
//------------------------------------------------------------------------
void GHeapFreeBin::CheckIntegrity(UPInt shift) const
{
    UPInt i;
    const GHeapLNode* node;
    const GHeapLNode* root;
    for(i = 0; i < BinSize; ++i)
    {
        node = root = ListBin1.Roots[i];
        if (root) do
        {
            checkBlockIntegrity(node, shift);
            node = node->pNext;
        }
        while(node != root);

        node = root = ListBin2.Roots[i];
        if (root) do
        {
            checkBlockIntegrity(node, shift);
            node = node->pNext;
        }
        while(node != root);

        checkTreeIntegrity(TreeBin.Roots[i], shift);
    }
}

#else // GFC_BUILD_DEBUG

void GHeapFreeBin::checkBlockIntegrity(const GHeapLNode*, UPInt) const {}
void GHeapFreeBin::checkTreeIntegrity(const GHeapTNode*, UPInt) const {}
void GHeapFreeBin::CheckIntegrity(UPInt) const {}

#endif // GFC_BUILD_DEBUG


#ifdef GHEAP_CHECK_CORRUPTION

//------------------------------------------------------------------------
void GHeapFreeBin::CheckArea(const void* area, UPInt bytes) // static
{
    for(UPInt i = 0; i < bytes; ++i)
    {
        GHEAP_ASSERT(((const UByte*)area)[i] == 0xFE);
    }
}

//------------------------------------------------------------------------
void GHeapFreeBin::MemsetNode(GHeapLNode* node, UPInt shift) // static
{
    GHeapSegment* seg = node->pSegment;
    UPInt blocks = GetSize(node);
    memset(node, 0xFE, blocks << shift);
    SetSize((UByte*)node, blocks, shift);
    node->pSegment = seg;
}

//------------------------------------------------------------------------
void GHeapFreeBin::CheckNode(const GHeapLNode* node, UPInt shift) // static
{
    UPInt blocks = GetSize(node);
    UPInt bytes  = blocks << shift;
    const UByte* area  = (const UByte*)node;
    GHEAP_ASSERT(bytes >= GHeap_MinSize);
    if (bytes == GHeap_MinSize)
    {
        GHEAP_ASSERT(node->ShortSize == GHeap_MinSize / (UPInt(1) << shift));
        GHEAP_ASSERT(node->Filler == GHeap_MinSize / (UPInt(1) << shift));
        return;
    }
    if (blocks <= 1*BinSize)
    {
        GHEAP_ASSERT(blocks == *(const UPIntHalf*)(area + bytes - sizeof(UPIntHalf)));
        CheckArea(&node->Filler, bytes - 4*sizeof(void*));
        return;
    }
    const GHeapTNode* tnode = (const GHeapTNode*)node;
    GHEAP_ASSERT(tnode->ShortSize == BinSize+1);
    GHEAP_ASSERT(*(const UPIntHalf*)(area + bytes - sizeof(UPIntHalf)) == BinSize+1);
    GHEAP_ASSERT(blocks == *(const UPInt*)(area + bytes - 2*sizeof(UPInt)));
    CheckArea(&tnode->Filler, sizeof(tnode->Filler));
    CheckArea(area + bytes - sizeof(UPInt), sizeof(tnode->Filler));
    if (blocks <= 2*BinSize)
    {
        CheckArea(&tnode->Parent, bytes - 7*sizeof(void*));
    }
    else
    {
        CheckArea(area + sizeof(GHeapTNode), bytes - 11*sizeof(void*));
    }
}

#endif //GHEAP_CHECK_CORRUPTION
