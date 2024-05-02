/**********************************************************************

Filename    :   GFxStaticText.cpp
Content     :   Static text field character implementation
Created     :   May, 2007
Authors     :   Artem Bolgar

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GConfig.h"

#include "GUTF8Util.h"

#include "GRenderer.h"

#include "GFxCharacter.h"
#include "GFxShape.h"
#include "GFxStream.h"
#include "GFxLog.h"
#include "GFxFontResource.h"
#include "GFxText.h"

#include "AS/GASNumberObject.h"

// For Root()->GetRenderer
#include "GFxPlayerImpl.h"
#include "GFxSprite.h"

#include "GFxDisplayContext.h"
#include "GFxLoadProcess.h"

#include "GFxStaticText.h"
#include "GMath2D.h"

//
// TextCharacter
//

//////////////////////////////////////////////////////////////////////////
// GFxStaticTextRecord
//
GFxStaticTextRecord::GFxStaticTextRecord() :
    Offset(0.0f),       
    TextHeight(1.0f),
    FontId(0)
{
}

void    GFxStaticTextRecord::Read(GFxStream* in, int glyphCount, int glyphBits, int advanceBits)
{
    Glyphs.Resize(glyphCount);
    for (int i = 0; i < glyphCount; i++)
    {
        Glyphs[i].GlyphIndex = in->ReadUInt(glyphBits);
        Glyphs[i].GlyphAdvance = (Float) in->ReadSInt(advanceBits);
    }
}

// Obtains cumulative advance value.
Float   GFxStaticTextRecord::GetCumulativeAdvance() const
{
    Float advance = 0;
    for (UPInt i = 0, glyphCount = Glyphs.GetSize(); i < glyphCount; i++)
        advance += Glyphs[i].GlyphAdvance;
    return advance;
}
//////////////////////////////////////////////////////////////////////////
// GFxStaticTextRecordList
//
void GFxStaticTextRecordList::Clear()
{
    for (UPInt i=0, sz = Records.GetSize(); i < sz; i++)
        if (Records[i]) delete Records[i];
    Records.Clear();
}

GFxStaticTextRecord* GFxStaticTextRecordList::AddRecord()
{
    GFxStaticTextRecord* precord = GHEAP_AUTO_NEW(this) GFxStaticTextRecord;
    if (precord)    
        Records.PushBack(precord);     
    return precord;
}

//////////////////////////////////////////////////////////////////////////
// GFxStaticTextCharacterDef
//
GFxStaticTextCharacterDef::GFxStaticTextCharacterDef()
 : Flags(0)
{
}

void    GFxStaticTextCharacterDef::Read(GFxLoadProcess* p, GFxTagType tagType)
{                
    GASSERT(tagType == GFxTag_DefineText || tagType == GFxTag_DefineText2);
    GFxStream *in = p->GetStream();

    in->ReadRect(&TextRect);

    in->LogParse("  TextRect = { l: %f, t: %f, r: %f, b: %f }\n", 
        (float)TextRect.Left, (float)TextRect.Top, (float)TextRect.Right, (float)TextRect.Bottom);

    in->ReadMatrix(&MatrixPriv);

    in->LogParse("  mat:\n");
    in->LogParseClass(MatrixPriv);

    int GlyphBits = in->ReadU8();
    int AdvanceBits = in->ReadU8();

    in->LogParse("begin text records\n");

    bool    lastRecordWasStyleChange = false;
    
    GPointF offset;
    GColor color;
    Float textHeight    = 0;
    UInt16  fontId      = 0;
    GFxResourcePtr<GFxFontResource> pfont;

    // Style offset starts at 0 and is overwritten when specified.
    offset.SetPoint(0.0f);

    for (;;)
    {
        int FirstByte = in->ReadU8();
        
        if (FirstByte == 0)
        {
            // This is the end of the text records.
            in->LogParse("end text records\n");
            break;
        }

        // Style changes and glyph records just alternate.
        // (Contrary to what most SWF references say!)
        if (lastRecordWasStyleChange == false)
        {
            // This is a style change.

            lastRecordWasStyleChange = true;

            bool    hasFont     = (FirstByte >> 3) & 1;
            bool    hasColor    = (FirstByte >> 2) & 1;
            bool    hasYOffset  = (FirstByte >> 1) & 1;
            bool    hasXOffset  = (FirstByte >> 0) & 1;

            in->LogParse("  text style change\n");

            if (hasFont)
            {
                fontId = in->ReadU16();
                in->LogParse("  HasFont: font id = %d\n", fontId);
                // Create a font resource handle.
                GFxResourceHandle hres;
                p->GetResourceHandle(&hres, GFxResourceId(fontId));
                pfont.SetFromHandle(hres);
            }
            if (hasColor)
            {
                if (tagType == 11)
                {
                    in->ReadRgb(&color);                          
                }
                else
                {
                    GASSERT(tagType == 33);                         
                    in->ReadRgba(&color);
                }
                in->LogParse("  HasColor\n");
            }
            if (hasXOffset)
            {                   
                offset.x = in->ReadS16();
                in->LogParse("  XOffset = %g\n", offset.x);
            }
            else
            {
                // Leave carry-over from last record.                   
            }
            if (hasYOffset)
            {                   
                offset.y = in->ReadS16();
                in->LogParse("  YOffset = %g\n", offset.y);
            }
            else
            {
                // Leave carry-over from last record.
            }
            if (hasFont)
            {
                textHeight = in->ReadU16();
                in->LogParse("  TextHeight = %g\n", textHeight);
            }
        }
        else
        {
            // Read the glyph record.
            lastRecordWasStyleChange = false;

            int GlyphCount = FirstByte;
            // Don't mask the top GlyphCount bit; the first record is allowed to have > 127 glyphs.

            GFxStaticTextRecord* precord = TextRecords.AddRecord();
            if (precord)
            {
                precord->Offset     = offset;
                precord->pFont      = pfont;
                precord->TextHeight = textHeight;
                precord->Color      = color;
                precord->FontId     = fontId;
                precord->Read(in, GlyphCount, GlyphBits, AdvanceBits);

                // Add up advances and adjust offset.
                offset.x += precord->GetCumulativeAdvance();
            }

            in->LogParse("  GlyphRecords: count = %d\n", GlyphCount);
        }
    }
}

bool    GFxStaticTextCharacterDef::DefPointTestLocal(const GPointF &pt, bool testShape, const GFxCharacter* pinst) const
{
    GUNUSED2(testShape, pinst);
    // Flash only uses the bounding box for text - even if shape flag is used
    return TextRect.Contains(pt);
}

GFxCharacter*   GFxStaticTextCharacterDef::CreateCharacterInstance(GFxASCharacter* parent, GFxResourceId id,
                                                                   GFxMovieDefImpl *pbindingImpl)
{
    GFxStaticTextCharacter* ch = 
        GHEAP_AUTO_NEW(parent) GFxStaticTextCharacter(this, pbindingImpl, parent, id);
    return ch;
}

void    GSTDCALL GFx_DefineTextLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)  // Read a DefineText tag.
{
    GASSERT(tagInfo.TagType == GFxTag_DefineText || tagInfo.TagType == GFxTag_DefineText2);

    UInt16  characterId = p->ReadU16();
    
    GPtr<GFxStaticTextCharacterDef> pch =
        *GHEAP_NEW_ID(p->GetLoadHeap(),GFxStatMD_CharDefs_Mem) GFxStaticTextCharacterDef();
    p->LogParse("TextCharacter, id = %d\n", characterId);
    pch->Read(p, tagInfo.TagType);

    // Log print some parse stuff...

    p->AddResource(GFxResourceId(characterId), pch);
}


GFxStaticTextCharacter::GFxStaticTextCharacter(GFxStaticTextCharacterDef* pdef, 
                                               GFxMovieDefImpl *pbindingDefImpl, 
                                               GFxASCharacter* parent, 
                                               GFxResourceId id)
 : GFxGenericCharacter(pdef, parent, id), pHighlight(NULL), Flags(0)
{
//    if (id.GetIdIndex() != 286)
 //       return;
    Matrix invm = pdef->MatrixPriv;
    invm.Invert();
    GRectF r = invm.EncloseTransform(pdef->TextRect);
    UInt glyphAccum = 0;
    GPointF initialTL = r.TopLeft();
    r.ExpandToPoint(GPointF(0,0));

    for (UInt i = 0, n = pdef->TextRecords.GetSize(); i < n; ++i)
    {
        const GFxStaticTextRecord* prec = pdef->TextRecords.Records[i];

        // resolve font
        GASSERT(prec->FontId != 0);

        GFxResourceBindData fontData = pbindingDefImpl->GetResourceBinding().GetResourceData(prec->pFont);
        GFxFontResource* pfont = (GFxFontResource*)fontData.pResource.GetPtr();
        if (!pfont)
        {
            GFxLog* plog = pbindingDefImpl->GetLog();
            if (plog)
                plog->LogError("Error: text style with undefined font; FontId = %d\n", prec->FontId);
            return;
        }

        if (!pdef->HasInstances())
        {
            // do this check only once per def
            GFxMovieDataDef* pdataDef = pbindingDefImpl->GetDataDef();
            GFxImportData* pimport = pdataDef->GetFirstImport();
            for(; pimport != 0; pimport = pimport->pNext)
            {
                for (UInt j = 0; j<pimport->Imports.GetSize(); j++)
                {
                    if (pimport->Imports[j].BindIndex == prec->pFont.GetBindIndex())
                    {
                        // found import table entry, display warning.
                        // It is highly not recommended to use imported fonts for static texts
                        // since imported font might be substituted by the localization stuff. 
                        // Unfortunately, Flash may use imported font implicitly, for example, 
                        // when static text uses font "Arial" and imported font "$Font" represents
                        // also the font "Arial" (see Properties window for imported font). In this 
                        // case, Flash won't create a local "Arial" font for the static text, it
                        // will use the "$Font" instead. Since, static text does not reformat
                        // the text for the different fonts, replacing the "$Font" by another 
                        // font may screw up the static text because of different advance values.
                        GFxLog* plog = pbindingDefImpl->GetLog();
                        if (plog)
                            plog->LogWarning("Warning: static text uses imported font! FontId = %d, import name = %s\n", 
                            prec->FontId, pimport->Imports[j].SymbolName.ToCStr());
                    }
                }
            }
        }

        // It could be more efficient to obtain GFxFontHandle from manager, however,
        // this is a resource id based lookup (not font name based).
        GPtr<GFxFontHandle> pfontHandle = *GHEAP_AUTO_NEW(this) GFxFontHandle(NULL, pfont);
        GASSERT(pfont->GetFont()->IsResolved());

        GFxTextLineBuffer::Line* pline;
        UInt glyphsCount = (UInt)prec->Glyphs.GetSize();        
        if (glyphsCount <= 255 && !pfont->HasLayout())
        {
            // convert line to short form. Do not use short form if font has layout:
            // in this case we would need to set dimensions on lines and it is hard to
            // predict ranges of dimensions.
            pline = TextGlyphRecords.InsertNewLine(i, glyphsCount, 2, GFxTextLineBuffer::Line8);
        }
        else
        {
            // Regular form. Used if there are more than 255 glyphs, or font
            // has layout.
            pline = TextGlyphRecords.InsertNewLine(i, glyphsCount, 2, GFxTextLineBuffer::Line32);
        }
        GASSERT(pline);

        pline->SetTextPos(glyphAccum);
        glyphAccum += glyphsCount;

        GPointF offset = prec->Offset;
        Float lineHeight = prec->TextHeight;
        if (pfont->HasLayout())
        {
            // There is a layout in the font. This means there we have ascent and descent
            // members in the font and that is why we can set a baseline for the line 
            // (with appropriate correction of its offset.y). This might be necessary
            // to select the static text via TextSnapshot.
            // By default, static text font might have no ascent/descent. In this case
            // selection will not work.

            Double fontScale = prec->TextHeight / 1024;
 
            offset.y -= prec->TextHeight;
            offset -= r.TopLeft();

            // AB: seems to me, Flash uses the TextHeight as ascent value to calculate
            // height of the line. Height of the line is important for the selection
            // via TextSnapshot class.
            Float baseLine = prec->TextHeight;

            pline->SetBaseLineOffset(baseLine);

            lineHeight = Float(prec->TextHeight + pfont->GetDescent()* fontScale);
        }
        else
        {
            offset -= r.TopLeft();
            lineHeight = 0;
        }
        pline->SetOffset(RoundTwips(offset.x), RoundTwips(offset.y));
        pline->SetDimensions(0, 0);
        GFxTextLineBuffer::GlyphInserter gins(pline->GetGlyphs(), glyphsCount, pline->GetFormatData());

        SInt advanceAccum = 0;
        for (UInt i = 0; i < glyphsCount; ++i, ++gins)
        {
            GFxTextLineBuffer::GlyphEntry& glyph = gins.GetGlyph();
            glyph.SetLength(1);
            glyph.SetIndex(prec->Glyphs[i].GlyphIndex);
            glyph.SetAdvance((SInt)prec->Glyphs[i].GlyphAdvance);
            advanceAccum += (SInt)prec->Glyphs[i].GlyphAdvance;
            glyph.SetFontSize((Float)TwipsToPixels(prec->TextHeight));
            if (i == 0)
            {
                gins.AddFont(pfontHandle);
                gins.AddColor(prec->Color);
            }
        }

        if (pfont->HasLayout())
        {
            pline->SetDimensions(G_Max(0, advanceAccum), G_IRound(lineHeight));
            pline->SetTextLength(glyphsCount);

            //correct bottom-right corner of visible rectangle
            r.ExpandToPoint(GPointF(Float(pline->GetOffsetX() + pline->GetWidth()),
                                    Float(pline->GetOffsetY() + pline->GetHeight())));
        }
    }
    TextGlyphRecords.SetVisibleRect(r);
    TextGlyphRecords.SetStaticText();
    //TextGlyphRecords.Dump(); //@DBG
    Filter.SetDefaultShadow();
    pdef->SetHasInstances();   
}

GFxStaticTextCharacter::~GFxStaticTextCharacter()
{
    if (pHighlight)
        delete pHighlight;
}

void    GFxStaticTextCharacter::Display(GFxDisplayContext &context)   
{
    GFxStaticTextCharacterDef* pdef = static_cast<GFxStaticTextCharacterDef*>(pDef);
    // And displays records.
    GRenderer::Matrix m = *context.pParentMatrix;
    m.Prepend(GetMatrix());

    GRenderer::Cxform cx = *context.pParentCxform;
    cx.Concatenate(GetCxform());

    // Do viewport culling if bounds are available (and not a 3D object).
    if (
#ifndef GFC_NO_3D
        context.pParentMatrix3D == NULL && 
#endif
        !GetMovieRoot()->GetVisibleFrameRectInTwips().Intersects(m.EncloseTransform(pdef->TextRect)))
        if (!(context.GetRenderFlags() & GFxRenderConfig::RF_NoViewCull))
            return;

    GFxTextFieldParam param;
    param.LoadFromTextFilter(Filter);

    if (pdef->IsAAForReadability())
    {
        param.TextParam.SetOptRead(true);
        param.TextParam.SetAutoFit(true);
        param.ShadowParam.SetOptRead(true);
        param.ShadowParam.SetAutoFit(true);
    }

    m.Prepend(pdef->MatrixPriv);

    if (pHighlight)
    {
        // Refresh highlight drawing if one exists
        if (!pHighlight->IsValid())
            pHighlight->DrawBackground(TextGlyphRecords, TextGlyphRecords.Geom.VisibleRect);    

        // Draw highlight if one exists
        pHighlight->Display(context, m, cx);
    }
    
    context.pChar = this;
    TextGlyphRecords.Display(context, m, cx, 
        (Flags & Flags_NextFrame) != 0,
        param);

    Flags &= ~Flags_NextFrame;
}

//
// Creates the text highlighter
//
GFxStaticTextCharacter::HighlightDesc*  GFxStaticTextCharacter::CreateTextHighlighter()
{
    if (!pHighlight)
        pHighlight = GHEAP_NEW(GetMovieRoot()->GetHeap()) GFxStaticTextCharacter::HighlightDesc;
    return pHighlight;
}

// --------------------------------------------------------------------

#ifndef GFC_NO_FXPLAYER_AS_TEXTSNAPSHOT

GFxStaticTextSnapshotData::GFxStaticTextSnapshotData()
{
    SelectColor = GColor(255, 255, 0, 255); // Yellow
}

void    GFxStaticTextSnapshotData::Visit(GlyphVisitor* pvisitor, UPInt start, UPInt end) const
{
    if (!pvisitor) return;
    UPInt glyphIdx = 0;
    for (UPInt i = 0, n = StaticTextCharRefs.GetSize(); i < n; ++i)
    {
        Float glyphX = 0;
        Float glyphY = 0;
        GFxStaticTextCharacter* pstchar = StaticTextCharRefs[i].pChar;
        Float charX = 0, charY = 0;
        const GMatrix2D& matChar = pstchar->GetMatrix();
        matChar.Transform(&charX, &charY);
        for (GFxTextLineBuffer::Iterator iter = pstchar->TextGlyphRecords.Begin(); !iter.IsFinished(); ++iter)
        {
            GFxTextLineBuffer::Line& line = (*iter);
            UPInt runIdx = 0;
            Float xoff = (Float)line.GetOffsetX();
            if (xoff < glyphX)
                glyphX = xoff;
            glyphY = Float(line.GetOffsetY() + line.GetLeading());
            GFxTextLineBuffer::GlyphIterator glyphIt = line.Begin();
            GFxFontResource* pfont = glyphIt.GetFont();
            pvisitor->pFont = pfont->GetFont(); 
            pvisitor->Color = glyphIt.GetColor();
            for (; !glyphIt.IsFinished(); ++glyphIt)
            {
                GFxTextLineBuffer::GlyphEntry& glyph = glyphIt.GetGlyph();
                // Check range
                if (glyphIdx >= start && glyphIdx < end)
                {
                    // Setup state
                    pvisitor->RunIdx = runIdx;
                    pvisitor->Height = glyph.GetFontSize();
                    GMatrix2D mat;
                    mat.AppendRotation((Float)matChar.GetRotation());  // Correct
                    mat.AppendTranslation(charX + glyphX, charY + glyphY);
                    pvisitor->Matrix = mat;
                    GRectF corners;
                    pfont->GetGlyphBounds(glyph.GetIndex(), &corners);
                    pvisitor->Corners = mat.EncloseTransform(corners);
                    // Expensive..?
                    pvisitor->bSelected = IsSelected(glyphIdx, glyphIdx+1);
                    // Visit
                    pvisitor->OnVisit();

                    ++runIdx;
                }
                glyphX += glyph.GetAdvance();
                ++glyphIdx;
            }          
        }
    }
}

void    GFxStaticTextSnapshotData::Add(GFxStaticTextCharacter* pstChar)
{
    GFxStaticTextSnapshotData::CharRef  cRef;
    cRef.pChar = pstChar;
    cRef.CharCount = 0;    
    Float xoffset = 0;
    bool bfirst = true;

    // Iterate over line buffer to get string data and length
    for (GFxTextLineBuffer::Iterator iter = pstChar->TextGlyphRecords.Begin(); !iter.IsFinished(); ++iter)
    {        
        GFxTextLineBuffer::Line& line = (*iter);
        Float lineXOffset = Float(line.GetOffsetX());
        if (!bfirst && lineXOffset == xoffset)
            SnapshotString.AppendChar('\n');
        else if (bfirst)
            xoffset = lineXOffset;
        GFxTextLineBuffer::GlyphIterator glyphIt = line.Begin();
        GFxFont* pfont = glyphIt.GetFont()->GetFont();
        if (!pfont) continue;
        for (; !glyphIt.IsFinished(); ++glyphIt)
        {
            GFxTextLineBuffer::GlyphEntry& glyph = glyphIt.GetGlyph();
            UInt idx = glyph.GetIndex();
            // Get UCS2 char
            int ucs2 = pfont->GetCharValue(idx);
            if (ucs2 == -1) continue;
            // Store values
            cRef.CharCount++;
            SnapshotString.AppendChar(ucs2);
        }
        bfirst = false;
    }

    StaticTextCharRefs.PushBack(cRef);
}

UPInt   GFxStaticTextSnapshotData::GetCharCount() const
{
    UPInt count = 0;
    for (UPInt i = 0; i < StaticTextCharRefs.GetSize(); i++)
    {
        count += StaticTextCharRefs[i].CharCount;
    }
    return count;
}

GString   GFxStaticTextSnapshotData::GetSubString(UPInt start, UPInt end, bool binclNewLines) const
{
    // Strip out newlines if required
    GString ret;
    const char* pchar = SnapshotString.ToCStr();
    UInt32 cval;
    while ( (start < end) && (cval = GUTF8Util::DecodeNextChar(&pchar)) != '\0' )
    {
        if (cval != '\n')
        {
            ret.AppendChar(cval);
            start++;
        }
        else if (binclNewLines)
        {
            ret.AppendChar(cval);
        }
    }
    return ret;
}


GString GFxStaticTextSnapshotData::GetSelectedText(bool binclNewLines) const
{
    GString ret;
    UPInt baseIdx = 0;
    UPInt lastTextIdx = 0;
    const char* pchar = SnapshotString.ToCStr();
    UInt32 c;
    for (UPInt i = 0; i < StaticTextCharRefs.GetSize(); i++)
    {
        GFxStaticTextCharacter* pstchar = StaticTextCharRefs[i].pChar;
        GFxStaticTextCharacter::HighlightDesc* phighlight = pstchar->GetTextHighlighter();
        if (!phighlight)
            continue;

        GFxTextHighlighterRangeIterator ranges = phighlight->GetManager().GetRangeIterator();
        for (; !ranges.IsFinished(); ++ranges)
        {
            GFxTextHighlightDesc desc = (*ranges);
            // Correct range values
            desc.StartPos += baseIdx;
            // Should be added to string?
            UPInt& startIdx = desc.StartPos;
            UPInt   endIdx = startIdx + desc.Length;
            if (endIdx <= lastTextIdx)
                continue;   // Range has already been processed
            // Skip intermediate chars
            while (lastTextIdx < startIdx)
            {
                c = GUTF8Util::DecodeNextChar(&pchar);
                if (c == '\n')
                    lastTextIdx--;
                lastTextIdx++;
            }
            for (UPInt i = lastTextIdx; i < endIdx; i++)
            {
                do 
                {
                    c = GUTF8Util::DecodeNextChar(&pchar);                    
                    // Append newlines if required
                    if (binclNewLines && c == '\n')
                        ret.AppendChar(c);
                        
                } while (c == '\n');    // Skip newlines for actual processing
                ret.AppendChar(c);
            }
            lastTextIdx = endIdx;
        }
        baseIdx += StaticTextCharRefs[i].CharCount;
    }
    return ret;
}


//
// Special substring finder that supports case insensitive search. 
// Newlines in the source string are also skipped during comparison.
//
int   GFxStaticTextSnapshotData::FindText(UPInt start, const char* query, bool bcaseSensitive) const
{
    int startidx = (int)start;
    UInt32 chr;
    UInt32 first = GUTF8Util::DecodeNextChar(&query);
    const char* string = SnapshotString.ToCStr();

    for(int i = 0; 
        (chr = GUTF8Util::DecodeNextChar(&string)) != 0;
        i++)
    {
        if (i >= startidx && 
            (( (bcaseSensitive && chr == first)) || (!bcaseSensitive && G_toupper(chr) == G_toupper(first))))
        {
            const char* s1 = string;
            const char* s2 = query;
            UInt32 c1, c2 = 0; 
            SInt skipCount;
            for(;;)
            {
                skipCount = -1;

                c2 = GUTF8Util::DecodeNextChar(&s2);

                // newline check. if found, skip over
                do 
                {
                    c1 = GUTF8Util::DecodeNextChar(&s1);
                    ++skipCount;
                } 
                while (c1 == '\n');

                if (c1 == 0 || c2 == 0) break;

                if (bcaseSensitive)
                {
                    if (c1 != c2)
                        break;
                }
                else
                {
                    if (G_toupper(c1) != G_toupper(c2)) 
                        break;
                }
            }
            if (c2 == 0)
            {                
                return i;
            }
            if (c1 == 0)
            {
                return -1;
            }
            i -= skipCount;
        }
        // another newline check
        else if (chr == '\n')
            --i;
    }
    return -1;
}


//
// Return true if any char in input range is selected
//
bool    GFxStaticTextSnapshotData::IsSelected(UPInt start, UPInt end) const
{
    GString ret;
    UPInt baseIdx = 0, charStart, charEnd;
    UPInt const& selectStart = start, selectEnd = end;
    for (UPInt i = 0; i < StaticTextCharRefs.GetSize(); i++)
    {
        GFxStaticTextCharacter* pstchar = StaticTextCharRefs[i].pChar;
        GFxStaticTextCharacter::HighlightDesc* phighlight = pstchar->GetTextHighlighter();
        if (!phighlight)
            continue;
        charStart = baseIdx;
        charEnd = baseIdx + StaticTextCharRefs[i].CharCount;
        // In range?
        if ( (selectStart >= charStart && selectStart < charEnd) ||
             (charStart >= selectStart && charStart < selectEnd) )
        {
            if (phighlight->GetManager().IsAnyCharSelected(selectStart - baseIdx, selectEnd - baseIdx))
                return true;
        }
        baseIdx += StaticTextCharRefs[i].CharCount;
    }
    return false;
}


void    GFxStaticTextSnapshotData::SetSelected(UPInt start, UPInt end, bool bselect) const
{
    UPInt baseIdx = 0, lenUnprocessed = end - start, charStartIdx, charEndIdx; 
    for (UPInt i = 0; i < StaticTextCharRefs.GetSize(); i++)
    {
        charStartIdx = baseIdx;
        charEndIdx = baseIdx + StaticTextCharRefs[i].CharCount;
        // Is static text char in range?
        if ( (charStartIdx >= start && charStartIdx < end) ||
             (start >= charStartIdx && start < charEndIdx) )
        {
            GFxStaticTextCharacter* pstc = StaticTextCharRefs[i].pChar;
            // Is there already a highlighter?
            GFxStaticTextCharacter::HighlightDesc* phighlight = pstc->GetTextHighlighter();
            if (!phighlight)
                phighlight = pstc->CreateTextHighlighter();
            // Create descriptor
            GFxTextHighlightDesc desc;
            desc.Info.SetBackgroundColor(SelectColor);
            desc.StartPos = (baseIdx > start) ? 0 : (start - baseIdx);
            //desc.Length = lenUnprocessed < StaticTextCharRefs[i].CharCount ? lenUnprocessed : StaticTextCharRefs[i].CharCount;
            desc.Length = G_Min(lenUnprocessed, StaticTextCharRefs[i].CharCount - desc.StartPos);
            desc.GlyphNum = desc.Length;
            desc.AdjStartPos = desc.StartPos;
            if (bselect)
            {
                // Add highlight range
                phighlight->GetManager().Add(desc);
            }   
            else
            {
                // Remove highlight range
                phighlight->GetManager().Remove(desc);
            }
            lenUnprocessed -= desc.Length;
        }
        // Increment char position
        baseIdx = charEndIdx;        
    }    
}


void    GFxStaticTextSnapshotData::SetSelectColor(const GColor& color)
{
    SelectColor = color;
    // Set color and invalidate all highlights in all static chars
    for (UPInt i = 0; i < StaticTextCharRefs.GetSize(); i++)
    {
        GFxStaticTextCharacter* pstc = StaticTextCharRefs[i].pChar;
        // Is there already a highlighter?
        if (pstc->GetTextHighlighter())
            pstc->GetTextHighlighter()->SetSelectColor(SelectColor);
    }
}

GPointF ClosestPointOnRectangle(const GRectF& r, const GPointF& p)
{    
    const GPointF& tl = r.TopLeft();
    const GPointF& tr = r.TopRight();
    const GPointF& bl = r.BottomLeft();
    const GPointF& br = r.BottomRight();

    enum RegionFlags
    {
        Reg_X0 = 0x00,
        Reg_X1 = 0x01,
        Reg_X2 = 0x02,
        Reg_Y0 = 0x00,
        Reg_Y1 = 0x10,
        Reg_Y2 = 0x20,
    };
    UInt32 flags = 0;

    if      (p.x <= tl.x)   flags |= Reg_X0;
    else if (p.x >= tr.x)   flags |= Reg_X2;
    else                    flags |= Reg_X1;
    if      (p.y <= tl.y)   flags |= Reg_Y0;
    else if (p.y >= bl.y)   flags |= Reg_Y2;
    else                    flags |= Reg_Y1;

    Float t;
    GPointF ret(GFC_MAX_FLOAT, GFC_MAX_FLOAT);

    switch (flags)
    {
    case Reg_X0 | Reg_Y0:
        return tl;

    case Reg_X0 | Reg_Y1:
        t = GMath2D::CalcPointToSegmentPos(tl, bl, p);
        GASSERT(t >= 0 && t <= 1.0f);
        ret.x = tl.x + (bl.x - tl.x) * t;
        ret.y = tl.y + (bl.y - tl.y) * t;
        return ret;

    case Reg_X0 | Reg_Y2:
        return bl;

    case Reg_X1 | Reg_Y0:
        t = GMath2D::CalcPointToSegmentPos(tl, tr, p);
        GASSERT(t >= 0 && t <= 1.0f);
        ret.x = tl.x + (tr.x - tl.x) * t;
        ret.y = tl.y + (tr.y - tl.y) * t;
        return ret;

    case Reg_X1 | Reg_Y1:
        // Inside rectangle.. We don't care about this case
        // since this was checked already. Right?
        GASSERT(0);

    case Reg_X1 | Reg_Y2:
        t = GMath2D::CalcPointToSegmentPos(bl, br, p);
        GASSERT(t >= 0 && t <= 1.0f);
        ret.x = bl.x + (br.x - bl.x) * t;
        ret.y = bl.y + (br.y - bl.y) * t;
        return ret;

    case Reg_X2 | Reg_Y0:
        return tr;

    case Reg_X2 | Reg_Y1:
        t = GMath2D::CalcPointToSegmentPos(tr, br, p);
        GASSERT(t >= 0 && t <= 1.0f);
        ret.x = tr.x + (br.x - tr.x) * t;
        ret.y = tr.y + (br.y - tr.y) * t;
        return ret;
    case Reg_X2 | Reg_Y2:
        return br;
    }

    GASSERT(0); // Shouldn't be here
    return ret;
}

SInt    GFxStaticTextSnapshotData::HitTestTextNearPos(Float x, Float y, Float closedist) const
{
    GFxStaticTextCharacter* pclosestChar = NULL;
    Float minDist = GFC_MAX_FLOAT;
    UPInt tempBaseIdx = 0, baseIdx = 0;
    Float fixedX = 0, fixedY = 0;
    GPointF cpb(x, y);

    // First find closest Static Textfield
    for (UPInt i = 0; i < StaticTextCharRefs.GetSize(); i++)
    {
        GFxStaticTextCharacter* pstc = StaticTextCharRefs[i].pChar;    
        GPointF ip = pstc->GetMatrix().TransformByInverse(cpb);
        GRectF& tf = pstc->TextGlyphRecords.Geom.VisibleRect;        
        // If point is inside textfield rect, we're done searching
        if (tf.Contains(ip))
        {
            pclosestChar = pstc;
            baseIdx = tempBaseIdx;
            fixedX = ip.x;
            fixedY = ip.y;
            break;
        }
        // Else keep track of closest textfield
        else
        {
            GPointF rp = ClosestPointOnRectangle(tf, ip);
            Float dist2rect = GMath2D::CalcDistance(ip, rp);
            if (dist2rect < closedist && dist2rect < minDist)
            {
                pclosestChar = pstc;
                minDist = dist2rect;
                baseIdx = tempBaseIdx;
                fixedX = rp.x;
                fixedY = rp.y;
            }            
        }
        tempBaseIdx += StaticTextCharRefs[i].CharCount;
    }

    if (!pclosestChar)
        return -1;

    fixedX -= pclosestChar->TextGlyphRecords.Geom.VisibleRect.Left;
    fixedY -= pclosestChar->TextGlyphRecords.Geom.VisibleRect.Top;

    GFxTextLineBuffer::Iterator it = pclosestChar->TextGlyphRecords.FindLineAtOffset(GPointF(fixedX, fixedY));
    if (!it.IsFinished())
    {
        GFxTextLineBuffer::Line& line = *it;

        Float lineOffX = Float(line.GetOffsetX());
        if (fixedX >= lineOffX && fixedX <= lineOffX + Float(line.GetWidth()))
        {
            Float xoffInLine = fixedX - lineOffX;
            SInt xoffset = 0;

            // now need to find the glyph entry. Do the exhaustive search for now
            GFxTextLineBuffer::GlyphIterator git = line.Begin();
            UInt j = 0;
            for(; !git.IsFinished(); ++git)
            {
                const GFxTextLineBuffer::GlyphEntry& ge = git.GetGlyph();
                xoffset += ge.GetAdvance();

                if (xoffset > xoffInLine)
                    break;

                j += ge.GetLength();
            }
            
            return (SInt)(line.GetTextPos() + j + baseIdx);
        }
    }

    return -1;
}

#endif  // GFC_NO_FXPLAYER_AS_TEXTSNAPSHOT
