/**********************************************************************

Filename    :   GFxASBitmapFilter.cpp
Content     :   BitmapFilter reference class for ActionScript 2.0
Created     :   12/10/2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GConfig.h"

#ifndef GFC_NO_FXPLAYER_AS_FILTERS

#include "AS/GASBitmapFilter.h"

#include "AS/GASGlowFilter.h"
#include "AS/GASDropShadowFilter.h"
#include "AS/GASBlurFilter.h"
#include "AS/GASBevelFilter.h"
#include "AS/GASColorMatrixFilter.h"

#include "GFxAction.h"

//GASBitmapFilterObject::GASBitmapFilterObject(GFxFilterDesc::FilterType ft)
//{
//    Filter.Flags = (UInt8)ft;
//}

GASBitmapFilterObject::GASBitmapFilterObject(GASEnvironment* penv, GFxFilterDesc::FilterType ft, UInt blurMode)
: GASObject(penv)
{
    GUNUSED(penv);
    Filter.Flags = (UInt8)ft;
    Filter.Blur.Mode = blurMode;
}

GASBitmapFilterObject::~GASBitmapFilterObject()
{

}


// --------------------------------------------------------------------


GASBitmapFilterProto::GASBitmapFilterProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor) :
GASPrototype<GASBitmapFilterObject>(psc, prototype, constructor)
{
}


// --------------------------------------------------------------------


GASBitmapFilterCtorFunction::GASBitmapFilterCtorFunction(GASStringContext* psc)
:   GASCFunctionObject(psc, NULL)
{
    GUNUSED(psc);
}


GASFunctionRef GASBitmapFilterCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef ctor(*GHEAP_NEW(pgc->GetHeap()) GASBitmapFilterCtorFunction(&sc));
    GPtr<GASBitmapFilterProto> proto = 
        *GHEAP_NEW(pgc->GetHeap()) GASBitmapFilterProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_BitmapFilter, proto);
    pgc->FlashFiltersPackage->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_BitmapFilter), GASValue(ctor));
    return ctor;
}

GASBitmapFilterObject* GASBitmapFilterObject::CreateFromDesc(GASEnvironment* penv, const GFxFilterDesc& filter)
{
    GASBitmapFilterObject* pobj;
    GASBuiltinType type;
    if ((filter.Flags & 0xF) == GFxFilterDesc::Filter_Blur)
        type = GASBuiltin_BlurFilter;
    else if ((filter.Flags & 0xF) == GFxFilterDesc::Filter_DropShadow)
        type = GASBuiltin_DropShadowFilter;
    else if ((filter.Flags & 0xF) == GFxFilterDesc::Filter_Glow)
        type = GASBuiltin_GlowFilter;
    else if ((filter.Flags & 0xF) == GFxFilterDesc::Filter_Bevel)
        type = GASBuiltin_BevelFilter;
    else if ((filter.Flags & 0xF) == GFxFilterDesc::Filter_AdjustColor)
        type = GASBuiltin_ColorMatrixFilter;
    else
        return 0;

    pobj = static_cast<GASBitmapFilterObject*>(penv->OperatorNew(penv->GetGC()->FlashFiltersPackage,
        penv->GetBuiltin(type), 0));
    if (pobj)
        pobj->Filter = filter;
    return pobj;
}

#endif  // GFC_NO_FXPLAYER_AS_FILTERS
