/**********************************************************************

Filename    :   GFxSelection.cpp
Content     :   Implementation of IME class
Created     :   February, 2007
Authors     :   Ankur Mohan

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

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
#include "IME/GASIme.h"
#include "GFxText.h"
#include "GUTF8Util.h"
#include "AS/GASAsBroadcaster.h"
#include <GFxIMEManager.h>

#ifndef GFC_NO_IME_SUPPORT

GASIme::GASIme(GASEnvironment* penv) : GASObject(penv)
{
    commonInit(penv);
}

void GASIme::commonInit (GASEnvironment* penv)
{
    Set__proto__ (penv->GetSC(), penv->GetPrototype(GASBuiltin_Ime));
}

//////////////////////////////////////////
//
static void GFx_FuncStub(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
}

static const GASNameFunction GAS_ImeFunctionTable[] = 
{
    { "addListener",            &GFx_FuncStub },
    { "doConversion",           &GFx_FuncStub },
    { "getConversionMode",      &GFx_FuncStub },
    { "getEnabled",             &GFx_FuncStub },
    { "setCompositionString",   &GFx_FuncStub },
    { "setConversionMode",      &GFx_FuncStub },
    { "setEnabled",             &GFx_FuncStub },
    { "removeListener",         &GFx_FuncStub },
    { 0, 0 }
};

GASImeProto::GASImeProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
GASPrototype<GASIme>(psc, pprototype, constructor)
{
    InitFunctionMembers (psc, GAS_ImeFunctionTable);
}

void GASImeProto::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASIme> ab = *new GASIme(fn.Env);
    fn.Result->SetAsObject(ab.GetPtr());
}

const GASNameNumber GASImeCtorFunction::GASNumberConstTable[] = 
{
    { "ALPHANUMERIC_FULL",          0x00000000},
    { "ALPHANUMERIC_HALF",          0x00000001},
    { "CHINESE",                    0x00000002},
    { "JAPANESE_HIRAGANA",          0x00000004},
    { "JAPANESE_KATAKANA_FULL",     0x00000008},
    { "JAPANESE_KATAKANA_HALF",     0x00000016},
    { "KOREAN",                     0x00000032},
    { "UNKNOWN",                    0x00000064},
    { 0, 0 }
};

//////////////////
const GASNameFunction GASImeCtorFunction::StaticFunctionTable[] = 
{
    { "doConversion",           &GASImeCtorFunction::DoConversion },
    { "getConversionMode",      &GASImeCtorFunction::GetConversionMode },
    { "getEnabled",             &GASImeCtorFunction::GetEnabled },
    { "setCompositionString",   &GASImeCtorFunction::SetCompositionString },
    { "setConversionMode",      &GASImeCtorFunction::SetConversionMode },
    { "setEnabled",             &GASImeCtorFunction::SetEnabled },
    { 0, 0 }
};

GASImeCtorFunction::GASImeCtorFunction(GASStringContext *psc) :
    GASCFunctionObject(psc, GASObjectProto::GlobalCtor)
{
    GASAsBroadcaster::Initialize(psc, this);

    GASNameFunction::AddConstMembers(
        this, psc, StaticFunctionTable, 
        GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);

    for(int i = 0; GASNumberConstTable[i].Name; i++)
    {
        SetConstMemberRaw(psc, GASNumberConstTable[i].Name, GASValue(GASNumberConstTable[i].Number), 
            GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    }
}

void GASImeCtorFunction::DoConversion(const GASFnCall& fn)
{
    GUNUSED(fn);
}

void GASImeCtorFunction::GetConversionMode(const GASFnCall& fn)
{
    if (!fn.Env)
        return;

    // Try to get IME Manager.
    GPtr<GFxIMEManager> pimeManager = fn.Env->GetMovieRoot()->GetIMEManager();
    GASString resultStr = fn.Env->GetGC()->CreateConstString("UNKNOWN");

    if(pimeManager)
    {
        resultStr = pimeManager->GetConversionMode();
    }

    fn.Result->SetString(resultStr);

}

void GASImeCtorFunction::SetCompositionString(const GASFnCall& fn)
{
    if (!fn.Env)
        return;

    // Try to get IME Manager.
    GPtr<GFxIMEManager> pimeManager = fn.Env->GetMovieRoot()->GetIMEManager();
    bool result = false; 
    if(pimeManager)
        result = pimeManager->SetCompositionString(fn.Arg(0).ToString(fn.Env).ToCStr());

    fn.Result->SetBool(result);
}

void GASImeCtorFunction::SetConversionMode(const GASFnCall& fn)
{
    if (!fn.Env)
        return;

    // Try to get IME Manager.
    GPtr<GFxIMEManager> pimeManager = fn.Env->GetMovieRoot()->GetIMEManager();
    bool result = false;
    if(pimeManager)
        result = pimeManager->SetConversionMode((UInt)(fn.Arg(0).ToNumber(fn.Env)));

    fn.Result->SetBool(result);
}

void GASImeCtorFunction::SetEnabled(const GASFnCall& fn)
{

    if (!fn.Env)
        return;

    // Try to get IME Manager.
    GPtr<GFxIMEManager> pimeManager = fn.Env->GetMovieRoot()->GetIMEManager();
    bool result = false;
    if(pimeManager)
        result = pimeManager->SetEnabled(fn.Arg(0).ToBool(fn.Env));

    fn.Result->SetBool(result);
}

void GASImeCtorFunction::GetEnabled(const GASFnCall& fn)
{

    if (!fn.Env)
        return;

    // Try to get IME Manager.
    GPtr<GFxIMEManager> pimeManager = fn.Env->GetMovieRoot()->GetIMEManager();
    if(pimeManager)
        fn.Result->SetBool(pimeManager->GetEnabled());
}

GASFunctionRef GASImeCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef  ctor(*GHEAP_NEW(pgc->GetHeap()) GASImeCtorFunction(&sc));
    GPtr<GASObject> proto = *GHEAP_NEW(pgc->GetHeap()) GASImeProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_Ime, proto);
    pgc->SystemPackage->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_Ime), GASValue(ctor));
    return ctor;
}

void GASIme::BroadcastOnIMEComposition(GASEnvironment* penv, const GASString& compString)
{
    GASValue imeCtorVal;
    GASValue systemPackage;
    GASString systemPackageStr = penv->GetGC()->CreateConstString("System");

    if(penv->GetGC()->pGlobal->GetMemberRaw(penv->GetSC(), systemPackageStr, &systemPackage))
    {
        GASObject* psystemPackage = systemPackage.ToObject(penv);

        if (psystemPackage->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_Ime), &imeCtorVal))
        {
            GASObjectInterface* pimeObj = imeCtorVal.ToObject(penv);
            if (pimeObj)
            {
                if (!compString.IsEmpty())
                    penv->Push(GASValue(compString));
                else
                    penv->Push(GASValue::NULLTYPE);

                GASAsBroadcaster::BroadcastMessage(penv, pimeObj, penv->CreateConstString("onIMEComposition"), 1, penv->GetTopIndex());
                penv->Drop(1);
            }
        }
    }  
}

void GASIme::BroadcastOnSwitchLanguage(GASEnvironment* penv, const GASString& compString)
{
    GASValue imeCtorVal;
    GASValue systemPackage;
    GASString systemPackageStr = penv->GetGC()->CreateConstString("System");

    if(penv->GetGC()->pGlobal->GetMemberRaw(penv->GetSC(), systemPackageStr, &systemPackage))
    {
        GASObject* psystemPackage = systemPackage.ToObject(penv);

        if (psystemPackage->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_Ime), &imeCtorVal))
        {
            GASObjectInterface* pimeObj = imeCtorVal.ToObject(penv);
            if (pimeObj)
            {
                if (!compString.IsEmpty())
                    penv->Push(GASValue(compString));
                else
                    penv->Push(GASValue::NULLTYPE);

                GASAsBroadcaster::BroadcastMessage(penv, pimeObj, penv->CreateConstString("onSwitchLanguage"), 1, penv->GetTopIndex());
                penv->Drop(1);
            }
        }
    }  
}

void GASIme::BroadcastOnSetSupportedLanguages(GASEnvironment* penv, const GASString& supportedLangs)
{
    GASValue imeCtorVal;
    GASValue systemPackage;
    GASString systemPackageStr = penv->GetGC()->CreateConstString("System");

    if(penv->GetGC()->pGlobal->GetMemberRaw(penv->GetSC(), systemPackageStr, &systemPackage))
    {
        GASObject* psystemPackage = systemPackage.ToObject(penv);

        if (psystemPackage->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_Ime), &imeCtorVal))
        {
            GASObjectInterface* pimeObj = imeCtorVal.ToObject(penv);
            if (pimeObj)
            {
                if (!supportedLangs.IsEmpty())
                    penv->Push(GASValue(supportedLangs));
                else
                    penv->Push(GASValue::NULLTYPE);

                GASAsBroadcaster::BroadcastMessage(penv, pimeObj, penv->CreateConstString("onSetSupportedLanguages"), 1, penv->GetTopIndex());
                penv->Drop(1);
            }
        }
    }  
}

void GASIme::BroadcastOnSetSupportedIMEs(GASEnvironment* penv, const GASString& supportedImes)
{
    GASValue imeCtorVal;
    GASValue systemPackage;
    GASString systemPackageStr = penv->GetGC()->CreateConstString("System");

    if(penv->GetGC()->pGlobal->GetMemberRaw(penv->GetSC(), systemPackageStr, &systemPackage))
    {
        GASObject* psystemPackage = systemPackage.ToObject(penv);

        if (psystemPackage->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_Ime), &imeCtorVal))
        {
            GASObjectInterface* pimeObj = imeCtorVal.ToObject(penv);
            if (pimeObj)
            {
                if (!supportedImes.IsEmpty())
                    penv->Push(GASValue(supportedImes));
                else
                    penv->Push(GASValue::NULLTYPE);

                GASAsBroadcaster::BroadcastMessage(penv, pimeObj, penv->CreateConstString("onSetSupportedIMEs"), 1, penv->GetTopIndex());
                penv->Drop(1);
            }
        }
    }  
}

void GASIme::BroadcastOnSetCurrentInputLang(GASEnvironment* penv, const GASString& currentInputLang)
{
    GASValue imeCtorVal;
    GASValue systemPackage;
    GASString systemPackageStr = penv->GetGC()->CreateConstString("System");

    if(penv->GetGC()->pGlobal->GetMemberRaw(penv->GetSC(), systemPackageStr, &systemPackage))
    {
        GASObject* psystemPackage = systemPackage.ToObject(penv);

        if (psystemPackage->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_Ime), &imeCtorVal))
        {
            GASObjectInterface* pimeObj = imeCtorVal.ToObject(penv);
            if (pimeObj)
            {
                if (!currentInputLang.IsEmpty())
                    penv->Push(GASValue(currentInputLang));
                else
                    penv->Push(GASValue::NULLTYPE);

                GASAsBroadcaster::BroadcastMessage(penv, pimeObj, penv->CreateConstString("onSetCurrentInputLanguage"), 1, penv->GetTopIndex());
                penv->Drop(1);
            }
        }
    }  
}

void GASIme::BroadcastOnSetIMEName(GASEnvironment* penv, const GASString& imeName)
{
    GASValue imeCtorVal;
    GASValue systemPackage;
    GASString systemPackageStr = penv->GetGC()->CreateConstString("System");

    if(penv->GetGC()->pGlobal->GetMemberRaw(penv->GetSC(), systemPackageStr, &systemPackage))
    {
        GASObject* psystemPackage = systemPackage.ToObject(penv);

        if (psystemPackage->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_Ime), &imeCtorVal))
        {
            GASObjectInterface* pimeObj = imeCtorVal.ToObject(penv);
            if (pimeObj)
            {
                if (!imeName.IsEmpty())
                    penv->Push(GASValue(imeName));
                else
                    penv->Push(GASValue::NULLTYPE);

                GASAsBroadcaster::BroadcastMessage(penv, pimeObj, penv->CreateConstString("onSetIMEName"), 1, penv->GetTopIndex());
                penv->Drop(1);
            }
        }
    }  
}

void GASIme::BroadcastOnSetConversionMode(GASEnvironment* penv, const GASString& convStatus)
{
    GASValue imeCtorVal;
    GASValue systemPackage;
    GASString systemPackageStr = penv->GetGC()->CreateConstString("System");

    if(penv->GetGC()->pGlobal->GetMemberRaw(penv->GetSC(), systemPackageStr, &systemPackage))
    {
        GASObject* psystemPackage = systemPackage.ToObject(penv);

        if (psystemPackage->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_Ime), &imeCtorVal))
        {
            GASObjectInterface* pimeObj = imeCtorVal.ToObject(penv);
            if (pimeObj)
            {
                if (!convStatus.IsEmpty())
                    penv->Push(GASValue(convStatus));
                else
                    penv->Push(GASValue::NULLTYPE);

                GASAsBroadcaster::BroadcastMessage(penv, pimeObj, penv->CreateConstString("onSetConversionStatus"), 1, penv->GetTopIndex());
                penv->Drop(1);
            }
        }
    }  
}

void GASIme::BroadcastOnRemoveStatusWindow(GASEnvironment* penv)
{
    GASValue imeCtorVal;
    GASValue systemPackage;
    GASString systemPackageStr = penv->GetGC()->CreateConstString("System");

    if(penv->GetGC()->pGlobal->GetMemberRaw(penv->GetSC(), systemPackageStr, &systemPackage))
    {
        GASObject* psystemPackage = systemPackage.ToObject(penv);

        if (psystemPackage->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_Ime), &imeCtorVal))
        {
            GASObjectInterface* pimeObj = imeCtorVal.ToObject(penv);
            if (pimeObj)
            {
                GASAsBroadcaster::BroadcastMessage(penv, pimeObj, penv->CreateConstString("onRemoveStatusWindow"), 0, penv->GetTopIndex());
            }
        }
    }  
}

void GASIme::BroadcastOnDisplayStatusWindow(GASEnvironment* penv)
{
    GASValue imeCtorVal;
    GASValue systemPackage;
    GASString systemPackageStr = penv->GetGC()->CreateConstString("System");

    if(penv->GetGC()->pGlobal->GetMemberRaw(penv->GetSC(), systemPackageStr, &systemPackage))
    {
        GASObject* psystemPackage = systemPackage.ToObject(penv);

        if (psystemPackage->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_Ime), &imeCtorVal))
        {
            GASObjectInterface* pimeObj = imeCtorVal.ToObject(penv);
            if (pimeObj)
            {
                GASAsBroadcaster::BroadcastMessage(penv, pimeObj, penv->CreateConstString("onDisplayStatusWindow"), 0, penv->GetTopIndex());
            }
        }
    }  
}

#endif // GFC_NO_IME_SUPPORT
