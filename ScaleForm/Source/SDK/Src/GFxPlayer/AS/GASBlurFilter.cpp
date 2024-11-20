/**********************************************************************

Filename    :   GFxASBlurFilter.cpp
Content     :   BlurFilter reference class for ActionScript 2.0
Created     :   12/10/2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GConfig.h"

#ifndef GFC_NO_FXPLAYER_AS_FILTERS

#include "AS/GASBlurFilter.h"

#include "GFxAction.h"

// GASBlurFilterObject::GASBlurFilterObject()
// : GASBitmapFilterObject(GFxFilterDesc::Filter_Blur)
// {
// }
// 
GASBlurFilterObject::GASBlurFilterObject(GASEnvironment* penv)
: GASBitmapFilterObject(penv, GFxFilterDesc::Filter_Blur, GRenderer::Filter_Blur)
{
}

GASBlurFilterObject::~GASBlurFilterObject()
{
}

bool GASBlurFilterObject::SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags /* = GASPropFlags */)
{
    if (name == "blurX")
    {
        SetBlurX((Float)val.ToNumber(penv));
        return true;
    }
    else if (name == "blurY")
    {
        SetBlurY((Float)val.ToNumber(penv));
        return true;
    }
    else if (name == "quality")
    {
        SetPasses((SInt16)val.ToNumber(penv));
        return true;
    }
    return GASObject::SetMember(penv, name, val, flags);
}

bool GASBlurFilterObject::GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
{
    if (name == "blurX")
    {
        val->SetNumber(GetBlurX());
        return true;
    }
    else if (name == "blurY")
    {
        val->SetNumber(GetBlurY());
        return true;
    }
    else if (name == "quality")
    {
        val->SetInt(GetPasses());
        return true;
    }
    return GASObject::GetMember(penv, name, val);
}


// --------------------------------------------------------------------

const GASNameFunction GASBlurFilterProto::FunctionTable[] = 
{
    { "clone",         &GASBlurFilterProto::Clone },
    { 0, 0 }
};

GASBlurFilterProto::GASBlurFilterProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor) :
GASPrototype<GASBlurFilterObject>(psc, prototype, constructor)
{
    InitFunctionMembers(psc, FunctionTable, 0);
}

void GASBlurFilterProto::Clone(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, BlurFilter);
    GASBlurFilterObject* pthis = (GASBlurFilterObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    GPtr<GASObject> pfilter = *fn.Env->OperatorNew(fn.Env->GetGC()->FlashFiltersPackage, 
        fn.Env->GetBuiltin(GASBuiltin_BlurFilter), 0);
    GASBlurFilterObject* pnew = static_cast<GASBlurFilterObject*>(pfilter.GetPtr());
    pnew->SetFilter(pthis->GetFilter());
    fn.Result->SetAsObject(pfilter);
}


// --------------------------------------------------------------------


GASBlurFilterCtorFunction::GASBlurFilterCtorFunction(GASStringContext* psc) :
GASCFunctionObject(psc, GASBlurFilterCtorFunction::GlobalCtor)
{
    GUNUSED(psc);
}

void GASBlurFilterCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASBlurFilterObject> pnode;
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == GASObject::Object_BlurFilter)
        pnode = static_cast<GASBlurFilterObject*>(fn.ThisPtr);
    else
        pnode = *GHEAP_NEW(fn.Env->GetHeap()) GASBlurFilterObject(fn.Env);
    fn.Result->SetAsObject(pnode.GetPtr());

    pnode->SetAlpha(1.0f);
    pnode->SetBlurX(4.0f);
    pnode->SetBlurY(4.0f);
    // the blur AS2 class does not support this property,
    // but the internal representation requires it to be set to 1
    pnode->SetStrength(1.0f);

    if (fn.NArgs > 0)
    {
        // process blurX
        pnode->SetBlurX((Float)fn.Arg(0).ToNumber(fn.Env));

        if (fn.NArgs > 1)
        {
            // process blurY
            pnode->SetBlurY((Float)fn.Arg(1).ToNumber(fn.Env));

            if (fn.NArgs > 2)
            {
                // process quality 
                pnode->SetPasses((UInt)fn.Arg(2).ToNumber(fn.Env));
            }
        }
    }

    GASStringContext* psc = fn.Env->GetSC();
    pnode->SetMemberRaw(psc, psc->CreateConstString("blurX"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("blurY"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("quality"), GASValue::UNSET);
}

GASObject* GASBlurFilterCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{
    return GHEAP_NEW(penv->GetHeap()) GASBlurFilterObject(penv);
}

GASFunctionRef GASBlurFilterCtorFunction::Register(GASGlobalContext* pgc)
{
    // Register BitmapFilter here also
    if (!pgc->GetBuiltinClassRegistrar(pgc->GetBuiltin(GASBuiltin_BitmapFilter)))
        GASBitmapFilterCtorFunction::Register(pgc);

    GASStringContext sc(pgc, 8);
    GASFunctionRef ctor(*GHEAP_NEW(pgc->GetHeap()) GASBlurFilterCtorFunction(&sc));
    GPtr<GASBlurFilterProto> proto = 
        *GHEAP_NEW(pgc->GetHeap()) GASBlurFilterProto(&sc, pgc->GetPrototype(GASBuiltin_BitmapFilter), ctor);
    pgc->SetPrototype(GASBuiltin_BlurFilter, proto);
    pgc->FlashFiltersPackage->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_BlurFilter), GASValue(ctor));
    return ctor;
}

#endif  // GFC_NO_FXPLAYER_AS_FILTERS
