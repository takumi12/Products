/**********************************************************************

Filename    :   GFxPlayerStringInvoke.cpp
Content     :   Deprecated string returning variants of 
                GFxMovie::Invoke & GFxMovie::GetVariable
Created     :   
Authors     :   Prasad Silva

Copyright   :   (c) 2009 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include <GFxPlayer.h>
#include "GFxPlayerImpl.h"
#include "GFxSprite.h"
#include "AMP/GFxAmpViewStats.h"

const char*     GFxMovieView::GetVariable(const char* ppathToVar) const
{
    GFxMovieRoot* _this = (GFxMovieRoot*)this;
    if (!_this->pLevel0Movie) return 0;

    // Need to restore high precision mode of FPU for X86 CPUs.
    // Direct3D may set the Mantissa Precision Control Bits to 24-bit (by default 53-bits) and this 
    // leads to bad precision of FP arithmetic. For example, the result of 0.0123456789 + 1.0 will 
    // be 1.0123456789 with 53-bit mantissa mode and 1.012345671653 with 24-bit mode.
    GFxDoublePrecisionGuard dpg;

    GASEnvironment*                 penv = _this->pLevel0Movie->GetASEnvironment();
    GASString                       path(penv->CreateString(ppathToVar));

    GASValue retVal;
    if (penv->GetVariable(path, &retVal))
    {
        _this->pRetValHolder->ResetPos();
        _this->pRetValHolder->ResizeStringArray(0);

        // Store string into a return value holder, so that it is not released till next call.
        // This is thread safe from the multiple GFxMovieRoot point of view.
        UInt pos = _this->pRetValHolder->StringArrayPos++;
        if (_this->pRetValHolder->StringArray.GetSize() < _this->pRetValHolder->StringArrayPos)
            _this->pRetValHolder->ResizeStringArray(_this->pRetValHolder->StringArrayPos);
        _this->pRetValHolder->StringArray[pos] = retVal.ToString(penv);

        return _this->pRetValHolder->StringArray[pos].ToCStr();
    }
    return NULL;
}

const wchar_t*  GFxMovieView::GetVariableStringW(const char* ppathToVar) const
{
    GFxMovieRoot* _this = (GFxMovieRoot*)this;
    if (!_this->pLevel0Movie) return 0;

    // Need to restore high precision mode of FPU for X86 CPUs.
    // Direct3D may set the Mantissa Precision Control Bits to 24-bit (by default 53-bits) and this 
    // leads to bad precision of FP arithmetic. For example, the result of 0.0123456789 + 1.0 will 
    // be 1.0123456789 with 53-bit mantissa mode and 1.012345671653 with 24-bit mode.
    GFxDoublePrecisionGuard dpg;

    GASEnvironment*                 penv = _this->pLevel0Movie->GetASEnvironment();
    GASString                       path(penv->CreateString(ppathToVar));

    GASValue retVal;
    if (penv->GetVariable(path, &retVal))
    {
        _this->pRetValHolder->ResetPos();
        _this->pRetValHolder->ResizeStringArray(0);

        GASString str = retVal.ToString(penv);
        UInt len = str.GetLength();
        wchar_t* pdestBuf = (wchar_t*)_this->pRetValHolder->PreAllocateBuffer((len+1)*sizeof(wchar_t));

        GUTF8Util::DecodeString(pdestBuf,str.ToCStr());
        return pdestBuf;
    }
    return NULL;
}

const char*     GFxMovieView::Invoke(const char* pmethodName, const GFxValue* pargs, UInt numArgs)
{
    GFxMovieRoot* _this = (GFxMovieRoot*)this;
    if (!_this->pLevel0Movie) return 0;

#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer invokeTimer(_this->AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieView_Invoke, Amp_Profile_Level_Low);
#endif

    // Need to restore high precision mode of FPU for X86 CPUs.
    // Direct3D may set the Mantissa Precision Control Bits to 24-bit (by default 53-bits) and this 
    // leads to bad precision of FP arithmetic. For example, the result of 0.0123456789 + 1.0 will 
    // be 1.0123456789 with 53-bit mantissa mode and 1.012345671653 with 24-bit mode.
    GFxDoublePrecisionGuard dpg;

    GASValue resultVal;
    GASEnvironment* penv = _this->pLevel0Movie->GetASEnvironment();
    GASSERT(penv);

    // push arguments directly into environment
    for (int i = (int)numArgs - 1; i >= 0; --i)
    {
        GASSERT(pargs); // should be checked only if numArgs > 0
        const GFxValue& gfxVal = pargs[i];
        GASValue asval;
        _this->GFxValue2ASValue(gfxVal, &asval);
        penv->Push(asval);
    }
    bool retVal;

    // try to resolve alias
    GFxMovieRoot::InvokeAliasInfo* palias;
    if (_this->pInvokeAliases && (palias = _this->ResolveInvokeAlias(pmethodName)) != NULL)
        retVal = _this->InvokeAlias(pmethodName, *palias, &resultVal, numArgs);
    else
        retVal = _this->pLevel0Movie->Invoke(pmethodName, &resultVal, numArgs);
    penv->Drop(numArgs); // release arguments
    if (retVal)
    {
        _this->pRetValHolder->ResetPos();

        // Store string into a return value holder, so that it is not released till next call.
        // This is thread safe from the multiple GFxMovieRoot point of view.
        UInt pos = _this->pRetValHolder->StringArrayPos++;
        if (_this->pRetValHolder->StringArray.GetSize() < _this->pRetValHolder->StringArrayPos)
            _this->pRetValHolder->ResizeStringArray(_this->pRetValHolder->StringArrayPos);
        _this->pRetValHolder->StringArray[pos] = resultVal.ToString(_this->pLevel0Movie->GetASEnvironment());

        return _this->pRetValHolder->StringArray[pos].ToCStr();
    }
    return NULL;    
}
const char*     GFxMovieView::Invoke(const char* pmethodName, const char* pargFmt, ...)
{
    GFxMovieRoot* _this = (GFxMovieRoot*)this;
    if (!_this->pLevel0Movie) return 0;

#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer invokeTimer(_this->AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieView_Invoke, Amp_Profile_Level_Low);
#endif

    // Need to restore high precision mode of FPU for X86 CPUs.
    // Direct3D may set the Mantissa Precision Control Bits to 24-bit (by default 53-bits) and this 
    // leads to bad precision of FP arithmetic. For example, the result of 0.0123456789 + 1.0 will 
    // be 1.0123456789 with 53-bit mantissa mode and 1.012345671653 with 24-bit mode.
    GFxDoublePrecisionGuard dpg;

    GASValue resultVal;
    va_list args;
    va_start(args, pargFmt);

    bool retVal;
    // try to resolve alias
    GFxMovieRoot::InvokeAliasInfo* palias;
    if (_this->pInvokeAliases && (palias = _this->ResolveInvokeAlias(pmethodName)) != NULL)
        retVal = _this->InvokeAliasArgs(pmethodName, *palias, &resultVal, pargFmt, args);
    else
        retVal = _this->pLevel0Movie->InvokeArgs(pmethodName, &resultVal, pargFmt, args);
    va_end(args);
    if (retVal)
    {
        _this->pRetValHolder->ResetPos();

        // Store string into a return value holder, so that it is not released till next call.
        // This is thread safe from the multiple GFxMovieRoot point of view.
        UInt pos = _this->pRetValHolder->StringArrayPos++;
        if (_this->pRetValHolder->StringArray.GetSize() < _this->pRetValHolder->StringArrayPos)
            _this->pRetValHolder->ResizeStringArray(_this->pRetValHolder->StringArrayPos);
        _this->pRetValHolder->StringArray[pos] = resultVal.ToString(_this->pLevel0Movie->GetASEnvironment());

        return _this->pRetValHolder->StringArray[pos].ToCStr();
    }
    return NULL;    
}
const char*     GFxMovieView::InvokeArgs(const char* pmethodName, const char* pargFmt, va_list args)
{
    GFxMovieRoot* _this = (GFxMovieRoot*)this;
    if (_this->pLevel0Movie)
    {
#ifdef GFX_AMP_SERVER
        ScopeFunctionTimer invokeTimer(_this->AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieView_InvokeArgs, Amp_Profile_Level_Low);
#endif

        // Need to restore high precision mode of FPU for X86 CPUs.
        // Direct3D may set the Mantissa Precision Control Bits to 24-bit (by default 53-bits) and this 
        // leads to bad precision of FP arithmetic. For example, the result of 0.0123456789 + 1.0 will 
        // be 1.0123456789 with 53-bit mantissa mode and 1.012345671653 with 24-bit mode.
        GFxDoublePrecisionGuard dpg;

        GASValue resultVal;
        bool retVal;
        // try to resolve alias
        GFxMovieRoot::InvokeAliasInfo* palias;
        if (_this->pInvokeAliases && (palias = _this->ResolveInvokeAlias(pmethodName)) != NULL)
            retVal = _this->InvokeAliasArgs(pmethodName, *palias, &resultVal, pargFmt, args);
        else
            retVal = _this->pLevel0Movie->InvokeArgs(pmethodName, &resultVal, pargFmt, args);
        if (retVal)
        {
            _this->pRetValHolder->ResetPos();

            // Store string into a return value holder, so that it is not released till next call.
            // This is thread safe from the multiple GFxMovieRoot point of view.
            UInt pos = _this->pRetValHolder->StringArrayPos++;
            if (_this->pRetValHolder->StringArray.GetSize() < _this->pRetValHolder->StringArrayPos)
                _this->pRetValHolder->ResizeStringArray(_this->pRetValHolder->StringArrayPos);
            _this->pRetValHolder->StringArray[pos] = resultVal.ToString(_this->pLevel0Movie->GetASEnvironment());

            return _this->pRetValHolder->StringArray[pos].ToCStr();
        }
        return NULL;
    }
    return NULL;    
}

