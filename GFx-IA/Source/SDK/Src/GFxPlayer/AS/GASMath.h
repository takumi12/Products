/**********************************************************************

Filename    :   AS/GASMath.h
Content     :   AS2 Implementation of Math class
Created     :   January 6, 2009
Authors     :   Prasad Silva

Notes       :   
History     :   

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxMath_H
#define INC_GFxMath_H

#include "AS/GASObject.h"
#include "AS/GASObjectProto.h"
#include "GFxPlayerImpl.h"

class GASMath : public GASObject
{
protected:
    GASMath(GASStringContext *psc, GASObject* pprototype)
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }

public:
    GASMath(GASEnvironment* penv);

    static UInt32 GetNextRandom(GFxMovieRoot* proot);
};

class GASMathProto : public GASPrototype<GASMath>
{
public:
    GASMathProto (GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor);
};

// One-argument functions
#define GFC_AS2MATH_WRAP_FUNC1(funcname, cfunc)         \
    static GINLINE void funcname(const GASFnCall& fn)   \
    {                                                   \
        GASNumber   arg = fn.Arg(0).ToNumber(fn.Env);   \
        fn.Result->SetNumber((GASNumber)cfunc(arg));    \
    }

// Two-argument functions.
#define GFC_AS2MATH_WRAP_FUNC2(funcname, expr)          \
    static GINLINE void funcname(const GASFnCall& fn)   \
    {                                                   \
        GASNumber   arg0 = fn.Arg(0).ToNumber(fn.Env);  \
        GASNumber   arg1 = fn.Arg(1).ToNumber(fn.Env);  \
        fn.Result->SetNumber((GASNumber)expr);          \
    }

//
// Math static class
//
class GASMathCtorFunction : public GASCFunctionObject
{
    static const GASNameFunction StaticFunctionTable[];
    
    GFC_AS2MATH_WRAP_FUNC1(Abs, fabs);
    GFC_AS2MATH_WRAP_FUNC1(Acos, acos);
    GFC_AS2MATH_WRAP_FUNC1(Asin, asin);
    GFC_AS2MATH_WRAP_FUNC1(Atan, atan);
    GFC_AS2MATH_WRAP_FUNC1(Ceil, ceil);
    GFC_AS2MATH_WRAP_FUNC1(Cos, cos);
    GFC_AS2MATH_WRAP_FUNC1(Exp, exp);
    GFC_AS2MATH_WRAP_FUNC1(Floor, floor);
    GFC_AS2MATH_WRAP_FUNC1(Log, log);
    GFC_AS2MATH_WRAP_FUNC1(Sin, sin);
    GFC_AS2MATH_WRAP_FUNC1(Sqrt, sqrt);
    GFC_AS2MATH_WRAP_FUNC1(Tan, tan);

    GFC_AS2MATH_WRAP_FUNC2(Atan2, (atan2(arg0, arg1)));
    GFC_AS2MATH_WRAP_FUNC2(Max, (arg0 > arg1 ? arg0 : arg1));
    GFC_AS2MATH_WRAP_FUNC2(Min, (arg0 < arg1 ? arg0 : arg1));
    GFC_AS2MATH_WRAP_FUNC2(Pow, (pow(arg0, arg1)));

    static void Random(const GASFnCall& fn);
    static void Round(const GASFnCall& fn);

public:
    GASMathCtorFunction (GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);
    virtual GASObject* CreateNewObject(GASEnvironment*) const { return 0; }

    static GASFunctionRef Register(GASGlobalContext* pgc);
};

#endif // INC_GFxMath_H
