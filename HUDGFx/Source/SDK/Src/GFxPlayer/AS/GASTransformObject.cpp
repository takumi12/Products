/**********************************************************************

Filename    :   GFxTransform.cpp
Content     :   flash.geom.Transform reference class for ActionScript 2.0
Created     :   6/22/2006
Authors     :   Artyom Bolgar, Prasad Silva
Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "AS/GASTransformObject.h"
#include "GASFunctionRef.h"
#include "AS/GASMatrixObject.h"
#include "AS/GASRectangleObject.h"
#include "AS/GASColorTransform.h"

// GFC_NO_FXPLAYER_AS_TRANSFORM disables Transform class
#ifndef GFC_NO_FXPLAYER_AS_TRANSFORM

#include <stdio.h>
#include <stdlib.h>

// ****************************************************************************
// GASTransform  constructor
//
// Why are we storing the movie root and target handle? Flash resolves 
// characters using it's handle (name). The behavior for the following code in
// ActionScript..
// Frame 1 => (Create an instance mc1)
//            var t1:Transform = new Transform(mc1);
//            var m1:Matrix = new Matrix();
//            m2 = new Matrix(0.5, 0, 0, 1.4, 0, 0);
//            m1.concat(m2);
//            t1.matrix = m1;
// Frame 2 => (Delete mc1)
// Frame 3 => (Create another instance mc1)
//            m1 = t1.matrix;
//            m2 = new Matrix(0.5, 0, 0, 1.4, 0, 0);
//            m1.concat(m2);
//            t1.matrix = m1;
//
// ..is that the second mc1 instance will have the transform t1 applied to it.
//
GASTransformObject::GASTransformObject(GASEnvironment* penv, GFxASCharacter *pcharacter)
: GASObject(penv), pMovieRoot(0)
{
    SetTarget(pcharacter);
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Transform));

#ifndef GFC_NO_FXPLAYER_AS_MATRIX
    Matrix = *static_cast<GASMatrixObject*>
        (penv->OperatorNew(penv->GetGC()->FlashGeomPackage, penv->GetBuiltin(GASBuiltin_Matrix)));
#endif

#ifndef GFC_NO_FXPLAYER_AS_COLORTRANSFORM
    ColorTransform = *static_cast<GASColorTransformObject*>
        (penv->OperatorNew(penv->GetGC()->FlashGeomPackage, penv->GetBuiltin(GASBuiltin_ColorTransform)));
#endif

#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
    PixelBounds = *static_cast<GASRectangleObject*>
        (penv->OperatorNew(penv->GetGC()->FlashGeomPackage, penv->GetBuiltin(GASBuiltin_Rectangle)));
#endif
}

#ifndef GFC_NO_GC
template <class Functor>
void GASTransformObject::ForEachChild_GC(Collector* prcc) const
{
    GASObject::template ForEachChild_GC<Functor>(prcc);
#ifndef GFC_NO_FXPLAYER_AS_MATRIX
    if (Matrix)
        Functor::Call(prcc, Matrix);
#endif
#ifndef GFC_NO_FXPLAYER_AS_COLORTRANSFORM
    if (ColorTransform)
        Functor::Call(prcc, ColorTransform);
#endif
#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
    if (PixelBounds)
        Functor::Call(prcc, PixelBounds);
#endif
}
GFC_GC_PREGEN_FOR_EACH_CHILD(GASTransformObject)

void GASTransformObject::ExecuteForEachChild_GC(Collector* prcc, OperationGC operation) const
{
    GASRefCountBaseType::CallForEachChild<GASTransformObject>(prcc, operation);
}

void GASTransformObject::Finalize_GC()
{
    pMovieRoot = 0;
    TargetHandle.~GPtr<GFxCharacterHandle>();
    GASObject::Finalize_GC();
}
#endif //GFC_NO_GC

void GASTransformObject::SetTarget(GFxASCharacter *pcharacter)
{
    if (pcharacter)
    {
        TargetHandle    = pcharacter->GetCharacterHandle();
        pMovieRoot      = pcharacter->GetMovieRoot();
    }
    else
    {
        pMovieRoot      = NULL;
        TargetHandle    = NULL;
    }
}

// ****************************************************************************
// Intercept the get command so that dependent properties can be recalculated
//
bool GASTransformObject::GetMember(GASEnvironment *penv, const GASString& name, GASValue* val)
{
    if (name == "pixelBounds")
    {

#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
        bool rv = true;        
        if (pMovieRoot)
        {
            GPtr<GFxASCharacter> ch = TargetHandle->ResolveCharacter(pMovieRoot);
            if (ch)
            {
                GRectF bounds = ch->GetBounds(ch->GetMatrix());
                // Flash has 0.05 granularity and will cause the values below to
                // be off.
                GASRect r((GASNumber)gfrnd(TwipsToPixels(bounds.Left)), (GASNumber)gfrnd(TwipsToPixels(bounds.Top)), 
                          GSize<GASNumber>((GASNumber)gfrnd(TwipsToPixels(bounds.Width())), 
                                           (GASNumber)gfrnd(TwipsToPixels(bounds.Height()))));
                PixelBounds->SetProperties(penv, r);
                val->SetAsObject(PixelBounds);
            }
            else 
                rv = false;
        }
        else 
            rv = false;
        if (!rv) val->SetUndefined();
        return rv;
#else
        GFC_DEBUG_WARNING(1, "Rectangle ActionScript class was not included in this GFx build."); 
        val->SetUndefined();
        return false;
#endif
    }
    else if (name == "colorTransform")
    {

#ifndef GFC_NO_FXPLAYER_AS_COLORTRANSFORM
        bool rv = true;        
        if (pMovieRoot)
        {
            GPtr<GFxASCharacter> ch = TargetHandle->ResolveCharacter(pMovieRoot);
            if (ch)
            {
                GASColorTransform ct = ch->GetCxform();
                ColorTransform->SetColorTransform(ct);
                val->SetAsObject(ColorTransform);
            }
            else 
                rv = false;
        }
        else 
            rv = false;
        if (!rv) val->SetUndefined();
        return rv;
#else
        GFC_DEBUG_WARNING(1, "ColorTransform ActionScript class was not included in this GFx build."); 
        val->SetUndefined();
        return false;
#endif
    }
    else if (name == "matrix")
    {
#ifndef GFC_NO_FXPLAYER_AS_MATRIX
        bool rv = true;        
        if (pMovieRoot)
        {
            GPtr<GFxASCharacter> ch = TargetHandle->ResolveCharacter(pMovieRoot);
            if (ch)
            {
                GMatrix2D m = ch->GetMatrix();
                Matrix->SetMatrixTwips(penv->GetSC(), m);
                val->SetAsObject(Matrix);
            }
            else 
                rv = false;
        }
        else 
            rv = false;
        if (!rv) val->SetUndefined();
        return rv;
#else
        GFC_DEBUG_WARNING(1, "Matrix ActionScript class was not included in this GFx build."); 
        val->SetUndefined();
        return false;
#endif
    }
    else if (name == "concatenatedColorTransform")
    {

#ifndef GFC_NO_FXPLAYER_AS_COLORTRANSFORM
        // concatenatedColorTransform should be computed on the fly:
        // "A ColorTransform object representing the combined color transformations 
        // applied to this object and all of its parent objects, back to the root 
        // level. If different color transformations have been applied at different 
        // levels, each of those transformations will be concatenated into one ColorTransform 
        // object for this property."        
        GASColorTransform ct;
        // get the ASCharacter for this transform        
        if (pMovieRoot)
        {
            GPtr<GFxASCharacter> ch = TargetHandle->ResolveCharacter(pMovieRoot);
            if (ch)
            {
                // run through the scene hierarchy and cat the color transforms
                GFxASCharacter* curr = ch.GetPtr();
                while (curr != NULL)
                {
                    ct.Concatenate(curr->GetCxform());
                    curr = curr->GetParent();
                }
            }
        }
        // return a GASObject
        GPtr<GASColorTransformObject> pct = *GHEAP_NEW(penv->GetHeap()) GASColorTransformObject(penv);
        pct->SetColorTransform(ct);
        *val = GASValue(pct.GetPtr());
        return true;
#else
        GFC_DEBUG_WARNING(1, "ColorTransform ActionScript class was not included in this GFx build."); 
        val->SetUndefined();
        return false;    
#endif
    }
    else if (name == "concatenatedMatrix")
    {

#ifndef GFC_NO_FXPLAYER_AS_MATRIX
        // concatenatedMatrix should be computed on the fly:
        // "A Matrix object representing the combined transformation matrices of this 
        // object and all of its parent objects, back to the root level. If different 
        // transformation matrices have been applied at different levels, each of those 
        // matrices will be concatenated into one matrix for this property."
        GMatrix2D mt;
        // get the ASCharacter for this transform        
        if (pMovieRoot)
        {
            GPtr<GFxASCharacter> ch = TargetHandle->ResolveCharacter(pMovieRoot);
            if (ch)
            {
                // run through the scene hierarchy and cat the color transforms
                GFxASCharacter* curr = ch.GetPtr();
                while (curr != NULL)
                {
                    mt.Prepend(curr->GetMatrix());  // or Append?
                    curr = curr->GetParent();
                }
            }
        }
        // return a GASObject
        GPtr<GASMatrixObject> pmt = *GHEAP_NEW(penv->GetHeap()) GASMatrixObject(penv);
        pmt->SetMatrixTwips(penv->GetSC(), mt);
        *val = GASValue(pmt.GetPtr());
        return true;
#else
        GFC_DEBUG_WARNING(1, "Matrix ActionScript class was not included in this GFx build."); 
        val->SetUndefined();
        return false;  
#endif
    }
    return GASObject::GetMember(penv, name, val);
}

// ****************************************************************************
// Intercept the set command so that dependent properties can be recalculated
//
bool    GASTransformObject::SetMember(GASEnvironment *penv, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{
    if (name == "pixelBounds")
    {
        // Does nothing
        return true;
    }
    else if (name == "colorTransform")
    {

#ifndef GFC_NO_FXPLAYER_AS_COLORTRANSFORM
        if (pMovieRoot)
        {
            GPtr<GFxASCharacter> ch = TargetHandle->ResolveCharacter(pMovieRoot);
            if (ch)
            {
                // get GASColorTransform from val
                GPtr<GASObject> obj = val.ToObject(penv);
                if (obj && obj->GetObjectType() == GASObject::Object_ColorTransform)
                {
                    GASColorTransformObject* ctobj = (GASColorTransformObject*)obj.GetPtr();
                    if (ctobj)
                    {
                        GASColorTransform* ct = ctobj->GetColorTransform();
                        ch->SetCxform(*ct);
                        ch->SetAcceptAnimMoves(false);
                    }
                }
            }
        }
        return true;
#else
        GFC_DEBUG_WARNING(1, "ColorTransform ActionScript class was not included in this GFx build."); 
        return false;
#endif
    }
    else if (name == "matrix")
    {

#ifndef GFC_NO_FXPLAYER_AS_MATRIX        
        if (pMovieRoot)
        {
            GPtr<GFxASCharacter> ch = TargetHandle->ResolveCharacter(pMovieRoot);
            if (ch)
            {
                // get GMatrix2D from val
                GPtr<GASObject> obj = val.ToObject(penv);
                if (obj && obj->GetObjectType() == GASObject::Object_Matrix)
                {
                    GASMatrixObject* mobj = (GASMatrixObject*)obj.GetPtr();
                    if (mobj)
                    {
                        GMatrix2D m = mobj->GetMatrix(penv);
                        m.M_[0][2] = PixelsToTwips(m.M_[0][2]);
                        m.M_[1][2] = PixelsToTwips(m.M_[1][2]);
                        ch->SetMatrix(m);

                        GFxASCharacter::GeomDataType geomData;
                        ch->GetGeomData(geomData);
                        geomData.X          = (int)m.GetX();
                        geomData.Y          = (int)m.GetY();
                        geomData.Rotation   = m.GetRotation()*180/GFC_MATH_PI;
                        geomData.XScale     = m.GetXScale()*100;
                        geomData.YScale     = m.GetYScale()*100;
                        ch->SetGeomData(geomData);
                    }
                }
            }
        }
        return true;
#else
        GFC_DEBUG_WARNING(1, "Matrix ActionScript class was not included in this GFx build."); 
        return false;
#endif
    }
    return GASObject::SetMember(penv, name, val, flags);
}

// ****************************************************************************
// AS to GFx function mapping
//
const GASNameFunction GASTransformProto::FunctionTable[] = 
{
    // no functions
    { 0, 0 }
};

// ****************************************************************************
// GASTransform Prototype constructor
//
GASTransformProto::GASTransformProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASTransformObject>(psc, pprototype, constructor)
{
    // we make the functions enumerable
    InitFunctionMembers(psc, FunctionTable, GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete);

    // add the on-the-fly calculated properties
    GASValue undef;
    GASTransformObject::SetMemberRaw(psc, psc->CreateConstString("matrix"), undef, GASPropFlags::PropFlag_DontDelete);
    GASTransformObject::SetMemberRaw(psc, psc->CreateConstString("concatenatedMatrix"), undef, GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_ReadOnly);
    GASTransformObject::SetMemberRaw(psc, psc->CreateConstString("colorTransform"), undef, GASPropFlags::PropFlag_DontDelete);
    GASTransformObject::SetMemberRaw(psc, psc->CreateConstString("concatenatedColorTransform"), undef, GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_ReadOnly);
    GASTransformObject::SetMemberRaw(psc, psc->CreateConstString("pixelBounds"), undef, GASPropFlags::PropFlag_DontDelete);
    //InitFunctionMembers(psc, FunctionTable, pprototype);
}

// ****************************************************************************
// Called when the constructor is invoked for the Transform class
//
void GASTransformCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    GFxASCharacter* ptarget = NULL;
    if (fn.NArgs >= 1)
    {
        ptarget = fn.Env->FindTargetByValue(fn.Arg(0));
        if (NULL == ptarget)
        {
            fn.Result->SetUndefined();
        }
        else
        {
            GPtr<GASTransformObject> ptransform;
            if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == GASObject::Object_Transform && 
                !fn.ThisPtr->IsBuiltinPrototype())
                ptransform = static_cast<GASTransformObject*>(fn.ThisPtr);
            else
                ptransform = *GHEAP_NEW(fn.Env->GetHeap()) GASTransformObject(fn.Env);
            ptransform->SetTarget(ptarget);
            fn.Result->SetAsObject(ptransform.GetPtr());
        }
    }
}

GASObject* GASTransformCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{
    return GHEAP_NEW(penv->GetHeap()) GASTransformObject(penv);
}

GASFunctionRef GASTransformCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef  ctor(*GHEAP_NEW(pgc->GetHeap()) GASTransformCtorFunction(&sc));
    GPtr<GASObject> proto = *GHEAP_NEW(pgc->GetHeap()) GASTransformProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_Transform, proto);
    pgc->FlashGeomPackage->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_Transform), GASValue(ctor));
    return ctor;
}

#else

void GASTransform_DummyFunction() {}   // Exists to quelch compiler warning

#endif  //  GFC_NO_FXPLAYER_AS_TRANSFORM
