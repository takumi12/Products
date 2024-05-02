/**********************************************************************

Filename    :   GFxAction.h
Content     :   ActionScript implementation classes
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXACTION_H
#define INC_GFXACTION_H

#include "GFxActionTypes.h"
#include "GASValue.h"
#include "AS/GASObject.h"

#if !defined(GFC_OS_SYMBIAN) && !defined(GFC_CC_RENESAS) && !defined(GFC_OS_PS2)
#include <wchar.h>
#endif

// ***** Declared Classes
class GASLocalFrame;
class GASEnvironment;
class GASGlobalContext; 

// ***** External Classes
class GFxStream;
class GFxASCharacter;
class GFxMovieRoot;
class GFxCharacter;

#if !defined(GFC_NO_FXPLAYER_VERBOSE_ACTION) || !defined(GFC_NO_FXPLAYER_VERBOSE_PARSE_ACTION)
// Disassemble one instruction to the log.
class GFxDisasm
{
    GFxLog *                pLog;
    GFxLog::LogMessageType  MsgType;
public:

    GFxDisasm(GFxLog *plog, GFxLog::LogMessageType msgType)
    { pLog = plog; MsgType = msgType; }

    // Formatted version used
    void    Log(const char* pfmt, ...)
    {
        if (pLog)
        {
            va_list argList;
            va_start(argList, pfmt);
            pLog->LogMessageVarg(MsgType, pfmt, argList);
            va_end(argList);
        }
    }

    void LogDisasm(const unsigned char* InstructionData);
};
#endif

// Helper class used to log messages during action execution
class GFxActionLogger : public GFxLogBase<GFxActionLogger>
{
    GFxLog *    pLog;
    bool        VerboseAction;
    bool        VerboseActionErrors;
    bool        UseSuffix; //print LogSuffix after message 
    const char* LogSuffix; 
public:         
    GFxActionLogger(GFxCharacter *ptarget, const char* suffixStr = NULL);
    // GFxLogBase implementation
    GFxLog* GetLog() const          { return pLog;  }
    bool    IsVerboseAction() const { return VerboseAction; }
    bool    IsVerboseActionErrors() const   { return VerboseActionErrors; }
    void    LogScriptMessageVarg(LogMessageType messageType, const char* pfmt, va_list argList);
    void    LogScriptError(const char* pfmt,...);
    void    LogScriptWarning(const char* pfmt,...);
    void    LogScriptMessage(const char* pfmt,...);

    // Formatted version used
    GINLINE void    LogDisasm(const unsigned char* instructionData);
};

// ***** GASEnvironment
#ifdef GFC_BUILD_DEBUG
#define GFX_DEF_STACK_PAGE_SIZE 8 // to debug page allocations
#else
#define GFX_DEF_STACK_PAGE_SIZE 32
#endif


// ***** GASPagedStack - Stack used for ActionScript values.

// Performance of access is incredibly important here, as most of the values 
// are passed through the stack. The class is particularly optimized to ensure
// that the following methods are inlined and as efficient as possible:
// 
//  - Top0() - accesses the element on top of stack.
//  - Top1() - accesses the element one slot below stack top.
//  - Push() - push value; parameterized to allow copy constructors.
//  - Pop1() - pop one value from stack.
//
// Additionally the stack includes the following interesting features:
//  - Address of values on stack doesn't change.
//  - A default-constructed element always lives at the bottom of the stack.
//  - Top0()/Top1() are always valid, even if nothing was pushed.
//  - Pop() is allowed even if nothing was pushed.


template <class T, int DefPageSize = GFX_DEF_STACK_PAGE_SIZE>
class GASPagedStack : public GNewOverrideBase<GFxStatMV_ActionScript_Mem>
{    
    // State stack value pointers for fast access.
    T*                  pCurrent;
    T*                  pPageStart;
    T*                  pPageEnd;
    T*                  pPrevPageTop;

    // Page structure. This structure is GALLOC'ed and NOT new'd because 
    // we need to manage construction/cleanup of T values manually.    
    struct Page
    {        
        T       Values[DefPageSize];
        Page*   pNext;
    };

    // Page array.
    GArrayLH<Page*>         Pages;
    // Already allocated pages in a linked list.
    Page*                   pReserved;
    

    // *** Page allocator.

    Page*   NewPage()
    {   
        Page* p;
        if (pReserved) 
        {
            p = pReserved;
            pReserved = p->pNext;
        }
        else
        {
            p = (Page*) GHEAP_AUTO_ALLOC(this, sizeof(Page)); // Was: GFxStatMV_ActionScript_Mem
        }
        return p;
    }
       
    void    ReleasePage(Page* ppage)
    {
        ppage->pNext = pReserved;
        pReserved = ppage;
    }

    // *** PushPage and PopPage implementations.

    // This is called after pCurrent has been incremented to make space
    // available for new item, i.e. when pCurrent reached pPageEnd;
    // Adds a new page to stack and adjusts pointer values to refer to it.
    void    PushPage()
    {
        GASSERT(pCurrent == pPageEnd);

        Page* pnewPage = NewPage();
        if (pnewPage)
        {
            Pages.PushBack(pnewPage);
            // Previous page top == last page.
            pPrevPageTop= pPageEnd - 1;
            // Init new page pointers.
            pPageStart  = pnewPage->Values;
            pPageEnd    = pPageStart + DefPageSize;
            pCurrent    = pPageStart;            
        }
        else
        {
            // If for some reason allocation failed, we will overwrite last value.
            // (i.e. PushPage) will adjust pCurrent down by 1.
            pCurrent--;
        }
    }

    // Called from Pop() when we need to adjust pointer to the last page.
    // At his point, pCurrent should have already been decremented.
    void    PopPage()
    {
        GASSERT(pCurrent < pPageStart);

        if (Pages.GetSize() > 1)
        {            
            ReleasePage(Pages.Back());
            Pages.PopBack();

            Page* ppage = Pages.Back();

            pPageStart  = ppage->Values;
            pPageEnd    = pPageStart + DefPageSize;
            pCurrent    = pPageEnd - 1;

            if (Pages.GetSize() > 1)
            {
                pPrevPageTop = Pages[Pages.GetSize() - 2]->Values + DefPageSize - 1;
            }
            else
            {
                // If there are no more values, point pPrevPageTop to
                // the empty value in the bottom of the stack.
                pPrevPageTop = pPageStart;
            }            
        }
        else
        {
            // Nothing to pop. Adjust pCurrent and push default
            // value back on stack.
            pCurrent++;
            G_Construct<T>(pCurrent);
        }
    }

public:

    GASPagedStack()
        : Pages(), pReserved(0)
    {
        // Always allocate one page.       
        Page *ppage = NewPage();
        GASSERT(ppage);
        
        // Initialize the first element.
        // There is always an empty 'undefined' value living at the bottom of the stack.
        Pages.Reserve(15);
        Pages.PushBack(ppage);
        pPageStart  = ppage->Values;        
        pPageEnd    = pPageStart + DefPageSize;
        pCurrent    = pPageStart;
        pPrevPageTop= pCurrent;
        G_Construct<T>(pCurrent);
    }

    ~GASPagedStack()        
    {
        // Call destructor for all but the last page item.
        // (Stack will refuse to destroy last bottom item).
        Pop(TopIndex());
        // Call last item destructor and release its page.
        pCurrent->~T();
        ReleasePage(Pages.Back());

        // And release all reserved pages.        
        while(pReserved)
        {
            Page* pnext = pReserved->pNext;
            GFREE(pReserved);
            pReserved = pnext;
        }
    }

    void Reset()
    {
        // Call destructor for all but the last page item.
        // (Stack will refuse to destroy last bottom item).
        Pop(TopIndex());
        pCurrent->~T();
        Page *ppage = Pages.Back();

        pPageStart  = ppage->Values;        
        pPageEnd    = pPageStart + DefPageSize;
        pCurrent    = pPageStart;
        pPrevPageTop= pCurrent;
        G_Construct<T>(pCurrent);
    }

    // ***** High-performance Stack Access

    // Top0() and Top1() are used most commonly to access the stack.
    // Inline and optimize them as much as possible.

    // There is *always* one valid value on stack.
    // For an empty stack this value is at index 0: (undefined).
    GINLINE T*   Top0() const
    {        
        return pCurrent;
    }

    GINLINE T*   Top1() const
    {     
        return (pCurrent > pPageStart) ? (pCurrent - 1) : pPrevPageTop;     
    }

    template<class V>
    GINLINE void Push(const V& val)
    {
        pCurrent++;
        if (pCurrent >= pPageEnd)
        {
            // Adds a new page to stack and adjusts pointer values to refer to it.
            // If for some reason allocation failed, we will overwrite last value.
            // (i.e. PushPage) will adjust pCurrent down by 1.
            PushPage();
        }
        // Use copy constructor to initialize stack top.
        G_ConstructAlt<T, V>(pCurrent, val);
    }

    // Pop one element from stack. 
    GINLINE void Pop1()
    {
        pCurrent->~T();
        pCurrent--;
        if (pCurrent < pPageStart)
        {
            // We can always pop an element from stack. If these isn't one left after 
            // pop, PopPage will create a default undefined object and make it available.
            PopPage();
        }
    }

    GINLINE void Pop2()
    {
        if ((pCurrent-2) < pPageStart)
            Pop(2);
        else
        {
            pCurrent->~T();
            pCurrent--;
            pCurrent->~T();
            pCurrent--;
        }
    }

    void    Pop3()
    {
        if ((pCurrent-3) < pPageStart)
            Pop(3);
        else
        {
            pCurrent->~T();
            pCurrent--;
            pCurrent->~T();
            pCurrent--;
            pCurrent->~T();
            pCurrent--;
        }
    }

    // General Pop(number) implementation.
    void    Pop(UInt n)
    {
        while(n > 0)
        {
            Pop1();
            n--;
        }
    }

    // Returns absolute index of the item on top (*pCurrent).
    GINLINE UInt TopIndex () const
    {        
        return (((UInt)Pages.GetSize() - 1) * DefPageSize) + (UInt)(pCurrent - pPageStart);
    }

    
    // *** General Top(offset) / Bottom(offset) implementation.

    // These are used less frequently to can be less efficient then above.

    T*  Top(UInt offset) const
    {
        // Compute where in stack we are...
        T*   p        = 0; 
        UInt topIndex = TopIndex();

        if (offset <= topIndex)
        {
            const UInt absidx   = topIndex - offset;
            const UInt pageidx  = absidx / DefPageSize;
            const UInt idx      = absidx % DefPageSize;
            p = &Pages[pageidx]->Values[idx];
        }
        return p;
    }

    T*      Bottom(UInt offset) const
    {
        T*   p = 0;

        if (offset <= TopIndex())
        {        
            const UInt pageidx  = offset / DefPageSize;
            const UInt idx      = offset % DefPageSize;
            p = &Pages[pageidx]->Values[idx];
        }
        return p;
    }
};



// ActionScript "environment", essentially VM state?
class GASEnvironment : public GFxLogBase<GASEnvironment>, public GNewOverrideBase<GFxStatMV_ActionScript_Mem>
{
public:
    enum
    {
        MAX_RECURSION_LEVEL = 255
    };
    struct TryDescr
    {
        const UByte*    pTryBlock;
        UInt            TryBeginPC;
        UInt            TopStackIndex;

        TryDescr() { TopStackIndex = 0; }

        UInt GetTrySize() const     { return pTryBlock[1] | (((UInt16)pTryBlock[2]) << 8); }
        UInt GetCatchSize() const   { return pTryBlock[3] | (((UInt16)pTryBlock[4]) << 8); }
        UInt GetFinallySize() const { return pTryBlock[5] | (((UInt16)pTryBlock[6]) << 8); }
        bool IsCatchInRegister() const  { return (pTryBlock[0] & 0x4) != 0; }
        bool IsFinallyBlockFlag() const { return (pTryBlock[0] & 0x2) != 0; }
        bool IsCatchBlockFlag() const   { return (pTryBlock[0] & 0x1) != 0; }
        GASString GetCatchName(GASEnvironment* penv) const 
        { 
            GASString str = penv->GetBuiltin(GASBuiltin_empty_);
            if (IsCatchBlockFlag())
            {
                if (!IsCatchInRegister())
                    str = penv->CreateString((const char*) &pTryBlock[7]);
            }
            return str;
        }
        int GetCatchRegister() const 
        { 
            if (IsCatchBlockFlag())
            {
                if (IsCatchInRegister())
                    return (int)((UInt16)pTryBlock[7]);
            }
            return -1;
        }
    };

protected:
    GASPagedStack<GASValue>     Stack;
    GASValue                    GlobalRegister[4];
    GArrayLH<GASValue>          LocalRegister;  // function2 uses this
    GFxASCharacter*             Target;
    // Cached global contest and SWF version fast access.
    mutable GASStringContext    StringContext;
    
    GASPagedStack<GPtr<GASFunctionObject> > CallStack; // stack of calls

    GFxActionLogger*            pASLogger; //used to print filename, when available

    // try/catch/finally/throw stuff
    GArrayLH<TryDescr>  TryBlocks;             // array of "try" blocks
    GASValue            ThrowingValue;         // contains thrown object/value
    int                 ExecutionNestingLevel; // used to detect unhandled exceptions
    UInt16              FuncCallNestingLevel;  // count of nested function calls; MAX_RECURSION_LEVEL are allowed.
    bool                Unrolling       :1;    // indicates that "finally" blocks are being exctd
    bool                IsInvalidTarget :1;    // set to true, if tellTarget can't find the target

public:

    // For local vars.  Use empty names to separate frames.
    GArrayLH<GPtr<GASLocalFrame> >   LocalFrames;
    
    //AB: do we need to store whole stack here?
    //const GASWithStackArray* pWithStack; 


    GASEnvironment()
        :
        Target(0),
        pASLogger(NULL),
        ThrowingValue(GASValue::UNSET),
        ExecutionNestingLevel(0),
        FuncCallNestingLevel(0),
        Unrolling(false),
        IsInvalidTarget(false)
    {
    }

    ~GASEnvironment() 
    { }

    void            SetTarget(GFxASCharacter* ptarget);     
    void            SetInvalidTarget(GFxASCharacter* ptarget);
    // Used to set target right after construction
    void            SetTargetOnConstruct(GFxASCharacter* ptarget);
    GINLINE bool    IsTargetValid() const { return !IsInvalidTarget; }

    GINLINE GFxASCharacter*  GetTarget() const        { GASSERT(Target); return Target;  }
    bool            NeedTermination(GASActionBuffer::ExecuteType execType) const;
    
    // LogBase implementation.
    GFxLog*         GetLog() const;         
    bool            IsVerboseAction() const;
    bool            IsVerboseActionErrors() const;
    
    void                LogScriptError(const char* pfmt,...) const;
    void                LogScriptWarning(const char* pfmt,...) const;
    void                LogScriptMessage(const char* pfmt,...) const;
    void                SetASLogger(GFxActionLogger*   pasLogger) {pASLogger=pasLogger;}
    GFxActionLogger*    GetASLogger() {return pASLogger;}

    // Determines if gfxExtensions are enabled; returns 1 if enabled, 0 otherwise.
    bool            CheckExtensions() const;

    // Helper similar to GFxCharacter.  
    GFxMovieRoot*   GetMovieRoot() const;
    GINLINE UInt     GetVersion() const                  { return StringContext.GetVersion(); }
    GINLINE bool     IsCaseSensitive() const      { return StringContext.IsCaseSensitive(); }    
    
    // These are used *A LOT* - short names for convenience.
    // GetGC: Always returns a global object, never null.
    GINLINE GASGlobalContext*   GetGC() const            { return StringContext.pContext; }
    // GetSC: Always returns string context, never null.
    GINLINE GASStringContext*   GetSC() const            { return &StringContext; }
    // GetHeap: Always returns the current movie view heap.
    GINLINE GMemoryHeap*        GetHeap() const;
    // Convenient prototype access (delegates to GC).
    GINLINE GASObject*          GetPrototype(GASBuiltinType type) const;
    
    // Inline to built-in access through the context
    GINLINE const GASString&  GetBuiltin(GASBuiltinType btype) const;
    GINLINE GASString  CreateConstString(const char *pstr) const;
    GINLINE GASString  CreateString(const char *pstr) const;
    GINLINE GASString  CreateString(const wchar_t *pwstr) const;
    GINLINE GASString  CreateString(const char *pstr, UPInt length) const;
    GINLINE GASString  CreateString(const GString& str) const;

    // Stack access/manipulation
    // @@ TODO do more checking on these
    template<class V>
    GINLINE void        Push(const V& val)          { Stack.Push(val); }
    GINLINE void        Drop1()                     { Stack.Pop1(); }
    GINLINE void        Drop2()                     { Stack.Pop2(); }
    GINLINE void        Drop3()                     { Stack.Pop3(); }
    GINLINE void        Drop(UInt count)            { Stack.Pop(count); }
    GINLINE GASValue&   Top()                       { return *Stack.Top0(); }   
    GINLINE GASValue&   Top1()                      { return *Stack.Top1(); }
    GINLINE GASValue&   Top(UInt dist)              { return *Stack.Top(dist); }
    GINLINE GASValue&   Bottom(UInt index)          { return *Stack.Bottom (index); }
    GINLINE int         GetTopIndex() const         { return int(Stack.TopIndex()); }

    enum ExcludeFlags 
    {
        IgnoreLocals        = 0x1, // ignore local vars (incl this)
        IgnoreContainers    = 0x2, // ignore _root,_levelN,_global,
        NoLogOutput         = 0x4  // do not output error msgs into the log
    };

    // Invokes GetMember for the "pthisObj" and resolves properties (invokes getters)
    // or invokes __resolve handler.
    bool    GetMember(GASObjectInterface* pthisObj, 
        const GASString& memberName, 
        GASValue* pdestVal);

    // Resolves properties (invokes getter methods) and invokes __resolve handlers.
    bool    GetVariable(const GASString& varname, GASValue* presult, const GASWithStackArray* pwithStack = NULL, 
                        GFxASCharacter **ppnewTarget = 0, GASValue* powner = 0, UInt excludeFlags = 0);
    // Similar to GetVariable, determines if a var is available based on varName path.
    bool    IsAvailable(const GASString& varname, const GASWithStackArray* pwithStack = NULL) const;

    struct GetVarParams
    {
        const GASString&                        VarName;
        GASValue*                               pResult;
        const GASWithStackArray*                pWithStack;
        GFxASCharacter **                       ppNewTarget;
        GASValue*                               pOwner;
        UInt                                    ExcludeFlags;

        GetVarParams(const GASString& varname, GASValue* presult) : 
            VarName(varname), pResult(presult), pWithStack(NULL), ppNewTarget(NULL),
            pOwner(NULL), ExcludeFlags(0) {}
        GetVarParams(const GASString& varname, GASValue* presult, const GASWithStackArray* pwithStack, 
                     GFxASCharacter **ppnewTarget = 0, GASValue* powner = 0, UInt excludeFlags = 0) : 
            VarName(varname), pResult(presult), pWithStack(pwithStack), ppNewTarget(ppnewTarget),
            pOwner(powner), ExcludeFlags(excludeFlags) {}
        GetVarParams& operator=(const GetVarParams&) { return *this; }
    };
    // parses path; no property or __resolve invokes
    bool    FindAndGetVariableRaw(const GetVarParams& params) const;
    // no path stuff, no property or __resolve invokes
    bool    GetVariableRaw(const GetVarParams& params) const;
    bool    GetVariableRaw(const GASString& varname,
                           GASValue* presult,
                           const GASWithStackArray* pwithStack,
                           GASValue* powner,
                           UInt excludeFlags) const
    {
        return GetVariableRaw(GetVarParams(varname, presult, pwithStack, NULL, powner, excludeFlags));
    }
    Bool3W  CheckGlobalAndLevels(const GASEnvironment::GetVarParams& params) const;
    // find owner of varname. Return UNDEFINED for "this","_root","_levelN","_global".
    bool    FindOwnerOfMember (const GASString& varname, GASValue* presult,
                               const GASWithStackArray* pwithStack = NULL) const;

    // find variable by path. if onlyTarget == true then only characters can appear in the path
    bool    FindVariable(const GetVarParams& params, bool onlyTargets = false, GASString* varName = 0) const;

    bool    SetVariable(const GASString& path, const GASValue& val, const GASWithStackArray* pwithStack = NULL, bool doDisplayErrors = true);
    // no path stuff:
    void    SetVariableRaw(const GASString& path, const GASValue& val, const GASWithStackArray* pwithStack = NULL);

    void    SetLocal(const GASString& varname, const GASValue& val);
    void    AddLocal(const GASString& varname, const GASValue& val);    // when you know it doesn't exist.
    void    DeclareLocal(const GASString& varname); // Declare varname; undefined unless it already exists.

    // Parameter/local stack frame management.
    int     GetLocalFrameTop() const { return (UInt)LocalFrames.GetSize(); }
    void    SetLocalFrameTop(UInt t) { GASSERT(t <= LocalFrames.GetSize()); LocalFrames.Resize(t); }
    void    AddFrameBarrier() { LocalFrames.PushBack(0); }
    
    GASLocalFrame* GetTopLocalFrame (int off = 0) const;
    GASLocalFrame* CreateNewLocalFrame ();

    // Local registers.
    void    AddLocalRegisters(UInt RegisterCount)
    {
        LocalRegister.Resize(LocalRegister.GetSize() + RegisterCount);
    }
    void    DropLocalRegisters(UInt RegisterCount)
    {
        LocalRegister.Resize(LocalRegister.GetSize() - RegisterCount);
    }
    GASValue*   LocalRegisterPtr(UInt reg);

    // Internal.
    bool            FindLocal(const GASString& varname, GASValue* pvalue) const;
    GASValue*       FindLocal(const GASString& varname);
    const GASValue* FindLocal(const GASString& varname) const;  
    // excludeFlags - 0 or NoLogOutput
    GFxASCharacter* FindTarget(const GASString& path, UInt excludeFlags = 0) const;
    GFxASCharacter* FindTargetByValue(const GASValue& val);

    GASObject*      OperatorNew(const GASFunctionRef& constructor, int nargs = 0, int argsTopOff = -1);
    GASObject*      OperatorNew(GASObject* ppackageObj, const GASString &className, int nargs = 0, int argsTopOff = -1);
    GASObject*      OperatorNew(const GASString &className, int nargs = 0, int argsTopOff = -1);

    GASFunctionRef  GetConstructor(GASBuiltinType className);

    static bool     ParsePath(GASStringContext* psc, const GASString& varPath, GASString* path, GASString* var);
    static bool     IsPath(const GASString& varPath);

    GASValue        PrimitiveToTempObject(int index);
    GASValue        PrimitiveToTempObject(const GASValue& v);

    GINLINE void     CallPush(const GASFunctionObject* pfuncObj)
    {
        CallStack.Push(GPtr<GASFunctionObject>(const_cast<GASFunctionObject*>(pfuncObj)));
    }
    GINLINE void     CallPop()
    {
        GASSERT((int)CallStack.TopIndex() >= 0);
        CallStack.Pop1();
    }
    GINLINE GASValue CallTop(int off) const
    {
        if (CallStack.TopIndex() < (UInt)off)
            return GASValue(GASValue::NULLTYPE);
        else
            return GASValue(CallStack.Top(off)->GetPtr());
    }
    GINLINE UInt     GetCallStackSize() const { return (UInt)CallStack.TopIndex(); }

    GINLINE GASValue& GetGlobalRegister(int r)
    {
        GASSERT(r >= 0 && UInt(r) < sizeof(GlobalRegister)/sizeof(GlobalRegister[0]));
        return GlobalRegister[r];
    }
    GINLINE const GASValue& GetGlobalRegister(int r) const
    {
        GASSERT(r >= 0 && UInt(r) < sizeof(GlobalRegister)/sizeof(GlobalRegister[0]));
        return GlobalRegister[r];
    }

    // recursion guard for toString, valueOf, etc methods.
    // Returns true, if max recursion number is not reached.
    GINLINE bool RecursionGuardStart() 
    { 
        return (FuncCallNestingLevel++ < MAX_RECURSION_LEVEL);
    }
    GINLINE void RecursionGuardEnd()
    {
        GASSERT(FuncCallNestingLevel > 0);
        --FuncCallNestingLevel;
    }

    void Reset();

    // try/catch/finally/throw support
    int CheckExceptions(GASActionBuffer* pactBuf, 
        int nextPc, 
        int* plocalTryBlockCount, 
        GASValue* retval,
        const GASWithStackArray* pwithStack,
        GASActionBuffer::ExecuteType execType);

    void      CheckTryBlocks(int pc, int* plocalTryBlockCount);
    void      PushTryBlock(const TryDescr&);
    TryDescr  PopTryBlock();
    TryDescr& PeekTryBlock();
    void      Throw(const GASValue& val) { ThrowingValue = val; }
    void      ClearThrowing()            { ThrowingValue.SetUnset(); Unrolling = false; }
    bool      IsThrowing() const         { return ThrowingValue.IsSet(); }
    void      SetUnrolling()             { Unrolling = true; }
    void      ClearUnrolling()           { Unrolling = false; }
    bool      IsUnrolling() const        { return Unrolling; }
    const GASValue& GetThrowingValue() const { return ThrowingValue; }
    bool      IsInsideTryBlock(int pc);
    bool      IsInsideCatchBlock(int pc);
    bool      IsInsideFinallyBlock(int pc);

    void      EnteringExecution() { ++ExecutionNestingLevel; }
    void      LeavingExecution()  { --ExecutionNestingLevel; GASSERT(ExecutionNestingLevel >= 0); }
    bool      IsExecutionNestingLevelZero() const { return ExecutionNestingLevel == 0; }

    void      InvalidateOptAdvanceList() const;

    // Returns garbage collector, if turned on. Returns NULL otherwise.
    GASRefCountCollector* GetCollector(); 
};

//
// Some handy helpers
//

// Global Context, created once for a root movie
class GASGlobalContext : public GRefCountBaseNTS<GASGlobalContext, GFxStatMV_ActionScript_Mem>, public GASStringBuiltinManager
{
public:
    typedef GASFunctionRef (*RegisterBuiltinClassFunc)(GASGlobalContext*);
    struct ClassRegEntry
    {
        RegisterBuiltinClassFunc    RegistrarFunc;
        GPtr<GASFunctionObject>     ResolvedFunc;

        ClassRegEntry(RegisterBuiltinClassFunc f):RegistrarFunc(f) {}

        bool IsResolved() const { return ResolvedFunc.GetPtr() != NULL; }
        GASFunctionObject* GetResolvedFunc() const { return ResolvedFunc; }
    };
private:
    GHashUncachedLH<GASBuiltinType, GPtr<GASObject>, GFixedSizeHash<GASBuiltinType> >   Prototypes;
    GASStringHash<GASFunctionRef>                           RegisteredClasses;  
    GASStringHash<ClassRegEntry>                            BuiltinClassesRegistry;
    GFxMovieRoot*                                           pMovieRoot;
    GMemoryHeap*                                            pHeap;
public:
    GASObject* FlashGeomPackage; // obj for "flash.geom" package. Is holding by pGlobal
    GASObject* SystemPackage; // obj for "system" package. Is holding by pGlobal
    GASObject* FlashExternalPackage; // flash.external package
    GASObject* FlashDisplayPackage;  // flash.display package
    GASObject* FlashFiltersPackage; // flash.filters package
public:
    
    // Constructor, initializes the context variables
    GASGlobalContext(GFxMovieRoot*, GASStringManager*);
    //GASGlobalContext(GMemoryHeap*);
    ~GASGlobalContext();

    // Publicly accessible global members
    // ActionScript global object, corresponding to '_global'
    GPtr<GASObject>             pGlobal;

    Bool3W                      GFxExtensions;
 
    // A map of standard member names for efficient access. This hash
    // table is used by GASCharacter implementation. The values in it
    // are of GFxASCharacter::StandardMember type.
    GASStringHash<SByte>        StandardMemberMap;

    // this method returns built-in prototype (and register a class if necc).
    GASObject* GetPrototype(GASBuiltinType type);
    void SetPrototype(GASBuiltinType type, GPtr<GASObject> objproto)
    {
        Prototypes.Add(type, objproto);
    }
    // the method gets currently set prototype retrieving it from the corresponding
    // constructor function. 
    GASObject* GetActualPrototype(GASEnvironment* penv, GASBuiltinType type);

    // registerClass related methods
    bool RegisterClass(GASStringContext* psc, const GASString& className, const GASFunctionRef& ctorFunction);
    bool UnregisterClass(GASStringContext* psc, const GASString& className);
    bool FindRegisteredClass(GASStringContext* psc, const GASString& className, GASFunctionRef* pctorFunction);
    void UnregisterAllClasses();

    // Escape functions - src length required.
    static void EscapeWithMask(const char* psrc, UPInt length, GString* pescapedStr, const UInt* escapeMask);
    static void EscapePath(const char* psrc, UPInt length, GString* pescapedStr);
    static void Escape(const char* psrc, UPInt length, GString *pescapedStr);
    static void Unescape(const char* psrc, UPInt length, GString *punescapedStr);

    static GASObject* AddPackage(GASStringContext *psc, GASObject* pparent, GASObject* objProto, const char* const packageName);
    
    GASString FindClassName(GASEnvironment *penv, GASObjectInterface* iobj);

    GINLINE bool CheckExtensions() const { return GFxExtensions.IsTrue(); }

    // Returns the registration function. Note, it returns the function only once
    // (per class) to avoid multiple registrations.
    ClassRegEntry* GetBuiltinClassRegistrar(GASString className);

    template <GASBuiltinType type, class CtorFunc>
    void AddBuiltinClassRegistry(GASStringContext& sc, GASObject* pdest)
    {
        if (BuiltinClassesRegistry.Get(GetBuiltin(type)) == NULL)
        {
            BuiltinClassesRegistry.Add(GetBuiltin(type), CtorFunc::Register);
            pdest->SetMemberRaw(&sc, GetBuiltin(type), GASValue(GASValue::FUNCTIONNAME, GetBuiltin(type)));
        }
    }
    GASFunctionObject* ResolveFunctionName(const GASString& functionName);

    GMemoryHeap*        GetHeap() const         { return pHeap; }

    GFxMovieRoot* GetMovieRoot()                { return pMovieRoot; }
    const GFxMovieRoot* GetMovieRoot() const    { return pMovieRoot; }
    void    DetachMovieRoot()                   { pMovieRoot = NULL; }

    GASRefCountCollector* GetGC();

    void    PreClean(bool preserveBuiltinProps = false);
    void    Init(GFxMovieRoot* proot);
protected:   
    void    InitStandardMembers();
};

// Inline in environment for built-in access.
GINLINE const GASString&     GASEnvironment::GetBuiltin(GASBuiltinType btype) const
{    
    return (const GASString&)GetGC()->GetBuiltinNodeHolder(btype);
}
GINLINE GASString  GASEnvironment::CreateString(const char *pstr) const
{
    return GetGC()->CreateString(pstr);
}
GINLINE GASString  GASEnvironment::CreateString(const wchar_t *pwstr) const
{
    return GetGC()->CreateString(pwstr);
}
GINLINE GASString  GASEnvironment::CreateConstString(const char *pstr) const
{
    return GetGC()->CreateConstString(pstr);
}
GINLINE GASString  GASEnvironment::CreateString(const char *pstr, UPInt length) const
{
    return GetGC()->CreateString(pstr, length);
}
GINLINE GASString  GASEnvironment::CreateString(const GString& str) const
{
    return GetGC()->CreateString(str);
}
GINLINE GASObject* GASEnvironment::GetPrototype(GASBuiltinType type) const
{
    return GetGC()->GetPrototype(type);
}
GINLINE bool GASEnvironment::CheckExtensions() const
{
    return GetGC()->CheckExtensions();
}

GINLINE GASObject* GASEnvironment::OperatorNew(const GASString &className, int nargs, int argsTopOff)
{
    return OperatorNew(GetGC()->pGlobal, className, nargs, argsTopOff);
}

GINLINE GASValue* GASEnvironment::FindLocal(const GASString& varname)
{
    return const_cast<GASValue*>(const_cast<const GASEnvironment*>(this)->FindLocal(varname));
}

GINLINE bool GASEnvironment::FindLocal(const GASString& varname, GASValue* pvalue) const
{
    const GASValue* pval = FindLocal(varname);
    if (pval && pvalue)
    {
        *pvalue = *pval;
        return true;
    }
    return (pval != NULL);
}
GINLINE GMemoryHeap* GASEnvironment::GetHeap() const
{
    return GetGC()->GetHeap();
}


// Context-dependent inlines in GASStringContext.
GINLINE GMemoryHeap* GASStringContext::GetHeap() const
{
    return pContext->GetHeap();
}
GINLINE const GASString& GASStringContext::GetBuiltin(GASBuiltinType btype) const
{
    return (const GASString&)pContext->GetBuiltinNodeHolder(btype);
}
GINLINE GASString    GASStringContext::CreateConstString(const char *pstr) const
{
    return pContext->CreateConstString(pstr);
}
GINLINE GASString    GASStringContext::CreateString(const char *pstr) const
{
    return pContext->CreateString(pstr);
}
GINLINE GASString    GASStringContext::CreateString(const wchar_t *pwstr) const
{
    return pContext->CreateString(pwstr);
}
GINLINE GASString    GASStringContext::CreateString(const char *pstr, UPInt length) const
{
    return pContext->CreateString(pstr, length);
}
GINLINE GASString    GASStringContext::CreateString(const GString& str) const
{
    return pContext->CreateString(str);
}
GINLINE bool         GASStringContext::CompareConstString_CaseCheck(const GASString& pstr1, const char* pstr2)
{
    return CreateConstString(pstr2).Compare_CaseCheck(pstr1, IsCaseSensitive());
}
GINLINE bool         GASStringContext::CompareConstString_CaseInsensitive(const GASString& pstr1, const char* pstr2)
{
    return CreateConstString(pstr2).Compare_CaseCheck(pstr1, false);
}

GINLINE GASRefCountCollector* GASStringContext::GetGC() 
{ 
    return (pContext) ? pContext->GetGC() : NULL; 
}

GINLINE class GFxMovieRoot* GASStringContext::GetMovieRoot() 
{ 
    return (pContext) ? pContext->GetMovieRoot() : NULL; 
}

GINLINE GASValue& GASLocalFrame::Arg(int n) const
{
    GASSERT(n < NArgs);
    return Env->Bottom(FirstArgBottomIndex - n);
}

// Dispatching methods from C++.
bool        GAS_InvokeParsed(
                const char* pmethodName,
                GASValue* presult,
                GASObjectInterface* pthisPtr,
                GASEnvironment* penv,
                const char* pmethodArgFmt, 
                va_list args);

bool        GAS_InvokeParsed(const GASValue& method,
                             GASValue* presult,
                             GASObjectInterface* pthisPtr,
                             GASEnvironment* penv,
                             const char* pmethodArgFmt, 
                             va_list args,
                             const char* pmethodName);

bool        GAS_Invoke(
                const GASValue& method,
                GASValue* presult,
                GASObjectInterface* pthisPtr,
                GASEnvironment* penv, 
                int nargs, int firstArgBottomIndex,
                const char* pmethodName);

bool        GAS_Invoke(
                const GASValue& method,
                GASValue* presult,
                const GASValue& thisPtr,
                GASEnvironment* penv, 
                int nargs, int firstArgBottomIndex,
                const char* pmethodName);

bool        GAS_Invoke(
                const char* pmethodName,
                GASValue* presult,
                GASObjectInterface* pthis,
                GASEnvironment* penv,
                int numArgs, int firstArgBottomIndex);

GINLINE bool GAS_Invoke0(const GASValue& method,
                        GASValue* presult,
                        GASObjectInterface* pthis,
                        GASEnvironment* penv)
{
    return GAS_Invoke(method, presult, pthis, penv, 0, penv->GetTopIndex() + 1, NULL);
}

#ifdef GFC_OS_WIN32
#define snprintf _snprintf
#endif // GFC_OS_WIN32

#ifdef GFC_OS_SYMBIAN
#include <stdarg.h>

static GINLINE int snprintf(char *out, int n, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = G_vsprintf(out,fmt,args);
    va_end(args);
    return ret;
}
#endif

#endif // INC_GFXACTION_H
