/**********************************************************************

Filename    :   GSysAllocPagedMalloc.cpp
Content     :   Malloc System Allocator
Created     :   2009
Authors     :   Maxim Shemanarev

Notes       :   System Allocator that uses regular malloc/free

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GSysAllocPagedMalloc.h"
#include <stdlib.h>

//#include <stdio.h>   // DBG
//#include "GMemory.h" // DBG
//#include <string.h> // DBG

//------------------------------------------------------------------------
GSysAllocPagedMalloc::GSysAllocPagedMalloc(UPInt granularity) :
    Granularity((granularity + MinGranularity-1) / MinGranularity * MinGranularity),
    Footprint(0),
    Base(~UPInt(0))
{
    GASSERT(Granularity >= MinGranularity);
}

//------------------------------------------------------------------------
void GSysAllocPagedMalloc::GetInfo(Info* i) const
{
    i->MinAlign    = 1;
    i->MaxAlign    = 1;
    i->Granularity = Granularity;
    i->SysDirectThreshold = 32768;
    i->MaxHeapGranularity = 8192;
    i->HasRealloc  = false;
}

//------------------------------------------------------------------------
void* GSysAllocPagedMalloc::Alloc(UPInt size, UPInt)
{
    void* ptr = malloc(size);
    if (ptr)
    {
        Footprint += size;

        if (UPInt(ptr) < Base) // DBG
            Base = UPInt(ptr);

        //memset(ptr, 0xC0, size); // DBG
    }

    //if (GMemory::pGlobalHeap)             // DBG
    //    printf("+%u fp:%uK used:%f%%\n", 
    //        size/Granularity, Footprint/1024, 
    //        GMemory::pGlobalHeap->GetTotalUsedSpace()/double(Footprint)*100.0);

    return ptr;
}

//------------------------------------------------------------------------
bool GSysAllocPagedMalloc::Free(void* ptr, UPInt size, UPInt)
{
    free(ptr);
    Footprint -= size;

    //// With this DBG GFx crashes at exit. The crash is expected and normal.
    //if (GMemory::pGlobalHeap)             // DBG
    //    printf("-%u fp:%uK used:%f%%\n",
    //        size/Granularity, Footprint/1024, 
    //        GMemory::pGlobalHeap->GetTotalUsedSpace()/double(Footprint)*100.0);

    return true;
}

UPInt GSysAllocPagedMalloc::GetBase() const // DBG
{
    return Base;
}


