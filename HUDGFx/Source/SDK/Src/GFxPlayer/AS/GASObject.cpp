/**********************************************************************

Filename    :   GFxObject.cpp
Content     :   ActionScript Object implementation classes
Created     :   
Authors     :   Artyom Bolgar

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxPlayer.h"
#include "GFxAction.h"
#include "GFxSprite.h"
#include "GFxAmpServer.h"
#include "GASAsFunctionObject.h"
#include "AMP/GFxAmpViewStats.h"

#include "GFxPlayerImpl.h"

// **** Object Interface implementation

GASObjectInterface::GASObjectInterface()
#ifndef GFC_NO_FXPLAYER_AS_USERDATA
: pUserDataHolder(0)
#endif  // GFC_NO_FXPLAYER_AS_USERDATA
{

}

GASObjectInterface::~GASObjectInterface()
{
#ifndef GFC_NO_FXPLAYER_AS_USERDATA
    if (pUserDataHolder)
    {
        pUserDataHolder->NotifyDestroy(this);
        delete pUserDataHolder;
    }
#endif  // GFC_NO_FXPLAYER_AS_USERDATA
}

GFxASCharacter* GASObjectInterface::ToASCharacter()
{
    return IsASCharacter() ? static_cast<GFxASCharacter*>(this) : 0;
}

GASObject* GASObjectInterface::ToASObject()
{
    return IsASObject() ? static_cast<GASObject*>(this) : 0;
}

GFxSprite*      GASObjectInterface::ToSprite()
{
    return IsSprite() ? static_cast<GFxSprite*>(this) : 0;
}

const GFxASCharacter* GASObjectInterface::ToASCharacter() const 
{
    return IsASCharacter() ? static_cast<const GFxASCharacter*>(this) : 0;
}

const GASObject* GASObjectInterface::ToASObject() const
{
    return IsASObject() ? static_cast<const GASObject*>(this) : 0;
}


const GFxSprite*        GASObjectInterface::ToSprite() const 
{
    return IsSprite() ? static_cast<const GFxSprite*>(this) : 0;
}

GASFunctionRef  GASObjectInterface::ToFunction()
{
    return 0;
}

GASObject* GASObjectInterface::FindOwner(GASStringContext *psc, const GASString& name)
{
    for (GASObjectInterface* proto = this; proto != 0; proto = proto->Get__proto__())
    {
        if (proto->HasMember(psc, name, false)) 
            return static_cast<GASObject*>(proto);
    }
    return NULL;
}

#ifndef GFC_NO_FXPLAYER_AS_USERDATA

void    GASObjectInterface::SetUserData(GFxMovieView* pmovieView, GFxASUserData* puserData, bool isdobj)
{
    if (pUserDataHolder) { delete pUserDataHolder; }
    pUserDataHolder = GHEAP_AUTO_NEW(this) UserDataHolder(pmovieView, puserData);
    if (puserData)
    {
        GFxMovieRoot* proot = static_cast<GFxMovieRoot*>(pmovieView);
        if (isdobj)
        {
            GFxASCharacter* aschar = ToASCharacter();
            puserData->SetLastObjectValue(proot->pObjectInterface, aschar->GetCharacterHandle(), isdobj);
        }
        else
            puserData->SetLastObjectValue(proot->pObjectInterface, this, isdobj);
    }
}

#endif  // GFC_NO_FXPLAYER_AS_USERDATA



//
// ***** GASObject Implementation
//
GASObject::GASObject(GASRefCountCollector *pcc)
: GASRefCountBase<GASObject>(pcc), pWatchpoints(0), ArePropertiesSet(false)
{
    // NOTE: psc is potentially NULL here.
    Init();
    pProto = 0;
}

GASObject::GASObject(GASStringContext *psc) 
: GASRefCountBase<GASObject>(psc->GetGC()), pWatchpoints(0), ArePropertiesSet(false)
{
    // NOTE: psc is potentially NULL here.
    GUNUSED(psc);
    Init();
    pProto = 0;//*GHEAP_NEW() GASObjectProto ();
}

GASObject::GASObject(GASStringContext *psc, GASObject* proto) 
: GASRefCountBase<GASObject>(psc->GetGC()), pWatchpoints(0), ArePropertiesSet(false)
{
    Init();
    Set__proto__(psc, proto);
}

GASObject::GASObject (GASEnvironment* penv) 
: GASRefCountBase<GASObject>(penv->GetCollector()), pWatchpoints(0), ArePropertiesSet(false)
{
    Init();    
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Object));
}

GASObject::~GASObject()
{ 
    if (pWatchpoints)
        delete pWatchpoints;
}

void GASObject::Init()
{
    IsListenerSet = false; // See comments in GFxValue.cpp, CheckForListenersMemLeak.
}

#ifndef GFC_NO_GC
template <class Functor>
void GASObject::ForEachChild_GC(Collector* prcc) const
{
    MemberHash::ConstIterator it = Members.Begin();
    while(it != Members.End())
    {   
        const GASValue& value = it->Second.GetMemberValue();
        value.template ForEachChild_GC<Functor>(prcc);
        ++it;
    }
    ResolveHandler.template ForEachChild_GC<Functor>(prcc);
    if (pWatchpoints)
    {
        WatchpointHash::ConstIterator it = pWatchpoints->Begin();
        while(it != pWatchpoints->End())
        {   
            const Watchpoint& value = it->Second;
            value.template ForEachChild_GC<Functor>(prcc);
            ++it;
        }
    }
    if (pProto)
    {
        Functor::Call(prcc, pProto);
    }
}
GFC_GC_PREGEN_FOR_EACH_CHILD(GASObject)

void GASObject::ExecuteForEachChild_GC(Collector* prcc, OperationGC operation) const
{
    GASRefCountBaseType::CallForEachChild<GASObject>(prcc, operation);
}

void GASObject::Finalize_GC()
{
    Members.~MemberHash();
    ResolveHandler.Finalize_GC();
    if (pWatchpoints)
    {
        pWatchpoints->~WatchpointHash();
        GFREE(pWatchpoints);
    }

#ifndef GFC_NO_FXPLAYER_AS_USERDATA
    if (pUserDataHolder)
    {
        pUserDataHolder->NotifyDestroy(this);
        delete pUserDataHolder;
    }
#endif  // GFC_NO_FXPLAYER_AS_USERDATA
}
#endif //GFC_NO_GC

void GASObject::SetValue(GASEnvironment* penv, const GASValue& val)
{
    GUNUSED2(penv, val);
}

GASValue GASObject::GetValue() const
{
    return GASValue();
}

bool GASObject::SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{
    static const GASValue notsetVal(GASValue::UNSET);
    GASPropFlags _flags;

    MemberHash::Iterator it = Members.FindCaseCheck(name, penv->IsCaseSensitive());
    GASMember*           pmember;
    if (it == Members.End())
    {
        // Need to check whole prototypes' chain in the case if this is a property set into
        // the prototype.
        for (GASObject* pproto = Get__proto__(); pproto != 0; pproto = pproto->Get__proto__())
        {
            if (pproto->ArePropertiesSet)
            {
                MemberHash::Iterator protoit = pproto->Members.FindCaseCheck(name, penv->IsCaseSensitive());
                if (protoit != pproto->Members.End())
                {
                    if (protoit->Second.Value.IsProperty())
                        it = protoit;
                    break;
                }
            }
        }
    }
    if (it != Members.End())
    {
        pmember = &(*it).Second;
        _flags = pmember->GetMemberFlags();
        if (pmember->Value.IsProperty())
        {
            GASValue propVal = pmember->Value;
            // check, is the object 'this' came from GFxASCharacter. If so,
            // use original GFxASCharacter as this.
            GPtr<GFxASCharacter> paschar = GetASCharacter();
            propVal.SetPropertyValue(penv, (paschar)?(GASObjectInterface*)paschar.GetPtr():this, val);
            return true;
        }
        // Is the member read-only ?
        if (_flags.GetReadOnly())
            return false;
    }
    else
    {
        _flags = flags;
        pmember = 0;
    }

    GASValue        newVal;
    const GASValue *pval = &val;

    if (penv && pWatchpoints) // have set any watchpoints?
    {
        if (InvokeWatchpoint(penv, name, val, &newVal))
        {
            pval = &newVal;
        }
    }

    if (!pmember)
    {
        return SetMemberRaw(penv->GetSC(), name, *pval, _flags);
    }
    else
    {
        // Check is the value property or not. If yes - set ArePropertiesSet flag to true
        if (val.IsProperty())
            ArePropertiesSet = true;

        if (penv->IsCaseSensitive())
        {
            if (name == penv->GetBuiltin(GASBuiltin___proto__))
            {   
                if (val.IsSet())
                {
                    Set__proto__(penv->GetSC(), val.ToObject(penv));

                    // Invalidate optimized advance list, since setting __proto__
                    // may turn on/off onEnterFrame events.
                    penv->InvalidateOptAdvanceList();
                }
                pval = &notsetVal;
            }
            else if (name == penv->GetBuiltin(GASBuiltin___resolve))
            {   
                if (val.IsSet())
                    ResolveHandler = val.ToFunction(penv);
                pval = &notsetVal;
            }
            else if (name == penv->GetBuiltin(GASBuiltin_onEnterFrame))
            {
                // Invalidate optimized advance list.
                penv->InvalidateOptAdvanceList();
            }
        }
        else
        {
            name.ResolveLowercase();
            if (penv->GetBuiltin(GASBuiltin___proto__).CompareBuiltIn_CaseInsensitive_Unchecked(name))
            {   
                if (val.IsSet())
                {
                    Set__proto__(penv->GetSC(), val.ToObject(penv));

                    // Invalidate optimized advance list, since setting __proto__
                    // may turn on/off onEnterFrame events.
                    penv->InvalidateOptAdvanceList();
                }
                pval = &notsetVal;
            }
            else if (penv->GetBuiltin(GASBuiltin___resolve).CompareBuiltIn_CaseInsensitive_Unchecked(name))
            {   
                if (val.IsSet())
                    ResolveHandler = val.ToFunction(penv);
                pval = &notsetVal;
            }
            else if (name == penv->GetBuiltin(GASBuiltin_onEnterFrame))
            {
                // Invalidate optimized advance list.
                penv->InvalidateOptAdvanceList();
            }
        }
        pmember->Value = *pval;
    }
    return true;
}

static const GASValue   notsetVal(GASValue::UNSET);

bool GASObject::SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{
    const GASValue*         pval = &val;
    MemberHash::Iterator    it;

    // Hack for Tween class memory leak prevention. See comments in GFxValue.cpp, CheckForListenersMemLeak.
    // This is also necessary for MovieClipLoader since it adds itself as a listener.
    if (!IsListenerSet && val.IsObject() && 
        (name == psc->GetBuiltin(GASBuiltin__listeners)))
    {
        GASObject *pobject = val.ToObject(NULL);
        if (pobject && (pobject->GetObjectType() == Object_Array))
            IsListenerSet = true;
    }



    // For efficiency, do extra case-sensitivity branch.
    if (psc->IsCaseSensitive())
    {
        if (name == psc->GetBuiltin(GASBuiltin___proto__))
        {   
            if (val.IsSet())
            {
                Set__proto__(psc, val.ToObject(NULL));

                // Invalidate optimized advance list, since setting __proto__
                // may turn on/off onEnterFrame events.
                psc->InvalidateOptAdvanceList();
            }
            pval = &notsetVal;
        }
        else if (name == psc->GetBuiltin(GASBuiltin___resolve))
        {   
            if (val.IsSet())
                ResolveHandler = val.ToFunction(NULL);
            pval = &notsetVal;
        }
        else if (name == psc->GetBuiltin(GASBuiltin_onEnterFrame))
        {
            // Invalidate optimized advance list.
            psc->InvalidateOptAdvanceList();
        }
        // Find member case-sensitively.
        it = Members.Find(name);
    }
    else
    {
        name.ResolveLowercase();
        if (psc->GetBuiltin(GASBuiltin___proto__).CompareBuiltIn_CaseInsensitive_Unchecked(name))
        {   
            if (val.IsSet())
            {
                Set__proto__(psc, val.ToObject(NULL));

                // Invalidate optimized advance list, since setting __proto__
                // may turn on/off onEnterFrame events.
                psc->InvalidateOptAdvanceList();
            }
            pval = &notsetVal;
        }
        else if (psc->GetBuiltin(GASBuiltin___resolve).CompareBuiltIn_CaseInsensitive_Unchecked(name))
        {   
            if (val.IsSet())
                ResolveHandler = val.ToFunction(NULL);
            pval = &notsetVal;
        }
        else if (name == psc->GetBuiltin(GASBuiltin_onEnterFrame))
        {
            // Invalidate optimized advance list.
            psc->InvalidateOptAdvanceList();
        }
        // Find member case-insensitively.
        it = Members.FindCaseInsensitive(name);
    }

    // Check is the value property or not. If yes - set ArePropertiesSet flag to true
    if (val.IsProperty())
        ArePropertiesSet = true;
    
#ifdef GFX_AMP_SERVER
    if (GFxAmpServer::GetInstance().IsEnabled() && !name.IsEmpty())
    {
        GASFunctionRef fn = pval->ToFunction(NULL);
        if (!fn.IsNull())
        {
            GASFunctionObject* fo = fn.GetObjectPtr();
            if (fo->IsAsFunction())
            {
                GASAsFunctionObject* asfo = static_cast<GASAsFunctionObject*>(fo);
                const GASActionBufferData* bufData = asfo->GetActionBuffer()->GetActionBufferData();

                GFxMovieRoot* pRoot = psc->pContext->GetMovieRoot();
                pRoot->AdvanceStats->RegisterScriptFunction(bufData->GetSwdHandle(), 
                    bufData->GetSWFFileOffset() + asfo->GetStartPC(), 
                    name.ToCStr(), asfo->GetLength());
            }
        }
    }
#endif  // GFX_AMP_SERVER

    if (it != Members.End())
    {   
        // This matches correct Flash behavior for both case sensitive and insensitive versions.
        // I.E. for case insensitive strings the identifier name is not overwritten, retaining
        // the original capitalization it had before assignment.
        GASMember* pmember = &(*it).Second;
        GASSERT(pmember);
        pmember->Value = *pval;
    }
    else
    {
        // Since object wasn't there, no need to check capitalization again.
        Members.Set(name, GASMember(*pval, flags));
    }

    return true;
}

bool    GASObject::GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
{
    //!AB: TODO - eliminate the extra virtual call here; though, to reach this point it 
    // is necessary to require overloading of both - GetMember and GetMemberRaw always.
    // Actually, the difference between GetMember and GetMemberRaw is in 1st parameter
    // only; functionally, they are the same now. TODO.
    bool rv = GetMemberRaw(penv->GetSC(), name, val);
    // GetMember now doesn't invoke getter methods as well as __resolve handlers.
    // This is done by the GASEnvironment::GetMember and GASEnvironment::GetVariable.
    return rv;
}

bool    GASObject::GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val)
{    
    GASSERT(psc && val);
    /*
    int sz = sizeof(GHashNode<GASString, GASMember, GASStringHashFunctor>);
    int sz2 = sizeof(GASMember);
    int sz3 = sizeof(GASValue);
    int sz4 = sizeof(GASFunctionRefBase);

    printf("Size GASValue: = %d\n", sz3);
    */

    GASObject* current = this;
    bool resolveHandlerSet = false;

    // For efficiency, do extra case-sensitivity branch.
    if (psc->IsCaseSensitive())
    {
        while (current != NULL)
        {
            MemberHash::Iterator im;
            if (name == psc->GetBuiltin(GASBuiltin___proto__))
            {   
                GASObject* pproto = current->Get__proto__();
                if (pproto) 
                    val->SetAsObject(pproto);
                else
                    val->SetUndefined();
                return true;
            }
            else if (name == psc->GetBuiltin(GASBuiltin___resolve))
            {   
                if (!current->ResolveHandler.IsNull())
                    val->SetAsFunction(current->ResolveHandler);
                else
                    val->SetUndefined();
                return true;
            }

            // Find member case-sensitively.
            im = current->Members.Find(name);

            if (im != current->Members.End())
            {
                const GASValue& memval = im->Second.GetMemberValue();
                // if member's value is unset and current is not equal to "this" (to avoid
                // indefinite recursion, then we need to call virtual method GetMemberRaw 
                // to get the value.
                if (!memval.IsSet() && current != this)
                    return current->GetMemberRaw(psc, name, val);
                *val = memval;
                return true;
            }

            if (!resolveHandlerSet && !current->ResolveHandler.IsNull())
            {
                // if not found and we have ResolveHandler - put it into the value, thus
                // we will be able to invoke it later in Execute.
                val->SetAsResolveHandler(current->ResolveHandler);
                resolveHandlerSet = true;
            }
            current = current->Get__proto__();
        }
    }
    else
    {
        name.ResolveLowercase();
        GASString::NoCaseKey ikey(name);
        while (current != NULL)
        {
            if (psc->GetBuiltin(GASBuiltin___proto__).CompareBuiltIn_CaseInsensitive_Unchecked(name))
            {   
                GASObject* pproto = current->Get__proto__();
                if (pproto) 
                    val->SetAsObject(pproto);
                else
                    val->SetUndefined();
                return true;
            }
            else if (psc->GetBuiltin(GASBuiltin___resolve).CompareBuiltIn_CaseInsensitive_Unchecked(name))
            {   
                if (!current->ResolveHandler.IsNull())
                    val->SetAsFunction(current->ResolveHandler);
                else
                    val->SetUndefined();
                return true;
            }

            // Find member case-insensitively.
            MemberHash::Iterator im;

            im = current->Members.FindAlt(ikey);

            if (im != current->Members.End())
            {
                const GASValue& memval = im->Second.GetMemberValue();
                // if member's value is unset and current is not equal to "this" (to avoid
                // indefinite recursion, then we need to call virtual method GetMemberRaw 
                // to get the value.
                if (!memval.IsSet() && current != this)
                    return current->GetMemberRaw(psc, name, val);
                *val = memval;
                return true;
            }

            if (!resolveHandlerSet && !current->ResolveHandler.IsNull())
            {
                // if not found and we have ResolveHandler - put it into the value, thus
                // we will be able to invoke it later in Execute.
                val->SetAsResolveHandler(current->ResolveHandler);
                resolveHandlerSet = true;
            }
            current = current->Get__proto__();
        }
    }
/*
    if (im != Members.End())
    {
        *val = im->Second.GetMemberValue();
        return true;
    }
  */  
    // MA: Could optimize this a bit by unrolling recursion and thus avoiding
    // nested '__proto__' and '__resolve' checks.
    /*GASObject* pproto = Get__proto__();
    if (pproto)
        return pproto->GetMemberRaw(psc, name, val);*/

    return false;
}

bool    GASObject::FindMember(GASStringContext *psc, const GASString& name, GASMember* pmember)
{
    //printf("GET MEMBER: %s at %p for object %p\n", name.C_str(), val, this);
    GASSERT(pmember != NULL);
    return Members.GetCaseCheck(name, pmember, psc->IsCaseSensitive());
}

bool    GASObject::DeleteMember(GASStringContext *psc, const GASString& name)
{
    // Find member.
    MemberHash::ConstIterator it = Members.FindCaseCheck(name, psc->IsCaseSensitive());
    // Member must exist and be available for deletion.
    if (it == Members.End())
        return false;
    if (it->Second.GetMemberFlags().GetDontDelete())
        return false;
    if (name == psc->GetBuiltin(GASBuiltin_onEnterFrame))
    {
        // Invalidate optimized advance list.
        psc->InvalidateOptAdvanceList();
    }
    Members.Remove(name);
    return true;
}

bool    GASObject::SetMemberFlags(GASStringContext *psc, const GASString& name, const UByte flags)
{
    GASMember member;
    if (FindMember(psc, name, &member))
    {
        GASPropFlags f = member.GetMemberFlags();
        f.SetFlags(flags);
        member.SetMemberFlags(f);

        Members.Set(name, member);
        return true;
    }
    return false;
}

bool    GASObject::GetTableMember(GASEnvironment* penv, func_member_value& funceTable, void* pClass, GFxValue* val)
{
	MemberHash::ConstIterator it = Members.Begin();
    while(it != Members.End())
    {
		funceTable(pClass, it->First.ToCStr(), val);
        ++it;
	}
	//bool rv = GetMemberRaw(penv->GetSC(), name, val);
    // GetMember now doesn't invoke getter methods as well as __resolve handlers.
    // This is done by the GASEnvironment::GetMember and GASEnvironment::GetVariable.
    return true;
}

void    GASObject::VisitMembers(GASStringContext *psc, 
                                MemberVisitor *pvisitor, 
                                UInt visitFlags, 
                                const GASObjectInterface* instance) const
{
    MemberHash::ConstIterator it = Members.Begin();
    while(it != Members.End())
    {
        const GASPropFlags& memberFlags = it->Second.GetMemberFlags();
        if (!memberFlags.GetDontEnum() || (visitFlags & VisitMember_DontEnum))
        {
            const GASValue& value = it->Second.GetMemberValue();
            if (value.IsSet())
                pvisitor->Visit(it->First, value, memberFlags.Flags);
            else
            {
                // If value is not set - get it by GetMember
                GASValue value;
                if (!(visitFlags & GASObjectInterface::VisitMember_NamesOnly))
                {
                    //!AB: basically, we need just a name here. We need to use original instance 
                    // pointer to call GetMemberRaw (if it is not NULL). This will help to get
                    // correct value for members, whose names are stored in prototypes.
                    GASObjectInterface* pobj = 
                        const_cast<GASObjectInterface*>((instance) ? instance : 
                        static_cast<const GASObjectInterface*>(this));
                    pobj->GetMemberRaw(psc, it->First, &value);
                }
                pvisitor->Visit(it->First, value, memberFlags.Flags);
            }
        }
        ++it;
    }

    if ((visitFlags & VisitMember_Prototype) && pProto)
        pProto->VisitMembers(psc, pvisitor, visitFlags, (instance) ? instance : this);
}

bool    GASObject::HasMember(GASStringContext *psc, const GASString& name, bool inclPrototypes)
{
    GASMember member;
    if (Members.Get(name, &member))
    {
        return true;
    }
    else if (inclPrototypes && pProto)
    {
        return pProto->HasMember(psc, name, true);
    }
    return false;
}

bool GASObject::InstanceOf(GASEnvironment* penv, const GASObject* prototype, bool inclInterfaces) const
{
    for (const GASObject* proto = this; proto != 0; proto = const_cast<GASObject*>(proto)->Get__proto__())
    {
        if (inclInterfaces)
        {
            if (proto->DoesImplement(penv, prototype)) return true;
        }
        else
        {
            if (proto == prototype) return true;
        }
    }
    return false;
}

bool GASObject::Watch(GASStringContext *psc, const GASString& prop, const GASFunctionRef& callback, const GASValue& userData)
{
    Watchpoint wp;
    wp.Callback = callback;
    wp.UserData = userData;
    if (pWatchpoints == NULL)
    {
        pWatchpoints = GHEAP_NEW(psc->GetHeap()) WatchpointHash;
    }
    // Need to do a case check on set too, to differentiate overwrite behavior. 
    pWatchpoints->SetCaseCheck(prop, wp, psc->IsCaseSensitive());
    return true;
}

bool GASObject::Unwatch(GASStringContext *psc, const GASString& prop)
{
    if (pWatchpoints != NULL)
    {
        if (pWatchpoints->GetCaseCheck(prop, psc->IsCaseSensitive()) != 0)
        {
            pWatchpoints->RemoveCaseCheck(prop, psc->IsCaseSensitive());
            if (pWatchpoints->GetSize() == 0)
            {
                delete pWatchpoints;
                pWatchpoints = 0;
            }
            return true;
        }
    }
    return false;
}

bool GASObject::InvokeWatchpoint(GASEnvironment* penv, const GASString& prop, const GASValue& newVal, GASValue* resultVal)
{
    GASValue oldVal;
    // if property doesn't exist at this point, the "oldVal" value will be "undefined"
    GetMember(penv, prop, &oldVal);

    GASSERT(resultVal);

    GASValue result;
    const Watchpoint* wp = pWatchpoints->GetCaseCheck(prop, penv->IsCaseSensitive());
    if (wp && penv && pWatchpoints)
    {
        penv->Push(wp->UserData);
        penv->Push(newVal);
        penv->Push(oldVal);
        penv->Push(GASValue(prop));
        
        // check, is the object 'this' came from GFxASCharacter. If so,
        // use original GFxASCharacter as this.
        GPtr<GFxASCharacter> paschar = GetASCharacter();

        wp->Callback.Invoke(GASFnCall(&result, (paschar)?(GASObjectInterface*)paschar.GetPtr():this, penv, 4, penv->GetTopIndex()));
        penv->Drop(4);
        *resultVal = result;
        return true;
    }
    return false;
}

GASObject* GASObject::GetPrototype(GASGlobalContext* pgc, GASBuiltinType classNameId)
{
    return pgc->GetPrototype(classNameId);
}

///////////////////////
GASPrototypeBase::~GASPrototypeBase()
{
    if (pInterfaces)
        delete pInterfaces;
}

void GASPrototypeBase::InitFunctionMembers(
    GASObject* pthis,
    GASStringContext *psc, 
    const GASNameFunction* funcTable, 
    const GASPropFlags& flags)
{
    GPtr<GASObject> pfuncProto = GASObject::GetPrototype(psc->pContext, GASBuiltin_Function);
    for(int i = 0; funcTable[i].Name; i++)
    {
        pthis->SetMemberRaw(psc, psc->CreateConstString(funcTable[i].Name), 
            GASFunctionRef(
            *GHEAP_NEW(psc->GetHeap()) GASCFunctionObject(psc, pfuncProto, funcTable[i].Function)),
            flags);
    }
}

// 'Init' method is used from the GASPrototype ancestor ctor to init the GASPrototypeBase instance.
// It is actually a part of GASPrototypeBase ctor, but due to GASPrototypeBase is not GASObject we need
// to do this in separate function to avoid warning ("usage of 'this' in initializer list).
void GASPrototypeBase::Init(GASObject* pthis, GASStringContext *psc, const GASFunctionRef& constructor)
{
    SetConstructor(pthis, psc, constructor); 
    Constructor->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_prototype), GASValue(pthis),
        GASPropFlags::PropFlag_DontEnum | GASPropFlags::PropFlag_DontDelete);
}

bool GASPrototypeBase::SetConstructor(GASObject* pthis, GASStringContext *psc, const GASValue& ctor) 
{ 
    Constructor = ctor.ToFunction(NULL);
    pthis->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_constructor), GASValue(GASValue::UNSET),
        GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    return true; 
}

void GASPrototypeBase::AddInterface(GASStringContext *psc, int index, GASFunctionObject* pinterface)
{
    if (pInterfaces == 0 && pinterface == 0)
    {
        pInterfaces = GHEAP_NEW(psc->GetHeap()) InterfacesArray(index);
        return;
    }
    GASSERT(pinterface);
    GASValue prototypeVal;
    if (pinterface->GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_prototype), &prototypeVal))
    {
        (*pInterfaces)[index] = GPtr<GASObject>(prototypeVal.ToObject(NULL));
    }
}

bool GASPrototypeBase::DoesImplement(GASEnvironment* penv, const GASObject* prototype) const 
{ 
    if (pInterfaces)
    {
        for(UInt i = 0, n = UInt(pInterfaces->GetSize()); i < n; ++i)
        {
            GPtr<GASObject> intf = (*pInterfaces)[i];
            if (intf && intf->InstanceOf(penv, prototype))
                return true;
        }
    }
    return false;
}

bool GASPrototypeBase::GetMemberRawConstructor(GASObject* pthis, GASStringContext *psc, const GASString& name, GASValue* val, bool isConstructor2)
{
    GASMember m;
    GASValue lval(GASValue::UNSET);

    // if "constructor" is set as a member - return it.
    if (pthis->GASObject::FindMember(psc, name, &m))
    {
        lval = m.GetMemberValue();
    }
    if (lval.IsSet())
        *val = lval;
    else
    {
        GASFunctionRef ctor;
        if (isConstructor2)
            ctor = __Constructor__;
        else
            ctor = GetConstructor ();
        if (ctor.IsNull())
        {
            val->SetUndefined();
            GASObject* pproto = pthis->Get__proto__();
            // check __proto__
            if (pproto)
                return pproto->GetMemberRaw(psc, name, val);

        }
        else
            val->SetAsFunction(ctor);
    }
    return true;
}

///////////////////////
static const GASNameFunction GAS_ObjectFunctionTable[] = 
{
    { "addProperty",            &GASObjectProto::AddProperty },
    { "hasOwnProperty",         &GASObjectProto::HasOwnProperty },
    { "watch",                  &GASObjectProto::Watch },
    { "unwatch",                &GASObjectProto::Unwatch },
    { "isPropertyEnumerable",   &GASObjectProto::IsPropertyEnumerable },
    { "isPrototypeOf",          &GASObjectProto::IsPrototypeOf },
    { "toString",               &GASObjectProto::ToString },
    { "valueOf",                &GASObjectProto::ValueOf },
    { 0, 0 }
};

GASObjectProto::GASObjectProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor)
    : GASPrototype<GASObject>(psc, pprototype, constructor) 
{
    InitFunctionMembers(psc, GAS_ObjectFunctionTable);
}

// This ctor is used by 'extends' operation and does NOT 
// initialize the function table
GASObjectProto::GASObjectProto(GASStringContext *psc, GASObject* pprototype)
    : GASPrototype<GASObject>(psc, pprototype) 
{
}

GASObjectProto::GASObjectProto(GASStringContext *psc, const GASFunctionRef& constructor)
    : GASPrototype<GASObject>(psc, constructor) 
{
    InitFunctionMembers(psc, GAS_ObjectFunctionTable);
}

void GASObjectProto::AddProperty(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    if (fn.NArgs >= 2)
    {
        GASString       propName(fn.Arg(0).ToString(fn.Env));
        GASFunctionRef  getter = fn.Arg(1).ToFunction(fn.Env);

        if (!getter.IsNull())
        {
            GASFunctionRef setter;
            if (fn.NArgs >= 3 && fn.Arg(2).IsFunction())
                setter = fn.Arg(2).ToFunction(fn.Env);
            GASValue val(getter, setter, fn.Env->GetHeap(), fn.Env->GetCollector());
            fn.ThisPtr->SetMemberRaw(fn.Env->GetSC(), propName, val);
            fn.Result->SetBool(true);
        }
        else
            fn.Result->SetBool(false);
    }
    else
        fn.Result->SetBool(false);
}

void GASObjectProto::HasOwnProperty(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    fn.Result->SetBool(fn.ThisPtr->HasMember(fn.Env->GetSC(), fn.Arg(0).ToString(fn.Env), false));
}

void GASObjectProto::Watch(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    if (fn.NArgs >= 2)
    {        
        GASFunctionRef callback = fn.Arg(1).ToFunction(fn.Env);
        if (!callback.IsNull())
        {
            GASValue userData;
            if (fn.NArgs >= 3)
            {
                userData = fn.Arg(2);
            }
            fn.Result->SetBool(fn.ThisPtr->Watch(fn.Env->GetSC(), fn.Arg(0).ToString(fn.Env), callback, userData));
        }
        else
            fn.Result->SetBool(false);
    }
    else
        fn.Result->SetBool(false);
}

void GASObjectProto::Unwatch(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    if (fn.NArgs >= 1)
    {        
        fn.Result->SetBool(fn.ThisPtr->Unwatch(fn.Env->GetSC(), fn.Arg(0).ToString(fn.Env)));
    }
    else
        fn.Result->SetBool(false);
}

void GASObjectProto::IsPropertyEnumerable(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    if (fn.NArgs >= 1)
    {
        GASString prop(fn.Arg(0).ToString(fn.Env));
        bool rv = fn.ThisPtr->HasMember(fn.Env->GetSC(), prop, false);
        if (rv)
        {
            GASMember m;
            fn.ThisPtr->FindMember(fn.Env->GetSC(), prop, &m);
            if (m.GetMemberFlags().GetDontEnum())
                rv = false;
        }
        fn.Result->SetBool(rv);
    }
    else
        fn.Result->SetBool(false);
}

void GASObjectProto::IsPrototypeOf(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    if (fn.NArgs >= 1 && !fn.ThisPtr->IsASCharacter())
    {
        GASObject* pthis = (GASObject*)fn.ThisPtr;
        GASObjectInterface* iobj = fn.Arg(0).ToObjectInterface(fn.Env);
        if (iobj)
        {
            fn.Result->SetBool(iobj->InstanceOf(fn.Env, pthis, false));
            return;
        }
    }
    fn.Result->SetBool(false);
}

void GASObjectProto::ToString(const GASFnCall& fn)
{
    if (fn.ThisPtr->IsFunction()) // "[type Function]"
        fn.Result->SetString(fn.Env->GetBuiltin(GASBuiltin_typeFunction_));
    else if (fn.ThisPtr->IsASCharacter())
        fn.Result->SetString(GASValue(static_cast<GFxASCharacter*>(fn.ThisPtr)).GetCharacterNamePath(fn.Env));
    else // "[object Object]"
        fn.Result->SetString(fn.Env->GetBuiltin(GASBuiltin_objectObject_));
}

void GASObjectProto::ValueOf(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    fn.Result->SetAsObjectInterface(fn.ThisPtr);
}

// Constructor for ActionScript class Object.
void GASObjectProto::GlobalCtor(const GASFnCall& fn)
{
    if (fn.NArgs >= 1)
    {
        //!AB, special undocumented(?) case: if Object's ctor is invoked
        // with a parameter of type:
        // 1. number - it will create instance of Number;
        // 2. boolean - it will create instance of Boolean;
        // 3. array - it will create instance of Array;
        // 4. string - it will create instance of String;
        // 5. object - it will just return the same object.
        // "null" and "undefined" are ignored.
        const GASValue& arg0 = fn.Arg(0);
        GASValue res;
        if (arg0.IsNumber())
            res = arg0.ToNumber(fn.Env);
        else if (arg0.IsBoolean())
            res = arg0.ToBool(fn.Env);
        else if (arg0.IsString())
            res = arg0.ToString(fn.Env);
        else if (arg0.IsObject() || arg0.IsCharacter())
        {
            res = arg0;
        }
        if (!res.IsUndefined())
        {
            *fn.Result = res;
            return;
        }
    }
    GPtr<GASObject> o;
    if (fn.ThisPtr)
        o = static_cast<GASObject*>(fn.ThisPtr);
    else
        o = *GHEAP_NEW(fn.Env->GetHeap()) GASObject(fn.Env);

    // for some reasons Object always contains "constructor" property instead of __constructor__
    GASFunctionRef ctor = fn.Env->GetConstructor(GASBuiltin_Object);
    o->Set_constructor(fn.Env->GetSC(), ctor);

    fn.Result->SetAsObject(o);
}

//////////////////
const GASNameFunction GASObjectCtorFunction::StaticFunctionTable[] = 
{
    { "registerClass",       &GASObjectCtorFunction::RegisterClass },
    { 0, 0 }
};

GASObjectCtorFunction::GASObjectCtorFunction(GASStringContext *psc) :
    GASCFunctionObject(psc, GASObjectProto::GlobalCtor)
{
    GASNameFunction::AddConstMembers(
        this, psc, StaticFunctionTable, 
        GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    /*
    for(int i = 0; StaticFunctionTable[i].Name; i++)
    {
        SetMemberRaw(psc, psc->CreateConstString(StaticFunctionTable[i].Name),
                     GASValue(StaticFunctionTable[i].Function), 
                     GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete |
                     GASPropFlags::PropFlag_DontEnum);
    }
    */
}

void GASObjectCtorFunction::RegisterClass(const GASFnCall& fn)
{
    fn.Result->SetBool(false);
    if (fn.NArgs < 2)
    {
        fn.Env->LogScriptError ("Error: Too few parameters for Object.registerClass (%d)", fn.NArgs);   
        return;
    }

    GASGlobalContext* pgctxt = fn.Env->GetGC();
    GASString         classname(fn.Arg(0).ToString(fn.Env));
    GASSERT(pgctxt);
    if (fn.Arg(1).IsFunction())
    {
        // Registration
        GASFunctionRef func = fn.Arg(1).ToFunction(fn.Env);        
        fn.Result->SetBool(pgctxt->RegisterClass(fn.Env->GetSC(), classname, func));
    }
    else if (fn.Arg(1).IsNull())
    {
        // De-registration
        fn.Result->SetBool(pgctxt->UnregisterClass(fn.Env->GetSC(), classname));
    }
    else
    {
        GASString a1(fn.Arg(1).ToString(fn.Env));
        fn.Env->LogScriptError("Error: Second parameter of Object.registerClass(%s, %s) should be function or null\n", 
                               classname.ToCStr(), a1.ToCStr());
    }
}

GASObject* GASObjectCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{
    return GHEAP_NEW(penv->GetHeap()) GASObject(penv);
}


GASObject* GASFunctionCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{ 
    return GHEAP_NEW(penv->GetHeap()) GASCFunctionObject(penv->GetSC()); 
}


