/**********************************************************************

Filename    :   GFxASBevelFilter.cpp
Content     :   BevelFilter reference class for ActionScript 2.0
Created     :   12/10/2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GConfig.h"

#ifndef GFC_NO_FXPLAYER_AS_FILTERS

#include "AS/GASBevelFilter.h"

#include "GFxAction.h"

// GASBevelFilterObject::GASBevelFilterObject()
// : GASBitmapFilterObject(GFxFilterDesc::Filter_Bevel)
// {
// }

GASBevelFilterObject::GASBevelFilterObject(GASEnvironment* penv)
: GASBitmapFilterObject(penv, GFxFilterDesc::Filter_Bevel, GRenderer::Filter_Shadow|GRenderer::Filter_Highlight)
{
}

GASBevelFilterObject::~GASBevelFilterObject()
{
}

bool GASBevelFilterObject::SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags /* = GASPropFlags */)
{
    if (name == "angle")
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
    else if (name == "distance")
    {
        SetDistance((SInt16)val.ToInt32(penv));
        return true;
    }
    else if (name == "highlightAlpha")
    {
        GASNumber alpha = val.ToNumber(penv);
        SetAlpha((Float)alpha);
        return true;
    }
    else if (name == "highlightColor")
    {
        SetColor(val.ToUInt32(penv));
        return true;
    }
    else if (name == "shadowAlpha")
    {
        GASNumber alpha = val.ToNumber(penv);
        SetAlpha2((Float)alpha);
        return true;
    }
    else if (name == "shadowColor")
    {
        SetColor2(val.ToUInt32(penv));
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
    else if (name == "type")
    {
        GASString type = val.ToString(penv);
        if (type == "inner")
            SetInnerShadow(true);
        else
            SetInnerShadow(false);
        return true;
    }
    else if (name == "strength")
    {
        SetStrength((Float)val.ToNumber(penv));
        return true;
    }
    return GASObject::SetMember(penv, name, val, flags);
}

bool GASBevelFilterObject::GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
{
    if (name == "angle")
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
    else if (name == "distance")
    {
        val->SetInt(GetDistance());
        return true;
    }
    else if (name == "highlightAlpha")
    {
        val->SetNumber(GetAlpha());
        return true;
    }
    else if (name == "highlightColor")
    {
        val->SetUInt(GetColor());
        return true;
    }
    else if (name == "shadowAlpha")
    {
        val->SetNumber(GetAlpha2());
        return true;
    }
    else if (name == "shadowColor")
    {
        val->SetUInt(GetColor2());
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
    else if (name == "type")
    {
        if (IsInnerShadow())
            val->SetString(penv->CreateString("inner"));
        else
            val->SetString(penv->CreateString("outer"));
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

const GASNameFunction GASBevelFilterProto::FunctionTable[] = 
{
    { "clone",         &GASBevelFilterProto::Clone },
    { 0, 0 }
};

GASBevelFilterProto::GASBevelFilterProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor) :
GASPrototype<GASBevelFilterObject>(psc, prototype, constructor)
{
    InitFunctionMembers(psc, FunctionTable, 0);
}

void GASBevelFilterProto::Clone(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, BevelFilter);
    GASBevelFilterObject* pthis = (GASBevelFilterObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    GPtr<GASObject> pfilter = *fn.Env->OperatorNew(fn.Env->GetGC()->FlashFiltersPackage, 
        fn.Env->GetBuiltin(GASBuiltin_BevelFilter), 0);
    GASBevelFilterObject* pnew = static_cast<GASBevelFilterObject*>(pfilter.GetPtr());
    pnew->SetFilter(pthis->GetFilter());
    fn.Result->SetAsObject(pfilter);
}


// --------------------------------------------------------------------


GASBevelFilterCtorFunction::GASBevelFilterCtorFunction(GASStringContext* psc) :
GASCFunctionObject(psc, GASBevelFilterCtorFunction::GlobalCtor)
{
    GUNUSED(psc);
}

void GASBevelFilterCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASBevelFilterObject> pnode;
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == GASObject::Object_BevelFilter)
        pnode = static_cast<GASBevelFilterObject*>(fn.ThisPtr);
    else
        pnode = *GHEAP_NEW(fn.Env->GetHeap()) GASBevelFilterObject(fn.Env);
    fn.Result->SetAsObject(pnode.GetPtr());

    pnode->SetPasses(1);
    pnode->SetDistance(4);
    pnode->SetAngle(45);
    pnode->SetColor(GColor::Black);
    pnode->SetAlpha(1.0f);
    pnode->SetColor2(GColor::White);
    pnode->SetAlpha2(1.0f);
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
                // process highlight color
                pnode->SetColor(fn.Arg(2).ToUInt32(fn.Env));

                if (fn.NArgs > 3)
                {
                    // process highlight alpha
                    pnode->SetAlpha((Float)fn.Arg(3).ToNumber(fn.Env));

                    if (fn.NArgs > 4)
                    {
                        // process color
                        pnode->SetColor2(fn.Arg(4).ToUInt32(fn.Env));

                        if (fn.NArgs > 5)
                        {
                            // process alpha
                            pnode->SetAlpha2((Float)fn.Arg(5).ToNumber(fn.Env));

                            if (fn.NArgs > 6)
                            {
                                // process blurX
                                pnode->SetBlurX((Float)fn.Arg(6).ToNumber(fn.Env));

                                if (fn.NArgs > 7)
                                {
                                    // process blurY
                                    pnode->SetBlurY((Float)fn.Arg(7).ToNumber(fn.Env));

                                    if (fn.NArgs > 8)
                                    {
                                        // process strength 
                                        pnode->SetStrength((Float)fn.Arg(8).ToNumber(fn.Env));

                                        if (fn.NArgs > 9)
                                        {
                                            // process quality 
                                            pnode->SetPasses((UInt)fn.Arg(9).ToNumber(fn.Env));

                                            if (fn.NArgs > 10)
                                            {
                                                // process type
                                                GASString type = fn.Arg(10).ToString(fn.Env);
                                                if (type == "inner")
                                                    pnode->SetInnerShadow(true);
                                                else
                                                    pnode->SetInnerShadow(false);

                                                if (fn.NArgs > 11)
                                                {
                                                    // process knockout
                                                    pnode->SetKnockOut(fn.Arg(11).ToBool(fn.Env));
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
    }

    GASStringContext* psc = fn.Env->GetSC();
    pnode->SetMemberRaw(psc, psc->CreateConstString("shadowColor"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("shadowAlpha"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("highlightColor"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("highlightAlpha"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("blurX"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("blurY"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("strength"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("knockout"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("inner"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("type"), GASValue::UNSET);
    pnode->SetMemberRaw(psc, psc->CreateConstString("quality"), GASValue::UNSET);
}

GASObject* GASBevelFilterCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{
    return GHEAP_NEW(penv->GetHeap()) GASBevelFilterObject(penv);
}

GASFunctionRef GASBevelFilterCtorFunction::Register(GASGlobalContext* pgc)
{
    // Register BitmapFilter here also
    if (!pgc->GetBuiltinClassRegistrar(pgc->GetBuiltin(GASBuiltin_BitmapFilter)))
        GASBitmapFilterCtorFunction::Register(pgc);

    GASStringContext sc(pgc, 8);
    GASFunctionRef ctor(*GHEAP_NEW(pgc->GetHeap()) GASBevelFilterCtorFunction(&sc));
    GPtr<GASBevelFilterProto> proto = 
        *GHEAP_NEW(pgc->GetHeap()) GASBevelFilterProto(&sc, pgc->GetPrototype(GASBuiltin_BitmapFilter), ctor);
    pgc->SetPrototype(GASBuiltin_BevelFilter, proto);
    pgc->FlashFiltersPackage->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_BevelFilter), GASValue(ctor));
    return ctor;
}

#endif  // GFC_NO_FXPLAYER_AS_FILTERS
