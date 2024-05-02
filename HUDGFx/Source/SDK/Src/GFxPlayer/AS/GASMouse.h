/**********************************************************************

Filename    :   AS/GASMouse.h
Content     :   Implementation of Mouse class
Created     :   November, 2006
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXMOUSE_H
#define INC_GFXMOUSE_H

#include "GFxAction.h"
#include "GFxCharacter.h"
#include "GFxPlayerImpl.h"
#include "AS/GASObjectProto.h"
#include "AS/GASAsBroadcaster.h"

#ifndef GFC_NO_FXPLAYER_AS_MOUSE
// ***** Declared Classes
class GASAsBroadcaster;
class GASAsBroadcasterProto;
class GASAsBroadcasterCtorFunction;

// ***** External Classes
class GASArrayObject;
class GASEnvironment;



class GASMouse : public GASObject
{
protected:

    void commonInit (GASEnvironment* penv);
    
    GASMouse(GASStringContext *psc, GASObject* pprototype)
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }
public:
    GASMouse (GASEnvironment* penv);
};

class GASMouseProto : public GASPrototype<GASMouse>
{
public:
    GASMouseProto (GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor);
};

class GASMouseCtorFunction : public GASCFunctionObject, public GFxASMouseListener
{
    static int   PushListenersParams(GASEnvironment* penv, 
                                     UInt mouseIndex, 
                                     GASBuiltinType eventName, 
                                     const GASValue& eventMethod, 
                                     const GASString* ptargetName, 
                                     UInt button, 
                                     int delta,
                                     bool dblClick);

    void NotifyListeners(GASEnvironment *penv, UInt mouseIndex, 
                         GASBuiltinType eventName, const GASString* ptargetName = 0, 
                         UInt button = 0, int delta = 0, bool dblClick = false) const;
#ifndef GFC_NO_GC
protected:
    friend class GRefCountBaseGC<GFxStatMV_ActionScript_Mem>;
    template <class Functor> void ForEachChild_GC(Collector* prcc) const;
    virtual                  void ExecuteForEachChild_GC(Collector* prcc, OperationGC operation) const;

    virtual void Finalize_GC();

    void UpdateListenersArray(GASStringContext *psc, GASEnvironment* penv = NULL);
#endif //GFC_NO_GC
protected:
    enum 
    {
        Mouse_LEFT = 1,
        Mouse_RIGHT = 2,
        Mouse_MIDDLE = 3
    };
#ifdef GFC_NO_GC
    // Hack to store both WeakPtrs and correct interfaces.
    // If weak ptr is null, the interface pointer is unusable.
    // This is not necessary if cycle collector is used. In this case AsBroadcaster mechanism 
    // is used.
    GArrayLH<GWeakPtr<GRefCountWeakSupportImpl> >  ListenerWeakRefs;
    GArrayLH<GASObjectInterface*>                      Listeners;
#else
    // array of listeners; it is actually stored as a member of the Mouse object
    // (populated by AsBroadcaster). This member is necessary only to implement 
    // method "IsEmpty" to avoid unnecessary processing if there are no
    // listeners. It is updated by UpdateListenersArray method.
    GPtr<GASArrayObject>                    pListenersArray;

    class LocalInvokeCallback : public GASAsBroadcaster::InvokeCallback
    {
    public:
        int             MouseIndex;
        GASBuiltinType  EventName;
        const GASString* pTargetName; 
        UInt            Button; 
        int             Delta;
        bool            DoubleClick;

        virtual void Invoke(GASEnvironment* penv, GASObjectInterface* pthis, const GASFunctionRef& method)
        {
            int nArgs = PushListenersParams(penv, MouseIndex, EventName, method, pTargetName, Button, Delta, DoubleClick);
            if (nArgs >= 0)
            {
                GASValue result;
                method.Invoke(GASFnCall(&result, pthis, penv, nArgs, penv->GetTopIndex()));
                penv->Drop(nArgs);
            }
        }
    };
#endif // GFC_NO_GC
    mutable GPoint<int>                     LastMousePos;
    mutable UInt32                          LastClickTime;
    GASFunctionRef                          SetCursorTypeFunc;
    bool                                    MouseExtensions;

    virtual void OnMouseMove(GASEnvironment *penv, UInt mouseIndex) const;
    virtual void OnMouseDown(GASEnvironment *penv, UInt mouseIndex, UInt button, GFxASCharacter* ptarget) const;
    virtual void OnMouseUp(GASEnvironment *penv, UInt mouseIndex, UInt button, GFxASCharacter* ptarget) const;
    virtual void OnMouseWheel(GASEnvironment *penv, UInt mouseIndex, int sdelta, GFxASCharacter* ptarget) const;
    virtual bool IsEmpty() const;

    static const GASNameFunction StaticFunctionTable[];

#ifdef GFC_NO_GC
    void CleanupListeners();
    static void AddListener(const GASFnCall& fn);
    static void RemoveListener(const GASFnCall& fn);
#endif // GFC_NO_GC
    static void Hide(const GASFnCall& fn);
    static void Show(const GASFnCall& fn);

    // extension methods
    static void SetCursorType(const GASFnCall& fn);
    // Returns a character at the specified location. Four versions of this method: no params, 
    // 1 param, 2 params and 3 params. Method with no params just look for ANY character under 
    // the mouse pointer (with or without button handlers). Method with 1 parameter takes a boolean 
    // value as the parameter. It also look for character under the mouse pointer, though the 
    // parameter indicates type of characters: true - ANY character, false - only with button 
    // handlers. Version with two parameters takes custom X and Y coordinates in _root coordinate 
    // space and look for ANY character at these coordinates. Version with three params is same 
    // as version with two, the third parameter (boolean) indicates the type of character
    // (any/with button handler).
    static void GetTopMostEntity(const GASFnCall& fn);

    // returns position of mouse cursor, index of mouse is an optional parameter.
    // Returning value is Point object (or Object, if Point class is not included).
    // Undefined, if error occurred.
    static void GetPosition(const GASFnCall& fn);

    // returns status of mouse buttons, index of mouse is an optional parameter.
    // Returning value is a number, bit 0 indicates state of left button, 
    // bit 1 - right one, bit 2 - middle.
    // Undefined, if error occurred.
    static void GetButtonsState(const GASFnCall& fn);

    static void         GlobalCtor(const GASFnCall& fn);
    virtual GASObject*  CreateNewObject(GASEnvironment*) const { return 0; }
public:
    GASMouseCtorFunction (GASStringContext *psc, GFxMovieRoot* proot);

    virtual bool    GetMember(GASEnvironment *penv, const GASString& name, GASValue* val);
    virtual bool    SetMember(GASEnvironment *penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags());

    static bool     SetCursorType(GFxMovieRoot* proot, UInt mouseIndex, UInt cursorType);
    bool            HasOverloadedCursorTypeFunction(GASStringContext* psc) const;

    static GASFunctionRef Register(GASGlobalContext* pgc);
};

#endif //GFC_NO_FXPLAYER_AS_MOUSE

#endif // INC_GFXMOUSE_H
