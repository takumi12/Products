/**********************************************************************

Filename    :   GASFunctionRef.h
Content     :   ActionScript implementation classes
Created     :   
Authors     :   

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXFUNCTIONREF_H
#define INC_GFXFUNCTIONREF_H

#include "GFxActionTypes.h"

// ***** Declared Classes
class GASFunctionDef;
class GASCFunctionDef;
class GASAsFunctionDef;
class GASFunctionRefBase;
class GASFunctionRef;
class GASFunctionWeakRef;

// ***** External Classes
class GASFnCall;
class GASEnvironment;
class GASActionBuffer;
class GASLocalFrame;
class GASFunctionObject;

// DO NOT use GASFunctionRefBase anywhere but as a base class for 
// GASFunctionRef and GASFunctionWeakRef. DO NOT add virtual methods,
// since GASFunctionRef and GASFunctionWeakRef should be very simple 
// and fast. DO NOT add ctor/dtor and assignment operators to GASFunctionRefBase
// since it is used in union.
class GASFunctionRefBase
{
//protected:
public:
    GASFunctionObject*      Function;
    GASLocalFrame*          LocalFrame; 
    enum FuncRef_Flags
    {
        FuncRef_Internal = 1,
        FuncRef_Weak     = 2
    };
    UByte                   Flags;
public:
    GINLINE void Init (GASFunctionObject* funcObj = 0, UByte flags = 0);
    GINLINE void Init (GASFunctionObject& funcObj, UByte flags = 0);
    GINLINE void Init (const GASFunctionRefBase& orig, UByte flags = 0);
    GINLINE void DropRefs();

    void SetLocalFrame (GASLocalFrame* localFrame, bool internal = false);

    bool IsInternal () const { return (Flags & FuncRef_Internal); }
    void SetInternal (bool internal = false);

    // pmethodName is used for creation of "super" object:
    // it is important to find real owner of the function object
    // to make "super" works correctly.
    GINLINE void Invoke(const GASFnCall& fn, const char* pmethodName = NULL) const;  

    GINLINE bool operator== (const GASFunctionRefBase& f) const;
    GINLINE bool operator== (const GASCFunctionPtr f) const;
    GINLINE bool operator!= (const GASFunctionRefBase& f) const;
    GINLINE bool operator!= (const GASCFunctionPtr f) const;

    GINLINE bool IsCFunction () const;
    GINLINE bool IsAsFunction () const;
    GINLINE bool IsNull () const { return Function == 0; }

    void Assign(const GASFunctionRefBase& orig);

    GINLINE GASFunctionObject*           operator-> () { return Function; }
    GINLINE GASFunctionObject*           operator-> () const { return Function; }
    GINLINE GASFunctionObject&           operator* () { return *Function; }
    GINLINE GASFunctionObject&           operator* () const { return *Function; }
    GINLINE GASFunctionObject*           GetObjectPtr () { return Function; }
    GINLINE GASFunctionObject*           GetObjectPtr () const { return Function; }

#ifndef GFC_NO_GC
    typedef GRefCountBaseGC<GFxStatMV_ActionScript_Mem>::Collector Collector;
    template <class Functor> void ForEachChild_GC(Collector* prcc) const;
    void                          Finalize_GC() {}
#endif //GFC_NO_GC
};

class GASFunctionRef : public GASFunctionRefBase
{
public:
    GINLINE GASFunctionRef(GASFunctionObject* funcObj = 0)  { Init(funcObj);  }
    GINLINE GASFunctionRef(GASFunctionObject& funcObj)      { Init(funcObj); }
    GINLINE GASFunctionRef(const GASFunctionRefBase& orig)
    {
        Init(orig);
    }
    GINLINE GASFunctionRef(const GASFunctionRef& orig)
    {
        Init(orig);
    }
    GINLINE ~GASFunctionRef()
    {
        DropRefs();
    }

    GINLINE const GASFunctionRefBase& operator = (const GASFunctionRefBase& orig)
    {
        Assign(orig);
        return *this;
    }
    GINLINE const GASFunctionRef& operator = (const GASFunctionRef& orig)
    {
        Assign(orig);
        return *this;
    }
};

#ifdef GFC_NO_GC
// used for "constructor" in prototypes, they should have 
// weak reference to the function, because constructor itself
// contains strong reference to its prototype.
// Convertible to and from regular GASFunctionRef
class GASFunctionWeakRef : public GASFunctionRefBase
{
public:
    GASFunctionWeakRef(GASFunctionObject* funcObj = 0) 
    { 
        Init(funcObj, FuncRef_Weak); 
        SetInternal(true);
    }
    GASFunctionWeakRef(const GASFunctionRefBase& orig)
    {
        Init(orig, FuncRef_Weak);
        SetInternal(true);
    }
    GASFunctionWeakRef(const GASFunctionWeakRef& orig)
    {
        Init(orig, FuncRef_Weak);
        SetInternal(true);
    }
    GINLINE ~GASFunctionWeakRef()
    {
        DropRefs();
    }
    GINLINE const GASFunctionRefBase& operator = (const GASFunctionRefBase& orig)
    {
        Assign(orig);
        return *this;
    }
    GINLINE const GASFunctionWeakRef& operator = (const GASFunctionWeakRef& orig)
    {
        Assign(orig);
        return *this;
    }
};
#endif // GFC_NO_GC

#endif // INC_GFXFUNCTIONREF_H

