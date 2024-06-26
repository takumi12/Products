/**********************************************************************

Filename    :   GFxTextHighlight.h
Content     :   Highlighting functionality for rich text.
Created     :   August 6, 2007
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_TEXT_GFXTEXTHIGHLIGHT_H
#define INC_TEXT_GFXTEXTHIGHLIGHT_H

#include "GArray.h"
#include "GRange.h"
#include "GColor.h"
#include "GTypes2DF.h"

#include "GFxPlayerStats.h"

class GString;
class GFxTextHighlighter;
class GFxTextLineBuffer;
class GFxTextCompositionString;
class GFxDrawingContext;
class GFxTextLineBuffer;

#define GFX_TOPMOST_HIGHLIGHTING_INDEX      INT_MAX
#define GFX_WIDECURSOR_HIGHLIGHTING_INDEX   (GFX_TOPMOST_HIGHLIGHTING_INDEX - 1)
#define GFX_COMPOSSTR_HIGHLIGHTING_INDEX    (GFX_TOPMOST_HIGHLIGHTING_INDEX - 7)

// A struct that holds rendering parameters of the selection.
struct GFxTextHighlightInfo : public GNewOverrideBase<GFxStatMV_Text_Mem>
{
    enum UnderlineStyle
    {
        Underline_None   = 0,
        Underline_Single = 1,
        Underline_Thick  = 2,
        Underline_Dotted = 3,
        Underline_DitheredSingle = 4,
        Underline_DitheredThick  = 5
    };
    enum
    {
        Flag_UnderlineStyle     = 0x7, // mask
        Flag_Background         = 0x8,
        Flag_TextColor          = 0x10,
        Flag_UnderlineColor     = 0x20
    };
    GColor      BackgroundColor;
    GColor      TextColor;
    GColor      UnderlineColor;
    UByte       Flags;

    GFxTextHighlightInfo()                  { Reset(); }
    GFxTextHighlightInfo(UnderlineStyle st) { Reset(); SetUnderlineStyle(st); }

    void Reset() { BackgroundColor = TextColor = UnderlineColor = 0; Flags = 0; }
    void Append(const GFxTextHighlightInfo& mergee);
    void Prepend(const GFxTextHighlightInfo& mergee);
    bool IsEqualWithFlags(const GFxTextHighlightInfo& right, UInt flags);

    void SetSingleUnderline()     { SetUnderlineStyle(Underline_Single); }
    bool IsSingleUnderline() const{ return GetUnderlineStyle() == Underline_Single; }

    void SetThickUnderline()     { SetUnderlineStyle(Underline_Thick); }
    bool IsThickUnderline() const{ return GetUnderlineStyle() == Underline_Thick; }

    void SetDottedUnderline()     { SetUnderlineStyle(Underline_Dotted); }
    bool IsDottedUnderline() const{ return GetUnderlineStyle() == Underline_Dotted; }

    void SetDitheredSingleUnderline()     { SetUnderlineStyle(Underline_DitheredSingle); }
    bool IsDitheredSingleUnderline() const{ return GetUnderlineStyle() == Underline_DitheredSingle; }

    void SetDitheredThickUnderline()     { SetUnderlineStyle(Underline_DitheredThick); }
    bool IsDitheredThickUnderline() const{ return GetUnderlineStyle() == Underline_DitheredThick; }

    UnderlineStyle GetUnderlineStyle() const { return UnderlineStyle(Flags & Flag_UnderlineStyle); }
    bool  HasUnderlineStyle() const { return GetUnderlineStyle() != 0; }
    void  ClearUnderlineStyle() { Flags &= ~Flag_UnderlineStyle; }
    void  SetUnderlineStyle(UnderlineStyle us) { ClearUnderlineStyle(); Flags |= (us & Flag_UnderlineStyle); }

    void SetBackgroundColor(const GColor& backgr) { Flags |= Flag_Background; BackgroundColor = backgr; }
    GColor GetBackgroundColor() const             { return (HasBackgroundColor()) ? BackgroundColor : GColor(0, 0, 0, 0); }
    void ClearBackgroundColor()                   { Flags &= (~Flag_Background); }
    bool HasBackgroundColor() const               { return (Flags & Flag_Background) != 0; }

    void SetTextColor(const GColor& color) { Flags |= Flag_TextColor; TextColor = color; }
    GColor GetTextColor() const             { return (HasTextColor()) ? TextColor : GColor(0, 0, 0, 0); }
    void ClearTextColor()                   { Flags &= (~Flag_TextColor); }
    bool HasTextColor() const               { return (Flags & Flag_TextColor) != 0; }

    void SetUnderlineColor(const GColor& color) { Flags |= Flag_UnderlineColor; UnderlineColor = color; }
    GColor GetUnderlineColor() const            { return (HasUnderlineColor()) ? UnderlineColor : GColor(0, 0, 0, 0); }
    void ClearUnderlineColor()                  { Flags &= (~Flag_UnderlineColor); }
    bool HasUnderlineColor() const              { return (Flags & Flag_UnderlineColor) != 0; }

};

// A struct that holds positioning parameters of the selection.
struct GFxTextHighlightDesc
{
    UPInt                   StartPos; // text position
    UPInt                   Length;   // length in chars
    SPInt                   Offset;   // offset from StartPos, in number of glyphs

    UPInt                   AdjStartPos; // adjusted on composition str starting text pos
    UPInt                   GlyphNum;   // pre-calculated number of glyphs

    UInt                    Id;       // id of desc
    GFxTextHighlightInfo    Info;

    GFxTextHighlightDesc():
        StartPos(GFC_MAX_UPINT),Length(0),Offset(-1),AdjStartPos(0),GlyphNum(0),Id(0) {}
    GFxTextHighlightDesc(SPInt startPos, UPInt len):
        StartPos(startPos), Length(len), Offset(-1), AdjStartPos(0),GlyphNum(0), Id(0) {}

    bool ContainsPos(UPInt pos) const 
    { 
        return (Length > 0) ? (pos >= StartPos && pos < StartPos + Length) : false; 
    }
    bool ContainsIndex(UPInt index) const 
    { 
        return (GlyphNum > 0) ? (index >= AdjStartPos && index < AdjStartPos + GlyphNum) : false; 
    }
    bool IsEmpty() const { return Length == 0; }
};

// An iterator class that iterates highlights position by position.
class GFxTextHighlighterPosIterator
{
    const GFxTextHighlighter*   pManager;
    UPInt                       CurAdjStartPos;
    UPInt                       NumGlyphs;
    GFxTextHighlightDesc        CurDesc;

    void InitCurDesc();
public:
    GFxTextHighlighterPosIterator() { CurAdjStartPos = NumGlyphs = 0; }
    GFxTextHighlighterPosIterator(const GFxTextHighlighter* pmanager, UPInt startPos, UPInt len = GFC_MAX_UPINT);

    const GFxTextHighlightDesc& operator*() const { return CurDesc; }
    void operator++(int);
    void operator++() { operator++(0); }
    void operator+=(UPInt p);

    bool IsFinished() const;
};

// An iterator class that iterates highlights by ranges.
class GFxTextHighlighterRangeIterator
{
    const GFxTextHighlighter*   pManager;
    UPInt                       CurTextPos;
    UPInt                       CurRangeIndex;
    GFxTextHighlightDesc        CurDesc;
    UInt                        Flags;

    void InitCurDesc();
public:
    // flags - Flag_<> from GFxTextHighlighInfo
    GFxTextHighlighterRangeIterator(const GFxTextHighlighter* pmanager, UInt flags);
    GFxTextHighlighterRangeIterator(const GFxTextHighlighter* pmanager, UPInt startPos, UInt flags);

    GFxTextHighlightDesc operator*();
    void operator++(int);
    void operator++() { operator++(0); }

    bool IsFinished() const;
};

// This class provides functionality to manage, calculate and render the
// highlighting.
class GFxTextHighlighter : public GNewOverrideBase<GFxStatMV_Text_Mem>
{
    friend class GFxTextHighlighterRangeIterator;
    friend class GFxTextHighlighterPosIterator;
protected:
    GArrayLH<GFxTextHighlightDesc>  Highlighters;
    UInt                            LastId;
    UPInt                           CorrectionPos;
    UPInt                           CorrectionLen;
    bool                            Valid;
    mutable SInt8                   HasUnderline; // cached flag, 0 - not set, 1 - true, false otherwise

public:
    GFxTextHighlighter();

    bool IsEmpty() const { return Highlighters.GetSize() == 0; }

    // highlighter will be created with id first available
    GFxTextHighlightDesc* CreateNewHighlighter(GFxTextHighlightDesc* pdesc);
    
    // highlighter will be created with id specified in desc
    GFxTextHighlightDesc* CreateHighlighter(const GFxTextHighlightDesc& desc);
    
    // an empty highlighter will be created with id first available
    GFxTextHighlightDesc* CreateEmptyHighlighter(UInt *pid)
    {
        GFxTextHighlightDesc empty;
        GFxTextHighlightDesc* pdesc = CreateNewHighlighter(&empty);
        *pid = empty.Id;
        return pdesc;
    };

    GFxTextHighlightDesc GetHighlighter(UInt id) const;
    GFxTextHighlightDesc* GetHighlighterPtr(UInt id);
    bool SetHighlighter(UInt id, const GFxTextHighlightInfo& info);
    bool FreeHighlighter(UInt id);

    GFxTextHighlighterPosIterator   GetPosIterator(UPInt startPos = 0, UPInt len = GFC_MAX_UPINT) const;

    // flags - Flag_<> from GFxTextHighlighInfo
    GFxTextHighlighterRangeIterator GetRangeIterator(UPInt startPos = 0, UInt flags = GFC_MAX_UINT) const;

    void UpdateGlyphIndices(const GFxTextCompositionString* pcs);

    // valid/invalid flag control
    void Invalidate() { Valid = false; HasUnderline = 0; }
    void Validate()   { Valid = true; }
    bool IsValid() const { return Valid; }

    bool HasUnderlineHighlight() const;


    void DrawBackground(GFxDrawingContext& backgroundDrawing,
                        GFxTextLineBuffer& lineBuffer,
                        const GRectF& textRect,
                        UPInt firstVisibleChar,
                        Float hScrollOffset,
                        Float vScrollOffset);

    void Add(GFxTextHighlightDesc& merge);
    void Remove(const GFxTextHighlightDesc& cut);

    void SetSelectColor(const GColor& color);
    bool IsAnyCharSelected(UPInt selectStart, UPInt selectEnd);
};

#endif // INC_TEXT_GFXTEXTHIGHLIGHT_H
