/**********************************************************************

Filename    :   GFxMeshCacheManager.cpp
Content     :   
Created     :   
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxLoader.h"
#include "GFxMeshCacheManager.h"
#include "GFxRenderGen.h"
#include "GFxShape.h"
#include "GFxAmpServer.h"
#include "GRendererEventHandler.h"


class GFxMeshCacheRenEventHandler : public GRendererEventHandler
{
    GFxMeshCacheManager*    pMeshCache;
public:
    GFxMeshCacheRenEventHandler(GFxMeshCacheManager* pm) : pMeshCache(pm) {}

    // Called when renderer texture data is lost, or when it changes
    virtual void    OnEvent(class GRenderer* , EventType changeType)
    {
        if (changeType == Event_EndFrame)
            pMeshCache->EndDisplay();
    }
};                       


//------------------------------------------------------------------------
GFxMeshCacheManager::GFxMeshCacheManager(bool debugHeap) : 
    GFxState(State_MeshCacheManager), 
    pHeap(0),
    pRenderGen(0),
    pMeshCache(0),
    pRenEventHandler(0)
{
//    // Global Heap
//    pRenderGen = GNEW GFxRenderGen;

//    // Derived Heap
//    pRenderGen = GHEAP_AUTO_NEW(this) GFxRenderGen;

    // Our own Heap. Use 32-byte alignment here to save bookkeeping space
    // in allocator, as most of mesh cache allocations are large.
    UInt heapFlags = debugHeap ? GMemoryHeap::Heap_UserDebug : 0;

    //GMemoryHeap::HeapDesc desc(heapFlags, 32, 128*1024, 128*1024);
    GMemoryHeap::HeapDesc desc(heapFlags, 32, 64*1024, 0, ~UPInt(0));
    desc.HeapId = GHeapId_MeshCache;
    pHeap = GMemory::GetGlobalHeap()->CreateHeap("_Mesh_Cache", desc);

    pRenderGen = GHEAP_NEW(pHeap) GFxRenderGen(pHeap);
    pMeshCache = GHEAP_NEW(pHeap) GFxMeshCache(pHeap);
    //pRenderGenHeap->DestroyOnFree(pRenderGen);
}

//------------------------------------------------------------------------
GFxMeshCacheManager::~GFxMeshCacheManager()
{
    if (pRenEventHandler)
    {
        pRenEventHandler->RemoveFromList();
        delete pRenEventHandler;
    }
    pMeshCache->Release();
    pRenderGen->Release();
    pHeap->Release();
}

//------------------------------------------------------------------------
UPInt GFxMeshCacheManager::GetRenderGenMemLimit() const 
{ 
    return pRenderGen ? pRenderGen->GetMemLimit() : 0; 
}
void GFxMeshCacheManager::SetRenderGenMemLimit(UPInt lim)
{ 
    if (pRenderGen)
        pRenderGen->SetMemLimit(lim);
}

//------------------------------------------------------------------------
UPInt GFxMeshCacheManager::GetMeshCacheMemLimit() const 
{ 
    return pMeshCache ? pMeshCache->GetMemLimit() : 0; 
}
void GFxMeshCacheManager::SetMeshCacheMemLimit(UPInt lim)
{ 
    if (pMeshCache)
        pMeshCache->SetMemLimit(lim);
}


//------------------------------------------------------------------------
UInt  GFxMeshCacheManager::GetNumLockedFrames() const
{
    return pMeshCache ? pMeshCache->GetNumLockedFrames() : 1;
}
void  GFxMeshCacheManager::SetNumLockedFrames(UInt num)
{
    if (pMeshCache)
        pMeshCache->SetNumLockedFrames(num);
}

//------------------------------------------------------------------------
void  GFxMeshCacheManager::EnableCacheAllMorph(bool enable) 
{ 
    if (pMeshCache)
        pMeshCache->EnableCacheAllMorph(enable);
}
bool  GFxMeshCacheManager::IsCacheAllMorphEnabled() const 
{ 
    return pMeshCache ? pMeshCache->IsCacheAllMorphEnabled() : false;
}


//------------------------------------------------------------------------
void  GFxMeshCacheManager::EnableCacheAllScale9Grid(bool enable) 
{ 
    if (pMeshCache)
        pMeshCache->EnableCacheAllScale9Grid(enable);
}
bool  GFxMeshCacheManager::IsCacheAllScale9GridEnabled() const 
{ 
    return pMeshCache ? pMeshCache->IsCacheAllScale9GridEnabled() : false;
}

//------------------------------------------------------------------------
UInt GFxMeshCacheManager::GetNumTessellatedShapes() const
{
    return pRenderGen ? pRenderGen->NumTessellatedShapes : 0;
}

//------------------------------------------------------------------------
UInt GFxMeshCacheManager::GetMeshThrashing() const
{
    return pMeshCache ? pMeshCache->GetMeshThrashing() : 0;
}

void  GFxMeshCacheManager::ClearCache()
{
    pMeshCache->Clear();
    pRenderGen->ClearAndRelease();
}


//------------------------------------------------------------------------
UInt GFxMeshCacheManager::GetNumMeshes() const 
{ 
    return pMeshCache ? pMeshCache->GetNumMeshes() : 0;
}

UPInt GFxMeshCacheManager::GetNumBytes() const 
{ 
    return pMeshCache ? pMeshCache->GetNumBytes() : 0;
}

UInt GFxMeshCacheManager::GetNumStrokes() const 
{ 
    return pMeshCache ? pMeshCache->GetNumStrokes() : 0;
}

//------------------------------------------------------------------------
void GFxMeshCacheManager::ClearKillList()
{
    if (pMeshCache)
        pMeshCache->ClearKillList();
}

//------------------------------------------------------------------------
void GFxMeshCacheManager::AcquireFrameQueue()
{
    if (pMeshCache)
        pMeshCache->AcquireFrameQueue();
}


//------------------------------------------------------------------------
void GFxMeshCacheManager::EndDisplay()
{
    pMeshCache->ClearKillList();
    pMeshCache->AcquireFrameQueue();

    if (GetRenderGenMemLimit())
    {
        // Release RenderGen memory if necessary.
        GFxRenderGen* rg = GetRenderGen();
        if (rg->GetNumBytes() > GetRenderGenMemLimit())
        {
            rg->ClearAndRelease();
        }
    }
}

GRendererEventHandler* GFxMeshCacheManager::GetEventHandler()
{
    if (!pRenEventHandler)
    {
        pRenEventHandler = GNEW GFxMeshCacheRenEventHandler(this);
    }
    return pRenEventHandler;
}

bool GFxMeshCacheManager::HasEventHandler() const 
{ 
    return pRenEventHandler != NULL; 
}


//------------------------------------------------------------------------
GFxMeshCache::GFxMeshCache(GMemoryHeap* heap) :
    pHeap(heap),
// By default, we use 8 Meg MeshCache limit on PCs and 3 Megs on consoles.
#if defined(GFC_OS_WIN32) || defined(GFC_OS_DARWIN) || defined(GFC_OS_LINUX)
        MemLimit(1024*1024*8),
#else
        MemLimit(1024*1024*3),
#endif
    LimHandler(),
    //AllocatedBytes(0), 
    NumMeshes(0),
    NumStrokes(0),
    MeshSetAllocator(),
    FrameQueue(),
    CacheQueue(),
    MeshSetBags(),
    KillList(),
    KillListLock(),
    NumLockedFrames(1), 
    LockedFrame(1),
    CacheMorph(true),
    CacheScale9Grid(false),
    MeshThrashing(0),
    pSafetyBag(0)
{
    SetMemLimit(MemLimit);
}


//------------------------------------------------------------------------
GFxMeshCache::~GFxMeshCache()
{
    pHeap->SetLimit(0);
    pHeap->SetLimitHandler(0);
    Clear();
}


//------------------------------------------------------------------------
void GFxMeshCache::SetMemLimit(UPInt lim)
{ 
    MemLimit = lim;  
    LimHandler.CachePtr = this;
    pHeap->SetLimitHandler(&LimHandler);
    pHeap->SetLimit(MemLimit);
}


// Set the number of frames when the cache is locked. The meshes 
// added during these frames will not be extruded. In most cases it's 1, 
// but can be 2 for certain rendering back-ends, like Unreal.
//------------------------------------------------------------------------
void GFxMeshCache::SetNumLockedFrames(UInt num)
{
    if (num < 1)
        num = 1;
    NumLockedFrames = num;
    LockedFrame     = num;
}

// Internal private function.
// Delete the given mesh from the queue. If the bag gets empty it must 
// be removed too. But in certain, very heavy cases, the mesh that's 
// supposed to be deleted belongs to the same bag. If that happens the bag
// must not be deleted. safetyBag is the one that cannot be deleted.
//------------------------------------------------------------------------
void GFxMeshCache::deleteMeshSet(GFxCachedMeshSet* cachedMeshSet)
{
    GFxCachedMeshSetBag* bag = cachedMeshSet->pOwnerBag;
    GASSERT(bag->NumMeshSets);
    bag->MeshSetBag.Remove(cachedMeshSet);
    bag->NumMeshSets--;
    if (bag->NumMeshSets == 0 && bag != pSafetyBag)
    {
        // Removing the bag when there are no more meshes. This scope
        // must be synchronized with AddShapeToKillList(). 
        //---------------------
        GLock::Locker locker(&KillListLock);
        if (bag->pOwnerShape)
        {
            bag->pOwnerShape->SetMeshSetBag(0, 0);
        }
        else
        {
            KillList.Remove(bag);
        }
        MeshSetBags.Free(bag);
    }
    NumMeshes--;
    UInt32 numStrokes = cachedMeshSet->pMeshSet->GetNumStrokes();
    NumStrokes -= numStrokes;

    // The mesh may belong whether to FrameQueue or to CacheQueue, but
    // it's not known to which one. Since removing from the linked list
    // can be done without knowing anything about the list itself, it's 
    // safe to remove (static function GList::Remove is used).
    //----------------
    MeshSetQueueType::Remove(cachedMeshSet);
    delete cachedMeshSet->pMeshSet;
    MeshSetAllocator.Free(cachedMeshSet);
}


//------------------------------------------------------------------------
void GFxMeshCache::extrudeLRU()
{
    if (!CacheQueue.IsEmpty())
    {
        GFxCachedMeshSet* meshSet = CacheQueue.GetFirst();
        if (!meshSet->pMeshSet->IsLocked())
        {
            deleteMeshSet(meshSet);
            ++MeshThrashing;
        }
    }
}


//------------------------------------------------------------------------
void GFxMeshCache::Clear()
{
    ClearKillList();
    while(!CacheQueue.IsEmpty())
    {
        deleteMeshSet(CacheQueue.GetFirst());
    }
    MeshSetAllocator.ClearAndRelease();
    MeshSetBags.ClearAndRelease();
}


// Find the mesh according to its key value. If the mesh is found it's 
// moved to the end of the queue. Also, the function takes care of
// removing extra meshes in case of morphing and scale9grid. If caching
// of all morphing/scale9grid meshes is disabled only one mesh per 
// instance is allowed.
//------------------------------------------------------------------------
GFxMeshSet* GFxMeshCache::GetMeshSet(GFxShapeBase* ownerShape, 
                                     const void* instance,
                                     GFxMeshSet::KeyCategoryType keyCat, 
                                     const void* keyData,
                                     UInt keyLen)
{
    GFxCachedMeshSetBag* bag = ownerShape->GetMeshSetBag();
    if (bag == 0)
        return 0;

//// DBG
//{
//GFxCachedMeshSet* mesh = bag->MeshSetBag.GetFirst();
//int n = 0;
//while(!bag->MeshSetBag.IsNull(mesh))
//{
//    mesh = bag->MeshSetBag.GetNext(mesh);
//    ++n;
//}
//printf("%d ", n);
//}

    GFxCachedMeshSet* mesh = bag->MeshSetBag.GetFirst();
    while(!bag->MeshSetBag.IsNull(mesh))
    {
        if (mesh->pMeshSet->MeshKeyFits(keyCat, keyData, keyLen))
        {
            CacheQueue.SendToBack(mesh);
//printf("+"); // DBG
            return mesh->pMeshSet;
        }

        if (keyCat != GFxMeshSet::KeyCategory_Regular)
        {
            // The scale9grid uses only one GFxMeshSet per instance to avoid 
            // excessive memory consumption. So, when we found the situation the 
            // mesh does not fit the scale9grid, and the mesh belongs to the same 
            // instance we delete the mesh and re-tessellate it.
            // It means that animated scale9grid-ed shapes will always be 
            // re-tessellated. Different instances create different meshes unless 
            // the scale9GridKeys are the same. When the instance is deleted the 
            // respective mesh still exists in memory until extrusion.
            // 
            // The same behavior is applicable to shape morphing.
            //
            // In case CacheMorph and/or CacheScale9Grid are enabled
            // all versions of the meshes are cached. In some cases 
            // it may make sense.
            //-----------------------
            if ((!CacheMorph      && keyCat == GFxMeshSet::KeyCategory_Morph) ||
                (!CacheScale9Grid && keyCat == GFxMeshSet::KeyCategory_Scale9Grid))
            {
                if(instance == mesh->pMeshSet->GetCharacterInstance())
                {
                    RemoveMeshSet(ownerShape, mesh->pMeshSet);
                    return 0;
                }
            }
        }
        mesh = bag->MeshSetBag.GetNext(mesh);
    }
    return 0;
}

// Remove the given mesh. The function can be called only from Display() 
// function! When the shape is destroyed it must call AddShapeToKillList().
//------------------------------------------------------------------------
bool GFxMeshCache::RemoveMeshSet(GFxShapeBase* ownerShape, GFxMeshSet* meshSet)
{
    GFxCachedMeshSetBag* bag = ownerShape->GetMeshSetBag();
    if (bag == 0)
        return false;

    GFxCachedMeshSet* mesh = bag->MeshSetBag.GetFirst();
    while(!bag->MeshSetBag.IsNull(mesh))
    {
        if (mesh->pMeshSet == meshSet)
        {
            deleteMeshSet(mesh);
            return true;
        }
        mesh = bag->MeshSetBag.GetNext(mesh);
    }
    return false;
}

// When the shape is destroyed it's impossible to remove the respective 
// meshes, because it may happen in a different thread. So, the meshes are
// added to a special kill list which must be cleaned in the end of Display
// function. Call this function after the frame is complete.
//------------------------------------------------------------------------
void GFxMeshCache::ClearKillList()
{
    GLock::Locker locker(&KillListLock);
    while(!KillList.IsEmpty())
    {
        GFxCachedMeshSetBag* bag = KillList.GetFirst();
        while(!bag->MeshSetBag.IsEmpty())
        {
            deleteMeshSet(bag->MeshSetBag.GetFirst());
        }
    }
}

// Add the mesh to the queue. In case the memory limit is reached, the 
// function may extrude the least recently used meshes. The new mesh 
// is added to the special FrameQueue first, so, it and cannot be extruded
// during the NumLockedFrames. In the end of Display() function
// AcquireFrameQueue() must be called to move the meshes to the main queue.
//------------------------------------------------------------------------
void GFxMeshCache::AddMeshSet(GFxShapeBase* ownerShape, GFxMeshSet* meshSet)
{
//printf("+"); // DBG
    GFxCachedMeshSetBag* bag = ownerShape->GetMeshSetBag();
    if (bag == 0)
    {
        bag = MeshSetBags.Alloc();
        bag->pOwnerShape = ownerShape;
        ownerShape->SetMeshSetBag(bag, this);
    }
    pSafetyBag = bag;

    GFxCachedMeshSet* cachedMeshSet = MeshSetAllocator.Alloc();
    cachedMeshSet->pMeshSet  = meshSet;
    cachedMeshSet->pOwnerBag = bag;
    FrameQueue.PushBack(cachedMeshSet);
    //AllocatedBytes += numBytes;
    bag->MeshSetBag.PushBack(cachedMeshSet);
    bag->NumMeshSets++;
    NumMeshes++;
    UInt32 numStrokes = cachedMeshSet->pMeshSet->GetNumStrokes();
    NumStrokes += numStrokes;

    pSafetyBag = 0;
}

// Add the shape to the kill list. To be exact, not the shape itself, but 
// the mesh bag associated with this shape. Then it's cleaned up in the end
// of Display() by calling ClearKillList().
//------------------------------------------------------------------------
void GFxMeshCache::AddShapeToKillList(GFxShapeBase* shape)
{
    GLock::Locker locker(&KillListLock);
    GFxCachedMeshSetBag* bag = shape->GetMeshSetBag();
    if (bag)
    {
        shape->SetMeshSetBag(0, 0);
        bag->pOwnerShape = 0;
        KillList.PushBack(bag);
    }
}

// Move the Frame Queue to the main LRU queue. This function must be called
// from the end of Display().
//------------------------------------------------------------------------
void GFxMeshCache::AcquireFrameQueue()
{
    if (--LockedFrame == 0)
    {
        while(!FrameQueue.IsEmpty())
        {
            GFxCachedMeshSet* meshSet = FrameQueue.GetFirst();
            FrameQueue.Remove(meshSet);
            CacheQueue.PushBack(meshSet);
        }
        LockedFrame = NumLockedFrames;
    }

    // Update the statistics
    NumMeshes = 0;
    NumStrokes = 0;
    GFxCachedMeshSet* meshSet = CacheQueue.GetFirst();
    while(!CacheQueue.IsNull(meshSet))
    {
        NumMeshes++;

        UInt32 numStrokes = meshSet->pMeshSet->GetNumStrokes();
        NumStrokes += numStrokes;

        meshSet = CacheQueue.GetNext(meshSet);
    }
}

//------------------------------------------------------------------------
bool GFxMeshCacheLimit::OnExceedLimit(GMemoryHeap* heap, UPInt overLimit)
{
    UPInt oldUsed = heap->GetUsedSpace();
    UPInt newUsed = oldUsed;
    UPInt prevUsed = oldUsed;
    do
    {
        prevUsed = heap->GetUsedSpace();
        CachePtr->extrudeLRU();
        newUsed = heap->GetUsedSpace();
    }
    while(newUsed < prevUsed && newUsed + overLimit > oldUsed);

    // Increase limit if could not release enough space.
    if (newUsed + overLimit > oldUsed)
    {
        heap->SetLimit(heap->GetLimit() + overLimit);
    }
    return true;
}

//------------------------------------------------------------------------
void GFxMeshCacheLimit::OnFreeSegment(GMemoryHeap* heap, UPInt freeingSize)
{
    UPInt oldLim = heap->GetLimit();
    if (oldLim > freeingSize)
    {
        if (oldLim - freeingSize >= CachePtr->MemLimit)
        {
            heap->SetLimit(oldLim - freeingSize);
        }
    }
}
