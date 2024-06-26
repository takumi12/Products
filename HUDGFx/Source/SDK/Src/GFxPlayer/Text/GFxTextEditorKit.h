/**********************************************************************

Filename    :   GFxTextEditorKit.h
Content     :   Editor implementation
Created     :   April 29, 2008
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_TEXT_GFXEDITORKIT_H
#define INC_TEXT_GFXEDITORKIT_H

#include "Text/GFxTextDocView.h"

// This struct contains styles for IME composition string. Used
// by the GFxTextCompositionString.
struct GFxTextIMEStyle : public GNewOverrideBase<GFxStatMV_Other_Mem>
{
    enum Category
    {
        SC_CompositionSegment   = 0,
        SC_ClauseSegment        = 1,
        SC_ConvertedSegment     = 2,
        SC_PhraseLengthAdj      = 3,
        SC_LowConfSegment       = 4,

        SC_MaxNum
    };
    GFxTextHighlightInfo HighlightStyles[SC_MaxNum];
    UInt8                PresenceMask;

    GFxTextIMEStyle() : PresenceMask(0) {}

    const GFxTextHighlightInfo& GetElement(UPInt n) const
    {
        return HighlightStyles[n];
    }
    bool HasElement(UPInt n) const 
    { 
        GASSERT(n < sizeof(PresenceMask)*8); 
        return (PresenceMask & (1 << n)) != 0; 
    }

    void SetElement(UPInt n, const GFxTextHighlightInfo& hinfo) 
    { 
        GASSERT(n < sizeof(PresenceMask)*8); 
        PresenceMask |= (1 << n);
        HighlightStyles[n] = hinfo;
    }
    void Unite(const GFxTextIMEStyle& st)
    {
        for (UInt i = 0; i < SC_MaxNum; ++i)
        {
            if (st.HasElement(i))
                SetElement(i, st.GetElement(i));
        }
    }
};

#ifndef GFC_NO_IME_SUPPORT
// A composition string implementation to be used within GFxTextEditorKit.
class GFxTextCompositionString : public GRefCountBaseNTS<GFxTextCompositionString, GFxStatMV_Text_Mem>
{
    friend class GFxTextEditorKit;

    GPtr<GFxTextFormat> pDefaultFormat;
    GFxTextEditorKit*   pEditorKit;
    GPtr<GFxTextAllocator> pAllocator;
    GFxTextParagraph    String;
    UPInt               CursorPos;
    bool                HasHighlightingFlag;
    GFxTextIMEStyle     Styles;
    UInt                HighlightIds[GFxTextIMEStyle::SC_MaxNum*2];
    UInt8               HighlightIdsUsed; // number of highlight ids used

    GFxTextParagraph*   GetSourceParagraph();
    void                Reformat();
public:
    GFxTextCompositionString(GFxTextEditorKit* peditorKit);
    ~GFxTextCompositionString();

    GFxTextAllocator*   GetAllocator() const { return pAllocator; }

    void SetText(const wchar_t*, UPInt nchars = GFC_MAX_UPINT);
    const wchar_t* GetText() const { return String.GetText(); }

    void SetPosition(UPInt pos);
    UPInt GetPosition() const { return String.GetStartIndex(); }

    UPInt GetLength() const { return String.GetLength(); }

    GFxTextParagraph& GetParagraph() { return String; }
    GFxTextFormat* GetTextFormat(UPInt pos) { return String.GetTextFormatPtr(pos); }
    GFxTextFormat* GetDefaultTextFormat() const { return pDefaultFormat; }

    void  SetCursorPosition(UPInt pos);
    UPInt GetCursorPosition() const { return CursorPos; }

    void  HighlightText(UPInt pos, UPInt len, GFxTextIMEStyle::Category styleCategory);

    void  UseStyles(const GFxTextIMEStyle&);

    void  ClearHighlighting();
    bool  HasHighlighting() const { return HasHighlightingFlag; }

    static GFxTextIMEStyle GetDefaultStyles();
};
#endif //#ifndef GFC_NO_IME_SUPPORT

#ifndef GFC_NO_TEXT_INPUT_SUPPORT
// This class provides functionality for editing and selecting rich text.
// It is used in conjunction with GFxTextDocView.
class GFxTextEditorKit : public GRefCountBaseNTS<GFxTextEditorKit, GFxStatMV_Text_Mem>
{
    friend class GFxTextDocView;
    friend class GFxTextCompositionString;
protected:
    typedef GRenderer::Matrix       Matrix;
    typedef GRenderer::Cxform       Cxform;

    enum 
    {
        Flags_ReadOnly              = 1,
        Flags_Selectable            = 2,
        Flags_UseRichClipboard      = 4,
        Flags_CursorBlink           = 8,
        Flags_CursorTimerBlocked    = 0x10,
        Flags_MouseCaptured         = 0x20,
        Flags_ShiftPressed          = 0x40,
        Flags_OverwriteMode         = 0x80,
        Flags_WideCursor            = 0x100,
        Flags_LastDoubleClick       = 0x200,
        Flags_Focused               = 0x400
    };
    GPtr<GFxTextDocView>    pDocView;
    GPtr<GFxTextClipboard>  pClipboard;
    GPtr<GFxTextKeyMap>     pKeyMap;
#ifndef GFC_NO_IME_SUPPORT
    GPtr<GFxTextCompositionString> pComposStr;
#endif //#ifndef GFC_NO_IME_SUPPORT
    UPInt                   CursorPos;
    GColor                  CursorColor;
    int                     SelectPos;
    CachedValue<GRectF>     CursorRect;
    Double                  CursorTimer;
    Double                  LastAdvanceTime;
    Float                   LastHorizCursorPos;
    GPointF                 LastMousePos;
    UInt32                  LastClickTime;
    UInt32                  ActiveSelectionBkColor;
    UInt32                  ActiveSelectionTextColor;
    UInt32                  InactiveSelectionBkColor;
    UInt32                  InactiveSelectionTextColor;
    UInt16                  Flags;

    void SetCursorBlink()     { Flags |= Flags_CursorBlink; }
    void ClearCursorBlink()   { Flags &= (~Flags_CursorBlink); }
    bool IsCursorBlink() const{ return (Flags & Flags_CursorBlink) != 0; }

    void SetCursorTimerBlocked()     { Flags |= Flags_CursorTimerBlocked; }
    void ClearCursorTimerBlocked()   { Flags &= (~Flags_CursorTimerBlocked); }
    bool IsCursorTimerBlocked() const{ return (Flags & Flags_CursorTimerBlocked) != 0; }

    void SetMouseCaptured()     { Flags |= Flags_MouseCaptured; }
    void ClearMouseCaptured()   { Flags &= (~Flags_MouseCaptured); }

    void SetShiftPressed()     { Flags |= Flags_ShiftPressed; }
    void ClearShiftPressed()   { Flags &= (~Flags_ShiftPressed); }
    bool IsShiftPressed() const{ return (Flags & Flags_ShiftPressed) != 0; }

    void SetLastDoubleClickFlag()     { Flags |= Flags_LastDoubleClick; }
    void ClearLastDoubleClickFlag()   { Flags &= (~Flags_LastDoubleClick); }
    bool IsLastDoubleClickFlagSet() const{ return (Flags & Flags_LastDoubleClick) != 0; }

    // "notifyMask" represents
    // the combination of GFxTextDocView::ViewNotificationMasks
    virtual void OnDocumentChanged(UInt notifyMask);

    const GFxTextLineBuffer::GlyphEntry* GetGlyphEntryAtIndex(UPInt charIndex, UPInt* ptextPos);

    // convert glyph pos (taken from LineBuffer to-from text pos (as in StyledText)
    UPInt TextPos2GlyphOffset(UPInt textPos);
    UPInt TextPos2GlyphPos(UPInt textPos);
    UPInt GlyphPos2TextPos(UPInt glyphPos);

    void  InvalidateSelectionColors();

    GFxTextEditorKit(GFxTextDocView* pdocview);
    ~GFxTextEditorKit();
public:
    void SetReadOnly()     { Flags |= Flags_ReadOnly; }
    void ClearReadOnly()   { Flags &= (~Flags_ReadOnly); }
    bool IsReadOnly() const{ return (Flags & Flags_ReadOnly) != 0; }

    void SetSelectable()     { Flags |= Flags_Selectable; }
    void ClearSelectable()   { Flags &= (~Flags_Selectable); }
    bool IsSelectable() const{ return (Flags & Flags_Selectable) != 0; }

    void SetUseRichClipboard()     { Flags |= Flags_UseRichClipboard; }
    void ClearUseRichClipboard()   { Flags &= (~Flags_UseRichClipboard); }
    bool DoesUseRichClipboard() const{ return (Flags & Flags_UseRichClipboard) != 0; }

    void SetOverwriteMode()     { Flags |= Flags_OverwriteMode; }
    void ClearOverwriteMode()   { Flags &= (~Flags_OverwriteMode); }
    bool IsOverwriteMode() const{ return (Flags & Flags_OverwriteMode) != 0; }

    void SetWideCursor()     { Flags |= Flags_WideCursor; }
    void ClearWideCursor();
    bool IsWideCursor() const{ return (Flags & Flags_WideCursor) != 0; }

    bool IsMouseCaptured() const{ return (Flags & Flags_MouseCaptured) != 0; }

    void Advance(Double timer);
    void PreDisplay(GFxDisplayContext &context, const Matrix& mat, const Cxform& cx);
    void Display(GFxDisplayContext &context, const Matrix& mat, const Cxform& cx);
    void PostDisplay(GFxDisplayContext &context, const Matrix& mat, const Cxform& cx) { GUNUSED3(context, mat, cx); }
    void ResetBlink(bool state = 1, bool blocked = 0);

    bool HasCursor() const { return !IsReadOnly(); }

    void SetCursorPos(UPInt pos, bool selectionAllowed);
    void SetCursorPos(UPInt pos) { SetCursorPos(pos, IsSelectable()); }
    UPInt GetCursorPos() const;
    // scrolls the text to the position. If wideCursor is true, it will scroll
    // to make whole wide cursor visible.
    bool ScrollToPosition(UPInt pos, bool avoidComposStr, bool wideCursor = false);
    bool ScrollToCursor()
    {
        return ScrollToPosition(CursorPos, true, IsWideCursor());
    }

    // event handlers
    void OnMouseDown(Float x, Float y, int buttons);
    void OnMouseUp(Float x, Float y, int buttons);
    void OnMouseMove(Float x, Float y);
    bool OnKeyDown(int code, const GFxSpecialKeysState& specKeysState);
    bool OnKeyUp(int code, const GFxSpecialKeysState& specKeysState);
    bool OnChar(UInt32 wcharCode);
    void OnSetFocus();
    void OnKillFocus();

    GFxTextDocView* GetDocument() const { return pDocView; }
    void SetClipboard(GFxTextClipboard* pclipbrd) { pClipboard = pclipbrd; }
    void SetKeyMap(GFxTextKeyMap* pkeymap)        { pKeyMap = pkeymap; }

    bool CalcCursorRectInLineBuffer
        (UPInt charIndex, GRectF* pcursorRect, UInt* plineIndex = NULL, UInt* pglyphIndex = NULL, bool avoidComposStr = true,
         GFxTextLineBuffer::Line::Alignment* plineAlignment = NULL);
    bool CalcAbsCursorRectInLineBuffer(UPInt charIndex, GRectF* pcursorRect, UInt* plineIndex = NULL, UInt* pglyphIndex = NULL)
    {
        return CalcCursorRectInLineBuffer(charIndex, pcursorRect, plineIndex, pglyphIndex, false);
    }
    bool CalcCursorRectOnScreen
        (UPInt charIndex, GRectF* pcursorRect, UInt* plineIndex = NULL, UInt* pglyphIndex = NULL, bool avoidComposStr = true,
         GFxTextLineBuffer::Line::Alignment* plineAlignment = NULL);
    bool CalcAbsCursorRectOnScreen(UPInt charIndex, GRectF* pcursorRect, UInt* plineIndex = NULL, UInt* pglyphIndex = NULL)
    {
        return CalcCursorRectOnScreen(charIndex, pcursorRect, plineIndex, pglyphIndex, false);
    }

    // Selection, public API
    void SetSelection(UPInt startPos, UPInt endPos);
    void ClearSelection()                                  { pDocView->ClearSelection(); }
    UPInt GetBeginSelection() const                        { return pDocView->GetBeginSelection(); }
    UPInt GetEndSelection()   const                        { return pDocView->GetEndSelection(); }
    void SetBeginSelection(UPInt begSel)                   { SetSelection(begSel, GetEndSelection()); }
    void SetEndSelection(UPInt endSel)                     { SetSelection(GetBeginSelection(), endSel); }
    void SetActiveSelectionTextColor(UInt32 color)         { ActiveSelectionTextColor = color; InvalidateSelectionColors(); }
    void SetActiveSelectionBackgroundColor(UInt32 color)   { ActiveSelectionBkColor   = color; InvalidateSelectionColors(); }
    void SetInactiveSelectionTextColor(UInt32 color)       { InactiveSelectionTextColor = color; InvalidateSelectionColors(); }
    void SetInactiveSelectionBackgroundColor(UInt32 color) { InactiveSelectionBkColor   = color; InvalidateSelectionColors(); }
    UInt32 GetActiveSelectionTextColor() const             { return ActiveSelectionTextColor; }
    UInt32 GetActiveSelectionBackgroundColor() const       { return ActiveSelectionBkColor; }
    UInt32 GetInactiveSelectionTextColor() const           { return InactiveSelectionTextColor; }
    UInt32 GetInactiveSelectionBackgroundColor() const     { return InactiveSelectionBkColor; }

    // clipboard operations
    void CopyToClipboard(UPInt startPos, UPInt endPos, bool useRichClipboard);
    void CutToClipboard(UPInt startPos, UPInt endPos, bool useRichClipboard);
    // if successful, returns new projected cursor position; GFC_MAX_UPINT otherwise.
    UPInt PasteFromClipboard(UPInt startPos, UPInt endPos, bool useRichClipboard);

#ifndef GFC_NO_IME_SUPPORT
    // Composition string
    GFxTextCompositionString* CreateCompositionString();
    void ReleaseCompositionString();
    bool HasCompositionString() const { return (pComposStr && pComposStr->GetLength() > 0); }
    GFxTextCompositionString* GetCompositionString() { return pComposStr; }
#else
    bool HasCompositionString() const { return false; }
    GFxTextCompositionString* GetCompositionString() { return NULL; }
#endif //#ifndef GFC_NO_IME_SUPPORT
};

#else

// A stub class used if GFC_NO_TEXT_INPUT_SUPPORT macro is defined.
class GFxTextEditorKit : public GRefCountBaseNTS<GFxTextEditorKit, GFxStatMV_Text_Mem>
{
public:
    typedef GRenderer::Matrix       Matrix;
    typedef GRenderer::Cxform       Cxform;

    UPInt TextPos2GlyphOffset(UPInt ) { return 0; }
    UPInt TextPos2GlyphPos(UPInt )    { return 0; }
    UPInt GlyphPos2TextPos(UPInt )    { return 0; }

    void SetReadOnly()     { }
    void ClearReadOnly()   { }
    bool IsReadOnly() const{ return false; }

    void SetSelectable()     {  }
    void ClearSelectable()   {  }
    bool IsSelectable() const{ return false; }

    void SetUseRichClipboard()     {  }
    void ClearUseRichClipboard()   {  }
    bool DoesUseRichClipboard() const{ return false; }

    void SetOverwriteMode()     {  }
    void ClearOverwriteMode()   {  }
    bool IsOverwriteMode() const{ return false; }

    void SetWideCursor()     {  }
    void ClearWideCursor()   {  }
    bool IsWideCursor() const{ return false; }

    bool IsMouseCaptured() const{ return false; }

    void Advance(Double) {}
    void PreDisplay(GFxDisplayContext &, const Matrix&, const Cxform&) {}
    void Display(GFxDisplayContext &, const Matrix&, const Cxform&) {}
    void PostDisplay(GFxDisplayContext &, const Matrix&, const Cxform&) { }
    void ResetBlink(bool = 1, bool = 0) {}

    bool HasCursor() const { return !IsReadOnly(); }

    void SetCursorPos(UPInt, bool) { }
    void SetCursorPos(UPInt)       { }
    UPInt GetCursorPos() const     { return 0; }
    // scrolls the text to the position. If wideCursor is true, it will scroll
    // to make whole wide cursor visible.
    bool ScrollToPosition(UPInt, bool , bool = false) { return false; }
    bool ScrollToCursor() { return false; }

    // event handlers
    void OnMouseDown(Float , Float , int ) {}
    void OnMouseUp(Float , Float , int ) {}
    void OnMouseMove(Float , Float ) {}
    bool OnKeyDown(int , const GFxSpecialKeysState&) { return false; }
    bool OnKeyUp(int, const GFxSpecialKeysState&) { return false; }
    bool OnChar(UInt32) { return false; }
    void OnSetFocus() {}
    void OnKillFocus() {}

    GFxTextDocView* GetDocument() const { return 0; }
    void SetClipboard(GFxTextClipboard*) { }
    void SetKeyMap(GFxTextKeyMap*)       { }

    bool CalcAbsCursorRectOnScreen(UPInt , GRectF* , UInt* = NULL, UInt* = NULL)
    {
        return false;
    }

    // Selection, public API
    void SetSelection(UPInt , UPInt )                      {}
    void ClearSelection()                                  {}
    UPInt GetBeginSelection() const                        { return false; }
    UPInt GetEndSelection()   const                        { return false; }
    void SetBeginSelection(UPInt )                         { }
    void SetEndSelection(UPInt )                           { }
    void SetActiveSelectionTextColor(UInt32 )              { }
    void SetActiveSelectionBackgroundColor(UInt32 )        { }
    void SetInactiveSelectionTextColor(UInt32 )            { }
    void SetInactiveSelectionBackgroundColor(UInt32 )      { }
    UInt32 GetActiveSelectionTextColor() const             { return 0; }
    UInt32 GetActiveSelectionBackgroundColor() const       { return 0; }
    UInt32 GetInactiveSelectionTextColor() const           { return 0; }
    UInt32 GetInactiveSelectionBackgroundColor() const     { return 0; }

    // clipboard operations
    void CopyToClipboard(UPInt , UPInt , bool ) {}
    void CutToClipboard(UPInt , UPInt , bool )  {}
    // if successful, returns new projected cursor position; GFC_MAX_UPINT otherwise.
    UPInt PasteFromClipboard(UPInt , UPInt , bool ) { return 0; }

    bool HasCompositionString() const { return false; }
    GFxTextCompositionString* GetCompositionString() { return NULL; }
};
#endif //GFC_NO_TEXT_INPUT_SUPPORT

#endif //INC_TEXT_GFXEDITORKIT_H
