/**********************************************************************

Filename    :   GFxFilterDesc.h
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

#ifndef INC_GFxFilterDesc_H
#define INC_GFxFilterDesc_H

#include "GTypes.h"

#include "GFxPlayerStats.h"
#include "GFxStreamContext.h"

class GFxStream;
struct GFxTextFilter;

//------------------------------------------------------------------------
struct GFxFilterDesc
{
    enum { MaxFilters = 4 };

    enum FilterType
    {
        Filter_DropShadow   = 0,
        Filter_Blur         = 1,
        Filter_Glow         = 2,
        Filter_Bevel        = 3,
        Filter_GradientGlow = 4,
        Filter_Convolution  = 5,
        Filter_AdjustColor  = 6,
        Filter_GradientBevel= 7,
    };

    enum FlagsType
    {
        KnockOut   = 0x20,
        HideObject = 0x40,
        FineBlur   = 0x80
    };

    UInt8                       Flags;      // 0..3 - FilterType, 4..7 - Flags (GFxGlyphParam::Flags)
    SInt16                      Angle;      // In 1/10 degree
    SInt16                      Distance;   // In Twips
    GRenderer::BlurFilterParams Blur;
    Float                       ColorMatrix[20];

    void Clear()
    {
        SetIdentity();
        Flags = 0;
        Blur.Mode = 0;
        Blur.BlurX = Blur.BlurY = 0;
        Blur.Strength = 1;
	    Blur.Passes = 0;
        Blur.Offset.x = Blur.Offset.y = 0;
        Angle = 0;
        Distance = 0;
        Blur.Color = 0;
    }

    void SetIdentity()
    {
        Flags = Filter_AdjustColor;
        memset(ColorMatrix, 0, sizeof(Float)*20);
        for (UInt i = 0; i < 4; i++)
            ColorMatrix[i*5] = 1;
    }

    // in pixels
    GPointF GetShadowOffset() const
    {
        Float a = Float(Angle) * 3.1415926535897932384626433832795f / 1800.0f;
        return GPointF ((cosf(a) * Distance * 0.05f),
			(sinf(a) * Distance * 0.05f));
    }

    bool UsesBlurFilter() const
    {
        return ((Flags & 0xf) <= 4) && (Blur.BlurX > 1 || Blur.BlurY > 1);
    }
    bool UsesRenderTarget() const
    {
        return 1;
    }
    bool CheckCaps(UInt32 renderCaps, GRenderer* pRenderer) const
    {
	    if (UsesBlurFilter())
	    {
	        if (!(renderCaps & GRenderer::Cap_Filter_Blurs))
    		    return 0;
	        return (pRenderer->CheckFilterSupport(Blur) & GRenderer::FilterSupport_Ok) != 0;
	    }
	    else if ((Flags & 0xf) == Filter_AdjustColor)
	        return (renderCaps & GRenderer::Cap_Filter_ColorMatrix) != 0;
	    else
	        return 0;
    }

    static bool UsesRenderTarget(const GArray<GFxFilterDesc>& Filters)
    {
        for (UInt i = 0; i < Filters.GetSize(); i++)
            if (Filters[i].UsesRenderTarget())
                return true;
        return false;
    }
};


//------------------------------------------------------------------------
struct GFxTextFilter : public GRefCountBaseNTS<GFxTextFilter, GFxStatMV_Text_Mem>
{
    UInt8                           BlurX;            // Fixed point 4.4, unsigned
    UInt8                           BlurY;            // Fixed point 4.4, unsigned
    UInt8                           BlurStrength;     // Fixed point 4.4, unsigned
    UInt8                           ShadowFlags;      // Corresponds to GFxGlyphParam::Flags
    UInt8                           ShadowBlurX;      // Fixed point 4.4, unsigned
    UInt8                           ShadowBlurY;      // Fixed point 4.4, unsigned
    UInt8                           ShadowStrength;   // Fixed point 4.4, unsigned
    UInt8                           ShadowAlpha;
    UInt8                           GlowSize;         // Fixed point 4.4, unsigned
    SInt16                          ShadowAngle;      // In 1/10 degree
    SInt16                          ShadowDistance;   // In Twips
    SInt16                          ShadowOffsetX;    // In Twips, default 57 twips (cos(45) * 4 * 20)
    SInt16                          ShadowOffsetY;    // In Twips, default 57 twips (sin(45) * 4 * 20)
    GColor                          ShadowColor;      // Color with alpha
    GColor                          GlowColor;

    GFxTextFilter()
    {
        SetDefaultShadow();
    }

    void SetDefaultShadow()
    {
        BlurX = 0;
        BlurY = 0;
        BlurStrength   = 1 << 4;
        ShadowFlags    = GFxFilterDesc::FineBlur;
        ShadowBlurX    = 4 << 4;
        ShadowBlurY    = 4 << 4;
        ShadowStrength = 1 << 4;
        ShadowAlpha    = 255;
        ShadowAngle    = 450;
        ShadowDistance = 4*20;
        ShadowOffsetX  = 57;  // In Twips, default 57 twips (cos(45) * 4 * 20)
        ShadowOffsetY  = 57;  // In Twips, default 57 twips (sin(45) * 4 * 20)
        ShadowColor    = 0;   // Color with alpha
        GlowColor      = 0;
        GlowSize       = 0;
    }

    static UInt8 FloatToFixed44(Float v) { return UInt8(G_Clamp(UInt(v * 16.0f + 0.5f), UInt(0), UInt(255))); }
    static Float Fixed44toFloat(UInt8 v) { return Float(v) / 16.0f; }

    void UpdateShadowOffset()
    {
        Float a = Float(ShadowAngle) * 3.1415926535897932384626433832795f / 1800.0f;
        ShadowOffsetX = SInt16(cosf(a) * ShadowDistance);
        ShadowOffsetY = SInt16(sinf(a) * ShadowDistance);
    }

    void LoadFilterDesc(const GFxFilterDesc& filter);
};

// Helper function used to load filters.
template <class T> 
UInt GFx_LoadFilters(T* pin, GFxFilterDesc* filters, UPInt filterssz);

#endif
