/**********************************************************************

Filename    :   GFxNumber.cpp
Content     :   Number object functinality
Created     :   March 10, 2006
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "AS/GASNumberObject.h"
#include "GASFunctionRef.h"
#include "GASNumberUtil.h"
#include "GMsgFormat.h"

#include <stdio.h>
#include <stdlib.h>

////////////
GASNumberObject::GASNumberObject(GASEnvironment* penv) 
: GASObject(penv), Value(0)
{
    CommonInit (penv);
}

GASNumberObject::GASNumberObject(GASEnvironment* penv, GASNumber val) 
: GASObject(penv), Value(val)
{
    CommonInit(penv);
}

void GASNumberObject::CommonInit(GASEnvironment* penv)
{    
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Number));
}

const char* GASNumberObject::GetTextValue(GASEnvironment* penv) const
{
    GUNUSED(penv);
    char buf[GASNumberUtil::TOSTRING_BUF_SIZE];
    StringValue = GASNumberUtil::ToString(Value, buf, sizeof(buf), 10);
    return StringValue.ToCStr();
}

void        GASNumberObject::SetValue(GASEnvironment* penv, const GASValue& v)
{
    Value = v.ToNumber(penv);
}

GASValue    GASNumberObject::GetValue() const
{
    return GASValue(Value);
}

const char* GASNumberObject::ToString(int radix) const
{
    char buf[GASNumberUtil::TOSTRING_BUF_SIZE];
    StringValue = GASNumberUtil::ToString(Value, buf, sizeof(buf), radix);
    return StringValue.ToCStr();
}

bool GASNumberObject::InvokePrimitiveMethod(const GASFnCall& fnCall, const GASString& methodName)
{
    GASNumberObject* pthis = (GASNumberObject*) fnCall.ThisPtr;
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

////////////////////////
static void GAS_NumberToString(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Number);
    GASNumberObject* pthis = (GASNumberObject*) fn.ThisPtr;
    GASSERT(pthis);
    int radix = 10;

    if (fn.NArgs > 0) {
        // special case if optional parameter (radix) is specified.
        radix = (int) fn.Arg(0).ToNumber(fn.Env);
    }

    fn.Result->SetString(fn.Env->CreateString(pthis->ToString(radix)));
}

static void GAS_NumberValueOf(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Number);
    GASNumberObject* pthis = (GASNumberObject*) fn.ThisPtr;
    GASSERT(pthis);

    fn.Result->SetNumber (pthis->GetValue ().ToNumber(fn.Env));
}


static const GASNameFunction NumberFunctionTable[] = 
{
    { "toString",       &GAS_NumberToString  },
    { "valueOf",        &GAS_NumberValueOf  },
    { 0, 0 }
};

GASNumberProto::GASNumberProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASNumberObject>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, NumberFunctionTable);
}

////////////////////////////////////////
//
struct GASNameNumberFunc
{
    const char* Name;
    GASNumber (*Function)();
};

static const GASNameNumberFunc GASNumberConstTable[] = 
{
    { "MAX_VALUE",         GASNumberUtil::MAX_VALUE          },
    { "MIN_VALUE",         GASNumberUtil::MIN_VALUE          },
    { "NaN",               GASNumberUtil::NaN                },
    { "NEGATIVE_INFINITY", GASNumberUtil::NEGATIVE_INFINITY  },
    { "POSITIVE_INFINITY", GASNumberUtil::POSITIVE_INFINITY  },
    { 0, 0 }
};

GASNumberCtorFunction::GASNumberCtorFunction(GASStringContext *psc) :
    GASCFunctionObject(psc, GlobalCtor)
{
    for(int i = 0; GASNumberConstTable[i].Name; i++)
    {
        SetConstMemberRaw(psc, GASNumberConstTable[i].Name, GASValue(GASNumberConstTable[i].Function()),  
            GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    }
}

void GASNumberCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == GASObject::Object_Number && 
        !fn.ThisPtr->IsBuiltinPrototype())
    {
        GASNumberObject* nobj = static_cast<GASNumberObject*>(fn.ThisPtr);
        GASValue retVal = (fn.NArgs > 0)? fn.Arg(0) : GASValue();
        nobj->SetValue(fn.Env, retVal);
        *fn.Result = retVal;
    }
    else
        fn.Result->SetNumber((fn.NArgs == 0)?0:fn.Arg(0).ToNumber(fn.Env));
}

GASObject* GASNumberCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{
    return GHEAP_NEW(penv->GetHeap()) GASNumberObject(penv);
}


GASFunctionRef GASNumberCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef  ctor(*GHEAP_NEW(pgc->GetHeap()) GASNumberCtorFunction(&sc));
    GPtr<GASObject> proto = *GHEAP_NEW(pgc->GetHeap()) GASNumberProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_Number, proto);
    pgc->pGlobal->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_Number), GASValue(ctor));
    return ctor;
}

