/**********************************************************************

Filename    :   AS/GASObjectProto.h
Content     :   ActionScript Object prototype implementation classes
Created     :   
Authors     :   Artem Bolgar

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXOBJECTPROTO_H
#define INC_GFXOBJECTPROTO_H

#include "AS/GASObject.h"

class GASGlobalContext;

// The base class for GASPrototype template class. This class contains common
// functionality that does not depend on the template parameters.
class GASPrototypeBase
{
protected:
#ifndef GFC_NO_GC
    // refcount collection version
    class InterfacesArray : public GArrayLH<GASWeakPtr<GASObject> >
    {
    public:
        typedef GArrayLH<GASWeakPtr<GASObject> > BaseType;

        InterfacesArray() : BaseType() {}
        InterfacesArray(int size) : BaseType(size) {}
        ~InterfacesArray()
        {
            GASSERT(0); // should never be executed! Finalize_GC should!
        }
        void Finalize_GC()
        {
            AllocatorType::Free(Data.Data);
        }
    };
    typedef GASFunctionRef ConstructorFuncRef;
#else
    typedef GArrayLH<GASWeakPtr<GASObject> > InterfacesArray;
    typedef GASFunctionWeakRef ConstructorFuncRef;
#endif

    // Flash creates a "natural" circular reference by using standard
    // properties "constructor" (in prototype object) and "prototype" (in constructor
    // function object). The "constructor" prototype's property points on a constructor
    // function object and its "prototype" property points back to prototype.
    // This is not a big problem, if garbage collection is in use, the biggest problem
    // exists if just refcounting is used. However, even with GC, using strong pointers for
    // both properties causes extra collection calls and extra memory used.
    // To avoid this problem we use weak reference for the "constructor" property. 
    // Function object has both - weak and strong pointer to prototype. Strong pointer
    // is stored as a member in hash table, weak - just a pointer. This week ptr is necessary
    // to perform "constructor" property cleanup, when Function object dies.
    // "constructor" property in prototype might be strong as well. It becomes strong
    // when this property is assigned by ActionScript (see GASPrototype::GetMemberRaw).
    // The only known limitation of this approach is the following very rare situation:
    //  function Func() {}   
    //  var p = Func.prototype;
    //  Func = null;
    //  trace ("p.constructor           = "+p.constructor);
    // This trace will print [type Function] in Flash and "undefined" in GFx.
    ConstructorFuncRef Constructor;     // Weak reference, constructor has strong reference
    // to this prototype instance. If GC is in use, this is 
    // a strong reference.
    GASFunctionRef __Constructor__;     // Strong reference, __constructor__, a reference
    // to this prototype instance

    InterfacesArray*   pInterfaces; // array of implemented interfaces

public:
    GASPrototypeBase(): pInterfaces(0) {}

    virtual ~GASPrototypeBase();

    GINLINE GASFunctionRef  GetConstructor() { return Constructor; }
protected:
    void InitFunctionMembers(
        GASObject* pthis,
        GASStringContext *psc, 
        const GASNameFunction* funcTable, 
        const GASPropFlags& flags = GASPropFlags::PropFlag_DontEnum);

    // 'Init' method is used from the GASPrototype ancestor ctor to init the GASPrototypeBase instance.
    // It is actually a part of GASPrototypeBase ctor, but due to GASPrototypeBase is not GASObject we need
    // to do this in separate function to avoid warning ("usage of 'this' in initializer list).
    void Init(GASObject* pthis, GASStringContext *psc, const GASFunctionRef& constructor);

    bool                    SetConstructor(GASObject* pthis, GASStringContext *psc, const GASValue& ctor);
    void                    AddInterface(GASStringContext *psc, int index, GASFunctionObject* pinterface);

    bool                    DoesImplement(GASEnvironment* penv, const GASObject* prototype) const;
    bool                    GetMemberRawConstructor
        (GASObject* pthis, GASStringContext *psc, const GASString& name, GASValue* val, bool isConstructor2);
};

// This class contains common definition for prototype classes. It is template since
// basically all prototype objects are the instances of the same class (for example,
// String.prototype is an instance of String).
template <typename BaseClass, typename GASEnvironment> 
class GASPrototype : public BaseClass, public GASPrototypeBase
{
#ifndef GFC_NO_GC
    // refcount collection version
GFC_GC_PROTECTED:
    friend class GRefCountBaseGC<GFxStatMV_ActionScript_Mem>;
    typedef GRefCountBaseGC<GFxStatMV_ActionScript_Mem>::Collector Collector;

    template <typename Functor>
    void ForEachChild_GC(Collector* prcc) const
    {
        BaseClass::template ForEachChild_GC<Functor>(prcc);
        Constructor.template ForEachChild_GC<Functor>(prcc);
        __Constructor__.template ForEachChild_GC<Functor>(prcc);
        if (pInterfaces)
        {
            for (UPInt i = 0, n = pInterfaces->GetSize(); i < n; ++i)
            {
                GASWeakPtr<GASObject>& pintf = (*pInterfaces)[i];
                if (pintf)
                    Functor::Call(prcc, pintf);
            }
        }
    }
    virtual void ExecuteForEachChild_GC(
        Collector* prcc, GRefCountBaseGC<GFxStatMV_ActionScript_Mem>::OperationGC operation) const
    {
        GASRefCountBaseType::template CallForEachChild<GASPrototype<BaseClass, GASEnvironment> >(prcc, operation);
    }
    void Finalize_GC()
    {
        if (pInterfaces)
        {
            pInterfaces->Finalize_GC();
            GFREE(pInterfaces);
        }
        BaseClass::Finalize_GC();
    }
#else
protected:    
    // no refcount collection version

    virtual void CheckAndResetCtorRef(GASFunctionObject* pfunc) 
    {
        if (Constructor.GetObjectPtr() == pfunc)
            Constructor = NULL;
    }
#endif //GFC_NO_GC

protected:    
    void InitFunctionMembers(
        GASStringContext *psc, 
        const GASNameFunction* funcTable, 
        const GASPropFlags& flags = GASPropFlags::PropFlag_DontEnum)
    {
        GASPrototypeBase::InitFunctionMembers(
            this,
            psc, 
            funcTable, 
            flags);
    }
public:

    // MA: We pass psc to base class in case it may need to have member variables of GASString class,
    // as in GASStringObject. Forces GASStringContext constructors on bases. Any way to avoid this?
    GASPrototype(GASStringContext *psc, const GASFunctionRef& constructor)
        : BaseClass(psc), GASPrototypeBase()
    {
        GASPrototypeBase::Init(this, psc, constructor);
    }

    GASPrototype(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor)
        : BaseClass(psc, pprototype), GASPrototypeBase()
    {
        GASPrototypeBase::Init(this, psc, constructor);
    }

    GASPrototype(GASStringContext *psc, GASObject* pprototype)
        : BaseClass(psc, pprototype), GASPrototypeBase()
    {
    }

    ~GASPrototype()
    {
    }

    inline bool     GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
    {
        GUNUSED(penv);
        return GetMemberRaw(penv->GetSC(), name, val);
    }

    virtual bool    GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val)
    {
        bool isConstructor2 = psc->GetBuiltin(GASBuiltin___constructor__).CompareBuiltIn_CaseCheck(name, psc->IsCaseSensitive());
        
        // if (name == "constructor" || name == "__constructor__")
        if ( isConstructor2 ||
             psc->GetBuiltin(GASBuiltin_constructor).CompareBuiltIn_CaseCheck(name, psc->IsCaseSensitive()) )
        {
            return GetMemberRawConstructor(this, psc, name, val, isConstructor2);
        }
        return BaseClass::GetMemberRaw(psc, name, val);
    }

    virtual void    AddInterface(GASStringContext *psc, int index, GASFunctionObject* ctor)
    {
        GASPrototypeBase::AddInterface(psc, index, ctor);
    }
    virtual bool    DoesImplement(GASEnvironment* penv, const GASObject* prototype) const 
    { 
        if (BaseClass::DoesImplement(penv, prototype)) return true; 
        return GASPrototypeBase::DoesImplement(penv, prototype);
    }

    // is it prototype of built-in class?
    virtual bool            IsBuiltinPrototype() const { return true; }
};

// A prototype for Object class (Object.prototype)
class GASObjectProto : public GASPrototype<GASObject>
{
public:
    GASObjectProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);

    // This ctor is used by 'extends' operation and does NOT 
    // initialize the function table
    GASObjectProto(GASStringContext *psc, GASObject* pprototype);

    GASObjectProto(GASStringContext *psc, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);
    static void AddProperty(const GASFnCall& fn);
    static void HasOwnProperty(const GASFnCall& fn);
    static void IsPropertyEnumerable(const GASFnCall& fn);
    static void IsPrototypeOf(const GASFnCall& fn);
    static void Watch(const GASFnCall& fn);
    static void Unwatch(const GASFnCall& fn);
    static void ToString(const GASFnCall& fn);
    static void ValueOf(const GASFnCall& fn);
};

// Function Prototype (Function.prototype)
class GASFunctionProto : public GASPrototype<GASObject> 
{
public:
    GASFunctionProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor, bool initFuncs = true);
    ~GASFunctionProto() {}

    static void GlobalCtor(const GASFnCall& fn);
    static void Apply(const GASFnCall& fn);
    static void Call(const GASFnCall& fn);
    static void ToString(const GASFnCall& fn);
    static void ValueOf(const GASFnCall& fn);
};

// A special implementation for "super" operator
class GASSuperObject : public GASObject
{
    GPtr<GASObject>     SuperProto;
    GPtr<GASObject>     SavedProto;
    GASObjectInterface* RealThis;
    GASFunctionRef      Constructor;
#ifndef GFC_NO_GC
protected:
    friend class GRefCountBaseGC<GFxStatMV_ActionScript_Mem>;
    template <class Functor>
    void ForEachChild_GC(Collector* prcc) const
    {
        GASObject::template ForEachChild_GC<Functor>(prcc);
        if (SuperProto)
            Functor::Call(prcc, SuperProto);
        if (SavedProto)
            Functor::Call(prcc, SavedProto);
        Constructor.template ForEachChild_GC<Functor>(prcc);
    }
    virtual void ExecuteForEachChild_GC(Collector* prcc, OperationGC operation) const
    {
        GASRefCountBaseType::CallForEachChild<GASSuperObject>(prcc, operation);
    }
    virtual void Finalize_GC() 
    { 
        Constructor.Finalize_GC(); 
        GASObject::Finalize_GC();
    }
#endif //GFC_NO_GC
public:
    GASSuperObject(GASObject* superProto, GASObjectInterface* _this, const GASFunctionRef& ctor) :
        GASObject(superProto->GetCollector()), SuperProto(superProto), RealThis(_this), Constructor(ctor) { pProto = superProto; }

    virtual bool    SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags())
    {
        return SuperProto->SetMember(penv, name, val, flags);
    }
    virtual bool    SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags())
    {
        return SuperProto->SetMemberRaw(psc, name, val, flags);
    }
    virtual bool    GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
    {
        return SuperProto->GetMember(penv, name, val);
    }
    virtual bool    GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val)
    {
        return SuperProto->GetMemberRaw(psc, name, val);
    }
    virtual bool    FindMember(GASStringContext *psc, const GASString& name, GASMember* pmember)
    {
        return SuperProto->FindMember(psc, name, pmember);
    }
    virtual bool    DeleteMember(GASStringContext *psc, const GASString& name)
    {
        return SuperProto->DeleteMember(psc, name);
    }
    virtual bool    SetMemberFlags(GASStringContext *psc, const GASString& name, const UByte flags)
    {
        return SuperProto->SetMemberFlags(psc, name, flags);
    }
    virtual void    VisitMembers(
        GASStringContext *psc, MemberVisitor *pvisitor, UInt visitFlags, const GASObjectInterface* instance) const
    {
        SuperProto->VisitMembers(psc, pvisitor, visitFlags, instance);
    }
    virtual bool    HasMember(GASStringContext *psc, const GASString& name, bool inclPrototypes)
    {
        return SuperProto->HasMember(psc, name, inclPrototypes);
    }

    GASObjectInterface* GetRealThis() { return RealThis; }

    /* is it super-class object */
    virtual bool            IsSuper() const { return true; }

    /*virtual GASObject*      Get__proto__()
    { 
        return SuperProto;
    }*/

    /* gets __constructor__ property */
    virtual GASFunctionRef   Get__constructor__(GASStringContext *psc)
    { 
        GUNUSED(psc);
        return Constructor; 
    }

    void SetAltProto(GASObject* altProto)
    {
        if (altProto == SuperProto) return;
        GASSERT(!SavedProto);
        SavedProto = SuperProto;
        SuperProto = altProto;
        pProto = SuperProto;
    }
    void ResetAltProto()
    {
        if (SavedProto)
        {
            SuperProto = SavedProto;
            SavedProto = NULL;
            pProto = SuperProto;
        }
    }
    GASObject* GetSuperProto() { return SuperProto; }
};

#endif //INC_GFXOBJECTPROTO_H
