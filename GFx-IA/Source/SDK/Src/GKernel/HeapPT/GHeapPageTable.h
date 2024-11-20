/**********************************************************************

Filename    :   GHeapPageTable.h
Content     :   Allocator page table for mapping the address space.
            :   
Created     :   July 14, 2008
Authors     :   Michael Antonov, Boris Rayskiy, Maxim Shemanarev

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GHeapPageTable_H
#define INC_GHeapPageTable_H

#include "GList.h"
#include "GHeapTypes.h"
#include "GHeapStarter.h"



#ifdef GFC_64BIT_POINTERS

#define GHeap_Lv1_PageSize (UPInt(1) << GHeap_Lv1_PageShift)
#define GHeap_Lv2_PageSize (UPInt(1) << GHeap_Lv2_PageShift)
#define GHeap_Lv3_PageSize (UPInt(1) << GHeap_Lv3_PageShift)
#define GHeap_Lv4_PageSize (UPInt(1) << GHeap_Lv4_PageShift)
#define GHeap_Lv5_PageSize (UPInt(1) << GHeap_Lv5_PageShift)

#define GHeap_Lv1_PageMask ((UPInt(1) << GHeap_Lv1_PageShift) - 1)
#define GHeap_Lv2_PageMask ((UPInt(1) << GHeap_Lv2_PageShift) - 1)
#define GHeap_Lv3_PageMask ((UPInt(1) << GHeap_Lv3_PageShift) - 1)
#define GHeap_Lv4_PageMask ((UPInt(1) << GHeap_Lv4_PageShift) - 1)
#define GHeap_Lv5_PageMask ((UPInt(1) << GHeap_Lv5_PageShift) - 1)

#endif



template<class T, UPInt N>
class GHeapHeader
{
    T*      pTable;
    UPInt   RefCount;

public:

    GHeapHeader() : pTable(0), RefCount(0) {}

    T*      GetTable() const            { return pTable;                    }
    T*      GetTable(UPInt i) const     { return &pTable[i];                }
    //T*      GetTableSafe(UPInt i) const { return pTable ? &pTable[i] : 0;   } // DBG


#ifndef GFC_64BIT_POINTERS
     
    bool    HasTable() const    { return pTable != 0; }

#else

    GINLINE bool    HasTable(UPInt address) const;

#endif

    bool    AddRef(GHeapStarter* starter)
    {
        if (!pTable)
        {
            // For potential ref counting in the pointer: We must ensure
            // the alignment provides enough capacity for ref counting.
            //-------------------
            pTable = (T*)starter->Alloc(sizeof(T) * N, sizeof(void*) * N);
            if (!pTable)
                return false;

            // We must initialize the table to 0 since that is the correct value
            // of headers that indexing relies on.
            memset(pTable, 0, sizeof(T) * N);
        }
        RefCount++;
        return true;
    }

    void    Release(GHeapStarter* starter)
    {
        if (--RefCount == 0)
        {
            starter->Free((UByte*)pTable, sizeof(T) * N, sizeof(void*) * N);
            pTable = 0;
        }
    }
};


//------------------------------------------------------------------------
struct GHeapDebugNode;
struct GHeapHeader1
{
    GHeapSegment*   pSegment;
#ifdef GHEAP_DEBUG_INFO
    GHeapDebugNode* pDebugNode;
#endif
};

#ifndef GFC_64BIT_POINTERS

// Allocator header of level 2.
// Describes 1M memory block, using table containing 256 Lv1 headers.
//------------------------------------------------------------------------
typedef GHeapHeader<GHeapHeader1, GHeap_Lv1_TableSize> GHeapHeader2;
typedef GHeapHeader2 GHeapHeaderRoot;

#else

// Allocator header of level 2-5. For details see GHeapTypes.h
//________________________________________________________________________
typedef GHeapHeader<GHeapHeader1, GHeap_Lv1_TableSize> GHeapHeader2;
typedef GHeapHeader<GHeapHeader2, GHeap_Lv2_TableSize> GHeapHeader3;
typedef GHeapHeader<GHeapHeader3, GHeap_Lv3_TableSize> GHeapHeader4;
typedef GHeapHeader<GHeapHeader4, GHeap_Lv4_TableSize> GHeapHeader5;

typedef GHeapHeader5 GHeapHeaderRoot;


template<class T, UPInt N>
GINLINE bool GHeapHeader<T,N>::HasTable(UPInt address) const    
{
        UPInt i4 = (address & GHeap_Lv5_PageMask) >> GHeap_Lv4_PageShift;
        UPInt i3 = (address & GHeap_Lv4_PageMask) >> GHeap_Lv3_PageShift;
        UPInt i2 = (address & GHeap_Lv3_PageMask) >> GHeap_Lv2_PageShift;
        //UPInt i1 = (address & GHeap_Lv2_PageMask) >> GHeap_Lv1_PageShift; // DBG
        
        GHeapHeader4* table4 = this->GetTable();
        if (table4)
        {
            GHeapHeader3* table3 = table4[i4].GetTable();
            if (table3)
            {
                GHeapHeader2* table2 = table3[i3].GetTable();
                if (table2)
                {
                    GHeapHeader1* table1 = table2[i2].GetTable();
                    if (table1)
                    {
                        return true;
                    }
                }
            }
        }
        return false; 
}

#endif


// ***** GHeapPageTable
//
// Root Page Table.
// Root interface implementing a page table that spans an address space,
// implemented differently for 32 and 64-bit system. The interface supports
// mapping and un-mapping address ranges, such that after a memory range is
// mapped, a Header class is allocated that corresponds to every BlockSize.
//
// Internally the headers for mapped ranges are managed with reference counts.
//------------------------------------------------------------------------

extern class GHeapPageTable* GHeapGlobalPageTable;

class GHeapPageTable
{
public:
    GHeapPageTable();

    // Init creates a single global instance in static memory 
    // and assigns GHeapGlobalPageTable.
    static void Init();

    // Set/Get the bootstrapping starter allocator.
    void          SetStarter(GHeapStarter* s)   { pStarter = s; }
    GHeapStarter* GetStarter()                  { return pStarter;  }

    // Looks up Header1 address based on the global page table.
    // DOESN'T DO ERROR CHECKS - these are not necessary within internal addressing
    // in allocator (only needed for externally passed addresses).
    GINLINE GHeapSegment* GetSegment(UPInt address) const
    {
        
#ifndef GFC_64BIT_POINTERS

        UPInt                   rootIndex  = address >> GHeap_Root_PageShift;
        const GHeapHeaderRoot*  rootHeader = RootTable + rootIndex;
        UPInt i1 = (address & GHeap_Lv2_PageMask) >> GHeap_Lv1_PageShift;
        return rootHeader->GetTable()[i1].pSegment;

#else

        UPInt i5 = address >> GHeap_Lv5_PageShift;
        UPInt i4 = (address & GHeap_Lv5_PageMask) >> GHeap_Lv4_PageShift;
        UPInt i3 = (address & GHeap_Lv4_PageMask) >> GHeap_Lv3_PageShift;
        UPInt i2 = (address & GHeap_Lv3_PageMask) >> GHeap_Lv2_PageShift;
        UPInt i1 = (address & GHeap_Lv2_PageMask) >> GHeap_Lv1_PageShift;

        const GHeapHeaderRoot*  rootHeader = RootTable + i5;

        return rootHeader->
            GetTable(i4)->
            GetTable(i3)->
            GetTable(i2)->
            GetTable(i1)->pSegment;   
#endif

    }


    // Obtains a pointer to header with page table validity check.
    GINLINE GHeapSegment*  GetSegmentSafe(UPInt address) const
    {

#ifndef GFC_64BIT_POINTERS

        UPInt rootIndex = address >> GHeap_Root_PageShift;
        UPInt i1 = (address & GHeap_Lv2_PageMask) >> GHeap_Lv1_PageShift;
        const GHeapHeaderRoot* rootHeader = RootTable + rootIndex;

        return rootHeader->HasTable() ? (rootHeader->GetTable()[i1].pSegment) : 0;

#else

        UPInt i5 = address >> GHeap_Lv5_PageShift;
        UPInt i4 = (address & GHeap_Lv5_PageMask) >> GHeap_Lv4_PageShift;
        UPInt i3 = (address & GHeap_Lv4_PageMask) >> GHeap_Lv3_PageShift;
        UPInt i2 = (address & GHeap_Lv3_PageMask) >> GHeap_Lv2_PageShift;
        UPInt i1 = (address & GHeap_Lv2_PageMask) >> GHeap_Lv1_PageShift;

        const GHeapHeaderRoot* table5 = RootTable + i5;
        if (table5)
        {
            const GHeapHeader4* table4 = table5->GetTable(i4);
            if (table4)
            {
                const GHeapHeader3* table3 = table4->GetTable(i3);
                if (table3)
                {
                    const GHeapHeader2* table2 = table3->GetTable(i2);
                    if (table2)
                    {
                        const GHeapHeader1* table1 = table2->GetTable(i1);
                        if (table1)
                        {
                            return table1->pSegment;
                        }
                    }
                }
            }
        }

        return NULL;

#endif

    }

    // Maps an address range by allocating all of the headers for it.
    // Input:
    //      address         - start address of the heap segment.
    //      size            - Heap size in bytes.
    // Returns true if headers of Lv2 are successfully mapped, false
    // if allocation failed.
    bool    MapRange(void* address, UPInt size);
    
    // Unmaps a memory range mapped by MapRange. 
    void    UnmapRange(void* address, UPInt size);

    // Remaps a memory range mapped by MapRange. 
    bool    RemapRange(void* address, UPInt newSize, UPInt oldSize);

    // Helper function used to assign a range of segment pointers,
    // used for mapping and external nested heap assignments.
    void    SetSegmentInRange(UPInt address, UPInt size, GHeapSegment* seg);

#ifdef GHEAP_DEBUG_INFO
    void            SetDebugNode(UPInt address, GHeapDebugNode* page);
    GHeapDebugNode* GetDebugNode(UPInt address) const;
    GHeapDebugNode* FindDebugNode(UPInt address) const;
#endif

    void    VisitMem(GHeapMemVisitor* visitor) 
    {
        pStarter->VisitMem(visitor); 
    }

private:
    // Bootstrapping starter allocator used for pages needed for 
    // Map/Unmap implementation.
    GHeapStarter*   pStarter;

    // Root page table. We could make it static or allocate it in the future;
    // however, we can't grab it from page allocator as the later one has fixed
    // size while this table contains the remaining number of entries.
    GHeapHeaderRoot RootTable[GHeap_Root_TableSize];
};

#endif
