/**********************************************************************

Filename    :   GFxASBitmapFilter.h
Content     :   BitmapFilter reference class for ActionScript 2.0
Created     :   12/10/2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GASBitmapFilter_H
#define INC_GASBitmapFilter_H

#include "GConfig.h"

#ifndef GFC_NO_FXPLAYER_AS_FILTERS

#include "AS/GASObject.h"
#include "AS/GASObjectProto.h"

#include "GFxFilterDesc.h"

// ***** Declared Classes
class GASBitmapFilterObject;
class GASBitmapFilterProto;
class GASBitmapFilterCtorFunction;

// 
// Actionscript BitmapFilter object declaration
//

class GASBitmapFilterObject : public GASObject
{
    friend class GASBitmapFilterProto;
    friend class GASBitmapFilterCtorFunction;
private:
    GASBitmapFilterObject(const GASBitmapFilterObject&) : GASObject(GetCollector()) {} 
protected:

    GFxFilterDesc   Filter;

//?    GASBitmapFilterObject(GFxFilterDesc::FilterType ft); //?
//?    GASBitmapFilterObject(const GASBitmapFilterObject& obj) 
//?        : GASObject(obj->GetGC()), Filter(obj.Filter) {} //?AB: do we need it? no copy ctor for GASObject
    GASBitmapFilterObject(GASStringContext *psc, GASObject* pprototype)
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }

    virtual         ~GASBitmapFilterObject();

public:

    GASBitmapFilterObject(GASEnvironment* penv, GFxFilterDesc::FilterType ft, UInt blurMode = 0);

    virtual ObjectType      GetObjectType() const { return Object_BitmapFilter; }

    GINLINE void                    SetFilter(const GFxFilterDesc& filter) { Filter = filter; }
    GINLINE const GFxFilterDesc&    GetFilter() const { return Filter; }

    // The number of passes for blur and shadows (quality)
    GINLINE void            SetPasses(UInt d)     { Filter.Blur.Passes = d; }
    // The offset distance for the shadow, in pixels.
    GINLINE void            SetDistance(SInt16 d)   { Filter.Distance = d * 20; Filter.Blur.Offset = Filter.GetShadowOffset(); }
    // The angle of the shadow. Valid values are 0 to 360? (floating point).
    GINLINE void            SetAngle(SInt16 a)      { Filter.Angle = a * 10; Filter.Blur.Offset = Filter.GetShadowOffset(); }
    // The color of the shadow. Valid values are in hexadecimal format 0xRRGGBB
    GINLINE void            SetColor(UInt32 c)      { Filter.Blur.Color.SetColor(c, Filter.Blur.Color.GetAlpha()); }
    // The alpha transparency value for the shadow color. Valid values are 0 to 1.
    GINLINE void            SetAlpha(Float a)       { Filter.Blur.Color.SetAlphaFloat(a); }
    // The color of the highlight (for bevel). Valid values are in hexadecimal format 0xRRGGBB
    GINLINE void            SetColor2(UInt32 c)      { Filter.Blur.Color2.SetColor(c, Filter.Blur.Color2.GetAlpha()); }
    // The alpha transparency value for the highlight color. Valid values are 0 to 1.
    GINLINE void            SetAlpha2(Float a)       { Filter.Blur.Color2.SetAlphaFloat(a); }
    // The amount of horizontal blur. Valid values are 0 to 255 (floating point).
    GINLINE void            SetBlurX(Float b)       { Filter.Blur.BlurX = b; }
    // The amount of vertical blur. Valid values are 0 to 255 (floating point).
    GINLINE void            SetBlurY(Float b)       { Filter.Blur.BlurY = b; }
    // The strength of the imprint or spread. The higher the value, 
    // the more color is imprinted and the stronger the contrast between 
    // the shadow and the background. Valid values are from 0 to 255.
    GINLINE void            SetStrength(Float s)    { Filter.Blur.Strength = s; }
    // Applies a knockout effect (true), which effectively makes the 
    // object's fill transparent and reveals the background color of the document.
    GINLINE void            SetKnockOut(bool k) 
    { 
        if (k) 
        {
            Filter.Flags |= GFxFilterDesc::KnockOut;  
            Filter.Blur.Mode |= GRenderer::Filter_Knockout;
        }
        else 
        {
            Filter.Flags &= ~GFxFilterDesc::KnockOut;
            Filter.Blur.Mode &= ~GRenderer::Filter_Knockout;
        }
    }
    // Indicates whether or not the object is hidden. The value true indicates that 
    // the object itself is not drawn; only the shadow is visible.
    GINLINE void            SetHideObject(bool h) 
    { 
        if (h) 
        {
            Filter.Flags |= GFxFilterDesc::HideObject; 
            Filter.Blur.Mode |= GRenderer::Filter_HideObject;
        }
        else 
        {
            Filter.Flags &= ~GFxFilterDesc::HideObject;
            Filter.Blur.Mode &= ~GRenderer::Filter_HideObject;
        }
    }
    GINLINE void            SetInnerShadow(bool i) 
    { 
        if (i) 
            Filter.Blur.Mode |= GRenderer::Filter_Inner;
        else 
            Filter.Blur.Mode &= ~GRenderer::Filter_Inner;
    }
    GINLINE void            SetBlurFlags(UInt f)
    {
        Filter.Blur.Mode = f;
    }

    GINLINE UInt            GetPasses()     { return Filter.Blur.Passes; }
    GINLINE SInt16          GetDistance()   { return Filter.Distance / 20; }
    GINLINE SInt16          GetAngle()      { return Filter.Angle / 10; }
    GINLINE UInt32          GetColor()      { return Filter.Blur.Color.ToColor32() & 0x00FFFFFF; }
    GINLINE Float           GetAlpha()      { Float a; Filter.Blur.Color.GetAlphaFloat(&a); return a; }
    GINLINE UInt32          GetColor2()     { return Filter.Blur.Color2.ToColor32() & 0x00FFFFFF; }
    GINLINE Float           GetAlpha2()     { Float a; Filter.Blur.Color2.GetAlphaFloat(&a); return a; }
    GINLINE Float           GetBlurX()      { return Filter.Blur.BlurX; }
    GINLINE Float           GetBlurY()      { return Filter.Blur.BlurY; }
    GINLINE Float           GetStrength()   { return Filter.Blur.Strength; }
    GINLINE bool            IsKnockOut()    { return (Filter.Flags & GFxFilterDesc::KnockOut) != 0; }
    GINLINE bool            IsHideObject()  { return (Filter.Flags & GFxFilterDesc::HideObject) != 0; }
    GINLINE bool            IsInnerShadow() { return (Filter.Blur.Mode & GRenderer::Filter_Inner) != 0; }
    GINLINE UInt            GetBlurFlags()  { return Filter.Blur.Mode; }

    static GASBitmapFilterObject* CreateFromDesc(GASEnvironment* penv, const GFxFilterDesc& filter);
};


class GASBitmapFilterProto : public GASPrototype<GASBitmapFilterObject>
{
public:
    GASBitmapFilterProto(GASStringContext *psc, GASObject* prototype, 
        const GASFunctionRef& constructor);
};


class GASBitmapFilterCtorFunction : public GASCFunctionObject
{
public:
    GASBitmapFilterCtorFunction(GASStringContext *psc);

    //static void GlobalCtor(const GASFnCall& fn);

    static GASFunctionRef Register(GASGlobalContext* pgc);
};


#endif  // GFC_NO_FXPLAYER_AS_FILTERS

#endif  // INC_GASBitmapFilter_H
