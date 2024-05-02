/**********************************************************************

Filename    :   GFxGlyphParam.h
Content     :   
Created     :   
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxGlyphParam_H
#define INC_GFxGlyphParam_H

#include <math.h>
#include "GTypes.h"
#include "GFxFilterDesc.h"

class GFxFontResource;

//------------------------------------------------------------------------
struct GFxGlyphParam
{
    GFxFontResource* pFont;
    UInt16           GlyphIndex;

    UInt8            FontSize;     // 255 is the max size for raster glyphs
    UInt8            Flags;        // See enum FlagsType
    UInt8            BlurX;        // fixed point 4.4, unsigned
    UInt8            BlurY;        // fixed point 4.4, unsigned
    UInt8            BlurStrength; // fixed point 4.4, unsigned
    UInt8            FlagsEx;      // Extended Flags. Bits 1...4 are outline width

    enum FlagsType
    {
        OptRead    = 0x01,
        AutoFit    = 0x02,
        Stretch    = 0x04,
        FauxBold   = 0x08,
        FauxItalic = 0x10,
        KnockOut   = 0x20,
        HideObject = 0x40,
        FineBlur   = 0x80,
    };

    enum FlagsExType
    {
        BitmapFont = 0x01
    };

    static Float GetMaxBlur()    { return  15.75f; }
    static Float GetMaxOutline() { return  15.0f; }

    void Clear()
    {
        pFont=0; GlyphIndex=0; FontSize=0; Flags=0;
        BlurX=0; BlurY=0; BlurStrength=16; FlagsEx=0;
    }

    UInt  GetFontSize()     const { return FontSize; }
    bool  IsOptRead()       const { return (Flags & OptRead) != 0; }
    bool  IsAutoFit()       const { return (Flags & AutoFit) != 0; }
    bool  IsBitmapFont()    const { return (FlagsEx & BitmapFont) != 0; }
    UInt  GetStretch()      const { return (Flags & Stretch) ? 3 : 1; }
    bool  IsFauxBold()      const { return (Flags & FauxBold) != 0; }
    bool  IsFauxItalic()    const { return (Flags & FauxItalic) != 0; }
    bool  IsKnockOut()      const { return (Flags & KnockOut) != 0; }
    bool  IsHiddenObject()  const { return (Flags & HideObject) != 0; }
    bool  IsFineBlur()      const { return (Flags & FineBlur) != 0; }
    Float GetBlurX()        const { return Float(BlurX) / 16.0f; }
    Float GetBlurY()        const { return Float(BlurY) / 16.0f; }
    Float GetBlurStrength() const { return Float(BlurStrength) / 16.0f; }
    UInt  GetOutline()      const { return UInt((FlagsEx >> 1) & 0xF); }

    void SetFontSize(UInt s)      { FontSize = UInt8(s); }
    void SetOptRead(bool f)       { if(f) Flags |= OptRead;    else Flags &= ~OptRead; }
    void SetAutoFit(bool f)       { if(f) Flags |= AutoFit;    else Flags &= ~AutoFit; }
    void SetStretch(bool f)       { if(f) Flags |= Stretch;    else Flags &= ~Stretch; }
    void SetFauxBold(bool f)      { if(f) Flags |= FauxBold;   else Flags &= ~FauxBold; }
    void SetFauxItalic(bool f)    { if(f) Flags |= FauxItalic; else Flags &= ~FauxItalic; }
    void SetKnockOut(bool f)      { if(f) Flags |= KnockOut;   else Flags &= ~KnockOut; }
    void SetHideObject(bool f)    { if(f) Flags |= HideObject; else Flags &= ~HideObject; }
    void SetFineBlur(bool f)      { if(f) Flags |= FineBlur;   else Flags &= ~FineBlur; }
    void SetBlurX(Float v)        { BlurX = UInt8(v * 16.0f + 0.5f); }
    void SetBlurY(Float v)        { BlurY = UInt8(v * 16.0f + 0.5f); }
    void SetBlurStrength(Float v) { BlurStrength = UInt8(v * 16.0f + 0.5f); }
    void SetOutline(UInt v)      { FlagsEx &= 0xE1; FlagsEx |= UInt8(v) << 1; }
    void SetBitmapFont(bool f)
    { 
        if(f) 
        {
            FlagsEx |= BitmapFont; 
            SetOptRead(true);
            SetAutoFit(false);
            SetStretch(false);
        }
        else 
        {
            FlagsEx &= ~BitmapFont;
        }
    }

    UPInt operator()(const GFxGlyphParam& key) const
    {
        return (((UPInt)key.pFont) >> 6) ^ (UPInt)key.pFont ^ 
                 (UPInt)key.GlyphIndex ^
                 (UPInt)key.FontSize ^
                 (UPInt)key.Flags ^
                 (UPInt)key.BlurX ^     
                ((UPInt)key.BlurY << 1) ^
                 (UPInt)key.BlurStrength ^
                 (UPInt)key.FlagsEx;
    }

    bool operator == (const GFxGlyphParam& key) const 
    { 
        return  pFont        == key.pFont && 
                GlyphIndex   == key.GlyphIndex &&
                FontSize     == key.FontSize &&
                Flags        == key.Flags &&
                BlurX        == key.BlurX &&     
                BlurY        == key.BlurY &&
                BlurStrength == key.BlurStrength &&
                FlagsEx      == key.FlagsEx;
    }
    
    bool operator != (const GFxGlyphParam& key) const
    {
        return !(*this == key);
    }

};




//------------------------------------------------------------------------
struct GFxTextFieldParam
{
    enum { ShadowDisabled = 0x01 };

    GFxGlyphParam TextParam;
    GFxGlyphParam ShadowParam;
    GColor        ShadowColor;
    SInt16        ShadowOffsetX; // In Twips
    SInt16        ShadowOffsetY; // In Twips
    GColor        GlowColor;
    UInt8         GlowSize;

    void Clear()
    {
        TextParam.Clear();
        ShadowParam.Clear();
        ShadowColor = 0; ShadowOffsetX = 0; ShadowOffsetY = 0;
        GlowColor = 0; GlowSize = 0;
    }

    GFxTextFieldParam() { Clear(); }

    bool operator == (const GFxTextFieldParam& key) const 
    { 
        return TextParam     == key.TextParam &&
               ShadowParam   == key.ShadowParam &&
               ShadowColor   == key.ShadowColor &&
               ShadowOffsetX == key.ShadowOffsetX &&
               ShadowOffsetY == key.ShadowOffsetY &&
               GlowColor     == key.GlowColor &&
               GlowSize      == key.GlowSize;
    }

    bool operator != (const GFxTextFieldParam& key) const
    {
        return !(*this == key);
    }

    void LoadFromTextFilter(const GFxTextFilter& filter)
    {
        TextParam.BlurX        = filter.BlurX;
        TextParam.BlurY        = filter.BlurY;
        TextParam.Flags        = GFxGlyphParam::FineBlur;
        TextParam.BlurStrength = filter.BlurStrength;
      //TextParam.Outline      = ...;

        if ((filter.ShadowFlags & ShadowDisabled) == 0)
        {
            ShadowParam.Flags        = UInt8(filter.ShadowFlags & ~ShadowDisabled);
            ShadowParam.BlurX        = filter.ShadowBlurX;
            ShadowParam.BlurY        = filter.ShadowBlurY;
            ShadowParam.BlurStrength = filter.ShadowStrength;
          //ShadowParam.Outline      = ...;
            ShadowColor              = filter.ShadowColor;
            ShadowOffsetX            = filter.ShadowOffsetX;
            ShadowOffsetY            = filter.ShadowOffsetY;

        }

        GlowColor = filter.GlowColor;
        GlowSize = filter.GlowSize;
    }

};



//------------------------------------------------------------------------
class GFxGlyphCacheVisitor
{
public:
    virtual ~GFxGlyphCacheVisitor() {}
    virtual void Visit(const GFxGlyphParam& param, const GRectF& uvRect, UInt textureIdx) = 0;
};


//------------------------------------------------------------------------
class GFxGlyphCacheEventHandler
{
public:
    virtual ~GFxGlyphCacheEventHandler() {}
    virtual void OnEvictGlyph(const GFxGlyphParam& param, const GRectF& uvRect, UInt textureIdx) = 0;
    virtual void OnUpdateGlyph(const GFxGlyphParam& param, const GRectF& uvRect, UInt textureIdx,
                               const UByte* img, UInt imgW, UInt imgH, UInt imgPitch) = 0;
    virtual void OnFailure(const GFxGlyphParam& param, UInt imgW, UInt imgH) = 0;
};




#endif
