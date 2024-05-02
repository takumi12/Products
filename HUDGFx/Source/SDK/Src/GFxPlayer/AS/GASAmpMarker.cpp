/**********************************************************************

Filename    :   GASAmpMarker.cpp
Content     :   Implementation of AMP marker class
Created     :   May, 2010
Authors     :   Alex Mantzaris

Copyright   :   (c) 2001-2010 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "AS/GASAmpMarker.h"
#include "GASValue.h"
#include "GFxPlayerImpl.h"
#include "AMP/GFxAmpViewStats.h"

GASAmpMarker::GASAmpMarker(GASEnvironment* env) : GASObject(env)
{
    commonInit(env);
}

void GASAmpMarker::commonInit(GASEnvironment* env)
{
    Set__proto__(env->GetSC(), env->GetPrototype(GASBuiltin_Amp));
}

GASAmpMarkerProto::GASAmpMarkerProto(GASStringContext* sc, GASObject* prototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASAmpMarker>(sc, prototype, constructor)
{
}

//////////////////
const GASNameFunction GASAmpMarkerCtorFunction::StaticFunctionTable[] = 
{
    { "addMarker",   &GASAmpMarkerCtorFunction::AddMarker },
    { 0, 0 }
};

GASAmpMarkerCtorFunction::GASAmpMarkerCtorFunction(GASStringContext *sc) : GASCFunctionObject(sc, GlobalCtor)
{
    GASNameFunction::AddConstMembers(
        this, sc, StaticFunctionTable, 
        GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
}

void GASAmpMarkerCtorFunction::AddMarker(const GASFnCall& fn)
{
    fn.Result->SetNull();
    if (!fn.Env || fn.NArgs == 0)
    {
        return;
    }

    fn.Env->GetMovieRoot()->AdvanceStats->AddMarker(fn.Arg(0).ToString(fn.Env).ToCStr());
}

void GASAmpMarkerCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    fn.Result->SetNull();
}

bool GASAmpMarkerCtorFunction::GetMember(GASEnvironment* env, const GASString& name, GASValue* val)
{
    if (name == "addMarker") 
    {
        *val = GASValue(env->GetSC(), GASAmpMarkerCtorFunction::AddMarker);
        return true;
    }
    return GASFunctionObject::GetMember(env, name, val);
}

bool GASAmpMarkerCtorFunction::SetMember(GASEnvironment* env, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{
    GFxMovieRoot* proot = env->GetMovieRoot();
    if (name == "addMarker")
    {
        proot->AdvanceStats->AddMarker(val.ToString(env).ToCStr());
        return true;
    }
    return GASFunctionObject::SetMember(env, name, val, flags);
}


GASFunctionRef GASAmpMarkerCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef  ctor(*GHEAP_NEW(pgc->GetHeap()) GASAmpMarkerCtorFunction(&sc));
    GPtr<GASObject> proto = *GHEAP_NEW(pgc->GetHeap()) GASAmpMarkerProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_Amp, proto);
    pgc->pGlobal->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_Amp), GASValue(ctor));
    return ctor;
}
