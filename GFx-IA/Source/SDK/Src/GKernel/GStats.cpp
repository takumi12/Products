/**********************************************************************

Filename    :   GStats.cpp
Content     :   Statistics tracking and reporting APIs
Created     :   May 20, 2008
Authors     :   Michael Antonov

Notes       :   

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GStats.h"
#include "GMemory.h"

#include "GAtomic.h"


#ifdef GFC_NO_STAT

namespace { char dummyStatsVar; }; // to disable warning LNK4221 on PC/Xbox

#else

// For root, we allow GStatGroup_Default.
GDECLARE_STAT_GROUP(GStatGroup_Default, "Default", GStatGroup_Default)

GDECLARE_MEMORY_STAT_AUTOSUM_GROUP(GStat_Mem, "Used Memory", GStatGroup_Default)
GDECLARE_MEMORY_STAT(GStat_Default_Mem, "General", GStat_Mem)
GDECLARE_MEMORY_STAT(GStat_Image_Mem, "Image", GStat_Mem)
#ifndef GFC_NO_SOUND
GDECLARE_MEMORY_STAT(GStat_Sound_Mem, "Sound", GStat_Mem)
#endif
GDECLARE_MEMORY_STAT(GStat_String_Mem, "String", GStat_Mem)
#ifndef GFC_NO_VIDEO
GDECLARE_MEMORY_STAT(GStat_Video_Mem, "Video", GStat_Mem)
#endif

GDECLARE_MEMORY_STAT_AUTOSUM_GROUP(GStat_Debug_Mem, "Debug Memory", GStat_Mem)
GDECLARE_MEMORY_STAT(GStat_DebugHUD_Mem, "Debug HUD", GStat_Debug_Mem)
GDECLARE_MEMORY_STAT(GStat_DebugTracker_Mem, "Debug Tracker", GStat_Debug_Mem)
GDECLARE_MEMORY_STAT(GStat_StatBag_Mem, "StatBag", GStat_Debug_Mem)



/*
    Notes On Statistics
    -------------------

    These notes discuss some of concerns that can be addressed by the statistics
    system in the future. It is good to understand these while making a general
    stat tracking and display system.

    Counter Types

    The interpretation of the counter can have a significant effect on how it
    is updated and displayed. There are several possible counter types:

     1. Collector Count - This counter is reset to 0 before statistics report
                          is generated and then incremented as all the items
                          are "collected" for inclusion. Absolute Min/Max is
                          not known with this counter and recording it is not
                          useful within the statistic itself, although average
                          min/max can still be computed externally (subjects
                          to precision limits based on the sampling rate).

                          In GFx, it's planned to use such counters for memory
                          stat reporting, DP/Triangle counts, etc.

     2. Running Count   - This counter keeps the real-time state continuously,
                          and is thus incremented and decremented as objects/
                          memory/resources are created and destroyed. Min/Max
                          can be tracked correctly if necessary. Reset of counter
                          to 0 does not make sense as it would make the rest
                          of the results incorrect, but Min/Max could be reset
                          based on the sampling rate.

                          Heap memory sizes can be tracked that was so that
                          real Min/Max is known.

     3. Absolute Count -  Increment only version of the Running Count, which
                          can not be reset. Since the value is always incremented
                          tracking Min/Max does not make sense.

                          Could be applied to loaded data, such as the number
                          of shapes or fonts loaded from the SWF file, or the
                          total time it took for a resource to load.


    Statistical Data Collection and Display

    As statistics data is collected at specified intervals, there is a number
    of data elements which can be usefully tracked and displayed for every
    statistic. These include:

      - Instantaneous (final) value.
      - Average
      - Sum
      - Minimum
      - Maximum

    The last four statistics could be tracked and displayed based on the different
    sampling windows; for example, we can display  Average and Maximum values
    over the period of one Frame, one Second, or program Lifetime. Unfortunately,
    not all of this information is useful in all cases and displaying all of the
    possible values may make the display busy and hard to interpret.

    Here are a few examples:

     a) Advance Ticks,   - Average, Min, Max make sense over one second. Sum does
        Advanced MCCount   not since a second has many frames.

     b) MovieDef Memory  - Instantaneous value is more interesting, since as its
                           exact as opposed to averages (it is at least enough).
                           Min/Max not useful.

        Load Time Ticks  - Min/Max/Avg meaningless, since this is an absolute
                           count. Only the final value is useful.
        
     c) MovieView Memory - Min, Max, Instantaneous Val all useful.

     d) Tessellate Count, - Sum may be more interesting then Avg since tessellations
        Triangles Gen      only happen in certain frames and Avg would be small,
                           while knowing the total # of shapes/triangles processed
                           is useful.
    
   
   So, ultimately while displaying all of the values may be ok but confusing. Allowing
   developers select which value is displayed in the "Avg" column per stat may be
   more useful {Average, Sum, Instance}.

*/



// ***** Supported Interfaces

GStatInfo_InterfaceImpl<GMemoryStat>    GStat_MemoryInterface;
GStatInfo_InterfaceImpl<GTimerStat>     GStat_TimerInterface;
GStatInfo_InterfaceImpl<GCounterStat>   GStat_CounterInterface;

GStatInfo::StatInterface* GStats_InterfaceTable[GStat::Stat_TypeCount] =
{
    0,
    &GStat_MemoryInterface,
    &GStat_TimerInterface,
    &GStat_CounterInterface
};


// ***** Descriptor Registration

struct GStatDescRegistry
{
    enum
    {   
        Desc_MemEntryCount   = GStat_EntryCount * 2,

        Desc_PageShift       = 3,
        Desc_PageSize        = 1 << Desc_PageShift,
        Desc_PageTableSize   = GStat_MaxId / Desc_PageSize,

        // Page table entries are set to this value if no memory or slot
        // is allocated to them. We rely on default zero-initialization 
        // to assign Unused values.
        Desc_IdUnused        = 0
    };

    UInt        DescAllocOffset;  

    // Id page table. All the contained values are biased by one (+1) so
    // that a default value of '0' can represent the Unused value.
    UInt16      IdPageTable[Desc_PageTableSize];
    // Reserved memory for Ids.
    GStatDesc*  DescMem[Desc_MemEntryCount];
  

    void        RegisterDesc(GStatDesc* pdesc)
    {
        // Get offset of the page, and if page does not exit, allocate it.
        UInt16 pageOffset = IdPageTable[pdesc->GetId() >> Desc_PageShift];
        if (pageOffset == Desc_IdUnused)
        {            
            if (Desc_MemEntryCount < (DescAllocOffset + Desc_PageSize))
            {
                GFC_DEBUG_WARNING1(1,
                    "GStatDescRegistry out of reserved memory on statId %d", pdesc->GetId());
                return;
            }

            // Update the page index.
            pageOffset = (UInt16)(DescAllocOffset) + 1;
            IdPageTable[pdesc->GetId() >> Desc_PageShift] = pageOffset;

            // And mark all offsets as un-initialized.
            GStatDesc** p = DescMem + DescAllocOffset;
            for(int i = 0; i <Desc_PageSize; i++, p++)
                *p = 0;
            DescAllocOffset += Desc_PageSize;
        }

        // Record the descriptor entry.
        DescMem[pageOffset - 1 + (pdesc->GetId() & (Desc_PageSize-1))] = pdesc;
    }

    GStatDesc*  GetDescImpl(UInt statId)
    {
        UInt        pageOffset = IdPageTable[statId >> Desc_PageShift];
        GStatDesc*  pdesc;

        // If this ASSERT gets hit, it means that StatId is not declared.
        if (pageOffset == Desc_IdUnused)
            return 0;       
        pdesc = DescMem[(pageOffset-1) + (statId & (Desc_PageSize-1))];

        GASSERT(!pdesc || (pdesc->GetId() == statId));
        return pdesc;
    }

    GStatDesc*  GetDesc(UInt statId)
    {
        GStatDesc* pdesc = GetDescImpl(statId);

        // Simple debugging logic to display a message with a previous name in case 
        // that the statId is use was not declared.
#ifdef GFC_BUILD_DEBUG          
        if (!pdesc)
        {
            UInt        id = statId;
            GStatDesc*  pdescPrev;

            while (id > 0)
            {
                id--;
                pdescPrev = GetDescImpl(id);
                if (pdescPrev)
                {
                    GFC_DEBUG_ERROR3(1, "GStatId %d following after Id %d '%s' not registered.", 
                                     statId, id, pdescPrev->GetName());
                    break;
                }
            }

            GASSERT(0);
        }

#endif
        return pdesc;
    }


};

// We expect registry values to be zero-initialized. This is important
// because registration routines can run in any order, so we can not
// rely on the default constructor being available before RegisterDesc is called.
GStatDescRegistry GStatDescRegistryInstance = { 0, {0}, {0} };


/*  Child Tree Initialization

  We use two step delayed initialization of the descriptor child tree
  to avoid problems with different translation units coming in unknown
  order. 
    1) During static initialization, we register descriptors and 
       combine them into a linked list.
    2) On first call to GetDesc, we traverse the list and convert
       it into the tree. We use AtomicOps to make this singleton
       initialization thread safe (in case GetDesc() is called from
       different threads simultaneously on the first call).
*/

// Descriptor pointers used during static initialization.
static GStatDesc* GStats_pFirstDesc = 0;
static GStatDesc* GStats_pLastDesc  = 0;
// Flags used to track atomic initialization.
static volatile   UInt GStats_InitDone = 0;
static volatile   UInt GStats_InitByUs = 0;


// Register descriptors so that they can be looked up by id.
void         GStatDesc::RegisterDesc(GStatDesc* pdesc)
{
    GStatDescRegistryInstance.RegisterDesc(pdesc);

    // Link up all ids which can be children in a common list,
    // so that we can later initialize a descriptor tree.
    //if (pdesc->GroupId)
    {
        if (GStats_pLastDesc)
        {
            GStats_pLastDesc->pNextSibling = pdesc;
            GStats_pLastDesc               = pdesc;
        }
        else
        {
            GStats_pFirstDesc = pdesc;
            GStats_pLastDesc  = pdesc;
        }
    }   
}


const GStatDesc*   GStatDesc::GetDesc(UInt id)
{
    if (!GAtomicOps<UInt>::Load_Acquire(&GStats_InitDone))
        InitChildTree();

    return GStatDescRegistryInstance.GetDesc(id);
}


void    GStatDesc::InitChildTree()
{
    if (GAtomicOps<UInt>::Load_Acquire(&GStats_InitDone))
        return;

    // Make sure we are the only thread who is performing
    // static tree initialization. 
    UInt oldInitByUs;   
    do {
        oldInitByUs = GStats_InitByUs;

        if (oldInitByUs == 1)
        {
            // Spin if someone else started initialization.
            while (!GAtomicOps<UInt>::Load_Acquire(&GStats_InitDone))
                ;
            return;
        }
    } while (!GAtomicOps<UInt>::CompareAndSet_Sync(&GStats_InitByUs, oldInitByUs, 1));


    GStatDesc* pdesc, *pnext;
    
    for(pdesc = GStats_pFirstDesc; pdesc != 0; pdesc = pnext)
    {
        pnext = pdesc->pNextSibling;
        pdesc->pNextSibling = 0;

        // Link up the parent. 
        GStatDesc* pparentDesc = GStatDescRegistryInstance.GetDesc(pdesc->GroupId);

        if (pparentDesc != pdesc)
        {
            if (!pparentDesc->pChild)
                pparentDesc->pChild = pdesc;
            else
            {
                // Connect us in the end so that iteration order is preserved.          
                // This is safe to do here since all pNextSibling pointers modified
                // here would have been traversed in the previous iteration of our
                // containing for loop.
                GStatDesc* psibling = pparentDesc->pChild;
                while(psibling->pNextSibling)
                    psibling = psibling->pNextSibling;
                psibling->pNextSibling = pdesc;
            }
        }
        else
        {
            GASSERT(pdesc->Id == pdesc->GroupId);
        }
        
    }

    GStats_pFirstDesc = 0;
    GStats_pLastDesc  = 0;

    GAtomicOps<UInt>::Store_Release(&GStats_InitDone, 1);
}



// ***** GStatBag implementation

// Create a stat bag with specified number of entries.
GStatBag::GStatBag(GMemoryHeap *pheap, UInt memReserve)
{
    if (!pheap)
        pheap = GMemory::GetGlobalHeap();

    pMem    = (UByte*) GHEAP_ALLOC(pheap, memReserve, GStat_StatBag_Mem);
    MemSize = memReserve;
    Clear();
}

GStatBag::GStatBag( const GStatBag& source )
{
    pMem    = (UByte*) GALLOC(GStat_EntryCount * 16, GStat_StatBag_Mem);
    MemSize = GStat_EntryCount * 16;
    *this = source;
}

GStatBag::~GStatBag()
{
    GFREE(pMem);
}


// Clear out the bag, removing all states.
void    GStatBag::Clear()
{
    MemAllocOffset = 0;
    for(int i = 0; i< StatBag_PageTableSize; i++)
        IdPageTable[i] = StatBag_IdUnused;
}

// Reset stat values, usually causing peaks to be recorded.
void    GStatBag::Reset()
{
    for(int i = 0; i< StatBag_PageTableSize; i++)
    {
        UInt pageOffset = IdPageTable[i];

        if (pageOffset != StatBag_IdUnused)
        {
            UInt16* ptable = (UInt16*) (pMem + (pageOffset * StatBag_MemGranularity));

            for (int j=0; j < StatBag_PageSize; j++)
            {
                if (ptable[j] != StatBag_IdUnused)
                {
                    UInt           id    = j | (i << StatBag_PageShift);
                    StatInterface* psi   = GetInterface(id);
                    GStat*         pstat = (GStat*)(pMem + ptable[j] * StatBag_MemGranularity);
                    psi->Reset(pstat);
                }
            }
        }
    }  
}

GStatInfo::StatInterface* GStatBag::GetInterface(UInt id)
{
    const GStatDesc* pdesc = GStatDesc::GetDesc(id);
    GASSERT(pdesc->GetType() < GStat::Stat_TypeCount);
    return GStats_InterfaceTable[pdesc->GetType()];
}


// Add a statistic value of a certain id.
bool    GStatBag::Add(UInt statId, GStat* pstat)
{ 
    // Check that stat is registered.
    StatInterface* psi = GetInterface(statId);
    GStat*         pthisStat = GetStatRef(statId);
    
    if (!pthisStat)
    {
        // Try to add stat
        pthisStat = (GStat*)AllocStatData(statId, psi->GetStatDataSize());
        if (!pthisStat)
            return false;
        psi->Init(pthisStat);
    }
    
    psi->Add(pthisStat, pstat);
    return true;
}

bool    GStatBag::AddMemoryStat(UInt statId, const GMemoryStat& stat)
{
    GMemoryStat* pmemStat = (GMemoryStat*)GetStatRef(statId);
    if (!pmemStat)
    {
        pmemStat = (GMemoryStat*)AllocStatData(statId, sizeof(GMemoryStat));
        if (!pmemStat)
            return false;
        pmemStat->Init();
    }
 
    pmemStat->Add((GMemoryStat*)&stat);
    return true;
}

bool    GStatBag::IncrementMemoryStat(UInt statId, UPInt alloc, UPInt use)
{
    GMemoryStat* pmemStat = (GMemoryStat*)GetStatRef(statId);
    if (!pmemStat)
    {
        pmemStat = (GMemoryStat*)AllocStatData(statId, sizeof(GMemoryStat));
        if (!pmemStat)
            return false;
        pmemStat->Init();
    }

    pmemStat->Increment(alloc, use);
    return true;
}



bool    GStatBag::SetMin(UInt statId, GStat* pstat)
{
    // Check that stat is registered.
    StatInterface* psi = GetInterface(statId);
    GStat*         pthisStat = GetStatRef(statId);

    if (!pthisStat)
    {
        // Try to add stat
        pthisStat = (GStat*)AllocStatData(statId, psi->GetStatDataSize());
        if (!pthisStat)
            return false;
        psi->Init(pthisStat);
        psi->Add(pthisStat, pstat);
    }
    else
    {
        psi->SetMin(pthisStat, pstat);
    }

    return true;
}

bool    GStatBag::SetMax(UInt statId, GStat* pstat)
{
    StatInterface* psi = GetInterface(statId);
    GStat*         pthisStat = GetStatRef(statId);

    if (!pthisStat)
    {
        // Try to add stat
        pthisStat = (GStat*)AllocStatData(statId, psi->GetStatDataSize());
        if (!pthisStat)
            return false;
        psi->Init(pthisStat);
        psi->Add(pthisStat, pstat);
    }
    else
    {
        psi->SetMax(pthisStat, pstat);
    }

    return true;
}


// Add values of a different stat bag to ours.
void    GStatBag::CombineStatBags(const GStatBag& other,
                                  bool (GStatBag::*combineFunc)(UInt, GStat*))
{
    // Go through the items in the other bag, and add them one item at a time.
    for(int i = 0; i< StatBag_PageTableSize; i++)
    {
        UInt pageOffset = other.IdPageTable[i];
        if (pageOffset != StatBag_IdUnused)
        {
            UInt16* ptable = (UInt16*) (other.pMem + (pageOffset * StatBag_MemGranularity));

            for (int j=0; j < StatBag_PageSize; j++)
            {
                if (ptable[j] != StatBag_IdUnused)
                {
                    UInt id    = j | (i << StatBag_PageShift);
                    GStat*     pstat = (GStat*)(other.pMem + ptable[j] * StatBag_MemGranularity);
                    (this->*combineFunc)(id, pstat);
                }
            }
        }
    } 
}



void    GStatBag::RecursiveGroupUpdate(GStatDesc::Iterator it)
{
    const GStatDesc* pdesc = *it;

    if (pdesc)
    {
        GStatDesc::Iterator ichild = pdesc->GetChildIterator();     

        if (pdesc->IsAutoSumGroup())
        {
            // Add all children to us.
            while(!ichild.IsEnd())
            {
                RecursiveGroupUpdate(ichild);

                GStat* pstatData = GetStatRef(ichild.GetId());
                if (pstatData)              
                    Add(pdesc->GetId(), pstatData);
                ++ichild;
            }
        }
        else
        {
            while(!ichild.IsEnd())
            {
                RecursiveGroupUpdate(ichild);
                ++ichild;
            }
        }
    }
}

// Update all cumulative groups in the list.
void    GStatBag::UpdateGroups()
{
    if (!GAtomicOps<UInt>::Load_Acquire(&GStats_InitDone))
        GStatDesc::InitChildTree();

    // Traverse the StateDesc tree recursively and add the items.   
    RecursiveGroupUpdate(GStatDesc::GetGroupIterator(GStatGroup_Default));
}


GStat*  GStatBag::GetStatRef(UInt statId) const
{
    // GetStat size.
    if (statId >= GStat_MaxId)
    {
        GFC_DEBUG_WARNING1(1,
            "GStatBag::GetStat - statId value %d is out of supported range", statId);
        return 0;
    }

    UInt pageOffset = IdPageTable[statId >> StatBag_PageShift];   
    if (pageOffset == StatBag_IdUnused)
        return 0;

    UInt16* ptable = (UInt16*) (pMem + (pageOffset * StatBag_MemGranularity));

    UInt pageIndex = ptable[statId & (StatBag_PageSize-1)];
    if (pageIndex == StatBag_IdUnused)
        return 0;

    return (GStat*)(pMem + (pageIndex * StatBag_MemGranularity));
}


UByte*  GStatBag::AllocStatData(UInt statId, UPInt size)
{
    GASSERT(statId != 0);

    // Round up the size.
    size = (size + StatBag_MemGranularity - 1) & ~(StatBag_MemGranularity-1);

    UInt16 pageOffset = IdPageTable[statId >> StatBag_PageShift];   
    if (pageOffset == StatBag_IdUnused)
    {
        // If page does not exit, allocate it.
        UPInt   tableSize = ((StatBag_PageSize * sizeof(UInt16)) + StatBag_MemGranularity - 1) &
                            ~(StatBag_MemGranularity-1);

        if (MemSize < (MemAllocOffset + tableSize))
        {
            GFC_DEBUG_WARNING1(1, "GStatBag out of reserved memory on adding statId %d", statId);
            return 0;
        }

        // Update the page index.
        pageOffset = (UInt16)(MemAllocOffset / StatBag_MemGranularity);
        IdPageTable[statId >> StatBag_PageShift] = pageOffset;
        
        // And mark all offsets as un-initialized.
        UInt16* p = (UInt16*) (pMem + MemAllocOffset);
        for (int i = 0; i < StatBag_PageSize; i++, p++)
            *p = (UInt16)StatBag_IdUnused;

        MemAllocOffset += tableSize;        
    }

    UInt16* ptable = (UInt16*) (pMem + (pageOffset * StatBag_MemGranularity));
    UInt pageIndex = ptable[statId & (StatBag_PageSize-1)];   
    GASSERT(pageIndex == StatBag_IdUnused);
    GUNUSED(pageIndex);

    // Make sure there is enough memory.
    if (MemSize < (MemAllocOffset + size))
    {
        GFC_DEBUG_WARNING1(1, "GStatBag out of reserved memory on adding statId %d", statId);
        return 0;
    }

    ptable[statId & (StatBag_PageSize-1)] = (UInt16)(MemAllocOffset / StatBag_MemGranularity);
    UByte *pdata = pMem + MemAllocOffset;
    MemAllocOffset += size;
    return pdata;
}


// Does this accumulate data or not?
bool    GStatBag::GetStat(GStatInfo *pstat, UInt statId) const
{
    GStat* pstatData = GetStatRef(statId);
    if (!pstatData)
        return false;

    // Get type.
    *pstat = GStatInfo(statId, GetInterface(statId), pstatData);
    return true;
}




// *** GStatBag::Iterator Logic

GStatBag::Iterator::Iterator(GStatBag* pbag, UInt id, UInt groupId)
    : Id(id), GroupId(groupId), pBag(pbag)
{
    // Potentially advance Iterator once so that we point to
    // a valid item or end.   
    if (pbag)
    {
        AdvanceTillValid();
    }
    else
    {
        Id = StatBag_EndId;
    }
}

// Advance to the next valid item or fail.
bool GStatBag::Iterator::AdvanceTillValid()
{
    if (!GAtomicOps<UInt>::Load_Acquire(&GStats_InitDone))
        GStatDesc::InitChildTree();

    // Advance forward until we found a matching id or hit the end.
    while (Id < StatBag_EndId)
    {   
        // If this page is empty skip it.
        if (pBag->IdPageTable[Id >> StatBag_PageShift] == StatBag_IdUnused)
        {
            Id = (Id + StatBag_PageSize) & ~(StatBag_PageSize-1);
        }
        else
        {
            if (pBag->GetStat(&Result, Id))
            {
                const GStatDesc* pdesc = GStatDescRegistryInstance.GetDesc(Id);                 

                GASSERT(pdesc);
                if ((GroupId == GStat_MaxId) ||
                    (pdesc->GetGroupId() == GroupId))
                {
                    // Found id, we are done.
                    return true;
                }
            }
            
            Id++;
        }
    }
    return false;
}


// Obtains an Iterator for the specified stat group. Default implementation
// will return all of the stats in the bag.
GStatBag::Iterator    GStatBag::GetIterator(UInt groupId)
{
    return Iterator (this, 0, groupId);
}




// Wait for stats to be ready; useful if stat update
// request is queued up for update in a separate thread.
/*
void    GStatBag::WaitForData()
{
}
*/

#endif
