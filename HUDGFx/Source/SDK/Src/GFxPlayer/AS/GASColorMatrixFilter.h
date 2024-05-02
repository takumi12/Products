/**********************************************************************

Filename    :   GFxASColorMatrixFilter.h
Content     :   ColorMatrixFilter reference class for ActionScript 2.0
Created     :   12/10/2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GASColorMatrixFilter_H
#define INC_GASColorMatrixFilter_H

#include "GConfig.h"

#ifndef GFC_NO_FXPLAYER_AS_FILTERS

#include "AS/GASBitmapFilter.h"

// ***** Declared Classes
class GASColorMatrixFilterObject;
class GASColorMatrixFilterProto;
class GASColorMatrixFilterCtorFunction;


// 
// Actionscript ColorMatrixFilter object declaration
//

class GASColorMatrixFilterObject : public GASBitmapFilterObject
{
    friend class GASColorMatrixFilterProto;
    friend class GASColorMatrixFilterCtorFunction;

protected:

//     GASColorMatrixFilterObject();
//     GASColorMatrixFilterObject(const GASColorMatrixFilterObject& obj) : GASBitmapFilterObject(obj) {}
    GASColorMatrixFilterObject(GASStringContext *psc, GASObject* pprototype)
        : GASBitmapFilterObject(psc, pprototype)
    { 
    }

    virtual             ~GASColorMatrixFilterObject();

public:

    GASColorMatrixFilterObject(GASEnvironment* penv);

    virtual ObjectType  GetObjectType() const { return Object_ColorMatrixFilter; }

    virtual bool        SetMember(GASEnvironment* penv, 
        const GASString& name, const GASValue& val, 
        const GASPropFlags& flags = GASPropFlags());

    virtual bool        GetMember(GASEnvironment* penv, 
        const GASString& name, GASValue* val);
};


class GASColorMatrixFilterProto : public GASPrototype<GASColorMatrixFilterObject>
{
public:
    GASColorMatrixFilterProto(GASStringContext *psc, GASObject* prototype, 
        const GASFunctionRef& constructor);

    static const GASNameFunction FunctionTable[];

    //
    // Default BitmapFilter object functions
    //
    static void         Clone(const GASFnCall& fn);
};


class GASColorMatrixFilterCtorFunction : public GASCFunctionObject
{
public:
    GASColorMatrixFilterCtorFunction(GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);
    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;

    static GASFunctionRef Register(GASGlobalContext* pgc);
};


#endif  // GFC_NO_FXPLAYER_AS_FILTERS

#endif  // INC_GASColorMatrixFilter_H
