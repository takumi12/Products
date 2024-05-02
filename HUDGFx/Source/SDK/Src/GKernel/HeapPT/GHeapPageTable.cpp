/**********************************************************************

Filename    :   GHeapPageTable.cpp
Content     :   Allocator headers describing allocator data structures.
            :   
Created     :   July 14, 2008
Authors     :   Michael Antonov, Boris Rayskiy, Maxim Shemanarev

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GHeapPageTable.h"


//------------------------------------------------------------------------
UPInt GHeapPageTableMem[(sizeof(GHeapPageTable) + sizeof(UPInt) - 1) / sizeof(UPInt)];
GHeapPageTable* GHeapGlobalPageTable = 0;

GHeapPageTable::GHeapPageTable() : pStarter(0) 
{
    GHEAP_ASSERT(GHeapGlobalPageTable == 0);
    GHeapGlobalPageTable = this;
}

void GHeapPageTable::Init()
{
    ::new(GHeapPageTableMem) GHeapPageTable;
}


// Maps an address range by allocating all of the headers for it.
// Input:
//      mem             - start address of the heap segment.
//      size            - Heap size in bytes.
// Returns true if headers of Lv2 are successfully mapped, false
// if allocation failed.
//------------------------------------------------------------------------
bool GHeapPageTable::MapRange(void* mem, UPInt size)
{
    UPInt address = (UPInt)mem;

#ifndef  GFC_64BIT_POINTERS

     GHEAP_ASSERT(((address + size) >> GHeap_Lv2_PageShift) < GHeap_Lv2_TableSize);
   
    // Do mapping of addresses in two steps:
    //  1. Allocate the node tables needed, roll back if
    //     there was a memory error.
    //  2. Assign segment pointers into all memory range nodes.
    
    UPInt startIndex = address >> GHeap_Root_PageShift;    
    // We subtract 1 from end index and do inclusive iteration to make sure the last
    // address is mapped correctly if it didn't end on the border of the root node.
    UPInt endIndex   = (address + size - 1) >> GHeap_Root_PageShift;    

    // Iterate over root pages AddRef/Allocate nested tables as necessary.
    for (UPInt i = startIndex; i <= endIndex; i++)
    {
        if (!RootTable[i].AddRef(pStarter))
        {
            // If failed, roll back and release mapped pages.
            for(; i > startIndex; --i)
                RootTable[i-1].Release(pStarter);
            return false;
        }
    }

#else // GFC_64BIT_POINTERS
    
    UPInt startIndex5 = address >> GHeap_Lv5_PageShift;
    UPInt endIndex5   = (address + size - 1) >> GHeap_Lv5_PageShift;

    for (UPInt i5 = startIndex5; i5 <= endIndex5; ++i5)
    {
        GHeapHeader5* table5 = RootTable + i5;

        if(!table5->AddRef(pStarter))
        {
            UPInt badSize = (i5 - startIndex5) << GHeap_Lv5_PageShift;
            UnmapRange(mem, badSize);
            return false;
        }

        UPInt i4 = 0;
        UPInt endIndex4 = GHeap_Lv4_TableSize - 1;

        if (i5 == startIndex5)
            i4 = (address & GHeap_Lv5_PageMask) >> GHeap_Lv4_PageShift;
        if (i5 == endIndex5)
            endIndex4 = ((address + size - 1) & GHeap_Lv5_PageMask) >> GHeap_Lv4_PageShift;

        UPInt startIndex4 = i4;

        for (; i4 <= endIndex4; ++i4)
        {
            GHeapHeader4* table4 = table5->GetTable(i4);

            if (!table4->AddRef(pStarter))
            {
                UPInt badSize = ((i5 - startIndex5) << GHeap_Lv5_PageShift) + 
                                ((i4 - startIndex4) << GHeap_Lv4_PageShift);
                UnmapRange(mem, badSize);
                return false;
            }
            
            UPInt i3 = 0;
            UPInt endIndex3 = GHeap_Lv3_TableSize - 1;

            if (i4 == startIndex4)
                i3 = (address & GHeap_Lv4_PageMask) >> GHeap_Lv3_PageShift;
            if (i4 == endIndex4)
                endIndex3 = ((address + size - 1) & GHeap_Lv4_PageMask) >> GHeap_Lv3_PageShift;

            UPInt startIndex3 = i3;

            for (; i3 <= endIndex3; ++i3)
            {
                GHeapHeader3* table3 = table4->GetTable(i3);

                if (!table3->AddRef(pStarter))
                {
                    UPInt badSize = ((i5 - startIndex5) << GHeap_Lv5_PageShift) + 
                                    ((i4 - startIndex4) << GHeap_Lv4_PageShift) +
                                    ((i3 - startIndex3) << GHeap_Lv3_PageShift);
                    UnmapRange(mem, badSize);
                    return false;
                }

                UPInt i2 = 0;
                UPInt endIndex2 = GHeap_Lv2_TableSize - 1;

                if (i3 == startIndex3)
                    i2 = (address & GHeap_Lv3_PageMask) >> GHeap_Lv2_PageShift;
                if (i3 == endIndex3)
                    endIndex2 = ((address + size - 1) & GHeap_Lv3_PageMask) >> GHeap_Lv2_PageShift;

                UPInt startIndex2 = i2;

                for (; i2 <= endIndex2; ++i2)
                {
                    GHeapHeader2* table2 = table3->GetTable(i2);

                    if (!table2->AddRef(pStarter))
                    {
                        UPInt badSize = ((i5 - startIndex5) << GHeap_Lv5_PageShift) + 
                                        ((i4 - startIndex4) << GHeap_Lv4_PageShift) +
                                        ((i3 - startIndex3) << GHeap_Lv3_PageShift) +
                                        ((i2 - startIndex2) << GHeap_Lv2_PageShift);
                        UnmapRange(mem, badSize);
                        return false;
                    }
                }
            }
        }
    }

#endif // GFC_64BIT_POINTERS 

    return true;

}


// Unmaps a memory range mapped by MapRange. 
//------------------------------------------------------------------------
void GHeapPageTable::UnmapRange(void* mem, UPInt size)
{
    UPInt address = (UPInt)mem;

#ifndef GFC_64BIT_POINTERS

    GHEAP_ASSERT(((address + size) >> GHeap_Lv2_PageShift) < GHeap_Lv2_TableSize);
     
    UPInt startIndex = address >> GHeap_Root_PageShift;
    UPInt endIndex   = (address + size - 1) >> GHeap_Root_PageShift;

    // Iterate over root pages AddRef/Allocate nested tables as necessary.
    for (UPInt i = startIndex; i <= endIndex; i++)
        RootTable[i].Release(pStarter);

#else // GFC_64BIT_POINTERS

    GHEAP_ASSERT(address != NULL);
    GHEAP_ASSERT(((address + size) >> GHeap_Lv5_PageShift) < GHeap_Lv5_TableSize);

    UPInt startIndex5 = address >> GHeap_Lv5_PageShift;
    UPInt endIndex5   = (address + size - 1) >> GHeap_Lv5_PageShift;

    for (UPInt i5 = startIndex5; i5 <= endIndex5; ++i5)
    {
        GHeapHeaderRoot* table5 = RootTable + i5;

        UPInt i4 = 0;
        UPInt endIndex4 = GHeap_Lv4_TableSize - 1;

        if (i5 == startIndex5)
            i4 = (address & GHeap_Lv5_PageMask) >> GHeap_Lv4_PageShift;
        if (i5 == endIndex5)
            endIndex4 = ((address + size - 1) & GHeap_Lv5_PageMask) >> GHeap_Lv4_PageShift;

        UPInt startIndex4 = i4;

        for (; i4 <= endIndex4; ++i4)
        {
            GHeapHeader4* table4 = table5->GetTable(i4);

            UPInt i3 = 0;
            UPInt endIndex3 = GHeap_Lv3_TableSize - 1;

            if (i4 == startIndex4)
                i3 = (address & GHeap_Lv4_PageMask) >> GHeap_Lv3_PageShift;
            if (i4 == endIndex4)
                endIndex3 = ((address + size - 1) & GHeap_Lv4_PageMask) >> GHeap_Lv3_PageShift;

            UPInt startIndex3 = i3;

            for (; i3 <= endIndex3; ++i3)
            {
                GHeapHeader3* table3 = table4->GetTable(i3);

                UPInt i2 = 0;
                UPInt endIndex2 = GHeap_Lv2_TableSize - 1;

                if (i3 == startIndex3)
                    i2 = (address & GHeap_Lv3_PageMask) >> GHeap_Lv2_PageShift;
                if (i3 == endIndex3)
                    endIndex2 = ((address + size - 1) & GHeap_Lv3_PageMask) >> GHeap_Lv2_PageShift;

                for (; i2 <= endIndex2; ++i2)
                {
                    GHeapHeader2* table2 = table3->GetTable(i2);

                    table2->Release(pStarter);
                }
                table3->Release(pStarter);
            }
            table4->Release(pStarter);
        }
        table5->Release(pStarter);
    }

#endif // GFC_64BIT_POINTERS

}


// Remaps memory range from old size of segment to new one.
bool GHeapPageTable::RemapRange(void* mem, UPInt newSize, UPInt oldSize)
{
    GHEAP_ASSERT(oldSize);
    GHEAP_ASSERT(newSize);

    if (newSize == oldSize)
        return true;

#ifndef GFC_64BIT_POINTERS

    UPInt address = (UPInt)mem;

    // Grow.
    if (newSize > oldSize)
    {
        UPInt startIndex = (address + oldSize - 1) >> GHeap_Root_PageShift;
        UPInt endIndex   = (address + newSize - 1) >> GHeap_Root_PageShift;

        for (UPInt i = startIndex + 1; i <= endIndex; i++)
        {
            if (!RootTable[i].AddRef(pStarter))
            {
                for(--i; i > startIndex; --i)
                    RootTable[i].Release(pStarter);
                return false;
            }
        }
        SetSegmentInRange(UPInt(mem)+oldSize, newSize-oldSize, GetSegment(UPInt(mem)));
    }
    // Shrink.
    else
    {
        UPInt startIndex = (address + newSize - 1) >> GHeap_Root_PageShift;
        UPInt endIndex   = (address + oldSize - 1) >> GHeap_Root_PageShift;;

        for (UPInt i = startIndex + 1; i <= endIndex; i++)
            RootTable[i].Release(pStarter);
    }
    return true;

#else // GFC_64BIT_POINTERS

    GHeapSegment* seg = GetSegment(UPInt(mem));
#ifdef GHEAP_DEBUG_INFO
    GHeapDebugNode* node = GetDebugNode(UPInt(mem));
#endif

    bool  ret  = false;
    UPInt size = oldSize;
    UnmapRange(mem, oldSize);
    if (MapRange(mem, newSize))
    {
        size = newSize;
        ret  = true;
    }
    else
    {
        bool ok = MapRange(mem, oldSize);
        GUNUSED(ok);
        GHEAP_ASSERT(ok); // Must succeed!
    }
    SetSegmentInRange(UPInt(mem), size, seg);
#ifdef GHEAP_DEBUG_INFO
    SetDebugNode(UPInt(mem), node);
#endif
    return ret;

#endif // GFC_64BIT_POINTERS
}


// Helper function used to assign a range of 'seg' pointers.
//------------------------------------------------------------------------
void GHeapPageTable::SetSegmentInRange(UPInt address, UPInt size,
                                       GHeapSegment* seg)
{
    GHEAP_ASSERT(seg != NULL);
    
#ifndef GFC_64BIT_POINTERS

    UPInt startIndex = address >> GHeap_Root_PageShift;
    UPInt endIndex   = (address + size - 1) >> GHeap_Root_PageShift;

    // Set segment pointers within the papped node descriptors.
    for (UPInt i = startIndex; i <= endIndex; i++)
    {        
        UPInt j    = 0; 
        UPInt jend = GHeap_Lv1_TableSize-1; 

        if (i == startIndex)
            j = (address & GHeap_Lv2_PageMask) >> GHeap_Lv1_PageShift;
        if (i == endIndex)
            jend = ((address+size-1) & GHeap_Lv2_PageMask) >> GHeap_Lv1_PageShift;


        GHeapHeader1* table = RootTable[i].GetTable();

        while(j <= jend)
        {
            table[j++].pSegment = seg;
        }
    }

#else //  GFC_64BIT_POINTERS


    // DBG Working but slow.
    /*
    for (UPInt a = address; a < (address + size); a += 4096)
    {

        UPInt i5 = a >> GHeap_Lv5_PageShift;
        UPInt i4 = (a & GHeap_Lv5_PageMask) >> GHeap_Lv4_PageShift;
        UPInt i3 = (a & GHeap_Lv4_PageMask) >> GHeap_Lv3_PageShift;
        UPInt i2 = (a & GHeap_Lv3_PageMask) >> GHeap_Lv2_PageShift;
        UPInt i1 = (a & GHeap_Lv2_PageMask) >> GHeap_Lv1_PageShift;

        GHeapHeaderRoot*  rootHeader = RootTable + i5;

        rootHeader->GetTable(i4)->GetTable(i3)->GetTable(i2)->GetTable(i1)->pSegment = seg;
    }
    */

    UPInt lastAddr = address + size - 1;

    UPInt startIndex5 = address >> GHeap_Lv5_PageShift;
    UPInt endIndex5   = lastAddr >> GHeap_Lv5_PageShift;

    for (UPInt i5 = startIndex5; i5 <= endIndex5; ++i5)
    {
        const GHeapHeader5* table5 = RootTable + i5;

        UPInt startIndex4   = 0;
        UPInt endIndex4     = GHeap_Lv4_TableSize - 1;

        if (i5 == startIndex5)
            startIndex4 = (address & GHeap_Lv5_PageMask) >> GHeap_Lv4_PageShift;

        if (i5 == endIndex5)
            endIndex4   = (lastAddr & GHeap_Lv5_PageMask) >> GHeap_Lv4_PageShift;

        for (UPInt i4 = startIndex4; i4 <= endIndex4; ++i4)
        {
            const GHeapHeader4* table4 = table5->GetTable(i4);

            UPInt startIndex3   = 0;
            UPInt endIndex3     = GHeap_Lv3_TableSize - 1;

            if ((i4 == startIndex4) && (i5 == startIndex5))
                startIndex3 = (address & GHeap_Lv4_PageMask) >> GHeap_Lv3_PageShift;

            if ((i4 == endIndex4) && (i5 == endIndex5))
                endIndex3   = (lastAddr & GHeap_Lv4_PageMask) >> GHeap_Lv3_PageShift;

            for (UPInt i3 = startIndex3; i3 <= endIndex3; ++i3)
            {
                const GHeapHeader3* table3 = table4->GetTable(i3);

                UPInt startIndex2   = 0;
                UPInt endIndex2     = GHeap_Lv2_TableSize - 1;

                if ((i3 == startIndex3) && (i4 == startIndex4) && (i5 == startIndex5))
                    startIndex2 = (address & GHeap_Lv3_PageMask) >> GHeap_Lv2_PageShift;

                if ((i3 == endIndex3) && (i4 == endIndex4) && (i5 == endIndex5))
                    endIndex2   = (lastAddr & GHeap_Lv3_PageMask) >> GHeap_Lv2_PageShift;

                for (UPInt i2 = startIndex2; i2 <= endIndex2; ++i2)
                {
                    const GHeapHeader2* table2 = table3->GetTable(i2);

                    UPInt startIndex1   = 0;
                    UPInt endIndex1     = GHeap_Lv1_TableSize - 1;

                    if ((i2 == startIndex2) && (i3 == startIndex3) && (i4 == startIndex4) && (i5 == startIndex5))
                        startIndex1 = (address & GHeap_Lv2_PageMask) >> GHeap_Lv1_PageShift;
                    if ((i2 == endIndex2) && (i3 == endIndex3) && (i4 == endIndex4) && (i5 == endIndex5))
                        endIndex1   = (lastAddr & GHeap_Lv2_PageMask) >> GHeap_Lv1_PageShift;

                    for (UPInt i1 = startIndex1; i1 <= endIndex1; ++i1)
                    {
                        GHeapHeader1* table1 = table2->GetTable(i1);
                        table1->pSegment = seg;
                    }
                }
            }
        }
    }

#endif

}




//------------------------------------------------------------------------
#ifdef GHEAP_DEBUG_INFO

//------------------------------------------------------------------------
void GHeapPageTable::SetDebugNode(UPInt address, GHeapDebugNode* node)
{
#ifndef GFC_64BIT_POINTERS

    UPInt             rootIndex  = address >> GHeap_Root_PageShift;
    GHeapHeaderRoot*  rootHeader = RootTable + rootIndex;

    if (rootHeader->HasTable())
    {
        UPInt i1 = (address & GHeap_Lv2_PageMask) >> GHeap_Lv1_PageShift;
        rootHeader->GetTable()[i1].pDebugNode = node;
    }

#else

    UPInt i5  = address >> GHeap_Root_PageShift;
    GHeapHeaderRoot*  rootHeader = RootTable + i5;

    if (rootHeader->HasTable(address))
    {
        UPInt i1 = (address & GHeap_Lv2_PageMask) >> GHeap_Lv1_PageShift;
        UPInt i2 = (address & GHeap_Lv3_PageMask) >> GHeap_Lv2_PageShift;
        UPInt i3 = (address & GHeap_Lv4_PageMask) >> GHeap_Lv3_PageShift;
        UPInt i4 = (address & GHeap_Lv5_PageMask) >> GHeap_Lv4_PageShift;
        

        rootHeader->
            GetTable(i4)->
            GetTable(i3)->
            GetTable(i2)->
            GetTable(i1)->pDebugNode = node;
    }
 
#endif

}



//------------------------------------------------------------------------
GHeapDebugNode* GHeapPageTable::GetDebugNode(UPInt address) const
{

#ifndef GFC_64BIT_POINTERS

    UPInt                   rootIndex  = address >> GHeap_Root_PageShift;
    const GHeapHeaderRoot*  rootHeader = RootTable + rootIndex;

    GHEAP_ASSERT(rootHeader->HasTable());
    UPInt i1 = (address & GHeap_Lv2_PageMask) >> GHeap_Lv1_PageShift;
    return rootHeader->GetTable()[i1].pDebugNode;

#else

    UPInt i5  = address >> GHeap_Root_PageShift;
    const GHeapHeaderRoot*  rootHeader = RootTable + i5;

    GHEAP_ASSERT(rootHeader->HasTable(address));

    UPInt i1 = (address & GHeap_Lv2_PageMask) >> GHeap_Lv1_PageShift;
    UPInt i2 = (address & GHeap_Lv3_PageMask) >> GHeap_Lv2_PageShift;
    UPInt i3 = (address & GHeap_Lv4_PageMask) >> GHeap_Lv3_PageShift;
    UPInt i4 = (address & GHeap_Lv5_PageMask) >> GHeap_Lv4_PageShift;
   

    return rootHeader->GetTable(i4)->GetTable(i3)->GetTable(i2)->GetTable(i1)->pDebugNode;

#endif
}



//------------------------------------------------------------------------
GHeapDebugNode* GHeapPageTable::FindDebugNode(UPInt address) const
{

#ifndef GFC_64BIT_POINTERS

    UPInt                  rootIndex  = address >> GHeap_Root_PageShift;
    const GHeapHeaderRoot* rootHeader = RootTable + rootIndex;
    GHEAP_ASSERT(rootHeader->HasTable());
    UPInt i1 = (address & GHeap_Lv2_PageMask) >> GHeap_Lv1_PageShift;

    const GHeapHeader1* table = rootHeader->GetTable();

    if (table[i1].pDebugNode)
        return table[i1].pDebugNode;

    const GHeapSegment* thisSeg = table[i1].pSegment;

    for(;;)
    {
        for(; i1; --i1)
        {
            UPInt i = i1-1;

            if (table[i].pSegment != thisSeg)
                return 0;

            if (table[i].pDebugNode)
                return table[i].pDebugNode;
        }

        if (rootIndex == 0)
            break;

        table = RootTable[--rootIndex].GetTable();
        if (table == 0)
            break;

        i1 = GHeap_Lv1_TableSize;
    }

#else

    UPInt i5 = address >> GHeap_Lv5_PageShift;
    const GHeapHeaderRoot* rootHeader = RootTable + i5;
   
    GHEAP_ASSERT(rootHeader->HasTable(address));

    UPInt i4 = (address & GHeap_Lv5_PageMask) >> GHeap_Lv4_PageShift;
    UPInt i3 = (address & GHeap_Lv4_PageMask) >> GHeap_Lv3_PageShift;
    UPInt i2 = (address & GHeap_Lv3_PageMask) >> GHeap_Lv2_PageShift;
    UPInt i1 = (address & GHeap_Lv2_PageMask) >> GHeap_Lv1_PageShift;

   
    
    // First case: debug table is placed in the same Lv1 table.
    const GHeapHeader1* table1 = rootHeader->GetTable(i4)->GetTable(i3)->GetTable(i2)->GetTable(i1);

    if (table1->pDebugNode)
        return table1->pDebugNode;

    const GHeapSegment* thisSeg = table1->pSegment;

    // Second case: debug node is placed in the same Lv2 table.
    const GHeapHeader2* table2 = rootHeader->GetTable(i4)->GetTable(i3)->GetTable(i2);

    for(UPInt k1 = i1; k1 >= 0; --k1)
    {
        const GHeapHeader1* table1 = table2->GetTable(k1);

        if (table1->pSegment != thisSeg)
            return 0;

        if (table1->pDebugNode)
            return table1->pDebugNode;
    }

    // Third case: debug node is placed somewhere else. 
    for (UPInt k5 = 0; k5 <= i5; ++k5)
    {
        const GHeapHeader5* table5 = RootTable + k5;

        for (UPInt k4 = 0; k4 <= i4; ++k4)
        {
            const GHeapHeader4* table4 = table5->GetTable(k4);

            for (UPInt k3 = 0; k3 <= i3; ++k3)
            {
                const GHeapHeader3* table3 = table4->GetTable(k3);

                for (UPInt k2 = 0; k2 <= i2; ++k2)
                {
                   const GHeapHeader2* table2 = table3->GetTable(k2);

                   for (UPInt k1 = 0; k1 < GHeap_Lv1_TableSize; ++k1)
                   {     
                       const GHeapHeader1* table1 = table2->GetTable(k1);

                       if (table1->pSegment != thisSeg)
                           return 0;

                       if (table1->pDebugNode)
                           return table1->pDebugNode;
                    }
                }
            }
        }
    }

#endif

    return 0;
}

#endif //GHEAP_DEBUG_INFO
