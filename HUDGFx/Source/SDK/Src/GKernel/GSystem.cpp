/**********************************************************************

Filename    :   GSystem.cpp
Content     :   General kernel initalization/cleanup, including that
                of the memory allocator.
Created     :   Ferbruary 5, 2009
Authors     :   Michael Antonov

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GSystem.h"
#include "GThreads.h"
#include "GFxAmpServer.h"


// *****  GFxSystem Implementation

static GSysAllocBase*   GSystem_pSysAlloc = 0;

// Initializes GFxSystem core, setting the global heap that is needed for GFx
// memory allocations.  
void GSystem::Init(const GMemoryHeap::HeapDesc& rootHeapDesc, GSysAllocBase* psysAlloc)
{
    GFC_DEBUG_WARNING((GSystem_pSysAlloc != 0), "GSystem::Init failed - duplicate call.");

    if (!GSystem_pSysAlloc)
    {
        bool initSuccess = psysAlloc->initHeapEngine(&rootHeapDesc);
        GASSERT(initSuccess);
        GUNUSED(initSuccess);

        GSystem_pSysAlloc = psysAlloc;

#ifdef GFX_AMP_SERVER
        GFxAmpServer::Init();
#endif
    }
}

void GSystem::Destroy()
{
    GFC_DEBUG_WARNING(!GSystem_pSysAlloc, "GSystem::Destroy failed - GSystem not initialized.");
    if (GSystem_pSysAlloc)
    {
#ifdef GFX_AMP_SERVER
        GFxAmpServer::Uninit();
#endif

        // Wait for all threads to finish; this must be done so that memory
        // allocator and all destructors finalize correctly.
#ifndef GFC_NO_THREADSUPPORT
        GThread::FinishAllThreads();
#endif

        // Report leaks *after* AMP has been uninitialized.
        // shutdownHeapEngine() will report leaks so there is no need to do it explicitly.
        // GMemory::DetectMemoryLeaks();

        // Shutdown heap and destroy SysAlloc singleton, if any.
        GSystem_pSysAlloc->shutdownHeapEngine();
        GSystem_pSysAlloc = 0;
        GASSERT(GMemory::GetGlobalHeap() == 0);
    }
}
