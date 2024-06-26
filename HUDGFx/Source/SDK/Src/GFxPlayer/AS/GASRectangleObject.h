/**********************************************************************

Filename    :   AS/GASRectangleObject.h
Content     :   flash.geom.Rectangle reference class for ActionScript 2.0
Created     :   3/7/2007
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#ifndef INC_GFXRECTANGLE_H
#define INC_GFXRECTANGLE_H

#include "AS/GASObject.h"
#include "AS/GASObjectProto.h"

// GFC_NO_FXPLAYER_AS_RECTANGLE disables Rectangle class
#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE

// ***** Declared Classes
class GASRectangleObject;
class GASRectangleProto;

// ***** External Classes
class GASValue;

// Rectangle property indices
enum GFxRectangle_Property
{
    GFxRectangle_X,
    GFxRectangle_Y,
    GFxRectangle_Width,
    GFxRectangle_Height,
    GFxRectangle_NumProperties
};

// internal object
typedef GRect<GASNumber> GASRect;

// ****************************************************************************
// GAS Rectangle class
//
class GASRectangleObject : public GASObject
{
    friend class GASRectangleProto;
protected:
    GASRectangleObject(GASStringContext *psc, GASObject* pprototype)
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }
public:
    GASRectangleObject(GASEnvironment* penv);

    virtual ObjectType GetObjectType() const { return Object_Rectangle; }

    // getters and setters
    void GetProperties(GASStringContext *psc, GASValue params[GFxRectangle_NumProperties]);
    void SetProperties(GASStringContext* psc, const GASValue params[GFxRectangle_NumProperties]); 
    void GetProperties(GASEnvironment *penv, GASRect &r);
    void SetProperties(GASEnvironment *penv, const GASRect &r);

    virtual bool GetMember(GASEnvironment *penv, const GASString &name, GASValue* val);
    virtual bool SetMember(GASEnvironment *penv, const GASString &name, const GASValue &val, const GASPropFlags& flags = GASPropFlags());
};

// ****************************************************************************
// GAS Rectangle prototype class
//
class GASRectangleProto : public GASPrototype<GASRectangleObject>
{
public:
    GASRectangleProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor);

    static const GASNameFunction FunctionTable[];

    static void Clone(const GASFnCall& fn);
    static void Contains(const GASFnCall& fn);
    static void ContainsPoint(const GASFnCall& fn);
    static void ContainsRectangle(const GASFnCall& fn);
    static void Equals(const GASFnCall& fn);
    static void Inflate(const GASFnCall& fn);
    static void InflatePoint(const GASFnCall& fn);
    static void Intersection(const GASFnCall& fn);
    static void Intersects(const GASFnCall& fn);
    static void IsEmpty(const GASFnCall& fn);
    static void Offset(const GASFnCall& fn);
    static void OffsetPoint(const GASFnCall& fn);
    static void SetEmpty(const GASFnCall& fn);
    static void ToString(const GASFnCall& fn);
    static void Union(const GASFnCall& fn);
};

class GASRectangleCtorFunction : public GASCFunctionObject
{
public:
    GASRectangleCtorFunction(GASStringContext *psc) : GASCFunctionObject(psc, GlobalCtor) {}

    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;    

    static void GlobalCtor(const GASFnCall& fn);

    static GASFunctionRef Register(GASGlobalContext* pgc);
};


#endif  //  GFC_NO_FXPLAYER_AS_RECTANGLE
#endif  // INC_GFXRECTANGLE_H


