/**********************************************************************

Filename    :   GFxTextField.cpp
Content     :   Dynamic and input text field character implementation
Created     :   May, 2007
Authors     :   Artem Bolgar

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#include "GUTF8Util.h"

#include "GRenderer.h"
#include "GAutoPtr.h"
#include "GFxCharacter.h"
#include "GFxShape.h"
#include "GFxStream.h"
#include "GFxLog.h"
#include "GFxFontResource.h"
#include "GFxText.h"
#include "AS/GASTextFormat.h"
#include "AS/GASAsBroadcaster.h"

#ifndef GFC_NO_CSS_SUPPORT
#include "AS/GASStyleSheet.h"
#else
class GFxTextStyleManager;
#endif

#include "AS/GASNumberObject.h"
#include "AS/GASRectangleObject.h"
#include "AS/GASMouse.h"

// For Root()->GetRenderer
#include "GFxPlayerImpl.h"
#include "GFxSprite.h"

#include "GFxDisplayContext.h"
#include "GFxLoadProcess.h"

// For image substitution
#include "AS/GASBitmapData.h"
#include "GFxShape.h"
#include "AS/GASArrayObject.h"
#include <GFxIMEManager.h>

#ifndef GFC_NO_FXPLAYER_AS_FILTERS
#include "AS/GASBitmapFilter.h"
#endif  // GFC_NO_FXPLAYER_AS_FILTERS

#include "GRange.h"

//
// GFxEditTextCharacterDef
//
GFxEditTextCharacterDef::GFxEditTextCharacterDef()
:
    FontId(0),
    TextHeight(1.0f),
    MaxLength(0),
    LeftMargin(0.0f),
    RightMargin(0.0f),
    Indent(0.0f),
    Leading(0.0f),
    Flags(0),
    Alignment(ALIGN_LEFT)
{
    Color.SetColor(0, 0, 0, 255);
    TextRect.Clear();
}
GFxEditTextCharacterDef::~GFxEditTextCharacterDef()
{
}

void    GFxEditTextCharacterDef::Read(GFxLoadProcess* p, GFxTagType tagType)
{
    GUNUSED(tagType);        
    GASSERT(tagType == GFxTag_DefineEditText);
    GFxStream* in = p->GetStream();

    in->ReadRect(&TextRect);

    in->LogParse("  TextRect = { l: %f, t: %f, r: %f, b: %f }\n", 
        (float)TextRect.Left, (float)TextRect.Top, (float)TextRect.Right, (float)TextRect.Bottom);

    in->Align();
    bool    hasText = in->ReadUInt(1) ? true : false;
    SetWordWrap(in->ReadUInt(1) != 0);
    SetMultiline(in->ReadUInt(1) != 0);
    SetPassword(in->ReadUInt(1) != 0);
    SetReadOnly(in->ReadUInt(1) != 0);

    in->LogParse("  WordWrap = %d, Multiline = %d, Password = %d, ReadOnly = %d\n", 
        (int)IsWordWrap(), (int)IsMultiline(), (int)IsPassword(), (int)IsReadOnly());

    bool    hasColor = in->ReadUInt(1) ? true : false;
    bool    hasMaxLength = in->ReadUInt(1) ? true : false;
    bool    hasFont = in->ReadUInt(1) ? true : false;

    in->ReadUInt(1);    // reserved
    SetAutoSize(in->ReadUInt(1) != 0);
    bool    hasLayout = in->ReadUInt(1) ? true : false;
    SetSelectable(in->ReadUInt(1) == 0);
    SetBorder(in->ReadUInt(1) != 0);
    in->ReadUInt(1);    // reserved

    // In SWF 8 text is *ALWAYS* marked as HTML.
    // Correction, AB: no, only if kerning is ON
    // In SWF <= 7, that is not the case.
    SetHtml(in->ReadUInt(1) != 0);
    SetUseDeviceFont(in->ReadUInt(1) == 0);

    in->LogParse("  AutoSize = %d, Selectable = %d, Border = %d, Html = %d, UseDeviceFont = %d\n", 
        (int)IsAutoSize(), (int)IsSelectable(), (int)IsBorder(), (int)IsHtml(), (int)DoesUseDeviceFont());

    if (hasFont)
    {
        FontId = GFxResourceId(in->ReadU16());
        TextHeight = (Float) in->ReadU16();
        in->LogParse("  HasFont: font id = %d\n", FontId.GetIdIndex());

        // Create a font resource handle.
        GFxResourceHandle hres;
        p->GetResourceHandle(&hres, FontId);
        pFont.SetFromHandle(hres);
    }

    if (hasColor)
    {               
        in->ReadRgba(&Color);
        in->LogParse("  HasColor\n");
    }

    if (hasMaxLength)
    {
        MaxLength = in->ReadU16();
        in->LogParse("  HasMaxLength: len = %d\n", MaxLength);
    }

    if (hasLayout)
    {
        SetHasLayout();
        Alignment = (alignment) in->ReadU8();
        LeftMargin = (Float) in->ReadU16();
        RightMargin = (Float) in->ReadU16();
        Indent = (Float) in->ReadS16();
        Leading = (Float) in->ReadS16();
        in->LogParse("  HasLayout: alignment = %d, leftmarg = %f, rightmarg = %f, indent = %f, leading = %f\n", 
            (int)Alignment, LeftMargin, RightMargin, Indent, Leading);
    }

    in->ReadString(&VariableName);
    if (hasText)    
        in->ReadString(&DefaultText);    
    
    in->LogParse("EditTextChar, varname = %s, text = %s\n",
        VariableName.ToCStr(), DefaultText.ToCStr());
}

void GFxEditTextCharacterDef::InitEmptyTextDef()
{
    SetEmptyTextDef();

    Color = GColor((UByte)0, (UByte)0, (UByte)0, (UByte)255);
    TextHeight = PixelsToTwips(12.f);
    SetReadOnly();
    SetSelectable();
    SetUseDeviceFont();
    SetAAForReadability();
    // font name?
}

//
// GFxEditTextCharacter
//
class GFxEditTextCharacter : public GASTextField
{
    friend class GASTextFieldProto;

    GFxEditTextCharacterDef*    pDef;
    GPtr<GFxTextDocView>        pDocument;
    GFxResourceBinding*         pBinding; //?AB: is there any other way to get GFxFontResource*?

    enum
    {
        Flags_AutoSize      = 0x1,
        Flags_Html          = 0x2,
        Flags_Password      = 0x4,
        Flags_NoTranslate   = 0x8,
        Flags_CondenseWhite = 0x10,
        Flags_HandCursor    = 0x20,
        Flags_NextFrame     = 0x40,
        Flags_MouseWheelEnabled     = 0x80,
        Flags_UseRichClipboard      = 0x100,
        Flags_AlwaysShowSelection   = 0x200,
        Flags_NoAutoSelection       = 0x400,
        Flags_IMEDisabled           = 0x800,
        Flags_OriginalIsHtml        = 0x1000,
        Flags_NeedUpdateGeomData    = 0x2000,
        Flags_ForceAdvance          = 0x4000
    };

    GFxEditTextCharacterDef::alignment  Alignment;
    GColor      BackgroundColor;
    GColor      BorderColor;

    GASString   VariableName;

    GASValue    VariableVal;
    UInt16      Flags;

    GStringLH OriginalTextValue;

#ifndef GFC_NO_CSS_SUPPORT
public: // for Wii compiler 4.3 145
    struct CSSHolder : public GNewOverrideBase<GFxStatMV_ActionScript_Mem>
    {
        GPtr<GASStyleSheetObject>   pASStyleSheet; // an AS object, pASStyleSheet->CSS is the data
        struct UrlZone
        {
            GPtr<GFxStyledText>     SavedFmt;
            UInt                    HitCount;   // hit counter
            UInt                    OverCount;  // over counter

            UrlZone():HitCount(0),OverCount(0) {}

            bool operator==(const UrlZone& r) const 
            { 
                return SavedFmt == r.SavedFmt && HitCount == r.HitCount && OverCount == r.OverCount; 
            }
            bool operator!=(const UrlZone& r) const { return !operator==(r); }
        };
        GRangeDataArray<UrlZone>    UrlZones;
        struct MouseStateType 
        {
            UInt UrlZoneIndex;
            bool OverBit;
            bool HitBit;

            MouseStateType() : UrlZoneIndex(0), OverBit(false), HitBit(false) {}
        }                           MouseState[GFC_MAX_MICE_SUPPORTED];
    };
private:
    GAutoPtr<CSSHolder> pCSSData;
#endif //GFC_NO_CSS_SUPPORT

    inline void SetDirtyFlag()
    {
        GFxMovieRoot* proot = GetMovieRoot();
        if (proot)
            proot->SetDirtyFlag();
    }

    void SetAutoSize(bool v = true) { (v) ? Flags |= Flags_AutoSize : Flags &= (~Flags_AutoSize); }
    void ClearAutoSize()            { SetAutoSize(false); }
    bool IsAutoSize() const         { return (Flags & Flags_AutoSize) != 0; }

    void SetHtml(bool v = true) { (v) ? Flags |= Flags_Html : Flags &= (~Flags_Html); }
    void ClearHtml()            { SetHtml(false); }
    bool IsHtml() const         { return (Flags & Flags_Html) != 0; }

    void SetOriginalHtml(bool v = true) { (v) ? Flags |= Flags_OriginalIsHtml : Flags &= (~Flags_OriginalIsHtml); }
    void ClearOriginalHtml()            { SetOriginalHtml(false); }
    bool IsOriginalHtml() const         { return (Flags & Flags_OriginalIsHtml) != 0; }

    void SetPassword(bool v = true) { (v) ? Flags |= Flags_Password : Flags &= (~Flags_Password); }
    void ClearPassword()            { SetPassword(false); }
    bool IsPassword() const         { return (Flags & Flags_Password) != 0; }

    void SetCondenseWhite(bool v = true) { (v) ? Flags |= Flags_CondenseWhite : Flags &= (~Flags_CondenseWhite); }
    void ClearCondenseWhite()            { SetCondenseWhite(false); }
    bool IsCondenseWhite() const         { return (Flags & Flags_CondenseWhite) != 0; }

    void SetHandCursor(bool v = true) { (v) ? Flags |= Flags_HandCursor : Flags &= (~Flags_HandCursor); }
    void ClearHandCursor()            { SetHandCursor(false); }
    bool IsHandCursor() const         { return (Flags & Flags_HandCursor) != 0; }

    void SetMouseWheelEnabled(bool v = true) { (v) ? Flags |= Flags_MouseWheelEnabled : Flags &= (~Flags_MouseWheelEnabled); }
    void ClearMouseWheelEnabled()            { SetMouseWheelEnabled(false); }
    bool IsMouseWheelEnabled() const         { return (Flags & Flags_MouseWheelEnabled) != 0; }

    void SetSelectable(bool v = true) 
    { 
#ifndef GFC_NO_TEXT_INPUT_SUPPORT
        GASSERT(pDocument);
        GFxTextEditorKit* peditor = pDocument->GetEditorKit();
        if (v)
        {
            peditor = CreateEditorKit();
            peditor->SetSelectable();
        }
        else
        {
            if (peditor)
                peditor->ClearSelectable();
        }
#else
        GUNUSED(v);
#endif //GFC_NO_TEXT_INPUT_SUPPORT
    }
    void ClearSelectable()            { SetSelectable(false); }
    bool IsSelectable() const         { return (pDocument->GetEditorKit()) ? pDocument->GetEditorKit()->IsSelectable() : pDef->IsSelectable(); }

    void SetNoTranslate(bool v = true) { (v) ? Flags |= Flags_NoTranslate : Flags &= (~Flags_NoTranslate); }
    void ClearNoTranslate()            { SetNoTranslate(false); }
    bool IsNoTranslate() const         { return (Flags & Flags_NoTranslate) != 0; }

    bool IsReadOnly() const { return (pDocument->GetEditorKit()) ? pDocument->GetEditorKit()->IsReadOnly() : pDef->IsReadOnly(); }

    void SetUseRichClipboard(bool v = true) { (v) ? Flags |= Flags_UseRichClipboard : Flags &= (~Flags_UseRichClipboard); }
    void ClearUseRichClipboard()            { SetUseRichClipboard(false); }
    bool DoesUseRichClipboard() const       { return (Flags & Flags_UseRichClipboard) != 0; }

    void SetAlwaysShowSelection(bool v = true) { (v) ? Flags |= Flags_AlwaysShowSelection : Flags &= (~Flags_AlwaysShowSelection); }
    void ClearAlwaysShowSelection()            { SetAlwaysShowSelection(false); }
    bool DoesAlwaysShowSelection() const       { return (Flags & Flags_AlwaysShowSelection) != 0; }

    void SetNoAutoSelection(bool v = true) { (v) ? Flags |= Flags_NoAutoSelection : Flags &= (~Flags_NoAutoSelection); }
    void ClearNoAutoSelection()            { SetNoAutoSelection(false); }
    bool IsNoAutoSelection() const         { return (Flags & Flags_NoAutoSelection) != 0; }

    void SetIMEDisabledFlag(bool v = true) { (v) ? Flags |= Flags_IMEDisabled : Flags &= (~Flags_IMEDisabled); }
    void ClearIMEDisabledFlag()            { SetIMEDisabledFlag(false); }
    bool IsIMEDisabledFlag() const         { return (Flags & Flags_IMEDisabled) != 0; }

    void SetForceAdvance(bool v = true) { (v) ? Flags |= Flags_ForceAdvance : Flags &= (~Flags_ForceAdvance); }
    void ClearForceAdvance()            { SetForceAdvance(false); }
    bool IsForceAdvance() const         { return (Flags & Flags_ForceAdvance) != 0; }

    GFxTextEditorKit* CreateEditorKit()
    {
#ifndef GFC_NO_TEXT_INPUT_SUPPORT
        GASSERT(pDocument);
        GFxTextEditorKit* peditor = pDocument->GetEditorKit();
        if (peditor == NULL)
        {
            peditor = pDocument->CreateEditorKit();
            if (pDef->IsReadOnly())
                peditor->SetReadOnly();
            if (pDef->IsSelectable())
                peditor->SetSelectable();

            GPtr<GFxTextClipboard> pclipBoard = GetMovieRoot()->GetStateBagImpl()->GetTextClipboard();
            peditor->SetClipboard(pclipBoard);
            GPtr<GFxTextKeyMap> pkeymap = GetMovieRoot()->GetStateBagImpl()->GetTextKeyMap();
            peditor->SetKeyMap(pkeymap);

            if (DoesUseRichClipboard())
                peditor->SetUseRichClipboard();
            else
                peditor->ClearUseRichClipboard();
        }
        return peditor;
#else
        return NULL;
#endif
    }

    class TextDocumentListener : public GFxTextDocView::DocumentListener
    {
        GFxEditTextCharacter* GetTextField()
        {
            return (GFxEditTextCharacter*)(((UByte*)this) - 
                   ((UByte*)&(((GFxEditTextCharacter*)this)->TextDocListener) - 
                    (UByte*)((GFxEditTextCharacter*)this)) );
        }
    public:
        TextDocumentListener() : 
          DocumentListener(Mask_All & (~Mask_OnLineFormat)) 
        {
            GPtr<GFxTranslator> ptrans = GetTextField()->GetMovieRoot()->GetTranslator();
            if (ptrans)
            {
                if (ptrans->HandlesCustomWordWrapping())
                {
                   HandlersMask |= Mask_OnLineFormat;
                }
            }
        }

        void TranslatorChanged()
        {
            GPtr<GFxTranslator> ptrans = GetTextField()->GetMovieRoot()->GetTranslator();
            if (ptrans && ptrans->HandlesCustomWordWrapping())
                HandlersMask |= Mask_OnLineFormat;
            else
                HandlersMask &= (~Mask_OnLineFormat);
        }

        static void BroadcastMessage(const GASFnCall& fn)
        {
            GASString eventName(fn.Arg(0).ToString(fn.Env));
            GASAsBroadcaster::BroadcastMessage(fn.Env, fn.ThisPtr, eventName, fn.NArgs - 1, fn.Env->GetTopIndex() - 1);
        }
        void OnScroll(GFxTextDocView&)
        {
            GFxEditTextCharacter* ptextField = GetTextField();
            GASEnvironment* penv    = ptextField->GetASEnvironment();
            GFxMovieRoot::ActionEntry ae;
            ae.SetAction(ptextField, BroadcastMessage, NULL);
            if (!penv->GetMovieRoot()->HasActionEntry(ae))
            {
                GASValueArray params;
                params.PushBack(GASValue(penv->CreateConstString("onScroller")));
                params.PushBack(GASValue(ptextField));
                penv->GetMovieRoot()->InsertEmptyAction()->SetAction(ptextField, BroadcastMessage, &params);
            }
        }

        // events
        virtual void View_OnHScroll(GFxTextDocView& view, UInt newScroll)
        {
            GUNUSED(newScroll);
            OnScroll(view);
        }
        virtual void View_OnVScroll(GFxTextDocView& view, UInt newScroll)
        {
            GUNUSED(newScroll);
            OnScroll(view);
        }
        virtual void View_OnMaxScrollChanged(GFxTextDocView& view)
        {
            OnScroll(view);
        }
        virtual bool View_OnLineFormat(GFxTextDocView&, GFxTextDocView::LineFormatDesc& desc) 
        { 
            GPtr<GFxTranslator> ptrans = GetTextField()->GetMovieRoot()->GetTranslator();
            if (ptrans)
            {
                GFxTranslator::LineFormatDesc tdesc;
                tdesc.CurrentLineWidth = TwipsToPixels(desc.CurrentLineWidth);
                tdesc.DashSymbolWidth  = TwipsToPixels(desc.DashSymbolWidth);
                tdesc.LineStartPos     = desc.LineStartPos;
                tdesc.LineWidthBeforeWordWrap = TwipsToPixels(desc.LineWidthBeforeWordWrap);
                tdesc.NumCharsInLine   = desc.NumCharsInLine;
                tdesc.pWidths          = desc.pWidths;
                tdesc.pParaText        = desc.pParaText;
                tdesc.ParaTextLen      = desc.ParaTextLen;
                tdesc.ProposedWordWrapPoint = desc.ProposedWordWrapPoint;
                tdesc.VisibleRectWidth = TwipsToPixels(desc.VisibleRectWidth);
                tdesc.UseHyphenation   = desc.UseHyphenation;
                tdesc.Alignment        = desc.Alignment;
                // it is ok to modify values in the desc.pCharWidths since
                // they are not used by further formatting logic.
                for (UPInt i = 0; i < tdesc.NumCharsInLine; ++i)
                    desc.pWidths[i] = TwipsToPixels(desc.pWidths[i]);
                if (ptrans->OnWordWrapping(&tdesc))
                {
                    desc.ProposedWordWrapPoint = tdesc.ProposedWordWrapPoint;
                    desc.UseHyphenation        = tdesc.UseHyphenation;
                    return true;
                }
            }
            return false; 
        }
        virtual void    View_OnChanged(GFxTextDocView&)    
        {
            GFxEditTextCharacter* peditChar = GetTextField();
            peditChar->SetDirtyFlag();
        }
        // editor events
        virtual void    Editor_OnChanged(GFxTextEditorKit& editor) 
        { 
            GUNUSED(editor); 
            GFxEditTextCharacter* peditChar = GetTextField();
            peditChar->UpdateVariable();
            peditChar->NotifyChanged();
        }
        virtual GString GetCharacterPath()
        {
            GString ret;
            GetTextField()->GetAbsolutePath(&ret);
            return ret;
        }


    } TextDocListener;

    // Shadow structure - used to keep track of shadow color and offsets, allocated only
    // if those were assigned through the shadowStyle and shadowColor properties.
    struct ShadowParams : public GNewOverrideBase<GFxStatMV_ActionScript_Mem>
    {
        GColor              ShadowColor;
        GASString           ShadowStyleStr;
        GArrayLH<GPointF>   ShadowOffsets;
        GArrayLH<GPointF>   TextOffsets;

        ShadowParams(GASStringContext* psc)
            : ShadowStyleStr(psc->GetBuiltin(GASBuiltin_empty_))
        {
            ShadowColor = GColor(0,0,0,255);
        }
    };

    ShadowParams             *pShadow;

    struct RestrictParams : public GNewOverrideBase<GFxStatMV_ActionScript_Mem>
    {
        typedef GRangeDataArray<void*, GArrayLH<GRangeData<void*> > > RestrictRangesArrayType;

        RestrictRangesArrayType RestrictRanges;
    };
    RestrictParams          *pRestrict;

    GPtr<GASTextFieldObject> ASTextFieldObj;

    UInt                    FocusedControllerIdx;

#ifndef GFC_NO_FXPLAYER_AS_BITMAPDATA
    GFxStringHashLH<GPtr<GFxTextImageDesc> >* pImageDescAssoc;

    void ProceedImageSubstitution(const GASFnCall& fn, int idx, const GASValue* pve);
#endif
public:

    static void SetTextFormat(const GASFnCall& fn);
    static void SetNewTextFormat(const GASFnCall& fn);
    static void GetTextFormat(const GASFnCall& fn);
    static void GetNewTextFormat(const GASFnCall& fn);
    static void ReplaceSel(const GASFnCall& fn);
    static void ReplaceText(const GASFnCall& fn);
    static void RemoveTextField(const GASFnCall& fn);

#ifndef GFC_NO_TEXTFIELD_EXTENSIONS
    // extensions
    static void GetCharBoundaries(const GASFnCall& fn);
    static void GetCharIndexAtPoint(const GASFnCall& fn);
    static void GetExactCharBoundaries(const GASFnCall& fn);
    static void GetLineIndexAtPoint(const GASFnCall& fn);
    static void GetLineIndexOfChar(const GASFnCall& fn);
    static void GetLineOffset(const GASFnCall& fn);
    static void GetLineLength(const GASFnCall& fn);
    static void GetLineMetrics(const GASFnCall& fn);
    static void GetLineText(const GASFnCall& fn);
    static void GetFirstCharInParagraph(const GASFnCall& fn);
#ifndef GFC_NO_FXPLAYER_AS_BITMAPDATA
    static void SetImageSubstitutions(const GASFnCall& fn);
    static void UpdateImageSubstitution(const GASFnCall& fn);
#endif // GFC_NO_FXPLAYER_AS_BITMAPDATA
    static void AppendText(const GASFnCall& fn);
    static void AppendHtml(const GASFnCall& fn);
    static void CopyToClipboard(const GASFnCall& fn);
    static void CutToClipboard(const GASFnCall& fn);
    static void PasteFromClipboard(const GASFnCall& fn);
#endif //GFC_NO_TEXTFIELD_EXTENSIONS

    GFxEditTextCharacter(GFxEditTextCharacterDef* def, GFxMovieDefImpl *pbindingDefImpl, 
                         GFxASCharacter* parent, GFxResourceId id)
        :
        GASTextField(pbindingDefImpl, parent, id),
        pDef(def),
        VariableName(parent->GetASEnvironment()->CreateString(def->VariableName))
    {
        GASSERT(parent);
        GASSERT(pDef);
#ifndef GFC_NO_FXPLAYER_AS_BITMAPDATA
        pImageDescAssoc = NULL;
#endif
        pBinding = &pbindingDefImpl->GetResourceBinding();

        Flags = 0;
        FocusedControllerIdx = ~0u;

        //TextColor   = def->Color;
        Alignment   = def->Alignment;
        SetPassword(def->IsPassword());
        SetHtml(def->IsHtml());
        SetMouseWheelEnabled(true);
        pShadow     = 0;
        pRestrict   = 0;

        BackgroundColor = GColor(255, 255, 255, 0);
        BorderColor     = GColor(0,0,0,0);
        if (def->IsBorder())
        {
            BackgroundColor.SetAlpha(255);
            BorderColor.SetAlpha(255);
        }

        GFxMovieRoot* proot = parent->GetMovieRoot();
        // initialize built-in members by default values
        //ASTextFieldObj = *GHEAP_NEW(proot->GetMovieHeap()) GASTextFieldObject(GetGC(), this);
        // let the base class know what's going on
        //pProto = ASTextFieldObj->Get__proto__();
        pProto = GetGC()->GetActualPrototype(GetASEnvironment(), GASBuiltin_TextField);
        //pProto = GetGC()->GetPrototype(GASBuiltin_TextField);

        // add itself as a listener, the Flash behavior
        GASAsBroadcaster::InitializeInstance(parent->GetASEnvironment()->GetSC(), this);
        GASAsBroadcaster::AddListener(parent->GetASEnvironment(), this, this);
                
        GPtr<GFxTextAllocator> ptextAllocator = proot->GetTextAllocator();
        pDocument = 
            *GHEAP_NEW(proot->GetMovieHeap()) GFxTextDocView(ptextAllocator, parent->GetFontManager(), GetLog());
        pDocument->SetDocumentListener(&TextDocListener);
        pDocument->GetStyledText()->SetNewLine0D(); // Flash uses '\x0D' ('\r', #13) as a new-line char
        
        SetInitialFormatsAsDefault();

        if (def->MaxLength > 0)
            pDocument->SetMaxLength(UPInt(def->MaxLength));

        pDocument->SetViewRect(def->TextRect);
        if (def->DoesUseDeviceFont())
        {
            pDocument->SetUseDeviceFont();
            pDocument->SetAAForReadability();
        }
        if (def->IsMultiline())
            pDocument->SetMultiline();
        else
            pDocument->ClearMultiline();
        const bool  autoSizeByX = (def->IsAutoSize() && !(def->IsWordWrap() && def->IsMultiline()));
        const bool  autoSizeByY = (def->IsAutoSize() /*&& Multiline*/);
        if (autoSizeByX)
            pDocument->SetAutoSizeX();
        if (autoSizeByY)
            pDocument->SetAutoSizeY();
        if (def->IsWordWrap())
            pDocument->SetWordWrap();
        if (IsPassword())
            pDocument->SetPasswordMode();
        if (pDef->IsAAForReadability())
            pDocument->SetAAForReadability();
        if (!IsReadOnly() || pDef->IsSelectable())
        {
            CreateEditorKit();
        }
    }

    ~GFxEditTextCharacter()
    {
#ifndef GFC_NO_FXPLAYER_AS_BITMAPDATA
        ClearIdImageDescAssoc();
#endif
        pDocument->Close();
        delete pShadow;
        delete pRestrict;
    }

    GASTextFieldObject* GetTextFieldASObject()          
    { 
        if (!ASTextFieldObj)
        {
            ASTextFieldObj = *GHEAP_AUTO_NEW(this) GASTextFieldObject(GetGC(), this);
        }
        return ASTextFieldObj;
    }
    GASObject*  GetASObject () { return GetTextFieldASObject(); }
    GASObject*  GetASObject () const 
    { 
        return const_cast<GFxEditTextCharacter*>(this)->GetTextFieldASObject(); 
    }

    virtual void    SetFilters(const GArray<GFxFilterDesc> f)
    {
        GFxTextFilter tf;
        for (UInt i = 0; i < f.GetSize(); i++)
            tf.LoadFilterDesc(f[i]);
        SetFilters(tf);
    }

    virtual void SetFilters(const GFxTextFilter& f)
    { 
        if (pDocument)
            pDocument->SetFilters(f);
    }

    virtual GFxCharacterDef* GetCharacterDef() const        { return pDef; }

    void GetInitialFormats(GFxTextFormat* ptextFmt, GFxTextParagraphFormat* pparaFmt)
    {
        ptextFmt->InitByDefaultValues();
        pparaFmt->InitByDefaultValues();

        // get font params and use them as initial text format params.
        if (pDef->FontId.GetIdIndex())
        {
            // resolve font name, if possible
            GFxResourceBindData fontData = pBinding->GetResourceData(pDef->pFont);
            if (!fontData.pResource)
            {
                GPtr<GFxLog> plog = GetLog();
                if (plog)
                {
                    plog->LogError("Error: Resource for font id = %d is not found in text field id = %d, def text = '%s'\n", 
                        pDef->FontId.GetIdIndex(), GetId().GetIdIndex(), pDef->DefaultText.ToCStr());
                }
            }
            else if (fontData.pResource->GetResourceType() != GFxResource::RT_Font)
            {
                GPtr<GFxLog> plog = GetLog();
                if (plog)
                {
                    plog->LogError("Error: Font id = %d is referring to non-font resource in text field id = %d, def text = '%s'\n", 
                        pDef->FontId.GetIdIndex(), GetId().GetIdIndex(), pDef->DefaultText.ToCStr());
                }
            }
            else
            {
                GFxFontResource* pfont = (GFxFontResource*)fontData.pResource.GetPtr();
                if (pfont)
                {
                    ptextFmt->SetFontName(pfont->GetName());

                    // Don't set bold and italic as part of initial format, if HTML is used.
                    // Also, don't use font handle, since HTML should to set its own font by name.
                    if (!pDef->IsHtml()/* && pfont->GetFont()->IsResolved()*/)
                    {
                        ptextFmt->SetBold(pfont->IsBold());
                        ptextFmt->SetItalic(pfont->IsItalic());
                        if (!pDef->DoesUseDeviceFont() && pfont->GetFont()->IsResolved() )
                        {
                            GPtr<GFxFontHandle> pfontHandle = 
                                *GHEAP_AUTO_NEW(this) GFxFontHandle(NULL, pfont, 0, 0, pBinding->GetOwnerDefImpl());
                            ptextFmt->SetFontHandle(pfontHandle);
                        }
                    }
                }
            }
        }
        ptextFmt->SetFontSizeInTwips(UInt(pDef->TextHeight));
        ptextFmt->SetColor(pDef->Color);

        GFxTextParagraphFormat defaultParagraphFmt;
        switch (pDef->Alignment)
        {
        case GFxEditTextCharacterDef::ALIGN_LEFT: 
            pparaFmt->SetAlignment(GFxTextParagraphFormat::Align_Left);
            break;
        case GFxEditTextCharacterDef::ALIGN_RIGHT: 
            pparaFmt->SetAlignment(GFxTextParagraphFormat::Align_Right);
            break;
        case GFxEditTextCharacterDef::ALIGN_CENTER: 
            pparaFmt->SetAlignment(GFxTextParagraphFormat::Align_Center);
            break;
        case GFxEditTextCharacterDef::ALIGN_JUSTIFY: 
            pparaFmt->SetAlignment(GFxTextParagraphFormat::Align_Justify);
            break;
        }
        if (pDef->HasLayout())
        {
            pparaFmt->SetLeftMargin((UInt)TwipsToPixels(pDef->LeftMargin));
            pparaFmt->SetRightMargin((UInt)TwipsToPixels(pDef->RightMargin));
            pparaFmt->SetIndent((SInt)TwipsToPixels(pDef->Indent));
            pparaFmt->SetLeading((SInt)TwipsToPixels(pDef->Leading));
        }
    }

    void SetInitialFormatsAsDefault()
    {
        GFxTextFormat          defaultTextFmt(GMemory::GetHeapByAddress(this));
        GFxTextParagraphFormat defaultParagraphFmt;

        const GFxTextFormat*          ptextFmt = pDocument->GetDefaultTextFormat();
        const GFxTextParagraphFormat* pparaFmt = pDocument->GetDefaultParagraphFormat();

        if (!pDef->IsEmptyTextDef())
        {
            GetInitialFormats(&defaultTextFmt, &defaultParagraphFmt);
            if (ptextFmt)
                defaultTextFmt      = ptextFmt->Merge(defaultTextFmt);
            if (pparaFmt)
                defaultParagraphFmt = pparaFmt->Merge(defaultParagraphFmt);
        }
        else
        {
            if (ptextFmt)
                defaultTextFmt      = *ptextFmt;
            if (pparaFmt)
                defaultParagraphFmt = *pparaFmt;

            GFxTextFormat          eTextFmt(GMemory::GetHeapByAddress(this));
            GFxTextParagraphFormat eParagraphFmt;
            eTextFmt.InitByDefaultValues();
            eParagraphFmt.InitByDefaultValues();
            defaultTextFmt      = eTextFmt.Merge(defaultTextFmt);
            defaultParagraphFmt = eParagraphFmt.Merge(defaultParagraphFmt);
        }
        pDocument->SetDefaultTextFormat(defaultTextFmt);
        pDocument->SetDefaultParagraphFormat(defaultParagraphFmt);
    }

    bool UpdateTextFromVariable() 
    {
        if (!VariableName.IsEmpty()) 
        {   
            // Variable name can be a path, so use GetVariable().
            GASEnvironment* penv = GetASEnvironment();
            if (penv)
            {
                GASValue val;
                if (penv->GetVariable(VariableName, &val)) 
                {        
                    if (!val.IsEqual(penv, VariableVal))
                    {
                        VariableVal = val;
                        GASString str = val.ToString(penv);
                        SetTextValue(str.ToCStr(), false, false);
                        return true;
                    }
                }
                else
                {
                    // if variable name is specified but no variable found (were deleted, for example)
                    // then just set empty string
                    SetTextValue("", false, false);
                }
            }
            return false; // can't update the text for some reasons
        }
        return true; // do not need update anything, so, status of operation is OK.
    }

#ifndef GFC_USE_OLD_ADVANCE
    // Returns 0 if nothing to do
    // 1 - if need to add to optimized play list
    // -1 - if need to remove from optimized play list
    int             CheckAdvanceStatus(bool playingNow)
    {
        int rv = 0;
        bool advanceDisabled = IsAdvanceDisabled() || !CanAdvanceChar();
        if (!advanceDisabled)
        {
            // Textfield is always in 'playing' status if it has a variable associated.
            bool playing = IsForceAdvance() || !VariableName.IsEmpty();

            if (!playing)
            {
                // It is playing if there is a blinking cursor. It blinks only if 
                // the textfield is focused.
                // It is also playing if mouse is captured (for performing scrolling)
                GFxMovieRoot* proot = GetMovieRoot();
                GFxTextEditorKit* peditorKit = pDocument->GetEditorKit();
                if (IsVisibleFlagSet() && peditorKit && 
                    ((peditorKit->HasCursor() && proot->IsFocused(this)) || peditorKit->IsMouseCaptured()))
                    playing = true;
            }

            if (playing && !playingNow)
                rv = 1;
            else if (!playing && playingNow)
                rv = -1;
        }
        else if (playingNow)
            rv = -1;
        return rv;
    }

    virtual void SetStateChangeFlags(UInt8 flags) 
    { 
        GFxASCharacter::SetStateChangeFlags(flags);
        SetForceAdvance();
        ModifyOptimizedPlayListLocal<GFxEditTextCharacter>(GetMovieRoot());
    }
#endif //#ifndef GFC_USE_OLD_ADVANCE

    // Override AdvanceFrame so that variable dependencies can be updated.
    virtual void    AdvanceFrame(bool nextFrame, Float framePos)
    {
        UInt8 stateFlags      = GetStateChangeFlags();
        const UInt8 stateMask = GFxCharacter::StateChanged_FontLib | GFxCharacter::StateChanged_FontMap |
                                GFxCharacter::StateChanged_FontProvider | GFxCharacter::StateChanged_Translator;
        if ((stateFlags & stateMask) && pDocument)
        {
            pDocument->SetCompleteReformatReq();
            if (stateFlags & GFxCharacter::StateChanged_Translator)
                TextDocListener.TranslatorChanged();
            SetTextValue(OriginalTextValue.ToCStr(), IsOriginalHtml());
        }
        // use parent class' SetStateChangeFlags here to avoid extra calls to 
        // ModifyOptimizedPlayListLocal.
        GFxASCharacter::SetStateChangeFlags(stateFlags & (~stateMask));

        if (IsForceAdvance())
        {
            ClearForceAdvance();
            ModifyOptimizedPlayListLocal<GFxEditTextCharacter>(GetMovieRoot());
        }

        GUNUSED(framePos);
        if (nextFrame)
        {
            UpdateTextFromVariable();
            Flags |= Flags_NextFrame;
        }
        else
        {
            Flags &= ~Flags_NextFrame;
        }
        
        if (pDocument->HasEditorKit())
        {
            GFxMovieRoot* proot = GetMovieRoot();
            GFxTextEditorKit* peditorKit = pDocument->GetEditorKit();
            if (proot->IsFocused(this) || peditorKit->IsMouseCaptured())
            {
                Double timer = proot->GetTimeElapsed();
                peditorKit->Advance(timer);
                if (peditorKit->HasCursor())
                    proot->SetDirtyFlag(); // because cursor may blink
            }
        }
    }

    void ProcessImageTags(GFxStyledText::HTMLImageTagInfoArray& imageInfoArray);

#ifndef GFC_NO_CSS_SUPPORT
    bool HasStyleSheet() const { return pCSSData && pCSSData->pASStyleSheet.GetPtr() != NULL; }
    const GFxTextStyleManager* GetStyleSheet() const 
    { 
        return (HasStyleSheet()) ? pCSSData->pASStyleSheet->GetTextStyleManager() : NULL; 
    }
#else
    bool HasStyleSheet() const                       { return false; }
    const GFxTextStyleManager* GetStyleSheet() const { return NULL; }
#endif //GFC_NO_CSS_SUPPORT

    GINLINE void    SetText(const char* pnewText, bool reqHtml)
    {
        bool currHtml = IsHtml();
        if (reqHtml && !currHtml)        { SetHtml(true); }
        else if (!reqHtml && currHtml)   { SetHtml(false); }
        SetTextValue(pnewText, reqHtml);
    }

    GINLINE void    SetText(const wchar_t* pnewText, bool reqHtml)
    {
        bool currHtml = IsHtml();
        if (reqHtml && !currHtml)        { SetHtml(true); }
        else if (!reqHtml && currHtml)   { SetHtml(false); }

        enum { StackBuffSize = 256 };
        char* pbuff;
        char stackBuff[StackBuffSize];
        UPInt charSz = G_wcslen(pnewText);
        pbuff = charSz < StackBuffSize ? stackBuff : (char*)GHEAP_AUTO_ALLOC(this, charSz * 5 + 1);
        GUTF8Util::EncodeString(pbuff, pnewText);

        SetTextValue(pbuff, reqHtml);
        
        if (charSz >= StackBuffSize) GFREE(pbuff);
    }

    // Set our text to the given string. pnewText can have HTML or
    // regular formatting based on argument.
    virtual void    SetTextValue(const char* pnewText, bool html, bool notifyVariable = true)
    {
        // If CSS StyleSheet is set, then assignment to "text" works the same
        // way as to "htmlText".
        if (HasStyleSheet())
            html = true;

        OriginalTextValue = pnewText;
        SetOriginalHtml(html);
        // need to update the pnewText pointer, since assignment
        // to OriginalTextValue may cause its reallocation.
        pnewText = OriginalTextValue.ToCStr();

        bool doNotifyChanged = false;
        bool translated = false;
        if (!IsNoTranslate())
        {
            GPtr<GFxTranslator> ptrans = GetMovieRoot()->GetTranslator();
            if (ptrans)
            {
                class TranslateInfo : public GFxTranslator::TranslateInfo
                {
                public:
                    GFxWStringBuffer::Reserve<512> Res1, Res2;
                    GFxWStringBuffer ResultBuf;
                    GFxWStringBuffer KeyBuf;

                    TranslateInfo(const char* instanceName):ResultBuf(Res1), KeyBuf(Res2)
                    {
                        pResult       = &ResultBuf;
                        pInstanceName = instanceName;
                    }

                    void SetKey(const char* pkey)
                    {
                        int nchars = (int)GUTF8Util::GetLength(pkey);
                        KeyBuf.Resize(nchars + 1);
                        GUTF8Util::DecodeString(KeyBuf.GetBuffer(), pkey);
                        pKey = KeyBuf.GetBuffer();
                    }
                    void    SetKey() { pKey = KeyBuf.GetBuffer(); }
                    
                    bool    IsResultHtml() const { return (Flags & Flag_ResultHtml) != 0; }
                    bool    IsTranslated() const { return (Flags & Flag_Translated) != 0; }

                    void    SetKeyHtml() { Flags |= Flag_SourceHtml; }
                } translateInfo((!HasInstanceBasedName()) ? GetName().ToCStr() : "");

                if (!html || ptrans->CanReceiveHtml())
                {
                    // convert html or plain text to unicode w/o parsing
                    translateInfo.SetKey(pnewText);
                    if (html)
                        translateInfo.SetKeyHtml();
                    ptrans->Translate(&translateInfo);
                }
                else
                {
                    // if html, but need to pass plain text
                    //SetInitialFormatsAsDefault();
                    GFxTextFormat txtFmt(GMemory::GetHeapByAddress(this));
                    GFxTextParagraphFormat paraFmt;
                    GetInitialFormats(&txtFmt, &paraFmt);
                    pDocument->ParseHtml(pnewText, GFC_MAX_UPINT, IsCondenseWhite(), NULL, GetStyleSheet(), &txtFmt, &paraFmt);
                    pDocument->GetStyledText()->GetText(&translateInfo.KeyBuf);
                    if (ptrans->NeedStripNewLines())
                        translateInfo.KeyBuf.StripTrailingNewLines();
                    translateInfo.SetKey();
                    ptrans->Translate(&translateInfo);
                }
                translated = translateInfo.IsTranslated();
                if (translated)
                {
                    if (translateInfo.IsResultHtml())
                    {
                        //SetInitialFormatsAsDefault();
                        GFxTextFormat txtFmt(GMemory::GetHeapByAddress(this));
                        GFxTextParagraphFormat paraFmt;
                        GetInitialFormats(&txtFmt, &paraFmt);
                        GFxStyledText::HTMLImageTagInfoArray imageInfoArray(GMemory::GetHeapByAddress(this));
                        pDocument->ParseHtml(translateInfo.ResultBuf.ToWStr(), GFC_MAX_UPINT, IsCondenseWhite(), &imageInfoArray, GetStyleSheet(), &txtFmt, &paraFmt);
                        if (imageInfoArray.GetSize() > 0)
                        {
                            ProcessImageTags(imageInfoArray);
                        }
                    }
                    else
                    {
                        // Attempt to keep original formatting: get the format of the old text's first letter
                        // and apply to whole new text. This should prevent from losing formatting (bold, italic, etc)
                        // after translation.
                        const GFxTextFormat*          ptextFmt;
                        const GFxTextParagraphFormat* pparaFmt;
                        pDocument->GetTextAndParagraphFormat(&ptextFmt, &pparaFmt, 0);
                        pDocument->SetDefaultTextFormat(ptextFmt);
                        pDocument->SetDefaultParagraphFormat(pparaFmt);

                        pDocument->SetText(translateInfo.ResultBuf.ToWStr());
                    }
                    doNotifyChanged = true;
                }
            }
        }

        if (!translated)
        {
            if (html)
            {
                //SetInitialFormatsAsDefault(); //!
                GFxTextFormat txtFmt(GMemory::GetHeapByAddress(this));
                GFxTextParagraphFormat paraFmt;
                GetInitialFormats(&txtFmt, &paraFmt);
                GFxStyledText::HTMLImageTagInfoArray imageInfoArray(GMemory::GetHeapByAddress(this));
                pDocument->ParseHtml(pnewText, GFC_MAX_UPINT, IsCondenseWhite(), &imageInfoArray, GetStyleSheet(), &txtFmt, &paraFmt);
                if (imageInfoArray.GetSize() > 0)
                {
                    ProcessImageTags(imageInfoArray);
                }
            }
            else
            {
                pDocument->SetText(pnewText);        
            }
        }

        if (pDocument->HasEditorKit())
        {
            if (!IsReadOnly()/* || IsSelectable()*/)
            {
                //!AB, this causes single line text scrolled to the end when text is assigned and
                // it is longer than the visible text field.
                // This is not happening in Flash, so at the moment just comment
                // this out.
                //if (!pDocument->IsMultiline())
                //    pDocument->GetEditorKit()->SetCursorPos(pDocument->GetLength(), false);
                //else
                //    pDocument->GetEditorKit()->SetCursorPos(0, false);
                UPInt docLen = pDocument->GetLength();
                if (pDocument->GetEditorKit()->GetCursorPos() > docLen)
                    pDocument->GetEditorKit()->SetCursorPos(docLen, false);
            }
        }
        
        if (HasStyleSheet() && pDocument->MayHaveUrl())
        {
            // collect info about active zones for each URL
            CollectUrlZones();
        }

        // update variable, if necessary
        if (notifyVariable)
            UpdateVariable();

        //!AB MaxLength is available only for InputText; though, if
        // you set text by variable the maxlength is ignored.
        if (doNotifyChanged)
            NotifyChanged();

        SetNeedToUpdateGeomData();
    }

    // For ActionScript toString.
    virtual const char* GetTextValue(GASEnvironment* =0) const
    {       
        return "";//NULL;//pDocument->GetText()
    }

    virtual ObjectType  GetObjectType() const
    {
        return Object_TextField;
    }

    // GFxASCharacter override to indicate which standard members are handled for us.
    virtual UInt32  GetStandardMemberBitMask() const
    {       
        return  M_BitMask_PhysicalMembers |
                M_BitMask_CommonMembers |                               
                (1 << M_target) |
                (1 << M_url) |
                (1 << M_filters) |          
                (1 << M_tabEnabled) |
                (1 << M_tabIndex) |         
                (1 << M_quality) |
                (1 << M_highquality) |
                (1 << M_soundbuftime) |
                (1 << M_xmouse) |
                (1 << M_ymouse)
                ;
        // MA Verified: _lockroot does not exist/carry over from text fields, so don't include it.
        // If a movie is loaded into TextField, local _lockroot state is lost.
    }
    
    // Special method for complex objects API (see GFxValue::ObjectInterface::SetDisplayInfo)
    virtual GPointF    TransformToTextRectSpace(const GFxValue::DisplayInfo& pinfo) const
    {
        const Matrix& m = GetMatrix();
        GPointF     p(pinfo.IsFlagSet(GFxValue::DisplayInfo::V_x) ? (Float)PixelsToTwips(pinfo.GetX()) : pGeomData->X, 
                      pinfo.IsFlagSet(GFxValue::DisplayInfo::V_y) ? (Float)PixelsToTwips(pinfo.GetY()) : pGeomData->Y);
        p           = m.TransformByInverse(p);
        GPointF r   = pDocument->GetViewRect().TopLeft();
        p.x         -= r.x;
        p.y         -= r.y;
        p           = m.Transform(p);
        p.x         = TwipsToPixels(p.x);
        p.y         = TwipsToPixels(p.y);
        return p;
    }

    bool    SetStandardMember(StandardMember member, const GASValue& val, bool opcodeFlag)
    {
        // _width and _height assignment work differently from movieclip:
        // it just changes the textrect.
        switch(member)
        {
        case M_x:
            {
                // NOTE: If updating this logic, also update GFxValue::ObjectInterface::SetDisplayInfo
                // and TransformToTextRectSpace() above.
                Matrix m    = GetMatrix();
                Double newX = PixelsToTwips(val.ToNumber(GetASEnvironment()));
                // We need to modify the coordinate being set. We need to
                // use TextRect.TopLeft point for this modification. 
                // First of all translate the coordinate to the TextRect coordinate
                // space, do the adjustment by the -TextRect.TopLeft() and translate
                // coordinate back to parent's coordinate space.
                GPointF     p((Float)newX);
                p           = m.TransformByInverse(p);
                GPointF r   = pDocument->GetViewRect().TopLeft();
                p.x         -= r.x;
                p           = m.Transform(p);
                GASValue val;
                val.SetNumber(TwipsToPixels(p.x));

                if (GASTextField::SetStandardMember(member, val, 0))
                {
                    if (pGeomData)
                        pGeomData->X = G_IRound(newX);
                    return true;
                }
                return false;
            }

        case M_y:
            {
                // NOTE: If updating this logic, also update GFxValue::ObjectInterface::SetDisplayInfo
                // and TransformToTextRectSpace() above.
                Double newY = PixelsToTwips(val.ToNumber(GetASEnvironment()));
                Matrix m    = GetMatrix();
                // We need to modify the coordinate being set. We need to
                // use TextRect.TopLeft point for this modification. 
                // First of all translate the coordinate to the TextRect coordinate
                // space, do the adjustment by the -TextRect.TopLeft() and translate
                // coordinate back to parent's coordinate space.
                GPointF     p(0, (Float)newY);
                p           = m.TransformByInverse(p);
                GPointF r   = pDocument->GetViewRect().TopLeft();
                p.y         -= r.y;
                p           = m.Transform(p);
                GASValue val;
                val.SetNumber(TwipsToPixels(p.y));

                if (GASTextField::SetStandardMember(member, val, 0))
                {
                    if (pGeomData)
                        pGeomData->Y = G_IRound(newY);
                    return true;
                }
                return false;
            }
        case M_width:
            {
                Float newWidth = Float(PixelsToTwips(val.ToNumber(GetASEnvironment())));
                GRectF viewRect = pDocument->GetViewRect();
                viewRect.Right = viewRect.Left + newWidth;
                pDocument->SetViewRect(viewRect);
                SetNeedToUpdateGeomData();
                return true;
            }
        case M_height:
            {   
                Float newHeight = Float(PixelsToTwips(val.ToNumber(GetASEnvironment())));
                GRectF viewRect = pDocument->GetViewRect();
                viewRect.Bottom = viewRect.Top + newHeight;
                pDocument->SetViewRect(viewRect);
                SetNeedToUpdateGeomData();
                return true;
            }
        case M_xscale:
        case M_yscale:
        case M_rotation:
#ifndef GFC_NO_3D
        case M_zscale:
        case M_xrotation:
        case M_yrotation:
#endif
            // NOTE: If updating this logic, also update GFxValue::ObjectInterface::SetDisplayInfo
            SetNeedToUpdateGeomData();
            break;
        default:
            break;
        }

        if (GASTextField::SetStandardMember(member, val, opcodeFlag))
            return true;
        return false;
    }

    void UpdateAutosizeSettings()
    {
        const bool  autoSizeByX = (IsAutoSize() && !pDocument->IsWordWrap());
        const bool  autoSizeByY = (IsAutoSize());
        if (autoSizeByX)
            pDocument->SetAutoSizeX();
        else
            pDocument->ClearAutoSizeX();
        if (autoSizeByY)
            pDocument->SetAutoSizeY();
        else
            pDocument->ClearAutoSizeY();
        SetNeedToUpdateGeomData();
    }

    bool    SetMember(GASEnvironment* penv, const GASString& name, const GASValue& origVal, const GASPropFlags& flags)
    {
        StandardMember member = GetStandardMemberConstant(name);
        
        // Check, if there are watch points set. Invoke, if any. Do not invoke
        // watch for built-in properties.
        GASValue val(origVal);
        if (member < M_BuiltInProperty_Begin || member > M_BuiltInProperty_End)
        {
            if (penv && GetTextFieldASObject() && ASTextFieldObj->HasWatchpoint()) // have set any watchpoints?
            {
                GASValue        newVal;
                if (ASTextFieldObj->InvokeWatchpoint(penv, name, val, &newVal))
                {
                    val = newVal;
                }
            }
        }

        if (SetStandardMember(member, origVal, false))
            return true;

        // Handle TextField-specific "standard" members.
        switch(member)
        {
            case M_html:
                SetHtml(val.ToBool(penv));
                return true; 
                
            case M_htmlText:
                {
                    GASString str(val.ToStringVersioned(penv, GetVersion()));
                    SetTextValue(str.ToCStr(), IsHtml());
                    return true;
                }

            case M_text:
                {
                    ResetBlink(1);
                    GASString str(val.ToStringVersioned(penv, GetVersion()));
                    SetTextValue(str.ToCStr(), false);
                    return true;
                }

            case M_textColor:
                {
                    // The arg is 0xRRGGBB format.
                    UInt32  rgb = val.ToUInt32(penv);
                    
                    GFxTextFormat colorTextFmt(penv->GetHeap());
                    colorTextFmt.SetColor32(rgb);

                    pDocument->SetTextFormat(colorTextFmt);

                    colorTextFmt = *pDocument->GetDefaultTextFormat();
                    colorTextFmt.SetColor32(rgb);
                    pDocument->SetDefaultTextFormat(colorTextFmt);
                    SetDirtyFlag();
                    return true;
                }

            case M_autoSize:
                {
                    GASString asStr(val.ToString(penv));
                    GString str = (val.IsBoolean()) ? 
                        ((val.ToBool(penv)) ? "left":"none") : asStr.ToCStr();

                    GFxTextDocView::ViewAlignment oldAlignment = pDocument->GetAlignment();
                    bool oldAutoSize = IsAutoSize();
                    if (str == "none")
                    {
                        ClearAutoSize();
                        pDocument->SetAlignment(GFxTextDocView::Align_Left);
                    }
                    else 
                    {
                        SetAutoSize();
                        if (str == "left")
                            pDocument->SetAlignment(GFxTextDocView::Align_Left);
                        else if (str == "right")
                            pDocument->SetAlignment(GFxTextDocView::Align_Right);
                        else if (str == "center")
                            pDocument->SetAlignment(GFxTextDocView::Align_Center);
                    }
                    if ((oldAlignment != pDocument->GetAlignment()) || (oldAutoSize != IsAutoSize()))
                        UpdateAutosizeSettings();
                    SetDirtyFlag();
                    return true;
                }

            case M_wordWrap:
                {
                    bool oldWordWrap = pDocument->IsWordWrap();
                    bool wordWrap = val.ToBool(penv);
                    if (wordWrap != oldWordWrap)
                    {
                        if (wordWrap)
                            pDocument->SetWordWrap();
                        else
                            pDocument->ClearWordWrap();
                        UpdateAutosizeSettings();
                    }
                    SetDirtyFlag();
                    return true;
                }

            case M_multiline:
                {
                    bool oldML = pDocument->IsMultiline();
                    bool newMultiline = val.ToBool(penv);
                    if (oldML != newMultiline)
                    {
                        if (newMultiline)
                            pDocument->SetMultiline();
                        else
                            pDocument->ClearMultiline();
                        UpdateAutosizeSettings();
                    }
                    SetDirtyFlag();
                    return true;
                }

            case M_border:
                {
                    bool bc = val.ToBool(penv);
                    if (bc)
                        BorderColor.SetAlpha(255);
                    else
                        BorderColor.SetAlpha(0);
                    SetDirtyFlag();
                    return true;
                }

            case M_variable:
                {
                    //if (val.IsNull() || val.IsUndefined())
                    //    VariableName = "";
                    //!AB: even if we set variable name pointing to invalid or non-exisiting variable
                    // we need to save it and try to update text using it. If we can't get a value 
                    // (variable doesn't exist), then text shouldn't be changed.
                    {                        
                        VariableName = val.ToString(penv);
                        UpdateTextFromVariable();
                        SetForceAdvance();
                        ModifyOptimizedPlayListLocal<GFxEditTextCharacter>(GetMovieRoot());
                    }
                    return true;
                }

            case M_selectable:
                {
                    SetSelectable(val.ToBool(penv));
                    return true;
                }

            case M_embedFonts:
                {
                    if (val.ToBool(penv))
                        pDocument->ClearUseDeviceFont();
                    else
                        pDocument->SetUseDeviceFont();
                    return true;
                }

            case M_password:
                {
                    bool pswd = IsPassword();
                    bool password = val.ToBool(penv);
                    if (pswd != password)
                    {
                        SetPassword(password);
                        if (password)
                            pDocument->SetPasswordMode();
                        else
                            pDocument->ClearPasswordMode();
                        pDocument->SetCompleteReformatReq();
                    }
                    SetDirtyFlag();
                    return true;
                }

            case M_hscroll:
                {
                    SInt scrollVal = (SInt)val.ToInt32(penv);
                    if (scrollVal < 0)
                        scrollVal = 0;
                    pDocument->SetHScrollOffset((UInt)PixelsToTwips(scrollVal));    
                    SetDirtyFlag();
                    return true;
                }

            case M_scroll:
                {
                    SInt scrollVal = (SInt)val.ToInt32(penv);
                    if (scrollVal < 1)
                        scrollVal = 1;
                    pDocument->SetVScrollOffset((UInt)(scrollVal - 1));    
                    SetDirtyFlag();
                    return true;
                }
            case M_background:
                {
                    bool bc = val.ToBool(penv);
                    if (bc)
                        BackgroundColor.SetAlpha(255);
                    else
                        BackgroundColor.SetAlpha(0);
                    SetDirtyFlag();
                    return true;
                }
            case M_backgroundColor:
                {
                    GASNumber bc = val.ToNumber(penv);
                    if (!GASNumberUtil::IsNaN(bc))
                    {
                        UInt32 c = val.ToUInt32(penv);
                        BackgroundColor = (BackgroundColor & 0xFF000000u) | (c & 0xFFFFFFu);
                    }
                    SetDirtyFlag();
                    return true;
                }
            case M_borderColor:
                {
                    GASNumber bc = val.ToNumber(penv);
                    if (!GASNumberUtil::IsNaN(bc))
                    {
                        UInt32 c = val.ToUInt32(penv);
                        BorderColor = (BorderColor & 0xFF000000u) | (c & 0xFFFFFFu);
                    }
                    SetDirtyFlag();
                    return true;
                }
            case M_type:
                {
                    GASString typeStr = val.ToString(penv);
                    if (penv->GetSC()->CompareConstString_CaseCheck(typeStr, "dynamic"))
                    {
                        if (pDocument->HasEditorKit())
                        {
                            pDocument->GetEditorKit()->SetReadOnly();
                        }
                    }
                    else if (penv->GetSC()->CompareConstString_CaseCheck(typeStr, "input") && !HasStyleSheet())
                    {
                        GFxTextEditorKit* peditor = CreateEditorKit();
                        peditor->ClearReadOnly();
                    }
                    // need to reformat because scrolling might be changed
                    pDocument->SetReformatReq();
                    return true;
                }
            case M_maxChars:
                {
                    GASNumber ml = val.ToNumber(penv);
                    if (!GASNumberUtil::IsNaN(ml) && ml >= 0)
                    {
                        pDocument->SetMaxLength((UPInt)val.ToUInt32(penv));
                    }
                    return true;
                }
            case M_condenseWhite:
                {
                    SetCondenseWhite(val.ToBool(penv));
                    return true;
                }
            case M_antiAliasType:
                {
                    GASString typeStr = val.ToString(penv);
                    if (penv->GetSC()->CompareConstString_CaseCheck(typeStr, "normal"))
                    {
                        pDocument->ClearAAForReadability();
                    }
                    else if (penv->GetSC()->CompareConstString_CaseCheck(typeStr, "advanced"))
                    {
                        pDocument->SetAAForReadability();                    
                    }
                    SetDirtyFlag();
                    return true;
                }

            case M_mouseWheelEnabled:
                {
                    SetMouseWheelEnabled(val.ToBool(penv));
                    return true;
                }
            case M_styleSheet:
                {
#ifndef GFC_NO_CSS_SUPPORT
                    GASObject* pobj = val.ToObject(penv);
                    if (pobj && pobj->GetObjectType() == GASObject::Object_StyleSheet)
                    {
                        if (!pCSSData)
                            pCSSData = new CSSHolder;
                        pCSSData->pASStyleSheet = static_cast<GASStyleSheetObject*>(pobj);

                        // make the text field read-only
                        if (pDocument->HasEditorKit())
                        {
                            pDocument->GetEditorKit()->SetReadOnly();
                        }

                        SetDirtyFlag();
                    }
                    else if (pCSSData)
                        pCSSData->pASStyleSheet = NULL;
                    CollectUrlZones();
                    UpdateUrlStyles();
#endif //GFC_NO_CSS_SUPPORT
                    return true;
                }

            // extension
            case M_shadowStyle:
                if (!penv->CheckExtensions())
                    break;

                {
                    SetDirtyFlag();
                    GASString   styleStr = val.ToString(penv);
                    const char *pstr = styleStr.ToCStr();

                    if (!pShadow)
                        if ((pShadow = new ShadowParams(penv->GetSC())) == 0)
                            return true;

                    // The arg is 0xRRGGBB format.
                    UInt32  rgb = pDocument->GetShadowColor();
                    pShadow->ShadowColor.SetBlue((UByte)rgb);
                    pShadow->ShadowColor.SetGreen((UByte)(rgb >> 8));
                    pShadow->ShadowColor.SetRed((UByte)(rgb >> 16));

                    // When using the old fasioned shadow we have to disable the new one.
                    pDocument->DisableSoftShadow();

                reset:
                    pShadow->ShadowOffsets.Clear();
                    pShadow->TextOffsets.Clear();

                    // Parse shadow style text to generate the offset arrays.
                    // Format is similar to "s{1,0.5}{-1,0.5}t{0,0}".
                    const char *p = pstr;
                    GArrayLH<GPointF> *offsets = 0;
                    while (*p)
                    {
                        if (*p == 's' || *p == 'S')
                        {
                            offsets = &pShadow->ShadowOffsets;
                            p++;
                        }
                        else if (*p == 't' || *p == 'T')
                        {
                            offsets = &pShadow->TextOffsets;
                            p++;
                        }
                        else if (*p == '{' && offsets) // {x,y}
                        {
                            char pn[24];
                            p++;
                            const char *q = p;
                            while (*q && *q != ',') q++;
                            if (!*q || q-p > 23) { pstr = pShadow->ShadowStyleStr.ToCStr(); goto reset; }
                            memcpy(pn,p,q-p);
                            pn[q-p] = 0;
                            Float x = (Float)((GASValue(penv->CreateString(pn))).ToNumber(penv) * 20.0);
                            q++;
                            p = q;

                            while (*q && *q != '}') q++;
                            if (!*q || q-p > 23) { pstr = pShadow->ShadowStyleStr.ToCStr(); goto reset; }
                            memcpy(pn,p,q-p);
                            pn[q-p] = 0;
                            Float y = (Float)((GASValue(penv->CreateString(pn))).ToNumber(penv) * 20.0);
                            q++;
                            p = q;

                            offsets->PushBack(GPointF(x,y));
                        }
                        else
                        {
                            pstr = pShadow->ShadowStyleStr.ToCStr();
                            goto reset;
                        }
                    }

                    if (*pstr)
                        pShadow->ShadowStyleStr = penv->CreateString(pstr);
                    return true;
                }

            case M_shadowColor:
                if (penv->CheckExtensions())
                {
                    SetDirtyFlag();
                    pDocument->SetShadowColor(val.ToUInt32(penv));
                    if (pShadow)
                    {
                        // The arg is 0xRRGGBB format.
                        UInt32  rgb = pDocument->GetShadowColor();
                        pShadow->ShadowColor.SetBlue((UByte)rgb);
                        pShadow->ShadowColor.SetGreen((UByte)(rgb >> 8));
                        pShadow->ShadowColor.SetRed((UByte)(rgb >> 16));
                    }
                    return true;
                }
                break;

            case M_hitTestDisable:
                if (penv->CheckExtensions())
                {
                    SetHitTestDisableFlag(val.ToBool(GetASEnvironment()));
                    return 1;
                }
                break;

            case M_noTranslate:
                if (penv->CheckExtensions())
                {
                    SetNoTranslate(val.ToBool(GetASEnvironment()));
                    pDocument->SetCompleteReformatReq(); 
                    return 1;
                }
                break;

            case M_autoFit:
                if (penv->CheckExtensions())
                {
                    if (val.ToBool(penv))
                        pDocument->SetAutoFit();
                    else
                        pDocument->ClearAutoFit();
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_blurX:
                if (penv->CheckExtensions())
                {
                    pDocument->SetBlurX((Float)val.ToNumber(penv));
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_blurY:
                if (penv->CheckExtensions())
                {
                    pDocument->SetBlurY((Float)val.ToNumber(penv));
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_blurStrength:
                if (penv->CheckExtensions())
                {
                    pDocument->SetBlurStrength((Float)val.ToNumber(penv));
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_outline:
                if (penv->CheckExtensions())
                {
                    pDocument->SetOutline((UInt)val.ToNumber(penv));
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_fauxBold:
                if (penv->CheckExtensions())
                {
                    pDocument->SetFauxBold(val.ToBool(penv));
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_fauxItalic:
                if (penv->CheckExtensions())
                {
                    pDocument->SetFauxItalic(val.ToBool(penv));
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_shadowAlpha:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetShadowAlpha((Float)val.ToNumber(penv));
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_shadowAngle:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetShadowAngle((Float)val.ToNumber(penv));
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_shadowBlurX:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetShadowBlurX((Float)val.ToNumber(penv));
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_shadowBlurY:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetShadowBlurY((Float)val.ToNumber(penv));
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_shadowDistance:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetShadowDistance((Float)val.ToNumber(penv));
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_shadowHideObject:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetHideObject(val.ToBool(penv));
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_shadowKnockOut:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetKnockOut(val.ToBool(penv));
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_shadowQuality:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetShadowQuality((UInt)val.ToUInt32(penv));
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_shadowStrength:
                if (penv->CheckExtensions())
                {
                    pDocument->EnableSoftShadow();
                    pDocument->SetShadowStrength((Float)val.ToNumber(penv));
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_shadowOutline:
                // Not implemented
                //if (GetASEnvironment()->CheckExtensions())
                //{
                //    return true;
                //}
                break;

            case M_verticalAutoSize:
                if (penv->CheckExtensions())
                {
                    GASString asStr(val.ToString(penv));
                    GString str = asStr.ToCStr();

                    if (str == "none")
                    {
                        pDocument->ClearAutoSizeY();
                        pDocument->SetVAlignment(GFxTextDocView::VAlign_None);
                    }
                    else 
                    {
                        pDocument->SetAutoSizeY();
                        if (str == "top")
                            pDocument->SetVAlignment(GFxTextDocView::VAlign_Top);
                        else if (str == "bottom")
                            pDocument->SetVAlignment(GFxTextDocView::VAlign_Bottom);
                        else if (str == "center")
                            pDocument->SetVAlignment(GFxTextDocView::VAlign_Center);
                    }
                    SetNeedToUpdateGeomData();
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_fontScaleFactor:
                if (penv->CheckExtensions())
                {
                    GASNumber factor = val.ToNumber(penv);
                    if (factor > 0 && factor < 1000)
                        pDocument->SetFontScaleFactor((Float)factor);
                    SetDirtyFlag();
                }
                break;

            case M_verticalAlign:
                if (penv->CheckExtensions())
                {
                    GASString asStr(val.ToString(penv));
                    GString str = asStr.ToCStr();

                    if (str == "none")
                        pDocument->SetVAlignment(GFxTextDocView::VAlign_None);
                    else 
                    {
                        if (str == "top")
                            pDocument->SetVAlignment(GFxTextDocView::VAlign_Top);
                        else if (str == "bottom")
                            pDocument->SetVAlignment(GFxTextDocView::VAlign_Bottom);
                        else if (str == "center")
                            pDocument->SetVAlignment(GFxTextDocView::VAlign_Center);
                    }
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_textAutoSize:
                if (penv->CheckExtensions())
                {
                    GASString asStr(val.ToString(penv));
                    GString str = asStr.ToCStr();

                    if (str == "none")
                        pDocument->SetTextAutoSize(GFxTextDocView::TAS_None);
                    else 
                    {
                        if (str == "shrink")
                            pDocument->SetTextAutoSize(GFxTextDocView::TAS_Shrink);
                        else if (str == "fit")
                            pDocument->SetTextAutoSize(GFxTextDocView::TAS_Fit);
                    }
                    SetDirtyFlag();
                    return true;
                }
                break;

            case M_useRichTextClipboard:
                if (penv->CheckExtensions())
                {
                    SetUseRichClipboard(val.ToBool(GetASEnvironment()));
                    if (pDocument->HasEditorKit())
                    {
                        if (DoesUseRichClipboard())
                            pDocument->GetEditorKit()->SetUseRichClipboard();
                        else
                            pDocument->GetEditorKit()->ClearUseRichClipboard();
                    }
                    return 1;
                }
                break;

            case M_alwaysShowSelection:
                if (penv->CheckExtensions())
                {
                    SetAlwaysShowSelection(val.ToBool(GetASEnvironment()));
                    return 1;
                }
                break;

            case M_selectionTextColor:
                if (penv->CheckExtensions() && pDocument->HasEditorKit())
                {
                    pDocument->GetEditorKit()->SetActiveSelectionTextColor(val.ToUInt32(penv));
                    return true;
                }
                break;
            case M_selectionBkgColor:
                if (penv->CheckExtensions() && pDocument->HasEditorKit())
                {
                    pDocument->GetEditorKit()->SetActiveSelectionBackgroundColor(val.ToUInt32(penv));
                    return true;
                }
                break;
            case M_inactiveSelectionTextColor:
                if (penv->CheckExtensions() && pDocument->HasEditorKit())
                {
                    pDocument->GetEditorKit()->SetInactiveSelectionTextColor(val.ToUInt32(penv));
                    return true;
                }
                break;
            case M_inactiveSelectionBkgColor:
                if (penv->CheckExtensions() && pDocument->HasEditorKit())
                {
                    pDocument->GetEditorKit()->SetInactiveSelectionBackgroundColor(val.ToUInt32(penv));
                    return true;
                }
                break;
            case M_noAutoSelection:
                if (penv->CheckExtensions())
                {
                    SetNoAutoSelection(val.ToBool(penv));
                    return true;
                }
                break;
            case M_disableIME:
                if (penv->CheckExtensions())
                {
                    SetIMEDisabledFlag(val.ToBool(penv));
                    return true;
                }
            case M_filters:
                {
#ifndef GFC_NO_FXPLAYER_AS_FILTERS
                    GASObject* pobj = val.ToObject(penv);
                    if (pobj && pobj->InstanceOf(penv, penv->GetPrototype(GASBuiltin_Array)))
                    {
                        GFxTextFilter textfilter;
                        bool filterdirty = false;
                        GASArrayObject* parrobj = static_cast<GASArrayObject*>(pobj);
                        for (int i=0; i < parrobj->GetSize(); ++i)
                        {
                            GASValue* arrval = parrobj->GetElementPtr(i);
                            if (arrval)
                            {
                                GASObject* pavobj = arrval->ToObject(penv);
                                if (pavobj && pavobj->InstanceOf(penv, penv->GetPrototype(GASBuiltin_BitmapFilter)))
                                {
                                    GASBitmapFilterObject* pfilterobj = static_cast<GASBitmapFilterObject*>(pavobj);
                                    textfilter.LoadFilterDesc(pfilterobj->GetFilter());
                                    filterdirty = true;
                                }
                            }
                        }
                        if (filterdirty)
                        {
                            SetFilters(textfilter);                                    
                            SetDirtyFlag();
                            SetAcceptAnimMoves(false);
                        }
                    }
#endif  // GFC_NO_FXPLAYER_AS_FILTERS
                    return true;
                }
                break;

            case M_restrict:
                {
                    if (val.IsNull() || val.IsUndefined())
                    {
                        delete pRestrict;
                        pRestrict = NULL;
                    }
                    else
                    {
                        GASString   restrStr = val.ToString(penv);
                        // parse restrict string
                        ParseRestrict(restrStr);
                    }
                    break;
                }

            default:
                break;
        }
        return GFxASCharacter::SetMember(penv, name, val, flags);
    }

    bool ParseRestrict(const GASString& restrStr)
    {
        delete pRestrict;

        if ((pRestrict = new RestrictParams()) == 0)
            return false;
        void* ptr = 0;

        const char *pstr    = restrStr.ToCStr();
        bool negative       = false;
        const char *pestr   = pstr + restrStr.GetSize();
        UInt32 firstChInRange = 0, prevCh = 0;
        for (; pstr < pestr; )
        {
            UInt32 ch = GUTF8Util::DecodeNextChar(&pstr);
            if (ch == '^')
            {
                negative = !negative;
                continue;
            }
            else if (ch == '\\')
            {
                if (pstr >= pestr)
                    break; // safety check
                ch = GUTF8Util::DecodeNextChar(&pstr); // skip slash
            }
            else if (ch == '-')
            {
                firstChInRange = prevCh;
                continue;
            }

            if (!firstChInRange)
                firstChInRange = ch;
            if (ch < firstChInRange)
                ch = firstChInRange;

            // range is firstChInRange..ch
            if (!negative)
                pRestrict->RestrictRanges.SetRange(
                    (SPInt)firstChInRange, 
                    ch - firstChInRange + 1, ptr);
            else
            {
                if (pRestrict->RestrictRanges.Count() == 0)
                    // TODO, ranges do not support full 32-bit (SPInt is used for index)
                    pRestrict->RestrictRanges.SetRange(0, 65536, ptr);

                pRestrict->RestrictRanges.ClearRange(
                    (SPInt)firstChInRange, 
                    ch - firstChInRange + 1);
            }

            prevCh          = ch;
            firstChInRange  = 0;
        }
        return true;
    }

	// Special code for complex objects API (see GFxValue::ObjectInterface::SetPositionInfo)
    void    GetPosition(GFxValue::DisplayInfo* pinfo)
    {
        GeomDataType geomData;
        UpdateAndGetGeomData(&geomData);
        Double x = TwipsToPixels(Double(geomData.X));
        Double y = TwipsToPixels(Double(geomData.Y));
        Double rotation = geomData.Rotation;
        Double xscale = geomData.XScale;
        Double yscale = geomData.YScale;
        Double alpha = GetCxform().M_[3][0] * 100.F;
        bool visible = GetVisible();

#ifndef GFC_NO_3D
        Double zscale = geomData.ZScale;
        Double z = geomData.Z;
        Double xrotation = geomData.XRotation;
        Double yrotation = geomData.YRotation;
        pinfo->Set(x, y, rotation, xscale, yscale, alpha, visible, z, xrotation, yrotation, zscale);
#else
        pinfo->Set(x, y, rotation, xscale, yscale, alpha, visible, 0,0,0,0);
#endif
    }

    GASString    GetText(GASEnvironment* penv, bool reqHtml) const 
    {
        if (reqHtml)
        {
            if (HasStyleSheet())
            {
                // return original html if CSS is set
                return penv->CreateString(OriginalTextValue);
            }
            else
            {
                // For non-HTML field, non-HTML text is returned.
                if (IsHtml())
                    return penv->CreateString(pDocument->GetHtml());
                else
                    return penv->CreateString(pDocument->GetText());
            }
        }
        else
        {                   
            return penv->CreateString(pDocument->GetText());
        }
    }

    bool    GetMember(GASEnvironment* penv, const GASString& name, GASValue* pval)
    {
        StandardMember member = GetStandardMemberConstant(name);
        
        // Handle TextField-specific "standard" members.
        // We put this before GetStandardMember so that we can handle _width
        // and _height in a custom way.
        switch(member)
        {
            case M_x:
                {
                    // NOTE: If updating this logic, also update GetPosition() 
                    GeomDataType geomData;
                    UpdateAndGetGeomData(&geomData);
                    pval->SetNumber(TwipsToPixels(Double(geomData.X)));
                    return true;
                }
            case M_y:
                {
                    // NOTE: If updating this logic, also update GetPosition() 
                    GeomDataType geomData;
                    UpdateAndGetGeomData(&geomData);
                    pval->SetNumber(TwipsToPixels(Double(geomData.Y)));
                    return true;
                }

            case M_html:
                {               
                    pval->SetBool(IsHtml());
                    return true;
                }

            case M_htmlText:
                {
                    if (HasStyleSheet())
                    {
                        // return original html if CSS is set
                        pval->SetString(penv->CreateString(OriginalTextValue));
                    }
                    else
                    {
                        // For non-HTML field, non-HTML text is returned.
                        if (IsHtml())
                            pval->SetString(penv->CreateString(pDocument->GetHtml()));
                        else
                            pval->SetString(penv->CreateString(pDocument->GetText()));
                    }
                    return true;
                }

            case M_text:
                {                   
                    pval->SetString(penv->CreateString(pDocument->GetText()));
                    return true;
                }

            case M_textColor:
                {
                    // Return color in 0xRRGGBB format
                    const GFxTextFormat* ptf = pDocument->GetDefaultTextFormat();
                    GASSERT(ptf);
                    GColor c = ptf->GetColor();
                    UInt textColor = (((UInt32)c.GetRed()) << 16) | (((UInt32)c.GetGreen()) << 8) | ((UInt32)c.GetBlue());
                    pval->SetInt(textColor);
                    return true;
                }

            case M_textWidth:           
                {
                    // Return the width, in pixels, of the text as laid out.
                    // (I.E. the actual text content, not our defined
                    // bounding box.)
                    //
                    // In local coords.  Verified against Macromedia Flash.
                    pval->SetNumber((GASNumber)int(TwipsToPixels(pDocument->GetTextWidth())));
                    return true;
                }

            case M_textHeight:              
                {
                    // Return the height, in pixels, of the text as laid out.
                    // (I.E. the actual text content, not our defined
                    // bounding box.)
                    //
                    // In local coords.  Verified against Macromedia Flash.
                    pval->SetNumber((GASNumber)int(TwipsToPixels(pDocument->GetTextHeight())));
                    return true;
                }

            case M_length:
                {
                    pval->SetNumber((GASNumber)pDocument->GetLength());
                    return true;
                }

            case M_autoSize:
                {
                    if (!IsAutoSize())
                        pval->SetString(penv->CreateConstString("none"));
                    else
                    {
                        switch(pDocument->GetAlignment())
                        {
                        case GFxTextDocView::Align_Left:
                                pval->SetString(penv->CreateConstString("left")); 
                                break;
                        case GFxTextDocView::Align_Right:
                                pval->SetString(penv->CreateConstString("right")); 
                                break;
                        case GFxTextDocView::Align_Center:
                                pval->SetString(penv->CreateConstString("center")); 
                                break;
                        default: pval->SetUndefined();
                        }
                    }
                    return true;
                }
            case M_wordWrap:
                {
                    pval->SetBool(pDocument->IsWordWrap());
                    return true;
                }
            case M_multiline:
                {
                    pval->SetBool(pDocument->IsMultiline());
                    return true;
                }
            case M_border:
                {
                    pval->SetBool(BorderColor.GetAlpha() != 0);
                    return true;
                }
            case M_variable:
                {
                    if (!VariableName.IsEmpty())
                        pval->SetString(VariableName);
                    else
                        pval->SetNull();
                    return true;
                }
            case M_selectable:
                {
                    pval->SetBool(IsSelectable());
                    return true;
                }
            case M_embedFonts:
                {
                    pval->SetBool(!pDocument->DoesUseDeviceFont());
                    return true;
                }
            case M_password:
                {
                    pval->SetBool(IsPassword());
                    return true;
                }

            case M_shadowStyle:
                {
                    if (!penv->CheckExtensions())
                        break;

                    pval->SetString(pShadow ? pShadow->ShadowStyleStr : penv->GetBuiltin(GASBuiltin_empty_));
                    return true;
                }
            case M_shadowColor:
                if (penv->CheckExtensions())
                {
                    pval->SetInt(pDocument->GetShadowColor());
                    return true;
                }
                break;

            case M_hscroll:
                {
                    pval->SetNumber((GASNumber)TwipsToPixels(pDocument->GetHScrollOffset()));    
                    return 1;
                }
            case M_scroll:
                {
                    pval->SetNumber((GASNumber)(pDocument->GetVScrollOffset() + 1));    
                    return 1;
                }
            case M_maxscroll:
                {
                    pval->SetNumber((GASNumber)(pDocument->GetMaxVScroll() + 1));    
                    return 1;
                }
            case M_maxhscroll:
                {
                    pval->SetNumber((GASNumber)(TwipsToPixels(pDocument->GetMaxHScroll())));    
                    return 1;
                }
            case M_background:
                {
                    pval->SetBool(BackgroundColor.GetAlpha() != 0);
                    return 1;
                }
            case M_backgroundColor:
                {
                    pval->SetNumber(GASNumber(BackgroundColor.ToColor32() & 0xFFFFFFu));
                    return 1;
                }
            case M_borderColor:
                {
                    pval->SetNumber(GASNumber(BorderColor.ToColor32() & 0xFFFFFFu));
                    return 1;
                }
            case M_bottomScroll:
                {
                    pval->SetNumber((GASNumber)(pDocument->GetBottomVScroll() + 1));
                    return 1;
                }
            case M_type:
                {
                    pval->SetString(IsReadOnly() ? 
                        penv->CreateConstString("dynamic") : penv->CreateConstString("input")); 
                    return 1;
                }
            case M_maxChars:
                {
                    if (!pDocument->HasMaxLength())
                        pval->SetNull();
                    else
                        pval->SetNumber(GASNumber(pDocument->GetMaxLength()));
                    return 1;
                }
            case M_condenseWhite:
                {
                    pval->SetBool(IsCondenseWhite());
                    return 1;
                }

            case M_antiAliasType:
                {
                    pval->SetString(pDocument->IsAAForReadability() ? 
                        penv->CreateConstString("advanced") : penv->CreateConstString("normal"));
                    return 1;
                }
            case M_mouseWheelEnabled:
                {
                    pval->SetBool(IsMouseWheelEnabled());
                    return true;
                }
            case M_styleSheet:
                {
                    pval->SetUndefined();
#ifndef GFC_NO_CSS_SUPPORT
                    if (pCSSData && pCSSData->pASStyleSheet.GetPtr())
                        pval->SetAsObject(pCSSData->pASStyleSheet);
#endif //GFC_NO_CSS_SUPPORT
                    return true;
                }

             // extension
            case M_hitTestDisable:
                if (GetASEnvironment()->CheckExtensions())
                {
                    pval->SetBool(IsHitTestDisableFlagSet());
                    return 1;
                }
                break;

            case M_noTranslate:
                if (GetASEnvironment()->CheckExtensions())
                {
                    pval->SetBool(IsNoTranslate());
                    return 1;
                }
                break;

            case M_caretIndex:
                if (penv->CheckExtensions())
                {
                    if (pDocument->HasEditorKit())
                        pval->SetNumber(GASNumber(pDocument->GetEditorKit()->GetCursorPos()));
                    else
                        pval->SetNumber(-1);
                    return 1;
                }
                break;
            case M_selectionBeginIndex:
                if (penv->CheckExtensions())
                {
                    if (IsSelectable() && pDocument->HasEditorKit())
                        pval->SetNumber(GASNumber(GetBeginIndex()));
                    else
                        pval->SetNumber(-1);
                    return 1;
                }
                break;
            case M_selectionEndIndex:
                if (penv->CheckExtensions())
                {
                    if (IsSelectable() && pDocument->HasEditorKit())
                        pval->SetNumber(GASNumber(GetEndIndex()));
                    else
                        pval->SetNumber(-1);
                    return 1;
                }
                break;
            case M_autoFit:
                if (penv->CheckExtensions())
                {
                    pval->SetBool(pDocument->IsAutoFit());
                    return true;
                }
                break;

            case M_blurX:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetBlurX()));
                    return true;
                }
                break;

            case M_blurY:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetBlurY()));
                    return true;
                }
                break;

            case M_blurStrength:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetBlurStrength()));
                    return true;
                }
                break;

            case M_outline:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetOutline()));
                    return true;
                }
                break;

            case M_fauxBold:
                if (penv->CheckExtensions())
                {
                    pval->SetBool(pDocument->GetFauxBold());
                    return true;
                }
                break;

            case M_fauxItalic:
                if (penv->CheckExtensions())
                {
                    pval->SetBool(pDocument->GetFauxItalic());
                    return true;
                }
                break;


            case M_shadowAlpha:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetShadowAlpha()));
                    return true;
                }
                break;

            case M_shadowAngle:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetShadowAngle()));
                    return true;
                }
                break;

            case M_shadowBlurX:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetShadowBlurX()));
                    return true;
                }
                break;

            case M_shadowBlurY:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetShadowBlurY()));
                    return true;
                }
                break;

            case M_shadowDistance:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetShadowDistance()));
                    return true;
                }
                break;

            case M_shadowHideObject:
                if (penv->CheckExtensions())
                {
                    pval->SetBool(pDocument->IsHiddenObject());
                    return true;
                }
                break;

            case M_shadowKnockOut:
                if (penv->CheckExtensions())
                {
                    pval->SetBool(pDocument->IsKnockOut());
                    return true;
                }
                break;

            case M_shadowQuality:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetShadowQuality()));
                    return true;
                }
                break;

            case M_shadowStrength:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetShadowStrength()));
                    return true;
                }
                break;

            case M_shadowOutline:
                // Not implemented
                //if (penv->CheckExtensions())
                //{
                //    return true;
                //}
                break;

            case M_numLines:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(GASNumber(pDocument->GetLinesCount()));
                    return true;
                }
                break;

            case M_verticalAutoSize:
                if (penv->CheckExtensions())
                {
                    if (!pDocument->IsAutoSizeY())
                        pval->SetString(penv->CreateConstString("none"));
                    else
                    {
                        switch(pDocument->GetVAlignment())
                        {
                        case GFxTextDocView::VAlign_None:
                            pval->SetString(penv->CreateConstString("none")); 
                            break;
                        case GFxTextDocView::VAlign_Top:
                            pval->SetString(penv->CreateConstString("top")); 
                            break;
                        case GFxTextDocView::VAlign_Bottom:
                            pval->SetString(penv->CreateConstString("bottom")); 
                            break;
                        case GFxTextDocView::VAlign_Center:
                            pval->SetString(penv->CreateConstString("center")); 
                            break;
                        default: pval->SetUndefined();
                        }
                    }
                    return true;
                }
                break;

            case M_fontScaleFactor:
                if (penv->CheckExtensions())
                {
                    pval->SetNumber(pDocument->GetFontScaleFactor());
                }
                break;

            case M_verticalAlign:
                if (penv->CheckExtensions())
                {
                    switch(pDocument->GetVAlignment())
                    {
                    case GFxTextDocView::VAlign_None:
                        pval->SetString(penv->CreateConstString("none")); 
                        break;
                    case GFxTextDocView::VAlign_Top:
                        pval->SetString(penv->CreateConstString("top")); 
                        break;
                    case GFxTextDocView::VAlign_Bottom:
                        pval->SetString(penv->CreateConstString("bottom")); 
                        break;
                    case GFxTextDocView::VAlign_Center:
                        pval->SetString(penv->CreateConstString("center")); 
                        break;
                    default: pval->SetUndefined();
                    }
                    return true;
                }
                break;

            case M_textAutoSize:
                if (penv->CheckExtensions())
                {
                    switch(pDocument->GetTextAutoSize())
                    {
                    case GFxTextDocView::TAS_Shrink:
                        pval->SetString(penv->CreateConstString("shrink")); 
                        break;
                    case GFxTextDocView::TAS_Fit:
                        pval->SetString(penv->CreateConstString("fit")); 
                        break;
                    default:
                        pval->SetString(penv->CreateConstString("none")); 
                        break;
                    }
                }
                break;

            case M_useRichTextClipboard:
                if (penv->CheckExtensions())
                {
                    pval->SetBool(DoesUseRichClipboard());
                }
                break;

            case M_alwaysShowSelection:
                if (penv->CheckExtensions())
                {
                    pval->SetBool(DoesAlwaysShowSelection());
                }
                break;

            case M_selectionTextColor:
                if (penv->CheckExtensions() && pDocument->HasEditorKit())
                {
                    pval->SetNumber(GASNumber(pDocument->GetEditorKit()->GetActiveSelectionTextColor()));
                    return true;
                }
                break;
            case M_selectionBkgColor:
                if (penv->CheckExtensions() && pDocument->HasEditorKit())
                {
                    pval->SetNumber(GASNumber(pDocument->GetEditorKit()->GetActiveSelectionBackgroundColor()));
                    return true;
                }
                break;
            case M_inactiveSelectionTextColor:
                if (penv->CheckExtensions() && pDocument->HasEditorKit())
                {
                    pval->SetNumber(GASNumber(pDocument->GetEditorKit()->GetInactiveSelectionTextColor()));
                    return true;
                }
                break;
            case M_inactiveSelectionBkgColor:
                if (penv->CheckExtensions() && pDocument->HasEditorKit())
                {
                    pval->SetNumber(GASNumber(pDocument->GetEditorKit()->GetInactiveSelectionBackgroundColor()));
                    return true;
                }
                break;
            case M_noAutoSelection:
                if (penv->CheckExtensions())
                {
                    pval->SetBool(IsNoAutoSelection());
                    return true;
                }
                break;
            case M_disableIME:
                if (penv->CheckExtensions())
                {
                    pval->SetBool(IsIMEDisabledFlag());
                    return true;
                }
                break;
            case M_filters:
                {
                    pval->SetUndefined();
#ifndef GFC_NO_FXPLAYER_AS_FILTERS
                    // Not implemented because GFxTextFilter does not distinguish
                    // between drop shadow and glow filters
                    GFxLog* plog = penv->GetLog();
                    if (plog)
                        plog->LogWarning("Error: Retrieval of the TextField.filters property is not implemented.");
#endif  // GFC_NO_FXPLAYER_AS_FILTERS
                    return true;
                }

            default: 
                if (GetStandardMember(member, pval, 0))
                    return true;
                break;
        }

        if (ASTextFieldObj)
        {
            return ASTextFieldObj->GetMember(penv, name, pval);
        }
        else 
        {
            // Handle the __proto__ property here, since we are going to 
            // zero out it temporarily (see comments below).
            if (penv && name == penv->GetBuiltin(GASBuiltin___proto__))
            {
                GASObject* proto = Get__proto__();
                pval->SetAsObject(proto);
                return true;
            }

            // Now we can search in the __proto__
            GASObject* proto = Get__proto__();
            if (proto)
            {
                // ASMovieClipObj is not created yet; use __proto__ instead
                if (proto->GetMember(penv, name, pval))    
                {
                    return true;
                }
            }
        }
        // looks like _global is accessable from any character
        if (name == "_global" && penv)
        {
            pval->SetAsObject(penv->GetGC()->pGlobal);
            return true;
        }
        return false;
    }

    // Return the topmost entity that the given point covers.  NULL if none.
    // I.E. check against ourself.
    virtual GFxASCharacter* GetTopMostMouseEntity(const GPointF &pt, const TopMostParams& params)
    {
        if (IsHitTestDisableFlagSet() || !GetVisible())
            return 0;

        if (params.IgnoreMC == this)
            return 0;

        if (!IsFocusAllowed(params.pRoot, params.ControllerIdx))
            return 0;

        GRenderer::Matrix   m = GetMatrix();
        GPointF             p;

#ifndef GFC_NO_3D
        if (Is3D(true))
        {
            const GMatrix3D     *pPersp = GetPerspective3D(true);
            const GMatrix3D     *pView = GetView3D(true);
            if (pPersp)
                GetMovieRoot()->ScreenToWorld.SetPerspective(*pPersp);
            if (pView)
                GetMovieRoot()->ScreenToWorld.SetView(*pView);
            GetMovieRoot()->ScreenToWorld.SetWorld(GetWorldMatrix3D());
            GetMovieRoot()->ScreenToWorld.GetWorldPoint(&p);
        }
        else
#endif
        {
            m.TransformByInverse(&p, pt);
        }

        if ((ClipDepth == 0) && PointTestLocal(p, HitTest_TestShape))
        {
            if (params.TestAll || IsSelectable())
            {
                return this;
            }
            else if (!IsSelectable() && IsHtml() && 
                     pDocument->MayHaveUrl() && pDocument->IsUrlAtPoint(p.x, p.y))
            {
                // if not selectable, will need to check for url under the
                // mouse cursor. Return "this", if there is an url under 
                // the mouse cursor.
                return this;
            }
            else
            {
                GFxASCharacter* pparent = GetParent();
                while (pparent && pparent->IsSprite())
                {
                    // Parent sprite would have to be in button mode to grab attention.
                    GFxSprite * psprite = (GFxSprite*)pparent;
                    if (params.TestAll || psprite->ActsAsButton())
                    {
                        // Check if sprite should be ignored
                        if (!params.IgnoreMC || (psprite != params.IgnoreMC))
                            return psprite;
                    }
                    pparent = pparent->GetParent ();
                }
            }
        }

        return NULL;
    }

    bool    IsTabable() const
    {
        //return GetVisible() && !IsReadOnly() && !IsTabEnabledFlagFalse();
        if (GetVisible() && !IsReadOnly())
        {
            if (!IsTabEnabledFlagDefined())
            {
                GASObject* pproto = Get__proto__();
                if (pproto)
                {
                    // check prototype for tabEnabled
                    GASValue val;
                    const GASEnvironment* penv = GetASEnvironment();
                    if (pproto->GetMemberRaw(penv->GetSC(), penv->CreateConstString("tabEnabled"), &val))
                    {
                        if (!val.IsUndefined())
                            return val.ToBool(penv);
                    }
                }
            }
            return !IsTabEnabledFlagFalse();
        }
        return false;
    }

    bool    IsFocusRectEnabled() const { return false; }

    bool    IsFocusEnabled() const 
    { 
        // Flash allows to set focus on read-only non-selectable text fields.
        return true; // !IsReadOnly() || IsSelectable();
    }

    bool    DoesAcceptMouseFocus() const { return !IsReadOnly() || IsSelectable(); }

    UInt    GetCursorType() const 
    {
        if (IsHandCursor())
            return GFxMouseCursorEvent::HAND;
        if (IsSelectable())
            return GFxMouseCursorEvent::IBEAM;
        return GFxASCharacter::GetCursorType();
    }

    bool IsUrlUnderMouseCursor(UInt mouseIndex, GPointF* pPnt = NULL, GRange* purlRangePos = NULL)
    {
        // Local coord of mouse IN PIXELS.
        GFxMovieRoot* proot = GetMovieRoot();
        if (!proot)
            return false;
        GASSERT(proot->GetMouseState(mouseIndex));
        GPointF a = proot->GetMouseState(mouseIndex)->GetLastPosition();

        Matrix  m = GetWorldMatrix();
        GPointF b;

        m.TransformByInverse(&b, a);
        if (pPnt) *pPnt = b;

        return pDocument->IsUrlAtPoint(b.x, b.y, purlRangePos);
    }

    bool IsUrlTheSame(UInt mouseIndex, const GRange& urlRange)
    {
        bool rv = true;
#ifndef GFC_NO_CSS_SUPPORT
        if (pCSSData)
        {
            for (UInt i = 0, zonesNum = (UInt)pCSSData->UrlZones.Count(); i < zonesNum; ++i)
            {
                if (pCSSData->UrlZones[i].Intersects(urlRange))
                {
                    if (pCSSData->MouseState[mouseIndex].UrlZoneIndex != i + 1)
                    {
                        rv = false;
                        break;
                    }
                }
            }
        }
#else
        GUNUSED2(mouseIndex, urlRange);
#endif
        return rv;
    }

    void CollectUrlZones()
    {
#ifndef GFC_NO_CSS_SUPPORT
        if (pCSSData)
        {
            memset(pCSSData->MouseState, 0, sizeof(pCSSData->MouseState));
            pCSSData->UrlZones.RemoveAll();
            GFxStyledText* pstyledText = pDocument->GetStyledText();
            GString currentUrl;
            UPInt startPos = 0, length = 0;
            UInt n = pstyledText->GetParagraphsCount();
            for (UInt i = 0; i < n; ++i)
            {
                const GFxTextParagraph* ppara = pstyledText->GetParagraph(i);
                GFxTextParagraph::FormatRunIterator it = ppara->GetIterator();
                for (; !it.IsFinished(); ++it)
                {
                    UPInt indexInDoc = (UPInt)it->Index + ppara->GetStartIndex();
                    if (it->pFormat->IsUrlSet())
                    {
                        if (!currentUrl.IsEmpty())
                        {
                            if (indexInDoc != startPos + length || currentUrl != it->pFormat->GetUrl())
                            {
                                CSSHolder::UrlZone urlZone;
                                urlZone.SavedFmt = 
                                    *pDocument->GetStyledText()->CopyStyledText(startPos, startPos + length);
                                pCSSData->UrlZones.InsertRange(startPos, length, urlZone);
                                currentUrl.Clear();
                            }
                            else
                            {
                                length += it->Length;
                            }
                        }
                        if (currentUrl.IsEmpty())
                        {
                            startPos = indexInDoc;
                            length   = it->Length;
                            currentUrl = it->pFormat->GetUrl();
                        }
                    }
                }
            }
            if (!currentUrl.IsEmpty())
            {
                CSSHolder::UrlZone urlZone;
                urlZone.SavedFmt = 
                    *pDocument->GetStyledText()->CopyStyledText(startPos, startPos + length);
                pCSSData->UrlZones.InsertRange(startPos, length, urlZone);
            }
        }
#endif // GFC_NO_CSS_SUPPORT
    }

    enum LinkEvent
    {
        Link_press,
        Link_release,
        Link_rollover,
        Link_rollout
    };
    // Changes format of link according to event.
    void    ChangeUrlFormat(LinkEvent event, UInt mouseIndex, const GRange* purlRange = NULL);
    // Updates styles of links after new style sheet is installed.
    void    UpdateUrlStyles();

    void    PropagateMouseEvent(const GFxEventId& id)
    {
        GFxMovieRoot* proot = GetMovieRoot();
        if (!proot)
            return;
        
        if (id.Id == GFxEventId::Event_MouseMove)
        {
            // Implement mouse-drag.
            GFxASCharacter::DoMouseDrag();
        }

        GPtr<GFxASCharacter> ptopMostChar = proot->GetMouseState(id.MouseIndex)->GetTopmostEntity();

        // if ptopMostChar != thus - means that the cursor is out of the URL => restore URL fmt
        if (ptopMostChar != this && HasStyleSheet() && IsHtml() && pDocument->MayHaveUrl())
        {
            ChangeUrlFormat(Link_release, id.MouseIndex);
            ChangeUrlFormat(Link_rollout, id.MouseIndex);
        }

        if (ptopMostChar == this ||
           (pDocument->HasEditorKit() && pDocument->GetEditorKit()->IsMouseCaptured()))
        {
            if (!GetVisible())  
                return;
            switch(id.Id)
            {
            case GFxEventId::Event_MouseDown:
                {
#ifndef GFC_NO_CSS_SUPPORT
                    if (HasStyleSheet() && IsHtml() && pDocument->MayHaveUrl())
                    {
                        const GFxMouseState* pms = proot->GetMouseState(id.MouseIndex);
                        GASSERT(pms);

                        GRange urlRange;
                        if (IsUrlUnderMouseCursor(id.MouseIndex, NULL, &urlRange) && 
                            pCSSData && pCSSData->pASStyleSheet && (pms->GetButtonsState() & GFxMouseState::MouseButton_Left))
                        {
                            // apply CSS style - a:active
                            ChangeUrlFormat(Link_press, id.MouseIndex, &urlRange);
                        }
                    }
#endif // GFC_NO_CSS_SUPPORT
                    if (pDocument->HasEditorKit())
                    {
                        // Local coord of mouse IN PIXELS.
                        const GFxMouseState* pms = proot->GetMouseState(id.MouseIndex);
                        GASSERT(pms);

                        Matrix  m = GetWorldMatrix();
                        GPointF b;

                        m.TransformByInverse(&b, pms->GetLastPosition());
                        pDocument->GetEditorKit()->OnMouseDown(b.x, b.y, pms->GetButtonsState());

                        ModifyOptimizedPlayListLocal<GFxEditTextCharacter>(proot);
                    }
                }
                break;
            case GFxEventId::Event_MouseUp:
                {
#ifndef GFC_NO_CSS_SUPPORT
                    if (HasStyleSheet() && IsHtml() && pDocument->MayHaveUrl())
                    {
                        const GFxMouseState* pms = proot->GetMouseState(id.MouseIndex);
                        GASSERT(pms);

                        GRange urlRange;
                        if (IsUrlUnderMouseCursor(id.MouseIndex, NULL, &urlRange) && 
                            pCSSData && pCSSData->pASStyleSheet && !(pms->GetButtonsState() & GFxMouseState::MouseButton_Left))
                        {
                            // apply CSS style - a:active
                            ChangeUrlFormat(Link_release, id.MouseIndex, &urlRange);
                        }
                        ChangeUrlFormat(Link_release, id.MouseIndex);
                    }
#endif // GFC_NO_CSS_SUPPORT
                    if (pDocument->HasEditorKit())
                    {
                        // Local coord of mouse IN PIXELS.
                        const GFxMouseState* pms = proot->GetMouseState(id.MouseIndex);
                        GASSERT(pms);

                        Matrix  m = GetWorldMatrix();
                        GPointF b;

                        m.TransformByInverse(&b, pms->GetLastPosition());
                        pDocument->GetEditorKit()->OnMouseUp(b.x, b.y, pms->GetButtonsState());

                        ModifyOptimizedPlayListLocal<GFxEditTextCharacter>(proot);
                    }
                }
                break;
            case GFxEventId::Event_MouseMove:
                {
                    const GFxMouseState* pms = proot->GetMouseState(id.MouseIndex);
                    GASSERT(pms);
                    if (pDocument->HasEditorKit())
                    {
                        // Local coord of mouse IN PIXELS.

                        Matrix  m = GetWorldMatrix();
                        GPointF b;

                        m.TransformByInverse(&b, pms->GetLastPosition());
                        pDocument->GetEditorKit()->OnMouseMove(b.x, b.y);
                    }
#ifndef GFC_NO_CSS_SUPPORT
                    if (HasStyleSheet() && IsHtml() && pDocument->MayHaveUrl())
                    {
                        GRange urlRange;
                        bool urlIsUnderCursor = IsUrlUnderMouseCursor(id.MouseIndex, NULL, &urlRange);

                        // apply CSS style - either a:active or a:hover
                        if (urlIsUnderCursor)
                        {
                            if (!IsUrlTheSame(id.MouseIndex, urlRange))
                            {
                                ChangeUrlFormat(Link_release, id.MouseIndex);
                                ChangeUrlFormat(Link_rollout, id.MouseIndex);
                            }
                            ChangeUrlFormat(
                                (pms->GetButtonsState() & GFxMouseState::MouseButton_Left) ? Link_press : Link_rollover, 
                                id.MouseIndex, &urlRange);
                        }
                        else
                        {
                            ChangeUrlFormat(Link_release, id.MouseIndex);
                            ChangeUrlFormat(Link_rollout, id.MouseIndex);
                        }
                        SetHandCursor(urlIsUnderCursor);
                        proot->ChangeMouseCursorType(id.MouseIndex, GetCursorType());
                    }
                    else 
#endif // GFC_NO_CSS_SUPPORT
                    {
                        if (IsHtml() && pDocument->MayHaveUrl())
                        {
                            SetHandCursor(IsUrlUnderMouseCursor(id.MouseIndex));
                            proot->ChangeMouseCursorType(id.MouseIndex, GetCursorType());
                        }
                        else if (IsHandCursor())
                        {
                            ChangeUrlFormat(Link_rollout, id.MouseIndex);
                            ClearHandCursor();
                            proot->ChangeMouseCursorType(id.MouseIndex, GetCursorType());
                        }
                    }
                }
                break;
            default: break;
            }
        }
        GFxASCharacter::PropagateMouseEvent(id);
    }

    // handle Event_MouseDown to grab focus
    virtual bool OnButtonEvent(const GFxEventId& event)
    { 
        if (event.Id == GFxEventId::Event_Release)
        {
            // if url is clicked - execute the action
            if (IsHtml() && pDocument->MayHaveUrl())
            {
                GPointF p;
                if (IsUrlUnderMouseCursor(event.MouseIndex, &p))
                {
                    UPInt pos = pDocument->GetCharIndexAtPoint(p.x, p.y);
                    if (!G_IsMax(pos))
                    {
                        const GFxTextFormat* ptextFmt;
                        if (pDocument->GetTextAndParagraphFormat(&ptextFmt, NULL, pos))
                        {
                            if (ptextFmt->IsUrlSet())
                            {
                                const char* url = ptextFmt->GetUrl();
                                
                                // url should represent link in asfunction protocol:
                                // "asfunction:funcName, parametersString"
                                // NOTE: parametersString is not parsed.
                                GFxMovieRoot* proot = GetMovieRoot();
                                if (proot && GString::CompareNoCase(url, "asfunction:", sizeof("asfunction:") - 1) == 0)
                                {
                                    url += sizeof("asfunction:") - 1;
                                    // now get the function's name
                                    const char* pnameEnd = strchr(url, ',');
                                    GString funcName;
                                    GFxValue param;
                                    UInt paramCnt = 0;
                                    if (pnameEnd)
                                    {
                                        funcName.AppendString(url, pnameEnd - url);
                                        ++pnameEnd;
                                        paramCnt = 1;
                                        param.SetString(pnameEnd);
                                    }
                                    else
                                        funcName = url;
                                    GFxValue result;
                                    GPtr<GFxASCharacter> par = GetParent();
                                    GFxSprite* parSpr = NULL;
                                    if (par)
                                    {
                                        parSpr = par->ToSprite();
                                    }
                                    if (parSpr)
                                        proot->Invoke(parSpr, funcName, &result, &param, paramCnt);
                                    else
                                        proot->Invoke(funcName, &result, &param, paramCnt);
                                }
                                //printf ("%s\n", url);
                            }
                        }
                    }
                }
            }

            return true;
        }
        return false; 
    }

    virtual bool   IsFocusAllowed(GFxMovieRoot* proot, UInt controllerIdx) const
    {
        return (FocusedControllerIdx == ~0u || FocusedControllerIdx == controllerIdx) && 
            GFxASCharacter::IsFocusAllowed(proot, controllerIdx);
    }

    virtual bool   IsFocusAllowed(GFxMovieRoot* proot, UInt controllerIdx)
    {
        return (FocusedControllerIdx == ~0u || FocusedControllerIdx == controllerIdx) && 
            GFxASCharacter::IsFocusAllowed(proot, controllerIdx);
    }

    void OnFocus(FocusEventType event, GFxASCharacter* oldOrNewFocusCh, UInt controllerIdx, GFxFocusMovedType fmt)
    {
        if (IsSelectable())
        {
            // manage the selection
            if (event == GFxASCharacter::SetFocus)
            {
                if (!IsNoAutoSelection() && fmt != GFx_FocusMovedByMouse)
                {
                    UPInt len = pDocument->GetLength();
                    SetSelection(0, (SPInt)len);
                }
                FocusedControllerIdx = controllerIdx;
                if (pDocument->HasEditorKit())
                {
                    pDocument->GetEditorKit()->OnSetFocus();
                    SetDirtyFlag();
                }
            }
            else if (event == GFxASCharacter::KillFocus)
            {
                FocusedControllerIdx = ~0u;
                if (pDocument->HasEditorKit())
                {
                    if (!DoesAlwaysShowSelection())
                        pDocument->GetEditorKit()->ClearSelection();

                    pDocument->GetEditorKit()->OnKillFocus();
                    SetDirtyFlag();
                }
            }
        }
        if (!IsReadOnly() || IsSelectable())
        {
            if (event == GFxASCharacter::SetFocus)
            {
                ResetBlink(1, 1);
            }

            GFxASCharacter::OnFocus(event, oldOrNewFocusCh, controllerIdx, fmt);

            if (pDocument->HasEditorKit() && pDocument->GetEditorKit()->HasCursor())
            {
                SetForceAdvance();
                ModifyOptimizedPlayListLocal<GFxEditTextCharacter>(GetMovieRoot());
            }
        }
    }

    bool OnLosingKeyboardFocus(GFxASCharacter*, UInt , GFxFocusMovedType fmt) 
    {
        if (fmt == GFx_FocusMovedByMouse)
        {
            //!AB: Not sure, is this case still actual or not.
            // Though, this "if" may lead to incorrect behavior if the text field
            // hides itself in onPress handler: mouseCaptured flag won't be cleared
            // since no OnMouseUp will be received.
            //if (pDocument->HasEditorKit() && pDocument->GetEditorKit()->IsMouseCaptured())
            //    return false; // prevent focus loss, if mouse is captured
        }
        return true;
    }

    void UpdateVariable()
    {
        if (!VariableName.IsEmpty())
        {
            GASEnvironment* penv = GetASEnvironment();
            if (penv)
            {
                VariableVal.SetString(penv->CreateString(pDocument->GetText()));
                penv->SetVariable(VariableName, VariableVal);
            }
        }
    }

    void NotifyChanged()
    {
        GASEnvironment* penv = GetASEnvironment();

        int nargs = 1;
        if (penv->CheckExtensions() && FocusedControllerIdx != ~0u)
        {
            penv->Push(int(FocusedControllerIdx));
            ++nargs;
        }
        penv->Push(GASValue(this));
        GASAsBroadcaster::BroadcastMessage(penv, this, penv->CreateConstString("onChanged"), nargs, penv->GetTopIndex());
        penv->Drop(nargs);
    }

    virtual GRectF  GetBounds(const Matrix &t) const
    {
        return t.EncloseTransform(pDocument->GetViewRect());
    }

    virtual bool    PointTestLocal(const GPointF &pt, UInt8 hitTestMask = 0) const
    {
        if (IsHitTestDisableFlagSet())
            return false;
        if ((hitTestMask & HitTest_IgnoreInvisible) && !GetVisible())
            return false;
        // Flash only uses the bounding box for text - even if shape flag is used
        return pDocument->GetViewRect().Contains(pt);
    }

    // Draw the dynamic string.
    void    Display(GFxDisplayContext &context)   
    {    
        GRenderer* prenderer = context.GetRenderer();
        if (!prenderer)
            return;

        //UpdateTextFromVariable (); // this is necessary for text box with binded variables. (??)
        
        GRenderer::Matrix   mat;
        
#ifndef GFC_NO_3D
        if (context.Is3D == false)
#endif
        {
            mat = *context.pParentMatrix;         
            mat.Prepend(GetMatrix());
        }
        GRenderer::Cxform   cx = *context.pParentCxform;
        cx.Concatenate(GetCxform());

        GFxMovieRoot* proot = GetMovieRoot();
        if (!proot)
            return;

#ifndef GFC_NO_3D
        GMatrix3D *poldmatrix3D = context.pParentMatrix3D;
        bool bOld3D = context.Is3D;
        GMatrix3D worldMat3D = poldmatrix3D ? *poldmatrix3D : GMatrix3D::Identity;
        if (context.Is3D)
        {
            worldMat3D.Prepend(GetLocalMatrix3D());
            context.GetRenderer()->SetWorld3D(&worldMat3D);
            context.pParentMatrix3D = &worldMat3D;
            mat = GMatrix2D::Identity;
        }
#endif

        // Do viewport culling if bounds are available (and not a 3D object)
        if (
#ifndef GFC_NO_3D
            context.pParentMatrix3D == false && 
#endif
            !proot->GetVisibleFrameRectInTwips().Intersects(mat.EncloseTransform(pDocument->GetViewRect())))
            if (!(context.GetRenderFlags() & GFxRenderConfig::RF_NoViewCull))
                return;

        // If Alpha is Zero, don't draw text.        
        if ((fabs(cx.M_[3][0]) < 0.001f) &&
            (fabs(cx.M_[3][1]) < 1.0f) )
            return;

        // Show white background + black bounding box.
        if (BorderColor.GetAlpha() > 0 || BackgroundColor.GetAlpha() > 0)
        {        
            static const UInt16   indices[6] = { 0, 1, 2, 2, 1, 3 };

            // Show white background + black bounding box.
            prenderer->SetCxform(cx);
            Matrix m(mat);

            GRectF newRect;
            GFx_RecalculateRectToFit16Bit(m, pDocument->GetViewRect(), &newRect);
            prenderer->SetMatrix(m);

            GPointF         coords[4];
            coords[0] = newRect.TopLeft();
            coords[1] = newRect.TopRight();
            coords[2] = newRect.BottomLeft();
            coords[3] = newRect.BottomRight();

            GRenderer::VertexXY16i icoords[4];
            icoords[0].x = (SInt16) coords[0].x;
            icoords[0].y = (SInt16) coords[0].y;
            icoords[1].x = (SInt16) coords[1].x;
            icoords[1].y = (SInt16) coords[1].y;
            icoords[2].x = (SInt16) coords[2].x;
            icoords[2].y = (SInt16) coords[2].y;
            icoords[3].x = (SInt16) coords[3].x;
            icoords[3].y = (SInt16) coords[3].y;

            const SInt16  linecoords[10] = 
            {
                // outline
                (SInt16) coords[0].x, (SInt16) coords[0].y,
                (SInt16) coords[1].x, (SInt16) coords[1].y,
                (SInt16) coords[3].x, (SInt16) coords[3].y,
                (SInt16) coords[2].x, (SInt16) coords[2].y,
                (SInt16) coords[0].x, (SInt16) coords[0].y,
            };
            
            prenderer->FillStyleColor(BackgroundColor);
            prenderer->SetVertexData(icoords, 4, GRenderer::Vertex_XY16i);
            
            // Fill the inside
            prenderer->SetIndexData(indices, 6, GRenderer::Index_16);
            prenderer->DrawIndexedTriList(0, 0, 4, 0, 2);
            prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
            // And draw outline
            prenderer->SetVertexData(linecoords, 5, GRenderer::Vertex_XY16i);
            prenderer->LineStyleColor(BorderColor);
            prenderer->DrawLineStrip(0, 4);
            // Done
            prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
            prenderer->SetIndexData(0, 0, GRenderer::Index_None);
        }

        bool nextFrame = (Flags & Flags_NextFrame) != 0;
        Flags &= ~Flags_NextFrame;

        bool isFocused = proot->IsFocused(this);
        pDocument->PreDisplay(context, mat, cx, isFocused);
        if (pShadow)
        {
            for (UInt i = 0; i < pShadow->ShadowOffsets.GetSize(); i++)
            {
                GMatrix2D   shm(mat);
                GRenderer::Cxform c(cx);
                c.M_[0][0] = c.M_[1][0] = c.M_[2][0] = 0;
                c.M_[0][1] = (Float)(pShadow->ShadowColor.GetRed());   // R
                c.M_[1][1] = (Float)(pShadow->ShadowColor.GetGreen()); // G
                c.M_[2][1] = (Float)(pShadow->ShadowColor.GetBlue());  // B

                shm.PrependTranslation(pShadow->ShadowOffsets[i].x, pShadow->ShadowOffsets[i].y);
                pDocument->Display(context, shm, c, nextFrame);
            }
            
            // Draw our actual text.
            if (pShadow->TextOffsets.GetSize() == 0)
                pDocument->Display(context, mat, cx, nextFrame);
            else
            {
                for (UInt j = 0; j < pShadow->TextOffsets.GetSize(); j++)
                {
                    GMatrix2D   shm(mat);

                    shm.PrependTranslation(pShadow->TextOffsets[j].x, pShadow->TextOffsets[j].y);
                    pDocument->Display(context, shm, cx, nextFrame);
                }
            }
        }
        else
        {
            context.pChar = this;
            pDocument->Display(context, mat, cx, nextFrame);
        }
        if (pDocument->HasEditorKit() && proot->IsFocused(this))
            pDocument->GetEditorKit()->Display(context, mat, cx);

#ifndef GFC_NO_3D
        if (context.Is3D)
            context.GetRenderer()->SetWorld3D(poldmatrix3D);
        context.Is3D = bOld3D;
        context.pParentMatrix3D = poldmatrix3D;
#endif

        DoDisplayCallback();
        pDocument->PostDisplay(context, mat, cx, isFocused);
    }


    void    ResetBlink(bool state = 1, bool blocked = 0)
    {
        if (pDocument->HasEditorKit())  
        {
            pDocument->GetEditorKit()->ResetBlink(state, blocked);
            SetDirtyFlag();
        }
    }

    void SetSelection(SPInt beginIndex, SPInt endIndex)
    {
#ifndef GFC_NO_TEXT_INPUT_SUPPORT
        // Flash allows do selection in read-only non-selectable textfields.
        // To reproduce the same behavior we need to create EditorKit, even, 
        // though read-only non-selectable textfield doesn't need it.
        if (!pDocument->HasEditorKit())
            CreateEditorKit();
        if (pDocument->HasEditorKit())
        {
            beginIndex = G_Max((SPInt)0, beginIndex);
            endIndex   = G_Max((SPInt)0, endIndex);
            UPInt len = pDocument->GetLength();
            beginIndex = G_Min((SPInt)len, beginIndex);
            endIndex   = G_Min((SPInt)len, endIndex);
            pDocument->GetEditorKit()->SetSelection((UPInt)beginIndex, (UPInt)endIndex);
            SetDirtyFlag();
        }
#else
        GUNUSED2(beginIndex, endIndex);
#endif //GFC_NO_TEXT_INPUT_SUPPORT
    }

    SPInt GetCaretIndex()
    {
#ifndef GFC_NO_TEXT_INPUT_SUPPORT
        if (!IsReadOnly() || IsSelectable())
        {
            if (pDocument->HasEditorKit())
            {
                return (SPInt)pDocument->GetEditorKit()->GetCursorPos();
            }
        }
#endif //GFC_NO_TEXT_INPUT_SUPPORT
        return -1;
    }

    UPInt GetBeginIndex()
    {
#ifndef GFC_NO_TEXT_INPUT_SUPPORT
        if ((!IsReadOnly() || IsSelectable()) && pDocument->HasEditorKit())
        {
            return pDocument->GetEditorKit()->GetBeginSelection();
        }
#endif //GFC_NO_TEXT_INPUT_SUPPORT
        return 0;
    }

    UPInt GetEndIndex()
    {
#ifndef GFC_NO_TEXT_INPUT_SUPPORT
        if ((!IsReadOnly() || IsSelectable()) && pDocument->HasEditorKit())
        {
            return pDocument->GetEditorKit()->GetEndSelection();
        }
#endif //GFC_NO_TEXT_INPUT_SUPPORT
        return 0;
    }

#ifndef GFC_NO_KEYBOARD_SUPPORT
    static void KeyProcessing(const GASFnCall& fn)
    {
        GASSERT(fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField);
        GASSERT(fn.NArgs == 2); // Arg(0) = 0 - keydown, 1 - keyup, 2 - char 
                                // Arg(1) = keyCode | (specKeysState << 16) or wchar code

        GFxEditTextCharacter* peditChar = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (peditChar->pDocument->HasEditorKit() && (!peditChar->IsReadOnly() || peditChar->IsSelectable()))
        {
            switch (fn.Arg(0).ToInt32(fn.Env))
            {
            case 0: // keydown
                {
                    int arg = fn.Arg(1).ToInt32(fn.Env);
                    peditChar->pDocument->GetEditorKit()->OnKeyDown
                        (arg & 0xFFFF, GFxSpecialKeysState(UInt8(arg >> 16)));
                    
                    // toggle overwrite/insert modes
                    if ((arg & 0xFFFF) == GFxKey::Insert)
                    {
                        peditChar->SetOverwriteMode(!peditChar->IsOverwriteMode());
                    }
                }
                break;
            case 1: // keyup
                {
                    int arg = fn.Arg(1).ToInt32(fn.Env);
                    peditChar->pDocument->GetEditorKit()->OnKeyUp
                        (arg & 0xFFFF, GFxSpecialKeysState(UInt8(arg >> 16)));
                }
                break;
            case 2: // char
                peditChar->pDocument->GetEditorKit()->OnChar((wchar_t)fn.Arg(1).ToInt32(fn.Env));
                break;
            default: GASSERT(0); break;
            }
        }
    }
    virtual bool    OnKeyEvent(const GFxEventId& id, int* pkeyMask)
    {
        GUNUSED(pkeyMask);

        GFxMovieRoot* proot = GetMovieRoot();
        UInt focusGroup     = proot->GetFocusGroupIndex(id.KeyboardIndex);
        if (!((*pkeyMask) & ((1 << focusGroup) & GFxCharacter::KeyMask_FocusedItemHandledMask)))
        {
            if (proot && proot->IsFocused(this, id.KeyboardIndex))
            {
                if (id.Id == GFxEventId::Event_KeyDown || id.Id == GFxEventId::Event_KeyUp) 
                {
                    // cursor positioning is available for selectable readonly textfields
                    if (pDocument->HasEditorKit() && (!IsReadOnly() || IsSelectable()))
                    {
                        // need to queue up key processing. Flash doesn't handle 
                        // keys from keyevents, it executes them afterward.
                        GASValueArray params;
                        int v = id.KeyCode | (UInt32(id.SpecialKeysState.States) << 16);
                        if (id.Id == GFxEventId::Event_KeyDown)
                            params.PushBack(GASValue(int(0)));
                        else
                            params.PushBack(GASValue(int(1)));
                        params.PushBack(GASValue(v));
                        proot->InsertEmptyAction()->SetAction(this, 
                            GFxEditTextCharacter::KeyProcessing, &params);
                    }
                    (*pkeyMask) |= ((1 << focusGroup) & GFxCharacter::KeyMask_FocusedItemHandledMask);
                    return true;
                }
            }
        }
        return false;
    }

    virtual bool    OnCharEvent(UInt32 wcharCode, UInt controllerIdx)
    {
        // cursor positioning is available for selectable readonly textfields
        GFxMovieRoot* proot = GetMovieRoot();
        if (proot && pDocument->HasEditorKit() && (!IsReadOnly() || IsSelectable()) && FocusedControllerIdx == controllerIdx)
        {
            if (pRestrict)
            {
                if (pRestrict->RestrictRanges.GetIteratorAt(int(wcharCode)).IsFinished())
                {
                    int up  = G_towupper((wchar_t)wcharCode); 
                    int low = G_towlower((wchar_t)wcharCode); 
                    if ((int)wcharCode != up)
                        wcharCode = (UInt32)up;
                    else 
                        wcharCode = (UInt32)low;
                    if (pRestrict->RestrictRanges.GetIteratorAt(int(wcharCode)).IsFinished())
                        return false;
                }
            }
            // need to queue up key processing. Flash doesn't handle 
            // keys from keyevents, it executes them afterward.
            GASValueArray params;
            params.PushBack(GASValue(int(2)));
            params.PushBack(GASValue(int(wcharCode)));
            proot->InsertEmptyAction()->SetAction(this, 
                GFxEditTextCharacter::KeyProcessing, &params);
        }
        return true;
    }

    virtual void  SetOverwriteMode(bool overwMode = true)
    {
        if (pDocument->HasEditorKit())
        {
            if (overwMode)
                pDocument->GetEditorKit()->SetOverwriteMode();
            else
                pDocument->GetEditorKit()->ClearOverwriteMode();
        }
    }
    bool IsOverwriteMode() const
    {
        if (pDocument->HasEditorKit())
            return pDocument->GetEditorKit()->IsOverwriteMode();
        return false;
    }

    virtual void  SetWideCursor(bool wideCursor = true)
    {
        if (pDocument->HasEditorKit())
        {
            if (wideCursor)
                pDocument->GetEditorKit()->SetWideCursor();
            else
                pDocument->GetEditorKit()->ClearWideCursor();
            SetDirtyFlag();
        }
    }
#else
    virtual void  SetOverwriteMode(bool) {}
    virtual void  SetWideCursor(bool)    {}
    bool IsOverwriteMode() const { return false; }
#endif //#ifndef GFC_NO_KEYBOARD_SUPPORT

    // Special event handler; mouse wheel support
    bool            OnMouseWheelEvent(int mwDelta)
    { 
        GUNUSED(mwDelta); 
        if (IsMouseWheelEnabled() && IsSelectable())
        {
            SInt vscroll = (SInt)pDocument->GetVScrollOffset();
            vscroll -= mwDelta;
            if (vscroll < 0)
                vscroll = 0;
            if (vscroll > (SInt)pDocument->GetMaxVScroll())
                vscroll = (SInt)pDocument->GetMaxVScroll();
            pDocument->SetVScrollOffset((UInt)vscroll);
            SetDirtyFlag();
            return true;
        }
        return false; 
    }

#ifndef GFC_NO_FXPLAYER_AS_BITMAPDATA
    void AddIdImageDescAssoc(const char* idStr, GFxTextImageDesc* pdesc)
    {
        if (!pImageDescAssoc)
            pImageDescAssoc = GHEAP_AUTO_NEW(this) GFxStringHashLH<GPtr<GFxTextImageDesc> >;
        pImageDescAssoc->Set(idStr, pdesc);
    }
    void RemoveIdImageDescAssoc(const char* idStr)
    {
        if (pImageDescAssoc)
        {
            pImageDescAssoc->Remove(idStr);
        }
    }
    void ClearIdImageDescAssoc()
    {
        delete pImageDescAssoc;
        pImageDescAssoc = NULL;
    }
#endif

    virtual void OnEventLoad()
    {
        // finalize the initialization. We need to initialize text here rather than in ctor
        // since the name of instance is not set yet in the ctor and setting text might cause
        // GFxTranslator to be invoked, which uses the instance name.
        if (pDef->DefaultText.GetLength() > 0)
        {
            bool varExists = false;
            // check, does variable exist or not. If it doesn't exist, we
            // will need to call UpdateVariable in order to create this variable.
            if (!VariableName.IsEmpty())
            {
                GASEnvironment* penv = GetASEnvironment();
                if (penv)
                {
                    GASValue val;
                    varExists = penv->GetVariable(VariableName, &val);
                }
            }
            if (!varExists)
            {
                SetTextValue(pDef->DefaultText.ToCStr(), IsHtml());
                UpdateVariable();
            }
        }
        else
        {
            // This assigns both Text and HtmlText vars.
            SetTextValue("", IsHtml(), false);
        }

        // update text, if variable is used
        UpdateTextFromVariable();
        pDocument->Format();

        GFxASCharacter::OnEventLoad();

        // invoked when textfield is being added into displaylist;
        // we need to modify GeomData to reflect correct X and Y coords.
        // It is incorrect to take X and Y coordinates directly from the matrix
        // as it usually happens for movieclip. The matrix of textfields 
        // contains some offset (usually, 2 pixels by both axis), because TextRect's
        // topleft corner is usually (-2, -2), not (0,0).
        // Thus, we need to recalculate X and Y received from the matrix with using 
        // TextRect topleft corner.
        if (!pGeomData)
        {
            GeomDataType geomData;
            UpdateAndGetGeomData(&geomData, true);
        }
    }

	// Exposed for the complex objects API (see GFxValue::ObjectInterface::SetDisplayInfo)
    virtual void SetNeedToUpdateGeomData()
    {
        Flags |= Flags_NeedUpdateGeomData;
        SetDirtyFlag();
    }
    GeomDataType& UpdateAndGetGeomData(GeomDataType* pgeomData, bool force = false)
    {
        GASSERT(pgeomData);
        GetGeomData(*pgeomData);
        if (force || (Flags & Flags_NeedUpdateGeomData) != 0)
        {
            GRectF rr   = pDocument->GetViewRect();
            Matrix m    = GetMatrix();
            GPointF p   = rr.TopLeft();

            // same as m.Transform(p), but with two exception:
            // all calculations are being done in doubles.
            Double x = Double(m.M_[0][0]) * p.x + Double(m.M_[0][1]) * p.y + Double(m.M_[0][2]);
            Double y = Double(m.M_[1][0]) * p.x + Double(m.M_[1][1]) * p.y + Double(m.M_[1][2]);

            pgeomData->X = G_IRound(x);
            pgeomData->Y = G_IRound(y);
            SetGeomData(*pgeomData);
            Flags &= ~Flags_NeedUpdateGeomData;
        }
        return *pgeomData;
    }

    virtual void OnEventUnload()
    {
#ifndef GFC_NO_IME_SUPPORT
        // text field is unloading, so we need to make sure IME was finalized
        // for this text field.
        GFxMovieRoot* proot = GetMovieRoot();
        if (proot)
        {
            GPtr<GFxIMEManager> pIMEManager = proot->GetIMEManager();
            if (pIMEManager && pIMEManager->IsTextFieldFocused(this))
            {
                pIMEManager->DoFinalize();
            }
        }
#endif //#ifndef GFC_NO_IME_SUPPORT
        SetDirtyFlag();
        GFxASCharacter::OnEventUnload();
    }

    virtual void ReplaceText(const wchar_t*, UPInt len, UPInt beginPos, UPInt endPos);
    virtual GRectF GetCursorBounds(UPInt cursorPos, Float* phscroll, Float* pvscroll);
    virtual UInt32 GetSelectedTextColor() const;
    virtual UInt32 GetSelectedBackgroundColor() const;

#ifndef GFC_NO_IME_SUPPORT
    // IME related methods
    virtual bool IsIMEEnabled() const { return !IsIMEDisabledFlag() && !IsReadOnly() && !IsPassword(); }
    virtual void CreateCompositionString();
    virtual void ClearCompositionString();
    virtual void ReleaseCompositionString();
    virtual void CommitCompositionString(const wchar_t* pstr, UPInt len);
    virtual void SetCompositionStringText(const wchar_t* pstr, UPInt len);
    virtual void SetCompositionStringPosition(UPInt pos);
    virtual UPInt GetCompositionStringPosition() const;
    virtual UPInt GetCompositionStringLength() const;
    virtual void SetCursorInCompositionString(UPInt pos);
    virtual void HighlightCompositionStringText(UPInt posInCompStr, UPInt len, GFxTextIMEStyle::Category hcategory);
    virtual GFxFontResource* GetFontResource();
#endif //#ifndef GFC_NO_IME_SUPPORT

};


GFxCharacter*   GFxEditTextCharacterDef::CreateCharacterInstance(GFxASCharacter* parent, GFxResourceId id,
                                                                 GFxMovieDefImpl *pbindingImpl)
{
    GFxEditTextCharacter*   ch = GHEAP_AUTO_NEW(parent) GFxEditTextCharacter(this, pbindingImpl, parent, id);
    return ch;
}

// Read a DefineText tag.
void    GSTDCALL GFx_DefineEditTextLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT(tagInfo.TagType == GFxTag_DefineEditText);

    UInt16  characterId = p->ReadU16();

    GPtr<GFxEditTextCharacterDef> pch = 
        *GHEAP_NEW_ID(p->GetLoadHeap(),GFxStatMD_CharDefs_Mem) GFxEditTextCharacterDef();
    p->LogParse("EditTextChar, id = %d\n", characterId);
    pch->Read(p, tagInfo.TagType);

    p->AddResource(GFxResourceId(characterId), pch);
}

void GFxEditTextCharacter::SetTextFormat(const GASFnCall& fn)
{
#ifndef GFC_NO_FXPLAYER_AS_TEXTFORMAT
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (pthis->HasStyleSheet()) // doesn't work if style sheet is set
            return;
        if (fn.NArgs == 1)
        {
            // my_textField.setTextFormat(textFormat:TextFormat)
            // Applies the properties of textFormat to all text in the text field.
            GASObject* pobjVal = fn.Arg(0).ToObject(fn.Env);
            if (pobjVal && pobjVal->GetObjectType() == Object_TextFormat)
            {
                GASTextFormatObject* ptextFormatObj = static_cast<GASTextFormatObject*>(pobjVal);
                pthis->pDocument->SetTextFormat(ptextFormatObj->TextFormat);
                pthis->pDocument->SetParagraphFormat(ptextFormatObj->ParagraphFormat);
                pthis->SetDirtyFlag();
            }
        }
        else if (fn.NArgs == 2)
        {
            // my_textField.setTextFormat(beginIndex:Number, textFormat:TextFormat)
            // Applies the properties of textFormat to the character at the beginIndex position.
            GASObject* pobjVal = fn.Arg(1).ToObject(fn.Env);
            if (pobjVal && pobjVal->GetObjectType() == Object_TextFormat)
            {
                GASTextFormatObject* ptextFormatObj = static_cast<GASTextFormatObject*>(pobjVal);
                GASNumber pos = fn.Arg(0).ToNumber(fn.Env);
                if (pos >= 0)
                {
                    UInt upos = UInt(pos);
                    pthis->pDocument->SetTextFormat(ptextFormatObj->TextFormat, upos, upos + 1);
                    pthis->pDocument->SetParagraphFormat(ptextFormatObj->ParagraphFormat, upos, upos + 1);
                    pthis->SetDirtyFlag();
                }
            }
        }
        else if (fn.NArgs >= 3)
        {
            // my_textField.setTextFormat(beginIndex:Number, endIndex:Number, textFormat:TextFormat)
            // Applies the properties of the textFormat parameter to the span of text from the 
            // beginIndex position to the endIndex position.
            GASObject* pobjVal = fn.Arg(2).ToObject(fn.Env);
            if (pobjVal && pobjVal->GetObjectType() == Object_TextFormat)
            {
                GASNumber beginIndex = G_Max((GASNumber)0, fn.Arg(0).ToNumber(fn.Env));
                GASNumber endIndex   = G_Max((GASNumber)0, fn.Arg(1).ToNumber(fn.Env));
                if (beginIndex <= endIndex)
                {
                    GASTextFormatObject* ptextFormatObj = static_cast<GASTextFormatObject*>(pobjVal);
                    pthis->pDocument->SetTextFormat(ptextFormatObj->TextFormat, (UInt)beginIndex, (UInt)endIndex);
                    pthis->pDocument->SetParagraphFormat(ptextFormatObj->ParagraphFormat, (UInt)beginIndex, (UInt)endIndex);
                    pthis->SetDirtyFlag();
                }
            }
        }
    }
#else
    GUNUSED(fn);
    GFC_DEBUG_WARNING(1, "TextField.setTextFormat is not supported due to GFC_NO_FXPLAYER_AS_TEXTFORMAT macro.");
#endif //#ifndef GFC_NO_FXPLAYER_AS_TEXTFORMAT
}

void GFxEditTextCharacter::GetTextFormat(const GASFnCall& fn)
{
#ifndef GFC_NO_FXPLAYER_AS_TEXTFORMAT
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        UPInt beginIndex = 0, endIndex = GFC_MAX_UPINT;
        if (fn.NArgs >= 1)
        {
            // beginIndex
            GASNumber v = G_Max((GASNumber)0, fn.Arg(0).ToNumber(fn.Env));
            if (v >= 0)
                beginIndex = (UInt)v;
        }
        if (fn.NArgs >= 2)
        {
            // endIndex
            GASNumber v = G_Max((GASNumber)0, fn.Arg(1).ToNumber(fn.Env));
            if (v >= 0)
                endIndex = (UInt)v;
        }
        else if (fn.NArgs >= 1)
        {
            GASNumber v = G_Max((GASNumber)0, fn.Arg(0).ToNumber(fn.Env) + 1);
            if (v >= 0)
                endIndex = (UInt)v;
        }
        if (beginIndex <= endIndex)
        {
            GFxTextFormat textFmt(fn.Env->GetHeap());
            GFxTextParagraphFormat paraFmt;
            pthis->pDocument->GetTextAndParagraphFormat(&textFmt, &paraFmt, beginIndex, endIndex);

            GPtr<GASTextFormatObject> pasTextFmt = *GHEAP_NEW(fn.Env->GetHeap()) GASTextFormatObject(fn.Env);
            pasTextFmt->SetTextFormat(fn.Env->GetSC(), textFmt);
            pasTextFmt->SetParagraphFormat(fn.Env->GetSC(), paraFmt);
            fn.Result->SetAsObject(pasTextFmt);
        }
        else
            fn.Result->SetUndefined();
    }
    else
        fn.Result->SetUndefined();
#else
    GUNUSED(fn);
    GFC_DEBUG_WARNING(1, "TextField.getTextFormat is not supported due to GFC_NO_FXPLAYER_AS_TEXTFORMAT macro.");
#endif //#ifndef GFC_NO_FXPLAYER_AS_TEXTFORMAT
}

void GFxEditTextCharacter::GetNewTextFormat(const GASFnCall& fn)
{
#ifndef GFC_NO_FXPLAYER_AS_TEXTFORMAT
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        const GFxTextFormat* ptextFmt = pthis->pDocument->GetDefaultTextFormat();
        const GFxTextParagraphFormat* pparaFmt = pthis->pDocument->GetDefaultParagraphFormat();

        GPtr<GASTextFormatObject> pasTextFmt = *GHEAP_NEW(fn.Env->GetHeap()) GASTextFormatObject(fn.Env);
        if (ptextFmt)
            pasTextFmt->SetTextFormat(fn.Env->GetSC(), *ptextFmt);
        if (pparaFmt)
            pasTextFmt->SetParagraphFormat(fn.Env->GetSC(), *pparaFmt);
        fn.Result->SetAsObject(pasTextFmt);
   }
    else
        fn.Result->SetUndefined();
#else
    GUNUSED(fn);
    GFC_DEBUG_WARNING(1, "TextField.getNewTextFormat is not supported due to GFC_NO_FXPLAYER_AS_TEXTFORMAT macro.");
#endif //#ifndef GFC_NO_FXPLAYER_AS_TEXTFORMAT
}

void GFxEditTextCharacter::SetNewTextFormat(const GASFnCall& fn)
{
#ifndef GFC_NO_FXPLAYER_AS_TEXTFORMAT
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (pthis->HasStyleSheet()) // doesn't work if style sheet is set
            return;
        if (fn.NArgs >= 1)
        {
            GASObject* pobjVal = fn.Arg(0).ToObject(fn.Env);
            if (pobjVal && pobjVal->GetObjectType() == Object_TextFormat)
            {
                GASTextFormatObject* ptextFormatObj = static_cast<GASTextFormatObject*>(pobjVal);

                const GFxTextFormat* ptextFmt = pthis->pDocument->GetDefaultTextFormat();
                const GFxTextParagraphFormat* pparaFmt = pthis->pDocument->GetDefaultParagraphFormat();
                pthis->pDocument->SetDefaultTextFormat(ptextFmt->Merge(ptextFormatObj->TextFormat));
                pthis->pDocument->SetDefaultParagraphFormat(pparaFmt->Merge(ptextFormatObj->ParagraphFormat));
            }
        }
    }
#else
    GUNUSED(fn);
    GFC_DEBUG_WARNING(1, "TextField.setNewTextFormat is not supported due to GFC_NO_FXPLAYER_AS_TEXTFORMAT macro.");
#endif //#ifndef GFC_NO_FXPLAYER_AS_TEXTFORMAT
}

void GFxEditTextCharacter::ReplaceText(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (pthis->HasStyleSheet()) // doesn't work if style sheet is set
            return;
        if (fn.NArgs >= 3)
        {
            GASNumber start = fn.Arg(0).ToNumber(fn.Env);
            GASNumber end   = fn.Arg(1).ToNumber(fn.Env);
            GASString str   = fn.Arg(2).ToString(fn.Env);

            UInt len = str.GetLength();
            UInt startPos = UInt(start);
            UInt endPos   = UInt(end);
            if (start >= 0 && end >= 0 && startPos <= endPos)
            {
                // replaceText does *NOT* use default text format, if replaced text
                // is located in the middle of the text; however, it uses the default
                // format, if the replacing text is at the very end of the text. This
                // is so, because Flash doesn't know what is the format at the cursor pos
                // if the cursor is behind the last char (caretIndex == length).
                // Our implementation knows always about formats, thus, we need artificially
                // replicate this behavior. Though, it still works differently from Flash,
                // since that default text format will be combined with current text format
                // at the cursor position. Not sure, we should worry about this.
                const GFxTextFormat* ptextFmt;
                const GFxTextParagraphFormat* pparaFmt;
                UPInt prevLen = pthis->pDocument->GetLength();
                UPInt newLen  = prevLen - (endPos - startPos) + len;
                if (startPos >= prevLen)
                {
                    // use default text format
                    ptextFmt = pthis->pDocument->GetDefaultTextFormat();
                    pparaFmt = pthis->pDocument->GetDefaultParagraphFormat();
                }
                else
                    pthis->pDocument->GetTextAndParagraphFormat(&ptextFmt, &pparaFmt, startPos);
                if (ptextFmt) ptextFmt->AddRef(); // save format from possible releasing
                if (pparaFmt) pparaFmt->AddRef(); // save format from possible releasing

                wchar_t buf[1024];
                if (len < sizeof(buf)/sizeof(buf[0]))
                {
                    GUTF8Util::DecodeString(buf, str.ToCStr());
                    pthis->pDocument->ReplaceText(buf, startPos, endPos);
                }
                else
                {
                    wchar_t* pbuf = (wchar_t*)GALLOC((len + 1) * sizeof(wchar_t), GFxStatMV_Text_Mem);
                    GUTF8Util::DecodeString(pbuf, str.ToCStr());
                    pthis->pDocument->ReplaceText(pbuf, startPos, endPos);
                    GFREE(pbuf);
                }
                if (pthis->pDocument->HasEditorKit())
                {
                    // Replace text does not change the cursor position, unless
                    // this position doesn't exist anymore.

                    if (pthis->pDocument->GetEditorKit()->GetCursorPos() > newLen)
                        pthis->pDocument->GetEditorKit()->SetCursorPos(newLen, false);
                }
                if (pparaFmt)
                    pthis->pDocument->SetParagraphFormat(*pparaFmt, startPos, startPos + len);
                if (ptextFmt)
                {
                    pthis->pDocument->SetTextFormat(*ptextFmt, startPos, startPos + len);
                }
                if (ptextFmt) ptextFmt->Release();
                if (pparaFmt) pparaFmt->Release();
                pthis->SetDirtyFlag();
            }
        }
    }
}

void GFxEditTextCharacter::ReplaceSel(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (pthis->HasStyleSheet()) // doesn't work if style sheet is set
            return;
        if (fn.NArgs >= 1 && pthis->pDocument->HasEditorKit())
        {
            GASString str = fn.Arg(0).ToString(fn.Env);

            const GFxTextFormat*            ptextFmt = pthis->pDocument->GetDefaultTextFormat();
            const GFxTextParagraphFormat*   pparaFmt = pthis->pDocument->GetDefaultParagraphFormat();

            UInt len = str.GetLength();
            UPInt startPos = pthis->pDocument->GetEditorKit()->GetBeginSelection();
            UPInt endPos   = pthis->pDocument->GetEditorKit()->GetEndSelection();
            wchar_t buf[1024];
            if (len < sizeof(buf)/sizeof(buf[0]))
            {
                GUTF8Util::DecodeString(buf, str.ToCStr());
                pthis->pDocument->ReplaceText(buf, startPos, endPos);
            }
            else
            {
                wchar_t* pbuf = (wchar_t*)GALLOC((len + 1) * sizeof(wchar_t), GFxStatMV_Text_Mem);
                GUTF8Util::DecodeString(pbuf, str.ToCStr());
                pthis->pDocument->ReplaceText(pbuf, startPos, endPos);
                GFREE(pbuf);
            }
            pthis->pDocument->GetEditorKit()->SetCursorPos(startPos + len, false);
            if (pparaFmt)
                pthis->pDocument->SetParagraphFormat(*pparaFmt, startPos, startPos + len);
            if (ptextFmt)
                pthis->pDocument->SetTextFormat(*ptextFmt, startPos, startPos + len);
            pthis->SetDirtyFlag();
        }
    }
}

void GFxEditTextCharacter::RemoveTextField(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (pthis->GetDepth() < 16384)
        {
            pthis->LogScriptWarning("%s.removeMovieClip() failed - depth must be >= 0\n",
                pthis->GetName().ToCStr());
            return;
        }
        pthis->RemoveDisplayObject();
    }
}

#ifndef GFC_NO_TEXTFIELD_EXTENSIONS
void GFxEditTextCharacter::AppendText(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (pthis->HasStyleSheet()) // doesn't work if style sheet is set
            return;
        if (fn.NArgs >= 1)
        {
            GASString str = fn.Arg(0).ToString(fn.Env);
            pthis->pDocument->AppendText(str.ToCStr());
            pthis->SetDirtyFlag();
        }
    }
}

void GFxEditTextCharacter::AppendHtml(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (pthis->HasStyleSheet()) // doesn't work if style sheet is set
            return;
        if (fn.NArgs >= 1)
        {
            GASString str = fn.Arg(0).ToString(fn.Env);
            GFxStyledText::HTMLImageTagInfoArray imageInfoArray(GMemory::GetHeapByAddress(pthis));
            pthis->pDocument->AppendHtml(str.ToCStr(), GFC_MAX_UPINT, false, &imageInfoArray);
            if (imageInfoArray.GetSize() > 0)
            {
                pthis->ProcessImageTags(imageInfoArray);
            }
            pthis->SetDirtyFlag();
        }
    }
}

void GFxEditTextCharacter::GetCharBoundaries(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            UInt charIndex = (UInt)fn.Arg(0).ToNumber(fn.Env);
            GRectF charBounds;
            charBounds.Clear();
            if (pthis->pDocument->GetCharBoundaries(&charBounds, charIndex))
            {

#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
                GPtr<GASRectangleObject> prect = *GHEAP_NEW(fn.Env->GetHeap()) GASRectangleObject(fn.Env);

                GASRect gasRect;
                gasRect.Left    = TwipsToPixels(GASNumber(charBounds.Left));
                gasRect.Top     = TwipsToPixels(GASNumber(charBounds.Top));
                gasRect.Right   = TwipsToPixels(GASNumber(charBounds.Right));
                gasRect.Bottom  = TwipsToPixels(GASNumber(charBounds.Bottom));

                prect->SetProperties(fn.Env, gasRect);

#else
                GPtr<GASObject> prect = *GHEAP_NEW(fn.Env->GetHeap()) GASObject(fn.Env);

                GASStringContext *psc = fn.Env->GetSC();
                prect->SetConstMemberRaw(psc, "x", TwipsToPixels(GASNumber(charBounds.Left)));
                prect->SetConstMemberRaw(psc, "y", TwipsToPixels(GASNumber(charBounds.Top)));
                prect->SetConstMemberRaw(psc, "width", TwipsToPixels(GASNumber(charBounds.Right)));
                prect->SetConstMemberRaw(psc, "height", TwipsToPixels(GASNumber(charBounds.Bottom)));
#endif
                fn.Result->SetAsObject(prect.GetPtr());
            }
            else
                fn.Result->SetNull();
        }
    }
}

void GFxEditTextCharacter::GetExactCharBoundaries(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            UInt charIndex = (UInt)fn.Arg(0).ToUInt32(fn.Env);
            GRectF charBounds;
            charBounds.Clear();
            if (pthis->pDocument->GetExactCharBoundaries(&charBounds, charIndex))
            {

#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
                GPtr<GASRectangleObject> prect = *GHEAP_NEW(fn.Env->GetHeap()) GASRectangleObject(fn.Env);

                GASRect gasRect;
                gasRect.Left    = TwipsToPixels(GASNumber(charBounds.Left));
                gasRect.Top     = TwipsToPixels(GASNumber(charBounds.Top));
                gasRect.Right   = TwipsToPixels(GASNumber(charBounds.Right));
                gasRect.Bottom  = TwipsToPixels(GASNumber(charBounds.Bottom));
                prect->SetProperties(fn.Env, gasRect);

#else
                GPtr<GASObject> prect = *GHEAP_NEW(fn.Env->GetHeap()) GASObject(fn.Env);

                GASStringContext *psc = fn.Env->GetSC();
                prect->SetConstMemberRaw(psc, "x", TwipsToPixels(GASNumber(charBounds.Left)));
                prect->SetConstMemberRaw(psc, "y", TwipsToPixels(GASNumber(charBounds.Top)));
                prect->SetConstMemberRaw(psc, "width", TwipsToPixels(GASNumber(charBounds.Right)));
                prect->SetConstMemberRaw(psc, "height", TwipsToPixels(GASNumber(charBounds.Bottom)));
#endif
                fn.Result->SetAsObject(prect.GetPtr());
            }
            else
                fn.Result->SetNull();
        }
    }
}

void GFxEditTextCharacter::GetCharIndexAtPoint(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 2)
        {
            GASNumber x = fn.Arg(0).ToNumber(fn.Env);
            GASNumber y = fn.Arg(1).ToNumber(fn.Env);
            UPInt pos = pthis->pDocument->GetCharIndexAtPoint(Float(PixelsToTwips(x)), Float(PixelsToTwips(y)));
            if (!G_IsMax(pos))
                fn.Result->SetNumber(GASNumber(pos));
            else
                fn.Result->SetNumber(-1);
        }
    }
}

void GFxEditTextCharacter::GetLineIndexAtPoint(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 2)
        {
            GASNumber x = fn.Arg(0).ToNumber(fn.Env);
            GASNumber y = fn.Arg(1).ToNumber(fn.Env);
            UInt pos = pthis->pDocument->GetLineIndexAtPoint(Float(PixelsToTwips(x)), Float(PixelsToTwips(y)));
            if (!G_IsMax(pos))
                fn.Result->SetNumber(GASNumber(pos));
            else
                fn.Result->SetNumber(-1);
        }
    }
}

void GFxEditTextCharacter::GetLineOffset(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            SInt lineIndex = SInt(fn.Arg(0).ToInt32(fn.Env));
            if (lineIndex < 0)
                fn.Result->SetNumber(-1);
            else
            {
                UPInt off = pthis->pDocument->GetLineOffset(UInt(lineIndex));
                if (!G_IsMax(off))
                    fn.Result->SetNumber(GASNumber(off));
                else
                    fn.Result->SetNumber(-1);
            }
        }
    }
}

void GFxEditTextCharacter::GetLineLength(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            SInt lineIndex = SInt(fn.Arg(0).ToInt32(fn.Env));
            if (lineIndex < 0)
                fn.Result->SetNumber(-1);
            else
            {
                UPInt len = pthis->pDocument->GetLineLength(UInt(lineIndex));
                if (!G_IsMax(len))
                    fn.Result->SetNumber(GASNumber(len));
                else
                    fn.Result->SetNumber(-1);
            }
        }
    }
}

void GFxEditTextCharacter::GetLineMetrics(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            SInt lineIndex = SInt(fn.Arg(0).ToInt32(fn.Env));
            if (lineIndex < 0)
                fn.Result->SetUndefined();
            else
            {
                GFxTextDocView::LineMetrics metrics;
                bool rv = pthis->pDocument->GetLineMetrics(UInt(lineIndex), &metrics);
                if (rv)
                {
                    GPtr<GASObject> pobj = *GHEAP_NEW(fn.Env->GetHeap()) GASObject(fn.Env);
                    pobj->SetConstMemberRaw(fn.Env->GetSC(), "ascent", GASValue(TwipsToPixels((GASNumber)metrics.Ascent)));
                    pobj->SetConstMemberRaw(fn.Env->GetSC(), "descent", GASValue(TwipsToPixels((GASNumber)metrics.Descent)));
                    pobj->SetConstMemberRaw(fn.Env->GetSC(), "width", GASValue(TwipsToPixels((GASNumber)metrics.Width)));
                    pobj->SetConstMemberRaw(fn.Env->GetSC(), "height", GASValue(TwipsToPixels((GASNumber)metrics.Height)));
                    pobj->SetConstMemberRaw(fn.Env->GetSC(), "leading", GASValue(TwipsToPixels((GASNumber)metrics.Leading)));
                    pobj->SetConstMemberRaw(fn.Env->GetSC(), "x", GASValue(TwipsToPixels((GASNumber)metrics.FirstCharXOff)));
                    fn.Result->SetAsObject(pobj);
                }
                else
                    fn.Result->SetUndefined();
            }
        }
    }
}

void GFxEditTextCharacter::GetLineText(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            SInt lineIndex = SInt(fn.Arg(0).ToInt32(fn.Env));
            if (lineIndex < 0)
                fn.Result->SetUndefined();
            else
            {
                UPInt len = 0;
                const wchar_t* ptext = pthis->pDocument->GetLineText(UInt(lineIndex), &len);
                if (ptext)
                {
                    GString str;
                    str.AppendString(ptext, (SPInt)len);
                    GASString asstr = fn.Env->CreateString(str);
                    fn.Result->SetString(asstr);
                }
                else
                    fn.Result->SetString(fn.Env->CreateConstString(""));
            }
        }
    }
}


void GFxEditTextCharacter::GetFirstCharInParagraph(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            SInt charIndex = SInt(fn.Arg(0).ToInt32(fn.Env));
            if (charIndex < 0)
                fn.Result->SetNumber(-1);
            else
            {
                UPInt off = pthis->pDocument->GetFirstCharInParagraph(UInt(charIndex));
                if (!G_IsMax(off))
                    fn.Result->SetNumber(GASNumber(off));
                else
                    fn.Result->SetNumber(-1);
            }
        }
    }
}

void GFxEditTextCharacter::GetLineIndexOfChar(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            SInt charIndex = SInt(fn.Arg(0).ToInt32(fn.Env));
            if (charIndex < 0)
                fn.Result->SetNumber(-1);
            else
            {
                UInt off = pthis->pDocument->GetLineIndexOfChar(UInt(charIndex));
                if (!G_IsMax(off))
                    fn.Result->SetNumber(GASNumber(off));
                else
                    fn.Result->SetNumber(-1);
            }
        }
    }
}

#ifndef GFC_NO_FXPLAYER_AS_BITMAPDATA
void GFxEditTextCharacter::ProceedImageSubstitution(const GASFnCall& fn, int idx, const GASValue* pve)
{
    if (pve && pve->IsObject())
    {
        GASObject* peobj = pve->ToObject(fn.Env);
        GASSERT(peobj);
        GASValue val;
        GFxTextDocView::ImageSubstitutor* pimgSubst = pDocument->CreateImageSubstitutor();
        if (!pimgSubst)
            return;
        GFxTextDocView::ImageSubstitutor::Element isElem;
        if (peobj->GetConstMemberRaw (fn.Env->GetSC(), "subString", &val))
        {
            GASString str = val.ToString(fn.Env);
            UPInt wstrLen = str.GetLength();
            wstrLen = G_PMin(wstrLen, (UPInt)(sizeof(isElem.SubString)/sizeof(isElem.SubString[0]) - 1));

            GUTF8Util::DecodeString(isElem.SubString, str.ToCStr(), wstrLen + 1);
            isElem.SubStringLen = (UByte)wstrLen;

            if (isElem.SubStringLen > 15)
            {
                LogScriptWarning("%s.setImageSubstitutions() failed for #%d element - length of subString should not exceed 15 characters\n",
                    GetName().ToCStr(), idx);
                return;
            }
        }
        else
        {
            // subString is mandatory!
            LogScriptWarning("%s.setImageSubstitutions() failed for #%d element - subString should be specified\n",
                GetName().ToCStr(), idx);
            return;
        }
        GPtr<GFxShapeWithStyles> pimgShape;
        Float screenWidth = 0, screenHeight = 0;
        Float origWidth = 0, origHeight = 0;
        Float baseLineX = 0, baseLineY = 0;
        const char* idStr = NULL;
        if (peobj->GetConstMemberRaw (fn.Env->GetSC(), "image", &val))
        {
            GASObject* piobj = val.ToObject(fn.Env);
            if (piobj && piobj->GetObjectType() == Object_BitmapData)
            {
                GASBitmapData* pbmpData = static_cast<GASBitmapData*>(piobj);
                GFxImageResource* pimgRes = pbmpData->GetImage();
                pimgShape = *GHEAP_NEW(fn.Env->GetHeap()) GFxShapeWithStyles();
                pimgShape->SetToImage(pimgRes, true);

                GRect<SInt> dimr = pimgRes->GetImageInfo()->GetRect();
                screenWidth  = origWidth  = (Float)PixelsToTwips(dimr.Width());
                screenHeight = origHeight = (Float)PixelsToTwips(dimr.Height());
                if (origWidth == 0 || origHeight == 0)
                {
                    LogScriptWarning("%s.setImageSubstitutions() failed for #%d element - image has one zero dimension\n",
                        GetName().ToCStr(), idx);
                    return;
                }
            }
        }
        if (!pimgShape)
        {
            LogScriptWarning("%s.setImageSubstitutions() failed for #%d element - 'image' is not specified or not a BitmapData\n",
                GetName().ToCStr(), idx);
            return;
        }
        if (peobj->GetConstMemberRaw (fn.Env->GetSC(), "width", &val))
        {
            GASNumber v = val.ToNumber(fn.Env);
            screenWidth = (Float)PixelsToTwips(v);
        }
        if (peobj->GetConstMemberRaw (fn.Env->GetSC(), "height", &val))
        {
            GASNumber v = val.ToNumber(fn.Env);
            screenHeight = (Float)PixelsToTwips(v);
        }
        if (peobj->GetConstMemberRaw (fn.Env->GetSC(), "baseLineX", &val))
        {
            GASNumber v = val.ToNumber(fn.Env);
            baseLineX = (Float)PixelsToTwips(v);
        }
        if (peobj->GetConstMemberRaw (fn.Env->GetSC(), "baseLineY", &val))
        {
            GASNumber v = val.ToNumber(fn.Env);
            baseLineY = (Float)PixelsToTwips(v);
        }
        else
        {
            // if baseLineY is not specified, then use (origHeight - 1) pts instead
            baseLineY = origHeight - PixelsToTwips(1.0f);
        }
        if (peobj->GetConstMemberRaw (fn.Env->GetSC(), "id", &val))
        {
            idStr = val.ToString(fn.Env).ToCStr();
        }

        isElem.pImageDesc = *GHEAP_NEW(fn.Env->GetHeap()) GFxTextImageDesc;
        GASSERT(isElem.pImageDesc);
        isElem.pImageDesc->pImageShape  = pimgShape;
        isElem.pImageDesc->BaseLineX    = (SInt)baseLineX;
        isElem.pImageDesc->BaseLineY    = (SInt)baseLineY;
        isElem.pImageDesc->ScreenWidth  = (UInt)screenWidth;
        isElem.pImageDesc->ScreenHeight = (UInt)screenHeight;
        if (idStr)
            AddIdImageDescAssoc(idStr, isElem.pImageDesc);
        isElem.pImageDesc->Matrix.AppendTranslation(-baseLineX, -baseLineY);
        isElem.pImageDesc->Matrix.AppendScaling(screenWidth/origWidth, screenHeight/origHeight);

        pimgSubst->AddImageDesc(isElem);
        pDocument->SetCompleteReformatReq();
        SetDirtyFlag();
    }
}


void GFxEditTextCharacter::SetImageSubstitutions(const GASFnCall& fn)
{
    fn.Result->SetBool(false);
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            if (fn.Arg(0).IsNull())
            {
                // clear all substitutions
                pthis->ClearIdImageDescAssoc();
                pthis->pDocument->ClearImageSubstitutor();
                pthis->pDocument->SetCompleteReformatReq();
            }
            else
            {
                GASObject* pobj = fn.Arg(0).ToObject(fn.Env);
                if (pobj)
                {
                    // if array is specified as a parameter, proceed it; otherwise
                    // if an object is specified - proceed it as single element.
                    if (pobj->GetObjectType() == Object_Array)
                    {
                        GASArrayObject* parr = static_cast<GASArrayObject*>(pobj);
                        for (int i = 0, n = parr->GetSize(); i < n; ++i)
                        {
                            GASValue* pve = parr->GetElementPtr(i);
                            pthis->ProceedImageSubstitution(fn, i, pve);
                        }
                    }
                    else
                    {
                        const GASValue& ve = fn.Arg(0);
                        pthis->ProceedImageSubstitution(fn, 0, &ve);
                    }
                }
                else
                {
                    pthis->LogScriptWarning("%s.setImageSubstitutions() failed: parameter should be either 'null', object or array\n",
                        pthis->GetName().ToCStr());
                }
            }
        }
    }
}

void GFxEditTextCharacter::UpdateImageSubstitution(const GASFnCall& fn)
{
    fn.Result->SetBool(false);
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (fn.NArgs >= 1)
        {
            GASString idStr = fn.Arg(0).ToString(fn.Env);
            if (pthis->pImageDescAssoc)
            {
                GPtr<GFxTextImageDesc>* ppimgDesc = pthis->pImageDescAssoc->Get(idStr.ToCStr());
                if (ppimgDesc)
                {
                    GFxTextImageDesc* pimageDesc = ppimgDesc->GetPtr();
                    GASSERT(pimageDesc);
                    if (fn.NArgs >= 2)
                    {
                        if (fn.Arg(1).IsNull() || fn.Arg(1).IsUndefined())
                        {
                            // if null or undefined - remove the substitution and reformat
                            GFxTextDocView::ImageSubstitutor* pimgSubst = pthis->pDocument->CreateImageSubstitutor();
                            if (pimgSubst)
                            {
                                pimgSubst->RemoveImageDesc(pimageDesc);
                                pthis->pDocument->SetCompleteReformatReq();
                                pthis->RemoveIdImageDescAssoc(idStr.ToCStr());
                                pthis->SetDirtyFlag();
                            }
                        }
                        else
                        {
                            GASObject* piobj = fn.Arg(1).ToObject(fn.Env);
                            if (piobj && piobj->GetObjectType() == Object_BitmapData)
                            {
                                GASBitmapData* pbmpData = static_cast<GASBitmapData*>(piobj);
                                GFxImageResource* pimgRes = pbmpData->GetImage();
                                GPtr<GFxShapeWithStyles> pimgShape = *GHEAP_NEW(fn.Env->GetHeap()) GFxShapeWithStyles();
                                pimgShape->SetToImage(pimgRes, true);

                                pimageDesc->pImageShape = pimgShape;
                                pthis->SetDirtyFlag();
                            }
                        }
                    }
                }
            }
        }
    }
}
#endif //#ifndef GFC_NO_FXPLAYER_AS_BITMAPDATA

// Extension method: copyToClipboard([richClipboard:Boolean], [startIndex:Number], [endIndex:Number])
// richClipboard by default is equal to "useRichTextClipboard";
// default values of startIndex and endIndex are equal to selectionBeginIndex/selectionEndIndex
void GFxEditTextCharacter::CopyToClipboard(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (pthis->pDocument->HasEditorKit())
        {
            GFxTextEditorKit* pedKit = pthis->pDocument->GetEditorKit();
            bool richClipboard  = pedKit->DoesUseRichClipboard();
            UInt startIndex     = (UInt)pedKit->GetBeginSelection();
            UInt endIndex       = (UInt)pedKit->GetEndSelection();
            if (fn.NArgs >= 1)
                richClipboard = fn.Arg(0).ToBool(fn.Env);
            if (fn.NArgs >= 2)
                startIndex = fn.Arg(1).ToUInt32(fn.Env);
            if (fn.NArgs >= 3)
                endIndex = fn.Arg(2).ToUInt32(fn.Env);
            pedKit->CopyToClipboard(startIndex, endIndex, richClipboard);
        }
    }
}

// Extension method: cutToClipboard([richClipboard:Boolean], [startIndex:Number], [endIndex:Number])
// richClipboard by default is equal to "useRichTextClipboard";
// default values of startIndex and endIndex are equal to selectionBeginIndex/selectionEndIndex
void GFxEditTextCharacter::CutToClipboard(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (pthis->pDocument->HasEditorKit())
        {
            GFxTextEditorKit* pedKit = pthis->pDocument->GetEditorKit();
            bool richClipboard  = pedKit->DoesUseRichClipboard();
            UInt startIndex     = (UInt)pedKit->GetBeginSelection();
            UInt endIndex       = (UInt)pedKit->GetEndSelection();
            if (fn.NArgs >= 1)
                richClipboard = fn.Arg(0).ToBool(fn.Env);
            if (fn.NArgs >= 2)
                startIndex = fn.Arg(1).ToUInt32(fn.Env);
            if (fn.NArgs >= 3)
                endIndex = fn.Arg(2).ToUInt32(fn.Env);
            pedKit->CutToClipboard(startIndex, endIndex, richClipboard);
            pthis->SetDirtyFlag();
        }
    }
}

// Extension method: pasteFromClipboard([richClipboard:Boolean], [startIndex:Number], [endIndex:Number])
// richClipboard by default is equal to "useRichTextClipboard";
// default values of startIndex and endIndex are equal to selectionBeginIndex/selectionEndIndex
void GFxEditTextCharacter::PasteFromClipboard(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_TextField)
    {
        GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
        if (pthis->pDocument->HasEditorKit())
        {
            GFxTextEditorKit* pedKit = pthis->pDocument->GetEditorKit();
            bool richClipboard  = pedKit->DoesUseRichClipboard();
            UInt startIndex     = (UInt)pedKit->GetBeginSelection();
            UInt endIndex       = (UInt)pedKit->GetEndSelection();
            if (fn.NArgs >= 1)
                richClipboard = fn.Arg(0).ToBool(fn.Env);
            if (fn.NArgs >= 2)
                startIndex = fn.Arg(1).ToUInt32(fn.Env);
            if (fn.NArgs >= 3)
                endIndex = fn.Arg(2).ToUInt32(fn.Env);
            pedKit->PasteFromClipboard(startIndex, endIndex, richClipboard);
            pthis->SetDirtyFlag();
        }
    }
}
#endif //GFC_NO_TEXTFIELD_EXTENSIONS

#if !defined(GFC_NO_FXPLAYER_AS_BITMAPDATA)
void GFxEditTextCharacter::ProcessImageTags(GFxStyledText::HTMLImageTagInfoArray& imageInfoArray)
{
    GASEnvironment* penv = GetASEnvironment();

    UPInt n = imageInfoArray.GetSize();

    if (n > 0)
        SetDirtyFlag();

    for (UPInt i = 0; i < n; ++i)
    {
        GFxStyledText::HTMLImageTagInfo& imgTagInfo = imageInfoArray[i];
        GASSERT(imgTagInfo.pTextImageDesc);
        // TODO: Url should be used to load image by the same way as loadMovie does.
        // At the moment there is no good way to get a callback when movie is loaded.
        // Therefore, at the moment, Url is used as export name only. "imgps://" also
        // may be used.

        GFxResourceBindData resBindData;
        GPtr<GFxMovieDefImpl> md = penv->GetTarget()->GetResourceMovieDef();
        if (md) // can it be NULL?
        {
            bool userImageProtocol = false;
            // check for user protocol img:// and imgps://
            if (imgTagInfo.Url.GetLength() > 0 && (imgTagInfo.Url[0] == 'i' || imgTagInfo.Url[0] == 'I'))
            {
                GString urlLowerCase = imgTagInfo.Url.ToLower();
                if (urlLowerCase.Substring(0, 6) == "img://")
                    userImageProtocol = true;
                else if (urlLowerCase.Substring(0, 8) == "imgps://")
                    userImageProtocol = true;
            }
            if (!userImageProtocol)
            {
                if (!penv->GetMovieRoot()->FindExportedResource(md, &resBindData, imgTagInfo.Url))
                {
                    penv->LogScriptWarning("ProcessImageTags: can't find a resource for export name '%s'\n", imgTagInfo.Url.ToCStr());
                    continue;
                }
                GASSERT(resBindData.pResource.GetPtr() != 0);
            }
            // Must check resource type, since users can theoretically pass other resource ids.
            if (userImageProtocol || resBindData.pResource->GetResourceType() == GFxResource::RT_Image)
            {
                // bitmap
                GPtr<GASBitmapData> pbmpData = *GASBitmapData::LoadBitmap(penv, imgTagInfo.Url);
                if (!pbmpData)
                {
                    penv->LogScriptWarning("ProcessImageTags: can't load the image '%s'\n", imgTagInfo.Url.ToCStr());
                    continue;
                }
                GFxImageResource* pimgRes = pbmpData->GetImage();
                GASSERT(pimgRes);
                GPtr<GFxShapeWithStyles> pimgShape = *GHEAP_NEW(penv->GetHeap()) GFxShapeWithStyles();
                GASSERT(pimgShape);
                pimgShape->SetToImage(pimgRes, true);
                imgTagInfo.pTextImageDesc->pImageShape = pimgShape;

                Float origWidth = 0, origHeight = 0;
                Float screenWidth = 0, screenHeight = 0;
                GRect<SInt> dimr = pimgRes->GetImageInfo()->GetRect();
                screenWidth  = origWidth  = (Float)PixelsToTwips(dimr.Width());
                screenHeight = origHeight = (Float)PixelsToTwips(dimr.Height());
                if (imgTagInfo.Width)
                    screenWidth = Float(imgTagInfo.Width);
                if (imgTagInfo.Height)
                    screenHeight = Float(imgTagInfo.Height);

                Float baseLineY = origHeight - PixelsToTwips(1.0f);
                baseLineY += imgTagInfo.VSpace;
                imgTagInfo.pTextImageDesc->ScreenWidth  = (UInt)screenWidth;
                imgTagInfo.pTextImageDesc->ScreenHeight = (UInt)screenHeight;
                imgTagInfo.pTextImageDesc->Matrix.AppendTranslation(0, -baseLineY);
                imgTagInfo.pTextImageDesc->Matrix.AppendScaling(screenWidth/origWidth, screenHeight/origHeight);

                pDocument->SetCompleteReformatReq();
            }
            else if (resBindData.pResource->GetResourceType() == GFxResource::RT_SpriteDef)
            {
                // sprite
                //GFxSpriteDef* psprRes = (GFxSpriteDef*)resBindData.pResource.GetPtr();
            }
        }
    }
}
#else
void GFxEditTextCharacter::ProcessImageTags(GFxStyledText::HTMLImageTagInfoArray&) 
{
    GFC_DEBUG_WARNING(1, "<IMG> tags are not supported due to defined GFC_NO_FXPLAYER_AS_BITMAPDATA.\n");
}
#endif //#if !defined(GFC_NO_FXPLAYER_AS_BITMAPDATA)

void GFxEditTextCharacter::ReplaceText(const wchar_t* ptext, UPInt beginPos, UPInt endPos, UPInt textLen)
{
    if (pDocument->HasEditorKit())
    {
        textLen = pDocument->ReplaceText(ptext, beginPos, endPos, textLen);
        if (pDocument->HasEditorKit())
        {
            // Replace text does not change the cursor position, unless
            // this position doesn't exist anymore.
            UPInt newLen = pDocument->GetLength();
            if (pDocument->GetEditorKit()->GetCursorPos() > newLen)
                pDocument->GetEditorKit()->SetCursorPos(newLen, false);
        }
        UpdateVariable();
        NotifyChanged();
    }
}

GRectF GFxEditTextCharacter::GetCursorBounds(UPInt cursorPos, Float* phscroll, Float* pvscroll)
{
    if (pDocument->HasEditorKit())
    {
        GFxTextEditorKit* peditor = pDocument->GetEditorKit();
        GRectF newCursorRect;
        if (peditor->CalcAbsCursorRectOnScreen(cursorPos, &newCursorRect, NULL, NULL))
        {
            if (phscroll)
                *phscroll = Float(pDocument->GetHScrollOffset());
            if (pvscroll)
                *pvscroll = Float(pDocument->GetVScrollOffset());
            return newCursorRect;
        }
    }
    if (phscroll)
        *phscroll = 0;
    if (pvscroll)
        *pvscroll = 0;
    return GRectF();
}

#ifndef GFC_NO_IME_SUPPORT
GFxFontResource* GFxEditTextCharacter::GetFontResource()
{
    if (!pDef->FontId.GetIdIndex())
        return NULL;
    // resolve font name, if possible
    GFxResourceBindData fontData = pBinding->GetResourceData(pDef->pFont);
    if (!fontData.pResource)
    {
        GPtr<GFxLog> plog = GetLog();
        if (plog)
        {
            plog->LogError("Error: Resource for font id = %d is not found in text field id = %d, def text = '%s'\n", 
                pDef->FontId.GetIdIndex(), GetId().GetIdIndex(), pDef->DefaultText.ToCStr());
        }
    }
    else if (fontData.pResource->GetResourceType() != GFxResource::RT_Font)
    {
        GPtr<GFxLog> plog = GetLog();
        if (plog)
        {
            plog->LogError("Error: Font id = %d is referring to non-font resource in text field id = %d, def text = '%s'\n", 
                pDef->FontId.GetIdIndex(), GetId().GetIdIndex(), pDef->DefaultText.ToCStr());
        }
    }
    else
    {
        GFxFontResource* pfont = (GFxFontResource*)fontData.pResource.GetPtr();
        return pfont;
    }
    return NULL;
}

void GFxEditTextCharacter::CreateCompositionString()
{
    if (pDocument->HasEditorKit())
    {
        GPtr<GFxTextCompositionString> cs = pDocument->GetEditorKit()->CreateCompositionString();
        // need to grab styles for IME compos/clause/etc
        if (ASTextFieldObj)
        {
            GFxTextIMEStyle* pcsStyles = ASTextFieldObj->GetIMECompositionStringStyles();
            if (pcsStyles)
                cs->UseStyles(*pcsStyles);
        }
        cs->SetText(L"");
        cs->SetPosition(pDocument->GetEditorKit()->GetCursorPos());
        SetDirtyFlag();
    }
}

void GFxEditTextCharacter::ClearCompositionString()
{
    if (pDocument->HasEditorKit())
    {
        GPtr<GFxTextCompositionString> cs = pDocument->GetEditorKit()->GetCompositionString();
        if (cs)
		{
            cs->SetText(L"");
			cs->SetPosition(0);
		}
        SetDirtyFlag();
    }
}

void GFxEditTextCharacter::ReleaseCompositionString()
{
    if (pDocument->HasEditorKit())
    {
        pDocument->GetEditorKit()->ReleaseCompositionString();
        SetDirtyFlag();
    }
}

void GFxEditTextCharacter::CommitCompositionString(const wchar_t* pstr, UPInt len)
{
    if (pDocument->HasEditorKit())
    {
        GPtr<GFxTextCompositionString> cs = pDocument->GetEditorKit()->GetCompositionString();
        if (cs)
        {
            if (!pstr)
            {
                pstr = cs->GetText();
                len  = cs->GetLength();
            }
            else if (len == GFC_MAX_UPINT)
            {
                len = G_wcslen(pstr);
            }
            // check for max length
            if (pDocument->HasMaxLength() && pDocument->GetLength() + len > pDocument->GetMaxLength())
                len = pDocument->GetMaxLength() - pDocument->GetLength();

			if (cs->GetPosition() != pDocument->GetEditorKit()->GetCursorPos())
			{
				// Bad behavior, log it
				// This is important for the following reason. Most IME's (according to the documentation) follow this sequence of IME messages:
				// StartComposition -> IMEComposition -> EndComposition. This allows us to set the current position of the caret (in the textfield) in the 
				// composition string while processing the StartComposition message. Now, if the start composition message is NOT received,
				// the position of the caret will not be preset, and when we finalize the composed text, this text will be inserted at the wrong
				// position in the textfield, since the insertion position is the position of the caret that should have been set while processing
				// StartComposition. The NewPhonetic IME on Chinese Trad XP after installing office trad 2003 has the following quirk. It obeys
				// the regular sequence of IME messages while processing keys that result in chinese chars (most letter keys), but for the num pad
				// keys, it only sends a IMEComposition (with result flag set) message without the accompanying start and end composition messages.
				// This means that the cs->GetPosition() used in the GetStyledText call below will return a potentially incorrect caret position
				// (since the caret position in the composition was set when the LAST startcomposition was received, and the current caret position
				// could have changed (due to using arrow keys for example). This would lead to the number being inserted in the wrong position 
				// (corresponding to the last set caret position). The line of code below accounts for this special case. It's really trying to compensate
				// for the behavior of a buggy ime.
				cs->SetPosition(pDocument->GetEditorKit()->GetCursorPos());
			}
            
			//pDocument->GetStyledText()->InsertString(pstr, cs->GetPosition(), len, 
			//cs->GetDefaultTextFormat(), pDocument->GetDefaultParagraphFormat());
            cs->SetText(L"");
			int charCount = 0;
			GFxMovieRoot* proot = GetMovieRoot();
			for (unsigned i = 0; i < len; i++)
			{
				UInt32 wcharCode = pstr[i];
				// check for restrict:
				if (proot && pDocument->HasEditorKit() && (!IsReadOnly() || IsSelectable()))
				{
					if (pRestrict)
					{
						if (pRestrict->RestrictRanges.GetIteratorAt(int(wcharCode)).IsFinished())
						{
							int up  = G_towupper((wchar_t)wcharCode); 
							int low = G_towlower((wchar_t)wcharCode); 
							if ((int)wcharCode != up)
								wcharCode = (UInt32)up;
							else 
								wcharCode = (UInt32)low;
							if (pRestrict->RestrictRanges.GetIteratorAt(int(wcharCode)).IsFinished())
								continue;
						}
					}
				}
				charCount++;
				// We don't use OnCharEvent here for the following reason: Some customers like to use while instead of for in the
				// message loop (while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))). This means that advance will be called only
				// after all the windows messages for an ime composition character have been processed. Since OnCharEvent creates
				// an empty action for every character which is processed during advance, processing of some characters can take place
				// too late. For example, in korean, if you already have a composition character and then type another key which can't
				// be composed with the current character, the existing character will be finalized, and a new composition string containing
				// the new character will be created. However, there must be an advance between these events otherwise the glyphs corresponding
				// to the finalized character and the new composition character will be displayed at wrong positions. 
			//	if (OnCharEvent(pstr[i],0))
			//		charCount++;
				pDocument->GetEditorKit()->OnChar(wcharCode);
			}
            UPInt newCursorPos = cs->GetPosition() + charCount;
            cs->SetPosition(newCursorPos);
            //pDocument->GetEditorKit()->SetCursorPos(newCursorPos, false);
            UpdateVariable();
            NotifyChanged();
        }
    }
}

void GFxEditTextCharacter::SetCompositionStringText(const wchar_t* pstr, UPInt len)
{
    if (pDocument->HasEditorKit())
    {
        GPtr<GFxTextCompositionString> cs = pDocument->GetEditorKit()->GetCompositionString();
        if (cs)
        {
            cs->SetText(pstr, len);
            SetDirtyFlag();
        }
    }
}

void GFxEditTextCharacter::SetCompositionStringPosition(UPInt pos)
{
    if (pDocument->HasEditorKit())
    {
        GPtr<GFxTextCompositionString> cs = pDocument->GetEditorKit()->GetCompositionString();
        if (cs)
        {
            cs->SetPosition(pos);
            SetDirtyFlag();
        }
    }
}

UPInt GFxEditTextCharacter::GetCompositionStringPosition() const
{
    if (pDocument->HasEditorKit())
    {
        GPtr<GFxTextCompositionString> cs = pDocument->GetEditorKit()->GetCompositionString();
        if (cs)
        {
            return cs->GetPosition();
        }
    }
    return GFC_MAX_UPINT;
}

UPInt GFxEditTextCharacter::GetCompositionStringLength() const
{
    if (pDocument->HasEditorKit())
    {
        GPtr<GFxTextCompositionString> cs = pDocument->GetEditorKit()->GetCompositionString();
        if (cs)
        {
            return cs->GetLength();
        }
    }
    return 0;
}

void GFxEditTextCharacter::SetCursorInCompositionString(UPInt pos)
{
    if (pDocument->HasEditorKit())
    {
        GPtr<GFxTextCompositionString> cs = pDocument->GetEditorKit()->GetCompositionString();
        if (cs)
        {
            cs->SetCursorPosition(pos);
            SetDirtyFlag();
        }
    }
}

void GFxEditTextCharacter::HighlightCompositionStringText(UPInt posInCompStr, UPInt len, GFxTextIMEStyle::Category hcategory)
{
    if (pDocument->HasEditorKit())
    {
        GPtr<GFxTextCompositionString> cs = pDocument->GetEditorKit()->GetCompositionString();
        if (cs)
        {
            cs->HighlightText(posInCompStr, len, hcategory);
            SetDirtyFlag();
        }
    }
}
#endif //#ifndef GFC_NO_IME_SUPPORT

UInt32 GFxEditTextCharacter::GetSelectedTextColor() const
{
    if (pDocument->HasEditorKit())
    {
        return pDocument->GetEditorKit()->GetActiveSelectionTextColor();
    }
    return GFX_ACTIVE_SEL_TEXTCOLOR;
}

UInt32 GFxEditTextCharacter::GetSelectedBackgroundColor() const
{
    if (pDocument->HasEditorKit())
    {
        return pDocument->GetEditorKit()->GetActiveSelectionBackgroundColor();
    }
    return GFX_ACTIVE_SEL_BKCOLOR;
}

void GFxEditTextCharacter::UpdateUrlStyles()
{
#ifndef GFC_NO_CSS_SUPPORT
    if (pCSSData)
    {
        for (UInt i = 0, zonesNum = (UInt)pCSSData->UrlZones.Count(); i < zonesNum; ++i)
        {
            if (pCSSData->pASStyleSheet)
            {
                const GFxTextStyle* pAstyle = 
                    pCSSData->pASStyleSheet->GetTextStyleManager()->GetStyle(GFxTextStyleKey::CSS_Tag, "a");

                const GFxTextStyle* pALinkStyle = 
                    pCSSData->pASStyleSheet->GetTextStyleManager()->GetStyle(GFxTextStyleKey::CSS_Tag, "a:link");

                GFxTextFormat fmt(GMemory::GetHeapByAddress(this));
                if (pAstyle)
                    fmt = fmt.Merge(pAstyle->TextFormat);
                if (pALinkStyle)
                    fmt = fmt.Merge(pALinkStyle->TextFormat);

                // save current formatting data first
                UPInt stPos = (UPInt)pCSSData->UrlZones[i].FirstIndex(), endPos = (UPInt)pCSSData->UrlZones[i].NextIndex();

                pDocument->SetTextFormat(fmt, stPos, endPos);

                // Update saved fmt
                pCSSData->UrlZones[i].GetData().SavedFmt = 
                    *pDocument->GetStyledText()->CopyStyledText(stPos, endPos);
            }
        }
    }
#endif
}

void GFxEditTextCharacter::ChangeUrlFormat(LinkEvent event, UInt mouseIndex, const GRange* purlRange)
{
#ifndef GFC_NO_CSS_SUPPORT
    if (!HasStyleSheet())
        return;

    const char* styleName = NULL;

    //CSSHolder::UrlZone* purlZone = NULL;
    GRangeData<CSSHolder::UrlZone>* purlZone = NULL;
    switch(event)
    {
    case Link_press:
        if (pCSSData->MouseState[mouseIndex].UrlZoneIndex > 0)
        {
            if (pCSSData->MouseState[mouseIndex].HitBit)
                return; // this mouse already has hit the url
            pCSSData->MouseState[mouseIndex].HitBit = true;
            UPInt urlZoneIdx = pCSSData->MouseState[mouseIndex].UrlZoneIndex - 1;
            purlZone = &pCSSData->UrlZones[urlZoneIdx];
            if (purlZone->GetData().HitCount > 0)
            {
                ++purlZone->GetData().HitCount;
                return; // hit counter is not zero - do nothing
            }
        }
        else
        {
            for (UInt i = 0, zonesNum = (UInt)pCSSData->UrlZones.Count(); i < zonesNum; ++i)
            {
                if (pCSSData->UrlZones[i].Intersects(*purlRange))
                {
                    purlZone = &pCSSData->UrlZones[i];
                    //purlZone->StartPos = (UPInt)pCSSData->UrlZones[i].Index;
                    //purlZone->EndPos = purlZone->StartPos + pCSSData->UrlZones[i].Length;
                    pCSSData->MouseState[mouseIndex].UrlZoneIndex = i + 1;
                    pCSSData->MouseState[mouseIndex].HitBit = true;
                    break;
                }
            }
        }
        if (purlZone)
        {
            if (purlZone->GetData().HitCount++ == 0)
            {
                styleName = "a:active";
            }
            else
                return;
        }
        break;
    case Link_release:
        if (pCSSData->MouseState[mouseIndex].UrlZoneIndex > 0)
        {
            if (!pCSSData->MouseState[mouseIndex].HitBit)
                return; // this mouse didn't hit the url
            pCSSData->MouseState[mouseIndex].HitBit = false;
            UPInt urlZoneIdx = pCSSData->MouseState[mouseIndex].UrlZoneIndex - 1;
            if (!pCSSData->MouseState[mouseIndex].OverBit) // if neither hit nor over left - clean up
                pCSSData->MouseState[mouseIndex].UrlZoneIndex = 0;
            purlZone = &pCSSData->UrlZones[urlZoneIdx];
            if (purlZone->GetData().HitCount > 0)
            {
                --purlZone->GetData().HitCount;
                if (purlZone->GetData().HitCount != 0)
                    return; // hit count is still bigger than zero, return
                if (purlZone->GetData().OverCount > 0)
                    styleName = "a:hover";
            }
        }
        break;
    case Link_rollover:
        if (pCSSData->MouseState[mouseIndex].UrlZoneIndex > 0)
        {
            if (pCSSData->MouseState[mouseIndex].OverBit)
                return; // this mouse already has rolled over the url
            pCSSData->MouseState[mouseIndex].OverBit = true;
            UPInt urlZoneIdx = pCSSData->MouseState[mouseIndex].UrlZoneIndex - 1;
            purlZone = &pCSSData->UrlZones[urlZoneIdx];
            if (purlZone->GetData().OverCount > 0)
            {
                ++purlZone->GetData().OverCount;
                return; // over counter is not zero - do nothing
            }
        }
        else
        {
            for (UInt i = 0, zonesNum = (UInt)pCSSData->UrlZones.Count(); i < zonesNum; ++i)
            {
                if (pCSSData->UrlZones[i].Intersects(*purlRange))
                {
                    purlZone = &pCSSData->UrlZones[i];
                    //purlZone->StartPos = (UPInt)pCSSData->UrlZones[i].Index;
                    //purlZone->EndPos = purlZone->StartPos + pCSSData->UrlZones[i].Length;
                    pCSSData->MouseState[mouseIndex].UrlZoneIndex = i + 1;
                    pCSSData->MouseState[mouseIndex].OverBit = true;
                    break;
                }
            }
        }
        if (purlZone)
        {
            if (purlZone->GetData().HitCount == 0 && purlZone->GetData().OverCount++ == 0)
            {
                styleName = "a:hover";
            }
            else
                return;
        }
        break;
    case Link_rollout:
        if (pCSSData->MouseState[mouseIndex].UrlZoneIndex > 0)
        {
            if (!pCSSData->MouseState[mouseIndex].OverBit)
                return; // this mouse didn't hit the url
            pCSSData->MouseState[mouseIndex].OverBit = false;
            UPInt urlZoneIdx = pCSSData->MouseState[mouseIndex].UrlZoneIndex - 1;
            purlZone = &pCSSData->UrlZones[urlZoneIdx];
            if (!pCSSData->MouseState[mouseIndex].HitBit) // if neither hit nor over left - clean up
                pCSSData->MouseState[mouseIndex].UrlZoneIndex = 0;
            if (purlZone->GetData().OverCount > 0)
            {
                --purlZone->GetData().OverCount;
                if (purlZone->GetData().OverCount != 0)
                    return; // over count is still bigger than zero, return
                if (purlZone->GetData().HitCount > 0)
                    styleName = "a:active";
            }
        }
        break;
    }

    if (purlZone && purlZone->GetData().SavedFmt)
    {
        // restore originally saved format
        UPInt stPos = (UPInt)purlZone->FirstIndex();
        UPInt enPos = (UPInt)purlZone->NextIndex();
        pDocument->RemoveText(stPos, enPos);
        pDocument->InsertStyledText(*purlZone->GetData().SavedFmt.GetPtr(), stPos);
    }

    if (styleName)
    {
        // now need to apply a:hover style from CSS, combined with "a:link" and "a" ones
        const GFxTextStyle* pstyle = 
            pCSSData->pASStyleSheet->GetTextStyleManager()->GetStyle(GFxTextStyleKey::CSS_Tag, styleName);
        if (pstyle && purlZone)
        {
            const GFxTextStyle* pAstyle = 
                pCSSData->pASStyleSheet->GetTextStyleManager()->GetStyle(GFxTextStyleKey::CSS_Tag, "a");

            const GFxTextStyle* pALinkStyle = 
                pCSSData->pASStyleSheet->GetTextStyleManager()->GetStyle(GFxTextStyleKey::CSS_Tag, "a:link");

            GFxTextFormat fmt(GMemory::GetHeapByAddress(this));
            if (pAstyle)
                fmt = fmt.Merge(pAstyle->TextFormat);
            if (pALinkStyle)
                fmt = fmt.Merge(pALinkStyle->TextFormat);
            fmt = fmt.Merge(pstyle->TextFormat);

            // save current formatting data first
            UPInt stPos = (UPInt)purlZone->FirstIndex(), endPos = (UPInt)purlZone->NextIndex();

            pDocument->SetTextFormat(fmt, stPos, endPos);
        }
    }
#else
    GUNUSED3(event, mouseIndex, purlRange);
#endif // GFC_NO_CSS_SUPPORT
}

//////////////////////////////////////////
GASTextFieldObject::GASTextFieldObject(GASStringContext *psc, GASObject* pprototype)
: GASObject(psc)
{ 
    pIMECompositionStringStyles = NULL;
    Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
}

GASTextFieldObject::GASTextFieldObject(GASEnvironment* penv)
: GASObject(penv)
{ 
    pIMECompositionStringStyles = NULL;
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_TextField)); // this ctor is used for prototype obj only
}

GASTextFieldObject::GASTextFieldObject(GASGlobalContext* gCtxt, GFxASCharacter* ptextfield) 
: GASObject(gCtxt->GetGC()), pTextField(ptextfield)
{
    GASStringContext* psc = ptextfield->GetASEnvironment()->GetSC();
    //Set__proto__(psc, gCtxt->GetPrototype(GASBuiltin_TextField));
    Set__proto__(psc, ptextfield->Get__proto__());
    pIMECompositionStringStyles = NULL;
}

GASTextFieldObject::~GASTextFieldObject()
{
    delete pIMECompositionStringStyles;
}

GFxTextIMEStyle*   GASTextFieldObject::GetIMECompositionStringStyles()
{
    if (pIMECompositionStringStyles)
        return pIMECompositionStringStyles;
    GASObject* pproto = Get__proto__();
    if (pproto && pproto->GetObjectType() == GASObject::Object_TextFieldASObject)
    {
        GASTextFieldObject* ptextproto = static_cast<GASTextFieldObject*>(pproto);
        return ptextproto->GetIMECompositionStringStyles();
    }
    return NULL;
}

void GASTextFieldObject::SetIMECompositionStringStyles(const GFxTextIMEStyle& imeStyles)
{
    if (!pIMECompositionStringStyles)
        pIMECompositionStringStyles = GHEAP_AUTO_NEW(this) GFxTextIMEStyle(imeStyles);
    else
        *pIMECompositionStringStyles = imeStyles;
}

//////
static const GASNameFunction GAS_TextFieldFunctionTable[] = 
{
    { "getDepth",           &GFxASCharacter::CharacterGetDepth },
    { "getTextFormat",      &GFxEditTextCharacter::GetTextFormat },
    { "getNewTextFormat",   &GFxEditTextCharacter::GetNewTextFormat },
    { "setTextFormat",      &GFxEditTextCharacter::SetTextFormat },
    { "setNewTextFormat",   &GFxEditTextCharacter::SetNewTextFormat },
    { "replaceSel",         &GFxEditTextCharacter::ReplaceSel },
    { "replaceText",        &GFxEditTextCharacter::ReplaceText },
    { "removeTextField",    &GFxEditTextCharacter::RemoveTextField },

    { 0, 0 }
};

#ifndef GFC_NO_TEXTFIELD_EXTENSIONS
static const GASNameFunction GAS_TextFieldExtFunctionTable[] = 
{
    { "appendText",             &GFxEditTextCharacter::AppendText },
    { "appendHtml",             &GFxEditTextCharacter::AppendHtml },
    { "copyToClipboard",        &GFxEditTextCharacter::CopyToClipboard },
    { "cutToClipboard",         &GFxEditTextCharacter::CutToClipboard },
    { "getCharBoundaries",      &GFxEditTextCharacter::GetCharBoundaries },
    { "getCharIndexAtPoint",    &GFxEditTextCharacter::GetCharIndexAtPoint },
    { "getExactCharBoundaries", &GFxEditTextCharacter::GetExactCharBoundaries },
    { "getFirstCharInParagraph",&GFxEditTextCharacter::GetFirstCharInParagraph },
    { "getIMECompositionStringStyle",  &GASTextFieldProto::GetIMECompositionStringStyle },
    { "getLineIndexAtPoint",    &GFxEditTextCharacter::GetLineIndexAtPoint },
    { "getLineIndexOfChar",     &GFxEditTextCharacter::GetLineIndexOfChar },
    { "getLineOffset",          &GFxEditTextCharacter::GetLineOffset },
    { "getLineMetrics",         &GFxEditTextCharacter::GetLineMetrics },
    { "getLineText",            &GFxEditTextCharacter::GetLineText },
    { "getLineLength",          &GFxEditTextCharacter::GetLineLength },
    { "pasteFromClipboard",     &GFxEditTextCharacter::PasteFromClipboard },
    { "setIMECompositionStringStyle",  &GASTextFieldProto::SetIMECompositionStringStyle },
#ifndef GFC_NO_FXPLAYER_AS_BITMAPDATA
    { "setImageSubstitutions",  &GFxEditTextCharacter::SetImageSubstitutions },
    { "updateImageSubstitution",&GFxEditTextCharacter::UpdateImageSubstitution },
#endif

    { 0, 0 }
};
#endif //GFC_NO_TEXTFIELD_EXTENSIONS

GASTextFieldProto::GASTextFieldProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor) :
    GASPrototype<GASTextFieldObject>(psc, prototype, constructor)
{
    GASAsBroadcaster::InitializeProto(psc, this);
    InitFunctionMembers(psc, GAS_TextFieldFunctionTable);
    SetConstMemberRaw(psc, "scroll", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "hscroll", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "maxscroll", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "maxhscroll", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);

    SetConstMemberRaw(psc, "background", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "backgroundColor", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "border", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "borderColor", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "bottomScroll", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "mouseWheelEnabled", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);

    SetConstMemberRaw(psc, "antiAliasType", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "autoSize", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "condenseWhite", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "embedFonts", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    // there is no method to retrieve filters.
    //SetConstMemberRaw(psc, "filters", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "html", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "htmlText", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "length", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "maxChars", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);
    SetConstMemberRaw(psc, "multiline", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);    
    SetConstMemberRaw(psc, "password", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);      
    SetConstMemberRaw(psc, "restrict", GASValue::NULLTYPE, GASPropFlags::PropFlag_DontDelete);     
    SetConstMemberRaw(psc, "selectable", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);    
    SetConstMemberRaw(psc, "styleSheet", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);    
    // can't add tabEnabled here, since its presence will stop prototype chain lookup, even if
    // it is UNSET or "undefined".
    //SetConstMemberRaw(psc, "tabEnabled", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete); 
    SetConstMemberRaw(psc, "tabIndex", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);    
    SetConstMemberRaw(psc, "text", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);  
    SetConstMemberRaw(psc, "textColor", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);  
    SetConstMemberRaw(psc, "textHeight", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);  
    SetConstMemberRaw(psc, "textWidth", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);  
    SetConstMemberRaw(psc, "type", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);  
    SetConstMemberRaw(psc, "variable", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);  
    SetConstMemberRaw(psc, "wordWrap", GASValue::UNSET, GASPropFlags::PropFlag_DontDelete);  

#ifndef GFC_NO_TEXTFIELD_EXTENSIONS
    // init extensions
    InitFunctionMembers(psc, GAS_TextFieldExtFunctionTable);
#endif

#ifndef GFC_NO_IME_SUPPORT
    // create default styles for IME
    SetIMECompositionStringStyles(GFxTextCompositionString::GetDefaultStyles());
#endif // GFC_NO_IME_SUPPORT
}

static const GASNameFunction GAS_TextFieldStaticFunctionTable[] = 
{
    { "getFontList",  &GASTextFieldCtorFunction::GetFontList },
    { 0, 0 }
};

GASTextFieldCtorFunction::GASTextFieldCtorFunction(GASStringContext *psc)
: GASCFunctionObject(psc, GlobalCtor)
{
    GASNameFunction::AddConstMembers(
        this, psc, GAS_TextFieldStaticFunctionTable, 
        GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
}

GASObject* GASTextFieldCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{
    return GHEAP_NEW(penv->GetHeap()) GASTextFieldObject(penv);
}

void GASTextFieldCtorFunction::GlobalCtor(const GASFnCall& fn) 
{
    GUNUSED(fn);
    //fn.Result->SetAsObject(GPtr<GASObject>(*new GASTextFieldObject()));
}

GASFunctionRef GASTextFieldCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef  ctor(*GHEAP_NEW(pgc->GetHeap()) GASTextFieldCtorFunction(&sc));
    GPtr<GASObject> proto = *GHEAP_NEW(pgc->GetHeap()) GASTextFieldProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_TextField, proto);
    pgc->pGlobal->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_TextField), GASValue(ctor));

#ifndef GFC_NO_CSS_SUPPORT
    pgc->AddBuiltinClassRegistry<GASBuiltin_StyleSheet, GASStyleSheetCtorFunction>(sc, ctor.GetObjectPtr()); 
#endif // GFC_NO_CSS_SUPPORT

    return ctor;
}


void GASTextFieldCtorFunction::GetFontList(const GASFnCall& fn)
{
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    GFxMovieDefImpl* pmoviedef = static_cast<GFxMovieDefImpl*>(proot->GetMovieDef());

    // get fonts from ..
    GStringHash<GString> fontnames;
    // fontresource
    struct FontsVisitor : public GFxMovieDef::ResourceVisitor 
    { 
        GStringHash<GString>*       pFontNames;

        FontsVisitor(GStringHash<GString>* pfontnames) : pFontNames(pfontnames) {}

        virtual void Visit(GFxMovieDef*, GFxResource* presource, 
            GFxResourceId, const char*) 
        { 
            if (presource->GetResourceType() == GFxResource::RT_Font)
            {
                GFxFontResource* pfontResource = static_cast<GFxFontResource*>(presource);
                GString fontname(pfontResource->GetName());
                pFontNames->Set(fontname, fontname);
            }
        }
    } fontsVisitor(&fontnames);
    pmoviedef->VisitResources(&fontsVisitor, GFxMovieDef::ResVisit_Fonts);
    // fontlib
    GFxFontLib* pfontlib = proot->GetFontLib();
    if (pfontlib)
    {
        pfontlib->LoadFontNames(fontnames);
    }
    // fontprovider
    GFxFontProvider* pfontprovider = proot->GetFontProvider();
    if (pfontprovider)
    {
        pfontprovider->LoadFontNames(fontnames);
    }

    // returns an array of strings
    GPtr<GASArrayObject> parr = *GHEAP_NEW(fn.Env->GetHeap()) GASArrayObject(fn.Env);
    for (GStringHash<GString>::ConstIterator iter = fontnames.Begin(); iter != fontnames.End(); ++iter)
    {
        GASValue name(fn.Env->CreateString(iter->First));
        parr->PushBack(name);
    }
    fn.Result->SetAsObject(parr);    
}

// Read CSMTextSettings tag
void    GSTDCALL GFx_CSMTextSettings(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);        
    GASSERT(tagInfo.TagType == GFxTag_CSMTextSettings);

    UInt16  characterId = p->ReadU16();

    GFxStream*  pin = p->GetStream();    

    UInt  flagType  = pin->ReadUInt(2);
    UInt  gridFit   = pin->ReadUInt(3);
    Float thickness = pin->ReadFloat();
    Float sharpness = pin->ReadFloat();

    if (pin->IsVerboseParse())
    {
        static const char* const gridfittypes[] = { "None", "Pixel", "LCD" };
        p->LogParse("CSMTextSettings, id = %d\n", characterId);
        p->LogParse("  FlagType = %s, GridFit = %s\n", 
            (flagType == 0) ? "System" : "Internal", gridfittypes[gridFit] );
        p->LogParse("  Thinkness = %f, Sharpnesss = %f\n", 
            (float)thickness, (float)sharpness);
    }

    GFxResourceHandle handle;
    if (p->GetResourceHandle(&handle, GFxResourceId(characterId)))
    {
        GFxResource* rptr = handle.GetResourcePtr();
        if (rptr)
        { 
            if (rptr->GetResourceType() == GFxResource::RT_EditTextDef)
            {
                GFxEditTextCharacterDef* pch = static_cast<GFxEditTextCharacterDef*>(rptr);
                pch->SetAAForReadability();
            }
            else if (rptr->GetResourceType() == GFxResource::RT_TextDef)
            {
                GFxStaticTextCharacterDef* pch = static_cast<GFxStaticTextCharacterDef*>(rptr);
                pch->SetAAForReadability();
            }
        }
    }
}

GFxTextHighlightInfo GASTextFieldProto::ParseStyle
    (const GASFnCall& fn, UInt paramIndex, const GFxTextHighlightInfo& initialHInfo)
{
    GFxTextHighlightInfo hinfo = initialHInfo;
    if (fn.NArgs >= 1)
    {
        GPtr<GASObject> pobj = fn.Arg(paramIndex).ToObject(fn.Env);
        if (pobj)
        {
            GASValue val;
            if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("textColor"), &val))
            {
                if (val.ToString(fn.Env) == "none")
                    hinfo.ClearTextColor();
                else
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        hinfo.SetTextColor(val.ToUInt32(fn.Env) | 0xFF000000u);
                }
            }
            if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("backgroundColor"), &val))
            {
                if (val.ToString(fn.Env) == "none")
                    hinfo.ClearBackgroundColor();
                else
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        hinfo.SetBackgroundColor(val.ToUInt32(fn.Env) | 0xFF000000u);
                }
            }
            if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("underlineColor"), &val))
            {
                if (val.ToString(fn.Env) == "none")
                    hinfo.ClearUnderlineColor();
                else
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        hinfo.SetUnderlineColor(val.ToUInt32(fn.Env) | 0xFF000000u);
                }
            }
            if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("underlineStyle"), &val))
            {
                GASString str = val.ToString(fn.Env);
                if (str == "dotted")
                    hinfo.SetDottedUnderline();
                else if (str == "single")
                    hinfo.SetSingleUnderline();
                else if (str == "thick")
                    hinfo.SetThickUnderline();
                else if (str == "ditheredSingle")
                    hinfo.SetDitheredSingleUnderline();
                else if (str == "ditheredThick")
                    hinfo.SetDitheredThickUnderline();
                else 
                    hinfo.ClearUnderlineStyle();
            }
        }
    }
    return hinfo;
}

void GASTextFieldProto::MakeStyle(const GASFnCall& fn, const GFxTextHighlightInfo& hinfo)
{
    GPtr<GASObject> pobj = *GHEAP_NEW(fn.Env->GetHeap()) GASObject(fn.Env);
    if (hinfo.HasUnderlineStyle())
    {
        const char* pstyle;
        switch(hinfo.GetUnderlineStyle())
        {
        case GFxTextHighlightInfo::Underline_Single: pstyle = "single"; break;
        case GFxTextHighlightInfo::Underline_Thick:  pstyle = "thick"; break;
        case GFxTextHighlightInfo::Underline_Dotted: pstyle = "dotted"; break;
        case GFxTextHighlightInfo::Underline_DitheredSingle: pstyle = "ditheredSingle"; break;
        case GFxTextHighlightInfo::Underline_DitheredThick:  pstyle = "ditheredThick"; break;
        default: pstyle = NULL;
        }
        if (pstyle)
            pobj->SetConstMemberRaw(fn.Env->GetSC(), "underlineStyle", GASValue(fn.Env->CreateConstString(pstyle)));
    }
    if (hinfo.HasUnderlineColor())
    {
        GASNumber c = (GASNumber)(hinfo.GetUnderlineColor().ToColor32() & 0xFFFFFFu);
        pobj->SetConstMemberRaw(fn.Env->GetSC(), "underlineColor", GASValue(c));
    }
    if (hinfo.HasBackgroundColor())
    {
        GASNumber c = (GASNumber)(hinfo.GetBackgroundColor().ToColor32() & 0xFFFFFFu);
        pobj->SetConstMemberRaw(fn.Env->GetSC(), "backgroundColor", GASValue(c));
    }
    if (hinfo.HasTextColor())
    {
        GASNumber c = (GASNumber)(hinfo.GetTextColor().ToColor32() & 0xFFFFFFu);
        pobj->SetConstMemberRaw(fn.Env->GetSC(), "textColor", GASValue(c));
    }
    fn.Result->SetAsObject(pobj);
}

static GFxTextIMEStyle::Category GFx_StringToIMEStyleCategory(const GASString& categoryStr)
{
    GFxTextIMEStyle::Category cat = GFxTextIMEStyle::SC_MaxNum;
    if (categoryStr == "compositionSegment")
        cat = GFxTextIMEStyle::SC_CompositionSegment;
    else if (categoryStr == "clauseSegment")
        cat = GFxTextIMEStyle::SC_ClauseSegment;
    else if (categoryStr == "convertedSegment")
        cat = GFxTextIMEStyle::SC_ConvertedSegment;
    else if (categoryStr == "phraseLengthAdj")
        cat = GFxTextIMEStyle::SC_PhraseLengthAdj;
    else if (categoryStr == "lowConfSegment")
        cat = GFxTextIMEStyle::SC_LowConfSegment;
    return cat;
}

void GASTextFieldProto::SetIMECompositionStringStyle(const GASFnCall& fn)
{
    if (fn.ThisPtr)
    {
        GPtr<GASTextFieldObject> pasTextObj;
        if (fn.ThisPtr->GetObjectType() == Object_TextField)
        {
            GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
            pasTextObj = pthis->ASTextFieldObj;
        }
        else if (fn.ThisPtr->GetObjectType() == Object_TextFieldASObject)
        {
            pasTextObj = static_cast<GASTextFieldObject*>(fn.ThisPtr);
        }
        if (pasTextObj)
        {
            if (fn.NArgs >= 1)
            {
                GASString categoryStr(fn.Arg(0).ToString(fn.Env));
                GFxTextIMEStyle::Category cat = GFx_StringToIMEStyleCategory(categoryStr);
                if (cat >= GFxTextIMEStyle::SC_MaxNum)
                    return;

                GFxTextIMEStyle* pimeStyles = pasTextObj->GetIMECompositionStringStyles();
                GFxTextIMEStyle imeStyles;
                if (pimeStyles)
                    imeStyles = *pimeStyles;
                imeStyles.SetElement(cat, ParseStyle(fn, 1, imeStyles.GetElement(cat)));
                pasTextObj->SetIMECompositionStringStyles(imeStyles);
            }
        }
    }
}

void GASTextFieldProto::GetIMECompositionStringStyle(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    if (fn.ThisPtr)
    {
        GPtr<GASTextFieldObject> pasTextObj;
        if (fn.ThisPtr->GetObjectType() == Object_TextField)
        {
            GFxEditTextCharacter* pthis = static_cast<GFxEditTextCharacter*>(fn.ThisPtr);
            pasTextObj = pthis->ASTextFieldObj;
        }
        else if (fn.ThisPtr->GetObjectType() == Object_TextFieldASObject)
        {
            pasTextObj = static_cast<GASTextFieldObject*>(fn.ThisPtr);
        }
        if (pasTextObj)
        {
            GFxTextIMEStyle* pimeStyles = pasTextObj->GetIMECompositionStringStyles();
            if (pimeStyles)
            {
                GASString categoryStr(fn.Arg(0).ToString(fn.Env));
                GFxTextIMEStyle::Category cat = GFx_StringToIMEStyleCategory(categoryStr);
                if (cat >= GFxTextIMEStyle::SC_MaxNum)
                    return;
                const GFxTextHighlightInfo& hinfo = pimeStyles->GetElement(cat);
                MakeStyle(fn, hinfo);
            }
        }
    }
}

//////////////////////////////////////////////////
GFxTextClipboard::GFxTextClipboard() : 
    GFxState(State_TextClipboard), pStyledText(NULL)
{
}

GFxTextClipboard::~GFxTextClipboard() 
{
    ReleaseStyledText();
}

void GFxTextClipboard::ReleaseStyledText()
{
    if (pStyledText)
    {
        pStyledText->Release();
        pStyledText = NULL;
    }
}

void GFxTextClipboard::SetPlainText(const wchar_t* ptext, UPInt len)
{
    PlainText.SetString(ptext, len);
    OnTextStore(PlainText.ToWStr(), PlainText.GetLength());
}

void GFxTextClipboard::SetText(const GString& str)
{
    ReleaseStyledText();
    UPInt len = str.GetLength();
    PlainText.Resize(len + 1);
    GUTF8Util::DecodeString(PlainText.GetBuffer(), str.ToCStr(), (SPInt)len);
    OnTextStore(PlainText.ToWStr(), PlainText.GetLength());
}

void GFxTextClipboard::SetText(const wchar_t* ptext, UPInt len)
{
    ReleaseStyledText();
    SetPlainText(ptext, len);
}

const GFxWStringBuffer& GFxTextClipboard::GetText() const
{
    return PlainText;
}

void GFxTextClipboard::SetStyledText(class GFxStyledText* pstyledText)
{
    if (pStyledText)
        pStyledText->Release();
    // need to make a copy of passed styledText into the global heap one.
    GMemoryHeap* pheap = GMemory::GetGlobalHeap();
    GPtr<GFxTextAllocator> pallocator = *GHEAP_NEW(pheap) GFxTextAllocator(pheap, GFxTextAllocator::Flags_Global);
    pStyledText = GHEAP_NEW(pheap) GFxStyledText(pallocator);
    pstyledText->CopyStyledText(pStyledText);
}

void GFxTextClipboard::SetTextAndStyledText(const wchar_t* ptext, UPInt len, class GFxStyledText* pstyledText)
{
    SetText(ptext, len);
    SetStyledText(pstyledText);
}

class GFxStyledText* GFxTextClipboard::GetStyledText() const
{
    return pStyledText;
}

#ifndef GFC_NO_TEXT_INPUT_SUPPORT
////////////////////////////////////////
// GFxTextKeyMap

GFxTextKeyMap::GFxTextKeyMap() : GFxState(State_TextKeyMap)
{

}

namespace
{
    struct KeyMapEntryComparator
    {
        static int Compare(const GFxTextKeyMap::KeyMapEntry& p1, const GFxTextKeyMap::KeyMapEntry& p2)
        {
            return (int)p1.KeyCode - (int)p2.KeyCode;
        }
        static int Less(const GFxTextKeyMap::KeyMapEntry& p1, const GFxTextKeyMap::KeyMapEntry& p2)
        {
            return Compare(p1, p2) < 0;
        }
    };

    struct KeyCodeComparator
    {
        static int Compare(const GFxTextKeyMap::KeyMapEntry& p1, UInt keyCode)
        {
            return (int)p1.KeyCode - (int)keyCode;
        }
        static int Less(const GFxTextKeyMap::KeyMapEntry& p1, UInt keyCode)
        {
            return Compare(p1, keyCode) < 0;
        }
    };
}

void GFxTextKeyMap::AddKeyEntry(const KeyMapEntry& keyMapEntry)
{
    UPInt i = G_LowerBound(Map, keyMapEntry, KeyMapEntryComparator::Less);
    Map.InsertAt(i, keyMapEntry);
}

const GFxTextKeyMap::KeyMapEntry* GFxTextKeyMap::FindFirstEntry(UInt keyCode) const
{
    UPInt i = G_LowerBound(Map, keyCode, KeyCodeComparator::Less);
    if (i < Map.GetSize() && Map[i].KeyCode == keyCode)
        return &Map[i];
    return NULL;
}

const GFxTextKeyMap::KeyMapEntry* GFxTextKeyMap::FindNextEntry(const KeyMapEntry* pcur) const
{
    UPInt curIdx = pcur - &Map[0];
    if (curIdx + 1 < Map.GetSize() && Map[curIdx + 1].KeyCode == pcur->KeyCode)
    {
        return &Map[curIdx + 1];
    }
    return NULL;
}

const GFxTextKeyMap::KeyMapEntry* GFxTextKeyMap::Find(UInt keyCode, const GFxSpecialKeysState& specKeys, KeyState state) const
{
    const KeyMapEntry* p = FindFirstEntry(keyCode);
    while(p)
    {
        if (p->State == state && (p->SpecKeysPressed & specKeys.States) == p->SpecKeysPressed)
            return p;
        p = FindNextEntry(p);
    }
    return NULL;
}


GFxTextKeyMap* GFxTextKeyMap::InitWindowsKeyMap()
{
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_EnterSelectionMode, GFxKey::Shift));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_LeaveSelectionMode, GFxKey::Shift, 0, State_Up));

    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_Up, GFxKey::Up));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_Down, GFxKey::Down));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_Left, GFxKey::Left));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_Right, GFxKey::Right));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_PageUp, GFxKey::PageUp));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_PageDown, GFxKey::PageDown));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_LineHome, GFxKey::Home));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_LineEnd, GFxKey::End));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_PageHome, GFxKey::PageUp, GFxSpecialKeysState::Key_CtrlPressed));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_PageEnd, GFxKey::PageDown, GFxSpecialKeysState::Key_CtrlPressed));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_DocHome, GFxKey::Home, GFxSpecialKeysState::Key_CtrlPressed));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_DocEnd, GFxKey::End, GFxSpecialKeysState::Key_CtrlPressed));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_Backspace, GFxKey::Backspace));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_Delete, GFxKey::Delete));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_Return, GFxKey::Return));
    
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_Copy, GFxKey::C, GFxSpecialKeysState::Key_CtrlPressed));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_Copy, GFxKey::Insert, GFxSpecialKeysState::Key_CtrlPressed));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_Paste, GFxKey::V, GFxSpecialKeysState::Key_CtrlPressed));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_Paste, GFxKey::Insert, GFxSpecialKeysState::Key_ShiftPressed));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_Cut, GFxKey::X, GFxSpecialKeysState::Key_CtrlPressed));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_Cut, GFxKey::Delete, GFxSpecialKeysState::Key_ShiftPressed));
    AddKeyEntry(GFxTextKeyMap::KeyMapEntry(KeyAct_SelectAll, GFxKey::A, GFxSpecialKeysState::Key_CtrlPressed));
    return this;
}

GFxTextKeyMap* GFxTextKeyMap::InitMacKeyMap()
{
    return InitWindowsKeyMap();//TODO
}
#endif //GFC_NO_TEXT_INPUT_SUPPORT
