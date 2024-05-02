/**********************************************************************

Filename    :   GFxValue.cpp
Content     :   GASValue implementation
Created     :   
Authors     :   Artyom Bolgar

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxAction.h"
#include "AS/GASObject.h"
#include "GASFunctionRefImpl.h"
#include "GFxCharacter.h"
#include "GFxASString.h"
#include "AS/GASStringObject.h"
#include "AS/GASArrayObject.h"
#include "AS/GASNumberObject.h"
#include "AS/GASBooleanObject.h"
#include "GFxLog.h"

#include <stdio.h>
#include <stdlib.h>

// Utility.  Try to convert str to a number.  If successful,
// put the result in *result, and return true.  If not
// successful, put 0 in *result, and return false.
// Also, "Infinity", "-Infinity", and "NaN"
// are recognized.
// !DP (SInt32)strtoul is used for oct and hex to match Flash behavior  
static bool StringToNumber(GASNumber* result, const char* str)
{
    char* tail = 0;
    UPInt len = G_strlen (str);

    if (*str == '0' && G_tolower (*(str+1)) == 'x')  // hexadecimal
    {
        // hexadecimal values can't be negative
        *result = (GASNumber)(SInt32)G_strtoul(str, &tail, 0);
    }
    else
    {
        // check, does the string represent floating point number
        if (strcspn (str, ".Ee") != len)
        {
            *result = (GASNumber)G_strtod(str, &tail);
        }
        else
        {
            UPInt plen = len;
            const char* positive = str;
            int sign = 1;
            
            if (*str == '-')
            {
                positive = str + 1;
                sign = -1;
                --plen;
            }
            else if (*str == '+')
            {
                positive = str + 1;
                --plen;
            }
            
            // octal numbers may be negative
            if (*positive == '0' && strspn (positive, "01234567") == plen)  // octal 
            {
                *result = (GASNumber)(SInt32)(G_strtoul(positive, &tail, 8) * sign);
            }
            else
            {
                *result = (GASNumber)(G_strtod (positive, &tail) * sign);
            }
        }  
    }
    if (tail == str || *tail != 0)
    {
        //!AB, string values "Infinity", "-Infinity" should be
        // converted to NaN.
        // Moock mentioned that these strings should be converted in appropriate
        // number values, but in practice it is not so: neither for Flash 5-6, nor
        // for Flash 7+
        /*
        if(G_strcmp(str, "NaN") == 0)
        {
            *result = GASNumberUtil::NaN();
            return true;
        }
        if(G_strcmp(str, "Infinity") == 0)
        {
            *result = GASNumberUtil::POSITIVE_INFINITY();
            return true;
        }
        if(G_strcmp(str, "-Infinity") == 0)
        {
            *result = GASNumberUtil::NEGATIVE_INFINITY();
            return true;
        }*/
        // Failed conversion to Number.
        return false;
    }
    return true;
}

GASValueProperty::GASValueProperty
    (GASRefCountCollector* pCC, const GASFunctionRef& getterMethod, const GASFunctionRef& setterMethod) 
:   GASRefCountBase<GASValueProperty>(pCC),
    GetterMethod(getterMethod), SetterMethod(setterMethod)
{
}

//
// GASValue -- ActionScript value type
//

GASValue::GASValue(const GASValue& v)
{
    T.Type = v.T.Type;
   
    switch(v.T.Type)
    {    
    case BOOLEAN:
        V.BooleanValue = v.V.BooleanValue;
        break;

    case FUNCTIONNAME:
    case STRING:        
        V.pStringNode = v.V.pStringNode;
        V.pStringNode->AddRef();
        break;

    case NUMBER:
        NV.NumberValue = v.NV.NumberValue;
        break;
        
    case INTEGER:
        NV.Int32Value = v.NV.Int32Value;
        break;

    case OBJECT:
        if (v.V.pObjectValue)
        {
            if (v.V.pObjectValue->IsFunction())
            {            
                T.Type = FUNCTION;
                V.FunctionValue.Init(v.V.pObjectValue->ToFunction());
            }
            else
            {
                V.pObjectValue = v.V.pObjectValue;
                V.pObjectValue->AddRef();
            }
        }
        else
        {
            V.pObjectValue = 0;
        }        
        break;

    case CHARACTER:
        V.pCharHandle = v.V.pCharHandle;
        if (V.pCharHandle)
            V.pCharHandle->AddRef();
        break;

    case RESOLVEHANDLER:
    case FUNCTION:
        V.FunctionValue.Init(v.V.FunctionValue);
        break;
    
    case PROPERTY:
        V.pProperty = v.V.pProperty;
        V.pProperty->AddRef();
        break;

    default:;
    }    
}


void    GASValue::operator = (const GASValue& v)
{
    // Cleanup previous value.
    if (T.Type >= STRING)
        DropRefs();

    // Perform assignment: same as copy constructor above.
    // We paste this logic because it is used very heavily
    // and should thus be optimized as much as possible.
    T.Type = v.T.Type;

    switch(v.T.Type)
    {    
    case BOOLEAN:
        V.BooleanValue = v.V.BooleanValue;
        break;

    case FUNCTIONNAME:
    case STRING:        
        V.pStringNode = v.V.pStringNode;
        V.pStringNode->AddRef();
        break;

    case NUMBER:
        NV.NumberValue = v.NV.NumberValue;
        break;

    case INTEGER:
        NV.Int32Value = v.NV.Int32Value;
        break;

    case OBJECT:
        if (v.V.pObjectValue)
        {
            if (v.V.pObjectValue->IsFunction())
            {            
                T.Type = FUNCTION;
                V.FunctionValue.Init(v.V.pObjectValue->ToFunction());
            }
            else
            {
                V.pObjectValue = v.V.pObjectValue;
                V.pObjectValue->AddRef();
            }
        }
        else
        {
            V.pObjectValue = 0;
        }        
        break;

    case CHARACTER:
        V.pCharHandle = v.V.pCharHandle;
        if (V.pCharHandle)
            V.pCharHandle->AddRef();
        break;

    case RESOLVEHANDLER:
    case FUNCTION:
        V.FunctionValue.Init(v.V.FunctionValue);
        break;

    case PROPERTY:    
        V.pProperty = v.V.pProperty;
        V.pProperty->AddRef();
        break;
    }
}



GASValue::GASValue(GASObject* pobj)
{
    if (pobj && pobj->IsFunction()) 
    {
        T.Type = FUNCTION;
        V.FunctionValue.Init(pobj->ToFunction());
    }
    else
    {
        T.Type = OBJECT;
        V.pObjectValue = pobj;
        if (V.pObjectValue)   
            V.pObjectValue->AddRef(); 
    }
}

GASValue::GASValue(GFxASCharacter* pcharacter)    
{    
    T.Type = CHARACTER;

    if (!pcharacter)    
        V.pCharHandle = 0;
    else if ((V.pCharHandle = pcharacter->GetCharacterHandle())!=0)
        V.pCharHandle->AddRef();
}

GASValue::GASValue(GASStringContext* psc, GASCFunctionPtr func)
{
    T.Type = FUNCTION;
    V.FunctionValue.Init(*GHEAP_NEW(psc->GetHeap()) GASCFunctionObject (psc, func));
} 

GASValue::GASValue(const GASFunctionRef& func)    
{
    T.Type = FUNCTION;
    V.FunctionValue.Init(func);
}

GASValue::GASValue(const GASFunctionRef& getter, 
                   const GASFunctionRef& setter, 
                   GMemoryHeap* pheap, 
                   GASRefCountCollector* pCC)    
{
    T.Type = PROPERTY;
    V.pProperty = GHEAP_NEW(pheap) GASValueProperty(pCC, getter, setter);
}

GASValue::~GASValue()
{
    if (T.Type >= STRING)
        DropRefs();
}

#ifndef GFC_NO_GC
template <class Functor>
void GASValue::ForEachChild_GC(Collector* prcc) const
{
    if (IsFunctionObject())
        V.FunctionValue.template ForEachChild_GC<Functor>(prcc);
    else if (IsObject() && V.pObjectValue)
        Functor::Call(prcc, V.pObjectValue);
    else if (IsProperty())
        Functor::Call(prcc, V.pProperty);
}
GFC_GC_PREGEN_FOR_EACH_CHILD(GASValue)

void GASValue::Finalize_GC()
{
    if (IsFunctionObject())
        V.FunctionValue.Finalize_GC();
    else if (T.Type >= STRING && !IsObject() && !IsProperty())
        DropRefs();
    // Finalize_GC for Objects and Properties will be called by GRefCountCollector
}
#endif // GFC_NO_GC

// Conversion to string.
GASString   GASValue::ToStringImpl(GASEnvironment* penv, int precision, bool debug) const
{
    GASSERT(penv);
    GASString stringVal(penv->GetBuiltin(GASBuiltin_empty_));

    switch(T.Type)
    {
    case STRING:
        {
            // Don't need to do anything.
            stringVal.AssignNode(V.pStringNode);
        }
        break;

    case NUMBER:
        {  
            char buf[GASNumberUtil::TOSTRING_BUF_SIZE];
            stringVal = 
                penv->CreateString(GASNumberUtil::ToString(NV.NumberValue, buf, sizeof(buf), (precision < 0) ? 10 : -precision));

        }
        break;
    case INTEGER:
        {  
            char buf[GASNumberUtil::TOSTRING_BUF_SIZE];
            stringVal = 
                penv->CreateString(GASNumberUtil::IntToString(NV.Int32Value, buf, sizeof(buf)));
        }
        break;
    case UNSET:
    case UNDEFINED:
        {
            // Behavior depends on file version.  In
            // version 7+, it's "undefined", in versions
            // 6-, it's "".
            //
            // We'll go with the v7 behavior by default,
            // and conditionalize via Versioned()
            // functions.

            // MA: Should we (it is easy to do now with penv);
            // however, aren't there cases when v6 also returns 'undefined' ?
            stringVal = penv->GetBuiltin(GASBuiltin_undefined);
        }
        break;
    case NULLTYPE:
        {
            stringVal = penv->GetBuiltin(GASBuiltin_null);
        }
        break;
    case BOOLEAN:
        {
            stringVal = penv->GetBuiltin(V.BooleanValue ? GASBuiltin_true : GASBuiltin_false);
        }
        break;
    case OBJECT:
    case CHARACTER:
        {
            // @@ Moock says: 
            //    - "the value that results from calling ToString() on the object".
            //    - The default ToString() returns "[object Object]" but may be customized.
            //    - A Movieclip returns the absolute path of the object.
            // 

            // Do we have a "toString" method?
            GASValue             toStringFunc;
            GASObjectInterface * piobj = ToObjectInterface(penv);

            if (!debug && piobj &&
                piobj->GetMemberRaw(penv->GetSC(),
                penv->GetBuiltin(GASBuiltin_toString), &toStringFunc)) 
            {
                //!AB: recursion guard is necessary to prevent infinite recursion in the case
                // if "toString" returns "this" or another object that may finally returns "this".
                // This is especially dangerous because "toString" method might be invoked 
                // implicitly during the type casting.
                if (penv->RecursionGuardStart())
                {
                    GASValue result;
                    //GAS_Invoke(toStringFunc, &result, piobj, penv, 0, 0, NULL);
                    GASFunctionRef  func(toStringFunc.ToFunction(penv));
                    if (func != NULL)
                    {
                        GASFnCall fnCall(&result, piobj, penv, 0, 0); 
                        func.Function->Invoke(fnCall, func.LocalFrame, NULL);
                    }
                    stringVal = result.ToString(penv);
                }
                else
                {
                    // [type Object]
                    stringVal = penv->GetBuiltin(GASBuiltin_typeObject_);
                    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                    if (penv->IsVerboseActionErrors())
                        penv->LogScriptError("Error: Stack overflow, max level of 255 nested calls of toString is reached.\n");
                    #endif
                }
                penv->RecursionGuardEnd();
            }
            else
            {   
                // Used within action log or if object is null.            
                const char* ptext;

                if ((T.Type == OBJECT) && V.pObjectValue && 
                    (ptext = V.pObjectValue->GetTextValue(penv))!=0)
                {
                    stringVal = penv->CreateString(ptext);
                }
                else if ((T.Type == CHARACTER) && V.pCharHandle)
                {
                    stringVal = GetCharacterNamePath(penv);
                }
                else
                {
                    // This is the default: [object Object]
                    stringVal = penv->GetBuiltin(GASBuiltin_objectObject_);
                }
            }
        }
        break;
    case FUNCTION:
    case FUNCTIONNAME:
        {
            // [type Function]
            stringVal =  penv->GetBuiltin(GASBuiltin_typeFunction_);
        }
        break;
    case PROPERTY:
        {
            // [property]
            stringVal =  penv->CreateConstString("[property]");
        }
        break;
    case RESOLVEHANDLER:
        {
            // [resolveHandler]
            stringVal =  penv->CreateConstString("[resolveHandler]");
        }
        break;
    default:
        {
            stringVal = penv->GetBuiltin(GASBuiltin_badtype_); // "<bad type>";
            GASSERT(0);
        }
    }
    return stringVal;
}

GASString   GASValue::ToDebugString(GASEnvironment* penv) const
{
    GASString s = ToStringImpl(penv, -1, 1);
    return s;
}

// Conversion to const GASString.
GASString   GASValue::ToStringVersioned(GASEnvironment* penv, UInt version) const
{
    if (IsUndefined())
    {
        // Version-dependent behavior.
        if (version > 0 && version <= 6)
        {
            return penv->GetBuiltin(GASBuiltin_empty_);
        }
        else
        {
            return penv->GetBuiltin(GASBuiltin_undefined);
        }
    }
    
    return ToStringImpl(penv, -1, 0);
}


// Conversion to double/float.
GASNumber   GASValue::ToNumber(GASEnvironment* penv) const
{
    if (T.Type == NUMBER)
    {
        return NV.NumberValue;
    }
    else if (T.Type == INTEGER)
    {
        return (GASNumber)NV.Int32Value;
    }
    else if (T.Type == STRING)
    {
        // @@ Moock says the rule here is: if the
        // string is a valid Float literal, then it
        // gets converted; otherwise it is set to NaN.
        //
        GASNumber num;
        if (!StringToNumber(&num, V.pStringNode->pData))
        {
            // Failed conversion to Number.
            num = GASNumberUtil::NaN(); //!AB
        }
        return num;
    }    
    else if (T.Type == NULLTYPE)
    {
        UInt version = penv->GetVersion();
        if (version <= 6)
            return GFX_ASNUMBER_ZERO;
        else
            return GASNumberUtil::NaN();
    }
    else if (T.Type == BOOLEAN)
    {
        return V.BooleanValue ? (GASNumber)1 : (GASNumber)0;
    }
    else if (T.Type == CHARACTER)
    {
        // ToNumber for movieclip returns always NaN (according to Moock)
        return GASNumberUtil::NaN();
    }
    else if ( ((T.Type == OBJECT) && V.pObjectValue) ||
              (T.Type == FUNCTION))
    {
        // @@ Moock says the result here should be
        // "the return value of the object's ValueOf() method".     
        //
        // Arrays and Movieclips should return NaN.

        GASValue             toValueFunc;
        GASObjectInterface * piobj = ToObjectInterface(penv);

        if (penv && piobj->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_valueOf), &toValueFunc)) 
        {
            //!AB: recursion guard is necessary to prevent infinite recursion in the case
            // if "valueOf" returns "this" or another object that may finally returns "this".
            // This is especially dangerous because "valueOf" method might be invoked 
            // implicitly during the type casting.
            GASNumber retVal;
            if (penv->RecursionGuardStart())
            {
                GASValue result;
                //GAS_Invoke(toValueFunc, &result, piobj, penv, 0, 0, NULL);
                GASFunctionRef  func(toValueFunc.ToFunction(penv));
                if (func != NULL)
                {
                    GASFnCall fnCall(&result, piobj, penv, 0, 0); 
                    func.Function->Invoke(fnCall, func.LocalFrame, NULL);
                }

                if (result.IsPrimitive())
                    retVal = result.ToNumber(penv);
                else
                    retVal = GASNumberUtil::NaN();
            }
            else
            {
                retVal = GASNumberUtil::NaN();
                #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                if (penv->IsVerboseActionErrors())
                    penv->LogScriptError("Error: Stack overflow, max level of 255 nested calls of valueOf is reached.\n");
                #endif
            }
            penv->RecursionGuardEnd();
            return retVal;
        }
        else
        {
            // AB: valueOf should be implemented for each object!
            // leave this branch just for now.
            // Text characters with var names could get in here.
            if (T.Type == CHARACTER)
                return GASNumberUtil::NaN();
            
            const char* ptextval = piobj->GetTextValue(penv);
            if (ptextval)
            {
                return (GASNumber)atof(ptextval);
            }
        }
        return GFX_ASNUMBER_ZERO;
    }
    else if (IsUndefined())
    {
        UInt version = penv->GetVersion();

        //!AB: Flash 6 and below: 0
        //     Flash 7 and above: NaN
        if (version > 0 && version <= 6)
            return GFX_ASNUMBER_ZERO;
        else
            return GASNumberUtil::NaN();
    }
    else
    {
        return GFX_ASNUMBER_ZERO;
    }
}

SInt32  GASValue::ToInt32(GASEnvironment* penv) const
{
    if (T.Type == INTEGER)
    {
        return NV.Int32Value;
    }
    else
    {
        // follow to algorithm described in ECMA-262, clause 9.5
        static const GASNumber UINT32MAX = GASNumber(~0u);
        static const GASNumber INT32MAX  = GASNumber(1UL<<31)-1;
        static const GASNumber INT32MIN  = -GASNumber(1UL<<31);

        GASNumber v = ToNumber(penv);
        if (v == 0 || GASNumberUtil::IsNaNOrInfinity(v))
            return 0;
        if (v >= INT32MIN && v <= INT32MAX)
            return (SInt32)v;

        GASNumber anv = floor(G_Abs(v));
        UInt32 uv = (UInt32)fmod(anv, UINT32MAX + 1);

        SInt32 rv = -SInt32(~0u - uv + 1); // actually, this is an analog of (uv - 2^32)
        if (v < 0) rv = -rv;

        return rv;
    }
}

UInt32    GASValue::ToUInt32(GASEnvironment* penv) const
{
    if (T.Type == INTEGER)
    {
        return NV.UInt32Value;
    }
    else
    {
        // follow to algorithm described in ECMA-262, clause 9.6
        static const GASNumber UINT32MAX = GASNumber(~0u);

        GASNumber v = ToNumber(penv);
        if (v == 0 || GASNumberUtil::IsNaNOrInfinity(v))
            return 0;
        if (v >= 0 && v <= UINT32MAX)
            return (UInt32)v;

        GASNumber anv = floor(G_Abs(v));
        UInt32 uv = (UInt32)fmod(anv, UINT32MAX + 1);
        if (v < 0) uv = 0u-uv;
        return uv;
    }
}

// Conversion to boolean.
bool    GASValue::ToBool(const GASEnvironment* penv) const
{
    if (T.Type == STRING)
    {
        //!AB, "true" and "false" should never be converted
        // to appropriate boolean value.
        // For Flash 6, the only string containing non-zero numeric
        // value is converted to boolean as "true".
        // For Flash 7+, any non-empty string is converted to 
        // boolean as "true".
        /*
        if (StringValue == "false")
        {
            return false;
        }
        else if (StringValue == "true")
        {
            return true;
        }*/

        // Empty string --> false
        if (V.pStringNode->Size == 0)
            return false;

        if (penv->GetVersion() <= 6)
        {
            // @@ Moock: "true if the string can
            // be converted to a valid nonzero number".
            //
            GASNumber num;
            if (StringToNumber(&num, V.pStringNode->pData))
            {
                if (GASNumberUtil::IsNaN(num)) return false;
                return (!!num);
            }
            return false;
        }
        return true;
    }
    else if (T.Type == NUMBER)
    {
        // @@ Moock says, NaN --> false
        if (GASNumberUtil::IsNaN(NV.NumberValue))
            return false;
        return NV.NumberValue != GFX_ASNUMBER_ZERO;
    }
    else if (T.Type == INTEGER)
    {
        return NV.Int32Value != 0;
    }
    else if (T.Type == BOOLEAN)
    {           
        return V.BooleanValue;
    }
    else if (T.Type == OBJECT)
    {
        return V.pObjectValue != NULL;
    }
    else if (T.Type == CHARACTER)
    {
        return ToASCharacter(penv) != NULL;
    }
    /*else if (Type == C_FUNCTION)
    {
        return C_FunctionValue != NULL;
    }  */
    else if (T.Type == FUNCTION)
    {
        return !V.FunctionValue.IsNull ();
    }
    else if (T.Type == FUNCTIONNAME)
    {
        return true;
    }
    else
    {
        GASSERT(IsUndefined() || T.Type == NULLTYPE);
        return false;
    }
}


// Return value as an object.
GASObject*  GASValue::ToObject(const GASEnvironment* penv) const
{
    switch(T.Type)
    {
    case OBJECT:
        return V.pObjectValue;

    case FUNCTION:
        if (!V.FunctionValue.IsNull ())
            return V.FunctionValue.GetObjectPtr();
        break;
    case FUNCTIONNAME:
        {
            GASFunctionRef funcRef = ResolveFunctionName(penv);
            if (!funcRef.IsNull())
            {
                return funcRef.GetObjectPtr();
            }
        }
        break;
    case CHARACTER:
        {
            // If this occurs, it means that code used ToObject() call instead of ToObjectInterface() in a
            // place where characters should be permitted.
            // In rare cases that this is necessary, an explicit != CHARACTER check should be done,
            // so that we can keep this useful warning.
            GFC_DEBUG_WARNING(1, "GASValue::ToObject() invoked on a character. Need to use ToObjectInterface?");
        }
        break;
    case PROPERTY:
        if (IsProperty())
        {
            GPtr<GFxASCharacter> paschar = penv->GetTarget();
            if (paschar)
            {
                GASValue val;
                //!AB: environment shouldn't be constant here, since it can be modified by the
                // GetPropertyValue call. TODO - remove all 'const' attributes from env parameters.
                if (GetPropertyValue(const_cast<GASEnvironment*>(penv), paschar.GetPtr(), &val))
                    return val.ToObject(penv);
            }
        }
        break;
    default:;
    }
    return NULL;
}

GFxASCharacter*     GASValue::ToASCharacter(const GASEnvironment* penv) const
{
    if (T.Type == CHARACTER && penv)
    {
        if (V.pCharHandle)
            return V.pCharHandle->ResolveCharacter(penv->GetMovieRoot());     
    }
    return 0;   
}

GASObjectInterface* GASValue::ToObjectInterface(const GASEnvironment* penv) const
{
    if (T.Type == CHARACTER)
        return ToASCharacter(penv);
    return ToObject(penv);
}   



// Return value as a function.  Returns NULL if value is
// not a function.
GASFunctionRef  GASValue::ToFunction(const GASEnvironment* penv) const
{
    switch(T.Type)
    {
    case FUNCTIONNAME:
        return ResolveFunctionName(penv);
    case FUNCTION:
        return GASFunctionRef(V.FunctionValue);
    default:;
    }
    return GASFunctionRef();
}

GASFunctionRef  GASValue::ToResolveHandler() const
{
    if (T.Type == RESOLVEHANDLER)
        return V.FunctionValue;
    return GASFunctionRef();
}

// Force type to number.
void    GASValue::ConvertToNumber(GASEnvironment* pEnv)
{
    SetNumber(ToNumber(pEnv));
}

// Force type to string.
void    GASValue::ConvertToString(GASEnvironment* pEnv)
{
    GASString str = ToString(pEnv);    
    DropRefs();
    T.Type = STRING;  // force type.
    V.pStringNode = str.GetNode();
    V.pStringNode->AddRef();
}


// Force type to string.
void    GASValue::ConvertToStringVersioned(GASEnvironment* pEnv, UInt version)
{
    GASString str = ToStringVersioned(pEnv, version);    
    DropRefs();
    T.Type = STRING;  // force type.
    V.pStringNode = str.GetNode();
    V.pStringNode->AddRef();
}


void    GASValue::SetAsObject(GASObject* obj)
{
    if (obj && obj->IsFunction())
    {
        SetAsFunction(obj->ToFunction());
    }
    else if (T.Type != OBJECT || V.pObjectValue != obj)
    {
        DropRefs();
        T.Type         = OBJECT;
        V.pObjectValue = obj;
        if (V.pObjectValue)
            V.pObjectValue->AddRef();
    }
}
    
void    GASValue::SetAsCharacterHandle(GFxCharacterHandle* pchar)
{
    if (T.Type != CHARACTER || V.pCharHandle != pchar)
    {
        DropRefs();
        T.Type        = CHARACTER;
        V.pCharHandle = pchar;
        if (V.pCharHandle)
            V.pCharHandle->AddRef();
    }
}

void    GASValue::SetAsCharacter(GFxASCharacter* pchar)
{
    SetAsCharacterHandle(pchar ? pchar->GetCharacterHandle() : 0);
}

void    GASValue::SetAsObjectInterface(GASObjectInterface* pobj)
{
    // More expensive then either SetAsObject or SetAsCharacter, should be called only when necessary.
    if (pobj->IsASCharacter())
        SetAsCharacter((GFxASCharacter*) pobj);
    else
        SetAsObject((GASObject*) pobj);
}


void    GASValue::SetAsFunction(const GASFunctionRefBase& func)
{
    if (T.Type != FUNCTION || V.FunctionValue != func)
    {
        DropRefs();
        T.Type = FUNCTION;
        V.FunctionValue.Init(func);
    }
}

void    GASValue::SetAsResolveHandler(const GASFunctionRefBase& func)
{
    DropRefs();
    T.Type = RESOLVEHANDLER;
    V.FunctionValue.Init(func);
}


void    GASValue::Add(GASEnvironment* penv, const GASValue& v)  
{ 
    GASValue pv1, pv2;

    pv1 = ToPrimitive (penv);
    pv2 = v.ToPrimitive (penv);

    if(pv1.T.Type == STRING || pv2.T.Type == STRING) 
    {
        UInt version = penv->GetVersion();

        pv1.ConvertToStringVersioned(penv, version);
        pv1.StringConcat(penv, pv2.ToStringVersioned(penv, version));
        SetString(pv1.ToString(penv));
    }
    else 
    {
        SetNumber(pv1.ToNumber(penv) + pv2.ToNumber(penv)); 
    }
}

void    GASValue::Add(GASEnvironment* penv, int v2)  
{ 
    GASValue pv1;

    pv1 = ToPrimitive (penv);

    if(pv1.T.Type == STRING) 
    {
        UInt version = penv->GetVersion();

        pv1.ConvertToStringVersioned(penv, version);
        pv1.StringConcat(penv, GASValue(v2).ToStringVersioned(penv, version));
        SetString(pv1.ToString(penv));
    }
    else 
    {
        SetNumber(pv1.ToNumber(penv) + (GASNumber)v2); 
    }
}


// Return true if operands are equal.
bool    GASValue::IsEqual (GASEnvironment* penv, const GASValue& v) const
{
    bool    thisNullType = (IsUndefined()) || (T.Type == NULLTYPE);
    bool    vNullType    = (v.IsUndefined()) || (v.GetType() == NULLTYPE);

    if (thisNullType || vNullType)
    {
        return thisNullType == vNullType;
    }

    switch(T.Type)
    {
        case STRING:
            if (v.IsNumber())
                // ECMA-262, 11.9.3.17
                return GASValue(ToNumber(penv)).IsEqual(penv, v);
            else if (v.T.Type == BOOLEAN)
                // ECMA-262, 11.9.3.19
                return IsEqual(penv, GASValue(v.ToNumber(penv)));
            else if (v.IsFunction() || v.IsObject())
            {
                // ECMA-262, 11.9.3.20
                GASValue primVal = v.ToPrimitive(penv);
                if (primVal.IsPrimitive())
                    return IsEqual(penv, primVal);
                else
                    return false;
            }
            return GASString(V.pStringNode) == v.ToString(penv);

        case NUMBER:
        {
            GASNumber pv;
            if (v.T.Type == BOOLEAN)
                // ECMA-262, 11.9.3.19
                pv = v.ToNumber(penv);
            else if (v.IsFunction() || v.IsObject())
            {
                // ECMA-262, 11.9.3.20
                GASValue primVal = v.ToPrimitive(penv);
                if (primVal.IsPrimitive())
                    return IsEqual(penv, primVal);
                else
                    return false;
            }
            else
                pv = v.ToNumber(penv);
            // ECMA-262, 11.9.3.5 - 7
            if (GASNumberUtil::IsNaN(NV.NumberValue) && GASNumberUtil::IsNaN(pv))
                return true;
            else if (GASNumberUtil::IsNaN(NV.NumberValue) || GASNumberUtil::IsNaN(pv))
                return false;
            return NV.NumberValue == pv;
        }
        case INTEGER:
            {
                if (v.T.Type == NUMBER)
                {
                    // can't compare integer with floating point directly;
                    // so, convert integer to a fp and compare.
                    GASValue dv;
                    dv.SetNumber((GASNumber)NV.Int32Value);
                    return dv.IsEqual(penv, v);
                }
                SInt32 pv;
                if (v.T.Type == BOOLEAN)
                    // ECMA-262, 11.9.3.19
                    pv = v.ToInt32(penv);
                else if (v.IsFunction() || v.IsObject())
                {
                    // ECMA-262, 11.9.3.20
                    GASValue primVal = v.ToPrimitive(penv);
                    if (primVal.IsPrimitive())
                        return IsEqual(penv, primVal);
                    else
                        return false;
                }
                else
                    pv = v.ToInt32(penv);
                // ECMA-262, 11.9.3.5 - 7
                return NV.Int32Value == pv;
            }
        case BOOLEAN:
            // According to ECMA-262, clause 11.9.3, if type(x) or (y) is Boolean
            // return ToNumber(x) == y (or x == ToNumber(y))
            return GASValue(ToNumber(penv)).IsEqual(penv, v);

        // Objects and functions are compared by reference.
        case OBJECT:
        case FUNCTION:
            if (v.IsNumber() || v.T.Type == STRING || v.T.Type == BOOLEAN)
            {
                // ECMA-262, 11.9.3.19
                // ECMA-262, 11.9.3.21
                GASValue primVal = ToPrimitive(penv);
                if (primVal.IsPrimitive())
                    return primVal.IsEqual(penv, v);
                else
                    return false;
            }
            if (T.Type == OBJECT && v.T.Type == OBJECT)
                return V.pObjectValue == v.V.pObjectValue;
            else if (T.Type == FUNCTION && v.T.Type == FUNCTION)
                return V.FunctionValue == v.V.FunctionValue;
            return false;

        // Flash MovieClips are always compared by path only
        // (although this is technically not always correct).
        case CHARACTER:
            if (v.T.Type != CHARACTER)
                return false; // type mismatch, return false
            if (!V.pCharHandle || !v.V.pCharHandle) // should not happen.
                return V.pCharHandle == v.V.pCharHandle;    
            return GetCharacterNamePath(penv) == v.GetCharacterNamePath(penv);


        default:
        break;
    }

    // That's it. Anything else?    
    return T.Type == v.T.Type;
}

// action: 0 - equals, -1 - less than, 1 - greater than
// return value: either BOOLEAN (true\false) or undefined.
GASValue    GASValue::Compare (GASEnvironment* penv, const GASValue& v, int action) const
{
    if (action == 0)
        return GASValue(IsEqual(penv, v));

    // do comparison according to ECMA-262, 11.8.5
    GASValue pv1 = ToPrimitive(penv);
    GASValue pv2 = v.ToPrimitive(penv);
    bool result = false;
    if (pv1.IsString() && pv2.IsString())
    {
        // do strings comparison, see 11.8.5.16
        if (action < 0)
            return (pv1.ToString(penv) < pv2.ToString(penv));
        else
            return (pv1.ToString(penv) > pv2.ToString(penv));
    }
    if (penv->GetVersion() > 6 && (pv1.IsUndefined() || pv2.IsUndefined()))
        return GASValue();

    // do operator < comparison
    GASNumber val1, val2;
    if (action < 0)
    {
        val1 = pv1.ToNumber(penv);
        val2 = pv2.ToNumber(penv);
    }
    else
    {
        val2 = pv1.ToNumber(penv);
        val1 = pv2.ToNumber(penv);
    }

    if (GASNumberUtil::IsNaN(val1) || GASNumberUtil::IsNaN(val2))
        return GASValue();
    if (val1 == val2)
        result = false;
    else if (GASNumberUtil::IsNEGATIVE_ZERO(val1) && GASNumberUtil::IsPOSITIVE_ZERO(val2))
        result = false;
    else if (GASNumberUtil::IsNEGATIVE_ZERO(val2) && GASNumberUtil::IsPOSITIVE_ZERO(val1))
        result = false;
    else if (GASNumberUtil::IsPOSITIVE_INFINITY(val1))
        result = false;
    else if (GASNumberUtil::IsPOSITIVE_INFINITY(val2))
        result = true;
    else if (GASNumberUtil::IsNEGATIVE_INFINITY(val2))
        result = false;
    else if (GASNumberUtil::IsNEGATIVE_INFINITY(val1))
        result = true;
    else
        result = (val1 < val2);
    return GASValue(result);
}


// Sets *this to this string plus the given string.
void    GASValue::StringConcat(GASEnvironment* penv, const GASString& str)
{
    GASString tempstr(ToString(penv) + str);
    DropRefs();
    T.Type        = STRING;
    V.pStringNode = tempstr.GetNode();
    V.pStringNode->AddRef();
}

// This hack is used to prevent memory leak in situations like this:
//
//  var o = new Object;
//  o._listeners = new Array;
//  o._listeners[0] = o;
//
// This is used in Tween class, for example.
// After GC is implemented we will remove this hack. (!AB)

static void GAS_CheckForListenersMemLeak(GASObject* pobjectValue)
{
#ifdef GFC_NO_GC
    if (pobjectValue->GetRefCount() == 2 && pobjectValue->IsListenerSet)
    {
        GASMember val;
        // Hack-in-hack: use raw key because we don't have string context
        // here, while this is called from DropRefs and destructor.
        GASString::RawCompareKey rkey("_listeners", sizeof("_listeners")-1);

        if (pobjectValue->Members.GetAlt(rkey, &val))
        {
            GASObject* vobj = val.Value.ToObject(NULL);
            if (vobj && vobj->GetObjectType() == GASObject::Object_Array)
            {
                GASArrayObject* arrayObj = static_cast<GASArrayObject*>(vobj);
                for(int i = 0, n = arrayObj->GetSize(); i < n; ++i)
                {
                    GASValue* elem = arrayObj->GetElementPtr(i);
                    if (elem && elem->IsObject())
                    {
                        if (elem->ToObject(NULL) == pobjectValue)
                        {
                            elem->V.pObjectValue->Release();
                            elem->V.pObjectValue = 0;
                            break;
                        }
                    }
                }
            }
        }
    }
#else
    GUNUSED(pobjectValue);
#endif
}


// Drop any ref counts we have; this happens prior to changing our value.
void    GASValue::DropRefs()
{
    switch(T.Type)
    {
    case FUNCTIONNAME:
    case STRING:
        // pStringNode can not be null (null strings are not supported)!
        V.pStringNode->Release();
        break;

    case FUNCTION:
    case RESOLVEHANDLER:
        if (V.FunctionValue != NULL)
        {
            V.FunctionValue.DropRefs();
        }
        break;

    case OBJECT:
        if (V.pObjectValue)
        {
            // Tween class '_listeners' leak detection hack! Comments above.
            GAS_CheckForListenersMemLeak(V.pObjectValue);
            V.pObjectValue->Release();
            V.pObjectValue = 0;
        }
        break;

    case CHARACTER:
        if (V.pCharHandle)
        {
            V.pCharHandle->Release();
            V.pCharHandle = 0;
        }
        break;

    case PROPERTY:
        if (V.pProperty)
        {
            V.pProperty->Release();
            V.pProperty = 0;
        }
        break;
    }
    
}

// TODO: use hint
GASValue GASValue::ToPrimitive(GASEnvironment* penv, GASValue::Hint /*hint*/) const
{
    if ((T.Type == OBJECT) || (T.Type == CHARACTER) || (T.Type == FUNCTION))
    {
        GASValue             toValueFunc;
        GASObjectInterface * piobj = ToObjectInterface(penv);

        if (penv && piobj && piobj->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_valueOf), &toValueFunc)) 
        {
            GASValue result;
            if (penv->RecursionGuardStart())
            {
                //GAS_Invoke(toValueFunc, &result, piobj, penv, 0, 0, NULL);
                GASFunctionRef  func(toValueFunc.ToFunction(penv));
                if (func != NULL)
                {
                    GASFnCall fnCall(&result, piobj, penv, 0, 0); 
                    func.Function->Invoke(fnCall, func.LocalFrame, NULL);
                }
            }
            else
            {
                #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                if (penv->IsVerboseActionErrors())
                    penv->LogScriptError("Error: Stack overflow, max level of 255 nested calls of valueOf is reached.\n");
                #endif
            }
            penv->RecursionGuardEnd();

            return result;
        }
        else
        {
            GASValue    ret;
            const char* pstr;

            if ((T.Type == CHARACTER) && V.pCharHandle)
            {
                ret.SetString(GetCharacterNamePath(penv));
            }
            else if ((T.Type == OBJECT) && V.pObjectValue && 
                     (pstr = V.pObjectValue->GetTextValue(penv))!=0)
            {
                ret.SetString(penv->CreateString(pstr));
            }
            else
            {
                ret.SetString(ToString(penv));//??
                //ret.SetString ("[object Object]"); // default (??)
            }
            return ret;
            /* //AB, will think about this later
            else if (penv != 0) {
                GFxLog* plog = penv->GetTarget()->GetLog();

                plog->LogScriptError ("Error: \"valueOf\" method is not found in ToPrimitive\n");
            }
            else 
                GASSERT (penv != 0); //AB ??? not sure what to do*/
        }
    }
    else if (T.Type == FUNCTIONNAME)
    {
        GASFunctionRef funcRef = ResolveFunctionName(penv);
        if (!funcRef.IsNull())
        {
            return funcRef;
        }
    }

    // TODO: do we need to do anything with other types?
    return *this;
}

bool        GASValue::GetPropertyValue(GASEnvironment* penv, GASObjectInterface* pthis, GASValue* value) const
{
    if (IsProperty() && penv)
    {
        if (!V.pProperty->GetterMethod.IsNull())
        {
            GASValue result;
            V.pProperty->GetterMethod.Invoke(GASFnCall(&result, pthis, penv, 0, 0));
            *value = result;
            return true;
        }
        else
        {
            if (penv->IsVerboseActionErrors())
                penv->LogScriptError("Error: getter method is null.");
        }
    }
    return false;
}

void        GASValue::SetPropertyValue(GASEnvironment* penv, GASObjectInterface* pthis, const GASValue& val)
{
    if (IsProperty() && penv)
    {
        if (!V.pProperty->SetterMethod.IsNull())
        {
            GASValue result;
            penv->Push(val);
            V.pProperty->SetterMethod.Invoke(GASFnCall(&result, pthis, penv, 1, penv->GetTopIndex()));
            penv->Drop(1);
            //?? What to do if penv is NULL?
        }
        else
        {
            if (penv->IsVerboseActionErrors())
                penv->LogScriptError("Error: setter method is null.");
        }
    }
}

const GASString& GASValue::GetCharacterNamePath(GASEnvironment* penv) const
{
    // If character is resolvable - return GetNamePath,  else return empty string
    if(V.pCharHandle)
        if(V.pCharHandle->ResolveCharacter(penv->GetMovieRoot()))
            return V.pCharHandle->GetNamePath();
    return penv->GetBuiltin(GASBuiltin_empty_);
}

GASFunctionRef GASValue::ResolveFunctionName(const GASEnvironment* penv) const
{
    if (penv && T.Type == FUNCTIONNAME)
    {
        GASString functionName(penv->GetBuiltin(GASBuiltin_empty_));
        functionName.AssignNode(V.pStringNode);

        GASFunctionRef ctor(penv->GetGC()->ResolveFunctionName(functionName));
        return ctor;
    }
    return GASFunctionRef();
}

void    GASValue::Div (GASEnvironment* penv, const GASValue& v)   
{ 
    GASNumber v1 = ToNumber(penv);
    GASNumber v2 = v.ToNumber(penv);
    GASNumber res;
    if (v2 == 0)
    {
        if (v1 == 0)
            res = GASNumberUtil::NaN();               // division 0 / 0
        else if (v1 >= 0)
            res = GASNumberUtil::POSITIVE_INFINITY(); // division v / 0
        else
            res = GASNumberUtil::NEGATIVE_INFINITY(); // division -v / 0
    }
    else
        res = v1 / v2; 
    SetNumber(res); 
}  

//////////////////////////////////////////////////////////////////////////
GASValueGuard::GASValueGuard(const GASEnvironment* penv, const GASValue& val)
: pEnv(penv), Value(val)
{
    GASSERT(pEnv);
    if (val.IsCharacter())
    {
        pChar = val.ToASCharacter(pEnv);
        if (pChar)
            pChar->AddRef();
    }
    else
        pChar = NULL;
}

GASValueGuard::~GASValueGuard()
{
    if (pChar)
        pChar->Release();
}

GASValueGuard& GASValueGuard::operator=(const GASValue& val)
{
    GASSERT(pEnv);
    Value = val;
    if (pChar)
        pChar->Release();
    if (val.IsCharacter())
    {
        pChar = val.ToASCharacter(pEnv);
        pChar->AddRef();
    }
    else
        pChar = NULL;
    return *this;
}

/////// GASFnCall //////
// Access a particular argument.
GASValue& GASFnCall::Arg(int n) const
{
    GASSERT(n < NArgs);
    return Env->Bottom(FirstArgBottomIndex - n);
}

// Logging script support
GFxLog* GASFnCall::GetLog() const
{ 
    return Env->GetLog();   
}

bool    GASFnCall::IsVerboseAction() const
{ 
    return Env->IsVerboseAction(); 
}

bool    GASFnCall::IsVerboseActionErrors() const
{ 
    return Env->IsVerboseActionErrors(); 
}

bool    GASFnCall::CheckThisPtr(UInt type) const
{
    if (!ThisPtr || ThisPtr->GetObjectType() != (GASObjectInterface::ObjectType)type)
    {
        return false;
    }
    return true;
}

void GASFnCall::ThisPtrError(const char* className, const char* psrcfile, int line) const
{
#ifdef GFC_BUILD_DEBUG
    const char* pf = strrchr(psrcfile, '\\');
    if (!pf)
        pf = strrchr(psrcfile, '/');
    if (pf)
        ++pf;
    else
        pf = psrcfile;
    Env->LogScriptError
        ("Error: Null or invalid 'this' is used for a method of %s class, file %s, line %d\n", 
        className, pf, line);
#else
    GUNUSED2(psrcfile, line);
    Env->LogScriptError
        ("Error: Null or invalid 'this' is used for a method of %s class.\n", 
        className);
#endif
}
