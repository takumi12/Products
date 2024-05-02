/**********************************************************************

Filename    :   GASFunctionRefImpl.h
Content     :   ActionScript implementation classes
Created     :   
Authors     :   

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXFUNCTIONREFIMPL_H
#define INC_GFXFUNCTIONREFIMPL_H

#include "GFxAction.h"

//
//////////// GASFunctionRefBase //////////////////
//
GINLINE void GASFunctionRefBase::Init(GASFunctionObject* funcObj, UByte flags)
{
    Flags = flags;
    Function = funcObj; 
    if (!(Flags & FuncRef_Weak) && Function)
        Function->AddRef();
    LocalFrame = 0; 
}

GINLINE void GASFunctionRefBase::Init(GASFunctionObject& funcObj, UByte flags)
{
    Flags = flags;
    Function = &funcObj; 
    LocalFrame = 0; 
}

GINLINE void GASFunctionRefBase::Init(const GASFunctionRefBase& orig, UByte flags)
{
    Flags = flags;
    Function = orig.Function; 
    if (!(Flags & FuncRef_Weak) && Function)
        Function->AddRef();
    LocalFrame = 0; 
    if (orig.LocalFrame != 0) 
        SetLocalFrame (orig.LocalFrame, orig.Flags & FuncRef_Internal);
}

GINLINE void GASFunctionRefBase::DropRefs()
{
    if (!(Flags & FuncRef_Weak) && Function)
        Function->Release ();
    Function = 0;
    if (!(Flags & FuncRef_Internal) && LocalFrame != 0)
        LocalFrame->Release ();
    LocalFrame = 0;

}

GINLINE void GASFunctionRefBase::Invoke(const GASFnCall& fn, const char* pmethodName) const
{
    GASSERT (Function);
    Function->Invoke(fn, LocalFrame, pmethodName);
}

GINLINE bool GASFunctionRefBase::operator== (const GASFunctionRefBase& f) const
{
    return Function == f.Function;
}

GINLINE bool GASFunctionRefBase::operator!= (const GASFunctionRefBase& f) const
{
    return Function != f.Function;
}

GINLINE bool GASFunctionRefBase::operator== (const GASCFunctionPtr f) const
{
    if (f == NULL)
        return Function == NULL;
    return IsCFunction() && static_cast<GASCFunctionObject*>(Function)->pFunction == f;
}

GINLINE bool GASFunctionRefBase::operator!= (const GASCFunctionPtr f) const
{
    if (f == NULL)
        return Function != NULL;
    return IsCFunction() && static_cast<GASCFunctionObject*>(Function)->pFunction == f;
}

GINLINE bool GASFunctionRefBase::IsCFunction () const
{
    GASSERT (Function);
    return Function->IsCFunction ();
}

GINLINE bool GASFunctionRefBase::IsAsFunction () const
{
    GASSERT (Function);
    return Function->IsAsFunction ();
}

#endif // INC_GFXFUNCTIONREFIMPL_H

