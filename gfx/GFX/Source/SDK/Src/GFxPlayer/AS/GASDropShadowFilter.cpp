/**********************************************************************

Filename    :   GFxASDropShadowFilter.cpp
Content     :   DropShadowFilter reference class for ActionScript 2.0
Created     :   12/10/2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GConfig.h"

#ifndef GFC_NO_FXPLAYER_AS_FILTERS

#include "AS/GASDropShadowFilter.h"

#include "GFxAction.h"

// GASDropShadowFilterObject::GASDropShadowFilterObject()
// : GASBitmapFilterObject(GFxFilterDesc::Filter_DropShadow)
// {
// }

GASDropShadowFilterObject::GASDropShadowFilterObject(GASEnvironment* penv)
: GASBitmapFilterObject(penv, GFxFilterDesc::Filter_DropShadow, GRenderer::Filter_Shadow)
{
}

GASDropShadowFilterObject::~GASDropShadowFilterObject()
{
}

bool GASDropShadowFilterObject::SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags /* = GASPropFlags */)
{
    if (name == "alpha")
    {
        GASNumber alpha = val.ToNumber(penv);
        SetAlpha((Float)alpha);
        return true;
    }
    else if (name == "angle")
    {
        SetAngle((SInt16)val.ToInt32(penv));
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
    else if (name == "distance")
    {
        SetDistance((SInt16)val.ToInt32(penv));
        return true;
    }
    else if (name == "hideObject")
    {
        SetHideObject(val.ToBool(penv));
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

bool GASDropShadowFilterObject::GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
{
    if (name == "alpha")
    {
        val->SetNumber(GetAlpha());
        return true;
    }
    else if (name == "angle")
    {
        val->SetInt(GetAngle());
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
    else if (name == "distance")
    {
        val->SetInt(GetDistance());
        return true;
    }
    else if (name == "hideObject")
    {
        val->SetBool(IsHideObject());
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

const GASNameFunction GASDropShadowFilterProto::FunctionTable[] = 
{
    { "clone",         &GASDropShadowFilterProto::Clone },
    { 0, 0 }
};

GASDropShadowFilterProto::GASDropShadowFilterProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor) :
GASPrototype<GASDropShadowFilterObject>(psc, prototype, constructor)
{
    InitFunctionMembers(psc, FunctionTable, 0);
}

void GASDropShadowFilterProto::Clone(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, DropShadowFilter);
    GASDropShadowFilterObject* pthis = (GASDropShadowFilterObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    GPtr<GASObject> pfilter = *fn.Env->OperatorNew(fn.Env->GetGC()->FlashFiltersPackage, 
        fn.Env->GetBuiltin(GASBuiltin_DropShadowFilter), 0);
    GASDropShadowFilterObject* pnew = static_cast<GASDropShadowFilterObject*>(pfilter.GetPtr());
    pnew->SetFilter(pthis->GetFilter());
    fn.Result->SetAsObject(pfilter);
}


// --------------------------------------------------------------------


GASDropShadowFilterCtorFunction::GASDropShadowFilterCtorFunction(GASStringContext* psc) :
GASCFunctionObject(psc, GASDropShadowFilterCtorFunction::GlobalCtor)
{
    GUNUSED(psc);
}

void GASDropShadowFilterCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASDropShadowFilterObject> pnode;
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == GASObject::Object_DropShadowFilter)
        pnode = static_cast<GASDropShadowFilterObject*>(fn.ThisPtr);
    else
        pnode = *GHEAP_NEW(fn.Env->GetHeap()) GASDropShadowFilterObject(fn.Env);
    fn.Result->SetAsObject(pnode.GetPtr());

    pnode->SetPasses(1);
    pnode->SetDistance(4);
    pnode->SetAngle(45);
    pnode->SetColor(GColor::Black);
    pnode->SetAlpha(1.0f);
    pnode->SetBlurX(4.0f);
    pnode->SetBlurY(4.0f);
    pnode->SetStrength(1.0f);
    pnode->SetKnockOut(false);
    pnode->SetHideObject(false);

    if (fn.NArgs > 0)
    {
        // process distance 
        pnode->SetDistance((SInt16)fn.Arg(0).ToInt32(fn.Env));
        if (fn.NArgs > 1)
        {
            // process angle 
            pnode->SetAngle((SInt16)fn.Arg(1).ToInt32(fn.Env));

            if (fn.NArgs > 2)
            {
                // process color
                pnode->SetColor(fn.Arg(2).ToUInt32(fn.Env));

                if (fn.NArgs > 3)
                {
                    // process alpha
                    pnode->SetAlpha((Float)fn.Arg(3).ToNumber(fn.Env));

                    if (fn.NArgs > 4)
                    {
                        // process blurX
                        pnode->SetBlurX((Float)fn.Arg(4).ToNumber(fn.Env));

                        if (fn.NArgs > 5)
                        {
                            // process blurY
                            pnode->SetBlurY((Float)fn.Arg(5).ToNumber(fn.Env));

                            if (fn.NArgs > 6)
                            {
                                // process strength 
                                pnode->SetStrength((Float)fn.Arg(6).ToNumber(fn.Env));

                                if (fn.NArgs > 7)
                                {
                                    // process quality 
                                    pnode->SetPasses((UInt)fn.Arg(7).ToNumber(fn.Env));

                                    if (fn.NArgs > 8)
                                    {
                                        // process inner
                                        pnode->SetInnerShadow(fn.Arg(8).ToBool(fn.Env));

                                        if (fn.NArgs > 9)
                                        {
                                            // process knockout
                                            pnode->SetKnockOut(fn.Arg(9).ToBool(fn.Env));

                                            if (fn.NArgs > 10)
                                            {
                                                // process hideObject
                                                pnode->SetHideObject(fn.Arg(10).ToBool(fn.Env));
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    GASStringContext* psc = fn.Env->GetSC();
    pnode->SetMemberRaw(psc, psc->CreateConstString("distance"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("angle"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("color"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("alpha"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("blurX"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("blurY"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("strength"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("knockout"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("hideObject"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("inner"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("quality"), GASValue::UNSET);
}

GASObject* GASDropShadowFilterCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{
    return GHEAP_NEW(penv->GetHeap()) GASDropShadowFilterObject(penv);
}

GASFunctionRef GASDropShadowFilterCtorFunction::Register(GASGlobalContext* pgc)
{
    // Register BitmapFilter here also
    if (!pgc->GetBuiltinClassRegistrar(pgc->GetBuiltin(GASBuiltin_BitmapFilter)))
        GASBitmapFilterCtorFunction::Register(pgc);

    GASStringContext sc(pgc, 8);
    GASFunctionRef ctor(*GHEAP_NEW(pgc->GetHeap()) GASDropShadowFilterCtorFunction(&sc));
    GPtr<GASDropShadowFilterProto> proto = 
        *GHEAP_NEW(pgc->GetHeap()) GASDropShadowFilterProto(&sc, pgc->GetPrototype(GASBuiltin_BitmapFilter), ctor);
    pgc->SetPrototype(GASBuiltin_DropShadowFilter, proto);
    pgc->FlashFiltersPackage->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_DropShadowFilter), GASValue(ctor));
    return ctor;
}

#endif  // GFC_NO_FXPLAYER_AS_FILTERS
