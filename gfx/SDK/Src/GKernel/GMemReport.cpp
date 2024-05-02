/**********************************************************************

Filename    :   GMemReport.cpp
Content     :   Forming string buffer with memory report.

Created     :   July 15, 2009
Authors     :   Boris Rayskiy

Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GMemoryHeap.h"
#include "GHeapTypes.h"
#include "GMemory.h"
#include "GString.h"
#include "GMsgFormat.h"
#include "GRefCount.h"
#include "GAlgorithm.h"
#include "GStats.h"
#include "GFxLog.h"
#include "GFxAmpProfileFrame.h"
#include "GHeapNew.h"

void GMemoryHeap::MemReport(GFxLog* log, MemReportType reportType)
{
    GStringBuffer buffer;
    MemReport(buffer, reportType);
    if (log != NULL)
    {
        // Breaking string into chunks so that it doesn't get truncated by some log implementations
        static const UPInt chunkSize = 100;
        for (UPInt i = 0; i < buffer.GetLength(); i += chunkSize)
        {
            log->LogMessage("%s", GString(buffer.ToCStr() + i, G_Min(chunkSize, buffer.GetLength() - i)).ToCStr());
        }
    }

}

#ifndef GFC_NO_STAT

// This class creates the memory report
// It lives here so we don't have to pollute the public GMemoryHeap header with internals
//
class StatsUpdate
{
public:
    StatsUpdate() : NextHandle(0) { }
    // Public interface
    // Called by GMemoryHeap::MemReport
    void MemReport(GFxAmpMemItem* rootItem, GMemoryHeap::MemReportType reportType);

private:
    // Heap visitor that adds all the child heaps to an array
    struct HolderVisitor : public GMemoryHeap::HeapVisitor
    {
        GArray<GMemoryHeap*> Heaps;

        virtual void Visit(GMemoryHeap* pParent, GMemoryHeap *heap)
        { 
            GUNUSED(pParent);
            Heaps.PushBack(heap); 
        }
    };

    // This struct holds the memory stats for a given heap
    // It is part of the heap visitor, StatIdVisitor, below
    struct HeapStats : public GRefCountBase<HeapStats, GStat_Default_Mem> 
    {
        HeapStats(GMemoryHeap* heap) : Heap(heap)
        {
            Heap->GetStats(&StatBag);
        }
        GMemoryHeap*    Heap;
        GStatBag        StatBag;
    };

    // Heap visitor that traverses the tree and adds all non-debug heaps to an array
    // The actual HeapStats for each heap is not set by the visitor
    struct StatIdVisitor : public GMemoryHeap::HeapVisitor
    {
        GArray< GPtr<HeapStats> > HeapStatsArray;
        GArray<UPInt>             HeapUsedMemory;

        virtual void Visit(GMemoryHeap* parent, GMemoryHeap* heap)
        { 
            GUNUSED(parent);
            if (((heap->GetFlags() & GMemoryHeap::Heap_UserDebug) == 0) && heap->GetFootprint())
            {
                HeapStatsArray.PushBack(*GNEW HeapStats(heap));
                HeapUsedMemory.PushBack(0);
            }

            heap->VisitChildHeaps(this);
        }
    };

    // Custom heap visitor class that keeps track of memory per StatID
    class SummaryStatIdVisitor : public GMemoryHeap::HeapVisitor
    {
    public:
        virtual void Visit(GMemoryHeap* parent, GMemoryHeap *heap)
        {
            GUNUSED(parent);
            if (((heap->GetFlags() & GMemoryHeap::Heap_UserDebug) == 0))
            {
                if (heap->GetId() != GHeapId_FontCache)
                {
                    GStatBag bag;
                    heap->GetStats(&bag);
                    StatIdBag.Add(bag);

                    heap->VisitChildHeaps(this);
                }
            }
        }
        UPInt GetStatIdMemory(GStatDesc::Iterator it) const
        {
            GStatInfo statInfo;
            UPInt totalMemory = 0;

            if (StatIdBag.GetStat(&statInfo, it->GetId()))
            {
                GStat::StatValue statValue;
                statInfo.GetStat(&statValue, 0);
                totalMemory = statValue.IValue;
            }

            for (GStatDesc::Iterator itChild = it->GetChildIterator(); !itChild.IsEnd(); ++itChild)
            {
                totalMemory += GetStatIdMemory(itChild);
            }

            return totalMemory;
        }
    private:
        GStatBag        StatIdBag;
    };


    // Custom heap visitor class that keeps track of Movie, Mesh, and Font memory
    class SummaryMemoryVisitor : public GMemoryHeap::HeapVisitor
    {
    public:
        SummaryMemoryVisitor() : MovieViewMemory(0), MovieDataMemory(0), MeshCacheMemory(0), FontCacheMemory(0) {}
        virtual void Visit(GMemoryHeap* parent, GMemoryHeap *heap)
        {
            GUNUSED(parent);
            if (((heap->GetFlags() & GMemoryHeap::Heap_UserDebug) == 0))
            {
                switch (heap->GetId())
                {
                case GHeapId_MovieView:
                    MovieViewMemory += heap->GetUsedSpace();
                    break;
                case GHeapId_MovieData:
                    MovieDataMemory += heap->GetUsedSpace();
                    break;
                case GHeapId_MeshCache:
                    MeshCacheMemory += heap->GetUsedSpace();
                    break;
                case GHeapId_FontCache:
                    FontCacheMemory += heap->GetUsedSpace();
                    break;
                }
            }
            heap->VisitChildHeaps(this);
        }
        UPInt           GetMovieViewMemory() const { return MovieViewMemory; }
        UPInt           GetMovieDataMemory() const { return MovieDataMemory; }
        UPInt           GetMeshCacheMemory() const { return MeshCacheMemory; }
        UPInt           GetFontCacheMemory() const { return FontCacheMemory; }
    private:
        UPInt           MovieViewMemory;
        UPInt           MovieDataMemory;
        UPInt           MeshCacheMemory;
        UPInt           FontCacheMemory;
    };

    class HeapStatBagVisitor : public GMemoryHeap::HeapVisitor
    {
    public:
        HeapStatBagVisitor(GStatBag* statBag) : StatBag(statBag) { }
        virtual void Visit(GMemoryHeap*, GMemoryHeap* heap)
        {
            if (heap->GetStats(StatBag))
            {
                heap->VisitChildHeaps(this);
            }
        }

        const GStatBag* GetStatBag() const { return StatBag; }

    private:
        GStatBag* StatBag;
    };

    // Custom heap visitor class that keeps track of Movie, Mesh, and Font memory
    class HeapTypeVisitor : public GMemoryHeap::HeapVisitor
    {
    public:
        HeapTypeVisitor(GMemoryHeap* heap, GFxAmpMemItem* rootItem, StatsUpdate* memReporter) : 
          RootHeap(heap),
              MovieViewMemory(0), 
              MovieDataMemory(0), 
              OtherMemory(0),
              NextHandle(rootItem->ID + 1),
              MemReporter(memReporter)            
          {
              heap->GetRootStats(&RootHeapStats);

              SysTotalItem = rootItem->AddChild(NextHandle++, "Total Footprint", 
                  static_cast<UInt32>(RootHeapStats.SysMemFootprint));
              SysTotalItem->StartExpanded = true;
              SysUsedItem = SysTotalItem->AddChild(NextHandle++, "Used Space");
              SysUsedItem->StartExpanded = true;
              SysTotalItem->AddChild(NextHandle++, "Debug Data", 
                  static_cast<UInt32>(RootHeapStats.UserDebugFootprint + RootHeapStats.DebugInfoFootprint));
              if (RootHeapStats.PageMapFootprint > 0)
              {
                  SysTotalItem->AddChild(NextHandle++, "Page Map", 
                      static_cast<UInt32>(RootHeapStats.PageMapFootprint));
              }
              if (RootHeapStats.BookkeepingFootprint > 0)
              {
                  SysTotalItem->AddChild(NextHandle++, "Bookkeeping", 
                      static_cast<UInt32>(RootHeapStats.BookkeepingFootprint));
              }
              SysUnusedItem = SysTotalItem->AddChild(NextHandle++, "Unused Space");

              GFxAmpMemItem* globalMemItem = SysUsedItem->AddChild(NextHandle++, "Global Heap", 
                  static_cast<UInt32>(heap->GetUsedSpace()));
              ViewMemItem = SysUsedItem->AddChild(NextHandle++, "Movie View Heaps");
              DataMemItem = SysUsedItem->AddChild(NextHandle++, "Movie Data Heaps");
              OtherMemItem = SysUsedItem->AddChild(NextHandle++, "Other Heaps");

#ifdef GHEAP_DEBUG_INFO
              GStatBag statBag;
              heap->GetStats(&statBag);
              MemReporter->GetStatMemory(GStatDesc::GetGroupIterator(GStat_Mem), &statBag, globalMemItem);
#else
              GUNUSED(globalMemItem);
#endif
              SysUnusedItem->AddChild(NextHandle++, heap->GetName(), static_cast<UInt32>(heap->GetFootprint() - heap->GetUsedSpace()));
          }

          ~HeapTypeVisitor()
          {
              SysUsedItem->SetValue(static_cast<UInt32>(RootHeap->GetUsedSpace() + GetTotalMemory()));
              SysUnusedItem->SetValue(static_cast<UInt32>(RootHeapStats.SysMemFootprint - RootHeapStats.UserDebugFootprint - RootHeapStats.DebugInfoFootprint - RootHeap->GetUsedSpace() - GetTotalMemory()));
              ViewMemItem->SetValue(static_cast<UInt32>(MovieViewMemory));
              DataMemItem->SetValue(static_cast<UInt32>(MovieDataMemory));
              OtherMemItem->SetValue(static_cast<UInt32>(OtherMemory));
          }

          virtual void Visit(GMemoryHeap* parent, GMemoryHeap* heap)
          {
              GUNUSED(parent);
              if (((heap->GetFlags() & GMemoryHeap::Heap_UserDebug) == 0))
              {
                  GFxAmpMemItem* heapGroupMemItem = NULL;
                  switch (heap->GetId())
                  {
                  case GHeapId_MovieView:
                      MovieViewMemory += heap->GetTotalUsedSpace();
                      heapGroupMemItem = ViewMemItem;
                      break;
                  case GHeapId_MovieData:
                      MovieDataMemory += heap->GetTotalUsedSpace();
                      heapGroupMemItem = DataMemItem;
                      break;
                  default:
                      OtherMemory += heap->GetTotalUsedSpace();
                      heapGroupMemItem = OtherMemItem;
                      break;
                  }

                  GFxAmpMemItem* heapMemItem = heapGroupMemItem->AddChild(NextHandle++, heap->GetName(), 0);
#ifdef GHEAP_DEBUG_INFO
                  GStatBag statBag;
                  HeapStatBagVisitor visitor(&statBag);
                  visitor.Visit(NULL, heap);
                  MemReporter->GetStatMemory(GStatDesc::GetGroupIterator(GStat_Mem), &statBag, heapMemItem);
#endif
                  heapMemItem->SetValue(static_cast<UInt32>(heap->GetTotalUsedSpace()));
                  SysUnusedItem->AddChild(NextHandle++, heap->GetName(), static_cast<UInt32>(heap->GetFootprint() - heap->GetUsedSpace()));
              }
          }


    private:
        UPInt   GetTotalMemory() const        { return MovieViewMemory + MovieDataMemory + OtherMemory; }

        GMemoryHeap::RootStats  RootHeapStats;
        GMemoryHeap*            RootHeap;
        GFxAmpMemItem*          SysTotalItem;
        GFxAmpMemItem*          SysUsedItem;
        GFxAmpMemItem*          SysUnusedItem;
        GFxAmpMemItem*          ViewMemItem;
        GFxAmpMemItem*          DataMemItem;
        GFxAmpMemItem*          OtherMemItem;
        UPInt                   MovieViewMemory;
        UPInt                   MovieDataMemory;
        UPInt                   OtherMemory;
        UInt32                  NextHandle;
        StatsUpdate*            MemReporter;
    };

    // This struct holds the memory stats for a file
    // It is part of the heap visitor, FileVisitor, below
    struct FileStats
    {
        FileStats() : TotalMemory(0) { }
        GStatBag        StatBag;
        UPInt           TotalMemory;
    };

    // Heap visitor that traverses the tree and creates a map of 
    // file names to memory statistics
    // The total memory for each file is not updated by the visitor
    struct FileVisitor : public GMemoryHeap::HeapVisitor
    {
        GStringHash<FileStats> FileStatsMap;

        // recursively updates the file statistics by examining child heaps
        void UpdateMovieHeap(GMemoryHeap* heap, GStatBag* fileStatBag)
        {
            if ((heap->GetFlags() & GMemoryHeap::Heap_UserDebug) == 0)
            {
                GStatBag statBag;
                heap->GetStats(&statBag);
                *fileStatBag += statBag;

                HolderVisitor visitor;
                heap->VisitChildHeaps(&visitor);
                for (UPInt i = 0; i < visitor.Heaps.GetSize(); ++i)
                {
                    UpdateMovieHeap(visitor.Heaps[i], fileStatBag);
                }
            }
        }

        virtual void Visit(GMemoryHeap* parent, GMemoryHeap* heap)
        { 
            GUNUSED(parent);

            if ((heap->GetFlags() & GMemoryHeap::Heap_UserDebug) != 0)
            {
                return;
            }

            switch (heap->GetId())
            {
            case GHeapId_MovieDef:
            case GHeapId_MovieData:
            case GHeapId_MovieView:
                {
                    // Recover the file name
                    GString heapName = heap->GetName();
                    UPInt heapNameLength = heapName.GetLength();
                    UPInt startIndex = 0;
                    UPInt endIndex = heapNameLength;
                    for (UPInt i = 0; i < heapNameLength; ++i)
                    {
                        UPInt iIndex = heapNameLength - i - 1;
                        if (heapName[iIndex] == '"')
                        {
                            if (endIndex == heapNameLength)
                            {
                                endIndex = iIndex;
                            }
                            else
                            {
                                startIndex = iIndex + 1;
                                break;
                            }
                        }
                    }
                    GASSERT(startIndex < endIndex);
                    GString filename = heapName.Substring(startIndex, endIndex);

                    // Add the file name to the map
                    GStringHash<FileStats>::Iterator it = FileStatsMap.FindCaseInsensitive(filename);
                    if (it == FileStatsMap.End())
                    {
                        FileStatsMap.SetCaseInsensitive(filename, FileStats());
                        it = FileStatsMap.FindCaseInsensitive(filename);
                    }

                    // update the stats for this file
                    UpdateMovieHeap(heap, &(it->Second.StatBag));
                }
                break;
            default:
                break;
            }
        }
    };

    UInt32 GetRoundUpKilobytes(UPInt bytes);
    UInt32 GetNearestKilobytes(UPInt bytes);

    void MemReportStatId(GFxAmpMemItem* rootItem, GMemoryHeap::MemReportType reportType, UPInt systemMemory);
    UPInt GetStatIdMemory(GStatDesc::Iterator it, StatIdVisitor& heapVisitor, GFxAmpMemItem* rootItem, GMemoryHeap::MemReportType reportType);
    void MemReportHeaps(GMemoryHeap* pHeap, GFxAmpMemItem* rootItem, GMemoryHeap::MemReportType reportType);
    void GetHeapMemory(GStatDesc::Iterator it, GStatBag* statBag, GFxAmpMemItem* rootItem);
    UInt32 GetStatMemory(GStatDesc::Iterator it, GStatBag* statBag, GFxAmpMemItem* rootItem);
    void MemReportFile(GFxAmpMemItem* rootItem, GMemoryHeap::MemReportType reportType);
    void MemReportHeapsDetailed(GFxAmpMemItem* rootItem, GMemoryHeap* heap);
    UPInt GetFileMemory(GStatDesc::Iterator it, FileStats& kFileStats, GFxAmpMemItem* rootItem, GMemoryHeap::MemReportType reportType);

    UInt32 NextHandle;
};

void StatsUpdate::MemReport(GFxAmpMemItem* rootItem, GMemoryHeap::MemReportType reportType)
{
    rootItem->ID = NextHandle++;
    rootItem->StartExpanded = true;

    GMemoryHeap::RootStats rootStats;
    GMemory::pGlobalHeap->GetRootStats(&rootStats);

    switch (reportType)
    {
    case GMemoryHeap::MemReportHeapDetailed:
        MemReportHeapsDetailed(rootItem, GMemory::pGlobalHeap);
        break;

    case GMemoryHeap::MemReportSimple:
    case GMemoryHeap::MemReportSimpleBrief:
#ifdef GHEAP_DEBUG_INFO
        MemReportStatId(rootItem, reportType, rootStats.SysMemFootprint - rootStats.UserDebugFootprint - rootStats.DebugInfoFootprint);
#endif
        break;

    case GMemoryHeap::MemReportFileSummary:
        MemReportFile(rootItem, reportType);
        break;

    default:

        if (reportType != GMemoryHeap::MemReportSummary && reportType != GMemoryHeap::MemReportHeapsOnly)
        {
            G_Format(rootItem->Name,
                "Memory {0:sep:,}K / {1:sep:,}K",
                GetNearestKilobytes(rootStats.SysMemUsedSpace - rootStats.UserDebugFootprint - 
                rootStats.DebugInfoFootprint),
                GetNearestKilobytes(rootStats.SysMemFootprint - rootStats.UserDebugFootprint - 
                rootStats.DebugInfoFootprint));

            if (reportType != GMemoryHeap::MemReportBrief)
            {
                GFxAmpMemItem* sysSummaryItem = rootItem->AddChild(NextHandle++, "System Summary");
                if (rootStats.SysMemFootprint > 0)
                {
                    sysSummaryItem->AddChild(NextHandle++, "System Memory Footprint", static_cast<UInt32>(rootStats.SysMemFootprint));
                }
                if (rootStats.SysMemUsedSpace > 0)
                {
                    sysSummaryItem->AddChild(NextHandle++, "System Memory Used Space", static_cast<UInt32>(rootStats.SysMemUsedSpace));
                }
                if (rootStats.PageMapFootprint > 0)
                {
                    sysSummaryItem->AddChild(NextHandle++, "Page Mapping Footprint", static_cast<UInt32>(rootStats.PageMapFootprint));
                }
                if (rootStats.SysMemFootprint > 0)
                {
                    sysSummaryItem->AddChild(NextHandle++, "Page Mapping Used Space", static_cast<UInt32>(rootStats.PageMapUsedSpace));
                }
                if (rootStats.BookkeepingFootprint > 0)
                {
                    sysSummaryItem->AddChild(NextHandle++, "Bookkeeping Footprint", static_cast<UInt32>(rootStats.BookkeepingFootprint));
                }
                if (rootStats.BookkeepingUsedSpace > 0)
                {
                    sysSummaryItem->AddChild(NextHandle++, "Bookkeeping Used Space", static_cast<UInt32>(rootStats.BookkeepingUsedSpace));
                }
                if (rootStats.DebugInfoFootprint > 0)
                {
                    sysSummaryItem->AddChild(NextHandle++, "Debug Info Footprint", static_cast<UInt32>(rootStats.DebugInfoFootprint));
                }
                if (rootStats.DebugInfoUsedSpace > 0)
                {
                    sysSummaryItem->AddChild(NextHandle++, "Debug Info Used Space", static_cast<UInt32>(rootStats.DebugInfoUsedSpace));
                }
                if (rootStats.UserDebugFootprint > 0)
                {
                    sysSummaryItem->AddChild(NextHandle++, "HUD Footprint", static_cast<UInt32>(rootStats.UserDebugFootprint));
                }
                if (rootStats.UserDebugUsedSpace > 0)
                {
                    sysSummaryItem->AddChild(NextHandle++, "HUD Used Space", static_cast<UInt32>(rootStats.UserDebugUsedSpace));
                }
            }
        }

        if (reportType != GMemoryHeap::MemReportHeapsOnly)
        {
            GFxAmpMemItem* summaryItem;

            if (reportType == GMemoryHeap::MemReportBrief)
            {
                summaryItem = rootItem;
            }
            else
            {
                summaryItem = rootItem->AddChild(NextHandle++, "Summary");
            }
            summaryItem->StartExpanded = true;

#ifdef GHEAP_DEBUG_INFO
            SummaryStatIdVisitor statVisit;
            statVisit.Visit(NULL, GMemory::GetGlobalHeap());

            UPInt imageMemory = statVisit.GetStatIdMemory(GStatDesc::GetGroupIterator(GStat_Image_Mem));
            if (imageMemory > 0)
            {
                summaryItem->AddChild(NextHandle++, "Image", static_cast<UInt32>(imageMemory));
            }

#ifndef GFC_NO_SOUND
            UPInt soundMemory = statVisit.GetStatIdMemory(GStatDesc::GetGroupIterator(GStat_Sound_Mem));
            if (soundMemory > 0)
            {
                summaryItem->AddChild(NextHandle++, "Sound", static_cast<UInt32>(soundMemory)); 
            }
#endif

#ifndef GFC_NO_VIDEO
            UPInt videoMemory = statVisit.GetStatIdMemory(GStatDesc::GetGroupIterator(GStat_Video_Mem));
            if (videoMemory > 0)
            {
                summaryItem->AddChild(NextHandle++, "Video", static_cast<UInt32>(videoMemory));
            }
#endif
#endif
            SummaryMemoryVisitor heapVisit;
            heapVisit.Visit(NULL, GMemory::GetGlobalHeap());
            summaryItem->AddChild(NextHandle++, "Movie View", static_cast<UInt32>(heapVisit.GetMovieViewMemory()));
            summaryItem->AddChild(NextHandle++, "Movie Data", static_cast<UInt32>(heapVisit.GetMovieDataMemory()));
            summaryItem->AddChild(NextHandle++, "Mesh Cache", static_cast<UInt32>(heapVisit.GetMeshCacheMemory()));
            summaryItem->AddChild(NextHandle++, "Font Cache", static_cast<UInt32>(heapVisit.GetFontCacheMemory()));
        }

        if (reportType != GMemoryHeap::MemReportSummary && reportType != GMemoryHeap::MemReportBrief)
        {
            MemReportHeaps(GMemory::pGlobalHeap, rootItem, reportType);
        }
        break;
    }
}



UInt32 StatsUpdate::GetRoundUpKilobytes(UPInt bytes)
{
    return (static_cast<UInt32>(bytes) + 1023) / 1024;
}

UInt32 StatsUpdate::GetNearestKilobytes(UPInt bytes)
{
    return (static_cast<UInt32>(bytes) + 512) / 1024;
}

// Adds the memory per StatId
// Under each StatId the memory is broken down per heap
void StatsUpdate::MemReportStatId(GFxAmpMemItem* rootItem, GMemoryHeap::MemReportType reportType, UPInt systemMemory)
{
    rootItem->Name = "Total Allocated Memory";
    rootItem->SetValue(static_cast<UInt32>(systemMemory));

    StatIdVisitor heapVisitor;
    heapVisitor.Visit(GMemory::pGlobalHeap, GMemory::pGlobalHeap);

    UPInt totalUsed = GetStatIdMemory(GStatDesc::GetGroupIterator(GStat_Mem), heapVisitor, rootItem, reportType);

    UPInt totalOverhead = 0;
    UPInt numNonZeroHeaps = 0;
    int totalUnused = 0;
    UPInt totalFootprint = 0;
    GPtr<GFxAmpMemItem> overheadItem = *GHEAP_AUTO_NEW(rootItem) GFxAmpMemItem(NextHandle++);
    GPtr<GFxAmpMemItem> unusedItem = *GHEAP_AUTO_NEW(rootItem) GFxAmpMemItem(NextHandle++);
    GStatInfo           footprintInfo;
    GStat::StatValue    footprintValue;
    //GStatInfo           usedInfo;
    //GStat::StatValue    usedValue;
    GStatInfo           overheadInfo;
    GStat::StatValue    overheadValue;
    for (UPInt i = 0; i < heapVisitor.HeapStatsArray.GetSize(); ++i)
    {
        HeapStats* heapStats = heapVisitor.HeapStatsArray[i];
        heapStats->StatBag.GetStat(&footprintInfo, GStatHeap_LocalFootprint);
        footprintInfo.GetStat(&footprintValue, 0);

        //heapStats->StatBag.GetStat(&usedInfo, GStatHeap_LocalUsedSpace);
        //usedInfo.GetStat(&usedValue, 0);

        heapStats->StatBag.GetStat(&overheadInfo, GStatHeap_Bookkeeping);
        overheadInfo.GetStat(&overheadValue, 0);

        //int unusedAmount = G_Max(0, static_cast<int>(footprintValue.IValue) - static_cast<int>(usedValue.IValue) - static_cast<int>(overheadValue.IValue));
        int unusedAmount = G_Max(0, static_cast<int>(footprintValue.IValue) - static_cast<int>(heapVisitor.HeapUsedMemory[i]) - static_cast<int>(overheadValue.IValue));
        if (reportType != GMemoryHeap::MemReportSimpleBrief)
        {
            GString heapName("[Heap] ");
            heapName += heapStats->Heap->GetName();
            GMemoryHeap::HeapInfo kInfo;
            heapStats->Heap->GetHeapInfo(&kInfo);
            if (kInfo.pParent != NULL && kInfo.pParent != GMemory::GetGlobalHeap())
            {
                heapName += kInfo.pParent->GetName();
            }

            overheadItem->AddChild(NextHandle++, heapName, static_cast<UInt32>(overheadValue.IValue));

            if (unusedAmount > 0)
            {
                unusedItem->AddChild(NextHandle++, heapName, static_cast<UInt32>(unusedAmount));
            }
        }
        totalOverhead += overheadValue.IValue;
        totalUnused += unusedAmount;
        totalFootprint += footprintValue.IValue;
        ++numNonZeroHeaps;
    }

    if (totalOverhead != 0)
    {
        overheadItem->Name = "Overhead";
        overheadItem->SetValue(static_cast<UInt32>(totalOverhead));
        rootItem->Children.PushBack(overheadItem);
    }

    unusedItem->Name = "Unused Memory";
    unusedItem->SetValue(static_cast<UInt32>(systemMemory - totalUsed - totalOverhead));
    rootItem->Children.PushBack(unusedItem);

    UInt32 systemUnused = static_cast<UInt32>(totalFootprint - totalUsed) - static_cast<UInt32>(totalUnused);
    if (systemUnused != 0 && reportType != GMemoryHeap::MemReportSimpleBrief)
    {
        unusedItem->AddChild(NextHandle++, "System Unused Memory", systemUnused);
    }
}

UPInt StatsUpdate::GetStatIdMemory(GStatDesc::Iterator it, StatIdVisitor& heapVisitor, GFxAmpMemItem* rootItem, GMemoryHeap::MemReportType reportType)
{
    GStatInfo           statInfo;
    GStat::StatValue    statValue;

    GPtr<GFxAmpMemItem> statIdItem = *GHEAP_AUTO_NEW(rootItem) GFxAmpMemItem(NextHandle++);
    statIdItem->StartExpanded = true;

    UPInt total = 0;
    UPInt numNonZeroHeaps = 0;
    for (UPInt i = 0; i < heapVisitor.HeapStatsArray.GetSize(); ++i)
    {
        HeapStats* heapStats = heapVisitor.HeapStatsArray[i];
        if (heapStats->StatBag.GetStat(&statInfo, it.GetId()))
        {
            statInfo.GetStat(&statValue, 0);

            if (reportType != GMemoryHeap::MemReportSimpleBrief)
            {
                GMemoryHeap::HeapInfo kInfo;
                heapStats->Heap->GetHeapInfo(&kInfo);
                GString heapName("[Heap] ");
                heapName += heapStats->Heap->GetName();
                if (kInfo.pParent != NULL && kInfo.pParent != GMemory::GetGlobalHeap())
                {
                    heapName += kInfo.pParent->GetName();
                }
                statIdItem->AddChild(NextHandle++, heapName, static_cast<UInt32>(statValue.IValue));
                statIdItem->StartExpanded = false;
            }
            heapVisitor.HeapUsedMemory[i] += statValue.IValue;
            total += statValue.IValue;
            ++numNonZeroHeaps;
        }
    }

    for (GStatDesc::Iterator itChild = it->GetChildIterator(); !itChild.IsEnd(); ++itChild)
    {
        UPInt childTotal = GetStatIdMemory(itChild, heapVisitor, statIdItem, reportType);
        if (childTotal > 0)
        {
            total += childTotal;
            ++numNonZeroHeaps;
        }
    }

    if (total > 0)
    {
        statIdItem->Name = it->GetName();
        statIdItem->SetValue(static_cast<UInt32>(total));
        rootItem->Children.PushBack(statIdItem);
    }

    return total;
}

void StatsUpdate::MemReportHeaps(GMemoryHeap* pHeap, GFxAmpMemItem* rootItem, GMemoryHeap::MemReportType reportType)
{
    if (((pHeap->GetFlags() & GMemoryHeap::Heap_UserDebug) == 0) && pHeap->GetFootprint() > 0)
    {
        GStatBag            statBag;
        GStatInfo           statInfo;
        GStat::StatValue    statValue;

        pHeap->GetStats(&statBag);

        statBag.GetStat(&statInfo, GStatHeap_TotalFootprint);
        statInfo.GetStat(&statValue, 0);

        GString buffer;
        G_Format(buffer, "[Heap] {0}", pHeap->GetName());
        GFxAmpMemItem* heapItem = rootItem->AddChild(NextHandle++, buffer, static_cast<UInt32>(statValue.IValue));

#ifdef GHEAP_DEBUG_INFO
        if (reportType == GMemoryHeap::MemReportFull || reportType == GMemoryHeap::MemReportMedium)
        {
            GStatDesc::Iterator it = GStatDesc::GetGroupIterator(GStatGroup_Core);
#ifdef GFC_BUILD_DEBUG
            if (reportType == GMemoryHeap::MemReportFull)
            {
                it = GStatDesc::GetGroupIterator(GStatGroup_Default);
            }
#endif
            GetHeapMemory(it, &statBag, heapItem);

            statBag.UpdateGroups();
            GStatInfo sumStat;
            if (statBag.GetStat(&sumStat, GStat_Mem))
            {
                heapItem->AddChild(NextHandle++, "Allocations Count", static_cast<UInt32>(sumStat.ToMemoryStat()->GetAllocCount()));
            }
        }
#endif
        if (reportType != GMemoryHeap::MemReportMedium)
        {
            HolderVisitor hv;
            pHeap->VisitChildHeaps(&hv);

            for (UPInt i = 0; i < hv.Heaps.GetSize(); ++i)
            {
                MemReportHeaps(hv.Heaps[i], heapItem, reportType);
            }
        }
    }
}

// Invokes recursively memory statistic data belonging to the given heap.
void StatsUpdate::GetHeapMemory(GStatDesc::Iterator it, GStatBag* statBag, GFxAmpMemItem* rootItem)
{
    const GStatDesc* pdesc = *it;

    if (pdesc)
    {
        GPtr<GFxAmpMemItem> childItem;
        if (pdesc->GetGroupId() == 0)
        {
            if (pdesc->GetId() != 0)
            {
                if ((pdesc->GetType() == (UByte)GStat::Stat_Memory) ||
                    (pdesc->GetType() == (UByte)GStat::Stat_LogicalGroup))
                {
                    childItem = *GHEAP_AUTO_NEW(rootItem) GFxAmpMemItem(NextHandle++);
                    childItem->Name = pdesc->GetName();
                    if (pdesc->GetId() != GStatHeap_Summary)
                    {
                        childItem->StartExpanded = true;
                    }
                }
            }
        }

        if (!childItem)
        {
            childItem = *GHEAP_AUTO_NEW(rootItem) GFxAmpMemItem(NextHandle++);
            childItem->Name = pdesc->GetName();
        }

        GStatDesc::Iterator ichild = pdesc->GetChildIterator();  
        while (!ichild.IsEnd())
        {
            if ((pdesc->GetGroupId() == 0) && (pdesc->GetId() == 0))
            {
                GetHeapMemory(ichild, statBag, rootItem);
            }
            else if (childItem)
            {
                GetHeapMemory(ichild, statBag, childItem);
            }

            if (childItem)
            {
                GStatInfo           statInfo;
                GStat::StatValue    statValue;

                if (statBag->GetStat(&statInfo, ichild.GetId()))
                {
                    statInfo.GetStat(&statValue, 0);
                    if (statValue.IValue > 0)
                    {
                        childItem->AddChild(NextHandle++, statInfo.GetName(), static_cast<UInt32>(statValue.IValue));
                    }
                }
            }
            ++ichild;
        }

        if (childItem && childItem->Children.GetSize() > 0)
        {
            rootItem->Children.PushBack(childItem);
        }
    }
}

// Invokes recursively memory statistic data belonging to the given heap.
UInt32 StatsUpdate::GetStatMemory(GStatDesc::Iterator it, GStatBag* statBag, GFxAmpMemItem* rootItem)
{
    UInt32 totalMemory = 0;
    const GStatDesc* pdesc = *it;

    if (pdesc)
    {
        GStatDesc::Iterator ichild = pdesc->GetChildIterator();  
        while (!ichild.IsEnd())
        {
            GStatInfo           statInfo;
            GStat::StatValue    statValue;
            GPtr<GFxAmpMemItem> childItem = *GHEAP_AUTO_NEW(rootItem) GFxAmpMemItem(NextHandle++);
            childItem->Name = ichild->GetName();

            UInt32 childMemory = GetStatMemory(ichild, statBag, childItem);

            if (statBag->GetStat(&statInfo, ichild.GetId()))
            {
                statInfo.GetStat(&statValue, 0);
                childMemory += static_cast<UInt32>(statValue.IValue);
            }

            childItem->SetValue(childMemory);
            if (childMemory > 0)
            {
                rootItem->Children.PushBack(childItem);
            }
            totalMemory += childMemory;

            ++ichild;
        }
    }
    return totalMemory;
}

void StatsUpdate::MemReportFile(GFxAmpMemItem* rootItem, GMemoryHeap::MemReportType reportType)
{
    FileVisitor visitor;
    GMemory::pGlobalHeap->VisitChildHeaps(&visitor);

    GStringHash<FileStats>& statsMap = visitor.FileStatsMap;
    GStringHash<FileStats>::Iterator itFile;
    for (itFile = statsMap.Begin(); itFile != statsMap.End(); ++itFile)
    {
        GString buffer;
        G_Format(buffer, "Movie File {0}", itFile->First);
        GFxAmpMemItem* childItem = rootItem->AddChild(NextHandle++, buffer);

        GetFileMemory(GStatDesc::GetGroupIterator(GStat_Mem), itFile->Second, childItem, reportType);
    }
}

void StatsUpdate::MemReportHeapsDetailed(GFxAmpMemItem* rootItem, GMemoryHeap* heap)
{
    HeapTypeVisitor heapVisit(heap, rootItem, this);
    heap->VisitChildHeaps(&heapVisit);
}

// Recursively adds the memory per StatId for the given file
UPInt StatsUpdate::GetFileMemory(GStatDesc::Iterator it, FileStats& fileStats, GFxAmpMemItem* rootItem, GMemoryHeap::MemReportType reportType)
{
    GStatInfo           statInfo;
    GStat::StatValue    statValue;

    UPInt total = 0;
    if (fileStats.StatBag.GetStat(&statInfo, it.GetId()))
    {
        statInfo.GetStat(&statValue, 0);

        fileStats.TotalMemory += statValue.IValue;
        total += statValue.IValue;
    }

    GPtr<GFxAmpMemItem> childItem = *GHEAP_AUTO_NEW(rootItem) GFxAmpMemItem(NextHandle++);
    for (GStatDesc::Iterator itChild = it->GetChildIterator(); !itChild.IsEnd(); ++itChild)
    {

        UPInt childTotal = GetFileMemory(itChild, fileStats, childItem, reportType);
        if (childTotal > 0)
        {
            total += childTotal;
        }
    }

    if (total > 0)
    {
        childItem->Name = it->GetName();
        childItem->SetValue(static_cast<UInt32>(total));
        rootItem->Children.PushBack(childItem);
    }

    return total;
}

//------------------------------------------------------------------------
// Collects memory statistic data from heaps and puts obtained data in string
// buffer. 
void GMemoryHeap::MemReport(GStringBuffer& buffer, MemReportType reportType)
{   
    GPtr<GFxAmpMemItem> rootItem = *GNEW GFxAmpMemItem(0);
    StatsUpdate().MemReport(rootItem, reportType);
    rootItem->ToString(&buffer);
}

void GMemoryHeap::MemReport(GFxAmpMemItem* rootItem, MemReportType reportType)
{
    StatsUpdate().MemReport(rootItem, reportType);
}

#else // GFC_NO_STAT

void GMemoryHeap::MemReport(GStringBuffer& buffer, MemReportType reportType)
{
    GUNUSED(buffer);
    GUNUSED(reportType);
}

void GMemoryHeap::MemReport(GFxAmpMemItem* rootItem, MemReportType reportType)
{
    GUNUSED(rootItem);
    GUNUSED(reportType);
}


#endif // GFC_NO_STAT
