/**********************************************************************

Filename    :   AS/GASNumberObject.h
Content     :   Number object functinality
Created     :   March 10, 2006
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXNUMBER_H
#define INC_GFXNUMBER_H

#include "GFxAction.h"
#include "AS/GASObjectProto.h"


// ***** Declared Classes
class GASNumberUtil;
class GASNumberObject;
class GASNumberProto;
class GASNumberCtorFunction;

class GASNumberObject : public GASObject
{
protected:
    GASNumber           Value;
    mutable GStringLH   StringValue;

    void CommonInit(GASEnvironment* penv);

    GASNumberObject(GASStringContext *psc, GASObject* pprototype) 
        : GASObject(psc), Value(0)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }
#ifndef GFC_NO_GC
protected:
    virtual void Finalize_GC()
    {
        StringValue.~GStringLH();
        GASObject::Finalize_GC();
    }
#endif //GFC_NO_GC
public:
    GASNumberObject(GASEnvironment* penv);
    GASNumberObject(GASEnvironment* penv, GASNumber val);

    virtual const char* GetTextValue(GASEnvironment* penv = 0) const;
    ObjectType          GetObjectType() const   { return Object_Number; }

    const char* ToString (int radix) const;

    virtual void        SetValue(GASEnvironment* penv, const GASValue&);
    virtual GASValue    GetValue() const;

    //
    // This method is used to invoke some methods for primitive number,
    // like it is possible to invoke toString (radix) or valueOf even for non-object Number.
    //
    static bool InvokePrimitiveMethod (const GASFnCall& fnCall, const GASString& methodName);
};

class GASNumberProto : public GASPrototype<GASNumberObject>
{
public:
    GASNumberProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);
};

class GASNumberCtorFunction : public GASCFunctionObject
{
public:
    GASNumberCtorFunction (GASStringContext *psc);

    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;

    static void GlobalCtor(const GASFnCall& fn);
    static GASFunctionRef Register(GASGlobalContext* pgc);
};

#endif

