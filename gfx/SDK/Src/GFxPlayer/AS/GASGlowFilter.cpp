/**********************************************************************

Filename    :   GFxASGlowFilter.cpp
Content     :   GlowFilter reference class for ActionScript 2.0
Created     :   12/10/2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GConfig.h"

#ifndef GFC_NO_FXPLAYER_AS_FILTERS

#include "AS/GASGlowFilter.h"

#include "GFxAction.h"

// GASGlowFilterObject::GASGlowFilterObject()
// : GASBitmapFilterObject(GFxFilterDesc::Filter_Glow)
// {
// }

GASGlowFilterObject::GASGlowFilterObject(GASEnvironment* penv)
: GASBitmapFilterObject(penv, GFxFilterDesc::Filter_Glow, GRenderer::Filter_Shadow)
{
}

GASGlowFilterObject::~GASGlowFilterObject()
{
}

bool GASGlowFilterObject::SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags /* = GASPropFlags */)
{
    if (name == "alpha")
    {
        GASNumber alpha = val.ToNumber(penv);
        SetAlpha((Float)alpha);
        return true;
    }
    else if (name == "blurX")
    {
        SetBlurX((Float)val.ToNumber(penv));
        return true;
    }
    else if (name == "blurY")
    {
        SetBlurY((Float)val.ToNumber(penv));
        return true;
    }
    else if (name == "color")
    {
        SetColor(val.ToUInt32(penv));
        return true;
    }
    else if (name == "inner")
    {
        SetInnerShadow(val.ToBool(penv));
        return true;
    }
    else if (name == "knockout")
    {
        SetKnockOut(val.ToBool(penv));
        return true;
    }
    else if (name == "quality")
    {
        SetPasses((UInt)val.ToNumber(penv));
        return true;
    }
    else if (name == "strength")
    {
        SetStrength((Float)val.ToNumber(penv));
        return true;
    }
    return GASObject::SetMember(penv, name, val, flags);
}

bool GASGlowFilterObject::GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
{
    if (name == "alpha")
    {
        val->SetNumber(GetAlpha());
        return true;
    }
    else if (name == "blurX")
    {
        val->SetNumber(GetBlurX());
        return true;
    }
    else if (name == "blurY")
    {
        val->SetNumber(GetBlurY());
        return true;
    }
    else if (name == "color")
    {
        val->SetUInt(GetColor());
        return true;
    }
    else if (name == "inner")
    {
        val->SetBool(IsInnerShadow());
        return true;
    }
    else if (name == "knockout")
    {
        val->SetBool(IsKnockOut());
        return true;
    }
    else if (name == "quality")
    {
        val->SetInt(GetPasses());
        return true;
    }
    else if (name == "strength")
    {
        val->SetNumber(GetStrength());
        return true;
    }
    return GASObject::GetMember(penv, name, val);
}


// --------------------------------------------------------------------

const GASNameFunction GASGlowFilterProto::FunctionTable[] = 
{
    { "clone",         &GASGlowFilterProto::Clone },
    { 0, 0 }
};

GASGlowFilterProto::GASGlowFilterProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor) :
GASPrototype<GASGlowFilterObject>(psc, prototype, constructor)
{
    InitFunctionMembers(psc, FunctionTable, 0);
}

void GASGlowFilterProto::Clone(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, GlowFilter);
    GASGlowFilterObject* pthis = (GASGlowFilterObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    GPtr<GASObject> pfilter = *fn.Env->OperatorNew(fn.Env->GetGC()->FlashFiltersPackage, 
        fn.Env->GetBuiltin(GASBuiltin_GlowFilter), 0);
    GASGlowFilterObject* pnew = static_cast<GASGlowFilterObject*>(pfilter.GetPtr());
    pnew->SetFilter(pthis->GetFilter());
    fn.Result->SetAsObject(pfilter);
}


// --------------------------------------------------------------------


GASGlowFilterCtorFunction::GASGlowFilterCtorFunction(GASStringContext* psc) :
GASCFunctionObject(psc, GASGlowFilterCtorFunction::GlobalCtor)
{
    GUNUSED(psc);
}

void GASGlowFilterCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASGlowFilterObject> pnode;
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == GASObject::Object_GlowFilter)
        pnode = static_cast<GASGlowFilterObject*>(fn.ThisPtr);
    else
        pnode = *GHEAP_NEW(fn.Env->GetHeap()) GASGlowFilterObject(fn.Env);
    fn.Result->SetAsObject(pnode.GetPtr());

    pnode->SetDistance(0);
    pnode->SetAngle(0);
    pnode->SetColor(GColor::Red);
    pnode->SetAlpha(1.0f);
    pnode->SetBlurX(6.0f);
    pnode->SetBlurY(6.0f);
    pnode->SetStrength(2.0f);
    pnode->SetKnockOut(false);
    pnode->SetHideObject(false);

    if (fn.NArgs > 0)
    {
        // process color
        pnode->SetColor(fn.Arg(0).ToUInt32(fn.Env));

        if (fn.NArgs > 1)
        {
            // process alpha
            pnode->SetAlpha((Float)fn.Arg(1).ToNumber(fn.Env));

            if (fn.NArgs > 2)
            {
                // process blurX
                pnode->SetBlurX((Float)fn.Arg(2).ToNumber(fn.Env));

                if (fn.NArgs > 3)
                {
                    // process blurY
                    pnode->SetBlurY((Float)fn.Arg(3).ToNumber(fn.Env));

                    if (fn.NArgs > 4)
                    {
                        // process strength 
                        pnode->SetStrength((Float)fn.Arg(4).ToNumber(fn.Env));

                        if (fn.NArgs > 5)
                        {
                            // process quality 
                            pnode->SetPasses((UInt)fn.Arg(5).ToNumber(fn.Env));

                            if (fn.NArgs > 6)
                            {
                                // process inner
                                pnode->SetInnerShadow(fn.Arg(6).ToBool(fn.Env));

                                if (fn.NArgs > 7)
                                {
                                    // process knockout
                                    pnode->SetKnockOut(fn.Arg(7).ToBool(fn.Env));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    GASStringContext* psc = fn.Env->GetSC();
    pnode->SetMemberRaw(psc, psc->CreateConstString("color"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("alpha"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("blurX"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("blurY"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("strength"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("knockout"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("inner"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("quality"), GASValue::UNSET);
}

GASObject* GASGlowFilterCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{
    return GHEAP_NEW(penv->GetHeap()) GASGlowFilterObject(penv);
}

GASFunctionRef GASGlowFilterCtorFunction::Register(GASGlobalContext* pgc)
{
    // Register BitmapFilter here also
    if (!pgc->GetBuiltinClassRegistrar(pgc->GetBuiltin(GASBuiltin_BitmapFilter)))
        GASBitmapFilterCtorFunction::Register(pgc);

    GASStringContext sc(pgc, 8);
    GASFunctionRef ctor(*GHEAP_NEW(pgc->GetHeap()) GASGlowFilterCtorFunction(&sc));
    GPtr<GASGlowFilterProto> proto = 
        *GHEAP_NEW(pgc->GetHeap()) GASGlowFilterProto(&sc, pgc->GetPrototype(GASBuiltin_BitmapFilter), ctor);
    pgc->SetPrototype(GASBuiltin_GlowFilter, proto);
    pgc->FlashFiltersPackage->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_GlowFilter), GASValue(ctor));
    return ctor;
}

#endif  // GFC_NO_FXPLAYER_AS_FILTERS
