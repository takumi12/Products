/**********************************************************************

Filename    :   AS/GASKeyObject.h
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

#ifndef INC_GFXKEYOBJECT_H
#define INC_GFXKEYOBJECT_H

#include "GFxAction.h"
#include "GFxCharacter.h"
#include "GFxPlayerImpl.h"
#include "AS/GASObjectProto.h"

#ifndef GFC_NO_KEYBOARD_SUPPORT
//
// Key AS class implementation
//
class GASKeyObject : public GASObject
{
protected:

    void commonInit (GASEnvironment* penv);

    GASKeyObject(GASStringContext *psc, GASObject* pprototype)
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }
    ObjectType          GetObjectType() const   { return Object_Key; }
public:
};

class GASKeyProto : public GASPrototype<GASKeyObject>
{
public:
    GASKeyProto (GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor);
};

class GASKeyCtorFunction : public GASCFunctionObject, public GFxKeyboardState::IListener
{
protected:
#ifdef GFC_NO_GC
    // Hack to store both WeakPtrs and correct interfaces.
    // If weak ptr is null, the interface pointer is unusable.
    // TBD: This should be an array of GASValues, or at least a union of { GASObject, GFxCharacterHandle }.
    // This is not necessary if cycle collector is used. In this case AsBroadcaster mechanism 
    // is used.
    GArrayLH<GWeakPtr<GRefCountWeakSupportImpl> >   ListenerWeakRefs;
    GArrayLH<GASObjectInterface*>                       Listeners;
#endif //GFC_NO_GC
    struct State
    {
        int                         LastKeyCode;
        UByte                       LastAsciiCode;
        UInt32                      LastWcharCode;

        State():LastKeyCode(0), LastAsciiCode(0), LastWcharCode(0) {}
    } States[GFX_MAX_CONTROLLERS_SUPPORTED];
    GFxMovieRoot*               pMovieRoot;
    //UInt8                       LastKeyboardIndex;


    // GFxKeyboardState::IListener methods
    virtual void OnKeyDown(GASStringContext *psc, int code, UByte ascii, 
        UInt32 wcharCode, UInt8 keyboardIndex) 
    {
        NotifyListeners(psc, code, ascii, wcharCode, GFxEventId::Event_KeyDown, keyboardIndex);
    }       
    virtual void OnKeyUp(GASStringContext *psc, int code, UByte ascii, 
        UInt32 wcharCode, UInt8 keyboardIndex) 
    {
        NotifyListeners(psc, code, ascii, wcharCode, GFxEventId::Event_KeyUp, keyboardIndex);
    }
    virtual void Update(int code, UByte ascii, UInt32 wcharCode, 
        UInt8 keyboardIndex);

#ifndef GFC_NO_GC
    void Finalize_GC()
    {    
        pMovieRoot = 0;
        GASObject::Finalize_GC();
    }
#endif // GFC_NO_GC

    static const GASNameFunction StaticFunctionTable[];

    virtual GASObject* CreateNewObject(GASEnvironment*) const { return 0; }
    static void        GlobalCtor(const GASFnCall& fn);
public:

    GASKeyCtorFunction(GASStringContext *psc, GFxMovieRoot* proot);

    void NotifyListeners(GASStringContext *psc, int code, UByte ascii, UInt32 wcharCode, 
        int eventId, UInt8 keyboardIndex);

    bool IsKeyDown(int code,    UInt keyboardIndex);
    bool IsKeyToggled(int code, UInt keyboardIndex);

    // If there is no cycle collector (CC) we need to implement custom listeners logic
    // to avoid circular references. If CC is used then we use just regular AsBroadcaster
    // mechanism.
#ifdef GFC_NO_GC
    // Remove dead entries in the listeners list.  (Since
    // we use WeakPtr's, listeners can disappear without
    // notice.)
    void CleanupListeners();
    void AddListener(GRefCountWeakSupportImpl *pref, GASObjectInterface* listener);
    void RemoveListener(GRefCountWeakSupportImpl *pref, GASObjectInterface* listener);

    // support for _listeners
    bool GetMember(GASEnvironment* penv, const GASString& name, GASValue* val);

    static void KeyAddListener(const GASFnCall& fn);
    static void KeyRemoveListener(const GASFnCall& fn);
#endif // GFC_NO_GC

    static void KeyGetAscii(const GASFnCall& fn);
    static void KeyGetCode(const GASFnCall& fn);
    static void KeyIsDown(const GASFnCall& fn);
    static void KeyIsToggled(const GASFnCall& fn);

    inline int GetLastKeyCode(UInt controllerIdx) const 
    { 
        GASSERT(controllerIdx < GFX_MAX_CONTROLLERS_SUPPORTED);
        return States[controllerIdx].LastKeyCode; 
    }
    inline int GetLastAsciiCode(UInt controllerIdx) const 
    { 
        GASSERT(controllerIdx < GFX_MAX_CONTROLLERS_SUPPORTED);
        return States[controllerIdx].LastAsciiCode; 
    }
};
#endif // GFC_NO_KEYBOARD_SUPPORT

#endif // INC_GFXKEYOBJECT_H
