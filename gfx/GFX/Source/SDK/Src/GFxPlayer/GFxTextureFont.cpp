/**********************************************************************

Filename    :   GFxTextureFont.cpp
Content     :   Application-created texture fonts.
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxTextureFont.h"
#include "GFxFontResource.h"
#include "GFxShape.h"
#include "GMemory.h"

GFxTextureFont::GFxTextureFont(const char* name, UInt fontFlags, UInt numGlyphs)
: GFxFont(fontFlags), Name(NULL)
{
    if (name)
    {
        Name = (char*)GALLOC(G_strlen(name) + 1, GFxStatMD_Fonts_Mem);
        if (Name)
            G_strcpy(Name, G_strlen(name) + 1, name);
    }
    else
        Name = NULL;

    pTextureGlyphData = *GNEW GFxTextureGlyphData(numGlyphs, true);

    Flags = FF_HasLayout|FF_GlyphShapesStripped|FF_WideCodes;
}


GFxTextureFont::~GFxTextureFont()
{
    Glyphs.Resize(0);

    // Delete the name string.
    if (Name)
    {
        GFREE(Name);
        Name = NULL;
    }
}

void GFxTextureFont::SetTextureParams(GFxFontPackParams::TextureConfig params, UInt flags, UInt padding)
{
    pTextureGlyphData->SetTextureConfig(params);
    pTextureGlyphData->SetTextureFlags(flags);
    pTextureGlyphData->SetCharPadding(padding);
}

void GFxTextureFont::AddTextureGlyph(UInt glyphIndex, GFxImageResource* pimage,
    GRenderer::Rect UvBounds, GRenderer::Point UvOrigin, AdvanceEntry advance)
{
    GFxTextureGlyph tg;
    tg.SetImageResource(pimage);
    tg.UvBounds = UvBounds;
    tg.UvOrigin = UvOrigin;
    pTextureGlyphData->AddTextureGlyph(glyphIndex, tg);

    if (AdvanceTable.GetSize() <= glyphIndex)
    {
        AdvanceTable.Resize(glyphIndex+1);
        if (AdvanceTable.GetSize() <= glyphIndex)
            return;
    }

    AdvanceTable[glyphIndex] = advance;
}


int GFxTextureFont::GetGlyphIndex(UInt16 code) const
{
    UInt16 glyphIndex;
    if (CodeTable.Get(code, &glyphIndex))
    {
        return glyphIndex;
    }
    return -1;
}

int     GFxTextureFont::GetCharValue(UInt glyphIndex) const
{
    for (GHashIdentityLH<UInt16, UInt16>::ConstIterator iter = CodeTable.Begin();
        iter != CodeTable.End(); ++iter)
    {
        if (iter->Second == glyphIndex)
            return iter->First;
    }
    return -1;
}

Float   GFxTextureFont::GetAdvance(UInt GlyphIndex) const
{
    // how could this be if GlyphIndex is unsigned?
    if (GlyphIndex == (UInt)-1)
        return GetDefaultGlyphWidth();

    if (AdvanceTable.GetSize() == 0)
    {
        // No layout info for this GFxTextureFont!!!
        static bool Logged = false;
        if (Logged == false)
        {
            Logged = true;
            GFC_DEBUG_ERROR1(1, "Empty advance table in font %s", GetName());
        }

        // MA: It is better to return default advance, make char substitution show up better
        return GetDefaultGlyphWidth();        
    }

    if (GlyphIndex < AdvanceTable.GetSize())
    {
        GASSERT(GlyphIndex != (UInt)-1 );
        return AdvanceTable[GlyphIndex].Advance;
    }
    else
    {
        // Bad glyph index.  Due to bad data file?
        GASSERT(0);
        return 0;
    }
}

Float   GFxTextureFont::GetGlyphWidth(UInt glyphIndex) const
{
    if (glyphIndex == (UInt)-1)
        return GetDefaultGlyphWidth();

    if (glyphIndex < AdvanceTable.GetSize())
    {
        GASSERT(glyphIndex != (UInt)-1 );
        Float w = Float(AdvanceTable[glyphIndex].Width)/20.f;
        if (w != 0)
            return w;
    }
    return GetAdvance(glyphIndex); 
}

Float   GFxTextureFont::GetGlyphHeight(UInt glyphIndex) const
{
    // How could this be if GlyphIndex is unsigned?
    if (glyphIndex == (UInt)-1 || AdvanceTable.GetSize() == 0)    
        return GetDefaultGlyphHeight();

    if (glyphIndex < AdvanceTable.GetSize())
    {
        GASSERT(glyphIndex != (UInt)-1 );
        return Float(AdvanceTable[glyphIndex].Height)/20.f;
    }
    else
    {
        // Bad glyph index.  Due to bad data file?
        // This is possible, if font doesn't have layout (for static text).
        // But AdvanceTable should be empty in this case.
        GASSERT(AdvanceTable.GetSize() == 0);
        return 0;
    }
}

GRectF& GFxTextureFont::GetGlyphBounds(UInt glyphIndex, GRectF* prect) const
{
    if (glyphIndex == (UInt)-1)
    {
        prect->Left = prect->Top = 0;
        prect->SetWidth(GetGlyphWidth(glyphIndex));
        prect->SetHeight(GetGlyphHeight(glyphIndex));
    }
    else if (glyphIndex < AdvanceTable.GetSize())
    {
        GASSERT(glyphIndex != (UInt)-1 );
        const AdvanceEntry& e = AdvanceTable[glyphIndex];
        Float w = Float(e.Width)/20.f;
        if (w == 0)
            w = e.Advance;
        Float h = Float(e.Height)/20.f;
        prect->Left = Float(e.Left)/20.f;
        prect->Top  = Float(e.Top)/20.f;
        prect->SetWidth(w);
        prect->SetHeight(h);
    }
    else
    {
        // Bad glyph index.  Due to bad data file?
        // This is possible, if font doesn't have layout (for static text).
        // But AdvanceTable should be empty in this case.
        //GASSERT(AdvanceTable.GetSize() == 0);
        GFC_DEBUG_ERROR3((AdvanceTable.GetSize() != 0), 
            "Glyph index %d exceeds advance table (size = %d), font = %s", 
            glyphIndex, (int)AdvanceTable.GetSize(), GetName());

        prect->Left = prect->Top = 0;
        prect->SetWidth(0);
        prect->SetHeight(0);

        // so try to calculate the bounds
        if (glyphIndex < Glyphs.GetSize())
        {
            GFxShapeBase* s = Glyphs[glyphIndex];
            if (s)
            {
                GRectF bounds;
                s->ComputeBound(&bounds);
                if (bounds.IsNormal())
                {
                    Float left = bounds.Left;
                    Float top  = bounds.Top;
                    Float w = bounds.Width();
                    Float h = bounds.Height();
                    prect->Left    = left;
                    prect->Top     = top;
                    prect->SetWidth(w);
                    prect->SetHeight(h);
                }
            }
        }
    }
    return *prect;
}

// Return the adjustment in advance between the given two
// characters.  Normally this will be 0; i.E. the 
Float   GFxTextureFont::GetKerningAdjustment(UInt LastCode, UInt code) const
{
    Float   adjustment;
    KerningPair k;
    k.Char0 = (UInt16)LastCode;
    k.Char1 = (UInt16)code;
    if (KerningPairs.Get(k, &adjustment))
    {
        return adjustment;
    }
    return 0;
}
