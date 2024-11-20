/**********************************************************************

Filename    :   GFxCharacter.cpp
Content     :   Base functionality for characters, which are display
                list objects.
Created     :   May 25, 2006  (moved from GFxPlayerImpl)
Authors     :   Michael Antonov
Notes       :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.


Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxCharacter.h"
#include "GFxPlayerImpl.h"
#include "GFxSprite.h"
#include "AS/GASNumberObject.h"
#include "AS/GASArrayObject.h"
#include "AS/GASBitmapFilter.h"
#include "GMsgFormat.h"
#include "GMatrix3D.h"

// ***** GFxCharacter   

GFxCharacter::GFxCharacter(GFxASCharacter* parent, GFxResourceId id)
    :
    Id(id),
    Depth(-1),
    CreateFrame(0),
    Ratio(0.0f),
    pParent(parent),
#ifndef GFC_NO_3D
    pMatrix3D_1(NULL),
    pPerspectiveMatrix3D(NULL),
    pViewMatrix3D(NULL),       
    PerspFOV(0),
#endif
    ClipDepth(0),
    Flags(0)
{
    GASSERT((parent == NULL && Id == GFxResourceId::InvalidId) ||
        (parent != NULL));
}

GFxCharacter::~GFxCharacter()
{
#ifndef GFC_NO_3D
    if (pMatrix3D_1)
    {
        delete pMatrix3D_1;
        pMatrix3D_1 = NULL;
    }
    if (pPerspectiveMatrix3D)
    {
        delete pPerspectiveMatrix3D;
        pPerspectiveMatrix3D = NULL;
    }
    if (pViewMatrix3D)
    {
        delete pViewMatrix3D;
        pViewMatrix3D = NULL;
    }
#endif
}

void GFxCharacter::CreateMatrix3D(GMatrix3DNewable** pmat)
{
#ifndef GFC_NO_3D
    if (pmat == NULL)
        pmat = &pMatrix3D_1;
    if (*pmat == NULL)
    {
        *pmat = GHEAP_NEW(GetMovieRoot()->GetMovieHeap())  GMatrix3DNewable;
    }
#else
    GUNUSED(pmat);
#endif
}

#ifndef GFC_NO_3D
Float GFxCharacter::GetPerspectiveFOV(bool checkAncestors) const           
{ 
    if (PerspFOV != 0 || checkAncestors == false)
        return PerspFOV; 

    if (pParent == NULL)
        return GetMovieRoot() ? GetMovieRoot()->PerspFOV : PerspFOV;

    return pParent->GetPerspectiveFOV(checkAncestors);
}

void GFxCharacter::SetPerspectiveFOV(Float fov)        
{ 
    GFxMovieRoot *pRoot = GetMovieRoot();
    if (fov != PerspFOV && pRoot && pRoot->GetRenderer())
    {
        PerspFOV = fov; 
        GMatrix3D matView;
        GMatrix3D matPersp;
        pRoot->GetRenderer()->MakeViewAndPersp3D(pRoot->VisibleFrameRect, matView, matPersp, PerspFOV);
        SetPerspective3D(matPersp);
        SetView3D(matView);
    }
}

// recompute view and perspective if necessary (usually when visible frame rect changes)
void GFxCharacter::UpdateViewAndPerspective()
{
    GFxMovieRoot *pRoot = GetMovieRoot();
    if (pRoot && pViewMatrix3D && pPerspectiveMatrix3D && pRoot->GetRenderer())
        pRoot->GetRenderer()->MakeViewAndPersp3D(pRoot->VisibleFrameRect, *pViewMatrix3D, *pPerspectiveMatrix3D, PerspFOV);
}
#endif

void GFxCharacter::SetDirtyFlag()
{
    GetMovieRoot()->SetDirtyFlag();
}

// Character transform implementation.
void    GFxCharacter::GetWorldMatrix(Matrix *pmat) const
{           
    if (pParent)
    {
        pParent->GetWorldMatrix(pmat);
        pmat->Prepend(GetMatrix());
    }
    else
    {
        *pmat = GetMatrix();
    }
}

// Character transform implementation.
void    GFxCharacter::GetLevelMatrix(Matrix *pmat) const
{           
    if (pParent)
    {
        pParent->GetLevelMatrix(pmat);
        pmat->Prepend(GetMatrix());
    }
    else
    {
        *pmat = Matrix();
    }
}

// Character color transform implementation.
void    GFxCharacter::GetWorldCxform(Cxform *pcxform) const
{
    if (pParent)
    {
        pParent->GetWorldCxform(pcxform);
        pcxform->Concatenate(GetCxform());
    }
    else
    {
        *pcxform = GetCxform();     
    }
}

#ifndef GFC_NO_3D


bool GFxCharacter::Is3D(bool checkAncestors) const                        
{ 
    if (checkAncestors)     // return whether I or any of my ancestors is in 3D
    {
        // am I in 3D?
        if (pMatrix3D_1 != NULL)
            return true;

        if (pParent)
            return pParent->Is3D(checkAncestors);
    }
    return (pMatrix3D_1 != NULL); 
}

// return the complete local matrix including both 2D and 3D
GMatrix3D GFxCharacter::GetLocalMatrix3D() const  
{
    GMatrix3D totalMat = (GetMatrix3D() ? *GetMatrix3D() : GMatrix3D::Identity);
    totalMat.Append(GMatrix3D(GetMatrix()));
    return totalMat;
}

// return the complete world matrix including both 2D and 3D
void    GFxCharacter::GetWorldMatrix3D(GMatrix3D *pmat) const
{           
    if (pParent)
    {
        pParent->GetWorldMatrix3D(pmat);
        pmat->Prepend(GetLocalMatrix3D());
    }
    else
    {
        *pmat = GetLocalMatrix3D();
    }
}

const GMatrix3D* GFxCharacter::GetPerspective3D(bool checkAncestors) const
{
    if (pPerspectiveMatrix3D || checkAncestors==false)
        return pPerspectiveMatrix3D;
    if (pParent)
        return pParent->GetPerspective3D(checkAncestors);
    if (GetMovieRoot())
        return GetMovieRoot()->pPerspectiveMatrix3D;
    return NULL;
}

const GMatrix3D* GFxCharacter::GetView3D(bool checkAncestors) const
{
    if (pViewMatrix3D || checkAncestors==false)
        return pViewMatrix3D;
    if (pParent)
        return pParent->GetView3D(checkAncestors);
    if (GetMovieRoot())
        return GetMovieRoot()->pViewMatrix3D;
    return NULL;
}
#endif

#ifndef GFC_NO_3D
static float scaleX(float left, float width, float x)
{
    return left + width * (x+1) / 2.f;
}

static float scaleY(float top, float height, float y)
{
    return top + height * (-y+1) / 2.f;
}
#endif

//
// get 3D bounds and transform to stage coords
//
GRectF GFxCharacter::GetProjectedBounds(const GMatrix3D *pWorldMatrix3D) const
{
#ifndef GFC_NO_3D
    GMatrix3D wvp3D;
    if (pWorldMatrix3D == NULL)
        GetWorldMatrix3D(&wvp3D);
    else
        wvp3D = *pWorldMatrix3D;

    wvp3D.Append(*GetView3D(true));
    wvp3D.Append(*GetPerspective3D(true));

    GViewport viewport;
    GetMovieRoot()->GetViewport(&viewport);

    GRectF visFrameRect = GetMovieRoot()->GetVisibleFrameRect();

    // get screen space bounds
    GRectF bounds3D = GetBounds(wvp3D, true);       // returns bounds in -1 to 1 space
    bounds3D.Left = scaleX(visFrameRect.Left, visFrameRect.Width(), bounds3D.Left);
    float bottom    = scaleY(visFrameRect.Top, visFrameRect.Height(), bounds3D.Top);     // negate top and bottom to invert Y
    bounds3D.Right  = scaleX(visFrameRect.Left, visFrameRect.Width(), bounds3D.Right);
    bounds3D.Top    = scaleY(visFrameRect.Top, visFrameRect.Height(), bounds3D.Bottom);
    bounds3D.Bottom = bottom;

    return bounds3D;
#else
    GUNUSED(pWorldMatrix3D); 
    return GRectF(0);
#endif
}

//
// get 3D non-axis-aligned bounds and transform to stage coords, bounds returned as an array of 4 pts
//
void GFxCharacter::GetProjectedBounds(GPointF *pts, const GMatrix3D *pWorldMatrix3D) const
{
#ifndef GFC_NO_3D
    GMatrix3D wvp3D;
    if (pWorldMatrix3D == NULL)
        GetWorldMatrix3D(&wvp3D);
    else
        wvp3D = *pWorldMatrix3D;

    wvp3D.Append(*GetView3D(true));
    wvp3D.Append(*GetPerspective3D(true));

    GViewport viewport;
    GetMovieRoot()->GetViewport(&viewport);

    GRectF visFrameRect = GetMovieRoot()->GetVisibleFrameRect();

    // get screen space bounds
    GetBounds(pts, wvp3D, true);       // returns bounds in -1 to 1 space

    pts[0].x = scaleX(visFrameRect.Left, visFrameRect.Width(), pts[0].x);
    pts[0].y = scaleY(visFrameRect.Top, visFrameRect.Height(), pts[0].y);
    pts[1].x = scaleX(visFrameRect.Left, visFrameRect.Width(), pts[1].x);
    pts[1].y = scaleY(visFrameRect.Top, visFrameRect.Height(), pts[1].y);
    pts[2].x = scaleX(visFrameRect.Left, visFrameRect.Width(), pts[2].x);
    pts[2].y = scaleY(visFrameRect.Top, visFrameRect.Height(), pts[2].y);
    pts[3].x = scaleX(visFrameRect.Left, visFrameRect.Width(), pts[3].x);
    pts[3].y = scaleY(visFrameRect.Top, visFrameRect.Height(), pts[3].y);
#else
    GUNUSED2(pts, pWorldMatrix3D); 
#endif
}

// Used during rendering.

// Temporary - used until blending logic is improved.
GRenderer::BlendType    GFxCharacter::GetActiveBlendMode() const
{
    GRenderer::BlendType    blend = GRenderer::Blend_None;
    const GFxCharacter*     pchar = this;
    
    while (pchar)
    {
        blend = pchar->GetBlendMode();
        if (blend > GRenderer::Blend_Layer)
            return blend;
        pchar = pchar->GetParent();
    }
    // Return last blend mode.
    return blend;
}


// This is not because of GFxMovieDefImpl dependency not available in the header.
UInt GFxCharacter::GetVersion() const
{
    return GetResourceMovieDef()->GetVersion();
}

GASEnvironment*         GFxCharacter::GetASEnvironment()
{
    GFxASCharacter* pparent = GetParent();
    while(pparent && !pparent->IsSprite())
        pparent = pparent->GetParent();
    if (!pparent)
        return 0;
    return pparent->ToSprite()->GetASEnvironment();
}

const GASEnvironment*   GFxCharacter::GetASEnvironment() const
{
    // Call non-const version. Const-ness really only matters for GFxSprite.
    return const_cast<GFxCharacter*>(this)->GetASEnvironment();
}

// (needs to be implemented in .cpp, so that GFxMovieRoot is visible)
GFxLog*     GFxCharacter::GetLog() const
{
    // By default, GetMovieRoot will delegate to parent.
    // GFxSprite should override GetMovieRoot to return the correct object.
    return GetMovieRoot()->GetCachedLog();
}
bool        GFxCharacter::IsVerboseAction() const
{
    return GetMovieRoot()->IsVerboseAction();
}

bool        GFxCharacter::IsVerboseActionErrors() const
{
    return !GetMovieRoot()->IsSuppressActionErrors();
}

GFxScale9GridInfo* GFxCharacter::CreateScale9Grid(Float pixelScale) const
{
    GFxASCharacter* parent   = GetParent();
    GMatrix2D       shapeMtx = GetMatrix();
    while (parent)
    {
        if (parent->GetScale9Grid())
        {
            GRectF bounds = parent->GetRectBounds(GMatrix2D());
            return GHEAP_AUTO_NEW(this) GFxScale9GridInfo(parent->GetScale9Grid(), 
                                                          parent->GetMatrix(), shapeMtx, 
                                                          pixelScale, bounds);
        }
        shapeMtx.Append(parent->GetMatrix());
        parent = parent->GetParent();
    }
    return 0;
}

void GFxCharacter::OnEventUnload()
{
    SetUnloading();

    // should it be before Event_Unload or after? (AB)
    if (IsTopmostLevelFlagSet())
    {
        GFxMovieRoot* proot = GetMovieRoot();
        proot->RemoveTopmostLevelCharacter(this);
    }
    if (!IsUnloaded())
    {
        OnEvent(GFxEventId::Event_Unload);
        SetUnloaded();
    }
}


bool GFxCharacter::CheckLastHitResult(Float x, Float y) const
{
    return 
        (Flags & Mask_HitTest) != 0 &&
        x == LastHitTestX && y == LastHitTestY;
}

void GFxCharacter::SetLastHitResult(Float x, Float y, bool result) const // TO DO: Revise "const"
{
    LastHitTestX = x;
    LastHitTestY = y;
    Flags &= ~Mask_HitTest;
    Flags |= result ? Mask_HitTestPositive : Mask_HitTestNegative;
}

void GFxCharacter::InvalidateHitResult() const                           // TO DO: Revise "const"
{
    Flags &= ~Mask_HitTest;
}


// ***** GFxCharacterHandle



GFxCharacterHandle::GFxCharacterHandle(const GASString& name, GFxASCharacter *pparent, GFxASCharacter* pcharacter)
    : Name(name), NamePath(name.GetManager()->CreateEmptyString()), OriginalName(name)
{
    RefCount    = 1;
    pCharacter  = pcharacter;    
    
    // Compute path based on parent
    GString namePathBuff;
    if (pparent)
    {
        pparent->GetAbsolutePath(&namePathBuff);
        namePathBuff += ".";
    }
    namePathBuff += Name.ToCStr();
    NamePath = name.GetManager()->CreateString(namePathBuff);
}

GFxCharacterHandle::~GFxCharacterHandle()
{
}

// Release a character reference, used when character dies
void    GFxCharacterHandle::ReleaseCharacter()
{
    pCharacter = 0;
}

    
// Changes the name.
void    GFxCharacterHandle::ChangeName(const GASString& name, GFxASCharacter *pparent)
{
    Name = name;
    // Compute path based on parent
    GString namePathBuff;
    if (pparent)
    {
        pparent->GetAbsolutePath(&namePathBuff);
        namePathBuff += ".";
    }
    namePathBuff += Name.ToCStr();
    NamePath = name.GetManager()->CreateString(namePathBuff);

    // Do we need to update paths in all parents ??
}

// Resolve the character, considering path if necessary.
GFxASCharacter* GFxCharacterHandle::ResolveCharacter(GFxMovieRoot *proot) const
{
    if (pCharacter)
        return pCharacter;
    // Resolve a global path based on Root.
    return proot->FindTarget(NamePath);
}

GFxASCharacter* GFxCharacterHandle::ForceResolveCharacter(GFxMovieRoot *proot) const
{
    // Resolve a global path based on Root.
    return proot->FindTarget(NamePath);
}

void GFxCharacterHandle::ResetName(GASStringContext* psc)
{
    Name     = psc->GetBuiltin(GASBuiltin_empty_);
    NamePath = Name; 
}


// ***** GFxASCharacter

// Constructor.
GFxASCharacter::GFxASCharacter(GFxMovieDefImpl* pbindingDefImpl,
                               GFxASCharacter* pparent, GFxResourceId id)
    :
    GFxCharacter(pparent, id),
    pDefImpl(pbindingDefImpl),
    pGeomData(0),
    Flags(0),
    TabIndex (0),
    RollOverCnt(0),
    pDisplayCallback(NULL),
    DisplayCallbackUserPtr(NULL)
{
    FocusGroupMask = 0;
    //--FocusGroupMask; // make 0xFFFF

    SetASCharacterFlag();
#ifndef GFC_USE_OLD_ADVANCE
    pPlayNext = pPlayPrev = NULL;
    pPlayNextOpt = NULL;
#endif
    BlendMode = (UInt8)GRenderer::Blend_None;
    pFilters = NULL;

    SetVisibleFlag();
    SetAcceptAnimMovesFlag();
    SetEnabledFlag();
    SetInstanceBasedNameFlag();
}

GFxASCharacter::~GFxASCharacter()
{
#ifndef GFC_USE_OLD_ADVANCE
    GASSERT(!pPlayNext && !pPlayPrev); // actually already should be removed
#endif
    if (pNameHandle)
        pNameHandle->ReleaseCharacter();
    if (pGeomData)
        delete pGeomData;
    if (pFilters)
        delete pFilters;
}

UInt16 GFxASCharacter::GetFocusGroupMask() const
{
    if (FocusGroupMask == 0)
        return GetParent()->GetFocusGroupMask();
    return FocusGroupMask;
}

UInt16 GFxASCharacter::GetFocusGroupMask()
{
    if (FocusGroupMask == 0 && GetParent())
        FocusGroupMask = GetParent()->GetFocusGroupMask();
    return FocusGroupMask;
}

bool   GFxASCharacter::IsFocusAllowed(GFxMovieRoot* proot, UInt controllerIdx) const
{
    UInt focusIdx = proot->GetFocusGroupIndex(controllerIdx);
    return (GetFocusGroupMask() & (1 << focusIdx)) != 0;
}

bool   GFxASCharacter::IsFocusAllowed(GFxMovieRoot* proot, UInt controllerIdx)
{
    UInt focusIdx = proot->GetFocusGroupIndex(controllerIdx);
    return (GetFocusGroupMask() & (1 << focusIdx)) != 0;
}

void GFxASCharacter::OnEventUnload()
{
    SetUnloading();

    GFxMovieRoot* proot = GetMovieRoot();
    RemoveFromPlaylist(proot);

    if (proot->IsDraggingCharacter(this))
        proot->StopDrag();

    GFxCharacter::OnEventUnload();
    
    // Need to check, is this character currently focused or not. If yes, we need
    // to reset a pointer to currently focused character. OnEventUnload might be
    // called from the sprite's destructor (so refcnt == 0 but weak ref is not set to NULL yet)
    if (proot)
        proot->ResetFocusForChar(this);

    // need to release the character to avoid accidental reusing unloaded character.
    if (pNameHandle)
        pNameHandle->ReleaseCharacter();
}

bool GFxASCharacter::OnUnloading()
{
    bool rv = GFxCharacter::OnUnloading();
    GFxMovieRoot* proot = GetMovieRoot();
    RemoveFromPlaylist(proot);
    return rv;
}

void GFxASCharacter::AddToPlayList(GFxMovieRoot* proot)
{
#ifndef GFC_USE_OLD_ADVANCE
    GASSERT(proot->pPlayListHead != this);
    GASSERT(!pPlayNext && !pPlayPrev);

    //   printf("=------------- %s\n", pch->GetCharacterHandle()->GetNamePath().ToCStr()); //DBG

    if (proot->pPlayListHead)
    {
        proot->pPlayListHead->pPlayPrev = this;
        pPlayNext                       = proot->pPlayListHead;
    }
    proot->pPlayListHead                = this;
    proot->SetDirtyFlag();
#else
    GUNUSED(proot);
#endif
}

void GFxASCharacter::AddToOptimizedPlayList(GFxMovieRoot* proot)
{
#ifndef GFC_USE_OLD_ADVANCE
    if (IsOptAdvancedListFlagSet() || proot->IsOptAdvanceListInvalid() || IsUnloaded() || IsUnloading())
        return;
    GASSERT(proot->pPlayListHead == this || pPlayPrev);
    GASSERT(proot->pPlayListOptHead != this && !pPlayNextOpt);

    // find the place in the optimized playlist
    GFxASCharacter* pcur = pPlayPrev;
    for (; pcur && !pcur->IsOptAdvancedListFlagSet(); pcur = pcur->pPlayPrev)
        ;
    if (pcur)
    {
        pPlayNextOpt        = pcur->pPlayNextOpt;
        pcur->pPlayNextOpt  = this;
    }
    else
    {
        pPlayNextOpt            = proot->pPlayListOptHead;
        proot->pPlayListOptHead = this;
    }
    SetOptAdvancedListFlag();
    proot->SetDirtyFlag();
#else
    GUNUSED(proot);
#endif
}

void GFxASCharacter::RemoveFromPlaylist(GFxMovieRoot* proot)
{
    RemoveFromPreDisplayList(proot);
#ifndef GFC_USE_OLD_ADVANCE
    GASSERT(proot);

    RemoveFromOptimizedPlaylist(proot);

    if (pPlayNext)
        pPlayNext->pPlayPrev = pPlayPrev;
    if (pPlayPrev)
        pPlayPrev->pPlayNext = pPlayNext;
    else
    {
        if (proot->pPlayListHead == this)
            proot->pPlayListHead = pPlayNext;
    }
    pPlayNext = pPlayPrev = NULL;
    proot->SetDirtyFlag();
#else
    GUNUSED(proot);
#endif
}

void GFxASCharacter::RemoveFromOptimizedPlaylist(GFxMovieRoot* proot)
{
#ifndef GFC_USE_OLD_ADVANCE
    if (IsOptAdvancedListFlagSet())
    {
        if (!proot->IsOptAdvanceListInvalid())
        {
            GASSERT(proot);

            // find previous element in optimized list
            GFxASCharacter* pcur = pPlayPrev;
            for (; pcur && !pcur->IsOptAdvancedListFlagSet(); pcur = pcur->pPlayPrev)
                ;
            if (pcur)
                pcur->pPlayNextOpt = pPlayNextOpt;
            else
            {
                GASSERT(proot->pPlayListOptHead == this);
                proot->pPlayListOptHead = pPlayNextOpt;
            }
        }
        pPlayNextOpt = NULL;
        ClearOptAdvancedListFlag();
        proot->SetDirtyFlag();
    }
#else
    GUNUSED(proot);
#endif
}

void GFxASCharacter::UpdateAlphaFlag()
{
    if ((fabs(ColorTransform.M_[3][0]) < 0.001f) &&
        (fabs(ColorTransform.M_[3][1]) < 1.0f) )
        SetAlpha0Flag();
    else
        ClearAlpha0Flag();
}

void GFxASCharacter::CloneInternalData(const GFxASCharacter* src)
{
    GASSERT(src);
    if (src->pGeomData)
        SetGeomData(*src->pGeomData);
    EventHandlers = src->EventHandlers;
}

GASGlobalContext*   GFxASCharacter::GetGC() const
{
    return GetMovieRoot()->pGlobalContext;
}

void    GFxASCharacter::SetName(const GASString& name)
{
    if (!name.IsEmpty())
        ClearInstanceBasedNameFlag();

    if (pNameHandle)
    {
        pNameHandle->ChangeName(name, pParent);
        
        // TBD: Propagate update to all children ??
    }
    else
    {
        pNameHandle = *GHEAP_AUTO_NEW(this) GFxCharacterHandle(name, pParent, this);
    }
}

void    GFxASCharacter::SetOriginalName(const GASString& name)
{
    GFxASCharacter::SetName(name);
    GFxCharacterHandle* pnameHandle = GetCharacterHandle();
    if (pnameHandle)
        return pnameHandle->SetOriginalName(name);
}

const GASString& GFxASCharacter::GetOriginalName() const
{
    GFxCharacterHandle* pnameHandle = GetCharacterHandle();
    if (pnameHandle)
        return pnameHandle->GetOriginalName();
    return GetName();
}

// Determines the absolute path of the character.
void    GFxASCharacter::GetAbsolutePath(GString *ppath) const
{
    if (pParent)
    {
        GASSERT(pParent != this);
        pParent->GetAbsolutePath(ppath);
        *ppath += ".";
        *ppath += GetName().ToCStr();
    }
    else
    {
        if (IsSprite())
            G_Format(*ppath, "_level{0}", ((const GFxSprite*)this)->GetLevel());
        else
        {
            // Non-sprite characters must have parents.
            GASSERT(0);
            ppath->Clear();
        }
    }
}

GFxCharacterHandle* GFxASCharacter::CreateCharacterHandle() const
{
    if (!pNameHandle)
    {   
        GFxMovieRoot* proot = GetMovieRoot();

        // Hacky, but this can happen.
        // Will clearing child handles recursively on parent release work better?
        if (IsSprite() && ((const GFxSprite*)this)->IsUnloaded())
        {
            GFC_DEBUG_WARNING(1, "GetCharacterHandle called on unloaded sprite");
            // Returns temp handle which is essentially useless.
            pNameHandle = *GHEAP_NEW(proot->GetMovieHeap())
                            GFxCharacterHandle(GetASEnvironment()->GetBuiltin(GASBuiltin_empty_), 0, 0);
            return pNameHandle;
        }        

        // Create new instance names as necessary.
        GASString name(proot->CreateNewInstanceName());
        // SetName logic duplicated to take advantage of 'mutable' pNameHandle.
        pNameHandle = *GHEAP_NEW(proot->GetMovieHeap())
                            GFxCharacterHandle(name, pParent, const_cast<GFxASCharacter*>(this));
    }

    return pNameHandle;
}

// Implement mouse-dragging for this pMovie.
void    GFxASCharacter::DoMouseDrag()
{
    GFxMovieRoot::DragState     st;
    GFxMovieRoot*   proot = GetMovieRoot();
    proot->GetDragState(&st);

    if (this == st.pCharacter)
    {
        // We're being dragged!
        GPointF worldMouse = proot->GetMouseState(0)->GetLastPosition();
        GPointF parentMouse;
        Matrix  parentWorldMat;
        if (pParent)
            parentWorldMat = pParent->GetWorldMatrix();

        parentWorldMat.TransformByInverse(&parentMouse, worldMouse);

#ifndef GFC_NO_3D
        if (Is3D(true))
        {
            const GMatrix3D     *pPersp = GetPerspective3D(true);
            const GMatrix3D     *pView = GetView3D(true);
            if (pPersp)
                GetMovieRoot()->ScreenToWorld.SetPerspective(*pPersp);
            if (pView)
                GetMovieRoot()->ScreenToWorld.SetView(*pView);
            GetMovieRoot()->ScreenToWorld.SetWorld(pParent->GetWorldMatrix3D());
            GetMovieRoot()->ScreenToWorld.GetWorldPoint(&parentMouse);
        }
#endif

        // if (!st.LockCenter) is not necessary, because then st.CenterDelta == 0.
        parentMouse += st.CenterDelta;

        if (st.Bound)
        {           
            // Clamp mouse coords within a defined rectangle
            parentMouse.x = G_Clamp(parentMouse.x, st.BoundLT.x, st.BoundRB.x);
            parentMouse.y = G_Clamp(parentMouse.y, st.BoundLT.y, st.BoundRB.y);
        }

        // Once touched, object is no longer animated by the timeline
        SetAcceptAnimMoves(0);

        // Place our origin so that it coincides with the mouse coords
        // in our parent frame.
        SetStandardMember(M_x, TwipsToPixels(Double(parentMouse.x)), false);
        SetStandardMember(M_y, TwipsToPixels(Double(parentMouse.y)), false);
        
        //Matrix    local = GetMatrix();
        //local.M_[0][2] = parentMouse.x;
        //local.M_[1][2] = parentMouse.y;
        //SetMatrix(local);

    }
}


// *** Shared ActionScript methods.

// Depth implementation - same in MovieClip, Button, TextField.
void    GFxASCharacter::CharacterGetDepth(const GASFnCall& fn)
{
    GFxASCharacter* pcharacter = fn.ThisPtr->ToASCharacter();
    if (!pcharacter)    
         pcharacter = fn.Env->GetTarget();  
    GASSERT(pcharacter);

    // Depth always starts at -16384,
    // probably to allow user assigned depths to start at 0.
    fn.Result->SetInt(pcharacter->GetDepth() - 16384);
}

// *** Standard member support


GFxASCharacter::MemberTableType GFxASCharacter::MemberTable[] =
{
    { "_x",             GFxASCharacter::M_x,            true },  
    { "_y",             GFxASCharacter::M_y,            true },
    { "_xscale",        GFxASCharacter::M_xscale,       true },
    { "_yscale",        GFxASCharacter::M_yscale,       true },
    { "_currentframe",  GFxASCharacter::M_currentframe, true },
    { "_totalframes",   GFxASCharacter::M_totalframes,  true },
    { "_alpha",         GFxASCharacter::M_alpha,        true },
    { "_visible",       GFxASCharacter::M_visible,      true },
    { "_width",         GFxASCharacter::M_width,        true },
    { "_height",        GFxASCharacter::M_height,       true },
    { "_rotation",      GFxASCharacter::M_rotation,     true },
    { "_target",        GFxASCharacter::M_target,       true },
    { "_framesloaded",  GFxASCharacter::M_framesloaded, true },
    { "_name",          GFxASCharacter::M_name,         true },
    { "_droptarget",    GFxASCharacter::M_droptarget,   true },
    { "_url",           GFxASCharacter::M_url,          true },
    { "_highquality",   GFxASCharacter::M_highquality,  true },
    { "_focusrect",     GFxASCharacter::M_focusrect,    true },
    { "_soundbuftime",  GFxASCharacter::M_soundbuftime, true },
    // SWF 5+.
    { "_quality",       GFxASCharacter::M_quality,  true },
    { "_xmouse",        GFxASCharacter::M_xmouse,   true },
    { "_ymouse",        GFxASCharacter::M_ymouse,   true }, 

    // Extra shared properties which can have a default implementation.
    { "_parent",        GFxASCharacter::M_parent,       false },     
    { "blendMode",      GFxASCharacter::M_blendMode,    false },
    { "cacheAsBitmap",  GFxASCharacter::M_cacheAsBitmap, false },
    { "filters",        GFxASCharacter::M_filters,      false },
    { "enabled",        GFxASCharacter::M_enabled,      false },
    { "trackAsMenu",    GFxASCharacter::M_trackAsMenu,  false },
    { "_lockroot",      GFxASCharacter::M_lockroot,     false },
    { "tabEnabled",     GFxASCharacter::M_tabEnabled,   false },
    { "tabIndex",       GFxASCharacter::M_tabIndex,     false },
    { "useHandCursor",  GFxASCharacter::M_useHandCursor, false },  
    
    // Not shared.
    { "menu",           GFxASCharacter::M_menu, false },

    // MovieClip custom.
    { "focusEnabled",   GFxASCharacter::M_focusEnabled, false },
    { "tabChildren",    GFxASCharacter::M_tabChildren, false },
    { "transform",      GFxASCharacter::M_transform, false },
    { "scale9Grid",     GFxASCharacter::M_scale9Grid, false },
    { "hitArea",        GFxASCharacter::M_hitArea, false },

    // TextField custom.
    { "text",           GFxASCharacter::M_text, false },
    { "textWidth",      GFxASCharacter::M_textWidth, false },
    { "textHeight",     GFxASCharacter::M_textHeight, false },
    { "textColor",      GFxASCharacter::M_textColor, false },
    { "length",         GFxASCharacter::M_length, false },
    { "html",           GFxASCharacter::M_html, false },
    { "htmlText",       GFxASCharacter::M_htmlText, false },
    { "styleSheet",     GFxASCharacter::M_styleSheet, false },
    { "autoSize",       GFxASCharacter::M_autoSize, false },
    { "wordWrap",       GFxASCharacter::M_wordWrap, false },
    { "multiline",      GFxASCharacter::M_multiline, false },
    { "border",         GFxASCharacter::M_border, false },
    { "variable",       GFxASCharacter::M_variable, false },
    { "selectable",     GFxASCharacter::M_selectable, false },
    { "embedFonts",     GFxASCharacter::M_embedFonts, false },
    { "antiAliasType",  GFxASCharacter::M_antiAliasType, false },
    { "hscroll",        GFxASCharacter::M_hscroll, false },
    { "scroll",         GFxASCharacter::M_scroll, false },
    { "maxscroll",      GFxASCharacter::M_maxscroll, false },
    { "maxhscroll",     GFxASCharacter::M_maxhscroll, false },
    { "background",     GFxASCharacter::M_background, false },
    { "backgroundColor",GFxASCharacter::M_backgroundColor, false },
    { "borderColor",    GFxASCharacter::M_borderColor, false },
    { "bottomScroll",   GFxASCharacter::M_bottomScroll, false },
    { "type",           GFxASCharacter::M_type, false },
    { "maxChars",       GFxASCharacter::M_maxChars, false },
    { "condenseWhite",  GFxASCharacter::M_condenseWhite, false },
    { "mouseWheelEnabled", GFxASCharacter::M_mouseWheelEnabled, false },
    { "password",       GFxASCharacter::M_password, false },
    
    // GFx extensions
    { "shadowStyle",    GFxASCharacter::M_shadowStyle, false },
    { "shadowColor",    GFxASCharacter::M_shadowColor, false },
    { "hitTestDisable", GFxASCharacter::M_hitTestDisable, false },
    { "noTranslate",    GFxASCharacter::M_noTranslate, false },
    { "caretIndex",     GFxASCharacter::M_caretIndex, false },
    { "numLines",       GFxASCharacter::M_numLines, false   },
    { "verticalAutoSize",GFxASCharacter::M_verticalAutoSize, false },
    { "fontScaleFactor", GFxASCharacter::M_fontScaleFactor, false },
    { "verticalAlign",  GFxASCharacter::M_verticalAlign, false },
    { "textAutoSize",   GFxASCharacter::M_textAutoSize, false },
    { "useRichTextClipboard", GFxASCharacter::M_useRichTextClipboard, false },
    { "alwaysShowSelection",  GFxASCharacter::M_alwaysShowSelection, false },
    { "selectionBeginIndex",  GFxASCharacter::M_selectionBeginIndex, false },
    { "selectionEndIndex",    GFxASCharacter::M_selectionEndIndex, false },
    { "selectionBkgColor",    GFxASCharacter::M_selectionBkgColor, false },
    { "selectionTextColor",   GFxASCharacter::M_selectionTextColor, false },
    { "inactiveSelectionBkgColor",  GFxASCharacter::M_inactiveSelectionBkgColor, false },
    { "inactiveSelectionTextColor", GFxASCharacter::M_inactiveSelectionTextColor, false },
    { "noAutoSelection", GFxASCharacter::M_noAutoSelection, false },
    { "disableIME",      GFxASCharacter::M_disableIME, false },
    { "topmostLevel",    GFxASCharacter::M_topmostLevel, false },
    { "noAdvance",       GFxASCharacter::M_noAdvance, false },
    { "focusGroupMask",      GFxASCharacter::M_focusGroupMask, false },

    // Dynamic Text
    { "autoFit",        GFxASCharacter::M_autoFit, false },
    { "blurX",          GFxASCharacter::M_blurX, false },
    { "blurY",          GFxASCharacter::M_blurY, false },
    { "blurStrength",   GFxASCharacter::M_blurStrength, false },
    { "outline",        GFxASCharacter::M_outline, false },
    { "fauxBold",       GFxASCharacter::M_fauxBold, false },
    { "fauxItalic",     GFxASCharacter::M_fauxItalic, false },
    { "restrict",       GFxASCharacter::M_restrict, false },
    
    // Dynamic Text Shadow
    { "shadowAlpha",      GFxASCharacter::M_shadowAlpha, false },
    { "shadowAngle",      GFxASCharacter::M_shadowAngle, false },
    { "shadowBlurX",      GFxASCharacter::M_shadowBlurX, false },
    { "shadowBlurY",      GFxASCharacter::M_shadowBlurY, false },
    { "shadowDistance",   GFxASCharacter::M_shadowDistance, false },
    { "shadowHideObject", GFxASCharacter::M_shadowHideObject, false },
    { "shadowKnockOut",   GFxASCharacter::M_shadowKnockOut, false },
    { "shadowQuality",    GFxASCharacter::M_shadowQuality, false },
    { "shadowStrength",   GFxASCharacter::M_shadowStrength, false },
    { "shadowOutline",    GFxASCharacter::M_shadowOutline, false },

#ifndef GFC_NO_3D
    { "_z",               GFxASCharacter::M_z, true },
    { "_zscale",          GFxASCharacter::M_zscale, true },
    { "_xrotation",       GFxASCharacter::M_xrotation, true },
    { "_yrotation",       GFxASCharacter::M_yrotation, true },
    { "_matrix3d",        GFxASCharacter::M_matrix3d, true },
    { "_perspfov",        GFxASCharacter::M_perspfov, true },
#endif

    { "$version",    GFxASCharacter::M__version, false },

    // Done.
    { 0,  GFxASCharacter::M_InvalidMember, false }
};

void    GFxASCharacter::InitStandardMembers(GASGlobalContext *pcontext)
{
    GCOMPILER_ASSERT( (sizeof(MemberTable)/sizeof(MemberTable[0]))
                        == M_StandardMemberCount + 1);

    // Add all standard members.
    MemberTableType* pentry;
    GASStringManager* pstrManager = pcontext->GetStringManager();

    pcontext->StandardMemberMap.SetCapacity(M_StandardMemberCount);  

    for (pentry = MemberTable; pentry->pName; pentry++)
    {
        GASString name(pstrManager->CreateConstString(pentry->pName, strlen(pentry->pName),
                                                      GASString::Flag_StandardMember |
                                                      (pentry->CaseInsensitive?GASString::Flag_CaseInsensitive:0)));
        pcontext->StandardMemberMap.Add(name, (SByte)pentry->Id);
    }
}

bool             GFxASCharacter::IsStandardMember(const GASString& memberName, GASString* pcaseInsensitiveName)
{
    if (memberName.IsStandardMember())
        return true;
    // now need to check a special case. Flash 7+ uses case sensitive names
    // for every variable/member but some exceptions. Some standard members
    // still might be referenced case insensitively.
    if (memberName.GetLength() > 0 && memberName.GetCharAt(0) == '_')
    {
        GASString lowerCase = memberName.ToLower();
        if (lowerCase.IsCaseInsensitive())
        {
            if (pcaseInsensitiveName)
                *pcaseInsensitiveName = lowerCase;
            return true;
        }
    }
    return false;
}

// Looks up a standard member and returns M_InvalidMember if it is not found.
GFxASCharacter::StandardMember  GFxASCharacter::GetStandardMemberConstant(const GASString& memberName) const
{
    SByte   memberConstant = M_InvalidMember; // Has to be signed or conversion is incorrect!
    GASString lowerCaseName = GetGC()->GetBuiltin(GASBuiltin_empty_);
    if (IsStandardMember(memberName, &lowerCaseName))
    {
        GASGlobalContext* pcontext = GetGC();
        bool caseSensitive = lowerCaseName.IsEmpty();
        pcontext->StandardMemberMap.GetCaseCheck(memberName, &memberConstant, caseSensitive);
    }        
    
    GASSERT((memberConstant != M_InvalidMember) ? 
        (memberName.IsStandardMember() || (lowerCaseName.IsCaseInsensitive() && lowerCaseName.IsStandardMember())) : 1);
    return (StandardMember) memberConstant;
}

GFxASCharacter::GeomDataType& GFxASCharacter::GetGeomData(GFxASCharacter::GeomDataType& geomData) const
{
    if (!pGeomData)
    {
        // fill GeomData using Matrix_1
        const Matrix& m = GetMatrix();
        geomData.X = int(m.GetX());
        geomData.Y = int(m.GetY());
        geomData.XScale = m.GetXScale()*(Double)100.;
        geomData.YScale = m.GetYScale()*(Double)100.;
        geomData.Rotation = (m.GetRotation()*(Double)180.)/GFC_MATH_PI;
        geomData.OrigMatrix = Matrix_1;

#ifndef GFC_NO_3D
        const GMatrix3D* pm3D = GetMatrix3D();
        if (pm3D)
        {
            geomData.Z = pm3D->GetZ();
            geomData.ZScale = pm3D->GetZScale() * (Double)100.;
            float fX, fY;
            pm3D->GetRotation(&fX, &fY, NULL);
            geomData.XRotation = GFC_RADTODEG(fX);
            geomData.YRotation = GFC_RADTODEG(fY);
        }
#endif
    }
    else
    {
        geomData = *pGeomData;
    }
    return geomData;
}

void    GFxASCharacter::SetGeomData(const GeomDataType& gd)
{
    if (pGeomData)
        *pGeomData = gd;
    else
        pGeomData = GHEAP_AUTO_NEW(this) GeomDataType(gd);
}

void GFxASCharacter::EnsureGeomDataCreated()
{
    if (!pGeomData)
    {
        GeomDataType geomData;
        SetGeomData(GetGeomData(geomData));
    }
}

void    GFxASCharacter::SetAcceptAnimMoves(bool accept)
{ 
    if (!accept)
        EnsureGeomDataCreated();
    GFxMovieRoot* proot = GetMovieRoot();
    SetAcceptAnimMovesFlag(accept); 

    // cache the flag locally
    SetContinueAnimationFlag(proot->IsContinueAnimationFlagSet()); 
    
    if (proot->IsContinueAnimationFlagSet() && accept)
    {
        // if continueAnimation flag is set and we restore timeline acceptance - 
        // remove the pGeomData to provide correct coordinates/scales by accessing
        // via ActionScript.
        delete pGeomData;
        pGeomData = NULL;
    }
    proot->SetDirtyFlag();
}

// BlendMode lookup table.
// Should be moved elsewhere so that it can apply to buttons, etc.
static const char * GFx_BlendModeNames[1+ 14] =
{
    "normal",   // 0?
    "normal",   
    "layer",
    "multiply",
    "screen",
    "lighten",
    "darken",
    "difference",
    "add",
    "subtract",
    "invert",
    "alpha",
    "erase",
    "overlay",
    "hardlight"
};

void GFxASCharacter_MatrixScaleAndRotate2x2(GMatrix2D& m, Float sx, Float sy, Float radians)
{
    Float   cosAngle = cosf(radians);
    Float   sinAngle = sinf(radians);
    Float   x00 = m.M_[0][0];
    Float   x01 = m.M_[0][1];
    Float   x10 = m.M_[1][0];
    Float   x11 = m.M_[1][1];

    // avoid creating irreversible 2D matrices
    //!AB: should be revised, since Matrix2D already has code that prevents from creation of irreversible matrices.
    // The condition below causes a problem when xscale/yscale is set to 0.
    //if (fabsf(sx) > 1e-4f && fabsf(sy) > 1e-4f)
    {
        m.M_[0][0] = (x00*cosAngle-x10*sinAngle)*sx;
        m.M_[0][1] = (x01*cosAngle-x11*sinAngle)*sy;
        m.M_[1][0] = (x00*sinAngle+x10*cosAngle)*sx;
        m.M_[1][1] = (x01*sinAngle+x11*cosAngle)*sy;
    }
}

// Handles built-in members. Return 0 if member is not found or not supported.
bool    GFxASCharacter::SetStandardMember(StandardMember member, const GASValue& val, bool opcodeFlag)
{   
    if (opcodeFlag && ((member < M_BuiltInProperty_Begin) || (member > M_BuiltInProperty_End)))
    {
        LogScriptError("Invalid SetProperty request, property number %d\n", member);
        return 0;
    }
    // Make sure that this character class supports the constant.
    if (member == M_InvalidMember)
        return 0;
    if (!((member <= M_SharedPropertyEnd) && (GetStandardMemberBitMask() & (1<<member))))
        return 0;
    GASEnvironment* pEnv = GetASEnvironment ();

    switch(member)
    {
    case M_x:
        {
            // NOTE: If updating this logic, also update GFxValue::ObjectInterface::SetDisplayInfo
            GASNumber xval = val.ToNumber(pEnv);
            if (val.IsUndefined() || GASNumberUtil::IsNaN(xval))
                return 1;
            if (GASNumberUtil::IsNEGATIVE_INFINITY(xval) || GASNumberUtil::IsPOSITIVE_INFINITY(xval))
                xval = 0;
            SetAcceptAnimMoves(0);
            GASSERT(pGeomData);

            Matrix  m = GetMatrix();
            pGeomData->X = int(floor(PixelsToTwips(xval)));
            m.M_[0][2] = (Float) pGeomData->X;
            if (m.IsValid())
                SetMatrix(m);
            return 1;
        }

    case M_y:
        {
            // NOTE: If updating this logic, also update GFxValue::ObjectInterface::SetDisplayInfo
            GASNumber yval = val.ToNumber(pEnv);
            if (val.IsUndefined() || GASNumberUtil::IsNaN(yval))
                return 1;
            if (GASNumberUtil::IsNEGATIVE_INFINITY(yval) || GASNumberUtil::IsPOSITIVE_INFINITY(yval))
                yval = 0;
            SetAcceptAnimMoves(0);
            GASSERT(pGeomData);

            Matrix  m = GetMatrix();
            pGeomData->Y = int(floor(PixelsToTwips(yval)));
            m.M_[1][2] = (Float) pGeomData->Y;
            if (m.IsValid())
                SetMatrix(m);
            return 1;
        }

    case M_xscale:
        {
            // NOTE: If updating this logic, also update GFxValue::ObjectInterface::SetDisplayInfo
            Double newXScale = val.ToNumber(pEnv);
            if (val.IsUndefined() || GASNumberUtil::IsNaN(newXScale) ||
                GASNumberUtil::IsNEGATIVE_INFINITY(newXScale) || GASNumberUtil::IsPOSITIVE_INFINITY(newXScale))
            {
                return 1;
            }
            SetAcceptAnimMoves(0);
            GASSERT(pGeomData);

            const Matrix& chm = GetMatrix();
            Matrix m = pGeomData->OrigMatrix;
            m.M_[0][2] = chm.M_[0][2];
            m.M_[1][2] = chm.M_[1][2];

            Double origXScale = m.GetXScale();
            pGeomData->XScale = newXScale;
            if (origXScale == 0 || newXScale > 1E+16)
            {
                newXScale = 0;
                origXScale = 1;
            }

            GFxASCharacter_MatrixScaleAndRotate2x2(m,
                Float(newXScale/(origXScale*100.)), 
                Float(pGeomData->YScale/(m.GetYScale()*100.)),
                Float(-m.GetRotation() + pGeomData->Rotation * GFC_MATH_PI / 180.));

            if (m.IsValid())
                SetMatrix(m);
            return 1;
        }

    case M_yscale:
        {
            // NOTE: If updating this logic, also update GFxValue::ObjectInterface::SetDisplayInfo
            Double newYScale = val.ToNumber(pEnv);
            if (val.IsUndefined() || GASNumberUtil::IsNaN(newYScale) ||
                GASNumberUtil::IsNEGATIVE_INFINITY(newYScale) || GASNumberUtil::IsPOSITIVE_INFINITY(newYScale))
            {
                return 1;
            }
            SetAcceptAnimMoves(0);
            GASSERT(pGeomData);

            const Matrix& chm = GetMatrix();
            Matrix m = pGeomData->OrigMatrix;
            m.M_[0][2] = chm.M_[0][2];
            m.M_[1][2] = chm.M_[1][2];

            Double origYScale = m.GetYScale();
            pGeomData->YScale = newYScale;
            if (origYScale == 0 || newYScale > 1E+16)
            {
                newYScale = 0;
                origYScale = 1;
            }

            GFxASCharacter_MatrixScaleAndRotate2x2(m,
                Float(pGeomData->XScale/(m.GetXScale()*100.)), 
                Float(newYScale/(origYScale* 100.)),
                Float(-m.GetRotation() + pGeomData->Rotation * GFC_MATH_PI / 180.));

            if (m.IsValid())
                SetMatrix(m);
            return 1;
        }

    case M_rotation:
        {
            // NOTE: If updating this logic, also update GFxValue::ObjectInterface::SetDisplayInfo
            GASNumber rval = val.ToNumber(pEnv);
            if (val.IsUndefined() || GASNumberUtil::IsNaN(rval))
                return 1;
            SetAcceptAnimMoves(0);
            GASSERT(pGeomData);

            Double r = fmod((Double)rval, (Double)360.);
            if (r > 180)
                r -= 360;
            else if (r < -180)
                r += 360;
            pGeomData->Rotation = r;

            const Matrix& chm = GetMatrix();
            Matrix m = pGeomData->OrigMatrix;
            m.M_[0][2] = chm.M_[0][2];
            m.M_[1][2] = chm.M_[1][2];

            Double origRotation = m.GetRotation();

            // remove old rotation by rotate back and add new one

            GFxASCharacter_MatrixScaleAndRotate2x2(m,
                Float(pGeomData->XScale/(m.GetXScale()*100.)), 
                Float(pGeomData->YScale/(m.GetYScale()*100.)),
                Float(-origRotation + r * GFC_MATH_PI / 180.));

            if (m.IsValid())
                SetMatrix(m);
            return 1;
        }

    case M_width:
        {
            // MA: Width/Height modification in Flash is unusual in that it performs
            // relative axis scaling in the x local axis (width scaling) and y local
            // axis (height scaling). The resulting width after scaling does not
            // actually equal the original, instead, it is related to rotation!
            // AB: I am second on that! Looks like a bug in Flash.

            // NOTE: Although it works for many cases, this is still not correct. Modification 
            // of width seems very strange (if not buggy) in Flash.
            GASNumber wval = val.ToNumber(pEnv);
            if (val.IsUndefined() || GASNumberUtil::IsNaN(wval) || GASNumberUtil::IsNEGATIVE_INFINITY(wval))
                return 1;
            if (GASNumberUtil::IsPOSITIVE_INFINITY(wval))
                wval = 0;

            SetAcceptAnimMoves(0);
            GASSERT(pGeomData);

            Matrix m = pGeomData->OrigMatrix;
            const Matrix& chm = GetMatrix();
            m.M_[0][2] = chm.M_[0][2];
            m.M_[1][2] = chm.M_[1][2];

            Matrix im = m;
            im.AppendRotation(Float(-m.GetRotation() + pGeomData->Rotation * GFC_MATH_PI / 180.));

            Float oldWidth      = GetBounds(im).Width(); // width should be in local coords!
            Float newWidth      = Float(PixelsToTwips(wval));
            Float multiplier    = (fabsf(oldWidth) > 1e-6f) ? (newWidth / oldWidth) : 0.0f;

            Double origXScale = m.GetXScale();
            Double newXScale = origXScale * multiplier * 100;
            pGeomData->XScale = newXScale;
            if (origXScale == 0)
            {
                newXScale = 0;
                origXScale = 1;
            }
        
            GFxASCharacter_MatrixScaleAndRotate2x2(m,
                Float(fabs(newXScale/(origXScale*100.))), 
                Float(fabs(pGeomData->YScale/(m.GetYScale()*100.))),
                Float(-m.GetRotation() + pGeomData->Rotation * GFC_MATH_PI / 180.));

            pGeomData->XScale = fabs(pGeomData->XScale);
            pGeomData->YScale = fabs(pGeomData->YScale);

            if (m.IsValid())
                SetMatrix(m);
            return 1;
        }

    case M_height:
        {
            GASNumber hval = val.ToNumber(pEnv);
            if (val.IsUndefined() || GASNumberUtil::IsNaN(hval) || GASNumberUtil::IsNEGATIVE_INFINITY(hval))
                return 1;
            if (GASNumberUtil::IsPOSITIVE_INFINITY(hval))
                hval = 0;

            SetAcceptAnimMoves(0);
            GASSERT(pGeomData);

            Matrix m = pGeomData->OrigMatrix;
            const Matrix& chm = GetMatrix();
            m.M_[0][2] = chm.M_[0][2];
            m.M_[1][2] = chm.M_[1][2];

            Matrix im = m;
            im.AppendRotation(Float(-m.GetRotation() + pGeomData->Rotation * GFC_MATH_PI / 180.));

            Float oldHeight     = GetBounds(im).Height(); // height should be in local coords!
            Float newHeight     = Float(PixelsToTwips(hval));
            Float multiplier    = (fabsf(oldHeight) > 1e-6f) ? (newHeight / oldHeight) : 0.0f;

            Double origYScale = m.GetYScale();
            Double newYScale = origYScale * multiplier * 100;;
            pGeomData->YScale = newYScale;
            if (origYScale == 0)
            {
                newYScale = 0;
                origYScale = 1;
            }

            GFxASCharacter_MatrixScaleAndRotate2x2(m,
                Float(fabs(pGeomData->XScale/(m.GetXScale()*100.))), 
                Float(fabs(newYScale/(origYScale* 100.))),
                Float(-m.GetRotation() + pGeomData->Rotation * GFC_MATH_PI / 180.));

            pGeomData->XScale = fabs(pGeomData->XScale);
            pGeomData->YScale = fabs(pGeomData->YScale);

            if (m.IsValid())
                SetMatrix(m);
            return 1;
        }
    

    case M_alpha:
        {
            // NOTE: If updating this logic, also update GFxValue::ObjectInterface::SetDisplayInfo
            GASNumber aval = val.ToNumber(pEnv);
            if (val.IsUndefined() || GASNumberUtil::IsNaN(aval))
                return 1;

            // Set alpha modulate, in percent.
            Cxform  cx = GetCxform();
            cx.M_[3][0] = Float(aval / 100.);
            SetCxform(cx);
            SetAcceptAnimMoves(0);
            return 1;
        }

    case M_visible:
        {
            // NOTE: If updating this logic, also update GFxValue::ObjectInterface::SetDisplayInfo
            SetVisible(val.ToBool(pEnv));           
            // Setting visibility does not affect AnimMoves.
            return 1;
        }

    case M_blendMode:
        {
            // NOTE: Setting "blendMode" in Flash does NOT disconnect object from time-line,
            // so just setting the value is the correct behavior.
            if (val.GetType() == GASValue::STRING)
            {
                GASString  asstr(val.ToString(pEnv));
                GString  str = asstr.ToCStr();
                for (UInt i=1; i<(sizeof(GFx_BlendModeNames)/sizeof(GFx_BlendModeNames[0])); i++)
                {
                    if (str == GFx_BlendModeNames[i])
                    {
                        SetBlendMode((BlendType) i);
                        return 1;
                    }
                }
            }
            else
            {
                SInt mode = (SInt)val.ToNumber(pEnv);
                mode = G_Max<SInt>(G_Min<SInt>(mode,14),1);
                SetBlendMode((BlendType) mode);
            }
            return 1;
        }


    case M_name:
        {
            SetName(val.ToString(pEnv));
            return 1;
        }
        
    case M_enabled:
        {
            SetEnabledFlag(val.ToBool(pEnv));
            return 1;
        }       

    case M_trackAsMenu:
        {
            SetTrackAsMenuFlag(val.ToBool(pEnv));
            return 1;
        }

    // Read-only properties.
    case M_droptarget:
    case M_target:
    case M_url:
    case M_xmouse:
    case M_ymouse:  
    case M_parent: // TBD: _parent is not documented as read only. Can it be changed ??

    // These are only implemented for MovieClip. Technically, they should not be here;
    // however, they are Flash built-ins. Since they are read-only, we can handle them here.
    case M_currentframe:
    case M_totalframes:
    case M_framesloaded:        

        pEnv->LogScriptWarning("Attempt to write read-only property %s.%s, ignored\n",
            GetName().ToCStr(), GFxASCharacter::MemberTable[member].pName);
        return 1;

    case M_tabEnabled:
        if (!val.IsUndefined())
            SetTabEnabledFlag(val.ToBool(GetASEnvironment()));
        else 
            UndefineTabEnabledFlag();
        return 1;

    case M_tabIndex:
        TabIndex = SInt16(val.ToNumber(GetASEnvironment()));
        return 1;

    case M_focusrect:
        if (!val.IsUndefined())
            SetFocusRectFlag(val.ToBool(GetASEnvironment()));
        else 
            UndefineFocusRectFlag();
        SetDirtyFlag();
        return 1;

    // Un-implemented properties:
    case M_soundbuftime:
    case M_highquality:
    case M_quality:
        // We have a default get below, so return 1.
        return 1;

    case M_useHandCursor:
        if (!val.IsUndefined())
            SetUseHandCursorFlag(val.ToBool(GetASEnvironment()));
        else
            UndefineUseHandCursorFlag();
        return 1;

    case M_filters:
        {
#ifndef GFC_NO_FXPLAYER_AS_FILTERS
            GASObject* pobj = val.ToObject(pEnv);
            if (pobj && pobj->InstanceOf(pEnv, pEnv->GetPrototype(GASBuiltin_Array)))
            {
                GASArrayObject* parrobj = static_cast<GASArrayObject*>(pobj);
                GArray<GFxFilterDesc> newfilters;
                CharFilterDesc* pOldFilters = GetFilters();
                bool hasfilters = pOldFilters && pOldFilters->HasFilters;
                for (int i=0; i < parrobj->GetSize(); ++i)
                {
                    GASValue* arrval = parrobj->GetElementPtr(i);
                    if (arrval)
                    {
                        GASObject* pavobj = arrval->ToObject(pEnv);
                        if (pavobj && pavobj->InstanceOf(pEnv, pEnv->GetPrototype(GASBuiltin_BitmapFilter)))
                        {
                            GASBitmapFilterObject* pfilterobj = static_cast<GASBitmapFilterObject*>(pavobj);
                            newfilters.PushBack(pfilterobj->GetFilter());
                        }
                    }
                }
                SetFilters(newfilters);
                if (newfilters.GetSize() || hasfilters)
                {
                    SetDirtyFlag();
                    SetAcceptAnimMoves(false);
                }
            }
#endif  // GFC_NO_FXPLAYER_AS_FILTERS
            return true;
        }
        break;

    case M_cacheAsBitmap:
    case M_lockroot:
        // Allow at least property assignment in map for now.
        return 0;

    default:
        // Property not handled, fall out.
        return 0;
    }
}

bool    GFxASCharacter::GetStandardMember(StandardMember member, GASValue* val, bool opcodeFlag) const
{
    if (opcodeFlag && ((member < M_BuiltInProperty_Begin) || (member > M_BuiltInProperty_End)))
    {
        GetASEnvironment()->LogScriptError("Invalid GetProperty query, property number %d\n", member);
        return 0;
    }

    // Make sure that this character class supports the constant.
    if (member == M_InvalidMember)
        return 0;
    if (!((member <= M_SharedPropertyEnd) && (GetStandardMemberBitMask() & (1<<member))))
        return 0;

    switch(member)
    {
    case M_x:
        {           
            // NOTE: If updating this logic, also update GFxValue::ObjectInterface::GetDisplayInfo
            GeomDataType geomData;
            val->SetNumber(TwipsToPixels(Double(GetGeomData(geomData).X)));
            return 1;
        }
    case M_y:       
        {   
            // NOTE: If updating this logic, also update GFxValue::ObjectInterface::GetDisplayInfo
            GeomDataType geomData;
            val->SetNumber(TwipsToPixels(Double(GetGeomData(geomData).Y)));
            return 1;
        }

    case M_xscale:      
        {           
            // NOTE: If updating this logic, also update GFxValue::ObjectInterface::GetDisplayInfo
            GeomDataType geomData;
            val->SetNumber(GetGeomData(geomData).XScale);   // result in percent
            return 1;
        }
    case M_yscale:      
        {           
            // NOTE: If updating this logic, also update GFxValue::ObjectInterface::GetDisplayInfo
            GeomDataType geomData;
            val->SetNumber(GetGeomData(geomData).YScale);   // result in percent
            return 1;
        }

    case M_width:   
        {
            //GRectF    boundRect = GetLevelBounds();
            //!AB: width and height of nested movieclips returned in the coordinate space of its parent!
            GRectF  boundRect = GetBounds(GetMatrix());
            val->SetNumber(TwipsToPixels(floor((Double)boundRect.Width())));
            return 1;
        }
    case M_height:      
        {
            //GRectF    boundRect = GetLevelBounds();
            //!AB: width and height of nested movieclips returned in the coordinate space of its parent!
            GRectF  boundRect = GetBounds(GetMatrix());
            val->SetNumber(TwipsToPixels(floor((Double)boundRect.Height())));
            return 1;
        }

    case M_rotation:
        {
            // NOTE: If updating this logic, also update GFxValue::ObjectInterface::GetDisplayInfo
            GeomDataType geomData;
            // Verified against Macromedia player using samples/TestRotation.Swf
            val->SetNumber(GetGeomData(geomData).Rotation);
            return 1;
        }

    case M_alpha:   
        {
            // NOTE: If updating this logic, also update GFxValue::ObjectInterface::GetDisplayInfo
            // Alpha units are in percent.
            val->SetNumber(GetCxform().M_[3][0] * 100.F);
            return 1;
        }

    case M_visible: 
        {
            // NOTE: If updating this logic, also update GFxValue::ObjectInterface::GetDisplayInfo
            val->SetBool(GetVisible());
            return 1;
        }

    case M_blendMode:
        {
            val->SetString(GetASEnvironment()->CreateString(GFx_BlendModeNames[GetBlendMode()]));
            // Note: SWF 8 can sometimes report "undefined". TBD.
            return 1;
        }

    case M_name:
        {
            val->SetString(GetName());
            return 1;
        }
        
    case M_enabled:
        {           
            val->SetBool(GetEnabled());
            return 1;
        }   

    case M_trackAsMenu:
        {
            val->SetBool(GetTrackAsMenu());
            return 1;
        }

    case M_target:
        {
            // Full path to this object; e.G. "/sprite1/sprite2/ourSprite"
            GStringBuffer      name;
            GPtr<GFxASCharacter> root = GetASRootMovie();

            for (const GFxASCharacter* p = this; p != 0 && p != root; p = p->GetParent())
            {
                const GASString& cname = p->GetName();
                name.Insert (cname.ToCStr(), 0);
                name.Insert ("/", 0);
            }
            val->SetString(GetASEnvironment()->CreateString(name.ToCStr(),name.GetSize()));
            return 1;
        }

    case M_parent:
        {
            if (pParent)
                val->SetAsCharacter(pParent);
            else
                val->SetUndefined();
            return 1;
        }

    case M_droptarget:
        {
            // Absolute path in slash syntax where we were last Dropped (?)
            // @@ TODO
            //val->SetString(GetASEnvironment()->GetBuiltin(GASBuiltin__root));
            //return 1;

            // Crude implementation..
            val->SetUndefined();
            GFxMovieRoot* proot = GetASEnvironment()->GetMovieRoot();
            GPointF mousePos = proot->GetMouseState(0)->GetLastPosition();
            GFxASCharacter* ptopCh = proot->GetTopMostEntity(mousePos, 0 /* controller */, true, this);
            GStringBuffer      name;
            for (const GFxASCharacter* p = ptopCh; p != 0; p = p->GetParent())
            {
                const GASString& cname = p->GetName();
                name.Insert (cname.ToCStr(), 0);
                name.Insert ("/", 0);
            }
            val->SetString(GetASEnvironment()->CreateString(name.ToCStr(),name.GetSize()));
            return 1;
        }

    case M_url: 
        {
            // our URL.
            GArray<char>        urlArray;
            const char*         purl = GetResourceMovieDef()->GetFileURL();
            UPInt               urlSize = purl ? G_strlen(purl) : 0;
            urlArray.Resize(urlSize + 1);
                   
            for (UPInt i = 0; i < urlSize; ++i)
                urlArray[i] = (purl[i] == '\\') ? '/' : purl[i];
            urlArray[urlSize] = 0;
            
            GString urlStr;
            GASGlobalContext::EscapePath(&urlArray[0], urlSize, &urlStr);
            val->SetString(GetASEnvironment()->CreateString(urlStr));
            return 1;
        }

    case M_highquality: 
        {
            // Whether we're in high quality mode or not.
            val->SetBool(true);
            return 1;
        }
    case M_quality:
        {
            val->SetString(GetASEnvironment()->CreateString("HIGH"));
            return 1;
        }

    case M_focusrect:   
        {
            // Is a yellow rectangle visible around a focused GFxASCharacter Clip (?)
            if (IsFocusRectFlagDefined())
                val->SetBool(IsFocusRectFlagTrue());
            else
                val->SetNull();
            return 1;
        }

    case M_soundbuftime:        
        {
            // Number of seconds before sound starts to GFxStream.
            val->SetNumber(0.0);
            return 1;
        }

    case M_xmouse:  
        {
            // Local coord of mouse IN PIXELS.
            GPointF a = GetMovieRoot()->GetMouseState(0)->GetLastPosition();

            Matrix  m = GetWorldMatrix();
            GPointF b;
            
            m.TransformByInverse(&b, a);
            val->SetNumber(TwipsToPixels(floor(b.x+0.5)));
            return 1;
        }

    case M_ymouse:  
        {
            // Local coord of mouse IN PIXELS.
            GPointF a = GetMovieRoot()->GetMouseState(0)->GetLastPosition();

            Matrix  m = GetWorldMatrix();
            GPointF b;
            
            m.TransformByInverse(&b, a);
            val->SetNumber(TwipsToPixels(floor(b.y+0.5)));
            return 1;
        }

    case M_tabEnabled:
        if (IsTabEnabledFlagDefined())
            val->SetBool(IsTabEnabledFlagTrue());
        else
            val->SetUndefined();
        return 1;

    case M_tabIndex:
        val->SetNumber((GASNumber)TabIndex);
        return 1;

    case M_useHandCursor:
        if (!IsUseHandCursorFlagDefined())
            return 0;
        val->SetBool(IsUseHandCursorFlagTrue());
        return 1;

    case M_filters:
        {
#ifndef GFC_NO_FXPLAYER_AS_FILTERS
            GASEnvironment* penv = const_cast<GASEnvironment*>(GetASEnvironment());
            GPtr<GASArrayObject> arrayObj = *GHEAP_NEW(penv->GetHeap()) GASArrayObject(penv);
            CharFilterDesc* pFilters = GetFilters();
            if (pFilters && pFilters->HasFilters)
            {
                UInt lastFilter = (UInt)pFilters->Filters.GetSize();
                if (pFilters->LastIsTemp)
                    lastFilter--;
                for (UInt i = 0 ; i < lastFilter; i++)
                {
                    GASBitmapFilterObject* pfilterobj = GASBitmapFilterObject::CreateFromDesc(penv, pFilters->Filters[i]);
                    if (pfilterobj)
                    {
                        arrayObj->PushBack(pfilterobj);
                        pfilterobj->Release();
                    }
                }
            }
            val->SetAsObject(arrayObj);
#endif  // GFC_NO_FXPLAYER_AS_FILTERS
        }
        return 1;

    // Un-implemented properties:
    case M_cacheAsBitmap:
    case M_lockroot:
        // Allow at least property assignment in map for now.
        return 0;

    // These are only implemented for MovieClip. Technically, they should not be here;
    // however, they are Flash built-ins. Return 0 so that GFxSprite properly handles them.
    case M_currentframe:
    case M_totalframes:
    case M_framesloaded:
        return 0;   

    default:
        break;
    }

    return 0;
}



// Duplicate the object with the specified name and add it with a new name at a new depth.
GFxASCharacter* GFxASCharacter::CloneDisplayObject(const GASString& newname, SInt depth, const GASObjectInterface *psource)
{
    GFxSprite* pparent = GetParent()->ToSprite();
    if (!pparent)
        return 0;
    if ((depth < 0) || (depth > (2130690045 + 16384)))
        return 0;

    // Clone us.    
    GFxCharPosInfo pos( GetId(), depth,
                        1, GetCxform(), 1, GetMatrix(),
                        GetRatio(), GetClipDepth());

    // true: replace if depth is occupied
    GFxCharacter* pnewCh = pparent->AddDisplayObject(
        pos, newname, 0, psource, 
        GFC_MAX_UINT, GFxDisplayList::Flags_ReplaceIfDepthIsOccupied, 0, this);
    if (pnewCh)
        return pnewCh->CharToASCharacter();
    return NULL;
}

// Remove the object with the specified name.
void    GFxASCharacter::RemoveDisplayObject()
{
    if (!GetParent())
        return;
    GFxSprite* pparent = GetParent()->ToSprite();
    if (!pparent)
        return; 
    pparent->RemoveDisplayObject(GetDepth(), GetId());
}


void    GFxASCharacter::CopyPhysicalProperties(const GFxASCharacter *poldChar)
{
    // Copy physical properties, used by loadMovie().
    SetDepth(poldChar->GetDepth());
    SetCxform(poldChar->GetCxform());
    SetMatrix(poldChar->GetMatrix());
    EventHandlers = poldChar->EventHandlers;
    if (poldChar->pGeomData)
        SetGeomData(*poldChar->pGeomData);

}

void    GFxASCharacter::MoveNameHandle(GFxASCharacter *poldChar)
{
    // Re-link all ActionScript references.
    pNameHandle = poldChar->pNameHandle;
    poldChar->pNameHandle = 0;
    if (pNameHandle)
        pNameHandle->pCharacter = this;
}

static GINLINE GFxEventId GFx_TreatEventId(const GFxEventId& id)
{
    // for keyDown/keyUp search by id only (w/o KeyCode/AsciiCode)
    if (id.Id == GFxEventId::Event_KeyDown || id.Id == GFxEventId::Event_KeyUp)
        return GFxEventId(id.Id); 
    return id;
}

bool    GFxASCharacter::HasClipEventHandler(const GFxEventId& id) const
{ 
    const EventsArray* evts = EventHandlers.Get(GFx_TreatEventId(id));
    return evts != 0; 
}

bool    GFxASCharacter::InvokeClipEventHandlers(GASEnvironment* penv, const GFxEventId& id)
{ 
    const EventsArray* evts = EventHandlers.Get(GFx_TreatEventId(id));
    if (evts)
    {
        for (UPInt i = 0, n = evts->GetSize(); i < n; ++i)
        {
            const GASValue& method = (*evts)[i];
            GAS_Invoke0(method, NULL, this, penv);
        }
        return true;
    }
    return false; 
}

void    GFxASCharacter::SetSingleClipEventHandler(const GFxEventId& id, const GASValue& method)
{ 
    GASSERT(id.GetEventsCount() == 1);
    EventsArray* evts = EventHandlers.Get(GFx_TreatEventId(id));
    if (!evts)
    {
        // Local arrays need to be allocated for the right heap.
        EventsArray *pea = GHEAP_AUTO_NEW(this) EventsArray;
        pea->PushBack(method);               
        EventHandlers.Set(id, *pea); 
        delete pea;
    }
    else
    {
        evts->PushBack(method);
    }
}

void    GFxASCharacter::SetClipEventHandlers(const GFxEventId& id, const GASValue& method)
{ 
    UInt numOfEvents = id.GetEventsCount();
    GASSERT(numOfEvents > 0);
    if (numOfEvents == 1)
    {
        SetSingleClipEventHandler(id, method);
    }
    else
    {
        // need to create multiple event handlers with the same method. This is
        // necessary when "on(release, releaseOutside)" kind of handlers is used.
        for (UInt i = 0, mask = 0x1; i < numOfEvents; mask <<= 1)
        {
            if (id.Id & mask)
            {
                ++i;
                GFxEventId copied = id;
                copied.Id = mask;
                SetSingleClipEventHandler(copied, method);
            }
        }
    }
}


// Execute this even immediately (called for processing queued event actions).
bool    GFxASCharacter::ExecuteEvent(const GFxEventId& id)
{
    // Keep GASEnvironment alive during any method calls!
    GPtr<GFxASCharacter> thisPtr(this);
    GASEnvironment* env = GetASEnvironment();
    // Keep target of GASEnvironment alive during any method calls!
    // note, that env->GetTarget() is not always equal to this (for buttons, for example)
    GPtr<GFxASCharacter> targetPtr(env->GetTarget());

#ifndef GFC_NO_KEYBOARD_SUPPORT
    if (env && (id.Id == GFxEventId::Event_KeyDown || id.Id == GFxEventId::Event_KeyUp))
    {
        // We need to update listeners of KeyboardState (actually, there is only one
        // listener - GASKeyAsObject). The UpdateListeners method updates last ascii and
        // key codes in the singleton GASKeyAsObject, thus, ActionScript's Key.getAscii()
        // and Key.getCode() will return actual values when they are being used inside the
        // onClipEvent(keyDown / keyUp) event handlers.
        GFxMovieRoot* proot = env->GetMovieRoot();
        if (proot)
        {
            GFxKeyboardState* keyboardState = proot->GetKeyboardState(id.KeyboardIndex);
            GASSERT(keyboardState);
            keyboardState->UpdateListeners(id.KeyCode, id.AsciiCode, id.WcharCode);
        }
    }
#endif //#ifndef GFC_NO_KEYBOARD_SUPPORT

    int         handlerFoundCnt = 0;
    GASValue    method;

    // First, check for built-in onClipEvent() handler.
    if (HasClipEventHandler(id))
    {
        if (id.RollOverCnt == 0)
        {
            // Dispatch.
            InvokeClipEventHandlers(env, id);
            ++handlerFoundCnt;
        }
    }

    // Check for member function, it is called after onClipEvent(). 
    // In ActionScript 2.0, event method names are CASE SENSITIVE.
    // In ActionScript 1.0, event method names are CASE INSENSITIVE.    
    GASString    methodName(id.GetFunctionName(env->GetSC()));
    if (methodName.GetSize() > 0)
    {           
        if (GetMemberRaw(env->GetSC(), methodName, &method))
        {
            if (method.IsProperty())
            {
                GASValue actualMethod;
                method.GetPropertyValue(env, thisPtr, &actualMethod);
                method = actualMethod;
            }
            if (!method.IsNull())
            {
                // check, do we need to pass mouse index as a parameter
                if (env->CheckExtensions())
                {
                    UInt nArgs = 0;
                    bool handlerFound = true;
                    if (id.RollOverCnt != 0)
                    {
                        // we need to check do we need to invoke nested drag/roll/Over/Out
                        // for multiple mice. If the corresponding handler function has 2
                        // or more parameters, then it is assumed to be ready for handling
                        // multiple rollovers, so, onRollOver/Out (and onDrag...) will be
                        // invoked for each mouse. If there are less than two parameters 
                        // (or, most likely, no parameters at all) then onRollOver will be
                        // invoked only once when the first mouse cursor appears above the clip
                        // and onRollOut will be invoked once when the last cursor leave the clip.
                        GASFunctionRef mfref = method.ToFunction(env);
                        if (!mfref.IsNull())
                        {
                            if (mfref->GetNumArgs() < 2)
                                handlerFound = false;
                        }
                    }

                    if (handlerFound)
                    {
                        ++handlerFoundCnt;
                        if (env->IsVerboseAction())
                            env->LogAction("\n!!! ExecuteEvent started '%s' = %p for %s\n", methodName.ToCStr(), 
                            method.ToFunction(env).GetObjectPtr(), GetCharacterHandle()->GetNamePath().ToCStr());

                        // Enable button index for applicable events (including all aux events)
                        if (id.Id & GFxEventId::Event_AuxEventMask || GFxEventId::Event_DragOver || 
                            id.Id == GFxEventId::Event_DragOut || id.Id == GFxEventId::Event_ReleaseOutside || 
                            id.Id == GFxEventId::Event_Release || id.Id == GFxEventId::Event_Press)
                        {
                            const GFxButtonEventId& btnEvtId = static_cast<const GFxButtonEventId&>(id);
                            env->Push(GASValue(btnEvtId.ButtonId));
                            ++nArgs;
                        }

                        if (id.Id == GFxEventId::Event_RollOver || id.Id == GFxEventId::Event_RollOut ||
                            id.Id == GFxEventId::Event_DragOver || id.Id == GFxEventId::Event_DragOut ||
                            id.Id == GFxEventId::Event_DragOverAux || id.Id == GFxEventId::Event_DragOutAux)
                        {
                            env->Push(GASValue(id.RollOverCnt));
                            ++nArgs;
                        }
                        else if (id.Id == GFxEventId::Event_Press || id.Id == GFxEventId::Event_Release ||
                                 id.Id == GFxEventId::Event_PressAux || id.Id == GFxEventId::Event_ReleaseAux)
                        {
                            // push a negative number if keyboard was used; 0, if mouse.
                            if (id.KeyCode == GFxKey::VoidSymbol)
                                env->Push(GASValue(0));
                            else
                                env->Push(GASValue(-1));
                            ++nArgs;
                        }
                        if (id.MouseIndex >= 0 || nArgs > 0)
                        {
                            env->Push(GASValue(id.MouseIndex));
                            ++nArgs;
                        }
                        GAS_Invoke(method, NULL, this, env, nArgs, env->GetTopIndex(), methodName.ToCStr());
                        if (nArgs > 0)
                            env->Drop(nArgs);

                        if (env->IsVerboseAction())
                            env->LogAction("!!! ExecuteEvent finished '%s' = %p for %s\n\n", methodName.ToCStr(), 
                            method.ToFunction(env).GetObjectPtr(), GetCharacterHandle()->GetNamePath().ToCStr());
                    }
                }
                else
                {
                    if (id.RollOverCnt == 0)
                    {
                        ++handlerFoundCnt;
                        if (env->IsVerboseAction())
                            env->LogAction("\n!!! ExecuteEvent started '%s' = %p for %s\n", methodName.ToCStr(), 
                            method.ToFunction(env).GetObjectPtr(), GetCharacterHandle()->GetNamePath().ToCStr());

                        GAS_Invoke0(method, NULL, this, env);

                        if (env->IsVerboseAction())
                            env->LogAction("!!! ExecuteEvent finished '%s' = %p for %s\n\n", methodName.ToCStr(), 
                            method.ToFunction(env).GetObjectPtr(), GetCharacterHandle()->GetNamePath().ToCStr());
                    }
                }
            }
        }
    }

    return handlerFoundCnt != 0;
}

bool    GFxASCharacter::ExecuteFunction(const GASFunctionRef& function, const GASValueArray& params)
{
    if (function.GetObjectPtr() != 0)
    {
        GASValue result;
        GASEnvironment* penv = GetASEnvironment();
        GASSERT(penv);

        int nArgs = (int)params.GetSize();
        if (nArgs > 0)
        {
            for (int i = nArgs - 1; i >= 0; --i)
                penv->Push(params[i]);
        }
        function.Invoke(GASFnCall(&result, this, penv, nArgs, penv->GetTopIndex()));
        if (nArgs > 0)
        {
            penv->Drop(nArgs);
        }
        return true;
    }
    return false;
}

bool    GFxASCharacter::ExecuteCFunction(const GASCFunctionPtr function, const GASValueArray& params)
{
    if (function != 0)
    {
        GASValue result;
        GASEnvironment* penv = GetASEnvironment();
        GASSERT(penv);

        int nArgs = (int)params.GetSize();
        if (nArgs > 0)
        {
            for (int i = nArgs - 1; i >= 0; --i)
                penv->Push(params[i]);
        }
        function(GASFnCall(&result, this, penv, nArgs, penv->GetTopIndex()));
        if (nArgs > 0)
        {
            penv->Drop(nArgs);
        }
        return true;
    }
    return false;
}

// set this.__proto__ = psrcObj->prototype
void    GFxASCharacter::SetProtoToPrototypeOf(GASObjectInterface* psrcObj)
{
    GASValue prototype;
    GASStringContext *psc = GetASEnvironment()->GetSC();
    if (psrcObj->GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_prototype), &prototype))
        Set__proto__(psc, prototype.ToObject(NULL));
}

void    GFxASCharacter::VisitMembers(GASStringContext *psc, 
                                     MemberVisitor *pvisitor, 
                                     UInt visitFlags, 
                                     const GASObjectInterface*) const
{
    GPtr<GASObject> asObj = GetASObject();
    if (asObj) 
        asObj->VisitMembers(psc, pvisitor, visitFlags, this); 
}


// Delete a member field, if not read-only. Return true if deleted.
bool    GFxASCharacter::DeleteMember(GASStringContext *psc, const GASString& name)
{
    if (IsStandardMember(name))
    {    
        StandardMember member = GetStandardMemberConstant(name);
        if (member != M_InvalidMember && (member <= M_SharedPropertyEnd))
        {
            if (GetStandardMemberBitMask() & (1<<member))
            {
                switch(member)
                {
                    case M_useHandCursor:
                        UndefineUseHandCursorFlag();
                        return true;

                    default:
                        return false;
                }
            }
        }
    }

    GPtr<GASObject> asObj = GetASObject();
    if (asObj) 
    {
        if (asObj->DeleteMember(psc, name))
            return true;
    }
    return false;
}

bool    GFxASCharacter::SetMemberFlags(GASStringContext *psc, const GASString& name, const UByte flags)
{
    GPtr<GASObject> asObj = GetASObject();
    if (asObj) 
        return asObj->SetMemberFlags(psc, name, flags); 
    //TODO: Standard members?... (AB)
    return false;
}

bool    GFxASCharacter::SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{    
#ifndef GFC_NO_3D
    void GFxValue_UpdateTransform(GFxASCharacter* pchar);
#endif
    if (IsStandardMember(name))
    {
        StandardMember member = GetStandardMemberConstant(name);
        if (SetStandardMember(member, val, 0))
            return true;

        switch(member)
        {
        case M_topmostLevel:
            if (GetASEnvironment()->CheckExtensions())
            {
                SetTopmostLevelFlag(val.ToBool(GetASEnvironment())); 
                if (IsTopmostLevelFlagSet())
                    GetMovieRoot()->AddTopmostLevelCharacter(this);
                else
                    GetMovieRoot()->RemoveTopmostLevelCharacter(this);
            }
            break; // do not return - need to save it to members too

        case M_noAdvance:
            if (GetASEnvironment()->CheckExtensions())
            {
#ifndef GFC_USE_OLD_ADVANCE
                bool noAdv = val.ToBool(GetASEnvironment());
                if (noAdv != IsNoAdvanceLocalFlagSet())
                {
                    SetNoAdvanceLocalFlag(noAdv);
                    ModifyOptimizedPlayList(GetMovieRoot());
                    GFxASCharacter* pparent = GetParent();
                    if (pparent && !pparent->IsNoAdvanceLocalFlagSet())
                        PropagateNoAdvanceLocalFlag();
                }
#else
                SetNoAdvanceLocalFlag(val.ToBool(GetASEnvironment()));
#endif
            }
            break; // do not return - need to save it to members too

#ifndef GFC_NO_3D
            // shadow code in GFxValueImpl.cpp SetDisplayInfo
        case M_perspfov:
            if (GetASEnvironment()->CheckExtensions())
            {
                // NOTE: If updating this logic, also update GFxValue::ObjectInterface::SetDisplayInfo
                GASNumber perspval = val.ToNumber(GetASEnvironment());
                if (val.IsUndefined() || GASNumberUtil::IsNaN(perspval))
                {
                    break;
                }
                if (GASNumberUtil::IsNEGATIVE_INFINITY(perspval) || GASNumberUtil::IsPOSITIVE_INFINITY(perspval))
                    perspval = 0;
                SetPerspectiveFOV(Float(perspval));
            }
            break;
        case M_z:
            if (GetASEnvironment()->CheckExtensions())
            {
                // NOTE: If updating this logic, also update GFxValue::ObjectInterface::SetDisplayInfo
                GASNumber zval = val.ToNumber(GetASEnvironment());
                if (val.IsUndefined() || GASNumberUtil::IsNaN(zval))
                {
                    break;
                }
                if (GASNumberUtil::IsNEGATIVE_INFINITY(zval) || GASNumberUtil::IsPOSITIVE_INFINITY(zval))
                    zval = 0;
                //SetAcceptAnimMoves(0);
                EnsureGeomDataCreated(); // let timeline anim continue
                GASSERT(pGeomData);

                pGeomData->Z = zval;
                GFxValue_UpdateTransform(this);
            }
            break;

        case M_zscale:
            if (GetASEnvironment()->CheckExtensions())
            {
                // NOTE: If updating this logic, also update GFxValue::ObjectInterface::SetDisplayInfo
                Double newZScale = val.ToNumber(GetASEnvironment());
                if (val.IsUndefined() || GASNumberUtil::IsNaN(newZScale) ||
                    GASNumberUtil::IsNEGATIVE_INFINITY(newZScale) || GASNumberUtil::IsPOSITIVE_INFINITY(newZScale))
                {
                    break;
                }
                //SetAcceptAnimMoves(0);
                EnsureGeomDataCreated(); // let timeline anim continue
                GASSERT(pGeomData);

                pGeomData->ZScale = newZScale;
                GFxValue_UpdateTransform(this);
            }
            break;

        case M_matrix3d:
            if (GetASEnvironment()->CheckExtensions())
            {
                // NOTE: If updating this logic, also update GFxValue::ObjectInterface::SetDisplayInfo

                GASObject *pObj = val.ToObject(penv);
                if (pObj && pObj->GetObjectType() != GASObjectInterface::Object_Array)
                {
                    break;
                }
                if (pObj == NULL)
                {
                    // remove 3D transform
                    if (pMatrix3D_1)
                    {
                        delete pMatrix3D_1;
                        pMatrix3D_1 = NULL;
                        EnsureGeomDataCreated(); // let timeline anim continue
                    }
                    break;
                }
                GASArrayObject *arrayObj = static_cast<GASArrayObject*>(pObj);
                GASSERT(arrayObj->GetSize()==16);
                GMatrix3D m;
                for(int i = 0, n = arrayObj->GetSize(); i < n; ++i)
                {
                    GASValue* elem = arrayObj->GetElementPtr(i);
                    if (elem && elem->IsNumber())
                        m.GetAsFloatArray()[i] = (Float)elem->ToNumber(penv);
                }
                //SetAcceptAnimMoves(0);
                EnsureGeomDataCreated(); // let timeline anim continue
                GASSERT(pGeomData);
                CreateMatrix3D();
                if (m.IsValid())
                    SetMatrix3D(m);
            }
            break;

        case M_yrotation:
        case M_xrotation:
            // shadow code in GFxValueImpl.cpp SetDisplayInfo
            if (GetASEnvironment()->CheckExtensions())
            {
                // NOTE: If updating this logic, also update GFxValue::ObjectInterface::SetDisplayInfo
                GASNumber rval = val.ToNumber(GetASEnvironment());
                if (val.IsUndefined() || GASNumberUtil::IsNaN(rval))
                {
                    break;
                }
                //SetAcceptAnimMoves(0);
                EnsureGeomDataCreated(); // let timeline anim continue
                GASSERT(pGeomData);

                Double r = fmod((Double)rval, (Double)360.);
                if (r > 180)
                    r -= 360;
                else if (r < -180)
                    r += 360;

                if (member == M_xrotation)
                    pGeomData->XRotation= r;
                else
                    pGeomData->YRotation= r;

                GFxValue_UpdateTransform(this);
            }
            break;
#endif
        case M_focusGroupMask:
            if (GetASEnvironment()->CheckExtensions())
            {
                if (val.IsUndefined())
                {
                    break;
                }
                // NOTE: If updating this logic, also update GFxValue::ObjectInterface::SetDisplayInfo
                UInt32 rval = val.ToUInt32(GetASEnvironment());
                FocusGroupMask = (UInt16)rval;
                PropagateFocusGroupMask(FocusGroupMask);
            }
            break;
        default:
            break;
        }
    }

    // a special case for setting __proto__ to a movieclip, button or textfield
    // need to set it into the character, not to the AS object, since
    // Get__proto__ is no more virtual.
    if (penv->IsCaseSensitive())
    {
        if (name == penv->GetBuiltin(GASBuiltin___proto__))
        {   
            if (val.IsSet())
                Set__proto__(penv->GetSC(), val.ToObject(NULL));
        }
    }
    else
    {
        name.ResolveLowercase();
        if (penv->GetBuiltin(GASBuiltin___proto__).CompareBuiltIn_CaseInsensitive_Unchecked(name))
        {   
            if (val.IsSet())
                Set__proto__(penv->GetSC(), val.ToObject(NULL));
        }
    }
    // Note that MovieClipObject will also track setting of button
    // handlers, i.e. 'onPress', etc.
    GASObject* asObj = GetASObject();
    if (asObj) 
        return asObj->SetMember(penv, name, val, flags);
    return false;
}

bool GFxASCharacter::HasMember(GASStringContext *psc, const GASString& name, bool inclPrototypes)
{
    if (IsStandardMember(name))
    {
        StandardMember member = GetStandardMemberConstant(name);
        if (member != M_InvalidMember && (member <= M_SharedPropertyEnd))
        {
            if (GetStandardMemberBitMask() & (1<<member))
                return true;
        }
    }    

    GPtr<GASObject> asObj = GetASObject();
    if (asObj) 
        return asObj->HasMember(psc, name, inclPrototypes); 
    return false;
}

bool GFxASCharacter::FindMember(GASStringContext *psc, const GASString& name, GASMember* pmember)
{
    GPtr<GASObject> obj = GetASObject();
    return (obj) ? obj->FindMember(psc, name, pmember) : false;
}

void            GFxASCharacter::Set__proto__(GASStringContext *psc, GASObject* protoObj)
{
    GPtr<GASObject> obj = GetASObject();
    if (obj)
    {
        obj->Set__proto__(psc, protoObj);
    }
    pProto = protoObj;
}

/*GASObject*      GFxASCharacter::Get__proto__()
{
    GPtr<GASObject> obj = GetASObject();
    return (obj) ? obj->Get__proto__() : NULL;
}*/

bool GFxASCharacter::InstanceOf(GASEnvironment* penv, const GASObject* prototype, bool inclInterfaces) const
{
    GPtr<GASObject> obj = GetASObject();
    return (obj) ? obj->InstanceOf(penv, prototype, inclInterfaces) : false;
}

bool GFxASCharacter::Watch(GASStringContext *psc, const GASString& prop, const GASFunctionRef& callback, const GASValue& userData)
{
    GPtr<GASObject> obj = GetASObject();
    return (obj) ? obj->Watch(psc, prop, callback, userData) : false;
}

bool GFxASCharacter::Unwatch(GASStringContext *psc, const GASString& prop)
{
    GPtr<GASObject> obj = GetASObject();
    return (obj) ? obj->Unwatch(psc, prop) : false;
}

bool GFxASCharacter::IsFocusRectEnabled() const
{
    if (IsFocusRectFlagDefined())
        return IsFocusRectFlagTrue();
    //!AB: _focusrect seems to ignore lockroot. that is why we specify "true" as a parameter
    // of GetASRootMovie.
    GFxASCharacter* prootMovie = this->GetASRootMovie(true);
    if (prootMovie != this)
        return prootMovie->IsFocusRectEnabled();
    return true;
}

void GFxASCharacter::OnFocus(FocusEventType event, GFxASCharacter* oldOrNewFocusCh, UInt controllerIdx, GFxFocusMovedType)
{
    GASValue focusMethodVal;
    
    GASEnvironment* penv = GetASEnvironment();
    if (!penv) // penv == NULL if object is removed from its parent's display list
        return;
    GASStringContext* psc = penv->GetSC();
    GASString   focusMethodName(psc->GetBuiltin((event == SetFocus) ?
                                GASBuiltin_onSetFocus : GASBuiltin_onKillFocus));   
    if (GetMemberRaw(psc, focusMethodName, &focusMethodVal))
    {
        GASFunctionRef focusMethod = focusMethodVal.ToFunction(NULL);
        if (!focusMethod.IsNull())
        {
            UInt nargs = 1;
            if (penv->CheckExtensions())
            {
                penv->Push(GASNumber(controllerIdx));
                ++nargs;
            }
            if (oldOrNewFocusCh)
                penv->Push(GASValue(oldOrNewFocusCh));
            else
                penv->Push(GASValue::NULLTYPE);
            GASValue result;
            focusMethod.Invoke(GASFnCall(&result, GASValue(this), penv, nargs, penv->GetTopIndex()));
            penv->Drop(nargs);
        }
    }
}

void  GFxASCharacter::SetFilters(const GArray<GFxFilterDesc> f)
{
    GFxMovieRoot* proot = GetMovieRoot();

    if (!pFilters)
    {
        if (f.GetSize() == 0)
            return;
        pFilters = GHEAP_AUTO_NEW(this) CharFilterDesc(f);
    }
    else
        pFilters->Filters = f;
    pFilters->LastIsTemp = 0;
    pFilters->HasFilters = (!proot->pRenderConfig
        || (proot->pRenderConfig->GetRendererCapBits() & GRenderer::Cap_RenderTargets)) ? (f.GetSize() > 0 ? 1 : 0) : 0;

#ifndef GFC_NO_FILTERS_PREPASS
    bool oldPrePass = (Flags & Mask_NeedPreDisplay) != 0;
    bool newPrePass = proot->pRenderConfig && GFxFilterDesc::UsesRenderTarget(f) && 
        (proot->pRenderConfig->GetRendererCapBits() & GRenderer::Cap_RenderTargetPrePass);
    if (newPrePass)
    {
        pFilters->HasFilters = 1;
        pFilters->MainPassFilters.Clear();
        // a single pass filter can be processed in the main pass
        if (!f.Back().UsesBlurFilter() ||
            !(proot->pRenderConfig->GetRenderer()->CheckFilterSupport(f.Back().Blur) & GRenderer::FilterSupport_Multipass))
            pFilters->MainPassFilters.PushBack(f.Back());
        else
        {
            // Set a ColorMatrix of 1 to render the prepass results in the main pass
            GFxFilterDesc null;
            null.SetIdentity();
            pFilters->MainPassFilters.PushBack(null);
            pFilters->Filters.PushBack(null);
            pFilters->LastIsTemp = 1;
        }

        if (!oldPrePass)
            AddToPreDisplayList(proot);
    }
    else if (oldPrePass)
    {
        pFilters->MainPassFilters.Clear();
        pFilters->Filters.Clear();
        RemoveFromPreDisplayList(proot);
    }
#endif
}

void GFxASCharacter::OnRendererChanged()
{
    if (pFilters)
    {
        if (pFilters->LastIsTemp)
            pFilters->Filters.PopBack();
        SetFilters(pFilters->Filters);
    }
}

void GFxASCharacter::AddToPreDisplayList(GFxMovieRoot* proot)
{
    if (!IsInPreDisplayList())
    {
        GASSERT(proot);
        proot->AddToPreDisplayList(this);
        Flags |= Mask_NeedPreDisplay;

        // children must be processed before parents, so move parent to the end
        GFxCharacter* pparent = GetParent();
        while (pparent)
        {
            GFxASCharacter* pparentAS = pparent->CharToASCharacter();
            if (pparentAS && pparentAS->IsInPreDisplayList())
            {
                pparentAS->RemoveFromPreDisplayList(proot);
                pparentAS->AddToPreDisplayList(proot);
                break;
            }
            pparent = pparent->GetParent();
        }
    }
}

void GFxASCharacter::RemoveFromPreDisplayList(GFxMovieRoot* proot)
{
    if (IsInPreDisplayList())
    {
        GASSERT(proot);
        proot->RemoveFromPreDisplayList(this);
        Flags &= ~Mask_NeedPreDisplay;
    }
}


// ***** GFxGenericCharacter

GFxGenericCharacter::GFxGenericCharacter(GFxCharacterDef* pdef, GFxASCharacter* pparent, GFxResourceId id)
: GFxCharacter(pparent, id)
{
    pDef = pdef;
    GASSERT(pDef);
}

GFxASCharacter*  GFxGenericCharacter::GetTopMostMouseEntity(const GPointF &pt, const TopMostParams& params)
{   
    if (!GetVisible())
        return 0;

    Matrix  m = GetMatrix();
    GPointF p;

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
        GetMovieRoot()->ScreenToWorld.GetWorldPoint(&p);
    }
    else
#endif
    {
        m.TransformByInverse(&p, pt);
    }

    if ((ClipDepth == 0) && pDef->DefPointTestLocal(p, 1, this))
    {
        // The mouse is inside the shape.
//        printf("INSIDE %s\n", pParent ? pParent->GetName().ToCStr() : "NULL");
        GFxASCharacter* pParent = GetParent();
        while (pParent && pParent->IsSprite())
        {
            // Parent sprite would have to be in button mode to grab attention.
            GFxSprite * psprite = (GFxSprite*)pParent;
            if (params.TestAll || psprite->ActsAsButton() 
                || (psprite->GetHitAreaHolder() && psprite->GetHitAreaHolder()->ActsAsButton()))
            {
                // Check if sprite should be ignored
                if (!params.IgnoreMC || (params.IgnoreMC != psprite))
                    return psprite;
            }
            pParent = pParent->GetParent ();
        }
    }
    else
    {
//        printf("outside %s\n", pParent ? pParent->GetName().ToCStr() : "NULL");
    }
    return 0;
}

bool GFxGenericCharacter::PointTestLocal(const GPointF &pt, UInt8 hitTestMask) const
{     
    return pDef->DefPointTestLocal(pt, hitTestMask & HitTest_TestShape, this);
}

