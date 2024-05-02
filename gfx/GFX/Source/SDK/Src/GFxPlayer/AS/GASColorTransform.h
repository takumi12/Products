/**********************************************************************

Filename    :   AS/GASColorTransform.h
Content     :   flash.geom.Rectangle reference class for ActionScript 2.0
Created     :   3/19/2007
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#ifndef INC_GFXCOLORTRANSFORM_H
#define INC_GFXCOLORTRANSFORM_H

#include "AS/GASObject.h"
#include "AS/GASObjectProto.h"


// GFC_NO_FXPLAYER_AS_COLORTRANSFORM disables ColorTransform support
#ifndef GFC_NO_FXPLAYER_AS_COLORTRANSFORM


// **** Declared Classes
class GASColorTransformObject;
class GASColorTransformProto;

// **** External Classes
class GASValue;

// internal storage object
typedef GRenderer::Cxform GASColorTransform;

// ColorTransform property indices
enum GFxColorTransform_Property
{
    GFxColorTransform_RedMultiplier,
    GFxColorTransform_GreenMultiplier,
    GFxColorTransform_BlueMultiplier,
    GFxColorTransform_AlphaMultiplier,
    GFxColorTransform_RedOffset,
    GFxColorTransform_GreenOffset,
    GFxColorTransform_BlueOffset,
    GFxColorTransform_AlphaOffset,
    GFxColorTransform_NumProperties
};

// ****************************************************************************
// GAS ColorTransform class
//
class GASColorTransformObject : public GASObject
{
    friend class GASColorTransformProto;
    friend class GASColorTransformCtorFunction;

    GASColorTransform ColorTransform;
protected:
    GASColorTransformObject(GASStringContext *psc, GASObject* pprototype) 
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }
public:
    GASColorTransformObject(GASEnvironment* penv);

    virtual ObjectType GetObjectType() const { return Object_ColorTransform; }

    GASColorTransform* GetColorTransform() { return &ColorTransform; }
    void SetColorTransform(const GASColorTransform &ct)
    {
        ColorTransform = ct;
    }
    // getters and setters
    void GetProperties(GASStringContext *psc, GASValue params[GFxColorTransform_NumProperties]);
    void SetProperties(GASStringContext *psc, const GASValue params[GFxColorTransform_NumProperties]);

    virtual bool GetMember(GASEnvironment *penv, const GASString &name, GASValue* val);
    virtual bool SetMember(GASEnvironment *penv, const GASString &name, const GASValue &val,
                           const GASPropFlags& flags = GASPropFlags());
};

// ****************************************************************************
// GAS ColorTransform prototype class
//
class GASColorTransformProto : public GASPrototype<GASColorTransformObject>
{
public:
    GASColorTransformProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor);

    static const GASNameFunction FunctionTable[];

    static void Concat(const GASFnCall& fn);
    static void ToString(const GASFnCall& fn);

};

class GASColorTransformCtorFunction : public GASCFunctionObject
{
public:
    GASColorTransformCtorFunction(GASStringContext *psc) 
        : GASCFunctionObject(psc, GlobalCtor) {}

    static void GlobalCtor(const GASFnCall& fn);
    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;

    static GASFunctionRef Register(GASGlobalContext* pgc);
};

void GFxObject_GetColorTransformProperties(GASEnvironment *penv, GASObject *pobj,
                                           GASValue params[GFxColorTransform_NumProperties]);

#endif  //  GFC_NO_FXPLAYER_AS_COLORTRANSFORM
#endif  // INC_GFXCOLORTRANSFORM_H
