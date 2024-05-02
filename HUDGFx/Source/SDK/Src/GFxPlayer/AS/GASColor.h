/**********************************************************************

Filename    :   AS/GASColor.h
Content     :   Text character implementation
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXCOLOR_H
#define INC_GFXCOLOR_H

#include "GConfig.h"
#ifndef GFC_NO_FXPLAYER_AS_COLOR
#include "GFxCharacter.h"
#include "AS/GASObject.h"

#include "GFxAction.h"
#include "AS/GASObjectProto.h"

// ***** Declared Classes
class GASColorObject;
class GASColorProto;


// ActionScript Color object

class GASColorObject : public GASObject
{
    friend class GASColorProto;
    friend class GASColorCtorFunction;

    GWeakPtr<GFxASCharacter> pCharacter;
#ifndef GFC_NO_GC
protected:
    virtual void Finalize_GC()
    {
        pCharacter.~GWeakPtr<GFxASCharacter>();
        GASObject::Finalize_GC();
    }
#endif //GFC_NO_GC
protected:
    GASColorObject(GASStringContext *psc, GASObject* pprototype) // for prototype 
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype);
    }
public:
    GASColorObject(GASEnvironment* penv, GFxASCharacter *pcharacter)
        : GASObject(penv), pCharacter(pcharacter)
    {        
        Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Color));
    }
    void               SetTarget(GFxASCharacter *pcharacter) { pCharacter = pcharacter; }
    virtual ObjectType GetObjectType() const   { return Object_Color; }
};

class GASColorProto : public GASPrototype<GASColorObject>
{
public:
    GASColorProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor);

    // NOTE: SetTransform allows for negative color transforms (inverse mul/color subtract?).
    // TODO: Such transforms probably won't work in renderer -> needs testing!

    static void SetTransform(const GASFnCall& fn);
    static void GetTransform(const GASFnCall& fn);
    static void SetRGB(const GASFnCall& fn);
    static void GetRGB(const GASFnCall& fn);
};

class GASColorCtorFunction : public GASCFunctionObject
{
public:
    GASColorCtorFunction(GASStringContext *psc) 
        : GASCFunctionObject(psc, GlobalCtor) {}

    static void GlobalCtor(const GASFnCall& fn);
    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;

    static GASFunctionRef Register(GASGlobalContext* pgc);
};
#endif //#ifndef GFC_NO_FXPLAYER_AS_COLOR

#endif // INC_GFXCOLOR_H

