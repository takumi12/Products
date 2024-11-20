/**********************************************************************

Filename    :   GFxStaticText.h
Content     :   Static text field character implementation
Created     :   May, 2007
Authors     :   Artem Bolgar, Prasad Silva

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxStaticText_H
#define INC_GFxStaticText_H

#include "GConfig.h"

#include "GFxCharacter.h"
#include "GFxText.h"


//////////////////////////////////////////////////////////////////////////
// GFxStaticTextCharacter
//
class GFxStaticTextCharacter : public GFxGenericCharacter
{
public:
    enum
    {
        Flags_FontsResolved = 1,
        Flags_NextFrame     = 2
    };

    GFxTextLineBuffer           TextGlyphRecords;
    GFxTextFilter               Filter;

    class HighlightDesc : public GNewOverrideBase<GFxStatMV_Text_Mem>
    {
        GFxTextHighlighter          HighlightManager;
        GFxDrawingContext           BackgroundDrawing;

    public:
        GINLINE void    DrawBackground(GFxTextLineBuffer& lineBuf, const GRectF& textRect)
        {
            HighlightManager.DrawBackground(BackgroundDrawing, lineBuf, textRect, 0, 0, 0);
            HighlightManager.Validate();
        }
        GINLINE void    Display(GFxDisplayContext &context, const Matrix& mat, const GRenderer::Cxform& cx)  
        {
            if (!BackgroundDrawing.IsEmpty())
                BackgroundDrawing.Display(context, mat, cx, GRenderer::Blend_None, false);
        }
        GINLINE GFxTextHighlighter& GetManager()    { return HighlightManager; }
        GINLINE bool    IsEmpty() const             { return BackgroundDrawing.IsEmpty(); }
        void Invalidate()                           { HighlightManager.Invalidate(); }
        bool IsValid() const                        { return HighlightManager.IsValid(); }
        GINLINE void SetSelectColor(const GColor& color)    { HighlightManager.SetSelectColor(color); }

    } *pHighlight;

    UInt8                       Flags;

    GFxStaticTextCharacter(GFxStaticTextCharacterDef* pdef, GFxMovieDefImpl *pbindingDefImpl, 
        GFxASCharacter* parent, GFxResourceId id);

    virtual ~GFxStaticTextCharacter();

    virtual void    AdvanceFrame(bool nextFrame, Float framePos)
    {
        GUNUSED(framePos);
        if (nextFrame)
            Flags |= Flags_NextFrame;
        else
            Flags &= ~Flags_NextFrame;
    }

    virtual void    SetFilters(const GArray<GFxFilterDesc> f)
    {
        GFxTextFilter tf;
        for (UInt i = 0; i < f.GetSize(); i++)
            tf.LoadFilterDesc(f[i]);
        SetFilters(tf);
    }

    virtual void    SetFilters(const GFxTextFilter& f)
    {
        Filter = f;
    }

    GINLINE bool            HasHighlights()     { return pHighlight != NULL; }

    HighlightDesc*          CreateTextHighlighter();
    GINLINE HighlightDesc*  GetTextHighlighter()        { return pHighlight; }

    void    Display(GFxDisplayContext &context);
};


#ifndef GFC_NO_FXPLAYER_AS_TEXTSNAPSHOT

//
// Data storage for static text character snapshot
//
// See GASTextSnapshot.h for usage
//
class GFxStaticTextSnapshotData
{
public:
    class GlyphVisitor
    {
        friend class GFxStaticTextSnapshotData;

    public:
        virtual ~GlyphVisitor() {}
        virtual void OnVisit() = 0;

        GINLINE const char*         GetFontName()   { return pFont->GetName(); }
        GINLINE UPInt               GetRunIndex()   { return RunIdx; }
        GINLINE const GColor&       GetColor()      { return Color; }
        GINLINE Float               GetHeight()     { return Height; }
        GINLINE const GMatrix2D&    GetMatrix()     { return Matrix; }
        GINLINE const GRectF&       GetCorners()    { return Corners; }
        GINLINE bool                IsSelected()    { return bSelected; }

    private:      
        GMatrix2D                       Matrix;
        GRectF                          Corners;
        GFxTextLineBuffer::GlyphEntry*  pGlyph;
        GFxFont*                        pFont;
        UPInt                           RunIdx;
        Float                           Height;
        GColor                          Color;
        bool                            bSelected;
    };


    GFxStaticTextSnapshotData();

    void            Add(GFxStaticTextCharacter* pstChar);

    UPInt           GetCharCount() const;

    GString         GetSubString(UPInt start, UPInt end, bool binclNewLines) const;
    GString         GetSelectedText(bool binclNewLines) const;
    int             FindText(UPInt start, const char* query, bool bcaseSensitive) const;
    bool            IsSelected(UPInt start, UPInt end) const;
    void            SetSelected(UPInt start, UPInt end, bool bselect) const;
    void            SetSelectColor(const GColor& color);
    SInt            HitTestTextNearPos(Float x, Float y, Float closedist) const;

    void            Visit(GlyphVisitor* pvisitor, UPInt start, UPInt end) const;
        
private:

    struct CharRef
    {
        GPtr<GFxStaticTextCharacter>    pChar;
        UPInt                           CharCount;
    };

    GArrayLH<CharRef>       StaticTextCharRefs;
    GStringLH               SnapshotString;
    GColor                  SelectColor;
};


#endif  // GFC_NO_FXPLAYER_AS_TEXTSNAPSHOT

#endif  // INC_GFxStaticText_H
