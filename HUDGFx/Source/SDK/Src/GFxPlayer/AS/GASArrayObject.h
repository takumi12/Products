/**********************************************************************

Filename    :   AS/GASArrayObject.h
Content     :   Array object functinality
Created     :   March 10, 2006
Authors     :   Maxim Shemanarev, Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXARRAY_H
#define INC_GFXARRAY_H

#include "GAlgorithm.h"
#include "GFxAction.h"
#include "AS/GASObjectProto.h"


// ***** Declared Classes
class GASRecursionGuard;
class GASArraySortFunctor;
class GASArraySortOnFunctor;
class GASArrayObject;
class GASArrayProto;
class GASArrayCtorFunction;



class GASRecursionGuard
{
public:
    GASRecursionGuard(const GASArrayObject* pThis);
    ~GASRecursionGuard();
    int Count() const;
private:
    const GASArrayObject* ThisPtr;
};


class GASArraySortFunctor
{
public:
    GASArraySortFunctor() {}
    GASArraySortFunctor(GASObjectInterface* pThis, 
                        int flags,
                        const GASFunctionRef& func,
                        GASEnvironment* env,
                        const GFxLog* log=0) : 
        This(pThis), 
        Flags(flags), 
        Func(func), 
        Env(env), 
        LogPtr(log)
    {}

    int Compare(const GASValue* a, const GASValue* b) const;
    bool operator()(const GASValue* a, const GASValue* b) const
    {
        return Compare(a, b) < 0;
    }

private:
    GASObjectInterface* This;
    int                 Flags;
    GASFunctionRef      Func;
    GASEnvironment*     Env;
    const GFxLog*       LogPtr;
};

struct GASIndexValue
{
	UPInt Index;
	GASValue* pValue;
};

class GASArraySortIndexFunctor
{
public:
	GASArraySortIndexFunctor(GASArraySortFunctor* psf) : pSf(psf) {}

	int Compare(const GASIndexValue a, const GASIndexValue b) const
	{
		return pSf->Compare(a.pValue, b.pValue);
	}
	bool operator()(const GASIndexValue a, const GASIndexValue b) const
	{
		return Compare(a, b) < 0;
	}
private:
	GASArraySortFunctor* pSf;
};

class GASArraySortOnFunctor
{
public:
    GASArraySortOnFunctor(GASObjectInterface* pThis, 
                          const GArrayCC<GASString, GFxStatMV_ActionScript_Mem>& fieldArray,
                          const GArray<int>& flagsArray,
                          GASEnvironment* env,
                          const GFxLog* log=0); 

    int Compare(const GASValue* a, const GASValue* b) const;
    bool operator()(const GASValue* a, const GASValue* b) const
    {
        return Compare(a, b) < 0;
    }

private:
    GASObjectInterface*             This;
    const GArrayCC<GASString, GFxStatMV_ActionScript_Mem>* FieldArray;
    GASEnvironment*                 Env;
    const GFxLog*                   LogPtr;
    GArray<GASArraySortFunctor>     FunctorArray;
};

class GASArraySortOnIndexFunctor
{
public:
	GASArraySortOnIndexFunctor(GASArraySortOnFunctor* psf) : pSf(psf) {}

	int Compare(const GASIndexValue a, const GASIndexValue b) const
	{
		return pSf->Compare(a.pValue, b.pValue);
	}
	bool operator()(const GASIndexValue a, const GASIndexValue b) const
	{
		return Compare(a, b) < 0;
	}
private:
	GASArraySortOnFunctor* pSf;
};


class GASArrayObject : public GASObject
{
    friend class GASArrayProto;
    friend class GASArrayCtorFunction;

#ifndef GFC_NO_GC
protected:
    friend class GRefCountBaseGC<GFxStatMV_ActionScript_Mem>;
    template <class Functor> void ForEachChild_GC(Collector* prcc) const;
    virtual void                  ExecuteForEachChild_GC(Collector* prcc, OperationGC operation) const;
    virtual void                  Finalize_GC();
#endif // GFC_NO_GC
protected:
    GASArrayObject(GASStringContext *psc, GASObject* pprototype); // for prototype only

    void InitArray(const GASFnCall& fn);
public:
    enum SortFlags
    {
        SortFlags_CaseInsensitive    = 1,
        SortFlags_Descending         = 2, 
        SortFlags_UniqueSort         = 4, 
        SortFlags_ReturnIndexedArray = 8,
        SortFlags_Numeric            = 16,

        // extension
        SortFlags_Locale             = 1024
    };
    
    friend class GASRecursionGuard;

    GASArrayObject(GASEnvironment* penv);
    GASArrayObject(GASStringContext *psc);
    ~GASArrayObject();
    virtual bool    SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags());
    virtual bool    GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val);
    virtual bool    DeleteMember(GASStringContext *psc, const GASString& name);
    virtual void    VisitMembers(GASStringContext *psc, MemberVisitor *pvisitor, UInt visitFlags, const GASObjectInterface* instance = 0) const;   
    virtual bool    HasMember(GASStringContext *psc, const GASString& name, bool inclPrototypes);

    // pEnv is required for Array's GetTextValue.
    virtual const char* GetTextValue(GASEnvironment* penv) const;
    virtual ObjectType  GetObjectType() const;

    int             GetSize() const { return (UInt)Elements.GetSize(); }
    void            Resize(int newSize);
          GASValue* GetElementPtr(int i)       { return Elements[i]; }
    const GASValue* GetElementPtr(int i) const { return Elements[i]; }
    void            SetElement(int i, const GASValue& val);
    // SetElementSafe may set any element even if idx is beyond the array's size.
    void            SetElementSafe(int idx, const GASValue& val);
    void            PushBack(const GASValue& val);
    void            PushBack();
    void            PopBack();
    void            PopFront();
    void            RemoveElements(int start, int count);
    void            InsertEmpty(int start, int count);
    void            JoinToString(GASEnvironment* pEnv, GStringBuffer* pbuffer, const char* pDelimiter) const;
    void            Concat(GASEnvironment* pEnv, const GASValue& val);
    void            ShallowCopyFrom(const GASArrayObject& ao);
    void            MakeDeepCopy(GMemoryHeap *pheap);
    void            MakeDeepCopyFrom(GMemoryHeap *pheap, const GASArrayObject& ao);
    void            DetachAll();
    void            Reverse();
    void            Reserve(UPInt sz) { Elements.Reserve(sz); }

    template<class SortFunctor>
    bool            Sort(SortFunctor& sf)
    {
        if (Elements.GetSize() > 0)
        {
            GArrayAdaptor<GASValue*> a(&Elements[0], (UInt)Elements.GetSize());
            return G_QuickSortSafe(a, sf);
        }
        return true;
    }

    static int      ParseIndex(const GASString& name);

    bool            RecursionLimitReached() const;
    const GFxLog*   GetLogPtr() const { return LogPtr; }

    static void     GAS_ArrayConcat(const GASFnCall& fn);
    static void     GAS_ArrayJoin(const GASFnCall& fn);
    static void     GAS_ArrayPop(const GASFnCall& fn);
    static void     GAS_ArrayPush(const GASFnCall& fn);
    static void     GAS_ArrayReverse(const GASFnCall& fn);
    static void     GAS_ArrayShift(const GASFnCall& fn);
    static void     GAS_ArraySlice(const GASFnCall& fn);
    static void     GAS_ArraySort(const GASFnCall& fn);
    static void     GAS_ArraySortOn(const GASFnCall& fn);
    static void     GAS_ArraySplice(const GASFnCall& fn);
    static void     GAS_ArrayToString(const GASFnCall& fn);
    static void     GAS_ArrayUnshift(const GASFnCall& fn);
    static void     GAS_ArrayLength(const GASFnCall& fn);
    static void     GAS_ArrayValueOf(const GASFnCall& fn);
private:
    void CallProlog() const { RecursionCount++; }
    void CallEpilog() const { RecursionCount--; }

    const GFxLog*           LogPtr;
    GArrayLH<GASValue*>     Elements;
    mutable GStringLH       StringValue;
    mutable int             RecursionCount;
    bool                    LengthValueOverriden;
};


class GASArrayProto : public GASPrototype<GASArrayObject>
{
public:
    GASArrayProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor);
};


class GASArrayCtorFunction : public GASCFunctionObject
{
public:
    GASArrayCtorFunction (GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);
    static void DeclareArray(const GASFnCall& fn);

    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;
    static GASFunctionRef Register(GASGlobalContext* pgc);
};

#endif
