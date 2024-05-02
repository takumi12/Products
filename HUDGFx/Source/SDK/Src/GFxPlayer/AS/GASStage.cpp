/**********************************************************************

Filename    :   GFxStage.cpp
Content     :   Stage class implementation
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

#include "AS/GASStage.h"

#ifndef GFC_NO_FXPLAYER_AS_STAGE
#include "GASFunctionRef.h"
#include "GASValue.h"
#include "AS/GASAsBroadcaster.h"
#include "AS/GASRectangleObject.h"
#include "AS/GASPointObject.h"

#include <stdio.h>
#include <stdlib.h>

GASStageObject::GASStageObject(GASEnvironment* penv)
: GASObject(penv)
{    
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Stage));
}


//////////////////////////////////////////
//
static void GFx_StageFuncStub(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
}

static const GASNameFunction GFx_StageFunctionTable[] = 
{
    { "addListener",        &GFx_StageFuncStub },
    { "removeListener",     &GFx_StageFuncStub },
    { "broadcastMessage",   &GFx_StageFuncStub },
    { 0, 0 }
};

GASStageProto::GASStageProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASStageObject>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GFx_StageFunctionTable);
}


//////////////////
const GASNameFunction GASStageCtorFunction::StaticFunctionTable[] = 
{
    { "translateToScreen", &GASStageCtorFunction::TranslateToScreen },
    { 0, 0 }
};

GASStageCtorFunction::GASStageCtorFunction(GASStringContext *psc, GFxMovieRoot* movieRoot) : 
    GASCFunctionObject(psc, GlobalCtor), pMovieRoot(movieRoot) 
{ 
    // Stage is a broadcaster
    GASAsBroadcaster::Initialize(psc, this);

    GASFunctionObject::SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_width), GASValue(GASValue::UNSET));
    GASFunctionObject::SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_height), GASValue(GASValue::UNSET));
    SetConstMemberRaw(psc, "scaleMode", GASValue(GASValue::UNSET));
    SetConstMemberRaw(psc, "align", GASValue(GASValue::UNSET));
    
    SetConstMemberRaw(psc, "showMenu", GASValue(true));

    GASNameFunction::AddConstMembers(
        this, psc, StaticFunctionTable, 
        GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
}


void GASStageCtorFunction::TranslateToScreen(const GASFnCall& fn)
{
    GASEnvironment* penv = fn.Env;
    GASSERT(penv);
    if (fn.NArgs < 0) return;

    GASValue px(0), py(0);
    GASValue arg = fn.Arg(0);
    GASObject* pin = arg.ToObject(penv);
    if (!pin) return;

    pin->GetMember(penv, penv->GetBuiltin(GASBuiltin_x), &px);
    pin->GetMember(penv, penv->GetBuiltin(GASBuiltin_y), &py);

    GPointF ip((Float)px.ToNumber(penv), (Float)py.ToNumber(penv));
    GPointF op = fn.Env->GetMovieRoot()->TranslateToScreen(ip);
    
#ifndef GFC_NO_FXPLAYER_AS_POINT
    GPtr<GASPointObject> pobj = *static_cast<GASPointObject*>
        (fn.Env->OperatorNew(penv->GetGC()->FlashGeomPackage, fn.Env->GetBuiltin(GASBuiltin_Point)));
    GASSERT(pobj);
    GASPoint aspt(op.x, op.y);
    pobj->SetProperties(penv, aspt);
#else
    GPtr<GASObject> pobj = *penv->OperatorNew(penv->GetBuiltin(GASBuiltin_Object));
    GASStringContext *psc = penv->GetSC();
    pobj->SetConstMemberRaw(psc, "x", (GASNumber)op.x);
    pobj->SetConstMemberRaw(psc, "y", (GASNumber)op.y);
#endif
    fn.Result->SetAsObject(pobj.GetPtr());
}


bool    GASStageCtorFunction::GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val)
{
    if (psc->GetBuiltin(GASBuiltin_width).CompareBuiltIn_CaseCheck(name, psc->IsCaseSensitive()))        
    {
        val->SetInt(int(TwipsToPixels(pMovieRoot->VisibleFrameRect.Width())));
        return true;
    }
    else if (psc->GetBuiltin(GASBuiltin_height).CompareBuiltIn_CaseCheck(name, psc->IsCaseSensitive()))
    {
        val->SetInt(int(TwipsToPixels(pMovieRoot->VisibleFrameRect.Height())));
        return true;
    }
    else if (psc->CompareConstString_CaseCheck(name, "scaleMode"))
    {
        const char* scaleMode; 
        switch(pMovieRoot->GetViewScaleMode())
        {
            case GFxMovieView::SM_NoScale:  scaleMode = "noScale"; break;
            case GFxMovieView::SM_ExactFit: scaleMode = "exactFit"; break;
            case GFxMovieView::SM_NoBorder: scaleMode = "noBorder"; break;
            default: scaleMode = "showAll";
        }
        val->SetString(psc->CreateConstString(scaleMode));
        return true;
    }
    else if (psc->CompareConstString_CaseCheck(name, "align"))
    {
        const char* align; 
        switch(pMovieRoot->GetViewAlignment())
        {
            case GFxMovieView::Align_TopCenter:     align = "T"; break;
            case GFxMovieView::Align_BottomCenter:  align = "B"; break;
            case GFxMovieView::Align_CenterLeft:    align = "L"; break;
            case GFxMovieView::Align_CenterRight:   align = "R"; break;
            case GFxMovieView::Align_TopLeft:       align = "LT"; break; // documented as TL, but returns LT
            case GFxMovieView::Align_TopRight:      align = "TR"; break;
            case GFxMovieView::Align_BottomLeft:    align = "LB"; break; // documented as BL, but returns LB
            case GFxMovieView::Align_BottomRight:   align = "RB"; break; // documented as BR, but returns RB
            default: align = "";
        }
        val->SetString(psc->CreateConstString(align));
        return true;
    }
    return GASFunctionObject::GetMemberRaw(psc, name, val);
}

GASValue GASStageCtorFunction::CreateRectangleObject(GASEnvironment *penv, const GRectF& rect)
{
    GASValue v;
#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
    GPtr<GASRectangleObject> prect = *static_cast<GASRectangleObject*>
        (penv->OperatorNew(penv->GetGC()->FlashGeomPackage, penv->GetBuiltin(GASBuiltin_Rectangle)));
    GASRect gr( (GASNumber)rect.Left,  (GASNumber)rect.Top,
                (GASNumber)rect.Right, (GASNumber)rect.Bottom); 
    prect->SetProperties(penv, gr);
    v.SetAsObject(prect);
#else
    GPtr<GASObject> bounds = *penv->OperatorNew(penv->GetBuiltin(GASBuiltin_Object));
    GASStringContext *psc = penv->GetSC();
    bounds->SetConstMemberRaw(psc, "x", (GASNumber)rect.Left);
    bounds->SetConstMemberRaw(psc, "y", (GASNumber)rect.Top);
    bounds->SetConstMemberRaw(psc, "width", (GASNumber)rect.Width());
    bounds->SetConstMemberRaw(psc, "height", (GASNumber)rect.Height());
    v.SetAsObject(bounds);
#endif
    return v;
}

bool    GASStageCtorFunction::GetMember(GASEnvironment *penv, const GASString& name, GASValue* val)
{
    if (penv->CheckExtensions())
    {
        if (penv->GetSC()->CompareConstString_CaseCheck(name, "visibleRect"))
        {
            GRectF frameRect = penv->GetMovieRoot()->GetVisibleFrameRect();
            *val = CreateRectangleObject(penv, frameRect);
            return true;
        }
        else if (penv->GetSC()->CompareConstString_CaseCheck(name, "safeRect"))
        {
            GRectF safeRect = penv->GetMovieRoot()->GetSafeRect();
            if (safeRect.IsEmpty())
                safeRect = penv->GetMovieRoot()->GetVisibleFrameRect();
            *val = CreateRectangleObject(penv, safeRect);
            return true;
        }
        else if (penv->GetSC()->CompareConstString_CaseCheck(name, "originalRect"))
        {
            GRectF origRect = penv->GetMovieRoot()->GetMovieDef()->GetFrameRect();
            *val = CreateRectangleObject(penv, origRect);
            return true;
        }
    }
    return GASFunctionObject::GetMember(penv, name, val);
}

bool    GASStageCtorFunction::SetMember(GASEnvironment *penv, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{
    GASStringContext * const psc = penv->GetSC();

    if (psc->CompareConstString_CaseCheck(name, "scaleMode"))
    {   
        const GASString& scaleModeStr = val.ToString(penv);
        GFxMovieView::ScaleModeType scaleMode, prevScaleMode = pMovieRoot->GetViewScaleMode();

        if (psc->CompareConstString_CaseInsensitive(scaleModeStr, "noScale"))
            scaleMode = GFxMovieView::SM_NoScale;
        else if (psc->CompareConstString_CaseInsensitive(scaleModeStr, "exactFit"))
            scaleMode = GFxMovieView::SM_ExactFit;
        else if (psc->CompareConstString_CaseInsensitive(scaleModeStr, "noBorder"))
            scaleMode = GFxMovieView::SM_NoBorder;
        else
            scaleMode = GFxMovieView::SM_ShowAll;
        pMovieRoot->SetViewScaleMode(scaleMode);
        if (prevScaleMode != scaleMode && scaleMode == GFxMovieView::SM_NoScale)
        {
            // invoke onResize
            NotifyOnResize(penv);
        }
        return true;
    }
    else if (psc->CompareConstString_CaseCheck(name, "align"))
    {
        const GASString& alignStr = val.ToString(penv).ToUpper();
        GFxMovieView::AlignType align;
        UInt char1 = 0, char2 = 0;
        UInt len = alignStr.GetLength();
        if (len >= 1)
        {
            char1 = alignStr.GetCharAt(0);
            if (len >= 2)
                char2 = alignStr.GetCharAt(1);
        }
        if ((char1 == 'T' && char2 == 'L') || (char1 == 'L' && char2 == 'T')) // TL or LT
            align = GFxMovieView::Align_TopLeft;
        else if ((char1 == 'T' && char2 == 'R') || (char1 == 'R' && char2 == 'T')) // TR or RT
            align = GFxMovieView::Align_TopRight;
        else if ((char1 == 'B' && char2 == 'L') || (char1 == 'L' && char2 == 'B')) // BL or LB
            align = GFxMovieView::Align_BottomLeft;
        else if ((char1 == 'B' && char2 == 'R') || (char1 == 'R' && char2 == 'B')) // BR or RB
            align = GFxMovieView::Align_BottomRight;
        else if (char1 == 'T')
            align = GFxMovieView::Align_TopCenter;
        else if (char1 == 'B')
            align = GFxMovieView::Align_BottomCenter;
        else if (char1 == 'L')
            align = GFxMovieView::Align_CenterLeft;
        else if (char1 == 'R')
            align = GFxMovieView::Align_CenterRight;
        else 
            align = GFxMovieView::Align_Center;
        pMovieRoot->SetViewAlignment(align);
        return true;
    }
    return GASFunctionObject::SetMember(penv, name, val, flags);
}

bool    GASStageCtorFunction::SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{
    if (psc->GetBuiltin(GASBuiltin_width).CompareBuiltIn_CaseCheck(name, psc->IsCaseSensitive()) ||
        psc->GetBuiltin(GASBuiltin_height).CompareBuiltIn_CaseCheck(name, psc->IsCaseSensitive()))
    {
        // read-only, do nothing
        return true;
    }
    return GASFunctionObject::SetMemberRaw(psc, name, val, flags);
}

void GASStageCtorFunction::NotifyOnResize(GASEnvironment* penv)
{
    if (penv->CheckExtensions())
    {
        // extension: onResize handler receives a parameter (flash.geom.Rectangle) with 
        // the currently visible rectangle.

        GRectF frameRect = penv->GetMovieRoot()->GetVisibleFrameRect();
        GASValue v = CreateRectangleObject(penv, frameRect);
        penv->Push(v);
        GASAsBroadcaster::BroadcastMessage(penv, this, penv->CreateConstString("onResize"),  1, penv->GetTopIndex());
        penv->Drop1();
    }
    else
    {
        GASAsBroadcaster::BroadcastMessage(penv, this, penv->CreateConstString("onResize"), 0, 0);
    }
}

void GASStageCtorFunction::NotifyOnResize(const GASFnCall& fn)
{
    GASValue stageCtorVal;
    if (fn.Env->GetGC()->pGlobal->GetMemberRaw(fn.Env->GetSC(), fn.Env->GetBuiltin(GASBuiltin_Stage), &stageCtorVal))
    {
        //GRectF r = fn.Env->GetMovieRoot()->GetVisibleFrameRect();
        //printf("Frame rect = %f, %f, {%f, %f}\n", r.Top, r.Left, r.Width(), r.Height());
        if (!stageCtorVal.IsFunctionName()) // is it resolved already? if not - skip
        {
            GASObjectInterface* pstageObj = stageCtorVal.ToObject(fn.Env);
            if (pstageObj)
            {
                static_cast<GASStageCtorFunction*>(pstageObj)->NotifyOnResize(fn.Env);
            }
            else
                GASSERT(0);
        }
    }
    else
        GASSERT(0);
}

void GASStageCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    fn.Result->SetNull();
}

GASFunctionRef GASStageCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef  ctor(*GHEAP_NEW(pgc->GetHeap()) GASStageCtorFunction(&sc, pgc->GetMovieRoot()));
    GPtr<GASObject> proto = *GHEAP_NEW(pgc->GetHeap()) GASStageProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_Stage, proto);
    pgc->pGlobal->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_Stage), GASValue(ctor));
    return ctor;
}
#endif //#ifndef GFC_NO_FXPLAYER_AS_STAGE
