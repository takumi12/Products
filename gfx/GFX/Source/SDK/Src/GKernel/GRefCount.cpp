/**********************************************************************

Filename    :   GRefCount.cpp
Content     :   Reference counting implementation
Created     :   January 14, 1999
Authors     :   Michael Antonov
Notes       :

History     :   

Copyright   :   (c) 1999-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GRefCount.h"
#include "GArray.h"
#include "GHeapNew.h"
#include "GAtomic.h"


// ***** Reference Count Base implementation

GRefCountImplCore::~GRefCountImplCore()
{
    // RefCount can be either 1 or 0 here.
    //  0 if Release() was properly called.
    //  1 if the object was declared on stack or as an aggregate.
    GASSERT(RefCount <= 1);
}

#ifdef GFC_BUILD_DEBUG
void GRefCountImplCore::reportInvalidDelete(void *pmem)
{
    GFC_DEBUG_ERROR1(1,
        "Invalid delete call on ref-counted object at %p. Please use Release()", pmem);
    GASSERT(0);
}
#endif


// *** Thread-Safe GRefCountImpl

void    GRefCountImpl::AddRef()
{
    GAtomicOps<SInt>::ExchangeAdd_NoSync(&RefCount, 1);
}
void    GRefCountImpl::Release()
{
    if ((GAtomicOps<SInt>::ExchangeAdd_NoSync(&RefCount, -1) - 1) == 0)
        delete this;
}

// *** NON-Thread-Safe GRefCountImpl

void    GRefCountNTSImpl::Release()
{
    RefCount--;
    if (RefCount == 0)
        delete this;
}

// *** WeakProxy support GRefCountImpl

GRefCountWeakSupportImpl::~GRefCountWeakSupportImpl()
{
    if (pWeakProxy)
    {
        pWeakProxy->NotifyObjectDied();
        pWeakProxy->Release();
    }
}

// Create/return create proxy, users must release proxy when no longer needed
GWeakPtrProxy*  GRefCountWeakSupportImpl::CreateWeakProxy() const
{
    if (!pWeakProxy)
        if ((pWeakProxy = GHEAP_AUTO_NEW(this)
                GWeakPtrProxy(const_cast<GRefCountWeakSupportImpl*>(this)))==0)
            return 0;
    pWeakProxy->AddRef();
    return pWeakProxy;
}
