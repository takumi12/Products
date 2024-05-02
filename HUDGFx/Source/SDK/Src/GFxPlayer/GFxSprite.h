/**********************************************************************

Filename    :   GFxSprite.h
Content     :   Sprite Implementation header file.
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   This file contains class declarations used in
                GFxSprite.cpp only. Declarations that need to be
                visible by other player files should be placed
                in GFxCharacter.h.


Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXSPRITE_H
#define INC_GFXSPRITE_H

//#include "GFxPlayerImpl.h"

#include "GFxCharacter.h"
#include "GFxDlist.h"

// For now
#include "GFxMovieDef.h"

#include "GFxAction.h"
#include "AS/GASObjectProto.h"

// For drawing API container.
#include "GFxDrawingContext.h"

#include "GFxMediaInterfaces.h"

// ***** Declared Classes
class GFxSpriteDef;
class GFxSprite;
class GASMovieClipObject;
class GASMovieClipProto;

// Forward decl - from GFxPlayerImpl.h
class GFxMovieRoot;

class GFxStaticTextSnapshotData;

#ifndef GFC_NO_SOUND
class GASSoundObject;
class GFxSoundResource;
#endif

//
// ***** GFxSpriteDef
//

// A sprite is a mini movie-within-a-movie.  It doesn't define
// its own characters; it uses the characters from the parent
// GFxMovieDefImpl, but it has its own frame counter, display list, etc.
//
// The sprite implementation is divided into a
// GFxSpriteDef and a GFxSprite.  The Definition
// holds the immutable data for a sprite, while the Instance
// contains the state for a specific instance being updated
// and displayed in the parent GFxASCharacter's display list.

class GFxSpriteDef : public GFxTimelineIODef
{
    // MovieDef to which this sprite belongs. Note that this pointer can be 
    // invalid temporarily during loading if all references to MovieDataDef
    // have been released but load thread hasn't yet died. This shouldn't
    // be a problem since constructor / Read methods don't use this pointer.
    GFxMovieDataDef*        pMovieDef;

    GFxStringHashLH<UInt>   NamedFrames;    // stores 0-based frame #'s
    int                     FrameCount;
    int                     LoadingFrame;

    // GFxSprite control events for each frame.
    GArrayLH<Frame, GFxStatMD_Other_Mem>  Playlist;

    GFxScale9Grid*          pScale9Grid;
#ifndef GFC_NO_SOUND
    GPtr<GFxSoundStreamDef> pSoundStream;
#endif

    enum
    {
        Flags_Has_Frame_up      = 0x1,
        Flags_Has_Frame_down    = 0x2,
        Flags_Has_Frame_over    = 0x4,
        Flags_HasSpecialFrames = (Flags_Has_Frame_up|Flags_Has_Frame_down|Flags_Has_Frame_over)
    };
    UByte                   Flags;

public:
    
    GFxSpriteDef(GFxMovieDataDef* pmd);
    ~GFxSpriteDef();
    
    // Initialize an empty clip.
    void            InitEmptyClipDef();
    // Load the sprite from disk (usually called from background thread).
    void            Read(GFxLoadProcess* p, GFxResourceId charId);

    
    // Overloads from GFxCharacterDef. 
    virtual GFxCharacter*   CreateCharacterInstance(GFxASCharacter* parent, GFxResourceId id,
                                                    GFxMovieDefImpl *pbindingImpl);


    // overloads from GFxMovieDef
    virtual Float           GetWidth() const            { return 1; }
    virtual Float           GetHeight() const           { return 1; }
    virtual bool            DefPointTestLocal(const GPointF &pt, bool testShape = 0,
                                              const GFxCharacter *pinst = 0) const;
    //virtual GRectF        GetBoundsLocal() const                                      { return GRectF(0); }   
    virtual UInt            GetFrameCount() const       { return FrameCount; }
    virtual Float           GetFrameRate() const        { return pMovieDef->GetFrameRate(); }
    virtual GRectF          GetFrameRect() const        { return GRectF(0,0,1,1); }
    virtual UInt            GetLoadingFrame() const     { return LoadingFrame; }
    virtual UInt            GetVersion() const          { return pMovieDef->GetVersion(); }
    virtual UInt            GetSWFFlags() const         { return pMovieDef->GetSWFFlags(); }
    virtual UInt            GetTagCount() const         { return 0; }
    virtual UInt32          GetFileBytes() const        { return pMovieDef->GetFileBytes(); }
    virtual const char*     GetFileURL() const          { return pMovieDef->GetFileURL(); }
         

    // Returns 0-based frame #  
    bool                    GetLabeledFrame(const char* label, UInt* frameNumber, bool translateNumbers = 1)
    {
        return GFxMovieDataDef::TranslateFrameString(NamedFrames, label, frameNumber, translateNumbers);        
    }
  
      
    // Labels the frame currently being loaded with the
    // given name.  A copy of the name string is made and
    // kept in this object.
    virtual void    AddFrameName(const GString& name, GFxLog *plog);  
    virtual void    SetLoadingPlaylistFrame(const Frame& frame)
    {
        GASSERT(Playlist[LoadingFrame].TagCount == 0);
        Playlist[LoadingFrame] = frame;
    }
    
    // *** GFxTimelineDef implementation

    // FrameNumber is 0-based
    virtual const Frame  GetPlaylist(int frameNumber) const    
    {
        return Playlist[frameNumber];
    }    
    virtual bool         GetInitActions(Frame* pframe, int frameNumber) const
    {
        GUNUSED2(frameNumber, pframe);
        // Sprites do not have init actions in their playlists!
        // Only the root sprite (GFxMovieDataDef) does.
        return false;
    }    

#ifndef GFC_NO_SOUND
    virtual GFxSoundStreamDef*  GetSoundStream() const;
    virtual void                SetSoundStream(GFxSoundStreamDef* psoundStream);
#endif    

    // *** GFxResource implementation

    // Query Resource type code, which is a combination of ResourceType and ResourceUse.
    virtual UInt            GetResourceTypeCode() const     { return MakeTypeCode(RT_SpriteDef); }

    void                    SetScale9Grid(const GRectF& r) 
    {
        if (pScale9Grid == 0) 
            pScale9Grid = GHEAP_AUTO_NEW(this) GFxScale9Grid;
        *pScale9Grid = r;
    }

    const GFxScale9Grid*    GetScale9Grid() const { return pScale9Grid; }

    bool                    HasSpecialFrames() const { return (Flags & Flags_HasSpecialFrames) != 0; }
    bool                    HasFrame_up() const   { return (Flags & Flags_Has_Frame_up) != 0; }
    bool                    HasFrame_down() const { return (Flags & Flags_Has_Frame_down) != 0; }
    bool                    HasFrame_over() const { return (Flags & Flags_Has_Frame_over) != 0; }
};


//
// ***** GFxSprite
//

class GFxSprite : public GFxASCharacter
{
    friend class GFxASCharacter;

    typedef GFxMovie::PlayState PlayState;

    typedef GFxTimelineDef::Frame Frame;
    // Data source for playlist (either GFxSpriteDef of GFxMovieDataDef).
    GPtr<GFxTimelineDef>    pDef;
    GFxMovieRoot*           pRoot;
    GFxScale9Grid*          pScale9Grid;

    GFxDisplayList      DisplayList;    

    PlayState           PlayStatePriv;
    UInt                CurrentFrame;
    GArrayLH<bool>      InitActionsExecuted;    // a bit-GArray class would be ideal for this

    GASEnvironment      ASEnvironment;


    // Used only for root-level sprites; otherwise -1
    SInt                Level;

    // A node used by every root sprite in a tree so that they can be
    // scanned and initialized efficiently to a thread-synched Advance.
    // Null for all non-root sprites.
    struct GFxMovieDefRootNode* pRootNode;
    
    // AS object
    GPtr<GASMovieClipObject> ASMovieClipObj;

#if !defined(GFC_NO_SOUND)
public:
    // keep a track of active sound playing in this sprite
    struct ActiveSoundItem : public GRefCountBaseNTS<ActiveSoundItem, GFxStatMV_Other_Mem>
    {
        ActiveSoundItem();
        ~ActiveSoundItem();

        GPtr<GSoundChannel> pChannel;          // active channel
        GASSoundObject*     pSoundObject;      // AS sound object which created this sound
                                               // it can be NULL for sound which were created 
                                               // by a sound object (event,streaming,video sounds)
                                               // it is used for calling Sound.onSoundComplete handler
        GFxSoundResource*   pResource;         // resource from the swf resource lib 
                                               // could be NULL for stream and video sounds.
                                               // the object will be addrefed manually to avoid compile dependences
                                               // it is used for stopping sounds by linkage name
    };
    // list of active sound and sound attributes and sound objects
    struct ActiveSounds : public GNewOverrideBase<GFxStatMV_Other_Mem>
    {
        ActiveSounds();
        ~ActiveSounds();

        SInt                Volume;            // volume for this movie clip
        SInt                SubVolume;         // volume for SubAudio track (if available)
        SInt                Pan;               // specifying the left-right balance for a sound (-100 .. 100)
        GArrayLH<GPtr<ActiveSoundItem> > Sounds;   // list of active sound channels
        GArrayLH<GASSoundObject*>        ASSounds; // list of all AS sound objects attached to this sprite
        GPtr<GSoundChannel> pStreamSound;      // streaming sound of this movie clip
    };

private:
    // active sound started in this sprite
    ActiveSounds*          pActiveSounds;
	SInt                   FindActiveSound(ActiveSoundItem* item);

#endif //  GFC_NO_SOUND

    GPtr<GFxDrawingContext>  pDrawingAPI;

    union
    {
        GFxSprite*           pMaskCharacter;  // a refcnt ptr to mask clip
        GFxSprite*           pMaskOwner;      // a weak ptr to mask owner clip
    };

    // hitArea for this sprite
    GPtr<GFxCharacterHandle>  pHitAreaHandle;
    // Pointer to sprite which has this sprite as hit area  
    GFxSprite*                pHitAreaHolder; 

    // Transient holder for user renderer data set from Actionscript
    struct UserRendererData : public GNewOverrideBase<GFxStatMV_ActionScript_Mem>
    {           
        GASString               StringVal;
        float                   FloatVal;
        float                   MatrixVal[16];
        GRenderer::UserData     UserData;
        UserRendererData(GASStringContext* psc)
            : StringVal(psc->CreateString((const char*)NULL)), FloatVal(0.0f) { }
    };
    UserRendererData*       pUserData;

    enum
    {
        Flags_UpdateFrame       = 0x1,
        Flags_HasLoopedPriv     = 0x2,
        Flags_Unloaded          = 0x4, // Set once Unload has been called.
        Flags_OnEventLoadCalled = 0x8,

        // This flag is set if sprite was loaded with loadMovie.
        // If so, _lockroot applies.
        Flags_LoadedSeparately  = 0x10,
        Flags_LockRoot          = 0x20,
        Flags_SpriteDef         = 0x40,
        Flags_OnLoadReqd        = 0x80
    };
    UByte               Flags;

    // focus related stuff
    Bool3W              FocusEnabled;
    Bool3W              TabChildren;

    void SetUpdateFrame(bool v = true)      { Flags = (UByte)((v) ? (Flags | Flags_UpdateFrame)     : (Flags & (~Flags_UpdateFrame))); }
    void SetHasLoopedPriv(bool v = true)    { Flags = (UByte)((v) ? (Flags | Flags_HasLoopedPriv)   : (Flags & (~Flags_HasLoopedPriv))); }
    void SetOnEventLoadCalled(bool v = true){ Flags = (UByte)((v) ? (Flags | Flags_OnEventLoadCalled)   : (Flags & (~Flags_OnEventLoadCalled))); }
    void SetLockRoot(bool v = true)         { Flags = (UByte)((v) ? (Flags | Flags_LockRoot)        : (Flags & (~Flags_LockRoot))); }
    void SetLoadedSeparately(bool v = true) { Flags = (UByte)((v) ? (Flags | Flags_LoadedSeparately): (Flags & (~Flags_LoadedSeparately))); }
    void SetOnLoadReqd(bool v = true)       { Flags = (UByte)((v) ? (Flags | Flags_OnLoadReqd)      : (Flags & (~Flags_OnLoadReqd))); }

    bool IsUpdateFrame() const              { return (Flags & Flags_UpdateFrame) != 0; }
    bool IsOnEventLoadCalled() const        { return (Flags & Flags_OnEventLoadCalled) != 0; }
    bool IsLockRoot() const                 { return (Flags & Flags_LockRoot) != 0; }
    bool IsLoadedSeparately() const         { return (Flags & Flags_LoadedSeparately) != 0; }
    bool IsOnLoadReqd() const               { return (Flags & Flags_OnLoadReqd) != 0; }

    void                CloneInternalData(const GFxASCharacter* src);

    // used by both GetMember & GetMemberRaw to share common part of code
    bool                GetMember(GASEnvironment* penv, GASStringContext *psc, const GASString& name, GASValue* pval);

    void                SetDirtyFlag();

    // internal method; will create GASMovieClip object on-demand
    GASMovieClipObject* GetMovieClipObject ();

public: 

    enum MouseState
    {
        UP = 0,
        DOWN,
        OVER
    };
    MouseState          MouseStatePriv;

public:
    // Both pdef (play-list) and pdefImpl (binding) pointers must be provided.
    GFxSprite(GFxTimelineDef* pdef, GFxMovieDefImpl* pdefImpl,
              GFxMovieRoot* pr, GFxASCharacter* parent,
              GFxResourceId id, bool loadedSeparately = false);

    virtual ~GFxSprite();

    void            SetRendererString(GASString str);
    void            SetRendererFloat(float num);
    void            SetRendererMatrix(float *m, UInt count = 16);

    void            ForceShutdown ();
    
    void            ClearDisplayList();

    // this method is called only with threaded loading to set correct values
    // when the first has just been loaded
    void            SetRootNodeLoadingStat(UInt bytesLoaded, UInt loadingFrame);

#ifndef GFC_NO_IME_SUPPORT
    void            SetIMECandidateListFont(GFxFontResource* pfont);
#endif //#ifndef GFC_NO_IME_SUPPORT

    virtual void    OnIntervalTimer(void *timer);

    // These accessor methods have custom implementation in GFxSprite.
    virtual GFxCharacterDef*    GetCharacterDef() const        { return pDef.GetPtr(); }
    virtual GFxMovieDefImpl*    GetResourceMovieDef() const;    
    virtual GFxFontManager*     GetFontManager() const;
    virtual GFxMovieRoot*       GetMovieRoot() const;
    virtual GFxASCharacter*     GetLevelMovie(SInt level) const;
    virtual GFxASCharacter*     GetASRootMovie(bool ignoreLockRoot = false) const;
    virtual const GFxScale9Grid* GetScale9Grid() const { return pScale9Grid; }
    virtual void                PropagateScale9GridExists();

    GASEnvironment*             GetASEnvironment()          { return &ASEnvironment; }
    const GASEnvironment*       GetASEnvironment() const    { return &ASEnvironment; }
    GASObject*                  GetASObject ();
    GASObject*                  GetASObject () const;

    virtual void                SetName(const GASString& name);

    // Bounds computation.
    // "transform" matrix describes the transform applied to parent and us,
    // including the object's matrix itself. This means that if transform is
   // identity, GetBoundsTransformed will return local bounds and NOT parent bounds.
 #ifndef GFC_NO_3D
    virtual GRectF  GetBounds(const GMatrix3D &transform, bool bDivideByW = false) const;       
    virtual void    GetBounds(GPointF *pts, const GMatrix3D &transform, bool bDivideByW = false) const  
    {   
        GRectF r = GetBounds(transform, bDivideByW);
        pts[0] = r.TopLeft();
        pts[1] = r.TopRight();
        pts[2] = r.BottomRight();
        pts[3] = r.BottomLeft();
    }
#endif
    virtual GRectF  GetBounds(const Matrix &transform) const;       
    virtual GRectF  GetRectBounds(const Matrix &transform) const;       

#ifndef GFC_NO_FXPLAYER_AS_TEXTSNAPSHOT
    void            GetTextSnapshot(GFxStaticTextSnapshotData* pdata) const;
#endif

    UInt            GetCurrentFrame() const { return CurrentFrame; }
    void            UpdateRootFrame();
    UInt            GetFrameCount() const   { return pDef->GetFrameCount(); } 
    UInt32          GetBytesLoaded() const;
    UInt            GetLoadingFrame() const;

    // Level access.
    inline SInt     GetLevel() const        { return Level; }
    void            SetLevel(SInt level);
    inline bool     IsLevelsMovie() const   { return Level >= 0; }

    // Checks that load was called and does so if necessary.
    // Used for root-level sprites.
    void            ExecuteFrame0Events();


    // Returns 0-based frame #
    bool            GetLabeledFrame(const char* label, UInt* frameNumber, bool translateNumbers = 1)
    {
        return pDef->GetLabeledFrame(label, frameNumber, translateNumbers);
    }


    // Stop or play the sprite.
    void            SetPlayState(PlayState s);
    PlayState       GetPlayState() const        { return PlayStatePriv; }


    void            Restart();

    // Returns 0 if nothing to do
    // 1 - if need to add to optimized play list
    // -1 - if need to remove from optimized play list
    virtual int     CheckAdvanceStatus(bool playingNow);
    virtual void    PropagateNoAdvanceGlobalFlag();
    virtual void    PropagateNoAdvanceLocalFlag();
    virtual void    PropagateFocusGroupMask(UInt mask);
    virtual bool    HasLooped() const { return (Flags & Flags_HasLoopedPriv) != 0; }

    // Combine the flags to avoid a conditional. It would be faster with a macro.
    GINLINE int      Transition(int a, int b) const
    {
        return (a << 2) | b;
    }

    // Return the topmost entity that the given point covers.  NULL if none.
    // Coords are in parent's frame.    
    virtual GFxASCharacter* GetTopMostMouseEntity(const GPointF &pt, const TopMostParams&);   
    
    GFxSprite*          GetHitAreaHolder() const {return pHitAreaHolder;};
    void                SetHitAreaHolder(GFxSprite* phitAreaHolder) {pHitAreaHolder = phitAreaHolder ;}
    GFxSprite*          GetHitArea() const;
    void                SetHitArea (GFxSprite* phitArea);
    int                 GetHitAreaIndex();
    
    // Increment CurrentFrame, and take care of looping.
    void            IncrementFrameAndCheckForLoop();
    

    virtual void    AdvanceFrame(bool nextFrame, Float framePos);
    
    
    // Execute the tags associated with the specified frame.
    // noRemove is specified if Removal tags are not to be executed (causing warning). Useful
    // for executing tags in the target frame after a backwards seek.
    // frame is 0-based
    void            ExecuteFrameTags(UInt frame);
    // Executes init action tags only for the frame.
    void            ExecuteInitActionFrameTags(UInt frame);

    // Set the sprite state at the specified frame number.
    // 0-based frame numbers!!  (in contrast to ActionScript and Flash MX)
    void            GotoFrame(UInt targetFrameNumber);



    // Look up the labeled frame, and jump to it.
    bool            GotoLabeledFrame(const char* label, SInt offset = 0);
            
    void            Display(GFxDisplayContext &context);

    void            SetPause(bool pause);

    // *** Display List management.
    
    // Find previous flag options.  
    enum {
        FindPrev_AllowMove  = 1,    // Allow move tags to be found. If not specified, we will look for add/replace only.
        FindPrev_NeedMatrix = 2,    // Must find a valid prev matrix (because it was replaced in next tag).
        FindPrev_NeedCXForm = 4,    // Must find a valid cxform.
        FindPrev_NeedBlend  = 8,    // Must find a previous blend mode.
        FindPrev_NeedFilters= 16    // Must find a previous filter list.
    };

    // Add an object to the display list.
    // CreateFrame specifies frame used as 'CreateFrame' for the new object (def: ~0 == CurrentFrame).
    GFxCharacter*   AddDisplayObject(
                        const GFxCharPosInfo &pos, const GASString& name,
                        const GArrayLH<GFxSwfEvent*, GFxStatMD_Tags_Mem>* peventHandlers,
                        const GASObjectInterface *pinitSource,
                        UInt createFrame = GFC_MAX_UINT,
                        UInt32 addFlags = 0,
                        GFxCharacterCreateInfo* pcharCreateOverride = 0,
                        GFxASCharacter* origChar = 0);

    // Updates the transform properties of the object at the specified depth.   
    void            MoveDisplayObject(const GFxCharPosInfo &pos);
    void            ReplaceDisplayObject(const GFxCharPosInfo &pos, const GASString& name);
    void            ReplaceDisplayObject(const GFxCharPosInfo &pos, GFxCharacter* ch, const GASString& name);
        
    // Remove the object at the specified depth.
    // If id != -1, then only remove the object at depth with matching id.
    void            RemoveDisplayObject(SInt depth, GFxResourceId id);
    // Remove *this* object from its parent.
    void            RemoveDisplayObject() {GFxASCharacter::RemoveDisplayObject(); }
    

    // Movie Loading support
    virtual bool        ReplaceChildCharacterOnLoad(GFxASCharacter *poldChar, GFxASCharacter *pnewChar);
    virtual bool        ReplaceChildCharacter(GFxASCharacter *poldChar, GFxASCharacter *pnewChar);
    
        
    // For debugging -- return the id of the GFxCharacter at the specified depth.
    // Return 0 if nobody's home.
    GFxCharacter*   GetCharacterAtDepth(int depth);
    // Return -1 if nobody's home.
    GFxResourceId   GetIdAtDepth(int depth);

    

    void            ExecuteImportedInitActions(GFxMovieDef* psourceMovie);

    // Add the given action buffer to the list of action
    // buffers to be processed at the end of the next frame advance in root.    
    void            AddActionBuffer(GASActionBuffer* a,
                                    GFxActionPriority::Priority prio = GFxActionPriority::AP_Frame);


    // Need to access depths, etc.
    static void         SpriteSwapDepths(const GASFnCall& fn);

    // Returns maximum used depth, -1 if no depth is available.
    SInt        GetLargestDepthInUse() const { return DisplayList.GetLargestDepthInUse();}


    //
    // *** Button logc support
    // 

    // Returns 1 if sprite acts as a button due to handlers.
    bool            ActsAsButton() const;


    //
    // *** ActionScript support
    //
    
    // GFxASCharacter override to indicate which standard members are handled by us.
    virtual UInt32      GetStandardMemberBitMask() const;

    // Handles built-in members. Return 0 if member is not found or not supported.
    virtual bool        SetStandardMember(StandardMember member, const GASValue& val, bool opcodeFlag);
    virtual bool        GetStandardMember(StandardMember member, GASValue* val, bool opcodeFlag) const;

    // GASObjectInterface implementation.
    virtual bool        GetMember(GASEnvironment* penv, const GASString& name, GASValue* pval)
    {
        return GetMember(penv, NULL, name, pval);
    }
    virtual bool        DeleteMember(GASStringContext *psc, const GASString& name);
    virtual void        VisitMembers(GASStringContext *psc, MemberVisitor *pvisitor, UInt visitFlags = 0, const GASObjectInterface* instance = 0) const;
    virtual ObjectType  GetObjectType() const { return Object_Sprite; }
    virtual bool        SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags());
    virtual bool        GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* pval)
    {
        return GetMember(NULL, psc, name, pval);
    }
    
    virtual GFxASCharacter* GetRelativeTarget(const GASString& name, bool first_call);

    // Execute the actions for the specified frame. 
    virtual void        CallFrameActions(UInt frameNumber);


    // Override to check handlers for an id.
    // Propagates an event to all children.
    virtual void    PropagateMouseEvent(const GFxEventId& id);
    virtual void    PropagateKeyEvent(const GFxEventId& id, int* pkeyMask);
    
    // Dispatch event Handler(s), if any.
    virtual bool    OnEvent(const GFxEventId& id);
    // Special event handler; ensures that unload is called on child items in timely manner.
    virtual void    OnEventUnload();
    virtual bool    OnUnloading();
    virtual void    DetachChild(GFxASCharacter* pchild);
    // Dispatch key event
    virtual bool    OnKeyEvent(const GFxEventId& id, int* pkeyMask);

    // Execute action buffer
    virtual bool    ExecuteBuffer(GASActionBuffer* pactionbuffer);
    // Execute this even immediately (called for processing queued event actions).
    virtual bool    ExecuteEvent(const GFxEventId& id);

    bool            HasEventHandler(const GFxEventId& id) const;

    // Handle a button event.
    virtual bool    OnButtonEvent(const GFxEventId& id);

    // Do the events That (appear to) happen as the GFxASCharacter
    // loads.  frame1 tags and actions are Executed (even
    // before Advance() is called).  Then the onLoad event
    // is triggered.
    virtual void    OnEventLoad();

    // Do the events that happen when there is XML data waiting
    // on the XML socket connection.
    virtual void    OnEventXmlsocketOnxml();    
    // Do the events That (appear to) happen on a specified interval.
    virtual void    OnEventIntervalTimer(); 
    // Do the events that happen as a MovieClip (swf 7 only) loads.
    virtual void    OnEventLoadProgress();

    bool            Invoke(const char* methodName, GASValue* presult, UInt numArgs);
    bool            InvokeArgs(const char* methodName, GASValue *presult, const char* methodArgFmt, va_list args);     
    //virtual const char* InvokeArgs(const char* methodName, const char* methodArgFmt, const void* const* methodArgs, int numArgs);

    void SetHasButtonHandlers (bool has);

    virtual bool    PointTestLocal(const GPointF &pt, UInt8 hitTestMask = 0) const;

    void CalcDisplayListHitTestMaskArray(GArray<UByte> *phitTest, const GPointF &p, bool testShape) const;

    void NotifyMemberChanged (const GASString& name, const GASValue& val);

    // focus stuff
    virtual bool            IsTabable() const;
    virtual void            FillTabableArray(FillTabableParams* params);
    // reports focusEnabled property state (used for Selection.setFocus())
    virtual bool            IsFocusEnabled() const;
    // invoked when item is going to get focus (Selection.setFocus is invoked, or TAB is pressed)
    virtual void            OnGettingKeyboardFocus(UInt controllerIdx);
    // invoked when focused item is about to lose keyboard focus input (mouse moved, for example)
    virtual bool            OnLosingKeyboardFocus(GFxASCharacter* newFocusCh, UInt controllerIdx, GFxFocusMovedType fmt);
    // returns rectangle for focusrect, in local coords
    virtual GRectF          GetFocusRect() const;

    // notify the sprite to be inserted as a _levelN movie
    virtual void            OnInsertionAsLevel(int level);

    // Set state changed flags
    virtual void            SetStateChangeFlags(UInt8 flags);


    UInt                    GetCursorType() const;

    void                    SetMask(GFxSprite* ch);
    GFxSprite*              GetMask() const;

    void                    SetMaskOwner(GFxSprite* ch);
    GFxSprite*              GetMaskOwner() const;

    void                    SetScale9Grid(const GFxScale9Grid* gr);
    virtual void            SetVisible(bool visible);

    GFxSpriteDef*           GetSpriteDef() 
    { 
        return (Flags & Flags_SpriteDef) ? static_cast<GFxSpriteDef*>(pDef.GetPtr()) : NULL; 
    }

    //
    // *** Drawing API support
    //
    void Clear();
    void SetNoFill();
    void SetNoLine();
    void SetLineStyle(Float lineWidth, 
                      UInt  rgba = 0, 
                      bool  hinting = false, 
                      GFxLineStyle::LineStyle scaling = GFxLineStyle::LineScaling_Normal, 
                      GFxLineStyle::LineStyle caps    = GFxLineStyle::LineCap_Round,
                      GFxLineStyle::LineStyle joins   = GFxLineStyle::LineJoin_Round,
                      Float miterLimit = 3.0f);
    GFxFillStyle* BeginFill();
    GFxFillStyle* CreateLineComplexFill();
    void BeginFill(UInt rgba);    
    void BeginBitmapFill(GFxFillType fillType,
                         GFxImageResource* pimageRes, 
                         const Matrix& mtx);
    void EndFill();
    void MoveTo(Float x, Float y);
    void LineTo(Float x, Float y);
    void CurveTo(Float cx, Float cy, Float ax, Float ay);
    bool AcquirePath(bool newShapeFlag);

#if !defined(GFC_NO_SOUND) 
    // check if load sound is already playing
    bool              IsSoundPlaying(GASSoundObject* psobj);
    // add an active sound
    void              AddActiveSound(GSoundChannel* pchan, GASSoundObject* psobj, GFxSoundResource* pres);
    // add a streaming sound
    void              SetStreamingSound(GSoundChannel* pchan);
    // return a streaming sound
    GSoundChannel*    GetStreamingSound() const;
    // Detach an AS sound object from this sprite
    void              DetachSoundObject(GASSoundObject*);
    // Return a sound pan which is saved inside this object.
    // It is used just for displaying this value
    SInt              GetSoundPan();
    // Return a calculated sound volume.
    Float             GetRealSoundPan();
    // Save the new sound volume and propagate it to the child movieclip
    void              SetSoundPan(SInt volume);
    // Return a sound volume which is saved inside this object.
    // It is used just for displaying this value
    SInt              GetSoundVolume();
    // Return a sound volume for SubAudio track which is saved inside this object.
    SInt              GetSubSoundVolume();
    // Return a calculated sound volume.
    Float             GetRealSoundVolume();
    // Return a calculated sound volume for SubAudio track.
    Float             GetRealSubSoundVolume();
    // Save the new sound volume and propagate it to the child movieclip
    void              SetSoundVolume(SInt volume, SInt subvol = 100);
    // Stop all active sound in this movieclip and all child clips
    void              StopActiveSounds();
    // Stop all active sound in this movieclip and all child clips
    //void              StopActiveSounds(const GString& name);
    // Stop all with the give resource in this sprite with the given linkage name
    void              StopActiveSounds(GFxSoundResource* pres);
    // update the volume of all sound started by this sprite
    void              UpdateActiveSoundVolume();
    // update the pan of all sound started by this sprite
    void              UpdateActiveSoundPan();
    // release an active sound which is attached to this sprite. It is used to pass
    // active sounds between movieclips with MovieClip.attachAudio method
    ActiveSoundItem*  ReleaseActiveSound(GSoundChannel*);
    // attach an active sound released by ReleaseActiveSound mehtod.
    void              AttachActiveSound(ActiveSoundItem*);
    // register an AS sound object which is attached to this sprite
    void              AttachSoundObject(GASSoundObject*);
    // return an active sound playing position
    Float             GetActiveSoundPosition(GASSoundObject* psobj);
    // stop streaming sound
    void              StopStreamSound();
    // Go through the list of active sounds and remove sounds that are not 
    // playing anymore (will call Sound.onSoundComplete event handler)
    // this method should be called from AdvenceFrame.
    void              CheckActiveSounds();

#endif //  GFC_NO_SOUND

    void MakeSnapshot(GFxTimelineSnapshot* psnapshot, UInt startFrame, UInt endFrame);
    void ExecuteSnapshot(GFxTimelineSnapshot* psnapshot, GFxActionPriority::Priority prio=GFxActionPriority::AP_Frame);

#ifndef GFC_NO_3D
    virtual void UpdateViewAndPerspective();
    virtual bool Has3D() const;   // returns true if sprite contains 3D characters
#endif
#ifdef GFC_BUILD_DEBUG
    void DumpDisplayList(int indent, const char* title = NULL);
#else
    inline void DumpDisplayList(int , const char*  = NULL) {}
#endif
};



//!AB should be separated later.
// Class that represents MovieClip ActionScript class.
class GASMovieClipObject : public GASObject
{
    friend class GASMovieClipProto;
    friend class GASMovieClipCtorFunction;

    GWeakPtr<GFxSprite> pSprite;    // weak ref on sprite obj
    
    // Bit mask of all button events set on the movieclip
    enum
    {
        Mask_onPress            = 0x001,
        Mask_onRelease          = 0x002,
        Mask_onReleaseOutside   = 0x004,
        Mask_onRollOver         = 0x008,
        Mask_onRollOut          = 0x010,
        Mask_onDragOver         = 0x020,
        Mask_onDragOut          = 0x040,

        Mask_onPressAux             = 0x080,
        Mask_onReleaseAux           = 0x100,
        Mask_onReleaseOutsideAux    = 0x200,
        Mask_onDragOverAux          = 0x400,
        Mask_onDragOutAux           = 0x800
    };
    UInt16 ButtonEventMask;

#ifndef GFC_NO_GC
protected:
    void Finalize_GC()
    {
        pSprite.~GWeakPtr<GFxSprite>();
        GASObject::Finalize_GC();
    }
#endif // GFC_NO_GC
    void commonInit();
protected:
    // If any of these variables are non-zero, sprite acts as a button,
    // catching all of buttonAction logic and disabling nested buttons
    // or similar clip actions.

    // Set if the sprite has any button event handlers.
    bool                HasButtonHandlers;
    // Count of dynamic handlers installed by SetMember: onPress, onRollOver, etc.
    //UByte               DynButtonHandlerCount;  

    // Updates DynButtonHandlerCount if name is begin added through SetMember,
    // or deleted 
    void            TrackMemberButtonHandler(GASStringContext* psc, const GASString& name, bool deleteFlag = 0);

    GASMovieClipObject(GASStringContext *psc, GASObject* pprototype) 
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
        commonInit();
    }
    GASMovieClipObject(GASEnvironment* penv)
        : GASObject(penv)
    { 
        commonInit();
        Set__proto__(penv->GetSC(), penv->GetGC()->GetActualPrototype(penv, GASBuiltin_MovieClip));
    }
    void    SetMemberCommon(GASStringContext *psc, 
                            const GASString& name, 
                            const GASValue& val);
public:
    GASMovieClipObject(GASGlobalContext* gCtxt, GFxSprite* psprite) 
        : GASObject(gCtxt->GetGC()), pSprite(psprite)
    {
        commonInit ();
        GASStringContext* psc = psprite->GetASEnvironment()->GetSC();
        Set__proto__ (psc, psprite->Get__proto__());
    }

    // Set the named member to the value.  Return true if we have
    // that member; false otherwise.
    virtual bool        SetMemberRaw(GASStringContext *psc, 
                                     const GASString& name, 
                                     const GASValue& val, 
                                     const GASPropFlags& flags = GASPropFlags());
    virtual bool        SetMember(GASEnvironment* penv, 
                                    const GASString& name, 
                                    const GASValue& val, 
                                    const GASPropFlags& flags = GASPropFlags());
    //virtual bool      GetMember(GASEnvironment* penv, const GASString& name, GASValue* val);
    virtual bool        DeleteMember(GASStringContext *psc, const GASString& name);

    virtual void        Set__proto__(GASStringContext *psc, GASObject* protoObj);

    // Returns 1 if sprite acts as a button due to handlers.
    bool                ActsAsButton() const;

    GINLINE void        SetHasButtonHandlers(bool has)      { HasButtonHandlers=has; }

    virtual ObjectType  GetObjectType() const   { return Object_MovieClipObject; }

    GPtr<GFxSprite>     GetSprite() { return pSprite; }

    void                ForceShutdown();

    virtual GFxASCharacter*         GetASCharacter() { return GPtr<GFxSprite>(pSprite).GetPtr(); }

    GPtr<GASObject>     Exchange__proto__(GASObject* newproto)
    {
        GPtr<GASObject> pproto = pProto;
        pProto = newproto;
        return pproto;
    }

    // Returns mask for the button event name. 0, if not found.
    static UInt16        GetButtonEventNameMask(GASStringContext* psc, const GASString& name);

    // Returns 'true' if the 'name' is a button event name. Actually, the retur
    static bool        IsButtonEventName(GASStringContext* psc, const GASString& name)
    {
        return GetButtonEventNameMask(psc, name) != 0;
    }
};

class GASMovieClipProto : public GASPrototype<GASMovieClipObject>
{
public:
    GASMovieClipProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor);

    //static void GlobalCtor(const GASFnCall& fn);

};

class GASMovieClipCtorFunction : public GASCFunctionObject
{
public:
    GASMovieClipCtorFunction(GASStringContext *psc) : GASCFunctionObject(psc, GlobalCtor) {}

    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;

    static void GlobalCtor(const GASFnCall& fn);
};

#endif // INC_GFXSPRITE_H
