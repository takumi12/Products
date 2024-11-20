/**********************************************************************

Filename    :   AS/GASObject.h
Content     :   ActionScript Object implementation classes
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXOBJECT_H
#define INC_GFXOBJECT_H

#include "GRefCount.h"
#include "GArray.h"
#include "GFxASString.h"
#include "GFxPlayer.h"
#include "GFxActionTypes.h"
#include "GFxStringBuiltins.h"
#include "GASFunctionRef.h"
#include "GASValue.h"
#include "GHeapNew.h"

// ***** Declared Classes
class GASPropFlags;
class GASMember;
class GASObject;
class GASObjectInterface;
class GASObjectProto;
class GASObjectCtorFunction;
class GASFunctionObject;
class GASFunctionProto;
class GASFunctionCtorFunction;
class GASCustomFunctionObject;
class GASSuperObject;
class GASAsFunctionObject;

// Parameterized so GASEnvironment can be used before definition
template <typename BaseClass, typename GASEnvironment = GASEnvironment> 
class GASPrototype;

// ***** External Classes
class GASEnvironment;
class GFxSprite;
class GFxASUserData;


// ***** GASPropFlags

// Flags defining the level of protection of an ActionScript member (GASMember).
// Members can be marked as readOnly, non-enumeratebale, etc.

class GASPropFlags
{
public:
    // Numeric flags
    UByte        Flags;
    // Flag bit values
    enum PropFlagConstants
    {
        PropFlag_ReadOnly   =   0x04,
        PropFlag_DontDelete =   0x02,
        PropFlag_DontEnum   =   0x01,
        PropFlag_Mask       =   0x07
    };
    
    // Constructors
    GASPropFlags()
    {
        Flags = 0;        
    }
    
    // Constructor, from numerical value
    GASPropFlags(UByte flags)    
    {
        Flags = flags;
    }
            
    GASPropFlags(bool readOnly, bool dontDelete, bool dontEnum)         
    {
        Flags = (UByte) ( ((readOnly) ?   PropFlag_ReadOnly : 0) |
                          ((dontDelete) ? PropFlag_DontDelete : 0) |
                          ((dontEnum) ?   PropFlag_DontEnum : 0) );
    }
        
    // Accessors
    bool    GetReadOnly() const     { return ((Flags & PropFlag_ReadOnly)   ? true : false); }
    bool    GetDontDelete() const   { return ((Flags & PropFlag_DontDelete) ? true : false); }
    bool    GetDontEnum() const     { return ((Flags & PropFlag_DontEnum)   ? true : false); }
    UByte   GetFlags() const        { return Flags; }       
    
    // set the numerical flags value (return the new value )
    // If unlocked is false, you cannot un-protect from over-write,
    // you cannot un-protect from deletion and you cannot
    // un-hide from the for..in loop construct
    UByte SetFlags(UByte setTrue, UByte setFalse)
    {
        Flags = Flags & (~setFalse);
        Flags |= setTrue;
        return GetFlags();
    }
    UByte SetFlags(UByte flags)
    {        
        Flags = flags;
        return GetFlags();
    }
};
 

// ActionScript member: combines value and its properties. Used in GASObject,
// and potentially other types.
class GASMember
{    
public:
    GASValue        Value;
    
    // Constructors
    GASMember()
    { Value.SetPropFlags(0); }
    
    GASMember(const GASValue &value,const GASPropFlags flags = GASPropFlags())
        : Value(value)
    { Value.SetPropFlags(flags.GetFlags()); }

    GASMember(const GASMember &src)
        : Value(src.Value)
    {
        // Copy PropFlags explicitly since they are not copied as part of value.
        Value.SetPropFlags(src.Value.GetPropFlags());
    }

    void operator = (const GASMember &src)     
    {
        // Copy PropFlags explicitly since they are not copied as part of value.
        Value = src.Value;
        Value.SetPropFlags(src.Value.GetPropFlags());
    }
    
    // Accessors
    const GASValue& GetMemberValue() const                      { return Value; }
    GASPropFlags    GetMemberFlags() const                      { return GASPropFlags(Value.GetPropFlags()); }
    void            SetMemberValue(const GASValue &value)       { Value = value; }
    void            SetMemberFlags(const GASPropFlags &flags)   { Value.SetPropFlags(flags.GetFlags()); }

#ifndef GFC_NO_GC
    void Finalize_GC() { Value.Finalize_GC(); }
#endif
};

// This is the base class for all ActionScript-able objects ("GAS_" stands for ActionScript).   
class GASObjectInterface
{
public:
    GASObjectInterface();
    virtual ~GASObjectInterface();

    // Specific class types.
    enum ObjectType
    {
        Object_Unknown,

        // This type is for non-scriptable characters; it is not GASObjectInterface.
        Object_BaseCharacter,
        
        // These need to be grouped in range for IsASCharacter().
        Object_Sprite,
        Object_ASCharacter_Begin    = Object_Sprite,
        Object_Button,
        Object_TextField,
        Object_Video,
        Object_ASCharacter_End      = Object_Video,

        Object_ASObject,
        Object_ASObject_Begin       = Object_ASObject,
        Object_Array,
        Object_String,
        Object_Number,
        Object_Boolean,
        Object_MovieClipObject,
        Object_ButtonASObject,
        Object_TextFieldASObject,
        Object_VideoASObject,
        Object_Matrix,
        Object_Point,
        Object_Rectangle,
        Object_ColorTransform,
        Object_Capabilities,
        Object_Transform,
        Object_Color,
        Object_Key,
        Object_Function,
        Object_Stage,
        Object_MovieClipLoader,
        Object_BitmapData,
        Object_LoadVars,
        Object_XML,
        Object_XMLNode,
        Object_TextFormat,
        Object_StyleSheet,
        Object_Sound,
        Object_NetConnection,
        Object_NetStream,
        Object_Date,
        Object_AsBroadcaster,
        Object_BitmapFilter,
        Object_DropShadowFilter,
        Object_GlowFilter,
        Object_BlurFilter,
        Object_BevelFilter,
        Object_ColorMatrixFilter,
        Object_TextSnapshot,
        Object_SharedObject,
        Object_ASObject_End        = Object_SharedObject
    };

    // Returns text representation of the object.
    // - penv parameter is optional and may be not required for most of the types.
    virtual const char*         GetTextValue(GASEnvironment* penv = 0) const    { GUNUSED(penv); return 0; }

    virtual ObjectType          GetObjectType() const                           { return Object_Unknown; }

    struct MemberVisitor {
        virtual ~MemberVisitor () { }
        virtual void    Visit(const GASString& name, const GASValue& val, UByte flags) = 0;
    };

    enum VisitMemberFlags
    {
        VisitMember_Prototype   = 0x01, // Visit prototypes.
        VisitMember_ChildClips  = 0x02, // Visit child clips in sprites as members.
        VisitMember_DontEnum    = 0x04, // Visit members marked as not enumerable.
        VisitMember_NamesOnly   = 0x08  // Get names only, value will be reported empty
    };
    virtual bool            GetTableMember(GASEnvironment* penv, func_member_value& name, void* pClass, GFxValue* val)                       = 0;
    
    virtual bool            SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags()) = 0;
    virtual bool            GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)                       = 0;
    virtual bool            FindMember(GASStringContext *psc, const GASString& name, GASMember* pmember)                = 0;
    virtual bool            DeleteMember(GASStringContext *psc, const GASString& name)                                  = 0;
    virtual bool            SetMemberFlags(GASStringContext *psc, const GASString& name, const UByte flags)              = 0;
    virtual void            VisitMembers(GASStringContext *psc, MemberVisitor *pvisitor, 
                                         UInt visitFlags = 0, const GASObjectInterface* instance = 0) const     = 0;
    virtual bool            HasMember(GASStringContext *psc, const GASString& name, bool inclPrototypes)                = 0;

    virtual bool            SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val,
                                         const GASPropFlags& flags = GASPropFlags())                                    = 0;
    virtual bool            GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val)                   = 0;

    // Helper for string names: registers string automatically.
    // Should not be used for built-ins.
    bool    SetMemberRaw(GASStringContext *psc, const char *pname, const GASValue& val)
    {
        return SetMemberRaw(psc, GASString(psc->CreateString(pname)), val);        
    }
    bool    SetConstMemberRaw(GASStringContext *psc, const char *pname, const GASValue& val)
    {
        return SetMemberRaw(psc, GASString(psc->CreateConstString(pname)), val);
    }
    bool    SetConstMemberRaw(GASStringContext *psc, const char *pname, const GASValue& val, const GASPropFlags& flags)
    {
        return SetMemberRaw(psc, GASString(psc->CreateConstString(pname)), val, flags);
    }
    bool    GetConstMemberRaw(GASStringContext *psc, const char *pname, GASValue* val)
    {
        return GetMemberRaw(psc, GASString(psc->CreateConstString(pname)), val);
    }

    // Convenience commonly used RTTI inlines.
    GINLINE static bool     IsTypeASCharacter(ObjectType t) { return (t>=Object_ASCharacter_Begin) && (t<=Object_ASCharacter_End);  }
    GINLINE static bool     IsTypeASObject(ObjectType t) { return (t>=Object_ASObject_Begin) && (t<=Object_ASObject_End);  }

    GINLINE bool            IsASCharacter() const   { return IsTypeASCharacter(GetObjectType());  }
    GINLINE bool            IsASObject() const      { return IsTypeASObject(GetObjectType());  }
    GINLINE bool            IsSprite() const        { return GetObjectType() == Object_Sprite; }
    GINLINE bool            IsFunction() const      { return GetObjectType() == Object_Function; }

    // Convenience casts; in cpp file because these require GFxSprite/GFxASCharacter definitions.
    GFxASCharacter*         ToASCharacter();
    GASObject*              ToASObject();
    GFxSprite*              ToSprite();
    virtual GASFunctionRef  ToFunction();

    const GFxASCharacter*   ToASCharacter() const;
    const GASObject*        ToASObject() const;
    const GFxSprite*        ToSprite() const;

    // sets __proto__ property 
    virtual void            Set__proto__(GASStringContext *psc, GASObject* protoObj) = 0;

    // gets __proto__ property
    inline GASObject*       Get__proto__() const { return pProto; }


    // sets __constructor__ property
    void                    Set__constructor__(GASStringContext *psc, const GASFunctionRef& ctorFunc) 
    {   
        SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin___constructor__), ctorFunc,
                     GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    }

    // gets __constructor__ property
    // virtual because it's overridden in GASSuperObject
    virtual GASFunctionRef  Get__constructor__(GASStringContext *psc)
    { 
        GASValue val;
        if (GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin___constructor__), &val))
            return val.ToFunction(NULL);
        return 0; 
    }

    // sets "constructor" property
    void                    Set_constructor(GASStringContext *psc, const GASFunctionRef& ctorFunc) 
    {
        SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_constructor), ctorFunc,
                     GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    }
    // gets "constructor" property
    GASFunctionRef          Get_constructor(GASStringContext *psc)
    { 
        GASValue val;
        if (GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_constructor), &val))
            return val.ToFunction(NULL);
        return 0; 
    }
    
    // is it super-class object
    virtual bool            IsSuper() const { return false; }
    // is it prototype of built-in class?
    virtual bool            IsBuiltinPrototype() const { return false; }

    // adds interface to "implements-of" list. If ctor is null, then index
    // specifies the total number of interfaces to pre-allocate the array.
    virtual void AddInterface(GASStringContext *psc, int index, GASFunctionObject* ctor) { GUNUSED3(psc, index, ctor); } // implemented for prototypes only

    virtual bool InstanceOf(GASEnvironment*, const GASObject* prototype, bool inclInterfaces = true) const { GUNUSED2(prototype, inclInterfaces); return false; }

    virtual bool Watch(GASStringContext *psc, const GASString& prop, const GASFunctionRef& callback, const GASValue& userData) { GUNUSED4(psc, prop,callback,userData); return false; }
    virtual bool Unwatch(GASStringContext *psc, const GASString& prop)  { GUNUSED2(psc, prop); return false; }

    GASObject*   FindOwner(GASStringContext *psc, const GASString& name);


#ifndef GFC_NO_FXPLAYER_AS_USERDATA
protected:
    struct UserDataHolder : public GNewOverrideBase<GFxStatMV_ActionScript_Mem>
    {
        GFxMovieView*       pMovieView;
        GFxASUserData*      pUserData;
        UserDataHolder(GFxMovieView* pmovieView, GFxASUserData* puserData) :
        pMovieView(pmovieView), pUserData(puserData) {}
        void    NotifyDestroy(GASObjectInterface* pthis) const 
        {
            if (pUserData)  
            {
                // Remove user data weak ref
                pUserData->SetLastObjectValue(NULL, NULL, false);
                pUserData->OnDestroy(pMovieView, pthis);                
            }
        }
    } *pUserDataHolder;

public:
    GFxASUserData*      GetUserData() const     { return pUserDataHolder ? pUserDataHolder->pUserData : NULL; }
    void                SetUserData(GFxMovieView* pmovieView, GFxASUserData* puserData, bool isdobj);
#endif  // GFC_NO_FXPLAYER_AS_USERDATA


protected:
    GPtr<GASObject>     pProto;          // __proto__
};


// ***** GASObject

// A generic bag of attributes.  Base-class for ActionScript
// script-defined objects.

//class GASObject : public GRefCountBase<GASObject, GFxStatMV_ActionScript_Mem>, public GASObjectInterface
class GASObject : public GASRefCountBase<GASObject>, public GASObjectInterface
{
private:
    GASObject(const GASObject&):GASRefCountBase<GASObject>(NULL) {}
public:

    struct Watchpoint
    {
        GASFunctionRef  Callback;
        GASValue        UserData;

#ifndef GFC_NO_GC
        template <class Functor>
        void ForEachChild_GC(Collector* prcc) const
        {
            Callback.template ForEachChild_GC<Functor>(prcc);
            UserData.template ForEachChild_GC<Functor>(prcc);
        }
        void Finalize_GC() 
        { 
            Callback.Finalize_GC(); 
            UserData.Finalize_GC(); 
        }
#endif //GFC_NO_GC
    };

#ifndef GFC_NO_GC
    // Hash table used for ActionScript members, garbage collection version
    typedef GASStringHash_GC<GASMember>     MemberHash;
    typedef GASStringHash_GC<Watchpoint>    WatchpointHash;
#else
    // Hash table used for ActionScript members.
    typedef GASStringHash<GASMember>        MemberHash;
    typedef GASStringHash<Watchpoint>       WatchpointHash;
#endif

    MemberHash          Members;
    GASFunctionRef      ResolveHandler;  // __resolve
    
    WatchpointHash*     pWatchpoints;

    bool                ArePropertiesSet; // indicates if any property (Object.addProperty) was set
    bool                IsListenerSet; // See comments in GFxValue.cpp, CheckForListenersMemLeak. 

    GASObject(GASStringContext *psc);
    GASObject(GASStringContext *psc, GASObject* proto);
    GASObject(GASEnvironment* pEnv);

    virtual ~GASObject();
    
    virtual const char* GetTextValue(GASEnvironment* penv = 0) const { GUNUSED(penv); return NULL; }
	virtual bool    GetTableMember(GASEnvironment* penv, func_member_value& funceTable, void* pClass, GFxValue* val);
    virtual bool    SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags());
    virtual bool    GetMember(GASEnvironment* penv, const GASString& name, GASValue* val);
    virtual bool    FindMember(GASStringContext *psc, const GASString& name, GASMember* pmember);
    virtual bool    DeleteMember(GASStringContext *psc, const GASString& name);
    virtual bool    SetMemberFlags(GASStringContext *psc, const GASString& name, const UByte flags);
    virtual void    VisitMembers(GASStringContext *psc, MemberVisitor *pvisitor, UInt visitFlags, const GASObjectInterface* instance = 0) const;
    virtual bool    HasMember(GASStringContext *psc, const GASString& name, bool inclPrototypes);

    virtual bool    SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags());
    virtual bool    GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val);

    virtual ObjectType  GetObjectType() const
    {
        return Object_ASObject;
    }

    void    Clear()
    {
        Members.Clear();
        pProto = 0;             
    }

    // sets __proto__ property 
    virtual void            Set__proto__(GASStringContext *psc, GASObject* protoObj) 
    {   
        if (!pProto)
            SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin___proto__), GASValue(GASValue::UNSET),
                         GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
        pProto = protoObj;
    }
    // gets __proto__ property
    /*virtual GASObject*      Get__proto__()
    { 
        return pProto; 
    }*/

    virtual void        SetValue(GASEnvironment* penv, const GASValue&);
    virtual GASValue    GetValue() const;

    virtual bool        InstanceOf(GASEnvironment* penv, const GASObject* prototype, bool inclInterfaces = true) const;
    virtual bool        DoesImplement(GASEnvironment*, const GASObject* prototype) const { return this == prototype; }

    virtual bool        Watch(GASStringContext *psc, const GASString& prop, const GASFunctionRef& callback, const GASValue& userData);
    virtual bool        Unwatch(GASStringContext *psc, const GASString& prop);
    bool                InvokeWatchpoint(GASEnvironment* penv, const GASString& prop, const GASValue& newVal, GASValue* resultVal);
    bool                HasWatchpoint() const { return pWatchpoints != NULL; }

    // these method will return an original GFxASCharacter for GASObjects from GFxSprite, GFxButton,
    // GFxText.
    virtual GFxASCharacter*         GetASCharacter() { return 0; }
    virtual const GFxASCharacter*   GetASCharacter() const { return 0; }

    static GASObject*   GetPrototype(GASGlobalContext* pgc, GASBuiltinType classNameId);

    // checks and reset the "constructor" property (in the case if param is equal to ctor). 
    // Works only for prototypes, otherwise does nothing.
    virtual void CheckAndResetCtorRef(GASFunctionObject*) { }


protected:
    GASObject(GASRefCountCollector *pcc);
private:
    void Init();
#ifndef GFC_NO_GC
protected:
    friend class GRefCountBaseGC<GFxStatMV_ActionScript_Mem>;
    template <class Functor> void ForEachChild_GC(Collector* prcc) const;
    virtual                  void ExecuteForEachChild_GC(Collector* prcc, OperationGC operation) const;

    virtual void Finalize_GC();
public:
    GASRefCountCollector *GetCollector() 
    { 
        return reinterpret_cast<GASRefCountCollector*>(GASRefCountBase<GASObject>::GetCollector());
    }
#else
public:
    GASRefCountCollector *GetCollector() { return NULL; }
#endif //GFC_NO_GC

};

// Base class for AS Function object.
class GASFunctionObject : public GASObject 
{
    friend class GASFunctionProto;
    friend class GASFunctionCtorFunction;
    friend class GASCustomFunctionObject;

#ifdef GFC_NO_GC
    GASObject* pPrototype; // also hold firmly by members
public:
    virtual bool  SetMember(GASEnvironment *penv, 
        const GASString& name, 
        const GASValue& val, 
        const GASPropFlags& flags = GASPropFlags());
#endif

protected:
    GASFunctionObject(GASStringContext* psc);
    GASFunctionObject(GASEnvironment* penv);
public:
    ~GASFunctionObject();

    virtual ObjectType      GetObjectType() const   { return Object_Function; }

    virtual GASEnvironment* GetEnvironment(const GASFnCall& fn, GPtr<GFxASCharacter>* ptargetCh) = 0;
    virtual void Invoke (const GASFnCall& fn, GASLocalFrame*, const char* pmethodName) = 0;

    virtual bool IsNull () const                           = 0;
    virtual bool IsEqual(const GASFunctionObject& f) const = 0;

    virtual bool IsCFunction() const   { return false; }
    virtual bool IsAsFunction() const  { return false; }

    /* This method is used for creation of object by AS "new" (GASEnvironment::OperatorNew)
       If this method returns NULL, then constructor function is responsible for
       object's creation. */
    virtual GASObject* CreateNewObject(GASEnvironment*) const { return 0; }

    void SetPrototype(GASStringContext* psc, GASObject* pprototype);
    void SetProtoAndCtor(GASStringContext* psc, GASObject* pprototype);

    GASFunctionRef  ToFunction();

    // returns number of arguments expected by this function;
    // returns -1 if number of arguments is unknown (for C functions)
    virtual int     GetNumArgs() const = 0;

    GASObject*      GetPrototype(GASStringContext* psc);
};

// Function object for built-in functions ("C-functions")
class GASCFunctionObject : public GASFunctionObject 
{
    friend class GASFunctionProto;
    friend class GASFunctionCtorFunction;
    friend class GASCustomFunctionObject;
    friend class GASFunctionRefBase;
protected:
    //GASCFunctionDef     Def;
    GASCFunctionPtr pFunction;

    GASCFunctionObject(GASStringContext* psc);
    GASCFunctionObject(GASEnvironment* penv);
public:
    //GASCFunctionObject(GASEnvironment* penv, GASCFunctionDef* func);
    GASCFunctionObject(GASStringContext* psc, GASObject* pprototype, GASCFunctionPtr func);
    GASCFunctionObject(GASStringContext* psc, GASCFunctionPtr func);
    ~GASCFunctionObject();

    virtual GASEnvironment* GetEnvironment(const GASFnCall& fn, GPtr<GFxASCharacter>* ptargetCh);
    void Invoke (const GASFnCall& fn, GASLocalFrame*, const char* pmethodName);

    virtual bool IsNull () const { return pFunction == 0; }
    virtual bool IsEqual(const GASFunctionObject& f) const;
    bool IsEqual (GASCFunctionPtr f) const { return pFunction == f; }

    virtual bool IsCFunction() const { return true; }

    // returns number of arguments expected by this function;
    // returns -1 if number of arguments is unknown (for C functions)
    int     GetNumArgs() const { return -1; }
};

// A constructor function object for Object
class GASObjectCtorFunction : public GASCFunctionObject
{
    static const GASNameFunction StaticFunctionTable[];

    static void RegisterClass (const GASFnCall& fn);

public:
    GASObjectCtorFunction(GASStringContext *psc);
    ~GASObjectCtorFunction() {}

    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;
};

// A constructor function object for class "Function"
class GASFunctionCtorFunction : public GASCFunctionObject
{
public:
    GASFunctionCtorFunction(GASStringContext *psc);

    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;
};

class GASLocalFrame : public GASRefCountBase<GASLocalFrame>
{
#ifndef GFC_NO_GC
    // Garbage collection version
    friend class GRefCountBaseGC<GFxStatMV_ActionScript_Mem>;
    template <class Functor> void ForEachChild_GC(Collector* prcc) const;
    virtual void                  ExecuteForEachChild_GC(Collector* prcc, OperationGC operation) const;
    virtual void                  Finalize_GC();
public:
    GASStringHash_GC<GASValue>  Variables;
#else
public:
    // registry for locally declared funcs.
    GHashLH<GASFunctionObject*,int, GFixedSizeHash<GASFunctionObject*> > LocallyDeclaredFuncs; 

    GASStringHash<GASValue>     Variables;
#endif //GFC_NO_GC
public:
    GPtr<GASLocalFrame>         PrevFrame;
    GASObjectInterface*         SuperThis; //required for "super" in SWF6

    // these members are necessary for direct access to function call parameters
    GASEnvironment*             Env;
    int                         NArgs;
    int                         FirstArgBottomIndex;

    // callee and caller
    GASValue                    Callee, Caller;


    GASLocalFrame(GASRefCountCollector* pcc) 
        : GASRefCountBase<GASLocalFrame>(pcc), SuperThis(0), Env(0), NArgs(0), FirstArgBottomIndex(0) {}
    ~GASLocalFrame() 
    { }

    void ReleaseFramesForLocalFuncs ();
    GINLINE GASValue& Arg(int n) const;
};

#include "GASFunctionRefImpl.h"

#endif //INC_GFXOBJECT_H
