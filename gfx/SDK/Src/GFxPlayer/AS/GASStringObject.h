/**********************************************************************

Filename    :   GASStringObject.h
Content     :   Implementation of AS2 String class
Created     :   
Authors     :   Maxim Shemanarev, Artem Bolgar

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXSTRINGOBJECT_H
#define INC_GFXSTRINGOBJECT_H

#include "GFxAction.h"
#include "AS/GASObjectProto.h"


// ***** Declared Classes
class GASStringObject;
class GASStringProto;
class GASStringCtorFunction;

// ***** External Classes
class GASArrayObject;
class GASEnvironment;



class GASStringObject : public GASObject
{
protected:
    GASString Value;

    void commonInit (GASEnvironment* penv);
    
    GASStringObject(GASStringContext *psc, GASObject* pprototype) :
        GASObject(psc), Value(psc->GetBuiltin(GASBuiltin_empty_))
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }
#ifndef GFC_NO_GC
protected:
    virtual void Finalize_GC()
    {
        Value.~GASString();
        GASObject::Finalize_GC();
    }
#endif //GFC_NO_GC
public:
    GASStringObject(GASEnvironment* penv);
    GASStringObject(GASEnvironment* penv, const GASString& val);

    bool                GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val);
    const char*         GetTextValue(GASEnvironment* =0) const;
    ObjectType          GetObjectType() const   { return Object_String; }

    const GASString&    GetString() const               { return Value; }
    void                SetString(const GASString& val) { Value = val; }

    virtual void        SetValue(GASEnvironment* penv, const GASValue&);
    virtual GASValue    GetValue() const;
};

class GASStringProto : public GASPrototype<GASStringObject>
{
public:
    GASStringProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);

    static void StringCharAt(const GASFnCall& fn);
    static void StringCharCodeAt(const GASFnCall& fn);
    static void StringConcat(const GASFnCall& fn);
    static void StringIndexOf(const GASFnCall& fn);
    static void StringLastIndexOf(const GASFnCall& fn);
    static void StringLocaleCompare(const GASFnCall& fn);
    static void StringSlice(const GASFnCall& fn);
    static void StringSplit(const GASFnCall& fn);        
    static void StringSubstr(const GASFnCall& fn);
    static void StringSubstring(const GASFnCall& fn);
    static void StringToLowerCase(const GASFnCall& fn);
    static void StringToString(const GASFnCall& fn);
    static void StringToUpperCase(const GASFnCall& fn);
    static void StringValueOf(const GASFnCall& fn);

    // Helper methods.
    static GASString StringSubstring(const GASString& str, 
                                     int start, int length);
    static GPtr<GASArrayObject> StringSplit(GASEnvironment* penv, const GASString& str,
                                            const char* delimiters, int limit = 0x3FFFFFFF);

    // Creates a ASString based on two char* pointers
    static GASString CreateStringFromCStr(GASStringContext* psc, const char* start, const char* end = 0);
};

class GASStringCtorFunction : public GASCFunctionObject
{
    static const GASNameFunction StaticFunctionTable[];

    static void StringFromCharCode(const GASFnCall& fn);
public:
    GASStringCtorFunction(GASStringContext *psc);

    virtual GASObject* CreateNewObject(GASEnvironment *penv) const;

    static void GlobalCtor(const GASFnCall& fn);
    static GASFunctionRef Register(GASGlobalContext* pgc);
};


#endif // INC_GFXSTRINGOBJECT_H
