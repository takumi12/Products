/**********************************************************************

Filename    :   GHeapRootMH.h
Content     :   Heap root used for bootstrapping and bookkeeping.
            :   
Created     :   2009
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GHeapRootMH_H
#define INC_GHeapRootMH_H

#include "GAtomic.h"
#include "GMemoryHeapMH.h"
#include "GSysAlloc.h"
#include "GRadixTree.h"

//------------------------------------------------------------------------
struct GHeapPageMH : public GListNode<GHeapPageMH>
{
    enum 
    { 
        PageSize    = 4096, 
        PageMask    = PageSize-1, 
        UnitShift   = 4,
        UnitSize    = 1 << UnitShift,
        UnitMask    = UnitSize - 1,
        BitSetBytes = PageSize/UnitSize/4,
        MaxBytes    = 512
    };

    GMemoryHeapMH *pHeap;     // Heap this page belongs to.
    UByte*         Start;
};


//------------------------------------------------------------------------
struct GPageTableMH
{
    enum 
    {
        Index1Shift = 7,
        TableSize   = 1 << Index1Shift,
        Index0Mask  = TableSize - 1
    };

    struct Level0Entry
    {
        GHeapPageMH*    FirstPage;
        UPInt           SizeMask;
    };

    Level0Entry Entries[TableSize];
};

struct GHeapDebugDataMH;

//------------------------------------------------------------------------
struct GHeapMagicHeader
{
    enum { MagicValue = 0x5FC0 }; // "SFCO" - Scaleform Corporation

    UInt16 Magic;       // Magic marker
    UInt16 UseCount;    // Existing allocation count
    UInt32 Index;       // 2-component index in page tables
    GHeapDebugDataMH* DebugHeader; 
#ifndef GFC_64BIT_POINTERS
    UInt32 Filler;
#endif

    UInt32  GetIndex0() const { return Index &  GPageTableMH::Index0Mask; }
    UInt32  GetIndex1() const { return Index >> GPageTableMH::Index1Shift; }
};

//------------------------------------------------------------------------
struct GHeapNodeMH
{
    enum { Align4 = 0, Align8 = 1, Align16 = 2, MoreInfo = 3 };

    GHeapNodeMH* Parent;
    GHeapNodeMH* Child[2];
    UPInt        pHeap;
#ifdef GHEAP_DEBUG_INFO
    GHeapDebugDataMH* DebugHeader;
#endif
    //------------------ Optional data
    UPInt   Align;

    static UPInt GetNodeSize(UPInt align)
    {
        UPInt size = 4*sizeof(void*);
#ifdef GHEAP_DEBUG_INFO
        size += sizeof(void*);
#endif
        return (align <= 16) ? size : size + sizeof(UPInt);
    }

    GMemoryHeapMH* GetHeap() const 
    { 
        return (GMemoryHeapMH*)(pHeap & ~UPInt(3)); 
    }

    UPInt GetAlign() const
    {
        return ((pHeap & 3) < 3) ? (UPInt(1) << ((pHeap & 3) + 2)) : Align;
    }

    void SetHeap(GMemoryHeapMH* heap, UPInt align)
    {
        switch(align)
        {
        case 1: case 2: case 4:
            pHeap = UPInt(heap);
            break;

        case 8:
            pHeap = UPInt(heap) | Align8;
            break;

        case 16:
            pHeap = UPInt(heap) | Align16;
            break;

        default:
            pHeap = UPInt(heap) | MoreInfo;
            Align = align;
            break;
        }
    }
};


//--------------------------------------------------------------------
struct GHeapTreeNodeAccessor
{
    static       UPInt         GetKey     (const GHeapNodeMH* n)          { return  UPInt(n); }
    static       GHeapNodeMH*  GetChild   (      GHeapNodeMH* n, UPInt i) { return  n->Child[i]; }
    static const GHeapNodeMH*  GetChild   (const GHeapNodeMH* n, UPInt i) { return  n->Child[i]; }
    static       GHeapNodeMH** GetChildPtr(      GHeapNodeMH* n, UPInt i) { return &n->Child[i]; }

    static       GHeapNodeMH*  GetParent  (      GHeapNodeMH* n)          { return n->Parent; }
    static const GHeapNodeMH*  GetParent  (const GHeapNodeMH* n)          { return n->Parent; }

    static void SetParent (GHeapNodeMH* n, GHeapNodeMH* p)                { n->Parent   = p; }
    static void SetChild  (GHeapNodeMH* n, UPInt i, GHeapNodeMH* c)       { n->Child[i] = c; }
    static void ClearLinks(GHeapNodeMH* n) { n->Parent = n->Child[0] = n->Child[1] = 0; }
};


//------------------------------------------------------------------------
struct GHeapMagicHeadersInfo
{
    GHeapMagicHeader* Header1; // Header on the left ptr or 0
    GHeapMagicHeader* Header2; // Header on the right ptr or 0
    UInt32*      BitSet;  // Pointer to the bitset
    UByte*       AlignedStart; // Pointer to the beginning of the page, aligned 
    UByte*       AlignedEnd;   // Pointer to the end of the page, aligned 
    UByte*       Bound;   // The bound between physical pages
    GHeapPageMH* Page;    // Page, null after GetMagicHeaders()
};


//------------------------------------------------------------------------
struct GHeapPageInfoMH
{
#ifdef GHEAP_DEBUG_INFO
    GHeapDebugDataMH** DebugHeaders[3];
#endif
    GHeapPageMH* Page;
    GHeapNodeMH* Node;
    UPInt        UsableSize;
};

//------------------------------------------------------------------------
void GHeapGetMagicHeaders(UPInt pageStart, GHeapMagicHeadersInfo* headers);


//------------------------------------------------------------------------
class GHeapRootMH
{
    typedef GRadixTree<GHeapNodeMH, GHeapTreeNodeAccessor> HeapTreeType;
public:
    GHeapRootMH(GSysAlloc* sysAlloc);
    ~GHeapRootMH();

    void                FreeTables();

    GSysAlloc*        GetSysAlloc() { return pSysAlloc; }

    GMemoryHeapMH*      CreateHeap(const char*  name, 
                                   GMemoryHeapMH* parent,
                                   const GMemoryHeap::HeapDesc& desc);

    void                DestroyHeap(GMemoryHeapMH* heap);

    GLockSafe*   GetLock()    { return &RootLock; }

    GHeapPageMH* AllocPage(GMemoryHeapMH* heap);
    void    FreePage(GHeapPageMH* page);
    UInt32  GetPageIndex(const GHeapPageMH* page) const;

    GHeapPageMH* ResolveAddress(UPInt addr) const;

    GHeapNodeMH* AddToGlobalTree(UByte* ptr, UPInt size, UPInt align, GMemoryHeapMH* heap)
    {
        GHeapNodeMH* node = (GHeapNodeMH*)(ptr + size);
        node->SetHeap(heap, align);
#ifdef GHEAP_DEBUG_INFO
        node->DebugHeader = 0;
#endif
        HeapTree.Insert(node);
        return node;
    }

    GHeapNodeMH* FindNodeInGlobalTree(UByte* ptr)
    {
        return (GHeapNodeMH*)HeapTree.FindGrEq(UPInt(ptr));
    }

    void    RemoveFromGlobalTree(GHeapNodeMH* node)
    {
        HeapTree.Remove(node);
    }

#ifdef GHEAP_TRACE_ALL
    void OnAlloc(const GMemoryHeapMH* heap, UPInt size, UPInt align, unsigned sid, const void* ptr);
    void OnRealloc(const GMemoryHeapMH* heap, const void* oldPtr, UPInt newSize, const void* newPtr);
    void OnFree(const GMemoryHeapMH* heap, const void* ptr);
#endif


private:
    bool allocPagePool();
    static void setMagic(UByte* pageStart, unsigned magicValue);

    GSysAlloc*        pSysAlloc;
    mutable GLockSafe   RootLock;
    GList<GHeapPageMH>  FreePages;
    unsigned            TableCount;
    HeapTreeType        HeapTree;

    static GMemoryHeap::HeapTracer* pTracer;
};

extern GHeapRootMH*  GHeapGlobalRootMH;
extern GPageTableMH  GHeapGlobalPageTableMH;
extern GHeapPageMH   GHeapGlobalEmptyPage;

#endif
