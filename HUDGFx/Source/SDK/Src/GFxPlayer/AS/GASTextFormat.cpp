/**********************************************************************

Filename    :   GFxTextFormat.cpp
Content     :   TextFormat object functinality
Created     :   April 17, 2007
Authors     :   Artyom Bolgar
Notes       :   

Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "AS/GASTextFormat.h"

#ifndef GFC_NO_FXPLAYER_AS_TEXTFORMAT
#include "AS/GASArrayObject.h"
#include "AS/GASNumberObject.h"
#include "Text/GFxTextDocView.h"
#include "GFxPlayerImpl.h"

#include <stdio.h>
#include <stdlib.h>


GASTextFormatObject::GASTextFormatObject(GASEnvironment* penv)
    : GASObject(penv), TextFormat(penv->GetHeap())
{
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_TextFormat));
    SetConstMemberRaw(penv->GetSC(), "align", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(penv->GetSC(), "blockIndent", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(penv->GetSC(), "bold", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(penv->GetSC(), "bullet", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(penv->GetSC(), "color", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(penv->GetSC(), "font", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(penv->GetSC(), "indent", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(penv->GetSC(), "italic", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(penv->GetSC(), "leading", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(penv->GetSC(), "leftMargin", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(penv->GetSC(), "rightMargin", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(penv->GetSC(), "size", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(penv->GetSC(), "tabStops", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(penv->GetSC(), "target", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(penv->GetSC(), "underline", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(penv->GetSC(), "url", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    if (penv->GetVersion() >= 8)
    {
        SetConstMemberRaw(penv->GetSC(), "kerning", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
        SetConstMemberRaw(penv->GetSC(), "letterSpacing", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    }
    if (penv->CheckExtensions())
    {
        SetConstMemberRaw(penv->GetSC(), "alpha", GASValue(GASValue::NULLTYPE), GASPropFlags::PropFlag_DontDelete);
    }
}

bool GASTextFormatObject::SetMember(GASEnvironment *penv, const GASString& name, 
                                    const GASValue& val, const GASPropFlags& flags)
{
    GASValue valset(val);
    if (name == "align")
    {
        GASString strval = val.ToString(penv);
        if (strval == "left")
        {
            ParagraphFormat.SetAlignment(GFxTextParagraphFormat::Align_Left);
        }
        else if (strval == "right")
        {
            ParagraphFormat.SetAlignment(GFxTextParagraphFormat::Align_Right);
        }
        else if (strval == "center")
        {
            ParagraphFormat.SetAlignment(GFxTextParagraphFormat::Align_Center);
        }
        else if (strval == "justify")
        {
            ParagraphFormat.SetAlignment(GFxTextParagraphFormat::Align_Justify);
        }
        else
        {
            ParagraphFormat.ClearAlignment();
            valset.SetNull();
        }
    }
    else if (name == "blockIndent")
    {
        if (!val.IsNull() && !val.IsUndefined())
        {
            SInt32 v = val.ToInt32(penv);
            valset.SetNumber((GASNumber)v);
            if (v < 0) v = 0;
            else if (v > 720) v = 720;
            ParagraphFormat.SetBlockIndent((UInt)v);
        }
        else
        {
            ParagraphFormat.ClearBlockIndent();
            valset.SetNull();
        }
    }
    else if (name == "bold")
    {
        if (!val.IsNull() && !val.IsUndefined())
        {
            bool v = val.ToBool(penv);
            valset.SetBool(v);
            TextFormat.SetBold(v);
        }
        else
        {
            TextFormat.ClearBold();
            valset.SetNull();
        }
    }
    else if (name == "bullet")
    {
        if (!val.IsNull() && !val.IsUndefined())
        {
            bool v = val.ToBool(penv);
            valset.SetBool(v);
            ParagraphFormat.SetBullet(v);
        }
        else
        {
            ParagraphFormat.ClearBullet();
            valset.SetNull();
        }
    }
    else if (name == "color")
    {
        if (!val.IsNull() && !val.IsUndefined())
        {
            UInt v = (UInt)val.ToInt32(penv);
            valset.SetNumber((GASNumber)v);
            TextFormat.SetColor32(v);
        }
        else
        {
            TextFormat.ClearColor();
            valset.SetNull();
        }
    }
    else if (name == "font")
    {
        if (!val.IsNull() && !val.IsUndefined())
        {
            GASString v = val.ToString(penv);
            valset.SetString(v);
            TextFormat.SetFontList(v.ToCStr());
        }
        else
        {
            TextFormat.ClearFontList();
            valset.SetNull();
        }
    }
    else if (name == "indent")
    {
        if (!val.IsNull() && !val.IsUndefined())
        {
            SInt32 v = val.ToInt32(penv);
            valset.SetNumber((GASNumber)v);
            if (v < -720) v = -720;
            else if (v > 720) v = 720;
            ParagraphFormat.SetIndent((SInt)v);
        }
        else
        {
            ParagraphFormat.ClearIndent();
            valset.SetNull();
        }
    }
    else if (name == "italic")
    {
        if (!val.IsNull() && !val.IsUndefined())
        {
            bool v = val.ToBool(penv);
            valset.SetBool(v);
            TextFormat.SetItalic(v);
        }
        else
        {
            TextFormat.ClearItalic();
            valset.SetNull();
        }
    }
    else if (name == "leading")
    {
        if (!val.IsNull() && !val.IsUndefined())
        {
            SInt32 v = val.ToInt32(penv);
            valset.SetNumber((GASNumber)v);
            if (v < -720) v = -720;
            else if (v > 720) v = 720;
            ParagraphFormat.SetLeading((SInt)v);
        }
        else
        {
            ParagraphFormat.ClearLeading();
            valset.SetNull();
        }
    }
    else if (name == "leftMargin")
    {
        if (!val.IsNull() && !val.IsUndefined())
        {
            SInt32 v = val.ToInt32(penv);
            valset.SetNumber((GASNumber)v);
            if (v < 0) v = 0;
            else if (v > 720) v = 720;
            ParagraphFormat.SetLeftMargin((UInt)v);
        }
        else
        {
            ParagraphFormat.ClearLeftMargin();
            valset.SetNull();
        }
    }
    else if (name == "rightMargin")
    {
        if (!val.IsNull() && !val.IsUndefined())
        {
            SInt32 v = val.ToInt32(penv);
            valset.SetNumber((GASNumber)v);
            if (v < 0) v = 0;
            else if (v > 720) v = 720;
            ParagraphFormat.SetRightMargin((UInt)v);
        }
        else
        {
            ParagraphFormat.ClearRightMargin();
            valset.SetNull();
        }
    }
    else if (name == "size")
    {
        if (!val.IsNull() && !val.IsUndefined())
        {
            SInt32 v = val.ToInt32(penv);
            valset.SetNumber((GASNumber)v);
            if (v >= 0)
            {
                if (v < 128)
                    TextFormat.SetFontSize((Float)(UInt)v);
                else
                    TextFormat.SetFontSize(127);
            }
        }
        else
        {
            TextFormat.ClearFontSize();
            valset.SetNull();
        }
    }
    else if (name == "tabStops")
    {
        if (!val.IsNull() && !val.IsUndefined())
        {
            if (val.IsObject() && val.ToObject(penv)->GetObjectType() == GASObject::Object_Array)
            {
                GASArrayObject* parr = static_cast<GASArrayObject*>(val.ToObject(penv));
                ParagraphFormat.SetTabStopsNum((UInt)parr->GetSize());
                for(int i = 0, n = parr->GetSize(); i < n; ++i)
                {
                    GASNumber v = parr->GetElementPtr(i)->ToNumber(penv);
                    parr->SetElement(i, GASNumber((UInt)v));
                    ParagraphFormat.SetTabStopsElement((UInt)i, (UInt)v);
                }
                return GASObject::SetMember(penv, name, GASValue(parr), flags);
            }
        }
        else
        {
            ParagraphFormat.ClearTabStops();
            valset.SetNull();
        }
    }
    else if (name == "underline")
    {
        if (!val.IsNull() && !val.IsUndefined())
        {
            bool v = val.ToBool(penv);
            valset.SetBool(v);
            TextFormat.SetUnderline(v);
        }
        else
        {
            TextFormat.ClearUnderline();
            valset.SetNull();
        }
    }
    else if (name == "url")
    {
        if (!val.IsNull() && !val.IsUndefined())
        {
            GASString v = val.ToString(penv);
            valset.SetString(v);
            TextFormat.SetUrl(v.ToCStr());
        }
        else
        {
            TextFormat.ClearUrl();
            valset.SetNull();
        }
    }
//     else if (name == "target")
//     {
//     }
    else
    {
        if (penv->GetVersion() >= 8)
        {
            if (name == "letterSpacing")
            {
                if (!val.IsNull() && !val.IsUndefined())
                {
                    SInt32 v = val.ToInt32(penv);
                    valset.SetNumber((GASNumber)v);
                    if (v < -720) v = -720;
                    else if (v > 720) v = 720;
                    TextFormat.SetLetterSpacing(Float(v));
                }
                else
                {
                    TextFormat.ClearLetterSpacing();
                    valset.SetNull();
                }
            }
            else if (name == "kerning")
            {
                if (!val.IsNull() && !val.IsUndefined())
                {
                    bool v = val.ToBool(penv);
                    valset.SetBool(v);
                    TextFormat.SetKerning(v);
                }
                else
                {
                    TextFormat.ClearKerning();
                    valset.SetNull();
                }
            }
        }
        if (penv->CheckExtensions())
        {
            if (name == "alpha")
            {
                if (!val.IsNull() && !val.IsUndefined())
                {
                    SInt32 v = val.ToInt32(penv);
                    valset.SetNumber((GASNumber)v);
                    if (v < 0) v = 0;
                    else if (v > 100) v = 100;
                    TextFormat.SetAlpha(UInt8(v * 255. / 100.));
                }
                else
                {
                    TextFormat.ClearAlpha();
                    valset.SetNull();
                }
            }
        }
    }
    return GASObject::SetMember(penv, name, valset, flags);
}

void GASTextFormatObject::SetTextFormat(GASStringContext* psc, const GFxTextFormat& textFmt) 
{ 
    TextFormat = textFmt; 
    GASValue nullVal(GASValue::NULLTYPE);
    SetConstMemberRaw(psc, "bold", (textFmt.IsBoldSet())?GASValue(textFmt.IsBold()):nullVal);
    SetConstMemberRaw(psc, "italic", (textFmt.IsItalicSet())?GASValue(textFmt.IsItalic()):nullVal);
    SetConstMemberRaw(psc, "underline", (textFmt.IsUnderlineSet())?GASValue(textFmt.IsUnderline()):nullVal);
    SetConstMemberRaw(psc, "size", (textFmt.IsFontSizeSet())?GASValue(GASNumber(textFmt.GetFontSize())):nullVal);
    SetConstMemberRaw(psc, "font", (textFmt.IsFontListSet())?GASValue(psc->CreateString(textFmt.GetFontList())):nullVal);
    SetConstMemberRaw(psc, "color", (textFmt.IsColorSet())?GASValue(GASNumber(textFmt.GetColor32() & 0xFFFFFFu)):nullVal);
    SetConstMemberRaw(psc, "letterSpacing", (textFmt.IsLetterSpacingSet())?GASValue(GASNumber(textFmt.GetLetterSpacing())):nullVal);
    SetConstMemberRaw(psc, "kerning", (textFmt.IsKerningSet())?GASValue(textFmt.IsKerning()):nullVal);
    SetConstMemberRaw(psc, "url", (textFmt.IsUrlSet())?GASValue(psc->CreateString(textFmt.GetUrl())):nullVal);
    if (psc->pContext->CheckExtensions())
    {
        SetConstMemberRaw(psc, "alpha", (textFmt.IsColorSet())?GASValue(GASNumber(textFmt.GetAlpha() * 100. / 255.)):nullVal);
    }
}

void GASTextFormatObject::SetParagraphFormat(GASStringContext* psc, const GFxTextParagraphFormat& paraFmt) 
{ 
    ParagraphFormat = paraFmt; 
    GASValue nullVal(GASValue::NULLTYPE);
    if (paraFmt.IsAlignmentSet())
    {
        const char* newAlign = "left";
        switch(paraFmt.GetAlignment())
        {
        case GFxTextParagraphFormat::Align_Center:  newAlign = "center"; break;
        case GFxTextParagraphFormat::Align_Right:   newAlign = "right"; break;
        case GFxTextParagraphFormat::Align_Justify: newAlign = "justify"; break;
        default: break;
        }
        SetConstMemberRaw(psc, "align", GASValue(psc->CreateString(newAlign)));
    }
    else
        SetConstMemberRaw(psc, "align", nullVal);

    SetConstMemberRaw(psc, "bullet", (paraFmt.IsBulletSet())?GASValue(paraFmt.IsBullet()):nullVal);
    SetConstMemberRaw(psc, "blockIndent", (paraFmt.IsBlockIndentSet())?GASValue(GASNumber(paraFmt.GetBlockIndent())):nullVal);
    SetConstMemberRaw(psc, "indent", (paraFmt.IsIndentSet())?GASValue(GASNumber(paraFmt.GetIndent())):nullVal);
    SetConstMemberRaw(psc, "leading", (paraFmt.IsLeadingSet())?GASValue(GASNumber(paraFmt.GetLeading())):nullVal);
    SetConstMemberRaw(psc, "leftMargin", (paraFmt.IsLeftMarginSet())?GASValue(GASNumber(paraFmt.GetLeftMargin())):nullVal);
    SetConstMemberRaw(psc, "rightMargin", (paraFmt.IsRightMarginSet())?GASValue(GASNumber(paraFmt.GetRightMargin())):nullVal);
    if (paraFmt.IsTabStopsSet())
    {
        UInt num = 0;
        const UInt* ptabStops = paraFmt.GetTabStops(&num);
        GASSERT(num > 0 && ptabStops);
        GPtr<GASArrayObject> parr = *GHEAP_NEW(psc->GetHeap()) GASArrayObject(psc);
        parr->Resize((int)num);
        for (UInt i = 0; i < num; i++)
            parr->SetElement(i, GASValue((GASNumber)ptabStops[i]));
        SetConstMemberRaw(psc, "tabStops", GASValue(parr));
    }
    else 
        SetConstMemberRaw(psc, "tabStops", nullVal);
}

////////////////////////////////////////////////
//
static const GASNameFunction GAS_TextFormatFunctionTable[] = 
{
    { "getTextExtent", GASTextFormatProto::GetTextExtent },
    { 0, 0 }
};

GASTextFormatProto::GASTextFormatProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASTextFormatObject>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GAS_TextFormatFunctionTable);
}

void GASTextFormatProto::GetTextExtent(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    if (fn.NArgs == 0)
        return; // expecting at least one param - string
    CHECK_THIS_PTR(fn, TextFormat);
    GASTextFormatObject* pfmtObj = static_cast<GASTextFormatObject*>(fn.ThisPtr);
    GPtr<GFxASCharacter> ptarget = fn.Env->GetTarget();
    if (ptarget)
    {
        GPtr<GASObject> pobj = *GHEAP_NEW(fn.Env->GetHeap()) GASObject(fn.Env);
        GASString str = fn.Arg(0).ToString(fn.Env);

        GPtr<GFxTextDocView> pdocument = 
            *GHEAP_NEW(fn.Env->GetHeap()) GFxTextDocView(fn.Env->GetMovieRoot()->GetTextAllocator(), 
                                                         ptarget->GetFontManager(), NULL);
        pdocument->GetStyledText()->SetNewLine0D(); // Flash uses '\x0D' ('\r', #13) as a new-line char
        pdocument->SetAutoSizeX();
        pdocument->SetAutoSizeY();
        if (fn.Env->GetVersion() >= 7)
        {
            if (fn.NArgs >= 2)
            {
                GASNumber width = fn.Arg(1).ToNumber(fn.Env);
                pdocument->ClearAutoSizeX();
                pdocument->SetWordWrap();
                GRectF vr(0, 0, GSizeF(PixelsToTwips((Float)width), 0));
                pdocument->SetViewRect(vr);
            }
        }
        pdocument->SetMultiline();

        GFxTextFormat eTextFmt(fn.Env->GetHeap());
        GFxTextParagraphFormat eParagraphFmt;
        eTextFmt.InitByDefaultValues();
        eParagraphFmt.InitByDefaultValues();
        eTextFmt      = eTextFmt.Merge(pfmtObj->TextFormat);
        eParagraphFmt = eParagraphFmt.Merge(pfmtObj->ParagraphFormat);

        pdocument->SetDefaultTextFormat(eTextFmt);
        pdocument->SetDefaultParagraphFormat(eParagraphFmt);
        pdocument->SetText(str.ToCStr());
        pdocument->Format();

        pobj->SetConstMemberRaw(fn.Env->GetSC(), "textFieldWidth", 
            GASValue((GASNumber)TwipsToPixels((Double)pdocument->GetTextWidth() + GFX_TEXT_GUTTER*2)));
        pobj->SetConstMemberRaw(fn.Env->GetSC(), "textFieldHeight", 
            GASValue((GASNumber)TwipsToPixels((Double)pdocument->GetTextHeight() + GFX_TEXT_GUTTER*2)));
        pobj->SetConstMemberRaw(fn.Env->GetSC(), "width", 
            GASValue((GASNumber)TwipsToPixels((Double)pdocument->GetTextWidth())));
        pobj->SetConstMemberRaw(fn.Env->GetSC(), "height", 
            GASValue((GASNumber)TwipsToPixels((Double)pdocument->GetTextHeight())));

        // now determine the ascent and descent of used font
        GPtr<GFxFontHandle> pfontHandle = *pdocument->GetFontManager()->CreateFontHandle(
            eTextFmt.GetFontList(), 
            eTextFmt.IsBold(), 
            eTextFmt.IsItalic(),
            true);
        Double ascent = 0, descent = 0;
        if (pfontHandle)
        {
            ascent  = pfontHandle->GetFont()->GetAscent();
            descent = pfontHandle->GetFont()->GetDescent();
        }
        if (ascent == 0)
            ascent = 960.f;
        if (descent == 0)
            descent = 1024.f - ascent;
        Double scale = PixelsToTwips((Double)eTextFmt.GetFontSize()) / 1024.0f; // the EM square is 1024 x 1024  
        UInt uascent  = (UInt)TwipsToPixels(ascent * scale);
        UInt udescent = (UInt)TwipsToPixels(descent * scale);
        pobj->SetConstMemberRaw(fn.Env->GetSC(), "ascent", GASValue(GASNumber(uascent)));
        pobj->SetConstMemberRaw(fn.Env->GetSC(), "descent", GASValue(GASNumber(udescent)));
        fn.Result->SetAsObject(pobj);
    }
}

////////////////////////////////////////////////
//
GASTextFormatCtorFunction::GASTextFormatCtorFunction(GASStringContext *psc) :
    GASCFunctionObject(psc, GlobalCtor)
{
    GUNUSED(psc);
}

void GASTextFormatCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASTextFormatObject> ab;
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextFormat && !fn.ThisPtr->IsBuiltinPrototype())
        ab = static_cast<GASTextFormatObject*>(fn.ThisPtr);
    else
        ab = *GHEAP_NEW(fn.Env->GetHeap()) GASTextFormatObject(fn.Env);
    
    if (fn.NArgs >= 1)
    {
        GASStringContext* psc = fn.Env->GetSC();
        ab->SetMember(fn.Env, psc->CreateConstString("font"), fn.Arg(0));
        if (fn.NArgs >= 2)
        {
            ab->SetMember(fn.Env, psc->CreateConstString("size"), fn.Arg(1));
            if (fn.NArgs >= 3)
            {
                ab->SetMember(fn.Env, psc->CreateConstString("color"), fn.Arg(2));
                if (fn.NArgs >= 4)
                {
                    ab->SetMember(fn.Env, psc->CreateConstString("bold"), fn.Arg(3));
                    if (fn.NArgs >= 5)
                    {
                        ab->SetMember(fn.Env, psc->CreateConstString("italic"), fn.Arg(4));
                        if (fn.NArgs >= 6)
                        {
                            ab->SetMember(fn.Env, psc->CreateConstString("underline"), fn.Arg(5));
                            if (fn.NArgs >= 7)
                            {
                                ab->SetMember(fn.Env, psc->CreateConstString("url"), fn.Arg(6));
                                if (fn.NArgs >= 8)
                                {
                                    ab->SetMember(fn.Env, psc->CreateConstString("target"), fn.Arg(7));
                                    if (fn.NArgs >= 9)
                                    {
                                        ab->SetMember(fn.Env, psc->CreateConstString("align"), fn.Arg(8));
                                        if (fn.NArgs >= 10)
                                        {
                                            ab->SetMember(fn.Env, psc->CreateConstString("leftMargin"), fn.Arg(9));
                                            if (fn.NArgs >= 11)
                                            {
                                                ab->SetMember(fn.Env, psc->CreateConstString("rightMargin"), fn.Arg(10));
                                                if (fn.NArgs >= 12)
                                                {
                                                    ab->SetMember(fn.Env, psc->CreateConstString("indent"), fn.Arg(11));
                                                    if (fn.NArgs >= 13)
                                                    {
                                                        ab->SetMember(fn.Env, psc->CreateConstString("leading"), fn.Arg(12));
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
    }
    fn.Result->SetAsObject(ab.GetPtr());
}

GASObject*  GASTextFormatCtorFunction::CreateNewObject(GASEnvironment* penv) const
{
    return GHEAP_NEW(penv->GetHeap()) GASTextFormatObject(penv);
}

GASFunctionRef GASTextFormatCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef  ctor(*GHEAP_NEW(pgc->GetHeap()) GASTextFormatCtorFunction(&sc));
    GPtr<GASObject> proto = *GHEAP_NEW(pgc->GetHeap()) GASTextFormatProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_TextFormat, proto);
    pgc->pGlobal->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_TextFormat), GASValue(ctor));
    return ctor;
}
#endif //#ifndef GFC_NO_FXPLAYER_AS_TEXTFORMAT
