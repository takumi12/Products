/**********************************************************************

Filename    :   AS/GASSharedObject.h
Content     :   AS2 Implementation of SharedObject class
Created     :   January 20, 2009
Authors     :   Prasad Silva

Notes       :   
History     :   

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxSharedObject_H
#define INC_GFxSharedObject_H

#include "GConfig.h"
#if !defined(GFC_NO_FXPLAYER_AS_SHAREDOBJECT)

#include "AS/GASObject.h"
#include "AS/GASObjectProto.h"
#include "GFxPlayerImpl.h"
#include "GFxSharedObject.h"

//
// AS2 Shared Object
//
class GASSharedObject : public GASObject
{
protected:
    GASSharedObject(GASStringContext *psc, GASObject* pprototype)
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }

public:
#ifndef GFC_NO_GC
    void Finalize_GC() 
    { 
        Name.~GString(); 
        LocalPath.~GString(); 
        GASObject::Finalize_GC();
    }
#endif

    GASSharedObject(GASEnvironment* penv);

    virtual ObjectType      GetObjectType() const { return Object_SharedObject; }

    const GString& GetName() const { return Name; }
    const GString& GetLocalPath() const { return LocalPath; }

    // Overloaded to prohibit setting 'data' member from AS runtime
    virtual bool        SetMember(GASEnvironment* penv, 
        const GASString& name, const GASValue& val, 
        const GASPropFlags& flags = GASPropFlags());

    // Special setter to initialize the 'data' member
    void    SetDataObject(GASEnvironment* penv, GASObject* pobj);

    bool    SetNameAndLocalPath(const GString& name, const GString& localPath );
    
    UPInt   ComputeSizeInBytes(GASEnvironment* penv);
    void    Flush(GASEnvironment* penv, GFxSharedObjectVisitor* writer);

private:
    GString     Name;
    GString     LocalPath;
};


//
// AS2 Shared Object Prototype
//
class GASSharedObjectProto : public GASPrototype<GASSharedObject>
{
    static const GASNameFunction FunctionTable[];

public:
    GASSharedObjectProto (GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor);

    static void         Clear(const GASFnCall& fn);
    static void         Flush(const GASFnCall& fn);
    static void         GetSize(const GASFnCall& fn);
};


//
// Proxy for SharedObject pointers (needed for GC)
//
class GASSharedObjectPtr : public GPtr<GASSharedObject> 
{
public:
    GINLINE GASSharedObjectPtr(GASSharedObject *robj) : GPtr<GASSharedObject>(robj) {}
    GINLINE virtual ~GASSharedObjectPtr() {}

#ifndef GFC_NO_GC
    typedef GRefCountBaseGC<GFxStatMV_ActionScript_Mem>::Collector Collector;
    template <class Functor> void   ForEachChild_GC(Collector* prcc) const    { Functor::Call(prcc, pObject); }
    virtual void                    Finalize_GC() {}
#endif // GFC_NO_GC
};

//
// AS2 SharedObject Constructor Function
//
class GASSharedObjectCtorFunction : public GASCFunctionObject
{
    static const GASNameFunction StaticFunctionTable[];

    static void         GetLocal(const GASFnCall& fn);

    typedef GASStringHash_GC<GASSharedObjectPtr>     SOLookupType;
    SOLookupType        SharedObjects;

#ifndef GFC_NO_GC
protected:
    friend class GRefCountBaseGC<GFxStatMV_ActionScript_Mem>;
    template <class Functor> void ForEachChild_GC(Collector* prcc) const;
    virtual void                  ExecuteForEachChild_GC(Collector* prcc, OperationGC operation) const;
    virtual void                  Finalize_GC();
#endif // GFC_NO_GC

public:
    GASSharedObjectCtorFunction (GASStringContext *psc);

    static void             GlobalCtor(const GASFnCall& fn);
    virtual GASObject*      CreateNewObject(GASEnvironment*) const;

    static GASFunctionRef   Register(GASGlobalContext* pgc);
};


#endif  // GFC_NO_FXPLAYER_AS_SHAREDOBJECT

#endif // INC_GFxSharedObject_H
