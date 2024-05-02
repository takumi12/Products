/**********************************************************************

Filename    :   GFxMeshCacheManager.h
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

#ifndef INC_GFxMeshCacheManager_H
#define INC_GFxMeshCacheManager_H

#include "GRefCount.h"
#include "GHeapNew.h"
#include "GList.h"
#include "GListAlloc.h"
#include "GAtomic.h"
#include "GFxMesh.h"


// ***** GFxCachedMeshSet
// 
// A simple structure to describe the cached mesh. It's stored in two 
// linked lists: In the main LRU queue and in the MeshSetBag. 
// pMeshSet is created externally, in GFxShape.cpp, but destroyed locally, 
// in GFxMeshCacheManager.cpp
//------------------------------------------------------------------------
struct GFxCachedMeshSet : GListNode<GFxCachedMeshSet>
{
    GFxCachedMeshSet*           pPrevInBag;
    GFxCachedMeshSet*           pNextInBag;
    GFxMeshSet*                 pMeshSet;
    struct GFxCachedMeshSetBag* pOwnerBag;
};

// ***** GFxCachedMeshSet_InBag
//
// Class-accessor for the MeshSetBag to handle the linked list.
//------------------------------------------------------------------------
struct GFxCachedMeshSet_InBag
{
inline static void SetPrev(GFxCachedMeshSet* self, GFxCachedMeshSet* what)  { self->pPrevInBag = what; }
inline static void SetNext(GFxCachedMeshSet* self, GFxCachedMeshSet* what)  { self->pNextInBag = what; }
inline static const GFxCachedMeshSet* GetPrev(const GFxCachedMeshSet* self) { return self->pPrevInBag; }
inline static const GFxCachedMeshSet* GetNext(const GFxCachedMeshSet* self) { return self->pNextInBag; }
inline static       GFxCachedMeshSet* GetPrev(GFxCachedMeshSet* self)       { return self->pPrevInBag; }
inline static       GFxCachedMeshSet* GetNext(GFxCachedMeshSet* self)       { return self->pNextInBag; }
};


// ***** GFxCachedMeshSetBag
//
// A collection of GFxCachedMeshSet that belongs to the given shape.
//------------------------------------------------------------------------
class GFxShapeBase;
struct GFxCachedMeshSetBag : GListNode<GFxCachedMeshSetBag>
{
    GFxCachedMeshSetBag() : pOwnerShape(0), NumMeshSets(0), MeshSetBag() {}

    GFxShapeBase*                                       pOwnerShape;
    UInt                                                NumMeshSets;
    GList2<GFxCachedMeshSet, GFxCachedMeshSet_InBag>    MeshSetBag;
};


//------------------------------------------------------------------------
struct GFxMeshCacheLimit : GMemoryHeap::LimitHandler
{
    class GFxMeshCache* CachePtr;

    GFxMeshCacheLimit() : CachePtr(0) {}

    virtual bool OnExceedLimit(GMemoryHeap* heap, UPInt overLimit);
    virtual void OnFreeSegment(GMemoryHeap* heap, UPInt freeingSize);
};


// ***** GFxMeshCache
//
// An LRU mesh cache. It collects shape meshes until the memory limit is
// exceeded. After that least recently used meshed are extruded.
//------------------------------------------------------------------------
class GFxMeshCache : public GRefCountBaseNTS<GFxMeshCache, GStatRG_Mem>
{
    friend struct GFxMeshCacheLimit;

public:
    GFxMeshCache(GMemoryHeap* pheap);
    ~GFxMeshCache();

    UPInt       GetMemLimit() const         { return MemLimit; }
    void        SetMemLimit(UPInt lim);

    UInt        GetNumLockedFrames() const  { return NumLockedFrames; }
    void        SetNumLockedFrames(UInt num);

    void        EnableCacheAllMorph     (bool enable) { CacheMorph      = enable; }
    void        EnableCacheAllScale9Grid(bool enable) { CacheScale9Grid = enable; }
    bool        IsCacheAllMorphEnabled     () const { return CacheMorph; }
    bool        IsCacheAllScale9GridEnabled() const { return CacheScale9Grid; }
    UInt        GetMeshThrashing() const { return MeshThrashing; }

    void        Clear();
    void        AddShapeToKillList(GFxShapeBase* shape);

    GFxMeshSet* GetMeshSet(GFxShapeBase* ownerShape, 
                           const void* instance,
                           GFxMeshSet::KeyCategoryType keyCat, 
                           const void* keyData,
                           UInt keyLen);

    bool        RemoveMeshSet(GFxShapeBase* ownerShape, GFxMeshSet* meshSet);

    void        AddMeshSet(GFxShapeBase* ownerShape, GFxMeshSet* meshSet);

    UInt        GetNumMeshes() const { return NumMeshes; }
    UPInt       GetNumBytes()  const { return pHeap->GetUsedSpace(); }
    UInt        GetNumStrokes() const { return NumStrokes; }

    GMemoryHeap* GetHeap() const { return pHeap; }

    void        ClearKillList();
    void        AcquireFrameQueue();

private:
    void        deleteMeshSet(GFxCachedMeshSet* cachedMeshSet);
    void        extrudeLRU();

    typedef GListAllocLH_POD<
        GFxCachedMeshSet,
        127,
        GStatRG_Other_Mem>          MeshSetAllocatorType;

    typedef GList<GFxCachedMeshSet> MeshSetQueueType;

    typedef GListAllocLH<
        GFxCachedMeshSetBag,
        127,
        GStatRG_Other_Mem>          MeshSetBagAllocatorType;

    typedef GList<GFxCachedMeshSetBag>  KillListType;

    GMemoryHeap*            pHeap;
    UPInt                   MemLimit;
    GFxMeshCacheLimit       LimHandler; 
    //UPInt                   AllocatedBytes;
    UInt                    NumMeshes;
    UInt                    NumStrokes;
    MeshSetAllocatorType    MeshSetAllocator;
    MeshSetQueueType        FrameQueue;
    MeshSetQueueType        CacheQueue;
    MeshSetBagAllocatorType MeshSetBags;
    KillListType            KillList;
    GLock                   KillListLock;
    UInt                    NumLockedFrames;
    UInt                    LockedFrame;
    bool                    CacheMorph;
    bool                    CacheScale9Grid;
    UInt                    MeshThrashing;
    GFxCachedMeshSetBag*    pSafetyBag;
};


#endif

