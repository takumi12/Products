/**********************************************************************

Filename    :   AS/GASPointObject.h
Content     :   flash.geom.Point reference class for ActionScript 2.0
Created     :   3/7/2007
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXPOINT_H
#define INC_GFXPOINT_H

// GFC_NO_FXPLAYER_AS_POINT disables Point class
#ifndef GFC_NO_FXPLAYER_AS_POINT

#include "AS/GASObject.h"

// ***** Declared Classes
class GASPointObject;
class GASPointProto;
class GASPointCtorFunction;

// Point property indices
enum GFxPoint_Property 
{
    GFxPoint_X, 
    GFxPoint_Y, 
    GFxPoint_NumProperties
};

// internal object
typedef GPoint<GASNumber> GASPoint;

// loader functions
void GFxObject_GetPointProperties(GASEnvironment *penv, GASObject *pobj, GASValue params[GFxPoint_NumProperties]);
void GFxObject_GetPointProperties(GASEnvironment *penv, GASObject *pobj, GASPoint &pt);

// ****************************************************************************
// GAS Point class
// 
class GASPointObject : public GASObject
{
    friend class GASPointProto;
protected:
    GASPointObject(GASStringContext *psc, GASObject* pprototype)
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }
public:
    GASPointObject(GASEnvironment* penv);
    GASPointObject(GASEnvironment* penv, GASPointObject* po);

    virtual ObjectType GetObjectType() const { return Object_Point; }

    // getters and setters
    void GetProperties(GASStringContext *psc, GASValue params[GFxPoint_NumProperties]);
    void SetProperties(GASStringContext *psc, const GASValue params[GFxPoint_NumProperties]); 
    void GetProperties(GASEnvironment *penv, GASPoint &pt);
    void SetProperties(GASEnvironment *penv, const GASPoint &pt);

    virtual bool GetMember(GASEnvironment* penv, const GASString& name, GASValue* val);
};

// ****************************************************************************
// GAS Point prototype class
//
class GASPointProto : public GASPrototype<GASPointObject>
{
public:
    GASPointProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);

    static const GASNameFunction FunctionTable[];

    static void Add(const GASFnCall& fn);
    static void Clone(const GASFnCall& fn);
    static void Equals(const GASFnCall& fn);
    static void Normalize(const GASFnCall& fn);
    static void Offset(const GASFnCall& fn);
    static void Subtract(const GASFnCall& fn);
    static void ToString(const GASFnCall& fn);
};

// ****************************************************************************
// GAS Point Constructor function class
//
class GASPointCtorFunction : public GASCFunctionObject
{
protected:
    static const GASNameFunction StaticFunctionTable[];

    static void Distance(const GASFnCall& fn);
    static void Interpolate(const GASFnCall& fn);
    static void Polar(const GASFnCall& fn); 

public:
    GASPointCtorFunction (GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);

    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;

    static GASFunctionRef Register(GASGlobalContext* pgc);
};

void GFxObject_GetPointProperties(GASEnvironment *penv, GASObject *pobj, GASValue params[GFxPoint_NumProperties]);

#endif  // GFC_NO_FXPLAYER_AS_POINT
#endif  // INC_GFXPOINT_H
