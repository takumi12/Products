/**********************************************************************

Filename    :   AS/GASKeyObject.cpp
Content     :   Implementation of Key class
Created     :   December, 2008
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#include "AS/GASKeyObject.h"
#include "AS/GASArrayObject.h"
#include "AS/GASAsBroadcaster.h"
#include "GFxSprite.h" // to get Environment from level0 movie

#ifndef GFC_NO_KEYBOARD_SUPPORT
//////////////////////////////////////////
//
static void GFx_KeyFuncStub(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
}

static const GASNameFunction GAS_KeyFunctionTable[] = 
{
    {"addListener",     &GFx_KeyFuncStub},
    {"getAscii",        &GFx_KeyFuncStub},
    {"getCode",         &GFx_KeyFuncStub},
    {"isDown",          &GFx_KeyFuncStub},
    {"isToggled",       &GFx_KeyFuncStub},
    {"removeListener",  &GFx_KeyFuncStub},
    { 0, 0 }
};

GASKeyProto::GASKeyProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor)
: GASPrototype<GASKeyObject>(psc, pprototype, constructor)
{
    InitFunctionMembers (psc, GAS_KeyFunctionTable);
}

//////////////////////////////////////////
//
const GASNameFunction GASKeyCtorFunction::StaticFunctionTable[] =
{
#ifdef GFC_NO_GC
    {"addListener",     &GASKeyCtorFunction::KeyAddListener},
    {"removeListener",  &GASKeyCtorFunction::KeyRemoveListener},
#endif // GFC_NO_GC
    {"getAscii",        &GASKeyCtorFunction::KeyGetAscii},
    {"getCode",         &GASKeyCtorFunction::KeyGetCode},
    {"isDown",          &GASKeyCtorFunction::KeyIsDown},
    {"isToggled",       &GASKeyCtorFunction::KeyIsToggled},
    {0, 0}
};

GASKeyCtorFunction::GASKeyCtorFunction(GASStringContext *psc, GFxMovieRoot* proot)
:   GASCFunctionObject(psc, GlobalCtor), 
    pMovieRoot(proot)
{
#ifndef GFC_NO_GC
    GASAsBroadcaster::Initialize(psc, this);
#endif // GFC_NO_GC

    // constants
#define KEY_CONST(kname, key) SetConstMemberRaw(psc, #kname, GFxKey::key)
    KEY_CONST(BACKSPACE, Backspace);
    KEY_CONST(CAPSLOCK, CapsLock);
    KEY_CONST(CONTROL, Control);
    KEY_CONST(DELETEKEY, Delete);
    KEY_CONST(DOWN, Down);
    KEY_CONST(END, End);
    KEY_CONST(ENTER, Return);
    KEY_CONST(ESCAPE, Escape);
    KEY_CONST(HOME, Home);
    KEY_CONST(INSERT, Insert);
    KEY_CONST(LEFT, Left);
    KEY_CONST(PGDN, PageDown);
    KEY_CONST(PGUP, PageUp);
    KEY_CONST(RIGHT, Right);
    KEY_CONST(SHIFT, Shift);
    KEY_CONST(SPACE, Space);
    KEY_CONST(TAB, Tab);
    KEY_CONST(UP, Up);

    // methods
    GASNameFunction::AddConstMembers(this, psc, StaticFunctionTable);

    proot->SetKeyboardListener(this);
}

void GASKeyCtorFunction::Update(int code, UByte ascii, UInt32 wcharCode,
                                UInt8 keyboardIndex)
{
    GASSERT(keyboardIndex < GFX_MAX_CONTROLLERS_SUPPORTED);
    States[keyboardIndex].LastKeyCode         = code;
    States[keyboardIndex].LastAsciiCode       = ascii;
    States[keyboardIndex].LastWcharCode       = wcharCode;
}

void GASKeyCtorFunction::NotifyListeners(GASStringContext *psc, int code, UByte ascii, 
                                         UInt32 wcharCode, int eventId,
                                         UInt8 keyboardIndex)
{
    States[keyboardIndex].LastKeyCode         = code;
    States[keyboardIndex].LastAsciiCode       = ascii;
    States[keyboardIndex].LastWcharCode       = wcharCode;

#ifdef GFC_NO_GC
    // Non-CC version
    if (Listeners.GetSize() > 0)
    {            
        struct ListenersCopyStruct : public GNewOverrideBase<GFxStatMV_ActionScript_Mem>
        {
            GArrayLH<GWeakPtr<GRefCountWeakSupportImpl> >   ListenerWeakRefs;
            GArrayLH<GASObjectInterface* >                  Listeners;

            ListenersCopyStruct(GArrayLH<GWeakPtr<GRefCountWeakSupportImpl> > &weak,
                GArrayLH<GASObjectInterface*> &listeners)
                : ListenerWeakRefs(weak), Listeners(listeners) { }
        };

        // Copy of listeners needs to be created in AS heap (not on stack).
        ListenersCopyStruct* plcopy = GHEAP_NEW(psc->GetHeap()) ListenersCopyStruct(ListenerWeakRefs, Listeners);

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
                GASStringContext *  psctemp = psc;
                if (listener->IsASCharacter())
                    psctemp = listener->ToASCharacter()->GetASEnvironment()->GetSC();

                if (listener
                    && listener->GetMemberRaw(psctemp, GFxEventId(UByte(eventId)).GetFunctionName(psctemp), &method))
                {
                    // MA: Warning: environment should no longer be null.
                    // May be ok because function will substitute its own environment.
                    GAS_Invoke(method, NULL, listener, NULL /* or root? */, 0, 0, NULL);
                }
            }       
        }

        delete plcopy;
    }
#else // GFC_NO_GC
    // CC version
    GASString handlerName = GFxEventId(UByte(eventId)).GetFunctionName(psc);
    
    if (pMovieRoot)
    {
        GFxSprite* plevel = pMovieRoot->GetLevelMovie(0);
        if (plevel)
        {
            GASEnvironment* penv = plevel->GetASEnvironment();
            if (penv)
            {
                int nargs = 0;
                if (penv->CheckExtensions())
                {
                    // push index of controller
                    penv->Push(keyboardIndex);
                    ++nargs;
                }
                GASAsBroadcaster::BroadcastMessage(penv, this, handlerName, nargs, penv->GetTopIndex());
				if (nargs)
					penv->Drop(nargs);
            }
        }    
    }    
#endif // GFC_NO_GC
} 

bool    GASKeyCtorFunction::IsKeyDown(int code, UInt keyboardIndex)
{    
    GFxKeyboardState* pkeyboard = pMovieRoot->GetKeyboardState(keyboardIndex);
    if (pkeyboard)
    {
        return pkeyboard->IsKeyDown(code);
    }
    return false;
}

bool    GASKeyCtorFunction::IsKeyToggled(int code, UInt keyboardIndex)
{    
    GFxKeyboardState* pkeyboard = pMovieRoot->GetKeyboardState(keyboardIndex);
    if (pkeyboard)
    {
        return pkeyboard->IsKeyToggled(code);
    }
    return false;
}

#ifdef GFC_NO_GC
// Non-CC methods

// Remove dead entries in the listeners list.  (Since
// we use WeakPtr's, listeners can disappear without
// notice.)
void    GASKeyCtorFunction::CleanupListeners()
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

void    GASKeyCtorFunction::AddListener(GRefCountWeakSupportImpl *pref, GASObjectInterface* listener)
{
    CleanupListeners();

    for (UPInt i = 0, n = Listeners.GetSize(); i < n; i++)
    {
        if (Listeners[i] == listener)
        {
            // Already in the list.
            return;
        }
    }

    Listeners.PushBack(listener);
    ListenerWeakRefs.PushBack(pref);
}

void    GASKeyCtorFunction::RemoveListener(GRefCountWeakSupportImpl *pref, GASObjectInterface* listener)
{
    GUNUSED(pref);

    CleanupListeners();

    for (int i = (int)Listeners.GetSize() - 1; i >= 0; i--)
    {
        if (Listeners[i] == listener)
        {
            Listeners.RemoveAt(i);
            ListenerWeakRefs.RemoveAt(i);
        }
    }
}

// support for _listeners
bool GASKeyCtorFunction::GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
{
    if (penv && (name == penv->GetBuiltin(GASBuiltin__listeners)))
    {
        // Create a temporary GASArrayObject...
        GPtr<GASObject> obj = *penv->OperatorNew(penv->GetBuiltin(GASBuiltin_Array));
        if (obj)
        {
            GASArrayObject* arrObj = static_cast<GASArrayObject*>(obj.GetPtr());

            // Iterate through listeners.
            UPInt i;
            UPInt n = Listeners.GetSize();
            for (i = 0; i < n; i++)
            {
                GPtr<GRefCountWeakSupportImpl>  listernerRef = ListenerWeakRefs[i];

                if (listernerRef)
                {
                    GASObjectInterface* listener = Listeners[i];
                    GASValue v;
                    if (!listener->IsASCharacter())
                    {
                        GASObject* obj = static_cast<GASObject*>(listener);
                        v.SetAsObject(obj);
                    }
                    else
                    {
                        v.SetAsCharacter(listener->ToASCharacter());
                    }
                    arrObj->PushBack(v);
                }       
            }

        }
        val->SetAsObject(obj);
    }
    return GASObject::GetMember(penv, name, val);
}

// Add a Listener (first arg is object reference) to our list.
// Listeners will have "onKeyDown" and "onKeyUp" methods
// called on them when a key changes state.
void    GASKeyCtorFunction::KeyAddListener(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
    {
        fn.Env->LogScriptError("Error: KeyAddListener needs one Argument (the listener object)\n");
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
        fn.Env->LogScriptError("Error: KeyAddListener passed a NULL object; ignored\n");
        return;
    }

    GASKeyCtorFunction* ko = (GASKeyCtorFunction*)fn.ThisPtr;
    GASSERT(ko);

    ko->AddListener(listenerRef, listener);
}

// Remove a previously-added listener.
void    GASKeyCtorFunction::KeyRemoveListener(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
    {
        fn.Env->LogScriptError("Error: KeyRemoveListener needs one Argument (the listener object)\n");
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
        fn.Env->LogScriptError("Error: KeyRemoveListener passed a NULL object; ignored\n");
        return;
    }

    GASKeyCtorFunction* ko = (GASKeyCtorFunction*)fn.ThisPtr;
    GASSERT(ko);

    ko->RemoveListener(listenerRef, listener);
}
#endif // GFC_NO_GC

// Return the ascii value of the last key pressed.
void    GASKeyCtorFunction::KeyGetAscii(const GASFnCall& fn)
{
    GASKeyCtorFunction* ko = (GASKeyCtorFunction*)fn.ThisPtr;
    GASSERT(ko);

    UInt controllerIdx = 0;
    if (fn.Env->CheckExtensions())
    {
        if (fn.NArgs >= 1)
            controllerIdx = fn.Arg(0).ToUInt32(fn.Env);
    }

    fn.Result->SetInt(ko->GetLastAsciiCode(controllerIdx));
}

// Returns the keycode of the last key pressed.
void    GASKeyCtorFunction::KeyGetCode(const GASFnCall& fn)
{
    GASKeyCtorFunction* ko = (GASKeyCtorFunction*)fn.ThisPtr;
    GASSERT(ko);

    UInt controllerIdx = 0;
    if (fn.Env->CheckExtensions())
    {
        if (fn.NArgs >= 1)
            controllerIdx = fn.Arg(0).ToUInt32(fn.Env);
    }

    fn.Result->SetInt(ko->GetLastKeyCode(controllerIdx));
}

// Return true if the Specified (first arg keycode) key is pressed.
void    GASKeyCtorFunction::KeyIsDown(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
    {
        fn.Env->LogScriptError("Error: KeyIsDown needs one Argument (the key code)\n");
        return;
    }

    int code = fn.Arg(0).ToInt32(fn.Env);

    UInt controllerIdx = 0;
    if (fn.Env->CheckExtensions())
    {
        if (fn.NArgs >= 2)
            controllerIdx = fn.Arg(1).ToUInt32(fn.Env);
    }

    GASKeyCtorFunction* ko = (GASKeyCtorFunction*)fn.ThisPtr;
    GASSERT(ko);

    fn.Result->SetBool(ko->IsKeyDown(code, controllerIdx));
}

// Given the keycode of NUM_LOCK or CAPSLOCK, returns true if
// the associated state is on.
void    GASKeyCtorFunction::KeyIsToggled(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
    {
        fn.Env->LogScriptError("Error: KeyIsToggled needs one Argument (the key code)\n");
        return;
    }

    int code = fn.Arg(0).ToInt32(fn.Env);

    UInt controllerIdx = 0;
    if (fn.Env->CheckExtensions())
    {
        if (fn.NArgs >= 2)
            controllerIdx = fn.Arg(1).ToUInt32(fn.Env);
    }

    GASKeyCtorFunction* ko = (GASKeyCtorFunction*)fn.ThisPtr;
    GASSERT(ko);

    fn.Result->SetBool(ko->IsKeyToggled(code, controllerIdx));
}

void GASKeyCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    fn.Result->SetNull();
}
#else

void GASKey_DummyFunction() {}   // Exists to quelch compiler warning

#endif // GFC_NO_KEYBOARD_SUPPORT

