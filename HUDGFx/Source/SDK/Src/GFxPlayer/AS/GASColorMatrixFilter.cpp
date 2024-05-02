/**********************************************************************

Filename    :   GFxASColorMatrixFilter.cpp
Content     :   ColorMatrixFilter reference class for ActionScript 2.0
Created     :   
Authors     :   
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GConfig.h"

#ifndef GFC_NO_FXPLAYER_AS_FILTERS

#include "AS/GASColorMatrixFilter.h"
#include "AS/GASArrayObject.h"

#include "GFxAction.h"

// GASColorMatrixFilterObject::GASColorMatrixFilterObject()
// : GASBitmapFilterObject(GFxFilterDesc::Filter_ColorMatrix)
// {
// }

GASColorMatrixFilterObject::GASColorMatrixFilterObject(GASEnvironment* penv)
: GASBitmapFilterObject(penv, GFxFilterDesc::Filter_AdjustColor)
{
}

GASColorMatrixFilterObject::~GASColorMatrixFilterObject()
{
}

bool GASColorMatrixFilterObject::SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags /* = GASPropFlags */)
{
    if (name == "matrix")
    {
        GASObject* pobj = val.ToObject(penv);
        if (pobj && pobj->InstanceOf(penv, penv->GetPrototype(GASBuiltin_Array)))
        {
            const UInt Index[] = {0,1,2,3,16, 4,5,6,7,17, 8,9,10,11,18, 12,13,14,15,19};
            GASArrayObject* parrobj = static_cast<GASArrayObject*>(pobj);
            for (int i=0; i < parrobj->GetSize(); ++i)
                Filter.ColorMatrix[Index[i]] = (Float)parrobj->GetElementPtr(i)->ToNumber(penv);
        }
        return true;
    }
    return GASObject::SetMember(penv, name, val, flags);
}

bool GASColorMatrixFilterObject::GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
{
    if (name == "matrix")
    {
        const UInt Index[] = {0,1,2,3,16, 4,5,6,7,17, 8,9,10,11,18, 12,13,14,15,19};
        GPtr<GASArrayObject> arrayObj = *GHEAP_NEW(penv->GetHeap()) GASArrayObject(penv);
        arrayObj->Resize(20);
        for(int i=0;i<20;i++)
            arrayObj->SetElement(i, GASValue(Filter.ColorMatrix[Index[i]]));

        val->SetAsObject(arrayObj);
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

const GASNameFunction GASColorMatrixFilterProto::FunctionTable[] = 
{
    { "clone",         &GASColorMatrixFilterProto::Clone },
    { 0, 0 }
};

GASColorMatrixFilterProto::GASColorMatrixFilterProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor) :
GASPrototype<GASColorMatrixFilterObject>(psc, prototype, constructor)
{
    InitFunctionMembers(psc, FunctionTable, 0);
}

void GASColorMatrixFilterProto::Clone(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, ColorMatrixFilter);
    GASColorMatrixFilterObject* pthis = (GASColorMatrixFilterObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    GPtr<GASObject> pfilter = *fn.Env->OperatorNew(fn.Env->GetGC()->FlashFiltersPackage, 
        fn.Env->GetBuiltin(GASBuiltin_ColorMatrixFilter), 0);
    GASColorMatrixFilterObject* pnew = static_cast<GASColorMatrixFilterObject*>(pfilter.GetPtr());
    pnew->SetFilter(pthis->GetFilter());
    fn.Result->SetAsObject(pfilter);
}


// --------------------------------------------------------------------


GASColorMatrixFilterCtorFunction::GASColorMatrixFilterCtorFunction(GASStringContext* psc) :
GASCFunctionObject(psc, GASColorMatrixFilterCtorFunction::GlobalCtor)
{
    GUNUSED(psc);
}

void GASColorMatrixFilterCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASColorMatrixFilterObject> pnode;
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == GASObject::Object_ColorMatrixFilter)
        pnode = static_cast<GASColorMatrixFilterObject*>(fn.ThisPtr);
    else
        pnode = *GHEAP_NEW(fn.Env->GetHeap()) GASColorMatrixFilterObject(fn.Env);
    fn.Result->SetAsObject(pnode.GetPtr());

    if (fn.NArgs > 0)
    {
        // process matrix
        GASObject* pobj = fn.Arg(0).ToObject(fn.Env);
        if (pobj && pobj->InstanceOf(fn.Env, fn.Env->GetPrototype(GASBuiltin_Array)))
        {
            const UInt Index[] = {0,1,2,3,16, 4,5,6,7,17, 8,9,10,11,18, 12,13,14,15,19};

            GASArrayObject* parrobj = static_cast<GASArrayObject*>(pobj);
            for (int i=0; i < parrobj->GetSize(); ++i)
                pnode->Filter.ColorMatrix[Index[i]] = (Float)parrobj->GetElementPtr(i)->ToNumber(fn.Env);
        }
    }

    GASStringContext* psc = fn.Env->GetSC();
    pnode->SetMemberRaw(psc, psc->CreateConstString("matrix"), GASValue::UNSET);
}

GASObject* GASColorMatrixFilterCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{
    return GHEAP_NEW(penv->GetHeap()) GASColorMatrixFilterObject(penv);
}

GASFunctionRef GASColorMatrixFilterCtorFunction::Register(GASGlobalContext* pgc)
{
    // Register BitmapFilter here also
    if (!pgc->GetBuiltinClassRegistrar(pgc->GetBuiltin(GASBuiltin_BitmapFilter)))
        GASBitmapFilterCtorFunction::Register(pgc);

    GASStringContext sc(pgc, 8);
    GASFunctionRef ctor(*GHEAP_NEW(pgc->GetHeap()) GASColorMatrixFilterCtorFunction(&sc));
    GPtr<GASColorMatrixFilterProto> proto = 
        *GHEAP_NEW(pgc->GetHeap()) GASColorMatrixFilterProto(&sc, pgc->GetPrototype(GASBuiltin_BitmapFilter), ctor);
    pgc->SetPrototype(GASBuiltin_ColorMatrixFilter, proto);
    pgc->FlashFiltersPackage->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_ColorMatrixFilter), GASValue(ctor));
    return ctor;
}

#endif  // GFC_NO_FXPLAYER_AS_FILTERS
