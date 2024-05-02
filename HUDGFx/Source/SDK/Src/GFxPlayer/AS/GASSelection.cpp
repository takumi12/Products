/**********************************************************************

Filename    :   GFxSelection.cpp
Content     :   Implementation of Selection class
Created     :   February, 2007
Authors     :   Artyom Bolgar

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "AS/GASSelection.h"
#include "GFxLog.h"
#include "GFxString.h"
#include "GFxAction.h"
#include "AS/GASArrayObject.h"
#include "AS/GASNumberObject.h"
#include "GFxSprite.h"
#include "GFxText.h"
#include "GUTF8Util.h"
#include "AS/GASAsBroadcaster.h"

GASSelection::GASSelection(GASEnvironment* penv)
: GASObject(penv)
{
    commonInit(penv);
}

void GASSelection::commonInit (GASEnvironment* penv)
{
    Set__proto__ (penv->GetSC(), penv->GetPrototype(GASBuiltin_Selection));
}

void GASSelection::BroadcastOnSetFocus(GASEnvironment* penv, 
                                       GFxASCharacter* pOldFocus, 
                                       GFxASCharacter* pNewFocus,
                                       UInt controllerIdx)
{
    GASValue selectionCtorVal;
    if (penv->GetGC()->pGlobal->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_Selection), &selectionCtorVal))
    {
        GASObjectInterface* pselectionObj = selectionCtorVal.ToObject(penv);
        if (pselectionObj)
        {
            int nargs = 2;
            if (penv->CheckExtensions())
            {
                penv->Push(GASNumber(controllerIdx));
                ++nargs;
            }
            if (pNewFocus)
                penv->Push(GASValue(pNewFocus));
            else
                penv->Push(GASValue::NULLTYPE);
            if (pOldFocus)
                penv->Push(GASValue(pOldFocus));
            else
                penv->Push(GASValue::NULLTYPE);
            GASAsBroadcaster::BroadcastMessage(penv, pselectionObj, penv->CreateConstString("onSetFocus"), nargs, penv->GetTopIndex());
            penv->Drop(nargs);
        }
    }
}

void GASSelection::DoTransferFocus(const GASFnCall& fn)
{
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    
    GASSERT(proot && fn.NArgs == 3);
    GASSERT(fn.Arg(0).IsCharacter() || fn.Arg(0).IsNull());
    GFxFocusMovedType fmt = GFxFocusMovedType(int(fn.Arg(1).ToNumber(fn.Env)));
    UInt controllerIdx = fn.Arg(2).ToUInt32(fn.Env);

    proot->TransferFocus(fn.Arg(0).ToASCharacter(fn.Env), controllerIdx, fmt);
}

void GASSelection::QueueSetFocus
    (GASEnvironment* penv, GFxASCharacter* pNewFocus, UInt controllerIdx, GFxFocusMovedType fmt)
{
    GASSERT(penv);

    GASValueArray params;
    if (pNewFocus)
        params.PushBack(GASValue(pNewFocus));
    else
        params.PushBack(GASValue::NULLTYPE);
    params.PushBack(GASValue((int)fmt));
    params.PushBack(GASNumber(controllerIdx));
    penv->GetMovieRoot()->InsertEmptyAction()->SetAction(penv->GetMovieRoot()->GetLevelMovie(0), 
        GASSelection::DoTransferFocus, &params);
}

#ifndef GFC_NO_FXPLAYER_AS_SELECTION
//////////////////////////////////////////
//
static void GFx_FuncStub(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
}

static const GASNameFunction GAS_SelectionFunctionTable[] = 
{
    { "addListener",        &GFx_FuncStub },
    { "broadcastMessage",   &GFx_FuncStub },
    { "getBeginIndex",      &GFx_FuncStub },
    { "getCaretIndex",      &GFx_FuncStub },
    { "getEndIndex",        &GFx_FuncStub },
    { "getFocus",           &GFx_FuncStub },
    { "setFocus",           &GFx_FuncStub },
    { "setSelection",       &GFx_FuncStub },
    { "removeListener",     &GFx_FuncStub },
    { 0, 0 }
};

GASSelectionProto::GASSelectionProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASSelection>(psc, pprototype, constructor)
{
    InitFunctionMembers (psc, GAS_SelectionFunctionTable);
}

//////////////////
const GASNameFunction GASSelectionCtorFunction::StaticFunctionTable[] = 
{
    { "getBeginIndex",  &GASSelectionCtorFunction::GetBeginIndex },
    { "getCaretIndex",  &GASSelectionCtorFunction::GetCaretIndex },
    { "getEndIndex",    &GASSelectionCtorFunction::GetEndIndex },
    { "getFocus",       &GASSelectionCtorFunction::GetFocus },
    { "setFocus",       &GASSelectionCtorFunction::SetFocus },
    { "setSelection",   &GASSelectionCtorFunction::SetSelection },
    { 0, 0 }
};

GASSelectionCtorFunction::GASSelectionCtorFunction(GASStringContext *psc)
:   GASCFunctionObject(psc, GlobalCtor)
{
    GASAsBroadcaster::Initialize(psc, this);

    GASNameFunction::AddConstMembers(
        this, psc, StaticFunctionTable, 
        GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
}

void GASSelectionCtorFunction::GetFocus(const GASFnCall& fn)
{
    fn.Result->SetNull();
    if (!fn.Env)
        return;

    UInt controllerIdx = 0;
    if (fn.Env->CheckExtensions())
    {
        if (fn.NArgs >= 1)
            controllerIdx = fn.Arg(0).ToUInt32(fn.Env);
    }

    GPtr<GFxASCharacter> focusedChar = fn.Env->GetMovieRoot()->GetFocusedCharacter(controllerIdx);
    if (focusedChar)
    {
        fn.Result->SetString(focusedChar->GetCharacterHandle()->GetNamePath());
    }
}

void GASSelectionCtorFunction::GetFocusArray(const GASFnCall& fn)
{
    fn.Result->SetNull();
    if (!fn.Env)
        return;

    if (fn.Env->CheckExtensions())
    {
        if (fn.NArgs < 1)
            return;

        GPtr<GFxASCharacter> f = fn.Arg(0).ToASCharacter(fn.Env);

        GPtr<GASArrayObject> ao = *static_cast<GASArrayObject*>
            (fn.Env->OperatorNew(fn.Env->GetBuiltin(GASBuiltin_Array)));
        GASSERT(ao);
        ao->Reserve(GFC_MAX_KEYBOARD_SUPPORTED);
        for (UInt i = 0; i < GFC_MAX_KEYBOARD_SUPPORTED; ++i)
        {
            GPtr<GFxASCharacter> focusedChar = fn.Env->GetMovieRoot()->GetFocusedCharacter(i);
            if (focusedChar == f)
            {
                ao->PushBack(GASValue((int)i));
            }
        }
        fn.Result->SetAsObject(ao);
    }
}

void GASSelectionCtorFunction::GetFocusBitmask(const GASFnCall& fn)
{
    fn.Result->SetNull();
    if (!fn.Env)
        return;

    if (fn.Env->CheckExtensions())
    {
        if (fn.NArgs < 1)
            return;

        GPtr<GFxASCharacter> f = fn.Arg(0).ToASCharacter(fn.Env);

        UInt bm = 0;
        for (UInt i = 0, mask = 0x1; i < GFC_MAX_KEYBOARD_SUPPORTED; ++i, mask <<= 1)
        {
            GPtr<GFxASCharacter> focusedChar = fn.Env->GetMovieRoot()->GetFocusedCharacter(i);
            if (focusedChar == f)
            {
                bm |= mask;
            }
        }
        fn.Result->SetNumber(bm & 0xFFFF);
    }
}

void GASSelectionCtorFunction::SetFocus(const GASFnCall& fn)
{
    fn.Result->SetBool(false);
    if (fn.NArgs < 1 || !fn.Env)
        return;

    GPtr<GFxASCharacter> newFocus;
    if (fn.Arg(0).IsString())
    {
        // resolve path
        GASValue val;
        bool retVal = fn.Env->FindVariable(GASEnvironment::GetVarParams(fn.Arg(0).ToString(fn.Env), &val));
        if (retVal && val.IsCharacter())
        {
            newFocus = val.ToASCharacter(fn.Env);
        }
    }
    else
        newFocus = fn.Arg(0).ToASCharacter(fn.Env);
    UInt controllerIdx = 0;
    if (fn.Env->CheckExtensions())
    {
        if (fn.NArgs >= 2)
            controllerIdx = fn.Arg(1).ToUInt32(fn.Env);
    }
    if (newFocus)
    {
        if (newFocus->IsFocusEnabled())
        {
            fn.Env->GetMovieRoot()->SetKeyboardFocusTo(newFocus, controllerIdx);
            fn.Result->SetBool(true);
        }
    }
    else
    {
        // just remove the focus
        fn.Env->GetMovieRoot()->SetKeyboardFocusTo(NULL, controllerIdx);
        fn.Result->SetBool(true);

    }
}

void GASSelectionCtorFunction::GetCaretIndex(const GASFnCall& fn)
{
    fn.Result->SetNumber(-1);
    if (!fn.Env)
        return;

    UInt controllerIdx = 0;
    if (fn.Env->CheckExtensions())
    {
        if (fn.NArgs >= 1)
            controllerIdx = fn.Arg(0).ToUInt32(fn.Env);
    }
    GPtr<GFxASCharacter> focusedChar = fn.Env->GetMovieRoot()->GetFocusedCharacter(controllerIdx);
    if (focusedChar && focusedChar->GetObjectType() == Object_TextField)
    {
        GASTextField* ptextField = static_cast<GASTextField*>(focusedChar.GetPtr());
        fn.Result->SetNumber((GASNumber)ptextField->GetCaretIndex());
    }
}

void GASSelectionCtorFunction::SetSelection(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    if (!fn.Env)
        return;

    UInt controllerIdx = 0;
    if (fn.Env->CheckExtensions())
    {
        if (fn.NArgs >= 3)
            controllerIdx = fn.Arg(2).ToUInt32(fn.Env);
    }
    GPtr<GFxASCharacter> focusedChar = fn.Env->GetMovieRoot()->GetFocusedCharacter(controllerIdx);
    if (focusedChar && focusedChar->GetObjectType() == Object_TextField)
    {
        GASTextField* ptextField = static_cast<GASTextField*>(focusedChar.GetPtr());

        SPInt bi = 0, ei = GFC_MAX_SPINT;

        if (fn.NArgs >= 2)
        {
            bi = fn.Arg(0).ToInt32(fn.Env);
            ei = fn.Arg(1).ToInt32(fn.Env);
        }

        ptextField->SetSelection(bi, ei);
    }
}

void GASSelectionCtorFunction::GetBeginIndex(const GASFnCall& fn)
{
    fn.Result->SetNumber(-1);
    if (!fn.Env)
        return;

    UInt controllerIdx = 0;
    if (fn.Env->CheckExtensions())
    {
        if (fn.NArgs >= 1)
            controllerIdx = fn.Arg(0).ToUInt32(fn.Env);
    }
    GPtr<GFxASCharacter> focusedChar = fn.Env->GetMovieRoot()->GetFocusedCharacter(controllerIdx);
    if (focusedChar && focusedChar->GetObjectType() == Object_TextField)
    {
        GASTextField* ptextField = static_cast<GASTextField*>(focusedChar.GetPtr());
        fn.Result->SetNumber((GASNumber)ptextField->GetBeginIndex());
    }
}

void GASSelectionCtorFunction::GetEndIndex(const GASFnCall& fn)
{
    fn.Result->SetNumber(-1);
    if (!fn.Env)
        return;

    UInt controllerIdx = 0;
    if (fn.Env->CheckExtensions())
    {
        if (fn.NArgs >= 1)
            controllerIdx = fn.Arg(0).ToUInt32(fn.Env);
    }
    GPtr<GFxASCharacter> focusedChar = fn.Env->GetMovieRoot()->GetFocusedCharacter(controllerIdx);
    if (focusedChar && focusedChar->GetObjectType() == Object_TextField)
    {
        GASTextField* ptextField = static_cast<GASTextField*>(focusedChar.GetPtr());
        fn.Result->SetNumber((GASNumber)ptextField->GetEndIndex());
    }
}

void GASSelectionCtorFunction::CaptureFocus(const GASFnCall& fn)
{
    fn.Result->SetUndefined();

    bool capture;
    if (fn.NArgs >= 1)
        capture = fn.Arg(0).ToBool(fn.Env);
    else
        capture = true;
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();

    UInt controllerIdx = 0;
    if (fn.NArgs >= 2)
        controllerIdx = fn.Arg(1).ToUInt32(fn.Env);

    GPtr<GFxASCharacter> focusedChar = proot->GetFocusedCharacter(controllerIdx);
    if (!focusedChar)
    {
        // if no characters currently focused - simulate pressing of Tab
        proot->ActivateFocusCapture(controllerIdx);
        focusedChar = proot->GetFocusedCharacter(controllerIdx);
    }
    if (capture)
    {
        if (focusedChar && focusedChar->IsFocusEnabled())
        {
             proot->SetKeyboardFocusTo(focusedChar, controllerIdx);
        }
    }
    else
    {
        proot->HideFocusRect(controllerIdx);
    }
    if (focusedChar)
        fn.Result->SetAsCharacter(focusedChar);
}

// 1st parameter - strings "up", "down", "left", "right", "tab".
// 2nd parameter (optional) - the character to begin search from.
// 3rd parameter (optional) - bool, true to include only focusEnabled = true chars
// 4th parameter (optional) - keyboard index
void GASSelectionCtorFunction::MoveFocus(const GASFnCall& fn)
{
    fn.Result->SetUndefined();

    if (fn.NArgs == 0)
        return;

    UInt controllerIdx = 0;
    if (fn.NArgs >= 4)
        controllerIdx = fn.Arg(3).ToUInt32(fn.Env);

    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    GPtr<GFxASCharacter> startChar;
    if (fn.NArgs >= 2 && !fn.Arg(1).IsUndefined() && !fn.Arg(1).IsNull())        
        startChar = fn.Arg(1).ToASCharacter(fn.Env);
    else
        startChar = proot->GetFocusedCharacter(controllerIdx).GetPtr();

    bool includeFocusEnabled = (fn.NArgs >= 3) ? 
        fn.Arg(2).ToBool(fn.Env) : false;

    GASString directionStr = fn.Arg(0).ToString(fn.Env);
    short keycode;
    GFxSpecialKeysState specKeysState;
    if (directionStr == "up")
        keycode = GFxKey::Up;
    else if (directionStr == "down")
        keycode = GFxKey::Down;
    else if (directionStr == "left")
        keycode = GFxKey::Left;
    else if (directionStr == "right")
        keycode = GFxKey::Right;
    else if (directionStr == "tab")
        keycode = GFxKey::Tab;
    else if (directionStr == "shifttab")
    {
        keycode = GFxKey::Tab;
        specKeysState.SetShiftPressed(true);
    }
    else
    {
        if (fn.Env->GetLog())
            fn.Env->GetLog()->LogWarning("moveFocus: invalid string id for key: '%s'\n",
            directionStr.ToCStr());
        return;
    }
    GFxMovieRoot::ProcessFocusKeyInfo focusInfo;
    GFxInputEventsQueue::QueueEntry::KeyEntry ke;
    ke.KeyboardIndex            = (UInt8)controllerIdx;
    ke.Code                     = keycode;
    ke.SpecialKeysState         = specKeysState.States;
    proot->InitFocusKeyInfo(&focusInfo, ke, includeFocusEnabled);
    focusInfo.CurFocused        = startChar;
    focusInfo.ManualFocus       = true;
    proot->ProcessFocusKey(GFxEvent::KeyDown, ke, &focusInfo);
    proot->FinalizeProcessFocusKey(&focusInfo);
    fn.Result->SetAsCharacter(focusInfo.CurFocused);
}

// 1st parameter - strings "up", "down", "left", "right", "tab".
// 2nd parameter - a parent container (panel), or null
// 3rd           - bool, if true then loop
// 4th parameter (optional) - the character to begin search from.
// 5th parameter (optional) - bool, true to include only focusEnabled = true chars
// 6th parameter (optional) - keyboard index
void GASSelectionCtorFunction::FindFocus(const GASFnCall& fn)
{
    fn.Result->SetUndefined();

    if (fn.NArgs == 0)
        return;

    UInt controllerIdx = 0;
    if (fn.NArgs >= 6)
        controllerIdx = fn.Arg(5).ToUInt32(fn.Env);

    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    GPtr<GFxASCharacter> startChar;
    if (fn.NArgs >= 4 && !fn.Arg(3).IsUndefined() && !fn.Arg(3).IsNull())        
        startChar = fn.Arg(3).ToASCharacter(fn.Env);
    else
        startChar = proot->GetFocusedCharacter(controllerIdx).GetPtr();

    bool includeFocusEnabled = (fn.NArgs >= 5) ? 
        fn.Arg(4).ToBool(fn.Env) : false;

    GASString directionStr = fn.Arg(0).ToString(fn.Env);
    short keycode;
    GFxSpecialKeysState specKeysState;
    if (directionStr == "up")
        keycode = GFxKey::Up;
    else if (directionStr == "down")
        keycode = GFxKey::Down;
    else if (directionStr == "left")
        keycode = GFxKey::Left;
    else if (directionStr == "right")
        keycode = GFxKey::Right;
    else if (directionStr == "tab")
        keycode = GFxKey::Tab;
    else if (directionStr == "shifttab")
    {
        keycode = GFxKey::Tab;
        specKeysState.SetShiftPressed(true);
    }
    else
    {
        /* // PPS: Warning causes unnecessary spew in log with CLIK. CLIK's FocusHandler
           // passes all unhandled key events to findFocus.
        if (fn.Env->GetLog())
            fn.Env->GetLog()->LogWarning("findFocus: invalid string id for key: '%s'\n",
            directionStr.ToCStr());
        */
        return;
    }

    GPtr<GFxASCharacter> panelChar;
    if (fn.NArgs >= 2)
    {
        GASValue context = fn.Arg(1);
        if (!context.IsNull() && !context.IsUndefined())
            panelChar = context.ToASCharacter(fn.Env);
        else
        {
            // take a modal clip as panelChar, if the parameter is "null"
            panelChar = proot->GetModalClip(controllerIdx);
        }
    }

    bool looping = (fn.NArgs >= 3) ? fn.Arg(2).ToBool(fn.Env) : false;

    GFxMovieRoot::ProcessFocusKeyInfo focusInfo;
    GFxInputEventsQueue::QueueEntry::KeyEntry ke;
    ke.KeyboardIndex            = (UInt8)controllerIdx;
    ke.Code                     = keycode;
    ke.SpecialKeysState         = specKeysState.States;
    GFxMovieRoot::FocusGroupDescr focusGroup(fn.Env->GetHeap()); 
    focusGroup.ModalClip        = (panelChar) ? panelChar->GetCharacterHandle() : NULL;
    focusGroup.LastFocused      = startChar;
    proot->InitFocusKeyInfo(&focusInfo, ke, includeFocusEnabled, &focusGroup);
    focusInfo.ManualFocus       = true;
    proot->ProcessFocusKey(GFxEvent::KeyDown, ke, &focusInfo);
    if (focusInfo.CurFocused && focusInfo.CurFocused != startChar)
        fn.Result->SetAsCharacter(focusInfo.CurFocused);
    else
    {
        if (!looping || focusGroup.TabableArray.GetSize() == 0)
            fn.Result->SetNull();
        else
        {
            if (keycode == GFxKey::Tab)
            {
                if (!specKeysState.IsShiftPressed())
                    fn.Result->SetAsCharacter(focusGroup.TabableArray[0]);
                else
                    fn.Result->SetAsCharacter(focusGroup.TabableArray[focusGroup.TabableArray.GetSize()-1]);
            }
            else
            {
                Float lastV = GFC_MIN_FLOAT;
                UPInt index = 0;
                for (UPInt i = 0, n = focusGroup.TabableArray.GetSize(); i < n; ++i)
                {
                    GPtr<GFxASCharacter> b = focusGroup.TabableArray[i];
                    if ((!focusInfo.InclFocusEnabled && !b->IsTabable()) ||
                        !b->IsFocusAllowed(proot, focusInfo.KeyboardIndex))
                    {
                        // If this is not for manual focus and not tabable - ignore.
                        continue;
                    }
                    GFxCharacter::Matrix mb = b->GetLevelMatrix();
                    GRectF bRect  = mb.EncloseTransform(b->GetFocusRect());
                    switch (keycode)
                    {
                    case GFxKey::Down: 
                        // find top
                        if (lastV == GFC_MIN_FLOAT || bRect.Top < lastV)
                        {
                            lastV = bRect.Top;
                            index = i;
                        }
                        break; //
                    case GFxKey::Up: 
                        // find bottom
                        if (lastV == GFC_MIN_FLOAT || bRect.Bottom > lastV)
                        {
                            lastV = bRect.Bottom;
                            index = i;
                        }
                        break; //
                    case GFxKey::Left: 
                        // find most right
                        if (lastV == GFC_MIN_FLOAT || bRect.Right > lastV)
                        {
                            lastV = bRect.Right;
                            index = i;
                        }
                        break; //
                    case GFxKey::Right: 
                        // find most left
                        if (lastV == GFC_MIN_FLOAT || bRect.Left < lastV)
                        {
                            lastV = bRect.Left;
                            index = i;
                        }
                        break; //
                    default: GASSERT(0);
                    }
                }
                fn.Result->SetAsCharacter(focusGroup.TabableArray[index]);
            }
        }
    }
}

// 1st param - modal clip
// 2nd param - controller id
void GASSelectionCtorFunction::SetModalClip(const GASFnCall& fn)
{
    fn.Result->SetUndefined();

    if (fn.NArgs < 1)
        return;

    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    GFxASCharacter* pchar = fn.Arg(0).ToASCharacter(fn.Env);
    UInt controllerIdx = (fn.NArgs >= 2) ? fn.Arg(1).ToUInt32(fn.Env) : 0;
    if (pchar && pchar->GetObjectType() == GASObjectInterface::Object_Sprite)
    {
        GFxSprite* pspr = static_cast<GFxSprite*>(pchar);
        proot->SetModalClip(pspr, controllerIdx);
    }
    else
        proot->SetModalClip(NULL, controllerIdx);
}

// 1st param - controller id
// ret - modal clip
void GASSelectionCtorFunction::GetModalClip(const GASFnCall& fn)
{
    fn.Result->SetUndefined();

    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    UInt controllerIdx = (fn.NArgs >= 1) ? fn.Arg(0).ToUInt32(fn.Env) : 0;
    GFxSprite* pmodalClip = proot->GetModalClip(controllerIdx);
    fn.Result->SetAsCharacter(pmodalClip);
}

// 1st param - an id of logical controller (focus group index)
// returns a bit-mask where each bit represents a physical controller, 
// associated with the specified focus group.
void GASSelectionCtorFunction::GetControllerMaskByFocusGroup(const GASFnCall& fn)
{
    fn.Result->SetUndefined();

    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    UInt focusGroupIdx = (fn.NArgs >= 1) ? fn.Arg(0).ToUInt32(fn.Env) : 0;
    fn.Result->SetNumber((int)proot->GetControllerMaskByFocusGroup(focusGroupIdx));
}

// 1st param - controller idx
// 2nd param - focus group idx
// ret - bool, true, if succ
void GASSelectionCtorFunction::SetControllerFocusGroup(const GASFnCall& fn)
{
    fn.Result->SetUndefined();

    if (fn.NArgs < 2)
        return;

    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    UInt controllerIdx = fn.Arg(0).ToUInt32(fn.Env);
    UInt groupIdx      = fn.Arg(1).ToUInt32(fn.Env);
    fn.Result->SetBool(proot->SetControllerFocusGroup(controllerIdx, groupIdx));
}

// 1st param - controller id
// ret - focus group idx
void GASSelectionCtorFunction::GetControllerFocusGroup(const GASFnCall& fn)
{
    fn.Result->SetUndefined();

    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    UInt controllerIdx = (fn.NArgs >= 1) ? fn.Arg(0).ToUInt32(fn.Env) : 0;
    fn.Result->SetNumber(proot->GetControllerFocusGroup(controllerIdx));
}

bool GASSelectionCtorFunction::GetMember(GASEnvironment* penv, const GASString& name, GASValue* pval)
{
    if (penv->CheckExtensions())
    {
        GFxMovieRoot* proot = penv->GetMovieRoot();
        // Extensions are always case-insensitive
        if (name == "captureFocus") 
        {
            *pval = GASValue(penv->GetSC(), GASSelectionCtorFunction::CaptureFocus);
            return true;
        }
        else if (name == "disableFocusAutoRelease")
        {
            if (proot->IsDisableFocusAutoReleaseSet())
                pval->SetBool(proot->IsDisableFocusAutoRelease());
            else
                pval->SetUndefined();
            return true;
        }
        else if (name == "alwaysEnableArrowKeys")
        {
            if (proot->IsAlwaysEnableFocusArrowKeysSet())
                pval->SetBool(proot->IsAlwaysEnableFocusArrowKeys());
            else
                pval->SetUndefined();
            return true;
        }
        else if (name == "alwaysEnableKeyboardPress")
        {
            if (proot->IsAlwaysEnableKeyboardPressSet())
                pval->SetBool(proot->IsAlwaysEnableKeyboardPress());
            else
                pval->SetUndefined();
            return true;
        }
        else if (name == "disableFocusRolloverEvent")
        {
            if (proot->IsDisableFocusRolloverEventSet())
                pval->SetBool(proot->IsDisableFocusRolloverEvent());
            else
                pval->SetUndefined();
            return true;
        }
        else if (name == "disableFocusKeys")
        {
            if (proot->IsDisableFocusKeysSet())
                pval->SetBool(proot->IsDisableFocusKeys());
            else
                pval->SetUndefined();
            return true;
        }
        else if (name == "modalClip")
        {
            GFxSprite* pmodalClip = proot->GetModalClip(0);
            pval->SetAsCharacter(pmodalClip);
            return true;
        }
        else if (name == "moveFocus") 
        {
            *pval = GASValue(penv->GetSC(), GASSelectionCtorFunction::MoveFocus);
            return true;
        }
        else if (name == "findFocus") 
        {
            *pval = GASValue(penv->GetSC(), GASSelectionCtorFunction::FindFocus);
            return true;
        }
        else if (name == "setModalClip") 
        {
            *pval = GASValue(penv->GetSC(), GASSelectionCtorFunction::SetModalClip);
            return true;
        }
        else if (name == "getModalClip") 
        {
            *pval = GASValue(penv->GetSC(), GASSelectionCtorFunction::GetModalClip);
            return true;
        }
        else if (name == "setControllerFocusGroup") 
        {
            *pval = GASValue(penv->GetSC(), GASSelectionCtorFunction::SetControllerFocusGroup);
            return true;
        }
        else if (name == "getControllerFocusGroup") 
        {
            *pval = GASValue(penv->GetSC(), GASSelectionCtorFunction::GetControllerFocusGroup);
            return true;
        }
        else if (name == "getFocusBitmask") 
        {
            *pval = GASValue(penv->GetSC(), GASSelectionCtorFunction::GetFocusBitmask);
            return true;
        }
        else if (name == "numFocusGroups") 
        {
            *pval = GASValue(int(proot->GetFocusGroupCount()));
            return true;
        }
        else if (name == "getControllerMaskByFocusGroup")
        {
            *pval = GASValue(penv->GetSC(), GASSelectionCtorFunction::GetControllerMaskByFocusGroup);
            return true;
        }
        else if (name == "getFocusArray") 
        {
            *pval = GASValue(penv->GetSC(), GASSelectionCtorFunction::GetFocusArray);
            return true;
        }
    }
    return GASFunctionObject::GetMember(penv, name, pval);
}

bool GASSelectionCtorFunction::SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{
    if (penv->CheckExtensions())
    {
        GFxMovieRoot* proot = penv->GetMovieRoot();
        if (name == "disableFocusAutoRelease")
        {
            proot->SetDisableFocusAutoRelease(val.ToBool(penv));
            return true;
        }
        else if (name == "alwaysEnableArrowKeys")
        {
            proot->SetAlwaysEnableFocusArrowKeys(val.ToBool(penv));
            return true;
        }
        else if (name == "alwaysEnableKeyboardPress")
        {
            proot->SetAlwaysEnableKeyboardPress(val.ToBool(penv));
            return true;
        }
        else if (name == "disableFocusRolloverEvent")
        {
            proot->SetDisableFocusRolloverEvent(val.ToBool(penv));
            return true;
        }
        else if (name == "disableFocusKeys")
        {
            proot->SetDisableFocusKeys(val.ToBool(penv));
            return true;
        }
        else if (name == "modalClip")
        {
            GFxASCharacter* pchar = val.ToASCharacter(penv);
            if (pchar && pchar->GetObjectType() == GASObjectInterface::Object_Sprite)
            {
                GFxSprite* pspr = static_cast<GFxSprite*>(pchar);
                proot->SetModalClip(pspr, 0);
            }
            else
                proot->SetModalClip(NULL, 0);
            return true;
        }
    }
    return GASFunctionObject::SetMember(penv, name, val, flags);
}

void GASSelectionCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    fn.Result->SetNull();
}

GASFunctionRef GASSelectionCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef  ctor(*GHEAP_NEW(pgc->GetHeap()) GASSelectionCtorFunction(&sc));
    GPtr<GASObject> proto = *GHEAP_NEW(pgc->GetHeap()) GASSelectionProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_Selection, proto);
    pgc->pGlobal->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_Selection), GASValue(ctor));
    return ctor;
}
#endif //#ifndef GFC_NO_FXPLAYER_AS_SELECTION
