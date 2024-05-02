/**********************************************************************

Filename    :   GFxCharacter.h
Content     :   Defines base functionality for characters, which are
                display list objects.
Created     :   
Authors     :   Michael Antonov, TU

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   More implementation-specific classes are
                defined in the GFxPlayerImpl.h

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXCHARACTER_H
#define INC_GFXCHARACTER_H


#include "GArray.h"
#include "GRefCount.h"
#include "GDebug.h"

#include "GRenderer.h"

#include "GFxLog.h"
#include "GFxPlayer.h"

#include "AS/GASObject.h"

#include "GTypes2DF.h"

#include "GFxCharacterDef.h"
#include "GFxScale9Grid.h"


// ***** Declared Classes
class GFxCharacter;
class GFxASCharacter;
class GFxGenericCharacter;

// ***** External Classes

class GFxMovieRoot;
class GFxMovieDataDef;
class GFxMovieDefImpl;
class GASEnvironment;
struct GFxFilterDesc;

//class GFxRenderConfig;

//#define GFC_USE_OLD_ADVANCE
#if defined(GFC_BUILD_DEBUG) && !defined(GFC_BUILD_DEBUGOPT)
    //#define GFC_TRACE_ADVANCE
    //#define GFC_ADVANCE_EXECUTION_TRACE
#endif //GFC_BUILD_DEBUG

// Used to indicate how focus was transfered
enum GFxFocusMovedType
{
    GFx_FocusMovedByMouse     = 1,
    GFx_FocusMovedByKeyboard  = 2
};


// Forward decl
void GFxASCharacter_MatrixScaleAndRotate2x2(GMatrix2D& m, Float sx, Float sy, Float radians);


// *****  GFxCharacter - Active object or shape on stage


// GFxCharacter is a live, active state instance of a GFxCharacterDef.
// It represents a single active element in a display list and thus includes
// the physical properties necessary for all objects on stage.
// However, it does not include the properties necessary for scripting
// or external access - that is the job of GFxASCharacter.

class GFxCharacter : public GRefCountBaseWeakSupport<GFxCharacter, GFxStatMV_MovieClip_Mem>,
                     public GFxLogBase<GFxCharacter>
{
public:

    // Public typedefs
    typedef GRenderer::Matrix       Matrix;
    typedef GRenderer::Cxform       Cxform;
    typedef GRenderer::BlendType    BlendType;

    typedef GASObjectInterface::ObjectType ObjectType;

protected:

    // These are physical properties of all display list entries.
    // Properties that belong only to scriptable objects, such as _name are in GFxASCharacter.
    GFxResourceId           Id;
    int                     Depth;
    UInt                    CreateFrame;
    Float                   Ratio;
    union   {
        GFxASCharacter*         pParent;        // Parent is always GFxASCharacter.     
        GFxCharacter*           pParentChar;    // So that we can access it directly.
    };

    Cxform                  ColorTransform;
    Matrix                  Matrix_1;   
#ifndef GFC_NO_3D
    GMatrix3DNewable       *pMatrix3D_1;
    GMatrix3DNewable       *pPerspectiveMatrix3D;   // 3D perspective matrix
    GMatrix3DNewable       *pViewMatrix3D;          // 3D view matrix
    Float                   PerspFOV;
#endif
    mutable Float           LastHitTestX;   // TO DO: Revise "mutable". 
    mutable Float           LastHitTestY;   // TO DO: Revise "mutable". 
    
    enum FlagMasks
    {
        Mask_Scale9GridExists   = 0x01,
        Mask_TopmostLevel       = 0x02,
        Mask_HitTestNegative    = 0x04,
        Mask_HitTestPositive    = 0x08,
        Mask_HitTest            = Mask_HitTestNegative | Mask_HitTestPositive,
        Mask_Unloaded           = 0x10, // Set once Unload event has been called.
        Mask_JustLoaded         = 0x20, // Set once char is loaded
        Mask_MarkedForRemove    = 0x40, // Char is marked for remove from the DisplayList 
        Mask_ASCharacter        = 0x80,
        Mask_Unloading          = 0x100, // Set when OnEventUnload is about to be called.
        Mask_TimelineObject     = 0x200
    };
    UInt16                  ClipDepth;
    mutable UInt16          Flags; // TO DO: Revise "mutable". 

    void SetTopmostLevelFlag(bool v = true) { (v) ? Flags |= Mask_TopmostLevel : Flags &= (~Mask_TopmostLevel); }
    void ClearTopmostLevelFlag()            { SetTopmostLevelFlag(false); }
    void SetDirtyFlag();
    virtual void UpdateAlphaFlag() {}

    void SetASCharacterFlag() { Flags |= Mask_ASCharacter; }
public:

    // Constructor
    GFxCharacter(GFxASCharacter* pparent, GFxResourceId id);
    ~GFxCharacter();
    
    // *** Accessors for physical display info

    GFxResourceId           GetId() const                       { return Id; }
    GFxASCharacter*         GetParent() const                   { return pParent; }
    void                    SetParent(GFxASCharacter* parent)   { pParent = parent; }  // for extern GFxMovieSub
    int                     GetDepth() const                    { return Depth; }
    void                    SetDepth(int d)                     { Depth = d; }  
    UInt                    GetCreateFrame() const              { return CreateFrame;}
    void                    SetCreateFrame(UInt frame)          { CreateFrame = frame; }
    const Matrix&           GetMatrix() const                   { return Matrix_1; }
    void                    SetMatrix(const Matrix& m)          { GASSERT(m.IsValid()); Matrix_1 = m;}
    void                    CreateMatrix3D(GMatrix3DNewable** pmat = NULL);
#ifndef GFC_NO_3D
    const GMatrix3D*        GetMatrix3D() const                 { return pMatrix3D_1; }
    void                    SetMatrix3D(const GMatrix3D& m3d)   { GASSERT(m3d.IsValid()); CreateMatrix3D(); *pMatrix3D_1 = m3d; }
    const GMatrix3D*        GetPerspective3D(bool checkAncestors=false) const;
    void                    SetPerspective3D(const GMatrix3D &m3d) { CreateMatrix3D(&pPerspectiveMatrix3D); *pPerspectiveMatrix3D = m3d; }
    const GMatrix3D*        GetView3D(bool checkAncestors=false) const;
    void                    SetView3D(const GMatrix3D &m3d)     { CreateMatrix3D(&pViewMatrix3D); *pViewMatrix3D = m3d; }
    Float                   GetPerspectiveFOV(bool checkAncestors=false) const;
    void                    SetPerspectiveFOV(Float fov);
    bool                    Is3D(bool checkAncestors=false) const;
    virtual bool            Has3D() const                       { return Is3D(); }
    GMatrix3D               GetWorldMatrix3D() const            { GMatrix3D m; GetWorldMatrix3D(&m); return m; }
    void                    GetWorldMatrix3D(GMatrix3D *pmat) const;    // returns complete 2D + 3D world matrix
    GMatrix3D               GetLocalMatrix3D() const;                   // returns complete 2D + 3D local matrix
    virtual void            UpdateViewAndPerspective();
#endif
    const Cxform&           GetCxform() const                   { return ColorTransform; }
    void                    SetCxform(const Cxform& cx)         { ColorTransform = cx; UpdateAlphaFlag(); }
    void                    ConcatenateCxform(const Cxform& cx) { ColorTransform.Concatenate(cx); UpdateAlphaFlag(); }
    void                    ConcatenateMatrix(const Matrix& m)  { Matrix_1.Prepend(m); }
    Float                   GetRatio() const                    { return Ratio; }
    void                    SetRatio(Float f)                   { Ratio = f; }
    UInt16                  GetClipDepth() const                { return ClipDepth; }
    void                    SetClipDepth(UInt16 d)              { ClipDepth = d; }

    void                    SetScale9GridExists(bool v)      
    { 
        (v) ? Flags |= Mask_Scale9GridExists : Flags &= (~Mask_Scale9GridExists); 
    }
    bool                    DoesScale9GridExist() const         { return (Flags & Mask_Scale9GridExists) != 0; }
    bool                    IsTopmostLevelFlagSet() const       { return (Flags & Mask_TopmostLevel) != 0; }

    // Implemented only in GASCharacter. Return None/empty string by default on other characters.
    //virtual const GASString& GetName() const;
    //virtual const GASString& GetOriginalName() const            { return GFxCharacter::GetName(); }     
    virtual bool            GetVisible() const                  { return true; }
    virtual BlendType       GetBlendMode() const                { return GRenderer::Blend_None; }   
    virtual bool            GetContinueAnimationFlag() const    { return false; }
    virtual bool            GetAcceptAnimMoves() const          { return true;  }
    // If any of below ASSERTs is hit, it is either because (1) their states are being set by tags
    // and therefore should be moved from GFxASCharacter here, or (2) because there is a bug
    // in external logic.
    virtual void            SetAcceptAnimMoves(bool accept)     { GUNUSED(accept); GASSERT(0); }
    virtual void            SetBlendMode(BlendType blend)       { GUNUSED(blend);  GASSERT(blend == GRenderer::Blend_None); }
    // Name should not be necessary for text; however, it was seen for static text - needs research.
    virtual void            SetName(const GASString& name)
        { GFC_DEBUG_WARNING1(1, "GFxCharacter::SetName('%s') called on a non-scriptable character", name.ToCStr()); GUNUSED(name); }
    virtual void            SetOriginalName(const GASString& name) { SetName(name); }

    // Text and ASCharacter
    virtual void            SetFilters(const GArray<GFxFilterDesc> f) {}

    bool CheckLastHitResult(Float x, Float y) const; 
    bool WasLastHitPositive() const { return (Flags & Mask_HitTestPositive) != 0; }
    void SetLastHitResult(Float x, Float y, bool result) const; // TO DO: Revise "const"
    void InvalidateHitResult() const;                           // TO DO: Revise "const"

    void            SetUnloaded(bool v = true)   
    { 
        if (!v) SetUnloading(false); // clear "unloading" if clearing "unloaded"
        (v) ? (Flags |= Mask_Unloaded)  : (Flags &= (~Mask_Unloaded)); 
    }
    void            SetJustLoaded(bool v = true) { (v) ? (Flags |= Mask_JustLoaded): (Flags &= (~Mask_JustLoaded)); }
    bool            IsUnloaded() const           { return (Flags & Mask_Unloaded) != 0; }    
    bool            IsJustLoaded() const         { return (Flags & Mask_JustLoaded) != 0; }    

    // Unloading flag is set when OnUnloading is called, thus, OnEventUnload will have this
    // flag already set.
    void            SetUnloading(bool v = true)   
    { 
        (v) ? (Flags |= Mask_Unloading)  : (Flags &= (~Mask_Unloading)); 
    }
    bool            IsUnloading() const          { return (Flags & Mask_Unloading) != 0 || IsUnloadQueuedUp(); }    
    // If sprite is unloading (onUnload is queued up) then it is transferred to the depth = -32769 - curDepth
    // which equals to -1 - curDepth in GFx (we shift depth by +16384)
    bool            IsUnloadQueuedUp() const     { return (Depth < -1); }    

    // *** Inherited transform access.

    // Get our concatenated matrix (all our ancestor transforms, times our matrix).
    // Maps from our local space into "world" space (i.e. root movie clip space).
    void                    GetWorldMatrix(Matrix *pmat) const;
    void                    GetWorldCxform(Cxform *pcxform) const;
    void                    GetLevelMatrix(Matrix *pmat) const;

    // Convenience versions.
    Matrix                  GetWorldMatrix() const              { Matrix m; GetWorldMatrix(&m); return m; }
    Cxform                  GetWorldCxform() const              { Cxform m; GetWorldCxform(&m); return m; }
    Matrix                  GetLevelMatrix() const              { Matrix m; GetLevelMatrix(&m); return m; }
    
    // Temporary - used until blending logic is improved.
    GRenderer::BlendType    GetActiveBlendMode() const;


    // *** Geometry query methods, applicable to all characters.

    // Return character bounds as Axis Aligned rect in specified coordinate space.
    virtual GRectF          GetBounds(const GMatrix3D &t, bool bDivideByW = false) const  
    {   // transform the 2D bounds    
        return t.EncloseTransform(GetBounds(GMatrix2D::Identity), bDivideByW);
    }

    // Return character bounds as 4 points in specified coordinate space.
    virtual void            GetBounds(GPointF *pts, const GMatrix3D &t, bool bDivideByW = false) const  
    {   // transform the 2D bounds    
        t.EncloseTransform(pts, GetBounds(GMatrix2D::Identity), bDivideByW);
    }

    virtual GRectF          GetBounds(const Matrix &t) const     { GUNUSED(t); return GRectF(0); }
    virtual GRectF          GetRectBounds(const Matrix &t) const { GUNUSED(t); return GRectF(0); }
    // Return transformed bounds of character in root movie space.
    GINLINE GRectF          GetWorldBounds() const              { return GetBounds(GetWorldMatrix()); }
    GINLINE GRectF          GetLevelBounds() const              { return GetBounds(GetLevelMatrix()); }
    // get 3D bounds and transform to stage coords
    GRectF                  GetProjectedBounds(const GMatrix3D *pWorldMatrix3D) const;                  
    // get 3D non-axis-aligned bounds and transform to stage coords, bounds returned as an array of 4 pts
    void                    GetProjectedBounds(GPointF *pts, const GMatrix3D *pWorldMatrix3D) const;    

    GFxScale9GridInfo*      CreateScale9Grid(Float pixelScale) const;

    // Hit-testing of shape/sprite.
    enum HitTestMask
    {
        HitTest_TestShape       = 1,
        HitTest_IgnoreInvisible = 2
    };
    virtual bool            PointTestLocal(const GPointF &pt, UInt8 hitTestMask = 0) const 
        { GUNUSED2(pt, hitTestMask); return false; }      
    
    struct TopMostParams
    {
        GFxMovieRoot*           pRoot;
        const GFxASCharacter*   IgnoreMC;
        UInt                    ControllerIdx;
        bool                    TestAll;

        TopMostParams(GFxMovieRoot* proot, 
                      UInt controllerIdx, 
                      bool testAll = false, 
                      const GFxASCharacter* ignoreMC = NULL) 
            : pRoot(proot), IgnoreMC(ignoreMC), ControllerIdx(controllerIdx), TestAll(testAll) {}
    };

    // Will aways find action scriptable objects (parent is returned for regular shapes).
    virtual GFxASCharacter *GetTopMostMouseEntity(const GPointF &pt, const TopMostParams&) 
        { GUNUSED(pt); return NULL; }

    virtual const GFxScale9Grid* GetScale9Grid() const { return 0; }
    virtual void                 PropagateScale9GridExists() {}

    // *** Resource/Asset Accessors
    
    // Returns the absolute root object, which corresponds to movie view and contains levels.
    virtual GFxMovieRoot*   GetMovieRoot() const                
    { 
        return (pParentChar) ? pParentChar->GetMovieRoot() : NULL; 
    }

    // Obtains character definition relying on us. Must be overridden.
    virtual GFxCharacterDef* GetCharacterDef() const            = 0;

    // Returns SWF nested movie definition from which we access movie resources.
    // This definition may have been imported or loaded with 'loadMovie', so
    // it can be a nested movie (it does not need to correspond to root).
    virtual GFxMovieDefImpl* GetResourceMovieDef() const        { return pParentChar->GetResourceMovieDef(); }

    // Obtain the font manager. Font managers exist in sprites with their own GFxMovieDefImpl.
    virtual class GFxFontManager* GetFontManager() const        { return pParentChar->GetFontManager(); }

    // Returns a movie for a certain level.
    virtual GFxASCharacter* GetLevelMovie(SInt level) const     { return pParentChar->GetLevelMovie(level); }

    // Returns _root movie, which may or many not be the same as _level0.
    // The meaning of _root may change if _lockroot is set on a clip into which
    // a nested movie was loaded.
    virtual GFxASCharacter* GetASRootMovie(bool ignoreLockRoot = false) const { return pParentChar->GetASRootMovie(ignoreLockRoot); }

    // Helper that gets movie-resource relative versions.
    // Necessary because version of SWF execution depends on loadMovie nesting.
    UInt                    GetVersion() const;
    GINLINE bool            IsCaseSensitive() const             { return GetVersion() > 6; }

    // ASEnvironment - sometimes necessary for strings.
    virtual GASEnvironment* GetASEnvironment();
    virtual const GASEnvironment* GetASEnvironment() const;

    
    // *** GFxDisplayList required methods  

    // Displays the character onto the renderer, called from display list.
    virtual void            Display(GFxDisplayContext&)             { }
    // Called from DisplayList on ReplaceDisplayObject.
    virtual void            Restart()                               { }
        
    // Advance Frame / "tick" functionality.
    // Set nextFrame = 1 if the frame is advanced by one step, 0 otherwise.
    // framePos must be in range [0,1) indicating where in a frame we are;
    // that could be useful for some animation optimizations.
    virtual void            AdvanceFrame(bool nextFrame, Float framePos = 0.0f) 
        { GUNUSED2(nextFrame, framePos); }

    // ActionScript event handler.  Returns true if a handler was called.
    virtual bool            OnEvent(const GFxEventId& id) { GUNUSED(id); return false; }
    // Special event handler; sprites also execute their frame1 actions on this event.
    virtual void            OnEventLoad()                           { OnEvent(GFxEventId(GFxEventId::Event_Load)); }
    // Special event handler; ensures that unload is called on child items in timely manner.
    virtual void            OnEventUnload();
    virtual bool            OnUnloading() { return true; }
    virtual void            DetachChild(GFxASCharacter* pchild) { GUNUSED(pchild); }
    // Special event handler; key down and up
    // See also PropagateKeyEvent, KeyMask
    virtual bool            OnKeyEvent(const GFxEventId& id, int* pkeyMask)     
        { GUNUSED(pkeyMask); return OnEvent(id); }
    // Special event handler; char
    virtual bool            OnCharEvent(UInt32 wcharCode, UInt controllerIdx)     
        { GUNUSED2(wcharCode, controllerIdx); return false; }
    // Special event handler; mouse wheel support
    virtual bool            OnMouseWheelEvent(int mwDelta)
        { GUNUSED(mwDelta); return false; }

    // Propagates mouse event to all of the eligible clips.
    // Called to notify handlers of onClipEvent(mouseMove, mouseDown, mouseUp)
    virtual void            PropagateMouseEvent(const GFxEventId& id)   
        { OnEvent(id); }

    // Propagates key event to all of the eligible clips.
    // Called to notify handlers of onClipEvent(keyDown, keyUp), onKeyDown/onKeyUp, on(keyPress)
    enum KeyMask {
        KeyMask_KeyPress            = 0x010000,
        KeyMask_onKeyDown           = 0x020000,
        KeyMask_onKeyUp             = 0x040000,
        KeyMask_onClipEvent_keyDown = 0x080000,
        KeyMask_onClipEvent_keyUp   = 0x100000,
        KeyMask_FocusedItemHandledMask = 0xFFFF // should be set, if focused item already handled the keyevent (one bit per controller)
    };
    virtual void            PropagateKeyEvent(const GFxEventId& id, int* pkeyMask)
    { 
        OnKeyEvent(id, pkeyMask); 
    }


    // *** Log support.

    // Implement logging through the root delegation.
    // (needs to be implemented in .cpp, so that GFxMovieRoot is visible)
    virtual GFxLog*         GetLog() const;
    virtual bool            IsVerboseAction() const;
    virtual bool            IsVerboseActionErrors() const;

    // Dynamic casting support - necessary to support ToASCharacter.
    virtual ObjectType      GetObjectType() const   { return GASObjectInterface::Object_BaseCharacter; }
    
    // Methods IsCharASCharacter and CharToASCharacter are similar to 
    // GASObjectInterface's IsASCharacter and ToASCharacter, but they are not virtual in 
    // faster., CharToASCharacter_Un
    GINLINE bool            IsCharASCharacter() const   { return (Flags & Mask_ASCharacter) != 0; }
    GINLINE GFxASCharacter* CharToASCharacter(); 
    GINLINE GFxASCharacter* CharToASCharacter_Unsafe(); // doesn't check for IsASCharacter

    virtual bool            IsUsedAsMask() const { return false; }

    enum StateChangedFlags
    {
        StateChanged_FontLib       = 0,
        StateChanged_FontMap       = 1,
        StateChanged_FontProvider  = 2,
        StateChanged_Translator    = 3
    };
    virtual void SetStateChangeFlags(UInt8 flags) { GUNUSED(flags); }
    virtual UInt8 GetStateChangeFlags() const { return 0; }

    virtual void OnRendererChanged()            { }

    GINLINE bool    IsMarkedForRemove() const   { return ( Flags & Mask_MarkedForRemove) != 0; }
    // Only advance characters if they are not just loaded and not marked for removal.
    GINLINE bool    CanAdvanceChar() const      { return !(Flags & Mask_MarkedForRemove); }

    GINLINE void    MarkForRemove(bool remove)
    {
        if (remove) Flags |= Mask_MarkedForRemove;
        else        Flags &= ~Mask_MarkedForRemove;
    }
    GINLINE void    ClearMarkForRemove() { MarkForRemove(false); }

    void            SetTimelineObjectFlag(bool v)      
    { 
        (v) ? Flags |= Mask_TimelineObject : Flags &= (~Mask_TimelineObject); 
    }
    bool            IsTimelineObjectFlagSet() const     { return (Flags & Mask_TimelineObject) != 0; }
};

// ***** Character Handle


/*  GFxCharacterHandle implementation - stores name and path

    Flash character handles are used to implement the "movieclip" type in ActionScript.
    This type has somewhat mysterious behavior in that it sometimes appears to behave
    as a pointer, while at other times working as a path.

    The most obvious part of this behavior is that it starts out acting as a pointer
    until the pointed-to character dies the first time. This means that during its
    'pointer-like' lifetime different movieclip types can disambiguate references
    to different sprite objects with the same name. Furthermore, it updates itself on
    name changes.
    
    After the original clip object dies, however, the movieclip type remembers its
    global path which it uses to resolve objects. If the name of the object changes,
    the change is not tracked and pointer reference is lost.

    TBD:
    This behavior is not entirely correct. Studio "instance names" seem to have more
    visibility then explicit name changes though AS (test_name_pathconflicts.swf).

    Also, this is not efficient in memory use and will be slow for character accesses
    after the original dies.
*/


class GFxCharacterHandle : public GNewOverrideBase<GFxStatMV_ActionScript_Mem>
{
    friend class GFxASCharacter;

    SInt                        RefCount;       // Custom NTS ref-count implementation to save space.   
    GFxASCharacter*             pCharacter;     // Character, can be null if it was destroyed!
        
    // This name is the same as object reference for all levels except _levelN.
    GASString                   Name;
    // Full Path from root, including _levelN. Used to resolve dead characters.
    GASString                   NamePath;
    // Original placement name; Used to correctly handle life cycle of the character,
    // since the 'Name' might be changed via ActionScript. See GFxSprite::AddDisplayObject.
    GASString                   OriginalName; 

    // Release a character reference, used when character dies
    void                        ReleaseCharacter();

public:
    
    GFxCharacterHandle(const GASString& pname, GFxASCharacter *pparent, GFxASCharacter* pcharacter = 0);
    // Must remove from parent hash and clean parent up.
    ~GFxCharacterHandle();

    GINLINE const GASString&    GetName() const         { return Name; }
    GINLINE const GASString&    GetNamePath() const     { return NamePath; }
    GINLINE GFxASCharacter*     GetCharacter() const    { return pCharacter; }
    GINLINE const GASString&    GetOriginalName() const { return OriginalName; }
    GINLINE void                SetOriginalName(const GASString& on) { OriginalName = on; }

    // Resolve the character, considering path if necessary.
    GFxASCharacter*             ResolveCharacter(GFxMovieRoot *poot) const; 
    GFxASCharacter*             ForceResolveCharacter(GFxMovieRoot *poot) const;

    // Ref-count implementation.
    GINLINE void                AddRef()                { RefCount++; }
    GINLINE void                Release(UInt flags=0)   
    { 
        GUNUSED(flags); 
        RefCount--; 
        if (RefCount<=0) 
            delete this; 
    }

    // Changes the name.
    void                        ChangeName(const GASString& pname, GFxASCharacter* pparent);

    // Resets names to blank ones
    void                        ResetName(GASStringContext* psc);
};



// *****  GFxCharacter - ActionScript controllable object on stage

// GFxASCharacter is a regular character that adds ActionScript control capabilities
// and extra fields, such as _name, which are only available in those objects.

class GFxASCharacter : public GFxCharacter, public GASObjectInterface
{
    friend class GFxSprite;
    friend class GFxButtonCharacter;
#ifndef GFC_USE_OLD_ADVANCE
public:
    // pPlayNext and pPrevPlay
    GFxASCharacter* pPlayNext;
    GFxASCharacter* pPlayPrev;
    union
    {
        GFxASCharacter* pPlayNextOpt;  // next optimized char to play (OptAdvancedList flag is set)
        GFxASCharacter* pNextUnloaded; // next unloaded char (Unloaded flag is set)
    };
#endif
protected:
    // Data binding root for character; store it since there is no binding 
    // information in the corresponding GFxSpriteDef, GFxButtonCharacterDef, etc.
    // Resources for locally loaded handles come from here.
    // Technically, some other GFxCharacter's might need this too, but in most
    // cases they are ok just relying on binding info within GFxDisplayContext.
    GPtr<GFxMovieDefImpl>   pDefImpl;

    mutable GPtr<GFxCharacterHandle> pNameHandle;

public:
    struct GeomDataType : public GNewOverrideBase<GFxStatMV_ActionScript_Mem>
    {
        int     X, Y; // stored in twips
        Double  XScale, YScale;
        Double  Rotation;
        Matrix  OrigMatrix;

#ifndef GFC_NO_3D
        Double  Z;       
        Double  ZScale;                 // this is a percentage, ie. 100
        Double  XRotation, YRotation;   // in degrees
    public:
#endif
        GeomDataType() 
        { 
            X=Y= 0; Rotation= 0; XScale=YScale= 100; 
#ifndef GFC_NO_3D
            Z=XRotation=YRotation=0;
            ZScale=100;
#endif
        }
    } *pGeomData;

    struct CharFilterDesc : public GNewOverrideBase<GFxStatMV_Other_Mem>
    {
        GArray<GFxFilterDesc> Filters;
        bool                  HasFilters;
        bool                  LastIsTemp;

#ifndef GFC_NO_FILTERS_PREPASS
        GPtr<GTexture>        PrePassResult;
        GRectF                FrameRect;
        UInt                  Width, Height;
        GArray<GFxFilterDesc> MainPassFilters;
#endif

        CharFilterDesc(const GArray<GFxFilterDesc>& f) : Filters(f) {}
    };

protected:
    typedef GArrayLH<GASValue, GFxStatMV_ActionScript_Mem> EventsArray;
    GHashLH<GFxEventId, EventsArray, GFxEventIdHashFunctor, GFxStatMV_ActionScript_Mem> EventHandlers;

    enum FlagMasks
    {
        Mask_Visible            = 0x1,
        Mask_Alpha0             = 0x2,   // set, if alpha is zero
        Mask_NoAdvanceLocal     = 0x4,   // disable advance locally
        Mask_NoAdvanceGlobal    = 0x8,   // disable advance globally
        Mask_Enabled            = 0x10,
        Mask_TabEnabled         = 0x60,  // 0 - undef, 1,2 - false, 3 - true
        Mask_FocusRect          = 0x180, // 0 - undef, 1,2 - false, 3 - true
        Mask_UseHandCursor      = 0x600, // 0 - undef, 1,2 - false, 3 - true
        Mask_HitTestDisable     = 0x800,
        Mask_AcceptAnimMoves    = 0x1000,
        Mask_TrackAsMenu        = 0x2000,
        Mask_InstanceBasedName  = 0x4000,
        Mask_UsedAsMask         = 0x8000, // movie is used as a mask for setMask method
        Mask_ChangedFlags       = 0x30000, // State changed flags (see StateChangedFlags)
        Mask_ReqPartialAdvance  = 0x40000, // should get partial advance calls
        Mask_OptAdvancedList    = 0x80000, // is in optimized advance list
        Mask_ContinueAnimation  = 0x100000,// cached extension flag "continueAnimation"
                                           // (it continues timeline anim after AS touch)
        Mask_NeedPreDisplay     = 0x200000,
        
        Shift_ChangedFlags      = 16
    };
    UInt32      Flags;
    SInt16      TabIndex;
    UInt16      FocusGroupMask; // 0 means - take from parent, 0xFFFF - all ctrlrs may focus on
    UInt8       BlendMode;
    UInt8       RollOverCnt; // count of rollovers (for multiple mouse cursors)

    CharFilterDesc  *pFilters;

    void SetVisibleFlag(bool v = true) { (v) ? Flags |= Mask_Visible : Flags &= (~Mask_Visible); }
    void ClearVisibleFlag()            { SetVisibleFlag(false); }
    bool IsVisibleFlagSet() const      { return (Flags & Mask_Visible) != 0; }

    void SetAlpha0Flag(bool v = true)  { (v) ? Flags |= Mask_Alpha0 : Flags &= (~Mask_Alpha0); }
    void ClearAlpha0Flag()             { SetAlpha0Flag(false); }
    bool IsAlpha0FlagSet() const       { return (Flags & Mask_Alpha0) != 0; }

public:
    void SetNoAdvanceLocalFlag(bool v = true)  { (v) ? Flags |= Mask_NoAdvanceLocal : Flags &= (~Mask_NoAdvanceLocal); }
    void ClearNoAdvanceLocalFlag()             { SetNoAdvanceLocalFlag(false); }
    bool IsNoAdvanceLocalFlagSet() const       { return (Flags & Mask_NoAdvanceLocal) != 0; }

    void SetOptAdvancedListFlag(bool v = true) { (v) ? Flags |= Mask_OptAdvancedList : Flags &= (~Mask_OptAdvancedList); }
    void ClearOptAdvancedListFlag()            { SetOptAdvancedListFlag(false); }
    bool IsOptAdvancedListFlagSet() const      { return (Flags & Mask_OptAdvancedList) != 0; }

    void SetReqPartialAdvanceFlag(bool v = true) { (v) ? Flags |= Mask_ReqPartialAdvance : Flags &= (~Mask_ReqPartialAdvance); }
    void ClearReqPartialAdvanceFlag()            { SetReqPartialAdvanceFlag(false); }
    bool IsReqPartialAdvanceFlagSet() const      { return (Flags & Mask_ReqPartialAdvance) != 0; }

    UInt16 GetFocusGroupMask() const;
    UInt16 GetFocusGroupMask();
    virtual bool   IsFocusAllowed(GFxMovieRoot* proot, UInt controllerIdx) const;
    virtual bool   IsFocusAllowed(GFxMovieRoot* proot, UInt controllerIdx);

protected:

    bool IsAdvanceDisabled() const { return (Flags & (Mask_NoAdvanceLocal | Mask_NoAdvanceGlobal)) != 0; }

    void SetNoAdvanceGlobalFlag(bool v = true)  { (v) ? Flags |= Mask_NoAdvanceGlobal : Flags &= (~Mask_NoAdvanceGlobal); }
    void ClearNoAdvanceGlobalFlag()             { SetNoAdvanceGlobalFlag(false); }
    bool IsNoAdvanceGlobalFlagSet() const       { return (Flags & Mask_NoAdvanceGlobal) != 0; }

    // Once moved by script, don't accept tag moves.
    void SetAcceptAnimMovesFlag(bool v = true) { (v) ? Flags |= Mask_AcceptAnimMoves : Flags &= (~Mask_AcceptAnimMoves); }
    void ClearAcceptAnimMovesFlag()            { SetAcceptAnimMovesFlag(false); }
    bool IsAcceptAnimMovesFlagSet() const      { return (Flags & Mask_AcceptAnimMoves) != 0; }

    void SetContinueAnimationFlag(bool v = true)  { (v) ? Flags |= Mask_ContinueAnimation : Flags &= (~Mask_ContinueAnimation); }
    void ClearContinueAnimationFlag()             { SetContinueAnimationFlag(false); }
    bool IsContinueAnimationFlagSet() const       { return (Flags & Mask_ContinueAnimation) != 0; }
    virtual bool GetContinueAnimationFlag() const { return IsContinueAnimationFlagSet(); }

    // Technically TrackAsMenu and Enabled are only available in Sprite and Button,
    // but its convenient to put them here.
    void SetTrackAsMenuFlag(bool v = true) { (v) ? Flags |= Mask_TrackAsMenu : Flags &= (~Mask_TrackAsMenu); }
    void ClearTrackAsMenuFlag()            { SetTrackAsMenuFlag(false); }
    bool IsTrackAsMenuFlagSet() const      { return (Flags & Mask_TrackAsMenu) != 0; }

    void SetEnabledFlag(bool v = true) { (v) ? Flags |= Mask_Enabled : Flags &= (~Mask_Enabled); }
    void ClearEnabledFlag()            { SetEnabledFlag(false); }
    bool IsEnabledFlagSet() const      { return (Flags & Mask_Enabled) != 0; }

    // Set if the instance name was assigned dynamically.
    void SetInstanceBasedNameFlag(bool v = true) { (v) ? Flags |= Mask_InstanceBasedName : Flags &= (~Mask_InstanceBasedName); }
    void ClearInstanceBasedNameFlag()            { SetInstanceBasedNameFlag(false); }
    bool IsInstanceBasedNameFlagSet() const      { return (Flags & Mask_InstanceBasedName) != 0; }

    void SetTabEnabledFlag(bool v = true) 
    { 
        (v) ? (Flags |= Mask_TabEnabled) : Flags = ((Flags & (~Mask_TabEnabled)) | ((Mask_TabEnabled - 1) & Mask_TabEnabled)); 
    }
    void ClearTabEnabledFlag()            { SetTabEnabledFlag(false); }
    void UndefineTabEnabledFlag()         { Flags &= (~Mask_TabEnabled); }
    bool IsTabEnabledFlagDefined() const  { return (Flags & Mask_TabEnabled) != 0; }
    bool IsTabEnabledFlagTrue() const     { return (Flags & Mask_TabEnabled) == Mask_TabEnabled; }
    bool IsTabEnabledFlagFalse() const    { return IsTabEnabledFlagDefined() && !IsTabEnabledFlagTrue(); }

    void SetFocusRectFlag(bool v = true) 
    { 
        (v) ? (Flags |= Mask_FocusRect) : Flags = ((Flags & (~Mask_FocusRect)) | ((Mask_FocusRect - 1) & Mask_FocusRect)); 
    }
    void ClearFocusRectFlag()            { SetFocusRectFlag(false); }
    void UndefineFocusRectFlag()         { Flags &= (~Mask_FocusRect); }
    bool IsFocusRectFlagDefined() const  { return (Flags & Mask_FocusRect) != 0; }
    bool IsFocusRectFlagTrue() const     { return (Flags & Mask_FocusRect) == Mask_FocusRect; }
    bool IsFocusRectFlagFalse() const    { return IsFocusRectFlagDefined() && !IsFocusRectFlagTrue(); }

    void SetUseHandCursorFlag(bool v = true) 
    { 
        (v) ? (Flags |= Mask_UseHandCursor) : Flags = ((Flags & (~Mask_UseHandCursor)) | ((Mask_UseHandCursor - 1) & Mask_UseHandCursor)); 
    }
    void ClearUseHandCursorFlag()            { SetUseHandCursorFlag(false); }
    void UndefineUseHandCursorFlag()         { Flags &= (~Mask_UseHandCursor); }
    bool IsUseHandCursorFlagDefined() const  { return (Flags & Mask_UseHandCursor) != 0; }
    bool IsUseHandCursorFlagTrue() const     { return (Flags & Mask_UseHandCursor) == Mask_UseHandCursor; }
    bool IsUseHandCursorFlagFalse() const    { return IsUseHandCursorFlagDefined() && !IsUseHandCursorFlagTrue(); }

    void SetHitTestDisableFlag(bool v = true) { (v) ? Flags |= Mask_HitTestDisable : Flags &= (~Mask_HitTestDisable); }
    void ClearHitTestDisableFlag()            { SetHitTestDisableFlag(false); }
    bool IsHitTestDisableFlagSet() const      { return (Flags & Mask_HitTestDisable) != 0; }

    void SetUsedAsMask(bool v = true) { (v) ? Flags |= Mask_UsedAsMask : Flags &= (~Mask_UsedAsMask); }
    void ClearUsedAsMask()            { SetUsedAsMask(false); }
    bool IsUsedAsMask() const         { return (Flags & Mask_UsedAsMask) != 0; }

    // Display callbacks
    void                    (*pDisplayCallback)(void*);
    void*                   DisplayCallbackUserPtr;

    void                    UpdateAlphaFlag();

    GFxCharacterHandle*     CreateCharacterHandle() const;
public:

    GeomDataType&           GetGeomData(GeomDataType&) const;
    void                    SetGeomData(const GeomDataType&);

    typedef GASObjectInterface::ObjectType  ObjectType;

    // Constructor.
    GFxASCharacter(GFxMovieDefImpl* pbindingDefImpl, GFxASCharacter* pparent, GFxResourceId id);
    ~GFxASCharacter();

    // Implementation for GFxCharacter optional data.
    virtual BlendType       GetBlendMode() const                { return (BlendType)BlendMode; }
    virtual void            SetBlendMode(BlendType blend)       { BlendMode = (UInt8)blend; SetDirtyFlag(); }
    virtual void            SetName(const GASString& name);
    virtual void            SetOriginalName(const GASString& name);
    virtual void            SetVisible(bool visible)            { SetVisibleFlag(visible); SetDirtyFlag(); }
    virtual bool            GetVisible() const                  { return IsVisibleFlagSet(); }
    bool                    IsDisplayable() const               
    { 
        // Do not display invisible or completely transparent (alpha == 0) objects.
        // But still have to display them, if the object is used as a mask (regardless to
        // alpha and visibility).
        return ((Flags & (Mask_Visible|Mask_Alpha0)) ==  Mask_Visible || (Flags & Mask_UsedAsMask)); 
    }

    GINLINE const GASString& GetName() const;
    const GASString& GetOriginalName() const;

    // Timeline animation support: when (AcceptAnimMoves == 0), timeline has no effect.
    virtual void            SetAcceptAnimMoves(bool accept);
    virtual bool            GetAcceptAnimMoves() const          { return IsAcceptAnimMovesFlagSet(); }
    void                    EnsureGeomDataCreated();

    // Button & Sprite shared vars access.
    GINLINE bool            GetTrackAsMenu() const              { return IsTrackAsMenuFlagSet(); } 
    GINLINE bool            GetEnabled() const                  { return IsEnabledFlagSet();   }

    GINLINE bool            HasInstanceBasedName() const        { return IsInstanceBasedNameFlagSet(); }

    // Determines the absolute path of the character.
    void                    GetAbsolutePath(GString *ppath) const;
    GINLINE GFxCharacterHandle* GetCharacterHandle() const
    {
        if (!pNameHandle)
            return CreateCharacterHandle();
        return pNameHandle;
    }
    
    // Retrieves a global AS context - there is only one in root.
    // GFxCharacter will implement this as { return GetMovieRoot()->pGlobalContext; }
    // This is short because it is used frequently in expressions.
    virtual GASGlobalContext*   GetGC() const;

    // Focus related stuff
    virtual bool            IsTabable() const = 0;
    inline  bool            IsTabIndexed() const { return TabIndex > 0; }
    inline  int             GetTabIndex() const { return TabIndex; }
    // returns true, if yellow focus rect is enabled for the character
    virtual bool            IsFocusRectEnabled() const;
    // should return true, if focus may be set to this character by keyboard or
    // ActionScript.
    virtual bool            IsFocusEnabled() const { return true; }
    // should return true, if focus may be transfered to this character by clicking on
    // left mouse button. An example of such character is selectable textfield.
    virtual bool            DoesAcceptMouseFocus() const { return false; }

    struct FillTabableParams
    {
        GArrayDH<GPtr<GFxASCharacter>, GFxStatMV_Other_Mem>* Array;
        bool    TabIndexed; 
        bool    InclFocusEnabled;
        Bool3W  TabChildrenInProto;
        UInt16  FocusGroupMask;         // current focus group mask
        UInt    ControllerIdx;

        FillTabableParams():TabIndexed(false), InclFocusEnabled(false) {}
    };
    virtual void            FillTabableArray(FillTabableParams* params) { GUNUSED(params); }
    // Special event handler; ensures that unload is called on child items in timely manner.
    virtual void            OnEventUnload();
    virtual bool            OnUnloading();
    enum FocusEventType
    {
        KillFocus,
        SetFocus
    };
    // invoked when lose/gain focus
    virtual void            OnFocus
        (FocusEventType event, GFxASCharacter* oldOrNewFocusCh, UInt controllerIdx, GFxFocusMovedType fmt);

    // invoked when item is going to get focus (Selection.setFocus is invoked, or TAB is pressed).
    virtual void            OnGettingKeyboardFocus(UInt controllerIdx) { GUNUSED(controllerIdx); }
    // invoked when focused item is about to lose keyboard focus input (mouse moved, for example)
    // if returns false, focus will not be transfered.
    virtual bool            OnLosingKeyboardFocus
        (GFxASCharacter* newFocusCh, UInt controllerIdx, GFxFocusMovedType fmt = GFx_FocusMovedByKeyboard) 
    { 
        GUNUSED3(newFocusCh, fmt, controllerIdx); 
        return true; 
    }
    // returns rectangle for focusrect, in local coords
    virtual GRectF          GetFocusRect() const 
    {
        return GetBounds(GRenderer::Matrix());
    }

    virtual void            CloneInternalData(const GFxASCharacter* src);
    virtual void            SetPause(bool pause)    { GUNUSED(pause); }

    // Event handler accessors.
    bool                    HasClipEventHandler(const GFxEventId& id) const;
    bool                    InvokeClipEventHandlers(GASEnvironment* penv, const GFxEventId& id);
    void                    SetSingleClipEventHandler(const GFxEventId& id, const GASValue& method);
    void                    SetClipEventHandlers(const GFxEventId& id, const GASValue& method);

    // From MovieSub: necessary?
    virtual GASObject*      GetASObject ()          { return NULL; }
    virtual GASObject*      GetASObject () const    { return NULL; }    
    virtual bool            OnButtonEvent(const GFxEventId& id)     { return OnEvent(id); }

    virtual GFxASCharacter* GetRelativeTarget(const GASString& name, bool first_call)    
        { GUNUSED2(name, first_call); return 0; } // for buttons and edit box should just return NULL

    virtual void            SetDisplayCallback(void (*callback)(void*), void* userPtr)
    {
        pDisplayCallback = callback;
        DisplayCallbackUserPtr = userPtr;
    }
    virtual void            DoDisplayCallback()
    {
        if (pDisplayCallback)
            (*pDisplayCallback)(DisplayCallbackUserPtr);
    }

    // Utility.
    void                    DoMouseDrag();

    
    // *** Shared ActionScript methods.

    // Depth implementation - same in MovieClip, Button, TextField.
    static void CharacterGetDepth(const GASFnCall& fn);


    // *** Standard ActionScript property support

    
    // StandardMember property constants are used to support built-in properties,
    // to provide common implementation for those properties for which it is
    // identical in multiple character types, and to enable the use of switch()
    // statement in GetMember/SetMember implementations instead of string compares.

    // Standard properties are divided into several categories based on range.
    //
    //  1). [0 - 21] - These are properties directly used by SetProperty and
    //                 GetProperty opcodes in ActionScript. Thus, they cannot 
    //                 be renumbered.
    //
    //  2). [0 - 31] - These properties have conditional flags indicating
    //                 whether default implementation of GFxASCharacter is
    //                 usable for the specific character type.
    //
    //  3). [31+]    - These properties can be in a list, however, they
    //                 cannot have a default implementation.

    enum StandardMember
    {
        M_InvalidMember = -1,
        
        // Built-in property constants. These must come in specified order since
        // they are used by ActionScript opcodes.
        M_x             = 0,
        M_BuiltInProperty_Begin = M_x,
        M_y             = 1,
        M_xscale        = 2,
        M_yscale        = 3,
        M_currentframe  = 4,
        M_totalframes   = 5,
        M_alpha         = 6,
        M_visible       = 7,
        M_width         = 8,
        M_height        = 9,
        M_rotation      = 10,
        M_target        = 11,
        M_framesloaded  = 12,
        M_name          = 13,
        M_droptarget    = 14,
        M_url           = 15,
        M_highquality   = 16,
        M_focusrect     = 17,
        M_soundbuftime  = 18,
        // SWF 5+.
        M_quality       = 19,
        M_xmouse        = 20,
        M_ymouse        = 21,
        M_BuiltInProperty_End= M_ymouse,

        // Extra shared properties which can have default implementation
        M_parent        = 22,       
        M_blendMode     = 23,
        M_cacheAsBitmap = 24,   // B, M
        M_filters       = 25,
        M_enabled       = 26,   // B, M
        M_trackAsMenu   = 27,   // B, M
        M_lockroot      = 28,   // M, however, it can stick around
        M_tabEnabled    = 29,       
        M_tabIndex      = 30,       
        M_useHandCursor = 31,   // B, M
        M_SharedPropertyEnd = M_useHandCursor,
        
        // Properties used by some characters.
        M_menu, // should be shared, not enough bits.
        
        // Movie clip.
        M_focusEnabled,
        M_tabChildren,
        M_transform,
        M_scale9Grid,
        M_hitArea,

        // Dynamic text.
        M_text,
        M_textWidth,
        M_textHeight,
        M_textColor,
        M_length,
        M_html,
        M_htmlText,
        M_styleSheet,
        M_autoSize,
        M_wordWrap,
        M_multiline,
        M_border,
        M_variable,
        M_selectable,
        M_embedFonts,
        M_antiAliasType,
        M_hscroll,
        M_scroll,
        M_maxscroll,
        M_maxhscroll,
        M_background,
        M_backgroundColor,
        M_borderColor,
        M_bottomScroll,
        M_type,
        M_maxChars,
        M_condenseWhite,
        M_mouseWheelEnabled,
        // Input text
        M_password,

        // GFx extensions
        M_shadowStyle,
        M_shadowColor,
        M_hitTestDisable,
        M_noTranslate,
        M_caretIndex,
        M_numLines,
        M_verticalAutoSize,
        M_fontScaleFactor,
        M_verticalAlign,
        M_textAutoSize,
        M_useRichTextClipboard,
        M_alwaysShowSelection, 
        M_selectionBeginIndex,
        M_selectionEndIndex,
        M_selectionBkgColor,
        M_selectionTextColor,
        M_inactiveSelectionBkgColor,
        M_inactiveSelectionTextColor,
        M_noAutoSelection,
        M_disableIME,
        M_topmostLevel,
        M_noAdvance,
        M_focusGroupMask,

        // Dynamic Text
        M_autoFit,
        M_blurX,
        M_blurY,
        M_blurStrength,
        M_outline,
        M_fauxBold,
        M_fauxItalic,
        M_restrict,

        // Dynamic Text Shadow
        M_shadowAlpha,
        M_shadowAngle,
        M_shadowBlurX,
        M_shadowBlurY,
        M_shadowDistance,
        M_shadowHideObject,
        M_shadowKnockOut,
        M_shadowQuality,
        M_shadowStrength,
        M_shadowOutline,

        // 3D
#ifndef GFC_NO_3D
        M_z,
        M_zscale,
        M_xrotation,
        M_yrotation,
        M_matrix3d,
        M_perspfov,
#endif
        M__version,

        M_StandardMemberCount,

        
        // Standard bit masks.

        // Physical transform related members.
        M_BitMask_PhysicalMembers   = (1 << M_x) | (1 << M_y) | (1 << M_xscale) | (1 << M_yscale) |
                                      (1 << M_rotation) | (1 << M_width) | (1 << M_height) | (1 << M_alpha),        
        // Members shared by Button, MovieClip, TextField and Video.
        M_BitMask_CommonMembers     = (1 << M_visible) | (1 << M_parent) | (1 << M_name)        
    };
    struct MemberTableType
    {
        const char *                    pName;
        GFxASCharacter::StandardMember  Id;
        bool                            CaseInsensitive;
    };
    static MemberTableType  MemberTable[];

    // Returns a bit mask of supported standard members withing this character type.
    // GetStandardMember/SetStandardMember methods will fail if the requested constant is not in the bit mask.
    virtual UInt32          GetStandardMemberBitMask() const { return 0; }

    // Looks up a standard member and returns M_InvalidMember if it is not found.
    StandardMember          GetStandardMemberConstant(const GASString& newname) const;
    // Initialization helper - called to initialize member hash in GASGlobalContext
    static void             InitStandardMembers(GASGlobalContext *pcontext);

    static bool             IsStandardMember(const GASString& memberName, GASString* pcaseInsensitiveName = NULL);


    // Handles built-in members. Return 0 if member is not found or not supported.
    virtual bool            SetStandardMember(StandardMember member, const GASValue& val, bool opcodeFlag);
    virtual bool            GetStandardMember(StandardMember member, GASValue* val, bool opcodeFlag) const;



    // GASObjectInterface stuff - for now.
	virtual bool            GetTableMember(GASEnvironment* penv, func_member_value& funceTable, void* pClass, GFxValue* val) { return false;};
    virtual bool            SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags());
    virtual bool            GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
        { GUNUSED3(penv, name, val); GASSERT(0); return false; }
    virtual void            VisitMembers(GASStringContext *psc, MemberVisitor *pvisitor, UInt visitFlags = 0, const GASObjectInterface* instance = 0) const;
    virtual bool            DeleteMember(GASStringContext *psc, const GASString& name);
    virtual bool            SetMemberFlags(GASStringContext *psc, const GASString& name, const UByte flags);
    virtual bool            HasMember(GASStringContext *psc, const GASString& name, bool inclPrototypes);              

    virtual bool            SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags())
    {   GUNUSED(psc);
        return SetMember(GetASEnvironment(), name, val, flags);
    }
    virtual bool            GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val)
    {   GUNUSED(psc);
        return GetMember(GetASEnvironment(), name, val);
    }

    virtual bool            FindMember(GASStringContext *psc, const GASString& name, GASMember* pmember);
    virtual bool            InstanceOf(GASEnvironment* penv, const GASObject* prototype, bool inclInterfaces = true) const;
    virtual bool            Watch(GASStringContext *psc, const GASString& prop, const GASFunctionRef& callback, const GASValue& userData);
    virtual bool            Unwatch(GASStringContext *psc, const GASString& prop);

    virtual void            Set__proto__(GASStringContext *psc, GASObject* protoObj);
    //virtual GASObject*      Get__proto__();

    // set this.__proto__ = psrcObj->prototype
    void                    SetProtoToPrototypeOf(GASObjectInterface* psrcObj);

    // Override these so they don't cause conflicts due to being present in both of our bases.
    virtual ObjectType      GetObjectType() const   { return GFxCharacter::GetObjectType(); }
    
    //GINLINE bool            IsASCharacter() const   { return GASObjectInterface::IsASCharacter(); }
    //GINLINE GFxASCharacter* ToASCharacter()         { return GASObjectInterface::ToASCharacter(); }

    void                    SetFilters(const GArray<GFxFilterDesc> f);
    CharFilterDesc*         GetFilters() const                          { return pFilters; }

    // Returns 0 if nothing to do
    // 1 - if need to add to optimized play list
    // -1 - if need to remove from optimized play list
    virtual int             CheckAdvanceStatus(bool playingNow) { GUNUSED(playingNow); return 0; }
    // Modify the optimized playlist by adding/removing this char from it.
    void                    ModifyOptimizedPlayList(GFxMovieRoot* proot)
    {
        //if (proot->IsOptAdvanceListInvalid()) return;
        switch (CheckAdvanceStatus(IsOptAdvancedListFlagSet()))
        {
        case 1:  AddToOptimizedPlayList(proot);         break;
        case -1: RemoveFromOptimizedPlaylist(proot);    break;
        }
    }
    template <class T>
    void                    ModifyOptimizedPlayListLocal(GFxMovieRoot* proot)
    {
        //if (proot->IsOptAdvanceListInvalid()) return;
        switch (static_cast<T*>(this)->T::CheckAdvanceStatus(IsOptAdvancedListFlagSet()))
        {
        case 1:  AddToOptimizedPlayList(proot);         break;
        case -1: RemoveFromOptimizedPlaylist(proot);    break;
        }
    }
    virtual void            PropagateNoAdvanceGlobalFlag() {}
    virtual void            PropagateNoAdvanceLocalFlag() {}
    virtual void            PropagateFocusGroupMask(UInt mask) { GUNUSED(mask); }


    // *** Parent list manipulation functions (work on all types of characters)

    // Duplicate *this* object with the specified name and add it with a new name 
    // at a new depth in our parent (work only if parent is a sprite).
    GFxASCharacter*     CloneDisplayObject(const GASString& newname, SInt depth, const GASObjectInterface *psource);
    // Remove *this* object from its parent.
    void                RemoveDisplayObject();

    // Adds a child character to playlist.
    void                AddToPlayList(GFxMovieRoot* proot);
    void                AddToOptimizedPlayList(GFxMovieRoot* proot);

    // Removes the character from the playlist
    void                RemoveFromPlaylist(GFxMovieRoot* proot);
    void                RemoveFromOptimizedPlaylist(GFxMovieRoot* proot);

    // *** Movie Loading support

    // Called to substitute poldChar child of this control from new child.
    // This is done to handle loadMovie only; physical properties of the character are copied.
    virtual bool        ReplaceChildCharacterOnLoad(GFxASCharacter *poldChar, GFxASCharacter *pnewChar) 
        { GUNUSED2(poldChar, pnewChar); return 0; }
    // Called to substitute poldChar child of this control from new child.
    // This is done to handle loadMovie with progressive loading only; 
    // physical properties of the character are copied; actions processing is not call
    // because the first frame may not be loaded yet.
    virtual bool        ReplaceChildCharacter(GFxASCharacter *poldChar, GFxASCharacter *pnewChar)
         { GUNUSED2(poldChar, pnewChar); return 0; }

    // Helper for ReplaceChildCharacterOnLoad.
    // Copies physical properties. Does not modify the oldChar
    void                CopyPhysicalProperties(const GFxASCharacter *poldChar);
    // Reassigns character name handle.
    void                MoveNameHandle(GFxASCharacter *poldChar);

    // *** GFxSprite's virtual methods.

    // These timeline related methods are implemented only in GFxSprite; for other
    // ActionScript characters they do nothing. It is convenient to put them here
    // to avoid many type casts in ActionScript opcode implementation.
    virtual UInt            GetCurrentFrame() const             
        { return 0; }   
    virtual bool            GetLabeledFrame(const char* plabel, UInt* frameNumber, bool translateNumbers = 1) 
        { GUNUSED3(plabel, frameNumber, translateNumbers); return 0; }
    virtual void            GotoFrame(UInt targetFrameNumber)   
        { GUNUSED(targetFrameNumber); }
    virtual void            SetPlayState(GFxMovie::PlayState s) 
        { GUNUSED(s); }

    // Execute action buffer
    virtual bool            ExecuteBuffer(GASActionBuffer* pactionBuffer) 
        { GUNUSED(pactionBuffer); return false; }
    // Execute this even immediately (called for processing queued event actions).
    virtual bool            ExecuteEvent(const GFxEventId& id);
    // Execute function
    virtual bool            ExecuteFunction(const GASFunctionRef& function, const GASValueArray& params);
    // Execute ?function
    virtual bool            ExecuteCFunction(const GASCFunctionPtr function, const GASValueArray& params);

    virtual UInt            GetCursorType() const { return GFxMouseCursorEvent::ARROW; }

    virtual void SetStateChangeFlags(UInt8 flags) 
    { 
        Flags = (Flags & ~Mask_ChangedFlags) | ((flags << Shift_ChangedFlags) & Mask_ChangedFlags); 
    }
    virtual UInt8 GetStateChangeFlags() const { return UInt8((Flags & Mask_ChangedFlags) >> Shift_ChangedFlags); }

    virtual void OnRendererChanged();

    UInt IncrementRollOverCnt() { return RollOverCnt++; }
    UInt DecrementRollOverCnt() { return (RollOverCnt != 0) ? --RollOverCnt : ~0u; }

    void                    AddToPreDisplayList(GFxMovieRoot*);
    void                    RemoveFromPreDisplayList(GFxMovieRoot*);
    bool                    IsInPreDisplayList() const { return (Flags & Mask_NeedPreDisplay) != 0; }
};

GINLINE GFxASCharacter* GFxCharacter::CharToASCharacter()
{
    return IsCharASCharacter() ? static_cast<GFxASCharacter*>(this) : 0;
}

GINLINE GFxASCharacter* GFxCharacter::CharToASCharacter_Unsafe()
{
    return static_cast<GFxASCharacter*>(this);
}

GINLINE const GASString& GFxASCharacter::GetName() const
{
    GFxCharacterHandle* pnameHandle = GetCharacterHandle();
    if (pnameHandle)
        return pnameHandle->GetName();
    return GetASEnvironment()->GetBuiltin(GASBuiltin_empty_);
}


// ***** GFxGenericCharacter - character implementation with no extra data

// GFxGenericCharacter is used for non ActionScriptable characters that don't store
// unusual state in their instances. It is used for shapes, morph shapes and static
// text fields.

class GFxGenericCharacter : public GFxCharacter
{
public:
    GFxCharacterDef*    pDef;
   
    GFxGenericCharacter(GFxCharacterDef* pdef, GFxASCharacter* pparent, GFxResourceId id);

    virtual GFxCharacterDef* GetCharacterDef() const
    {
        return pDef;
    }
  
    virtual void    Display(GFxDisplayContext &context)
    {
        pDef->Display(context, this);    // pass in transform info       
    }
    
    // These are used for finding bounds, width and height.
    virtual GRectF  GetBounds(const Matrix &transform) const
    {       
        return transform.EncloseTransform(pDef->GetBoundsLocal());
    }   
    virtual GRectF  GetRectBounds(const Matrix &transform) const
    {       
        return transform.EncloseTransform(pDef->GetRectBoundsLocal());
    }   


    virtual bool    PointTestLocal(const GPointF &pt, UInt8 hitTestMask = 0) const;

    // Override this to hit-test shapes and make Button-Mode sprites work.
    GFxASCharacter*  GetTopMostMouseEntity(const GPointF &pt, const TopMostParams&);
};


#endif // INC_GFXCHARACTER_H
