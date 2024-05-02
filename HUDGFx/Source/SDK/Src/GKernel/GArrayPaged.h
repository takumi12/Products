/**********************************************************************

Filename    :   GArrayPaged.h
Content     :   
Created     :   
Authors     :   Maxim Shemanarev
Copyright   :   (c) 2001-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GArrayPaged_H
#define INC_GArrayPaged_H

#include "GAllocator.h"


// ***** GConstructorPagedPOD
//
// A modification of GConstructorPOD with paged construction/destruction
// Used to avoid possible run-time overhead for POD types.
//------------------------------------------------------------------------
template<class T> 
class GConstructorPagedPOD : public GConstructorPOD<T>
{
public:
    static void ConstructArrayPaged(T**, UPInt, UPInt, UPInt, UPInt) {}
    static void DestructArrayPaged (T**, UPInt, UPInt, UPInt, UPInt) {}
};


// ***** GConstructorPagedMov
//
// Correct C++ paged construction and destruction
//------------------------------------------------------------------------
template<class T> 
class GConstructorPagedMov : public GConstructorMov<T>
{
public:
    static void ConstructArrayPaged(T** pages, UPInt start, UPInt end, UPInt pageShift, UPInt pageMask)
    {   
        for (UPInt i = start; i < end; ++i)
        {
            GConstructorMov<T>::Construct(pages[i >> pageShift] + (i & pageMask));
        }
    }

    static void DestructArrayPaged(T** pages, UPInt start, UPInt end, UPInt pageShift, UPInt pageMask)
    {   
        for (UPInt i = end; i > start; --i)
        {
            GConstructorMov<T>::Destruct(pages[(i-1) >> pageShift] + ((i-1) & pageMask));
        }
    }
};


// ***** GAllocatorPaged*
//
// Simple wraps as specialized allocators
//------------------------------------------------------------------------
template<class T, int SID> struct GAllocatorPagedGH_POD : GAllocatorBaseGH<SID>, GConstructorPagedPOD<T> {};
template<class T, int SID> struct GAllocatorPagedGH     : GAllocatorBaseGH<SID>, GConstructorPagedMov<T> {};

template<class T, int SID> struct GAllocatorPagedLH_POD : GAllocatorBaseLH<SID>, GConstructorPagedPOD<T> {};
template<class T, int SID> struct GAllocatorPagedLH     : GAllocatorBaseLH<SID>, GConstructorPagedMov<T> {};


// ***** GArrayPagedBase
//
// A simple class template to store data similar to std::deque
// It doesn't reallocate memory but instead, uses pages of data of size 
// of (1 << PageSh), that is, power of two. The data is NOT contiguous in memory, 
// so the only valid access methods are operator [], At(), ValueAt(), Front(), Back()
// The container guarantees the persistence of elements' addresses in memory, 
// so the elements can be used in intrusive linked lists. 
// 
// Reallocs occur only when the pool of pointers to pages needs 
// to be extended (happens very rarely). You can control the increment 
// value by PtrPoolInc. 
// 
//-------------------
// The code of this class template was taken from the Anti-Grain Geometry
// Project and modified for the use by Scaleform. 
// Permission to use without restrictions is hereby granted to 
// Scaleform Corp. by the author of Anti-Grain Geometry Project.
//------------------------------------------------------------------------
template<class T, int PageSh, int PtrPoolInc, class Allocator>
class GArrayPagedBase
{
public:
    enum PageConsts
    {   
        PageShift = PageSh,
        PageSize  = 1 << PageShift,
        PageMask  = PageSize - 1
    };

    typedef GArrayPagedBase<T, PageSh, PtrPoolInc, Allocator>   SelfType;
    typedef T                                                   ValueType;
    typedef Allocator                                           AllocatorType;

    ~GArrayPagedBase()
    {
        ClearAndRelease();
    }

    GArrayPagedBase() : 
        Size(0),
        NumPages(0),
        MaxPages(0),
        Pages(0)
    {}

    void ClearAndRelease()
    {
        if(NumPages)
        {
            T** blk = Pages + NumPages - 1;
            UPInt freeCount = Size & PageMask;
            while(NumPages--)
            {
                Allocator::DestructArray(*blk, freeCount);
                Allocator::Free(*blk);
                freeCount = PageSize;
                --blk;
            }
            Allocator::Free(Pages);
        }
        Size = NumPages = MaxPages = 0;
        Pages = 0;
    }

    void Clear()
    { 
        Allocator::DestructArrayPaged(Pages, 0, Size, PageShift, PageMask);
        Size = 0; 
    }

    void PushBack(const T& val)
    {
        Allocator::Construct(acquireDataPtr(), val);
        ++Size;
    }

    bool PushBackSafe(const T& val)
    {
        T* p = acquireDataPtrSafe();
        if (!p) return false;
        Allocator::Construct(p, val);
        ++Size;
        return true;
    }

    template<class S>
    void PushBackAlt(const S& val)
    {
        Allocator::ConstructAlt(acquireDataPtr(), val);
        ++Size;
    }

    void PopBack()
    {
        if(Size) 
        {
            Allocator::Destruct(&At(Size - 1));
            --Size;
        }
    }

    UPInt GetCapacity() const
    {
        return NumPages << PageShift;
    }

    UPInt GetNumBytes() const
    { 
        return GetCapacity() * sizeof(T) + MaxPages * sizeof(T*);
    }

    void Reserve(UPInt newCapacity)
    {
        if(newCapacity > GetCapacity())
        {
            UPInt newNumPages = (newCapacity + PageMask) >> PageShift;
            for(UPInt i = NumPages; i < newNumPages; ++i)
                allocatePage(i);
        }
    }

    void Resize(UPInt newSize)
    {
        if(newSize > Size)
        {
            UPInt newNumPages = (newSize + PageMask) >> PageShift;
            for(UPInt i = NumPages; i < newNumPages; ++i)
                allocatePage(i);
            Allocator::ConstructArrayPaged(Pages, Size, newSize, PageShift, PageMask);
            Size = newSize;
            return;
        }
        if(newSize < Size)
        {
            Allocator::DestructArrayPaged(Pages, newSize, Size, PageShift, PageMask);
            Size = newSize;
        }
    }

    void CutAt(UPInt newSize)
    {
        if(newSize < Size) 
        {
            Allocator::DestructArrayPaged(Pages, newSize, Size, PageShift, PageMask);
            Size = newSize;
        }
    }

    void InsertAt(UPInt pos, const T& val)
    {
        if(pos >= Size) 
        {
            PushBack(val);
        }
        else
        {
            Allocator::Construct(acquireDataPtr());
            ++Size;
            UPInt i;
            // TBD: Optimize page copying
            for(i = Size-1; i > pos; --i)
            {
                At(i) = At(i - 1);
            }
            At(pos) = val;
        }
    }

    void RemoveAt(UPInt pos)
    {
        if(Size)
        {
            // TBD: Optimize page copying
            for(++pos; pos < Size; pos++)
            {
                At(pos-1) = At(pos);
            }
            Allocator::Destruct(&At(Size - 1));
            --Size;
        }
    }

    UPInt GetSize() const 
    { 
        return Size; 
    }

    const T& operator [] (UPInt i) const
    {
        return Pages[i >> PageShift][i & PageMask];
    }

    T& operator [] (UPInt i)
    {
        return Pages[i >> PageShift][i & PageMask];
    }

    const T& At(UPInt i) const
    { 
        return Pages[i >> PageShift][i & PageMask];
    }

    T& At(UPInt i) 
    { 
        return Pages[i >> PageShift][i & PageMask];
    }

    T ValueAt(UPInt i) const
    { 
        return Pages[i >> PageShift][i & PageMask];
    }

    const T& Front() const
    {
        return Pages[0][0];
    }

    T& Front()
    {
        return Pages[0][0];
    }

    const T& Back() const
    {
        return At(Size - 1);
    }

    T& Back()
    {
        return At(Size - 1);
    }

private:
    // Copying is prohibited
    GArrayPagedBase(const SelfType&);
    const SelfType& operator = (const SelfType&);

    void allocatePage(UPInt nb)
    {
        if(nb >= MaxPages) 
        {
            if(Pages)
            {
                Pages = (T**)Allocator::Realloc(
                    Pages, (MaxPages + PtrPoolInc) * sizeof(T*), 
                    __FILE__, __LINE__);
            }
            else
            {
                Pages = (T**)Allocator::Alloc(
                    this, PtrPoolInc * sizeof(T*), 
                    __FILE__, __LINE__);
            }
            MaxPages += PtrPoolInc;
        }
        Pages[nb] = (T*)Allocator::Alloc(this, PageSize * sizeof(T), __FILE__, __LINE__);
        NumPages++;
    }

    GINLINE T* acquireDataPtr()
    {
        UPInt np = Size >> PageShift;
        if(np >= NumPages)
        {
            allocatePage(np);
        }
        return Pages[np] + (Size & PageMask);
    }

    bool allocatePageSafe(UPInt nb)
    {
        if(nb >= MaxPages) 
        {
            T** newPages;
            if(Pages)
            {
                newPages = (T**)Allocator::Realloc(
                    Pages, (MaxPages + PtrPoolInc) * sizeof(T*), 
                    __FILE__, __LINE__);
            }
            else
            {
                newPages = (T**)Allocator::Alloc(
                    this, PtrPoolInc * sizeof(T*), 
                    __FILE__, __LINE__);
            }
            if (!newPages)
                return false;
            Pages = newPages;
            MaxPages += PtrPoolInc;
        }
        Pages[nb] = (T*)Allocator::Alloc(this, PageSize * sizeof(T), __FILE__, __LINE__);
        if (!Pages[nb])
            return false;
        NumPages++;
        return true;
    }

    GINLINE T* acquireDataPtrSafe()
    {
        UPInt np = Size >> PageShift;
        if(np >= NumPages)
        {
            if (!allocatePageSafe(np))
                return NULL;
        }
        return Pages[np] + (Size & PageMask);
    }

    UPInt Size;
    UPInt NumPages;
    UPInt MaxPages;
    T**   Pages;
};




// ***** GArrayPagedPOD
//
// General purpose paged array for objects that DOES NOT require  
// construction/destruction. Constructors and destructors are not called! 
// Global heap is in use.
//------------------------------------------------------------------------
template<class T, int PageSh=6, int PtrPoolInc=64, int SID=GStat_Default_Mem>
class GArrayPagedPOD : 
    public GArrayPagedBase<T, PageSh, PtrPoolInc, GAllocatorPagedGH_POD<T, SID> >
{
public:
    typedef GArrayPagedPOD<T, PageSh, PtrPoolInc, SID>  SelfType;
    typedef T                                           ValueType;
    typedef GAllocatorPagedGH_POD<T, SID>               AllocatorType;
};


// ***** GArrayPaged
//
// General purpose paged array for objects that require 
// explicit construction/destruction. 
// Global heap is in use.
//------------------------------------------------------------------------
template<class T, int PageSh=6, int PtrPoolInc=64, int SID=GStat_Default_Mem>
class GArrayPaged : 
    public GArrayPagedBase<T, PageSh, PtrPoolInc, GAllocatorPagedGH<T, SID> >
{
public:
    typedef GArrayPaged<T, PageSh, PtrPoolInc, SID>     SelfType;
    typedef T                                           ValueType;
    typedef GAllocatorPagedGH<T, SID>                   AllocatorType;
};


// ***** GArrayPagedLH_POD
//
// General purpose paged array for objects that require 
// explicit construction/destruction. 
// Local heap is in use.
//------------------------------------------------------------------------
template<class T, int PageSh=6, int PtrPoolInc=64, int SID=GStat_Default_Mem>
class GArrayPagedLH_POD : 
    public GArrayPagedBase<T, PageSh, PtrPoolInc, GAllocatorPagedLH_POD<T, SID> >
{
public:
    typedef GArrayPagedLH_POD<T, PageSh, PtrPoolInc, SID>   SelfType;
    typedef T                                               ValueType;
    typedef GAllocatorPagedLH_POD<T, SID>                   AllocatorType;
};


// ***** GArrayPagedLH
//
// General purpose paged array for objects that DOES NOT require  
// construction/destruction. Constructors and destructors are not called! 
// Local heap is in use.
//------------------------------------------------------------------------
template<class T, int PageSh=6, int PtrPoolInc=64, int SID=GStat_Default_Mem>
class GArrayPagedLH : 
    public GArrayPagedBase<T, PageSh, PtrPoolInc, GAllocatorPagedLH<T, SID> >
{
public:
    typedef GArrayPagedLH<T, PageSh, PtrPoolInc, SID>   SelfType;
    typedef T                                           ValueType;
    typedef GAllocatorPagedLH<T, SID>                   AllocatorType;
};



#endif
