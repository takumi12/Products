/**********************************************************************

Filename    :   GArrayUnsafe.h
Content     :   Template implementation for GArrayUnsafe
Created     :   2008
Authors     :   MaximShemanarev
Copyright   :   (c) 2001-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GArrayUnsafe_H
#define INC_GArrayUnsafe_H

#include "GAllocator.h"

// ***** GArrayUnsafeBase
//
// A simple and unsafe modification of the array. It DOES NOT automatically
// resizes the array on PushBack, Append and so on! 
//
// Functions Reserve() and Resize() always result in losing all data!
// It's used in performance critical algorithms, such as tessellation,
// where the maximal number of elements is known in advance. There's no 
// check for memory overrun, but the address persistence is guaranteed.
// Use with CAUTION! 
//
// The code of this class template was taken from the Anti-Grain Geometry
// Project and modified for the use by Scaleform. 
// Permission to use without restrictions is hereby granted to 
// Scaleform Corp. by the author of Anti-Grain Geometry Project.
//------------------------------------------------------------------------
template<class T, class Allocator> class GArrayUnsafeBase
{
public:
    typedef GArrayUnsafeBase<T, Allocator>  SelfType;
    typedef T                               ValueType;
    typedef Allocator                       AllocatorType;

    ~GArrayUnsafeBase() 
    { 
        Allocator::DestructArray(Data, Size);
        Allocator::Free(Data);
    }

    GArrayUnsafeBase() : Data(0), Size(0), Capacity(0) {}

    GArrayUnsafeBase(UPInt capacity) :
        Size(0), 
        Capacity(capacity), 
        Data((T*)Allocator::Alloc(this, Capacity * sizeof(T), __FILE__, __LINE__))
    {}

    GArrayUnsafeBase(const SelfType& v) : 
        Size(v.Size), 
        Capacity(v.Capacity),
        Data(Capacity ? (T*)Allocator::Alloc(this, Capacity * sizeof(T), __FILE__, __LINE__) : 0)
    {
        if(Size)
            Allocator::CopyArrayForward(Data, v.Data, Size);
    }

    void Clear()
    { 
        Allocator::DestructArray(Data, Size);
        Size = 0; 
    }

    void ClearAndRelease()
    {
        Allocator::DestructArray(Data, Size);
        Allocator::Free(Data);
        Size = Capacity = 0; 
        Data = 0;
    }

    void CutAt(UPInt newSize) 
    { 
        if(newSize < Size) 
        {
            Allocator::DestructArray(Data + newSize, Size - newSize);
            Size = newSize; 
        }
    }

    // Set new capacity. All data is lost, size is set to zero.
    void Reserve(UPInt cap, UPInt extraTail=0)
    {
        Allocator::DestructArray(Data, Size);
        if(cap > Capacity)
        {
            Allocator::Free(Data);
            Capacity = cap + extraTail;
            Data = Capacity ? (T*)Allocator::Alloc(this, Capacity * sizeof(T), __FILE__, __LINE__) : 0;
        }
        Size = 0;
    }

    UPInt GetCapacity() const 
    { 
        return Capacity; 
    }

    UPInt GetNumBytes() const
    { 
        return GetCapacity() * sizeof(ValueType); 
    }

    // Allocate n elements. All data is lost, 
    // but elements can be accessed in range 0...size-1. 
    void Resize(UPInt size, UPInt extraTail=0)
    {
        Reserve(size, extraTail);
        Size = size;
        Allocator::ConstructArray(Data, size);
    }

    void Zero()
    {
        memset(Data, 0, sizeof(T) * Size);
    }

    void PushBack(const T& v)
    { 
        Allocator::Construct(&Data[Size++], v);
    }

    template<class S>
    void PushBackAlt(const S& val)
    {
        Allocator::ConstructAlt(&Data[Size++], val);
    }

    void InsertAt(UPInt pos, const T& val)
    {
        if(pos >= Size) 
        {
            PushBack(val);
        }
        else
        {
            Allocator::Construct(Data + Size);
            Allocator::CopyArrayBackward(Data + pos + 1, Data + pos, Size - pos);
            Allocator::Construct(Data + pos, val);
            ++Size;
        }
    }

    // Removing an element from the array is an expensive operation!
    // It compacts only after removing the last element.
    void RemoveAt(UPInt pos)
    {
        if (Data.Size == 1)
        {
            Clear();
        }
        else
        {
            Allocator::Destruct(Data + pos);
            Allocator::CopyArrayForward(
                Data + pos, 
                Data + pos + 1,
                Size - 1 - pos);
            --Size;
        }
    }

    UPInt GetSize() const 
    { 
        return Size; 
    }

    const SelfType& operator = (const SelfType& v)
    {
        if(&v != this)
        {
            Resize(v.Size);
            if(Size) Allocator::CopyArrayForward(Data, v.Data, Size);
        }
        return *this;
    }

    const T& operator [] (UPInt i) const { return Data[i]; }
          T& operator [] (UPInt i)       { return Data[i]; }
    const T& At(UPInt i) const           { return Data[i]; }
          T& At(UPInt i)                 { return Data[i]; }
    T   ValueAt(UPInt i) const           { return Data[i]; }

    const T* GetDataPtr() const { return Data; }
          T* GetDataPtr()       { return Data; }

private:
    T*    Data;
    UPInt Size;
    UPInt Capacity;
};



// ***** GArrayUnsafePOD
//
// General purpose "unsafe" array for movable objects that DOES NOT require 
// construction/destruction. Constructors and destructors are not called! 
// Global heap is in use.
//------------------------------------------------------------------------
template<class T, int SID=GStat_Default_Mem>
class GArrayUnsafePOD : 
    public GArrayUnsafeBase<T, GAllocatorGH_POD<T, SID> >
{
public:
    typedef T                                   ValueType;
    typedef GAllocatorGH_POD<T, SID>            AllocatorType;
    typedef GArrayUnsafeBase<T, AllocatorType>  BaseType;
    typedef GArrayUnsafePOD<T, SID>             SelfType;
    GArrayUnsafePOD()                   : BaseType() {}
    GArrayUnsafePOD(UPInt capacity)     : BaseType(capacity) {}
    GArrayUnsafePOD(const SelfType& v)  : BaseType(v) {}
    const SelfType& operator=(const SelfType& a) { BaseType::operator=(a); return *this; }
};


// ***** GArrayUnsafe
//
// General purpose "unsafe" array for movable objects that require explicit 
// construction/destruction. Global heap is in use.
//------------------------------------------------------------------------
template<class T, int SID=GStat_Default_Mem>
class GArrayUnsafe : 
    public GArrayUnsafeBase<T, GAllocatorGH<T, SID> >
{
public:
    typedef T                                   ValueType;
    typedef GAllocatorGH<T, SID>                AllocatorType;
    typedef GArrayUnsafeBase<T, AllocatorType>  BaseType;
    typedef GArrayUnsafe<T, SID>                SelfType;
    GArrayUnsafe()                   : BaseType() {}
    GArrayUnsafe(UPInt capacity)     : BaseType(capacity) {}
    GArrayUnsafe(const SelfType& v)  : BaseType(v) {}
    const SelfType& operator=(const SelfType& a) { BaseType::operator=(a); return *this; }
};


// ***** GArrayUnsafeBase
//
// General purpose "unsafe" array for movable objects that DOES NOT require  
// construction/destruction. Constructors and destructors are not called! 
// Local heap is in use.
//------------------------------------------------------------------------
template<class T, int SID=GStat_Default_Mem>
class GArrayUnsafeLH_POD : 
    public GArrayUnsafeBase<T, GAllocatorLH_POD<T, SID> >
{
public:
    typedef T                                   ValueType;
    typedef GAllocatorLH_POD<T, SID>            AllocatorType;
    typedef GArrayUnsafeBase<T, AllocatorType>  BaseType;
    typedef GArrayUnsafeLH_POD<T, SID>          SelfType;
    GArrayUnsafeLH_POD()                   : BaseType() {}
    GArrayUnsafeLH_POD(UPInt capacity)     : BaseType(capacity) {}
    GArrayUnsafeLH_POD(const SelfType& v)  : BaseType(v) {}
    const SelfType& operator=(const SelfType& a) { BaseType::operator=(a); return *this; }
};


// ***** GArrayUnsafeLH
//
// General purpose "unsafe" array for movable objects that require explicit 
// construction/destruction. Local heap is in use.
//------------------------------------------------------------------------
template<class T, int SID=GStat_Default_Mem>
class GArrayUnsafeLH : 
    public GArrayUnsafeBase<T, GAllocatorLH<T, SID> >
{
public:
    typedef T                                   ValueType;
    typedef GAllocatorLH<T, SID>                AllocatorType;
    typedef GArrayUnsafeBase<T, AllocatorType>  BaseType;
    typedef GArrayUnsafeLH<T, SID>              SelfType;
    GArrayUnsafeLH()                   : BaseType() {}
    GArrayUnsafeLH(UPInt capacity)     : BaseType(capacity) {}
    GArrayUnsafeLH(const SelfType& v)  : BaseType(v) {}
    const SelfType& operator=(const SelfType& a) { BaseType::operator=(a); return *this; }
};


#endif
