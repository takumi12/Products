/**********************************************************************

Filename    :   GFxValueImpl.cpp
Content     :   Complex Objects API Implementation
Created     :   
Authors     :   Prasad Silva

Copyright   :   (c) 2009 Scaleform Corp. All Rights Reserved.

Notes       :   Contains GFxMovieRoot, GFxValue::ObjectInterface and 
                GFxValue method definitions.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include <GFxPlayer.h>
#include "GFxPlayerImpl.h"
#include "GFxSprite.h"
#include "GFxText.h"
#include "AS/GASObject.h"
#include "AS/GASArrayObject.h"
#include "AMP/GFxAmpViewStats.h"

// Helper macro to determine the offset of a member. The 16 value is an 
// arbitrary number used as a pointer. The commonly used pointer, NULL,
// produces warnings on some compilers, so 16 is used.
#define OFFSETOF(st, m) ((size_t) ( (char *)&((st *)(16))->m - (char *)16 ))


// ***** Custom AS2 function object

// Custom GASFunctionObject that wraps the function context as well as
// the user data.
class GASUserDefinedFunctionObject : public GASFunctionObject
{
    friend class GASFunctionProto;
    friend class GASFunctionCtorFunction;
    friend class GASCustomFunctionObject;
    friend class GASFunctionRefBase;
protected:
    GPtr<GFxFunctionHandler>            pContext;
    void*                               pUserData;

    GASUserDefinedFunctionObject(GASStringContext* psc) : GASFunctionObject(psc) { GUNUSED(psc); }
    GASUserDefinedFunctionObject(GASEnvironment* penv) : GASFunctionObject(penv) { GUNUSED(penv); }

#ifndef GFC_NO_GC
    
    // PPS: We may need to link up ForEachChild_GC via a visitor interface
    //      later on to somewhat automate the cleaning up of any GFxValues 
    //      held by the function context.
    
    virtual void Finalize_GC()
    {
        pContext = NULL;
        GASFunctionObject::Finalize_GC();
    }
#endif // GFC_NO_GC
public:
    GASUserDefinedFunctionObject(GASStringContext* psc, GFxFunctionHandler* pcontext, void* puserData) 
        : GASFunctionObject(psc), pContext(pcontext), pUserData(puserData)
    {
        Set__proto__(psc, psc->pContext->GetPrototype(GASBuiltin_Function));
    }
    virtual ~GASUserDefinedFunctionObject() {}

    virtual GASEnvironment* GetEnvironment(const GASFnCall& fn, GPtr<GFxASCharacter>* ptargetCh) 
    {
        GUNUSED(ptargetCh);
        return fn.Env;
    }
    virtual void Invoke (const GASFnCall& fn, GASLocalFrame*, const char* pmethodName);

    virtual bool IsNull () const { return 0 == pContext; }
    virtual bool IsEqual(const GASFunctionObject& f) const 
    { 
        GUNUSED(f); 
        return false;   // AB wants it returning false
    }

    // returns number of arguments expected by this function;
    // returns -1 if number of arguments is unknown (for C functions)
    int     GetNumArgs() const { return -1; }

private:
    void    InvokeImpl(const GASFnCall& fn);
};


void GASUserDefinedFunctionObject::Invoke(const GASFnCall& fn, GASLocalFrame*, const char*) 
{    
    if (0 != pContext) {
        GASObjectInterface* pthis   = fn.ThisPtr;
        if (pthis && pthis->IsSuper())
        {
            GASObjectInterface* prealThis = static_cast<GASSuperObject*>(pthis)->GetRealThis();
            GASFnCall fn2(fn.Result, prealThis, fn.Env, fn.NArgs, fn.FirstArgBottomIndex);
            InvokeImpl(fn2);
            static_cast<GASSuperObject*>(pthis)->ResetAltProto(); // reset alternative proto, if set
        }
        else
            InvokeImpl(fn);
    }
}

void GASUserDefinedFunctionObject::InvokeImpl(const GASFnCall& fn)
{
    GASEnvironment* penv = fn.Env;
    GASSERT(penv);
    GArrayCPP<GFxValue> args;

    GFxValue thisVal, retVal;
    GASValue thisAS;
    if (fn.ThisPtr) thisAS.SetAsObjectInterface(fn.ThisPtr);
    else thisAS.SetNull();
    penv->GetMovieRoot()->ASValue2GFxValue(penv, thisAS, &thisVal);
    args.PushBack(thisVal);  // Used to pack 'this' on callback

    for (int i=0; i < fn.NArgs; i++)
    {
        GFxValue arg;
        penv->GetMovieRoot()->ASValue2GFxValue(penv, fn.Arg(i), &arg);
        args.PushBack(arg);
    }

    GFxFunctionHandler::Params params;
    params.pMovie               = penv->GetMovieRoot();
    params.pRetVal              = &retVal;
    params.pThis                = &thisVal;
    params.pArgs                = fn.NArgs > 0 ? &args[1] : NULL;
    params.pArgsWithThisRef     = &args[0];
    params.ArgCount             = (UInt)args.GetSize() - 1;
    params.pUserData            = pUserData;
    pContext->Call(params);

    if (!retVal.IsUndefined()) 
        penv->GetMovieRoot()->GFxValue2ASValue(retVal, fn.Result);
}


// ***** GFxMovieRoot definitions

// Convert a GFxValue to an internal GASValue representation
void GFxMovieRoot::GFxValue2ASValue(const GFxValue& gfxVal, GASValue* pdestVal) const
{   
    GASSERT(pdestVal);
    switch(gfxVal.GetType())
    {
    case GFxValue::VT_Undefined: pdestVal->SetUndefined(); break;
    case GFxValue::VT_Boolean:   pdestVal->SetBool(gfxVal.GetBool()); break;
    case GFxValue::VT_Null:      pdestVal->SetNull(); break;
    case GFxValue::VT_Number:    pdestVal->SetNumber(gfxVal.GetNumber()); break;
    case GFxValue::VT_String:    
        {
            if (gfxVal.IsManagedValue())
            {
                size_t memberOffset = OFFSETOF(GASStringNode, pData);
                // Must be an assignment of a string from the same movie root
                GASSERT(gfxVal.pObjectInterface->IsSameContext(pObjectInterface));
                GASStringNode* strnode = (GASStringNode*)(((UByte*)gfxVal.Value.pStringManaged) - memberOffset);
                GASString str(strnode);
                pdestVal->SetString(str);
            }
            else
                pdestVal->SetString(pGlobalContext->CreateString(gfxVal.GetString()));
        }
        break;
    case GFxValue::VT_StringW:    
        {
            if (gfxVal.IsManagedValue())
            {                
                size_t memberOffset = OFFSETOF(GFxMovieRoot::WideStringStorage, pData);
                // Must be an assignment of a string from the same movie root
                GASSERT(gfxVal.pObjectInterface->IsSameContext(pObjectInterface));
                WideStringStorage* wstr = (WideStringStorage*)(((UByte*)gfxVal.Value.pStringW) - memberOffset);
				if(wstr->pNode == nullptr)
				{
					return;
				}
				GASString str(wstr->pNode);
				pdestVal->SetString(str);
            }
            else 
                pdestVal->SetString(pGlobalContext->CreateString(gfxVal.GetStringW()));
        }
        break;
    case GFxValue::VT_Array:
    case GFxValue::VT_Object:
        {
            // Must be an assignment of an object from the same movie root
            GASSERT(gfxVal.pObjectInterface->IsSameContext(pObjectInterface));
            GASObjectInterface* pifc = (GASObjectInterface*)gfxVal.Value.pData;
            pdestVal->SetAsObject((GASObject*) pifc);
        }
        break;
    case GFxValue::VT_DisplayObject:
        {
            // Must be an assignment of an object from the same movie root
            GASSERT(gfxVal.pObjectInterface->IsSameContext(pObjectInterface));
            GFxCharacterHandle* pifc = (GFxCharacterHandle*)gfxVal.Value.pData;
            pdestVal->SetAsCharacterHandle(pifc);
        }
        break;
    default:
        break;
    }
}

// Convert a GASValue to GFxValue representation
void GFxMovieRoot::ASValue2GFxValue(GASEnvironment* penv, const GASValue& value, GFxValue* pdestVal) const
{
    GASSERT(pdestVal);
    int toType;
    if (pdestVal->GetType() & GFxValue::VTC_ConvertBit)
        toType = pdestVal->GetType() & (~GFxValue::VTC_ConvertBit);
    else
    {
        switch(value.GetType())
        {
        case GASValue::UNSET:
        case GASValue::UNDEFINED:    toType = GFxValue::VT_Undefined; break;
        case GASValue::BOOLEAN:      toType = GFxValue::VT_Boolean; break;
        case GASValue::NULLTYPE:     toType = GFxValue::VT_Null; break;
        case GASValue::INTEGER:
        case GASValue::NUMBER:       toType = GFxValue::VT_Number; break;
        case GASValue::OBJECT:       
        case GASValue::FUNCTION:
                                     toType = GFxValue::VT_Object; break;            
        case GASValue::CHARACTER:    toType = GFxValue::VT_DisplayObject; break;
        // GASValue::PROPERTY is returned as a string 
        default:                     toType = GFxValue::VT_String; break;
        }
    }

    // Clear existing ref
    if (pdestVal->IsManagedValue()) pdestVal->ReleaseManagedValue();

    switch(toType)
    {
    case GFxValue::VT_Undefined:
    case GFxValue::VT_Null:
        {
            pdestVal->Type = GFxValue::ValueType(toType);
        }
        break;
    case GFxValue::VT_Boolean:      
        {
            pdestVal->Type = GFxValue::ValueType(toType);
            pdestVal->Value.BValue = value.ToBool(penv);
        }
        break;
    case GFxValue::VT_Number:
        {
            pdestVal->Type = GFxValue::ValueType(toType);
            pdestVal->Value.NValue = value.ToNumber(penv);
        }
        break;
    case GFxValue::VT_String:    
        {
            size_t memberOffset = OFFSETOF(GASStringNode, pData);
            GASString str = value.ToString(penv);
            GASStringNode* strNode = str.GetNode();
            pdestVal->Type = GFxValue::ValueType(GFxValue::VT_String | GFxValue::VTC_ManagedBit);
            pdestVal->Value.pStringManaged = (const char**)(((UByte*)strNode) + memberOffset);
            pdestVal->pObjectInterface = pObjectInterface;
            pObjectInterface->ObjectAddRef(pdestVal, pdestVal->Value.pData);
        }
        break;
    case GFxValue::VT_StringW:    
        {   
            size_t memberOffset = OFFSETOF(GFxMovieRoot::WideStringStorage, pData);
            GASString str = value.ToString(penv);
            GASStringNode* strNode = str.GetNode();
            UInt len = str.GetLength() + 1;
            void* pdata = GHEAP_ALLOC(pHeap, sizeof(WideStringStorage) - sizeof(UByte) + (sizeof(wchar_t) * len), GFxStatMV_Other_Mem);            
            pdestVal->Type = GFxValue::ValueType(GFxValue::VT_StringW | GFxValue::VTC_ManagedBit);
            GPtr<WideStringStorage> wstr = *::new(pdata) WideStringStorage(strNode, len);
            pdestVal->Value.pStringW = (const wchar_t*)(((UByte*)wstr.GetPtr()) + memberOffset);
            pdestVal->pObjectInterface = pObjectInterface;
            pObjectInterface->ObjectAddRef(pdestVal, pdestVal->Value.pData);
        }
        break;
    case GFxValue::VT_Object:
        {
            GFxValue::ValueType type = GFxValue::VT_Object;
            GASObjectInterface* pifc = (GASObjectInterface*)value.ToObjectInterface(penv);
            if (pifc->IsASObject())
            {
                GASObject* pobj = pifc->ToASObject();
                if (pobj->GetObjectType() == GASObjectInterface::Object_Array)
                    type = GFxValue::VT_Array;                
            }
            pdestVal->Type = GFxValue::ValueType(type | GFxValue::VTC_ManagedBit);
            pdestVal->Value.pData = pifc;
            pdestVal->pObjectInterface = pObjectInterface;
            pObjectInterface->ObjectAddRef(pdestVal, pdestVal->Value.pData);
        }
        break;
    case GFxValue::VT_DisplayObject:
        {
            GASObjectInterface* pifc = (GASObjectInterface*)value.ToObjectInterface(penv);
            GFxASCharacter* aschar = pifc->ToASCharacter();
            pdestVal->Type = GFxValue::ValueType(GFxValue::VT_DisplayObject | GFxValue::VTC_ManagedBit);
            pdestVal->Value.pData = aschar->GetCharacterHandle();
            pdestVal->pObjectInterface = pObjectInterface;
            pObjectInterface->ObjectAddRef(pdestVal, pdestVal->Value.pData);
        }
    }
}

void GFxMovieRoot::CreateString(GFxValue* pvalue, const char* pstring)
{
    GASEnvironment* penv = pLevel0Movie->GetASEnvironment();
    GASStringContext* psc = penv->GetSC();
    GASString asstr = psc->CreateString(pstring);
    ASValue2GFxValue(penv, asstr, pvalue);
}

void GFxMovieRoot::CreateStringW(GFxValue* pvalue, const wchar_t* pstring)
{
    GASEnvironment* penv = pLevel0Movie->GetASEnvironment();
    GASStringContext* psc = penv->GetSC();
    GASString asstr = psc->CreateString(pstring);
    pvalue->SetConvertStringW();
    ASValue2GFxValue(penv, asstr, pvalue);
}

void GFxMovieRoot::CreateObject(GFxValue* pvalue, const char* className, const GFxValue* pargs, UInt nargs)
{
    GASEnvironment* penv = pLevel0Movie->GetASEnvironment();
    GPtr<GASObject> pobj;

    if (className)
    {
        // Push params to stack
        if (nargs > 0)
        {
            for (SInt i=(nargs-1); i > -1; --i)
            {
                GASValue asval;
                GFxValue2ASValue(pargs[i], &asval);
                penv->Push(asval);
            }
        }

        // Check if the className is part of a package (has '.')
        const char* p = strchr(className, '.');
        if (p)
        {
            // For package resolution
            GASStringContext* psc = penv->GetSC();
            char            buf[256];
            const char*     pname = className;
            GPtr<GASObject> parent = penv->GetGC()->pGlobal;

            while(pname)
            {
                const char* p = strchr(pname, '.');
                UPInt sz;
                if (p)
                    sz = p++ - pname + 1;
                else
                    break;  // class name found

                GASSERT(sz <= sizeof(buf));
                if (sz > sizeof(buf)) 
                    sz = sizeof(buf);

                memcpy(buf, pname, sz-1);
                buf[sz-1] = '\0';

                pname = p;

                GASValue        pkgObjVal;
                GPtr<GASObject> pkgObj;
                GASString       memberName(psc->CreateString(buf));

                if (parent->GetMemberRaw(psc, memberName, &pkgObjVal))
                    pkgObj = pkgObjVal.ToObject(NULL);
                else
                {
                    pvalue->SetUndefined();
                    return; // Package in path not found; bail
                }

                parent = pkgObj;
            }
            
            pobj = *penv->OperatorNew(parent, penv->CreateString(pname), nargs);
        }
        else
            pobj = *penv->OperatorNew(penv->CreateString(className), nargs);
        
        if (pobj)
            ASValue2GFxValue(penv, GASValue(pobj), pvalue);
        else
            pvalue->SetUndefined();

        // Cleanup; pop params from stack
        if (nargs > 0)
            penv->Drop(nargs);
    }
    else
    {
        pobj = *penv->OperatorNew(penv->GetBuiltin(GASBuiltin_Object));
        ASValue2GFxValue(penv, GASValue(pobj), pvalue);
    }    
}

void GFxMovieRoot::CreateArray(GFxValue* pvalue)
{
    GASEnvironment* penv = pLevel0Movie->GetASEnvironment();
    GPtr<GASArrayObject> parr = *static_cast<GASArrayObject*>(penv->OperatorNew(penv->GetBuiltin(GASBuiltin_Array)));
    GASValue asval(parr);
    ASValue2GFxValue(penv, asval, pvalue);
}

void GFxMovieRoot::CreateFunction(GFxValue* pvalue, GFxFunctionHandler* pfc, void* puserData /* = NULL */)
{
    GASEnvironment* penv = pLevel0Movie->GetASEnvironment();
    GASStringContext* psc = penv->GetSC();
    GASValue asval;
    GPtr<GASUserDefinedFunctionObject> pfuncObj = *GHEAP_NEW(psc->GetHeap()) GASUserDefinedFunctionObject(psc, pfc, puserData);
    asval.SetAsFunction( GASFunctionRef(pfuncObj.GetPtr()) );
    ASValue2GFxValue(penv, asval, pvalue);
}

// ***** GFxValue definitions

GString GFxValue::ToString() const
{
    GString retVal;

    switch(GetType())
    {
    case VT_Undefined:
        {
            retVal = GString("undefined");
        }
        break;
    case VT_Null:
        {  
            retVal = GString("null");
        }
        break;
    case VT_Boolean:
        {  
            retVal = GString(Value.BValue ? "true" : "false");
        }
        break;
    case VT_Number:
        {
            char buf[GASNumberUtil::TOSTRING_BUF_SIZE];
            retVal = GString(GASNumberUtil::ToString(Value.NValue, buf, sizeof(buf))); // Precision is 10 by default.
        }
        break;
    case VT_String:
        {
            retVal = GString(GetString());
        }
        break;
    case VT_StringW:
        {
            retVal = GString(GetStringW());
        }
        break;
    case VT_Object:
    case VT_Array:
    case VT_DisplayObject:
        {               
            pObjectInterface->ToString(&retVal, *this);
        }
        break;
    default:
        {
            retVal = "<bad type>";
            GASSERT(0);
        }
    }
    return retVal;
}

const wchar_t* GFxValue::ToStringW(wchar_t* pwstr, UPInt len) const
{
    switch(GetType())
    {
    case VT_StringW:
        {
            G_wcscpy(pwstr, len, GetStringW());
            return pwstr;
        }
        break;
    default:
        {
            GUTF8Util::DecodeString(pwstr, ToString().ToCStr(), len);   
        }
        break;
    }
    return pwstr;
}


// ***** GFxValue::ObjectInterface definitions

#ifdef GFC_BUILD_DEBUG
void GFxValue::ObjectInterface::DumpTaggedValues() const
{
    GFC_DEBUG_MESSAGE(1, "** Begin Tagged GFxValues Dump **");
    const GFxValue* data = ExternalObjRefs.GetFirst();
    while (!ExternalObjRefs.IsNull(data))
    {
        const char* ptypestr = NULL;
        switch (data->GetType())
        {
        case GFxValue::VT_Array: ptypestr = "Array"; break;
        case GFxValue::VT_DisplayObject: ptypestr = "DispObj"; break;
        default: ptypestr = "Object";
        }
        GFC_DEBUG_MESSAGE2(1, "> [%s] : %p", ptypestr, data);
        data = ExternalObjRefs.GetNext(data);
    }
    GFC_DEBUG_MESSAGE(1, "** End Tagged GFxValues Dump **");
}
#endif

void    GFxValue::ObjectInterface::ObjectAddRef(GFxValue* val, void* pobj)
{
    // Addref complex object
    switch (val->GetType())
    {
    case GFxValue::VT_String:
        {
            // pobj points to const char**
            size_t memberOffset = OFFSETOF(GASStringNode, pData);
            GASStringNode* ps = (GASStringNode*)(((UByte*)pobj) - memberOffset);
            ps->AddRef();            
        }
        break;
    case GFxValue::VT_StringW:
        {
            // pobj points to const wchar_t*
            size_t memberOffset = OFFSETOF(GFxMovieRoot::WideStringStorage, pData);
            GFxMovieRoot::WideStringStorage* pws = (GFxMovieRoot::WideStringStorage*)(((UByte*)pobj) - memberOffset);
            pws->AddRef();            
        }
        break;
    case GFxValue::VT_DisplayObject:
        {
            GFxCharacterHandle* pifc = (GFxCharacterHandle*)pobj;
            pifc->AddRef();
        }
        break;
    case GFxValue::VT_Object:
    case GFxValue::VT_Array:
        {
            GASObjectInterface* pifc = (GASObjectInterface*)pobj;
            pifc->ToASObject()->AddRef();
        }
        break;
    default:
        GASSERT(0);
    }

#ifdef GFC_BUILD_DEBUG
    ExternalObjRefs.PushBack(val);
#endif
}
void    GFxValue::ObjectInterface::ObjectRelease(GFxValue* val, void* pobj)
{
    // NOTE: The next statements may cause an access violation exception
    // if the GFxMovie containing the AS object has been released. To
    // avoid this issue, make sure to release any GFxValues holding
    // references to AS objects before the movie that holds those objects
    // is released.

    switch (val->GetType())
    {
    case GFxValue::VT_String:
        {
            // pobj points to const char**
            size_t memberOffset = OFFSETOF(GASStringNode, pData);
            GASStringNode* ps = (GASStringNode*)(((UByte*)pobj) - memberOffset);
            ps->Release();
        }
        break;
    case GFxValue::VT_StringW:
        {
            // pobj points to const wchar_t*
            size_t memberOffset = OFFSETOF(GFxMovieRoot::WideStringStorage, pData);
            GFxMovieRoot::WideStringStorage* pws = (GFxMovieRoot::WideStringStorage*)(((UByte*)pobj) - memberOffset);
            pws->Release();
        }
        break;
    case GFxValue::VT_DisplayObject:
        {
            GFxCharacterHandle* pifc = (GFxCharacterHandle*)pobj;
            pifc->Release();
        }
        break;
    case GFxValue::VT_Object:
    case GFxValue::VT_Array:
        {
            GASObjectInterface* pifc = (GASObjectInterface*)pobj;
            pifc->ToASObject()->Release();
        }
        break;
    default:
        GASSERT(0);
    }

#ifdef GFC_BUILD_DEBUG
    ExternalObjRefs.Remove(val);
#endif
}

bool GFxValue::ObjectInterface::HasMember(void* pdata, const char* name, bool isdobj) const 
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_HasMember, Amp_Profile_Level_Low);
#endif

    GASObjectInterface* pthis = static_cast<GASObjectInterface*>(pdata);
    if (isdobj)
    {
        GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
        pthis = pch->ResolveCharacter(pMovieRoot);
        if (!pthis) return false;
    }
    GASValue asval;
    GASEnvironment* penv = pMovieRoot->pLevel0Movie->GetASEnvironment();
    GASStringContext* psc = penv->GetSC();

    // Look for member
    GASValue member;
    // PPS: We are using GetMember instead of FindMember because certain GASObjectInterface
    //      subclasses do not implement FindMember (such as Sprite). GetMember has guaranteed
    //      behavior.
	if (!pthis->GetMember(penv, psc->CreateConstString(name), &member))
    {
        // Member not found
        return false;
    }
    return true;
}

bool GFxValue::ObjectInterface::HashTableMember(void* pdata, func_member_value& funceTable, void* pClass, GFxValue* val, bool isdobj) const
{
	GASObjectInterface* pthis = static_cast<GASObjectInterface*>(pdata);
	if (!isdobj)
	{
		GASEnvironment* penv = pMovieRoot->pLevel0Movie->GetASEnvironment();
		if (pthis->GetTableMember(penv, funceTable, pClass, val))
		{
			return true;
		}
	}
	return false;
}

bool GFxValue::ObjectInterface::GetMember(void* pdata, const char* name, GFxValue* pval, bool isdobj) const
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_GetMember, Amp_Profile_Level_Low);
#endif

    GASObjectInterface* pthis = static_cast<GASObjectInterface*>(pdata);
    if (isdobj)
    {
        GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
        pthis = pch->ResolveCharacter(pMovieRoot);
        if (!pthis) 
        {
            if (pval) pval->SetUndefined();
            return false;
        }
    }
    GASValue asval;
    GASEnvironment* penv = pMovieRoot->pLevel0Movie->GetASEnvironment();

    // Look for member
    if (!pthis->GetMember(penv, penv->CreateString(name), &asval))
    {
        // Member not found
        if (pval) pval->SetUndefined();
        return false;
    }
    
    // Resolve property if required
    if (asval.IsProperty())
    {
        GASObjectInterface*  pobj = NULL;
        GFxASCharacter* paschar = NULL;
        if (pthis->IsASObject())
            pobj    = pthis->ToASObject();
        if (pthis->IsASCharacter())
            paschar = pthis->ToASCharacter();
        asval.GetPropertyValue(penv, paschar?(GASObjectInterface*)paschar:pobj, &asval);        
    }

    // Found the member
    pMovieRoot->ASValue2GFxValue(penv, asval, pval);
    return true;
}

bool GFxValue::ObjectInterface::SetMember(void* pdata, const char* name, const GFxValue& value, bool isdobj)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_SetMember, Amp_Profile_Level_Low);
#endif

    GASObjectInterface* pthis = static_cast<GASObjectInterface*>(pdata);
    if (isdobj)
    {
        GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
        pthis = pch->ResolveCharacter(pMovieRoot);
        if (!pthis) return false;
    }
    GASEnvironment* penv = pMovieRoot->pLevel0Movie->GetASEnvironment();

    GASValue asval;
    pMovieRoot->GFxValue2ASValue(value, &asval);

    // Call SetMember to resolve properties
    return pthis->SetMember(penv, penv->GetSC()->CreateString(name), asval);
}

bool GFxValue::ObjectInterface::Invoke(void* pdata, GFxValue* presult, const char* name, const GFxValue* pargs, UPInt nargs, bool isdobj)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_Invoke, Amp_Profile_Level_Low);
#endif

    GASObjectInterface* pthis = static_cast<GASObjectInterface*>(pdata);
    if (isdobj)
    {
        GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
        pthis = pch->ResolveCharacter(pMovieRoot);
        if (!pthis) return false;
    }
    GASEnvironment* penv = pMovieRoot->pLevel0Movie->GetASEnvironment();
    GASValue member, result;

    // Look for member
    if (!pthis->GetConstMemberRaw(penv->GetSC(), name, &member))
    {
        // Warn?
        return false;
    }

    GASValue asArg;
    for (SInt i=SInt(nargs-1); i > -1; i--)
    {
        pMovieRoot->GFxValue2ASValue(pargs[i], &asArg);
        penv->Push(asArg);
    }
    bool retVal = GAS_Invoke(member, &result, pthis, penv, UInt(nargs), penv->GetTopIndex(), NULL);
    penv->Drop(UInt(nargs));

    // Process the return value if needed
    if (presult)
        pMovieRoot->ASValue2GFxValue(penv, result, presult);

    return retVal;
}

bool GFxValue::ObjectInterface::DeleteMember(void* pdata, const char* name, bool isdobj)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_DeleteMember, Amp_Profile_Level_Low);
#endif

    GASObjectInterface* pthis = static_cast<GASObjectInterface*>(pdata);
    if (isdobj)
    {
        GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
        pthis = pch->ResolveCharacter(pMovieRoot);
        if (!pthis) return false;
    }
    GASEnvironment* penv = pMovieRoot->pLevel0Movie->GetASEnvironment();
    GASStringContext* psc = penv->GetSC();

    return pthis->DeleteMember(psc, GASString(psc->CreateConstString(name)));
}

void GFxValue::ObjectInterface::VisitMembers(void* pdata, GFxValue::ObjectVisitor* visitor, bool isdobj) const
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_VisitMembers, Amp_Profile_Level_Low);
#endif

    const GASObjectInterface* pthis = static_cast<const GASObjectInterface*>(pdata);
    if (isdobj)
    {
        GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
        pthis = pch->ResolveCharacter(pMovieRoot);
        if (!pthis) return;
    }
    GASEnvironment* penv = pMovieRoot->pLevel0Movie->GetASEnvironment();
    GASStringContext* psc = penv->GetSC();

    class VisitorProxy : public GASObjectInterface::MemberVisitor
    {
    public:
        VisitorProxy(GFxMovieRoot* pmovieRoot, GFxValue::ObjectVisitor* visitor) 
            : pMovieRoot(pmovieRoot), pVisitor(visitor) 
        {
            pEnv = pMovieRoot->pLevel0Movie->GetASEnvironment();
        }
        void    Visit(const GASString& name, const GASValue& val, UByte flags)
        {
            GUNUSED(flags);
            GFxValue v;
            pMovieRoot->ASValue2GFxValue(pEnv, val, &v);
            pVisitor->Visit(name.ToCStr(), v);
        };
    private:
        GFxMovieRoot*               pMovieRoot;
        GASEnvironment*             pEnv;
        GFxValue::ObjectVisitor*    pVisitor;
    } visitorProxy(pMovieRoot, visitor);

    pthis->VisitMembers(psc, &visitorProxy, GASObjectInterface::VisitMember_Prototype | GASObjectInterface::VisitMember_ChildClips);
}

UInt GFxValue::ObjectInterface::GetArraySize(void* pdata) const
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_GetArraySize, Amp_Profile_Level_Low);
#endif

    GASObjectInterface* pobj = static_cast<GASObjectInterface*>(pdata);
    const GASArrayObject* parr = static_cast<const GASArrayObject*>(pobj);

    return (UInt)parr->GetSize();
}

bool GFxValue::ObjectInterface::SetArraySize(void* pdata, UInt sz)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_SetArraySize, Amp_Profile_Level_Low);
#endif

    GASObjectInterface* pobj = static_cast<GASObjectInterface*>(pdata);
    GASArrayObject* parr = static_cast<GASArrayObject*>(pobj);

    parr->Resize(sz);
    return true;
}

bool GFxValue::ObjectInterface::GetElement(void* pdata, UInt idx, GFxValue *pval) const
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_GetElement, Amp_Profile_Level_Low);
#endif

    GASObjectInterface* pobj = static_cast<GASObjectInterface*>(pdata);
    const GASArrayObject* parr = static_cast<const GASArrayObject*>(pobj);

    pval->SetUndefined();
    if (idx >= (UInt)parr->GetSize()) return false;

    const GASValue* pelem = parr->GetElementPtr(idx);
    if (!pelem) return false;

    GASEnvironment* penv = pMovieRoot->pLevel0Movie->GetASEnvironment();
    pMovieRoot->ASValue2GFxValue(penv, *pelem, pval);
    return true;
}

bool GFxValue::ObjectInterface::SetElement(void* pdata, UInt idx, const GFxValue& value)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_SetElement, Amp_Profile_Level_Low);
#endif

    GASObjectInterface* pobj = static_cast<GASObjectInterface*>(pdata);
    GASArrayObject* parr = static_cast<GASArrayObject*>(pobj);

    GASValue asval;
    pMovieRoot->GFxValue2ASValue(value, &asval);
    parr->SetElementSafe(idx, asval);
    return true;
}

void GFxValue::ObjectInterface::VisitElements(void* pdata, GFxValue::ArrayVisitor* visitor, UInt idx, SInt count) const
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_VisitElements, Amp_Profile_Level_Low);
#endif

    GASObjectInterface* pobj = static_cast<GASObjectInterface*>(pdata);
    GASArrayObject* parr = static_cast<GASArrayObject*>(pobj);
    GASEnvironment* penv = pMovieRoot->pLevel0Movie->GetASEnvironment();
    GFxValue val;

    UInt sz = (UInt)parr->GetSize();
    if (idx >= sz) return;
    if (count < 0) { count = (sz - idx); }

    UInt eidx = G_Min(sz, idx + count);
    for (UInt i=idx; i < eidx; i++)
    {
        const GASValue* pelem = parr->GetElementPtr(i);
        if (pelem)
            pMovieRoot->ASValue2GFxValue(penv, *pelem, &val);
        else
            val.SetUndefined();
        visitor->Visit(i, val);
    }
}

bool GFxValue::ObjectInterface::PushBack(void *pdata, const GFxValue &value)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_PushBack, Amp_Profile_Level_Low);
#endif

    GASObjectInterface* pobj = static_cast<GASObjectInterface*>(pdata);
    GASArrayObject* parr = static_cast<GASArrayObject*>(pobj);

    GASValue asval;
    pMovieRoot->GFxValue2ASValue(value, &asval);
    parr->PushBack(asval);
    return true;
}

bool GFxValue::ObjectInterface::PopBack(void* pdata, GFxValue* pval)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_PopBack, Amp_Profile_Level_Low);
#endif

    GASObjectInterface* pobj = static_cast<GASObjectInterface*>(pdata);
    GASArrayObject* parr = static_cast<GASArrayObject*>(pobj);
    GASEnvironment* penv = pMovieRoot->pLevel0Movie->GetASEnvironment();

    int sz = parr->GetSize();
    if (sz <= 0) 
    {
        if (pval) pval->SetUndefined();
        return false;
    }

    if (pval) 
    {
        GASValue* pasval = parr->GetElementPtr(sz - 1);
        pMovieRoot->ASValue2GFxValue(penv, *pasval, pval);
    }

    parr->PopBack();
    return true;
}

bool GFxValue::ObjectInterface::RemoveElements(void *pdata, UInt idx, SInt count)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_RemoveElements, Amp_Profile_Level_Low);
#endif

    GASObjectInterface* pobj = static_cast<GASObjectInterface*>(pdata);
    GASArrayObject* parr = static_cast<GASArrayObject*>(pobj);

    UInt sz = (UInt)parr->GetSize();
    if (idx >= sz) return false;
    if (count < 0) { count = (sz - idx); }

    parr->RemoveElements(idx, G_Min(sz - idx, (UInt)count));
    return true;
}

bool GFxValue::ObjectInterface::GetDisplayMatrix(void* pdata, GMatrix2D* pmat) const
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_GetDisplayMatrix, Amp_Profile_Level_Low);
#endif

    GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
    GFxASCharacter* pchar = pch->ResolveCharacter(pMovieRoot);
    if (!pchar) return false;

    GMatrix2D m = pchar->GetMatrix();
    m.M_[1][2] = TwipsToPixels(m.M_[0][2]);
    m.M_[0][2] = TwipsToPixels(m.M_[1][2]);
    *pmat = m;

    return true;
}

bool GFxValue::ObjectInterface::SetDisplayMatrix(void* pdata, const GMatrix2D& mat)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_SetDisplayMatrix, Amp_Profile_Level_Low);
#endif

    GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
    GFxASCharacter* pchar = pch->ResolveCharacter(pMovieRoot);
    if (!pchar) return false;

    // Extra checks
    if (!mat.IsValid())
    {
        GFC_DEBUG_WARNING(1, "GFxValue::SetDisplayMatrix called with an invalid matrix.\n"
                             "At least one of the matrix values is invalid (non-finite).\n");
        return false;
    }

    GMatrix2D m = mat;
    m.M_[0][2] = PixelsToTwips(m.M_[0][2]);
    m.M_[1][2] = PixelsToTwips(m.M_[1][2]);
    pchar->SetMatrix(m);

    // Also set the geom data structure to reflect changes for the runtime
    GFxASCharacter::GeomDataType geomData;
    pchar->GetGeomData(geomData);
    geomData.X = int(mat.GetX());
    geomData.Y = int(mat.GetY());
    geomData.XScale = mat.GetXScale()*(Double)100.;
    geomData.YScale = mat.GetYScale()*(Double)100.;
    geomData.Rotation = (mat.GetRotation()*(Double)180.)/GFC_MATH_PI;
    pchar->SetGeomData(geomData);

    return true;
}

bool GFxValue::ObjectInterface::GetMatrix3D(void* pdata, GMatrix3D* pmat) const
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_GetMatrix3D, Amp_Profile_Level_Low);
#endif

#ifndef GFC_NO_3D
    GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
    GFxASCharacter* pchar = pch->ResolveCharacter(pMovieRoot);
    if (!pchar) return false;

    GMatrix3D m = pchar->GetMatrix3D() ? *pchar->GetMatrix3D() : GMatrix3D::Identity;
    m.SetX(TwipsToPixels(m.GetX()));
    m.SetY(TwipsToPixels(m.GetY()));
    *pmat = m;

    return true;
#else
    GUNUSED2(pdata, pmat);
    return false;
#endif
}

bool GFxValue::ObjectInterface::SetMatrix3D(void* pdata, const GMatrix3D& mat)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_SetMatrix3D, Amp_Profile_Level_Low);
#endif

#ifndef GFC_NO_3D
    GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
    GFxASCharacter* pchar = pch->ResolveCharacter(pMovieRoot);
    if (!pchar) return false;

    // Extra checks
    if (!mat.IsValid())
    {
        GFC_DEBUG_WARNING(1, "GFxValue::SetDisplayMatrix called with an invalid matrix.\n"
            "At least one of the matrix values is invalid (non-finite).\n");
        return false;
    }

    GMatrix3D m = mat;
//    m.SetX(PixelsToTwips(m.GetX()));
//    m.SetY(PixelsToTwips(m.GetY()));
    pchar->SetMatrix3D(m);

    // Also set the geom data structure to reflect changes for the runtime
    GFxASCharacter::GeomDataType geomData;
    pchar->GetGeomData(geomData);
    geomData.Z = mat.GetZ();
    geomData.ZScale = mat.GetZScale()*(Double)100.;

    float xr, yr;
    mat.GetRotation(&xr, &yr, NULL);
    geomData.XRotation = GFC_RADTODEG(xr);
    geomData.YRotation = GFC_RADTODEG(yr);

    pchar->SetGeomData(geomData);

    return true;
#else
    GUNUSED2(pdata, mat);
    return false;
#endif
}

bool    GFxValue::ObjectInterface::IsDisplayObjectActive(void* pdata) const
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_IsDisplayObjectActive, Amp_Profile_Level_Low);
#endif

    GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
    GFxASCharacter* pchar = pch->ResolveCharacter(pMovieRoot);
    return pchar != NULL;
}

bool GFxValue::ObjectInterface::GetDisplayInfo(void* pdata, DisplayInfo* pinfo) const
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_GetDisplayInfo, Amp_Profile_Level_Low);
#endif

    // The getter code was lifted from GFxASCharacter::GetStandardMember

    GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
    GFxASCharacter* pchar = pch->ResolveCharacter(pMovieRoot);
    if (!pchar) return false;

    GFxASCharacter::GeomDataType geomData;
    pchar->GetGeomData(geomData);
    if (pchar->GetObjectType() == GASObjectInterface::Object_TextField)
    {
        // Special case for TextField
        GASTextField* ptf = static_cast<GASTextField*>(pchar);
        ptf->GetPosition(pinfo);
    }
    else
    {
        Double x = TwipsToPixels(Double(geomData.X));
        Double y = TwipsToPixels(Double(geomData.Y));
        Double rotation = geomData.Rotation;
        Double xscale = geomData.XScale;
        Double yscale = geomData.YScale;
        Double alpha = pchar->GetCxform().M_[3][0] * 100.F;
        bool visible = pchar->GetVisible();
#ifndef GFC_NO_3D
        Double xrotation = geomData.XRotation;
        Double yrotation = geomData.YRotation;
        Double zscale = geomData.ZScale;
        Double z = geomData.Z;
        pinfo->Set(x, y, rotation, xscale, yscale, alpha, visible, z, xrotation, yrotation, zscale);
        pinfo->SetPerspFOV(pchar->GetPerspectiveFOV());        
        pinfo->SetPerspective3D(pchar->GetPerspective3D());
        pinfo->SetView3D(pchar->GetView3D());
#else
        pinfo->Set(x, y, rotation, xscale, yscale, alpha, visible, 0,0,0,0);
#endif
    }    
    return true;
}

#ifndef GFC_NO_3D
void GFxValue_UpdateTransform(GFxASCharacter* pchar)
{
    GMatrix3D rotXMat, transMat, scaleMat, rotYMat;

    // build new matrix
    transMat.SetZ((Float)pchar->pGeomData->Z);
    scaleMat.SetZScale((Float)pchar->pGeomData->ZScale/100.f);

    rotXMat.RotateX((Float)GFC_DEGTORAD(pchar->pGeomData->XRotation));
    rotYMat.RotateY((Float)GFC_DEGTORAD(pchar->pGeomData->YRotation));

    // Apply transforms in SRT order
    GMatrix3D m(scaleMat);
    m.Append(rotXMat);
    m.Append(rotYMat);
    m.Append(transMat);

    if (m.IsValid())
        pchar->SetMatrix3D(m);
}
#endif

bool GFxValue::ObjectInterface::SetDisplayInfo(void *pdata, const DisplayInfo &cinfo)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_SetDisplayInfo, Amp_Profile_Level_Low);
#endif

    // The getter code was lifted from GFxASCharacter::SetStandardMember, however
    // some logic was compressed for efficiency (e.g.: set the matrix once, instead
    // of 5 different times - worst case)

    GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
    GFxASCharacter* pchar = pch->ResolveCharacter(pMovieRoot);
    if (!pchar) return false;

    // Check for TextField special case:
    bool istf = (pchar->GetObjectType() == GASObjectInterface::Object_TextField);
    GASTextField* ptf = static_cast<GASTextField*>(pchar);

    if (cinfo.IsFlagSet(DisplayInfo::V_alpha))
    {
        if (!GASNumberUtil::IsNaN(cinfo.GetAlpha()))
        {
            // Set alpha modulate, in percent.
            GFxASCharacter::Cxform  cx = pchar->GetCxform();
            cx.M_[3][0] = Float(cinfo.GetAlpha() / 100.);
            pchar->SetCxform(cx);
            pchar->SetAcceptAnimMoves(0);
        }
    }
    if (cinfo.IsFlagSet(DisplayInfo::V_visible))
    {
        pchar->SetVisible(cinfo.GetVisible());
    }

#ifndef GFC_NO_3D
    bool UpdateTransforms = false;

    if (cinfo.IsFlagSet(DisplayInfo::V_z))
    {
        GASNumber zval = cinfo.GetZ();
        if (GASNumberUtil::IsNaN(zval))
            zval = 0;
        if (GASNumberUtil::IsNEGATIVE_INFINITY(zval) || GASNumberUtil::IsPOSITIVE_INFINITY(zval))
            zval = 0;

        pchar->EnsureGeomDataCreated(); // let timeline anim continue
        GASSERT(pchar->pGeomData);

        if (zval != pchar->pGeomData->Z)
        {
            pchar->pGeomData->Z = zval;

            UpdateTransforms = true;
        }
    }

    if (cinfo.IsFlagSet(DisplayInfo::V_zscale))
    {
        GASNumber newZScale = cinfo.GetZScale();
        if (GASNumberUtil::IsNaN(newZScale) ||
            GASNumberUtil::IsNEGATIVE_INFINITY(newZScale) || GASNumberUtil::IsPOSITIVE_INFINITY(newZScale))
        {
            newZScale = 100.;
        }

        pchar->EnsureGeomDataCreated(); // let timeline anim continue
        GASSERT(pchar->pGeomData);

        if (pchar->pGeomData->ZScale != newZScale)
        {
            pchar->pGeomData->ZScale = newZScale;

            UpdateTransforms = true;
        }
    }

    if (cinfo.IsFlagSet(DisplayInfo::V_xrotation))
    {
        GASNumber rval = cinfo.GetXRotation();

        pchar->EnsureGeomDataCreated(); // let timeline anim continue
        GASSERT(pchar->pGeomData);

        if (rval != pchar->pGeomData->XRotation)
        {       
            Double r = fmod((Double)rval, (Double)360.);
            if (r > 180)
                r -= 360;
            else if (r < -180)
                r += 360;

            pchar->pGeomData->XRotation = r;

            UpdateTransforms = true;
        }
    }

    if (cinfo.IsFlagSet(DisplayInfo::V_yrotation))
    {
        GASNumber rval = cinfo.GetYRotation();

        pchar->EnsureGeomDataCreated(); // let timeline anim continue
        GASSERT(pchar->pGeomData);

        if (rval != pchar->pGeomData->YRotation)
        {      
            Double r = fmod((Double)rval, (Double)360.);
            if (r > 180)
                r -= 360;
            else if (r < -180)
                r += 360;

            pchar->pGeomData->YRotation= r;

            UpdateTransforms = true;
        }
    }

    if (UpdateTransforms)
        GFxValue_UpdateTransform(pchar);

    if (cinfo.IsFlagSet(DisplayInfo::V_perspFOV))
    {
        GASNumber rval = cinfo.GetPerspFOV();
        if (rval != pchar->GetPerspectiveFOV())
        {       
            Double r = fmod((Double)rval, (Double)180.);
            pchar->SetPerspectiveFOV((float)r);
        }
    }

    if (cinfo.IsFlagSet(DisplayInfo::V_perspMatrix3D))
    {
        pchar->SetPerspective3D(*cinfo.GetPerspective3D());
    }

    if (cinfo.IsFlagSet(DisplayInfo::V_viewMatrix3D))
    {
        pchar->SetView3D(*cinfo.GetView3D());
    }
#endif

    if (cinfo.IsFlagSet(DisplayInfo::V_x) || cinfo.IsFlagSet(DisplayInfo::V_y) || cinfo.IsFlagSet(DisplayInfo::V_rotation) || 
        cinfo.IsFlagSet(DisplayInfo::V_xscale) || cinfo.IsFlagSet(DisplayInfo::V_yscale))
    {       
        // Special case for TextField:
        if (istf && (cinfo.IsFlagSet(DisplayInfo::V_xscale) || 
                     cinfo.IsFlagSet(DisplayInfo::V_yscale) || 
                     cinfo.IsFlagSet(DisplayInfo::V_rotation)))
            ptf->SetNeedToUpdateGeomData();

        pchar->SetAcceptAnimMoves(0);
        GASSERT(pchar->pGeomData);

        GFxASCharacter::Matrix  m = pchar->GetMatrix();

        GPointD pt;
        // Special case for TextField:
        if (istf && (cinfo.IsFlagSet(DisplayInfo::V_x) || cinfo.IsFlagSet(DisplayInfo::V_y)))
        {
            GPointF tpt = ptf->TransformToTextRectSpace(cinfo);
            pt.x = tpt.x;
            pt.y = tpt.y;
        }
        else
        {
            if (cinfo.IsFlagSet(GFxValue::DisplayInfo::V_x))
                pt.x = cinfo.GetX();
            if (cinfo.IsFlagSet(GFxValue::DisplayInfo::V_y))
                pt.y = cinfo.GetY();
        }

        if (cinfo.IsFlagSet(DisplayInfo::V_rotation) || cinfo.IsFlagSet(DisplayInfo::V_xscale) || cinfo.IsFlagSet(DisplayInfo::V_yscale))
        {
            GFxASCharacter::Matrix om = pchar->pGeomData->OrigMatrix;
            om.M_[0][2] = m.M_[0][2];
            om.M_[1][2] = m.M_[1][2];

            Double origRotation = om.GetRotation();
            Double origXScale = om.GetXScale();
            Double origYScale = om.GetYScale();

            Double newXScale = pchar->pGeomData->XScale / 100;
            Double newYScale = pchar->pGeomData->YScale / 100;
            Double newRotation = GFC_DEGTORAD(pchar->pGeomData->Rotation);

            // _rotation
            GASNumber rval = cinfo.IsFlagSet(DisplayInfo::V_rotation) ? cinfo.GetRotation() : GASNumberUtil::NaN();
            if (!GASNumberUtil::IsNaN(rval))
            {
                newRotation = fmod((Double)rval, (Double)360.);
                if (newRotation > 180)
                    newRotation -= 360;
                else if (newRotation < -180)
                    newRotation += 360;
                pchar->pGeomData->Rotation = newRotation;
                newRotation = GFC_DEGTORAD(newRotation); // Convert to rads
            }
            // _xscale
            Double nx = cinfo.IsFlagSet(DisplayInfo::V_xscale) ? (cinfo.GetXScale()/100) : GASNumberUtil::NaN();
            if (!(GASNumberUtil::IsNaN(nx) || 
                GASNumberUtil::IsNEGATIVE_INFINITY(nx) || GASNumberUtil::IsPOSITIVE_INFINITY(nx)) && (nx != newXScale))
            {
                pchar->pGeomData->XScale = cinfo.GetXScale();
                if (origXScale == 0 || nx > 1E+16)
                {
                    nx = 0;
                    origXScale = 1;
                }
                newXScale = nx;
            }
            // _yscale
            Double ny = cinfo.IsFlagSet(DisplayInfo::V_yscale) ? (cinfo.GetYScale()/100) : GASNumberUtil::NaN();
            if (!(GASNumberUtil::IsNaN(ny) || 
                GASNumberUtil::IsNEGATIVE_INFINITY(ny) || GASNumberUtil::IsPOSITIVE_INFINITY(ny)) && (ny != newYScale))
            {                
                pchar->pGeomData->YScale = cinfo.GetYScale();
                if (origYScale == 0 || ny > 1E+16)
                {
                    ny = 0;
                    origYScale = 1;
                }
                newYScale = ny;
            }              

            // remove old rotation by rotate back and add new one
            GFxASCharacter_MatrixScaleAndRotate2x2(om,
                Float(newXScale/origXScale), 
                Float(newYScale/origYScale),
                Float(newRotation - origRotation));

            m = om;
        }



        // _x
        GASNumber xval = cinfo.IsFlagSet(DisplayInfo::V_x) ? pt.x : GASNumberUtil::NaN();
        if (!GASNumberUtil::IsNaN(xval))
        {
            if (GASNumberUtil::IsNEGATIVE_INFINITY(xval) || GASNumberUtil::IsPOSITIVE_INFINITY(xval))
                xval = 0;
            pchar->pGeomData->X = int(floor(PixelsToTwips(xval)));
            m.M_[0][2] = (Float) pchar->pGeomData->X;
        }
        // _y
        GASNumber yval = cinfo.IsFlagSet(DisplayInfo::V_y) ? pt.y : GASNumberUtil::NaN();
        if (!GASNumberUtil::IsNaN(yval))
        {
            if (GASNumberUtil::IsNEGATIVE_INFINITY(yval) || GASNumberUtil::IsPOSITIVE_INFINITY(yval))
                yval = 0;
            pchar->pGeomData->Y = int(floor(PixelsToTwips(yval)));
            m.M_[1][2] = (Float) pchar->pGeomData->Y;
        }

        if (m.IsValid())
            pchar->SetMatrix(m);

        // Special case for TextField:
        if (istf) 
        {
            if (cinfo.IsFlagSet(GFxValue::DisplayInfo::V_x))
                pchar->pGeomData->X = G_IRound(PixelsToTwips(pt.x));
            if (cinfo.IsFlagSet(GFxValue::DisplayInfo::V_y))
                pchar->pGeomData->Y = G_IRound(PixelsToTwips(pt.y));
        }
    }

    return true;
}

bool GFxValue::ObjectInterface::SetText(void *pdata, const char *ptext, bool reqHtml)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_SetText, Amp_Profile_Level_Low);
#endif

    GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
    GFxASCharacter* pchar = pch->ResolveCharacter(pMovieRoot);
    if (!pchar)
        return false;

    if (pchar->GetObjectType() != GASObjectInterface::Object_TextField)
        return SetMember(pdata, (reqHtml) ? "htmlText" : "text", ptext, true);
    else 
    {
        GASTextField* ptf = static_cast<GASTextField*>(pchar);
        ptf->SetText(ptext, reqHtml);
        return true;
    }
}

bool GFxValue::ObjectInterface::SetText(void *pdata, const wchar_t *ptext, bool reqHtml)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_SetText, Amp_Profile_Level_Low);
#endif

    GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
    GFxASCharacter* pchar = pch->ResolveCharacter(pMovieRoot);
    if (!pchar)
        return false;

    if (pchar->GetObjectType() != GASObjectInterface::Object_TextField)
        return SetMember(pdata, (reqHtml) ? "htmlText" : "text", ptext, true);
    else 
    {
        GASTextField* ptf = static_cast<GASTextField*>(pchar);
        ptf->SetText(ptext, reqHtml);
        return true;
    }
}

bool GFxValue::ObjectInterface::GetText(void* pdata, GFxValue* pval, bool reqHtml) const
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_GetText, Amp_Profile_Level_Low);
#endif

    GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
    GFxASCharacter* pchar = pch->ResolveCharacter(pMovieRoot);
    if (!pchar)
        return false;

    if (pchar->GetObjectType() != GASObjectInterface::Object_TextField)
        return GetMember(pdata, (reqHtml) ? "htmlText" : "text", pval, true);
    else 
    {
        GASEnvironment* penv = pMovieRoot->pLevel0Movie->GetASEnvironment();
        GASTextField* ptf = static_cast<GASTextField*>(pchar);
        GASString astext = ptf->GetText(penv, reqHtml);
        pMovieRoot->ASValue2GFxValue(penv, astext, pval);
        return true;
    }
}

bool GFxValue::ObjectInterface::GotoAndPlay(void *pdata, const char *frame, bool stop)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_GotoAndPlay, Amp_Profile_Level_Low);
#endif

    GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
    GFxASCharacter* pchar = pch->ResolveCharacter(pMovieRoot);
    if (!pchar || !pchar->IsSprite())
        return false;

    UInt frameNum;
    if (pchar->GetLabeledFrame(frame, &frameNum))
    {
        pchar->GotoFrame(frameNum);
        pchar->SetPlayState(stop ? GFxMovie::Stopped : GFxMovie::Playing);
        return true;
    }
    return false;
}

bool GFxValue::ObjectInterface::GotoAndPlay(void *pdata, UInt frame, bool stop)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_GotoAndPlay, Amp_Profile_Level_Low);
#endif

    GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
    GFxASCharacter* pchar = pch->ResolveCharacter(pMovieRoot);
    if (!pchar || !pchar->IsSprite())
        return false;

    // Incoming frame number is 1-based, but internally GFx is 0 based.
    pchar->GotoFrame(frame-1);
    pchar->SetPlayState(stop ? GFxMovie::Stopped : GFxMovie::Playing);
    return true;
}

bool GFxValue::ObjectInterface::GetCxform(void *pdata, GRenderer::Cxform *pcx) const
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_GetCxform, Amp_Profile_Level_Low);
#endif

    GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
    GFxASCharacter* pchar = pch->ResolveCharacter(pMovieRoot);
    if (!pchar) return false;

    *pcx = pchar->GetCxform();
    return true;
}

bool GFxValue::ObjectInterface::SetCxform(void *pdata, const GRenderer::Cxform &cx)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_SetCxform, Amp_Profile_Level_Low);
#endif

    GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
    GFxASCharacter* pchar = pch->ResolveCharacter(pMovieRoot);
    if (!pchar) return false;

    pchar->SetCxform(cx);
    pchar->SetAcceptAnimMoves(false);
    return true;
}

bool GFxValue::ObjectInterface::CreateEmptyMovieClip(void* pdata, GFxValue* pmc, 
                                                     const char* instanceName, SInt depth)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_CreateEmptyMovieClip, Amp_Profile_Level_Low);
#endif

    GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
    GFxASCharacter* pchar = pch->ResolveCharacter(pMovieRoot);
    if (!pchar || !pchar->IsSprite())
        return false;

    GFxSprite* psprite = (GFxSprite*)pchar;
    GASEnvironment* penv = pMovieRoot->pLevel0Movie->GetASEnvironment();

    // Check for specified depth; else get next highest depth
    if (depth < 0)
        depth = G_Max<SInt>(0, psprite->GetLargestDepthInUse() - 16384 + 1);

    // Create a new object and add it.
    GFxCharPosInfo pos = GFxCharPosInfo( GFxResourceId(GFxCharacterDef::CharId_EmptyMovieClip),
        depth + 16384,
        1, GRenderer::Cxform::Identity, 1, GRenderer::Matrix::Identity);
    // Bounds check depth.
    if ((pos.Depth < 0) || (pos.Depth > (2130690045 + 16384)))
        return false;

    GPtr<GFxCharacter> newCh = psprite->AddDisplayObject(
        pos, penv->CreateString(instanceName), NULL, NULL, GFC_MAX_UINT, 
        GFxDisplayList::Flags_ReplaceIfDepthIsOccupied);  
    if (newCh)
    {
        newCh->SetAcceptAnimMoves(false);

        // Return newly created clip.
        GFxASCharacter* pspriteCh = newCh->CharToASCharacter(); 
        GASSERT (pspriteCh != 0);
        GASValue asval(pspriteCh);
        pMovieRoot->ASValue2GFxValue(penv, asval, pmc);
    }
    return true;
}

bool GFxValue::ObjectInterface::AttachMovie(void* pdata, GFxValue* pmc, 
                                            const char* symbolName, const char* instanceName, 
                                            SInt depth, const GFxValue* initObj)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(pMovieRoot->AdvanceStats, NativeCodeSwdHandle, Func_GFxValue_ObjectInterface_AttachMovie, Amp_Profile_Level_Low);
#endif

    GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
    GFxASCharacter* pchar = pch->ResolveCharacter(pMovieRoot);
    if (!pchar || !pchar->IsSprite())
        return false;

    GFxSprite* psprite = (GFxSprite*)pchar;
    GASEnvironment* penv = pMovieRoot->pLevel0Movie->GetASEnvironment();

    GFxResourceBindData resBindData;
    if (!pMovieRoot->FindExportedResource(psprite->GetResourceMovieDef(), &resBindData, symbolName))
    {
        psprite->LogScriptWarning("Error: %s.attachMovie() failed - export name \"%s\" is not found.\n",
            psprite->GetCharacterHandle()->GetNamePath().ToCStr(), symbolName);
        return false;
    }

    GASSERT(resBindData.pResource.GetPtr() != 0); // MA TBD: Could this be true?     
    if (!(resBindData.pResource->GetResourceType() & GFxResource::RT_CharacterDef_Bit))
    {
        psprite->LogScriptWarning("Error: %s.attachMovie() failed - \"%s\" is not a movieclip.\n",
            psprite->GetCharacterHandle()->GetNamePath().ToCStr(), symbolName);
        return false;
    }

    GFxCharacterCreateInfo ccinfo;
    ccinfo.pCharDef     = (GFxCharacterDef*) resBindData.pResource.GetPtr();
    ccinfo.pBindDefImpl = resBindData.pBinding->GetOwnerDefImpl();

    // Check for specified depth; else get next highest depth
    if (depth < 0)
        depth = G_Max<SInt>(0, psprite->GetLargestDepthInUse() - 16384 + 1);

    // Create a new object and add it.
    GFxCharPosInfo pos( ccinfo.pCharDef->GetId(), depth + 16384,
        1, GRenderer::Cxform::Identity, 1, GRenderer::Matrix::Identity);
    // Bounds check depth.
    if ((pos.Depth < 0) || (pos.Depth > (2130690045 + 16384)))
    {
        psprite->LogScriptWarning("Error: %s.attachMovie(\"%s\") failed - depth (%d) must be >= 0\n",
            psprite->GetCharacterHandle()->GetNamePath().ToCStr(), symbolName, pos.Depth);
        return false;
    }

    // Check for init obj
    GASObjectInterface* pinitobj = NULL;
    if (initObj)
    {
        GASValue oa;
        pMovieRoot->GFxValue2ASValue(*initObj, &oa);
        pinitobj = oa.ToObjectInterface(penv);
    }

    // We pass pchDef to make sure that symbols from nested imports use
    // the right definition scope, which might be different from that of psprite.
    GPtr<GFxCharacter> newCh = psprite->AddDisplayObject(
        pos, penv->CreateString(instanceName), 0,
        pinitobj, GFC_MAX_UINT, 
        GFxDisplayList::Flags_ReplaceIfDepthIsOccupied, &ccinfo);
    if (newCh)
    {
        newCh->SetAcceptAnimMoves(false);

        //!AB: attachMovie in Flash 6 and above should return newly created clip
        if (psprite->GetVersion() >= 6)
        {
            GFxASCharacter* pspriteCh = newCh->CharToASCharacter(); 
            GASSERT (pspriteCh != 0);
            GASValue asval(pspriteCh);
            pMovieRoot->ASValue2GFxValue(penv, asval, pmc);
        }
    }
    return true;
}

void GFxValue::ObjectInterface::ToString(GString* pstr, const GFxValue& thisVal) const
{
    GASEnvironment* penv = pMovieRoot->pLevel0Movie->GetASEnvironment();

    GASValue asVal;
    penv->GetMovieRoot()->GFxValue2ASValue(thisVal, &asVal);

    *pstr = GString(asVal.ToString(penv).ToCStr());
}


#ifndef GFC_NO_FXPLAYER_AS_USERDATA

GFxASUserData*  GFxValue::ObjectInterface::GetUserData(void* pdata, bool isdobj) const 
{
    GASObjectInterface* pthis = static_cast<GASObjectInterface*>(pdata);
    if (isdobj)
    {
        GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
        pthis = pch->ResolveCharacter(pMovieRoot);
        if (!pthis) return NULL;
    }

    return pthis->GetUserData();
}

void    GFxValue::ObjectInterface::SetUserData(void* pdata, GFxASUserData* puserdata, bool isdobj) 
{
    GASObjectInterface* pthis = static_cast<GASObjectInterface*>(pdata);
    if (isdobj)
    {
        GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pdata);
        pthis = pch->ResolveCharacter(pMovieRoot);
        if (!pthis) return;
    }

    return pthis->SetUserData(pMovieRoot, puserdata, isdobj);
}


void    GFxASUserData::SetLastObjectValue(GFxValue::ObjectInterface* pobjIfc, void* pdata, bool isdobj)
{
    pLastObjectInterface = pobjIfc;
    pLastData = pdata;
    IsLastDispObj = isdobj;
}

bool    GFxASUserData::GetLastObjectValue(GFxValue* value) const
{
    if (!pLastObjectInterface || !pLastData || !value) return false;

    GFxValue::ValueType type = GFxValue::VT_Object;
    GASObjectInterface* pifc = static_cast<GASObjectInterface*>(pLastData);
    if (IsLastDispObj)
    {
        type = GFxValue::VT_DisplayObject;
        GFxCharacterHandle* pch = static_cast<GFxCharacterHandle*>(pLastData);
        GFxMovieRoot* pmovieRoot = pLastObjectInterface->GetMovieRoot();
        GASSERT(pmovieRoot);
        pifc = pch->ResolveCharacter(pmovieRoot);
    }
    else
    {
        GASObject* pobj = pifc->ToASObject();
        if (pobj->GetObjectType() == GASObjectInterface::Object_Array)
            type = GFxValue::VT_Array;
    }

    // Clear existing managed ref first
    if (value->IsManagedValue()) value->ReleaseManagedValue();
    value->Type = GFxValue::ValueType(type | GFxValue::VTC_ManagedBit);
    value->Value.pData = pLastData;
    value->pObjectInterface = pLastObjectInterface;
    // Make sure to add-ref before returning
    pLastObjectInterface->ObjectAddRef(value, pLastData);

    return true;
}

#endif  // GFC_NO_FXPLAYER_AS_USERDATA
