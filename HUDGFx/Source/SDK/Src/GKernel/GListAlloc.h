/**********************************************************************

Filename    :   GListAlloc.h
Content     :   A simple and fast allocator for "recyclable" elements.
Created     :   2008
Authors     :   MaximShemanarev
Copyright   :   (c) 2001-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GListAlloc_H
#define INC_GListAlloc_H

#include "GAllocator.h"

// ***** GListAllocBase
//
// A simple and fast allocator for "recyclable" elements. It allocates
// pages of elements of "PageSize" and never performs reallocations, 
// maintaining minimal memory fragmentation. The allocator also keeps
// a linked list of freed elements. 
//
// The allocator is mostly used together with GList and GList2.
//
// Function Alloc() first checks the freeMem list, and if not empty returns 
// the first freeMem element. If the freeMem list is empty the function 
// allocates elements in pages, creating new pages as needed. 
//
// Function Free(T* element) recycles the element adding it into the 
// freeMem list.
//
// The constructors and destructors are called in Alloc() and Free()
// respectively. Constructors and destructors are not called in the 
// "POD" modification of the allocator.
// 
// Function ClearAndRelease() and the allocator's destructor DO NOT call 
// elements' destructors! They just freeMem the allocated pages. 
//
// The use policy is that if you Alloc() the element you must Free() it.
// The reason of such a behavior is that it's impossible to determine 
// whether or not the element in the page was destroyed, so that, 
// ClearAndRelease cannot call the elements' destructors, otherwise 
// there will be "double destruction".
//------------------------------------------------------------------------
template<class T, int PageSize, class Allocator> class GListAllocBase
{
public:
    typedef T                                       ValueType;
    typedef Allocator                               AllocatorType;
    typedef GListAllocBase<T, PageSize, Allocator>  SelfType;

private:
    struct PageType
    {
        ValueType   Data[PageSize];
        PageType*   pNext;
    };

    struct NodeType
    {
        NodeType*   pNext;
    };
protected:
    GListAllocBase(void* p) :
        FirstPage(0),
        LastPage(0),
        NumElementsInPage(PageSize),
        FirstEmptySlot(0),
        pHeapOrPtr(p)
    {
        GCOMPILER_ASSERT(sizeof(T) >= sizeof(T*));
    }

public:
    GListAllocBase() :
        FirstPage(0),
        LastPage(0),
        NumElementsInPage(PageSize),
        FirstEmptySlot(0)
    {
        GCOMPILER_ASSERT(sizeof(T) >= sizeof(T*));
        pHeapOrPtr = this;
    }

    ~GListAllocBase()
    {
        freeMem();
    }

    void ClearAndRelease()
    {
        freeMem();
        FirstPage           = 0;
        LastPage            = 0;
        NumElementsInPage   = PageSize;
        FirstEmptySlot      = 0;
    }

    UPInt GetNumBytes() const
    {
        UPInt numBytes = 0;
        const PageType* page = FirstPage;
        while (page)
        {
            numBytes += sizeof(PageType);
            page = page->pNext;
        }
        return numBytes;
    }

    T* Alloc()
    {
        T* ret = allocate();
        Allocator::Construct(ret);
        return ret;
    }

    T* Alloc(const T& v)
    {
        T* ret = allocate();
        Allocator::Construct(ret, v);
        return ret;
    }

    template<class S>
    T* AllocAlt(const S& v)
    {
        ValueType* ret = allocate();
        Allocator::ConstructAlt(ret, v);
        return ret;
    }

    void Free(T* element)
    {
        Allocator::Destruct(element);
        ((NodeType*)element)->pNext = FirstEmptySlot;
        FirstEmptySlot = (NodeType*)element;
    }

private:
    // Copying is prohibited
    GListAllocBase(const SelfType&);
    const SelfType& operator = (SelfType&);

    GINLINE T* allocate()
    {
        if (FirstEmptySlot)
        {
            T* ret = (T*)FirstEmptySlot;
            FirstEmptySlot = FirstEmptySlot->pNext;
            return ret;
        }
        if (NumElementsInPage < PageSize)
        {   
            return &LastPage->Data[NumElementsInPage++];
        }

        PageType* next = (PageType*)Allocator::Alloc(pHeapOrPtr, sizeof(PageType), __FILE__, __LINE__);
        next->pNext = 0;
        if (LastPage)
            LastPage->pNext = next;
        else
            FirstPage = next;
        LastPage = next;
        NumElementsInPage = 1;
        return &LastPage->Data[0];
    }

    GINLINE void freeMem()
    {
        PageType* page = FirstPage;
        while (page)
        {
            PageType* next = page ->pNext;
            Allocator::Free(page);
            page = next;
        }
    }

    PageType*   FirstPage;
    PageType*   LastPage;
    unsigned    NumElementsInPage;
    NodeType*   FirstEmptySlot;
    void*       pHeapOrPtr;
};



// ***** GListAllocPOD
//
// Paged list allocator for objects that DOES NOT require  
// construction/destruction. Constructors and destructors are not called! 
// Global heap is in use.
//------------------------------------------------------------------------
template<class T, int PageSize=127, int SID=GStat_Default_Mem>
class GListAllocPOD : 
    public GListAllocBase<T, PageSize, GAllocatorGH_POD<T, SID> >
{
public:
    typedef GListAllocPOD<T, PageSize, SID>     SelfType;
    typedef T                                   ValueType;
    typedef GAllocatorGH_POD<T, SID>            AllocatorType;
};


// ***** GListAlloc
//
// Paged list allocator array for objects that require 
// explicit construction/destruction. 
// Global heap is in use.
//------------------------------------------------------------------------
template<class T, int PageSize=127, int SID=GStat_Default_Mem>
class GListAlloc : 
    public GListAllocBase<T, PageSize, GAllocatorGH<T, SID> >
{
public:
    typedef GListAlloc<T, PageSize, SID>        SelfType;
    typedef T                                   ValueType;
    typedef GAllocatorGH<T, SID>                AllocatorType;
};


// ***** GListAllocLH_POD
//
// Paged list allocator for objects that DOES NOT require  
// construction/destruction. Constructors and destructors are not called! 
// Local heap is in use.
//------------------------------------------------------------------------
template<class T, int PageSize=127, int SID=GStat_Default_Mem>
class GListAllocLH_POD : 
    public GListAllocBase<T, PageSize, GAllocatorLH_POD<T, SID> >
{
public:
    typedef GListAllocLH_POD<T, PageSize, SID>  SelfType;
    typedef T                                   ValueType;
    typedef GAllocatorLH_POD<T, SID>            AllocatorType;
};


// ***** GListAllocLH
//
// Paged list allocator for objects that require 
// explicit construction/destruction. 
// Local heap is in use.
//------------------------------------------------------------------------
template<class T, int PageSize=127, int SID=GStat_Default_Mem>
class GListAllocLH : 
    public GListAllocBase<T, PageSize, GAllocatorLH<T, SID> >
{
public:
    typedef GListAllocLH<T, PageSize, SID>      SelfType;
    typedef T                                   ValueType;
    typedef GAllocatorLH<T, SID>                AllocatorType;
};

// ***** GListAllocDH_POD
//
// Paged list allocator for objects that DOES NOT require  
// construction/destruction. Constructors and destructors are not called! 
// The specified heap is in use.
//------------------------------------------------------------------------
template<class T, int PageSize=127, int SID=GStat_Default_Mem>
class GListAllocDH_POD : 
    public GListAllocBase<T, PageSize, GAllocatorDH_POD<T, SID> >
{
public:
    typedef GListAllocDH_POD<T, PageSize, SID>  SelfType;
    typedef T                                   ValueType;
    typedef GAllocatorDH_POD<T, SID>            AllocatorType;

    GListAllocDH_POD(void* pHeap)
     : GListAllocBase<T, PageSize, GAllocatorDH_POD<T, SID> >(pHeap) {}
};


// ***** GListAllocDH
//
// Paged list allocator for objects that require 
// explicit construction/destruction. 
// The specified heap is in use.
//------------------------------------------------------------------------
template<class T, int PageSize=127, int SID=GStat_Default_Mem>
class GListAllocDH : 
    public GListAllocBase<T, PageSize, GAllocatorDH<T, SID> >
{
public:
    typedef GListAllocDH<T, PageSize, SID>      SelfType;
    typedef T                                   ValueType;
    typedef GAllocatorDH<T, SID>                AllocatorType;

    GListAllocDH(void* pHeap)
     : GListAllocBase<T, PageSize, GAllocatorDH<T, SID> >(pHeap) {}
};

#endif

