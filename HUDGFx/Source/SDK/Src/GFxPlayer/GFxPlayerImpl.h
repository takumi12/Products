/**********************************************************************

Filename    :   GFxPlayerImpl.h
Content     :   MovieRoot and Definition classes
Created     :   
Authors     :   Michael Antonov, Artem Bolgar, Prasad Silva

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

Notes       :   This file contains class declarations used in
                GFxPlayerImpl.cpp only. Declarations that need to be
                visible by other player files should be placed
                in GFxCharacter.h.


Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxPlayerImpl_H
#define INC_GFxPlayerImpl_H

#include "GFxCharacter.h"
#include "GMath.h"
#include "GFile.h"

#include "GFxAction.h"
#include "GFxDlist.h"
#include "GFxLoaderImpl.h"
#include "GFxFontResource.h"
#include "GFxShape.h"
#include "AS/GASMovieClipLoader.h"
#include "AS/GASLoadVars.h"
#include "GFxMediaInterfaces.h"

#include <GFxXMLSupport.h>
#ifndef GFC_NO_CSS_SUPPORT
#include "AS/GASStyleSheet.h"
#endif

#ifndef GFC_NO_3D
#include "GScreenToWorld.h"
#endif

// Font managers are allocated in sprite root nodes.
#include "GFxFontManager.h"

#include "Text/GFxTextCore.h" // for GFxTextAllocator

// For now
#include "GFxMovieDef.h"

#if defined(GFC_OS_WIN32) && !defined(GFC_NO_BUILTIN_KOREAN_IME) && !defined(GFC_NO_IME_SUPPORT)
#include "IME/GFxIMEImm32Dll.h"
#endif //defined(GFC_OS_WIN32) && !defined(GFC_NO_BUILTIN_KOREAN_IME) && !defined(GFC_NO_IME_SUPPORT)

// This is internal file that uses GHEAP_NEW.
#include "GHeapNew.h"

#define GFX_MAX_CONTROLLERS_SUPPORTED 16

// ***** Declared Classes
class GFxMovieDefImpl;
class GFxMovieRoot;
class GFxSpriteDef;
class GFxSprite;
// Helpers
class GFxImportInfo;
class GFxSwfEvent;
class GFxLoadQueueEntry;
class GFxDoublePrecisionGuard;
// Tag classes
class GFxPlaceObject;
class GFxPlaceObject2;
class GFxPlaceObject3;
class GFxRemoveObject;
class GFxRemoveObject2;
class GFxSetBackgroundColor;
// class GASDoAction;           - in GFxAction.cpp
// class GFxStartSoundTag;      - in GFxSound.cpp

// ***** External Classes
class GFxLoader;
class GFxAmpViewStats;

// From "AS/GASTimers.h"
class GASIntervalTimer;

class GFxIMECandidateListStyle;

#ifndef GFC_NO_SOUND
class GFxAudioBase;
class GSoundRenderer;
#endif

// ****


// This class provides queue for input events, such as keyboard and mouse (mice)
// events
class GFxInputEventsQueue
{
public:
    typedef UInt16 ButtonsStateType;
    enum MouseButtonMask
    {
        MouseButton_Left   = 1,
        MouseButton_Right  = 2,
        MouseButton_Middle = 4,

        MouseButton_MaxNum = (sizeof(ButtonsStateType)*8),
        MouseButton_LastButton = (1 << (MouseButton_MaxNum - 1)),   // 0x8000
        MouseButton_AllMask    = ((1 << MouseButton_MaxNum) - 1)    // 0xFFFF
    };
    enum MouseButtonFlags
    {
        MouseButton_Released = 0x80,  // indicates mouse button released
        MouseButton_Move     = 0x40,  // indicates mouse moved
        MouseButton_Wheel    = 0x20
    };
    struct QueueEntry
    {
        enum QueueEntryType
        {
            QE_Mouse,
            QE_Key
        } t;
        struct MouseEntry
        {
            Float               PosX, PosY;     // mouse coords, in twips, when button event happened
            ButtonsStateType    ButtonsState; // bit-mask for buttons (see MouseButtonMask)
            SInt8               WheelScrollDelta; // delta for mouse wheel
            UInt8               Flags;        // see MouseButtonFlags
            UInt8               MouseIndex;

            GPointF GetPosition() const   { return GPointF(PosX, PosY); }
            UInt    GetMouseIndex() const { return MouseIndex; }

            bool IsButtonsStateChanged() const  { return ButtonsState != 0; }
            bool IsAnyButtonPressed() const  
            { 
                return !(Flags & (MouseButton_Move | MouseButton_Released)) && 
                    ButtonsState != 0; 
            }
            bool IsAnyButtonReleased() const 
            { 
                return !(Flags & MouseButton_Move) && 
                    (Flags & MouseButton_Released) && ButtonsState != 0; 
            }
            bool IsMoved() const          { return (Flags & MouseButton_Move) != 0; }
            bool IsLeftButton() const     { return (ButtonsState & MouseButton_Left) != 0; }
            bool IsRightButton() const    { return (ButtonsState & MouseButton_Right) != 0; }
            bool IsMiddleButton() const   { return (ButtonsState & MouseButton_Middle) != 0; }
            // button is specified as index: 0 - left, 1 - right, 2 - middle, 3 - .... etc
            bool IsIndexedButton(UInt index) const { return (ButtonsState & (1 << index)) != 0; }
            // buttons are specified as a bit mask
            bool AreButtons(UInt mask) const { return (ButtonsState & mask) != 0; }
            bool IsMouseWheel() const     { return (Flags & MouseButton_Wheel) != 0; }
        };
        struct KeyEntry
        {
            UInt32              WcharCode;
            short               Code;
            UByte               AsciiCode;
            UInt8               SpecialKeysState;
            UInt8               KeyboardIndex;
            bool                KeyIsDown;
        };
        union Entry
        {
            MouseEntry mouseEntry;
            KeyEntry   keyEntry;
        } u;

        bool IsMouseEntry() const { return t == QE_Mouse; }
        bool IsKeyEntry() const { return t == QE_Key; }

        MouseEntry& GetMouseEntry()             { return u.mouseEntry; }
        const MouseEntry& GetMouseEntry() const { return u.mouseEntry; }

        KeyEntry& GetKeyEntry()             { return u.keyEntry; }
        const KeyEntry& GetKeyEntry() const { return u.keyEntry; }
    };
protected:
    enum
    {
        Queue_Length = 100
    };
    QueueEntry  Queue[Queue_Length];
    UPInt       StartPos;
    UPInt       UsedEntries;
    GPointF     LastMousePos[GFC_MAX_MICE_SUPPORTED];
    UInt32      LastMousePosMask;

    const QueueEntry*   PeekLastQueueEntry() const; 
    QueueEntry*         PeekLastQueueEntry()
    {
        return const_cast<QueueEntry*>(const_cast<const GFxInputEventsQueue*>(this)->PeekLastQueueEntry());
    }
    QueueEntry*         AddEmptyQueueEntry();
    bool                IsAnyMouseMoved() const;
public:
    GFxInputEventsQueue();

    GINLINE bool IsQueueEmpty() const { return UsedEntries == 0 && LastMousePosMask == 0; }
    const QueueEntry* GetEntry();
    void ResetQueue() { UsedEntries = 0; }

    // mouse related methods
    void AddMouseMove(UInt mouseIndex, const GPointF& pos); 
    void AddMouseButtonEvent(UInt mouseIndex, const GPointF& pos, UInt buttonsSt, UInt flags);
    void AddMouseWheel(UInt mouseIndex, const GPointF& pos, SInt delta);

    GINLINE void AddMousePress(UInt mouseIndex, const GPointF& pos, UInt buttonsSt)
    {   
        AddMouseButtonEvent(mouseIndex, pos, buttonsSt, 0);
    }
    GINLINE void AddMouseRelease(UInt mouseIndex, const GPointF& pos, UInt buttonsSt)
    {
        AddMouseButtonEvent(mouseIndex, pos, buttonsSt, MouseButton_Released);
    }

    // keyboard related events
    void AddKeyEvent(short code, UByte ascii, UInt32 wcharCode, bool isKeyDown, GFxSpecialKeysState specialKeysState = 0,
                     UInt8 keyboardIndex = 0);
    void AddKeyDown(short code, UByte ascii, GFxSpecialKeysState specialKeysState = 0,
                    UInt8 keyboardIndex = 0)
    {
        AddKeyEvent(code, ascii, 0, true, specialKeysState, keyboardIndex);
    }
    void AddKeyUp  (short code, UByte ascii, GFxSpecialKeysState specialKeysState = 0,
                    UInt8 keyboardIndex = 0)
    {
        AddKeyEvent(code, ascii, 0, false, specialKeysState, keyboardIndex);
    }
    void AddCharTyped(UInt32 wcharCode, UInt8 keyboardIndex = 0)
    {
        AddKeyEvent(0, 0, wcharCode, true, 0, keyboardIndex);
    }
};


// ***** GFxMouseState
// This class keeps mouse states and queue of mouse events, such as down/up, wheel, move.
// Also, this class tracks topmost entities for generating button events later.
// If player supports more than one mouse, then implementation should contain
// corresponding number of GFxMouseState's instances.
class GFxMouseState
{
public:
    typedef UInt16 ButtonsStateType;
    enum MouseButtonMask
    {
        MouseButton_Left   = 1,
        MouseButton_Right  = 2,
        MouseButton_Middle = 4,

        MouseButton_MaxNum = (sizeof(ButtonsStateType)*8),
        MouseButton_LastButton = (1 << (MouseButton_MaxNum - 1)),   // 0x8000
        MouseButton_AllMask    = ((1 << MouseButton_MaxNum) - 1)    // 0xFFFF
    };
    enum MouseButtonFlags
    {
        MouseButton_Released = 0x80,  // indicates mouse button released
        MouseButton_Move     = 0x40,  // indicates mouse moved
        MouseButton_Wheel    = 0x20
    };
protected:
    mutable GWeakPtr<GFxASCharacter>    TopmostEntity;  // what's underneath the mouse right now
    mutable GWeakPtr<GFxASCharacter>    PrevTopmostEntity; // previous topmost entity
    mutable GWeakPtr<GFxASCharacter>    ActiveEntity;   // entity that currently owns the mouse pointer

    UInt        CurButtonsState;  // current mouse buttons state, pressed buttons marked by bits set
    UInt        PrevButtonsState; // previous mouse buttons state, pressed buttons marked by bits set
    GPointF     LastPosition;
    
    UInt        CursorType;   // type of cursor

    bool        TopmostEntityIsNull           :1; // indicates if topmost ent is null or not
    bool        PrevTopmostEntityWasNull      :1;
    bool        MouseInsideEntityLast         :1;
    bool        MouseMoved                    :1; // indicates mouse was moved
    bool        Activated                     :1; // is mouse activated

public:
    GFxMouseState();

    void ResetState();

    void UpdateState(const GFxInputEventsQueue::QueueEntry&);

    GINLINE bool IsMouseMoved() const { return MouseMoved; }

    void SetCursorType(UInt curs)     { CursorType = curs; }
    UInt GetCursorType() const        { return CursorType; }

    GPointF GetLastPosition() const 
    { 
        return LastPosition;
    }
    UInt GetButtonsState() const { return CurButtonsState; }
    UInt GetPrevButtonsState() const { return PrevButtonsState; }

    GPtr<GFxASCharacter> GetActiveEntity() const  
    { 
        return *const_cast<GWeakPtr<GFxASCharacter>* >(&ActiveEntity); 
    }
    GPtr<GFxASCharacter> GetTopmostEntity() const 
    { 
        return *const_cast<GWeakPtr<GFxASCharacter>* >(&TopmostEntity); 
    }

    void SetActiveEntity(GFxASCharacter* pch)
    {
        ActiveEntity = pch;
    }
    void SetTopmostEntity(GFxASCharacter* pch)
    {
        PrevTopmostEntity           = TopmostEntity;
        PrevTopmostEntityWasNull    = TopmostEntityIsNull;
        TopmostEntity               = pch;
        TopmostEntityIsNull         = (pch == NULL);
    }
    bool IsTopmostEntityChanged() const
    {
        GPtr<GFxASCharacter> ptop  = TopmostEntity;
        GPtr<GFxASCharacter> pprev = PrevTopmostEntity;
        return (ptop != pprev) ||
               (!ptop && !TopmostEntityIsNull) ||
               (!pprev && !PrevTopmostEntityWasNull);
    }
    void SetMouseInsideEntityLast(bool f) { MouseInsideEntityLast = f; }
    bool IsMouseInsideEntityLast() const { return MouseInsideEntityLast; }

    bool IsActivated() const { return Activated; }

    void ResetPrevButtonsState() { PrevButtonsState = CurButtonsState; }
};


// ***** Load Queue used by movie root

class GFxLoadQueueEntry : public GNewOverrideBase<GFxStatMV_Other_Mem>
{
    friend class GFxMovieRoot;
    // Next in load queue
    GFxLoadQueueEntry*          pNext;

public:

    // Load type, technically not necessary since URL & Level values are enough.
    // May become useful in the future, as we add other load options.
    enum LoadTypeFlags 
    {
        LTF_None        = 0,
        LTF_UnloadFlag  = 0x01, // Unload instead of load
        LTF_LevelFlag   = 0x02,  // Level instead of target character.
        LTF_VarsFlag    = 0x04,
        LTF_XMLFlag     = 0x08,
        LTF_CSSFlag     = 0x10,
    };  
    enum LoadType
    {   
        LT_LoadMovie    = LTF_None,
        LT_UnloadMovie  = LTF_UnloadFlag,
        LT_LoadLevel    = LTF_LevelFlag,
        LT_UnloadLevel  = LTF_UnloadFlag | LTF_LevelFlag,
        LT_LoadXML      = LTF_XMLFlag,
        LT_LoadCSS      = LTF_CSSFlag,
    };

    enum LoadMethod
    {
        LM_None,
        LM_Get,
        LM_Post
    };

    LoadType                    Type;
    LoadMethod                  Method;
    GString                     URL;
    SInt                        Level;
    GPtr<GFxCharacterHandle>    pCharacter;
    bool                        QueitOpen;
    // movie clip loader and variables loader should be held by GASValue because 
    // there is a cross-reference inside of their instances: loader._listener[0] = loader. 
    // GASValue can release such cross-referenced objects w/o memory leak, whereas GPtr can't.
    GASValue                    MovieClipLoaderHolder;
    GASValue                    LoadVarsHolder;
    UInt32                      EntryTime;

#ifndef GFC_NO_XML_SUPPORT
    // the following structure is used to load xml files. the GASValue holds the
    // actionscript object (explanation above). The loader is kept as a thread safe
    // reference. this seperation is required because the loader object is used
    // inside the loading thread in progressive loading mode. this causes problems
    // with the GASValue reference.
    struct XMLHolderType
    {
        GASValue                ASObj;
        GPtr<GFxASXMLFileLoader> Loader;
    };
    XMLHolderType               XMLHolder;
#endif

#ifndef GFC_NO_CSS_SUPPORT
    // the following structure is used to load xml files. the GASValue holds the
    // actionscript object (explanation above). The loader is kept as a thread safe
    // reference. this seperation is required because the loader object is used
    // inside the loading thread in progressive loading mode. this causes problems
    // with the GASValue reference.
    struct CSSHolderType
    {
        GASValue                ASObj;
        GPtr<GFxASCSSFileLoader> Loader;
    };
    CSSHolderType               CSSHolder;
#endif

    bool                        Canceled;

    // Constructor helper.      
    void    PConstruct(LoadType type, GFxCharacterHandle* pchar, SInt level, const GString &url, LoadMethod method, bool queitOpen)
    {
        Type        = type;
        pCharacter  = pchar;
        Method      = method;
        Level       = level;
        pNext       = 0;
        URL         = url;
        QueitOpen   = queitOpen;
        EntryTime   = GFC_MAX_UINT32;
        Canceled    = false;
    }

    // *** Constructors
    GFxLoadQueueEntry(const GString &url, LoadMethod method, bool loadingVars = false, bool queitOpen = false)
    {
        LoadTypeFlags typeFlag;
        if (loadingVars)
        {
            typeFlag = LTF_VarsFlag;
        }
        else
        {
            typeFlag = url ? LTF_None : LTF_UnloadFlag;
        }
        PConstruct((LoadType) (typeFlag), NULL, -1, url, method, queitOpen);
    }

    GFxLoadQueueEntry(GFxCharacterHandle* pchar, const GString &url, LoadMethod method, bool loadingVars = false, bool queitOpen = false)
    {
        LoadTypeFlags typeFlag;
        if (loadingVars)
        {
            typeFlag = LTF_VarsFlag;
        }
        else
        {
            typeFlag = url ? LTF_None : LTF_UnloadFlag;
        }
        PConstruct((LoadType) (typeFlag), pchar, -1, url, method, queitOpen);
    }   
    
    GFxLoadQueueEntry(SInt level, const GString &url, LoadMethod method, bool loadingVars = false, bool queitOpen = false)
    {
        LoadTypeFlags typeFlag;
        if (loadingVars)
        {
            typeFlag = LTF_VarsFlag;
        }
        else
        {
            typeFlag = url ? LTF_None : LTF_UnloadFlag;
        }
        PConstruct((LoadType) (LTF_LevelFlag | typeFlag), 0, level, url, method, queitOpen);
    }

    void CancelByNamePath(const GASString& namePath)
    {
        if (pCharacter && pCharacter->GetNamePath() == namePath)
            Canceled = true;
    }
    void CancelByLevel(SInt level)
    {
        if (Level != -1 && Level == level)
            Canceled = true;
    }
#ifndef GFC_NO_XML_SUPPORT
    void CancelByXMLASObjPtr(GASObject* pxmlobj)
    {
        if (!XMLHolder.ASObj.IsUndefined() && pxmlobj == XMLHolder.ASObj.ToObject(NULL))
            Canceled = true;
    }
#endif
#ifndef GFC_NO_CSS_SUPPORT
    void CancelByCSSASObjPtr(GASObject* pobj)
    {
        if (!CSSHolder.ASObj.IsUndefined() && pobj == CSSHolder.ASObj.ToObject(NULL))
            Canceled = true;
    }
#endif
};

class GFxLoadQueueEntryMT : public GNewOverrideBase<GFxStatMV_Other_Mem>
{
protected:
    friend class GFxMovieRoot;
    GFxLoadQueueEntryMT*      pNext;
    GFxLoadQueueEntryMT*      pPrev;

    GFxMovieRoot*             pMovieRoot;
    GFxLoadQueueEntry*        pQueueEntry;

public:
    GFxLoadQueueEntryMT(GFxLoadQueueEntry* pqueueEntry, GFxMovieRoot* pmovieRoot);
    virtual ~GFxLoadQueueEntryMT();

    void Cancel();

    // Check if a movie is loaded. Returns false in the case if the movie is still being loaded.
    // Returns true if the movie is loaded completely or in the case of errors.
    virtual bool LoadFinished() = 0;

    // Check if preloading stage of the loading task finished.
    virtual bool IsPreloadingFinished() { return true; }
};

// ***** GFxMoviePreloadTask
// The purpose of this task is to find a requested movie on the media, create a moviedefimp and 
// submit a task for the actual movie loading.
class GFxMoviePreloadTask : public GFxTask
{
public:
    GFxMoviePreloadTask(GFxMovieRoot* pmovieRoot, const GString& url, bool stripped, bool quietOpen);

    virtual void    Execute();

    GFxMovieDefImpl* GetMoiveDefImpl();

    bool IsDone() const;

private:
    GPtr<GFxLoadStates>   pLoadStates;
    UInt                  LoadFlags;
    GString               Level0Path;

    GString               Url;
    GString               UrlStrGfx;
    GPtr<GFxMovieDefImpl> pDefImpl;
    volatile UInt         Done;
};

//  ****** GFxLoadingMovieEntry
class GFxLoadQueueEntryMT_LoadMovie : public GFxLoadQueueEntryMT
{
    friend class GFxMovieRoot;

    GPtr<GFxMoviePreloadTask> pPreloadTask;
    GPtr<GFxSprite>           pNewChar;
    bool                      FirstCheck;
    GPtr<GFxASCharacter>      pOldChar;      
    GFxResourceId             NewCharId;
    bool                      CharSwitched;
    UInt                      BytesLoaded;
    bool                      FirstFrameLoaded;
public:
    GFxLoadQueueEntryMT_LoadMovie(GFxLoadQueueEntry* pqueueEntry, GFxMovieRoot* pmovieRoot);
    ~GFxLoadQueueEntryMT_LoadMovie();

    // Check if a movie is loaded. Returns false in the case if the movie is still being loaded.
    // Returns true if the movie is loaded completely or in the case of errors.
    bool LoadFinished();
    bool IsPreloadingFinished();

};

#ifndef GFC_NO_FXPLAYER_AS_LOADVARS
// ****** GFxLoadVarsTask
// Reads a file with loadVars data on a separate thread
class GFxLoadVarsTask : public GFxTask
{
public:
    GFxLoadVarsTask(GFxLoadStates* pls, const GString& level0Path, const GString& url);

    virtual void    Execute();

    // Retrieve loadVars data and file length. Returns false if data is not ready yet.
    // if returned fileLen is less then 0 then the requested url was not read successfully. 
    bool GetData(GString* data, SInt* fileLen, bool* succeeded) const;

private:
    GPtr<GFxLoadStates> pLoadStates;
    GString             Level0Path;
    GString             Url;

    GString             Data;
    SInt                FileLen;
    volatile UInt       Done;
    bool                Succeeded;
};

class GFxLoadQueueEntryMT_LoadVars : public GFxLoadQueueEntryMT
{
    GPtr<GFxLoadVarsTask> pTask;
    GPtr<GFxLoadStates>   pLoadStates;

public:
    GFxLoadQueueEntryMT_LoadVars(GFxLoadQueueEntry* pqueueEntry, GFxMovieRoot* pmovieRoot);
    ~GFxLoadQueueEntryMT_LoadVars();

    // Check if a movie is loaded. Returns false in the case if the movie is still being loaded.
    // Returns true if the movie is loaded completely or in the case of errors.
    bool LoadFinished();
};
#endif //#ifndef GFC_NO_FXPLAYER_AS_LOADVARS

#ifndef GFC_NO_XML_SUPPORT
// ****** GFxLoadXMLTask
// Reads a file with loadXML data on a separate thread
class GFxLoadXMLTask : public GFxTask
{
public:
    GFxLoadXMLTask(GFxLoadStates* pls, const GString& level0Path, const GString& url, 
        GFxLoadQueueEntry::XMLHolderType xmlholder);

    virtual void    Execute();

    bool IsDone() const;

private:
    GPtr<GFxLoadStates>      pLoadStates;
    GString                  Level0Path;
    GString                  Url;
    GPtr<GFxASXMLFileLoader> pXMLLoader;

    volatile UInt            Done;
};

class GFxLoadQueueEntryMT_LoadXML : public GFxLoadQueueEntryMT
{
    GPtr<GFxLoadXMLTask>  pTask;
    GPtr<GFxLoadStates>   pLoadStates;

public:
    GFxLoadQueueEntryMT_LoadXML(GFxLoadQueueEntry* pqueueEntry, GFxMovieRoot* pmovieRoot);
    ~GFxLoadQueueEntryMT_LoadXML();

    // Check if a movie is loaded. Returns false in the case if the movie is still being loaded.
    // Returns true if the movie is loaded completely or in the case of errors.
    bool LoadFinished();
};
#endif


#ifndef GFC_NO_CSS_SUPPORT
// ****** GFxLoadCSSTask
// Reads a file with loadXML data on a separate thread
class GFxLoadCSSTask : public GFxTask
{
public:
    GFxLoadCSSTask(GFxLoadStates* pls, const GString& level0Path, const GString& url, 
        GFxLoadQueueEntry::CSSHolderType xmlholder);

    virtual void    Execute();

    bool IsDone() const;

private:
    GPtr<GFxLoadStates>      pLoadStates;
    GString                  Level0Path;
    GString                  Url;
    GPtr<GFxASCSSFileLoader> pLoader;

    volatile UInt            Done;
};

class GFxLoadQueueEntryMT_LoadCSS : public GFxLoadQueueEntryMT
{
    GPtr<GFxLoadCSSTask>  pTask;
    GPtr<GFxLoadStates>   pLoadStates;

public:
    GFxLoadQueueEntryMT_LoadCSS(GFxLoadQueueEntry* pqueueEntry, GFxMovieRoot* pmovieRoot);
    ~GFxLoadQueueEntryMT_LoadCSS();

    // Check if a movie is loaded. Returns false in the case if the movie is still being loaded.
    // Returns true if the movie is loaded completely or in the case of errors.
    bool LoadFinished();
};
#endif


//  ****** GFxASMouseListener

class GFxASMouseListener
{
public:
    virtual ~GFxASMouseListener() {}

    virtual void OnMouseMove(GASEnvironment *penv, UInt mouseIndex) const = 0;
    virtual void OnMouseDown(GASEnvironment *penv, UInt mouseIndex, UInt button, GFxASCharacter* ptarget) const = 0;
    virtual void OnMouseUp(GASEnvironment *penv, UInt mouseIndex, UInt button, GFxASCharacter* ptarget) const   = 0;
    virtual void OnMouseWheel(GASEnvironment *penv, UInt mouseIndex, int sdelta, GFxASCharacter* ptarget) const = 0;

    virtual bool IsEmpty() const = 0;
};

//  ***** GFxMovieDefRootNode

// GFxMovieDefRootNode is maintained in GFxMovieRoot for each GFxMovieDefImpl; this
// node is referenced by every GFxSprite which has its own GFxMovieDefImpl. We keep
// a distinction between imported and loadMovie based root sprites.
// 
// Purpose:
//  1) Allows us to cache temporary state of loading MovieDefs in the beginning of
//     Advance so that it is always consistent for all instances. If not done, it
//     would be possible to different instances of the same progressively loaded
//     file to be in different places in the same GfxMovieView frame.
//  2) Allows for GFxFontManager to be shared among same-def movies.

struct GFxMovieDefRootNode : public GListNode<GFxMovieDefRootNode>, public GNewOverrideBase<GFxStatMV_Other_Mem>
{
    // The number of root sprites that are referencing this GFxMovieDef. If this
    // number goes down to 0, the node is deleted.
    UInt        SpriteRefCount;

    // GFxMovieDef these root sprites use. No need to AddRef because GFxSprite does.
    GFxMovieDefImpl* pDefImpl;

    // Cache the number of frames that was loaded.
    UInt        LoadingFrame;
    UInt32      BytesLoaded;
    // Imports don't get to rely on LoadingFrame because they are usually nested
    // and have independent frame count from import root. This LoadingFrame is
    // not necessary anyway because imports are guaranteed to be fully loaded.
    bool        ImportFlag;

    // The pointer to font manager used for a sprite. We need to keep font manager
    // here so that it can keep references to all GFxMovieDefImpl instances for
    // all fonts obtained through GFxFontLib.
    GPtr<GFxFontManager> pFontManager;

    GFxMovieDefRootNode(GFxMovieDefImpl *pdefImpl, bool importFlag = 0)
        : SpriteRefCount(1), pDefImpl(pdefImpl), ImportFlag(importFlag)
    {  }
};

template <UInt32 mask>
inline void G_SetFlag(UInt32& pvalue, bool state)
{
    (state) ? (pvalue |= mask) : (pvalue &= (~mask));
}


template <UInt32 mask>
inline bool G_IsFlagSet(UInt32 value)
{
    return (value & mask) != 0;
}


// set 3 way flag: 0 - not set, -1 - false, 1 - true
template <UInt32 shift>
inline void G_Set3WayFlag(UInt32& pvalue, int state)
{
    pvalue = (pvalue & (~(3U<<shift))) | UInt32((state & 3) << shift);
}

template <UInt32 shift>
inline int G_Get3WayFlag(UInt32 value)
{
    UInt32 v = ((value >> shift) & 3);
    return (v == 3) ? -1 : int(v);
}

template <UInt32 shift>
inline bool G_Is3WayFlagTrue(UInt32 value)
{
    return G_Get3WayFlag<shift>(value) == 1;
}

template <UInt32 shift>
inline bool G_Is3WayFlagSet(UInt32 value)
{
    return G_Get3WayFlag<shift>(value) != 0;
}

template <UInt32 shift>
inline bool G_Is3WayFlagFalse(UInt32 value)
{
    return G_Get3WayFlag<shift>(value) != 1;
}


//
// ***** GFxMovieRoot
//
// Global, shared root state for a GFxMovieSub and all its characters.
//

class GFxMovieRoot : public GFxMovieView, public GFxActionPriority
{
    friend class GFxMovieView;

public:
    typedef GRenderer::Matrix Matrix;   

    // *** Complex object interface support

    // VM specific interface used to access complex objects (GASObject, etc.)
    GFxValue::ObjectInterface*  pObjectInterface;
    
    // Storage for wide char conversions (VT_ConvertStringW). Stored per GFxValue (if needed)
    struct WideStringStorage : public GRefCountBase<WideStringStorage, GFxStatMV_Other_Mem>
    {
        GASStringNode*  pNode;
        UByte           pData[1];

        WideStringStorage(GASStringNode* pnode, UPInt len) : pNode(pnode) 
        {
            pNode->AddRef();
            // NOTE: pData must be guaranteed to have enough space for (wchar_t * strLen+1)
            GUTF8Util::DecodeString((wchar_t*)pData, pNode->pData, len);
        }
        ~WideStringStorage()                { pNode->Release(); }
        GINLINE const wchar_t*  ToWStr()    { return (const wchar_t*)pData; }

    private:
        // Copying is prohibited
        WideStringStorage(const WideStringStorage&);
        const WideStringStorage& operator = (const WideStringStorage&);
    };

    struct LevelInfo
    {
        // Level must be >= 0. -1 is used to indicate lack of a level
        // elsewhere, but that is not allowed here.
        SInt            Level;
        GPtr<GFxSprite> pSprite;
    };

    // Create a movie instance.
    class MemoryContextImpl : public GFxMovieDef::MemoryContext
    {
    public:
        GMemoryHeap*                Heap;
#ifndef GFC_NO_GC
        GPtr<GASRefCountCollector>  ASGC;
#endif
        GPtr<GASStringManager>      StringManager;
        GPtr<GFxTextAllocator>      TextAllocator;

        struct HeapLimit : GMemoryHeap::LimitHandler
        {
            enum
            {
                INITIAL_DYNAMIC_LIMIT = (128*1024)
            };

            MemoryContextImpl*  MemContext;
            UPInt               UserLevelLimit;
            UPInt               LastCollectionFootprint;
            UPInt               CurrentLimit;
            Float               HeapLimitMultiplier;

            HeapLimit() :  MemContext(NULL), UserLevelLimit(0), LastCollectionFootprint(0), 
                CurrentLimit(0), HeapLimitMultiplier(0.25) {}

            void Collect(GMemoryHeap* heap);
            void Reset(GMemoryHeap* heap);

            virtual bool OnExceedLimit(GMemoryHeap* heap, UPInt overLimit);
            virtual void OnFreeSegment(GMemoryHeap* heap, UPInt freeingSize);
        } LimHandler;

        MemoryContextImpl() : Heap(NULL)
        {
            LimHandler.MemContext = this;
        }
        ~MemoryContextImpl()
        {
            Heap->SetLimitHandler(NULL);
        }
    };

    // Memory context for this view, for the case of multiple views sharing a heap
    GPtr<MemoryContextImpl> MemContext;

    // Keep track of the number of advances for shared GC
    UInt                    NumAdvancesSinceCollection;
    UInt                    LastCollectionFrame;

    // Heap used for all allocations within GFxMovieRoot.
    GMemoryHeap*            pHeap;

    // Sorted array of currently loaded movie levels. Level 0 has
    // special significance, since it dictates viewport scale, etc. 
    GArrayLH<LevelInfo, GFxStatMV_Other_Mem>  MovieLevels;
    // Pointer to sprite in level0 or null. Stored for convenient+efficient access.
    GFxSprite*              pLevel0Movie;
    // Convenience pointer to _level0's Def. Keep this around even if _level0 unloads,
    // to avoid external crashes.
    GPtr<GFxMovieDefImpl>   pLevel0Def;

    // A list of root MovieDefImpl objects used by all root sprites; these objects
    // can be loaded progressively in the background.
    GList<GFxMovieDefRootNode> RootMovieDefNodes;

    GViewport               Viewport;
    Float                   PixelScale;
    // View scale values, used to adjust input mouse coordinates
    // map viewport -> movie coordinates.
    Float                   ViewScaleX, ViewScaleY;
    Float                   ViewOffsetX, ViewOffsetY; // in stage pixels (not viewport's ones)
    ScaleModeType           ViewScaleMode;
    AlignType               ViewAlignment;
    GRectF                  VisibleFrameRect; // rect, in swf coords (twips), visible in curr viewport
    GRectF                  SafeRect;         // storage for user-define safe rect
    Matrix                  ViewportMatrix; 
#ifndef GFC_NO_3D
    GMatrix3DNewable       *pPerspectiveMatrix3D;   // 3D perspective matrix
    GMatrix3DNewable       *pViewMatrix3D;          // 3D view matrix
    GScreenToWorld          ScreenToWorld;
    Float                   PerspFOV;
#endif
    GPtr<GFxStateBagImpl>   pStateBag;

#ifndef GFC_NO_VIDEO
    GHashSet<GPtr<GFxVideoProvider> > VideoProviders;
#endif

    // *** States cached in Advance()

    mutable GPtr<GFxLog>        pCachedLog;    

    // Handler pointer cached during advance.
    GPtr<GFxUserEventHandler>   pUserEventHandler;
    GPtr<GFxFSCommandHandler>   pFSCommandHandler;
    GPtr<GFxExternalInterface>  pExtIntfHandler;
    GPtr<GFxRenderConfig>       pRenderConfig;
    
    GPtr<GFxFontManagerStates>  pFontManagerStates;

#ifndef GFC_NO_SOUND
    // We don't keep them here as GPtr<> to avoid dependances from GSoundRenderer
    // in another parts of GFx (like IME). We hold these pointer here only for caching propose
    GFxAudioBase*               pAudio;
    GSoundRenderer*             pSoundRenderer;
#endif
    
    // *** Special reference for XML object manager
    GFxExternalLibPtr* pXMLObjectManager;

    
    // Obtains cached states. The mat is only accessed if CachedStatesFlag is not
    // set, which meas we are outside of Advance and Display.
    GFxLog*                 GetCachedLog() const
    {
         // Do not modify CachedLogFlag; that is only for Advance/Display.
        if (!G_IsFlagSet<Flag_CachedLogFlag>(Flags))
            pCachedLog = GetLog();
        return pCachedLog;
    }
    
    // Amount of time that has elapsed since start of playback. Used for
    // reporting in action instruction and setTimeout/setInterval. In microseconds.
    UInt64                  TimeElapsed;
    // Time remainder from previous advance, updated every frame.
    Float                   TimeRemainder;
    // Cached seconds per frame; i.e., 1.0f / FrameRate.
    Float                   FrameTime;

    UInt                    ForceFrameCatchUp;

    GFxInputEventsQueue     InputEventsQueue;

    GColor                  BackgroundColor;
    GFxMouseState           MouseState[GFC_MAX_MICE_SUPPORTED];
    UInt                    MouseCursorCount;
    UInt                    ControllerCount;
    void*                   UserData;
   
    const GFxASMouseListener*     pASMouseListener; // a listener for AS Mouse class. Only one is necessary.
    
#ifndef GFC_NO_KEYBOARD_SUPPORT
    // Keyboard
    GFxKeyboardState        KeyboardState[GFC_MAX_KEYBOARD_SUPPORTED];

    const GFxKeyboardState* GetKeyboardState(UInt keyboardIndex) const 
    { 
        if (keyboardIndex < GFC_MAX_KEYBOARD_SUPPORTED)
            return &KeyboardState[keyboardIndex]; 
        return NULL;
    }
    GFxKeyboardState* GetKeyboardState(UInt keyboardIndex) 
    { 
        if (keyboardIndex < GFC_MAX_KEYBOARD_SUPPORTED)
            return &KeyboardState[keyboardIndex]; 
        return NULL;
    }
    void SetKeyboardListener(GFxKeyboardState::IListener*);
#endif

    // Global Action Script state
    GPtr<GASGlobalContext>  pGlobalContext;

    // Return value class - allocated after global context.
    // Used because GASString has no global or GNewOverrideBase.
    struct ReturnValueHolder : public GNewOverrideBase<GFxStatMV_ActionScript_Mem>
    {
        char*     CharBuffer;
        UInt      CharBufferSize;
        GArrayCC<GASString, GFxStatMV_ActionScript_Mem>   StringArray;
        UInt      StringArrayPos;

        ReturnValueHolder(GASGlobalContext* pmgr)
            : CharBuffer(0), CharBufferSize(0),
            StringArray(pmgr->GetBuiltin(GASBuiltin_empty_)),
            StringArrayPos(0) { }
        ~ReturnValueHolder() { if (CharBuffer) GFREE(CharBuffer); }

        GINLINE char* PreAllocateBuffer(UInt size)
        {
            size = (size + 4095)&(~(4095));
            if (CharBufferSize < size || (CharBufferSize > size && (CharBufferSize - size) > 4096))
            {
                if (CharBuffer)
                    CharBuffer = (char*)GREALLOC(CharBuffer, size, GFxStatMV_ActionScript_Mem);
                else
                    CharBuffer = (char*)GALLOC(size, GFxStatMV_ActionScript_Mem);
                CharBufferSize = size;
            }
            return CharBuffer;
        }
        GINLINE void ResetPos() { StringArrayPos = 0; }
        GINLINE void ResizeStringArray(UInt n)
        {
            StringArray.Resize(G_Max(1u,n));
        }
    };

    ReturnValueHolder*      pRetValHolder;

    // Return value storage for ExternalInterface.call.
    GASValue                ExternalIntfRetVal;
    struct InvokeAliasInfo // aliases set by ExtIntf.addCallback
    {
        GPtr<GASObject>             ThisObject;
        GPtr<GFxCharacterHandle>    ThisChar;
        GASFunctionRef              Function;
    };
    GASStringHash<InvokeAliasInfo>* pInvokeAliases; // aliases set by ExtIntf.addCallback


    // Instance name assignment counter.
    UInt32                  InstanceNameCount;

    
    class DragState
    {
    public:
        GFxASCharacter* pCharacter;
        bool            LockCenter;
        bool            Bound;
        // Bound coordinates
        GPointF         BoundLT;
        GPointF         BoundRB;
        // The difference between character origin and mouse location
        // at the time of dragStart, used and computed if LockCenter == 0.
        GPointF         CenterDelta;

        DragState()
            : pCharacter(0), LockCenter(0), Bound(0), 
              BoundLT(0.0), BoundRB(0.0), CenterDelta(0.0)
            { }

        // Initializes lockCenter and mouse centering delta
        // based on the character.
        void InitCenterDelta(bool lockCenter);
    };

    DragState               CurrentDragState;   // @@ fold this into GFxMouseButtonState?


    // Sticky variable hash link node.
    struct StickyVarNode : public GNewOverrideBase<GFxStatMV_ActionScript_Mem>
    {
        GASString       Name;
        GASValue        Value;
        StickyVarNode*  pNext;
        bool            Permanent;

        StickyVarNode(const GASString& name, const GASValue &value, bool permanent)
            : Name(name), Value(value), pNext(0), Permanent(permanent) { }
        StickyVarNode(const StickyVarNode &node)
            : Name(node.Name), Value(node.Value), pNext(node.pNext), Permanent(node.Permanent) { }
        const StickyVarNode& operator = (const StickyVarNode &node)
            { pNext = node.pNext; Name = node.Name; Value = node.Value; Permanent = node.Permanent; return *this; }
    };

    // Sticky variable clip hash table.
    GASStringHash<StickyVarNode*>  StickyVariables;



    // *** Action Script execution

    // Action queue is stored as a singly linked list queue. List nodes must be traversed
    // in order for execution. New actions are inserted at the insert location, which is
    // commonly the end; however, in some cases insert location can be modified to allow
    // insertion of items before other items.


    // Action list to be executed 
    struct ActionEntry : public GNewOverrideBase<GFxStatMV_ActionScript_Mem>
    {
        enum EntryType
        {
            Entry_None,
            Entry_Buffer,   // Execute pActionBuffer(pSprite env)
            Entry_Event,        // 
            Entry_Function,
            Entry_CFunction
        };

        ActionEntry*        pNextEntry;
        EntryType           Type;
        GPtr<GFxASCharacter>  pCharacter;
        GPtr<GASActionBuffer> pActionBuffer;
        GFxEventId          EventId;
        GASFunctionRef      Function;
        GASCFunctionPtr     CFunction;
        GASValueArray       FunctionParams;

        UInt                SessionId;

        GINLINE ActionEntry();
        GINLINE ActionEntry(const ActionEntry &src);
        GINLINE const ActionEntry& operator = (const ActionEntry &src);

        // Helper constructors
        GINLINE ActionEntry(GFxASCharacter *pcharacter, GASActionBuffer* pbuffer);
        GINLINE ActionEntry(GFxASCharacter *pcharacter, const GFxEventId id);
        GINLINE ActionEntry(GFxASCharacter *pcharacter, const GASFunctionRef& function, const GASValueArray* params = 0);
        GINLINE ActionEntry(GFxASCharacter *pcharacter, const GASCFunctionPtr function, const GASValueArray* params = 0);

        GINLINE void SetAction(GFxASCharacter *pcharacter, GASActionBuffer* pbuffer);
        GINLINE void SetAction(GFxASCharacter *pcharacter, const GFxEventId id);
        GINLINE void SetAction(GFxASCharacter *pcharacter, const GASFunctionRef& function, const GASValueArray* params = 0);
        GINLINE void SetAction(GFxASCharacter *pcharacter, const GASCFunctionPtr function, const GASValueArray* params = 0);
        GINLINE void ClearAction();

        // Executes actions in this entry
        void    Execute(GFxMovieRoot *proot) const;

        bool operator==(const ActionEntry&) const;
    };

  
    struct ActionQueueEntry
    {
        // This is a root of an action list. Root node action is 
        // always Entry_None and thus does not have to be executed.
        ActionEntry*                pActionRoot;

        // This is an action insert location. In a beginning, &ActionRoot, afterwards
        // points to the node after which new actions are added.
        ActionEntry*                pInsertEntry;

        // Pointer to a last entry
        ActionEntry*                pLastEntry;

        ActionQueueEntry() { Reset(); }
        inline void Reset() { pActionRoot = pLastEntry = pInsertEntry = NULL; }
    };
    struct ActionQueueType
    {
        ActionQueueEntry            Entries[AP_Count];
        // This is a modification id to track changes in queue during its execution
        int                         ModId;
        // This is a linked list of un-allocated entries.   
        ActionEntry*                pFreeEntry;
        UInt                        CurrentSessionId;
        UInt                        FreeEntriesCount;
        UInt                        LastSessionId;

        GMemoryHeap*                pHeap;

        ActionQueueType(GMemoryHeap* pheap);
        ~ActionQueueType();

        GMemoryHeap*                GetMovieHeap() const { return pHeap; }

        ActionQueueEntry& operator[](UInt i) { GASSERT(i < AP_Count); return Entries[i]; }
        const ActionQueueEntry& operator[](UInt i) const { GASSERT(i < AP_Count); return Entries[i]; }

        ActionEntry*                InsertEntry(Priority prio);
        void                        AddToFreeList(ActionEntry*);
        void                        Clear();
        ActionEntry*                GetInsertEntry(Priority prio)
        {
            return Entries[prio].pInsertEntry;
        }
        ActionEntry*                SetInsertEntry(Priority prio, ActionEntry* pinsertEntry)
        {
            ActionEntry* pie = Entries[prio].pInsertEntry;
            Entries[prio].pInsertEntry = pinsertEntry;
            return pie;
        }
        ActionEntry*                FindEntry(Priority prio, const ActionEntry&);

        UInt                        StartNewSession(UInt* pprevSessionId);
        void                        RestoreSession(UInt sId)  { CurrentSessionId = sId; }
    };
    struct ActionQueueIterator
    {
        int                         ModId;
        ActionQueueType*            pActionQueue;
        ActionEntry*                pLastEntry;
        int                         CurrentPrio;

        ActionQueueIterator(ActionQueueType*);
        ~ActionQueueIterator();

        const ActionEntry*          getNext();
    };
    struct ActionQueueSessionIterator : public ActionQueueIterator
    {
        UInt                        SessionId;

        ActionQueueSessionIterator(ActionQueueType*, UInt sessionId);

        const ActionEntry*          getNext();
    };
    ActionQueueType                     ActionQueue;

    GArray<GFxASCharacter*>             PreDisplayList; // used for filters

    enum FlagsType
    {
        // Set once the viewport has been specified explicitly.
        Flag_ViewportSet                    = 0x0001,

        // States are cached then this flag is set.
        Flag_CachedLogFlag                  = 0x0002,

        // Verbosity - assigned from GFxActionControl.
        Flag_VerboseAction                  = 0x0004,
        Flag_LogRootFilenames               = 0x0008,
        Flag_LogChildFilenames              = 0x0010,
        Flag_LogLongFilenames               = 0x0020,
        Flag_SuppressActionErrors           = 0x0040,    

        Flag_NeedMouseUpdate                = 0x0080,

        Flag_LevelClipsChanged              = 0x0100,
        // Set if Advance has not been called yet - generates warning on Display.
        Flag_AdvanceCalled                  = 0x0200,
        Flag_DirtyFlag                      = 0x0400,
        Flag_NoInvisibleAdvanceFlag         = 0x0800,
        Flag_SetCursorTypeFuncOverloaded    = 0x1000,

        // Flags for event handlers
        Flag_OnEventXmlsocketOndataCalled   = 0x2000,
        Flag_OnEventXmlsocketOnxmlCalled    = 0x4000,
        Flag_OnEventLoadProgressCalled      = 0x8000,

//        Flag_IsShowingRect                  = 0x010000,

        Flag_BackgroundSetByTag             = 0x020000,
        Flag_MovieIsFocused                 = 0x040000,
        Flag_OptimizedAdvanceListInvalid    = 0x080000,

        Flag_Paused                         = 0x100000,
        Flag_ContinueAnimation              = 0x200000,

        // Focus-related AS extension properties
        // Disables focus release when mouse moves
        Shift_DisableFocusAutoRelease       = 22, // 0xC00000

        // Enables moving focus by arrow keys even if _focusRect is set to false
        Shift_AlwaysEnableFocusArrowKeys    = 24, // 0x3000000

        // Enables firing onPress/onRelease even if _focusRect is set to false
        Shift_AlwaysEnableKeyboardPress     = 26, // 0xC000000

        // Disables firing onRollOver/Out if focus is changed by arrow keys
        Shift_DisableFocusRolloverEvent     = 28, // 0x30000000

        // Disables default focus handling by arrow and tab keys
        Shift_DisableFocusKeys              = 30  // 0xC0000000
    };
    UInt32 Flags;

    GArrayLH<GPtr<GFxASCharacter>, GFxStatMV_Other_Mem> TopmostLevelCharacters;

    GArrayLH<GPtr<GFxSprite>, GFxStatMV_Other_Mem>    SpritesWithHitArea;
    UInt64                              StartTickMs, PauseTickMs;

    // interval timer stuff
    GArrayLH<GASIntervalTimer *,GFxStatMV_Other_Mem> IntervalTimers;
    int                                 LastIntervalTimerId;

    // Focus management stuff
    struct FocusGroupDescr
    {
        enum
        {
            TabableArray_Initialized        = 0x1,
            TabableArray_WithFocusEnabled   = 0x2
        };
        UInt8                               TabableArrayStatus;
        GArrayDH<GPtr<GFxASCharacter>, GFxStatMV_Other_Mem> TabableArray; 
        mutable GWeakPtr<GFxASCharacter>    LastFocused;
        GPtr<GFxCharacterHandle>            ModalClip;
        short                               LastFocusKeyCode;
        GRectF                              LastFocusedRect;
        bool                                FocusRectShown;

        FocusGroupDescr(GMemoryHeap* heap = NULL):
        TabableArrayStatus(0), TabableArray((!heap) ? GMemory::GetHeapByAddress(GetThis()): heap), 
            LastFocusKeyCode(0), FocusRectShown(false) {}

        bool                IsFocused(const GFxASCharacter* ch) const 
        { 
            GPtr<GFxASCharacter> lch = LastFocused; return lch.GetPtr() == ch; 
        }
        void                ResetFocus() { LastFocused = NULL; }
        void                ResetFocusDirection() { LastFocusKeyCode = 0; }
        void                ResetTabableArray()
        {
            if (TabableArrayStatus & TabableArray_Initialized)
            {
                TabableArray.Resize(0);
                TabableArrayStatus = 0;
            }
        }
        GFxSprite* GetModalClip(GFxMovieRoot* proot);

    private:
        FocusGroupDescr* GetThis() { return this; }
    } FocusGroups[GFX_MAX_CONTROLLERS_SUPPORTED];
    UInt                                    FocusGroupsCnt;
    // Map controller index to focus group
    UInt8                                   FocusGroupIndexes[GFX_MAX_CONTROLLERS_SUPPORTED];

    // The head of the playlist.
    // Playlist head, the first child for events or frame actions execution.
    // "Playable" children characters (ASCharacters) are linked by GFxASCharacter::pPlayNext
    // and GFxASCharacter::pPlayPrev pointers. If pPlayPrev == NULL, then this is the first
    // element in playlist, if pPlayNext == NULL - the last one.
    GFxASCharacter*                     pPlayListHead;
    GFxASCharacter*                     pPlayListOptHead;
    // List of all unloaded but not destroyed yet characters.
    GFxASCharacter*                     pUnloadListHead;

    GFxIMECandidateListStyle*   pIMECandidateListStyle; // stored candidate list style
#if defined(GFC_OS_WIN32) && !defined(GFC_NO_BUILTIN_KOREAN_IME) && !defined(GFC_NO_IME_SUPPORT)
    GFxIMEImm32Dll      Imm32Dll;
#endif

    // *** Constructor / Destructor

    GFxMovieRoot(MemoryContextImpl* memContext);
    ~GFxMovieRoot();

    static GFxMovieRoot* Create(GFxMovieDef::MemoryContext* memContext)
    {
        MemoryContextImpl* memContextImpl = static_cast<MemoryContextImpl*>(memContext);

        return GHEAP_NEW(memContextImpl->Heap) GFxMovieRoot(memContextImpl);
    }


    // Non-virtual version of GetHeap(), used for efficiency.
    GMemoryHeap*        GetMovieHeap() const { return pHeap; }

    // Register AS classes defined in aux libraries
    void                RegisterAuxASClasses();
        
    // Sets a movie at level; used for initialization.
    bool                SetLevelMovie(SInt level, GFxSprite *psprite);
    // Returns a movie at level, or null if no such movie exists.
    bool                ReleaseLevelMovie(SInt level);
    // Returns a movie at level, or null if no such movie exists.
    GFxSprite*          GetLevelMovie(SInt level) const;
    
    // Finds a character given a global path. Path must 
    // include a _levelN entry as first component.
    GFxASCharacter*     FindTarget(const GASString& path) const;

    // Helper: parses _levelN tag and returns the end of it.
    // Returns level index, of -1 if there is no match.
    static SInt         ParseLevelName(const char* pname, const char **ptail, bool caseSensitive);

    
    // Dragging support.
    void                SetDragState(const DragState& st)   { CurrentDragState = st; }
    void                GetDragState(DragState* st)         { *st = CurrentDragState; }
    void                StopDrag()                          { CurrentDragState.pCharacter = NULL; }
    bool                IsDraggingCharacter(const GFxASCharacter* ch) const { return CurrentDragState.pCharacter == ch; }


    // Internal use in characters, etc.
    // Use this to retrieve the last state of the mouse.
    virtual void        GetMouseState(UInt mouseIndex, Float* x, Float* y, UInt* buttons);

    const GFxMouseState* GetMouseState(UInt mouseIndex) const
    {
        if (mouseIndex >= GFC_MAX_MICE_SUPPORTED)
            return NULL;
        return &MouseState[mouseIndex];
    }
    void                SetMouseCursorCount(UInt n)
    {
        MouseCursorCount = (n <= GFC_MAX_MICE_SUPPORTED) ? n : GFC_MAX_MICE_SUPPORTED;
    }
    UInt                GetMouseCursorCount() const
    {
        return MouseCursorCount;
    }
    virtual void        SetControllerCount(UInt n)
    {
        ControllerCount = (n <= GFC_MAX_KEYBOARD_SUPPORTED) ? n : GFC_MAX_KEYBOARD_SUPPORTED;
    }
    virtual UInt        GetControllerCount() const
    {
        return ControllerCount;
    }
    bool                IsMouseSupportEnabled() const { return MouseCursorCount > 0; }
    
    // Return the size of a logical GFxMovieSub pixel as
    // displayed on-screen, with the current device
    // coordinates.
    Float               GetPixelScale() const               { return PixelScale; }

    // Returns time elapsed since the first Advance, in seconds (with fraction part)
    Double              GetTimeElapsed() const              { return Double(TimeElapsed)/1000; }
    // Returns time elapsed since the first Advance, in milliseconds
    UInt64              GetTimeElapsedMs() const            { return TimeElapsed; }

    Float               GetFrameTime() const                { return FrameTime; }
    UInt64              GetStartTickMs() const              { return StartTickMs; }
    UInt64              GetASTimerMs() const;
    UInt32              GetNumHiddenMovies() const;
    UInt32              GetNumAlpha0Movies() const;
    UInt32              GetNumAnimatingHiddenMovies() const;
    UInt32              GetNestedDepth() const;
    UInt32              GetNumNestedObjects() const;

    
    // Create new instance names for unnamed objects.
    GASString           CreateNewInstanceName();

    // *** Load/Unload movie support
    
    // Head of load queue.
    GFxLoadQueueEntry*  pLoadQueueHead;
    UInt32              LastLoadQueueEntryCnt;

    // Adds load queue entry and takes ownership of it.
    void                AddLoadQueueEntry(GFxLoadQueueEntry *pentry);

    // Adds load queue entry based on parsed url and target path.
    void                AddLoadQueueEntry(const char* ptarget, const char* purl, GASEnvironment* env,
                                          GFxLoadQueueEntry::LoadMethod method = GFxLoadQueueEntry::LM_None,
                                          GASMovieClipLoader* pmovieClipLoader = NULL);
    void                AddLoadQueueEntry(GFxASCharacter* ptarget, const char* purl,
                                          GFxLoadQueueEntry::LoadMethod method = GFxLoadQueueEntry::LM_None,
                                          GASMovieClipLoader* pmovieClipLoader = NULL);
    // Load queue entries for loading variables
    void                AddVarLoadQueueEntry(const char* ptarget, const char* purl,
                                          GFxLoadQueueEntry::LoadMethod method = GFxLoadQueueEntry::LM_None);
    void                AddVarLoadQueueEntry(GFxASCharacter* ptarget, const char* purl,
                                          GFxLoadQueueEntry::LoadMethod method = GFxLoadQueueEntry::LM_None);
#ifndef GFC_NO_FXPLAYER_AS_LOADVARS
    void                AddVarLoadQueueEntry(GASLoadVarsObject* ploadVars, const char* purl,
                                          GFxLoadQueueEntry::LoadMethod method = GFxLoadQueueEntry::LM_None);
    void                ProcessLoadVars(GFxLoadQueueEntry *pentry, GFxLoadStates* pls, const GFxString& level0Path);
    // called from ProcessLoadVars should be private
    void                LoadVars(GFxLoadQueueEntry *pentry, GFxLoadStates* pls, const GFxString& data, SInt fileLen);
    // called from LoadVars should be private
    GFxSprite*          CreateEmptySprite(GFxLoadStates* pls, SInt level);
#endif //#ifndef GFC_NO_FXPLAYER_AS_LOADVARS

#ifndef GFC_NO_XML_SUPPORT
    void                AddXmlLoadQueueEntry(GASObject* pxmlobj, GFxASXMLFileLoader* pxmlLoader, 
                                          const char* purl,
                                          GFxLoadQueueEntry::LoadMethod method = GFxLoadQueueEntry::LM_None);
#endif

#ifndef GFC_NO_CSS_SUPPORT
    void                AddCssLoadQueueEntry(GASObject* pobj, GFxASCSSFileLoader* pLoader, const char* purl,
                                             GFxLoadQueueEntry::LoadMethod method = GFxLoadQueueEntry::LM_None);
#endif

    // Processes the load queue handling load/unload instructions.  
    void                ProcessLoadQueue();

#ifndef GFC_NO_XML_SUPPORT
    void                ProcessLoadXML(GFxLoadQueueEntry *pentry, GFxLoadStates* pls, const GString& level0Path);
#endif

#ifndef GFC_NO_CSS_SUPPORT
    void                ProcessLoadCSS(GFxLoadQueueEntry *pentry, GFxLoadStates* pls, const GString& level0Path);
#endif

    void                ProcessLoadMovieClip(GFxLoadQueueEntry *pentry, GFxLoadStates* pls, const GString& level0Path);

    GFxLoadQueueEntryMT* pLoadQueueMTHead;
    void                AddLoadQueueEntryMT(GFxLoadQueueEntry *pqueueEntry);
    void                AddMovieLoadQueueEntry(GFxLoadQueueEntry* pentry);


    // *** Helpers for loading images.

    typedef GFxLoader::FileFormatType FileFormatType;
        
    // Create a loaded image MovieDef based on image resource.
    // If load states are specified, they are used for bind states. Otherwise,
    // new states are created based on pStateBag and our loader.
    GFxMovieDefImpl*  CreateImageMovieDef(GFxImageResource *pimageResource, bool bilinear,
                                          const char *purl, GFxLoadStates *pls = 0);

    // Fills in a file system path relative to _level0. The path
    // will contain a trailing '/' if not empty, so that relative
    // paths can be concatenated directly to it.
    // Returns 1 if the path is non-empty.
    bool                GetLevel0Path(GString *ppath) const;
    

    // *** Action List management

    // Take care of this frame's actions, by executing ActionList. 
    void                DoActions();
    // Execute only actions from the certain session; all other actions are staying in the queue
    void                DoActionsForSession(UInt sessionId); 

    // Inserts an empty action and returns a pointer to it. Should call SetAction afterwards.   
    ActionEntry*        InsertEmptyAction(Priority prio = AP_Frame) { return ActionQueue.InsertEntry(prio); }
    bool                HasActionEntry(const ActionEntry& entry,Priority prio = AP_Frame)       
        { return ActionQueue.FindEntry(prio, entry) != NULL; }

   
    // *** GFxMovieView implementation

    virtual void        SetViewport(const GViewport& viewDesc);
    virtual void        GetViewport(GViewport *pviewDesc) const;
    virtual void        SetViewScaleMode(ScaleModeType);
    virtual ScaleModeType   GetViewScaleMode() const                        { return ViewScaleMode; }
    virtual void        SetViewAlignment(AlignType);
    virtual AlignType   GetViewAlignment() const                            { return ViewAlignment; }
    virtual GRectF      GetVisibleFrameRect() const                         { return TwipsToPixels(VisibleFrameRect); }
    const GRectF&       GetVisibleFrameRectInTwips() const                  { return VisibleFrameRect; }

    virtual void        SetPerspective3D(const GMatrix3D &projMatIn);
    virtual void        SetPerspectiveFOV(Float fov);
    virtual void        SetView3D(const GMatrix3D &viewMatIn);

    virtual GRectF      GetSafeRect() const                                 { return SafeRect; }
    virtual void        SetSafeRect(const GRectF& rect)                     { SafeRect = rect; }

    void                UpdateViewport();
/*
#ifndef GFC_NO_SOUND
    virtual void           SetSoundPlayer(GSoundRenderer* ps)                 { pSoundPlayer = ps; }
    virtual GSoundRenderer*  GetSoundPlayer() const                           { return pSoundPlayer; }
#else
    virtual void           SetSoundPlayer(GSoundRenderer* ps)                 { }
    virtual GSoundRenderer*  GetSoundPlayer() const                           { return 0; }
#endif
*/
    virtual void        SetVerboseAction(bool verboseAction)                
    { 
#ifdef GFC_NO_FXPLAYER_VERBOSE_ACTION
        GFC_DEBUG_WARNING(1, "VerboseAction is disabled by the GFC_NO_FXPLAYER_VERBOSE_ACTION macro in GConfig.h\n");
#endif
        G_SetFlag<Flag_VerboseAction>(Flags, verboseAction); 
    }
    // Turn off/on log for ActionScript errors..
    virtual void        SetActionErrorsSuppress(bool suppressActionErrors)  { G_SetFlag<Flag_SuppressActionErrors>(Flags, suppressActionErrors); }
    // Background color.
    virtual void        SetBackgroundColor(const GColor color)      { BackgroundColor = color; }
#ifdef GFC_OS_PS2
    virtual void        SetBackgroundAlpha(Float alpha)             { BackgroundColor.SetAlpha( G_Clamp<UInt>((UInt)(alpha*255.0f), 0, 255) ); }
#else
    virtual void        SetBackgroundAlpha(Float alpha)             { BackgroundColor.SetAlpha( G_Clamp<UByte>((UByte)(alpha*255.0f), 0, 255) ); }
#endif
    virtual Float       GetBackgroundAlpha() const                  { return BackgroundColor.GetAlpha() / 255.0f; }

    bool                IsBackgroundSetByTag() const                { return G_IsFlagSet<Flag_BackgroundSetByTag>(Flags); }
    void                SetBackgroundColorByTag(const GColor color) 
    { 
        SetBackgroundColor(color); 
        G_SetFlag<Flag_BackgroundSetByTag>(Flags, true); 
    }



    // Actual execution and timeline control.
    virtual Float       Advance(Float deltaT, UInt frameCatchUpCount);
    // An internal method that advances frames for movieroot's sprites
    void                AdvanceFrame(bool nextFrame, Float framePos);
    // Releases list of unloaded characters
    void                ReleaseUnloadList();
    void                InvalidateOptAdvanceList() { G_SetFlag<Flag_OptimizedAdvanceListInvalid>(Flags, true); }
    bool                IsOptAdvanceListInvalid() const { return G_IsFlagSet<Flag_OptimizedAdvanceListInvalid>(Flags); }
    void                ProcessUnloadQueue();

    // Events.
    virtual UInt        HandleEvent(const GFxEvent &event);
    virtual void        NotifyMouseState(Float x, Float y, UInt buttons, UInt mouseIndex = 0);
    virtual bool        HitTest(Float x, Float y, HitTestType testCond = HitTest_Shapes, UInt controllerIdx = 0);
    virtual bool        HitTest3D(GPoint3F *ptout, Float x, Float y, UInt controllerIdx = 0);

    /*
    virtual void        SetUserEventHandler(UserEventCallback phandler, void* puserData) 
    { 
        pUserEventHandler = phandler; 
        pUserEventHandlerData = puserData;
    }

    // FSCommand
    virtual void        SetFSCommandCallback (FSCommandCallback phandler)   { pFSCommandCallback = phandler; }
    */

    virtual void*       GetUserData() const                                 { return UserData; }
    virtual void        SetUserData(void* ud)                               { UserData = ud;  }

    virtual bool        AttachDisplayCallback(const char* pathToObject, void (GCDECL *callback)(void* userPtr), void* userPtr);

    virtual void        SetExternalInterfaceRetVal(const GFxValue&);

    void                AddInvokeAlias(const GASString& alias, 
                                       GFxCharacterHandle* pthisChar,
                                       GASObject* pthisObj, 
                                       const GASFunctionRef& func);
    // returns timerId to use with ClearIntervalTimer
    int                 AddIntervalTimer(GASIntervalTimer *timer);  
    void                ClearIntervalTimer(int timerId);      
    void                ShutdownTimers();

#ifndef GFC_NO_VIDEO
    void                AddVideoProvider(GFxVideoProvider*);
    void                RemoveVideoProvider(GFxVideoProvider*);
#endif
    
    // *** GFxMovie implementation

    // Many of these methods delegate to pMovie methods of the same name; however,
    // they are hidden into the .cpp file because GFxSprite is not yet defined.
    
    virtual GFxMovieDef* GetMovieDef() const;
    
    virtual UInt        GetCurrentFrame() const;
    virtual bool        HasLooped() const;  
    virtual void        Restart();  
    virtual void        GotoFrame(UInt targetFrameNumber);
    virtual bool        GotoLabeledFrame(const char* label, SInt offset = 0);

    virtual void        Display();
    virtual void        SetPause(bool pause);
    virtual bool        IsPaused() const;

    virtual void        SetPlayState(PlayState s);
    virtual PlayState   GetPlayState() const;
    virtual void        SetVisible(bool visible);
    virtual bool        GetVisible() const;
    
    // Action Script access
    virtual void        CreateString(GFxValue* pvalue, const char* pstring);
    virtual void        CreateStringW(GFxValue* pvalue, const wchar_t* pstring);
    virtual void        CreateObject(GFxValue* pvalue, const char* className = NULL, const GFxValue* pargs = NULL, UInt nargs = 0);
    virtual void        CreateArray(GFxValue* pvalue);
    virtual void        CreateFunction(GFxValue* pvalue, GFxFunctionHandler* pfc, void* puserData = NULL);

    virtual bool        SetVariable(const char* ppathToVar, const GFxValue& value, SetVarType setType = SV_Sticky);
    virtual bool        GetVariable(GFxValue *pval, const char* ppathToVar) const;
    virtual bool        SetVariableArray(SetArrayType type, const char* ppathToVar,
                                         UInt index, const void* pdata, UInt count, SetVarType setType = SV_Sticky);
    virtual bool        SetVariableArraySize(const char* ppathToVar, UInt count, SetVarType setType = SV_Sticky);
    virtual UInt        GetVariableArraySize(const char* ppathToVar);
    virtual bool        GetVariableArray(SetArrayType type, const char* ppathToVar,
                                         UInt index, void* pdata, UInt count);

    virtual bool        IsAvailable(const char* ppathToVar) const;
    
    virtual bool        Invoke(const char* pmethodName, GFxValue *presult, const GFxValue* pargs, UInt numArgs);
    virtual bool        Invoke(const char* pmethodName, GFxValue *presult, const char* pargFmt, ...);
    virtual bool        InvokeArgs(const char* pmethodName, GFxValue *presult, const char* pargFmt, va_list args);

    bool                Invoke(GFxSprite* thisSpr, const char* pmethodName, GFxValue *presult, const GFxValue* pargs, UInt numArgs);

    // Implementation used by SetVariable/GetVariable.
    //bool                SetVariable(const char* pathToVar, const GASValue &val, SetVarType setType);
    void                GFxValue2ASValue(const GFxValue& gfxVal, GASValue* pdestVal) const;
    void                ASValue2GFxValue(GASEnvironment* penv, const GASValue& value, GFxValue* pdestVal) const;

    void                AddStickyVariable(const GASString& fullPath, const GASValue &val, SetVarType setType);
    void                ResolveStickyVariables(GFxASCharacter *pcharacter);
    void                ClearStickyVariables();

    GFxASCharacter*     GetTopMostEntity(
        const GPointF& mousePos, UInt controllerIdx, bool testAll, const GFxASCharacter* ignoreMC = NULL);

    // Profiling
    GPtr<GFxAmpViewStats> AdvanceStats;
    GPtr<GFxAmpViewStats> DisplayStats;

    // Obtains statistics for the movie view.
    virtual void        GetStats(GStatBag* pbag, bool reset = true);

    virtual GMemoryHeap* GetHeap() const { return pHeap; }

    // Forces to run garbage collection, if it is enabled. Does nothing otherwise.
    virtual void        ForceCollectGarbage();

    // ***** GFxStateBag implementation
    
    virtual GFxStateBag* GetStateBagImpl() const   { return pStateBag.GetPtr(); }

    // Mark/unmark the movie as one that has "focus". This is user responsibility to set or clear
    // the movie focus. This is important for stuff like IME functions correctly.
    void                OnMovieFocus(bool set);
    virtual bool        IsMovieFocused() const { return G_IsFlagSet<Flag_MovieIsFocused>(Flags); }

    // Returns and optionally resets the "dirty flag" that indicates 
    // whether anything was changed on the stage (and need to be 
    // re-rendered) or not.
    virtual bool        GetDirtyFlag(bool doReset = true);
    void                SetDirtyFlag() { G_SetFlag<Flag_DirtyFlag>(Flags, true); }

    void                SetNoInvisibleAdvanceFlag(bool f) { G_SetFlag<Flag_NoInvisibleAdvanceFlag>(Flags, f); }
    void                ClearNoInvisibleAdvanceFlag()     { SetNoInvisibleAdvanceFlag(false); }
    bool                IsNoInvisibleAdvanceFlagSet() const { return G_IsFlagSet<Flag_NoInvisibleAdvanceFlag>(Flags); }

    void                SetContinueAnimationFlag(bool f) { G_SetFlag<Flag_ContinueAnimation>(Flags, f); }
    void                ClearContinueAnimationFlag()     { SetContinueAnimationFlag(false); }
    bool                IsContinueAnimationFlagSet() const { return G_IsFlagSet<Flag_ContinueAnimation>(Flags); }

    bool                IsVerboseAction() const        { return G_IsFlagSet<Flag_VerboseAction>(Flags); }
    bool                IsSuppressActionErrors() const { return G_IsFlagSet<Flag_SuppressActionErrors>(Flags); }
    bool                IsLogRootFilenames() const  { return G_IsFlagSet<Flag_LogRootFilenames>(Flags); }
    bool                IsLogChildFilenames() const { return G_IsFlagSet<Flag_LogChildFilenames>(Flags); }
    bool                IsLogLongFilenames() const  { return G_IsFlagSet<Flag_LogLongFilenames>(Flags); }
    bool                IsAlwaysEnableKeyboardPress() const; 
    bool                IsAlwaysEnableKeyboardPressSet() const;
    void                SetAlwaysEnableKeyboardPress(bool f);
    bool                IsDisableFocusRolloverEvent() const;
    bool                IsDisableFocusRolloverEventSet() const;
    void                SetDisableFocusRolloverEvent(bool f);
    bool                IsDisableFocusAutoRelease() const;
    bool                IsDisableFocusAutoReleaseSet() const;
    void                SetDisableFocusAutoRelease(bool f);
    bool                IsAlwaysEnableFocusArrowKeys() const;
    bool                IsAlwaysEnableFocusArrowKeysSet() const;
    void                SetAlwaysEnableFocusArrowKeys(bool f);
    bool                IsDisableFocusKeys() const;
    bool                IsDisableFocusKeysSet() const;
    void                SetDisableFocusKeys(bool f);

    // Focus related functionality
    struct ProcessFocusKeyInfo : public GNewOverrideBase<GFxStatMV_Other_Mem>
    {
        FocusGroupDescr*                    pFocusGroup;
        GPtr<GFxASCharacter>                CurFocused;
        int                                 CurFocusIdx;
        GRectF                              Prev_aRect;
        short                               PrevKeyCode;
        UInt8                               KeyboardIndex;
        bool                                ManualFocus;
        bool                                InclFocusEnabled;
        bool                                Initialized;

        ProcessFocusKeyInfo():pFocusGroup(NULL), CurFocusIdx(-1),PrevKeyCode(0),
            KeyboardIndex(0), ManualFocus(false),InclFocusEnabled(false),Initialized(false) {}
    };
    // Focus-related methods
    void                InitFocusKeyInfo(ProcessFocusKeyInfo* pfocusInfo, 
                                         const GFxInputEventsQueue::QueueEntry::KeyEntry& keyEntry, 
                                         bool inclFocusEnabled,
                                         FocusGroupDescr* pfocusGroup = NULL);
    // Process keyboard input for focus
    void                ProcessFocusKey(GFxEvent::EventType event, 
                                        const GFxInputEventsQueue::QueueEntry::KeyEntry& keyEntry,
                                        ProcessFocusKeyInfo* pfocusInfo);
    void                FinalizeProcessFocusKey(ProcessFocusKeyInfo* pfocusInfo);

    // Internal methods that processes keyboard and mouse input
    void                ProcessInput();
    void                ProcessKeyboard(GASStringContext* psc, 
                                        const GFxInputEventsQueue::QueueEntry* qe, 
                                        ProcessFocusKeyInfo* focusKeyInfo);
    void                ProcessMouse
        (GASEnvironment* penv, const GFxInputEventsQueue::QueueEntry* qe, UInt32* miceProceededMask);

#ifdef GFC_BUILD_LOGO
    GPtr<GImageInfo>    pDummyImage;
#endif

protected:
    void                    FillTabableArray(const ProcessFocusKeyInfo* pfocusInfo);
    FocusGroupDescr&        GetFocusGroup(UInt controllerIdx) 
    { 
        GASSERT(controllerIdx < GFX_MAX_CONTROLLERS_SUPPORTED &&
                FocusGroupIndexes[controllerIdx] < GFX_MAX_CONTROLLERS_SUPPORTED);
        return FocusGroups[FocusGroupIndexes[controllerIdx]]; 
    }
    const FocusGroupDescr&  GetFocusGroup(UInt controllerIdx) const 
    {    
        GASSERT(controllerIdx < GFX_MAX_CONTROLLERS_SUPPORTED &&
            FocusGroupIndexes[controllerIdx] < GFX_MAX_CONTROLLERS_SUPPORTED);
        return FocusGroups[FocusGroupIndexes[controllerIdx]]; 
    }

    void                ResetMouseState();
    void                ResetKeyboardState();
public:
    UInt                GetFocusGroupIndex(UInt controllerIdx) const
    {
        return FocusGroupIndexes[controllerIdx]; 
    }
    // returns a bit-mask where each bit represents a physical controller, 
    // associated with the specified focus group.
    UInt32              GetControllerMaskByFocusGroup(UInt focusGroupIndex);
    // Displays yellow rectangle around focused item
    void                DisplayFocusRect(const GFxDisplayContext& context);
    // Hides yellow focus rectangle. This may happen if mouse moved.
    void                HideFocusRect(UInt controllerIdx);
    // Returns the character currently with focus
    GPtr<GFxASCharacter> GetFocusedCharacter(UInt controllerIdx) 
    { 
        return GPtr<GFxASCharacter>(GetFocusGroup(controllerIdx).LastFocused); 
    }
    // Checks, is the specified item focused by the specified controller or not?
    bool                IsFocused(const GFxASCharacter* ch, UInt controllerIdx) const 
    { 
        return GetFocusGroup(controllerIdx).IsFocused(ch); 
    }
    // Checks, if the 'ch' focused by ANY controller.
    bool                IsFocused(const GFxASCharacter* ch) const;

    // Checks, is the specified item focused or not for keyboard input
    bool                IsKeyboardFocused(const GFxASCharacter* ch, UInt controllerIdx) const 
    { 
        return (IsFocused(ch, controllerIdx) && IsFocusRectShown(controllerIdx)); 
    }
    // Transfers focus to specified item. For movie clips and buttons the keyboard focus will not be set
    // to specified character, as well as focus rect will not be drawn.
    void                SetFocusTo(GFxASCharacter*, UInt controllerIdx);

    // sets LastFocused to NULL
    void                ResetFocus(UInt controllerIdx) { GetFocusGroup(controllerIdx).LastFocused = NULL; }
    void                ResetFocusForChar(GFxASCharacter* ch);
    
    // Queue up setting of focus. ptopMostCh is might be necessary for stuff like IME to 
    // determine its state; might be NULL, if not available.
    void                QueueSetFocusTo
        (GFxASCharacter* ch, GFxASCharacter* ptopMostCh, UInt controllerIdx, GFxFocusMovedType fmt);
    // Instantly transfers focus to specified item. For movie clips and buttons the keyboard focus will not be set
    // Instantly invokes pOldFocus->OnKillFocus, pNewFocus->OnSetFocus and Selection.bradcastMessage("onSetFocus").
    void                TransferFocus(GFxASCharacter* pNewFocus, UInt controllerIdx, GFxFocusMovedType fmt);
    // Transfers focus to specified item. The keyboard focus transfered as well, as well as focus rect 
    // will be drawn (unless it is disabled).
    void                SetKeyboardFocusTo(GFxASCharacter*, UInt controllerIdx);
    // Returns true, if yellow focus rect CAN be shown at the time of call. This method will
    // return true, even if _focusrect = false and it is a time to show the rectangle.
    inline bool         IsFocusRectShown(UInt controllerIdx) const 
    { 
        return GetFocusGroup(controllerIdx).FocusRectShown; 
    }
    void                ResetTabableArrays();
    void                ResetFocusStates();

    void                ActivateFocusCapture(UInt controllerIdx);
    // Sets modal movieclip. That means focus keys (TAB, arrow keys) will 
    // move focus only inside of this movieclip. To reset the modal clip set
    // to NULL.
    void                SetModalClip(GFxSprite* pmovie, UInt controllerIdx);
    GFxSprite*          GetModalClip(UInt controllerIdx);

    // associate a focus group with a controller.
    virtual bool        SetControllerFocusGroup(UInt controllerIdx, UInt logCtrlIdx);

    // returns focus group associated with a controller
    virtual UInt        GetControllerFocusGroup(UInt controllerIdx) const;

    UInt                GetFocusGroupCount() const { return FocusGroupsCnt; }

    void                AddTopmostLevelCharacter(GFxASCharacter*);
    void                RemoveTopmostLevelCharacter(GFxCharacter*);
    void                DisplayTopmostLevelCharacters(GFxDisplayContext &context) const;

    // Sets style of candidate list. Invokes OnCandidateListStyleChanged callback.
    void                SetIMECandidateListStyle(const GFxIMECandidateListStyle& st);
    // Gets style of candidate list
    void                GetIMECandidateListStyle(GFxIMECandidateListStyle* pst) const;

#if defined(GFC_OS_WIN32) && !defined(GFC_NO_BUILTIN_KOREAN_IME) && !defined(GFC_NO_IME_SUPPORT)
    // handle korean IME (core part)
    UInt                HandleKoreanIME(const GFxIMEEvent& imeEvent);
#endif // GFC_NO_BUILTIN_KOREAN_IME

    GFxTextAllocator*   GetTextAllocator();

    GASRefCountCollector*  GetASGC()
    {
#ifndef GFC_NO_GC
        return MemContext->ASGC;
#else
        return NULL;
#endif
    }
    void            AddToPreDisplayList(GFxASCharacter* pch);
    void            RemoveFromPreDisplayList(GFxASCharacter* pch);
    void            DisplayPrePass();

    // Translates the point or rectangle in Flash coordinates to screen 
    // (window) coordinates. These methods takes into account the world matrix
    // of root, the viewport matrix and the user matrix from the renderer. 
    // Source coordinates should be in root coordinate space, in pixels.
    virtual GPointF TranslateToScreen(const GPointF& p, GRenderer::Matrix userMatrix = GRenderer::Matrix::Identity);
    virtual GRectF  TranslateToScreen(const GRectF& p, GRenderer::Matrix userMatrix = GRenderer::Matrix::Identity);

    // Translates the point in the character's coordinate space to the point on screen (window).
    // pathToCharacter - path to a movieclip, textfield or button, i.e. "_root.hud.mc";
    // pt is in pixels, in coordinate space of the character, specified by the pathToCharacter
    // returning value is in pixels of screen.
    virtual bool    TranslateLocalToScreen(const char* pathToCharacter, 
                                   const GPointF& pt, 
                                   GPointF* presPt, 
                                   GRenderer::Matrix userMatrix = GRenderer::Matrix::Identity);
    // Changes the shape of the mouse cursor. If Mouse.setCursorType is overriden by ActionScript
    // when it will be invoked. This function should be used instead of GASMouseCtorFunc::SetCursorType
    // to provide callback in user's ActionScript.
    void    ChangeMouseCursorType(UInt mouseIdx, UInt newCursorType);

    GFxMovieDef::MemoryContext* GetMemoryContext() const
    {
        return MemContext;
    }

    // Release method for custom behavior
    void    Release();   

    // This function is similar to GFxMovieDefImpl::GetExportedResource but besides searching in local
    // it looks also in all imported moviedefs starting from level0 def.
    bool                FindExportedResource(GFxMovieDefImpl* localDef, GFxResourceBindData *pdata, const GString& symbol);
private:
    void                CheckMouseCursorType(UInt mouseIdx, GFxASCharacter* ptopMouseCharacter);

    InvokeAliasInfo*    ResolveInvokeAlias(const char* pstr) const;
    bool                InvokeAlias(const char* pmethodName, 
                                    const InvokeAliasInfo& alias, 
                                    GASValue *presult, 
                                    UInt numArgs);
    bool                InvokeAliasArgs(const char* pmethodName, 
                                        const InvokeAliasInfo& alias, 
                                        GASValue *presult, 
                                        const char* methodArgFmt, 
                                        va_list args);
#ifndef GFC_NO_3D
    void                Setup3DDisplay(GFxDisplayContext &context);         // set up 3D view and perspective matrices in the renderer
    void                Make3D();                                           // force the movie to be fully 3D
    void                UpdateViewAndPerspective();                         // update after viewport changes
#endif
};

// ** Inline Implementation


GINLINE GFxMovieRoot::ActionEntry::ActionEntry()
{ 
    pNextEntry = 0;
    Type = Entry_None; pActionBuffer = 0;
    SessionId = 0;
}

GINLINE GFxMovieRoot::ActionEntry::ActionEntry(const GFxMovieRoot::ActionEntry &src)
{
    pNextEntry      = src.pNextEntry;
    Type            = src.Type;
    pCharacter      = src.pCharacter;
    pActionBuffer   = src.pActionBuffer;
    EventId         = src.EventId;
    Function        = src.Function;
    CFunction       = src.CFunction;
    FunctionParams  = src.FunctionParams;
    SessionId       = src.SessionId;
}

GINLINE const GFxMovieRoot::ActionEntry& 
GFxMovieRoot::ActionEntry::operator = (const GFxMovieRoot::ActionEntry &src)
{           
    pNextEntry      = src.pNextEntry;
    Type            = src.Type;
    pCharacter      = src.pCharacter;
    pActionBuffer   = src.pActionBuffer;
    EventId         = src.EventId;
    Function        = src.Function;
    CFunction       = src.CFunction;
    FunctionParams  = src.FunctionParams;
    SessionId       = src.SessionId;
    return *this;
}

GINLINE GFxMovieRoot::ActionEntry::ActionEntry(GFxASCharacter *pcharacter, GASActionBuffer* pbuffer)
{
    pNextEntry      = 0;
    Type            = Entry_Buffer;
    pCharacter      = pcharacter;
    pActionBuffer   = pbuffer;
    SessionId       = 0;
}
    
GINLINE GFxMovieRoot::ActionEntry::ActionEntry(GFxASCharacter *pcharacter, const GFxEventId id)
{
    pNextEntry      = 0;
    Type            = Entry_Event;
    pCharacter      = pcharacter;
    pActionBuffer   = 0;
    EventId         = id;
    SessionId       = 0;
}

GINLINE GFxMovieRoot::ActionEntry::ActionEntry(GFxASCharacter *pcharacter, const GASFunctionRef& function, const GASValueArray* params)
{
    pNextEntry      = 0;
    Type            = Entry_Function;
    pCharacter      = pcharacter;
    pActionBuffer   = 0;
    Function        = function;
    if (params)
        FunctionParams = *params;
    SessionId       = 0;
}

GINLINE GFxMovieRoot::ActionEntry::ActionEntry(GFxASCharacter *pcharacter, const GASCFunctionPtr function, const GASValueArray* params)
{
    pNextEntry      = 0;
    Type            = Entry_CFunction;
    pCharacter      = pcharacter;
    pActionBuffer   = 0;
    CFunction        = function;
    if (params)
        FunctionParams = *params;
    SessionId       = 0;
}

GINLINE void GFxMovieRoot::ActionEntry::SetAction(GFxASCharacter *pcharacter, GASActionBuffer* pbuffer)
{
    Type            = Entry_Buffer;
    pCharacter      = pcharacter;
    pActionBuffer   = pbuffer;
    EventId.Id      = GFxEventId::Event_Invalid;
}

GINLINE void GFxMovieRoot::ActionEntry::SetAction(GFxASCharacter *pcharacter, const GFxEventId id)
{
    Type            = Entry_Event;
    pCharacter      = pcharacter;
    pActionBuffer   = 0;
    EventId         = id;
}

GINLINE void GFxMovieRoot::ActionEntry::SetAction(GFxASCharacter *pcharacter, const GASFunctionRef& function, const GASValueArray* params)
{
    Type            = Entry_Function;
    pCharacter      = pcharacter;
    pActionBuffer   = 0;
    Function        = function;
    if (params)
        FunctionParams = *params;
}

GINLINE void GFxMovieRoot::ActionEntry::SetAction(GFxASCharacter *pcharacter, const GASCFunctionPtr function, const GASValueArray* params)
{
    Type            = Entry_CFunction;
    pCharacter      = pcharacter;
    pActionBuffer   = 0;
    CFunction       = function;
    if (params)
        FunctionParams = *params;
}

GINLINE void GFxMovieRoot::ActionEntry::ClearAction()
{
    Type            = Entry_None;
    pActionBuffer   = 0;
    pCharacter      = 0;
    Function.DropRefs();
    FunctionParams.Resize(0);
}

GINLINE bool GFxMovieRoot::ActionEntry::operator==(const GFxMovieRoot::ActionEntry& e) const
{
    return Type == e.Type && pActionBuffer == e.pActionBuffer && pCharacter == e.pCharacter &&
           CFunction == e.CFunction && Function == e.Function && EventId == e.EventId;
}

GINLINE bool GFxMovieRoot::IsAlwaysEnableKeyboardPress() const 
{
    return G_Is3WayFlagTrue<Shift_AlwaysEnableKeyboardPress>(Flags);
}
GINLINE bool GFxMovieRoot::IsAlwaysEnableKeyboardPressSet() const 
{
    return G_Is3WayFlagSet<Shift_AlwaysEnableKeyboardPress>(Flags);
}
GINLINE void GFxMovieRoot::SetAlwaysEnableKeyboardPress(bool f) 
{
    G_Set3WayFlag<Shift_AlwaysEnableKeyboardPress>(Flags, f);
}

GINLINE bool GFxMovieRoot::IsDisableFocusRolloverEvent() const 
{
    return G_Is3WayFlagTrue<Shift_DisableFocusRolloverEvent>(Flags);
}
GINLINE bool GFxMovieRoot::IsDisableFocusRolloverEventSet() const 
{
    return G_Is3WayFlagSet<Shift_DisableFocusRolloverEvent>(Flags);
}
GINLINE void GFxMovieRoot::SetDisableFocusRolloverEvent(bool f) 
{
    G_Set3WayFlag<Shift_DisableFocusRolloverEvent>(Flags, f);
}

GINLINE bool GFxMovieRoot::IsDisableFocusAutoRelease() const
{
    return G_Is3WayFlagTrue<Shift_DisableFocusAutoRelease>(Flags);
}
GINLINE bool GFxMovieRoot::IsDisableFocusAutoReleaseSet() const
{
    return G_Is3WayFlagSet<Shift_DisableFocusAutoRelease>(Flags);
}
GINLINE void GFxMovieRoot::SetDisableFocusAutoRelease(bool f) 
{
    G_Set3WayFlag<Shift_DisableFocusAutoRelease>(Flags, f);
}

GINLINE bool GFxMovieRoot::IsAlwaysEnableFocusArrowKeys() const
{
    return G_Is3WayFlagTrue<Shift_AlwaysEnableFocusArrowKeys>(Flags);
}
GINLINE bool GFxMovieRoot::IsAlwaysEnableFocusArrowKeysSet() const
{
    return G_Is3WayFlagSet<Shift_AlwaysEnableFocusArrowKeys>(Flags);
}
GINLINE void GFxMovieRoot::SetAlwaysEnableFocusArrowKeys(bool f) 
{
    G_Set3WayFlag<Shift_AlwaysEnableFocusArrowKeys>(Flags, f);
}

GINLINE bool GFxMovieRoot::IsDisableFocusKeys() const
{
    return G_Is3WayFlagTrue<Shift_DisableFocusKeys>(Flags);
}
GINLINE bool GFxMovieRoot::IsDisableFocusKeysSet() const
{
    return G_Is3WayFlagSet<Shift_DisableFocusKeys>(Flags);
}
GINLINE void GFxMovieRoot::SetDisableFocusKeys(bool f) 
{
    G_Set3WayFlag<Shift_DisableFocusKeys>(Flags, f);
}

GINLINE void GFxMovieRoot::SetPerspectiveFOV(Float fov)
{
#ifndef GFC_NO_3D
    PerspFOV = fov;
    if (pPerspectiveMatrix3D)
    {
        GMatrix3D matView;
        if (GetRenderer())
            GetRenderer()->MakeViewAndPersp3D(VisibleFrameRect, matView, *pPerspectiveMatrix3D, PerspFOV);
    }
#else
    GUNUSED(fov);
#endif
}

GINLINE void GFxMovieRoot::SetPerspective3D(const GMatrix3D &projMatIn)
{
#ifndef GFC_NO_3D
    if (pPerspectiveMatrix3D == NULL)
        pPerspectiveMatrix3D = GHEAP_NEW(pHeap) GMatrix3DNewable;
    
    *pPerspectiveMatrix3D = projMatIn;
    Make3D();           // Make the movie fully 3D so that all parts use the 3D persp matrix
#else
    GUNUSED(projMatIn);
#endif
}

GINLINE void GFxMovieRoot::SetView3D(const GMatrix3D &viewMatIn)
{
#ifndef GFC_NO_3D
    if (pViewMatrix3D == NULL)
        pViewMatrix3D = GHEAP_NEW(pHeap) GMatrix3DNewable;

    *pViewMatrix3D = viewMatIn;
    Make3D();           // Make the movie fully 3D so that all parts use the 3D view matrix
#else
    GUNUSED(viewMatIn);
#endif
}

// ** End Inline Implementation


//!AB: This class restores high precision mode of FPU for X86 CPUs.
// Direct3D may set the Mantissa Precision Control Bits to 24-bit (by default 53-bits) and this 
// leads to bad precision of FP arithmetic. For example, the result of 0.0123456789 + 1.0 will 
// be 1.0123456789 with 53-bit mantissa mode and 1.012345671653 with 24-bit mode.
class GFxDoublePrecisionGuard
{
    unsigned    fpc;
    //short       fpc;
public:

    GFxDoublePrecisionGuard ()
    {
#if defined (GFC_CC_MSVC) && defined(GFC_CPU_X86)
  #if (GFC_CC_MSVC >= 1400)
        // save precision mode (control word)
        _controlfp_s(&fpc, 0, 0);
        // set 53 bit precision
        unsigned _fpc;
        _controlfp_s(&_fpc, _PC_53, _MCW_PC);
  #else
        // save precision mode (control word)
        fpc = _controlfp(0,0);
        // set 53 bit precision
        _controlfp(_PC_53, _MCW_PC);
  #endif
        /*
        // save precision mode (only for X86)
        GASM fstcw fpc;
        //short _fpc = (fpc & ~0x300) | 0x300;  // 64-bit mantissa (REAL10)
        short _fpc = (fpc & ~0x300) | 0x200;  // 53-bit mantissa (REAL8)
        // set 53 bit precision
        GASM fldcw _fpc;
        */
#endif
    }

    ~GFxDoublePrecisionGuard ()
    {
#if defined (GFC_CC_MSVC) && defined (GFC_CPU_X86)
  #if (GFC_CC_MSVC >= 1400)
        // restore precision mode
        unsigned _fpc;
        _controlfp_s(&_fpc, fpc, _MCW_PC);
  #else
        _controlfp(fpc, _MCW_PC);
  #endif
        /*
        // restore precision mode (only for X86)
        GASM fldcw fpc;
        */
#endif
    }
};


#endif // INC_GFxPlayerImpl_H
