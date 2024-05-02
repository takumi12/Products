/**********************************************************************

Filename    :   GFxBoolean.cpp
Content     :   Boolean object functinality
Created     :   April 10, 2006
Authors     :   
Notes       :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "AS/GASBooleanObject.h"

#include <stdio.h>
#include <stdlib.h>


GASBooleanObject::GASBooleanObject(GASEnvironment* penv) 
: GASObject(penv), Value (0)
{
    CommonInit(penv);
}

GASBooleanObject::GASBooleanObject(GASEnvironment* penv, bool v) 
: GASObject(penv), Value (v)
{
    CommonInit(penv);
}

void GASBooleanObject::CommonInit(GASEnvironment* penv)
{ 
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Boolean));
}

const char* GASBooleanObject::GetTextValue(GASEnvironment*) const
{
    return (Value) ? "true" : "false";
}

bool GASBooleanObject::InvokePrimitiveMethod(const GASFnCall& fnCall, const GASString& methodName)
{
    GASBooleanObject* pthis = (GASBooleanObject*) fnCall.ThisPtr;
    GASSERT(pthis);

    // if (methodName == "toString" || methodName == "valueOf")
    if (fnCall.Env->GetBuiltin(GASBuiltin_toString).CompareBuiltIn_CaseCheck(methodName, fnCall.Env->IsCaseSensitive()) ||
        fnCall.Env->GetBuiltin(GASBuiltin_valueOf).CompareBuiltIn_CaseCheck(methodName, fnCall.Env->IsCaseSensitive()) )
    {
        GASValue method;
        if (pthis->GetMemberRaw(fnCall.Env->GetSC(), methodName, &method)) 
        {
            GASFunctionRef  func = method.ToFunction(fnCall.Env);
            if (!func.IsNull())
            {
                // It's a C function.  Call it.
                func.Invoke(fnCall);
                return true;
            }
        }
    }
    fnCall.Result->SetUndefined();
    return false;
}

void        GASBooleanObject::SetValue(GASEnvironment* penv, const GASValue& v)
{
    Value = v.ToBool(penv);
}

GASValue    GASBooleanObject::GetValue() const
{
    return GASValue(Value);
}

////////////////////////
static void GAS_BooleanToString(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Boolean);
    GASBooleanObject* pthis = (GASBooleanObject*) fn.ThisPtr;
    GASSERT(pthis);

    fn.Result->SetString(pthis->GetValue().ToString(fn.Env));
}

static void GAS_BooleanValueOf(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Boolean);
    GASBooleanObject* pthis = (GASBooleanObject*) fn.ThisPtr;
    GASSERT(pthis);

    fn.Result->SetBool(pthis->GetValue().ToBool(fn.Env));
}


static const GASNameFunction GAS_BooleanFunctionTable[] = 
{
    { "toString",       &GAS_BooleanToString  },
    { "valueOf",        &GAS_BooleanValueOf  },
    { 0, 0 }
};

GASBooleanProto::GASBooleanProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASBooleanObject>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GAS_BooleanFunctionTable);
}

GASBooleanCtorFunction::GASBooleanCtorFunction(GASStringContext *psc) :
    GASCFunctionObject(psc, GlobalCtor)
{
    GUNUSED(psc);
}

GASObject* GASBooleanCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{
    return GHEAP_NEW(penv->GetHeap()) GASBooleanObject(penv);
}

void GASBooleanCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == GASObject::Object_Boolean &&
        !fn.ThisPtr->IsBuiltinPrototype())
    {
        GASBooleanObject* nobj = static_cast<GASBooleanObject*>(fn.ThisPtr);
        GASValue retVal = (fn.NArgs > 0)? fn.Arg(0) : GASValue();
        nobj->SetValue(fn.Env, retVal);
        *fn.Result = retVal;
    }
    else
    {
        if (fn.NArgs == 0)
            fn.Result->SetBool(0);
        else
            fn.Result->SetBool(fn.Arg(0).ToBool(fn.Env));
    }
}

GASFunctionRef GASBooleanCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef  ctor(*GHEAP_NEW(pgc->GetHeap()) GASBooleanCtorFunction(&sc));
    GPtr<GASObject> proto = *GHEAP_NEW(pgc->GetHeap()) GASBooleanProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_Boolean, proto);
    pgc->pGlobal->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_Boolean), GASValue(ctor));
    return ctor;
}
