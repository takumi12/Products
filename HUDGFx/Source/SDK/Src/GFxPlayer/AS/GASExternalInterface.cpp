/**********************************************************************

Filename    :   GFxExternalInterface.cpp
Content     :   ExternalInterface AS class implementation
Created     :   
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "AS/GASExternalInterface.h"
#include "GASFunctionRef.h"
#include "GASValue.h"
#include "AMP/GFxAmpViewStats.h"

#include <stdio.h>
#include <stdlib.h>

#define GFX_MAX_NUM_OF_VALUES_ON_STACK 10

GASExternalInterface::GASExternalInterface(GASEnvironment* penv)
: GASObject(penv)
{    
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_ExternalInterface));
}


static const GASNameFunction GFx_EIFunctionTable[] = 
{
    { 0, 0 }
};

GASExternalInterfaceProto::GASExternalInterfaceProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASExternalInterface>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GFx_EIFunctionTable);
}

const GASNameFunction GASExternalInterfaceCtorFunction::StaticFunctionTable[] = 
{
    { "addCallback", &GASExternalInterfaceCtorFunction::AddCallback },
    { "call",        &GASExternalInterfaceCtorFunction::Call   },
    { 0, 0 }
};

GASExternalInterfaceCtorFunction::GASExternalInterfaceCtorFunction(GASStringContext *psc) : 
    GASCFunctionObject(psc, GlobalCtor)
{ 
    SetConstMemberRaw(psc, "available", GASValue(GASValue::UNSET));

    GASNameFunction::AddConstMembers(
        this, psc, StaticFunctionTable, 
        GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
}

bool    GASExternalInterfaceCtorFunction::GetMember(GASEnvironment *penv, const GASString& name, GASValue* val)
{
    if (penv->GetSC()->CompareConstString_CaseCheck(name, "available"))
    {
        // "available" returns "true" if external interface handler is set.
        GFxMovieRoot* proot = penv->GetMovieRoot();
        GASSERT(proot);

        val->SetBool((proot->pExtIntfHandler)?true:false);
        return true;
    }
    return GASFunctionObject::GetMember(penv, name, val);
}

void    GASExternalInterfaceCtorFunction::AddCallback(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    if (fn.NArgs < 3)
        return;

    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    GASSERT(proot);

    GASString methodName       = fn.Arg(0).ToString(fn.Env);
    GASFunctionRef  function   = fn.Arg(2).ToFunction(fn.Env);
    GPtr<GFxCharacterHandle>    thisChar;
    GPtr<GASObject>             thisObject;

    if (fn.Arg(1).IsCharacter())
    {
        GPtr<GFxASCharacter> pchar = fn.Arg(1).ToASCharacter(fn.Env);
        if (pchar)
            thisChar = pchar->GetCharacterHandle();
    }
    else
        thisObject = fn.Arg(1).ToObject(fn.Env);

    proot->AddInvokeAlias(methodName, thisChar, thisObject, function);

    fn.Result->SetBool(true);
}

void    GASExternalInterfaceCtorFunction::Call(const GASFnCall& fn)
{
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    GASSERT(proot);

    if (proot->pExtIntfHandler)
    {
        ScopeFunctionTimer extIntTimer(proot->AdvanceStats, NativeCodeSwdHandle, Func_GFxExternalInterface_Callback, Amp_Profile_Level_Medium);

        UInt nArgs = 0;
        GASString methodName(fn.Env->GetBuiltin(GASBuiltin_empty_));
        if (fn.NArgs >= 1)
        {
            methodName = fn.Arg(0).ToString(fn.Env);
            nArgs = fn.NArgs - 1;
        }

        // convert arguments into GFxValue, starting from index 1
        UByte argArrayOnStack[sizeof(GFxValue) * GFX_MAX_NUM_OF_VALUES_ON_STACK];
        GFxValue* pargArray;
        if (nArgs > GFX_MAX_NUM_OF_VALUES_ON_STACK)
            pargArray = (GFxValue*)
                GHEAP_ALLOC(fn.Env->GetHeap(), sizeof(GFxValue) * nArgs, GFxStatMV_ActionScript_Mem);
        else
            pargArray = (GFxValue*)argArrayOnStack;

        // convert params to GFxValue array
        for (UInt i = 0, n = nArgs; i < n; ++i)
        {
            const GASValue& val = fn.Arg(i + 1);
            GFxValue* pdestVal = G_Construct<GFxValue>(&pargArray[i]);
            proot->ASValue2GFxValue(fn.Env, val, pdestVal);
        }
        
        proot->ExternalIntfRetVal.SetUndefined();

        proot->pExtIntfHandler->Callback
            (proot, (methodName.IsEmpty()) ? NULL : methodName.ToCStr(), pargArray, nArgs);

        *fn.Result = proot->ExternalIntfRetVal;

        // destruct elements in GFxValue array
        for (UInt i = 0, n = nArgs; i < n; ++i)
        {
            pargArray[i].~GFxValue();
        }
        if (nArgs > GFX_MAX_NUM_OF_VALUES_ON_STACK)
        {
            GHEAP_FREE(fn.Env->GetHeap(), pargArray);
        }
    }
    else
    {
        fn.LogScriptWarning("Warning: ExternalInterface.call - handler is not installed.\n");
        fn.Result->SetUndefined();
    }
}

void GASExternalInterfaceCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
}

GASObject* GASExternalInterfaceCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{
    return GHEAP_NEW(penv->GetHeap()) GASExternalInterface(penv);
}


GASFunctionRef GASExternalInterfaceCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef  ctor(*GHEAP_NEW(pgc->GetHeap()) GASExternalInterfaceCtorFunction(&sc));
    GPtr<GASObject> proto = *GHEAP_NEW(pgc->GetHeap()) GASExternalInterfaceProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_ExternalInterface, proto);
    pgc->FlashExternalPackage->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_ExternalInterface), GASValue(ctor));
    return ctor;
}
