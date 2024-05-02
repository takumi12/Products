/**********************************************************************

Filename    :   AS/GASTransformObject.h
Content     :   flash.geom.Transform reference class for ActionScript 2.0
Created     :   6/22/2006
Authors     :   Artyom Bolgar, Prasad Silva
Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#ifndef INC_GFXTRANSFORM_H
#define INC_GFXTRANSFORM_H

#include "GRefCount.h"
#include "GFxCharacter.h"
#include "AS/GASObject.h"
#include "GFxPlayerImpl.h"
#include "AS/GASMatrixObject.h"
#include "AS/GASColorTransform.h"
#include "AS/GASRectangleObject.h"
#include "AS/GASObjectProto.h"

// GFC_NO_FXPLAYER_AS_TRANSFORM disables Transform class
#ifndef GFC_NO_FXPLAYER_AS_TRANSFORM

// ***** Declared Classes
class GASTransformObject;
class GASTransformProto;
class GASEnvironment;

// ****************************************************************************
// GAS Transform class
//
class GASTransformObject : public GASObject
{
    friend class GASTransformProto;
    friend class GASTransformCtorFunction;

    GFxMovieRoot*                   pMovieRoot;
    GPtr<GFxCharacterHandle>        TargetHandle;

#ifndef GFC_NO_FXPLAYER_AS_MATRIX
    GPtr<GASMatrixObject>           Matrix;
#endif

#ifndef GFC_NO_FXPLAYER_AS_COLORTRANSFORM
    GPtr<GASColorTransformObject>   ColorTransform;
#endif 

#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
    GPtr<GASRectangleObject>        PixelBounds;
#endif 

protected:
    GASTransformObject(GASStringContext *psc, GASObject* pprototype)
        : GASObject(psc), pMovieRoot(0)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }

#ifndef GFC_NO_GC
protected:
    friend class GRefCountBaseGC<GFxStatMV_ActionScript_Mem>;
    template <class Functor>
    void ForEachChild_GC(Collector* prcc) const;
    virtual void ExecuteForEachChild_GC(Collector* prcc, OperationGC operation) const;
    virtual void Finalize_GC();
#endif //GFC_NO_GC
public:
    GASTransformObject(GASEnvironment* penv, GFxASCharacter *pcharacter = NULL);

    virtual ObjectType GetObjectType() const   { return Object_Transform; }

    // getters and setters
    virtual bool GetMember(GASEnvironment *penv, const GASString &name, GASValue* val);
    virtual bool SetMember(GASEnvironment *penv, const GASString &name, const GASValue &val, const GASPropFlags& flags = GASPropFlags());

    void SetTarget(GFxASCharacter *pcharacter);
};

// ****************************************************************************
// GAS Transform prototype class
//
class GASTransformProto : public GASPrototype<GASTransformObject>
{
public:
    GASTransformProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);

    static const GASNameFunction FunctionTable[];
};

class GASTransformCtorFunction : public GASCFunctionObject
{
public:
    GASTransformCtorFunction(GASStringContext *psc) 
        : GASCFunctionObject(psc, GlobalCtor) {}

    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;    

    static void GlobalCtor(const GASFnCall& fn);

    static GASFunctionRef Register(GASGlobalContext* pgc);
};


#endif // GFC_NO_FXPLAYER_AS_TRANSFORM
#endif // INC_GFXTRANSFORM_H

