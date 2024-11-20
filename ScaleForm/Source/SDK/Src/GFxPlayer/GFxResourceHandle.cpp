/**********************************************************************

Filename    :   GFxResourceHandle.h
Content     :   Resource handle and resource binding support for GFx
Created     :   February 8, 2007
Authors     :   Michael Antonov

Notes       :   

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#include "GFxResourceHandle.h"

#include "GFxPlayerStats.h"


// ***** GFxResourceBinding

GFxResourceBinding::GFxResourceBinding(GMemoryHeap *pheap)
  : pHeap(pheap)
{    
    pResources    = 0;
    ResourceCount = 0;
    Frozen        = 0;
    pOwnerDefImpl = 0;
}
GFxResourceBinding::~GFxResourceBinding()
{
    Destroy();
}

void GFxResourceBinding::Destroy()
{
    if (pResources)
    {
        G_DestructArray<GFxResourceBindData>(pResources, ResourceCount);
        GHEAP_FREE(pHeap, pResources);
        pResources = 0;
    }
}


// TBD: Technically binding array can grow as Bind catches up
// with Read. Therefore, we might need to deal with some threading
// considerations here.
void    GFxResourceBinding::SetBindData(UInt index, const GFxResourceBindData &bd)
{
    // If this object is frozen, we should not be calling Set.
    GASSERT(Frozen == 0);

    // We can use a lock - free system later
    GLock::Locker lock(&ResourceLock);
    
    UInt size = ((index + 1) + 15) & ~15;

    if (size > ResourceCount)
    {
        if (!pResources)
        {
            // Can't use new[] since GFxResourceBindData doesn't derive
            // from GNewOverrideBase.
            pResources = (GFxResourceBindData*)GHEAP_ALLOC(pHeap, size * sizeof(GFxResourceBindData),
                                                           GFxStatMD_Other_Mem);
            GASSERT(pResources);
            G_ConstructArray<GFxResourceBindData>(pResources, size);
            ResourceCount = size;
        }
        else
        {
            GFxResourceBindData* pnewRes = (GFxResourceBindData*)
                                           GHEAP_ALLOC(pHeap, size * sizeof(GFxResourceBindData),
                                                       GFxStatMD_Other_Mem);
            GASSERT(pnewRes);
            G_ConstructArray<GFxResourceBindData>(pnewRes, size);

            for (UInt i=0; i< ResourceCount; i++)
                pnewRes[i] = pResources[i];

            G_DestructArray<GFxResourceBindData>(pResources, ResourceCount);
            GHEAP_FREE(pHeap, pResources);
            pResources    = pnewRes;
            ResourceCount = size;
        }
    }
    
    pResources[index] = bd;
}


void GFxResourceBinding::GetResourceData_Locked(GFxResourceBindData *pdata, UInt index) const
{
    // Lock so that updates to SetBindData don't break our access.
    GLock::Locker lock(&ResourceLock);
    
    // Do bounds checks because it is possible that the table was not resized
    // to accommodate an index in a handle whose binding failed to load.
    *pdata = (index < ResourceCount) ? pResources[index] : GFxResourceBindData();
}


