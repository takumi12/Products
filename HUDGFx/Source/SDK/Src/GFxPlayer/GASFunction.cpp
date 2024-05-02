/**********************************************************************

Filename    :   GFxFunction.cpp
Content     :   Implementation of C and AS function objects
Created     :   
Authors     :   Artyom Bolgar

Copyright   :   (c) 2001-2008 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxPlayerImpl.h"
#include "GFxAction.h"
#include "GASFunctionRefImpl.h"
#include "AS/GASArrayObject.h"
#include "GFxASString.h"
#include "AS/GASStringObject.h"
#include "GASAsFunctionObject.h"

#include <stdio.h>
#include <stdlib.h>

//#define GFC_TRACK_STACK_USAGE_PER_AS_FUNCTION_CALL

//
////////// GASFunctionObject ///////////
//
#ifdef GFC_NO_GC
//////////////////////////////////////////////////////////////////////////
// non-CC versions
//
GASFunctionObject::GASFunctionObject (GASStringContext* psc) 
: GASObject(psc)
{
    GUNUSED(psc);
    // NOTE: psc can be 0 here to allow for default constructor; if we need 
    // a valid 'psc' here in the future we need to remove the default constructor.
    pPrototype = NULL;
}

GASFunctionObject::GASFunctionObject (GASEnvironment* penv) 
: GASObject(penv)
{
    GUNUSED(penv);
    pPrototype = NULL;
}

GASFunctionObject::~GASFunctionObject ()
{
    // Function object is being removed; make sure we cleanup
    // the prototype.constructor property to avoid crash.
    if (pPrototype)
    {
        pPrototype->CheckAndResetCtorRef(this);
    }
}

bool GASFunctionObject::SetMember(GASEnvironment *penv, 
                                  const GASString& name, 
                                  const GASValue& val, 
                                  const GASPropFlags& flags)
{
    if (name == penv->GetBuiltin(GASBuiltin_prototype))
    {   
        // Prototype is being set via ActionScript.
        // We need
        GASObject* pobj = val.ToObject(penv);
        if (pobj && pobj->IsBuiltinPrototype())
            pPrototype = static_cast<GASObjectProto*>(pobj);
        else
            pPrototype = NULL;
    }
    return GASObject::SetMember(penv, name, val, flags);
}

void GASFunctionObject::SetPrototype(GASStringContext* psc, GASObject* pprototype)
{
    pPrototype = pprototype;
    SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_prototype), GASValue(pprototype));
}
#else
////////////////////////////////////////////////
// CC versions
//
GASFunctionObject::GASFunctionObject (GASStringContext* psc) 
: GASObject(psc)
{
    GUNUSED(psc);
    // NOTE: psc can be 0 here to allow for default constructor; if we need 
    // a valid 'psc' here in the future we need to remove the default constructor.
}

GASFunctionObject::GASFunctionObject (GASEnvironment* penv) 
: GASObject(penv)
{
    GUNUSED(penv);
}

GASFunctionObject::~GASFunctionObject ()
{
}

void GASFunctionObject::SetPrototype(GASStringContext* psc, GASObject* pprototype)
{
    SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_prototype), GASValue(pprototype));
}
#endif // GFC_NO_GC

void GASFunctionObject::SetProtoAndCtor(GASStringContext* psc, GASObject* pprototype)
{
    Set__proto__(psc, pprototype);
    
    // function objects have "constructor" property as well
    GASFunctionRef ctor = pprototype->Get_constructor(psc);
    if (!ctor.IsNull())
        Set_constructor(psc, ctor);
}

GASFunctionRef  GASFunctionObject::ToFunction()
{
    return GASFunctionRef(this);
}

GASObject* GASFunctionObject::GetPrototype(GASStringContext* psc)
{
    GASValue prototypeVal;
    if (GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_prototype), &prototypeVal))
    {
        return prototypeVal.ToObject(NULL);
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
// GASCFunctionObject

GASCFunctionObject::GASCFunctionObject (GASStringContext* psc) 
: GASFunctionObject(psc), pFunction(NULL)
{
    GUNUSED(psc);
    // NOTE: psc can be 0 here to allow for default constructor; if we need 
    // a valid 'psc' here in the future we need to remove the default constructor.
}

GASCFunctionObject::GASCFunctionObject (GASEnvironment* penv) 
: GASFunctionObject(penv), pFunction(NULL) 
{
    GUNUSED(penv);
}

//GASCFunctionObject::GASCFunctionObject (GASEnvironment* penv, GASCFunctionDef* func) 
//: Def (func)
//{
//    GUNUSED(penv);
//}

// this constructor is used for C functions
GASCFunctionObject::GASCFunctionObject(GASStringContext* psc, GASObject* pprototype, GASCFunctionPtr func)
: GASFunctionObject(psc), pFunction(func)
{
    // C-function objects have __proto__ = Object.prototype
    //!AB, correction: C-function objects have __proto__ = Function.prototype (see GASFunctionObject::GetMember)
    //Set__proto__ (pprototype);
    //GUNUSED(pprototype); // not sure yet, is it necessary or not at all... (AB)
    Set__proto__(psc, pprototype);
}

GASCFunctionObject::GASCFunctionObject(GASStringContext* psc, GASCFunctionPtr func)
: GASFunctionObject(psc), pFunction(func)
{
    Set__proto__(psc, psc->pContext->GetPrototype(GASBuiltin_Function));
}

GASCFunctionObject::~GASCFunctionObject ()
{
}

void GASCFunctionObject::Invoke 
(const GASFnCall& fn, GASLocalFrame*, const char*) 
{
    if (0 != pFunction) {
        GASObjectInterface* pthis   = fn.ThisPtr;
        if (pthis && pthis->IsSuper())
        {
            GASObjectInterface* prealThis = static_cast<GASSuperObject*>(pthis)->GetRealThis();
            GASFnCall fn2(fn.Result, prealThis, fn.Env, fn.NArgs, fn.FirstArgBottomIndex);
            (pFunction)(fn2);
            static_cast<GASSuperObject*>(pthis)->ResetAltProto(); // reset alternative proto, if set
        }
        else
            (pFunction)(fn);
    }
    //Def.Invoke (fn, localFrame, NULL, pmethodName);
}

GASEnvironment* GASCFunctionObject::GetEnvironment(const GASFnCall& fn, GPtr<GFxASCharacter>*)
{
    return fn.Env;
}


bool GASCFunctionObject::IsEqual(const GASFunctionObject& f) const
{
    return f.IsCFunction() && pFunction == static_cast<const GASCFunctionObject&>(f).pFunction;
}



//////////////////////////////////////////////////////////////////////////
// GASAsFunctionObject

GASAsFunctionObject::GASAsFunctionObject(GASEnvironment* penv, GASActionBuffer* ab, 
                                         UInt start, UInt length,
                                         const GASWithStackArray* pwithStack,
                                         GASActionBuffer::ExecuteType execType)
:   GASFunctionObject(penv), 
    pMovieRoot(0),
    pActionBuffer(ab),    
    StartPc(start),
    Length(length),
    // Need to pass default value for copy-constructing array
    Args(ArgSpec(0, penv->GetBuiltin(GASBuiltin_empty_))),
    Function2Flags(0),
    ExecType((UByte)execType),
    LocalRegisterCount(0)
{
    GASSERT(pActionBuffer);

    if (pwithStack)
        WithStack = *pwithStack;

    if (penv && (execType!= GASActionBuffer::Exec_Event && execType!= GASActionBuffer::Exec_SpecialEvent))
    {
        GFxASCharacter* ch = penv->GetTarget();
        GASSERT (ch != 0);
        TargetHandle = ch->GetCharacterHandle();

        // MA: MovieRoot Lives in diferent heap now!
        pMovieRoot = ch->GetMovieRoot();
    }
}

GASObject* GASAsFunctionObject::CreateNewObject(GASEnvironment* penv) const
{
    return GHEAP_NEW(penv->GetHeap()) GASObject(penv);
}

GFxASCharacter* GASAsFunctionObject::GetTargetCharacter()
{    
    if (!pMovieRoot) return 0;
    return TargetHandle->ResolveCharacter(pMovieRoot);
}

GASEnvironment* GASAsFunctionObject::GetEnvironment(const GASFnCall& fn, GPtr<GFxASCharacter>* ptargetCh)
{
    GPtr<GFxASCharacter> ch = GetTargetCharacter();
    GASEnvironment*     pourEnv = 0;
    if (ch)
    {
        pourEnv = ch->GetASEnvironment();
    }
    if (ptargetCh)
        *ptargetCh = ch;

    GASSERT(pourEnv || fn.Env);

    pourEnv = pourEnv ? pourEnv : fn.Env; // the final decision about environment...
    return pourEnv;
}

#ifdef GFC_TRACK_STACK_USAGE_PER_AS_FUNCTION_CALL
volatile UInt32 __esp__ = 0, curESP = 0;
#endif

class InvokeContext
{
public:
    GASAsFunctionObject* pThis;
    GASEnvironment*     pOurEnv;
    const GASFnCall&    FnCall;
    GASLocalFrame*      pLocalFrame; 
    const char*         pMethodName;
    GPtr<GFxASCharacter> TargetCh; 
    GPtr<GFxASCharacter> FnEnvCh;
    GPtr<GFxASCharacter> PassedThisCh;
    GPtr<GASObject>      PassedThisObj;
    GPtr<GASLocalFrame> CurLocalFrame;
    int                 LocalStackTop;

    InvokeContext(GASAsFunctionObject* pthis, const GASFnCall& fn, GASLocalFrame* localFrame, const char* pmethodName)
        : pThis(pthis), FnCall(fn), pLocalFrame(localFrame), pMethodName(pmethodName)
    {
    }
    void Setup();
    void Cleanup();

private:
    InvokeContext& operator=(const InvokeContext&) { return *this; }
};

void InvokeContext::Setup() 
{
    pOurEnv->CallPush(pThis);

    // 'this' can be different from currently accessible environment scope.
    // In most 'this' is passed through FnCall, while the implicit scope
    // 'pEnv' is the scope of the original function declaration.

    GASObjectInterface* pthis   = FnCall.ThisPtr;
    //GASSERT(pthis); // pthis can be null (i.e. undefined), for example inside interval's handler

    GMemoryHeap* pheap = pOurEnv->GetHeap();

    // Set up local stack frame, for parameters and locals.
    LocalStackTop = pOurEnv->GetLocalFrameTop();

    if (pThis->IsFunction2() || (pThis->GetExecType() == GASActionBuffer::Exec_Function))
    {
        CurLocalFrame = pOurEnv->CreateNewLocalFrame();
        CurLocalFrame->PrevFrame = pLocalFrame;
    }
    else
        pOurEnv->AddFrameBarrier();

    GASObjectInterface* passedThis = pthis;
    // need to hold passedThis to avoid its release during the function execution
    if (passedThis)
    {
        PassedThisCh  = passedThis->ToASCharacter();
        PassedThisObj = passedThis->ToASObject();
    }

    if (pthis && pthis->IsSuper())
    {
        pthis = static_cast<GASSuperObject*>(pthis)->GetRealThis();
    }

    if (!pThis->IsFunction2())
    {
        int i;
        int version = pOurEnv->GetVersion();

        // AB, Flash 6 and below uses function 1 for methods and events too.
        // So, we need to care about setting correct "this".
        if (pthis)
        {
            GASValue thisVal;
            thisVal.SetAsObjectInterface(pthis);
            pOurEnv->AddLocal(pOurEnv->GetBuiltin(GASBuiltin_this), thisVal);
        }

        // save some info for recreation of "super". For function1 "super" is created
        // on demand.
        if (version >= 6 && CurLocalFrame)
        {
            CurLocalFrame->SuperThis = passedThis;
        }
        // Conventional function.

        if (CurLocalFrame && FnCall.Env && version >= 5)
        {
            // Optimization: function, type 1 may use "arguments" array and there is no
            // way to predict this usage. To avoid creation of this array for every call
            // (especially, the "arguments" array is used pretty rarely), we just
            // save some info, required for "lazy" creation of this array in the
            // GASEnvironment::GetVariableRaw method.
            if (pOurEnv != FnCall.Env)
                FnEnvCh = FnCall.Env->GetTarget(); // keep target char for FnCall.Env to ensure 
            // it won't be freed before call is completed.
            CurLocalFrame->Env                  = FnCall.Env;
            CurLocalFrame->NArgs                = FnCall.NArgs;
            CurLocalFrame->FirstArgBottomIndex  = FnCall.FirstArgBottomIndex;
            CurLocalFrame->Callee               = pOurEnv->CallTop(0);
            CurLocalFrame->Caller               = pOurEnv->CallTop(1);
        }

        // Push the arguments onto the local frame.
        int ArgsToPass = G_Min<int>(FnCall.NArgs, (int)pThis->Args.GetSize());
        for (i = 0; i < ArgsToPass; i++)
        {
            GASSERT(pThis->Args[i].Register == 0);
            pOurEnv->AddLocal(pThis->Args[i].Name, FnCall.Arg(i));
        }
        // need to fill remaining arguments by "undefined" values
        for (int n = (int)pThis->Args.GetSize(); i < n; i++)
        {
            GASSERT(pThis->Args[i].Register == 0);
            pOurEnv->AddLocal(pThis->Args[i].Name, GASValue());
        }
    }
    else
    {
        // function2: most args go in registers; any others get pushed.

        // Create local registers.
        pOurEnv->AddLocalRegisters(pThis->LocalRegisterCount);

        // Handle the explicit args.
        int ArgsToPass = G_Min<int>(FnCall.NArgs, (int)pThis->Args.GetSize());
        int i;
        for (i = 0; i < ArgsToPass; i++)
        {
            if (pThis->Args[i].Register == 0)
            {
                // Conventional arg passing: create a local var.
                pOurEnv->AddLocal(pThis->Args[i].Name, FnCall.Arg(i));
            }
            else
            {
                // Pass argument into a register.
                int reg = pThis->Args[i].Register;
                *(pOurEnv->LocalRegisterPtr(reg)) = FnCall.Arg(i);
            }
        }
        // need to fill remaining aguments by "undefined" values
        for (int n = (int)pThis->Args.GetSize(); i < n; i++)
        {
            if (pThis->Args[i].Register == 0)
            {
                // Conventional arg passing: create a local var.
                pOurEnv->AddLocal(pThis->Args[i].Name, GASValue());
            }
        }

        GPtr<GASSuperObject> superObj;
        if ((pThis->Function2Flags & 0x10) || !(pThis->Function2Flags & 0x20))
        {
            // need to create super
            GASSERT(passedThis);

            GPtr<GASObject> proto = passedThis->Get__proto__();
            //printf ("!!! passedThis.__proto__ = %s\n", (const char*)pOurEnv->GetGC()->FindClassName(pOurEnv->GetSC(), proto).ToCStr());
            //printf ("!!! real pthis.__proto__ = %s\n", (const char*)pOurEnv->GetGC()->FindClassName(pOurEnv->GetSC(), pthis->Get__proto__()).ToCStr());
            if (proto)
            {
                if (pMethodName)
                {
                    // Correct the prototype according to the owner of the function.
                    // This is important in the case if "this"'s class do not have
                    // the method being invoked and only the base class has it.
                    GPtr<GASObject> newProto = proto->FindOwner(pOurEnv->GetSC(), pOurEnv->CreateString(pMethodName));
                    if (newProto)
                    {
                        proto = newProto;
                        //printf ("!!! newproto = %s\n", (const char*)pOurEnv->GetGC()->FindClassName(pOurEnv->GetSC(), newProto).ToCStr());
                    }
                }
                GASFunctionRef __ctor__ = proto->Get__constructor__(pOurEnv->GetSC());
                //printf ("!!! __proto__.__ctor__ = %s\n", (const char*)pOurEnv->GetGC()->FindClassName(pOurEnv->GetSC(), __ctor__.GetObjectPtr()).ToCStr());
                //printf ("!!! __proto__.__proto__ = %s\n", (const char*)pOurEnv->GetGC()->FindClassName(pOurEnv->GetSC(), proto->Get__proto__()).ToCStr());
                superObj = *GHEAP_NEW(pheap) GASSuperObject(proto->Get__proto__(), pthis, __ctor__);
            }
        }

        // Handle the implicit args.
        int CurrentReg = 1;
        if (pThis->Function2Flags & 0x01)
        {
            // preload 'this' into a register.
            if (pthis)
                (*(pOurEnv->LocalRegisterPtr(CurrentReg))).SetAsObjectInterface(pthis);
            else
                (*(pOurEnv->LocalRegisterPtr(CurrentReg))).SetUndefined();
            CurrentReg++;
        }

        if (pThis->Function2Flags & 0x02)
        {
            // Don't put 'this' into a local var.
        }
        else
        {
            // Put 'this' in a local var.
            GASValue pthisVal;
            if (pthis) pthisVal.SetAsObjectInterface(pthis);
            pOurEnv->AddLocal(pOurEnv->GetBuiltin(GASBuiltin_this), pthisVal);
        }

        // Init arguments GArray, if it's going to be needed.
        GPtr<GASArrayObject>    pargArray;
        if ((pThis->Function2Flags & 0x04) || ! (pThis->Function2Flags & 0x08))
        {
            pargArray = *GHEAP_NEW(pheap) GASArrayObject(pOurEnv);
            pargArray->Resize(FnCall.NArgs);
            for (int i = 0; i < FnCall.NArgs; i++)
                pargArray->SetElement(i, FnCall.Arg(i));
        }

        if (pThis->Function2Flags & 0x04)
        {
            // preload 'arguments' into a register.
            (*(pOurEnv->LocalRegisterPtr(CurrentReg))).SetAsObject(pargArray.GetPtr());
            CurrentReg++;
        }

        if (pThis->Function2Flags & 0x08)
        {
            // Don't put 'arguments' in a local var.
        }
        else
        {
            // Put 'arguments' in a local var.
            pOurEnv->AddLocal(pOurEnv->GetBuiltin(GASBuiltin_arguments), GASValue(pargArray.GetPtr()));
            pargArray->SetMemberRaw(pOurEnv->GetSC(), pOurEnv->GetBuiltin(GASBuiltin_callee), pOurEnv->CallTop(0), 
                GASPropFlags::PropFlag_DontEnum | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_ReadOnly);
            pargArray->SetMemberRaw(pOurEnv->GetSC(), pOurEnv->GetBuiltin(GASBuiltin_caller), pOurEnv->CallTop(1), 
                GASPropFlags::PropFlag_DontEnum | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_ReadOnly);
        }

        if (pThis->Function2Flags & 0x10)
        {
            // Put 'super' in a register.
            //GASSERT(superObj);

            (*(pOurEnv->LocalRegisterPtr(CurrentReg))).SetAsObject(superObj);
            //(*(pOurEnv->LocalRegisterPtr(CurrentReg))).SetAsFunction(superObj);
            CurrentReg++;
        }

        if (pThis->Function2Flags & 0x20)
        {
            // Don't put 'super' in a local var.
        }
        else
        {
            // Put 'super' in a local var.
            //GASSERT(superObj);

            GASValue superVal;
            superVal.SetAsObject(superObj);
            pOurEnv->AddLocal(pOurEnv->GetBuiltin(GASBuiltin_super), superVal);
        }

        if (pThis->Function2Flags & 0x40)
        {
            // Put '_root' in a register.
            (*(pOurEnv->LocalRegisterPtr(CurrentReg))).SetAsCharacter(
                pOurEnv->GetTarget()->GetASRootMovie());
            CurrentReg++;
        }

        if (pThis->Function2Flags & 0x80)
        {
            // Put 'Parent' in a register.
            GASValue    parent; 
            pOurEnv->GetVariable(pOurEnv->GetBuiltin(GASBuiltin__parent), &parent);
            (*(pOurEnv->LocalRegisterPtr(CurrentReg))) = parent;
            CurrentReg++;
        }

        if (pThis->Function2Flags & 0x100)
        {
            // Put 'pGlobal' in a register.
            (*(pOurEnv->LocalRegisterPtr(CurrentReg))).SetAsObject(pOurEnv->GetGC()->pGlobal);
            CurrentReg++;
        }
    }
}

void InvokeContext::Cleanup()
{
    GASObjectInterface* passedThis = PassedThisObj.GetPtr();
    if (passedThis && passedThis->IsSuper())
    {
        static_cast<GASSuperObject*>(passedThis)->ResetAltProto(); // reset alternative proto, if set
    }

    if (!pThis->IsFunction2() || !(pThis->Function2Flags & 0x02))
    {
        // wipe explicit "this" off from the local frame to prevent memory leak
        pOurEnv->SetLocal(pOurEnv->GetBuiltin(GASBuiltin_this), GASValue());
    }
    if (!pThis->IsFunction2() || !(pThis->Function2Flags & 0x20))
    {
        // wipe explicit "super" off from the local frame to prevent memory leak
        pOurEnv->SetLocal(pOurEnv->GetBuiltin(GASBuiltin_super), GASValue());
    }

    if (CurLocalFrame)
        CurLocalFrame->ReleaseFramesForLocalFuncs ();

    // Clean up stack frame.
    pOurEnv->SetLocalFrameTop(LocalStackTop);

    if (pThis->IsFunction2())
    {
        // Clean up the local registers.
        pOurEnv->DropLocalRegisters(pThis->LocalRegisterCount);
    }

    // finalize...
    if (pOurEnv)
        pOurEnv->CallPop();
}

void GASAsFunctionObject::Invoke(const GASFnCall& fn, GASLocalFrame* localFrame, const char* pmethodName) 
{
    // do not make calls on unloaded ThisPtr
    if (fn.ThisPtr && fn.ThisPtr->IsASCharacter() && fn.ThisPtr->ToASCharacter()->IsUnloaded())
        return;

    InvokeContext ctxt(this, fn, localFrame, pmethodName);
    // push current function object into the stack.
    // need for atguments.callee/.caller.
    ctxt.pOurEnv = GetEnvironment(fn, &ctxt.TargetCh);

    if (ctxt.pOurEnv->GetTarget()->IsUnloaded())
    {
        // Do not use environment from the unloaded target, use caller environment instead.
        ctxt.pOurEnv = fn.Env;
    }
    else
    {
        // if environment (pourEnv) is not NULL then use it (and it MUST be the same as 
        // GASAsFunctionDef::GetEnvironment method returns, otherwise - call to GASAsFunctionDef::GetEnvironment
        GASSERT(ctxt.pOurEnv == GetEnvironment(fn, 0));
    }
    GASSERT(ctxt.pOurEnv);

#ifdef GFC_TRACK_STACK_USAGE_PER_AS_FUNCTION_CALL
    //volatile UInt32 curESP;
    #ifdef GFC_OS_WII
    asm 
    {   
        stw rsp, curESP  
    }
    #elif defined(GFC_OS_XBOX360)
    __asm stw r1, curESP
    #elif defined(GFC_OS_PS3)
    volatile UInt32 curESP;
    __asm__ volatile ("stw 1, %0\n\t" : "Z=" (curESP));
    #elif defined(GFC_OS_WIN32) && defined(GFC_CC_MSVC)
    __asm mov curESP, esp;
    #endif
    if (__esp__ != 0)
        printf("Call size is %d\n", __esp__ - curESP);
    __esp__ = curESP;
#endif // GFC_TRACK_STAC_USAGE_PER_AS_FUNCTION_CALL

    if (!ctxt.pOurEnv->RecursionGuardStart())
    {
        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
        if (ctxt.pOurEnv->IsVerboseActionErrors())
            ctxt.pOurEnv->LogScriptError("Error: Stack overflow, max level of 255 nested calls is reached.\n");
        #endif
        ctxt.pOurEnv->RecursionGuardEnd();
        return;
    }

    ctxt.Setup();
    
    // Execute the actions.
    pActionBuffer->Execute(ctxt.pOurEnv, StartPc, Length, fn.Result, &WithStack, GetExecType());

    ctxt.Cleanup();

    ctxt.pOurEnv->RecursionGuardEnd();
}

//
//////////// GASFunctionRefBase //////////////////
//
void GASFunctionRefBase::Assign(const GASFunctionRefBase& orig)
{
    if (this != &orig)
    {
        GASFunctionObject* pprevFunc = Function;
        if (!(Flags & FuncRef_Weak) && Function && (Function != orig.Function))
            Function->Release ();

        Function = orig.Function;

        // avoiding load-hit-store, access to orig.Function rather than Function
        if (!(Flags & FuncRef_Weak) && orig.Function && (pprevFunc != orig.Function))
            orig.Function->AddRef ();

        if (orig.LocalFrame != 0) 
            SetLocalFrame (orig.LocalFrame, (orig.Flags & FuncRef_Internal));
        else
            SetLocalFrame (0, 0);
    }
}

void GASFunctionRefBase::SetInternal (bool internal)
{
#ifdef GFC_NO_GC
    if (LocalFrame != 0 && bool(Flags & FuncRef_Internal) != internal)
    {
        if (!(Flags & FuncRef_Internal))
            LocalFrame->Release ();
        else
            LocalFrame->AddRef ();
    }
    if (internal) Flags |= FuncRef_Internal;
    else          Flags &= (~FuncRef_Internal);
#else
    GUNUSED(internal);
#endif
}

void GASFunctionRefBase::SetLocalFrame (GASLocalFrame* localFrame, bool internal)
{
    if (LocalFrame != 0 && !(Flags & FuncRef_Internal))
        LocalFrame->Release ();
    LocalFrame = localFrame;
    if (internal) Flags |= FuncRef_Internal;
    else          Flags &= (~FuncRef_Internal);

    // avoiding load-hit-store, access to localFrame rather than LocalFrame
    if (localFrame != 0 && !(Flags & FuncRef_Internal))
        localFrame->AddRef ();
}

#ifndef GFC_NO_GC
template <class Functor>
void GASFunctionRefBase::ForEachChild_GC(Collector* prcc) const
{
    if (Function)
        Functor::Call(prcc, Function);
    if (LocalFrame)
        Functor::Call(prcc, LocalFrame);
}
GFC_GC_PREGEN_FOR_EACH_CHILD(GASFunctionRefBase)

#endif // GFC_NO_GC

///////////// GASFunctionProto //////////////////
static const GASNameFunction GAS_FunctionObjectTable[] = 
{
    { "apply",               &GASFunctionProto::Apply },
    { "call",                &GASFunctionProto::Call  },
    { "toString",            &GASFunctionProto::ToString },
    { "valueOf",             &GASFunctionProto::ValueOf },
    { 0, 0 }
};

GASFunctionProto::GASFunctionProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor, bool initFuncs) : 
    GASPrototype<GASObject>(psc, pprototype, constructor)
{
    if (initFuncs)
        InitFunctionMembers(psc, GAS_FunctionObjectTable);
}

void GASFunctionProto::GlobalCtor(const GASFnCall& fn)
{
    if (fn.NArgs == 1)
    {
        // Seems this is just a type casting call, such as "Function(func)"
        // Just return the unchanged parameter.
        // Flash should generate a special opcode for type casting, and it does
        // do this for casts like "s : Selection = Selection(ss)". 
        // But, for some reasons, if the cast operator looks like "f = Function(func)"
        // Flash generates a function call to global function "Function" with a param.
        // In this case, just return the passed parameter. Do the same, if 
        // "f = new Function(func) is used; in this case f == func anyway.
        if (fn.Arg(0).IsFunction() || fn.Arg(0).IsFunctionName())
            fn.Result->SetAsObject(fn.Arg(0).ToObject(fn.Env));
        else
            fn.Result->SetNull();
    }
    else
    {
        GPtr<GASCFunctionObject> obj = *GHEAP_NEW(fn.Env->GetHeap()) GASCFunctionObject(fn.Env);
        fn.Result->SetAsObject(obj.GetPtr());
    }
}

void GASFunctionProto::ToString(const GASFnCall& fn)
{
    fn.Result->SetString(fn.Env->GetBuiltin(GASBuiltin_typeFunction_));
}

void GASFunctionProto::ValueOf(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    fn.Result->SetAsObject(static_cast<GASObject*>(fn.ThisPtr));
}

void GASFunctionProto::Apply(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    GPtr<GASObject> objectHolder;
    GPtr<GFxASCharacter> charHolder;
    GASObjectInterface* thisObj = 0;
    int nArgs = 0;

    fn.Result->SetUndefined();
    GPtr<GASArrayObject> arguments;

    if (fn.NArgs >= 1)
    {
        thisObj = fn.Arg(0).ToObjectInterface(fn.Env);
        if (thisObj)
        {
            if (thisObj->IsASCharacter())
                charHolder = thisObj->ToASCharacter();
            else
                objectHolder = static_cast<GASObject*>(thisObj);
        }
    }
    if (fn.NArgs >= 2)
    {
        // arguments array
        GASObject* args = fn.Arg(1).ToObject(fn.Env);
        if (args && args->GetObjectType() == GASObjectInterface::Object_Array)
        {
            arguments = static_cast<GASArrayObject*>(args);
            nArgs = arguments->GetSize();

            // push arguments into the environment's stack
            if (nArgs > 0)
            {
                for (int i = nArgs - 1; i >= 0; --i)
                    fn.Env->Push(*arguments->GetElementPtr(i));
            }
        }
    }
    
    GASValue result;
    // invoke function
    if (!fn.ThisFunctionRef.IsNull())
    {
        //!AB: if ThisFunctionRef is not null then we may do call with using it.
        // In this case localFrame will be correct, and this is critical for calling nested functions.
        fn.ThisFunctionRef.Invoke(GASFnCall(&result, thisObj, fn.Env, nArgs, fn.Env->GetTopIndex()));
    }
    else //!AB: is it necessary or ThisFunctionRef should be not null allways?
    {
        GPtr<GASFunctionObject> func = static_cast<GASFunctionObject*>(fn.ThisPtr);
        func->Invoke(GASFnCall(&result, thisObj, fn.Env, nArgs, fn.Env->GetTopIndex()), 0, NULL);
    }
    
    // wipe out arguments
    if (nArgs > 0)
    {
        fn.Env->Drop(nArgs);
    }
    *fn.Result = result;
}

void GASFunctionProto::Call(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    GPtr<GASObject> objectHolder;
    GPtr<GFxASCharacter> charHolder;
    GASObjectInterface* thisObj = 0;
    int nArgs = 0;

    fn.Result->SetUndefined();
    GPtr<GASArrayObject> arguments;

    if (fn.NArgs >= 1)
    {
        thisObj = fn.Arg(0).ToObjectInterface(fn.Env);
        if (thisObj)
        {
            if (thisObj->IsASCharacter())
                charHolder = thisObj->ToASCharacter();
            else
                objectHolder = static_cast<GASObject*>(thisObj);
        }
    }
    if (fn.NArgs >= 2)
    {
        nArgs = fn.NArgs - 1;

        for (int i = nArgs; i >= 1; --i)
            fn.Env->Push(fn.Arg(i));
    }

    GASValue result;
    // invoke function
    if (!fn.ThisFunctionRef.IsNull())
    {
        //!AB: if ThisFunctionRef is not null then we may do call with using it.
        // In this case localFrame will be correct, and this is critical for calling nested functions.
        fn.ThisFunctionRef.Invoke(GASFnCall(&result, thisObj, fn.Env, nArgs, fn.Env->GetTopIndex()));
    }
    else //!AB: is it necessary or ThisFunctionRef should be not null allways?
    {
        GPtr<GASFunctionObject> func = static_cast<GASFunctionObject*>(fn.ThisPtr);
        func->Invoke(GASFnCall(&result, thisObj, fn.Env, nArgs, fn.Env->GetTopIndex()), 0, NULL);
    }

    // wipe out arguments
    if (nArgs > 0)
    {
        fn.Env->Drop(nArgs);
    }
    *fn.Result = result;
}

/////////////////////////////////////////////////////
GASFunctionCtorFunction::GASFunctionCtorFunction (GASStringContext* psc) :
    GASCFunctionObject(psc, GASFunctionProto::GlobalCtor)
{
    GUNUSED(psc);
}
