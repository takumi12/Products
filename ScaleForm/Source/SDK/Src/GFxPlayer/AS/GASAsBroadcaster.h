/**********************************************************************

Filename    :   AS/GASAsBroadcaster.h
Content     :   Implementation of AsBroadcaster class
Created     :   October, 2006
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXASBROADCASTER_H
#define INC_GFXASBROADCASTER_H

#include "GFxAction.h"
#include "GFxCharacter.h"
#include "AS/GASObjectProto.h"


// ***** Declared Classes
class GASAsBroadcaster;
class GASAsBroadcasterProto;
class GASAsBroadcasterCtorFunction;

// ***** External Classes
class GASArrayObject;
class GASEnvironment;



class GASAsBroadcaster : public GASObject
{
protected:
    void commonInit (GASEnvironment* penv);
    
    GASAsBroadcaster (GASStringContext *psc, GASObject* pprototype)
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }
public:
    GASAsBroadcaster (GASEnvironment* penv);

    virtual ObjectType GetObjectType() const   { return Object_AsBroadcaster; }

    static bool Initialize(GASStringContext* psc, GASObjectInterface* pobj);
    static bool InitializeProto(GASStringContext* psc, GASObjectInterface* pobj);
    static bool InitializeInstance(GASStringContext* psc, GASObjectInterface* pobj);
    static bool AddListener(GASEnvironment* penv, GASObjectInterface* pthis, GASObjectInterface* plistener);
    static bool RemoveListener(GASEnvironment* penv, GASObjectInterface* pthis, GASObjectInterface* plistener);
    static bool BroadcastMessage(GASEnvironment* penv, 
                                 GASObjectInterface* pthis, 
                                 const GASString& eventName, 
                                 int nArgs, 
                                 int firstArgBottomIndex);
    
    class InvokeCallback
    {
    public:
        virtual ~InvokeCallback() {}
        virtual void Invoke(GASEnvironment* penv, GASObjectInterface* pthis, const GASFunctionRef& method) =0;
    };
    static bool BroadcastMessageWithCallback
        (GASEnvironment* penv, GASObjectInterface* pthis, const GASString& eventName, InvokeCallback* pcallback);
};

class GASAsBroadcasterProto : public GASPrototype<GASAsBroadcaster>
{
public:
    GASAsBroadcasterProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);

    static void AddListener(const GASFnCall& fn);
    static void BroadcastMessage(const GASFnCall& fn);
    static void RemoveListener(const GASFnCall& fn);
};

class GASAsBroadcasterCtorFunction : public GASCFunctionObject
{
    static const GASNameFunction StaticFunctionTable[];

    static void Initialize (const GASFnCall& fn);
public:
    GASAsBroadcasterCtorFunction (GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);

    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;    

    static GASFunctionRef Register(GASGlobalContext* pgc);
};


#endif // INC_GFXASBROADCASTER_H
