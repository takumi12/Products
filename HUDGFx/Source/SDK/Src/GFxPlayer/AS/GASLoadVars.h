/**********************************************************************

Filename    :   AS/GASLoadVars.h
Content     :   LoadVars reference class for ActionScript 2.0
Created     :   3/28/2007
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXLOADVARS_H
#define INC_GFXLOADVARS_H

#include "GConfig.h"
#include "AS/GASObject.h"
#ifndef GFC_NO_FXPLAYER_AS_LOADVARS
#include "GFxAction.h"
#include "AS/GASObjectProto.h"

// **** Declared Classes
class GASLoadVarsObject;
class GASLoadVarsProto;

// ****************************************************************************
// GAS LoadVars class
// 
class GASLoadVarsObject : public GASObject
{
    friend class GASLoadVarsProto;
    GASNumber BytesLoadedCurrent;
    GASNumber BytesLoadedTotal;
    
    void CommonInit()
    {
        BytesLoadedCurrent = -1;    // undefined
        BytesLoadedTotal = -1;      // undefined
    };


protected:
    GASLoadVarsObject(GASStringContext *psc, GASObject* pprototype)
        : GASObject (psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
        CommonInit();
    }
public:
    GASLoadVarsObject(GASEnvironment* penv);

    virtual ObjectType GetObjectType() const { return Object_LoadVars; }

    virtual bool SetMember(GASEnvironment *penv, const GASString &name, const GASValue &val, const GASPropFlags& flags = GASPropFlags());

    void NotifyOnData(GASEnvironment* penv, GASString& src);
    void NotifyOnHTTPStatus(GASEnvironment* penv, GASNumber httpStatus);
    void NotifyOnLoad(GASEnvironment* penv, bool success);

    void SetLoadedBytes(GASNumber bytes)
    {
        if (BytesLoadedTotal < 0) BytesLoadedTotal = 0; // set to 0 if undefined
        BytesLoadedCurrent = bytes;
        BytesLoadedTotal += bytes;
    }

};

// ****************************************************************************
// GAS LoadVars prototype class
//
class GASLoadVarsProto : public GASPrototype<GASLoadVarsObject>
{
public:
    GASLoadVarsProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);

    static const GASNameFunction FunctionTable[];

    static void AddRequestHeader(const GASFnCall& fn);
    static void Decode(const GASFnCall& fn);
    static void GetBytesLoaded(const GASFnCall& fn);
    static void GetBytesTotal(const GASFnCall& fn);
    static void Load(const GASFnCall& fn);
    static void Send(const GASFnCall& fn);
    static void SendAndLoad(const GASFnCall& fn);
    static void ToString(const GASFnCall& fn);

    static void DefaultOnData(const GASFnCall& fn);

    static bool LoadVariables(GASEnvironment *penv, GASObjectInterface* pchar, const GString& data);
};

class GASLoadVarsLoaderCtorFunction : public GASCFunctionObject
{
public:
    GASLoadVarsLoaderCtorFunction (GASStringContext *psc);

    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;

    static void GlobalCtor(const GASFnCall& fn);

    static GASFunctionRef Register(GASGlobalContext* pgc);
};
#else
class GASLoadVarsObject : public GASObject
{
public:
    GASLoadVarsObject (GASStringContext *psc) : GASObject(psc) {}
    void NotifyOnData(GASEnvironment* , GASString& ) {}
    void NotifyOnHTTPStatus(GASEnvironment* , GASNumber ) {}
    void NotifyOnLoad(GASEnvironment* , bool ) {}
    void SetLoadedBytes(GASNumber) {}
};
#endif //#ifndef GFC_NO_FXPLAYER_AS_LOADVARS

#endif  // INC_GFXLOADVARS_H
