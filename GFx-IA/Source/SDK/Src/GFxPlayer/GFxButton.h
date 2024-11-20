/**********************************************************************

Filename    :   GFxButton.h
Content     :   Implementation of Button character and its AS2 Button class
Created     :   
Authors     :   Michael Antonov, Artem Bolgar

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXBUTTON_H
#define INC_GFXBUTTON_H

#include "GRefCount.h"

#include "GFxTags.h"
#include "GFxCharacter.h"
#include "GFxAction.h"
#include "AS/GASObjectProto.h"

// ***** Declared Classes
class GFxMouseState;
class GFxButtonRecord;
class GFxButtonAction;
class GFxButtonCharacterDef;
class GASButtonObject;
class GASButtonProto;
class GFxSoundSampleImpl;

class GFxLoadProcess;

void    GFx_GenerateMouseButtonEvents(UInt mouseIndex, GFxMouseState* ms, UInt checkCount);

//
// button characters
//
class GFxButtonRecord
{
public:
    enum MouseState
    {
        MouseUp,
        MouseDown,
        MouseOver
    };

    GArray<GFxFilterDesc> Filters;            // array of filters, MOOSE change to pointer to array

    GRenderer::Matrix   ButtonMatrix;
    GRenderer::Cxform   ButtonCxform;

    GFxResourceId       CharacterId;    
    UInt16              ButtonLayer;

    GRenderer::BlendType BlendMode;
    enum RecordFlags
    {
        Mask_HitTest = 1,
        Mask_Down    = 2,
        Mask_Over    = 4,
        Mask_Up      = 8
    };
    UInt8               Flags;

    GFxButtonRecord() : Flags(0) {}

    bool    MatchMouseState(MouseState mouseState) const
    {
        if ((mouseState == MouseUp && IsUp())      ||
            (mouseState == MouseDown && IsDown())  ||
            (mouseState == MouseOver && IsOver()) )
            return true;
        return false;
    }

    bool    Read(GFxLoadProcess* p, GFxTagType tagType);

    bool    IsHitTest() const { return (Flags & Mask_HitTest) != 0; }
    bool    IsDown() const    { return (Flags & Mask_Down) != 0; }
    bool    IsOver() const    { return (Flags & Mask_Over) != 0; }
    bool    IsUp() const      { return (Flags & Mask_Up) != 0; }
};


class GFxButtonAction
{
public:
    enum ConditionType
    {
        IDLE_TO_OVER_UP = 1 << 0,
        OVER_UP_TO_IDLE = 1 << 1,
        OVER_UP_TO_OVER_DOWN = 1 << 2,
        OVER_DOWN_TO_OVER_UP = 1 << 3,
        OVER_DOWN_TO_OUT_DOWN = 1 << 4,
        OUT_DOWN_TO_OVER_DOWN = 1 << 5,
        OUT_DOWN_TO_IDLE = 1 << 6,
        IDLE_TO_OVER_DOWN = 1 << 7,
        OVER_DOWN_TO_IDLE = 1 << 8,
    };
    int Conditions;
    GArrayLH<GASActionBufferData*,GFxStatMD_CharDefs_Mem>   Actions;

    ~GFxButtonAction();
    void    Read(GFxStream* in, GFxTagType tagType, UInt actionLength);
};

#ifndef GFC_NO_SOUND
class GFxButtonSoundDef;
#endif // GFC_NO_SOUND

class GFxButtonCharacterDef : public GFxCharacterDef
{
public:

#ifndef GFC_NO_SOUND
    GFxButtonSoundDef*              pSound;
#endif // GFC_NO_SOUND

    bool Menu;
    GArrayLH<GFxButtonRecord,GFxStatMD_CharDefs_Mem>    ButtonRecords;
    GArrayLH<GFxButtonAction,GFxStatMD_CharDefs_Mem>    ButtonActions;
    GFxScale9Grid*                  pScale9Grid;


    GFxButtonCharacterDef();
    virtual ~GFxButtonCharacterDef();

    GFxCharacter*   CreateCharacterInstance(GFxASCharacter* pparent, GFxResourceId id,
                                            GFxMovieDefImpl *pbindingImpl);
    void    Read(GFxLoadProcess* p, GFxTagType tagType);

    // Obtains character bounds in local coordinate space.
    virtual GRectF  GetBoundsLocal() const
    {
        // This is not useful, because button bounds always depend on its state.
        GASSERT(0);
        return GRectF(0);
        /*
        Float h = 0;
        for(UInt i=0; i<ButtonRecords.GetSize(); i++)
            if (!ButtonRecords[i].HitTest)
                h = G_Max<Float>(h, ButtonRecords[i].pCharacterDef->GetHeightLocal());
        return h; */
    }


    void SetScale9Grid(const GRectF& r) 
    {        
        if (pScale9Grid == 0) 
        {
            // This is MovieDef's loaded version of Scale9Grid,
            // so allocate it from the same heap as GFxButtonCharacterDef.
            pScale9Grid = GHEAP_AUTO_NEW(this) GFxScale9Grid;
        }
        *pScale9Grid = r;
    }

    const GFxScale9Grid* GetScale9Grid() const { return pScale9Grid; }

    // *** GFxResource implementation

    // Query Resource type code, which is a combination of ResourceType and ResourceUse.
    virtual UInt            GetResourceTypeCode() const     { return MakeTypeCode(RT_ButtonDef); }
    
};

// ActionScript objects. Should be separated later.

class GASButtonObject : public GASObject
{
    friend class GASButtonProto;
    friend class GASButtonCtorFunction;

    GWeakPtr<GFxASCharacter> pButton;   // weak ref on button obj

    void commonInit ();
#ifndef GFC_NO_GC
protected:
    void Finalize_GC()
    {
        pButton.~GWeakPtr<GFxASCharacter>();
        GASObject::Finalize_GC();
    }
#endif // GFC_NO_GC
protected:

    GASButtonObject(GASStringContext *psc, GASObject* pprototype)
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
        commonInit();
    }
public:
    GASButtonObject(GASGlobalContext* gCtxt, GFxASCharacter* pbutton) 
        : GASObject(gCtxt->GetGC()), pButton(pbutton)
    {
        commonInit ();
        GASStringContext* psc = pbutton->GetASEnvironment()->GetSC();
        //Set__proto__ (psc, gCtxt->GetPrototype(GASBuiltin_Button));
        Set__proto__ (psc, pbutton->Get__proto__());
    }
    GASButtonObject(GASEnvironment *penv) : GASObject(penv)
    { 
        commonInit ();
        Set__proto__ (penv->GetSC(), penv->GetPrototype(GASBuiltin_Button));
    }

    virtual ObjectType      GetObjectType() const   { return Object_ButtonASObject; }

    GFxASCharacter* GetASCharacter() { return GPtr<GFxASCharacter>(pButton).GetPtr(); }
};

class GASButtonProto : public GASPrototype<GASButtonObject>
{
public:
    GASButtonProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor);
};

class GASButtonCtorFunction : public GASCFunctionObject
{
public:
    GASButtonCtorFunction(GASStringContext *psc) : GASCFunctionObject(psc, GlobalCtor) {}

    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;

    static void GlobalCtor(const GASFnCall& fn);

    static GASFunctionRef Register(GASGlobalContext* pgc);
};


#endif // INC_GFXBUTTON_H

