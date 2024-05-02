/**********************************************************************

Filename    :   GFxASBevelFilter.h
Content     :   BevelFilter reference class for ActionScript 2.0
Created     :   12/10/2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GASBevelFilter_H
#define INC_GASBevelFilter_H

#include "GConfig.h"

#ifndef GFC_NO_FXPLAYER_AS_FILTERS

#include "AS/GASBitmapFilter.h"

// ***** Declared Classes
class GASBevelFilterObject;
class GASBevelFilterProto;
class GASBevelFilterCtorFunction;


// 
// Actionscript BevelFilter object declaration
//

class GASBevelFilterObject : public GASBitmapFilterObject
{
    friend class GASBevelFilterProto;
    friend class GASBevelFilterCtorFunction;

protected:

//     GASBevelFilterObject();
//     GASBevelFilterObject(const GASBevelFilterObject& obj) : GASBitmapFilterObject(obj) {}
    GASBevelFilterObject(GASStringContext *psc, GASObject* pprototype)
        : GASBitmapFilterObject(psc, pprototype)
    { 
    }

    virtual             ~GASBevelFilterObject();

public:

    GASBevelFilterObject(GASEnvironment* penv);

    virtual ObjectType  GetObjectType() const { return Object_BevelFilter; }

    virtual bool        SetMember(GASEnvironment* penv, 
        const GASString& name, const GASValue& val, 
        const GASPropFlags& flags = GASPropFlags());

    virtual bool        GetMember(GASEnvironment* penv, 
        const GASString& name, GASValue* val);
};


class GASBevelFilterProto : public GASPrototype<GASBevelFilterObject>
{
public:
    GASBevelFilterProto(GASStringContext *psc, GASObject* prototype, 
        const GASFunctionRef& constructor);

    static const GASNameFunction FunctionTable[];

    //
    // Default BitmapFilter object functions
    //
    static void         Clone(const GASFnCall& fn);
};


class GASBevelFilterCtorFunction : public GASCFunctionObject
{
public:
    GASBevelFilterCtorFunction(GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);
    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;

    static GASFunctionRef Register(GASGlobalContext* pgc);
};


#endif  // GFC_NO_FXPLAYER_AS_FILTERS

#endif  // INC_GASBevelFilter_H
