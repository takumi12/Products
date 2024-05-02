/**********************************************************************

Filename    :   AS/GASStage.h
Content     :   Stage class implementation
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXSTAGE_H
#define INC_GFXSTAGE_H

#include "GConfig.h"
#ifndef GFC_NO_FXPLAYER_AS_STAGE
#include "GFxCharacter.h"
#include "AS/GASObject.h"
#include "AS/GASObjectProto.h"
#include "GFxPlayerImpl.h"


// ***** Declared Classes
class GASStageObject;
class GASStageProto;
class GASStageCtorFunction;

// ***** External Classes
class GASValue;


// ActionScript Stage objects.

class GASStageObject : public GASObject
{
    friend class GASStageProto;
protected:
    GASStageObject(GASStringContext *psc, GASObject* pprototype)
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }
public:
    GASStageObject(GASEnvironment* penv);

    virtual ObjectType          GetObjectType() const   { return Object_Stage; }
};

class GASStageProto : public GASPrototype<GASStageObject>
{
public:
    GASStageProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor);

};

class GASStageCtorFunction : public GASCFunctionObject
{
    static const GASNameFunction StaticFunctionTable[];

    static void TranslateToScreen(const GASFnCall& fn);

    GFxMovieRoot* pMovieRoot;

#ifndef GFC_NO_GC
    void Finalize_GC()
    {
        pMovieRoot = 0;
        GASCFunctionObject::Finalize_GC();
    }
#endif // GFC_NO_GC
    static GASValue CreateRectangleObject(GASEnvironment *penv, const GRectF& rect);
public:
    GASStageCtorFunction(GASStringContext *psc, GFxMovieRoot* movieRoot);

    bool    GetMember(GASEnvironment *penv, const GASString& name, GASValue* val);
    bool    GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val);
    bool    SetMember(GASEnvironment *penv, const GASString& name, const GASValue& val, const GASPropFlags& flags);
    bool    SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags);

    void    NotifyOnResize(GASEnvironment* penv);
    static void NotifyOnResize(const GASFnCall& fn);

    static void GlobalCtor(const GASFnCall& fn);
    virtual GASObject* CreateNewObject(GASEnvironment*) const { return 0; }

    static GASFunctionRef Register(GASGlobalContext* pgc);
};
#endif //#ifndef GFC_NO_FXPLAYER_AS_STAGE

#endif // INC_GFXSTAGE_H

