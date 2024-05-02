/**********************************************************************

Filename    :   AS/GASSelection.h
Content     :   Implementation of IME class
Created     :   Mar 2008
Authors     :   Ankur

Notes       :   
History     :   

Copyright   :   (c) 1998-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXIME_H
#define INC_GFXIME_H

#ifndef GFC_NO_IME_SUPPORT

#include "GFxAction.h"
#include "GFxCharacter.h"
#include "GFxPlayerImpl.h"

class GASIme : public GASObject
{
protected:
    void commonInit (GASEnvironment* penv);

    GASIme(GASStringContext *psc, GASObject* pprototype) 
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }
public:
    GASIme(GASEnvironment* penv);
    static void BroadcastOnIMEComposition(GASEnvironment* penv, const GASString& pString);
    static void BroadcastOnSwitchLanguage(GASEnvironment* penv, const GASString& compString);
    static void BroadcastOnSetSupportedLanguages(GASEnvironment* penv, const GASString& supportedLangs);
    static void BroadcastOnSetSupportedIMEs(GASEnvironment* penv, const GASString& supportedIMEs);
    static void BroadcastOnSetCurrentInputLang(GASEnvironment* penv, const GASString& currentInputLang);
    static void BroadcastOnSetIMEName(GASEnvironment* penv, const GASString& imeName);
    static void BroadcastOnSetConversionMode(GASEnvironment* penv, const GASString& imeName);
    static void BroadcastOnRemoveStatusWindow(GASEnvironment* penv);
    static void BroadcastOnDisplayStatusWindow(GASEnvironment* penv);
};

class GASImeProto : public GASPrototype<GASIme>
{
public:
    GASImeProto (GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);
};

//
// Selection static class
//
// A constructor function object for Object
class GASImeCtorFunction : public GASCFunctionObject
{
    static const GASNameFunction StaticFunctionTable[];
    static const GASNameNumber GASNumberConstTable[];
    static void DoConversion(const GASFnCall& fn);
    static void GetConversionMode(const GASFnCall& fn);
    static void GetEnabled(const GASFnCall& fn);
    static void SetCompositionString(const GASFnCall& fn);
    static void SetConversionMode(const GASFnCall& fn);
    static void SetEnabled(const GASFnCall& fn);

public:
    GASImeCtorFunction (GASStringContext *psc);

    virtual GASObject* CreateNewObject(GASStringContext*) const { return 0; }

    static GASFunctionRef Register(GASGlobalContext* pgc);
};

#endif // GFC_NO_IME_SUPPORT
#endif 
