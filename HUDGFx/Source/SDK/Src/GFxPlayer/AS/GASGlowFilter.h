/**********************************************************************

Filename    :   GFxASGlowFilter.h
Content     :   GlowFilter reference class for ActionScript 2.0
Created     :   12/10/2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GASGlowFilter_H
#define INC_GASGlowFilter_H

#include "GConfig.h"

#ifndef GFC_NO_FXPLAYER_AS_FILTERS

#include "AS/GASBitmapFilter.h"

// ***** Declared Classes
class GASGlowFilterObject;
class GASGlowFilterProto;
class GASGlowFilterCtorFunction;


// 
// Actionscript GlowFilter object declaration
//

class GASGlowFilterObject : public GASBitmapFilterObject
{
    friend class GASGlowFilterProto;
    friend class GASGlowFilterCtorFunction;

protected:

//     GASGlowFilterObject();
//     GASGlowFilterObject(const GASGlowFilterObject& obj) : GASBitmapFilterObject(obj) {}
    GASGlowFilterObject(GASStringContext *psc, GASObject* pprototype)
        : GASBitmapFilterObject(psc, pprototype)
    { 
    }

    virtual             ~GASGlowFilterObject();

public:

    GASGlowFilterObject(GASEnvironment* penv);

    virtual ObjectType  GetObjectType() const { return Object_GlowFilter; }

    virtual bool        SetMember(GASEnvironment* penv, 
        const GASString& name, const GASValue& val, 
        const GASPropFlags& flags = GASPropFlags());

    virtual bool        GetMember(GASEnvironment* penv, 
        const GASString& name, GASValue* val);
};


class GASGlowFilterProto : public GASPrototype<GASGlowFilterObject>
{
public:
    GASGlowFilterProto(GASStringContext *psc, GASObject* prototype, 
        const GASFunctionRef& constructor);

    static const GASNameFunction FunctionTable[];

    //
    // Default BitmapFilter object functions
    //
    static void         Clone(const GASFnCall& fn);
};


class GASGlowFilterCtorFunction : public GASCFunctionObject
{
public:
    GASGlowFilterCtorFunction(GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);
    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;

    static GASFunctionRef Register(GASGlobalContext* pgc);
};


#endif  // GFC_NO_FXPLAYER_AS_FILTERS

#endif  // INC_GASGlowFilter_H
