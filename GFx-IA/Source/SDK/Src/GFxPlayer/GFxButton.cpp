/**********************************************************************

Filename    :   GFxButton.cpp
Content     :   Implementation of Button character and its AS2 Button class
Created     :   
Authors     :   Michael Antonov, Artem Bolgar
Notes       :   

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.


Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxButton.h"

#include "GRenderer.h"

#include "GFxAction.h"
#include "GFxSound.h" 
#include "GFxStream.h"
#include "GFxFontResource.h"
#include "GFxPlayerImpl.h"
#include "GFxLoaderImpl.h"
#include "GFxSprite.h"
#include "AS/GASRectangleObject.h"
//#include "GFxLoader.h"
#include "GFxAudio.h"
#include "GFxSoundTagsReader.h"

#include "GFxLoadProcess.h"
#include "GFxDisplayContext.h"

void GFx_FindClassAndInitializeClassInstance(const GASFnCall& fn);
/*

Observations about button & mouse behavior

Entities that receive mouse events: only buttons and sprites, AFAIK

When the mouse button goes down, it becomes "captured" by whatever
element is topmost, directly below the mouse at that moment.  While
the mouse is captured, no other entity receives mouse events,
regardless of how the mouse or other elements move.

The mouse remains captured until the mouse button goes up.  The mouse
remains captured even if the element that captured it is removed from
the display list.

If the mouse isn't above a button or sprite when the mouse button goes
down, then the mouse is captured by the Background (i.E. mouse events
just don't get sent, until the mouse button goes up again).

MA: The only exception to this is Menu mode button, which will get
dragOver/dragOut and release even if the inital click was outside
of the button (unless it was in a NON-menu button initally, in
which case that button would capture the mouse).

Mouse events:

+------------------+---------------+-------------------------------------+
| Event            | Mouse Button  | description                         |
=========================================================================
| onRollOver       |     up        | sent to topmost entity when mouse   |
|                  |               | cursor initially goes over it       |
+------------------+---------------+-------------------------------------+
| onRollOut        |     up        | when mouse leaves entity, after     |
|                  |               | onRollOver                          |
+------------------+---------------+-------------------------------------+
| onPress          |  up -> down   | sent to topmost entity when mouse   |
|                  |               | button goes down.  onRollOver       |
|                  |               | always precedes onPress.  Initiates |
|                  |               | mouse capture.                      |
+------------------+---------------+-------------------------------------+
| onRelease        |  down -> up   | sent to active entity if mouse goes |
|                  |               | up while over the element           |
+------------------+---------------+-------------------------------------+
| onDragOut        |     down      | sent to active entity if mouse      |
|                  |               | is no longer over the entity        |
+------------------+---------------+-------------------------------------+
| onReleaseOutside |  down -> up   | sent to active entity if mouse goes |
|                  |               | up while not over the entity.       |
|                  |               | onDragOut always precedes           |
|                  |               | onReleaseOutside                    |
+------------------+---------------+-------------------------------------+
| onDragOver       |     down      | sent to active entity if mouse is   |
|                  |               | dragged back over it after          |
|                  |               | onDragOut                           |
+------------------+---------------+-------------------------------------+

There is always one active entity at any given Time (considering NULL to
be an active entity, representing the background, and other objects that
don't receive mouse events).

When the mouse button is up, the active entity is the topmost element
directly under the mouse pointer.

When the mouse button is down, the active entity remains whatever it
was when the button last went down.

The active entity is the only object that receives mouse events.

!!! The "trackAsMenu" property alters this behavior!  If trackAsMenu
is set on the active entity, then onReleaseOutside is filtered out,
and onDragOver from another entity is Allowed (from the background, or
another trackAsMenu entity). !!!

*/


void    GFx_GenerateMouseButtonEvents(UInt mouseIndex, GFxMouseState* ms, UInt checkCount)
{
    GPtr<GFxASCharacter>    ActiveEntity  = ms->GetActiveEntity();
    GPtr<GFxASCharacter>    TopmostEntity = ms->GetTopmostEntity();

    UInt changeMask = ms->GetPrevButtonsState() ^ ms->GetButtonsState();
    bool suppressRollOut = false;
    bool miel = ms->IsMouseInsideEntityLast();
    for (UInt8 buttonIdx = 0; buttonIdx < checkCount; ++buttonIdx)   // PPS: 16 is max number of mouse buttons supported
    {
        bool genAuxEvent = buttonIdx != 0;
        UInt8 buttonMask = 1 << buttonIdx;

        // Handle button state change
        if ( ((changeMask >> buttonIdx) & 0x1) != 0 )
        {
            // Button was down
            if (ms->GetPrevButtonsState() & buttonMask)
            {
                // Button is now up
                // Handle onRelease, onReleaseOutside
                if ((ms->GetButtonsState() & buttonMask) == 0)
                {
                    if (ActiveEntity)
                    {
                        if (ms->IsMouseInsideEntityLast())
                        {
                            // onRelease
                            ActiveEntity->OnButtonEvent(GFxButtonEventId(genAuxEvent ? GFxEventId::Event_ReleaseAux : GFxEventId::Event_Release, 
                                mouseIndex, 0, buttonIdx));
                        }
                        else
                        {
                            // onReleaseOutside
                            if (!ActiveEntity->GetTrackAsMenu())
                                ActiveEntity->OnButtonEvent(GFxButtonEventId(genAuxEvent ? GFxEventId::Event_ReleaseOutsideAux : GFxEventId::Event_ReleaseOutside, 
                                mouseIndex, 0, buttonIdx));                       
                            // Prevent Event_RollOut event below.
                            suppressRollOut = true;
                        }
                    }
                }
            }
            // Button was up
            if ((ms->GetPrevButtonsState() & buttonMask) == 0)
            {
                // Button is now down
                if ((ms->GetButtonsState() & buttonMask))
                {
                    // onPress
                    if (ActiveEntity)
                        ActiveEntity->OnButtonEvent(GFxButtonEventId(genAuxEvent ? GFxEventId::Event_PressAux : GFxEventId::Event_Press, mouseIndex, 0, buttonIdx));

                    miel = true;
                }
            }
        }
        else if (ms->GetButtonsState() & buttonMask)
        {
            // Handle buttons-specific continous polling states
            // Handle onDragOut, onDragOver
            if (!ms->IsMouseInsideEntityLast())
            {
                if (TopmostEntity == ActiveEntity)
                {
                    // onDragOver
                    if (ActiveEntity)  
                        ActiveEntity->OnButtonEvent(GFxButtonEventId
                        (genAuxEvent ? GFxEventId::Event_DragOverAux : GFxEventId::Event_DragOver, 
                        mouseIndex, ActiveEntity->IncrementRollOverCnt(), buttonIdx));

                    miel = true;
                }
            }
            else
            {
                // MouseInsideEntityLast == true
                if (TopmostEntity != ActiveEntity)
                {
                    // onDragOut
                    if (ActiveEntity)
                    {
                        ActiveEntity->OnButtonEvent(GFxButtonEventId
                            (genAuxEvent ? GFxEventId::Event_DragOutAux : GFxEventId::Event_DragOut, 
                            mouseIndex, ActiveEntity->DecrementRollOverCnt(), buttonIdx));
                    }
                    miel = false;
                }
            }
            // Handle trackAsMenu dragOver
            if (!ActiveEntity
                || ActiveEntity->GetTrackAsMenu())
            {
                if (TopmostEntity
                    && TopmostEntity != ActiveEntity
                    && TopmostEntity->GetTrackAsMenu())
                {
                    // Transfer to topmost entity, dragOver
                    ActiveEntity = TopmostEntity;
                    ActiveEntity->OnButtonEvent(GFxButtonEventId(genAuxEvent ? GFxEventId::Event_DragOverAux : GFxEventId::Event_DragOver, 
                        mouseIndex, 
                        ActiveEntity->IncrementRollOverCnt(), buttonIdx));
                    miel = true;
                }
            }
        }
    }
    // Handle non-button specific continuous polling states
    // New active entity is whatever is below the mouse right now.
    if (ms->GetButtonsState() == 0 && TopmostEntity != ActiveEntity)
    {
        // onRollOut
        if (!suppressRollOut && ActiveEntity)
            ActiveEntity->OnButtonEvent(GFxButtonEventId
            (GFxEventId::Event_RollOut, mouseIndex, ActiveEntity->DecrementRollOverCnt()));              

        ActiveEntity = TopmostEntity;

        // onRollOver
        if (ActiveEntity)
            ActiveEntity->OnButtonEvent(GFxButtonEventId
            (GFxEventId::Event_RollOver, mouseIndex, ActiveEntity->IncrementRollOverCnt()));

        miel = true;
    }

    ms->SetMouseInsideEntityLast(miel);

    // Write The (possibly modified) GPtr copies back
    // into the state struct.
    ms->SetActiveEntity(ActiveEntity);
}


class GFxButtonCharacter : public GFxASCharacter
{
public:
    GFxButtonCharacterDef*              pDef;
    GFxScale9Grid*                      pScale9Grid;

    GArrayLH<GPtr<GFxCharacter> >       RecordCharacter;

    enum MouseFlags
    {
        IDLE = 0,
        FLAG_OVER = 1,
        FLAG_DOWN = 2,
        OVER_DOWN = FLAG_OVER|FLAG_DOWN,

        // aliases
        OVER_UP = FLAG_OVER,
        OUT_DOWN = FLAG_DOWN
    };

    int             LastMouseFlags, MouseFlags;
    GFxButtonRecord::MouseState   MouseState;

    GPtr<GASButtonObject> ASButtonObj;

    GFxButtonCharacter(GFxButtonCharacterDef* def, GFxMovieDefImpl *pbindingDefImpl,
                       GFxASCharacter* parent, GFxResourceId id)
        :
        GFxASCharacter(pbindingDefImpl, parent, id),
        pDef(def),
        pScale9Grid(0),
        LastMouseFlags(IDLE),
        MouseFlags(IDLE),
        MouseState(GFxButtonRecord::MouseUp)
    {
        GASSERT(pDef);
        
        SetScale9Grid(def->GetScale9Grid());
        SetTrackAsMenuFlag(pDef->Menu);

        RecordCharacter.Resize(pDef->ButtonRecords.GetSize());

        //ASButtonObj = *GHEAP_AUTO_NEW(this) GASButtonObject(GetGC(), this);
        // let the base class know what's going on
        //pProto = ASButtonObj->Get__proto__();
        pProto = GetGC()->GetActualPrototype(GetASEnvironment(), GASBuiltin_Button);

        RecreateCharacters();
    }

    ~GFxButtonCharacter()
    {
        delete pScale9Grid;
    }

    inline void SetDirtyFlag()
    {
        GetMovieRoot()->SetDirtyFlag();
    }

    virtual GFxCharacterDef* GetCharacterDef() const
    {
        return pDef;
    }

    // Default implementation of these is ok for buttons.
    // In the future, however, objects will have to consider _lockroot (although this may not affect buttons).
    //virtual GFxMovieDef*  GetResourceMovieDef() const         { return pParent->GetResourceMovieDef(); }
    //virtual GFxMovieRoot* GetMovieRoot() const                { return pParent->GetMovieRoot(); }
    //virtual GFxMovieSub*  GetLevelMovie(SInt level) const     { return pParent->GetLevelMovie(level); }
    //virtual GFxMovieSub*  GetRootMovie2() const               { return pParent->GetRootMovie2(); }



    void SetScale9Grid(const GFxScale9Grid* gr)
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
                pScale9Grid = GHEAP_AUTO_NEW(this) GFxScale9Grid;
            *pScale9Grid = *gr;
            SetScale9GridExists(true);
        }
        SetDirtyFlag();
        if (propagate)
            PropagateScale9GridExists();
    }

    void PropagateScale9GridExists()
    {
        bool actualGrid = GetScale9Grid() != 0;
        // Stop cleaning up scale9Grid if actual one exists in the node
        if (!DoesScale9GridExist() && actualGrid)
            return; 
        
        for (UInt i = 0; i < RecordCharacter.GetSize(); i++)
        {
            GFxCharacter* ch = RecordCharacter[i];
            if (!ch)
                continue;

            ch->SetScale9GridExists(DoesScale9GridExist() || actualGrid);
            ch->PropagateScale9GridExists();
        }   
    }

#ifndef GFC_USE_OLD_ADVANCE
    void PropagateNoAdvanceGlobalFlag()
    {
        bool actualValue = IsNoAdvanceGlobalFlagSet();
        GFxMovieRoot* proot = GetMovieRoot();
        if (!proot)
            return;

        for (UInt i = 0; i < RecordCharacter.GetSize(); i++)
        {
            GFxCharacter* _ch = RecordCharacter[i];
            if (!_ch)
                continue;
            GFxASCharacter* ch = _ch->CharToASCharacter();
            if (ch)
            {
                ch->SetNoAdvanceGlobalFlag(IsNoAdvanceGlobalFlagSet() || actualValue);
                ch->PropagateNoAdvanceGlobalFlag();
                ch->ModifyOptimizedPlayList(proot);
            }
        }   
    }

    void PropagateNoAdvanceLocalFlag()
    {
        bool actualValue = IsNoAdvanceLocalFlagSet();
        GFxMovieRoot* proot = GetMovieRoot();
        if (!proot)
            return;

        for (UInt i = 0; i < RecordCharacter.GetSize(); i++)
        {
            GFxCharacter* _ch = RecordCharacter[i];
            if (!_ch)
                continue;
            GFxASCharacter* ch = _ch->CharToASCharacter();
            if (ch)
            {
                ch->SetNoAdvanceLocalFlag(IsNoAdvanceLocalFlagSet() || actualValue);
                ch->PropagateNoAdvanceLocalFlag();
                ch->ModifyOptimizedPlayList(proot);
            }
        }   
    }
#endif //#ifndef GFC_USE_OLD_ADVANCE

    void SetVisible(bool visible)            
    { 
        SetVisibleFlag(visible); 
        GFxMovieRoot* proot = GetMovieRoot();
        if (!proot)
            return;
#ifndef GFC_USE_OLD_ADVANCE
        bool noAdvGlob = !visible && proot->IsNoInvisibleAdvanceFlagSet();
        if (noAdvGlob != IsNoAdvanceGlobalFlagSet())
        {
            SetNoAdvanceGlobalFlag(noAdvGlob);
            ModifyOptimizedPlayListLocal<GFxButtonCharacter>(proot);
            GFxASCharacter* pparent = GetParent();
            if (pparent && !pparent->IsNoAdvanceGlobalFlagSet())
                PropagateNoAdvanceGlobalFlag();
        }
#else
        SetNoAdvanceGlobalFlag(!visible && proot->IsNoInvisibleAdvanceFlagSet());
#endif
        SetDirtyFlag(); 
    }

    void    Restart()
    {
        LastMouseFlags = IDLE;
        MouseFlags = IDLE;
        MouseState = GFxButtonRecord::MouseUp;
        RollOverCnt = 0;
        UPInt r, R_num = RecordCharacter.GetSize();
        for (r = 0; r < R_num; r++)
        {
            if (RecordCharacter[r])
                RecordCharacter[r]->Restart();
        }
        SetDirtyFlag();
    }

    // Obtains an active button record based on state, or -1.
    // Note: not quite correct, since multiple records can apply to a state.
    int     GetActiveRecordIndex() const
    {
        return GetRecordIndex(MouseState);
    }
    int     GetRecordIndex(GFxButtonRecord::MouseState mouseState) const
    {
        for (UInt i = 0; i < pDef->ButtonRecords.GetSize(); i++)
        {
            GFxButtonRecord& rec = pDef->ButtonRecords[i];
            if (!RecordCharacter[i])                
                continue;               
            if (rec.MatchMouseState(mouseState))
            {
                return i;
            }
        }
        // Not found
        return -1;
    }

    void OnEventUnload()
    {
        for (UInt i = 0; i < pDef->ButtonRecords.GetSize(); i++)
        {
            UnloadCharacterAtIndex(i); 
        }
        GFxASCharacter::OnEventUnload();
    }

#ifdef GFC_USE_OLD_ADVANCE
    int CheckAdvanceStatus(bool playingNow) { return (playingNow) ? -1 : 0; }

    virtual void    AdvanceFrame(bool nextFrame, Float framePos)
    {
        // Implement mouse-drag.
        GFxASCharacter::DoMouseDrag();

        for (UInt i = 0; i < pDef->ButtonRecords.GetSize(); i++)
        {
            GFxButtonRecord& rec = pDef->ButtonRecords[i];

            if (RecordCharacter[i] && rec.MatchMouseState(MouseState))
                RecordCharacter[i]->AdvanceFrame(nextFrame, framePos);
        }
    }
#endif //ifdef GFC_USE_OLD_ADVANCE

    void    Display(GFxDisplayContext& context)
    {
        if (!pDef->ButtonRecords.GetSize())
            return;

        // We must apply a new binding table if the character needs to
        // use resource data from the loaded/imported GFxMovieDefImpl.
        GFxResourceBinding *psave = context.pResourceBinding;
        context.pResourceBinding = &pDefImpl->GetResourceBinding();

        GFxMovieRoot* proot = GetMovieRoot(); 
        if (!proot)
            return;

        GRenderer* prenderer = context.GetRenderer();

        // save context transform pointers
        GFxDisplayContextTransforms oldXform;
        GFxDisplayContextFilters oldFilters;
        context.CopyTransformsTo(&oldXform);

        GRenderer::Cxform wcx;
        GMatrix2D matrix;       
#ifndef GFC_NO_3D
        GMatrix3D matrix3D;
        GMatrix3D *pmatrix3D = &matrix3D;
#else
        GMatrix3D *pmatrix3D = NULL;
#endif

        // not correct, doesn't support prepass, use Sprite behavior
        bool useFilters = (proot->pRenderConfig->GetRendererCapBits() & GRenderer::Cap_RenderTargets) != 0;

        // Button records already sorted by depth, so display them in order.
        for (UInt i = 0; i < pDef->ButtonRecords.GetSize(); i++)
        {
            GFxButtonRecord&    rec = pDef->ButtonRecords[i];
            GFxCharacter*       pch = RecordCharacter[i];
            
            if (pch && rec.MatchMouseState(MouseState))
            {
                // prepare context transform pointers for display
                context.PreDisplay(oldXform, this, &matrix, pmatrix3D, &wcx);

                prenderer->PushBlendMode(pch->GetBlendMode());
                bool filtered = useFilters && context.BeginFilters(oldFilters, pch, rec.Filters);
                pch->Display(context);
                if (filtered)
                    context.EndFilters(oldFilters, pch, rec.Filters);
                prenderer->PopBlendMode();
            }
        }

        context.pResourceBinding = psave;
        context.PostDisplay(oldXform);

        DoDisplayCallback();
    }

    // Combine the flags to avoid a conditional. It would be faster with a macro.
    GINLINE int     Transition(int a, int b) const
    {
        return (a << 2) | b;
    }

    virtual bool    PointTestLocal(const GPointF &pt, UInt8 hitTestMask = 0) const
    {
        if (IsHitTestDisableFlagSet())
            return false;

        if ((hitTestMask & HitTest_IgnoreInvisible) && !GetVisible())
            return false;

        if (!DoesScale9GridExist())
        {
            if (!GetBounds(GRenderer::Matrix()).Contains(pt))
                return false;
            else if (!(hitTestMask & HitTest_TestShape))
                return true;
        }

        for (UInt i = 0; i < pDef->ButtonRecords.GetSize(); i++)
        {
            GFxButtonRecord&    rec = pDef->ButtonRecords[i];
            if ((rec.CharacterId == GFxResourceId::InvalidId) || !rec.IsHitTest())
                continue;

            GFxCharacter* ch = RecordCharacter[i];
            if (!ch)
                continue;

            if ((hitTestMask & HitTest_IgnoreInvisible) && !ch->GetVisible())
                continue;

            GRenderer::Matrix   m = ch->GetMatrix();
            GPointF             p = m.TransformByInverse(pt);   

            if (ch->PointTestLocal (p, hitTestMask))
                return true;
        }   
        return false;
    }

    // Return the topmost entity that the given point covers.  NULL if none.
    // I.E. check against ourself.
    virtual GFxASCharacter* GetTopMostMouseEntity(const GPointF &pt, const TopMostParams& params)
    {
        if (!GetVisible())
            return 0;

        if (params.IgnoreMC == this)
            return 0;

        if (!IsFocusAllowed(params.pRoot, params.ControllerIdx))
            return 0;

        GRenderer::Matrix   m = GetMatrix();
        GPointF             p;
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

        for (UInt i = 0; i < pDef->ButtonRecords.GetSize(); i++)
        {
            GFxButtonRecord&    rec = pDef->ButtonRecords[i];
            if ((rec.CharacterId == GFxResourceId::InvalidId) || !rec.IsHitTest())
                continue;

            if (i < RecordCharacter.GetSize())
            {
                GFxCharacter* pcharacter = RecordCharacter[i];
                if (pcharacter)
                {
                    GRenderer::Matrix   m = pcharacter->GetMatrix();
                    GPointF             subp;

#ifndef GFC_NO_3D
                    if (pcharacter->Is3D(true))
                    {
                        const GMatrix3D     *pPersp = pcharacter->GetPerspective3D(true);
                        const GMatrix3D     *pView = pcharacter->GetView3D(true);
                        if (pPersp)
                            GetMovieRoot()->ScreenToWorld.SetPerspective(*pPersp);
                        if (pView)
                            GetMovieRoot()->ScreenToWorld.SetView(*pView);
                        GetMovieRoot()->ScreenToWorld.SetWorld(pcharacter->GetWorldMatrix3D());
                        GetMovieRoot()->ScreenToWorld.GetWorldPoint(&subp);
                    }
                    else
#endif
                    {
                        subp = m.TransformByInverse(p);   
                    }

                    if (pcharacter->PointTestLocal (subp, HitTest_TestShape))
                        return this;
                }
            }
        }
        return NULL;
    }

    bool IsTabable() const
    {
        if (!GetVisible()) return false;
        if (!IsTabEnabledFlagDefined())
        {
            GASObject* pproto = Get__proto__();
            if (pproto)
            {
                // check prototype for tabEnabled
                GASValue val;
                const GASEnvironment* penv = GetASEnvironment();
                if (pproto->GetMemberRaw(penv->GetSC(), penv->CreateConstString("tabEnabled"), &val))
                {
                    if (!val.IsUndefined())
                        return val.ToBool(penv);
                }
            }
            return true;
        }
        else
            return IsTabEnabledFlagTrue();
    }

    virtual ObjectType      GetObjectType() const
    {
        return Object_Button;
    }

    virtual const GFxScale9Grid* GetScale9Grid() const { return pScale9Grid; }

    // Return a single character bounds
    virtual GRectF  GetBoundsOfRecord(const Matrix &transform, UInt recNumber) const
    {
        // Custom based on state.
        GRectF  bounds(0);
        Matrix  m;

        if (RecordCharacter[recNumber])
        {
            m = transform;
            m.Prepend(RecordCharacter[recNumber]->GetMatrix());
            bounds = RecordCharacter[recNumber]->GetBounds(m);
        }
        
        return bounds;
    }

    // Return a single character "pure rectangle" bounds (not considering the stroke)
    GRectF  GetRectBounds(const Matrix &transform, UInt recNumber) const
    {
        // Custom based on state.
        GRectF  bounds(0);
        Matrix  m;

        if (RecordCharacter[recNumber])
        {
            m = transform;
            m.Prepend(RecordCharacter[recNumber]->GetMatrix());
            bounds = RecordCharacter[recNumber]->GetRectBounds(m);
        }
        
        return bounds;
    }


    // Get bounds. This is used,
    // among other things, to calculate button width & height.
    virtual GRectF  GetBounds(const Matrix &transform) const
    {
        // Custom based on state.
        GRectF  bounds(0);
        GRectF  tempRect;
        bool    boundsInit = 0;

        for (UInt i = 0; i < pDef->ButtonRecords.GetSize(); i++)
        {
            GFxButtonRecord& rec = pDef->ButtonRecords[i];

            if (rec.MatchMouseState(MouseState))
            {
                tempRect = GetBoundsOfRecord(transform, i);
                if (!tempRect.IsNull())
                {
                    if (!boundsInit)
                    {
                        bounds = tempRect;
                        boundsInit = 1;
                    }
                    else
                    {
                        bounds.Union(tempRect);
                    }
                }
            }
        }
        return bounds;
    }

    // "transform" matrix describes the transform applied to parent and us,
    // including the object's matrix itself. This means that if transform is
    // identity, GetBoundsTransformed will return local bounds and NOT parent bounds.
    virtual GRectF GetRectBounds(const Matrix &transform) const
    {
        // Custom based on state.
        GRectF  bounds(0);
        GRectF  tempRect;
        bool    boundsInit = 0;

        for (UInt i = 0; i < pDef->ButtonRecords.GetSize(); i++)
        {
            // TO DO: Clarify this, whether or not we should match the state.
            // If yes, it may change the scale9grid configuration and the appearance.
            //GFxButtonRecord& rec = pDef->ButtonRecords[i];
            //if (rec.MatchMouseState(MouseState)) 
            {
                tempRect = GetRectBounds(transform, i);
                if (!tempRect.IsNull())
                {
                    if (!boundsInit)
                    {
                        bounds = tempRect;
                        boundsInit = 1;
                    }
                    else
                    {
                        bounds.Union(tempRect);
                    }
                }
            }
        }
        return bounds;
    }


    enum ButtonState
    {
        Up,
        Over,
        Down,
        Hit
    };
    // returns the local boundaries of whole state
    virtual GRectF  GetBoundsOfState(const Matrix &transform, ButtonState state) const
    {
        // Custom based on state.
        GRectF  bounds(0);
        GRectF  tempRect;

        for (UInt i = 0; i < pDef->ButtonRecords.GetSize(); i++)
        {
            GFxButtonRecord& rec = pDef->ButtonRecords[i];

            if ((state == Hit && rec.IsHitTest()) || 
                (state == Down && rec.IsDown()) ||
                (state == Over && rec.IsOver()) ||
                (state == Up && rec.IsUp()))
            {
                tempRect = GetBoundsOfRecord(transform, i);
                if (!tempRect.IsNull())
                {
                    if (bounds.IsNull())
                        bounds = tempRect;
                    else
                        bounds.Union(tempRect);
                }
            }
        }
        
        return bounds;
    }

    // focus rect for buttons is calculated as follows:
    // 1) if "hit" state exists - boundary rect of "hit" shape
    // 2) if "down" state exists  - boundary rect of "down" shape
    // 3) if "over" state exists  - boundary rect of "over" shape
    // 4) otherwise - boundary rect of "up" shape
    virtual GRectF GetFocusRect() const 
    {
        GRenderer::Matrix m;
        GRectF tempRect;
        if (!(tempRect = GetBoundsOfState(m, Hit)).IsNull())
            return tempRect;
        else if (!(tempRect = GetBoundsOfState(m, Down)).IsNull())
            return tempRect;
        else if (!(tempRect = GetBoundsOfState(m, Over)).IsNull())
            return tempRect;
        else if (!(tempRect = GetBoundsOfState(m, Up)).IsNull())
            return tempRect;
        return GetBounds(m); // shouldn't reach this point, actually
    }

    // invoked when item is going to get focus (Selection.setFocus is invoked, or TAB is pressed)
    void OnGettingKeyboardFocus(UInt /*controllerIdx*/)
    {
        GFxMovieRoot* proot = GetMovieRoot();
        if (proot && !proot->IsDisableFocusRolloverEvent())
            OnButtonEvent(GFxEventId::Event_RollOver);
    }

    // invoked when focused item is about to lose keyboard focus input (mouse moved, for example)
    bool OnLosingKeyboardFocus(GFxASCharacter*, UInt controllerIdx, GFxFocusMovedType) 
    {
        GFxMovieRoot* proot = GetMovieRoot();
        if (proot->IsFocusRectShown(controllerIdx) && !proot->IsDisableFocusRolloverEvent())
            OnButtonEvent(GFxEventId::Event_RollOut);
        return true;
    }

    void    PropagateMouseEvent(const GFxEventId& event)
    {
        if (event.Id == GFxEventId::Event_MouseMove)
        {
            // Implement mouse-drag.
            GFxASCharacter::DoMouseDrag();
        }

        GFxASCharacter::PropagateMouseEvent(event);
    }

    /* returns true, if event fired */
    virtual bool    OnButtonEvent(const GFxEventId& event)
    {
        if (IsUnloading() || IsUnloaded())
            return false;
        // Enabled buttons are not manipulated by events.
        if (!IsEnabledFlagSet())
            return false;
        bool handlerFound = false;

        //GFxButtonRecord::MouseState prevMouseState = MouseState;
        if (event.RollOverCnt == 0) // RollOverCnt is not zero only when multiple mice used
                                    // and only for onRoll/Drag/Over/Out events
        {
            // Set our mouse State (so we know how to render).
            switch (event.Id)
            {
            case GFxEventId::Event_RollOut:
            case GFxEventId::Event_ReleaseOutside:
                MouseState = GFxButtonRecord::MouseUp;
                break;

            case GFxEventId::Event_Release:
            case GFxEventId::Event_RollOver:
                MouseState = GFxButtonRecord::MouseOver;
                break;
            case GFxEventId::Event_DragOut:
                if (GetTrackAsMenu())
                    MouseState = GFxButtonRecord::MouseUp;
                else
                    MouseState = GFxButtonRecord::MouseOver;
                break;

            case GFxEventId::Event_Press:
            case GFxEventId::Event_DragOver:
                MouseState = GFxButtonRecord::MouseDown;
                break;

            case GFxEventId::Event_KeyPress:
                break; 
            default:
                return false; // missed a case?
            };

#ifndef GFC_NO_SOUND
            // Button transition sounds.
            if (pDef->pSound != NULL)
            {
                int bi; // button sound GArray index [0..3]
                switch (event.Id)
                {
                case GFxEventId::Event_RollOut:
                    bi = 0;
                    break;
                case GFxEventId::Event_RollOver:
                    bi = 1;
                    break;
                case GFxEventId::Event_Press:
                    bi = 2;
                    break;
                case GFxEventId::Event_Release:
                    bi = 3;
                    break;
                default:
                    bi = -1;
                    break;
                }
                pDef->pSound->Play(this, bi);
            }
#endif // GFC_NO_SOUND

            // @@ eh, should just be a lookup table.
            int c = 0, kc = 0;
            if (event.Id == GFxEventId::Event_RollOver) c |= (GFxButtonAction::IDLE_TO_OVER_UP);
            else if (event.Id == GFxEventId::Event_RollOut) c |= (GFxButtonAction::OVER_UP_TO_IDLE);
            else if (event.Id == GFxEventId::Event_Press) c |= (GFxButtonAction::OVER_UP_TO_OVER_DOWN);
            else if (event.Id == GFxEventId::Event_Release) c |= (GFxButtonAction::OVER_DOWN_TO_OVER_UP);
            else if (event.Id == GFxEventId::Event_DragOut) c |= (GFxButtonAction::OVER_DOWN_TO_OUT_DOWN);
            else if (event.Id == GFxEventId::Event_DragOver) c |= (GFxButtonAction::OUT_DOWN_TO_OVER_DOWN);
            else if (event.Id == GFxEventId::Event_ReleaseOutside) c |= (GFxButtonAction::OUT_DOWN_TO_IDLE);
            else if (event.Id == GFxEventId::Event_KeyPress) 
            {
                // convert keycode/ascii to button's keycode
                kc = event.ConvertToButtonKeyCode();
            }
                //IDLE_TO_OVER_DOWN = 1 << 7,
            //OVER_DOWN_TO_IDLE = 1 << 8,

            // recreate the characters of the new state and release ones for previous state.
            RecreateCharacters();

            GFxASCharacter* pparent = GetParent();                
            if (pparent)
            {
                GFxSprite* pparentSprite = pparent->ToSprite();                
                if (pparentSprite)
                {
                    // Add appropriate actions to the GFxMovieSub's execute list...
                    for (UInt i = 0; i < pDef->ButtonActions.GetSize(); i++)
                    {
                        if (((pDef->ButtonActions[i].Conditions & (~0xFE00)) & c) ||     //!AB??
                            (kc > 0 && ((pDef->ButtonActions[i].Conditions >> 9) & 0x7F) == kc))
                        {
                            // Matching action.
                            GASStringContext* psc = pparentSprite->GetASEnvironment()->GetSC();
                            const UPInt n = pDef->ButtonActions[i].Actions.GetSize();
                            for (UPInt j = 0; j < n; ++j)
                            {
                                if (!pDef->ButtonActions[i].Actions[j]->IsNull())
                                {
                                    GPtr<GASActionBuffer> pbuff = 
                                        *GHEAP_NEW(psc->GetHeap()) GASActionBuffer(psc, pDef->ButtonActions[i].Actions[j]);
                                    pparentSprite->AddActionBuffer(pbuff);
                                }
                            }
                            if (n > 0) handlerFound = true;
                        }
                    }
                }
            }
//?            ModifyOptimizedPlayListLocal<GFxButtonCharacter>(GetMovieRoot());
        }
        // Call conventional attached method.
        // Check for member function, it is called after onClipEvent(). 
        // In ActionScript 2.0, event method names are CASE SENSITIVE.
        // In ActionScript 1.0, event method names are CASE INSENSITIVE.
        GASEnvironment *penv = GetASEnvironment();
        if (penv)
        {
            // penv can be NULL, if the character has already been removed
            // from the display list by earlier handlers (for example, by
            // "Mouse" class listeners, or by "on(...)" handler).
            GASString methodName(event.GetFunctionName(penv->GetSC()));

            if (methodName.GetSize() > 0)
            {
                GASValue method;
                if (GetMemberRaw(penv->GetSC(), methodName, &method))
                {
                    handlerFound = true;
                    // do actual dispatch
                    GFxMovieRoot::ActionEntry* pe = GetMovieRoot()->InsertEmptyAction();
                    if (pe) pe->SetAction(this, event);
                }
            }
        }
        return handlerFound;
    }

    void UnloadCharacterAtIndex(UInt i)
    {
        // unload character
        if (RecordCharacter[i])
        {
            GFxCharacter* ch = RecordCharacter[i];
            GFxASCharacter* pscriptCh = ch->CharToASCharacter();
            if (pscriptCh)
            {
                bool mayRemove = ch->OnUnloading();
                ch->SetUnloading(true);
                if (mayRemove)
                {
                    // We can remove the object instantly, so, do it now.
                    ch->OnEventUnload();
                }
                //else
                //{
                //    GFxMovieRoot* proot = GetMovieRoot();
                //    GASSERT(proot);
                //    pscriptCh->RemoveFromPlaylist(proot);
                //}
            }
            RecordCharacter[i] = NULL;
        }
    }

    void RecreateCharacters()
    {
        // Restart our relevant characters
        for (UInt i = 0; i < pDef->ButtonRecords.GetSize(); i++)
        {
            GFxButtonRecord* rec = &pDef->ButtonRecords[i];

            if (!RecordCharacter[i])
            {
                if (rec->MatchMouseState(MouseState) || rec->IsHitTest())
                {
                    // create character

                    // Resolve the GFxCharacter id.
                    GFxCharacterCreateInfo ccinfo = pDefImpl->GetCharacterCreateInfo(rec->CharacterId);
                    GASSERT(ccinfo.pCharDef && ccinfo.pBindDefImpl);

                    if (ccinfo.pCharDef)
                    {
                        const GRenderer::Matrix&    mat = pDef->ButtonRecords[i].ButtonMatrix;
                        const GRenderer::Cxform&    cx  = pDef->ButtonRecords[i].ButtonCxform;

                        GPtr<GFxCharacter>  ch = *ccinfo.pCharDef->CreateCharacterInstance(this, rec->CharacterId, ccinfo.pBindDefImpl);
                        RecordCharacter[i] = ch;
                        ch->SetMatrix(mat);
                        ch->SetCxform(cx);
                        ch->SetBlendMode(rec->BlendMode);

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

                        GFxASCharacter* pscriptCh   = ch->CharToASCharacter();
                        if (pscriptCh)
                        {
                            GFxMovieRoot* proot = GetMovieRoot();
                            GASSERT(proot);
                            if (pscriptCh->IsSprite())
                            {
                                GASGlobalContext* gctxt = this->GetGC();
                                GASSERT(gctxt);

                                GASFunctionRef ctorFunc;    
                                const GString* psymbolName = ch->GetResourceMovieDef()->GetNameOfExportedResource(rec->CharacterId);

                                if (psymbolName)
                                {
                                    GASString symbolName = GetASEnvironment()->CreateString(*psymbolName);

                                    if (gctxt->FindRegisteredClass(GetASEnvironment()->GetSC(), symbolName, &ctorFunc))
                                    {
                                        pscriptCh->SetProtoToPrototypeOf(ctorFunc.GetObjectPtr());

                                        // schedule "construct" event, if any
                                        //if (hasConstructEvent) //!AB, fire onConstruct always, since it is impossible
                                        {                        // to check whether onConstruct method exists or not
                                            GFxMovieRoot::ActionEntry* pe = proot->InsertEmptyAction(GFxMovieRoot::AP_Construct);
                                            if (pe) pe->SetAction(pscriptCh, GFxEventId::Event_Construct);
                                        }

                                        // now, schedule constructor invocation
                                        GFxMovieRoot::ActionEntry* pe = proot->InsertEmptyAction(GFxMovieRoot::AP_Construct);
                                        if (pe) pe->SetAction(pscriptCh, ctorFunc);
                                    }
                                    else
                                    {
                                        GASValueArray params;
                                        params.PushBack(GASValue(symbolName)); //[0]
                                        //params.PushBack(GASValue(hasConstructEvent)); //[1]

                                        GFxMovieRoot::ActionEntry* pe = proot->InsertEmptyAction(GFxMovieRoot::AP_Construct);
                                        if (pe) pe->SetAction(pscriptCh, GFx_FindClassAndInitializeClassInstance, &params);
                                    }
                                }
                            }

                            pscriptCh->AddToPlayList(proot);
                            pscriptCh->ModifyOptimizedPlayList(proot);

                            ch->OnEventLoad();
                        }
                    }            
                }
            }
            else
            {
                if (!rec->MatchMouseState(MouseState) && !rec->IsHitTest())
                {
                    // unload character
                    UnloadCharacterAtIndex(i);
                }
            }
        }
    }

    //
    // ActionScript overrides
    //

    
    // GFxASCharacter override to indicate which standard members are handled for us.
    virtual UInt32  GetStandardMemberBitMask() const
    {
        // MovieClip lets base handle all members it supports.
        return  UInt32(
                M_BitMask_PhysicalMembers |         
                M_BitMask_CommonMembers |
                (1 << M_target) |
                (1 << M_url) |
                (1 << M_parent) |
                (1 << M_blendMode) |
                (1 << M_cacheAsBitmap) |
                (1 << M_filters) |
                (1 << M_focusrect) |
                (1 << M_enabled) |
                (1 << M_trackAsMenu) |                                  
                (1 << M_tabEnabled) |
                (1 << M_tabIndex) |
                (1 << M_useHandCursor) |
                (1 << M_quality) |
                (1 << M_highquality) |
                (1 << M_soundbuftime) |
                (1 << M_xmouse) |
                (1 << M_ymouse)                                                                 
                );
    // MA Verified: _lockroot does not exist/carry over from buttons, so don't include it.
    // If a movie is loaded into button, local _lockroot state is lost.
    }

    GASButtonObject* GetButtonASObject()          
    { 
        if (!ASButtonObj)
        {
            ASButtonObj = *GHEAP_AUTO_NEW(this) GASButtonObject(GetGC(), this);
        }
        return ASButtonObj;
    }

    virtual GASObject*  GetASObject () { return GetButtonASObject(); }
    virtual GASObject*  GetASObject () const 
    { 
        return const_cast<GFxButtonCharacter*>(this)->GetButtonASObject(); 
    }

    virtual bool    GetMember(GASEnvironment* penv, const GASString& name, GASValue* pval)
    {
        if (name.IsStandardMember())                    
            if (GetStandardMember(GetStandardMemberConstant(name), pval, 0))
                return true;        

        if (ASButtonObj)
        {
            return ASButtonObj->GetMember(penv, name, pval);
        }
        else 
        {
            // Handle the __proto__ property here, since we are going to 
            // zero out it temporarily (see comments below).
            if (penv && name == penv->GetBuiltin(GASBuiltin___proto__))
            {
                GASObject* proto = Get__proto__();
                pval->SetAsObject(proto);
                return true;
            }

            // Now we can search in the __proto__
            GASObject* proto = Get__proto__();
            if (proto)
            {
                // ASMovieClipObj is not created yet; use __proto__ instead
                if (proto->GetMember(penv, name, pval))    
                {
                    return true;
                }
            }
        }
        // Looks like _global is accessible from any character
        if (penv && (name == penv->GetBuiltin(GASBuiltin__global)))
        {
            pval->SetAsObject(penv->GetGC()->pGlobal);
            return true;
        }
        return false;
    }


    virtual bool GetStandardMember(StandardMember member, GASValue* pval, bool opcodeFlag) const
    {
        if (GFxASCharacter::GetStandardMember (member, pval, opcodeFlag))
            return true;

        // Handle MovieClip specific "standard" members.
        switch(member)
        {
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

    virtual bool SetStandardMember(StandardMember member, const GASValue& origVal, bool opcodeFlag)
    {   
        GASValue val(origVal);
        GASEnvironment* penv = GetASEnvironment();
        if (member > M_BuiltInProperty_End)
        {
            // Check, if there are watch points set. Invoke, if any.
            if (penv && GetButtonASObject() && ASButtonObj->HasWatchpoint()) // have set any watchpoints?
            {
                GASValue newVal;
                if (ASButtonObj->InvokeWatchpoint(penv, 
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

            // extension
            case M_hitTestDisable:
                if (GetASEnvironment()->CheckExtensions())
                {
                    SetHitTestDisableFlag(val.ToBool(GetASEnvironment()));
                    return 1;
                }
                break;
            // No other custom properties to set for now.
            default:
            break;
        }
        return false;
    }



    virtual bool OnKeyEvent(const GFxEventId& id, int* pkeyMask) 
    {
        // Check for member function.   
        // In ActionScript 2.0, event method names are CASE SENSITIVE.
        // In ActionScript 1.0, event method names are CASE INSENSITIVE.
        GASEnvironment *penv = GetASEnvironment();
        GASString       methodName(id.GetFunctionName(penv->GetSC()));
        GFxMovieRoot*   proot = GetMovieRoot();

        if (methodName.GetSize() > 0)
        {           
            GASValue    method;
            if ((id.Id == GFxEventId::Event_KeyDown || id.Id == GFxEventId::Event_KeyUp) &&
                GetMemberRaw(penv->GetSC(), methodName, &method)) 
            {
                // onKeyDown/onKeyUp are available only in Flash 6 and later
                // (don't mess with onClipEvent (keyDown/keyUp)!)
                if (penv->GetVersion() >= 6 && proot->IsKeyboardFocused(this, id.KeyboardIndex))
                {
                    // also, onKeyDown/onKeyUp should be invoked only if focus
                    // is enabled and set to this button

                    // do actual dispatch
                    GFxMovieRoot::ActionEntry* pe = proot->InsertEmptyAction();
                    if (pe) pe->SetAction(this, id);
                }
            }
        }

        if (id.Id == GFxEventId::Event_KeyDown)
        {
            // covert Event_KeyDown to Event_KeyPress
            GASSERT (pkeyMask != 0);

            // check if keyPress already was handled then do not handle it again
            if (!((*pkeyMask) & GFxCharacter::KeyMask_KeyPress))
            {
                if (OnButtonEvent(GFxEventId (GFxEventId::Event_KeyPress, id.KeyCode, id.AsciiCode)))
                {
                    *pkeyMask |= GFxCharacter::KeyMask_KeyPress;
                }
            }

            if (proot->IsKeyboardFocused(this, id.KeyboardIndex) && (id.KeyCode == GFxKey::Return || id.KeyCode == GFxKey::Space))
            {
                if (IsFocusRectEnabled() || proot->IsAlwaysEnableKeyboardPress())
                {
                    // if focused and enter - simulate on(press)/onPress and on(release)/onRelease
                    OnButtonEvent(GFxEventId(GFxEventId::Event_Press, GFxKey::Return, 0, 0, id.KeyboardIndex));

                    GPtr<GFxASCharacter> thisHolder = this;
                    proot->Advance(0, 0); //??AB, allow on(press) to be executed 
                    // completely before on(release). Otherwise, these events may affect each other
                    // (see focusKB_test.swf). Need further investigations.

                    OnButtonEvent(GFxEventId(GFxEventId::Event_Release, GFxKey::Return, 0, 0, id.KeyboardIndex));
                }
            }
        }
        return true;
    }
    // Set state changed flags
    virtual void    SetStateChangeFlags(UInt8 flags)
    {
        GFxASCharacter::SetStateChangeFlags(flags);
        UPInt size = RecordCharacter.GetSize();
        for (UPInt i = 0; i < size; i++)
        {
            GFxCharacter* ch = RecordCharacter[i];
            if (ch)
                ch->SetStateChangeFlags(flags);
        }
    }

    UInt            GetCursorType() const 
    { 
        const GASEnvironment* penv = GetASEnvironment();
        GASValue val;

        // Theoretically, penv could be NULL if parent is null (means,
        // object is already disconnected).
        if (penv && IsEnabledFlagSet())
        {
            if (ASButtonObj)
                const_cast<GFxButtonCharacter*>(this)->GetMemberRaw
                    (penv->GetSC(), penv->GetBuiltin(GASBuiltin_useHandCursor), &val);
            else if (pProto)
                pProto->GetMemberRaw
                    (penv->GetSC(), penv->GetBuiltin(GASBuiltin_useHandCursor), &val);
            if (val.ToBool(penv))
                return GFxMouseCursorEvent::HAND;
        }
        return GFxASCharacter::GetCursorType();
    }
};


//
// GFxButtonRecord
//

// Return true if we read a record; false if this is a null record.
bool    GFxButtonRecord::Read(GFxLoadProcess* p, GFxTagType tagType)
{
    int flags = p->ReadU8();
    if (flags == 0) 
        return false;

    GFxStream*  pin = p->GetStream();

    pin->LogParse("-- action record:  ");
    Flags = 0;
    if (flags & 8)
    {
        Flags |= Mask_HitTest;
        pin->LogParse("HitTest ");
    }
    if (flags & 4)
    {
        Flags |= Mask_Down;
        pin->LogParse("Down ");
    }
    if (flags & 2)
    {
        Flags |= Mask_Over;
        pin->LogParse("Over ");
    }
    if (flags & 1)
    {
        Flags |= Mask_Up;
        pin->LogParse("Up ");
    }
    pin->LogParse("\n");
    
    int characterId = p->ReadU16();
    CharacterId     = GFxResourceId(characterId);   
    ButtonLayer     = p->ReadU16(); 
    pin->ReadMatrix(&ButtonMatrix);     

    pin->LogParse("   CharId = %d, ButtonLayer = %d\n", characterId, ButtonLayer);
    pin->LogParse("   mat:\n");
    pin->LogParseClass(ButtonMatrix);

    if (tagType == 34)
    {
        pin->ReadCxformRgba(&ButtonCxform);
        pin->LogParse("   cxform:\n");
        pin->LogParseClass(ButtonCxform);
    }

    // SWF 8 features.

    // Has filters.
    if (flags & 0x10)
    {       
        pin->LogParse("   HasFilters\n");
        // This loads only Blur and DropShadow/Glow filters only.
        GFxFilterDesc filters[GFxFilterDesc::MaxFilters];
        UInt numFilters = GFx_LoadFilters(pin, filters, GFxFilterDesc::MaxFilters);
        if (numFilters)
        {
#if 0
            if (!Filters)
                Filters = *new GArrayLH<GFxFilterDesc>;
            for (UInt i = 0; i < numFilters; ++i)
            {
                Filters.Append(filters, numFilters);
            }
#else
            Filters.Clear();
            Filters.Append(filters, numFilters);
#endif
        }
    }
    // Has custom blending.
    if (flags & 0x20)
    {       
        UByte   blendMode = pin->ReadU8();
        if ((blendMode < 1) || (blendMode>14))
        {
            GFC_DEBUG_WARNING(1, "ButtonRecord::Read - loaded blend mode out of range");
            blendMode = 1;
        }
        BlendMode = (GRenderer::BlendType) blendMode;
        pin->LogParse("   HasBlending, %d\n", (int)BlendMode);
    }
    else
    {
        BlendMode = GRenderer::Blend_None;
    }

    // Note: 'Use Bitmap Caching' toggle does not seem to be serialized to flags, perhaps 
    // because it makes no sense in button records, since they cannot be animated (?).

    return true;
}


//
// GFxButtonAction
//


GFxButtonAction::~GFxButtonAction()
{
    for (UPInt i = 0, n = Actions.GetSize(); i < n; i++)
    {
        Actions[i]->Release();
    }
    Actions.Resize(0);
}

void    GFxButtonAction::Read(GFxStream* pin, GFxTagType tagType, UInt actionLength)
{
    if (actionLength == 0)
        return;

    // Read condition flags.
    if (tagType == GFxTag_ButtonCharacter)
    {
        Conditions = OVER_DOWN_TO_OVER_UP;
    }
    else
    {
        GASSERT(tagType == GFxTag_ButtonCharacter2);
        Conditions = pin->ReadU16();
        actionLength -= 2;
    }
    pin->LogParse("-- action conditions %X\n", Conditions);

    // Read actions.
    pin->LogParseAction("-- actions in button\n"); // @@ need more info about which actions
    GASActionBufferData* a = GASActionBufferData::CreateNew();
    a->Read(pin, actionLength);
    Actions.PushBack(a);
}


//
// GFxButtonCharacterDef
//

GFxButtonCharacterDef::GFxButtonCharacterDef()
    :
#ifndef GFC_NO_SOUND
    pSound(NULL),
#endif// GFC_NO_SOUND
    pScale9Grid(NULL)
// Constructor.
{
}


GFxButtonCharacterDef::~GFxButtonCharacterDef()
{
#ifndef GFC_NO_SOUND
    delete pSound;
#endif  // GFC_NO_SOUND
    delete pScale9Grid;
}

static void SkipButtonSoundDef(GFxLoadProcess* p)
{
    GFxStream *in = p->GetStream();
    for (int i = 0; i < 4; i++)
    {
        UInt16 rid = p->ReadU16();
        if (rid == 0)
            continue;
        in->ReadUInt(2);    // skip reserved bits.
        in->ReadUInt(1);
        in->ReadUInt(1);
        bool HasEnvelope = in->ReadUInt(1) ? true : false;
        bool HasLoops = in->ReadUInt(1) ? true : false;
        bool HasOutPoint = in->ReadUInt(1) ? true : false;
        bool HasInPoint = in->ReadUInt(1) ? true : false;
        if (HasInPoint) in->ReadU32();
        if (HasOutPoint) in->ReadU32();
        if (HasLoops) in->ReadU16();
        if (HasEnvelope)
        {
            int nPoints = in->ReadU8();
            for (int i=0; i < nPoints; i++)
            {
                in->ReadU32();
                in->ReadU16();
                in->ReadU16();
            }
        }
    }
}

// Initialize from the given GFxStream.
void    GFxButtonCharacterDef::Read(GFxLoadProcess* p, GFxTagType tagType)
{
    GASSERT(tagType == GFxTag_ButtonCharacter ||
            tagType == GFxTag_ButtonSound ||
            tagType == GFxTag_ButtonCharacter2);

    if (tagType == GFxTag_ButtonCharacter)
    {
        // Old button tag.
            
        // Read button GFxCharacter records.
        for (;;)
        {
            GFxButtonRecord r;
            if (r.Read(p, tagType) == false)
            {
                // Null record; marks the end of button records.
                break;
            }

            // Search for the depth and insert in the right location. This ensures
            // that buttons are always drawn correctly.
            UInt i;
            for(i=0; i<ButtonRecords.GetSize(); i++)
                if (ButtonRecords[i].ButtonLayer > r.ButtonLayer)
                    break;
            ButtonRecords.InsertAt(i, r);
        }

        // Read actions.
        ButtonActions.Resize(ButtonActions.GetSize() + 1);
        ButtonActions.Back().Read(p->GetStream(), tagType, (UInt)(p->GetTagEndPosition() - p->Tell()));
    }

    else if (tagType == GFxTag_ButtonSound)
    {
#ifndef GFC_NO_SOUND
        GASSERT(pSound == NULL); // redefinition button sound is error
        GFxAudioBase* paudio = p->GetLoadStates()->GetAudio();
        if (paudio)
        {
            GASSERT(paudio->GetSoundTagsReader());
            pSound = paudio->GetSoundTagsReader()->ReadButtonSoundDef(p);
        }
        else
        {
            SkipButtonSoundDef(p);
            p->LogScriptWarning("GFxButtonCharacterDef::Read: Audio library is not set. Skipping sound definitions.");
        }
#else
        SkipButtonSoundDef(p);
#endif // GFC_NO_SOUND

    }

    else if (tagType == GFxTag_ButtonCharacter2)
    {
        // Read the menu flag.
        Menu = p->ReadU8() != 0;

        int Button2_actionOffset = p->ReadU16();
        int NextActionPos = p->Tell() + Button2_actionOffset - 2;

        // Read button records.
        for (;;)
        {
            GFxButtonRecord r;
            if (r.Read(p, tagType) == false)
            {
                // Null record; marks the end of button records.
                break;
            }
            // Search for the depth and insert in the right location.
            UInt i;
            for(i=0; i<ButtonRecords.GetSize(); i++)
                if (ButtonRecords[i].ButtonLayer > r.ButtonLayer)
                    break;
            ButtonRecords.InsertAt(i, r);
        }

        if (Button2_actionOffset > 0)
        {
            p->SetPosition(NextActionPos);

            // Read Button2ActionConditions
            for (;;)
            {
                UInt NextActionOffset = p->ReadU16();
                NextActionPos = p->Tell() + NextActionOffset - 2;

                ButtonActions.Resize(ButtonActions.GetSize() + 1);
                ButtonActions.Back().Read(p->GetStream(), tagType, 
                    (NextActionOffset) ? NextActionOffset - 2 : (UInt)(p->GetTagEndPosition() - p->Tell()));

                if (NextActionOffset == 0
                    || p->Tell() >= p->GetTagEndPosition())
                {
                    // done.
                    break;
                }

                // seek to next action.
                p->SetPosition(NextActionPos);
            }
        }
    }
}


// Create a mutable instance of our definition.
GFxCharacter*   GFxButtonCharacterDef::CreateCharacterInstance(GFxASCharacter* parent, GFxResourceId id,
                                                               GFxMovieDefImpl *pbindingImpl)
{
    GFxCharacter* ch = GHEAP_AUTO_NEW(parent) GFxButtonCharacter(this, pbindingImpl, parent, id);
    return ch;
}


void GASButtonObject::commonInit ()
{
}

static const GASNameFunction GAS_ButtonFunctionTable[] = 
{
    { "getDepth",       &GFxASCharacter::CharacterGetDepth },

    { 0, 0 }
};

GASButtonProto::GASButtonProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor) :
    GASPrototype<GASButtonObject>(psc, prototype, constructor)
{
    InitFunctionMembers(psc, GAS_ButtonFunctionTable);
    SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_useHandCursor), GASValue(true), 
        GASPropFlags::PropFlag_DontDelete);

    SetConstMemberRaw(psc, "blendMode", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "enabled", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "scale9Grid", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    // can't add tabEnabled here, since its presence will stop prototype chain lookup, even if
    // it is UNSET or "undefined".
    //SetConstMemberRaw(psc, "tabEnabled", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "tabIndex", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "trackAsMenu", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
}

void GASButtonCtorFunction::GlobalCtor(const GASFnCall& fn) 
{
    GUNUSED(fn);
    //fn.Result->SetAsObject(GPtr<GASObject>(*GHEAP_NEW(fn.Env->GetHeap()) GASButtonObject()));
}

GASObject* GASButtonCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{
    return GHEAP_NEW(penv->GetHeap()) GASButtonObject(penv);
}

GASFunctionRef GASButtonCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef  ctor(*GHEAP_NEW(pgc->GetHeap()) GASButtonCtorFunction(&sc));
    GPtr<GASObject> proto = *GHEAP_NEW(pgc->GetHeap()) GASButtonProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_Button, proto);
    pgc->pGlobal->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_Button), GASValue(ctor));
    return ctor;
}
