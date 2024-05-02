/**********************************************************************

Filename    :   GFxAsBroadcaster.cpp
Content     :   Implementation of AsBroadcaster class
Created     :   October, 2006
Authors     :   Artyom Bolgar

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GRefCount.h"
#include "GFxLog.h"
#include "GFxASString.h"
#include "GFxAction.h"
#include "AS/GASArrayObject.h"
#include "AS/GASNumberObject.h"
#include "AS/GASAsBroadcaster.h"
#include "GUTF8Util.h"

GASAsBroadcaster::GASAsBroadcaster(GASEnvironment* penv)
: GASObject(penv)
{
    commonInit(penv);
}

void GASAsBroadcaster::commonInit (GASEnvironment* penv)
{    
    Set__proto__ (penv->GetSC(), penv->GetPrototype(GASBuiltin_AsBroadcaster));
}

//////////////////////////////////////////
//
void GASAsBroadcasterProto::AddListener(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
        return;

    GASSERT(fn.ThisPtr);

    GASObjectInterface* plistener = fn.Arg(0).ToObjectInterface(fn.Env);
    GASAsBroadcaster::AddListener(fn.Env, fn.ThisPtr, plistener);
    fn.Result->SetBool(true);
}

void GASAsBroadcasterProto::BroadcastMessage(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
        return;

    GASSERT(fn.ThisPtr);

    GASString eventName(fn.Arg(0).ToString(fn.Env));
    GASAsBroadcaster::BroadcastMessage(fn.Env, fn.ThisPtr, eventName, fn.NArgs - 1, fn.Env->GetTopIndex() - 4);
    fn.Result->SetUndefined();
}

void GASAsBroadcasterProto::RemoveListener(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
        return;

    GASSERT(fn.ThisPtr);

    GASObjectInterface* plistener = fn.Arg(0).ToObjectInterface(fn.Env);
    fn.Result->SetBool(GASAsBroadcaster::RemoveListener(fn.Env, fn.ThisPtr, plistener));
}

static const GASNameFunction GAS_AsBcFunctionTable[] = 
{
    { "addListener",        &GASAsBroadcasterProto::AddListener        },
    { "broadcastMessage",   &GASAsBroadcasterProto::BroadcastMessage   },
    { "removeListener",     &GASAsBroadcasterProto::RemoveListener     },
    { 0, 0 }
};

GASAsBroadcasterProto::GASAsBroadcasterProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASAsBroadcaster>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GAS_AsBcFunctionTable);
}

//////////////////
const GASNameFunction GASAsBroadcasterCtorFunction::StaticFunctionTable[] = 
{
    { "initialize", &GASAsBroadcasterCtorFunction::Initialize },
    { 0, 0 }
};

GASAsBroadcasterCtorFunction::GASAsBroadcasterCtorFunction(GASStringContext *psc) :
    GASCFunctionObject(psc, GlobalCtor)
{
    GASNameFunction::AddConstMembers(
        this, psc, StaticFunctionTable, 
        GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
}

void GASAsBroadcasterCtorFunction::Initialize (const GASFnCall& fn)
{
    if (fn.NArgs < 1)
        return;
    GASObjectInterface* ptr = fn.Arg(0).ToObjectInterface(fn.Env);
    fn.Result->SetUndefined();
    GASAsBroadcaster::Initialize(fn.Env->GetSC(), ptr);
}
//////////////////
bool GASAsBroadcaster::Initialize(GASStringContext* psc, GASObjectInterface* pobj)
{
    InitializeProto(psc, pobj);
    return InitializeInstance(psc, pobj);
}

bool GASAsBroadcaster::InitializeProto(GASStringContext* psc, GASObjectInterface* pobj)
{
    if (!pobj)
        return false;
    GASNameFunction::AddConstMembers(pobj, psc, GAS_AsBcFunctionTable, GASPropFlags::PropFlag_DontEnum);    
    return true;
}

bool GASAsBroadcaster::InitializeInstance(GASStringContext* psc, GASObjectInterface* pobj)
{
    if (!pobj)
        return false;

    GPtr<GASArrayObject> ao = *GHEAP_NEW(psc->GetHeap()) GASArrayObject(psc);
    pobj->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin__listeners), GASValue(ao), GASPropFlags::PropFlag_DontEnum);
    return true;
}

bool GASAsBroadcaster::AddListener(GASEnvironment* penv, GASObjectInterface* pthis, GASObjectInterface* plistener)
{
    if (!pthis || !plistener)
        return false;

    GASValue listenersVal;
    if (pthis->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin__listeners), &listenersVal))
    {
        GASObject* pobj = listenersVal.ToObject(penv);
        if (pobj && pobj->GetObjectType() == Object_Array)
        {
            GPtr<GASArrayObject> parrObj = static_cast<GASArrayObject*>(pobj);

            for (UInt i = 0, n = parrObj->GetSize(); i < n; i++)
            {
                GASValue* pelem = parrObj->GetElementPtr(i);
                if (pelem && pelem->ToObjectInterface(penv) == plistener)
                {
                    // Already in the list.
                    return false;
                }
            }
            GASValue val;
            val.SetAsObjectInterface(plistener);
            parrObj->PushBack(val);
        }
    }
    return true;
}

bool GASAsBroadcaster::RemoveListener(GASEnvironment* penv, GASObjectInterface* pthis, GASObjectInterface* plistener)
{
    if (!pthis || !plistener)
        return false;

    GASValue listenersVal;
    if (pthis->GetMemberRaw(penv->GetSC(),
        penv->GetBuiltin(GASBuiltin__listeners), &listenersVal))
    {
        GASObject* pobj = listenersVal.ToObject(penv);
        if (pobj && pobj->GetObjectType() == Object_Array)
        {
            GPtr<GASArrayObject> parrObj = static_cast<GASArrayObject*>(pobj);

            for (int i = (int)parrObj->GetSize() - 1; i >= 0; --i)
            {
                GASValue* pelem = parrObj->GetElementPtr(i);
                if (pelem && pelem->ToObjectInterface(penv) == plistener)
                {
                    // Found in the list.
                    parrObj->RemoveElements(i, 1);
                    return true;
                }
            }
        }
    }
    return false;
}

bool GASAsBroadcaster::BroadcastMessage
    (GASEnvironment* penv, GASObjectInterface* pthis, const GASString& eventName, int nArgs, int firstArgBottomIndex)
{
    if (!pthis)
        return false;
    GASSERT(penv);
    class LocalInvokeCallback : public InvokeCallback
    {
    public:
        int NArgs;
        int FirstArgBottomIndex;

        LocalInvokeCallback(int na, int fa) : NArgs(na), FirstArgBottomIndex(fa) {}

        virtual void Invoke(GASEnvironment* penv, GASObjectInterface* pthis, const GASFunctionRef& method)
        {
            GASValue result;
            method.Invoke(GASFnCall(&result, pthis, penv, NArgs, FirstArgBottomIndex));
        }
    } Callback(nArgs, firstArgBottomIndex);
    BroadcastMessageWithCallback(penv, pthis, eventName, &Callback);
    return true;
}

bool GASAsBroadcaster::BroadcastMessageWithCallback
(GASEnvironment* penv, GASObjectInterface* pthis, const GASString& eventName, InvokeCallback* pcallback)
{
    if (!pthis)
        return false;
    GASSERT(penv);

    GASValue listenersVal;
    if (pthis->GetMemberRaw(penv->GetSC(),
        penv->GetBuiltin(GASBuiltin__listeners), &listenersVal))
    {
        GASObject* pobj = listenersVal.ToObject(penv);
        if (pobj && pobj->GetObjectType() == Object_Array)
        {
            GPtr<GASArrayObject> porigArrObj = static_cast<GASArrayObject*>(pobj);
            if (porigArrObj->GetSize() > 0)
            {
                // AB: Seems like Flash does copy the _listeners array before broadcasting 
                // and iterate using the copy, since if one of the listeners removes the 
                // entry(ies) then broadcast still invokes the removed handler(s).
                GPtr<GASArrayObject> parrObj = *GHEAP_NEW(penv->GetHeap()) GASArrayObject(penv);
                parrObj->MakeDeepCopyFrom(penv->GetHeap(), *porigArrObj);

                for (UInt i = 0, n = parrObj->GetSize(); i < n; i++)
                {
                    GASValue*           pelem = parrObj->GetElementPtr(i);
                    GASObjectInterface* plistener;

                    if (pelem && (plistener = pelem->ToObjectInterface(penv)) != NULL)
                    {
                        GPtr<GASObject> objHolder;
                        GPtr<GFxASCharacter> charHolder;
                        if (pelem->IsCharacter())
                            charHolder = pelem->ToASCharacter(penv);
                        else
                            objHolder = pelem->ToObject(penv);

                        GASValue methodVal;
                        if (plistener->GetMemberRaw(penv->GetSC(),
                            eventName, &methodVal))
                        {
                            GASFunctionRef method = methodVal.ToFunction(penv);
                            if (!method.IsNull())
                                pcallback->Invoke(penv, plistener, method);
                        }
                    }
                }
            }
        }
    }
    return true;
}

void GASAsBroadcasterCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == GASObject::Object_AsBroadcaster && 
        !fn.ThisPtr->IsBuiltinPrototype())
    {
        GPtr<GASAsBroadcaster> ab = static_cast<GASAsBroadcaster*>(fn.ThisPtr);
        fn.Result->SetAsObject(ab.GetPtr());
    }
    else
        fn.Result->SetNull();
}

GASObject* GASAsBroadcasterCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{
    return GHEAP_NEW(penv->GetHeap()) GASAsBroadcaster(penv);
}


GASFunctionRef GASAsBroadcasterCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef  ctor(*GHEAP_NEW(pgc->GetHeap()) GASAsBroadcasterCtorFunction(&sc));
    GPtr<GASObject> proto = *GHEAP_NEW(pgc->GetHeap()) GASAsBroadcasterProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_AsBroadcaster, proto);
    pgc->pGlobal->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_AsBroadcaster), GASValue(ctor));
    return ctor;
}
