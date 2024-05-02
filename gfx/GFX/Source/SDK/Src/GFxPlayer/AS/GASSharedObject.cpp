/**********************************************************************

Filename    :   AS/GFxSharedObject.cpp
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

#include "AS/GASSharedObject.h"

#if !defined(GFC_NO_FXPLAYER_AS_SHAREDOBJECT)

#include "GFxAction.h"
#include "GTypes.h"
#include "AS/GASArrayObject.h"
#include "GMsgFormat.h"

//#define GFC_DEBUG_GASSHAREDOBJECTLOADER

//
// Custom visitor for GASSharedObject that loads data into a 
// GASObject instance.
//
class GASSharedObjectLoader : public GFxSharedObjectVisitor
{
public:
    GASSharedObjectLoader(GASEnvironment* penv, GASObject* psobj) 
        : pEnv(penv), pData(psobj), bArrayIsTop(false) {}

    void Begin();
    void PushObject( const GString& name );
    void PushArray( const GString& name );
    void AddProperty( const GString& name, const GString& value, 
        GFxValue::ValueType type );
    void PopObject();
    void PopArray();
    void End();

private:
    GASEnvironment*     pEnv;
    GASObject*          pData;
    GArray<GASObject*>  ObjectStack;
    bool                bArrayIsTop;
};


void GASSharedObjectLoader::Begin()
{
#ifdef GFC_DEBUG_GASSHAREDOBJECTLOADER
    GFC_DEBUG_MESSAGE(1, "GASSharedObjectLoader::Begin");
#endif // GFC_DEBUG_GASSHAREDOBJECTLOADER

    ObjectStack.Clear();

    // Initialize root element
    ObjectStack.PushBack(pData);
}


void GASSharedObjectLoader::PushObject( const GString& name )
{
#ifdef GFC_DEBUG_GASSHAREDOBJECTLOADER
    GFC_DEBUG_MESSAGE1(1, "GASSharedObjectLoader::PushObject('%s')", name.ToCStr());
#endif  // GFC_DEBUG_GASSHAREDOBJECTLOADER

    GASSERT(ObjectStack.GetSize() > 0);
    GASObject* ptop = ObjectStack.Back();

    // Push an object to stack
    GPtr<GASObject> pobj = *pEnv->OperatorNew(pEnv->GetBuiltin(GASBuiltin_Object));

    if (bArrayIsTop)
    {
        GASArrayObject* ptoparr = static_cast<GASArrayObject*>(ptop);
        ptoparr->PushBack(GASValue(pobj));
    }
    else
        ptop->SetMember(pEnv, pEnv->CreateString(name), GASValue(pobj));

    bArrayIsTop = false;
    ObjectStack.PushBack(pobj);
}


void GASSharedObjectLoader::PushArray( const GString& name )
{
#ifdef GFC_DEBUG_GASSHAREDOBJECTLOADER
    GFC_DEBUG_MESSAGE1(1, "GASSharedObjectLoader::PushArray('%s')", name.ToCStr());
#endif  // GFC_DEBUG_GASSHAREDOBJECTLOADER

    GASSERT(ObjectStack.GetSize() > 0);
    GASObject* ptop = ObjectStack.Back();

    // Push an array to stack
    GPtr<GASObject> parr = *pEnv->OperatorNew(pEnv->GetBuiltin(GASBuiltin_Array));

    if (bArrayIsTop)
    {
        GASArrayObject* ptoparr = static_cast<GASArrayObject*>(ptop);
        ptoparr->PushBack(GASValue(parr));
    }
    else
        ptop->SetMember(pEnv, pEnv->CreateString(name), GASValue(parr));

    bArrayIsTop = true;
    ObjectStack.PushBack(parr);
}


void GASSharedObjectLoader::AddProperty(const GString &name, 
                                         const GString &value, 
                                         GFxValue::ValueType type)
{
#ifdef GFC_DEBUG_GASSHAREDOBJECTLOADER
    GFC_DEBUG_MESSAGE2(1, "GASSharedObjectLoader::PushProperty('%s', '%s')", 
        name.ToCStr(), value.ToCStr());
#endif  // GFC_DEBUG_GASSHAREDOBJECTLOADER

    GASSERT(ObjectStack.GetSize() > 0);
    GASObject* ptop = ObjectStack.Back();

    // Add property to top element
    GASValue v;
    switch (type)
    {
    case GFxValue::VT_Number:
        {
            v.SetNumber(atof(value.ToCStr()));
            break;
        }
    case GFxValue::VT_String:
        {
            v.SetString(pEnv->CreateString(value));
            break;
        }
    case GFxValue::VT_Boolean:
        {
            v.SetBool( !G_strncmp(value.ToCStr(), "true", 4) );
            break;
        }
    case GFxValue::VT_Undefined:
        {
            v.SetUndefined();
            break;
        }
    case GFxValue::VT_Null:
        {
            v.SetNull();
            break;
        }
    default:
        {
            GASSERT(0); // Unknown case
        }
    }

    if (bArrayIsTop)
    {
        GASArrayObject* ptoparr = static_cast<GASArrayObject*>(ptop);
        ptoparr->PushBack(v);
    }
    else
        ptop->SetMember(pEnv, pEnv->CreateString(name), v);
}


void GASSharedObjectLoader::PopObject()
{
#ifdef GFC_DEBUG_GASSHAREDOBJECTLOADER
    GFC_DEBUG_MESSAGE(1, "GASSharedObjectLoader::PopObject");
#endif  // GFC_DEBUG_GASSHAREDOBJECTLOADER

    // Root should not be popped
    GASSERT(ObjectStack.GetSize() > 1);
    ObjectStack.PopBack();
    // Evaluate current state
    GASObject* ptop = ObjectStack.Back();
    bArrayIsTop = (ptop->GetObjectType() == GASObject::Object_Array);
}


void GASSharedObjectLoader::PopArray()
{
#ifdef GFC_DEBUG_GASSHAREDOBJECTLOADER
    GFC_DEBUG_MESSAGE(1, "GASSharedObjectLoader::PopArray");
#endif  // GFC_DEBUG_GASSHAREDOBJECTLOADER

    PopObject();
}


void GASSharedObjectLoader::End()
{
#ifdef GFC_DEBUG_GASSHAREDOBJECTLOADER
    GFC_DEBUG_MESSAGE(1, "GASSharedObjectLoader::End");
#endif  // GFC_DEBUG_GASSHAREDOBJECTLOADER

    // Check stack state; only root element must exist
    GASSERT(bArrayIsTop == false);
    GASSERT(ObjectStack.GetSize() == 1);
}


///////////////////////////////////////////
//

GASSharedObject::GASSharedObject(GASEnvironment* penv)
: GASObject(penv)
{
    Set__proto__ (penv->GetSC(), penv->GetPrototype(GASBuiltin_SharedObject));
}

//
// From Flash Docs:
//
// name:String - A string that represents the name of the object. The 
//               name can include forward slashes (/); for example, 
//               work/addresses is a legal name. Spaces are not allowed 
//               in a shared object name, nor are the following characters: 
//
//               ~ % & \ ; : " ' , < > ? #
//
bool    GASSharedObject::SetNameAndLocalPath(const GString& name, 
                                             const GString& localPath )
{  
    const char* pstr;
    UInt32 c = name.GetFirstCharAt(0, &pstr);
    while (c)
    {
        switch (c)
        {
        case '~':
        case '%':
        case '&':
        case '\\':
        case ';':
        case ':':
        case '"':
        case '\'':
        case ',':
        case '<':
        case '>':
        case '?':
        case '#':
            return false;
        }
        // Check whitespace
        if (G_iswspace((wchar_t)c))
            return false;
        c = name.GetNextChar(&pstr);
    }
    Name = name;
    LocalPath = localPath;
    return true;
}


bool GASSharedObject::SetMember(GASEnvironment* penv, const GASString& name, 
                                const GASValue& val, 
                                const GASPropFlags& flags /* = GASPropFlags */)
{
    // No assignments to 'data' member are allowed
    if (name == "data")
        return true;
    return GASObject::SetMember(penv, name, val, flags);
}


void GASSharedObject::SetDataObject(GASEnvironment* penv, GASObject* pobj)
{
    GASObject::SetMember(penv, penv->CreateConstString("data"), GASValue(pobj));
}

//
// From Flash Docs:
//
// "Flash calculates the size of a shared object by stepping through 
//  all of its data properties; the more data properties the object 
//  has, the longer it takes to estimate its size. Estimating object 
//  size can take significant processing time, so you may want to 
//  avoid using this method unless you have a specific need for it."
//
UPInt GASSharedObject::ComputeSizeInBytes(GASEnvironment* penv)
{
    // Visit all members and accumulate member sizes
    GASValue val;
    bool hasData = GASObject::GetMember(penv, penv->CreateConstString("data"), &val);
    GASSERT(hasData);
    GUNUSED(hasData);
    GASObject* pdataObj = val.ToObject(penv);
    GASSERT(pdataObj);

    struct DataSizeEstimator : public GASObjectInterface::MemberVisitor
    {
        GASEnvironment*             pEnv;
        UPInt                       SizeInBytes;

        DataSizeEstimator(GASEnvironment *penv)
        {
            pEnv = penv;
            SizeInBytes = 0;
        }

        virtual void Visit(const GASString& name, const GASValue& val, UByte flags)
        {
            GUNUSED(flags);
            SizeInBytes += name.GetSize();
            switch (val.GetType())
            {
            case GASValue::NUMBER:
            case GASValue::INTEGER:
                {
                    SizeInBytes += sizeof(Float);
                    break;
                }
            case GASValue::STRING:
                {
                    SizeInBytes += val.ToString(pEnv).GetSize();
                    break;
                }
            case GASValue::OBJECT:
                {
                    GASObject* pobj = val.ToObject(pEnv);
                    pobj->VisitMembers(pEnv->GetSC(), this, 0);
                    break;
                }
            default:;
            }
        }
    } visitor(penv);    
    pdataObj->VisitMembers(penv->GetSC(), &visitor, 0);
    return visitor.SizeInBytes;
}


void GASSharedObject::Flush(GASEnvironment* penv, GFxSharedObjectVisitor* writer)
{
    if (!writer)
        return;

    // Visit all members and notify writer
    GASValue val;
    bool hasData = GASObject::GetMember(penv, penv->CreateConstString("data"), &val);
    GASSERT(hasData);
    GUNUSED(hasData);
    GASObject* pdataObj = val.ToObject(penv);
    GASSERT(pdataObj);
    
    struct DataWriter : public GASObjectInterface::MemberVisitor
    {
        // Hash is required to avoid infinite loops when objects have
        // circular references. If an object has been visited we do 
        // not visit it again.
        GHashIdentity<GASObject*, GASObject*>   VisitedObjects;
        GASEnvironment*                         pEnv;
        GFxSharedObjectVisitor*                 pWriter;

        DataWriter(GASEnvironment *penv, GFxSharedObjectVisitor* pwriter)
        {
            pEnv = penv;
            pWriter = pwriter;
        }

        virtual void Visit(const GASString& name, const GASValue& val, UByte flags)
        {
            GUNUSED(flags);
            GString propname(name.ToCStr(), name.GetSize());
            switch (val.GetType())
            {
            case GASValue::NUMBER:
            case GASValue::INTEGER:
                {
                    // writeout number                    
                    GDoubleFormatter f(val.ToNumber(pEnv));
                    f.Convert();
                    GString sval(f, f.GetSize());

                    pWriter->AddProperty( propname, sval, 
                        GFxValue::VT_Number);
                    break;
                }
            case GASValue::STRING:
                {
                    // writeout string
                    GString sval(val.ToString(pEnv).ToCStr());
                    pWriter->AddProperty( propname, sval, 
                        GFxValue::VT_String);
                    break;
                }
            case GASValue::UNDEFINED:
                {
                    // writeout undefined
                    pWriter->AddProperty( propname, "",
                        GFxValue::VT_Undefined);
                    break;
                }
            case GASValue::BOOLEAN:
                {
                    // writeout boolean                    
                    pWriter->AddProperty( propname, 
                        val.ToBool(pEnv) ? "true" : "false", GFxValue::VT_Boolean);
                    break;
                }
            case GASValue::NULLTYPE:
                {
                    // writeout null
                    pWriter->AddProperty( propname, "", GFxValue::VT_Null);
                    break;
                }
            case GASValue::OBJECT:
                {
                    // special case for Array
                    GASObject* pobj = val.ToObject(pEnv);
                    if (VisitedObjects.Get(pobj)) break;
                    VisitedObjects.Add(pobj, pobj);
                    if (pobj->GetObjectType() == GASObject::Object_Array)
                    {
                        // writeout array
                        pWriter->PushArray( propname );
                        pobj->VisitMembers(pEnv->GetSC(), this, 0);
                        pWriter->PopArray();
                    }
                    else
                    {
                        // writeout object
                        pWriter->PushObject( propname );
                        pobj->VisitMembers(pEnv->GetSC(), this, 0);
                        pWriter->PopObject();
                    }
                    break;
                }
            default:;
            }
        }
    } visitor(penv, writer);
    writer->Begin();
    pdataObj->VisitMembers(penv->GetSC(), &visitor, 0);
    writer->End();
}


//////////////////////////////////////////
//


const GASNameFunction GASSharedObjectProto::FunctionTable[] = 
{
    { "clear",          &GASSharedObjectProto::Clear },
    { "flush",          &GASSharedObjectProto::Flush },
    { "getSize",        &GASSharedObjectProto::GetSize },
    { 0, 0 }
};


GASSharedObjectProto::GASSharedObjectProto(GASStringContext* psc, 
                                           GASObject* pprototype, 
                                           const GASFunctionRef& constructor) : 
GASPrototype<GASSharedObject>(psc, pprototype, constructor)
{
    InitFunctionMembers (psc, FunctionTable);
}


//
// public clear() : Void
//
// Purges all the data from the shared object and deletes the shared object from the disk.
//
void GASSharedObjectProto::Clear(const GASFnCall &fn)
{
    CHECK_THIS_PTR(fn, SharedObject);
    GASSharedObject* pthis = (GASSharedObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    // Set a new data object (essentially clearing)
    GPtr<GASObject> pdataObj = *fn.Env->OperatorNew(fn.Env->GetBuiltin(GASBuiltin_Object));
    pthis->SetDataObject(fn.Env, pdataObj);

    // Write out
    GPtr<GFxSharedObjectManagerBase> psoMgr = fn.Env->GetMovieRoot()->GetSharedObjectManager();
    if (psoMgr)
    {
        GPtr<GFxSharedObjectVisitor> pwriter = *psoMgr->CreateWriter(pthis->GetName(), 
            pthis->GetLocalPath(), fn.Env->GetMovieRoot()->GetFileOpener());
       pthis->Flush(fn.Env, pwriter);
    }
}


//
// public flush([minDiskSpace:Number]) : Object
// 
// Immediately writes a locally persistent shared object to a local 
// file. If you don't use this method, Flash writes the shared object 
// to a file when the shared object session ends--that is, when the 
// SWF file is closed, that is when the shared object is 
// garbage-collected because it no longer has any references to it 
// or you call SharedObject.clear(). 
//
// Returns
// 
// Object - A Boolean value: true or false; or a string value of 
// "pending", as described in the following list:
// 
// * If the user has permitted local information storage for objects 
//   from this domain, and the amount of space allotted is sufficient 
//   to store the object, this method returns true. (If you have 
//   passed a value for minimumDiskSpace, the amount of space allotted 
//   must be at least equal to that value for true to be returned).
// * If the user has permitted local information storage for objects 
//   from this domain, but the amount of space allotted is not 
//   sufficient to store the object, this method returns "pending".
// * If the user has permanently denied local information storage for 
//   objects from this domain, or if Flash cannot save the object for 
//   any reason, this method returns false.
//
void GASSharedObjectProto::Flush(const GASFnCall &fn)
{
    CHECK_THIS_PTR(fn, SharedObject);
    GASSharedObject* pthis = (GASSharedObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    // Write out
    GPtr<GFxSharedObjectManagerBase> psoMgr = fn.Env->GetMovieRoot()->GetSharedObjectManager();
    if (psoMgr)
    {
        GPtr<GFxSharedObjectVisitor> pwriter = *psoMgr->CreateWriter(pthis->GetName(), 
            pthis->GetLocalPath(), fn.Env->GetMovieRoot()->GetFileOpener());
        pthis->Flush(fn.Env, pwriter);
    }
}


//
// public getSize() : Number
// 
// Gets the current size of the shared object, in bytes. 
//
// Returns
// 
// Number - A numeric value specifying the size of the shared object,
// in bytes.
//
void GASSharedObjectProto::GetSize(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, SharedObject);
    GASSharedObject* pthis = (GASSharedObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    fn.Result->SetUInt(UInt(pthis->ComputeSizeInBytes(fn.Env)));
}


//////////////////


const GASNameFunction GASSharedObjectCtorFunction::StaticFunctionTable[] = 
{
    { "getLocal",    &GASSharedObjectCtorFunction::GetLocal },
    { 0, 0 }
};


GASSharedObjectCtorFunction::GASSharedObjectCtorFunction(GASStringContext *psc)
:   GASCFunctionObject(psc, GlobalCtor)
{
    GASNameFunction::AddConstMembers(this, psc, StaticFunctionTable, 
        GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | 
        GASPropFlags::PropFlag_DontEnum);
}


#ifndef GFC_NO_GC
template <class Functor>
void GASSharedObjectCtorFunction::ForEachChild_GC(Collector* prcc) const
{
    GASObject::template ForEachChild_GC<Functor>(prcc);
    SOLookupType::ConstIterator it = SharedObjects.Begin();
    while(it != SharedObjects.End())
    {   
        const GASSharedObjectPtr& pobj = it->Second;
        pobj.template ForEachChild_GC<Functor>(prcc);
        ++it;
    }
}
GFC_GC_PREGEN_FOR_EACH_CHILD(GASSharedObjectCtorFunction)

void GASSharedObjectCtorFunction::ExecuteForEachChild_GC(Collector* prcc, OperationGC operation) const
{
    GASRefCountBaseType::CallForEachChild<GASSharedObjectCtorFunction>(prcc, operation);
}

void GASSharedObjectCtorFunction::Finalize_GC()
{
    SharedObjects.~SOLookupType();
    GASObject::Finalize_GC();
}
#endif //GFC_NO_GC


//
// public static getLocal(name:String, [localPath:String], [secure:Boolean]) : SharedObject
//
// Returns a reference to a locally persistent shared object that is 
// available only to the current client. If the shared object does 
// not already exist, this method creates one. This method is a static 
// method of the SharedObject class.
//
// Returns
//
// SharedObject - A reference to a shared object that is persistent 
// locally and is available only to the current client. If Flash 
// Player can't create or find the shared object (for example, if 
// localPath was specified but no such directory exists, or if the 
// secure parameter is used incorrectly) this method returns null. 
//
void GASSharedObjectCtorFunction::GetLocal(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
        return;

    GASString name = fn.Arg(0).ToString(fn.Env);
    GASString localPath = fn.Env->CreateConstString("");
    if (fn.NArgs > 1)
        localPath = fn.Arg(1).ToString(fn.Env);
    GString strName(name.ToCStr());
    GString strLocalPath(localPath.ToCStr());

    // Check for stored shared obj
    GASString strFullPath = fn.Env->CreateString(strLocalPath);
    strFullPath += ":";
    strFullPath += strName;
    GASFunctionRef fref = fn.Env->GetConstructor(GASBuiltin_SharedObject);
    GASSERT(fref.Function);
    GASSharedObjectCtorFunction* pctorobj = static_cast<GASSharedObjectCtorFunction*>(fref.Function);
    GASSharedObjectPtr* psavedObj = pctorobj->SharedObjects.Get(strFullPath);
    if (psavedObj)
    {
        // Found a stored shared obj; return it
        fn.Result->SetAsObject(*psavedObj);
        return;
    }

    GPtr<GASObject> pobj = *fn.Env->OperatorNew(fn.Env->GetBuiltin(GASBuiltin_SharedObject));
    GASSharedObject* pthis = static_cast<GASSharedObject*>(pobj.GetPtr());

    // If name and directory aren't formatted correctly, fail
    if (!pthis->SetNameAndLocalPath(strName, strLocalPath))
    {
        fn.Result->SetNull();
        return;
    }

    GPtr<GASObject> pdataObj = *fn.Env->OperatorNew(fn.Env->GetBuiltin(GASBuiltin_Object));
    GASSharedObjectLoader   loader(fn.Env, pdataObj);

    GPtr<GFxSharedObjectManagerBase> psoMgr = fn.Env->GetMovieRoot()->GetSharedObjectManager();
    GFxFileOpenerBase*  pfileopener = fn.Env->GetMovieRoot()->GetFileOpener();
    if (!psoMgr || !psoMgr->LoadSharedObject(strName, strLocalPath, &loader, pfileopener))
    {
        fn.Result->SetNull();
        return;
    }
    else
    {
        pthis->SetDataObject(fn.Env, pdataObj);
        fn.Result->SetAsObject(pthis);
        // Store for future lookup
        pctorobj->SharedObjects.Add(strFullPath, pthis);
    }
}


GASObject* GASSharedObjectCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{
    return GHEAP_NEW(penv->GetHeap()) GASSharedObject(penv);
}


void GASSharedObjectCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASSharedObject> ab = *GHEAP_NEW(fn.Env->GetHeap()) GASSharedObject(fn.Env);
    fn.Result->SetAsObject(ab.GetPtr());
}


GASFunctionRef GASSharedObjectCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef  ctor(*GHEAP_NEW(pgc->GetHeap()) GASSharedObjectCtorFunction(&sc));
    GPtr<GASObject> proto = *GHEAP_NEW(pgc->GetHeap()) GASSharedObjectProto(&sc, 
        pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_SharedObject, proto);
    pgc->pGlobal->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_SharedObject), GASValue(ctor));
    return ctor;
}


#endif  // GFC_NO_FXPLAYER_AS_SHAREDOBJECT
