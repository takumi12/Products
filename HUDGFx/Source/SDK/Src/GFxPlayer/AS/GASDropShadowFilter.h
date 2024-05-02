/**********************************************************************

Filename    :   GFxASDropShadowFilter.h
Content     :   DropShadowFilter reference class for ActionScript 2.0
Created     :   12/10/2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GASDropShadowFilter_H
#define INC_GASDropShadowFilter_H

#include "GConfig.h"

#ifndef GFC_NO_FXPLAYER_AS_FILTERS

#include "AS/GASBitmapFilter.h"

// ***** Declared Classes
class GASDropShadowFilterObject;
class GASDropShadowFilterProto;
class GASDropShadowFilterCtorFunction;


// 
// Actionscript DropShadowFilter object declaration
//

class GASDropShadowFilterObject : public GASBitmapFilterObject
{
    friend class GASDropShadowFilterProto;
    friend class GASDropShadowFilterCtorFunction;

protected:

//     GASDropShadowFilterObject();
//     GASDropShadowFilterObject(const GASDropShadowFilterObject& obj) : GASBitmapFilterObject(obj) {}
    GASDropShadowFilterObject(GASStringContext *psc, GASObject* pprototype)
        : GASBitmapFilterObject(psc, pprototype)
    { 
        Filter.Blur.Mode = GRenderer::Filter_Shadow;
    }

    virtual             ~GASDropShadowFilterObject();

public:

    GASDropShadowFilterObject(GASEnvironment* penv);

    virtual ObjectType  GetObjectType() const { return Object_DropShadowFilter; }

    virtual bool        SetMember(GASEnvironment* penv, 
        const GASString& name, const GASValue& val, 
        const GASPropFlags& flags = GASPropFlags());

    virtual bool        GetMember(GASEnvironment* penv, 
        const GASString& name, GASValue* val);
};


class GASDropShadowFilterProto : public GASPrototype<GASDropShadowFilterObject>
{
public:
    GASDropShadowFilterProto(GASStringContext *psc, GASObject* prototype, 
        const GASFunctionRef& constructor);

    static const GASNameFunction FunctionTable[];

    //
    // Default BitmapFilter object functions
    //
    static void         Clone(const GASFnCall& fn);
};


class GASDropShadowFilterCtorFunction : public GASCFunctionObject
{
public:
    GASDropShadowFilterCtorFunction(GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);
    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;

    static GASFunctionRef Register(GASGlobalContext* pgc);
};


#endif  // GFC_NO_FXPLAYER_AS_FILTERS

#endif  // INC_GASDropShadowFilter_H
