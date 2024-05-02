/**********************************************************************

Filename    :   GFxAsFunctionObject.h
Content     :   ActionScript function object
Created     :   May 5, 2008
Authors     :   Artyom Bolgar

Copyright   :   (c) 2001-2008 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GASASFUNCOBJ_H
#define INC_GASASFUNCOBJ_H

#include "GFxPlayerImpl.h"

// Function object for ActionScript functions ("As-functions")
class GASAsFunctionObject : public GASFunctionObject 
{
    friend class GASFunctionObject;
    friend class InvokeContext;
protected:
    GFxMovieRoot*               pMovieRoot;
    GPtr<GFxCharacterHandle>    TargetHandle;

    GPtr<GASActionBuffer>       pActionBuffer;
    GASWithStackArray           WithStack;      // initial with-stack on function entry.
    UInt                        StartPc;
    UInt                        Length;

    struct ArgSpec
    {
        int         Register;
        GASString   Name;

        ArgSpec(int r, const GASString &n)
            : Register(r), Name(n) { }
        ArgSpec(const ArgSpec& s)
            : Register(s.Register), Name(s.Name) { }
    };

    GArrayCC<ArgSpec, GFxStatMV_ActionScript_Mem> Args;
    UInt16                  Function2Flags; // used by function2 to control implicit arg register assignments
    UByte                   ExecType;   // GASActionBuffer::ExecuteType - Function2, Event, etc.    
    UByte                   LocalRegisterCount;

    GASAsFunctionObject(GASStringContext* psc);
    GASAsFunctionObject(GASEnvironment* penv);

#ifndef GFC_NO_GC
    void Finalize_GC()
    {
        Args.~GArrayCC();
        WithStack.~GASWithStackArray();
        pActionBuffer   = NULL;
        TargetHandle    = NULL;
        //pMovieRoot = 0;
        GASFunctionObject::Finalize_GC();
    }
#endif // GFC_NO_GC
public:
    GASAsFunctionObject(GASEnvironment* penv, GASActionBuffer* ab,  
        UInt start, UInt length,
        const GASWithStackArray* pwithStack,
        GASActionBuffer::ExecuteType execType = GASActionBuffer::Exec_Function);

    virtual GASEnvironment* GetEnvironment(const GASFnCall& fn, GPtr<GFxASCharacter>* ptargetCh);
    virtual void Invoke (const GASFnCall& fn, GASLocalFrame*, const char* pmethodName);

    virtual bool IsNull () const { return false; }
    virtual bool IsEqual(const GASFunctionObject&) const { return false; } // not impl

    virtual bool IsAsFunction() const  { return true; }

    //virtual GASFunctionRef  ToFunction();

    // returns number of arguments expected by this function;
    // returns -1 if number of arguments is unknown (for C functions)
    int              GetNumArgs()      const { return (UInt)Args.GetSize(); }
    const ArgSpec*   GetArg(int i)     const { return &Args[i]; }
    GASActionBuffer* GetActionBuffer() const { return pActionBuffer; }
    UInt             GetStartPC()      const { return StartPc; }
    UInt             GetLength()       const { return Length; }

    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;

    // AS-function specific methods
    void    SetExecType(GASActionBuffer::ExecuteType execType)      { ExecType = (UByte)execType; }
    GASActionBuffer::ExecuteType GetExecType() const                { return (GASActionBuffer::ExecuteType)ExecType; }

    bool    IsFunction2() const { return ExecType == GASActionBuffer::Exec_Function2; }

    void    SetLocalRegisterCount(UByte ct) { GASSERT(IsFunction2()); LocalRegisterCount = ct; }
    void    SetFunction2Flags(UInt16 flags) { GASSERT(IsFunction2()); Function2Flags = flags; }

    class GFxASCharacter* GetTargetCharacter();

    const ArgSpec&  AddArg(int argRegister, const GASString& name)
    {
        GASSERT(argRegister == 0 || IsFunction2() == true);
        Args.Resize(Args.GetSize() + 1);
        ArgSpec& arg = Args.Back();
        arg.Register = argRegister;
        arg.Name = name;
        return arg;
    }

    void    SetLength(int len) { GASSERT(len >= 0); Length = len; }

};

#endif //INC_GASASFUNCOBJ_H
