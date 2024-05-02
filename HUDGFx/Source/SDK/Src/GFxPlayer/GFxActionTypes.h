/**********************************************************************

Filename    :   GFxActionTypes.h
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


#ifndef INC_GFXACTIONTYPES_H
#define INC_GFXACTIONTYPES_H

#include "GRefCount.h"
#include "GFunctions.h"
#include "GArray.h"
#include "GFxASString.h"
#include "GFxEvent.h"
#include "GFxLog.h"
#include "GFxStringBuiltins.h"
#include "GFxStreamContext.h"

#include <stdio.h>

#if !defined(GFC_OS_SYMBIAN) && !defined(GFC_CC_RENESAS) && !defined(GFC_OS_PS2)
#include <wchar.h>
#endif

// ***** Declared Classes
struct GASNameFunction;
struct GASNameNumber;
class GFxEventId;
class GASWithStackEntry;
class GASActionBuffer;

// ***** External Classes
class GASValue;
class GASObject;
class GASFnCall;
class GASActionBuffer;
class GASObjectInterface;
class GFxStream;
class GASEnvironment;

class GASString;
class GASStringContext;
enum  GASBuiltinType;

class GFxASCharacter;
class GFxMovieDataDef;

class GASExecutionContext;

#ifndef GFC_NO_GC
// Refcount collection version
#include "GRefCountCollector.h"
typedef GRefCountBaseGC<GFxStatMV_ActionScript_Mem> GASRefCountBaseType;
class GASRefCountCollector : public GRefCountCollector<GFxStatMV_ActionScript_Mem>
{
    UInt    FrameCnt;
    UInt    MaxRootCount;
    UInt    PeakRootCount;
    UInt    LastRootCount;
    UInt    LastCollectedRoots;
    UInt    LastPeakRootCount;
    UInt    TotalFramesCount;
    UInt    LastCollectionFrameNum;

    UInt    PresetMaxRootCount;
    UInt    MaxFramesBetweenCollections;

    GINLINE void Collect(Stats* pstat = NULL)
    {
        GRefCountCollector<GFxStatMV_ActionScript_Mem>::Collect(pstat);
    }
public:
    GASRefCountCollector();

    void SetParams(UInt frameBetweenCollections, UInt maxRootCount);

    // This method should be called every frame (every full advance). 
    // It evaluates necessity of collection and performs it if necessary.
    void AdvanceFrame(UInt* movieFrameCnt, UInt* movieLastCollectFrame);

    // Forces collection.
    void ForceCollect();
    
    // Forces emergency collect. This method is called when memory heap cap is 
    // reached. It tries to free as much memory as possible.
    void ForceEmergencyCollect();
};
template <class T> class GASRefCountBase : public GASRefCountBaseType 
{
public:
    GASRefCountBase(GASRefCountCollector* pcc) :
      GASRefCountBaseType(pcc) {}
private:
    GASRefCountBase(const GASRefCountBase&) {} // forbidden
};
#define GASWeakPtr GPtr

#else
// Regular refcounting
class GASRefCountCollector : public GRefCountBase<GASRefCountCollector, GFxStatMV_ActionScript_Mem> { char __dummy; };
template <class T> class GASRefCountBase : public GRefCountBaseWeakSupport<T, GFxStatMV_ActionScript_Mem> 
{
public:
    GASRefCountBase(GASRefCountCollector*) :
      GRefCountBaseWeakSupport<T, GFxStatMV_ActionScript_Mem>() {}
private:
    GASRefCountBase(const GASRefCountBase&) {} // forbidden
};
#define GASWeakPtr GWeakPtr
#endif //GFC_NO_GC

typedef GArray<GASValue> GASValueArray;

// Floating point type used by ActionScript.
// Generally, this should be Double - a 64-bit floating point value; however,
// on systems that do not have Double (PS2) this can be another type, such as a float.
// A significant problem with Float is that in AS it is used to substitute integers,
// but it does not have enough mantissa bits so do so correctly. To account for this,
// we could store both Int + Float in GASFloat and do some conditional logic...

#ifdef GFC_NO_DOUBLE
typedef Float       GASNumber;
// This is a version of float that can be stored in a union; thus GASFloat 
// must be convertible to GASFloatValue, though perhaps with some extra calls.
typedef GASNumber   GASNumberValue;

//#define   GFX_ASNUMBER_NAN
#define GFX_ASNUMBER_ZERO   0.0f

#else

typedef Double      GASNumber;
// This is a version of float that can be stored in a union; thus GASFloat 
// must be convertible to GASFloatValue, though perhaps with some extra calls.
typedef GASNumber   GASNumberValue;

//#define   GFX_ASNUMBER_NAN
#define GFX_ASNUMBER_ZERO   0.0

#endif

// C-code function type used by values.
typedef void (*GASCFunctionPtr)(const GASFnCall& fn);

// Name-Function pair for statically defined operations 
// (like String::concat, etc)
struct GASNameFunction
{
    const char*     Name;
    GASCFunctionPtr Function;

    static void AddConstMembers(GASObjectInterface* pobj, GASStringContext *psc,
                                const GASNameFunction* pfunctions,
                                UByte flags = 0);
};

// Name-Number pair for statically defined constants 
struct GASNameNumber
{
    const char* Name;
    int         Number;
};

// 3-way bool type - Undefined, True, False states
class Bool3W
{
    enum
    {
        B3W_Undefined, B3W_True, B3W_False
    };
    UByte Value;
public:
    inline Bool3W() : Value(B3W_Undefined) {}
    inline Bool3W(bool v) : Value(UByte((v)?B3W_True:B3W_False)) {}
    inline Bool3W(const Bool3W& v): Value(v.Value) {}

    inline Bool3W& operator=(const Bool3W& o)
    {
        Value = o.Value;
        return *this;
    }
    inline Bool3W& operator=(bool o)
    {
        Value = UByte((o) ? B3W_True : B3W_False);
        return *this;
    }
    inline bool operator==(const Bool3W& o) const
    {
        return (Value == B3W_Undefined || o.Value == B3W_Undefined) ? false : Value == o.Value;
    }
    inline bool operator==(bool o) const
    {
        return (Value == B3W_Undefined) ? false : Value == ((o) ? B3W_True : B3W_False);
    }
    inline bool operator!=(const Bool3W& o) const
    {
        return (Value == B3W_Undefined || o.Value == B3W_Undefined) ? false : Value != o.Value;
    }
    inline bool operator!=(bool o) const
    {
        return (Value == B3W_Undefined) ? false : Value != ((o) ? B3W_True : B3W_False);
    }
    inline bool IsDefined() const { return Value != B3W_Undefined; }
    inline bool IsTrue() const    { return Value == B3W_True; }
    inline bool IsFalse() const   { return Value == B3W_False; }
};


// ***** GFxEventId

// Describes various ActionScript events that can be fired of and that
// users can install handlers for through on() and onClipEvent() handlers.
// Events include button and movie clip mouse actions, keyboard handling,
// load & unload notifications, etc. 

class GFxEventId
{
public:
    // These must match the function names in GFxEventId::GetFunctionName()
    enum IdCode
    {
        Event_Invalid,

        // These are for buttons & sprites.
        Event_Load              = 0x000001,
        Event_EnterFrame        = 0x000002,
        Event_Unload            = 0x000004,
        Event_MouseMove         = 0x000008,
        Event_MouseDown         = 0x000010,
        Event_MouseUp           = 0x000020,
        Event_KeyDown           = 0x000040,
        Event_KeyUp             = 0x000080,
        Event_Data              = 0x000100,
        Event_Initialize        = 0x000200,
        
        // button events
        Event_Press             = 0x000400,
        Event_Release           = 0x000800,
        Event_ReleaseOutside    = 0x001000,
        Event_RollOver          = 0x002000,
        Event_RollOut           = 0x004000,
        Event_DragOver          = 0x008000,
        Event_DragOut           = 0x010000,
        Event_KeyPress          = 0x020000,

        Event_Construct         = 0x040000,

        Event_PressAux          = 0x080000,
        Event_ReleaseAux        = 0x100000,
        Event_ReleaseOutsideAux = 0x200000,
        Event_DragOverAux       = 0x400000,
        Event_DragOutAux        = 0x800000,
        
        Event_AuxEventMask      = 0xF80000,
        Event_ButtonEventsMask  = 0xFBFC00,

        //Event_LastCombined      = Event_Construct,
        Event_LastCombined      = Event_DragOutAux,
        Event_NextAfterCombined = (Event_LastCombined << 1),
        
        // These are for the MoveClipLoader ActionScript only
        Event_LoadStart         = Event_NextAfterCombined,
        Event_LoadError,
        Event_LoadProgress,
        Event_LoadInit,

        // These are for the XMLSocket ActionScript only
        Event_SockClose,
        Event_SockConnect,
        Event_SockData,
        Event_SockXML,

        // These are for the XML ActionScript only
        Event_XMLLoad,
        Event_XMLData,

        Event_End,
        Event_COUNT = 20 + (Event_End - Event_NextAfterCombined) + 5  // PPS: 5 for Aux events
    };

    UInt32              Id;
    UInt32              WcharCode;
    short               KeyCode;
    union
    {
        UByte           AsciiCode;
        UInt8           ButtonId;
    };
    union
    {
        SInt8           ControllerIndex;
        SInt8           MouseIndex;
        SInt8           KeyboardIndex;
    };
    UInt8               RollOverCnt;
    GFxSpecialKeysState SpecialKeysState;

    GFxEventId() : Id(Event_Invalid), WcharCode(0), KeyCode(GFxKey::VoidSymbol),AsciiCode(0),
        RollOverCnt(0) { ControllerIndex = -1; }

    GFxEventId(UInt32 id)
        : Id(id), WcharCode(0), KeyCode(GFxKey::VoidSymbol), AsciiCode(0), 
        RollOverCnt(0)
    {
        ControllerIndex = -1;
        // For the button key events, you must supply a keycode.
        // Otherwise, don't.
        //!GASSERT((KeyCode == GFxKey::VoidSymbol && (Id != Event_KeyPress)) || 
        //!     (KeyCode != GFxKey::VoidSymbol && (Id == Event_KeyPress)));
    }

    GFxEventId(UInt32 id, short c, UByte ascii, UInt32 wcharCode = 0, 
        UInt8 keyboardIndex = 0)
        : Id(id), WcharCode(wcharCode), KeyCode(c), AsciiCode(ascii),
        RollOverCnt(0)
    {
        ControllerIndex = (UInt8)keyboardIndex;
        // For the button key events, you must supply a keycode.
        // Otherwise, don't.
        //!GASSERT((KeyCode == GFxKey::VoidSymbol && (Id != Event_KeyPress)) || 
        //!     (KeyCode != GFxKey::VoidSymbol && (Id == Event_KeyPress)));
    }

    GINLINE bool IsButtonEvent() const
        { return (Id & Event_ButtonEventsMask) != 0; }

    GINLINE bool IsKeyEvent() const
        { return (Id & Event_KeyDown) || (Id & Event_KeyUp); }

    bool    operator==(const GFxEventId& id) const 
    { 
        return Id == id.Id && (!(Id & Event_KeyPress) || (KeyCode == id.KeyCode)); 
    }

    // Return the name of a method-handler function corresponding to this event.
    GASString           GetFunctionName(GASStringContext *psc) const;
    GASBuiltinType      GetFunctionNameBuiltinType() const;

    // converts keyCode/asciiCode from this event to the on(keyPress <>) format
    int                 ConvertToButtonKeyCode () const;

    UPInt               HashCode() const
    {
        UPInt hash = Id;
        if (Id & Event_KeyPress) 
            hash ^= KeyCode;
        return hash;
    }

    UInt GetEventsCount() const { return GBitsUtil::Cardinality(Id); }
};

// This event id should be used for button events, like onRollOver, onRollOut, etc
class GFxButtonEventId : public GFxEventId
{
public:
    GFxButtonEventId(UInt32 id, SInt mouseIndex = -1, UInt rollOverCnt = 0, UInt8 buttonId = 0) :
        GFxEventId(id)
    {
        ButtonId    = buttonId;
        MouseIndex  = (SInt8)mouseIndex;
        RollOverCnt = (UInt8)rollOverCnt;
    }
};

struct GFxEventIdHashFunctor
{
    UPInt operator()(const GFxEventId& data) const 
    { 
        return data.HashCode();
    }
};

// ***** GASWithStackEntry

// The "with" stack is for Pascal-like 'with() { }' statement scoping. Passed
// as argument for GASActionBuffer execution and used by variable lookup methods.

class GASWithStackEntry
{
protected:
    union
    {
        GASObject*      pObject;
        GFxASCharacter* pCharacter;
    };
    enum
    {
        Mask_IsObject = 0x80000000
    };
    // combo of Mask_IsObject bit (that indicates to use pObject)
    // and the end PC of with-block.
    UInt32              BlockEndPc;

    bool                IsObject() const { return (BlockEndPc & Mask_IsObject) != 0; }
public: 
    
    GASWithStackEntry() : BlockEndPc(0) { pObject = NULL; }
    GASWithStackEntry(GASObject* pobj, int end);
    GASWithStackEntry(GFxASCharacter* pcharacter, int end);
    ~GASWithStackEntry();

    // Conversions: non-inline because they need to know character types.
    GASObject*          GetObject() const;
    GFxASCharacter*     GetCharacter() const;
    GASObjectInterface* GetObjectInterface() const;
    int                 GetBlockEndPc() const { return (int)(BlockEndPc & (~Mask_IsObject)); }
};

typedef GArrayLH_POD<GASWithStackEntry, GFxStatMV_ActionScript_Mem> GASWithStackArray;


// ***** GASActionBuffer

// ActionScript buffer for action opcodes. The associated dictionary is stored in a
// separate GASActionBuffer class, which needs to be local for GFxMovieView instances.
// This class holds actions from DoActions and DoInitActions tags as well as ones 
// from "on..." event handlers. It owns the action buffer (means, action buffer is 
// allocated in Read method and freed in dtor). 
// Allocations occur from global heap, since actions might be shared between
// different SWF files and these actions should be accessible even then the owning
// SWF file is unloaded.
// GASActionBufferData ref-counting must be ThreadSafe since AS is executed that AddRefs to us
// from different threads.
class GASActionBufferData : public GRefCountBase<GASActionBufferData, GFxStatMD_ActionOps_Mem>
{
protected:
    // Create using CreateNew static method
    GASActionBufferData() 
        : pBuffer(0), BufferLen(0), SwdHandle(0), SWFFileOffset(0)
    {   
        //AB: technically, pdataDef should be not null always to avoid crash in
        // situation when memory heap with opcodes is freed before AS-function object
        // is freed. However, gfxexport may use this ctor with NULL for FSCommand list.
        //GASSERT(pdataDef);
    }
public:
    ~GASActionBufferData() { if (pBuffer) GFREE(pBuffer); }

    // Use this method to create an instance
    static GASActionBufferData* CreateNew();

    bool                IsNull() const                      { return BufferLen < 1 || pBuffer[0] == 0; }
    UInt                GetLength() const                   { return BufferLen; }
    const UByte*        GetBufferPtr() const                { return (IsNull()) ? NULL : pBuffer; }

    UInt32              GetSWFFileOffset() const            { return SWFFileOffset; }
    void                SetSWFFileOffset(UInt32 swfOffset)  { SWFFileOffset = swfOffset; }
    UInt32              GetSwdHandle() const                { return SwdHandle; }
    void                SetSwdHandle(UInt32 handle)         { SwdHandle = handle; }

    const char*         GetFileName () const   
    { 
#ifdef GFC_BUILD_DEBUG
        return FileName.ToCStr();
#else
        return NULL;
#endif
    }

    // Reads action instructions from the SWF file stream (DoActions/DoInitActions tags)
    void                Read(GFxStream* in, UInt actionLength);
    // Reads actions from event handlers.
    void                Read(GFxStreamContext* psc, UInt eventLength);
protected:
    // ActionScript byte-code data.
    UByte*              pBuffer;
    UInt                BufferLen;

    UInt32              SwdHandle;
    UInt32              SWFFileOffset;

#ifdef GFC_BUILD_DEBUG
    GStringLH           FileName;
#endif
};

// GASActionBuffer - movie view instance specific ActionBuffer class. This class
// caches dictionary data that needs to be local to movie view instances.

class GASActionBuffer : public GRefCountBaseNTS<GASActionBuffer, GFxStatMV_ActionScript_Mem>
{   
    friend class GASExecutionContext;

    // Pointer to opcode byte data.
    GPtr<GASActionBufferData>   pBufferData;  
    // Cached dictionary.
    GArrayCC<GASString, GFxStatMV_ActionScript_Mem>   Dictionary;    
    int                         DeclDictProcessedAt;        

public:
    GASActionBuffer(GASStringContext *psc, GASActionBufferData *pbufferData);
    ~GASActionBuffer();
   
    // Type call for execution; a distinction is sometimes necessary 
    // to determine how certain ops, such as 'var' declarations are handled.
    enum ExecuteType
    {
        Exec_Unknown    = 0, // just regular action buffer (frame)
        Exec_Function   = 1, // regular function
        Exec_Function2  = 2, // function2
        Exec_Event      = 3, // clip event
        Exec_SpecialEvent = 4  // special clip event, such as init, construct, load, unload.
                               // These events are not interrupted if object is being unloaded.
        // Must fit in a byte.
    };

    void    Execute(GASEnvironment* env);
    void    Execute(GASEnvironment* env,
                    int startPc, int execBytes,
                    GASValue* retval,
                    const GASWithStackArray* pinitialWithStack,
                    ExecuteType execType);

    void    ProcessDeclDict(GASStringContext *psc, UInt StartPc, UInt StopPc, class GFxActionLogger &logger);
    
    bool    IsNull() const      { return pBufferData->IsNull(); }
    UInt    GetLength() const   { return pBufferData->GetLength(); }
    const UByte* GetBufferPtr() const { return pBufferData->GetBufferPtr(); }
    const GASActionBufferData* GetActionBufferData() const { return pBufferData; }

protected:
    // Used in GotoFrame2 (0x9F) and WaitForFrame2 (0x8D)
    bool ResolveFrameNumber (GASEnvironment* env, const GASValue& frameValue, GFxASCharacter** pptarget, UInt* frameNumber);
};

#endif //INC_GFXACTIONTYPES_H
