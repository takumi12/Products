/**********************************************************************

Filename    :   AS/GASSelection.h
Content     :   Implementation of Selection class
Created     :   February, 2007
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXSELECTION_H
#define INC_GFXSELECTION_H

#include "GConfig.h"
#include "GFxAction.h"
#include "GFxCharacter.h"
#include "GFxPlayerImpl.h"
#include "AS/GASObjectProto.h"


// ***** Declared Classes
class GASAsBroadcaster;
class GASAsBroadcasterProto;
class GASAsBroadcasterCtorFunction;

// ***** External Classes
class GASArrayObject;
class GASEnvironment;

class GASSelection : public GASObject
{
protected:
    GASSelection(GASStringContext *psc, GASObject* pprototype)
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }

    void commonInit (GASEnvironment* penv);

    static void DoTransferFocus(const GASFnCall& fn);
public:
    GASSelection(GASEnvironment* penv);

    static void QueueSetFocus(GASEnvironment* penv, GFxASCharacter* pNewFocus, UInt controllerIdx, GFxFocusMovedType fmt);
    static void BroadcastOnSetFocus(GASEnvironment* penv, GFxASCharacter* pOldFocus, GFxASCharacter* pNewFocus, UInt controllerIdx);
};

class GASSelectionProto : public GASPrototype<GASSelection>
{
public:
    GASSelectionProto (GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor);

};

#ifndef GFC_NO_FXPLAYER_AS_SELECTION

//
// Selection static class
//
// A constructor function object for Object
class GASSelectionCtorFunction : public GASCFunctionObject
{
    static const GASNameFunction StaticFunctionTable[];

    static void GetFocus(const GASFnCall& fn);
    static void SetFocus(const GASFnCall& fn);
    static void SetSelection(const GASFnCall& fn);
    static void GetCaretIndex(const GASFnCall& fn);
    static void GetBeginIndex(const GASFnCall& fn);
    static void GetEndIndex(const GASFnCall& fn);

    // extension methods
    // one parameter, boolean, true - to capture focus, false - to release one.
    // Returns currently focus character.
    static void CaptureFocus(const GASFnCall& fn);

    // 1st parameter - strings "up", "down", "left", "right", "tab".
    // 2nd parameter (optional) - the character to begin search from.
    // 3rd parameter (optional) - bool, true to include only focusEnabled = true chars
    // 4th parameter (optional) - keyboard index
    static void MoveFocus(const GASFnCall& fn);
    
    // 1st parameter - strings "up", "down", "left", "right", "tab".
    // 2nd parameter - a parent container (panel), or null
    // 3rd           - bool, if true then loop
    // 4th parameter (optional) - the character to begin search from.
    // 5th parameter (optional) - bool, true to include only focusEnabled = true chars
    // 6th parameter (optional) - keyboard index
    static void FindFocus(const GASFnCall& fn);

    // 1st param - modal clip
    // 2nd param - controller idx (opt)
    static void SetModalClip(const GASFnCall& fn);

    // 1st param - controller idx (opt)
    static void GetModalClip(const GASFnCall& fn);

    // 1st param - controller idx
    // 2nd param - focus group idx
    // ret - bool, true, if succ
    static void SetControllerFocusGroup(const GASFnCall& fn);

    // 1st param - controller id
    // ret - focus group idx
    static void GetControllerFocusGroup(const GASFnCall& fn);

    // 1st param - a movie clip
    // ret - array of controller indices focused on that movie clip
    static void GetFocusArray(const GASFnCall& fn);

    // 1st param - a movie clip
    // ret - a bitmask indicating controllers currently focused on that movie clip
    static void GetFocusBitmask(const GASFnCall& fn);

    // 1st param - an id of logical controller (focus group index)
    // returns a bit-mask where each bit represents a physical controller, 
    // associated with the specified focus group.
    static void GetControllerMaskByFocusGroup(const GASFnCall& fn);
public:
    GASSelectionCtorFunction (GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);
    virtual GASObject* CreateNewObject(GASEnvironment*) const { return 0; }

    virtual bool GetMember(GASEnvironment* penv, const GASString& name, GASValue* pval);
    virtual bool SetMember(GASEnvironment *penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags());

    static GASFunctionRef Register(GASGlobalContext* pgc);
};

#endif //#ifndef GFC_NO_FXPLAYER_AS_SELECTION

#endif // INC_GFXSELECTION_H
