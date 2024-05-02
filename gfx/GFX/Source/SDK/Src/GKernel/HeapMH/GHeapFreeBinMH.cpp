/**********************************************************************

Filename    :   GHeapFreeBinMH.cpp
Content     :   
Created     :   2010
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

#include "GHeapTypes.h"
#include "GHeapFreeBinMH.h"

//------------------------------------------------------------------------
GHeapListBinMH::GHeapListBinMH()
{
    Reset();
}

void GHeapListBinMH::Reset()
{
    Mask = 0;
    memset(Roots, 0, sizeof(Roots));
}

//--------------------------------------------------------------------
GINLINE void GHeapListBinMH::pushNode(UPInt idx, GHeapBinNodeMH* node)
{
    GHeapBinNodeMH* root = Roots[idx];
    if (root)
    {
        node->SetPrev(root);                // node->pPrev = root;
        node->SetNext(root->GetNext());     // node->pNext = root->pNext;
        root->GetNext()->SetPrev(node);     // root->pNext->pPrev = node;
        root->SetNext(node);                // root->pNext = node;
    }
    else
    {
        node->SetPrev(node);                // node->pPrev = node;
        node->SetNext(node);                // node->pNext = node;
    }
    Roots[idx] = node;
    Mask |= UPInt(1) << idx;
}


//--------------------------------------------------------------------
GINLINE void GHeapListBinMH::pullNode(UPInt idx, GHeapBinNodeMH* node)
{
    GHeapBinNodeMH* root = Roots[idx];
    if (node != root)
    {
        node->GetPrev()->SetNext(node->GetNext());  // node->pPrev->pNext = node->pNext;
        node->GetNext()->SetPrev(node->GetPrev());  // node->pNext->pPrev = node->pPrev;
        return;
    }
    if (root != root->GetNext())
    {
        Roots[idx] = root->GetNext();
        node->GetPrev()->SetNext(node->GetNext());  // node->pPrev->pNext = node->pNext;
        node->GetNext()->SetPrev(node->GetPrev());  // node->pNext->pPrev = node->pPrev;
        return;
    }
    Roots[idx] = 0;
    Mask &= ~(UPInt(1) << idx);
}



//--------------------------------------------------------------------
UByte* GHeapListBinMH::GetAlignedPtr(UByte* start, UPInt alignMask)
{
    UPInt aligned = (UPInt(start) + alignMask) & ~alignMask;
    return (UByte*)aligned;
}

//--------------------------------------------------------------------
GINLINE bool GHeapListBinMH::AlignmentIsOK(const GHeapBinNodeMH* node, UPInt blocks, UPInt alignMask)
{
    UPInt  bytes   = blocks << GHeapPageMH::UnitShift;
    UPInt  aligned = UPInt(GetAlignedPtr((UByte*)node, alignMask));
    return aligned + bytes <= UPInt(node) + node->GetBytes();
}

//--------------------------------------------------------------------
GHeapBinNodeMH* GHeapListBinMH::findAligned(GHeapBinNodeMH* root, UPInt blocks, UPInt alignMask)
{
    GHeapBinNodeMH* node = root;
    if (node) do
    {
        if (AlignmentIsOK(node, blocks, alignMask))
        {
            return node;
        }
        node = node->GetNext();
    }
    while(node != root);
    return 0;
}


//------------------------------------------------------------------------
GHeapBinNodeMH* GHeapListBinMH::PullBest(UPInt blocks)
{
    UPInt idx = G_Min<UPInt>(blocks-1, BinSize-1);
    GHeapBinNodeMH* best = 0;
    UPInt bits = Mask >> idx;
    if (bits)
    {
        // Go straight to the first not empty list. No searching loops.
        //----------------------
        UPInt i = idx + GHeapGetLowerBit(bits);
        best = Roots[i];
        GASSERT(best);
        pullNode(i, best);
    }
    return best;
}


//--------------------------------------------------------------------
GHeapBinNodeMH* GHeapListBinMH::PullBest(UPInt blocks, UPInt alignMask)
{
    UPInt idx = G_Min<UPInt>(blocks-1, BinSize-1);
    GHeapBinNodeMH* best = 0;
    UPInt bits = Mask >> idx;
    if (bits)
    {
        // Go straight to the first not empty list. No searching loops.
        //----------------------
        UPInt i = idx + GHeapGetLowerBit(bits);
        GASSERT(Roots[i]);

        // TO DO: Can be optimized by skipping empty roots 
        // according to the Mask.
        do
        {
            best = findAligned(Roots[i], blocks, alignMask);
            if (best)
            {
                pullNode(i, best);
                break;
            }
        }
        while(++i < BinSize);
    }
    return best;
}


// The node must be a fully constructed BinLNode
//------------------------------------------------------------------------
void GHeapListBinMH::Push(UByte* node)
{
    GHeapBinNodeMH* tn = (GHeapBinNodeMH*)node;
    UPInt blocks = tn->GetBlocks();
    pushNode(G_Min<UPInt>(blocks-1, BinSize-1), tn);
}


//------------------------------------------------------------------------
void GHeapListBinMH::Pull(UByte* node)
{
    GHeapBinNodeMH* tn = (GHeapBinNodeMH*)node;
    UPInt blocks = tn->GetBlocks();
    pullNode(G_Min<UPInt>(blocks-1, BinSize-1), tn);
}


//--------------------------------------------------------------------
GINLINE GHeapBinNodeMH* GHeapListBinMH::getPrevAdjacent(UByte* node) const
{
    // Get the previous adjacent free block. If there's 
    // an empty space in the bit-set it means that the previous
    // adjacent block is empty and valid. This statement simply
    // reads the length from the end of the previous adjacent 
    // block and subtracts the respective size from the "node" pointer.
    //------------------------
    return (GHeapBinNodeMH*)(node - ((GHeapBinNodeMH*)node)->GetPrevBytes());
}

//--------------------------------------------------------------------
GINLINE GHeapBinNodeMH* GHeapListBinMH::getNextAdjacent(UByte* node) const
{ 
    // Get the next adjacent free block. If there's 
    // an empty space in the bit-set it means that the next
    // adjacent block is empty and valid. It just takes
    // the size of the node and adds the respective size to 
    // the "node" pointer.
    //------------------------
    return (GHeapBinNodeMH*)(node + ((GHeapBinNodeMH*)node)->GetBytes());
}

// The "node" must be a fully constructed BinLNode. Use MakeNode().
//------------------------------------------------------------------------
void GHeapListBinMH::Merge(UByte* node, UPInt bytes, bool left, bool right, GHeapPageMH* page)
{
    GHeapBinNodeMH* mergeNode = (GHeapBinNodeMH*)node;
    UPInt      mergeSize = bytes >> GHeapPageMH::UnitShift;

    mergeNode->SetBlocks(mergeSize);
    if (left)
    {
        mergeNode  = getPrevAdjacent(node);
        mergeSize += mergeNode->GetBlocks();
        Pull((UByte*)mergeNode);
    }
    if (right)
    {
        GHeapBinNodeMH* nextAdjacent = getNextAdjacent(node);
        mergeSize += nextAdjacent->GetBlocks();
        Pull((UByte*)nextAdjacent);
    }
    mergeNode->SetBlocks(mergeSize);
    mergeNode->SetPage(page);
    pushNode(G_Min<UPInt>(mergeSize-1, BinSize-1), mergeNode);
}

