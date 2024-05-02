/**********************************************************************

Filename    :   GFxTextHighlight.cpp
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

#include "Text/GFxTextHighlight.h"
#include "Text/GFxTextEditorKit.h"
#include "GAlgorithm.h"

namespace
{
    struct IdComparator
    {
        static int Compare(const GFxTextHighlightDesc& p1, UInt id)
        {
            return (int)p1.Id - (int)id;
        }
        static int Less(const GFxTextHighlightDesc& p1, UInt id)
        {
            return Compare(p1, id) < 0;
        }
    };
}

//////////////////////////////////////////////////////////////////////////
// GFxTextHighlightDesc
// 
void GFxTextHighlightInfo::Append(const GFxTextHighlightInfo& mergee)
{
    if (!HasUnderlineStyle() && mergee.HasUnderlineStyle())
    {
        SetUnderlineStyle(mergee.GetUnderlineStyle());
    }
    if (!HasBackgroundColor() && mergee.HasBackgroundColor())
        SetBackgroundColor(mergee.GetBackgroundColor());
    if (!HasTextColor() && mergee.HasTextColor())
        SetTextColor(mergee.GetTextColor());
    if (!HasUnderlineColor() && mergee.HasUnderlineColor())
        SetUnderlineColor(mergee.GetUnderlineColor());
}

void GFxTextHighlightInfo::Prepend(const GFxTextHighlightInfo& mergee)
{
    if (mergee.HasUnderlineStyle())
    {
        SetUnderlineStyle(mergee.GetUnderlineStyle());
    }
    if (mergee.HasBackgroundColor())
        SetBackgroundColor(mergee.GetBackgroundColor());
    if (mergee.HasTextColor())
        SetTextColor(mergee.GetTextColor());
    if (mergee.HasUnderlineColor())
        SetUnderlineColor(mergee.GetUnderlineColor());
}

bool GFxTextHighlightInfo::IsEqualWithFlags(const GFxTextHighlightInfo& right, UInt flags)
{
    if (flags & Flag_UnderlineStyle)
    {
        if (GetUnderlineStyle() != right.GetUnderlineStyle())
            return false;
    }
    if (flags & Flag_Background)
    {
        if (HasBackgroundColor() != right.HasBackgroundColor() || 
            GetBackgroundColor() != right.GetBackgroundColor())
            return false;
    }
    if (flags & Flag_TextColor)
    {
        if (HasTextColor() != right.HasTextColor() || 
            GetTextColor() != right.GetTextColor())
            return false;
    }
    if (flags & Flag_UnderlineColor)
    {
        if (HasUnderlineColor() != right.HasUnderlineColor() || 
            GetUnderlineColor() != right.GetUnderlineColor())
            return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
// GFxTextHighlighter
// 
GFxTextHighlighter::GFxTextHighlighter() :
    LastId(0),CorrectionPos(0),CorrectionLen(0), Valid(false), HasUnderline(0) 
{
}

GFxTextHighlightDesc* GFxTextHighlighter::CreateNewHighlighter(GFxTextHighlightDesc* pdesc)
{
    Invalidate();
    do 
    {
        ++LastId;
        GFxTextHighlightDesc* pfdesc = GetHighlighterPtr(LastId);
        if (!pfdesc)
        {
            GASSERT(pdesc);
            pdesc->Id = LastId;
            UPInt i = G_LowerBound(Highlighters, pdesc->Id, IdComparator::Less);
            Highlighters.InsertAt(i, *pdesc);
            return &Highlighters[i];       
        }
    } while(1);
    return NULL;
}

GFxTextHighlightDesc* GFxTextHighlighter::CreateHighlighter(const GFxTextHighlightDesc& desc)
{
    GASSERT(desc.Id > 0); // Id should be initialized to value other than zero

    Invalidate();
    // check, is id already occupied
    GFxTextHighlightDesc* pdesc = GetHighlighterPtr(desc.Id);
    if (!pdesc)
    {
        UPInt i = G_LowerBound(Highlighters, desc.Id, IdComparator::Less);
        Highlighters.InsertAt(i, desc);
        return &Highlighters[i];       
    }
    return NULL;
}

GFxTextHighlightDesc* GFxTextHighlighter::GetHighlighterPtr(UInt id)
{
    UPInt i = G_LowerBound(Highlighters, id, IdComparator::Less);
    if (i < Highlighters.GetSize() && Highlighters[i].Id == id)
        return &Highlighters[i];
    return NULL;
}


GFxTextHighlightDesc GFxTextHighlighter::GetHighlighter(UInt id) const
{
    UPInt i = G_LowerBound(Highlighters, id, IdComparator::Less);
    if (i < Highlighters.GetSize() && Highlighters[i].Id == id)
        return Highlighters[i];
    return GFxTextHighlightDesc();
}

bool GFxTextHighlighter::SetHighlighter(UInt id, const GFxTextHighlightInfo& info)
{
    GFxTextHighlightDesc* pdesc = GetHighlighterPtr(id);
    if (pdesc)
    {
        pdesc->Info = info;
        Invalidate();
        return true;
    }
    return false;
}

bool GFxTextHighlighter::FreeHighlighter(UInt id)
{
    UPInt i = G_LowerBound(Highlighters, id, IdComparator::Less);
    if (i < Highlighters.GetSize() && Highlighters[i].Id == id)
    {
        Highlighters.RemoveAt(i);
        Invalidate();
        return true;
    }
    return false;
}

GFxTextHighlighterPosIterator GFxTextHighlighter::GetPosIterator(UPInt startPos, UPInt len) const
{
    return GFxTextHighlighterPosIterator(this, startPos, len);
}

GFxTextHighlighterRangeIterator GFxTextHighlighter::GetRangeIterator(UPInt startPos, UInt flags) const
{
    return GFxTextHighlighterRangeIterator(this, startPos, flags);
}

void GFxTextHighlighter::UpdateGlyphIndices(const GFxTextCompositionString* pcs)
{
    CorrectionPos = CorrectionLen = 0;
#ifndef GFC_NO_IME_SUPPORT
    if (pcs)
    {
        CorrectionPos = pcs->GetPosition();
        CorrectionLen = pcs->GetLength();
    }
#else
    GUNUSED(pcs);
#endif
    Invalidate();
    for(UPInt i = 0, n = Highlighters.GetSize(); i < n; ++i)
    {
        GFxTextHighlightDesc& desc = Highlighters[i];

        desc.AdjStartPos = desc.StartPos;
        desc.GlyphNum   = desc.Length;
        if (CorrectionLen > 0)
        {
            if (desc.ContainsPos(CorrectionPos))
            {
                if (desc.Offset >= 0)
                    desc.AdjStartPos += desc.Offset;
                else
                    desc.GlyphNum += CorrectionLen;
            }
            else if (desc.StartPos > CorrectionPos)
            {
                desc.AdjStartPos += CorrectionLen;
            }
        }
    }
}

bool GFxTextHighlighter::HasUnderlineHighlight() const
{
    if (HasUnderline == 0)
    {
        HasUnderline = -1;
        for(UPInt i = 0, n = Highlighters.GetSize(); i < n; ++i)
        {
            const GFxTextHighlightDesc& desc = Highlighters[i];
            if (desc.Info.HasUnderlineStyle())
            {
                HasUnderline = 1;
                break;
            }
        }
    }
    return HasUnderline == 1;
}

void GFxTextHighlighter::DrawBackground(GFxDrawingContext& backgroundDrawing,
                                        GFxTextLineBuffer& lineBuffer,
                                        const GRectF& textRect,
                                        UPInt firstVisibleChar,
                                        Float hScrollOffset,
                                        Float vScrollOffset)
{
    backgroundDrawing.Clear();
    UInt32 oldColor = 0;
    backgroundDrawing.Clear();
    backgroundDrawing.SetNonZeroFill(true);
    GFxTextHighlighterRangeIterator it = GetRangeIterator(firstVisibleChar);
    for (; !it.IsFinished(); ++it)
    {
        GFxTextHighlightDesc hdesc = *it;

        GFxTextLineBuffer::Iterator lit = lineBuffer.FindLineByTextPos(hdesc.AdjStartPos);
        bool rv = false;

        GRectF lineRect, prevLineRect;
        lineRect.Clear();
        prevLineRect.Clear();

        UPInt lenRemaining = hdesc.GlyphNum;
        for (;!lit.IsFinished() && lenRemaining > 0; ++lit)
        {
            GFxTextLineBuffer::Line& line = *lit;
            if (!lineBuffer.IsLineVisible(lit.GetIndex()))
                continue;
            UPInt indexInLine;
            if (line.GetTextPos() < hdesc.AdjStartPos)
            {
                // need to find an actual index of glyph record in the line.
                // Note, GlyphIndex already has some corrections (on IME composition
                // string for example). But we still need to correct it by
                // image substitution.
                GFxTextLineBuffer::GlyphIterator git = line.Begin();
                indexInLine = 0;
                SPInt len = (SPInt)(hdesc.AdjStartPos - line.GetTextPos());
                for(; len > 0 && !git.IsFinished(); ++git)
                {
                    const GFxTextLineBuffer::GlyphEntry& glyph = git.GetGlyph();
                    if (glyph.GetLength() == 0)
                        continue; // do not increase the index, if glyph is lengthless
                    len -= (int)glyph.GetLength();
                    ++indexInLine;
                }
            }
            else
            {
                indexInLine = 0;
                // correct the remaining len, if there is a gap between lines in characters positions
                lenRemaining -= line.GetTextPos() - (hdesc.AdjStartPos + (hdesc.GlyphNum - lenRemaining));
            }
            GFxTextLineBuffer::GlyphIterator git = line.Begin();
            SInt advance = 0;
            lineRect.Clear();
            for(UInt i = 0; !git.IsFinished() && lenRemaining > 0; ++git)
            {
                const GFxTextLineBuffer::GlyphEntry& glyph = git.GetGlyph();
                GRectF glyphRect;
                glyphRect.Clear();
                if (i >= indexInLine)
                {
                    rv = true;

                    glyphRect.Right = Float(glyph.GetAdvance());

                    glyphRect.Top      = 0;
                    glyphRect.Bottom   = Float(line.GetHeight() + line.GetNonNegLeading());

                    glyphRect += GPointF(Float(advance), 0);
                    lenRemaining -= glyph.GetLength();

                    if (i == indexInLine)
                        lineRect = glyphRect;
                    else
                        lineRect.Union(glyphRect);

                }
                advance += glyph.GetAdvance();
                if (glyph.GetLength() > 0)
                    ++i; // do not increase the index, if glyph is lengthless
            }
            lineRect.Offset(-hScrollOffset, -vScrollOffset);
            lineRect.Offset(Float(line.GetOffsetX()), Float(line.GetOffsetY()));
            lineRect.Offset(textRect.TopLeft());
            if (!prevLineRect.IsEmpty())
            {
                // align the bottom of previous rect to be the same as top of the current one
                // to avoid holes between selections (only in the case if prev bottom is close to top 
                // and if previous line is above the current one).
                if (lineRect.Top > prevLineRect.Bottom && prevLineRect.Bottom + 20.0f >= lineRect.Top && 
                    (lineRect.Right >= prevLineRect.Left && prevLineRect.Right >= lineRect.Left))
                {
                    prevLineRect.Bottom = lineRect.Top;
                }
                prevLineRect.Intersect(textRect);
                if (!prevLineRect.IsEmpty())
                {
                    UInt32 c = hdesc.Info.GetBackgroundColor().ToColor32();
                    if (c != oldColor)
                        backgroundDrawing.SetFill(oldColor = c);
                    backgroundDrawing.MoveTo(prevLineRect.Left,  prevLineRect.Top);
                    backgroundDrawing.LineTo(prevLineRect.Right, prevLineRect.Top);
                    backgroundDrawing.LineTo(prevLineRect.Right, prevLineRect.Bottom);
                    backgroundDrawing.LineTo(prevLineRect.Left, prevLineRect.Bottom);
                    backgroundDrawing.LineTo(prevLineRect.Left, prevLineRect.Top);
                    backgroundDrawing.AddPath();
                }
            }
            prevLineRect = lineRect;
        }
        if (!lineRect.IsEmpty())
        {
            lineRect.Intersect(textRect);
            if (!lineRect.IsEmpty())
            {
                UInt32 c = hdesc.Info.GetBackgroundColor().ToColor32();
                if (c != oldColor)
                    backgroundDrawing.SetFill(oldColor = c);
                backgroundDrawing.MoveTo(lineRect.Left, lineRect.Top);
                backgroundDrawing.LineTo(lineRect.Right, lineRect.Top);
                backgroundDrawing.LineTo(lineRect.Right, lineRect.Bottom);
                backgroundDrawing.LineTo(lineRect.Left, lineRect.Bottom);
                backgroundDrawing.LineTo(lineRect.Left, lineRect.Top);
                backgroundDrawing.AddPath();
            }
        }
    }
}


//
// Merge with the existing descs
// If no merge desc is found, it is added to the set
//
void GFxTextHighlighter::Add(GFxTextHighlightDesc& merge)
{
    //GFC_DEBUG_MESSAGE2(1, "GFxTextHighlighter::Add - %d, %d", merge.StartPos, merge.Length);
    bool bmerged = false;
    const UPInt& mergeStart = merge.StartPos;
    UPInt mergeEnd    = merge.StartPos + merge.Length;
    for (UPInt i = 0, n = Highlighters.GetSize(); i < n; ++i)
    {        
        GFxTextHighlightDesc& desc = Highlighters[i];
        //GFC_DEBUG_MESSAGE2(1, "\tchecking.. %d, %d", desc.StartPos, desc.Length);
        // Can desc be merged
        const UPInt& descStart    = desc.StartPos;
        UPInt descEnd       = desc.StartPos + desc.Length;
        if (mergeStart >= descStart && mergeStart <= descEnd)
        {
            // Special case: merge in middle? Done
            if (mergeEnd <= descEnd)
                continue;
            else
            {
                // Append on right side
                GASSERT(mergeEnd > descEnd);
                UPInt slice = (mergeEnd - descEnd);
                desc.Length += slice;
                desc.GlyphNum = desc.Length;
                bmerged = true;
                Invalidate();
            }
        }
        else if (descStart > mergeStart && descStart < mergeEnd)
        {
            // Append on left side
            GASSERT(descStart > mergeStart);
            UPInt slice = (descStart - mergeStart);
            desc.StartPos -= slice;
            desc.AdjStartPos = desc.StartPos;
            desc.Length += slice;
            desc.GlyphNum = desc.Length;
            bmerged = true;
            Invalidate();
            
            // Special case: expand on right side too
            if (descEnd <= mergeEnd)
            {
                slice = (mergeEnd - descEnd);
                GASSERT(mergeEnd >= descEnd);
                desc.Length += slice;
                desc.GlyphNum = desc.Length;
            }
        }
    }

    if (!bmerged)
    {
        GFxTextHighlightDesc* pdesc = CreateNewHighlighter(&merge);
        GASSERT(pdesc);
        GUNUSED(pdesc);
    }

    /*
    GFC_DEBUG_MESSAGE(1, ">> Highlighter states");
    for (UPInt i = 0, n = Highlighters.GetSize(); i < n; ++i)
    {
        GFxTextHighlightDesc& desc = Highlighters[i];
        GFC_DEBUG_MESSAGE2(1, "\t%d, %d", desc.StartPos, desc.Length);
    }
    */
}


//
// Cut existing descs with new one
// No effect if no existing descs are affected
//
void GFxTextHighlighter::Remove(const GFxTextHighlightDesc& cut)
{
    //GFC_DEBUG_MESSAGE2(1, "GFxTextHighlighter::Remove - %d, %d", cut.StartPos, cut.Length);
    GArray<GFxTextHighlightDesc>   addDescs;

    const UPInt& cutStart = cut.StartPos;
    UPInt cutEnd    = cut.StartPos + cut.Length;
    for (UPInt i = 0; i < Highlighters.GetSize(); ++i)
    {
        GFxTextHighlightDesc& desc = Highlighters[i];
        //GFC_DEBUG_MESSAGE2(1, "\tchecking.. %d, %d", desc.StartPos, desc.Length);
        // Is desc affected?
        UPInt descStart    = desc.StartPos;
        UPInt descEnd      = desc.StartPos + desc.Length;
        if (cutStart > descStart && cutStart < descEnd)
        {
            // Special case: cut in middle? This causes a split
            if (cutEnd < descEnd)
            {
                // Fix left side
                GASSERT(cutStart > descStart);
                UPInt slice = (cutStart - descStart);
                desc.Length = slice;
                desc.GlyphNum = desc.Length;
                // Spawn new right side
                GASSERT(descEnd > cutEnd);
                slice = (descEnd - cutEnd);
                GFxTextHighlightDesc newdesc = desc;                                
                newdesc.StartPos = cutEnd;
                newdesc.AdjStartPos = newdesc.StartPos;
                newdesc.Length = slice;
                newdesc.GlyphNum = newdesc.Length;
                addDescs.PushBack(newdesc);
                Invalidate();
            }
            else
            {
                // Cut right side
                GASSERT(descEnd >= cutStart);
                UPInt slice = (descEnd - cutStart);
                desc.Length -= slice;
                desc.GlyphNum = desc.Length;
                Invalidate();
            }
        }
        else if (descStart >= cutStart && descStart < cutEnd)
        {
            // Special case: cut whole desc? Drop it
            if (descEnd <= cutEnd)
            {
                Highlighters.RemoveAt(i--);
                Invalidate();
            }
            else
            {
                // Cut left side
                GASSERT(cutEnd >= descStart);
                UPInt slice = (cutEnd - descStart);
                desc.StartPos += slice;
                desc.AdjStartPos = desc.StartPos;
                desc.Length -= slice;
                desc.GlyphNum = desc.Length;
                Invalidate();
            }
        }  
    }

    for (UPInt i=0; i < addDescs.GetSize(); ++i)
    {
        CreateNewHighlighter(&addDescs[i]);
    }

    /*
    GFC_DEBUG_MESSAGE(1, ">> Highlighter states");
    for (UPInt i = 0, n = Highlighters.GetSize(); i < n; ++i)
    {
        GFxTextHighlightDesc& desc = Highlighters[i];
        GFC_DEBUG_MESSAGE2(1, "\t%d, %d", desc.StartPos, desc.Length);
    }
    */
}


void GFxTextHighlighter::SetSelectColor(const GColor& color)
{
    for (UPInt i = 0, n = Highlighters.GetSize(); i < n; ++i)
    {
        GFxTextHighlightDesc& desc = Highlighters[i];
        desc.Info.BackgroundColor = color;
    }
    Invalidate();
}


bool    GFxTextHighlighter::IsAnyCharSelected(UPInt selectStart, UPInt selectEnd)
{
    for (UPInt i = 0, n = Highlighters.GetSize(); i < n; ++i)
    {
        GFxTextHighlightDesc& desc = Highlighters[i];
        UPInt& descStart = desc.StartPos;
        UPInt descEnd = descStart + desc.Length;
        if ( (selectStart >= descStart && selectStart < descEnd) ||
             (descStart >= selectStart && descStart < selectEnd) )
        return true;
    }
    return false;
}


//////////////////////////////////////////////////////////////////////////
// GFxTextHighlighterRangeIterator
// 
GFxTextHighlighterRangeIterator::GFxTextHighlighterRangeIterator
(const GFxTextHighlighter* pmanager, UPInt startPos, UInt flags) : 
    pManager(pmanager), CurTextPos(startPos), CurRangeIndex(0), Flags(flags)
{
    InitCurDesc();
}

void GFxTextHighlighterRangeIterator::InitCurDesc()
{
    UPInt nearestNextPos;
    GFxTextHighlightDesc desc;
    do 
    {
        nearestNextPos = GFC_MAX_UPINT;
        for (UPInt i = 0, n = pManager->Highlighters.GetSize(); i < n; ++i)
        {
            const GFxTextHighlightDesc& curd = pManager->Highlighters[i];

            if ((curd.Info.Flags & Flags) != 0)
            {
                if (curd.ContainsIndex(CurTextPos))
                {
                    if (desc.IsEmpty())
                    {
                        desc     = curd;
                        desc.GlyphNum -= CurTextPos - desc.AdjStartPos;
                        desc.AdjStartPos= CurTextPos;
                        if (desc.AdjStartPos + desc.GlyphNum > nearestNextPos)
                        {
                            desc.GlyphNum = nearestNextPos - desc.AdjStartPos;
                            nearestNextPos = desc.AdjStartPos + desc.GlyphNum;
                        }
                        //c//urLen   = curd.GlyphNum;
                    }
                    else
                    {
                        if ((curd.Info.Flags & Flags) != Flags)
                        {
                            desc.Info.Prepend(curd.Info);
                            desc.GlyphNum = G_PMin(curd.AdjStartPos + curd.GlyphNum, desc.AdjStartPos + desc.GlyphNum) - desc.AdjStartPos;
                            nearestNextPos = desc.AdjStartPos + desc.GlyphNum;
                        }
                    }
                }
                if (curd.AdjStartPos > CurTextPos)
                {
                    nearestNextPos = G_PMin(nearestNextPos, curd.AdjStartPos);
                    if (!desc.IsEmpty() && desc.AdjStartPos + desc.GlyphNum > nearestNextPos)
                        desc.GlyphNum = nearestNextPos - desc.AdjStartPos;
                }
            }
        }
        CurDesc = desc;
        CurTextPos = nearestNextPos;
    } while(desc.IsEmpty() && nearestNextPos != GFC_MAX_UPINT);
}


GFxTextHighlightDesc GFxTextHighlighterRangeIterator::operator*()
{
    return CurDesc;
}

void GFxTextHighlighterRangeIterator::operator++(int)
{
    if (!IsFinished())
    {
        //CurTextPos = CurDesc.AdjStartPos + CurDesc.GlyphNum;
        InitCurDesc();
    }
}

bool GFxTextHighlighterRangeIterator::IsFinished() const
{
    return !(CurRangeIndex < pManager->Highlighters.GetSize()) || CurDesc.GlyphNum == 0;
}

//////////////////////////////////////////////////////////////////////////
// GFxTextHighlighterPosIterator
// 
GFxTextHighlighterPosIterator::GFxTextHighlighterPosIterator
    (const GFxTextHighlighter* pmanager, UPInt startPos, UPInt len) : 
    pManager(pmanager), CurAdjStartPos(startPos), NumGlyphs(len)
{
    InitCurDesc();
}

void GFxTextHighlighterPosIterator::InitCurDesc()
{
    if (!IsFinished())
    {
        GFxTextHighlightDesc desc;
        for (UPInt i = 0, n = pManager->Highlighters.GetSize(); i < n; ++i)
        {
            const GFxTextHighlightDesc& curd = pManager->Highlighters[i];
            if (curd.ContainsIndex(CurAdjStartPos))
                desc.Info.Prepend(curd.Info);
        }
        CurDesc = desc;
        CurDesc.GlyphNum   = 1;
    }
    else
    {
        CurDesc.Info.Reset();
        CurDesc.GlyphNum   = 0;
    }
    CurDesc.AdjStartPos = CurAdjStartPos;
    CurDesc.Id          = 0;
}



void GFxTextHighlighterPosIterator::operator++(int)
{
    if (!IsFinished())
    {
        ++CurAdjStartPos;
        InitCurDesc();
    }
}

void GFxTextHighlighterPosIterator::operator+=(UPInt p)
{
    if (!IsFinished() && p > 0)
    {
        CurAdjStartPos += p;
        InitCurDesc();
    }
}

bool GFxTextHighlighterPosIterator::IsFinished() const
{
    return CurAdjStartPos >= NumGlyphs;
}
