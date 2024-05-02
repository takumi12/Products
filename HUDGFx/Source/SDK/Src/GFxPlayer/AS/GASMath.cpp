/**********************************************************************

Filename    :   GFxMath.cpp
Content     :   AS2 Implementation of Math class
Created     :   January 6, 2009
Authors     :   Prasad Silva

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "AS/GASMath.h"
#include "GFxAction.h"
#include "GTypes.h"
#include "GFxRandom.h"
#include "GMsgFormat.h"

GASMath::GASMath(GASEnvironment* penv)
: GASObject(penv)
{
    Set__proto__ (penv->GetSC(), penv->GetPrototype(GASBuiltin_Math));
}

UInt32 GASMath::GetNextRandom(GFxMovieRoot* proot)
{
    UInt32 rn;
    GFxTestStream* pts = proot->GetTestStream();
    if (pts)
    {
        if (pts->TestStatus == GFxTestStream::Record)
        {
            rn =  GFxRandom::NextRandom();

            GLongFormatter f(rn);
            f.Convert();
            pts->SetParameter("random", f.ToCStr());
        }
        else
        {
            GString tstr;
            pts->GetParameter("random", &tstr);
            rn = G_strtoul(tstr.ToCStr(), NULL, 10);
        }
    }
    else
        rn = GFxRandom::NextRandom();

    return rn;
}
    
//////////////////////////////////////////
//

GASMathProto::GASMathProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASMath>(psc, pprototype, constructor)
{
    // Needed? Check back to GASSelection code
    //InitFunctionMembers (psc, GAS_MathFunctionTable);
}

//////////////////

const GASNameFunction GASMathCtorFunction::StaticFunctionTable[] = 
{
    { "abs",    &GASMathCtorFunction::Abs },
    { "acos",    &GASMathCtorFunction::Acos },
    { "asin",    &GASMathCtorFunction::Asin },
    { "atan",   &GASMathCtorFunction::Atan },
    { "atan2",  &GASMathCtorFunction::Atan2 },
    { "ceil",    &GASMathCtorFunction::Ceil },
    { "cos",    &GASMathCtorFunction::Cos },
    { "exp",    &GASMathCtorFunction::Exp },
    { "floor",    &GASMathCtorFunction::Floor },
    { "log",    &GASMathCtorFunction::Log },
    { "max",    &GASMathCtorFunction::Max },
    { "min",    &GASMathCtorFunction::Min },
    { "pow",    &GASMathCtorFunction::Pow },
    { "random",    &GASMathCtorFunction::Random },
    { "round",    &GASMathCtorFunction::Round },
    { "sin",    &GASMathCtorFunction::Sin },
    { "sqrt",    &GASMathCtorFunction::Sqrt },
    { "tan",    &GASMathCtorFunction::Tan },
    { 0, 0 }
};

GASMathCtorFunction::GASMathCtorFunction(GASStringContext *psc)
:   GASCFunctionObject(psc, GlobalCtor)
{
    // constant
    this->SetConstMemberRaw(psc, "E",       (GASNumber)2.7182818284590452354    );
    this->SetConstMemberRaw(psc, "LN2",     (GASNumber)0.69314718055994530942   );
    this->SetConstMemberRaw(psc, "LOG2E",   (GASNumber)1.4426950408889634074    );
    this->SetConstMemberRaw(psc, "LN10",    (GASNumber)2.30258509299404568402   );
    this->SetConstMemberRaw(psc, "LOG10E",  (GASNumber)0.43429448190325182765   );
    this->SetConstMemberRaw(psc, "PI",      (GASNumber)3.14159265358979323846   );
    this->SetConstMemberRaw(psc, "SQRT1_2", (GASNumber)0.7071067811865475244    );
    this->SetConstMemberRaw(psc, "SQRT2",   (GASNumber)1.4142135623730950488    );

    GASNameFunction::AddConstMembers(
        this, psc, StaticFunctionTable, 
        GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
}

void GASMathCtorFunction::Random(const GASFnCall& fn)
{
    // Random number between 0 and 1.
#ifdef GFC_NO_DOUBLE
    fn.Result->SetNumber((GASMath::GetNextRandom(fn.Env->GetMovieRoot()) & 0x7FFFFF) / Float(UInt32(0x7FFFFF)));
#else
    fn.Result->SetNumber(GASMath::GetNextRandom(fn.Env->GetMovieRoot())/ double(UInt32(0xFFFFFFFF)));
#endif
}

void GASMathCtorFunction::Round(const GASFnCall& fn)
{
    // round argument to nearest int.
    GASNumber   arg0 = fn.Arg(0).ToNumber(fn.Env);
    fn.Result->SetNumber((GASNumber)floor(arg0 + 0.5f));
}

void GASMathCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    fn.Result->SetNull();
}

GASFunctionRef GASMathCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef  ctor(*GHEAP_NEW(pgc->GetHeap()) GASMathCtorFunction(&sc));
    GPtr<GASObject> proto = *GHEAP_NEW(pgc->GetHeap()) GASMathProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_Math, proto);
    pgc->pGlobal->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_Math), GASValue(ctor));
    return ctor;
}
