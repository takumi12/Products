/**********************************************************************

Filename    :   GFxMouse.cpp
Content     :   Implementation of Mouse class
Created     :   November, 2006
Authors     :   Artyom Bolgar

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GRefCount.h"
#include "GFxLog.h"
#include "GFxString.h"
#include "GFxAction.h"
#include "AS/GASArrayObject.h"
#include "AS/GASNumberObject.h"
#include "GFxSprite.h"
#include "AS/GASMouse.h"
#include "AS/GASPointObject.h"
#include "GUTF8Util.h"

#ifndef GFC_NO_FXPLAYER_AS_MOUSE

GASMouse::GASMouse(GASEnvironment* penv)
: GASObject(penv)
{
    commonInit(penv);
}

void GASMouse::commonInit (GASEnvironment* penv)
{
    Set__proto__ (penv->GetSC(), penv->GetPrototype(GASBuiltin_Mouse));
}
    
//////////////////////////////////////////
//
static void GFx_MouseFuncStub(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
}

static const GASNameFunction GAS_MouseFunctionTable[] = 
{
    { "addListener",        &GFx_MouseFuncStub },
    { "removeListener",     &GFx_MouseFuncStub },
    { "show",               &GFx_MouseFuncStub },
    { "hide",               &GFx_MouseFuncStub },
    { 0, 0 }
};

GASMouseProto::GASMouseProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASMouse>(psc, pprototype, constructor)
{
    InitFunctionMembers (psc, GAS_MouseFunctionTable);
}

//////////////////
const GASNameFunction GASMouseCtorFunction::StaticFunctionTable[] = 
{
#ifdef GFC_NO_GC
    { "addListener",        &GASMouseCtorFunction::AddListener        },
    { "removeListener",     &GASMouseCtorFunction::RemoveListener     },
#endif // GFC_NO_GC
    { "show",               &GASMouseCtorFunction::Show               },
    { "hide",               &GASMouseCtorFunction::Hide               },
    { 0, 0 }
};

GASMouseCtorFunction::GASMouseCtorFunction (GASStringContext *psc, GFxMovieRoot* proot) :
    GASCFunctionObject(psc, GlobalCtor)
{
    GASSERT(proot->pASMouseListener == 0); // shouldn't be set!
    proot->pASMouseListener = this;

#ifndef GFC_NO_GC
    GASAsBroadcaster::Initialize(psc, this);
    UpdateListenersArray(psc);
#endif // GFC_NO_GC

    GASNameFunction::AddConstMembers(
        this, psc, StaticFunctionTable, 
        GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);

    SetCursorTypeFunc = GASValue(psc, GASMouseCtorFunction::SetCursorType).ToFunction(NULL);
    LastClickTime = 0;
}

#ifndef GFC_NO_GC
// CC version
template <class Functor>
void GASMouseCtorFunction::ForEachChild_GC(Collector* prcc) const
{
    SetCursorTypeFunc.template ForEachChild_GC<Functor>(prcc);
    if (pListenersArray)
        Functor::Call(prcc, pListenersArray);
    GASCFunctionObject::template ForEachChild_GC<Functor>(prcc);
}
GFC_GC_PREGEN_FOR_EACH_CHILD(GASMouseCtorFunction)

void GASMouseCtorFunction::ExecuteForEachChild_GC(Collector* prcc, OperationGC operation) const
{
    GASRefCountBaseType::CallForEachChild<GASMouseCtorFunction>(prcc, operation);
}

void GASMouseCtorFunction::Finalize_GC()
{
    GASCFunctionObject::Finalize_GC();
    SetCursorTypeFunc.Finalize_GC();
}

bool GASMouseCtorFunction::IsEmpty() const 
{ 
    if (pListenersArray)
        return (pListenersArray->GetSize() == 0);
    return false;
}

void GASMouseCtorFunction::UpdateListenersArray(GASStringContext *psc, GASEnvironment* penv)
{
    GASValue listenersVal;
    if (GetMemberRaw(psc,
        psc->GetBuiltin(GASBuiltin__listeners), &listenersVal))
    {
        GASObject* pobj = listenersVal.ToObject(penv);
        if (pobj && pobj->GetObjectType() == Object_Array)
            pListenersArray = static_cast<GASArrayObject*>(pobj);
        else
            pListenersArray = NULL;
    }
}

#else 
// non-CC methods
// Remove dead entries in the listeners list.  (Since
// we use WeakPtr's, listeners can disappear without
// notice.)
void    GASMouseCtorFunction::CleanupListeners()
{
    for (int i = (int)ListenerWeakRefs.GetSize() - 1; i >= 0; i--)
    {
        if (ListenerWeakRefs[i] == NULL)
        {
            Listeners.RemoveAt(i);
            ListenerWeakRefs.RemoveAt(i);
        }
    }
}

void    GASMouseCtorFunction::AddListener(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
    {
        fn.Env->LogScriptError("Error: Mouse.addListener needs one argument (the listener object)\n");
        return;
    }

    GASObject*          listenerObj     = fn.Arg(0).IsObject() ? fn.Arg(0).ToObject(fn.Env) : 0;
    GFxASCharacter*     listenerChar    = fn.Arg(0).ToASCharacter(fn.Env);
    GASObjectInterface* listener;
    GRefCountWeakSupportImpl* listenerRef;

    if (listenerObj)
    {
        listener    = listenerObj;
        listenerRef = listenerObj;  
    }
    else
    {
        listener    = listenerChar;
        listenerRef = listenerChar; 
    }

    if (listener == NULL)
    {
        fn.Env->LogScriptError("Error: Mouse.addListener passed a NULL object; ignored\n");
        return;
    }

    GASMouseCtorFunction* mo = static_cast<GASMouseCtorFunction*>(fn.ThisPtr);
    GASSERT(mo);

    mo->CleanupListeners();

    for (UPInt i = 0, n = mo->Listeners.GetSize(); i < n; i++)
    {
        if (mo->Listeners[i] == listener)
        {
            // Already in the list.
            return;
        }
    }

    mo->Listeners.PushBack(listener);
    mo->ListenerWeakRefs.PushBack(listenerRef);
}

void    GASMouseCtorFunction::RemoveListener(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
    {
        fn.Env->LogScriptError("Error: Mouse.removeListener needs one Argument (the listener object)\n");
        return;
    }

    GASObject*          listenerObj     = fn.Arg(0).IsObject() ? fn.Arg(0).ToObject(fn.Env) : 0;
    GFxASCharacter*     listenerChar    = fn.Arg(0).ToASCharacter(fn.Env);
    GASObjectInterface* listener;
    GRefCountWeakSupportImpl*  listenerRef;

    if (listenerObj)
    {
        listener    = listenerObj;
        listenerRef = listenerObj;  
    }
    else
    {
        listener    = listenerChar;
        listenerRef = listenerChar; 
    }

    if (listener == NULL)
    {
        fn.Env->LogScriptError("Error: Mouse.removeListener passed a NULL object; ignored\n");
        return;
    }

    GASMouseCtorFunction* mo = static_cast<GASMouseCtorFunction*>(fn.ThisPtr);
    GASSERT(mo);

    mo->CleanupListeners();

    for (SPInt i = (SPInt)mo->Listeners.GetSize() - 1; i >= 0; i--)
    {
        if (mo->Listeners[i] == listener)
        {
            mo->Listeners.RemoveAt(i);
            mo->ListenerWeakRefs.RemoveAt(i);
        }
    }
}

bool GASMouseCtorFunction::IsEmpty() const 
{ 
    return Listeners.GetSize() == 0; 
}
#endif //GFC_NO_GC

// Pushes parameters for listeners into stack; returns number of pushed params.
int GASMouseCtorFunction::PushListenersParams(GASEnvironment* penv, 
                                              UInt mouseIndex, 
                                              GASBuiltinType eventName, 
                                              const GASValue& eventMethod, 
                                              const GASString* ptargetName, 
                                              UInt button, 
                                              int delta,
                                              bool dblClick)
{
    GASSERT(penv);

    bool noExtraParams = !penv->CheckExtensions();
    if (penv->CheckExtensions() && button > 0 && 
        (eventName == GASBuiltin_onMouseDown || eventName == GASBuiltin_onMouseUp))
    {
        // check for number of parameters for onMouseDown/onMouseUp handler functions;
        // if more than zero - we can call them for right and middle buttons and we can
        // pass extra parameters for the left button's handler. This is done for compatibility.
        GASFunctionRef mfref = eventMethod.ToFunction(penv);
        if (!mfref.IsNull())
        {
            if (mfref->GetNumArgs() <= 0)
            {
                if (button > 1)
                    return -1; // do not call handlers for right/middle buttons, since handler has 0 parameters
                noExtraParams = true; // do not pass extra parameters in left button's handler
            }
        }
        else
            return -1; // method is empty! (assertion?)
    }
    int nArgs = 0;
    if (penv->CheckExtensions() && !noExtraParams) 
    {
        if (eventName == GASBuiltin_onMouseDown && dblClick)
        {
            // onMouseDown, the last parameter indicates doubleClick, if true.
            penv->Push(dblClick);
            ++nArgs;
        }
        const GFxMouseState* pms = penv->GetMovieRoot()->GetMouseState(mouseIndex);
        GPointF pt = pms->GetLastPosition();
        penv->Push((GASNumber)TwipsToPixels(floor(pt.y + 0.5)));
        penv->Push((GASNumber)TwipsToPixels(floor(pt.x + 0.5)));
        penv->Push((int)mouseIndex);
        nArgs += 3;
    }
    if (eventName != GASBuiltin_onMouseMove)
    {
        if (ptargetName && (eventName == GASBuiltin_onMouseWheel || !noExtraParams))
        {
            // push target for onMouseWheel and onMouseDown/Up, if allowed
            penv->Push(*ptargetName);
            ++nArgs;
        }
        else if (nArgs > 0)
        {
            // push null as a placeholder
            penv->Push(GASValue::NULLTYPE);
            ++nArgs;
        }
    }
    switch(eventName)
    {
    case GASBuiltin_onMouseWheel:
        penv->Push(delta);
        ++nArgs;
        break;
    case GASBuiltin_onMouseDown:
    case GASBuiltin_onMouseUp:
        if (button && !noExtraParams)
        {
            penv->Push(GASNumber(button));
            ++nArgs;
        }
        else if (nArgs > 0)
        {
            // push null as a placeholder
            penv->Push(GASValue::NULLTYPE);
            ++nArgs;
        }
        break;

    case GASBuiltin_onMouseMove:
        // pass (x, y, mouseIndex) as parameters of mouse move handler, if extensions
        // are on.
        break;

    default:
        break;
    }
    return nArgs;
}

void GASMouseCtorFunction::NotifyListeners
    (GASEnvironment *penv, UInt mouseIndex, GASBuiltinType eventName, 
     const GASString* ptargetName, UInt button, int delta, bool dblClick) const
{
    // GFx extends functionality of mouse listener handlers:
    // onMouseMove(mouseIndex:Number, x:Number, y:Number)
    // onMouseDown(button:Number, target:Object, mouseIndex:Number, x:Number, y:Number)
    // onMouseUp  (button:Number, target:Object, mouseIndex:Number, x:Number, y:Number)
    // onMouseWheel(delta:Number, target:Object, mouseIndex:Number, x:Number, y:Number)
    // Note, onMouseDown/onMouseUp might be invoked for left and middle buttons, but
    // AS functions for these handlers should be declared with parameters. Thus,
    // if you have empty parameter list for onMouseDown/Up it will be invoked only
    // for left mouse button.
#ifdef GFC_NO_GC
    // Non-CC code
    if (Listeners.GetSize() > 0)
    {
        struct ListenersCopyStruct : public GNewOverrideBase<GFxStatMV_ActionScript_Mem>
        {
            GArrayLH<GWeakPtr<GRefCountWeakSupportImpl> >   ListenerWeakRefs;
            GArrayLH<GASObjectInterface*>                       Listeners;

            ListenersCopyStruct(const GArrayLH<GWeakPtr<GRefCountWeakSupportImpl> > &weak,
                                const GArrayLH<GASObjectInterface*> &listeners)
                : ListenerWeakRefs(weak), Listeners(listeners) { }
        };

        // Copy of listeners needs to be created in AS heap (not on stack).
        ListenersCopyStruct* plcopy = 
            GHEAP_NEW(penv->GetHeap()) ListenersCopyStruct(ListenerWeakRefs, Listeners);

        // Notify listeners.
        UPInt i;
        UPInt n = plcopy->Listeners.GetSize();
        for (i = 0; i < n; i++)
        {
            GPtr<GRefCountWeakSupportImpl>  listernerRef = plcopy->ListenerWeakRefs[i];

            if (listernerRef)
            {
                GASValue            method;
                GASObjectInterface* listener = plcopy->Listeners[i];                    
                GASEnvironment *    penvtemp = penv;
                if (listener->IsASCharacter())
                    penvtemp = listener->ToASCharacter()->GetASEnvironment();
                GASStringContext *  psc = penvtemp->GetSC(); 
                if (listener
                    && listener->GetMemberRaw(psc, psc->GetBuiltin(eventName), &method))
                {
                    GASSERT(penvtemp);

                    int nArgs = PushListenersParams(penvtemp, mouseIndex, eventName, method, ptargetName, button, delta, dblClick);
                    if (nArgs >= 0)
                    {
                        GAS_Invoke(method, NULL, listener, penvtemp, nArgs, penvtemp->GetTopIndex(), NULL);
                        penvtemp->Drop(nArgs);
                    }
                }
            }    
        }

        delete plcopy;
    }
#else // GFC_NO_GC
    // CC version
    LocalInvokeCallback Callback;
    Callback.MouseIndex     = mouseIndex;
    Callback.EventName      = eventName;
    Callback.pTargetName    = ptargetName;
    Callback.Button         = button;
    Callback.Delta          = delta;
    Callback.DoubleClick    = dblClick;
    GASAsBroadcaster::BroadcastMessageWithCallback(penv, const_cast<GASMouseCtorFunction*>(this), 
        penv->GetBuiltin(eventName), &Callback);
#endif // GFC_NO_GC
} 

void GASMouseCtorFunction::OnMouseMove(GASEnvironment *penv, UInt mouseIndex) const
{
    NotifyListeners(penv, mouseIndex, GASBuiltin_onMouseMove);
}

void GASMouseCtorFunction::OnMouseDown(GASEnvironment *penv, UInt mouseIndex, UInt button, GFxASCharacter* ptarget) const
{
    bool doubleClicked = false;
    if (penv->CheckExtensions())
    {
        // detect double click
        UInt32 tm = (UInt32)(GTimer::GetTicks() / 1000);

        const GFxMouseState* pms = penv->GetMovieRoot()->GetMouseState(mouseIndex);
        GPointF pt = pms->GetLastPosition();
        int ax = (int)TwipsToPixels(pt.x);
        int ay = (int)TwipsToPixels(pt.y);

        // detect double click
        if (tm <= LastClickTime + GFX_DOUBLE_CLICK_TIME_MS && LastMousePos.x == ax && LastMousePos.y == ay)
        {
            doubleClicked = true;
            //printf("DOUBLE CLICK!\n");
        }
        //printf("new x = %f    y = %f     tm = %d\n", ax, ay, tm);
        LastMousePos.x = ax;
        LastMousePos.y = ay;
        LastClickTime  = tm;
    }
    if (ptarget)
    {
        GASString stringVal = ptarget->GetCharacterHandle()->GetNamePath();
        NotifyListeners(penv, mouseIndex, GASBuiltin_onMouseDown, &stringVal, button, 0, doubleClicked);
    }
    else
        NotifyListeners(penv, mouseIndex, GASBuiltin_onMouseDown, NULL, button, 0, doubleClicked);
}

void GASMouseCtorFunction::OnMouseUp(GASEnvironment *penv, UInt mouseIndex, UInt button, GFxASCharacter* ptarget) const
{
    if (ptarget)
    {
        GASString stringVal = ptarget->GetCharacterHandle()->GetNamePath();
        NotifyListeners(penv, mouseIndex, GASBuiltin_onMouseUp, &stringVal, button);
    }
    else
        NotifyListeners(penv, mouseIndex, GASBuiltin_onMouseUp, NULL, button);
}

void GASMouseCtorFunction::OnMouseWheel(GASEnvironment *penv, UInt mouseIndex, int sdelta, GFxASCharacter* ptarget) const
{
    if (ptarget)
    {
        GASString stringVal = ptarget->GetCharacterHandle()->GetNamePath();
        NotifyListeners(penv, mouseIndex, GASBuiltin_onMouseWheel, &stringVal, 0, sdelta);
    }
    else
        NotifyListeners(penv, mouseIndex, GASBuiltin_onMouseWheel, 0, 0, sdelta);
}

void    GASMouseCtorFunction::Show(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    if (proot->pUserEventHandler)
    {
        UInt mouseIndex = 0;
        if (fn.NArgs >= 1)
            mouseIndex = fn.Arg(0).ToUInt32(fn.Env);
        proot->pUserEventHandler->HandleEvent(proot, GFxMouseCursorEvent(GFxEvent::DoShowMouse, mouseIndex));
    }
    else
        fn.Env->LogScriptWarning("Warning: no user event handler interface is installed; Mouse.show failed.\n");
}

void    GASMouseCtorFunction::Hide(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    if (proot->pUserEventHandler)
    {
        UInt mouseIndex = 0;
        if (fn.NArgs >= 1)
            mouseIndex = fn.Arg(0).ToUInt32(fn.Env);
        proot->pUserEventHandler->HandleEvent(proot, GFxMouseCursorEvent(GFxEvent::DoHideMouse, mouseIndex));
    }
    else
        fn.Env->LogScriptWarning("Warning: no user event handler interface is installed; Mouse.hide failed.\n");
}

void    GASMouseCtorFunction::SetCursorType(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    UInt cursorShapeId = GFxMouseCursorEvent::ARROW;
    if (fn.NArgs > 0)
        cursorShapeId = (UInt)fn.Arg(0).ToNumber(fn.Env);
    UInt mouseIndex = 0;
    if (fn.NArgs >= 2)
        mouseIndex = (UInt)fn.Arg(1).ToNumber(fn.Env);
    if (!SetCursorType(proot, mouseIndex, cursorShapeId))
        fn.Env->LogScriptWarning("Warning: no user event handler interface is installed; Mouse.setCursorType failed.\n");
}

bool    GASMouseCtorFunction::SetCursorType(GFxMovieRoot* proot, UInt mouseIndex, UInt cursorType)
{
    GASSERT(proot);

    if (proot->pUserEventHandler)
    {
        proot->pUserEventHandler->HandleEvent(proot, 
            GFxMouseCursorEvent((GFxMouseCursorEvent::CursorShapeType)cursorType, mouseIndex));
    }
    else
        return false;
    return true;
}

void    GASMouseCtorFunction::GetTopMostEntity(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    GPointF mousePos;
    bool testAll = true;
    int params = 0;
    UInt mouseIndex = 0;

    if (fn.NArgs >= 1)
    {
        if (fn.Arg(0).IsBoolean()) // (testAll:boolean, [mouseIndex:int]) variant
        {
            testAll = fn.Arg(0).ToBool(fn.Env);
            if (fn.NArgs >= 2)
                mouseIndex = (int)fn.Arg(1).ToNumber(fn.Env);
            params  = 2;
        }
        else if (fn.NArgs == 1) // (mouseIndex:int) variant
        {
            mouseIndex = (int)fn.Arg(0).ToNumber(fn.Env);
            params  = 1;
        }
        else if (fn.NArgs >= 2) // (x:Number, y:Number, [testAll:boolean])
        {
            if (fn.NArgs >= 3)
                testAll = fn.Arg(2).ToBool(fn.Env);
            params = 3;
        }
    }

    if (params <= 2)
    {
        // one parameter version: parameter is a boolean value, indicates
        // do we need to test all characters (param is true) or only ones with button event handlers (false).
        // Coordinates - current mouse cursor coordinates
        // optional second parameter is the index of mouse.

        // no parameters version: takes current mouse (index = 0) coordinates, testing all.

        if (mouseIndex >= proot->GetMouseCursorCount())
            return; // invalid index of mouse
        GASSERT(proot->GetMouseState(mouseIndex)); // only first mouse is supported for now
        mousePos = proot->GetMouseState(mouseIndex)->GetLastPosition();
    }
    else 
    {
        // three parameters version: _x and _y in _root coordinates, optional boolean - testAll
        Float x = (Float)PixelsToTwips(fn.Arg(0).ToNumber(fn.Env));
        Float y = (Float)PixelsToTwips(fn.Arg(1).ToNumber(fn.Env));

        if (proot->pLevel0Movie)
        {
            // x and y vars are in _root coords. Need to transform them
            // into world coords
            GRenderer::Matrix m = proot->pLevel0Movie->GetWorldMatrix();
            GPointF a(x, y);
            m.Transform(&mousePos, a);
        }
        else
            return;
    }
    GFxASCharacter* ptopCh = proot->GetTopMostEntity(mousePos, mouseIndex, testAll);
    if (ptopCh)
    {
        fn.Result->SetAsCharacter(ptopCh);
    }
}

// returns position of mouse cursor, index of mouse is an optional parameter.
// Returning value is Point object (or Object, if Point class is not included).
// Undefined, if error occurred.
void    GASMouseCtorFunction::GetPosition(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    GPointF mousePos;
    UInt mouseIndex = 0;

    if (fn.NArgs > 0)
        mouseIndex = (int)fn.Arg(0).ToNumber(fn.Env);
    if (mouseIndex >= proot->GetMouseCursorCount())
        return; // invalid index of mouse
    mousePos = proot->GetMouseState(mouseIndex)->GetLastPosition();
#ifndef GFC_NO_FXPLAYER_AS_POINT
    GPtr<GASPointObject> ptObj = *GHEAP_NEW(fn.Env->GetHeap()) GASPointObject(fn.Env);
    GASPoint pt(TwipsToPixels(floor(mousePos.x + 0.5)), 
                TwipsToPixels(floor(mousePos.y + 0.5))); 
    ptObj->SetProperties(fn.Env, pt);
#else
    GPtr<GASObject> ptObj = *GHEAP_NEW(fn.Env->GetHeap()) GASObject(fn.Env);
    GASStringContext *psc = fn.Env->GetSC();
    ptObj->SetConstMemberRaw(psc, "x", TwipsToPixels(floor(mousePos.x + 0.5)));
    ptObj->SetConstMemberRaw(psc, "y", TwipsToPixels(floor(mousePos.y + 0.5)));
#endif
    fn.Result->SetAsObject(ptObj);  
}

// returns status of mouse buttons, index of mouse is an optional parameter.
// Returning value is a number, bit 0 indicates state of left button, 
// bit 1 - right one, bit 2 - middle.
// Undefined, if error occurred.
void    GASMouseCtorFunction::GetButtonsState(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    UInt mouseIndex = 0;

    if (fn.NArgs > 0)
        mouseIndex = (int)fn.Arg(0).ToNumber(fn.Env);
    if (mouseIndex >= proot->GetMouseCursorCount())
        return; // invalid index of mouse
    UInt state = proot->GetMouseState(mouseIndex)->GetButtonsState();
    fn.Result->SetNumber((GASNumber)state);  
}

bool    GASMouseCtorFunction::SetMember
    (GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{
    // Extensions are always case-insensitive
    if (name == penv->GetBuiltin(GASBuiltin_setCursorType))
    {
        if (penv->CheckExtensions())
        {
            SetCursorTypeFunc = val.ToFunction(penv);
            G_SetFlag<GFxMovieRoot::Flag_SetCursorTypeFuncOverloaded>
                (penv->GetMovieRoot()->Flags, HasOverloadedCursorTypeFunction(penv->GetSC()));
        }
    }
#ifndef GFC_NO_GC
    else if (name == penv->GetBuiltin(GASBuiltin__listeners))
    {
        // need to intercept assignment of _listeners (in the case, if 
        // somebody assigns a new array to _listeners) in order to report
        // correctly is the array empty or not (IsEmpty)
        bool rv = GASFunctionObject::SetMember(penv, name, val, flags);
        UpdateListenersArray(penv->GetSC(), penv);
        return rv;
    }
#endif // GFC_NO_GC
    return GASFunctionObject::SetMember(penv, name, val, flags);
}

bool    GASMouseCtorFunction::GetMember(GASEnvironment* penv, const GASString& name, GASValue* pval)
{
    if (penv->CheckExtensions())
    {
        // Extensions are always case-insensitive
        if (name == penv->GetBuiltin(GASBuiltin_setCursorType))
        {
            pval->SetAsFunction(SetCursorTypeFunc);
            return true;
        }
        else if (name == penv->GetBuiltin(GASBuiltin_LEFT))
        {
            pval->SetNumber(Mouse_LEFT);
        }
        else if (name == penv->GetBuiltin(GASBuiltin_RIGHT))
        {
            pval->SetNumber(Mouse_RIGHT);
        }
        else if (name == penv->GetBuiltin(GASBuiltin_MIDDLE))
        {
            pval->SetNumber(Mouse_MIDDLE);
        }
        else if (name == penv->GetBuiltin(GASBuiltin_ARROW))
        {
            pval->SetNumber(GFxMouseCursorEvent::ARROW);
        }
        else if (name == penv->GetBuiltin(GASBuiltin_HAND))
        {
            pval->SetNumber(GFxMouseCursorEvent::HAND);
        }
        else if (name == penv->GetBuiltin(GASBuiltin_IBEAM))
        {
            pval->SetNumber(GFxMouseCursorEvent::IBEAM);
        }
        else if (name == "getTopMostEntity")
        {
            *pval = GASValue(penv->GetSC(), GASMouseCtorFunction::GetTopMostEntity);
            return true;
        }
        else if (name == "getPosition")
        {
            *pval = GASValue(penv->GetSC(), GASMouseCtorFunction::GetPosition);
            return true;
        }
        else if (name == "getButtonsState")
        {
            *pval = GASValue(penv->GetSC(), GASMouseCtorFunction::GetButtonsState);
            return true;
        }
    }
    return GASObject::GetMember(penv, name, pval);
}

bool GASMouseCtorFunction::HasOverloadedCursorTypeFunction(GASStringContext* psc) const
{
    return SetCursorTypeFunc != GASValue(psc, GASMouseCtorFunction::SetCursorType).ToFunction(NULL);
}

void GASMouseCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    fn.Result->SetNull();
}

GASFunctionRef GASMouseCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef  ctor(*GHEAP_NEW(pgc->GetHeap()) GASMouseCtorFunction(&sc, pgc->GetMovieRoot()));
    GPtr<GASObject> proto = *GHEAP_NEW(pgc->GetHeap()) GASMouseProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_Mouse, proto);
    pgc->pGlobal->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_Mouse), GASValue(ctor));
    return ctor;
}
#else

void GASMouse_DummyFunction() {}   // Exists to quelch compiler warning

#endif // GFC_NO_FXPLAYER_AS_MOUSE


