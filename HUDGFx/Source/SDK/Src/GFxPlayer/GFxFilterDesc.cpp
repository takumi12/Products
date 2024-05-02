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

#include "GFxStream.h"
#include "GFxFilterDesc.h"


// ***** Tag Loaders implementation


/*
    struct swf_filter_drop_shadow 
    {
        swf_filter_type     f_type;     // 0
        swf_rgba            f_rgba;
        signed long fixed   f_blur_horizontal;
        signed long fixed   f_blur_vertical;
        signed long fixed   f_radian_angle;
        signed long fixed   f_distance;
        signed short fixed  f_strength;
        unsigned            f_inner_shadow     : 1;
        unsigned            f_knock_out        : 1;
        unsigned            f_composite_source : 1;
        unsigned            f_reserved1        : 1;
        unsigned            f_reserved2        : 4;
    };


    struct swf_filter_blur 
    {
        swf_filter_type     f_type;     // 1 
        unsigned long fixed f_blur_horizontal;
        unsigned long fixed f_blur_vertical;
        unsigned            f_passes   : 5;
        unsigned            f_reserved : 3;
    };


    struct swf_filter_glow 
    {
        swf_filter_type     f_type;     // 2
        swf_rgba            f_rgba;
        signed long fixed   f_blur_horizontal;
        signed long fixed   f_blur_vertical;
        signed short fixed  f_strength;
        unsigned            f_inner_shadow     : 1;
        unsigned            f_knock_out        : 1;
        unsigned            f_composite_source : 1;
        unsigned            f_reserved1        : 1;
        unsigned            f_reserved2        : 4;
    };
*/


//------------------------------------------------------------------------
static inline UInt8 GFx_Fixed1616toFixed44U(UInt32 v)
{
    UInt vi = v >> 16;
    if (vi > 15) vi = 15;
    return UInt8((vi << 4) | ((v >> 12) & 0xF));
}

//------------------------------------------------------------------------
static inline UInt8 GFx_Fixed88toFixed44U(UInt16 v)
{
    UInt vi = v >> 8;
    if (vi > 15) vi = 15;
    return UInt8((vi << 4) | ((v >> 4) & 0xF));
}

//------------------------------------------------------------------------
static inline SInt16 GFx_Radians1616toAngle(SInt32 v)
{
    Float vf = Float(v) / 65536.0f;
    return SInt16(fmodf(vf * 1800.0f / 3.1415926535897932384626433832795f, 3600.0f));
}

//------------------------------------------------------------------------
static inline SInt16 GFx_Fixed1616toTwips(SInt32 v)
{
    Float vf = Float(v) / 65536.0f;
    return SInt16(vf * 20.0f);
}

//------------------------------------------------------------------------
static inline Float GFx_Fixed1616toFloat(UInt32 v)
{
    return (Float(v) / 65536.0f);
}

//------------------------------------------------------------------------
static inline Float GFx_Fixed88toFloat(UInt16 v)
{
    return (Float(v) / 256.0f);
}

//------------------------------------------------------------------------
static inline UInt8 GFx_ParseFilterFlags(UInt8 f)
{
    UInt8 flags = 0;
    if  (f & 0x40)       flags |= GFxFilterDesc::KnockOut;
    if ((f & 0x20) == 0) flags |= GFxFilterDesc::HideObject;
    if ((f & 0x0F) > 1)  flags |= GFxFilterDesc::FineBlur;
    return flags;
}

static inline UInt GFx_ParseShadowFlags(UInt8 f)
{
    UInt flags = 0;
    if  (f & 0x80)       flags |= GRenderer::Filter_Inner;
    if  (f & 0x40)       flags |= GRenderer::Filter_Knockout;
    if ((f & 0x20) == 0) flags |= GRenderer::Filter_HideObject;
    return flags;
}

// Helper function used to load filters.
//------------------------------------------------------------------------

template <class T>
UInt GFx_LoadFilters(T* ps, GFxFilterDesc* filters, UPInt filterssz)
{
    UByte filterCount = ps->ReadU8();
    GFxFilterDesc filterDesc;
    UInt numFilters = 0;

    // Flag to notify that output filter buffer is full
    // Continue reading file, but do not store the extra filters
    bool done = false;

    while(filterCount--)
    {
        GFxFilterDesc::FilterType filter = (GFxFilterDesc::FilterType)ps->ReadU8();
        UInt numBytes = 0;
	    UInt flags;

        filterDesc.Clear();
        filterDesc.Flags = UInt8(filter);

        switch(filter)
        {
        case GFxFilterDesc::Filter_DropShadow: // 23 bytes
            ps->ReadRgba(&filterDesc.Blur.Color);
            filterDesc.Blur.BlurX    = GFx_Fixed1616toFloat(ps->ReadU32());
            filterDesc.Blur.BlurY    = GFx_Fixed1616toFloat(ps->ReadU32());
            filterDesc.Angle         = GFx_Radians1616toAngle(ps->ReadU32());
            filterDesc.Distance      = GFx_Fixed1616toTwips(ps->ReadU32());
            filterDesc.Blur.Strength = GFx_Fixed88toFloat(ps->ReadU16());
	        flags = ps->ReadU8();
            filterDesc.Flags        |= GFx_ParseFilterFlags((UInt8)flags);
	        filterDesc.Blur.Passes   = flags & 0x1f;
            filterDesc.Blur.Mode     = GRenderer::Filter_Shadow | GFx_ParseShadowFlags((UInt8)flags);
            filterDesc.Blur.Offset   = filterDesc.GetShadowOffset();
            if (!done && filters && numFilters < GFxFilterDesc::MaxFilters) 
                filters[numFilters++] = filterDesc;
            break;

        case GFxFilterDesc::Filter_Blur: // 9 bytes
            filterDesc.Blur.Mode     = GRenderer::Filter_Blur;
            filterDesc.Blur.BlurX  = GFx_Fixed1616toFloat(ps->ReadU32());
            filterDesc.Blur.BlurY  = GFx_Fixed1616toFloat(ps->ReadU32());
	        filterDesc.Blur.Passes = ps->ReadU8() >> 3;
            filterDesc.Flags      |= (filterDesc.Blur.Passes > 1) ? GFxFilterDesc::FineBlur : 0;
            if (!done && filters && numFilters < GFxFilterDesc::MaxFilters) 
                filters[numFilters++] = filterDesc;
            break;

        case GFxFilterDesc::Filter_Glow: // 15 bytes
            ps->ReadRgba(&filterDesc.Blur.Color);
            filterDesc.Blur.BlurX    = GFx_Fixed1616toFloat(ps->ReadU32());
            filterDesc.Blur.BlurY    = GFx_Fixed1616toFloat(ps->ReadU32());
            filterDesc.Blur.Strength = GFx_Fixed88toFloat(ps->ReadU16());
	        flags = ps->ReadU8();
            filterDesc.Flags        |= GFx_ParseFilterFlags((UInt8)flags);
            filterDesc.Blur.Mode     = GRenderer::Filter_Shadow | GFx_ParseShadowFlags((UInt8)flags);
	        filterDesc.Blur.Passes   = flags & 0x1f;
            if (!done && filters && numFilters < GFxFilterDesc::MaxFilters) 
                filters[numFilters++] = filterDesc;
            break;

        case GFxFilterDesc::Filter_Bevel:
            ps->ReadRgba(&filterDesc.Blur.Color);
            ps->ReadRgba(&filterDesc.Blur.Color2);
            filterDesc.Blur.BlurX    = GFx_Fixed1616toFloat(ps->ReadU32());
            filterDesc.Blur.BlurY    = GFx_Fixed1616toFloat(ps->ReadU32());
            filterDesc.Angle         = GFx_Radians1616toAngle(ps->ReadU32());
            filterDesc.Distance      = GFx_Fixed1616toTwips(ps->ReadU32());
            filterDesc.Blur.Strength = GFx_Fixed88toFloat(ps->ReadU16());
            flags = ps->ReadU8();
            filterDesc.Flags        |= GFx_ParseFilterFlags((UInt8)flags);
            filterDesc.Blur.Passes   = flags & 0xf;
            filterDesc.Blur.Mode     = GRenderer::Filter_Highlight | GRenderer::Filter_Shadow | GFx_ParseShadowFlags((UInt8)flags);
            filterDesc.Blur.Offset   = filterDesc.GetShadowOffset();
            if (!done && filters && numFilters < GFxFilterDesc::MaxFilters) 
                filters[numFilters++] = filterDesc;
            break;

        case GFxFilterDesc::Filter_GradientGlow:
            numBytes = 19 + ps->ReadU8()*5; 
            break;

        case GFxFilterDesc::Filter_Convolution:
            {
                UInt cols = ps->ReadU8();
                UInt rows = ps->ReadU8();
                numBytes = 4 + 4 + 4*cols*rows + 4 + 1;
            }
            break;

        case GFxFilterDesc::Filter_AdjustColor:
            {
                const UInt Index[] = {0,1,2,3,16, 4,5,6,7,17, 8,9,10,11,18, 12,13,14,15,19};
                for (UInt i = 0; i < 20; i++)
                    filterDesc.ColorMatrix[Index[i]] = ps->ReadFloat();
                for (UInt i = 16; i < 20; i++)
                    filterDesc.ColorMatrix[i] *= 1.f/255.f;
            }
            if (!done && filters && numFilters < GFxFilterDesc::MaxFilters) 
                filters[numFilters++] = filterDesc;
            break;

        case GFxFilterDesc::Filter_GradientBevel:
            numBytes = 19 + ps->ReadU8()*5; 
            break;
        }

        // Skip filter or the rest of the filter
        while(numBytes--)
            ps->ReadU8();

        // Check if output buffer is full
        if (!done && numFilters == filterssz)
        {
            done = true;
            GFC_DEBUG_WARNING1(1, "Attempt to load more than %d filters!", filterssz);
        }
    }
    return numFilters;
}
// Force concrete definitions inside .cpp
template UInt GFx_LoadFilters(GFxStream* ps, GFxFilterDesc* filters, UPInt filterssz);
template UInt GFx_LoadFilters(GFxStreamContext* ps, GFxFilterDesc* filters, UPInt filterssz);


// Loads a filter desc into a text filter object
//------------------------------------------------------------------------
void GFxTextFilter::LoadFilterDesc(const GFxFilterDesc& filter)
{
    if ((filter.Flags & 0xF) == GFxFilterDesc::Filter_Blur)
    {
        BlurX        = FloatToFixed44(filter.Blur.BlurX);
        BlurY        = FloatToFixed44(filter.Blur.BlurY);
        BlurStrength = FloatToFixed44(filter.Blur.Strength);
        return;
    }
    else if (((filter.Flags & 0xF) == GFxFilterDesc::Filter_DropShadow ||
        (filter.Flags & 0xF) == GFxFilterDesc::Filter_Glow) && (ShadowColor == 0 || ShadowDistance == 0))
    {
        ShadowFlags    = filter.Flags & 0xF0;
        ShadowBlurX    = FloatToFixed44(filter.Blur.BlurX);
        ShadowBlurY    = FloatToFixed44(filter.Blur.BlurY);
        ShadowStrength = FloatToFixed44(filter.Blur.Strength);
        ShadowAlpha    = filter.Blur.Color.GetAlpha();
        ShadowAngle    = filter.Angle;
        ShadowDistance = filter.Distance;
        ShadowOffsetX  = 0;
        ShadowOffsetY  = 0;
        ShadowColor    = filter.Blur.Color;
        UpdateShadowOffset();
        return;
    }
    if ((filter.Flags & 0xF) == GFxFilterDesc::Filter_Glow)
    {
        GlowColor = filter.Blur.Color;
        GlowSize  = G_Max(ShadowBlurX, ShadowBlurY);
    }
}

