/**********************************************************************

Filename    :   GFxASBlurFilter.h
Content     :   BlurFilter reference class for ActionScript 2.0
Created     :   12/10/2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GASBlurFilter_H
#define INC_GASBlurFilter_H

#include "GConfig.h"

#ifndef GFC_NO_FXPLAYER_AS_FILTERS

#include "AS/GASBitmapFilter.h"

// ***** Declared Classes
class GASBlurFilterObject;
class GASBlurFilterProto;
class GASBlurFilterCtorFunction;


// 
// Actionscript BlurFilter object declaration
//

class GASBlurFilterObject : public GASBitmapFilterObject
{
    friend class GASBlurFilterProto;
    friend class GASBlurFilterCtorFunction;

protected:

//     GASBlurFilterObject();
//     GASBlurFilterObject(const GASBlurFilterObject& obj) : GASBitmapFilterObject(obj) {}
    GASBlurFilterObject(GASStringContext *psc, GASObject* pprototype)
        : GASBitmapFilterObject(psc, pprototype)
    { 
    }

    virtual             ~GASBlurFilterObject();

public:

    GASBlurFilterObject(GASEnvironment* penv);

    virtual ObjectType  GetObjectType() const { return Object_BlurFilter; }

    virtual bool        SetMember(GASEnvironment* penv, 
        const GASString& name, const GASValue& val, 
        const GASPropFlags& flags = GASPropFlags());

    virtual bool        GetMember(GASEnvironment* penv, 
        const GASString& name, GASValue* val);
};


class GASBlurFilterProto : public GASPrototype<GASBlurFilterObject>
{
public:
    GASBlurFilterProto(GASStringContext *psc, GASObject* prototype, 
        const GASFunctionRef& constructor);

    static const GASNameFunction FunctionTable[];

    //
    // Default BitmapFilter object functions
    //
    static void         Clone(const GASFnCall& fn);
};


class GASBlurFilterCtorFunction : public GASCFunctionObject
{
public:
    GASBlurFilterCtorFunction(GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);
    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;

    static GASFunctionRef Register(GASGlobalContext* pgc);
};


#endif  // GFC_NO_FXPLAYER_AS_FILTERS

#endif  // INC_GASBlurFilter_H
