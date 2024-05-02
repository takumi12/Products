/**********************************************************************

Filename    :   GFxSprite.cpp
Content     :   Implementation of MovieClip character
Created     :   
Authors     :   Michael Antonov, Artem Bolgar

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GAlgorithm.h"
#include "GListAlloc.h"
#include "GList.h"
#include "GFxSprite.h"
#include "GFxAction.h"
#include "GFxButton.h"
#include "GFxLoaderImpl.h"
#include "AS/GASTransformObject.h"
#include "GFxFontResource.h"
#include "GFxLog.h"
#include "GFxMorphCharacter.h"
#include "GFxShape.h"
#include "AS/GASArrayObject.h"
#include "AS/GASMatrixObject.h"
#include "GFxStream.h"
#include "GFxStyles.h"
#include "GFxDlist.h"
//#include "AS/GASTimers.h"
#include "GFxPlayer.h"
#include "GFxAudio.h"
#include "GFxSound.h"
#include "GFxString.h"

#include "GFxLoadProcess.h"
#include "GFxDisplayContext.h"

#include "AS/GASBitmapData.h"
#include "GASSoundObject.h"

#include <GFxIMEManager.h>
#include "GFxVideoBase.h"

#include "AS/GASTextSnapshot.h"
#include "AMP/GFxAmpViewStats.h"

#include "GMsgFormat.h"

#include <string.h> // for memset
#include <float.h>
#include <stdlib.h>
#ifdef GFC_MATH_H
#include GFC_MATH_H
#else
#include <math.h>
#endif

#ifdef GFC_BUILD_DEBUG
//#define GFX_TRACE_TIMELINE_EXECUTION
//#define GFX_DUMP_DISPLAYLIST
//#define GFX_TRACE_DIPLAYLIST_EVERY_FRAME
#endif

// Version numbers for $version and getVersion()
// TBD: Perhaps these should be moved into a different header?
#if defined(GFC_OS_WIN32)
#define GFX_FLASH_VERSION "WIN 8,0,0,0"
#elif defined(GFC_OS_MAC)
#define GFX_FLASH_VERSION "MAC 8,0,0,0"
#elif defined(GFC_OS_LINUX)
#define GFX_FLASH_VERSION "LINUX 8,0,0,0"
#elif defined(GFC_OS_XBOX360)
#define GFX_FLASH_VERSION "XBOX360 8,0,0,0"
#elif defined(GFC_OS_XBOX)
#define GFX_FLASH_VERSION "XBOX 8,0,0,0"
#elif defined(GFC_OS_PSP)
#define GFX_FLASH_VERSION "PSP 8,0,0,0"
#elif defined(GFC_OS_PS2)
#define GFX_FLASH_VERSION "PS2 8,0,0,0"
#elif defined(GFC_OS_PS3)
#define GFX_FLASH_VERSION "PS3 8,0,0,0"
#elif defined(GFC_OS_WII)
#define GFX_FLASH_VERSION "WII 8,0,0,0"
#else
#define GFX_FLASH_VERSION "GFX 8,0,0,0"
#endif


// This class is designed to assemble and execute so called "timeline snapshot". 
// It is used for gotoFrame backward and forward. It actually is being assembled
// from ExecuteTags - PlaceObject and RemoveObject.
class GFxTimelineSnapshot
{
public:
    enum PlaceType
    {
        Place_Add       = GFxPlaceObject::Place_Add,
        Place_Move      = GFxPlaceObject::Place_Move,
        Place_Replace   = GFxPlaceObject::Place_Replace,
        Place_Remove,
        Place_RemoveAndAdd,

        Place_Unknown = 255
    };
    enum FlagsType
    {
        Flags_NoReplaceAllowed = 0x1, // TagType=4, no replaces allowed
        Flags_DeadOnArrival    = 0x2  // Character is released, but has unload handler
    };
    struct SourceTags
    {
        GFxPlaceObjectBase*         pMainTag;
        GFxPlaceObjectBase*         pMatrixTag;
        GFxPlaceObjectBase*         pCxFormTag;
        GFxPlaceObjectBase*         pFiltersTag;
        GFxPlaceObjectBase*         pBlendModeTag;
        GFxPlaceObjectBase*         pDepthTag;
        GFxPlaceObjectBase*         pClipDepthTag;
        GFxPlaceObjectBase*         pRatioTag;
        GFxPlaceObjectBase*         pCharIdTag;
        SourceTags() 
            : pMainTag(0), pMatrixTag(0), pCxFormTag(0), pFiltersTag(0), 
            pBlendModeTag(0), pDepthTag(0), pClipDepthTag(0), 
            pRatioTag(0), pCharIdTag(0) {}
        GINLINE bool HasMainTag() { return (pMainTag != 0); }
        GINLINE void Assign(GFxPlaceObjectBase* ptag)
        {
            pMainTag = pMatrixTag = pCxFormTag = pFiltersTag = 
                pBlendModeTag = pDepthTag = pClipDepthTag = pRatioTag = 
                pCharIdTag = ptag;
        }
        GINLINE void Union(GFxPlaceObjectBase* ptag)
        {
            GFxCharPosInfoFlags flags = ptag->GetFlags();
            if (flags.HasMatrix())      pMatrixTag = ptag;
            if (flags.HasCxform())      pCxFormTag = ptag;
            if (flags.HasFilters())     pFiltersTag = ptag;
            if (flags.HasBlendMode())   pBlendModeTag = ptag;
            if (flags.HasDepth())       pDepthTag = ptag;
            if (flags.HasClipDepth())   pClipDepthTag = ptag;
            if (flags.HasRatio())       pRatioTag = ptag;
            if (flags.HasCharacterId()) pCharIdTag = ptag;
        }
        GINLINE void Unpack(GFxPlaceObjectBase::UnpackedData& data) const
        {
            GASSERT(pMainTag);
            pMainTag->Unpack(data);

            // Override tag data for Depth, CharId, Matrix, CxForm, BlendMode, ClipDepth, Ratio, TextFilter
            GFxPlaceObjectBase::UnpackedData    unpackedData[8];    

            UPInt   dataIdx;
            if (pDepthTag != pMainTag)
            {
                pDepthTag->Unpack(unpackedData[0]);
                data.Pos.Depth = unpackedData[0].Pos.Depth;
                data.Pos.SetDepthFlag();
            }
            if (pCharIdTag != pMainTag)
            {
                if (pDepthTag == pCharIdTag) dataIdx = 0;
                else { pCharIdTag->Unpack(unpackedData[1]); dataIdx = 1; }
                data.Pos.CharacterId = unpackedData[dataIdx].Pos.CharacterId;
                data.Pos.SetCharacterIdFlag();
            }
            if (pMatrixTag != pMainTag)
            {
                if (pDepthTag == pMatrixTag) dataIdx = 0;
                else if (pCharIdTag == pMatrixTag) dataIdx = 1;
                else { pMatrixTag->Unpack(unpackedData[2]); dataIdx = 2; }
                data.Pos.Matrix_1 = unpackedData[dataIdx].Pos.Matrix_1;
                data.Pos.SetMatrixFlag();
            }
            if (pCxFormTag != pMainTag)
            {
                if (pDepthTag == pCxFormTag) dataIdx = 0;
                else if (pCharIdTag == pCxFormTag) dataIdx = 1;
                else if (pMatrixTag == pCxFormTag) dataIdx = 2;
                else { pCxFormTag->Unpack(unpackedData[3]); dataIdx = 3; }
                data.Pos.ColorTransform = unpackedData[dataIdx].Pos.ColorTransform;
                data.Pos.SetCxFormFlag();
            }
            if (pBlendModeTag != pMainTag)
            {
                if (pDepthTag == pBlendModeTag) dataIdx = 0;
                else if (pCharIdTag == pBlendModeTag) dataIdx = 1;
                else if (pMatrixTag == pBlendModeTag) dataIdx = 2;
                else if (pCxFormTag == pBlendModeTag) dataIdx = 3;
                else { pBlendModeTag->Unpack(unpackedData[4]); dataIdx = 4; }
                data.Pos.BlendMode = unpackedData[dataIdx].Pos.BlendMode;
                data.Pos.SetBlendModeFlag();
            }
            if (pClipDepthTag != pMainTag)
            {
                if (pDepthTag == pClipDepthTag) dataIdx = 0;
                else if (pCharIdTag == pClipDepthTag) dataIdx = 1;
                else if (pMatrixTag == pClipDepthTag) dataIdx = 2;
                else if (pCxFormTag == pClipDepthTag) dataIdx = 3;
                else if (pBlendModeTag == pClipDepthTag) dataIdx = 4;
                else { pClipDepthTag->Unpack(unpackedData[5]); dataIdx = 5; }
                data.Pos.ClipDepth = unpackedData[dataIdx].Pos.ClipDepth;
                data.Pos.SetClipDepthFlag();
            }
            if (pRatioTag != pMainTag)
            {
                if (pDepthTag == pRatioTag) dataIdx = 0;
                else if (pCharIdTag == pRatioTag) dataIdx = 1;
                else if (pMatrixTag == pRatioTag) dataIdx = 2;
                else if (pCxFormTag == pRatioTag) dataIdx = 3;
                else if (pBlendModeTag == pRatioTag) dataIdx = 4;
                else if (pClipDepthTag == pRatioTag) dataIdx = 5;
                else { pRatioTag->Unpack(unpackedData[6]); dataIdx = 6; }
                data.Pos.Ratio = unpackedData[dataIdx].Pos.Ratio;
                data.Pos.SetRatioFlag();
            }
            if (pFiltersTag != pMainTag)
            {
                if (pDepthTag == pFiltersTag) dataIdx = 0;
                else if (pCharIdTag == pFiltersTag) dataIdx = 1;
                else if (pMatrixTag == pFiltersTag) dataIdx = 2;
                else if (pCxFormTag == pFiltersTag) dataIdx = 3;
                else if (pBlendModeTag == pFiltersTag) dataIdx = 4;
                else if (pClipDepthTag == pFiltersTag) dataIdx = 5;
                else if (pRatioTag == pFiltersTag) dataIdx = 6;
                else { pFiltersTag->Unpack(unpackedData[7]); dataIdx = 7; }
                data.Pos.Filters = unpackedData[dataIdx].Pos.Filters;
                data.Pos.SetFiltersFlag();
            }
        }
    };
    struct SnapshotElement 
    {
        SnapshotElement*            pPrev;
        SnapshotElement*            pNext;

        UInt                        CreateFrame;
        int                         Depth;

        // Matrices, depth and character index.
        SourceTags                  Tags;

        UInt8                       PlaceType;
        UInt8                       Flags;

        SnapshotElement()
            :CreateFrame(~0u), PlaceType(Place_Unknown), Flags(0) {}
        SnapshotElement(int depth)
            :CreateFrame(~0u), PlaceType(Place_Unknown), Flags(0) { Depth = depth; }
        ~SnapshotElement() {}
    };
    struct SnapshotElementComparator
    {
        int Compare(const SnapshotElement& p1, int depth) const
        {
            return (int)p1.Depth - (int)depth;
        }
    };
    // a heap for elements
    GListAllocDH<SnapshotElement, 50>   SnapshotHeap;
    // a sorted by depth array of pointers to elements, ascending order
    GArrayDH_POD<SnapshotElement*>      SnapshotSortedArray;
    // a doubly-linked list of elements, in order of addition
    GList<SnapshotElement>              SnapshotList;
    // the owner sprite
    GFxSprite*                          pOwnerSprite;

    // finds the last element at the specified depth (upper-bound)
    SnapshotElement* FindDepth(int depth, UPInt* pidx = NULL)
    {
        UPInt i = G_UpperBound(SnapshotSortedArray, depth, DepthLess);
        if (i != 0)
        {
            --i;
            if (SnapshotSortedArray[UPInt(i)]->Depth == depth)
            {
                if (pidx)
                    *pidx = i;
                return SnapshotSortedArray[UPInt(i)];
            }
        }
        return NULL;
    }
private:
    static int DepthLess(int depth, const SnapshotElement* pse)
    {
        return (depth < pse->Depth);
    }
public:
    enum DirectionType
    {
        Direction_Forward,
        Direction_Backward
    } Direction;
    GFxTimelineSnapshot(DirectionType dir, GMemoryHeap* pheap, GFxSprite* powner) 
        : SnapshotHeap(pheap), SnapshotSortedArray(pheap), pOwnerSprite(powner), Direction(dir) {}

    ~GFxTimelineSnapshot()
    {
        G_FreeListElements(SnapshotList, SnapshotHeap);
    }

    // Add an element at the specified depth
    SnapshotElement* Add(int depth)
    {
        SnapshotElement* pe = SnapshotHeap.Alloc();
        if (pe)
        {
            SnapshotList.PushBack(pe);
            pe->Depth = depth;

            UPInt i = G_UpperBound(SnapshotSortedArray, depth, DepthLess);
            GASSERT(i == SnapshotSortedArray.GetSize() || SnapshotSortedArray[i]->Depth != depth);
            SnapshotSortedArray.InsertAt((UPInt)i, pe);
            return pe;
        }
        return NULL;
    }
    // Remove the last (upper-bound) element at the specified depth
    void Remove(int depth)
    {
        UPInt idx;
        SnapshotElement* pe = FindDepth(depth, &idx);
        GASSERT(pe);
        if (pe)
        {
            SnapshotList.Remove(pe);
            SnapshotSortedArray.RemoveAt(idx);
            SnapshotHeap.Free(pe);
        }
    }
    // Remove element at the specified index
    void RemoveAtIndex(UPInt idx)
    {
        SnapshotElement* pe = SnapshotSortedArray[idx];
        SnapshotList.Remove(pe);
        SnapshotSortedArray.RemoveAt(idx);
        SnapshotHeap.Free(pe);
    }
};

//
// ***** GFxSpriteDef
//

GFxSpriteDef::GFxSpriteDef(GFxMovieDataDef* pmd)
:
pMovieDef(pmd),
FrameCount(0),
LoadingFrame(0),
pScale9Grid(0),
//pSoundStream(NULL),
Flags(0)
{   
    GASSERT(pMovieDef);
}

GFxSpriteDef::~GFxSpriteDef()
{
    // Destroy frame data; the actual de-allocation of tag memory
    // takes place in the GFxMovieDataDef's tag block allocator.
    UInt i;
    for(i=0; i<Playlist.GetSize(); i++)
        Playlist[i].DestroyTags();        
    delete pScale9Grid;
    //#ifndef GFC_NO_SOUND
    //    if (pSoundStream)
    //        pSoundStream->Release();
    //#endif
}

// Create A (mutable) instance of our definition.  The instance is created to
// live (temporarily) on some level on the parent GFxASCharacter's display list.
GFxCharacter*   GFxSpriteDef::CreateCharacterInstance(GFxASCharacter* parent, GFxResourceId id,
                                                      GFxMovieDefImpl *pbindingImpl)
{
    // Most of the time pbindingImpl will be ResourceMovieDef come from parents scope.
    // In some cases we call CreateCharacterInstance with a different binding context
    // then the parent sprite; this can happen for import-bound characters, for example.
    GFxMovieRoot* proot = parent->GetMovieRoot();    
    GFxSprite*    psi = 
        GHEAP_NEW(proot->GetMovieHeap()) GFxSprite(this, pbindingImpl, proot, parent, id);
    return psi;
}


// Labels the frame currently being loaded with the
// given name.  A copy of the name string is made and
// kept in this object.
void    GFxSpriteDef::AddFrameName(const GString& name, GFxLog *plog)
{
    GASSERT(LoadingFrame >= 0 && LoadingFrame < FrameCount);

    UInt currentlyAssigned = 0;
    if (NamedFrames.GetAlt(name, &currentlyAssigned) == true)
    {
        if (plog)
            plog->LogError("AddFrameName(%d, '%s') -- frame name already assigned to frame %d; overriding\n",
            LoadingFrame, name.ToCStr(), currentlyAssigned);
    }
    // check if we have predefined special labels, such as "_up", "_down", "_over".
    if (name.GetLength() > 0 && name[0] == '_')
    {
        if (name == "_up")
            Flags |= Flags_Has_Frame_up;
        else if (name == "_down")
            Flags |= Flags_Has_Frame_down;
        else if (name == "_over")
            Flags |= Flags_Has_Frame_over;
    }
    NamedFrames.Set(name, LoadingFrame);   // stores 0-based frame #
}


// Read the sprite info.  Consists of a series of tags.
void    GFxSpriteDef::Read(GFxLoadProcess* p, GFxResourceId charId)
{
    GFxStream*  pin     = p->GetStream();
    UInt32      tagEnd = pin->GetTagEndPosition();


    p->EnterSpriteDef(this);

    FrameCount = pin->ReadU16();

    // ALEX: some SWF files have been seen that have 0-frame sprites.
    // The Macromedia player behaves as if they have 1 frame.
    if (FrameCount < 1)    
        FrameCount = 1;    
    Playlist.Resize(FrameCount);    // need a playlist for each frame

    pin->LogParse("  frames = %d\n", FrameCount);

    LoadingFrame = 0;

    while ((UInt32) pin->Tell() < tagEnd)
    {
        GFxTagInfo  tagInfo;
        GFxTagType  tagType = pin->OpenTag(&tagInfo);
        GFxLoaderImpl::LoaderFunction lf = NULL;

#ifdef GFC_DEBUG_COUNT_TAGS
        p->CountTag(tagType);
#endif

        p->ReportProgress(p->GetFileURL(), tagInfo, true);

        if (tagType == GFxTag_EndFrame)
        {
            // NOTE: In very rare cases FrameCount can LIE, containing
            // more frames then was reported in sprite header (wizardy.swf).
            if (LoadingFrame == (int)Playlist.GetSize())
            {
                Playlist.Resize(Playlist.GetSize()+1);
                pin->LogError("An extra frame is found for sprite id = %d, framecnt = %d, actual frames = %d\n",
                    (int)charId.GetIdIndex(), (int)FrameCount, (int)LoadingFrame+1);
            }

            p->CommitFrameTags();

            // show frame tag -- advance to the next frame.
            pin->LogParse("  ShowFrame (sprite, char id = %d)\n", charId.GetIdIndex());
            LoadingFrame++;
        }
        else if (GFxLoaderImpl::GetTagLoader(tagType, &lf))            
        {
            // call the tag loader.  The tag loader should add
            // characters or tags to the GFxASCharacter data structure.
            (*lf)(p, tagInfo);
        }
        else
        {
            // no tag loader for this tag type.
            pin->LogParse("*** no tag loader for type %d\n", tagType);
        }

        pin->CloseTag();
    }

    // In general, we should have EndFrame
    if (p->FrameTagsAvailable())
    {
        // NOTE: In very rare cases FrameCount can LIE, containing
        // more frames then was reported in sprite header (wizardy.swf).
        if (LoadingFrame == (int)Playlist.GetSize())
        {
            Playlist.Resize(Playlist.GetSize()+1);
            pin->LogError("An extra frame is found for sprite id = %d, framecnt = %d, actual frames = %d\n",
                (int)charId.GetIdIndex(), (int)FrameCount, (int)LoadingFrame+1);
        }

        p->CommitFrameTags();
        // ??
        // LoadingFrame++;
    }


    p->LeaveSpriteDef();

    pin->LogParse("  -- sprite END, char id = %d --\n", charId.GetIdIndex());
}

// Initialize an empty clip.
void GFxSpriteDef::InitEmptyClipDef()
{
    // Set FrameCount = 1; that is the default for an empty clip.
    FrameCount = 1;
    Playlist.Resize(FrameCount);
}

bool GFxSpriteDef::DefPointTestLocal(const GPointF &pt, bool testShape, const GFxCharacter* pinst) const  
{ 
    GUNUSED3(pt, testShape, pinst);
    return false;
}

#ifndef GFC_NO_SOUND

GFxSoundStreamDef*  GFxSpriteDef::GetSoundStream() const 
{ 
    return pSoundStream; 
}
void GFxSpriteDef::SetSoundStream(GFxSoundStreamDef* psoundStream) 
{ 
    //    if (pSoundStream)
    //        pSoundStream->Release();
    //    if (psoundStream)
    //        psoundStream->AddRef();
    pSoundStream = psoundStream; 
}

#endif // GFC_NO_SOUND

//
// ***** GFxSprite Implementation
//


GFxSprite::GFxSprite(GFxTimelineDef* pdef, GFxMovieDefImpl* pdefImpl,
                     GFxMovieRoot* pr, GFxASCharacter* pparent,
                     GFxResourceId id, bool loadedSeparately)
                     :
GFxASCharacter(pdefImpl, pparent, id),
pDef(pdef),    
pRoot(pr),
pScale9Grid(0),
PlayStatePriv(GFxMovie::Playing),
CurrentFrame(0),    
Level(-1),
pRootNode(0),
#ifndef GFC_NO_SOUND
pActiveSounds(NULL),
#endif
pHitAreaHandle(0),
pHitAreaHolder(0),
pUserData(0),
Flags(0),
MouseStatePriv(UP)
{
    GASSERT(pDef && pDefImpl);
    GASSERT(pRoot != NULL);

    pMaskCharacter = NULL;
#ifndef GFC_NO_SOUND
    pActiveSounds = NULL;
#endif
    if (pdef->GetResourceType() == GFxResource::RT_SpriteDef)
    {
        GFxSpriteDef* sp = static_cast<GFxSpriteDef*>(pdef);
        SetScale9Grid(sp->GetScale9Grid());

        Flags |= Flags_SpriteDef;
    }

    ASEnvironment.SetTargetOnConstruct(this);

    SetUpdateFrame(true);
    SetHasLoopedPriv(false);

    // By default LockRoot is false.
    SetLockRoot(false);
    SetLoadedSeparately(loadedSeparately);

    // Since loadedSeparately flag does not get set for imports (because it only
    // enables _lockroot behavior not related to imports), we need to check for
    // pMovieDefImpl specifically. Imports receive root node so that they can
    // have their own font manager.
    bool importFlag = pparent && !loadedSeparately && 
        (pparent->GetResourceMovieDef() != pdefImpl);

    if (loadedSeparately || !pparent || importFlag)
    {
        // We share the GFxMovieDefRootNode with other root sprites based on a ref 
        // count. Try to find the root node for the same MovieDef and import flag
        // as ours, if not found create one.

        // An exhaustive search should be ok here since we don't expect too
        // many different MovieDefs and root sprite construction happens rarely;
        // furthermore, a list is cheap to traverse in Advance where updates need
        // to take place. If this becomes a bottleneck we could introduce a
        // hash table in movie root.    
        GFxMovieDefRootNode *pnode = pRoot->RootMovieDefNodes.GetFirst();

        while(!pRoot->RootMovieDefNodes.IsNull(pnode))
        {
            if ((pnode->pDefImpl == pDefImpl) && (pnode->ImportFlag == importFlag))
            {
                // Found node identical to us.
                pnode->SpriteRefCount++;
                pRootNode = pnode;
                break;
            }
            pnode = pnode->pNext;
        }

        // If compatible root node not found, create one.
        if (!pRootNode)
        {
            GMemoryHeap* pheap = pr->GetMovieHeap();

            pRootNode = GHEAP_NEW(pheap) GFxMovieDefRootNode(pDefImpl, importFlag);
            // Bytes loaded must be grabbed first due to data race.        
            pRootNode->BytesLoaded  = pDefImpl->GetBytesLoaded();
            pRootNode->LoadingFrame = importFlag ? 0 : pDefImpl->GetLoadingFrame();

            // Create a local font manager.
            // Fonts created for sprite will match bindings of our DefImpl.
            pRootNode->pFontManager = *GHEAP_NEW(pheap) GFxFontManager(pDefImpl, pr->pFontManagerStates);
            pRoot->RootMovieDefNodes.PushFront(pRootNode);
        }
    }

    // Initialize the flags for init action executed.
    UInt frameCount = pDef->GetFrameCount();
    GASSERT(frameCount != 0);

    InitActionsExecuted.Resize(frameCount);
    memset(&InitActionsExecuted[0], 0, sizeof(InitActionsExecuted[0]) * frameCount);

    // From now on, the ASMovieClipObj is created on-demand by the GetMovieClipObject method
    //ASMovieClipObj = *GHEAP_NEW(pRoot->GetMovieHeap()) GASMovieClipObject(GetGC(), this);
    // let the base class know what's going on
    //pProto = ASMovieClipObj->Get__proto__();

    // Though, we still need pProto to set correctly.
    pProto = GetGC()->GetActualPrototype(&ASEnvironment, GASBuiltin_MovieClip);

    // NOTE: DoInitActions are executed separately in Advance.
}


GFxSprite::~GFxSprite()
{
    // free/reset mask
    if (GetMask())
        SetMask(NULL);

    GFxSprite* pmaskOwner = GetMaskOwner();
    if (pmaskOwner)
        pmaskOwner->SetMask(NULL);

    if (pRootNode)    
    {
        pRootNode->SpriteRefCount--;
        if (pRootNode->SpriteRefCount == 0)
        {
            pRoot->RootMovieDefNodes.Remove(pRootNode);
            delete pRootNode;
        }
    }

#ifndef GFC_NO_SOUND
    if (pActiveSounds)
        delete pActiveSounds;
#endif
    ClearDisplayList();
    delete pScale9Grid;
    //Root->DropRef();

    if (pUserData)
        delete pUserData;
}

void GFxSprite::SetRendererString(GASString str)
{
    if (!pUserData)
        pUserData = GHEAP_NEW(pRoot->GetMovieHeap()) UserRendererData(ASEnvironment.GetSC());
    pUserData->StringVal = str;
    pUserData->UserData.PropFlags |= GRenderer::UD_HasString;
    pUserData->UserData.pString = pUserData->StringVal.ToCStr();
}

void GFxSprite::SetRendererFloat(float num)
{
    if (!pUserData)
        pUserData = GHEAP_NEW(pRoot->GetMovieHeap()) UserRendererData(ASEnvironment.GetSC());
    pUserData->FloatVal = num;
    pUserData->UserData.PropFlags |= GRenderer::UD_HasFloat;
    pUserData->UserData.pFloat = &pUserData->FloatVal;
}

void GFxSprite::SetRendererMatrix(float *m, UInt count)
{
    if (!pUserData)
        pUserData = GHEAP_NEW(pRoot->GetMovieHeap()) UserRendererData(ASEnvironment.GetSC());
    pUserData->UserData.PropFlags |= GRenderer::UD_HasMatrix;
    memcpy(pUserData->MatrixVal, m, count * sizeof(float));
    pUserData->UserData.pMatrix = pUserData->MatrixVal;
    pUserData->UserData.MatrixSize = count;
}

void GFxSprite::SetDirtyFlag() 
{ 
    pRoot->SetDirtyFlag(); 
}

void GFxSprite::SetRootNodeLoadingStat(UInt bytesLoaded, UInt loadingFrame)
{
    if (pRootNode)
    {
        pRootNode->BytesLoaded = bytesLoaded;
        pRootNode->LoadingFrame = pRootNode->ImportFlag ? 0 : loadingFrame;
    }
}
#ifndef GFC_NO_IME_SUPPORT
void GFxSprite::SetIMECandidateListFont(GFxFontResource* pfont)
{
    GASSERT(Level == GFX_CANDIDATELIST_LEVEL);
    if (Level != GFX_CANDIDATELIST_LEVEL)
        return;
    GASSERT(pRootNode);
    if (!pRootNode)
        return;
    if (!pRootNode->pFontManager)
        return;
    GFxResourceBinding* pbinding = pfont->GetBinding();
    GPtr<GFxFontHandle> pfontHandle;
    if (pfont->GetFont()->IsResolved())
    {
        GFxMovieDefImpl* pdefImpl = pbinding ? pbinding->GetOwnerDefImpl() : NULL;
        pfontHandle = *GHEAP_NEW(pRoot->GetMovieHeap()) GFxFontHandle(NULL, pfont, GFX_CANDIDATELIST_FONTNAME, 0, pdefImpl);
    }
    else
    {
        GPtr<GFxFontHandle> ptmp = *pRootNode->pFontManager->CreateFontHandle(pfont->GetName(), pfont->GetFontFlags(), false);
        if (ptmp)
            pfontHandle = *GHEAP_NEW(pRoot->GetMovieHeap()) GFxFontHandle(NULL, ptmp->GetFont(), GFX_CANDIDATELIST_FONTNAME, 0, ptmp->pSourceMovieDef);
    }
    if (pfontHandle)
        pRootNode->pFontManager->SetIMECandidateFont(pfontHandle);
}
#endif //#ifndef GFC_NO_IME_SUPPORT

void GFxSprite::ForceShutdown ()
{
    RemoveFromPlaylist(pRoot);

    if (ASMovieClipObj)
        ASMovieClipObj->ForceShutdown();

}

void    GFxSprite::ClearDisplayList()
{
    DisplayList.Clear();
    SetDirtyFlag();
}


// These accessor methods have custom implementation in GFxSprite.
GFxMovieDefImpl* GFxSprite::GetResourceMovieDef() const
{
    // Return the definition where binding takes place.
    return pDefImpl.GetPtr();
}

GFxFontManager*  GFxSprite::GetFontManager() const
{
    if (pRootNode)
        return pRootNode->pFontManager;
    return GetParent()->GetFontManager();
}

GFxMovieRoot*   GFxSprite::GetMovieRoot() const
{
    return pRoot;
}
GFxASCharacter* GFxSprite::GetLevelMovie(SInt level) const
{   
    return pRoot->GetLevelMovie(level);
}

GFxASCharacter* GFxSprite::GetASRootMovie(bool ignoreLockRoot) const
{
    // Return _root, considering _lockroot. 
    if (!GetParent() ||
        (!ignoreLockRoot && IsLoadedSeparately() && IsLockRoot()) )
    {
        // If no parent, we ARE the root.
        return const_cast<GFxSprite*>(this);
    }

    //// HACK!
    if (IsUnloaded())
    {
        //!AB: This code seems still useful, since Invoke might use this method
        // and to call it on unloaded characters. This is happening in w****d1.swf
        // when char dies, for example. TODO: investigate.
        //GFC_DEBUG_WARNING(1, "GetASRootMovie called on unloaded sprite");
        return pRoot->GetLevelMovie(0);
    }

    return GetParent()->GetASRootMovie(ignoreLockRoot);
}

UInt32  GFxSprite::GetBytesLoaded() const
{  
    const GFxASCharacter *pchar = this;
    // Find a load root sprite and grab bytes loaded from it.
    while(pchar)
    {
        const GFxSprite* psprite = pchar->ToSprite();
        if (psprite && psprite->pRootNode)
            return psprite->pRootNode->BytesLoaded;
        pchar = pchar->GetParent();
    }
    // We should never end up here.
    return 0;
}

void    GFxSprite::SetLevel(SInt level)
{
    Level = level;

    // Assign the default name to level.
    // Note that for levels, Flash allows changing these names later,
    // but such new names can not be used for path lookup.
    char nameBuff[64] = "";
    G_Format(GStringDataPtr(nameBuff, sizeof(nameBuff)), "_level{0}", level);
    SetName(ASEnvironment.CreateString(nameBuff));
}


// Returns frames loaded, sensitive to background loading.
UInt    GFxSprite::GetLoadingFrame() const
{
    return (pRootNode && !pRootNode->ImportFlag) ?
        pRootNode->LoadingFrame : GetFrameCount();
}

// Checks that load was called and does so if necessary.
// Used for root-level sprites.
void    GFxSprite::ExecuteFrame0Events()
{
    if (!IsOnEventLoadCalled())
    {       
        // Must do loading events for root level sprites.
        // For child sprites this is done by the dlist,
        // but root movies don't get added to a dlist, so we do it here.
        // Also, this queues up tags and actions for frame 0.
        SetOnEventLoadCalled();
        // Can't call OnLoadEvent here because implementation of onLoad handler is different
        // for the root: it seems to be called after the first frame as a special case, presumably
        // to allow for load actions to exits. That, however, is NOT true for nested movie clips.
        ExecuteFrameTags(0);
        pRoot->InsertEmptyAction(GFxMovieRoot::AP_Load)->SetAction(this, GFxEventId::Event_Load);
    }
}

void    GFxSprite::OnIntervalTimer(void *timer)
{
    GUNUSED(timer);
}   


// "transform" matrix describes the transform applied to parent and us,
// including the object's matrix itself. This means that if transform is
// identity, GetBoundsTransformed will return local bounds and NOT parent bounds.
GRectF  GFxSprite::GetBounds(const Matrix &transform) const
{
    GFxCharacter*   ch;
    UPInt           i, n = DisplayList.GetCharacterCount();
    GRectF          r(0);   
    GRectF          tempRect;   
    Matrix          m;

    for (i = 0; i < n; i++)
    {
        ch = DisplayList.GetCharacter(i);
        if (ch != NULL)
        {
            // Build transform to target.       
            m = transform;
            m.Prepend(ch->GetMatrix());
            // get bounds transformed by matrix 
            tempRect = ch->GetBounds(m);                        

            if (!tempRect.IsEmpty ())
            {
                if (!r.IsEmpty ())
                    r.Union(tempRect);
                else
                    r = tempRect;
            }
        }
    }

    if (pDrawingAPI)
    {
        pDrawingAPI->ComputeBound(&tempRect);
        if (!tempRect.IsEmpty())
        {
            tempRect = transform.EncloseTransform(tempRect);
            if (!r.IsEmpty())
                r.Union(tempRect);
            else
                r = tempRect;
        }
    }

    return r;
}

#ifndef GFC_NO_3D
// "transform" matrix describes the transform applied to parent and us,
// including the object's matrix itself. This means that if transform is
// identity, GetBoundsTransformed will return local bounds and NOT parent bounds.
GRectF  GFxSprite::GetBounds(const GMatrix3D &transform, bool bDivideByW) const
{
    GFxCharacter*   ch;
    UPInt           i, n = DisplayList.GetCharacterCount();
    GRectF          r(0);   
    GRectF          tempRect;   
    GMatrix3D       m;

    for (i = 0; i < n; i++)
    {
        ch = DisplayList.GetCharacter(i);
        if (ch != NULL)
        {
            // Build transform to target.       
            m = transform;
            m.Prepend(ch->GetLocalMatrix3D());
            // get bounds transformed by matrix 
            tempRect = ch->GetBounds(m, bDivideByW);                        

            if (!tempRect.IsEmpty ())
            {
                if (!r.IsEmpty ())
                    r.Union(tempRect);
                else
                    r = tempRect;
            }
        }
    }

    if (pDrawingAPI)
    {
        pDrawingAPI->ComputeBound(&tempRect);
        if (!tempRect.IsEmpty())
        {
            tempRect = transform.EncloseTransform(tempRect, bDivideByW);
            if (!r.IsEmpty())
                r.Union(tempRect);
            else
                r = tempRect;
        }
    }

    return r;
}
#endif

// "transform" matrix describes the transform applied to parent and us,
// including the object's matrix itself. This means that if transform is
// identity, GetBoundsTransformed will return local bounds and NOT parent bounds.
GRectF  GFxSprite::GetRectBounds(const Matrix &transform) const
{
    GFxCharacter*   ch;
    UPInt           i, n = DisplayList.GetCharacterCount();
    GRectF          r(0);   
    GRectF          tempRect;   
    Matrix          m;

    for (i = 0; i < n; i++)
    {
        ch = DisplayList.GetCharacter(i);
        if (ch != NULL)
        {
            // Build transform to target.       
            m = transform;
            m.Prepend(ch->GetMatrix());
            // get bounds transformed by matrix 
            tempRect = ch->GetRectBounds(m);                        

            if (!tempRect.IsEmpty ())
            {
                if (!r.IsEmpty ())
                    r.Union(tempRect);
                else
                    r = tempRect;
            }
        }
    }
    return r;
}

GRectF GFxSprite::GetFocusRect() const
{
    // Here is the difference from Flash: GFx will use 'hitArea's rectangle
    // as a focusRect, if such hitArea is installed for the sprite.
    // The hitArea's rectangle will be used ONLY if the hitArea is installed
    // to 'this' sprite; in the case when hitArea is installed for one of its
    // child it will not be used. It is possible to find such hitAreas, however,
    // it is unclear what to do if more than one child has a hitArea. That is why,
    // at the moment only immediate hitArea is used.
    // The standard Flash Player doesn't use hitArea for focusRect at all.
    GFxSprite* phitArea = GetHitArea();
    Matrix     thisToHitAreaTransform;
    // This code searches for the first child's hitArea, but it is commented out
    // because of reason described above.
    //if (!phitArea)
    //{
    //    if (pRoot->SpritesWithHitArea.GetSize() > 0)
    //    {
    //        GFxSprite* pcurHA = NULL;
    //        for (UPInt i = 0, n = pRoot->SpritesWithHitArea.GetSize(); i < n; ++i)
    //        {
    //            pcurHA = pRoot->SpritesWithHitArea[i];

    //            GFxASCharacter* parent = pcurHA;
    //            // check if one of the parent of the current hitArea is 'this'.
    //            do {
    //                parent = parent->GetParent();
    //            } while (parent && parent != this);
    //            if (parent)
    //            {
    //                phitArea = pcurHA;
    //                break;
    //            }
    //        }

    //        GFxASCharacter* parent = phitArea;
    //        while (parent && parent != this)
    //        {
    //            thisToHitAreaTransform.Prepend(parent->GetMatrix());
    //            parent = parent->GetParent();
    //        }
    //    }
    //}
    //else
    //    thisToHitAreaTransform = phitArea->GetMatrix();
    if (phitArea)
    {
        thisToHitAreaTransform = phitArea->GetMatrix();

        // now we need to translate hitArea's focus rect to 'this' sprite's
        // coordinate system.
        GRectF haRect = phitArea->GetFocusRect();
        return thisToHitAreaTransform.EncloseTransform(haRect);
    }
    return GFxASCharacter::GetFocusRect();
}

#ifndef GFC_NO_FXPLAYER_AS_TEXTSNAPSHOT
void    GFxSprite::GetTextSnapshot(GFxStaticTextSnapshotData* pdata) const
{
    GFxCharacter*   ch;
    UPInt   i, n = DisplayList.GetCharacterCount();

    for (i = 0; i < n; i++)
    {
        ch = DisplayList.GetCharacter(i);
        if (ch != NULL)
        {
            // Check if character is a static textfield
            if (ch->GetCharacterDef()->GetResourceType() == GFxResource::RT_TextDef)
            {
                GFxStaticTextCharacter* pstextChar = static_cast<GFxStaticTextCharacter*>(ch);
                pdata->Add(pstextChar);
            }
        }
    }
}
#endif  // GFC_NO_FXPLAYER_AS_TEXTSNAPSHOT

void GFxSprite::UpdateRootFrame()
{
#ifdef GFX_AMP_SERVER
    if (pRoot && pRoot->AdvanceStats)
    {
        pRoot->AdvanceStats->SetCurrentFrame(CurrentFrame);
    }
#endif
}


void    GFxSprite::Restart()
{
    DisplayList.MarkAllEntriesForRemoval(0);
    CurrentFrame = 0;
    UpdateRootFrame();
    RollOverCnt = 0;
    SetUpdateFrame();
    SetHasLoopedPriv(false);
    PlayStatePriv = GFxMovie::Playing;

    ExecuteFrameTags(CurrentFrame);
    DisplayList.UnloadMarkedObjects();
    SetDirtyFlag();
}

void    GFxSprite::CalcDisplayListHitTestMaskArray(GArray<UByte> *phitTest, const GPointF &pt, bool testShape) const
{
    GUNUSED(testShape);

    UPInt i, n = DisplayList.GetCharacterCount();

    // Mask support
    for (i = 0; i < n; i++)
    {
        GFxCharacter* pmaskch = DisplayList.GetCharacter(i);

        if (pmaskch->GetClipDepth() > 0)
        {
            if (phitTest->GetSize()==0)
            {
                phitTest->Resize(n);
                memset(&(*phitTest)[0], 1, n);
            }

            GRenderer::Matrix   m = pmaskch->GetMatrix();
            GPointF             p = m.TransformByInverse(pt);

            (*phitTest)[i] = pmaskch->PointTestLocal(p, HitTest_TestShape);

            UPInt k = i+1;            
            while (k < n)
            {
                GFxCharacter* pch = DisplayList.GetCharacter(k);
                if (pch && (pch->GetDepth() > pmaskch->GetClipDepth()))
                    break;
                (*phitTest)[k] = (*phitTest)[i];
                k++;
            }
            i = k-1;
        }
    }
}

// Return the topmost entity that the given point covers.  NULL if none.
// Coords are in parent's frame.
GFxASCharacter* GFxSprite::GetTopMostMouseEntity(const GPointF &pt, const TopMostParams& params)
{
    // Invisible sprites/buttons don't receive mouse events (i.e. are disabled), unless it is a hitArea.
    // Masks also shouldn't receive mouse events (!AB)
    if (IsHitTestDisableFlagSet() || (!GetVisible() && !GetHitAreaHolder()) || IsUsedAsMask())
        return 0;

    if (params.IgnoreMC == this)
        return 0;

    if (!IsFocusAllowed(params.pRoot, params.ControllerIdx))
        return 0;

    GRenderer::Matrix   m = GetMatrix();
    GPointF             localPt;

#ifndef GFC_NO_3D
    if (Is3D(true))
    {
        const GMatrix3D     *pPersp = GetPerspective3D(true);
        const GMatrix3D     *pView = GetView3D(true);
        if (pPersp)
            GetMovieRoot()->ScreenToWorld.SetPerspective(*pPersp);
        if (pView)
            GetMovieRoot()->ScreenToWorld.SetView(*pView);
        GetMovieRoot()->ScreenToWorld.SetWorld(GetWorldMatrix3D());
        GetMovieRoot()->ScreenToWorld.GetWorldPoint(&localPt);
    }
    else
#endif
    {
        localPt = m.TransformByInverse(pt);   
    }


    SPInt i, n = (SPInt)DisplayList.GetCharacterCount();

    GFxSprite* pmask = GetMask();  
    if (pmask)
    {
        if (pmask->IsUsedAsMask() && !pmask->IsUnloaded())
        {
            GPointF pp;
#ifndef GFC_NO_3D
            if (pmask->Is3D(true))
            {   
                const GMatrix3D     *pPersp = pmask->GetPerspective3D(true);
                const GMatrix3D     *pView = pmask->GetView3D(true);
                if (pPersp)
                    GetMovieRoot()->ScreenToWorld.SetPerspective(*pPersp);
                if (pView)
                    GetMovieRoot()->ScreenToWorld.SetView(*pView);

                GetMovieRoot()->ScreenToWorld.SetWorld(pmask->GetWorldMatrix3D());
                GetMovieRoot()->ScreenToWorld.GetWorldPoint(&pp);
            }
            else
#endif
            {
                GRenderer::Matrix matrix;
                matrix.SetInverse(pmask->GetWorldMatrix());
                matrix.Prepend(GetWorldMatrix());
                pp = matrix.Transform(localPt);
            }
            if (!pmask->PointTestLocal(pp, HitTest_TestShape))
                return NULL;
        }
    }

    GArray<UByte> hitTest;
    CalcDisplayListHitTestMaskArray(&hitTest, localPt, 1);

    // Go backwards, to check higher objects first.
    for (i = n - 1; i >= 0; i--)
    {
        GFxCharacter* ch = DisplayList.GetCharacter(i);

        if (hitTest.GetSize() && (!hitTest[i] || ch->GetClipDepth()>0))
            continue;

        if (ch->IsTopmostLevelFlagSet()) // do not check children w/topmostLevel
            continue;

        // MA: This should consider submit masks in the display list,
        // such masks can obscure/clip out buttons for the purpose of
        // hit-testing as well.

        if (ch != NULL)
        {           
            GFxASCharacter* te = ch->GetTopMostMouseEntity(localPt, params);

            if (te && params.TestAll) // needed for mouse wheel     
                return te;

            // If we found anything and we are in button mode, this refers to us.
            if (ActsAsButton() || (pHitAreaHolder && pHitAreaHolder->ActsAsButton()))
            {   
                // It is either child or us; no matter - button mode takes precedence.
                // Note, if both - the hitArea and the holder of hitArea have button handlers
                // then the holder (parent) takes precedence; otherwise, if only the hitArea has handlers
                // it should be returned.
                if (te)
                {
                    if (pHitAreaHolder && pHitAreaHolder->ActsAsButton()) 
                        return pHitAreaHolder;               
                    else
                    {
                        // Sprites with hit area also shouldn't receive mouse events if this hit area is not the sprite's child
                        // We need to check here if a hit area is our child  
                        GFxSprite* phitArea = GetHitArea();
                        if (phitArea)
                        {
                            GFxASCharacter* parent = phitArea;
                            do {
                                parent = parent->GetParent();
                            } while (parent && parent != this);
                            if (!parent)
                                // hit area is not our child so we should not receive mouse events
                                return 0;
                            // delegate the call to the hit area
                            return phitArea->GetTopMostMouseEntity(localPt, params);
                        }
                        else
                            return this;
                    }
                }
            }
            else if (te && te != this)
            {
                // Found one.
                // @@ TU: GetVisible() needs to be recursive here!

                // character could be _root, in which case it would have no parent.
                if (te->GetParent() && te->GetParent()->GetVisible())
                    //if (te->GetVisible()) // TODO move this check to the base case(s) only
                {
                    // @@
                    // Vitaly suggests "return ch" here, but it breaks
                    // samples/test_button_functions.swf
                    //
                    // However, it fixes samples/clip_as_button.swf
                    //
                    // What gives?
                    //
                    // Answer: a button event must be passed up to parent until
                    // somebody handles it.
                    //
                    return te;
                }
                else
                {
                    return 0;
                }
            }           
        }
    }
    if (pDrawingAPI && ActsAsButton())
    {
        if (pDrawingAPI->DefPointTestLocal(localPt, true, this))
            return this;
    }
    return 0;
}

#ifndef GFC_NO_SOUND
GFxSprite::ActiveSounds::ActiveSounds()
{
    Volume = 100;
    Pan = 0;
}
GFxSprite::ActiveSounds::~ActiveSounds()
{
    size_t i;
    if (pStreamSound)
    {
        pStreamSound->Stop();
        pStreamSound = NULL;
    }
    for(i = 0; i < ASSounds.GetSize(); ++i)
    {
        ASSounds[i]->ReleaseTarget();
    }
}
GFxSprite::ActiveSoundItem::ActiveSoundItem()
{
    pSoundObject = NULL;
    pResource = NULL;
}

GFxSprite::ActiveSoundItem::~ActiveSoundItem()
{
    if (pChannel)
        pChannel->Stop();
    if (pResource)
    {
        pResource->DecPlayingCount();
        if (!pResource->IsPlaying())
        {
            pResource->GetSoundInfo()->ReleaseResource();
        }
        pResource->Release();
    }
}

bool    GFxSprite::IsSoundPlaying(GASSoundObject* psobj)
{
    if (pActiveSounds)
    {
        for(size_t i = 0; i < pActiveSounds->Sounds.GetSize(); ++i)
        {
            if (pActiveSounds->Sounds[i]->pSoundObject == psobj)
                return pActiveSounds->Sounds[i]->pChannel && pActiveSounds->Sounds[i]->pChannel->IsPlaying();
        }
    }
    return false;
}
// add an active sound
void    GFxSprite::AddActiveSound(GSoundChannel* pchan, GASSoundObject* psobj, GFxSoundResource* pres)
{
    GASSERT(pchan);
    if (!pActiveSounds)
        pActiveSounds = GNEW ActiveSounds;
    GPtr<ActiveSoundItem> psi;
    for(size_t i = 0; i < pActiveSounds->Sounds.GetSize(); ++i)
    {
        if (pActiveSounds->Sounds[i]->pChannel.GetPtr() == pchan)
        {
            psi = pActiveSounds->Sounds[i];
            break;
        }
    }
    if (!psi)
    {
        psi = *GNEW ActiveSoundItem;
        psi->pChannel = pchan;
        pActiveSounds->Sounds.PushBack(psi);
        ModifyOptimizedPlayListLocal<GFxSprite>(pRoot);
    }
    psi->pSoundObject = psobj;
    psi->pResource = pres;
    if (psi->pResource)
    {
        psi->pResource->IncPlayingCount();
        psi->pResource->AddRef();
    }
}

void    GFxSprite::SetStreamingSound(GSoundChannel* pchan)
{
    if (!pchan && !pActiveSounds)
        return;
    if (!pActiveSounds)
        pActiveSounds = GNEW ActiveSounds;
    if (pActiveSounds->pStreamSound)
        pActiveSounds->pStreamSound->Stop();
    pActiveSounds->pStreamSound = pchan;
    if (pActiveSounds->pStreamSound)
    {
        pActiveSounds->pStreamSound->SetVolume(GetRealSoundVolume());
        AddActiveSound(pchan, NULL, NULL); // TBD do we need to do this??
    }
}

GSoundChannel* GFxSprite::GetStreamingSound() const
{
    if (!pActiveSounds)
        return NULL;
    return pActiveSounds->pStreamSound;
}
// Detach an AS sound object from this sprite
void    GFxSprite::DetachSoundObject(GASSoundObject* psobj)
{
    if (pActiveSounds && psobj)
    {
        size_t i;
        for(i = 0; i < pActiveSounds->Sounds.GetSize(); ++i)
        {
            GPtr<ActiveSoundItem> psi = pActiveSounds->Sounds[i];
            if (psi->pSoundObject == psobj)
                psi->pSoundObject = NULL;
        }
        for(i = 0; i < pActiveSounds->ASSounds.GetSize(); ++i)
        {
            if (pActiveSounds->ASSounds[i] == psobj)
            {
                pActiveSounds->ASSounds.RemoveAt(i);
                break;
            }
        }
    }
}

Float   GFxSprite::GetActiveSoundPosition(GASSoundObject* psobj)
{
    if (pActiveSounds && psobj)
    {
        size_t i;
        for(i = 0; i < pActiveSounds->Sounds.GetSize(); ++i)
        {
            GPtr<ActiveSoundItem> psi = pActiveSounds->Sounds[i];
            if (psi->pSoundObject == psobj && psi->pChannel)
                return psi->pChannel->GetPosition();
        }
    }
    return 0.0f;
}
// register an AS sound object which is attached to this sprite
void    GFxSprite::AttachSoundObject(GASSoundObject* psobj)
{
    GASSERT(psobj);
    if (!pActiveSounds)
        pActiveSounds = GNEW ActiveSounds;
    pActiveSounds->ASSounds.PushBack(psobj);
}


// Return a sound volume which is saved inside this object.
// It is used just for displaying this value
SInt    GFxSprite::GetSoundVolume()
{
    if (pActiveSounds)
        return pActiveSounds->Volume;
    return 100;
}
// Return a sound volume for SubAudio track which is saved inside this object.
SInt    GFxSprite::GetSubSoundVolume()
{
    if (pActiveSounds)
        return pActiveSounds->SubVolume;
    return 100;
}

// Return a calculated sound volume.
Float    GFxSprite::GetRealSoundVolume()
{
    Float v = GetSoundVolume()/ 100.0f;
    GFxASCharacter* parent = GetParent();
    while (parent)
    {
        if (parent->IsSprite())
            v *= parent->ToSprite()->GetSoundVolume()/100.0f;
        parent = parent->GetParent();
    }
    return v;
}

// Return a calculated sound volume for SubAudio track.
Float    GFxSprite::GetRealSubSoundVolume()
{
    Float v = GetSubSoundVolume()/ 100.0f;
    GFxASCharacter* parent = GetParent();
    while (parent)
    {
        if (parent->IsSprite())
            v *= parent->ToSprite()->GetSubSoundVolume()/100.0f;
        parent = parent->GetParent();
    }
    return v;
}

// Save the new sound volume and propagate it to the child movieclip
void    GFxSprite::SetSoundVolume(SInt volume, SInt subvol)
{
    if (!pActiveSounds)
        pActiveSounds = GNEW ActiveSounds;
    pActiveSounds->Volume = volume;
    pActiveSounds->SubVolume = subvol;
    UpdateActiveSoundVolume();
}

// update the volume of all sound started by this sprite
void  GFxSprite::UpdateActiveSoundVolume()
{
    if (!pActiveSounds) return;
    Float v = GetRealSoundVolume();
    for (size_t i = 0; i < pActiveSounds->Sounds.GetSize(); ++i)
    {
        GPtr<ActiveSoundItem> psi = pActiveSounds->Sounds[i];
        psi->pChannel->SetVolume(v);
    }
    for (UInt i = 0; i< DisplayList.DisplayObjectArray.GetSize(); i++)
    {
        const GFxDisplayList::DisplayEntry& dobj = DisplayList.DisplayObjectArray[i];
        GFxCharacter* ch   = dobj.GetCharacter();
        if (ch->IsCharASCharacter())
        {
            GFxASCharacter* pasc = ch->CharToASCharacter_Unsafe();
            if (pasc->IsSprite())
                (pasc->ToSprite())->UpdateActiveSoundVolume();
        }
    }
}

// Return a sound pan which is saved inside this object.
// It is used just for displaying this value
SInt    GFxSprite::GetSoundPan()
{
    if (pActiveSounds)
        return pActiveSounds->Pan;
    return 0;
}

// Return a calculated sound pan.
Float    GFxSprite::GetRealSoundPan()
{
    Float v = GetSoundPan()/ 100.0f;
    GFxASCharacter* parent = GetParent();
    while (parent)
    {
        if (parent->IsSprite())
        {
            v *= parent->ToSprite()->GetSoundPan()/100.0f;
        }
        parent = parent->GetParent();
    }
    return v;
}

// Save the new sound volume and propagate it to the child movieclip
void    GFxSprite::SetSoundPan(SInt pan)
{
    if (!pActiveSounds)
        pActiveSounds = GNEW ActiveSounds;
    pActiveSounds->Pan = pan;
    UpdateActiveSoundPan();
}

// update the volume of all sound started by this sprite
void  GFxSprite::UpdateActiveSoundPan()
{
    if (!pActiveSounds) return;
    Float v = GetRealSoundPan();
    for (size_t i = 0; i < pActiveSounds->Sounds.GetSize(); ++i)
    {
        GPtr<ActiveSoundItem> psi = pActiveSounds->Sounds[i];
        psi->pChannel->SetPan(v);
    }
    for (UInt i = 0; i< DisplayList.DisplayObjectArray.GetSize(); i++)
    {
        const GFxDisplayList::DisplayEntry& dobj = DisplayList.DisplayObjectArray[i];
        GFxCharacter*       ch   = dobj.GetCharacter();
        if (ch->IsCharASCharacter())
        {
            GFxASCharacter* pasc = ch->CharToASCharacter_Unsafe();
            if (pasc->IsSprite())
                pasc->ToSprite()->UpdateActiveSoundPan();
        }
    }
}

// Stop all active sound in this sprite with the given name
// if the name is empty, stop all sound in this movieclip and all child clips
void    GFxSprite::StopActiveSounds()
{
    // if the sound name is not passed we need to stop all sound in this movie clip
    // and all child clips 
    if (pActiveSounds)
    {
        for (size_t i = 0; i < pActiveSounds->Sounds.GetSize(); ++i)
        {
            GPtr<ActiveSoundItem> psi = pActiveSounds->Sounds[i];
            psi->pChannel->Stop();
        }
        pActiveSounds->Sounds.Clear();
    }
    for (UInt i = 0; i< DisplayList.DisplayObjectArray.GetSize(); i++)
    {
        const GFxDisplayList::DisplayEntry& dobj = DisplayList.DisplayObjectArray[i];
        GFxCharacter*       ch   = dobj.GetCharacter();
        if (ch->IsCharASCharacter())
        {
            GFxASCharacter* pasc = ch->CharToASCharacter_Unsafe();
            if (pasc->IsSprite())
                (pasc->ToSprite())->StopActiveSounds();
        }
    }
}
/*
void    GFxSprite::StopActiveSounds(const GString& name)
{
if (pActiveSounds)
{
for (size_t i = 0; i < pActiveSounds->Sounds.GetSize(); )
{
ActiveSoundItem* psi = pActiveSounds->Sounds[i];
if (psi->pResource)
{
const GFxString* resName = GetResourceMovieDef()->GetNameOfExportedResource(psi->pResource->);
if (resName && *resName == name)
{
psi->pChannel->Stop();
delete psi;
pActiveSounds->Sounds.RemoveAt(i);
}
}
else
{
++i;
}
}
}
}
*/
void    GFxSprite::StopActiveSounds(GFxSoundResource* pres)
{
    if (pActiveSounds)
    {
        for (size_t i = 0; i < pActiveSounds->Sounds.GetSize(); )
        {
            GPtr<ActiveSoundItem> psi = pActiveSounds->Sounds[i];
            if (psi->pResource == pres)
            {
                psi->pChannel->Stop();
                pActiveSounds->Sounds.RemoveAt(i);
            }
            else
            {
                ++i;
            }
        }
    }
    for (UInt i = 0; i< DisplayList.DisplayObjectArray.GetSize(); i++)
    {
        const GFxDisplayList::DisplayEntry& dobj = DisplayList.DisplayObjectArray[i];
        GFxCharacter*       ch   = dobj.GetCharacter();
        if (ch->IsCharASCharacter())
        {
            GFxASCharacter* pasc = ch->CharToASCharacter_Unsafe();
            if (pasc->IsSprite())
                (pasc->ToSprite())->StopActiveSounds(pres);
        }
    }
}

void   GFxSprite::CheckActiveSounds()
{
    if (pActiveSounds)
    {
        // we need to make a copy of active sounds array here because
        // during onSoundComplete call this movie clip can be removed
        GArray<GPtr<ActiveSoundItem> > sounds;
        UPInt i;
        for(i = 0; i < pActiveSounds->Sounds.GetSize(); ++i)
            sounds.PushBack(pActiveSounds->Sounds[i]);

        GArray<GPtr<ActiveSoundItem> > completeSounds;

        for(i = 0; i < sounds.GetSize();)
        {
            GPtr<ActiveSoundItem> psi = sounds[i];
            if (!psi->pChannel->IsPlaying())
            {
                if (psi->pSoundObject)
                    psi->pSoundObject->ExecuteOnSoundComplete();
                completeSounds.PushBack(sounds[i]);
                sounds.RemoveAt(i);
                ModifyOptimizedPlayListLocal<GFxSprite>(pRoot);
            }
            else
                ++i;
        }

        // remove all completed sounds from active sounds array
        for (i = 0; i < completeSounds.GetSize(); ++i)
        {
            SInt idx = FindActiveSound(completeSounds[i]);
            if (idx != -1)
                pActiveSounds->Sounds.RemoveAt(idx);
        }
    }
}

// release an active sound which is attached to this sprite. It is used to pass
// active sounds between movieclips with MovieClip.attachAudio method
GFxSprite::ActiveSoundItem*  GFxSprite::ReleaseActiveSound(GSoundChannel* pchan)
{
    GASSERT(pchan);
    if (!pActiveSounds)
        return NULL;
    for(size_t i = 0; i < pActiveSounds->Sounds.GetSize(); ++i)
    {
        GPtr<ActiveSoundItem> pas = pActiveSounds->Sounds[i];
        if (pas->pChannel.GetPtr() == pchan)
        {
            pActiveSounds->Sounds.RemoveAt(i);
            ModifyOptimizedPlayListLocal<GFxSprite>(pRoot);
            pas->AddRef();
            return pas.GetPtr();
        }
    }
    return NULL;
}
// attach an active sound released by ReleaseActiveSound mehtod.
void    GFxSprite::AttachActiveSound(GFxSprite::ActiveSoundItem* psi)
{
    GASSERT(psi);
    if (!pActiveSounds)
        pActiveSounds = GNEW ActiveSounds;
    pActiveSounds->Sounds.PushBack(GPtr<ActiveSoundItem>(psi));
    ModifyOptimizedPlayListLocal<GFxSprite>(pRoot);
}

SInt   GFxSprite::FindActiveSound(ActiveSoundItem* item)
{
    if (pActiveSounds)
    {
        for (UPInt i = 0; i < pActiveSounds->Sounds.GetSize(); ++i)
        {
            if (pActiveSounds->Sounds[i] == item)
                return (SInt)i;
        }
    }
    return -1;
}

#endif // GFC_NO_SOUND

void    GFxSprite::SetPause(bool pause)
{
#ifndef GFC_NO_SOUND
    if (pActiveSounds)
    {
        for (size_t i = 0; i < pActiveSounds->Sounds.GetSize(); ++i)
        {
            ActiveSoundItem* psi = pActiveSounds->Sounds[i];
            if (psi->pChannel)
            {
                psi->pChannel->Pause(pause);
            }
        }
    }
    for (UInt i = 0; i< DisplayList.DisplayObjectArray.GetSize(); i++)
    {
        const GFxDisplayList::DisplayEntry& dobj = DisplayList.DisplayObjectArray[i];
        GFxCharacter*       ch   = dobj.GetCharacter();
        if (ch->IsCharASCharacter())
        {
            GFxASCharacter* pasc = ch->CharToASCharacter_Unsafe();
            pasc->SetPause(pause);
        }
    }
#else
    GUNUSED(pause);
#endif
}


// Increment CurrentFrame, and take care of looping.
void    GFxSprite::IncrementFrameAndCheckForLoop()
{
    CurrentFrame++;

    UInt loadingFrame = GetLoadingFrame();
    UInt frameCount   = pDef->GetFrameCount();

    if ((loadingFrame < frameCount) && (CurrentFrame >= loadingFrame))
    {
        // We can not advance past the loading frame, since those tags are not yet here.
        CurrentFrame = (loadingFrame > 0) ? loadingFrame - 1 : 0;
    }
    else if (CurrentFrame >= frameCount)
    {
        // Loop.
        CurrentFrame = 0;
        SetHasLoopedPriv(true);
        if (frameCount > 1)
        {
            DisplayList.MarkAllEntriesForRemoval();
            SetDirtyFlag();
        }
        else
        {
            // seems like we can stop advancing 1 frame sprite here
            SetPlayState(GFxMovie::Stopped);
        }
    }
    UpdateRootFrame();
}

#ifdef GFC_USE_OLD_ADVANCE
void    GFxSprite::AdvanceFrame(bool nextFrame, Float framePos)
{
    if (IsAdvanceDisabled() || IsUnloading())
        return;

    // Keep this (particularly GASEnvironment) alive during execution!
    GPtr<GFxSprite> thisPtr(this);
    pRoot->MCAdvanceCnt.AddCount(1);

    GASSERT(pDef && pRoot != NULL);

#ifndef GFC_NO_SOUND
    if (nextFrame && PlayStatePriv != GFxMovie::Stopped)
    {
        CheckActiveSounds();
        GFxSoundStreamDef* psoundDef = pDef->GetSoundStream();
        if (psoundDef)
        {
            if (!psoundDef->ProcessSwfFrame(pRoot, CurrentFrame, this))
                pDef->SetSoundStream(NULL);
        }
    }
#endif // GFC_NO_SOUND

    // So, here is the problem. EnterFrame should be processed in order reverse to order of tags processing.
    // That is why we need to dance with saving/restoring action queue positions....

    // Save insert location for child actions. Child actions have to be
    // queued before frame actions; however, frame tags affect their presence.
    GFxMovieRoot::ActionEntry *pchildInitClipPos    = pRoot->ActionQueue.GetInsertEntry(GFxMovieRoot::AP_InitClip);
    GFxMovieRoot::ActionEntry *pchildActionPos      = pRoot->ActionQueue.GetInsertEntry(GFxMovieRoot::AP_Frame);

    // Check for the end of frame
    if (IsJustLoaded())
    {
        SetJustLoaded(false);
    }
    else
    {
        if (nextFrame)
        {   
            // remove all unloaded

            // Update current and next frames.
            if (PlayStatePriv == GFxMovie::Playing)
            {
                UInt    CurrentFrame0 = CurrentFrame;
                IncrementFrameAndCheckForLoop();

                // Execute the current frame's tags.
                if (CurrentFrame != CurrentFrame0)
                {

#if 0
                    // Shape dump logic for external debugging.
                    FILE* fd = fopen("shapes", "at");
                    fprintf(fd, "Frame %d\n", CurrentFrame);
                    fclose(fd);
#endif 

                    // Init action tags must come before Event_EnterFrame
                    ExecuteInitActionFrameTags(CurrentFrame);

                    // Post OnEnterFrame event, so that it arrives before other
                    // frame actions generated by frame tags.
                    OnEvent(GFxEventId::Event_EnterFrame);

                    ExecuteFrameTags(CurrentFrame);
                }
                else
                {
                    // Post OnEnterFrame event, so that it arrives before other
                    // frame actions generated by frame tags.
                    OnEvent(GFxEventId::Event_EnterFrame);
                }
            }
            else
            {
                OnEvent(GFxEventId::Event_EnterFrame);
            }

            // Clean up display List (remove dead objects). Such objects
            // can only exist if we looped around to frame 0.
            if (CurrentFrame == 0)
                DisplayList.UnloadMarkedObjects();

            DisplayList.CheckConsistency();
        }
    }
    GFxMovieRoot::ActionEntry *psaveInitClipPos = pRoot->ActionQueue.SetInsertEntry(GFxMovieRoot::AP_InitClip, pchildInitClipPos);
    GFxMovieRoot::ActionEntry *psaveActionPos   = pRoot->ActionQueue.SetInsertEntry(GFxMovieRoot::AP_Frame, pchildActionPos);

    // Advance everything in the display list.
    // This call will automatically skip advancing children that were just loaded.
    DisplayList.AdvanceFrame(nextFrame, framePos);

    // Restore to tail location, but only if there were some actions generated by
    // the current frame logic at tail after children.
    // If no such actions existed, the new insert location is correct.
    if (psaveInitClipPos != pchildInitClipPos)
        pRoot->ActionQueue.SetInsertEntry(GFxMovieRoot::AP_InitClip, psaveInitClipPos);
    if (psaveActionPos != pchildActionPos)
        pRoot->ActionQueue.SetInsertEntry(GFxMovieRoot::AP_Frame, psaveActionPos);

#ifdef GFX_TRACE_DIPLAYLIST_EVERY_FRAME
    if (IsLevelsMovie()) 
        DumpDisplayList(0, "");
#endif
}
#else

void    GFxSprite::AdvanceFrame(bool nextFrame, Float framePos)
{
    GUNUSED(framePos);
    if (IsAdvanceDisabled() || IsUnloading())
        return;

    // Keep this (particularly GASEnvironment) alive during execution!
    GPtr<GFxSprite> thisPtr(this);

    GASSERT(pDef && pRoot != NULL);

#ifndef GFC_NO_SOUND
    if (nextFrame)// && PlayStatePriv != GFxMovie::Stopped)
    {
        CheckActiveSounds();
        if (PlayStatePriv != GFxMovie::Stopped)
        {
            GFxSoundStreamDef* psoundDef = pDef->GetSoundStream();
            if (psoundDef)
            {
                if (!psoundDef->ProcessSwfFrame(pRoot, CurrentFrame, this))
                    pDef->SetSoundStream(NULL);
            }
        }
    }
#endif // GFC_NO_SOUND

    // Adjust x,y of this character if it is being dragged.
    if (pRoot->IsMouseSupportEnabled())
        GFxASCharacter::DoMouseDrag();

    if (nextFrame)
    {   
        // remove all unloaded

        // Update current and next frames.
        if (PlayStatePriv == GFxMovie::Playing)
        {
            UInt    CurrentFrame0 = CurrentFrame;
            IncrementFrameAndCheckForLoop();

            // Execute the current frame's tags.
            if (CurrentFrame != CurrentFrame0)
            {

#if 0
                // Shape dump logic for external debugging.
                FILE* fd = fopen("shapes", "at");
                fprintf(fd, "Frame %d\n", CurrentFrame);
                fclose(fd);
#endif 

                // Init action tags must come before Event_EnterFrame
                ExecuteInitActionFrameTags(CurrentFrame);

                // Post OnEnterFrame event, so that it arrives before other
                // frame actions generated by frame tags.
                OnEvent(GFxEventId::Event_EnterFrame);

                ExecuteFrameTags(CurrentFrame);
            }
            else
            {
                // Post OnEnterFrame event, so that it arrives before other
                // frame actions generated by frame tags.
                OnEvent(GFxEventId::Event_EnterFrame);
            }
        }
        else
        {
            OnEvent(GFxEventId::Event_EnterFrame);
        }

        // Clean up display List (remove dead objects). Such objects
        // can only exist if we looped around to frame 0.
        if (CurrentFrame == 0)
            DisplayList.UnloadMarkedObjects();

        DisplayList.CheckConsistency();
    }

#ifdef GFX_TRACE_DIPLAYLIST_EVERY_FRAME
    if (IsLevelsMovie()) 
        DumpDisplayList(0, "");
#endif
}
#endif



// Adds the tag info to a timeline snapshot.
void GFxPlaceObjectUnpacked::AddToTimelineSnapshot(GFxTimelineSnapshot* psnapshot, UInt frame)
{
    Trace("\n");
    GFxTimelineSnapshot::SnapshotElement* pse = psnapshot->FindDepth(Pos.Depth);
    if (pse && pse->Flags & GFxTimelineSnapshot::Flags_DeadOnArrival)
        pse = NULL; // force to add a new SE, if the prev is marked as DOA
    if (!pse)
    {
        pse = psnapshot->Add(Pos.Depth);
        pse->Tags.Assign(this);
        pse->PlaceType   = (UInt8)GFxPlaceObject::Place_Add;
        pse->CreateFrame = frame;
    }
    else
    {
        // found something at the depth
        pse->Tags.Assign(this);
        pse->CreateFrame = frame;
    }
    pse->Flags |= GFxTimelineSnapshot::Flags_NoReplaceAllowed;
}

void GFxPlaceObjectUnpacked::Trace(const char* str)
{
#ifdef GFX_TRACE_TIMELINE_EXECUTION
    printf ("%s", str);
    printf ("PlaceObject(U): \tAdd\n");
    printf("\tPos.Depth: \t\t%d\n", Pos.Depth);
    printf("\tPos.CharacterId: \t%d\n", Pos.CharacterId);
#else
    GUNUSED(str);
#endif
}


void GFxPlaceObject::AddToTimelineSnapshot(GFxTimelineSnapshot* psnapshot, UInt frame)
{
    Trace("\n");
    UInt16 depth = GetDepth();
    GFC_DEBUG_ASSERT(depth != 7, "blag");
    GFxTimelineSnapshot::SnapshotElement* pse = psnapshot->FindDepth(depth);
    if (pse && pse->Flags & GFxTimelineSnapshot::Flags_DeadOnArrival)
        pse = NULL; // force to add a new SE, if the prev is marked as DOA
    if (!pse)
    {
        pse = psnapshot->Add(depth);
        pse->PlaceType   = (UInt8)GFxPlaceObject::Place_Add;
        pse->Tags.Assign(this);
        pse->CreateFrame = frame;
    }
    else
    {
        // found something at the depth
        pse->Tags.Assign(this);
        pse->CreateFrame = frame;
    }
    pse->Flags |= GFxTimelineSnapshot::Flags_NoReplaceAllowed;
}

void GFxPlaceObject::Trace(const char* str)
{
#ifdef GFX_TRACE_TIMELINE_EXECUTION
    UnpackedData data;
    Unpack(data);
    printf ("%s", str);
    printf ("PlaceObject: \tAdd\n");
    printf("\tPos.Depth: \t\t%d\n", data.Pos.Depth);
    printf("\tPos.CharacterId: \t%d\n", data.Pos.CharacterId);
#else
    GUNUSED(str);
#endif
}

void GFxPlaceObject2::AddToTimelineSnapshot(GFxTimelineSnapshot* psnapshot, UInt frame)
{
    Trace("\n");
    UInt16 depth = GetDepth();
    UInt8  placeType = (UInt8)GetPlaceType();
    GFxTimelineSnapshot::SnapshotElement* pse = psnapshot->FindDepth(depth);
    if (pse && pse->Flags & GFxTimelineSnapshot::Flags_DeadOnArrival)
        pse = NULL; // force to add a new SE, if the prev is marked as DOA

    if (!pse)
    {
        pse = psnapshot->Add(depth);
        pse->PlaceType   = placeType;
        pse->Tags.Assign(this);
        pse->CreateFrame = frame;
    }
    else
    {
        // found something at the depth
        switch(placeType)
        {
        case GFxPlaceObject::Place_Move:
            pse->Tags.Union(this);
            break;
        case GFxPlaceObject::Place_Replace:
            if (pse->PlaceType != GFxTimelineSnapshot::Place_Add)
                pse->PlaceType = GFxPlaceObject::Place_Replace;
            pse->Tags.Union(this);
            pse->CreateFrame = frame;
            break;
        default:
            pse->Tags.Assign(this);
            pse->CreateFrame = frame;
        }
    }
}

void GFxPlaceObject2::TraceBase(const char* str, UInt8 version)
{
#ifdef GFX_TRACE_TIMELINE_EXECUTION
    GFxPlaceObject2::UnpackedData   data;
    UnpackBase(data, version);
    printf ("%s", str);
    printf ("PlaceObject2: \t%s\n", (data.PlaceType ==Place_Add)?"Add":((data.PlaceType ==Place_Move)?"Move":"Replace"));
    printf ("\tName: \t%s\n", data.Name);
    printf("\tPos.Depth: \t\t%d\n", data.Pos.Depth);
    printf("\tPos.CharacterId: \t%d\n", data.Pos.CharacterId);
    printf("\tPos.ClipDepth: \t\t%d\n", data.Pos.ClipDepth);
    if (data.Pos.HasCxform())
    {
        char buff[2560];
        data.Pos.ColorTransform.Format(buff);
        printf ("\tPos.Cxform:\n");
        printf("%s", buff);

    }
    if (data.Pos.HasMatrix())
    {
        char buff[2560];
        data.Pos.Matrix_1.Format(buff);
        printf ("\tPos.Matrix:\n");
        printf("%s", buff);
    }
    if (data.Pos.HasBlendMode())
        printf("\tPos.BlendMode: \t%d\n", data.Pos.BlendMode);
    if (data.Pos.TextFilter)
        printf("\tPos.HasFilters\n");
#else
    GUNUSED2(str, version);
#endif
}

void GFxPlaceObject3::AddToTimelineSnapshot(GFxTimelineSnapshot* psnapshot, UInt frame)
{
    Trace("\n");
    UInt16 depth = GetDepth();
    UInt8 placeType = (UInt8)GetPlaceType();
    GFxTimelineSnapshot::SnapshotElement* pse = psnapshot->FindDepth(depth);
    if (pse && pse->Flags & GFxTimelineSnapshot::Flags_DeadOnArrival)
        pse = NULL; // force to add a new SE, if the prev is marked as DOA
    if (!pse)
    {
        pse = psnapshot->Add(depth);
        pse->PlaceType   = placeType;
        pse->Tags.Assign(this);
        pse->CreateFrame = frame;
    }
    else
    {
        // found something at the depth
        switch(placeType)
        {
        case GFxPlaceObject::Place_Move:
            pse->Tags.Union(this);
            break;
        case GFxPlaceObject::Place_Replace:
            if (pse->PlaceType != GFxTimelineSnapshot::Place_Add)
                pse->PlaceType = GFxPlaceObject2::Place_Replace;
            pse->Tags.Union(this);
            pse->CreateFrame = frame;
            break;
        default:
            pse->Tags.Assign(this);
            pse->CreateFrame = frame;
        }
    }
}

void GFxPlaceObject3::Trace(const char* str)
{
#ifdef GFX_TRACE_TIMELINE_EXECUTION
    GFxPlaceObject2::UnpackedData   data;
    Unpack(data);
    printf ("%s", str);
    printf ("PlaceObject3: \t%s\n", (data.PlaceType ==GFxPlaceObject2::Place_Add)?"Add":((data.PlaceType ==GFxPlaceObject2::Place_Move)?"Move":"Replace"));
    printf ("\tName: \t%s\n", data.Name);
    printf("\tPos.Depth: \t\t%d\n", data.Pos.Depth);
    printf("\tPos.CharacterId: \t%d\n", data.Pos.CharacterId);
    printf("\tPos.ClipDepth: \t\t%d\n", data.Pos.ClipDepth);
    if (data.Pos.HasCxform())
    {
        char buff[2560];
        data.Pos.ColorTransform.Format(buff);
        printf ("\tPos.Cxform:\n");
        printf("%s", buff);

    }
    if (data.Pos.HasMatrix())
    {
        char buff[2560];
        data.Pos.Matrix_1.Format(buff);
        printf ("\tPos.Matrix:\n");
        printf("%s", buff);
    }
    if (data.Pos.HasBlendMode())
        printf("\tPos.BlendMode: \t%d\n", data.Pos.BlendMode);
    if (data.Pos.TextFilter)
        printf("\tPos.HasFilters\n");
#else
    GUNUSED(str);
#endif
}


static void GFxRemoveObjectBase_AddToTimelineSnapshot(GASExecuteTag* ptag, UInt16 depth, GFxTimelineSnapshot* psnapshot)
{
    ptag->Trace("\n");
    UPInt index;
    GFxTimelineSnapshot::SnapshotElement* pse = psnapshot->FindDepth(depth, &index);
    if (pse && pse->PlaceType != GFxTimelineSnapshot::Place_Add)
    {
        psnapshot->RemoveAtIndex(index);
        pse = NULL;
    }
    if (pse)
    {
        if (pse->Tags.HasMainTag() && psnapshot->Direction == GFxTimelineSnapshot::Direction_Forward)
        {
            // forward direction: check if the event handler array contains onUnload event.
            // If it does, do not remove the element, just set flag Flags_DeadOnArrive.
            // ExecuteSnapshot will create the object, execute onInitialize, onConstruct,
            // onLoad and onUnload events and then kill the object.

            GFxPlaceObjectBase::EventArrayType* pEventHandlers = 
                pse->Tags.pMainTag->UnpackEventHandlers();

            if (pEventHandlers)
            {
                UPInt sz = pEventHandlers->GetSize();
                for (UPInt i = 0; i < sz; ++i)
                {
                    const GFxSwfEvent* pevt = pEventHandlers->At(i);
                    if (pevt->Event.Id & GFxEventId::Event_Unload)
                    {
                        // found!
                        pse->Flags |= GFxTimelineSnapshot::Flags_DeadOnArrival;
                        pse = NULL; // prevent from deletion
                        break;
                    }
                }
            }
        }
        if (pse)
            psnapshot->RemoveAtIndex(index);
    }
    if (!pse)
    {
        if (psnapshot->Direction == GFxTimelineSnapshot::Direction_Forward)
        {
            pse = psnapshot->Add(depth);
            pse->Depth  = depth;
            pse->PlaceType  = GFxTimelineSnapshot::Place_Remove;
            // need to keep this SE, don't delete it from Timeline Snapshot
            pse->Flags |= GFxTimelineSnapshot::Flags_DeadOnArrival; 
        }
        else
        {
            // if we are moving backward, then RemoveObject tag should not exist
            // w/o corresponding Add tag. So, at the moment I consider this situation
            // as an error. (!AB)
            GASSERT(0);
        }
    }
}

// Adds the tag info to a timeline snapshot.
void GFxRemoveObject::AddToTimelineSnapshot(GFxTimelineSnapshot* psnapshot, UInt)
{
    GFxRemoveObjectBase_AddToTimelineSnapshot(this, Depth, psnapshot);
}

void GFxRemoveObject2::AddToTimelineSnapshot(GFxTimelineSnapshot* psnapshot, UInt)
{
    GFxRemoveObjectBase_AddToTimelineSnapshot(this, Depth, psnapshot);
}

void GFxRemoveObject::Trace(const char* str)
{
#ifdef GFX_TRACE_TIMELINE_EXECUTION
    printf ("%s", str);
    printf ("RemoveObject:\n");
    printf("\tDepth: \t%d\n", Depth);
    printf("\tCharacterId: %d\n", Id);
#else
    GUNUSED(str);
#endif
}

void GFxRemoveObject2::Trace(const char* str)
{
#ifdef GFX_TRACE_TIMELINE_EXECUTION
    printf ("%s", str);
    printf ("RemoveObject2:\n");
    printf("\tDepth: \t%d\n", Depth);
#else
    GUNUSED(str);
#endif
}


void    GFxSprite::MakeSnapshot(GFxTimelineSnapshot* psnapshot, UInt startFrame, UInt endFrame)
{
#ifdef GFX_TRACE_TIMELINE_EXECUTION
    printf("+++ id = %d,%s fr = %d to %d, currentframe %d ++++++++\n", 
        GetId(), (GetName().ToCStr() ? GetName().ToCStr() : ""),
        startFrame, endFrame, CurrentFrame);
#endif

    // Keep this (particularly GASEnvironment) alive during execution!
    GPtr<GFxSprite> thisPtr(this);

    for (UInt f = startFrame; f <= endFrame; ++f)
    {
#ifdef GFX_TRACE_TIMELINE_EXECUTION
        printf("\n\t--- frame: %d ---\n", f);
#endif
        const Frame playlist = pDef->GetPlaylist(f);
        for (UInt i = 0; i < playlist.GetTagCount(); i++)
        {
            GASExecuteTag*  e = playlist.GetTag(i);
            e->AddToTimelineSnapshot(psnapshot, f);
        }        
    }
#ifdef GFX_TRACE_TIMELINE_EXECUTION
    printf("-------------\n");
#endif
}

void    GFxSprite::ExecuteSnapshot(GFxTimelineSnapshot* psnapshot, GFxActionPriority::Priority prio)
{
    GUNUSED(prio);
    if (!psnapshot->SnapshotList.IsEmpty())
    {
        const GFxTimelineSnapshot::SnapshotElement* pe;
        for(pe = psnapshot->SnapshotList.GetFirst(); ;
            pe = psnapshot->SnapshotList.GetNext(pe))
        {
            const GFxTimelineSnapshot::SnapshotElement& e = *pe;

            // [PPS] Place_Add/Move/Replace are always PlaceObject types?

            switch(e.PlaceType)
            {
            case GFxTimelineSnapshot::Place_Add:
                {
                    GFxPlaceObjectBase::UnpackedData data;
                    e.Tags.Unpack(data);

                    GASEnvironment *penv = GetASEnvironment();
                    GASString       sname((data.Name == NULL) ? penv->GetBuiltin(GASBuiltin_empty_) : penv->CreateString(data.Name));
                    UInt32 flags = GFxDisplayList::Flags_PlaceObject;
                    if (e.Flags & GFxTimelineSnapshot::Flags_DeadOnArrival)
                        flags |= GFxDisplayList::Flags_DeadOnArrival;
                    AddDisplayObject(
                        data.Pos, sname, data.pEventHandlers, 0, 
                        e.CreateFrame, flags);
                }
                break;
            case GFxTimelineSnapshot::Place_Move:
                {
                    GFxPlaceObjectBase::UnpackedData data;
                    e.Tags.Unpack(data);

                    MoveDisplayObject(data.Pos);
                }
                break;
            case GFxTimelineSnapshot::Place_Replace:
                {
                    GFxPlaceObjectBase::UnpackedData data;
                    e.Tags.Unpack(data);

                    GASEnvironment *penv = GetASEnvironment();
                    GASString       sname((data.Name == NULL) ? penv->GetBuiltin(GASBuiltin_empty_) : penv->CreateString(data.Name));
                    ReplaceDisplayObject(data.Pos, sname);
                }
                break;
            case GFxTimelineSnapshot::Place_Remove:
                {
                    //RemoveDisplayObject(e.Depth, pos.CharacterId);
                    RemoveDisplayObject(e.Depth, GFxResourceId());  // [PPS] GFxRemoveObject/2 does not store char id
                }
                break;
            }

            if (psnapshot->SnapshotList.IsLast(pe))
                break;
        }
    }
}

// Execute the tags associated with the specified frame.
// frame is 0-based
void    GFxSprite::ExecuteFrameTags(UInt frame)
{
    // Keep this (particularly GASEnvironment) alive during execution!
    GPtr<GFxSprite> thisPtr(this);

    GASSERT(frame != UInt(~0));
    //GASSERT(frame < GetLoadingFrame());
    if (frame >= GetLoadingFrame())
        return;

    // Execute this frame's init actions, if necessary.
    ExecuteInitActionFrameTags(frame);

    const Frame playlist = pDef->GetPlaylist(frame);
    for (UInt i = 0; i < playlist.GetTagCount(); i++)
    {
        GASExecuteTag*  e = playlist.GetTag(i);

#ifdef GFX_TRACE_TIMELINE_EXECUTION
        printf("\nexecution, fr = %d, %s\n", frame, GetCharacterHandle()->GetNamePath().ToCStr());
        e->Trace("");
#endif
        e->ExecuteWithPriority(this, GFxMovieRoot::AP_Frame);
    }
}

// Executes init action tags only for the frame.
void    GFxSprite::ExecuteInitActionFrameTags(UInt frame)
{       
    // Execute this frame's init actions, if necessary.
    if (InitActionsExecuted[frame] == false)
    {
        // Keep this (particularly GASEnvironment) alive during execution!
        GPtr<GFxSprite> thisPtr(this);

        bool markFrameAsVisited = false;

        /*
        // execute init clip actions for each first time used import movies.
        const GArray<GFxMovieDataDef*>* pimportMoviesSet = pDef->GetImportSourceMoviesForFrame(frame);
        if (pimportMoviesSet && pimportMoviesSet->GetSize() > 0)
        {
        // iterate through all import source movies for the current frame
        for(UInt f = 0, nf = pimportMoviesSet->GetSize(); f < nf; ++f)
        {
        GFxMovieDefSub* psrcImportMovie = (*pimportMoviesSet)[f];
        GASSERT(psrcImportMovie);
        if (psrcImportMovie->HasInitActions())
        {
        ExecuteImportedInitActions(psrcImportMovie);
        }
        }
        markFrameAsVisited = true;
        }
        */

        GFxTimelineDef::Frame initActionsFrame;        

        if (pDef->GetInitActions(&initActionsFrame, frame) &&
            initActionsFrame.GetTagCount() > 0)
        {
            // Need to execute these actions.
            for (UInt i= 0; i < initActionsFrame.GetTagCount(); i++)
            {
                GASExecuteTag*  e = initActionsFrame.GetTag(i);
#ifdef GFX_TRACE_TIMELINE_EXECUTION
                printf("\ninit action execution, fr = %d, %s\n", frame, GetCharacterHandle()->GetNamePath().ToCStr());
                e->Trace("");
#endif
                e->Execute(this);
            }
            markFrameAsVisited = true;
        }
        if (markFrameAsVisited)
        {
            // Mark this frame done, so we never execute these init actions again.
            InitActionsExecuted[frame] = true;
        }
    }
}

void GFxSprite::ExecuteImportedInitActions(GFxMovieDef* psourceMovie)
{
    GASSERT(psourceMovie);

    GFxMovieDefImpl* pdefImpl = static_cast<GFxMovieDefImpl*>(psourceMovie);
    GFxMovieDataDef* pdataDef = pdefImpl->GetDataDef();

    for(UPInt f = 0, fc = pdataDef->GetInitActionListSize(); f < fc; ++f)
    {
        Frame actionsList;
        if (pdataDef->GetInitActions(&actionsList, (int)f))
        {
            for(UInt i = 0; i < actionsList.GetTagCount(); ++i)
            {
                GASExecuteTag*  e = actionsList.GetTag(i);
                // InitImportActions must get the right binding context since
                // their binding scope does not match that of sprite.
                if (e->IsInitImportActionsTag())
                    ((GFxInitImportActions*)e)->ExecuteInContext(this, pdefImpl);
                else
                    e->ExecuteWithPriority(this, GFxMovieRoot::AP_Highest);
            }
        }        
    }
    SetDirtyFlag();
}


// Add the given action buffer to the list of action
// buffers to be processed at the end of the next frame advance in root.    
void    GFxSprite::AddActionBuffer(GASActionBuffer* a, GFxActionPriority::Priority prio) 
{
    GFxMovieRoot::ActionEntry* pe = pRoot->InsertEmptyAction(prio);
    if (pe) pe->SetAction(this, a);
}

// Set the sprite state at the specified frame number.
// 0-based frame numbers!!  (in contrast to ActionScript and Flash MX)
void    GFxSprite::GotoFrame(UInt targetFrameNumber)
{
    if (IsUnloading())
        return;
    //  IF_VERBOSE_DEBUG(LogMsg("sprite::GotoFrame(%d)\n", targetFrameNumber));//xxxxx

    targetFrameNumber = (UInt)G_Clamp<SInt>(targetFrameNumber, 0, (SInt)GetLoadingFrame() - 1);

#ifndef GFC_NO_SOUND
    // stop streaming sound
    SetStreamingSound(NULL);
#endif

    if (targetFrameNumber < CurrentFrame)
    {
        //GASSERT(!(GetName() == "ss7" && targetFrameNumber == 6 && CurrentFrame == 7));
#ifdef GFX_TRACE_TIMELINE_EXECUTION
        printf("\n #### gotoFrame backward, from %d to %d, this = %s\n", 
            CurrentFrame, targetFrameNumber, GetCharacterHandle()->GetNamePath().ToCStr());
        DumpDisplayList(0, "Before");
#endif
        // Mark all “deletable” characters for removal. The “deletable” characters include 
        // the following characters:
        //    the ones, created by the timeline, even if they were touched by ActionScript 
        //    (AcceptAnimMoves = false), but only if the “CreateFrame” property of these 
        //    characters is greater or equal than TargetFrame (so, they were created ON 
        //    or AFTER the TargetFrame). 
        DisplayList.MarkAllEntriesForRemoval((targetFrameNumber) ? targetFrameNumber-1 : 0);
        //DumpDisplayList(0, "Interm1");

        if (targetFrameNumber >= 1)
        {
            // We need to make a snapshot from Frame 0 to targetFrame - 1. 
            // The TargetFrame is excluded because we would execute all tags for 
            // the target frame by the regular way – calling ExecuteFrameTags. 
            GFxTimelineSnapshot snapshot(GFxTimelineSnapshot::Direction_Backward, pRoot->GetHeap(), this);
            MakeSnapshot(&snapshot, 0, targetFrameNumber-1);

            // Set the current frame to target one and execute the snapshot
            CurrentFrame = targetFrameNumber;
            ExecuteSnapshot(&snapshot);
        }
        else
        {
            // if targetFrame is 0, no snapshot is necessary
            CurrentFrame = targetFrameNumber;
        }
        //DumpDisplayList(0, "Interm2");

        // Execute target frame's tags
        ExecuteFrameTags(targetFrameNumber);
        //DumpDisplayList(0, "Interm3");

        // Unload all remaining mark-for-remove characters. Note, this is not just removing 
        // such characters from the display list. “Unload” means: if the character or one of 
        // its children has onUnload handler, then this character is not removed from the 
        // display list – it is being moved to another depth and the onUnload event is 
        // scheduled. If no onUnload event handler is defined (neither the character nor 
        // its children have it) then this character is unloaded instantly and removed 
        // from the display list. 
        DisplayList.UnloadMarkedObjects();

#ifdef GFX_TRACE_TIMELINE_EXECUTION
        DumpDisplayList(0, "After");
        printf(" ^^^^ end of gotoFrame backward, this = %s\n\n", 
            GetCharacterHandle()->GetNamePath().ToCStr());
#endif
        DisplayList.CheckConsistency();
    }
    else if (targetFrameNumber > CurrentFrame)
    {
#ifdef GFX_TRACE_TIMELINE_EXECUTION
        printf("\n #### gotoFrame forward, from %d to %d, this = %s\n", 
            CurrentFrame, targetFrameNumber, GetCharacterHandle()->GetNamePath().ToCStr());
        DumpDisplayList(0, "Before");
#endif
        // Need to gather a snapshot only if targetFrame is greater than 1
        // and if it is greater than next frame (CurrentFrame + 1). For the next 
        // frame it is not enough to just execute frame tags by the regular way - 
        // calling to ExecuteFrameTags. 
        if (targetFrameNumber > 1 && targetFrameNumber > CurrentFrame + 1)
        {
            GFxTimelineSnapshot snapshot(GFxTimelineSnapshot::Direction_Forward, pRoot->GetHeap(), this);
            // Making the snapshot starting from the CurrentFrame + 1 and finishing
            // at targetFrame - 1. We will execute tags for the target frame
            // on regular basis by calling to ExecuteFrameTags.
            MakeSnapshot(&snapshot, CurrentFrame + 1, targetFrameNumber-1);

            // need to run init actions for every frame being skipped, as well as
            // for target one. But for the target frame InitAction tags will be
            // executed from inside the ExecuteFrameTags.
            for (UInt f = CurrentFrame + 1; f < targetFrameNumber; ++f)
            {
                ExecuteInitActionFrameTags(f);
            }

            CurrentFrame = targetFrameNumber;
            ExecuteSnapshot(&snapshot);
        }
        else
            CurrentFrame = targetFrameNumber;

        // Execute target frame's tags
        ExecuteFrameTags(targetFrameNumber);

#ifdef GFX_TRACE_TIMELINE_EXECUTION
        DumpDisplayList(0, "After");
        printf(" ^^^^ end of gotoFrame forward, this = %s\n\n", 
            GetCharacterHandle()->GetNamePath().ToCStr());
#endif

        DisplayList.CheckConsistency();
    }

    // GotoFrame stops by default.
    PlayStatePriv = GFxMovie::Stopped;
}

bool    GFxSprite::GotoLabeledFrame(const char* label, SInt offset)
{
    UInt    targetFrame = GFC_MAX_UINT;
    if (pDef->GetLabeledFrame(label, &targetFrame, 0))
    {
        GotoFrame((UInt)((SInt)targetFrame + offset));
        return true;
    }
    else
    {       
        LogWarning("Error: %s, MovieImpl::GotoLabeledFrame('%s') unknown label\n",
            GetCharacterHandle()->GetNamePath().ToCStr(), label);
        return false;
    }
}

void    GFxSprite::SetPlayState(PlayState s)
{ 
    PlayStatePriv = s;
    if (!IsUnloading() && !IsUnloaded())
        ModifyOptimizedPlayListLocal<GFxSprite>(pRoot);
#ifndef GFC_NO_SOUND
    if (PlayStatePriv == GFxMovie::Stopped)
    {
        SetStreamingSound(NULL);
    }
#endif
}

void    GFxSprite::Display(GFxDisplayContext &context)
{
    // We're invisible, so don't display!
    //!AB: but mask should be drawn even if it is invisible!
    if (!IsDisplayable())
        return;

    // save context transform pointers
    GFxDisplayContextTransforms oldXform;
    context.CopyTransformsTo(&oldXform);

    // We must apply a new binding table if the character needs to
    // use resource data from the loaded/imported GFxMovieDefImpl.
    GFxResourceBinding *psave = context.pResourceBinding;
    context.pResourceBinding = &pDefImpl->GetResourceBinding();

    GRenderer::Cxform wcx;
    GMatrix2D matrix;       
#ifndef GFC_NO_3D
    GMatrix3D matrix3D;
    GMatrix3D *pmatrix3D = &matrix3D;
#else
    GMatrix3D *pmatrix3D = NULL;
#endif
    GFxSprite* pmask = GetMask();  
    bool       masked = false;
    if (pmask)
    {
        if (!pmask->IsUsedAsMask() || pmask->IsUnloaded())
        {
            // if mask character was unloaded or unmarked as a mask - remove it from the sprite
            SetMask(NULL);
        }
        else
        {
            // mask can be outside the hierarchy
            context.PreDrawMask(pmask, &matrix, pmatrix3D);     // prepare context transform pointers
            context.PushAndDrawMask(pmask);
            masked = true;
        }
    }

    // prepare context transform pointers for display
    context.PreDisplay(oldXform, this, &matrix, pmatrix3D, &wcx);

    GFxDisplayContextFilters oldFilters;
    CharFilterDesc* pFilters = GetFilters();
    bool filtered = 0;

    if (pFilters && pFilters->HasFilters)
    {
#ifndef GFC_NO_FILTERS_PREPASS
        if (pFilters->PrePassResult)
        {
            context.DisplayFilterPrePass(this, pFilters->MainPassFilters,
                pFilters->PrePassResult, pFilters->FrameRect, pFilters->Width, pFilters->Height);
            goto endDisplay;
        }
#endif
        filtered = context.BeginFilters(oldFilters, this, pFilters->Filters);
    }

    if (pDrawingAPI)
    {
        GRenderer::BlendType    blend      = GetActiveBlendMode();
        pDrawingAPI->Display(context, *context.pParentMatrix, wcx, blend, GetClipDepth() > 0);
    }

    // Check if we need to push user renderer data
    {
        bool bUserDataPushed = false;
        if (pUserData)
            bUserDataPushed = context.GetRenderer()->PushUserData(&pUserData->UserData);

        DisplayList.Display(context);

        // Check if we need to pop user renderer data or 3D data
        if (bUserDataPushed)
            context.GetRenderer()->PopUserData();
    }

    if (filtered)
        context.EndFilters(oldFilters, this, pFilters->Filters);

endDisplay:
    context.pResourceBinding = psave;
    context.PostDisplay(oldXform);
    DoDisplayCallback();

    if (masked)
    {
        context.PopMask();
    }
}

void    GFxSprite::PropagateMouseEvent(const GFxEventId& id)
{
    GPtr<GFxSprite> thisHolder(this); // PropagateMouseEvent may release "this"; preventing.

    if (id.Id == GFxEventId::Event_MouseMove)
    {
        // Adjust x,y of this character if it is being dragged.
        if (pRoot->IsMouseSupportEnabled())
            GFxASCharacter::DoMouseDrag();
    }

    // MA: Hidden clips don't receive mouse events.
    // Tested by assigning _visible property to false,
    // but the object didn't hide in Flash.
    if (GetVisible() == false)  
        return;

    DisplayList.PropagateMouseEvent(id);
    // Notify us after children.
    OnEvent(id);
}

void    GFxSprite::PropagateKeyEvent(const GFxEventId& id, int* pkeyMask)
{
    GPtr<GFxSprite> thisHolder(this); // PropagateKeyEvent may release "this"; preventing.

    // AB: Hidden clips don't receive key events.
    // Tested by assigning _visible property to false,
    // but the object didn't hide in Flash. nEed to test more:
    // what about hidden by timeline?
    if (!GetVisible())  
        return;

    DisplayList.PropagateKeyEvent (id, pkeyMask);
    // Notify us after children.
    OnKeyEvent(id, pkeyMask);
}

void    GFxSprite::CloneInternalData(const GFxASCharacter* src)
{
    GASSERT(src);
    GFxASCharacter::CloneInternalData(src);
    if (src->IsSprite())
    {
        const GFxSprite* pSrcSprite = src->ToSprite();
        if (pSrcSprite->ActsAsButton())
            SetHasButtonHandlers (true);
    }
    SetDirtyFlag();
}

// ThisPtr - sprite
// Arg(0) - constructor function of the class
static void GFx_InitializeClassInstance(const GASFnCall& fn)
{
    GPtr<GFxSprite> pscriptCh = fn.ThisPtr->ToSprite();
    GASSERT(pscriptCh);

    GASFunctionRef ctorFunc = fn.Arg(0).ToFunction(fn.Env);
    GASSERT(!ctorFunc.IsNull());

    pscriptCh->SetProtoToPrototypeOf(ctorFunc.GetObjectPtr());
}

// ThisPtr - sprite
// Arg(0) - String, name of instance
// Arg(1) - Boolean, true, if construct event should be fired (!AB: not anymore)
//static 
void GFx_FindClassAndInitializeClassInstance(const GASFnCall& fn)
{
    GASSERT(fn.Env);

    GASGlobalContext* gctxt = fn.Env->GetGC();
    GASSERT(gctxt);

    GASFunctionRef ctorFunc;
    const GASString symbolName(fn.Arg(0).ToString(fn.Env));
    if (!symbolName.IsEmpty())
    {
        if (gctxt->FindRegisteredClass(fn.Env->GetSC(), symbolName, &ctorFunc))
        {
            GASSERT(!ctorFunc.IsNull());

            GPtr<GFxSprite> pscriptCh = fn.ThisPtr->ToSprite();
            GASSERT(pscriptCh);

            pscriptCh->SetProtoToPrototypeOf(ctorFunc.GetObjectPtr());

            // now, check for 'construct' event and fire it if Arg(1) is set to TRUE
            //if (fn.Arg(1).ToBool(fn.Env)) //!AB, fire onConstruct always, since it is impossible
            {                               // to check whether onConstruct method exists or not
                GFxMovieRoot::ActionEntry e(pscriptCh, GFxEventId::Event_Construct);
                e.Execute(pscriptCh->GetMovieRoot());
            }

            // now, invoke a constructor
            GFxMovieRoot::ActionEntry e(pscriptCh, ctorFunc);
            e.Execute(pscriptCh->GetMovieRoot());
        }
        else
        {
            // a special case, when a component doesn't have a class (it is possible if Linkage->AS 2.0 Class field left empty)
            // but there is a construct event for its instance.

            // now, check for 'construct' event and fire it if Arg(1) is set to TRUE
            //if (fn.Arg(1).ToBool(fn.Env)) //!AB, fire onConstruct always, since it is impossible
            {                               // to check whether onConstruct method exists or not
                GPtr<GFxSprite> pscriptCh = fn.ThisPtr->ToSprite();
                GASSERT(pscriptCh);

                GFxMovieRoot::ActionEntry e(pscriptCh, GFxEventId::Event_Construct);
                e.Execute(pscriptCh->GetMovieRoot());
            }

        }
    }
}

// Arg(0) - Object, initializer object
static void GFx_InitObjectMembers(const GASFnCall& fn)
{
    GASSERT(fn.Env);
    GASSERT(fn.NArgs == 1);

    GPtr<GFxSprite> pscriptCh = fn.ThisPtr->ToSprite();
    GASSERT(pscriptCh);

    const GASObjectInterface* pinitSource = fn.Arg(0).ToObjectInterface(fn.Env);

    struct InitVisitor : public GASObject::MemberVisitor
    {
        GASEnvironment *pEnv;
        GFxASCharacter *pCharacter;

        InitVisitor(GASEnvironment *penv, GFxASCharacter *pchar)
            : pEnv(penv), pCharacter(pchar)
        { }

        virtual void    Visit(const GASString& name, const GASValue& val, UByte flags)
        {
            GUNUSED(flags);                
            pCharacter->SetMember(pEnv, name, val);
        }
    };
    InitVisitor memberVisitor(fn.Env, pscriptCh);
    pinitSource->VisitMembers(memberVisitor.pEnv->GetSC(), &memberVisitor);
}

// Returns 0 if nothing to do
// 1 - if need to add to optimized play list
// -1 - if need to remove from optimized play list
int GFxSprite::CheckAdvanceStatus(bool playingNow)
{
    int rv = 0;
#ifndef GFC_USE_OLD_ADVANCE
    // First of all, check if advance is disabled at all. Advance is disabled
    // * if noAdvance extension property is set;
    // * if sprite is invisible and _global.noInvisibleAdvance extension property is set;
    // * if sprite is unloading or unloaded from timeline.
    bool advanceDisabled = (IsAdvanceDisabled() || !CanAdvanceChar());

    // Check if movie is playable.
    //bool advancable = (!advanceDisabled && GetPlayState() == GFxMovie::Playing);
    bool advancable = (!advanceDisabled && (GetPlayState() == GFxMovie::Playing || 
        pRoot->IsDraggingCharacter(this)
#ifndef GFC_NO_SOUND
        || (pActiveSounds && pActiveSounds->Sounds.GetSize() != 0)
#endif
        ));

    if (advancable) 
    {
        // if it is already playing and advancable - do nothing (return 0).
        // otherwise, return 1 (play).
        rv = !playingNow; //(playingNow) ? 0 : 1
#ifdef GFC_TRACE_ADVANCE
        if (rv)
        {
            printf("+++ Advance: Starting to play: %s, state %s\n", GetCharacterHandle()->GetNamePath().ToCStr(),
                (GetPlayState() == GFxMovie::Playing) ? "playing" : "stopped");
        }
#endif
    }
    else
    {
        if (playingNow)
        {
            // check, if the onEnterFrame exists
            if (!advanceDisabled)
            {
                if (!HasEventHandler(GFxEventId::Event_EnterFrame))
                {
                    rv = -1;
#ifdef GFC_TRACE_ADVANCE
                    printf("+++ Advance: Stopping play (no onEnterFrame): %s, state %s\n", 
                        GetCharacterHandle()->GetNamePath().ToCStr(),
                        (GetPlayState() == GFxMovie::Playing) ? "playing" : "stopped");
#endif
                }
            }
            else
            {
                rv = -1;
#ifdef GFC_TRACE_ADVANCE
                printf("+++ Advance: Stopping play (adv disabled): %s, state %s\n", 
                    GetCharacterHandle()->GetNamePath().ToCStr(),
                    (GetPlayState() == GFxMovie::Playing) ? "playing" : "stopped");
#endif
            }
        }
        else
        {
            if (!advanceDisabled)
            {
                if (HasEventHandler(GFxEventId::Event_EnterFrame))
                {
                    rv = 1;
#ifdef GFC_TRACE_ADVANCE
                    printf("+++ Advance: Starting to play (onEnterFrame): %s, state %s\n", 
                        GetCharacterHandle()->GetNamePath().ToCStr(),
                        (GetPlayState() == GFxMovie::Playing) ? "playing" : "stopped");
#endif
                }
            }
        }
    }
#else
    GUNUSED(playingNow);
#endif //#ifndef GFC_USE_OLD_ADVANCE
    return rv;
}

// Add an object to the display list.
GFxCharacter*   GFxSprite::AddDisplayObject(
    const GFxCharPosInfo &pos,
    const GASString &name,
    const GArrayLH<GFxSwfEvent*, GFxStatMD_Tags_Mem> *peventHandlers,
    const GASObjectInterface *pinitSource,
    UInt createFrame,
    UInt32 addFlags,
    GFxCharacterCreateInfo* pcharCreateOverride,
    GFxASCharacter* origChar)
{
    GASSERT(pDef);

    bool placeObject = (addFlags & GFxDisplayList::Flags_PlaceObject) != 0;
    bool replaceIfDepthIsOccupied = (addFlags & GFxDisplayList::Flags_ReplaceIfDepthIsOccupied) != 0;

    // TBD:
    // Here we look up a character but do not return it's binding.
    // This means that while we will have the right character created,
    // its pDefImpl will correspond OUR table and not THEIR table.
    // However, imported characters MUST reference their own tables for resources!

    GFxCharacterCreateInfo ccinfo = pcharCreateOverride ? *pcharCreateOverride :    
        pDefImpl->GetCharacterCreateInfo(pos.CharacterId);
    if (!ccinfo.pCharDef)
    {
        LogError("%s, GFxSprite::AddDisplayObject(): unknown cid = %d\n",
            GetCharacterHandle()->GetNamePath().ToCStr(), pos.CharacterId.GetIdIndex());
        return NULL;
    }

    // If we already have this object on this plane, then move it instead of replacing it.
    // This can happen when wrapping around from last frame to first one, where
    // the first frame fill bring back to life objects marked for removal after last frame.
    // It can also happen when seeking back.
    bool markedForRemove = false;
    GFxCharacter* pexistingChar = DisplayList.GetCharacterAtDepth(pos.Depth, &markedForRemove);

    if (placeObject)
    {
        // If the object was moved explicitly in Action script, leave it there.
        bool animatedByTimeline = true;
        if (pexistingChar)
        {
            animatedByTimeline = pexistingChar->GetAcceptAnimMoves();
            if (!animatedByTimeline && pexistingChar->GetContinueAnimationFlag())
            {
                pexistingChar->SetAcceptAnimMoves(true);// reset the flag
                animatedByTimeline = true;
            }
        }
        if (!markedForRemove && pexistingChar && !animatedByTimeline)
            return NULL;        

        if ( pexistingChar && !pexistingChar->IsUnloadQueuedUp() &&
            (pexistingChar->GetId() == pos.CharacterId))
        {
            // We need to use OriginalName here to determine is it the same character
            // or a different one. We can't use the regular name (GetName() method), since
            // it could be changed via ActionScript (_name property) and this shouldn't
            // affect the timeline execution (see test_name_change1.swf)
            const GASString originalName
                ((pexistingChar->IsCharASCharacter()) ? pexistingChar->CharToASCharacter_Unsafe()->GetOriginalName() : 
                ASEnvironment.GetBuiltin(GASBuiltin_empty_));

            // The name can be set only for scriptable characters, such as Sprite, Button and TextField.
            // Though, theoretically it is possible that PlaceObject for generic character contains
            // a name. Technically, this is incorrect, but possible (see wizardy.swf).
            // In this case, we need to skip names comparison. We can use pexistingChar->IsASCharacter()
            // criteria, since Ids are equal.

            if (!pexistingChar->IsCharASCharacter() || 
                (name.IsEmpty() && originalName.IsEmpty()) ||
                (name.IsEmpty() && ((GFxASCharacter*)pexistingChar)->HasInstanceBasedName()) ||
                (!name.IsEmpty() && originalName == name))
            {
                // Move will bring the object back by clearing its MarkForRemove flag
                // in display list. We only do so if we are talking about THE SAME object,
                // i.e. created in the same frame. Flash considers objects created in
                // different frames to be different, even if they share a name.
                if (pexistingChar->GetCreateFrame() == ((createFrame == GFC_MAX_UINT) ? CurrentFrame : createFrame))
                {
                    GFxCharPosInfo newPos = pos;
                    if (pexistingChar->IsCharASCharacter())
                    {
                        GFxASCharacter* asCh = pexistingChar->CharToASCharacter_Unsafe();
                        if (!pos.HasFilters() && asCh->GetFilters())
                            newPos.SetFiltersFlag();
                    }
                    if (!pos.HasBlendMode() && pexistingChar->GetBlendMode() != GRenderer::Blend_None)
                    {
                        newPos.SetBlendModeFlag();
                        newPos.BlendMode = GRenderer::Blend_None;
                    }
                    if (!pos.HasCxform() && !pexistingChar->GetCxform().IsIdentity())
                    {
                        // if the object is moved by AddDisplayObject and no color transform suppose to 
                        // to be used then we need to reset the color transform for the existing
                        // character, since it may have it changed earlier. 
                        // This is actual when the character has changed Cxform matrix and
                        // then time-line goes to the first frame, where this Cxform doesn't
                        // exist. (!AB)
                        //                    GFxCharPosInfo newPos = pos;
                        newPos.SetCxFormFlag();
                        newPos.ColorTransform.SetIdentity();
                    }

                    MoveDisplayObject(newPos);             

                    // if character was marked for remove the
                    return NULL;
                }
                // Otherwise replace object with a new-state fresh instance of it.
                replaceIfDepthIsOccupied = 1;
            }
        }
    }
    else
    {
        replaceIfDepthIsOccupied = 1;
    }
    SetDirtyFlag();

    GPtr<GFxCharacter>  ch          = *ccinfo.pCharDef->CreateCharacterInstance(this, pos.CharacterId,
        ccinfo.pBindDefImpl);
    GASSERT(ch);
    GFxASCharacter*     pscriptCh   = ch->CharToASCharacter();
    GFxSprite*          pspriteCh   = pscriptCh ? pscriptCh->ToSprite() : 0;
    bool                nameSet     = 0;

    ch->SetScale9GridExists(false);

    const GFxASCharacter* parent = ch->GetParent();
    while (parent)
    {
        if (parent->GetScale9Grid())
        {
            ch->SetScale9GridExists(true);
            ch->PropagateScale9GridExists();
            break;
        }
        parent = parent->GetParent();
    }

    if (pscriptCh)
    {
        if (!name.IsEmpty())
        {
            ch->SetOriginalName(name);          
            nameSet = 1;         
        }

        if (origChar)
        {
            // if we are duplicating char - call CloneInternalData
            pscriptCh->CloneInternalData(origChar);
        }

        // in the case if AddDisplayObject is not initiated through timeline
        // (Flag_PlaceObject is NOT set) we need to mark the newly created
        // object by SetAcceptAnimMovies(false).
        if (!(addFlags & GFxDisplayList::Flags_PlaceObject))
            pscriptCh->SetAcceptAnimMoves(false);
        else
            pscriptCh->SetTimelineObjectFlag(true);
    }
    if ((addFlags & GFxDisplayList::Flags_PlaceObject))
        ch->SetTimelineObjectFlag(true);

    ch->SetCreateFrame((createFrame == GFC_MAX_UINT) ? CurrentFrame : createFrame);

    // Attach event Handlers (if any).

    bool hasConstructEvent = true; //!AB, fire onConstruct always, since it is impossible
    // to check whether onConstruct method exists or not
    GFxMovieRoot* proot = GetMovieRoot();
    if (pscriptCh && peventHandlers)
    {
        for (UPInt i = 0, n = peventHandlers->GetSize(); i < n; i++)   
        {       
            (*peventHandlers)[i]->AttachTo(pscriptCh);
            // If we installed a button handler, sprite will work in button mode...
            if (pspriteCh && (*peventHandlers)[i]->Event.IsButtonEvent())
                pspriteCh->SetHasButtonHandlers (true);

            if (placeObject)
            {
                if ((*peventHandlers)[i]->Event.Id == GFxEventId::Event_Initialize)
                {
                    GASActionBufferData* pactionOpData = (*peventHandlers)[i]->pActionOpData;
                    if (pactionOpData && !pactionOpData->IsNull())
                    {
                        GFxMovieRoot::ActionEntry* pe = proot->InsertEmptyAction(GFxMovieRoot::AP_Initialize);
                        if (pe) pe->SetAction(pscriptCh, GFxEventId::Event_Initialize);
                        //{
                        //    GPtr<GASActionBuffer> pbuff =
                        //        *GHEAP_NEW(proot->GetMovieHeap())
                        //        GASActionBuffer(pscriptCh->GetASEnvironment()->GetSC(), pactionOpData);
                        //    pe->SetAction(pscriptCh, pbuff);
                        //}
                    }
                }
                //!AB, fire onConstruct always, since it is impossible
                // to check whether onConstruct method exists or not. It is only possible
                // to check whether onClipEvent(construct) exists or not, but this is not enough.
                //if ((*peventHandlers)[i]->Event.Id == GFxEventId::Event_Construct)
                //{
                //    hasConstructEvent = true;
                //}
            }
        }
    }

    // Check for registered classes
    UInt sessionId = 0;
    if (pscriptCh)
    {
        GASGlobalContext* gctxt = this->GetGC();
        GASSERT(gctxt);

        GASFunctionRef ctorFunc;    
        const GString* psymbolName = ch->GetResourceMovieDef()->GetNameOfExportedResource(ccinfo.pCharDef->GetId());

        UInt oldSessionId;
        sessionId = proot->ActionQueue.StartNewSession(&oldSessionId);

        bool initSourceUsed = false;

        if (psymbolName)
        {
            GASString symbolName = GetASEnvironment()->CreateString(*psymbolName);

            if (gctxt->FindRegisteredClass(GetASEnvironment()->GetSC(), symbolName, &ctorFunc))
            {
                // first - schedule initializer of class instance. It will
                // assign proper prototype to the pscriptCh.
                GASValueArray params;
                GFxMovieRoot::ActionEntry* pe;

                pe = proot->InsertEmptyAction(GFxMovieRoot::AP_Initialize);
                params.PushBack(ctorFunc);
                if (pe) pe->SetAction(pscriptCh, GFx_InitializeClassInstance, &params);

                //if (hasConstructEvent) //!AB, fire onConstruct always, since it is impossible
                {                        // to check whether onConstruct method exists or not
                    GFxMovieRoot::ActionEntry* pe = proot->InsertEmptyAction(GFxMovieRoot::AP_Construct);
                    if (pe) pe->SetAction(pscriptCh, GFxEventId::Event_Construct);
                    hasConstructEvent = false;
                }

                // Queue up copying init properties if they exist (for SWF 6+).
                if (GetVersion() >= 6 && pinitSource && pscriptCh)
                {
                    params.Resize(0);
                    GASValue v;
                    v.SetAsObjectInterface(const_cast<GASObjectInterface*>(pinitSource));
                    params.PushBack(v);
                    GFxMovieRoot::ActionEntry* pe = proot->InsertEmptyAction(GFxMovieRoot::AP_Construct);
                    if (pe) pe->SetAction(pscriptCh, GFx_InitObjectMembers, &params);
                }
                initSourceUsed = true;

                // now, schedule constructor invocation
                pe = proot->InsertEmptyAction(GFxMovieRoot::AP_Construct);
                if (pe) pe->SetAction(pscriptCh, ctorFunc);
            }
            else if (placeObject)
            {
                GASValueArray params;
                params.PushBack(GASValue(symbolName)); //[0]
                //params.PushBack(GASValue(hasConstructEvent)); //[1]

                GFxMovieRoot::ActionEntry* pe = proot->InsertEmptyAction(GFxMovieRoot::AP_Construct);
                if (pe) pe->SetAction(pscriptCh, GFx_FindClassAndInitializeClassInstance, &params);
                hasConstructEvent = false;
            }
        }
        if (placeObject)
        {
            if (hasConstructEvent)
            {
                // if no class is registered, only onClipEvent(construct)

                GFxMovieRoot::ActionEntry* pe = proot->InsertEmptyAction(GFxMovieRoot::AP_Construct);
                if (pe) pe->SetAction(pscriptCh, GFxEventId::Event_Construct);
            }
        }
        else if (!initSourceUsed)
        {
            // Not a placeobject (means - attachMovie or duplicateMovieClip) and no registered class:
            // queue up copying init properties if they exist (for SWF 6+).
            if (GetVersion() >= 6 && pinitSource && pscriptCh)
            {
                GASValueArray params;
                GASValue v;
                v.SetAsObjectInterface(const_cast<GASObjectInterface*>(pinitSource));
                params.PushBack(v);
                GFxMovieRoot::ActionEntry* pe = proot->InsertEmptyAction(GFxMovieRoot::AP_Construct);
                if (pe) pe->SetAction(pscriptCh, GFx_InitObjectMembers, &params);
            }
        }
        proot->ActionQueue.RestoreSession(oldSessionId);
    }

    addFlags &= ~GFxDisplayList::Flags_ReplaceIfDepthIsOccupied;
    if (replaceIfDepthIsOccupied)
        addFlags |= GFxDisplayList::Flags_ReplaceIfDepthIsOccupied;

    if (pscriptCh)
    {
        pscriptCh->AddToPlayList(proot);
        pscriptCh->ModifyOptimizedPlayList(proot);
    }

    DisplayList.AddDisplayObject(pos, ch.GetPtr(), addFlags);   

    // Assign sticky variables (only applies if there is an identifiable name).
    if (nameSet)
        proot->ResolveStickyVariables(pscriptCh);

    GASSERT(!ch || ch->GetRefCount() > 1);

    // here is a special case for attachMovie. If we have queued up constructors' invocations before 
    // DisplayList.AddDisplayObject then we need to execute them right now.
    if (pscriptCh && !placeObject)
    {
        proot->DoActionsForSession(sessionId);

        // check if onLoad event is allowed. OnEventLoad puts onLoad event into a queue. 
        // This happens BEFORE all children are added and BEFORE constructor is executed.
        // But real decision about executing/not executing onLoad event may be done only
        // AFTER all children are added ("DisplayList.AddDisplayObject") and ctor is
        // executed ("DoActionsForSession"), but BEFORE the first frame execution. 
        // So, we set a flag "OnLoadReqd", if new sprite really needs "onLoad" event to 
        // be executed. This flag is tested in ExecuteEvent and if it is not set for 
        // non-timeline sprite, the onLoad event will not be executed, even though 
        // it is in the queue. This is necessary for correct execution of onLoad events
        // during "attachMovie" call.
        if (pspriteCh && pspriteCh->HasEventHandler(GFxEventId::Event_Load))
            pspriteCh->SetOnLoadReqd();    
    }
    else
    {
        // Allow onLoad events for timeline sprites
        if (pspriteCh)
            pspriteCh->SetOnLoadReqd();    
    }

    if (pscriptCh && nameSet)
    {
        // check if the name is "hitArea" and assign the hitArea if it is so.
        if (name == ASEnvironment.GetBuiltin(GASBuiltin_hitArea))
            SetHitArea((GFxSprite*) pscriptCh);
    }
    return ch.GetPtr();
}


void    GFxSprite::MoveDisplayObject(const GFxCharPosInfo &pos)
{
    DisplayList.MoveDisplayObject(pos);
    SetDirtyFlag();
}


void    GFxSprite::ReplaceDisplayObject(const GFxCharPosInfo &pos, const GASString& name)
{
    GASSERT(pDef);

    GFxCharacterCreateInfo ccinfo = pDefImpl->GetCharacterCreateInfo(pos.CharacterId);    
    if (ccinfo.pCharDef == NULL)
    {
        LogError("%s, Sprite::ReplaceDisplayObject(): unknown cid = %d\n", GetCharacterHandle()->GetNamePath().ToCStr(), pos.CharacterId.GetIdIndex());
        return;
    }
    GASSERT(ccinfo.pCharDef && ccinfo.pBindDefImpl);

    GPtr<GFxCharacter> ch = *ccinfo.pCharDef->CreateCharacterInstance(this, pos.CharacterId,
        ccinfo.pBindDefImpl);
    GASSERT(ch);
    //GASSERT(!ch->IsCharASCharacter()); //!AB: technically, AS chars shouldn't be used here

    ReplaceDisplayObject(pos, ch, name);

    // we need to set correct create frame to the character
    ch->SetCreateFrame(CurrentFrame);
}


void    GFxSprite::ReplaceDisplayObject(const GFxCharPosInfo &pos, GFxCharacter* ch, const GASString& name)
{
    GASSERT(ch != NULL);
    if (!name.IsEmpty())
        ch->SetName(name);  

    const GFxASCharacter* parent = ch->GetParent();
    while (parent)
    {
        if (parent->GetScale9Grid())
        {
            ch->SetScale9GridExists(true);
            ch->PropagateScale9GridExists();
            break;
        }
        parent = parent->GetParent();
    }

    //!AB: ReplaceDisplayObject shouldn't be used for "active" objects, such as sprites, 
    // buttons or dynamic textfields. For such "active" object the Remove tag should be 
    // executed first. However, Flash CS5 sometimes uses EditTextChar instead of
    // StaticText and applies "replace" to it w/o Remove tag. Not sure if we need
    // to add such objects in playlist or not since they were not intended to be "active".
    //!AB: make sure the replacee char is already in play list (if it is AS char)
    //GASSERT(!ch->IsCharASCharacter() || 
    //        ch->CharToASCharacter_Unsafe()->pPlayPrev != NULL || 
    //        pRoot->pPlayListHead == ch->CharToASCharacter_Unsafe());

    DisplayList.ReplaceDisplayObject(pos, ch);

    if (!name.IsEmpty() && ch->IsCharASCharacter())
        GetMovieRoot()->ResolveStickyVariables(ch->CharToASCharacter_Unsafe());
    SetDirtyFlag();
}


void    GFxSprite::RemoveDisplayObject(SInt depth, GFxResourceId id)
{
    DisplayList.RemoveDisplayObject(depth, id);
    SetDirtyFlag();
}


// For debugging -- return the id of the GFxCharacter at the specified depth.
// Return -1 if nobody's home.
GFxResourceId GFxSprite::GetIdAtDepth(int depth)
{
    GFxCharacter* pch = GetCharacterAtDepth(depth);
    return pch ? pch->GetId() : GFxResourceId();
}

GFxCharacter*   GFxSprite::GetCharacterAtDepth(int depth)
{
    UPInt index = DisplayList.GetDisplayIndex(depth);
    if (index == GFC_MAX_UPINT)    
        return 0;   
    return DisplayList.GetDisplayObject(index).GetCharacter();
}


// Movie Loading support
bool        GFxSprite::ReplaceChildCharacter(GFxASCharacter *poldChar, GFxASCharacter *pnewChar)
{
    GASSERT(poldChar != 0);
    GASSERT(pnewChar != 0);
    GASSERT(poldChar->GetParent() == this);

    // Get the display entry.
    UPInt index = DisplayList.GetDisplayIndex(poldChar->GetDepth());
    if (index == GFC_MAX_UPINT)
        return 0;

    // Copy physical properties & re-link all ActionScript references.
    pnewChar->CopyPhysicalProperties(poldChar);
    // For Sprites, we also copy _lockroot.
    if (pnewChar->IsSprite() && poldChar->IsSprite())
    {
        pnewChar->ToSprite()->SetLockRoot(poldChar->ToSprite()->IsLockRoot());

        // Filters
        GFxASCharacter::CharFilterDesc* poldfilters = poldChar->ToSprite()->GetFilters();
        if (poldfilters)
        {
            if (poldfilters->LastIsTemp)
            {
                GArray<GFxFilterDesc> newfilters (poldfilters->Filters);
                newfilters.PopBack();
                pnewChar->SetFilters(newfilters);
            }
            else
                pnewChar->SetFilters(poldfilters->Filters);
        }
    }

    // Inform old character of unloading.
    // Use OnUnloading instead of OnEventUnload to fire onUnload events.
    poldChar->OnUnloading(); 
    poldChar->SetUnloading(true);
    GetMovieRoot()->DoActions();

    pnewChar->MoveNameHandle(poldChar);

    // Alter the display list.
    // Need to get the index again since it might be changed because of DoActions.
    index = DisplayList.GetDisplayIndex(poldChar->GetDepth());
    if (index == GFC_MAX_UPINT)
        return 0;
    DisplayList.ReplaceDisplayObjectAtIndex(index, pnewChar);
    SetDirtyFlag();
    return true;
}

bool        GFxSprite::ReplaceChildCharacterOnLoad(GFxASCharacter *poldChar, GFxASCharacter *pnewChar)
{
    if (!ReplaceChildCharacter(poldChar, pnewChar))
        return false;

    // And call load.
    pnewChar->OnEventLoad();
    GetMovieRoot()->DoActions();
    SetDirtyFlag();
    return true;
}



//
// *** Sprite ActionScript support
//

// GFxASCharacter override to indicate which standard members are handled for us.
UInt32  GFxSprite::GetStandardMemberBitMask() const
{
    // MovieClip lets base handle all members it supports.
    return UInt32(
        M_BitMask_PhysicalMembers |
        M_BitMask_CommonMembers |           
        (1 << M_target) |
        (1 << M_droptarget) |
        (1 << M_url) |          
        (1 << M_blendMode) |
        (1 << M_cacheAsBitmap) |
        (1 << M_filters) |
        (1 << M_focusrect) |
        (1 << M_enabled) |
        (1 << M_trackAsMenu) |
        (1 << M_lockroot) |
        (1 << M_tabEnabled) |
        (1 << M_tabIndex) |
        (1 << M_useHandCursor) |        
        (1 << M_quality) |
        (1 << M_highquality) |
        (1 << M_soundbuftime) |
        (1 << M_xmouse) |
        (1 << M_ymouse) |

        // GFxASCharacter will report these as read-only for us,
        // but we still need to handle them in GetMember.
        (1 << M_currentframe) |
        (1 << M_totalframes) |
        (1 << M_framesloaded) 
        );
}

bool    GFxSprite::GetStandardMember(StandardMember member, GASValue* pval, bool opcodeFlag) const
{
    if (GFxASCharacter::GetStandardMember (member, pval, opcodeFlag))
        return true;

    // Handle MovieClip specific "standard" members.
    switch(member)
    {
    case M_currentframe:
        pval->SetInt(CurrentFrame + 1);
        return true;

    case M_totalframes:             
        pval->SetInt(pDef->GetFrameCount());
        return true;

    case M_framesloaded:
        pval->SetInt(GetLoadingFrame());
        return true;

    case M_lockroot:
        pval->SetBool(IsLockRoot());            
        return true;

    case M_focusEnabled:   
        {
            if (FocusEnabled.IsDefined())
                pval->SetBool(FocusEnabled.IsTrue());
            else
                pval->SetUndefined();
            return 1;
        }

    case M_tabChildren:   
        {
            // Is a yellow rectangle visible around a focused GFxASCharacter Clip (?)
            if (TabChildren.IsDefined())
                pval->SetBool(TabChildren.IsTrue());
            else
                pval->SetUndefined();
            return 1;
        }

    case M_hitArea:
        if (pHitAreaHandle)
        {
            pval->SetAsCharacterHandle(pHitAreaHandle);
            return 1;
        }
        else
            pval->SetUndefined();
        break;

    case M_scale9Grid:
        if (GetASEnvironment()->GetVersion() >= 8)
        {
            if (pScale9Grid)
            {
                GASEnvironment* penv = const_cast<GASEnvironment*>(GetASEnvironment());

#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
                GPtr<GASRectangleObject> rectObj = *GHEAP_NEW(penv->GetHeap()) GASRectangleObject(penv);
                GASRect gr(TwipsToPixels(pScale9Grid->x), 
                    TwipsToPixels(pScale9Grid->y), 
                    TwipsToPixels(pScale9Grid->x+pScale9Grid->w), 
                    TwipsToPixels(pScale9Grid->y+pScale9Grid->h)); 
                rectObj->SetProperties(penv, gr);
#else
                GPtr<GASObject> rectObj = *GHEAP_NEW(penv->GetHeap()) GASObject(penv);
                GASStringContext *psc = penv->GetSC();
                rectObj->SetConstMemberRaw(psc, "x", TwipsToPixels(pScale9Grid->x));
                rectObj->SetConstMemberRaw(psc, "y", TwipsToPixels(pScale9Grid->y));
                rectObj->SetConstMemberRaw(psc, "width", TwipsToPixels(pScale9Grid->x+pScale9Grid->w));
                rectObj->SetConstMemberRaw(psc, "height", TwipsToPixels(pScale9Grid->y+pScale9Grid->h));
#endif
                pval->SetAsObject(rectObj);
            }
            else
            {
                pval->SetUndefined();
            }
            return true;
        }
        break;

        // extension
    case M_hitTestDisable:
        if (GetASEnvironment()->CheckExtensions())
        {
            pval->SetBool(IsHitTestDisableFlagSet());
            return 1;
        }
        break;

    default:
        break;
    }
    return false;
}

bool    GFxSprite::SetStandardMember(StandardMember member, const GASValue& origVal, bool opcodeFlag)
{   
    GASValue val(origVal);
    GASEnvironment* penv = GetASEnvironment();
    if (member > M_BuiltInProperty_End)
    {
        // Check, if there are watch points set. Invoke, if any.
        if (penv && ASMovieClipObj && ASMovieClipObj->HasWatchpoint()) // have set any watchpoints?
        {
            GASValue newVal;
            if (ASMovieClipObj->InvokeWatchpoint(penv, 
                penv->CreateConstString(GFxASCharacter::MemberTable[member].pName), val, &newVal))
            {
                val = newVal;
            }
        }
    }
    if (GFxASCharacter::SetStandardMember (member, val, opcodeFlag))
        return true;

    // Handle MovieClip specific "standard" members.
    switch(member)
    {   
    case M_currentframe:
    case M_totalframes:
    case M_framesloaded:
        // Already handled as Read-Only in base,
        // so we won't get here.
        return true;

    case M_lockroot:
        SetLockRoot(val.ToBool(GetASEnvironment()));
        return true;

    case M_focusEnabled:
        if (!val.IsUndefined())
            FocusEnabled = val.ToBool(GetASEnvironment());
        else
            FocusEnabled = Bool3W();
        return 1;

    case M_tabChildren:
        if (!val.IsUndefined())
            TabChildren = val.ToBool(GetASEnvironment());
        else
            TabChildren = Bool3W();
        return 1;

        // extension
    case M_hitTestDisable:
        if (GetASEnvironment()->CheckExtensions())
        {
            SetHitTestDisableFlag(val.ToBool(GetASEnvironment()));
            return 1;
        }
        break;

    case M_hitArea:
        {
            // If hit area is not a MovieClip then it should be handled as regular member 
            GFxASCharacter* pcharacter = val.ToASCharacter(GetASEnvironment());
            if (pcharacter && pcharacter->IsSprite())
            {
                SetHitArea((GFxSprite*) pcharacter);
                return 1;
            }
            SetHitArea(0);
        }
        break;

    case M_scale9Grid:
        if (GetASEnvironment()->GetVersion() >= 8)
        {
            GASEnvironment* penv = GetASEnvironment();
            GASObject* pobj = val.ToObject(penv);

#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
            if (pobj && pobj->GetObjectType() == Object_Rectangle)
            {
                GASRectangleObject* prect = (GASRectangleObject*)pobj;
                GASRect gr;
                prect->GetProperties(penv, gr);
                GFxScale9Grid sg;
                sg.x = PixelsToTwips(Float(gr.Left));
                sg.y = PixelsToTwips(Float(gr.Top));
                sg.w = PixelsToTwips(Float(gr.Width()));
                sg.h = PixelsToTwips(Float(gr.Height()));
                SetScale9Grid(&sg);
            }
#else
            if (pobj)
            {
                GASStringContext *psc = penv->GetSC();
                GASValue params[4];
                pobj->GetConstMemberRaw(psc, "x", &params[0]);
                pobj->GetConstMemberRaw(psc, "y", &params[1]);
                pobj->GetConstMemberRaw(psc, "width", &params[2]);
                pobj->GetConstMemberRaw(psc, "height", &params[3]);
                GFxScale9Grid sg;
                sg.x = PixelsToTwips(Float(params[0].ToNumber(penv)));
                sg.y = PixelsToTwips(Float(params[1].ToNumber(penv)));
                sg.w = PixelsToTwips(Float(params[2].ToNumber(penv)));
                sg.h = PixelsToTwips(Float(params[3].ToNumber(penv)));
                SetScale9Grid(&sg);
            }
#endif
            else
                SetScale9Grid(0);
            return true;
        }
        break;

        // No other custom properties to set for now.
    default:
        break;
    }
    return false;
}

// Set the named member to the value.  Return true if we have
// that member; false otherwise.
bool    GFxSprite::SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{    
    if (IsStandardMember(name))
    {
        StandardMember member = GetStandardMemberConstant(name);
        if (SetStandardMember(member, val, 0))
            return true;
    }

    if (ASMovieClipObj || GetMovieClipObject())
    {
        // Note that MovieClipObject will also track setting of button
        // handlers, i.e. 'onPress', etc.
        return ASMovieClipObj->SetMemberRaw(psc, name, val, flags);
    }
    return false;
}

// internal method, a common implementation for both GetMember/GetMemberRaw.
// Only either penv or psc should be not NULL. If penv is not null, then this method
// works like GetMember, if psc is not NULL - it is like GetMemberRaw
bool    GFxSprite::GetMember(GASEnvironment* penv, GASStringContext *psc, const GASString& name, GASValue* pval)
{
    if (IsStandardMember(name))
    {
        StandardMember member = GetStandardMemberConstant(name);
        if (GetStandardMember(member, pval, 0))
            return true;

        switch(member)
        {
        case M_transform:
            {

#ifndef GFC_NO_FXPLAYER_AS_TRANSFORM
                // create a GASTransformObject
                GPtr<GASTransformObject> transfObj = 
                    *GHEAP_NEW(GetASEnvironment()->GetHeap()) GASTransformObject(GetASEnvironment(), this);
                pval->SetAsObject(transfObj);
                return true;
#else
                GFC_DEBUG_WARNING(1, "Transform ActionScript class was not included in this GFx build.");
                pval->SetUndefined();
                return true;
#endif
            }
        case M__version:
            if (IsLevelsMovie())
            {
                pval->SetString(GetASEnvironment()->CreateConstString(GFX_FLASH_VERSION));
                return true;
            }
            break;

#ifndef GFC_NO_3D
        case M_matrix3d:
            {
                const GMatrix3D* pm3D = GetMatrix3D();
                if (!pm3D)
                    pm3D = &GMatrix3D::Identity;

                // create a GASArrayObject
                GPtr<GASArrayObject> arrayObj = *GHEAP_NEW(penv->GetHeap()) GASArrayObject(GetASEnvironment());
                arrayObj->Resize(16);
                for(int i=0;i<16;i++)
                    arrayObj->SetElement(i, GASValue(pm3D->GetAsFloatArray()[i]));

                pval->SetAsObject(arrayObj);
                return true;
            }

        case M_perspfov:
            {
                GeomDataType geomData;
                pval->SetNumber(GetPerspectiveFOV());
                return true;
            }
        case M_z:
            {
                GeomDataType geomData;
                pval->SetNumber(GetGeomData(geomData).Z);
                return true;
            }

        case M_zscale:
            {
                GeomDataType geomData;
                pval->SetNumber(GetGeomData(geomData).ZScale);
                return true;
            }

        case M_xrotation:
            {
                GeomDataType geomData;
                pval->SetNumber(GetGeomData(geomData).XRotation);
                return true;
            }

        case M_yrotation:
            {
                GeomDataType geomData;
                pval->SetNumber(GetGeomData(geomData).YRotation);
                return true;
            }
#endif
        default:
            break;
        }
    }

    // Handle the __proto__ property here, since we are going to 
    // zero out it temporarily (see comments below).
    if ((penv && name == penv->GetBuiltin(GASBuiltin___proto__)) ||
        (psc  && name == psc->GetBuiltin(GASBuiltin___proto__)))
    {
        GASObject* proto = Get__proto__();
        pval->SetAsObject(proto);
        return true;
    }
    // The sprite's variables have higher priority than display list, BUT
    // the display list has HIGHER priority than prototype's properties.
    // That is why we would need to do a trick: set __proto__ of the ASMovieClipObj
    // to NULL in order to exclude the prototype and then put it back.
    if (ASMovieClipObj)
    {
        // HACK: zero the __proto__ of ASMovieClipObj to let DisplayList to be
        // checked before the prototype. TODO: redesign.
        GPtr<GASObject> savedProto = ASMovieClipObj->Exchange__proto__(NULL);
        GASSERT(savedProto == Get__proto__());
        bool rv = false;
        if (penv && ASMovieClipObj->GetMember(penv, name, pval))    
        {
            rv = true;
        }
        else if (psc && ASMovieClipObj->GetMemberRaw(psc, name, pval))    
        {
            rv = true;
        }
        // restore the __proto__
        ASMovieClipObj->Exchange__proto__(savedProto);
        if (rv)
            return true;
    }

    // Not a built-in property.  Check items on our display list.
    GFxASCharacter* pch = DisplayList.GetCharacterByName(ASEnvironment.GetSC(), name);
    if (pch)
    {
        // Found object.
        pval->SetAsCharacter(pch);
        return true;
    }
    else 
    {
        // Now we can search in the __proto__
        GASObject* proto = Get__proto__();
        if (proto)
        {
            // ASMovieClipObj is not created yet; use __proto__ instead
            if (penv && proto->GetMember(penv, name, pval))    
            {
                return true;
            }
            else if (psc && proto->GetMemberRaw(psc, name, pval))    
            {
                return true;
            }
        }
    }

    if (name.GetLength() > 0 && name[0] == '_')
    {
        GASEnvironment::GetVarParams params(name, pval);
        Bool3W rv = ASEnvironment.CheckGlobalAndLevels(params);
        if (rv.IsDefined())
            return rv.IsTrue();
    }
    return false;
}

// Find the GFxASCharacter which is one degree removed from us,
// given the relative pathname.
//
// If the pathname is "..", then return our parent.
// If the pathname is ".", then return ourself.  If
// the pathname is "_level0" or "_root", then return
// the root pMovie.
//
// Otherwise, the name should refer to one our our
// named characters, so we return it.
//
// NOTE: In ActionScript 2.0, top level Names (like
// "_root" and "_level0") are CASE SENSITIVE.
// GFxCharacter names in a display list are CASE
// SENSITIVE. Member names are CASE INSENSITIVE.  Gah.
//
// In ActionScript 1.0, everything seems to be CASE
// INSENSITIVE.
GFxASCharacter* GFxSprite::GetRelativeTarget(const GASString& name, bool first_call)
{
    bool           caseSensitive = IsCaseSensitive();
    GASEnvironment &e = ASEnvironment;

    if (name.IsBuiltin())
    {    
        // Special branches for case-sensitivity for efficiency.
        if (caseSensitive)
        {
            if (e.GetBuiltin(GASBuiltin_dot_) == name || // "."
                e.GetBuiltin(GASBuiltin_this) == name)
            {
                return this;
            }
            else if (e.GetBuiltin(GASBuiltin_dotdot_) == name || // ".."
                e.GetBuiltin(GASBuiltin__parent) == name)
            {
                return GetParent();
            }
            else if (e.GetBuiltin(GASBuiltin__root) == name)
            {
                return GetASRootMovie();
            }
        }
        else
        {   // Case-insensitive version.
            name.ResolveLowercase();
            if (e.GetBuiltin(GASBuiltin_dot_) == name ||
                e.GetBuiltin(GASBuiltin_this).CompareBuiltIn_CaseInsensitive_Unchecked(name))
            {
                return this;
            }
            else if (e.GetBuiltin(GASBuiltin_dotdot_) == name || // ".."
                e.GetBuiltin(GASBuiltin__parent).CompareBuiltIn_CaseInsensitive_Unchecked(name))
            {
                return GetParent();
            }
            else if (e.GetBuiltin(GASBuiltin__root).CompareBuiltIn_CaseInsensitive_Unchecked(name))
            {
                return GetASRootMovie();
            }
        }
    }

    if (name[0] == '_' && first_call)
    {
        // Parse level index.
        const char* ptail = 0;
        SInt        levelIndex = pRoot->ParseLevelName(name.ToCStr(), &ptail,
            caseSensitive);
        if ((levelIndex != -1) && !*ptail)
            return pRoot->GetLevelMovie(levelIndex);
    }

    // See if we have a match on the display list.
    return DisplayList.GetCharacterByName(e.GetSC(), name);
}



// Execute the actions for the specified frame.  The
// FrameSpec could be an integer or a string.
void    GFxSprite::CallFrameActions(UInt frameNumber)
{
    if (frameNumber == UInt(~0) || frameNumber >= GetLoadingFrame())
    {
        // No dice.
        LogError("Error: %s, CallFrame('%d') - unknown frame\n", GetCharacterHandle()->GetNamePath().ToCStr(), frameNumber);
        return;
    }

    // These actions will be executed immediately. Such out-of-order execution
    // only happens due to a CallFrame instruction. We implement this by
    // simply assigning a different session id to actions being queued during
    // the execution.

    UInt aqOldSession, aqCurSession = pRoot->ActionQueue.StartNewSession(&aqOldSession);

    // Execute the actions.
    const Frame playlist = pDef->GetPlaylist(frameNumber);
    for (UInt i = 0; i < playlist.GetTagCount(); i++)
    {
        GASExecuteTag*  e = playlist.GetTag(i);
        if (e->IsActionTag())
        {
            e->Execute(this);
        }
    }
    pRoot->ActionQueue.RestoreSession(aqOldSession);

    // Execute any new actions triggered by the tag.    
    pRoot->DoActionsForSession(aqCurSession);

    // If some other actions were queued during the DoAction (like gotoAnd<>)- execute them 
    // as usual, at the next Advance. (AB)
}



// Handle a button event.
bool    GFxSprite::OnButtonEvent(const GFxEventId& id)
{
    if (!IsEnabledFlagSet())
        return false;
    if (IsLevelsMovie()) // level movies should not handle button events
        return false;

    // Handle it ourself?
    bool            handled = OnEvent(id);
    GFxASCharacter* pparent;

    if (!handled && ((pparent=GetParent()) != 0))
    {
        // We couldn't handle it ourself, so
        // pass it up to our parent.
        return pparent->OnButtonEvent(id);
    }
    return false;
}

void    GFxSprite::DetachChild(GFxASCharacter* pchild) 
{ 
    DisplayList.RemoveCharacter(pchild);
}

// Special event handler; ensures that unload is called on child items in timely manner.
void    GFxSprite::OnEventUnload()
{
    SetUnloading();
#ifndef GFC_NO_SOUND
    if (pActiveSounds)
        delete pActiveSounds;
    pActiveSounds = NULL;
#endif

    if (pHitAreaHandle)
        SetHitArea(0);

    SetDirtyFlag();

    // Cause all children to be unloaded.
    DisplayList.Clear();
    GFxASCharacter::OnEventUnload();

    // now, wipe out the character, remove the name, release all local variables 
    // from environment...
    // This is necessary, if there are still pointers on this character somewhere.
    // For example, if the AS code was not terminated (see GASEnv::NeedTermination).
    // Actually, GFx does not release all local variables, and it doesn't wipe out
    // all members at the moment. TODO(?).

    //!AB: actually, we can't reset name here, because if another sprite with the same name
    // will appear after on then it may reuse the name handle and the name should be correct.
    //if (pNameHandle)
    //    pNameHandle->ResetName(ASEnvironment.GetSC());
    Set__proto__(ASEnvironment.GetSC(), NULL);
}

bool    GFxSprite::OnUnloading()
{
    RemoveFromPlaylist(pRoot);

    int haIndex = GetHitAreaIndex(); // Returns -1 if not found
    if (haIndex > -1)
        pRoot->SpritesWithHitArea.RemoveAt(haIndex);

    SetDirtyFlag();

    // Cause all children to be unloaded.
    bool mayRemove = DisplayList.UnloadAll();
    // check, if we can unload the sprite instantly or if we need just
    // to mark it as "unloading".
    if (mayRemove && HasEventHandler(GFxEventId::Event_Unload))
        mayRemove = false;
    if (!mayRemove)
    {
        // we can't instantly remove the character, because either it has one of the
        // children with Unload event or itself has the Unload handler.

        // Thus, we need to mark it as "unloading" and queue up the Unload event
        // (no matter, does this character have the unload handler or not; we would
        // need to re-mark it as "unloaded" after the event is executed, see
        // ExecuteEvent).

        // Here is a special case: if movie is loaded by attachMovie and its 
        // onLoad is not called yet, we need to simulate onLoad call before onUnload.
        // Otherwise, we will see only onUnload, since onLoad will be attempted to execute 
        // AFTER onLoad. See test_onload_onunload_attachmovie.swf 
        if (IsJustLoaded() && !IsAcceptAnimMovesFlagSet())
        {
            GFxMovieRoot::ActionEntry* pe = GetMovieRoot()->InsertEmptyAction();
            if (pe) pe->SetAction(this, GFxEventId::Event_Load);
        }
        GFxMovieRoot::ActionEntry* pe = GetMovieRoot()->InsertEmptyAction();
        if (pe) pe->SetAction(this, GFxEventId::Event_Unload);
    }
    return mayRemove;
}

bool    GFxSprite::OnKeyEvent(const GFxEventId& id, int* pkeyMask)
{ 
    bool rv = false;
    if (id.Id == GFxEventId::Event_KeyDown)
    {
        // run onKeyDown first.
        rv = OnEvent(id);

        // also simmulate on(keyPress)
        // covert Event_KeyDown to Event_KeyPress
        GASSERT(pkeyMask != 0);

        // check if keyPress already was handled then do not handle it again
        if (!((*pkeyMask) & GFxCharacter::KeyMask_KeyPress))
        {
            int kc = id.ConvertToButtonKeyCode();
            if (kc && (rv = OnEvent(GFxEventId(GFxEventId::Event_KeyPress, short(kc), 0)))!=0)
            {
                *pkeyMask |= GFxCharacter::KeyMask_KeyPress;
            }
        }

        GFxMovieRoot* proot = GetMovieRoot();
        if (proot->IsKeyboardFocused(this, id.KeyboardIndex) && (id.KeyCode == GFxKey::Return || id.KeyCode == GFxKey::Space))
        {
            if (IsFocusRectEnabled() || proot->IsAlwaysEnableKeyboardPress())
            {
                // if focused and enter - simulate on(press)/onPress and on(release)/onRelease
                OnEvent (GFxEventId(GFxEventId::Event_Press, GFxKey::Return, 0, 0, id.KeyboardIndex));
                OnEvent (GFxEventId(GFxEventId::Event_Release, GFxKey::Return, 0, 0, id.KeyboardIndex));
            }
        }
        return rv;
    }
    else
        return OnEvent(id); 
}

// Dispatch event Handler(s), if any.
bool    GFxSprite::OnEvent(const GFxEventId& id)
{
    /*  
    if(GetName().ToCStr() && GetName().ToCStr()[0])
    {
    // IF unload executed, clear pparent
    if (id.Id == GFxEventId::Event_Unload)                
    GFC_DEBUG_MESSAGE1(1, "Event_Unload event - %s", GetName().ToCStr());
    if (id.Id == GFxEventId::Event_Load)              
    GFC_DEBUG_MESSAGE1(1, "Event_Load event - %s", GetName().ToCStr());
    } */

    // Check if the sprite has special frames, such as _up, _down, _over.
    // Go to these frames if event is Release, Press or RollOver.
    GFxSpriteDef* sp;
    if ((sp = GetSpriteDef()) != NULL)
    {
        if (sp->HasSpecialFrames())
        {
            switch(id.Id)
            {
            case GFxEventId::Event_DragOut:
            case GFxEventId::Event_Release:
            case GFxEventId::Event_RollOver:
                if (sp->HasFrame_over())
                    GotoLabeledFrame("_over");
                break;
            case GFxEventId::Event_Press:
                if (sp->HasFrame_down())
                    GotoLabeledFrame("_down");
                break;
            case GFxEventId::Event_RollOut:
            case GFxEventId::Event_ReleaseOutside:
                if (sp->HasFrame_up())
                    GotoLabeledFrame("_up");
                break;
            default:;
            }
        }
    }

    do {

        // First, check for built-in event onClipEvent() handler.   
        if (HasClipEventHandler(id))
        {
            // Dispatch.
            continue;
        }

        // Check for member function.   
        // In ActionScript 2.0, event method names are CASE SENSITIVE.
        // In ActionScript 1.0, event method names are CASE INSENSITIVE.
        GASStringContext *psc = GetASEnvironment()->GetSC();
        GASString         methodName(id.GetFunctionName(psc));
        if (!methodName.IsEmpty())
        {
            // Event methods can only be in MovieClipObject or Environment.
            // It's important to not call GFx::GetMemberRaw here to save on overhead 
            // of checking display list and member constants, which will cost
            // us a lot when this is called in Advance.
            bool hasMethod = false;
            GASValue    method;

            // Resolving of event handlers should NOT involve __resolve and properties handles. Thus,
            // we need to use GetMemberRaw instead of GetMember. (AB)
            GASObject* asObj = (ASMovieClipObj) ? ASMovieClipObj : Get__proto__();
            if (asObj && asObj->GetMemberRaw(ASEnvironment.GetSC(), methodName, &method))    
                hasMethod = true;
            //!AB, now we put named functions as a member of target, thus they should be found by the
            // statement above.

            if (id.Id == GFxEventId::Event_KeyDown || id.Id == GFxEventId::Event_KeyUp) 
            {
                // onKeyDown/onKeyUp are available only in Flash 6 and later
                // (don't mess with onClipEvent (keyDown/keyUp)!)
                if (ASEnvironment.GetVersion() >= 6)
                {
                    // also, onKeyDown/onKeyUp should be invoked only if focus
                    // is enabled and set to this movieclip
                    if (!GetMovieRoot()->IsKeyboardFocused(this, id.KeyboardIndex))
                        hasMethod = false;
                }
                else
                {
                    // Flash 5 and below doesn't have onKeyDown/Up function handlers.
                    hasMethod = false;
                }
            }

            if (hasMethod)
                continue;
        }   

        return false;
    } while (0);

    // do actual dispatch
    GFxMovieRoot::ActionEntry* pe = GetMovieRoot()->InsertEmptyAction();
    if (pe) pe->SetAction(this, id);
    return true;
}

bool    GFxSprite::ExecuteBuffer(GASActionBuffer* pactionbuffer)
{
    if (!IsUnloaded()) 
    {
        pactionbuffer->Execute(GetASEnvironment());
        return true;
    }
    else
    {
        // This happens due to untimely Event_Load/Event_Unload during seek
        //GFC_DEBUG_WARNING1(1, "Action execute after unload: %s", pSprite->GetName().ToCStr());
    }
    return false;
}

// Execute this even immediately (called for processing queued event actions).
bool    GFxSprite::ExecuteEvent(const GFxEventId& id)
{
    if (IsUnloaded())
        return false; // do not execute events for unloaded-ready-to-die characters

    // Keep GASEnvironment alive during any method calls!
    GPtr<GFxSprite> thisPtr(this);

    if (id == GFxEventId::Event_Load)
    {
        SetJustLoaded(false);
        if (!IsAcceptAnimMovesFlagSet() && !IsOnLoadReqd())
            return false;
    }

    bool rv = GFxASCharacter::ExecuteEvent (id);
    // Handler done, mark us as unloaded.
    if (id == GFxEventId::Event_Unload)
    {
        // The character continues its existence after onUnload is invoked;
        // It is removed by the next Advance.
        SetUnloaded(true);

#ifndef GFC_USE_OLD_ADVANCE
        // Add to unloaded list
        GASSERT(!IsOptAdvancedListFlagSet());
        pNextUnloaded           = pRoot->pUnloadListHead;
        pRoot->pUnloadListHead  = this;
        this->AddRef(); // need to addref it since it might be not held by anyone else
#endif
    }
    return rv;
}

bool  GFxSprite::HasEventHandler(const GFxEventId& id) const
{
    if (HasClipEventHandler(id))
        return true;

    // Check for member function.   
    // In ActionScript 2.0, event method names are CASE SENSITIVE.
    // In ActionScript 1.0, event method names are CASE INSENSITIVE.
    GASStringContext *psc = GetASEnvironment()->GetSC();
    GASString         methodName(id.GetFunctionName(psc));
    if (!methodName.IsEmpty())
    {
        // Event methods can only be in MovieClipObject or Environment.
        // It's important to not call GFx::GetMemberRaw here to save on overhead 
        // of checking display list and member constants, which will cost
        // us a lot when this is called in Advance.
        GASValue    method;

        // Resolving of event handlers should NOT involve __resolve and properties handles. Thus,
        // we need to use GetMemberRaw instead of GetMember. (AB)
        GASObject* asObj = const_cast<GASObject*> ((ASMovieClipObj) ? ASMovieClipObj : Get__proto__());
        if (asObj && asObj->GetMemberRaw(ASEnvironment.GetSC(), methodName, &method))    
            return true;            

    }
    return false;
}


// Do the events That (appear to) happen as the GFxASCharacter
// loads. The OnLoad event is generated first. Then,
//  "constructor" frame1 tags and actions are Executed (even
// before Advance() is called). 
void    GFxSprite::OnEventLoad()
{   
    // check if the sprite is timeline animated or was created by attachMovie.
    // In the latter case we need to execute "onLoad" in frame priority to match up
    // with potentially following "onUnload" (if it gets unloaded right after attachMovie).
    if (HasClipEventHandler(GFxEventId::Event_Load))
    {
        pRoot->InsertEmptyAction(GFxActionPriority::AP_Frame)->SetAction(this, GFxEventId::Event_Load);
        SetJustLoaded(true);
    }
    else
    {
        pRoot->InsertEmptyAction(GFxActionPriority::AP_Load)->SetAction(this, GFxEventId::Event_Load);
        SetJustLoaded(true);
    }

    ExecuteFrameTags(0);

    // Check if this sprite is a HitArea, and set HitAreaHolder  
    UPInt spriteArraySize =  pRoot->SpritesWithHitArea.GetSize(); 
    for (UInt i = 0; i < spriteArraySize; i++)
        if (this == pRoot->SpritesWithHitArea[i]->GetHitArea())
            pRoot->SpritesWithHitArea[i]->SetHitArea(this); // Reset hitArea for efficient resolving

}

// Do the events that happen when there is XML data waiting
// on the XML socket connection.
void    GFxSprite::OnEventXmlsocketOnxml()
{
    GFC_DEBUG_WARNING(1, "OnEventXmlsocketOnxml: unimplemented");  
    OnEvent(GFxEventId::Event_SockXML);
}

// Do the events That (appear to) happen on a specified interval.
void    GFxSprite::OnEventIntervalTimer()
{
    //GFC_DEBUG_WARNING(1, "OnEventIntervalTimer: unimplemented");   
    //OnEvent(GFxEventId::TIMER);
}

// Do the events that happen as a MovieClip (swf 7 only) loads.
void    GFxSprite::OnEventLoadProgress()
{   
    GFC_DEBUG_WARNING(1, "OnEventLoadProgress: unimplemented");
    OnEvent(GFxEventId::Event_LoadProgress);
}

bool GFxSprite::Invoke(const char* methodName, GASValue* presult, UInt numArgs)
{
    // Keep GASEnvironment alive during any method calls!
    GPtr<GFxSprite> thisPtr(this);

    return GAS_Invoke(methodName, presult, this, &ASEnvironment, numArgs, ASEnvironment.GetTopIndex());
}

bool GFxSprite::InvokeArgs(const char* methodName, GASValue* presult, const char* methodArgFmt, va_list args)
{
    // Keep GASEnvironment alive during any method calls!
    GPtr<GFxSprite> thisPtr(this);

    return GAS_InvokeParsed(methodName, presult, this, &ASEnvironment, methodArgFmt, args);
}

/*const char* GFxSprite::InvokeArgs(const char* methodName, const char* methodArgFmt, 
const void* const* methodArgs, int numArgs)
{
// Keep GASEnvironment alive during any method calls!
GPtr<GFxSprite> thisPtr(this);

return GAS_InvokeParsed(&ASEnvironment, this, methodName, methodArgFmt, methodArgs, numArgs);
}*/


bool GFxSprite::ActsAsButton() const    
{ 
    if (IsLevelsMovie() || !IsEnabledFlagSet())
        return false; // _levelN should NOT act as a button no matter what.
    const GASMovieClipObject* asObj = (ASMovieClipObj) ? ASMovieClipObj.GetPtr() :
        static_cast<const GASMovieClipObject*>(Get__proto__());
    if (asObj) 
        return asObj->ActsAsButton(); 
    return false;
}

void GFxSprite::SetHasButtonHandlers(bool has)
{
    GASMovieClipObject* asObj = GetMovieClipObject();
    if (asObj) 
        asObj->SetHasButtonHandlers(has);  
}

bool GFxSprite::PointTestLocal(const GPointF &pt, UInt8 hitTestMask) const 
{ 
    if (IsHitTestDisableFlagSet())
        return false;

    if (!DoesScale9GridExist() 
#ifndef GFC_NO_3D
        && !Has3D() 
#endif
        && !GetBounds(GRenderer::Matrix()).Contains(pt))
        return false;

    if ((hitTestMask & HitTest_IgnoreInvisible) && !GetVisible())
        return false;

    SPInt i, n = (SPInt)DisplayList.GetCharacterCount();

    GFxSprite* pmask = GetMask();  
    if (pmask)
    {
        if (pmask->IsUsedAsMask() && !pmask->IsUnloaded())
        {
            GRenderer::Matrix matrix;
            matrix.SetInverse(pmask->GetWorldMatrix());
            matrix.Prepend(GetWorldMatrix());
            GPointF p = matrix.Transform(pt);

            if (!pmask->PointTestLocal(p, hitTestMask))
                return false;
        }
    }

    GArray<UByte> hitTest;
    CalcDisplayListHitTestMaskArray(&hitTest, pt, hitTestMask & HitTest_TestShape);

    GRenderer::Matrix m;
    GPointF           p = pt;

    // Go backwards, to check higher objects first.
    for (i = n - 1; i >= 0; i--)
    {
        GFxCharacter* pch = DisplayList.GetCharacter(i);

        if ((hitTestMask & HitTest_IgnoreInvisible) && !pch->GetVisible())
            continue;

        if (hitTest.GetSize() && (!hitTest[i] || pch->GetClipDepth()>0))
            continue;
#ifndef GFC_NO_3D
        if (pch->Is3D(true))
        {
            const GMatrix3D     *pPersp = pch->GetPerspective3D();
            const GMatrix3D     *pView = pch->GetView3D();
            if (pPersp)
                GetMovieRoot()->ScreenToWorld.SetPerspective(*pPersp);
            if (pView)
                GetMovieRoot()->ScreenToWorld.SetView(*pView);
            GetMovieRoot()->ScreenToWorld.SetWorld(pch->GetWorldMatrix3D());
            GetMovieRoot()->ScreenToWorld.GetWorldPoint(&p);
        }
        else
#endif
        {
            m = pch->GetMatrix();
            p = m.TransformByInverse(pt);   
        }

        if (pch->PointTestLocal(p, hitTestMask))
            return true;
    }   

    if (pDrawingAPI)
    {
        if (pDrawingAPI->DefPointTestLocal(pt, hitTestMask & HitTest_TestShape, this))
            return true;
    }

    return false;
}

void    GFxSprite::VisitMembers(GASStringContext *psc, 
                                MemberVisitor *pvisitor, 
                                UInt visitFlags, 
                                const GASObjectInterface*) const
{
    if (visitFlags & VisitMember_ChildClips)
        DisplayList.VisitMembers(pvisitor, visitFlags); 
    GFxASCharacter::VisitMembers(psc, pvisitor, visitFlags, this);
}


// Delete a member field, if not read-only. Return true if deleted.
bool    GFxSprite::DeleteMember(GASStringContext *psc, const GASString& name)
{
    if (GFxASCharacter::DeleteMember(psc, name))
        return true;
    return false; 
}

GASMovieClipObject* GFxSprite::GetMovieClipObject()          
{ 
    if (!ASMovieClipObj)
        ASMovieClipObj = *GHEAP_NEW(pRoot->GetMovieHeap()) GASMovieClipObject(GetGC(), this);
    return ASMovieClipObj;
}

GASObject* GFxSprite::GetASObject ()          
{ 
    return GetMovieClipObject(); 
}

GASObject* GFxSprite::GetASObject () const
{ 
    return const_cast<GFxSprite*>(this)->GetMovieClipObject(); 
}

bool    GFxSprite::IsTabable() const
{
    if (!GetVisible()) return false;
    if (!IsTabEnabledFlagDefined())
    {
        GASObject* pproto = Get__proto__();
        if (pproto)
        {
            // check prototype for tabEnabled
            GASValue val;
            if (pproto->GetMemberRaw(ASEnvironment.GetSC(), ASEnvironment.CreateConstString("tabEnabled"), &val))
            {
                if (!val.IsUndefined())
                    return val.ToBool(&ASEnvironment);
            }
        }
        return (ActsAsButton() || GetTabIndex() > 0);
    }
    else
        return IsTabEnabledFlagTrue();
}

bool     GFxSprite::IsFocusEnabled() const
{
    if (!FocusEnabled.IsDefined())
    {
        GASObject* pproto = Get__proto__();
        if (pproto)
        {
            // check prototype for focusEnabled
            GASValue val;
            if (pproto->GetMemberRaw(ASEnvironment.GetSC(), ASEnvironment.CreateConstString("focusEnabled"), &val))
            {
                if (!val.IsUndefined())
                    return val.ToBool(&ASEnvironment);
            }
        }
        return (ActsAsButton());
    }
    else if (FocusEnabled.IsFalse())
        return (ActsAsButton());
    else
        return FocusEnabled.IsTrue();
}

void    GFxSprite::FillTabableArray(FillTabableParams* params)
{
    GASSERT(params);

    //     if (!(GetFocusGroupMask() & (1 << params->ControllerIdx)))
    //         return;

    UPInt n = DisplayList.GetCharacterCount();
    if (n == 0)
        return;

    if (!TabChildren.IsDefined())
    {
        if (!params->TabChildrenInProto.IsDefined())
        {
            GASObject* pproto = Get__proto__();
            if (pproto)
            {
                // check prototype for tabChildren
                GASValue val;
                if (pproto->GetMemberRaw(ASEnvironment.GetSC(), ASEnvironment.CreateConstString("tabChildren"), &val))
                {
                    if (!val.IsUndefined())
                        params->TabChildrenInProto = val.ToBool(&ASEnvironment);
                }
            }
        }
    }
    if (TabChildren.IsFalse() || params->TabChildrenInProto.IsFalse()) return;

    GFxCharacter*   ch;

    for (UPInt i = 0; i < n; i++)
    {
        ch = DisplayList.GetCharacter(i);
        if (ch != NULL && ch->IsCharASCharacter() && ch->GetVisible())
        {
            GPtr<GFxASCharacter> asch = ch->CharToASCharacter_Unsafe();
            if (asch->IsTabIndexed() && !params->TabIndexed)
            {
                // the first char with tabIndex: release the current array and
                // start to fill it again according to tabIndex.
                params->Array->Clear();
                params->TabIndexed = true;
            }
            // Append for now; if tabIndexed - later it will be sorted by tabIndex
            // Note, we might add focusEnabled characters as well; we are doing this only for
            // 'moveFocus' extension: for regular focus handling these characters will
            // be ignored, if IsTabable returns false.
            if ((asch->IsTabable() || (params->InclFocusEnabled && asch->IsFocusEnabled())) && 
                (!params->TabIndexed || asch->IsTabIndexed()))
                params->Array->PushBack(asch);
            asch->FillTabableArray(params);
        }
    }
}

void GFxSprite::OnGettingKeyboardFocus(UInt)
{
    if (ActsAsButton() && !GetMovieRoot()->IsDisableFocusRolloverEvent()) 
        OnButtonEvent(GFxEventId::Event_RollOver);
}

// invoked when focused item is about to lose keyboard focus input (mouse moved, for example)
bool GFxSprite::OnLosingKeyboardFocus(GFxASCharacter*, UInt controllerIdx, GFxFocusMovedType) 
{
    if (ActsAsButton() && GetMovieRoot()->IsFocusRectShown(controllerIdx) && 
        !GetMovieRoot()->IsDisableFocusRolloverEvent())
    {
        OnButtonEvent(GFxEventId::Event_RollOut);
    }
    return true;
}

void GFxSprite::OnInsertionAsLevel(int level)
{
    if (level == 0)
        SetFocusRectFlag(); // _focusrect in _root is true by default
    else if (level > 0)
    {
        // looks like levels above 0 inherits _focusrect from _level0. Probably
        // another properties are also inherited, need investigate.
        GFxASCharacter* _level0 = GetLevelMovie(0);
        if (_level0)
        {
            SetFocusRectFlag(_level0->IsFocusRectEnabled());
        }
    }
    AddToPlayList(pRoot);
    ModifyOptimizedPlayListLocal<GFxSprite>(pRoot);
    FocusGroupMask = 0;
    --FocusGroupMask; // Set to 0xffff
}

UInt GFxSprite::GetCursorType() const
{
    if (ActsAsButton())
    {
        const GASEnvironment* penv = GetASEnvironment();
        GASValue val;

        if (const_cast<GFxSprite*>(this)->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_useHandCursor), &val))
        {
            if (val.IsUndefined() || !val.ToBool(penv))
                return GFxASCharacter::GetCursorType();
        }
        return GFxMouseCursorEvent::HAND;
    }
    return GFxASCharacter::GetCursorType();
}

void GFxSprite::SetMask(GFxSprite* pmaskSprite) 
{ 
    GFxSprite* poldMask = GetMask();
    if (poldMask)
    {
        // remove this movie clip from the mask holder for the old mask
        poldMask->SetMaskOwner(NULL);
    }

    // the sprite being masked cannot be a mask for another sprite
    GFxSprite* poldOwner = GetMaskOwner();
    if (poldOwner)
    {
        poldOwner->SetMask(NULL);
    }

    if (pMaskCharacter && !IsUsedAsMask())
        pMaskCharacter->Release();
    pMaskCharacter = pmaskSprite;
    SetUsedAsMask(false);
    // avoiding load-hit-store
    if (pmaskSprite)
        pmaskSprite->AddRef();

    if (pmaskSprite) 
    {
        // setMask method override layers' masking (by zeroing ClipDepth).
        // (Do we need to do same for the mask sprite too? !AB)
        SetClipDepth(0); 

        GFxSprite* poldOwner = pmaskSprite->GetMaskOwner();
        if (poldOwner)
        {
            poldOwner->SetMask(NULL);
        }

        pmaskSprite->SetMaskOwner(this);
    }
    SetDirtyFlag();
}

GFxSprite* GFxSprite::GetMask() const
{
    if (pMaskCharacter && !IsUsedAsMask())
    {
        GASSERT(pMaskCharacter->IsUsedAsMask() && pMaskCharacter->pMaskOwner == this);
        return pMaskCharacter;
    }
    return NULL;
}

void GFxSprite::SetMaskOwner(GFxSprite* pmaskOwner)
{
    if (GetMask()) 
        SetMask(NULL);

    SetUsedAsMask((pmaskOwner != NULL)?true:false);
    pMaskOwner = pmaskOwner;
}

GFxSprite* GFxSprite::GetMaskOwner() const
{
    if (pMaskOwner && IsUsedAsMask())
    {
        GASSERT(!pMaskOwner->IsUsedAsMask() && pMaskOwner->pMaskCharacter == this);
        return pMaskOwner;
    }
    return NULL;
}


void GFxSprite::SetScale9Grid(const GFxScale9Grid* gr)
{
    bool propagate = (gr != 0) != (pScale9Grid != 0);
    if (gr == 0)
    {
        delete pScale9Grid;
        pScale9Grid = 0;
        SetScale9GridExists(false);
    }
    else
    {
        if (pScale9Grid == 0) 
            pScale9Grid = GHEAP_NEW(pRoot->GetMovieHeap()) GFxScale9Grid;
        *pScale9Grid = *gr;
        SetScale9GridExists(true);
    }
    if (propagate)
        PropagateScale9GridExists();
    SetDirtyFlag();
}

void GFxSprite::PropagateScale9GridExists()
{
    bool actualGrid = GetScale9Grid() != 0;
    // Stop cleaning up scale9Grid if actual one exists in the node
    if (!DoesScale9GridExist() && actualGrid)
        return; 

    for (UPInt i = 0, n = DisplayList.GetCharacterCount(); i < n; ++i)
    {
        GFxCharacter* ch = DisplayList.GetCharacter(i);
        ch->SetScale9GridExists(DoesScale9GridExist() || actualGrid);
        ch->PropagateScale9GridExists();
    }   
}

void GFxSprite::PropagateNoAdvanceGlobalFlag()
{
    bool actualValue = IsNoAdvanceGlobalFlagSet();
    for (UPInt i = 0, n = DisplayList.GetCharacterCount(); i < n; ++i)
    {
        GFxASCharacter* ch = DisplayList.GetCharacter(i)->CharToASCharacter();
        if (ch)
        {
            ch->SetNoAdvanceGlobalFlag(IsNoAdvanceGlobalFlagSet() || actualValue);
            ch->PropagateNoAdvanceGlobalFlag();
            ch->ModifyOptimizedPlayList(pRoot);
        }
    }   
}

void GFxSprite::PropagateNoAdvanceLocalFlag()
{
    bool actualValue = IsNoAdvanceLocalFlagSet();
    for (UPInt i = 0, n = DisplayList.GetCharacterCount(); i < n; ++i)
    {
        GFxASCharacter* ch = DisplayList.GetCharacter(i)->CharToASCharacter();
        if (ch)
        {
            ch->SetNoAdvanceLocalFlag(IsNoAdvanceLocalFlagSet() || actualValue);
            ch->PropagateNoAdvanceLocalFlag();
            ch->ModifyOptimizedPlayList(pRoot);
        }
    }   
}

void GFxSprite::PropagateFocusGroupMask(UInt mask)
{
    for (UPInt i = 0, n = DisplayList.GetCharacterCount(); i < n; ++i)
    {
        GFxASCharacter* ch = DisplayList.GetCharacter(i)->CharToASCharacter();
        if (ch)
        {
            ch->FocusGroupMask = (UInt16)mask;
            ch->PropagateFocusGroupMask(mask);
        }
    }   
}

#ifndef GFC_NO_3D
void GFxSprite::UpdateViewAndPerspective() 
{
    GFxCharacter::UpdateViewAndPerspective();

    for (UPInt i = 0, n = DisplayList.GetCharacterCount(); i < n; ++i)
    {
        GFxCharacter* ch = DisplayList.GetCharacter(i);
        if (ch)
        {
            ch->UpdateViewAndPerspective();
        }
    }   
}

bool GFxSprite::Has3D() const
{
    if (GFxCharacter::Has3D())
        return true;

    for (UPInt i = 0, n = DisplayList.GetCharacterCount(); i < n; ++i)
    {
        GFxCharacter* ch = DisplayList.GetCharacter(i);
        if (ch && ch->Has3D())
            return true;
    }
    return false;
}
#endif

void GFxSprite::SetVisible(bool visible)            
{ 
    SetVisibleFlag(visible); 
#ifndef GFC_USE_OLD_ADVANCE
    bool noAdvGlob = !visible && pRoot->IsNoInvisibleAdvanceFlagSet();
    if (noAdvGlob != IsNoAdvanceGlobalFlagSet())
    {
        SetNoAdvanceGlobalFlag(noAdvGlob);
        ModifyOptimizedPlayListLocal<GFxSprite>(pRoot);
        GFxASCharacter* pparent = GetParent();
        if (pparent && !pparent->IsNoAdvanceGlobalFlagSet())
            PropagateNoAdvanceGlobalFlag();
    }
#else
    SetNoAdvanceGlobalFlag(!visible && pRoot->IsNoInvisibleAdvanceFlagSet());
#endif
    SetDirtyFlag(); 
}


// GFx_SpriteGetTarget can return NULL in the case when
// sprite-related method (such as 'attachMovie', etc) is being
// called for MovieClip instance without attached sprite, for 
// example:
// var m = new MovieClip;
// m.attachMovie(...);
// Here GFx_SpriteGetTarget wiil return NULL.
static GFxSprite* GFx_SpriteGetTarget(const GASFnCall& fn)
{
    GFxSprite* psprite = NULL;
    if (fn.ThisPtr == NULL)
    {
        psprite = (GFxSprite*) fn.Env->GetTarget();
    }
    else
    {
        if (fn.ThisPtr->IsSprite())
            psprite = (GFxSprite*) fn.ThisPtr;
    }
    //GASSERT(psprite);
    return psprite;
}

static GFxASCharacter* GFx_CharacterGetTarget(const GASFnCall& fn)
{
    GFxASCharacter* psprite = NULL;
    if (fn.ThisPtr == NULL)
    {
        psprite = (GFxASCharacter*) fn.Env->GetTarget();
    }
    else
    {
        if (fn.ThisPtr->IsASCharacter())
            psprite = (GFxASCharacter*) fn.ThisPtr;
    }
    //GASSERT(psprite);
    return psprite;
}

//
// *** Sprite Built-In ActionScript methods
//

static void GFx_SpritePlay(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;
    psprite->SetPlayState(GFxMovie::Playing);
}

static void GFx_SpriteStop(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;
    psprite->SetPlayState(GFxMovie::Stopped);
}

static void GFx_SpriteGotoAndPlay(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs < 1)
    {
        psprite->LogScriptError("Error: %s, GFx_SpriteGotoAndPlay needs one arg\n", psprite->GetCharacterHandle()->GetNamePath().ToCStr());
        return;
    }

    const GASValue& arg = fn.Arg(0);
    UInt            targetFrame = GFC_MAX_UINT;
    if (arg.GetType() == GASValue::STRING)
    {   // Frame label or string frame number
        GASString sa0(arg.ToString(fn.Env));
        if (!psprite->GetLabeledFrame(sa0.ToCStr(), &targetFrame))
            return;
    }
    else
    {   // Convert to 0-based
        targetFrame = arg.ToUInt32(fn.Env) - 1;
    }

    psprite->GotoFrame(targetFrame);
    psprite->SetPlayState(GFxMovie::Playing);
}

static void GFx_SpriteGotoAndStop(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs < 1)
    {
        psprite->LogScriptError("Error: %s, GFx_SpriteGotoAndPlay needs one arg\n", psprite->GetCharacterHandle()->GetNamePath().ToCStr());
        return;
    }

    const GASValue& arg = fn.Arg(0);
    UInt            targetFrame = GFC_MAX_UINT;
    if (arg.GetType() == GASValue::STRING)
    {   // Frame label or string frame number
        GASString sa0(arg.ToString(fn.Env));
        if (!psprite->GetLabeledFrame(sa0.ToCStr(), &targetFrame))
            return;
    }
    else
    {   // Convert to 0-based
        targetFrame = arg.ToUInt32(fn.Env) - 1;
    }

    psprite->GotoFrame(targetFrame);
    psprite->SetPlayState(GFxMovie::Stopped);
}

static void GFx_SpriteNextFrame(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    int availableFrameCount = psprite->GetLoadingFrame();
    int currentFrame = psprite->GetCurrentFrame();
    if (currentFrame < availableFrameCount)
        psprite->GotoFrame(currentFrame + 1);      
    psprite->SetPlayState(GFxMovie::Stopped);
}

static void GFx_SpritePrevFrame(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    int currentFrame = psprite->GetCurrentFrame();
    if (currentFrame > 0)
        psprite->GotoFrame(currentFrame - 1);
    psprite->SetPlayState(GFxMovie::Stopped);
}


static void GFx_SpriteGetBytesLoaded(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    // Value of getBytesLoaded must always match the frames loaded,
    // because many Flash files rely on it. In particular, if this value
    // is equal to getTotalBytes, they assume that all frames must be
    // available. If that does not hold, many pre-loader algorithms
    // will end up in infinite AS loops.

    fn.Result->SetInt(psprite->GetBytesLoaded());
}

static void GFx_SpriteGetBytesTotal(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;
    fn.Result->SetInt(psprite->GetResourceMovieDef()->GetFileBytes());
}


// Duplicate / remove clip.
static void GFx_SpriteDuplicateMovieClip(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL || fn.NArgs < 2) return;

    GPtr<GFxASCharacter> newCh = psprite->CloneDisplayObject(
        fn.Arg(0).ToString(fn.Env), ((SInt)fn.Arg(1).ToNumber(fn.Env)) + 16384,
        (fn.NArgs == 3) ? fn.Arg(2).ToObjectInterface(fn.Env) : 0);

    //!AB: duplicateMovieClip in Flash 6 and above should return newly created clip
    if (psprite->GetVersion() >= 6)
    {
        fn.Result->SetAsCharacter(newCh.GetPtr());
    }
}

static void GFx_SpriteAttachMovie(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL || fn.NArgs < 3) return;

    // Get resource id, looked up locally based on  
    GASString           sa0(fn.Arg(0).ToString(fn.Env));

    GFxResourceBindData resBindData;
    if (!psprite->GetMovieRoot()->FindExportedResource(psprite->GetResourceMovieDef(), &resBindData, sa0.ToCStr()))
    {
        psprite->LogScriptWarning("Error: %s.attachMovie() failed - export name \"%s\" is not found.\n",
            psprite->GetCharacterHandle()->GetNamePath().ToCStr(), sa0.ToCStr());
        return;
    }

    GASSERT(resBindData.pResource.GetPtr() != 0); // MA TBD: Could this be true?     
    if (!(resBindData.pResource->GetResourceType() & GFxResource::RT_CharacterDef_Bit))
    {
        psprite->LogScriptWarning("Error: %s.attachMovie() failed - \"%s\" is not a movieclip.\n",
            psprite->GetCharacterHandle()->GetNamePath().ToCStr(), sa0.ToCStr());
        return;
    }

    GFxCharacterCreateInfo ccinfo;
    ccinfo.pCharDef     = (GFxCharacterDef*) resBindData.pResource.GetPtr();
    ccinfo.pBindDefImpl = resBindData.pBinding->GetOwnerDefImpl();

    // Create a new object and add it.
    GFxCharPosInfo pos( ccinfo.pCharDef->GetId(), ((SInt)fn.Arg(2).ToNumber(fn.Env)) + 16384,
        1, GRenderer::Cxform::Identity, 1, GRenderer::Matrix::Identity);
    // Bounds check depth.
    if ((pos.Depth < 0) || (pos.Depth > (2130690045 + 16384)))
    {
        psprite->LogScriptWarning("Error: %s.attachMovie(\"%s\") failed - depth (%d) must be >= 0\n",
            psprite->GetCharacterHandle()->GetNamePath().ToCStr(), sa0.ToCStr(), pos.Depth);
        return;
    }

    // We pass pchDef to make sure that symbols from nested imports use
    // the right definition scope, which might be different from that of psprite.
    GPtr<GFxCharacter> newCh = psprite->AddDisplayObject(
        pos, fn.Arg(1).ToString(fn.Env), 0,
        (fn.NArgs == 4) ? fn.Arg(3).ToObjectInterface(fn.Env) : 0, GFC_MAX_UINT, 
        GFxDisplayList::Flags_ReplaceIfDepthIsOccupied, &ccinfo);
    if (newCh)
    {
        newCh->SetAcceptAnimMoves(false);

        //!AB: attachMovie in Flash 6 and above should return newly created clip
        if (psprite->GetVersion() >= 6)
        {
            GFxASCharacter* pspriteCh = newCh->CharToASCharacter(); 
            GASSERT (pspriteCh != 0);
            fn.Result->SetAsCharacter(pspriteCh);
        }
    }
}

#if !defined(GFC_NO_SOUND) && !defined(GFC_NO_VIDEO)

static void GFx_SpriteAttachAudio(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (!psprite)
        return;

    if (fn.NArgs < 1)
    {
        fn.Env->LogScriptError("Error: %s.attachAudio() needs one Argument\n",
            psprite->GetCharacterHandle()->GetNamePath().ToCStr());
        return;
    }
    GASObject* o = fn.Arg(0).ToObject(fn.Env);
    if (o)
    {
        if (o->GetObjectType() == GASObject::Object_NetStream)
        {
            GFxVideoBase* pvideoState = fn.Env->GetMovieRoot()->GetVideo();
            if (pvideoState)
                pvideoState->AttachAudio(o, psprite);
        }
        /*
        else if (o->GetObjectType() == GASObject::Object_Microphone)
        {
        }
        */
    }
}
#endif // GFC_NO_VIDEO && GFX_NO_SOUND 

static void GFx_SpriteAttachBitmap(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
#ifndef GFC_NO_FXPLAYER_AS_BITMAPDATA
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL || fn.NArgs < 2) 
        return;
    if (psprite->GetVersion() < 8) // supported only in Flash 8
        return;

    // Arg(0) - BitmapData
    // Arg(1) - depth
    // Arg(2) - String, pixelSnapping, optional (auto, always, never)
    // Arg(3) - Boolean, smoothing, optional

    GPtr<GASObject> pobj = fn.Arg(0).ToObject(fn.Env);
    if (!pobj || pobj->GetObjectType() != GASObject::Object_BitmapData)
    {
        psprite->LogScriptWarning("Error: %s.attachBitmap() failed - the argument is not a BitmapData.\n",
            psprite->GetCharacterHandle()->GetNamePath().ToCStr());
        return;
    }

    GASBitmapData*    pbmpData  = static_cast<GASBitmapData*>(pobj.GetPtr());
    GFxImageResource* pimageRes = pbmpData->GetImage();
    if (!pimageRes)
    {
        psprite->LogScriptWarning("Error: %s.attachBitmap() failed - no image set in BitmapData.\n",
            psprite->GetName().ToCStr());
        return;
    }

    // Create position info
    GFxCharPosInfo pos = GFxCharPosInfo(GFxResourceId(GFxCharacterDef::CharId_ImageMovieDef_ShapeDef),
        ((SInt)fn.Arg(1).ToNumber(fn.Env)) + 16384,
        1, GRenderer::Cxform::Identity, 1, GRenderer::Matrix::Identity);
    // Bounds check depth.
    if ((pos.Depth < 0) || (pos.Depth > (2130690045 + 16384)))
    {
        psprite->LogScriptWarning("Error: %s.attachBitmap() failed - depth (%d) must be >= 0\n",
            psprite->GetCharacterHandle()->GetNamePath().ToCStr(), pos.Depth);
        return;
    }


    bool                    smoothing = (fn.NArgs >= 4) ? fn.Arg(3).ToBool(fn.Env) : false;
    GFxMovieRoot*           pmroot = fn.Env->GetMovieRoot();
    GPtr<GFxMovieDefImpl>   pimageMovieDef = *pmroot->CreateImageMovieDef(pimageRes, smoothing, "");

    if (pimageMovieDef)
    {           
        // Empty sprite based on new Def. New Def gives correct Version, URL, and symbol lookup behavior.
        GPtr<GFxSprite> pchildSprite = *GHEAP_NEW(pmroot->GetMovieHeap())
            GFxSprite(pimageMovieDef->GetDataDef(), pimageMovieDef, pmroot, psprite,
            GFxResourceId(GFxCharacterDef::CharId_EmptyMovieClip), true);

        if (pchildSprite)
        {    

            // Verified: Flash assigns depth -16383 to image shapes (depth value = 1 in our list).
            SInt                        depth = 1;
            GFxCharPosInfo              locpos = GFxCharPosInfo(GFxResourceId(GFxCharacterDef::CharId_ImageMovieDef_ShapeDef),
                depth, 0, GRenderer::Cxform(), 1, GRenderer::Matrix());

            GASString                   emptyName(fn.Env->GetBuiltin(GASBuiltin_empty_));        

            pchildSprite->AddToPlayList(pmroot);
            pchildSprite->ModifyOptimizedPlayList(pmroot);

            pchildSprite->AddDisplayObject(locpos, emptyName, NULL, NULL, 1);
            psprite->ReplaceDisplayObject(pos, pchildSprite, emptyName);

            psprite->SetAcceptAnimMoves(false);
        }
    }
#else
    GFC_DEBUG_WARNING(1, "Error: attachBitmap is failed since GFC_NO_FXPLAYER_AS_BITMAPDATA is defined.");
#endif
}

static void GFx_SpriteCreateEmptyMovieClip(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL || fn.NArgs < 2) return;

    // Create a new object and add it.
    GFxCharPosInfo pos = GFxCharPosInfo( GFxResourceId(GFxCharacterDef::CharId_EmptyMovieClip),
        ((SInt)fn.Arg(1).ToNumber(fn.Env)) + 16384,
        1, GRenderer::Cxform::Identity, 1, GRenderer::Matrix::Identity);
    // Bounds check depth.
    if ((pos.Depth < 0) || (pos.Depth > (2130690045 + 16384)))
        return;

    GPtr<GFxCharacter> newCh = psprite->AddDisplayObject(
        pos, fn.Arg(0).ToString(fn.Env), NULL, NULL, GFC_MAX_UINT, 
        GFxDisplayList::Flags_ReplaceIfDepthIsOccupied);  
    if (newCh)
    {
        newCh->SetAcceptAnimMoves(false);

        // Return newly created clip.
        GFxASCharacter* pspriteCh = newCh->CharToASCharacter(); 
        GASSERT (pspriteCh != 0);
        fn.Result->SetAsCharacter(pspriteCh);
    }
}

static void GFx_SpriteCreateTextField(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL || fn.NArgs < 6) return; //?AB, could it work with less than 6 params specified?

    // Create a new object and add it.    
    GFxCharPosInfo pos = GFxCharPosInfo( GFxResourceId(GFxCharacterDef::CharId_EmptyTextField),
        ((SInt)fn.Arg(1).ToNumber(fn.Env)) + 16384,
        1, GRenderer::Cxform::Identity, 1, GRenderer::Matrix::Identity);
    // Bounds check depth.
    if ((pos.Depth < 0) || (pos.Depth > (2130690045 + 16384)))
        return;

    GPtr<GFxCharacter> newCh = psprite->AddDisplayObject(
        pos, fn.Arg(0).ToString(fn.Env), NULL, NULL, GFC_MAX_UINT, 
        GFxDisplayList::Flags_ReplaceIfDepthIsOccupied);  
    if (newCh)
    {
        newCh->SetAcceptAnimMoves(false);

        // Return newly created clip.
        GFxASCharacter* ptextCh = newCh->CharToASCharacter(); 
        GASSERT (ptextCh != 0);
        ptextCh->SetStandardMember(GFxSprite::M_x, fn.Arg(2), false);
        ptextCh->SetStandardMember(GFxSprite::M_y, fn.Arg(3), false);
        ptextCh->SetStandardMember(GFxSprite::M_width,  fn.Arg(4), false);
        ptextCh->SetStandardMember(GFxSprite::M_height, fn.Arg(5), false);
        fn.Result->SetAsCharacter(ptextCh);
    }
}


static void GFx_SpriteRemoveMovieClip(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (psprite->GetDepth() < 16384)
    {
        psprite->LogScriptWarning("%s.removeMovieClip() failed - depth must be >= 0\n",
            psprite->GetName().ToCStr());
        return;
    }
    psprite->RemoveDisplayObject();
}

// Implementation of MovieClip.setMask method. 
static void GFx_SpriteSetMask(const GASFnCall& fn)
{
    fn.Result->SetUndefined();

    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs >= 1)
    {
        if (fn.Arg(0).IsNull())
        {
            // remove mask
            psprite->SetMask(NULL);
        }
        else
        {
            GFxASCharacter* pobj = fn.Arg(0).ToASCharacter(fn.Env);
            if (pobj)
            {
                GFxSprite* pmaskSpr = pobj->ToSprite();
                psprite->SetMask(pmaskSpr);
            }
            else
                psprite->SetMask(NULL);
        }
    }
}

static void GFx_SpriteGetBounds(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    // Get target sprite.
    GFxASCharacter*     pobj    = (fn.NArgs > 0) ? fn.Arg(0).ToASCharacter(fn.Env) : psprite;
    GFxSprite*          ptarget = pobj ? pobj->ToSprite() : 0;

    GRectF              b(0);
    GRenderer::Matrix   matrix;

    if (ptarget)
    {
        // Transform first by sprite's matrix, then by inverse of target.
        if (ptarget != psprite) // Optimize identity case.
        {
            matrix.SetInverse(ptarget->GetWorldMatrix());
            matrix.Prepend(psprite->GetWorldMatrix());
        }

        // A "perfect" implementation would be { b = psprite->GetBounds(matrix); },
        // however, Flash always gets the local bounding box before a transform,
        // as follows.
        matrix.EncloseTransform(&b, psprite->GetBounds(GRenderer::Matrix()));
    }

    // Store into result object.    
    GPtr<GASObject>     presult = *GHEAP_NEW(fn.Env->GetHeap()) GASObject(fn.Env);
    GASStringContext*   psc = fn.Env->GetSC();
    presult->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_xMin), TwipsToPixels(Double(b.Left)));
    presult->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_xMax), TwipsToPixels(Double(b.Right)));
    presult->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_yMin), TwipsToPixels(Double(b.Top)));
    presult->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_yMax), TwipsToPixels(Double(b.Bottom)));

    fn.Result->SetAsObject(presult.GetPtr());   
}


//
// MovieClip.getRect()
//
// Returns properties that are the minimum and maximum x and y 
// coordinate values of the movie clip, based on the bounds parameter,
// excluding any strokes on shapes. The values that getRect() returns 
// are the same or smaller than those returned by MovieClip.getBounds(). 
//
static void GFx_SpriteGetRect(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    // Get target sprite.
    GFxASCharacter*     pobj    = (fn.NArgs > 0) ? fn.Arg(0).ToASCharacter(fn.Env) : psprite;
    GFxSprite*          ptarget = pobj ? pobj->ToSprite() : 0;

    GRectF              b(0);
    GRenderer::Matrix   matrix;



    if (ptarget)
    {
        // Transform first by sprite's matrix, then by inverse of target.
        if (ptarget != psprite) // Optimize identity case.
        {
            matrix.SetInverse(ptarget->GetWorldMatrix());
            matrix.Prepend(psprite->GetWorldMatrix());
        }

        // A "perfect" implementation would be { b = psprite->GetBounds(matrix); },
        // however, Flash always gets the local bounding box before a transform,
        // as follows.
        matrix.EncloseTransform(&b, psprite->GetRectBounds(GRenderer::Matrix()));
    }

    // Store into result object.    
    GPtr<GASObject>     presult = *GHEAP_NEW(fn.Env->GetHeap()) GASObject(fn.Env);
    GASStringContext*   psc = fn.Env->GetSC();
    presult->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_xMin), TwipsToPixels(Double(b.Left)));
    presult->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_xMax), TwipsToPixels(Double(b.Right)));
    presult->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_yMin), TwipsToPixels(Double(b.Top)));
    presult->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_yMax), TwipsToPixels(Double(b.Bottom)));

    fn.Result->SetAsObject(presult.GetPtr());     
}


static void GFx_SpriteLocalToGlobal(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL || fn.NArgs < 1) return;

    // Get target object.
    GASStringContext*   psc = fn.Env->GetSC();
    GASObjectInterface* pobj = fn.Arg(0).ToObjectInterface(fn.Env);
    if (!pobj) return;

    // Object must have x & y members, which are numbers; otherwise 
    // the function does nothing. This is the expected behavior.
    GASValue            xval, yval;  
    pobj->GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_x), &xval);
    pobj->GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_y), &yval);
    if (!xval.IsNumber() || !yval.IsNumber())
        return;

    // Compute transformation and set members.
    GPointF pt((Float)GFC_PIXELS_TO_TWIPS(xval.ToNumber(fn.Env)), (Float)GFC_PIXELS_TO_TWIPS(yval.ToNumber(fn.Env)));
    pt = psprite->GetWorldMatrix().Transform(pt);
    pobj->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_x), TwipsToPixels(Double(pt.x)));
    pobj->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_y), TwipsToPixels(Double(pt.y)));
}

static void GFx_SpriteGlobalToLocal(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL || fn.NArgs < 1) return;

    // Get target object.
    GASStringContext*   psc = fn.Env->GetSC();
    GASObjectInterface* pobj = fn.Arg(0).ToObjectInterface(fn.Env);
    if (!pobj) return;

    // Object must have x & y members, which are numbers; otherwise 
    // the function does nothing. This is the expected behavior.
    GASValue xval, yval;
    pobj->GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_x), &xval);
    pobj->GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_y), &yval);
    if (!xval.IsNumber() || !yval.IsNumber())
        return;

    // Compute transformation and set members.
    GPointF pt((Float)GFC_PIXELS_TO_TWIPS(xval.ToNumber(fn.Env)), (Float)GFC_PIXELS_TO_TWIPS(yval.ToNumber(fn.Env)));
    pt = psprite->GetWorldMatrix().TransformByInverse(pt);
    pobj->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_x), TwipsToPixels(Double(pt.x)));
    pobj->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_y), TwipsToPixels(Double(pt.y)));
}


// Hit-test implementation.
static void GFx_SpriteHitTest(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    fn.Result->SetBool(0);

    GRectF spriteLocalBounds = psprite->GetBounds(GRenderer::Matrix());
    if (spriteLocalBounds.IsNull())
        return;

    // Hit-test has either 1, 2 or 3 arguments.
    if (fn.NArgs >= 2)
    {
        UInt8 hitTestMask = 0;
        // x, y, shapeFlag version of hitTest.
        GPointF     pt( (Float)GFC_PIXELS_TO_TWIPS(fn.Arg(0).ToNumber(fn.Env)),
            (Float)GFC_PIXELS_TO_TWIPS(fn.Arg(1).ToNumber(fn.Env)) );
        if (fn.NArgs >= 3) // optional parameter shapeFlag
            hitTestMask |= (fn.Arg(2).ToBool(fn.Env)) ? GFxCharacter::HitTest_TestShape : 0;
        if (fn.NArgs >= 4) // optional parameter shapeFlag
            hitTestMask |= (fn.Arg(3).ToBool(fn.Env)) ? GFxCharacter::HitTest_IgnoreInvisible : 0;

        // check for 3D case
        GFxMovieRoot *proot = psprite->GetMovieRoot();
#ifndef GFC_NO_3D
        if (psprite->Is3D(true) && proot)
        {
            float w = (proot->VisibleFrameRect.Right - proot->VisibleFrameRect.Left);
            float h = (proot->VisibleFrameRect.Bottom - proot->VisibleFrameRect.Top);

            // note pt.x and pt.y already have ViewScaleXY taken into account (but not ViewOffsetXY)
            float nsx = 2.f * ((pt.x - PixelsToTwips(proot->ViewOffsetX)) / w - 0.5f);
            float nsy = 2.f * ((pt.y - PixelsToTwips(proot->ViewOffsetY)) / h - 0.5f);

            proot->ScreenToWorld.SetNormalizedScreenCoords(nsx, nsy);    // -1 to 1
//            proot->GetLog()->LogMessage("nsx=%.2f, nsy=%.2f\n", nsx, nsy);

            const GMatrix3D     *pPersp = psprite->GetPerspective3D(true); 
            const GMatrix3D     *pView = psprite->GetView3D(true);
            if (pPersp)
                proot->ScreenToWorld.SetPerspective(*pPersp);
            if (pView)
                proot->ScreenToWorld.SetView(*pView);
            proot->ScreenToWorld.SetWorld(psprite->GetWorldMatrix3D());
            GPointF pt2;
            proot->ScreenToWorld.GetWorldPoint(&pt2);

            fn.Result->SetBool(psprite->PointTestLocal (pt2, hitTestMask));
            return;
        }
#endif
        GPointF ptSpr = psprite->GetLevelMatrix().TransformByInverse(pt);
        // pt is already in root's coordinates!

        if (psprite->DoesScale9GridExist())
        {
            fn.Result->SetBool(psprite->PointTestLocal (ptSpr, hitTestMask));
        }
        else
        {
            if (spriteLocalBounds.Contains(ptSpr))
            {
                if (!(hitTestMask & GFxCharacter::HitTest_TestShape))
                    fn.Result->SetBool(true);
                else
                    fn.Result->SetBool(psprite->PointTestLocal (ptSpr, hitTestMask));
            }
            else
                fn.Result->SetBool(false);
            }
        }
    else if (fn.NArgs == 1)
    {
        // Target sprite version of hit-test.
        const GASValue&     arg = fn.Arg(0);
        GFxASCharacter*     ptarget = NULL;
        if (arg.IsCharacter())
        {
            ptarget = arg.ToASCharacter(fn.Env);
        }
        else
        {
            // if argument is not a character, then convert it to string
            // and try to resolve as variable (AB)
            GASString varName = arg.ToString(fn.Env);
            GASValue result;
            if (fn.Env->GetVariable(varName, &result))
            {
                ptarget = result.ToASCharacter(fn.Env);
            }
        }
        if (!ptarget)
            return;

        GRectF targetLocalRect  = ptarget->GetBounds(GRenderer::Matrix());
        if (targetLocalRect.IsNull())
            return;
        // Rect rectangles in same coordinate space & check intersection.

        // note - this only checks axis aligned rects, which may not be so good for 3D 
        GRectF spriteWorldRect, targetWorldRect;
#ifndef GFC_NO_3D
        if (psprite->Is3D(true))
            spriteWorldRect = psprite->GetWorldMatrix3D().EncloseTransform(spriteLocalBounds);
        else
#endif
            spriteWorldRect = psprite->GetWorldMatrix().EncloseTransform(spriteLocalBounds);
#ifndef GFC_NO_3D
        if (ptarget->Is3D(true))
            targetWorldRect = ptarget->GetWorldMatrix3D().EncloseTransform(targetLocalRect);
        else
#endif
            targetWorldRect = ptarget->GetWorldMatrix().EncloseTransform(targetLocalRect);

        fn.Result->SetBool(spriteWorldRect.Intersects(targetWorldRect));
    }
}

// Implemented as member function so that it can access Sprite's private methods.
void    GFxSprite::SpriteSwapDepths(const GASFnCall& fn)
{
    GFxASCharacter* pchar = GFx_CharacterGetTarget(fn);
    if (pchar == NULL || fn.NArgs < 1) return;

    GFxSprite* pparent = (GFxSprite*)pchar->GetParent();
    const GASValue& arg     = fn.Arg(0);
    int             depth2  = 0;
    GFxASCharacter* ptarget = 0;
    GFxSprite* psprite = (pchar->IsSprite()) ? (GFxSprite*)pchar : 0;

    // Depth can be a number at which the character is inserted.
    if (arg.IsNumber())
    {
        depth2 = ((int)arg.ToNumber(fn.Env)) + 16384;
        // Verified: Flash will discard smaller values.
        if (depth2 < 0)
            return;
        // Documented depth range is -16384 to 1048575,
        // but Flash does not seem to enforce that upper constraint. Instead
        // manually determined upper constraint is 2130690045.
        if (depth2 > (2130690045 + 16384))
            return;
    }
    else
    {
        // Need to search for target in our scope (environment scope might be different).
        if (psprite)
        {
            GFxASCharacter* poldTarget = fn.Env->GetTarget();
            fn.Env->SetTarget(psprite);
            ptarget = fn.Env->FindTargetByValue(arg);
            fn.Env->SetTarget(poldTarget);
        }
        else
            ptarget = fn.Env->FindTargetByValue(arg); //???

        if (!ptarget || ptarget == pchar)
            return;         
        // Must have same parent.
        if (pparent != ptarget->GetParent())
            return;

        // Can we swap with text fields, etc? Yes.
        depth2 = ptarget->GetDepth();
    }

    // Flash doesn't allow to swap depths for "semi-dead" objects (i.e 
    // depth_in_flash < -16384, depth_in_gfx < -1).
    if (pchar->GetDepth() < 0)
        return;

    pchar->SetAcceptAnimMoves(0);
    // Do swap or depth change.
    if (pparent && pparent->DisplayList.SwapDepths(pchar->GetDepth(), depth2, pparent->GetCurrentFrame()))
    {
        pparent->SetDirtyFlag();
        if (ptarget)
            ptarget->SetAcceptAnimMoves(0);
    }
}

static void GFx_SpriteGetNextHighestDepth(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    // Depths is always at least 0. No upper bound constraint is done
    // (i.e. 2130690046 will be returned in Flash, although invalid).
    SInt depth = G_Max<SInt>(0, psprite->GetLargestDepthInUse() - 16384 + 1);
    fn.Result->SetInt(depth);
}

static void GFx_SpriteGetInstanceAtDepth(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    // Note: this can also report buttons, etc.
    if (fn.NArgs >= 1)
    {
        GFxCharacter* pchar = psprite->GetCharacterAtDepth(((int)fn.Arg(0).ToNumber(fn.Env)) + 16384);
        if (pchar)
            fn.Result->SetAsCharacter(pchar->CharToASCharacter());
    }
}


//
// Returns a TextSnapshot object that contains the text in all the 
// static text fields in the specified movie clip; text in child movie 
// clips is not included. This method always returns a TextSnapshot 
// object.
//
static void GFx_SpriteGetTextSnapshot(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

#ifndef GFC_NO_FXPLAYER_AS_TEXTSNAPSHOT
    GPtr<GASTextSnapshotObject> ptextSnapshot = *GHEAP_NEW(fn.Env->GetHeap()) GASTextSnapshotObject(fn.Env);
    // Collect all static text chars
    ptextSnapshot->Process(psprite);
    // Return a TextSnapshot AS Object
    fn.Result->SetAsObject(ptextSnapshot);
#endif
}


static void GFx_SpriteGetSWFVersion(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    fn.Result->SetInt(psprite->GetVersion());
}

static void GFx_SpriteStartDrag(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    GFxMovieRoot::DragState st;
    bool                    lockCenter = false;

    // If there are arguments, init appropriate values.
    if (fn.NArgs > 0)
    {
        lockCenter = fn.Arg(0).ToBool(fn.Env);
        if (fn.NArgs > 4)
        {
            st.Bound = true;
            GRectF bounds;
            bounds.Left		= GFC_PIXELS_TO_TWIPS((Float) fn.Arg(1).ToNumber(fn.Env));
            bounds.Top		= GFC_PIXELS_TO_TWIPS((Float) fn.Arg(2).ToNumber(fn.Env));
            bounds.Right	= GFC_PIXELS_TO_TWIPS((Float) fn.Arg(3).ToNumber(fn.Env));
            bounds.Bottom	= GFC_PIXELS_TO_TWIPS((Float) fn.Arg(4).ToNumber(fn.Env));
            bounds.Normalize();
            st.BoundLT = bounds.TopLeft();
            st.BoundRB = bounds.BottomRight();
        }
    }

    st.pCharacter = psprite;
    // Init mouse offsets based on LockCenter flag.
    st.InitCenterDelta(lockCenter);

    // Begin dragging.
    GFxMovieRoot* proot = psprite->GetMovieRoot();
    proot->SetDragState(st);  
    psprite->ModifyOptimizedPlayListLocal<GFxSprite>(proot);
}


static void GFx_SpriteStopDrag(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    // Stop dragging.
    GFxMovieRoot* proot = psprite->GetMovieRoot();
    proot->StopDrag();    
    psprite->ModifyOptimizedPlayListLocal<GFxSprite>(proot);
}

static void GFx_SpriteLoadMovie(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs > 0)
    {
        GFxLoadQueueEntry::LoadMethod lm = GFxLoadQueueEntry::LM_None;

        // Decode load method argument.
        if (fn.NArgs > 1)
        {
            GASString str(fn.Arg(1).ToString(fn.Env).ToLower());
            if (str == "get")
                lm = GFxLoadQueueEntry::LM_Get;
            else if (str == "post")
                lm = GFxLoadQueueEntry::LM_Post;
        }

        // Post loadMovie into queue.
        GASString urlStr(fn.Arg(0).ToString(fn.Env));
        psprite->GetMovieRoot()->AddLoadQueueEntry(psprite, urlStr.ToCStr(), lm);
    }
}

static void GFx_SpriteUnloadMovie(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    // Post unloadMovie into queue (empty url == unload).
    psprite->GetMovieRoot()->AddLoadQueueEntry(psprite, "");
}

static void GFx_SpriteLoadVariables(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs > 0)
    {
        GFxLoadQueueEntry::LoadMethod lm = GFxLoadQueueEntry::LM_None;

        // Decode load method argument.
        if (fn.NArgs > 1)
        {
            GASString str(fn.Arg(1).ToString(fn.Env).ToLower());
            if (str == "get")
                lm = GFxLoadQueueEntry::LM_Get;
            else if (str == "post")
                lm = GFxLoadQueueEntry::LM_Post;
        }

        // Post loadMovie into queue.
        GASString urlStr(fn.Arg(0).ToString(fn.Env));
        psprite->GetMovieRoot()->AddVarLoadQueueEntry(psprite, urlStr.ToCStr(), lm);
    }
}

// Drawing API support
//=============================================================================
void GFxSprite::Clear()
{
    if(pDrawingAPI)
    {
        pDrawingAPI->Clear();
    }
    InvalidateHitResult();
    SetDirtyFlag();
}

void GFxSprite::SetNoLine()
{
    if (!pDrawingAPI) 
        pDrawingAPI = *GHEAP_NEW(pRoot->GetMovieHeap()) GFxDrawingContext;

    if (!pDrawingAPI->NoLine())
    {
        AcquirePath(false);
        pDrawingAPI->SetNoLine();
    }
}

void GFxSprite::SetNoFill()
{
    AcquirePath(true);
    pDrawingAPI->SetNoFill();
}

void GFxSprite::SetLineStyle(Float lineWidth, 
                             UInt  rgba, 
                             bool  hinting, 
                             GFxLineStyle::LineStyle scaling, 
                             GFxLineStyle::LineStyle caps,
                             GFxLineStyle::LineStyle joins,
                             Float miterLimit)
{
    if (!pDrawingAPI) 
        pDrawingAPI = *GHEAP_NEW(pRoot->GetMovieHeap()) GFxDrawingContext;

    if((rgba & 0xFF000000) != 0)
    {
        // Check the line width and set it to the possible minimum (hairline) 
        // if negative or zero.
        if (lineWidth <= 0.0f)
            lineWidth  = TwipsToPixels(1.0f);

        if (!pDrawingAPI->SameLineStyle(PixelsToTwips(lineWidth), rgba, hinting, scaling, caps, joins, miterLimit))
        {
            AcquirePath(false);
            pDrawingAPI->SetLineStyle(PixelsToTwips(lineWidth), rgba, hinting, scaling, caps, joins, miterLimit);
        }
    }
    else
    {
        if (!pDrawingAPI->NoLine())
        {
            AcquirePath(false);
            pDrawingAPI->SetNoLine();
        }
    }
}

// The function begins new fill with an "empty" style. 
// It returns the pointer to just created fill style, so the caller
// can set any values. The function is used in Action Script, beginGradientFill
GFxFillStyle* GFxSprite::BeginFill()
{
    AcquirePath(true);
    return pDrawingAPI->SetNewFill();
}

GFxFillStyle* GFxSprite::CreateLineComplexFill()
{
    AcquirePath(true);
    return pDrawingAPI->CreateLineComplexFill();
}

void GFxSprite::BeginFill(UInt rgba)
{
    AcquirePath(true);
    pDrawingAPI->SetFill(rgba);
}


void GFxSprite::BeginBitmapFill(GFxFillType fillType,
                                GFxImageResource* pimageRes,
                                const Matrix& mtx)
{
    AcquirePath(true);
    pDrawingAPI->SetBitmapFill(fillType, pimageRes, mtx);
}

void GFxSprite::EndFill()
{
    AcquirePath(true);
    pDrawingAPI->ResetFill();
}

void GFxSprite::MoveTo(Float x, Float y)
{
    AcquirePath(false);
    pDrawingAPI->MoveTo(PixelsToTwips(x), 
        PixelsToTwips(y));
    InvalidateHitResult();
    //FILE* fd = fopen("coord", "at");
    //fprintf(fd, "mc.moveTo(%.3f, %.3f);\n", x, y);
    //fclose(fd);
}

void GFxSprite::LineTo(Float x, Float y)
{
    if (!pDrawingAPI) 
        pDrawingAPI = *GHEAP_NEW(pRoot->GetHeap()) GFxDrawingContext;
    pDrawingAPI->LineTo(PixelsToTwips(x), 
        PixelsToTwips(y));
    InvalidateHitResult();
    //FILE* fd = fopen("coord", "at");
    //fprintf(fd, "mc.lineTo(%.3f, %.3f);\n", x, y);
    //fclose(fd);
}

void GFxSprite::CurveTo(Float cx, Float cy, Float ax, Float ay)
{
    if (!pDrawingAPI) 
        pDrawingAPI = *GHEAP_NEW(pRoot->GetMovieHeap()) GFxDrawingContext;
    pDrawingAPI->CurveTo(PixelsToTwips(cx), 
        PixelsToTwips(cy),
        PixelsToTwips(ax), 
        PixelsToTwips(ay));
    InvalidateHitResult();
}

bool GFxSprite::AcquirePath(bool newShapeFlag)
{
    if (!pDrawingAPI) 
        pDrawingAPI = *GHEAP_NEW(pRoot->GetMovieHeap()) GFxDrawingContext;
    SetDirtyFlag();
    InvalidateHitResult();
    return pDrawingAPI->AcquirePath(newShapeFlag);
}

void GFxSprite::SetStateChangeFlags(UInt8 flags)
{
    GFxASCharacter::SetStateChangeFlags(flags);
    UPInt size = DisplayList.GetCharacterCount();
    for (UPInt i = 0; i < size; ++i)
    {
        GFxCharacter* ch = DisplayList.GetCharacter(i);
        ch->SetStateChangeFlags(flags);
    }   
}

void GFxSprite::SetHitArea( GFxSprite* phitArea )
{
    GFxSprite* phaSprite = GetHitArea(); 
    if (phaSprite)
        phaSprite->SetHitAreaHolder(0);

    int haIndex = GetHitAreaIndex(); // Returns -1 if not found
    if (phitArea)
    {
        pHitAreaHandle = phitArea->GetCharacterHandle();
        if (haIndex == -1)
            pRoot->SpritesWithHitArea.PushBack(this);
        phitArea->SetHitAreaHolder(this);
    }
    else
    {
        pHitAreaHandle = 0;
        if (haIndex > -1)
            pRoot->SpritesWithHitArea.RemoveAt(haIndex);
    }
}

GFxSprite* GFxSprite::GetHitArea() const
{
    if (pHitAreaHandle)
    {
        GFxASCharacter* pasChararcter = pHitAreaHandle->ResolveCharacter(pRoot);
        if (pasChararcter)
            return pasChararcter->ToSprite();
    }
    return 0;
}

int GFxSprite::GetHitAreaIndex()
{
    UPInt spriteArraySize =  pRoot->SpritesWithHitArea.GetSize(); 
    if (pHitAreaHandle) // Should not be in array, so don't search  
    {
        for (UInt i = 0; i < spriteArraySize; i++)
            if (pRoot->SpritesWithHitArea[i] == this)
                return i;
    }
    return -1;
}

void GFxSprite::SetName(const GASString& name)
{
    GFxASCharacter::SetName(name);
    // need to reset cached character if name is changed.
    DisplayList.ResetCachedCharacter();
}

// Drawing API
//=============================================================================
static void GFx_SpriteClear(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;
    psprite->Clear();
}

static void GFx_SpriteBeginFill(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if(fn.NArgs > 0) // rgb:Number
    {
        UInt rgba = UInt(fn.Arg(0).ToUInt32(fn.Env) | 0xFF000000);
        if(fn.NArgs > 1) // alpha:Number
        {
            // Alpha is clamped to 0 to 100 range.
            Float alpha = ((Float)fn.Arg(1).ToNumber(fn.Env)) * 255.0f / 100.0f; 
            rgba &= 0xFFFFFF;            
            rgba |= UInt(G_Clamp(alpha, 0.0f, 255.0f)) << 24;
        }
        psprite->BeginFill(rgba);
    }
    else
    {
        psprite->SetNoFill();
    }
}

static void GFx_SpriteCreateGradient(const GASFnCall& fn, GFxFillStyle* fillStyle)
{
    // beginGradientFill(fillType:String, 
    //                   colors:Array, 
    //                   alphas:Array, 
    //                   ratios:Array, 
    //                   matrix:Object, 
    //                   [spreadMethod:String], 
    //                   [interpolationMethod:String], 
    //                   [focalPointRatio:Number]) : Void

    GASObject* arg = 0;
    if (fn.NArgs > 0)
    {
        GASString fillTypeStr(fn.Arg(0).ToString(fn.Env));

        if (fn.NArgs > 1 && (arg = fn.Arg(1).ToObject(fn.Env)) != NULL &&
            arg->GetObjectType() == GASObjectInterface::Object_Array)
        {
            const GASArrayObject* colors = (const GASArrayObject*)arg;
            if (fn.NArgs > 2 && (arg = fn.Arg(2).ToObject(fn.Env)) != NULL &&
                arg->GetObjectType() == GASObjectInterface::Object_Array)
            {
                const GASArrayObject* alphas = (const GASArrayObject*)arg;
                if (fn.NArgs > 3 && (arg = fn.Arg(3).ToObject(fn.Env)) != NULL &&
                    arg->GetObjectType() == GASObjectInterface::Object_Array)
                {
                    const GASArrayObject* ratios = (const GASArrayObject*)arg;
                    if (fn.NArgs > 4 && 
                        colors->GetSize() > 0 && 
                        colors->GetSize() == alphas->GetSize() &&
                        colors->GetSize() == ratios->GetSize())
                    {
                        GMatrix2D matrix;
                        GASValue v;
                        GASStringContext *psc = fn.Env->GetSC();

                        arg = fn.Arg(4).ToObject(fn.Env);

#ifndef GFC_NO_FXPLAYER_AS_MATRIX
                        if(arg->GetObjectType() == GASObjectInterface::Object_Matrix)
                        {
                            matrix = ((GASMatrixObject*)arg)->GetMatrix(fn.Env);
                        }
                        else
#endif
                        {
                            if (arg->GetConstMemberRaw(psc, "matrixType",  &v) && 
                                v.ToString(fn.Env) == "box")
                            {
                                Float x = 0;
                                Float y = 0;
                                Float w = 100;
                                Float h = 100;
                                Float r = 0;
                                if(arg->GetConstMemberRaw(psc, "x",  &v)) x = (Float)v.ToNumber(fn.Env);
                                if(arg->GetConstMemberRaw(psc, "y",  &v)) y = (Float)v.ToNumber(fn.Env);
                                if(arg->GetConstMemberRaw(psc, "w",  &v)) w = (Float)v.ToNumber(fn.Env);
                                if(arg->GetConstMemberRaw(psc, "h",  &v)) h = (Float)v.ToNumber(fn.Env);
                                if(arg->GetConstMemberRaw(psc, "r",  &v)) r = (Float)v.ToNumber(fn.Env);

                                x += w / 2;
                                y += h / 2;
                                w *= GASGradientBoxMagicNumber; 
                                h *= GASGradientBoxMagicNumber;

                                matrix.AppendRotation(r);
                                matrix.AppendScaling(w, h);
                                matrix.AppendTranslation(x, y);
                            }
                            else
                            {
                                // The matrix is transposed:
                                // a(0,0)  d(0,1)  g(0,2)
                                // b(1,0)  e(1,1)  h(1,2)
                                // c(2,0)  f(2,1)  i(2,2)
                                // Elements c,f,i are not used
                                if(arg->GetConstMemberRaw(psc, "a",  &v)) matrix.M_[0][0] = (Float)v.ToNumber(fn.Env) * GASGradientBoxMagicNumber;
                                if(arg->GetConstMemberRaw(psc, "d",  &v)) matrix.M_[0][1] = (Float)v.ToNumber(fn.Env) * GASGradientBoxMagicNumber;
                                if(arg->GetConstMemberRaw(psc, "g",  &v)) matrix.M_[0][2] = (Float)v.ToNumber(fn.Env);
                                if(arg->GetConstMemberRaw(psc, "b",  &v)) matrix.M_[1][0] = (Float)v.ToNumber(fn.Env) * GASGradientBoxMagicNumber;
                                if(arg->GetConstMemberRaw(psc, "e",  &v)) matrix.M_[1][1] = (Float)v.ToNumber(fn.Env) * GASGradientBoxMagicNumber;
                                if(arg->GetConstMemberRaw(psc, "h",  &v)) matrix.M_[1][2] = (Float)v.ToNumber(fn.Env);
                            }
                        }

                        int   spreadMethod    = 0; // pad
                        bool  linearRGB       = false;
                        Float focalPointRatio = 0;
                        if (fn.NArgs > 5)
                        {
                            GASString str(fn.Arg(5).ToString(fn.Env));

                            if(str == "reflect") spreadMethod = 1;
                            else if(str == "repeat")  spreadMethod = 2;

                            if (fn.NArgs > 6)
                            {
                                linearRGB = fn.Arg(6).ToString(fn.Env) == "linearRGB";
                                if (fn.NArgs > 7)
                                {
                                    focalPointRatio = (Float)(fn.Arg(7).ToNumber(fn.Env));
                                    if (GASNumberUtil::IsNaN(focalPointRatio))
                                        focalPointRatio = 0;
                                    else if (focalPointRatio < -1) focalPointRatio = -1;
                                    else if (focalPointRatio >  1) focalPointRatio =  1;
                                }
                            }
                        }

                        // Create Gradient itself
                        GFxFillType fillType = GFxFill_LinearGradient;
                        if (fillTypeStr == "radial")
                        {
                            fillType = GFxFill_RadialGradient;
                            if (focalPointRatio != 0) 
                                fillType = GFxFill_FocalPointGradient;
                        }

                        // Gradient data must live in a global heap since it is used as a key in ResourceLib.
                        GPtr<GFxGradientData> pgradientData = 
                            *GNEW GFxGradientData(fillType, (UInt16)colors->GetSize(), linearRGB);

                        if (pgradientData)
                        {
                            pgradientData->SetFocalRatio(focalPointRatio);

                            for (int i = 0; i < colors->GetSize(); i++)
                            {
                                UInt rgba = UInt(colors->GetElementPtr(i)->ToUInt32(fn.Env) | 0xFF000000);

                                // Alpha is clamped to 0 to 100 range.
                                Float alpha = ((Float)alphas->GetElementPtr(i)->ToNumber(fn.Env)) * 255.0f / 100.0f; 
                                rgba &= 0xFFFFFF;            
                                rgba |= UInt(G_Clamp(alpha, 0.0f, 255.0f)) << 24;

                                Float ratio = (Float)ratios->GetElementPtr(i)->ToNumber(fn.Env); 
                                ratio = G_Clamp(ratio, 0.0f, 255.0f);

                                GFxGradientRecord& record = (*pgradientData)[i];
                                record.Ratio = (UByte)ratio;
                                record.Color = rgba;
                            }

                            fillStyle->SetGradientFill(fillType, pgradientData, matrix);
                        }
                    }
                }
            }
        }
    }
}

static void GFx_SpriteBeginGradientFill(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    GFxFillStyle* fillStyle = psprite->BeginFill();
    if (fillStyle)
        GFx_SpriteCreateGradient(fn, fillStyle);
}

static void GFx_SpriteLineGradientStyle(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    GFxFillStyle* fillStyle = psprite->CreateLineComplexFill();
    if (fillStyle)
        GFx_SpriteCreateGradient(fn, fillStyle);
}

static void GFx_SpriteBeginBitmapFill(const GASFnCall& fn)
{
    GUNUSED(fn);
#ifndef GFC_NO_FXPLAYER_AS_BITMAPDATA
    //beginBitmapFill(bmp:BitmapData, 
    //               [matrix:Matrix], 
    //               [repeat:Boolean], 
    //               [smoothing:Boolean]) : Void

    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs > 0) // bmp:BitmapData,
    {
        GPtr<GASObject> pobj = fn.Arg(0).ToObject(fn.Env);
        if (!pobj || pobj->GetObjectType() != GASObject::Object_BitmapData)
            return;

        GASBitmapData*    pbmpData = static_cast<GASBitmapData*>(pobj.GetPtr());
        GFxImageResource* pimageRes = pbmpData->GetImage();
        if (!pimageRes)
            return;

        GMatrix2D matrix;
        bool repeat = true;
        bool smoothing = false;

        if (fn.NArgs > 1) // [matrix:Matrix]
        {
#ifndef GFC_NO_FXPLAYER_AS_MATRIX
            GASObject* arg = fn.Arg(1).ToObject(fn.Env);
            if (arg && arg->GetObjectType() == GASObjectInterface::Object_Matrix)
            {
                matrix = ((GASMatrixObject*)arg)->GetMatrix(fn.Env);
            }
#endif

            if (fn.NArgs > 2) // [repeat:Boolean]
            {
                repeat = fn.Arg(2).ToBool(fn.Env);
                if (fn.NArgs > 3) // [smoothing:Boolean]
                {
                    smoothing = fn.Arg(3).ToBool(fn.Env);
                }
            }
        }

        GFxFillType fillType;

        if (smoothing)
        {
            if (repeat) fillType = GFxFill_TiledSmoothImage;
            else        fillType = GFxFill_ClippedSmoothImage;
        }
        else
        {
            if (repeat) fillType = GFxFill_TiledImage;
            else        fillType = GFxFill_ClippedImage;
        }        

        psprite->BeginBitmapFill(fillType, pimageRes, matrix);
    }
#else
    GFC_DEBUG_WARNING(1, "Error: beginBitmapFill is failed since GFC_NO_FXPLAYER_AS_BITMAPDATA is defined.");
#endif
}

static void GFx_SpriteEndFill(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;
    psprite->EndFill();
}

static void GFx_SpriteLineStyle(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs > 0) // thickness:Number
    {
        Float lineWidth = (Float)(fn.Arg(0).ToNumber(fn.Env));
        UInt  rgba = 0xFF000000;
        bool  hinting = false;
        GFxLineStyle::LineStyle scaling = GFxLineStyle::LineScaling_Normal;
        GFxLineStyle::LineStyle caps    = GFxLineStyle::LineCap_Round;
        GFxLineStyle::LineStyle joins   = GFxLineStyle::LineJoin_Round;
        Float miterLimit = 3.0f;

        if(fn.NArgs > 1) // rgb:Number
        {
            rgba = fn.Arg(1).ToUInt32(fn.Env) | 0xFF000000;

            if(fn.NArgs > 2) // alpha:Number
            {
                Float alpha = ((Float)fn.Arg(2).ToNumber(fn.Env)) * 255.0f / 100.0f; 

                rgba &= 0xFFFFFF;                
                rgba |= UInt(G_Clamp(alpha, 0.0f, 255.0f)) << 24;

                if(fn.NArgs > 3) // pixelHinting:Boolean
                {
                    hinting = fn.Arg(3).ToBool(fn.Env);

                    if(fn.NArgs > 4) // noScale:String
                    {
                        GASString str(fn.Arg(4).ToString(fn.Env));
                        if(str == "none")       scaling = GFxLineStyle::LineScaling_None;
                        else if(str == "vertical")   scaling = GFxLineStyle::LineScaling_Vertical;
                        else if(str == "horizontal") scaling = GFxLineStyle::LineScaling_Horizontal;

                        if(fn.NArgs > 5) // capsStyle:String
                        {
                            str = fn.Arg(5).ToString(fn.Env);
                            if(str == "none")   caps = GFxLineStyle::LineCap_None;
                            else if(str == "square") caps = GFxLineStyle::LineCap_Square;

                            if(fn.NArgs > 6) // jointStyle:String
                            {
                                str = fn.Arg(6).ToString(fn.Env);
                                if(str == "miter") joins = GFxLineStyle::LineJoin_Miter;
                                else if(str == "bevel") joins = GFxLineStyle::LineJoin_Bevel;

                                if(fn.NArgs > 7) // miterLimit:Number
                                {
                                    miterLimit = (Float)(fn.Arg(7).ToNumber(fn.Env));
                                    if(miterLimit < 1.0f)   miterLimit = 1.0f;
                                    if(miterLimit > 255.0f) miterLimit = 255.0f;
                                }
                            }
                        }
                    }
                }
            }
        }
        psprite->SetLineStyle(lineWidth, rgba, hinting, scaling, caps, joins, miterLimit);
    }
    else
    {
        psprite->SetNoLine();
    }
}

static void GFx_SpriteMoveTo(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs >= 2) 
    {
        Float x = (Float)(fn.Arg(0).ToNumber(fn.Env));
        Float y = (Float)(fn.Arg(1).ToNumber(fn.Env));
        psprite->MoveTo(x, y);
    }
}

static void GFx_SpriteLineTo(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs >= 2) 
    {
        Float x = (Float)(fn.Arg(0).ToNumber(fn.Env));
        Float y = (Float)(fn.Arg(1).ToNumber(fn.Env));
        psprite->LineTo(x, y);
    }
}

static void GFx_SpriteCurveTo(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs >= 4) 
    {
        Float cx = (Float)(fn.Arg(0).ToNumber(fn.Env));
        Float cy = (Float)(fn.Arg(1).ToNumber(fn.Env));
        Float ax = (Float)(fn.Arg(2).ToNumber(fn.Env));
        Float ay = (Float)(fn.Arg(3).ToNumber(fn.Env));
        psprite->CurveTo(cx, cy, ax, ay);
    }
}

//////////////////////////////////////////
void GASMovieClipObject::commonInit ()
{
    ButtonEventMask         = 0;
    HasButtonHandlers       = 0;
}

UInt16   GASMovieClipObject::GetButtonEventNameMask(GASStringContext* psc, const GASString& name)
{
    if (psc->GetBuiltin(GASBuiltin_onPress) == name)
        return Mask_onPress;
    else if (psc->GetBuiltin(GASBuiltin_onRelease) == name)
        return Mask_onRelease;
    else if (psc->GetBuiltin(GASBuiltin_onReleaseOutside) == name)
        return Mask_onReleaseOutside;
    else if (psc->GetBuiltin(GASBuiltin_onRollOver) == name)
        return Mask_onRollOver;
    else if (psc->GetBuiltin(GASBuiltin_onRollOut) == name)
        return Mask_onRollOut;
    else if (psc->GetBuiltin(GASBuiltin_onDragOver) == name)
        return Mask_onDragOver;
    else if (psc->GetBuiltin(GASBuiltin_onDragOut) == name)
        return Mask_onDragOut;

    else if (psc->GetBuiltin(GASBuiltin_onPressAux) == name)
        return Mask_onPressAux;
    else if (psc->GetBuiltin(GASBuiltin_onReleaseAux) == name)
        return Mask_onReleaseAux;
    else if (psc->GetBuiltin(GASBuiltin_onReleaseOutsideAux) == name)
        return Mask_onReleaseOutsideAux;
    else if (psc->GetBuiltin(GASBuiltin_onDragOverAux) == name)
        return Mask_onDragOverAux;
    else if (psc->GetBuiltin(GASBuiltin_onDragOutAux) == name)
        return Mask_onDragOutAux;

    return 0;
}

// Updates DynButtonHandlerCount if name is begin added through SetMember or
// deleted through DeleteMember.
void    GASMovieClipObject::TrackMemberButtonHandler(GASStringContext* psc, const GASString& name, bool deleteFlag)
{
    // If we are one of button handlers, do the check.
    if (name.GetSize() > 2 && name[0] == 'o' && name[1] == 'n')
    {
        GASValue val;
        //!if (ASEnvironment.GetMember(name, &val))
        if (GetMemberRaw(psc, name, &val))
        {
            // Set and member already exists? No tracking increment.
            if (!deleteFlag)
                return;
        }
        else
        {
            // Delete and does not exist? No decrement.
            if (deleteFlag)
                return;
        }
        // Any button handler enables button behavior.
        UInt16 bemask;
        if ((bemask = GetButtonEventNameMask(psc, name)) != 0)
        {
            if (deleteFlag)
                ButtonEventMask &= ~bemask;
            else
                ButtonEventMask |= bemask;
        }
    }
}

void GASMovieClipObject::Set__proto__(GASStringContext *psc, GASObject* protoObj) 
{   
    GASObject::Set__proto__(psc, protoObj);
    // proto can be NULL, when we reset the proto in OnEventUnload
    //GASSERT(protoObj);
    if (protoObj && protoObj->GetObjectType() != Object_MovieClipObject)
    {
        //AB: if proto is being set to the sprite, and this proto is not 
        // an instance of MovieClipObject then we need to rescan it for button's
        // handlers to react on events correctly. This usually happen if 
        // component is extending MovieClip as a class and defines
        // buttons' handler as methods.
        struct MemberVisitor : GASObjectInterface::MemberVisitor
        {
            GPtr<GASMovieClipObject> obj;
            GASStringContext*        pStringContext;

            MemberVisitor(GASStringContext* psc, GASMovieClipObject* _obj)
                : obj(_obj), pStringContext(psc) { }

            virtual void Visit(const GASString& name, const GASValue& , UByte flags)
            {
                GUNUSED(flags);
                if (IsButtonEventName(pStringContext, name))
                {
                    obj->SetHasButtonHandlers(true);
                }
            }
        } visitor (psc, this);
        pProto->VisitMembers(psc, &visitor, 
            GASObjectInterface::VisitMember_Prototype |
            GASObjectInterface::VisitMember_DontEnum | 
            GASObjectInterface::VisitMember_NamesOnly);
    }
}

bool GASMovieClipObject::ActsAsButton() const   
{ 
    if ((ButtonEventMask != 0) || HasButtonHandlers)
        return true;
    for(GASObject* pproto = pProto; pproto != 0; pproto = pproto->Get__proto__())
    {
        if (pProto->GetObjectType() == Object_MovieClipObject) 
        {
            // MA: No need to AddRef to 'pproto'.
            GASMovieClipProto* proto = static_cast<GASMovieClipProto*>(pproto);
            return proto->ActsAsButton();
        }
    }
    return false;
}

void    GASMovieClipObject::SetMemberCommon(GASStringContext *psc, 
                                            const GASString& name, 
                                            const GASValue& val)
{
    // _levelN can't function as a button
    // MovieClip.prototype has no sprite (GetSprite() returns 0) but
    // it should track button handlers.
    GPtr<GFxSprite> spr = GetSprite();
    if (!spr || spr->GetASRootMovie() != spr)
        TrackMemberButtonHandler(psc, name);

    // Intercept the user data properties and cache them in pSprite
    if (spr && name.IsBuiltin())
    {
        if (name == psc->GetBuiltin(GASBuiltin_rendererString))
        {
            spr->SetRendererString(val.ToString(spr->GetASEnvironment()));
        }
        else if (name == psc->GetBuiltin(GASBuiltin_rendererFloat))
        {
            spr->SetRendererFloat((float)val.ToNumber(spr->GetASEnvironment()));
        }
        else if (name == psc->GetBuiltin(GASBuiltin_rendererMatrix))
        {
            GPtr<GASObject> pObj = val.ToObject(spr->GetASEnvironment());
            if (pObj && pObj->GetObjectType() == GASObjectInterface::Object_Array)
            {
                GASArrayObject* pArray = (GASArrayObject*)pObj.GetPtr();
                float f[16];
                UInt size = G_Min(16, pArray->GetSize());
                for (UInt i = 0; i < size; i++)
                    f[i] = (float)pArray->GetElementPtr(i)->ToNumber(spr->GetASEnvironment());
                spr->SetRendererMatrix(f, size);
            }
        }
    }
}

// Set the named member to the value.  Return true if we have
// that member; false otherwise.
// Note, we need to define both - SetMember and SetMemberRaw, since both of them
// might be called independently. So, moved common part to SetMemberCommon.
bool    GASMovieClipObject::SetMemberRaw(GASStringContext *psc, 
                                         const GASString& name, 
                                         const GASValue& val, 
                                         const GASPropFlags& flags)
{
    SetMemberCommon(psc, name, val);
    return GASObject::SetMemberRaw(psc, name, val, flags);
}

bool    GASMovieClipObject::SetMember(GASEnvironment *penv, 
                                      const GASString& name, 
                                      const GASValue& val, 
                                      const GASPropFlags& flags)
{
    SetMemberCommon(penv->GetSC(), name, val);
    return GASObject::SetMember(penv, name, val, flags);
}

// Set *val to the value of the named member and
// return true, if we have the named member.
// Otherwise leave *val alone and return false.
/*bool    GASMovieClipObject::GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
{
return GASObject::GetMember(penv, name, val);
}*/

// Delete a member field, if not read-only. Return true if deleted.
bool    GASMovieClipObject::DeleteMember(GASStringContext *psc, const GASString& name)
{
    TrackMemberButtonHandler(psc, name, 1);
    return GASObject::DeleteMember(psc, name);
}

void    GASMovieClipObject::ForceShutdown ()
{
    // traverse through all members, look for FUNCTION and release local frames.
    // Needed to prevent memory leak until we don't have garbage collection.
#ifdef GFC_NO_GC
    for (MemberHash::Iterator iter = Members.Begin(); iter != Members.End(); ++iter)
    {        
        GASValue& value = iter->Second.Value;
        if (value.IsFunction ())
        {
            value.V.FunctionValue.SetLocalFrame (0);
        }
    }
#endif
}

static const GASNameFunction MovieClipFunctionTable[] = 
{
    { "play",                   &GFx_SpritePlay },
    { "stop",                   &GFx_SpriteStop },
    { "gotoAndStop",            &GFx_SpriteGotoAndStop },
    { "gotoAndPlay",            &GFx_SpriteGotoAndPlay },
    { "nextFrame",              &GFx_SpriteNextFrame },
    { "prevFrame",              &GFx_SpritePrevFrame },
    { "getBytesLoaded",         &GFx_SpriteGetBytesLoaded },
    { "getBytesTotal",          &GFx_SpriteGetBytesTotal },

    { "getBounds",              &GFx_SpriteGetBounds },
    { "getRect",                &GFx_SpriteGetRect },
    { "localToGlobal",          &GFx_SpriteLocalToGlobal },
    { "globalToLocal",          &GFx_SpriteGlobalToLocal },
    { "hitTest",                &GFx_SpriteHitTest },

    { "attachBitmap",           &GFx_SpriteAttachBitmap },
    { "attachMovie",            &GFx_SpriteAttachMovie },
#if !defined(GFC_NO_SOUND) && !defined(GFC_NO_VIDEO)
    { "attachAudio",            &GFx_SpriteAttachAudio },
#endif
    { "duplicateMovieClip",     &GFx_SpriteDuplicateMovieClip },
    { "removeMovieClip",        &GFx_SpriteRemoveMovieClip },
    { "createEmptyMovieClip",   &GFx_SpriteCreateEmptyMovieClip },
    { "createTextField",        &GFx_SpriteCreateTextField },

    { "getDepth",               &GFxASCharacter::CharacterGetDepth },
    { "swapDepths",             &GFxSprite::SpriteSwapDepths },
    { "getNextHighestDepth",    &GFx_SpriteGetNextHighestDepth },
    { "getInstanceAtDepth",     &GFx_SpriteGetInstanceAtDepth },

    { "getTextSnapshot",        &GFx_SpriteGetTextSnapshot },

    { "getSWFVersion",          &GFx_SpriteGetSWFVersion },

    { "startDrag",              &GFx_SpriteStartDrag },
    { "stopDrag",               &GFx_SpriteStopDrag },

    { "setMask",                &GFx_SpriteSetMask },

    { "loadMovie",              &GFx_SpriteLoadMovie },
    { "unloadMovie",            &GFx_SpriteUnloadMovie },

    { "loadVariables",          &GFx_SpriteLoadVariables },

    // Drawing API

    { "clear",                  &GFx_SpriteClear     },
    { "beginFill",              &GFx_SpriteBeginFill },
    { "beginGradientFill",      &GFx_SpriteBeginGradientFill },
    { "beginBitmapFill",        &GFx_SpriteBeginBitmapFill },
    { "lineGradientStyle",      &GFx_SpriteLineGradientStyle },
    { "endFill",                &GFx_SpriteEndFill   },
    { "lineStyle",              &GFx_SpriteLineStyle },
    { "moveTo",                 &GFx_SpriteMoveTo    },
    { "lineTo",                 &GFx_SpriteLineTo    },
    { "curveTo",                &GFx_SpriteCurveTo   },

    { 0, 0 }
};

GASMovieClipProto::GASMovieClipProto(GASStringContext* psc, GASObject* prototype, const GASFunctionRef& constructor) :
GASPrototype<GASMovieClipObject>(psc, prototype, constructor)
{
    InitFunctionMembers(psc, MovieClipFunctionTable);
    SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_useHandCursor), GASValue(true), 
        GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);

    SetConstMemberRaw(psc, "enabled", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete|GASPropFlags::PropFlag_DontEnum);
    SetConstMemberRaw(psc, "scale9Grid", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete|GASPropFlags::PropFlag_DontEnum);
    // can't add tabEnabled, focusEnabled and tabChildren here, since their presence will stop prototype chain lookup, even if
    // it is UNSET or "undefined".
    //SetConstMemberRaw(psc, "focusEnabled", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete|GASPropFlags::PropFlag_DontEnum);
    //SetConstMemberRaw(psc, "tabChildren", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete|GASPropFlags::PropFlag_DontEnum);
    //SetConstMemberRaw(psc, "tabEnabled", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete|GASPropFlags::PropFlag_DontEnum);
    SetConstMemberRaw(psc, "tabIndex", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete|GASPropFlags::PropFlag_DontEnum);
    SetConstMemberRaw(psc, "trackAsMenu", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete|GASPropFlags::PropFlag_DontEnum);
    SetConstMemberRaw(psc, "transform", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete|GASPropFlags::PropFlag_DontEnum);  
}

GASObject* GASMovieClipCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{
    return GHEAP_NEW(penv->GetHeap()) GASMovieClipObject(penv);
}

void GASMovieClipCtorFunction::GlobalCtor(const GASFnCall& fn) 
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_MovieClipObject)
    {
        GASMovieClipObject* pobj = static_cast<GASMovieClipObject*>(fn.ThisPtr);
        fn.Result->SetAsObject(pobj);
    }
    else
        fn.Result->SetUndefined();
}



//////////////////////////////////////////////////////////////////////////
#ifdef GFC_BUILD_DEBUG
void GFxSprite::DumpDisplayList(int indent, const char* title)
{
    char indentStr[900];
    memset(indentStr, 32, indent*3);
    indentStr[indent*3] = 0;

    if (title)
        printf("%s -- %s --\n", indentStr, title);
    else
        printf("\n");

    if (indent == 0)
    {
        printf("%sSprite %s\n", indentStr, GetCharacterHandle()->GetNamePath().ToCStr());
        printf("%sId          = %d\n", indentStr, (int)GetId().GetIdValue());
        printf("%sCreateFrame = %d\n", indentStr, (int)GetCreateFrame());
        printf("%sDepth       = %d\n", indentStr, (int)GetDepth());
        printf("%sVisibility  = %s\n", indentStr, GetVisible() ? "true" : "false");
        indent++;
        memset(indentStr, 32, indent*3);
        indentStr[indent*3] = 0;
    }

    for (UInt i = 0; i<DisplayList.DisplayObjectArray.GetSize(); i++)
    {
        const GFxDisplayList::DisplayEntry& dobj = DisplayList.DisplayObjectArray[i];
        GFxCharacter*       ch   = dobj.GetCharacter();

        GFxASCharacter* pasch = NULL;
        if (ch->IsCharASCharacter())
        {
            pasch = ch->CharToASCharacter_Unsafe();
            if (pasch->IsSprite())
                printf("%sSprite %s\n", indentStr, pasch->GetCharacterHandle()->GetNamePath().ToCStr());
            else
                printf("%sCharacter %s\n", indentStr, pasch->GetCharacterHandle()->GetNamePath().ToCStr());
        }
        else
            printf("%sCharacter (generic)\n", indentStr);
        if (dobj.IsMarkedForRemove())
            printf("%s(marked for remove)\n", indentStr);
        printf("%sId          = %d\n", indentStr, (int)ch->GetId().GetIdValue());
        printf("%sCreateFrame = %d\n", indentStr, (int)ch->GetCreateFrame());
        printf("%sDepth       = %d\n", indentStr, (int)ch->GetDepth());
        printf("%sClipDepth   = %d\n", indentStr, (int)ch->GetClipDepth());
        printf("%sVisibility  = %s\n", indentStr, ch->GetVisible() ? "true" : "false");

        char buff[2560];
        const Cxform& cx = ch->GetCxform();
        if (!cx.IsIdentity())
        {
            cx.Format(buff);
            printf("%sCxform:\n%s\n", indentStr, buff);
        }
        const Matrix& mx = ch->GetMatrix();
        if (mx != Matrix::Identity)
        {
            mx.Format(buff);
            printf("%sMatrix:\n%s\n", indentStr, buff);
        }

        if (pasch && pasch->IsSprite())
        {
            //            printf("\n");
            pasch->ToSprite()->DumpDisplayList(indent+1);
        }
        else 
            printf("\n");
    }
    if (title)
        printf("%s-----------------\n", indentStr);
}
#endif // GFC_BUILD_DEBUG
