/**********************************************************************

Filename    :   GSysAllocWinAPI.cpp
Content     :   Win32 System Allocator
Created     :   2009
Authors     :   Maxim Shemanarev

Notes       :   Win32 System Allocator that uses VirtualAlloc
                and VirualFree.

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GHeapTypes.h"
#include "GSysAllocWinAPI.h"
#include "GSysAllocMemoryMap.h"
#ifndef GFC_OS_XBOX360
#include <windows.h>
#endif

//#include <stdio.h>   // DBG
//#include "GMemory.h" // DBG


//------------------------------------------------------------------------
UPInt GSysMemoryMapWinAPI::GetPageSize() const
{
#ifdef GFC_OS_XBOX360
    return 4096;
#else
    SYSTEM_INFO info;
    ::GetSystemInfo(&info);
    return info.dwPageSize;
#endif
}

//------------------------------------------------------------------------
void* GSysMemoryMapWinAPI::ReserveAddrSpace(UPInt size)
{
    return ::VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
}

//------------------------------------------------------------------------
bool GSysMemoryMapWinAPI::ReleaseAddrSpace(void* ptr, UPInt)
{
    return ::VirtualFree(ptr, 0, MEM_RELEASE) != 0;
}

//------------------------------------------------------------------------
void* GSysMemoryMapWinAPI::MapPages(void* ptr, UPInt size)
{
    return ::VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
}

//------------------------------------------------------------------------
bool GSysMemoryMapWinAPI::UnmapPages(void* ptr, UPInt size)
{
    return ::VirtualFree(ptr, size, MEM_DECOMMIT) != 0;
}





//------------------------------------------------------------------------
GSysAllocWinAPI::GSysAllocWinAPI(UPInt granularity, UPInt segSize) :
    Mapper(),
    pAllocator(::new(PrivateData) GSysAllocMemoryMap(&Mapper, segSize, granularity))
{
    GCOMPILER_ASSERT(sizeof(PrivateData) >= sizeof(GSysAllocMemoryMap));
}


//------------------------------------------------------------------------
void GSysAllocWinAPI::GetInfo(Info* i) const
{
    pAllocator->GetInfo(i);
}

//------------------------------------------------------------------------
void* GSysAllocWinAPI::Alloc(UPInt size, UPInt align)
{
    return pAllocator->Alloc(size, align);
}

//------------------------------------------------------------------------
bool GSysAllocWinAPI::ReallocInPlace(void* oldPtr, UPInt oldSize, UPInt newSize, UPInt align)
{
    return pAllocator->ReallocInPlace(oldPtr, oldSize, newSize, align);
}

//------------------------------------------------------------------------
bool GSysAllocWinAPI::Free(void* ptr, UPInt size, UPInt align)
{
    return pAllocator->Free(ptr, size, align);
}

//------------------------------------------------------------------------
UPInt GSysAllocWinAPI::GetFootprint() const 
{ 
    return pAllocator->GetFootprint(); 
}

//------------------------------------------------------------------------
UPInt GSysAllocWinAPI::GetUsedSpace() const 
{ 
    return pAllocator->GetUsedSpace(); 
}

//------------------------------------------------------------------------
const UInt32* GSysAllocWinAPI::GetBitSet(UPInt seg) const // DBG
{
    return pAllocator->GetBitSet(seg);
}

UPInt GSysAllocWinAPI::GetBase() const
{
    return pAllocator->GetBase();
}
