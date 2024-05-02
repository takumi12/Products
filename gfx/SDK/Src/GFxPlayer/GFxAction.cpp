/**********************************************************************

Filename    :   GFxAction.cpp
Content     :   ActionScript 1.0 and 2.0 opcode execution engine core
Created     :   
Authors     :   Artem Bolgar, Michael Antonov

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#include "GFunctions.h"

#include "GFxAction.h"
#include "GFxCharacter.h"
#include "GFxLoaderImpl.h"
#include "GFxLog.h"
#include "GFxStream.h"
#include "GFxRandom.h"

#include "GFxPlayerImpl.h"
#include "GASAsFunctionObject.h"

#include "GFxSprite.h"
#include "GFxText.h"
#include "AS/GASColor.h"
#include "AS/GASTransformObject.h"
#include "AS/GASMatrixObject.h"
#include "AS/GASMovieClipLoader.h"
#include "AS/GASBitmapData.h"
#include "AS/GASPointObject.h"
#include "AS/GASRectangleObject.h"
#include "AS/GASColorTransform.h"
#include "AS/GASStage.h"

#include "GFxASString.h"
#include "AS/GASStringObject.h"

#include "AS/GASArrayObject.h"
#include "AS/GASNumberObject.h"
#include "AS/GASBooleanObject.h"
#include "AS/GASMath.h"
#include "AS/GASTimers.h"
#include "AS/GASAsBroadcaster.h"
#include "AS/GASDate.h"
#include "AS/GASKeyObject.h"
#include "AS/GASMouse.h"
#include "AS/GASExternalInterface.h"
#include "AS/GASSelection.h"
#include "AS/GASAmpMarker.h"
#include "AS/GASTextFormat.h"
#include "AS/GASSharedObject.h"

#include "AS/GASTextSnapshot.h"

#include "IME/GASIme.h"
#include "GFxLoadProcess.h"
#include <GFxIMEManager.h>

#include "GFxFontResource.h"
#include "GFxButton.h"

#include "AS/GASCapabilities.h"
#include "AS/GASLoadVars.h"

#include <GFxXMLSupport.h>

#include "AS/GASBitmapFilter.h"
#include "AS/GASDropShadowFilter.h"
#include "AS/GASGlowFilter.h"
#include "AS/GASBlurFilter.h"
#include "AS/GASBevelFilter.h"
#include "AS/GASColorMatrixFilter.h"

#include "GFxAudio.h"
#include "GSoundRenderer.h"

#include "GMsgFormat.h"
#include "GASFunctionRefImpl.h"

#include "GFxAmpServer.h"
#include "AMP/GFxAmpViewStats.h"

#include <stdio.h>
#include <stdlib.h>


#ifdef GFC_OS_WIN32
#define snprintf _snprintf
#endif // GFC_OS_WIN32

#ifdef GFC_OS_WII
#pragma opt_usedef_mem_limit 250
#endif

// NOTES:
//
// Buttons
// On (press)                 onPress
// On (release)               onRelease
// On (releaseOutside)        onReleaseOutside
// On (rollOver)              onRollOver
// On (rollOut)               onRollOut
// On (dragOver)              onDragOver
// On (dragOut)               onDragOut
// On (keyPress"...")         onKeyDown, onKeyUp      <----- IMPORTANT
//
// Sprites
// OnClipEvent (load)         onLoad
// OnClipEvent (unload)       onUnload                Hm.
// OnClipEvent (enterFrame)   onEnterFrame
// OnClipEvent (mouseDown)    onMouseDown
// OnClipEvent (mouseUp)      onMouseUp
// OnClipEvent (mouseMove)    onMouseMove
// OnClipEvent (keyDown)      onKeyDown
// OnClipEvent (keyUp)        onKeyUp
// OnClipEvent (data)         onData

// Text fields have event handlers too!


///////////////////////////////////////////
// GFxActionLogger
//
GFxActionLogger::GFxActionLogger(GFxCharacter *ptarget, const char* suffixStr)
{
    GFxMovieRoot *proot = ptarget->GetMovieRoot();
    VerboseAction = proot->IsVerboseAction();
    VerboseActionErrors = !proot->IsSuppressActionErrors();
    LogSuffix = suffixStr;

    // Check if it is the main movie
    if (!LogSuffix)
        UseSuffix = false;
    else
    {
        if (G_strcmp(proot->GetMovieDef()->GetFileURL(), LogSuffix) == 0)
            UseSuffix = proot->IsLogRootFilenames();
        else
            UseSuffix = proot->IsLogChildFilenames();
    }

    pLog = proot->GetCachedLog();
    if(UseSuffix)
    {
        //find short filename
        if (!proot->IsLogLongFilenames())
        {
            for (SPInt i = (SPInt)G_strlen(suffixStr) ; i>0; i--) 
            {
                if (LogSuffix[i]=='/' || LogSuffix[i]=='\\') 
                {
                    LogSuffix = LogSuffix+i+1;
                    break;
                }
            }
        }
    }
}

GINLINE void   GFxActionLogger::LogScriptMessageVarg(LogMessageType messageType, const char* pfmt, va_list argList)
{
    if (!pLog)
        return;
    if (UseSuffix)
    { 
        char    fmtBuffer[256];
        UPInt   len = strlen(pfmt);

        G_Format(
            GStringDataPtr(fmtBuffer, sizeof(fmtBuffer)),
            "{0} : {1}\n",
            GStringDataPtr(pfmt, (pfmt[len - 1] == '\n' ? len - 1 : len)),
            LogSuffix
            );

        pLog->LogMessageVarg(messageType,fmtBuffer,argList);
        //pLog->LogScriptError(" : %s\n",LogSuffix);
    }
    else
        pLog->LogMessageVarg(messageType,pfmt,argList);
}

void    GFxActionLogger::LogScriptError(const char* pfmt,...)
{   
    va_list argList; va_start(argList, pfmt);
    LogScriptMessageVarg(Log_ScriptError, pfmt, argList);
    va_end(argList);
}

void    GFxActionLogger::LogScriptWarning(const char* pfmt,...)
{    
    va_list argList; va_start(argList, pfmt);
    LogScriptMessageVarg(Log_ScriptWarning, pfmt, argList);
    va_end(argList);
}

void    GFxActionLogger::LogScriptMessage(const char* pfmt,...)
{  
    va_list argList; va_start(argList, pfmt);
    LogScriptMessageVarg(Log_ScriptMessage, pfmt, argList);
    va_end(argList);
}

GINLINE void    GFxActionLogger::LogDisasm(const unsigned char* instructionData)
{
#ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
    if (pLog && IsVerboseAction())
    {
        GFxDisasm da(pLog, GFxLog::Log_Action);
        da.LogDisasm(instructionData);
    }
#else
    GUNUSED(instructionData);
#endif
}
//
// Function/method dispatch.
//

// FirstArgBottomIndex is the stack index, from the bottom, of the first argument.
// Subsequent arguments are at *lower* indices.  E.G. if FirstArgBottomIndex = 7,
// then arg1 is at env->Bottom(7), arg2 is at env->Bottom(6), etc.
bool        GAS_Invoke(const GASValue& method,
                       GASValue* presult,
                       GASObjectInterface* pthis,
                       GASEnvironment* penv,
                       int nargs,
                       int firstArgBottomIndex,
                       const char* pmethodName)
{
    GASFunctionRef  func = method.ToFunction(penv);
    if (presult) presult->SetUndefined();
    if (func != NULL)
    {
        func.Function->Invoke(GASFnCall(presult, pthis, penv, nargs, firstArgBottomIndex), func.LocalFrame, pmethodName);
        //func.Invoke(GASFnCall(presult, pthis, penv, nargs, firstArgBottomIndex), pmethodName);
        return true;
    }
    else
    {           
        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
        if (penv && penv->IsVerboseActionErrors())
        {
            if (pthis && pthis->IsASCharacter())
                penv->LogScriptError("Error: Invoked method %s.%s is not a function\n",
                    pthis->ToASCharacter()->GetCharacterHandle()->GetNamePath().ToCStr(),
                    (pmethodName ? pmethodName : "<unknown>"));
            else
                penv->LogScriptError("Error: Invoked method %s is not a function\n",
                (pmethodName ? pmethodName : "<unknown>"));
        }
        #endif // #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
    }

    return false;
}

bool        GAS_Invoke(const GASValue& method,
                       GASValue* presult,
                       const GASValue& pthis,
                       GASEnvironment* penv,
                       int nargs,
                       int firstArgBottomIndex,
                       const char* pmethodName)
{
    GASFunctionRef  func = method.ToFunction(penv);
    if (presult) presult->SetUndefined();
    if (func != NULL)
    {
        func.Invoke(GASFnCall(presult, pthis, penv, nargs, firstArgBottomIndex), pmethodName);
        return true;
    }
    else
    {           
        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
        if (penv && penv->IsVerboseActionErrors())
        {
            if (pthis.ToASCharacter(penv))
                penv->LogScriptError("Error: Invoked method %s.%s is not a function\n",
                    pthis.ToASCharacter(penv)->GetCharacterHandle()->GetNamePath().ToCStr(),
                    (pmethodName ? pmethodName : "<unknown>"));
            else
                penv->LogScriptError("Error: Invoked method %s is not a function\n",
                    (pmethodName ? pmethodName : "<unknown>"));
        }
        #endif // #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
    }

    return false;
}

// Printf-like vararg interface for calling ActionScript.
// Handy for external binding.
bool        GAS_InvokeParsed(const char* pmethodName,
                             GASValue* presult,
                             GASObjectInterface* pthis,
                             GASEnvironment* penv,
                             const char* pmethodArgFmt,
                             va_list args)
{
    if (!pmethodName || *pmethodName == '\0')
        return false;

    GFxASCharacter* pnewTarget  = 0;
    GASValue        method, owner;
    if (!penv->GetVariable(penv->CreateString(pmethodName), &method, NULL, &pnewTarget, &owner))
    {
        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
        if (pthis && pthis->IsASCharacter())
            penv->LogScriptError("Error: Can't find method '%s.%s' to invoke.\n", 
                pthis->ToASCharacter()->GetCharacterHandle()->GetNamePath().ToCStr(),
                pmethodName);
        else
            penv->LogScriptError("Error: Can't find method '%s' to invoke.\n", pmethodName);
        #endif // #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
        return false;
    }

    // Check method
    GASFunctionRef  func = method.ToFunction(penv);
    if (func == NULL)
    {           
        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
        if (pthis && pthis->IsASCharacter())
            penv->LogScriptError("Error: Invoked method '%s.%s' is not a function\n", 
                pthis->ToASCharacter()->GetCharacterHandle()->GetNamePath().ToCStr(),
                pmethodName);
        else
            penv->LogScriptError("Error: Invoked method '%s' is not a function\n", pmethodName);
        #endif // #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
        return false;
    }

    // MA: For C functions we need to specify a new target pthis here, if mehodName was nested.
    if (owner.IsCharacter() || owner.IsObject())
        pthis = owner.ToObjectInterface(penv);
    else if (pnewTarget)
        pthis = (GASObjectInterface*)pnewTarget;
    return GAS_InvokeParsed(method, presult, pthis, penv, pmethodArgFmt, args, pmethodName);
}

bool        GAS_InvokeParsed(const GASValue& method,
                             GASValue* presult,
                             GASObjectInterface* pthis,
                             GASEnvironment* penv,
                             const char* pmethodArgFmt,
                             va_list args,
                             const char* pmethodName)
{
    // Parse vaList args
    int  nargs = 0;
    if (pmethodArgFmt)
    {
        int startingIndex = penv->GetTopIndex();
        const char* p = pmethodArgFmt;
        for (;;)
        {
            char    c = *p++;
            if (c == 0)
            {
                // End of args.
                break;
            }
            else if (c == '%')
            {
                c = *p++;
                // Here's an arg.
                if (c == 'd')
                {
                    // Integer.
                    penv->Push(va_arg(args, int));
                }
                else if (c == 'u')
                {
                    // Undefined.
                    penv->Push(GASValue());
                }
                else if (c == 'n')
                {
                    // Null.
                    penv->Push(GASValue::NULLTYPE);
                }
                else if (c == 'b')
                {
                    // Boolean
                    penv->Push(bool(va_arg(args, int) != 0));
                }
                else if (c == 'f')
                {
                    // Double                   
                    penv->Push((GASNumber)va_arg(args, double));

                    // MA: What about float? C specification states that "float converts to
                    // double by the standard argument promotion", so this works. But,
                    // what happens on PS2?  Do we needs '%hf' specifier for float?
                }
                else if (c == 'h')
                {
                    c = *p++;
                    if (c == 'f')
                    {
                        // AB: %hf will be treated as double too, since C spec states
                        // that float is converted to double.
                        // Double                   
                        penv->Push((GASNumber)va_arg(args, double));
                        GFC_DEBUG_WARNING2(1, "InvokeParsed('%s','%s') - '%%hf' will be treated as double", 
                            pmethodName,
                            pmethodArgFmt);
                    }
                    else
                    {
                        penv->LogScriptError("Error: InvokeParsed('%s','%s') - invalid format '%%h%c'\n",
                            pmethodName,
                            pmethodArgFmt,
                            c);
                    }
                }
                else if (c == 's')
                {
                    // String                
                    penv->Push(penv->CreateString(va_arg(args, const char *)));
                }
                else if (c == 'l')
                {
                    c = *p++;
                    if (c == 's')
                    {
                        // Wide string.
                        penv->Push(penv->CreateString(va_arg(args, const wchar_t *)));
                    }
                    else
                    {
                        penv->LogScriptError("Error: InvokeParsed('%s','%s') - invalid format '%%l%c'\n",
                                                pmethodName,
                                                pmethodArgFmt,
                                                c);
                    }
                }
                else
                {
                    // Invalid fmt, warn.
                    penv->LogScriptError("Error: InvokeParsed('%s','%s') - invalid format '%%%c'\n",
                                pmethodName,
                                pmethodArgFmt,
                                c);
                }
            }
            else
            {
                // Invalid arg; warn.
                penv->LogScriptError("Error: InvokeParsed('%s','%s') - invalid char '%c'\n",
                                pmethodName,
                                pmethodArgFmt,
                                c);
            }
            // skip all whitespaces
            for (c = *p; c != '\0' && (c == ' ' || c == '\t' || c == ','); c = *++p) 
                ;
        }
        // Reverse the order of pushed args
        nargs  = penv->GetTopIndex() - startingIndex;
        for (int i = 0; i < (nargs >> 1); i++)
        {
            int i0 = startingIndex + 1 + i;
            int i1 = startingIndex + nargs - i;
            GASSERT(i0 < i1);

            G_Swap((penv->Bottom(i0)), (penv->Bottom(i1)));
        }
    }

    // Do the call.
    bool retVal = GAS_Invoke(method, presult, pthis, penv, nargs, penv->GetTopIndex(), pmethodName);
    penv->Drop(nargs);

    return retVal;
}

// arguments should be ALREADY pushed into the penv's stack!
bool GAS_Invoke(const char* pmethodName,
                GASValue* presult,
                GASObjectInterface* pthis,
                GASEnvironment* penv,
                int numArgs,
                int firstArgBottomIndex)
{
    if (!pmethodName || *pmethodName == '\0')
        return false;

    GFxASCharacter* pnewTarget  = 0;
    GASValue        method, owner;
    if (!penv->GetVariable(penv->CreateString(pmethodName), &method, NULL, &pnewTarget, &owner))
    {
        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
        if (pthis && pthis->IsASCharacter())
            penv->LogScriptError("Error: Can't find method '%s.%s' to invoke.\n", 
                pthis->ToASCharacter()->GetCharacterHandle()->GetNamePath().ToCStr(),
                pmethodName);
        else
            penv->LogScriptError("Error: Can't find method '%s' to invoke.\n", pmethodName);
        #endif // #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
        return false;
    }

    // Check method
    GASFunctionRef  func = method.ToFunction(penv);
    if (func == NULL)
    {           
        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
        if (pthis && pthis->IsASCharacter())
            penv->LogScriptError("Error: Invoked method '%s.%s' is not a function\n", 
                pthis->ToASCharacter()->GetCharacterHandle()->GetNamePath().ToCStr(),
                pmethodName);
        else
            penv->LogScriptError("Error: Invoked method '%s' is not a function\n", pmethodName);
        #endif // #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
        return false;
    }

    // Do the call.
    // MA: For C functions we need to specify a new target pthis here, if mehodName was nested.
    if (owner.IsCharacter() || owner.IsObject())
        pthis = owner.ToObjectInterface(penv);
    else if (pnewTarget)
        pthis = (GASObjectInterface*)pnewTarget;
    return GAS_Invoke(method, presult, pthis, penv, numArgs, firstArgBottomIndex, pmethodName);
}

//
// Built-in objects
//

//
// global init
//
static void    GAS_GlobalTrace(const GASFnCall& fn)
{
    GASSERT(fn.NArgs >= 1);

    // Special case for objects: try the ToString(env) method.
    GASObjectInterface* piobj = fn.Arg(0).ToObjectInterface(fn.Env);
    if (piobj) // OBJECT or CHARACTER
    {
        GASValue method;
        if (piobj->GetMemberRaw(fn.Env->GetSC(),
                                fn.Env->GetBuiltin(GASBuiltin_toString), &method) &&
            method.IsFunction())
        {
            GASValue result;
            GAS_Invoke0(method, &result, piobj, fn.Env);
            
            // Should be just a message
            GASString traceStr(result.ToString(fn.Env));            
            fn.LogScriptMessage("%s\n", traceStr.ToCStr());
            return;
        }
    }

    // Log our argument.
    //
    // @@ what if we get extra args?
    GASString arg0 = fn.Arg(0).ToString(fn.Env);
    char    buffStr[2000];
    size_t  szToCopy = (arg0.GetSize() < sizeof(buffStr)) ? arg0.GetSize() : sizeof(buffStr) - 1;

    // replace '\r' by '\n'
    G_strncpy(buffStr, sizeof(buffStr), arg0.ToCStr(), szToCopy);
    buffStr[szToCopy] = '\0';
    for (char* p = buffStr; *p != 0; ++p)
    {
        if (*p == '\r')
            *p = '\n';
    }
    if (arg0.GetSize() < sizeof(buffStr))
        fn.LogScriptMessage("%s\n", buffStr);
    else
        fn.LogScriptMessage("%s ...<truncated>\n", buffStr);
}

static void    GAS_GlobalParseInt(const GASFnCall& fn)
{       
    if (fn.NArgs < 1)
    {
        // return undefined
        return;
    }

    int         radix = 10;
    GASString   str(fn.Arg(0).ToString(fn.Env));
    UInt        len = str.GetSize();
    UInt        offset = 0;

    if (fn.NArgs >= 2)
    {
        radix = fn.Arg(1).ToInt32(fn.Env);
        if ((radix < 2) || (radix > 36))
        {
            // Should be NaN!!
            fn.Result->SetNumber(GASNumberUtil::NaN());
            return;
        }
    }
    else
    {
        // Need to skip whitespaces; but flash doesn't for HEX!
        //while ((str[offset] == ' ') && (offset<len))
        //  offset++;

        if (len > 1)
        {
            if (str[offset] == '0')
            {
                offset++;
                radix = 8;
                if (len > offset)
                {
                    if ((str[offset] == 'x') || (str[offset] == 'X'))
                    {
                        radix = 16;
                        offset++;
                    }
                }
            }
        }
    }
    
    // Convert to number.
    const char* pstart = str.ToCStr() + offset;
    char *      pstop  = 0;    
    long        result = strtol(str.ToCStr() + offset, &pstop, radix);

    // If text was not parsed or empty, return NaN.
    // If only "0x" is parsed - return NaN
    // If something like "0abc" or "0.2333" (looks like radix is 8) was parsed - return 0;
    if (pstart != pstop || radix == 8)
        fn.Result->SetInt(result);
    else
        fn.Result->SetNumber(GASNumberUtil::NaN());
}

static void    GAS_GlobalParseFloat(const GASFnCall& fn)
{       
    if (fn.NArgs < 1) // return undefined
        return;     
    // Convert.    
    GASString sa0(fn.Arg(0).ToString(fn.Env));
    const char* pstart = sa0.ToCStr();
    char *      pstop  = 0;
    GASNumber   result = (GASNumber)G_strtod(sa0.ToCStr(), &pstop);
    // If digits were consumed, report result.
    fn.Result->SetNumber((pstart != pstop) ? result : GASNumberUtil::NaN());
}

static void    GAS_GlobalIfFrameLoaded(const GASFnCall& fn)
{    
    if (fn.NArgs < 1) // return undefined
        return;     

    // Mark frame as not loaded initially.
    fn.Result->SetBool(false);

    GFxSprite* psprite = NULL;
    if (fn.ThisPtr == NULL)    
        psprite = (GFxSprite*) fn.Env->GetTarget();    
    else if (fn.ThisPtr->IsSprite())        
        psprite = (GFxSprite*) fn.ThisPtr;

    if (psprite)
    {
        // If frame is within range, mark it as loaded.
        SInt frame = fn.Arg(0).ToInt32(fn.Env);
        if (frame < (SInt)psprite->GetLoadingFrame())
            fn.Result->SetBool(true);
    }
}


static void    GAS_GlobalIsNaN(const GASFnCall& fn)
{       
    // isNaN() with no arguments should return true.
    if (fn.NArgs < 1)
        fn.Result->SetBool(true);
    else
        fn.Result->SetBool(GASNumberUtil::IsNaN(fn.Arg(0).ToNumber(fn.Env)));   
}

static void    GAS_GlobalIsFinite(const GASFnCall& fn)
{       
    // isFinite() with no arguments should return false.
    if (fn.NArgs < 1)
        fn.Result->SetBool(false);
    else    
    {
        GASNumber val = fn.Arg(0).ToNumber(fn.Env);

        if (GASNumberUtil::IsNaN(val) ||
            GASNumberUtil::IsNEGATIVE_INFINITY(val) ||
            GASNumberUtil::IsPOSITIVE_INFINITY(val))
            fn.Result->SetBool(false);
        else
            fn.Result->SetBool(true);
    }
}


// ASSetPropFlags function
static void    GAS_GlobalASSetPropFlags(const GASFnCall& fn)
{
    UInt version = fn.Env->GetVersion();
    
    // Check the arguments
    GASSERT(fn.NArgs == 3 || fn.NArgs == 4);
    GASSERT((version == 5) ? (fn.NArgs == 3) : true);
    
    GASObjectInterface* const obj = fn.Arg(0).ToObjectInterface(fn.Env);
    if (!obj)
        return;
    
    // list of child names
    const GASValue& arg1 = fn.Arg(1);
    GPtr<GASArrayObject> props;
    
    if (arg1.IsString ())
    {
        // special case, if the properties are specified as a comma-separated string
        props = GASStringProto::StringSplit(fn.Env, arg1.ToString(fn.Env), ",");
    }
    else if (arg1.IsObject ()) 
    {
        GASObject* _props = fn.Arg(1).ToObject(fn.Env);
        if (_props == NULL)
        {
            GASSERT(fn.Arg(1).GetType() == GASValue::NULLTYPE);
        }
        else if (_props->GetObjectType () == GASObject::Object_Array)
            props = static_cast<GASArrayObject*>(_props);
        else if (_props->GetObjectType () == GASObject::Object_String)
            props = GASStringProto::StringSplit(fn.Env, arg1.ToString(fn.Env), ",");
        else
            return;
    }
    else if (!arg1.IsNull ())
        return;
    // a number which represents three bitwise flags which
    // are used to determine whether the list of child names should be hidden,
    // un-hidden, protected from over-write, un-protected from over-write,
    // protected from deletion and un-protected from deletion
    UByte setTrue = (UByte) (fn.Arg(2).ToInt32(fn.Env) & GASPropFlags::PropFlag_Mask);
    
    // Is another integer bitmask that works like setTrue,
    // except it sets the attributes to false. The
    // setFalse bitmask is applied before setTrue is applied
    
    // ASSetPropFlags was exposed in Flash 5, however the fourth argument 'setFalse'
    // was not required as it always defaulted to the value '~0'.
    UByte setFalse = (UByte) ((fn.NArgs == 3 ?
        (version == 5 ? ~0 : 0) : fn.Arg(3).ToUInt32(fn.Env))
        & GASPropFlags::PropFlag_Mask);

    GASStringContext* psc = fn.Env->GetSC();
    
    if (!props)
    {
        // Take all the members of the object
        struct MemberVisitor : GASObjectInterface::MemberVisitor
        {
            GASObjectInterface* obj;
            GASStringContext*   pSC;
            UByte               SetTrue, SetFalse;

            MemberVisitor(GASStringContext* psc, GASObjectInterface* _obj, UByte setTrue, UByte setFalse)
                : obj(_obj), pSC(psc), SetTrue(setTrue), SetFalse(setFalse) {}

            virtual void Visit(const GASString& name, const GASValue&, UByte flags)
            {
                GASPropFlags fl(flags);
                fl.SetFlags(SetTrue, SetFalse);
                obj->SetMemberFlags (pSC, name, fl.Flags);
            }
        } visitor (psc, obj, setTrue, setFalse);

        obj->VisitMembers(psc, &visitor, GASObject::VisitMember_DontEnum | GASObject::VisitMember_NamesOnly);
    }
    else 
    {
        for (int i = 0, n = props->GetSize(); i < n; ++i)
        {
            const GASValue* elem = props->GetElementPtr(i);
            if (elem == 0)
                continue;

            GASString key = elem->ToString(fn.Env);
            GASMember member;
            if (obj->FindMember(psc, key, &member))
            {
                GASPropFlags fl = member.GetMemberFlags();
                fl.SetFlags(setTrue, setFalse);

                obj->SetMemberFlags(psc, key, fl.Flags);
            }
        }
    }
}

// ASnative, class 800, function 2 - returns mouse button state
static void    GAS_ASnativeMouseButtonStates(const GASFnCall& fn)
{
    if (fn.NArgs >= 1)
    {
        UInt mask = fn.Arg(0).ToUInt32(fn.Env);
        fn.Result->SetBool((fn.Env->GetMovieRoot()->GetMouseState(0)->GetButtonsState() & mask) == mask);
    }
}

// undocumented function ASnative. 
// Now, we support only class 800, function 2 - returns mouse button state
static void    GAS_GlobalASnative(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    if (fn.NArgs >= 2)
    {
        UInt classId = (UInt)fn.Arg(0).ToUInt32(fn.Env);
        UInt funcId  = (UInt)fn.Arg(1).ToUInt32(fn.Env);
        if (classId == 800 && funcId == 2)
        {
            GPtr<GASCFunctionObject> func = *GHEAP_NEW(fn.Env->GetHeap()) 
                GASCFunctionObject(fn.Env->GetSC(), GAS_ASnativeMouseButtonStates);
            fn.Result->SetAsFunction(GASFunctionRef(func));
        }
    }
}

void GASGlobalContext::EscapeWithMask(const char* psrc, UPInt length, GString* pescapedStr, const UInt* escapeMask)
{   
    GASSERT(pescapedStr);

    char            buf[256];
    char*           pbuf = buf;
    char* const     endp = pbuf + sizeof (buf) - 1;    

    for (UInt i = 0; i < length; ++i)
    {
        int ch = (unsigned char)psrc[i];
        if (pbuf + 4 >= endp)
        {
            // flush
            *pbuf = '\0';
            *pescapedStr += buf;
            pbuf = buf;
        }
        if (ch < 128 && (escapeMask[ch/32]&(1u<<(ch%32)))) // isalnum (ch)
        {
            *pbuf++ = char(ch);
        }
        else 
        {
            *pbuf++ = '%';
            *pbuf++ = char(ch/16 + ((ch/16 <= 9) ? '0' : 'A'-10));
            *pbuf++ = char(ch%16 + ((ch%16 <= 9) ? '0' : 'A'-10));
        }
    }
    // flush
    *pbuf = '\0';
    *pescapedStr += buf;
}

void GASGlobalContext::Escape(const char* psrc, UPInt length, GString* pescapedStr)
{
    // each bit in mask represent the condition "isalnum(char) == 1"
    // mask positioning is as follows: mask[char/32]&(1<<(char%32))
    static const UInt mask[] = { 0x00000000, 0x03FF0000, 0x07FFFFFE, 0x07FFFFFE };
    EscapeWithMask(psrc, length, pescapedStr, mask);
}

void GASGlobalContext::EscapePath(const char* psrc, UPInt length, GString* pescapedStr)
{
    // each bit in mask represent the condition "isalnum(char) || char == '\\' || char == '/' || char == '.' || char == ':'"
    // mask positioning is as follows: mask[char/32]&(1<<(char%32))
    static const UInt mask[] = { 0x00000000, 0x07FFC000, 0x17FFFFFE, 0x07FFFFFE };
    EscapeWithMask(psrc, length, pescapedStr, mask);
}

void GASGlobalContext::Unescape(const char* psrc, UPInt length, GString* punescapedStr)
{   
    GASSERT(punescapedStr);

    char                buf[256];
    const char* const   endcstr = psrc + length;
    char*               pbuf = buf;
    char* const         endp = pbuf + sizeof (buf) - 1;

    while (psrc < endcstr)
    {
        int ch = (unsigned char) *psrc++;
        if (pbuf + 1 >= endp)
        {
            // flush
            *pbuf = '\0';
            *punescapedStr += buf;
            pbuf = buf;
        }
        if (ch == '%')
        {
            int fd = G_toupper((unsigned char)*psrc++);
            int sd = G_toupper((unsigned char)*psrc++);
            fd = (fd - '0' > 9) ? fd - 'A' + 10 : fd - '0';
            sd = (sd - '0' > 9) ? sd - 'A' + 10 : sd - '0';
            if (fd < 16 && sd < 16)
            {
                *pbuf++ = char(fd*16 + sd);
            }
        }
        else 
        {
            *pbuf++ = char(ch);
        }
    }
    // flush
    *pbuf = '\0';
    *punescapedStr += buf;
}

static void GAS_GlobalEscape(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    if (fn.NArgs == 1)
    {
        GASString str(fn.Arg(0).ToString(fn.Env));
        GString escapedStr;
        GASGlobalContext::Escape(str.ToCStr(), str.GetSize(), &escapedStr);
        fn.Result->SetString(fn.Env->CreateString(escapedStr));
    }
}
static void GAS_GlobalUnescape(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    if (fn.NArgs == 1)
    {
        GASString str(fn.Arg(0).ToString(fn.Env));
        GString unescapedStr;
        GASGlobalContext::Unescape(str.ToCStr(), str.GetSize(), &unescapedStr);
        fn.Result->SetString(fn.Env->CreateString(unescapedStr));
    }
}


static void GAS_GlobalEscapeSpecialHTML(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    if (fn.NArgs == 1)
    {
        GASString str(fn.Arg(0).ToString(fn.Env));
        GString escapedStr;
        GString::EscapeSpecialHTML(str.ToCStr(), str.GetLength(), &escapedStr);
        fn.Result->SetString(fn.Env->CreateString(escapedStr));
    }
}

static void GAS_GlobalUnescapeSpecialHTML(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    if (fn.NArgs == 1)
    {
        GASString str(fn.Arg(0).ToString(fn.Env));
        GString unescapedStr;
        GString::UnescapeSpecialHTML(str.ToCStr(), str.GetLength(), &unescapedStr);
        fn.Result->SetString(fn.Env->CreateString(unescapedStr));
    }
}


static void GAS_GlobalUpdateAfterEvent(const GASFnCall& fn)
{
    GUNUSED(fn);
    // do nothing!
}

static void GAS_GlobalScreenToWorld(const GASFnCall& fn)
{
    if (fn.NArgs == 1)
    {
        // This is inefficient; find a better way than using the hash table
        GFxMovieRoot* proot     = fn.Env->GetMovieRoot();
        GPtr<GASObject> pobj = fn.Arg(0).ToObject(fn.Env);
        if (pobj)
        {
            GPointF pt;
            GPoint3F ptout;
            GASValue val;
            bool bSet = false;
            if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("x"), &val))
            {
                GASNumber n = val.ToNumber(fn.Env);
                pt.x = ((Float)n - proot->ViewOffsetX)/proot->ViewScaleX;           // remap back to window (mouse) coords
                bSet = true;
            }
            if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("y"), &val))
            {
                GASNumber n = val.ToNumber(fn.Env);
                pt.y = ((Float)n - proot->ViewOffsetY)/proot->ViewScaleY;           // remap back to window (mouse) coords
                bSet = true;
            }
            if (bSet && proot->HitTest3D(&ptout, pt.x, pt.y))
            {
                pobj->SetMember(fn.Env, fn.Env->CreateConstString("x"), ptout.x);
                pobj->SetMember(fn.Env, fn.Env->CreateConstString("y"), ptout.y);
                pobj->SetMember(fn.Env, fn.Env->CreateConstString("z"), ptout.z);
            }
        }
    }
}

#ifndef GFC_NO_IME_SUPPORT
static void GAS_GlobalIMECommand(const GASFnCall& fn)
{   
    if (fn.NArgs >= 2)
    {
        GFxASCharacter* const poriginalTarget   = fn.Env->GetTarget();   
        GFxMovieRoot* proot                     = poriginalTarget->GetMovieRoot();
        GPtr<GFxIMEManager> pimeMgr             = proot->GetIMEManager();
        if (pimeMgr)
        {
            pimeMgr->IMECommand(proot, fn.Arg(0).ToString(fn.Env).ToCStr(), fn.Arg(1).ToString(fn.Env).ToCStr());
        }
    }
}

static void GAS_SetIMECandidateListStyle(const GASFnCall& fn)
{     

    if (fn.NArgs >= 1)
    {
        GFxASCharacter* const poriginalTarget   = fn.Env->GetTarget();   
        GFxMovieRoot* proot                     = poriginalTarget->GetMovieRoot();
        GPtr<GFxIMEManager> pimeMgr             = proot->GetIMEManager();
        if (pimeMgr)
        {
            GPtr<GASObject> pobj = fn.Arg(0).ToObject(fn.Env);
            if (pobj)
            {
                GFxIMECandidateListStyle st;
                GASValue val;
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("textColor"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetTextColor((UInt32)n);
                }
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("backgroundColor"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetBackgroundColor((UInt32)n);
                }
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("indexBackgroundColor"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetIndexBackgroundColor((UInt32)n);
                }
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("selectedTextColor"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetSelectedTextColor((UInt32)n);
                }
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("selectedTextBackgroundColor"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetSelectedBackgroundColor((UInt32)n);
                }
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("selectedIndexBackgroundColor"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetSelectedIndexBackgroundColor((UInt32)n);
                }
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("fontSize"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetFontSize((UInt)n);
                }

                // Reading window parameters.
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("readingWindowTextColor"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetReadingWindowTextColor((UInt)n);
                }
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("readingWindowBackgroundColor"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetReadingWindowBackgroundColor((UInt)n);
                }
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("readingWindowFontSize"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetReadingWindowFontSize((UInt)n);
                }
                pimeMgr->SetCandidateListStyle(st);
            }
        }
    }
}

static void GAS_GetIMECandidateListStyle(const GASFnCall& fn)
{       
    GFxASCharacter* const poriginalTarget   = fn.Env->GetTarget();   
    GFxMovieRoot* proot                     = poriginalTarget->GetMovieRoot();
    GPtr<GFxIMEManager> pimeMgr             = proot->GetIMEManager();
    if (pimeMgr)
    {
        GFxIMECandidateListStyle st; 
        if (pimeMgr->GetCandidateListStyle(&st))
        {
            GPtr<GASObject> pobj = *GHEAP_NEW(fn.Env->GetHeap()) GASObject(fn.Env);
            if (st.HasTextColor())
            {
                GASNumber c = (GASNumber)(st.GetTextColor() & 0xFFFFFFu);
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "textColor", GASValue(c));
            }
            if (st.HasBackgroundColor())
            {
                GASNumber c = (GASNumber)(st.GetBackgroundColor() & 0xFFFFFFu);
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "backgroundColor", GASValue(c));
            }
            if (st.HasIndexBackgroundColor())
            {
                GASNumber c = (GASNumber)(st.GetIndexBackgroundColor() & 0xFFFFFFu);
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "indexBackgroundColor", GASValue(c));
            }
            if (st.HasSelectedTextColor())
            {
                GASNumber c = (GASNumber)(st.GetSelectedTextColor() & 0xFFFFFFu);
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "selectedTextColor", GASValue(c));
            }
            if (st.HasSelectedBackgroundColor())
            {
                GASNumber c = (GASNumber)(st.GetSelectedBackgroundColor() & 0xFFFFFFu);
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "selectedTextBackgroundColor", GASValue(c));
            }
            if (st.HasSelectedIndexBackgroundColor())
            {
                GASNumber c = (GASNumber)(st.GetSelectedIndexBackgroundColor() & 0xFFFFFFu);
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "selectedIndexBackgroundColor", GASValue(c));
            }
            if (st.HasFontSize())
            {
                GASNumber c = (GASNumber)st.GetFontSize();
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "fontSize", GASValue(c));
            }
            // Reading Window Styles

            if (st.HasReadingWindowTextColor())
            {
                GASNumber c = (GASNumber)st.GetReadingWindowTextColor();
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "readingWindowTextColor", GASValue(c));
            }

            if (st.HasReadingWindowBackgroundColor())
            {
                GASNumber c = (GASNumber)st.GetReadingWindowBackgroundColor();
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "readingWindowBackgroundColor", GASValue(c));
            }

            if (st.HasReadingWindowFontSize())
            {
                GASNumber c = (GASNumber)st.GetReadingWindowFontSize();
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "readingWindowFontSize", GASValue(c));
            }

            fn.Result->SetAsObject(pobj);
        }
    }
}

static void GAS_GetInputLanguage(const GASFnCall& fn)
{
    if (!fn.Env)
        return;

    // Try to get IME Manager.
    GPtr<GFxIMEManager> pimeManager = fn.Env->GetMovieRoot()->GetIMEManager();
    GASString resultStr = fn.Env->GetGC()->CreateConstString("UNKNOWN");

    if(pimeManager)
    {
        resultStr = pimeManager->GetInputLanguage();
    }

    fn.Result->SetString(resultStr);
}


#endif // GFC_NO_IME_SUPPORT

GASObject* GASGlobalContext::AddPackage(GASStringContext *psc, GASObject* pparent, GASObject* objProto, const char* const packageName)
{
    char            buf[256];
    UPInt           nameSz = strlen(packageName) + 1;
    const char*     pname = packageName;
    GPtr<GASObject> parent = pparent;

    while(pname)
    {
        const char* p = strchr(pname, '.');
        UPInt sz;
        if (p)
            sz = p++ - pname + 1;
        else
            sz = nameSz - (pname - packageName);

        GASSERT(sz <= sizeof(buf));
        if (sz > sizeof(buf)) 
            sz = sizeof(buf);
    
        //strncpy(buf, pname, sz-1);
        memcpy(buf, pname, sz-1);
        buf[sz-1] = '\0';

        pname = p;

        GASValue        pkgObjVal;
        GPtr<GASObject> pkgObj;
        GASString       memberName(psc->CreateString(buf));

        if (parent->GetMemberRaw(psc, memberName, &pkgObjVal))
        {
            pkgObj = pkgObjVal.ToObject(NULL);
        }
        else
        {
            pkgObj = *GHEAP_NEW(psc->GetHeap()) GASObject(psc, objProto);
            parent->SetMemberRaw(psc, memberName, GASValue(pkgObj));
        }
        parent = pkgObj;
    }
    return parent;
}


class GASGlobalObject : public GASObject
{
    GASGlobalContext* pGC;

public:
    GASGlobalObject(GASGlobalContext* pgc) : GASObject(pgc->GetGC()), pGC(pgc) {}
    ~GASGlobalObject() 
    {
    }

    virtual bool    GetMemberRaw(GASStringContext* psc, const GASString& name, GASValue* val)
    {
        if (name == psc->GetBuiltin(GASBuiltin_gfxExtensions))
        {
            if (pGC->GFxExtensions.IsDefined())
            {
                val->SetBool(pGC->GFxExtensions.IsTrue());
                return true;
            }
            else
                val->SetUndefined();
            return false;
        }
        return GASObject::GetMemberRaw(psc, name, val);
    }
    virtual bool    SetMember(GASEnvironment *penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags())
    {
        if (name == penv->GetSC()->GetBuiltin(GASBuiltin_gfxExtensions))
        {
            pGC->GFxExtensions = val.ToBool(penv);
            if (pGC->GFxExtensions.IsTrue())
            {
                SetConstMemberRaw(penv->GetSC(), "gfxVersion", GASValue(pGC->CreateConstString(GFC_FX_VERSION_STRING)));
            }
            else
            {
                DeleteMember(penv->GetSC(), pGC->CreateConstString("gfxVersion"));
            }
            return GASObject::SetMember(penv, name, GASValue(GASValue::UNSET), flags);
        }
        else if (pGC->GFxExtensions.IsTrue())
        {
            if (name == penv->GetSC()->GetBuiltin(GASBuiltin_noInvisibleAdvance))
            {
                GFxMovieRoot* proot = penv->GetMovieRoot();
                if (proot)
                    proot->SetNoInvisibleAdvanceFlag(val.ToBool(penv));
            }
            else if (name == penv->GetSC()->GetBuiltin(GASBuiltin_continueAnimation))
            {
                GFxMovieRoot* proot = penv->GetMovieRoot();
                if (proot)
                    proot->SetContinueAnimationFlag(val.ToBool(penv));
            }
        }
        return GASObject::SetMemberRaw(penv->GetSC(), name, val, flags);
    }
};

// Global functions for ActionScript
GASNameFunction GFxAction_Global_StaticFunctions[] =
{    
    {"trace",               GAS_GlobalTrace},   
    {"parseInt",            GAS_GlobalParseInt},
    {"parseFloat",          GAS_GlobalParseFloat},
    {"ifFrameLoaded",       GAS_GlobalIfFrameLoaded},
    {"isNaN",               GAS_GlobalIsNaN},
    {"isFinite",            GAS_GlobalIsFinite},

    {"escape",              GAS_GlobalEscape},
    {"unescape",            GAS_GlobalUnescape},
    {"escapeSpecialHTML",   GAS_GlobalEscapeSpecialHTML},
    {"unescapeSpecialHTML", GAS_GlobalUnescapeSpecialHTML},
    {"updateAfterEvent",    GAS_GlobalUpdateAfterEvent},
    {"setInterval",         GASIntervalTimer::SetInterval},
    {"clearInterval",       GASIntervalTimer::ClearInterval},
    {"setTimeout",          GASIntervalTimer::SetTimeout},
    {"clearTimeout",        GASIntervalTimer::ClearTimeout},

#ifndef GFC_NO_IME_SUPPORT
    // extensions for IME
    {"imecommand",          GAS_GlobalIMECommand},    
    {"setIMECandidateListStyle", GAS_SetIMECandidateListStyle},    
    {"getInputLanguage",    GAS_GetInputLanguage},   
    {"getIMECandidateListStyle", GAS_GetIMECandidateListStyle},
#endif

    // ASSetPropFlags/ASnative
    {"ASSetPropFlags",      GAS_GlobalASSetPropFlags},
    {"ASnative",            GAS_GlobalASnative},

    {0, 0}
};

// Helper function, initializes the global object.
GASGlobalContext::GASGlobalContext(GFxMovieRoot* proot, GASStringManager* strMgr)
    : GASStringBuiltinManager(strMgr)
{    
    Init(proot);
}

void GASGlobalContext::Init(GFxMovieRoot* proot)
{    
    pMovieRoot = proot;
    pHeap      = pMovieRoot->GetMovieHeap();
    // Initialization of built-ins is done in base.

    // String context for construction, delegates to us.
    // SWF Version does not matter for member creation.
    GASStringContext sc(this, 8);

    if (!pGlobal)
        pGlobal = *GHEAP_NEW(pHeap) GASGlobalObject(this);

    // Create constructors
    GASFunctionRef objCtor (*GHEAP_NEW(pHeap) GASObjectCtorFunction(&sc));
    GASFunctionRef functionCtor (*GHEAP_NEW(pHeap) GASFunctionCtorFunction(&sc));

    // Init built-in prototype instances
    GPtr<GASObject> objProto        = *GHEAP_NEW(pHeap) GASObjectProto(&sc, objCtor);
    GPtr<GASObject> functionProto   = *GHEAP_NEW(pHeap) GASFunctionProto(&sc, objProto, functionCtor);

    // Special case: Object.__proto__ == Function.prototype
    objCtor->Set__proto__(&sc, functionProto);

    // A special case for Function class: its __proto__ is equal to Function.prototype.
    // This is the undocumented feature.
    functionCtor->Set__proto__(&sc, functionProto);

    Prototypes.Add(GASBuiltin_Object, objProto);
    Prototypes.Add(GASBuiltin_Function, functionProto);

    GASFunctionRef movieClipCtor (*GHEAP_NEW(pHeap) GASMovieClipCtorFunction(&sc));
#ifndef GFC_NO_KEYBOARD_SUPPORT
    GASFunctionRef keyCtor (*GHEAP_NEW(pHeap) GASKeyCtorFunction(&sc, proot));
    GPtr<GASObject> keyProto  = *GHEAP_NEW(pHeap) GASKeyProto(&sc, objProto, keyCtor);
    Prototypes.Add(GASBuiltin_Key, keyProto);
#endif // GFC_NO_KEYBOARD_SUPPORT

    GPtr<GASObject> movieClipProto  = *GHEAP_NEW(pHeap) GASMovieClipProto(&sc, objProto, movieClipCtor);
    Prototypes.Add(GASBuiltin_MovieClip, movieClipProto);

    // @@ pGlobal should really be a
    // client-visible player object, which
    // contains one or more actual GFxASCharacter
    // instances.

    GASNameFunction::AddConstMembers(pGlobal, &sc, GFxAction_Global_StaticFunctions);

#ifndef GFC_NO_IME_SUPPORT
    // extensions for IME
    pGlobal->SetConstMemberRaw(&sc, "imecommand", GASValue(&sc, GAS_GlobalIMECommand));    
    pGlobal->SetConstMemberRaw(&sc, "setIMECandidateListStyle", GASValue(&sc, GAS_SetIMECandidateListStyle));
    pGlobal->SetConstMemberRaw(&sc, "getInputLanguage", GASValue(&sc, GAS_GetInputLanguage)); 
    pGlobal->SetConstMemberRaw(&sc, "getIMECandidateListStyle", GASValue(&sc, GAS_GetIMECandidateListStyle));
#endif // GFC_NO_IME_SUPPORT

    // 3Di extensions
    pGlobal->SetConstMemberRaw(&sc, "screenToWorld", GASValue(&sc, GAS_GlobalScreenToWorld));

    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Object), GASValue(objCtor));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Function), GASValue(functionCtor));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_MovieClip), GASValue(movieClipCtor));
#ifndef GFC_NO_KEYBOARD_SUPPORT
    // Key should be created here, since it is a listener of KeyboardState
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Key), GASValue(keyCtor));
#endif
    AddBuiltinClassRegistry<GASBuiltin_Math, GASMathCtorFunction>(sc, pGlobal);

    AddBuiltinClassRegistry<GASBuiltin_Array, GASArrayCtorFunction>(sc, pGlobal);
    AddBuiltinClassRegistry<GASBuiltin_Number, GASNumberCtorFunction>(sc, pGlobal);
    AddBuiltinClassRegistry<GASBuiltin_String, GASStringCtorFunction>(sc, pGlobal);
    AddBuiltinClassRegistry<GASBuiltin_Boolean, GASBooleanCtorFunction>(sc, pGlobal);
#ifndef GFC_NO_FXPLAYER_AS_COLOR
    AddBuiltinClassRegistry<GASBuiltin_Color, GASColorCtorFunction>(sc, pGlobal);
#endif
    AddBuiltinClassRegistry<GASBuiltin_Button, GASButtonCtorFunction>(sc, pGlobal);

#ifndef GFC_NO_FXPLAYER_AS_MOVIECLIPLOADER
    AddBuiltinClassRegistry<GASBuiltin_MovieClipLoader, GASMovieClipLoaderCtorFunction>(sc, pGlobal);
#endif
#ifndef GFC_NO_FXPLAYER_AS_LOADVARS
    AddBuiltinClassRegistry<GASBuiltin_LoadVars, GASLoadVarsLoaderCtorFunction>(sc, pGlobal);
#endif

#ifndef GFC_NO_FXPLAYER_AS_STAGE
    AddBuiltinClassRegistry<GASBuiltin_Stage, GASStageCtorFunction>(sc, pGlobal);
#endif
#ifndef GFC_NO_FXPLAYER_AS_SELECTION
    AddBuiltinClassRegistry<GASBuiltin_Selection, GASSelectionCtorFunction>(sc, pGlobal);
#endif
    AddBuiltinClassRegistry<GASBuiltin_AsBroadcaster, GASAsBroadcasterCtorFunction>(sc, pGlobal);
#ifndef GFC_NO_FXPLAYER_AS_MOUSE
    AddBuiltinClassRegistry<GASBuiltin_Mouse, GASMouseCtorFunction>(sc, pGlobal);
#endif

    AddBuiltinClassRegistry<GASBuiltin_TextField, GASTextFieldCtorFunction>(sc, pGlobal);
#ifndef GFC_NO_FXPLAYER_AS_TEXTFORMAT
    AddBuiltinClassRegistry<GASBuiltin_TextFormat, GASTextFormatCtorFunction>(sc, pGlobal);
#endif

#ifndef GFC_NO_FXPLAYER_AS_TEXTSNAPSHOT
    AddBuiltinClassRegistry<GASBuiltin_TextSnapshot, GASTextSnapshotCtorFunction>(sc, pGlobal);
#endif

#ifndef GFC_NO_FXPLAYER_AS_DATE
    AddBuiltinClassRegistry<GASBuiltin_Date, GASDateCtorFunction>(sc, pGlobal);
#endif

#ifndef GFC_NO_FXPLAYER_AS_SHAREDOBJECT
    AddBuiltinClassRegistry<GASBuiltin_SharedObject, GASSharedObjectCtorFunction>(sc, pGlobal);
#endif

    AddBuiltinClassRegistry<GASBuiltin_Amp, GASAmpMarkerCtorFunction>(sc, pGlobal);

    // classes from packages
    FlashGeomPackage = AddPackage(&sc, pGlobal, objProto, "flash.geom");
#ifndef GFC_NO_FXPLAYER_AS_TRANSFORM
    AddBuiltinClassRegistry<GASBuiltin_Transform, GASTransformCtorFunction>(sc, FlashGeomPackage);
#endif
#ifndef GFC_NO_FXPLAYER_AS_MATRIX
    AddBuiltinClassRegistry<GASBuiltin_Matrix, GASMatrixCtorFunction>(sc, FlashGeomPackage);
#endif
#ifndef GFC_NO_FXPLAYER_AS_POINT
    AddBuiltinClassRegistry<GASBuiltin_Point, GASPointCtorFunction>(sc, FlashGeomPackage);
#endif
#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
    AddBuiltinClassRegistry<GASBuiltin_Rectangle, GASRectangleCtorFunction>(sc, FlashGeomPackage);
#endif
#ifndef GFC_NO_FXPLAYER_AS_COLORTRANSFORM
    AddBuiltinClassRegistry<GASBuiltin_ColorTransform, GASColorTransformCtorFunction>(sc, FlashGeomPackage);
#endif

#ifndef GFC_NO_FXPLAYER_AS_CAPABILITES
    SystemPackage = AddPackage(&sc, pGlobal, objProto, "System");
    AddBuiltinClassRegistry<GASBuiltin_Capabilities, GASCapabilitiesCtorFunction>(sc, SystemPackage);
#endif

#if !defined(GFC_NO_FXPLAYER_AS_IME) && !defined(GFC_NO_IME_SUPPORT) 
    AddBuiltinClassRegistry<GASBuiltin_Ime, GASImeCtorFunction>(sc, SystemPackage);
#endif 

    FlashExternalPackage = AddPackage(&sc, pGlobal, objProto, "flash.external");
    AddBuiltinClassRegistry<GASBuiltin_ExternalInterface, GASExternalInterfaceCtorFunction>(sc, FlashExternalPackage);

#ifndef GFC_NO_FXPLAYER_AS_BITMAPDATA
    FlashDisplayPackage = AddPackage(&sc, pGlobal, objProto, "flash.display");
    AddBuiltinClassRegistry<GASBuiltin_BitmapData, GASBitmapDataCtorFunction>(sc, FlashDisplayPackage);
#endif

    FlashFiltersPackage = AddPackage(&sc, pGlobal, objProto, "flash.filters");
#ifndef GFC_NO_FXPLAYER_AS_FILTERS
    AddBuiltinClassRegistry<GASBuiltin_BitmapFilter, GASBitmapFilterCtorFunction>(sc, FlashFiltersPackage);
    AddBuiltinClassRegistry<GASBuiltin_DropShadowFilter, GASDropShadowFilterCtorFunction>(sc, FlashFiltersPackage);
    AddBuiltinClassRegistry<GASBuiltin_GlowFilter, GASGlowFilterCtorFunction>(sc, FlashFiltersPackage);
    AddBuiltinClassRegistry<GASBuiltin_BlurFilter, GASBlurFilterCtorFunction>(sc, FlashFiltersPackage);
    AddBuiltinClassRegistry<GASBuiltin_BevelFilter, GASBevelFilterCtorFunction>(sc, FlashFiltersPackage);
    AddBuiltinClassRegistry<GASBuiltin_ColorMatrixFilter, GASColorMatrixFilterCtorFunction>(sc, FlashFiltersPackage);
#endif  // GFC_NO_FXPLAYER_AS_FILTERS

    InitStandardMembers();

    // hide all members
    struct MemberVisitor : GASObjectInterface::MemberVisitor
    {
        GPtr<GASObject> obj;
        GASStringContext *psc;

        MemberVisitor (GASStringContext *_psc, GASObject* _obj) : obj(_obj), psc(_psc) {}

        virtual void Visit(const GASString& name, const GASValue& val, UByte flags)
        {
            GUNUSED(val);
            obj->SetMemberFlags(psc, name, (UByte)(flags| GASPropFlags::PropFlag_DontEnum));

        }
    } visitor (&sc, pGlobal);
    pGlobal->VisitMembers(&sc, &visitor, GASObjectInterface::VisitMember_DontEnum);
}

// GASGlobalContext::GASGlobalContext(GMemoryHeap* pheap)
// : GASStringBuiltinManager(pheap)
// {    
//     pHeap = pheap;
// }

GASGlobalContext::~GASGlobalContext()
{
    // ReleaseBuiltins() - release of built-ins is done in base destructor.
    // If that was not the case we would've had to clean up all local string maps first explicitly.
}

GASRefCountCollector* GASGlobalContext::GetGC() 
{ 
    return ((pMovieRoot) ? pMovieRoot->GetASGC() : NULL); 
}                                       

void GASGlobalContext::PreClean(bool preserveBuiltinProps)
{
    if (preserveBuiltinProps)
    {
        GASSERT(pMovieRoot);
        GPtr<GASGlobalObject> pnewglobal = *GHEAP_NEW(pHeap) GASGlobalObject(this);
        GASStringContext sc(this, 8);
        GASString gfxPlayer = sc.CreateConstString("gfxPlayer");
        GASString gfxLanguage = sc.CreateConstString("gfxLanguage");
        GASString gfxArg = sc.CreateConstString("gfxArg");
        GASValue v;
        pGlobal->GetMemberRaw(&sc, gfxPlayer, &v);
        pnewglobal->SetMemberRaw(&sc, gfxPlayer, v);
        pGlobal->GetMemberRaw(&sc, gfxLanguage, &v);
        pnewglobal->SetMemberRaw(&sc, gfxLanguage, v);
        pGlobal->GetMemberRaw(&sc, gfxArg, &v);
        pnewglobal->SetMemberRaw(&sc, gfxArg, v);
        pGlobal = pnewglobal;
    }
    else
        pGlobal = NULL;
    RegisteredClasses.Clear();
    BuiltinClassesRegistry.Clear();
    Prototypes.Clear();
    DetachMovieRoot();
}

GASFunctionObject* GASGlobalContext::ResolveFunctionName(const GASString& functionName) 
{
    // not found; lets try to resolve
    ClassRegEntry* regEntry = GetBuiltinClassRegistrar(functionName);
    if (regEntry)
    {
        if (!regEntry->IsResolved())
        {
            // found, do we need to resolve by calling RegistrarFunc
            GASFunctionRef f = regEntry->RegistrarFunc(this);

            // theoretically, RegistrarFunc may add another registration,
            // for example StyleSheet has been added from the TextField's Register func.
            // That is why we need to search for the regEntry again.
            regEntry = GetBuiltinClassRegistrar(functionName);
            GASSERT(regEntry);
            regEntry->ResolvedFunc = f.GetObjectPtr();
        }
        return regEntry->ResolvedFunc;
    }
    return NULL;
}

GASObject* GASGlobalContext::GetPrototype(GASBuiltinType type) 
{
    GPtr<GASObject>* presult = Prototypes.Get(type);
    if (!presult)
    {
        ResolveFunctionName(GetBuiltin(type));
        presult = Prototypes.Get(type);            
    }
    return (presult) ? presult->GetPtr() : NULL;
}

GASObject* GASGlobalContext::GetActualPrototype(GASEnvironment* penv, GASBuiltinType type) 
{
    // force registration
    GPtr<GASObject> result = GetPrototype(type);
    
    // need to check constructor function's "prototype" prop.
    GASValue v;
    if (pGlobal->GetMemberRaw(penv->GetSC(), GetBuiltin(type), &v))
    {
        GPtr<GASObject> o = v.ToObject(penv);
        if (o && o->GetMemberRaw(penv->GetSC(), GetBuiltin(GASBuiltin_prototype), &v))
            result = v.ToObject(penv);
    }
    return result;
}

GASGlobalContext::ClassRegEntry* GASGlobalContext::GetBuiltinClassRegistrar(GASString className)
{
    ClassRegEntry* pfuncPtr = BuiltinClassesRegistry.GetCaseCheck(className, true);
    if (pfuncPtr)
    {
        return pfuncPtr;
    }
    return NULL;
}

GASString GASGlobalContext::FindClassName(GASEnvironment *penv, GASObjectInterface* iobj)
{
    if (iobj)
    {
        GASObject* obj;
        if (iobj->IsASCharacter())
            obj = static_cast<GFxASCharacter*>(iobj)->GetASObject();
        else
            obj = static_cast<GASObject*>(iobj);

        GASObject::MemberHash::ConstIterator it = pGlobal->Members.Begin();
        
        for (; it != pGlobal->Members.End(); ++it)
        {
            const GASString& nm = it->First;
            const GASMember& m = it->Second;
            const GASValue& val = m.Value;
            if (obj->IsFunction())
            {
                if (val.IsFunction() && val.ToFunction(penv).GetObjectPtr() == obj)
                    return nm;
            }
            else
            {
                if (val.IsObject() && val.ToObject(penv) == obj)
                    return nm;
                if (val.IsFunction())
                {
                    GASFunctionRef f = val.ToFunction(penv);
                    GASValue protoVal;
                    if (f->GetMemberRaw(penv->GetSC(), GetBuiltin(GASBuiltin_prototype), &protoVal))
                    {
                        if (protoVal.IsObject() && protoVal.ToObject(penv) == obj)
                            return nm + ".prototype";
                    }
                }
            }
        }
    }
    return GetBuiltin(GASBuiltin_unknown_);
}

void    GASGlobalContext::InitStandardMembers()
{
    GFxASCharacter::InitStandardMembers(this);
}

bool GASGlobalContext::RegisterClass(GASStringContext* psc, const GASString& className, const GASFunctionRef& ctorFunction)
{
    RegisteredClasses.SetCaseCheck(className, ctorFunction, psc->IsCaseSensitive());
    return true;
}

bool GASGlobalContext::UnregisterClass(GASStringContext* psc, const GASString& className)
{    
    if (RegisteredClasses.GetCaseCheck(className, psc->IsCaseSensitive()) == 0)
        return false;
    RegisteredClasses.RemoveCaseCheck(className, psc->IsCaseSensitive());
    return true;
}

bool GASGlobalContext::FindRegisteredClass(GASStringContext* psc, const GASString& className, GASFunctionRef* pctorFunction)
{
    // MA TBD: Conditional case sensitivity
    const GASFunctionRef* ctor = RegisteredClasses.GetCaseCheck(className, psc->IsCaseSensitive());
    if (ctor == 0)
        return false;
    if (pctorFunction)
        *pctorFunction = *ctor;
    return true;
}

void GASGlobalContext::UnregisterAllClasses()
{
    RegisteredClasses.Resize(0);
}

//
// GASDoAction
//

// Thin wrapper around GASActionBuffer.
class GASDoAction : public GASExecuteTag
{
public:
    GPtr<GASActionBufferData> pBuf;

    void    Read(GFxLoadProcess* p)
    {
        GFxStream* in = p->GetStream();

        pBuf = *GASActionBufferData::CreateNew();
#ifdef GFX_AMP_SERVER
        int offset = in->Tell();
        UInt flags = p->GetProcessInfo().Header.SWFFlags;
        if (flags & GFxMovieInfo::SWF_Stripped)
        {
            p->GetNextCodeOffset(&offset);
        }
        if (flags & GFxMovieInfo::SWF_Compressed)
        {
            // Add the size of the 8-byte header, 
            // since that is not included in the compressed stream
            offset += 8;
        }

        pBuf->SetSWFFileOffset(offset);
        pBuf->SetSwdHandle(p->GetLoadTaskData()->GetSwdHandle());
#endif  // GFX_AMP_SERVER
        pBuf->Read(in, (UInt)(p->GetTagEndPosition() - p->Tell()));
    }

    virtual void    Execute(GFxSprite* m)
    {
        const GASEnvironment *penv = m->GetASEnvironment();
        if (pBuf && !pBuf->IsNull())
        {
            GPtr<GASActionBuffer> pbuff =
                *GHEAP_NEW(penv->GetHeap()) GASActionBuffer(penv->GetSC(), pBuf);
            m->AddActionBuffer(pbuff.GetPtr());
        }
    }

    virtual void    ExecuteWithPriority(GFxSprite* m, GFxActionPriority::Priority prio) 
    { 
        if (pBuf && !pBuf->IsNull())
        {
            const GASEnvironment *penv = m->GetASEnvironment();
            GPtr<GASActionBuffer> pbuff =
                *GHEAP_NEW(penv->GetHeap()) GASActionBuffer(penv->GetSC(), pBuf);
            m->AddActionBuffer(pbuff.GetPtr(), prio);
        }
    }
    // Don't override because actions should not be replayed when seeking the GFxSprite.
    //void  ExecuteState(GFxSprite* m) {}

    // Tell the caller that we are an action tag.
    virtual bool    IsActionTag() const
    {
        return true;
    }

    void Trace(const char* str)
    {
        GUNUSED(str);
#ifdef GFC_BUILD_DEBUG
        printf("  %s actions, file %s\n", str, pBuf->GetFileName());
#endif
    }
};

class GASDoInitAction : public GASDoAction
{
    GASDoInitAction(const GASDoInitAction&) {} // suppress warning
    GASDoInitAction& operator=(const GASDoInitAction&) { return *this; } // suppress warning
public:
    GASDoInitAction() {}

    virtual void    Execute(GFxSprite* m)
    {
        if (pBuf && !pBuf->IsNull())
        {
            const GASEnvironment *penv  = m->GetASEnvironment();
            GPtr<GASActionBuffer> pbuff = 
                *GHEAP_NEW(penv->GetHeap()) GASActionBuffer(penv->GetSC(), pBuf);
            m->AddActionBuffer(pbuff.GetPtr(), GFxMovieRoot::AP_InitClip);
        }
    }
    
    // Tell the caller that we are not a regular action tag.
    virtual bool    IsActionTag() const
    {
        return false;
    }
};

void    GSTDCALL GFx_DoActionLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    p->LogParse("tag %d: DoActionLoader\n", tagInfo.TagType);
    p->LogParseAction("-- actions in frame %d\n", p->GetLoadingFrame());

    GASSERT(p);
    GASSERT(tagInfo.TagType == GFxTag_DoAction);    
    
    GASDoAction* da = p->AllocTag<GASDoAction>();
    da->Read(p);
    p->AddExecuteTag(da);
}


//
// DoInitAction
//


void    GSTDCALL GFx_DoInitActionLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT(tagInfo.TagType == GFxTag_DoInitAction);

    int spriteCharacterId = p->ReadU16();

    p->LogParse("  tag %d: DoInitActionLoader\n", tagInfo.TagType);
    p->LogParseAction("  -- init actions for sprite %d\n", spriteCharacterId);

    GASDoAction* da = p->AllocTag<GASDoInitAction>();
    da->Read(p);
    p->AddInitAction(GFxResourceId(spriteCharacterId), da);
}


//
// GASActionBuffer
//

// ***** GASNameFunction

void GASNameFunction::AddConstMembers(GASObjectInterface* pobj, GASStringContext *psc,
                                      const GASNameFunction* pfunctions, UByte flags)
{
    GMemoryHeap* pheap = psc->GetHeap();
    GASObject* pfuncProto = psc->pContext->GetPrototype(GASBuiltin_Function);
    while(pfunctions->Name)
    {
        pobj->SetConstMemberRaw(psc, pfunctions->Name,
            GASFunctionRef(*GHEAP_NEW(pheap) GASCFunctionObject(psc, pfuncProto, pfunctions->Function)),
            flags);
        pfunctions++;
    }
}


// ***** With Stack implementation
GASWithStackEntry::GASWithStackEntry(GASObject* pobj, int end)
{
    // Compiler does not yet know GASObject.
    pObject     =  pobj;
    if (pObject)
        pObject->AddRef();
    GASSERT(end >= 0);
    BlockEndPc  = ((UInt32)end | Mask_IsObject);
}

GASWithStackEntry::GASWithStackEntry(GFxASCharacter* pcharacter, int end)
{
    pCharacter  = pcharacter;
    if (pCharacter)
        pCharacter->AddRef();
    GASSERT(end >= 0);
    BlockEndPc  = (UInt32)end;
}

GASWithStackEntry::~GASWithStackEntry()
{
    if (IsObject() && pObject)
        pObject->Release();
    else if (pCharacter)
        pCharacter->Release();
}

GASObject*          GASWithStackEntry::GetObject() const
{
    return IsObject() ? pObject : 0;
}
GFxASCharacter*     GASWithStackEntry::GetCharacter() const
{
    return IsObject() ? 0 : pCharacter;
}

GASObjectInterface* GASWithStackEntry::GetObjectInterface() const
{
    return IsObject() ? (GASObjectInterface*) pObject :
                        (GASObjectInterface*) pCharacter;
}

// ***** GASActionBuffer implementation
GASActionBufferData* GASActionBufferData::CreateNew()
{
    // We need to allocate ActionBufferData from global heap, since it is possible 
    // to have a pointer to ActionBufferData after its MovieDataDef is unloaded (like, AS
    // class was registered in _global from SWF loaded by "loadMovie" and when the movie
    // is unloaded). So, we allocate all actions from global heap for now.
    return GHEAP_NEW_ID(GMemory::GetGlobalHeap(), GFxStatMD_ActionOps_Mem) GASActionBufferData();
}

void    GASActionBufferData::Read(GFxStream* in, UInt actionLength)
{
#ifdef GFC_BUILD_DEBUG
    FileName = in->GetFileName();
#endif
    
    BufferLen = actionLength;

    GASSERT(pBuffer == NULL);   // Should not exist
    pBuffer = (UByte*)GHEAP_AUTO_ALLOC_ID(this, BufferLen, GFxStatMD_ActionOps_Mem);
    in->ReadToBuffer(pBuffer, BufferLen);

#ifndef GFC_NO_FXPLAYER_VERBOSE_PARSE_ACTION
    if (in->IsVerboseParseAction())
    {
        GFxStreamContext sc(pBuffer);
        for (;;)
        {
            UPInt   instructionStart    = sc.CurByteIndex;
            UPInt   pc                  = sc.CurByteIndex;
            UByte   actionId            = sc.ReadU8();

            if (actionId & 0x80)
            {
                // Action contains extra data.  Read it.
                int length = sc.ReadU16();
                sc.Skip(length);
            }

            in->LogParseAction("%4d\t", (int)pc);
            GFxDisasm da(in->GetLog(), GFxLog::Log_ParseAction);
            da.LogDisasm(&pBuffer[instructionStart]);

            if (actionId == 0)
            {
                // end of action buffer.
                break;
            }
        }
    }
#endif

#ifdef GFX_AMP_SERVER
    if (SwdHandle == 0)
    {
        SwdHandle = GFxAmpServer::GetInstance().GetNextSwdHandle();
    }
#endif

    // Last byte in buffer will always be zero.
    // However, some files may contain garbage at the end of the action buffer 
    // (for example world_wide_leader.swf). In this case we just check, if there is
    // a zero byte at all. Search backward.
    GASSERT(pBuffer[BufferLen-1] == 0 || G_memrchr(pBuffer, BufferLen, 0));
}

void    GASActionBufferData::Read(GFxStreamContext* psc, UInt eventLength)
{
    psc->Align();
    pBuffer = (UByte*)GHEAP_AUTO_ALLOC_ID(this, eventLength, GFxStatMD_ActionOps_Mem);
    memcpy(pBuffer, psc->pData + psc->CurByteIndex, eventLength);
    BufferLen = eventLength;
    psc->Skip(BufferLen);

#ifdef GFX_AMP_SERVER
    if (SwdHandle == 0)
    {
        SwdHandle = GFxAmpServer::GetInstance().GetNextSwdHandle();
    }
#endif

    // Last byte in buffer will always be zero.
    GASSERT(pBuffer[BufferLen-1] == 0);
}

GASActionBuffer::GASActionBuffer(GASStringContext *psc, GASActionBufferData *pbufferData)
:   pBufferData(pbufferData),  
    Dictionary(psc->GetBuiltin(GASBuiltin_empty_)),
    DeclDictProcessedAt(-1)
{ 
}

GASActionBuffer::~GASActionBuffer()
{ 
}

// Interpret the DeclDict opcode.  Don't read stopPc or
// later.  A dictionary is some static strings embedded in the
// action buffer; there should only be one dictionary per
// action buffer.
//
// NOTE: Normally the dictionary is declared as the first
// action in an action buffer, but I've seen what looks like
// some form of copy protection that amounts to:
//
// <start of action buffer>
//          push true
//          BranchIfTrue label
//          DeclDict   [0]   // this is never executed, but has lots of orphan data declared in the opcode
// label:   // (embedded inside the previous opcode; looks like an invalid jump)
//          ... "protected" code here, including the real DeclDict opcode ...
//          <end of the dummy DeclDict [0] opcode>
//
// So we just interpret the first DeclDict we come to, and
// cache the results.  If we ever hit a different DeclDict in
// the same GASActionBuffer, then we log an error and ignore it.
void    GASActionBuffer::ProcessDeclDict(GASStringContext *psc, UInt startPc, UInt stopPc, class GFxActionLogger &log)
{
    GASSERT(stopPc <= GetLength());
    const UByte*  Buffer = GetBufferPtr();

    if (DeclDictProcessedAt == (int)startPc)
    {
        // We've already processed this DeclDict.
        UInt    count = Buffer[startPc + 3] | (Buffer[startPc + 4] << 8);
        GASSERT(Dictionary.GetSize() == count);
        GUNUSED(count);
        return;
    }

    if (DeclDictProcessedAt != -1)
    {
        if (log.IsVerboseActionErrors())
            log.LogScriptError("Error: ProcessDeclDict(%d, %d) - DeclDict was already processed at %d\n",
                startPc,
                stopPc,
                DeclDictProcessedAt);
        return;
    }

    DeclDictProcessedAt = (int)startPc;

    // Actual processing.
    UInt    i = startPc;
    UInt    length = Buffer[i + 1] | (Buffer[i + 2] << 8);
    UInt    count = Buffer[i + 3] | (Buffer[i + 4] << 8);
    i += 2;
    GUNUSED(length);
    GASSERT(startPc + 3 + length == stopPc);

    Dictionary.Resize(count);    

    // Index the strings.
    for (UInt ct = 0; ct < count; ct++)
    {
        // Point into the current action buffer.        
        Dictionary[ct] = psc->CreateString((const char*) &Buffer[3 + i]);

        while (Buffer[3 + i])
        {
            // safety check.
            if (i >= stopPc)
            {
                if (log.IsVerboseActionErrors()) 
                    log.LogScriptError("Error: Action buffer dict length exceeded\n");

                // Jam something into the Remaining (invalid) entries.
                while (ct < count)
                {
                    Dictionary[ct] = psc->CreateString("<invalid>");
                    ct++;
                }
                return;
            }
            i++;
        }
        i++;
    }
}

bool    GASActionBuffer::ResolveFrameNumber 
    (GASEnvironment* env, const GASValue& frameValue, GFxASCharacter** pptarget, UInt* pframeNumber)
{
    GASSERT(pframeNumber);
    GFxASCharacter* target  = env->GetTarget();
    bool success            = false;

    if (frameValue.GetType() == GASValue::STRING)
    {   
        GASString   str(frameValue.ToString(env));
        int         mbLength = str.GetLength();
        int         i;

        // Parse possible sprite path...
        for (i=0; i<mbLength; i++)
        {
            if (str.GetCharAt(i) == ':')
            {
                // Must be a target label.
                GASString targetStr(str.Substring(0, i));
                if ((target = env->FindTarget(targetStr))!=0)
                {
                    if (i >= mbLength)
                        target = 0; // No frame.
                    else
                    {   
                        // The remainder of the string is frame number or label.
                        str = str.Substring(i+1, mbLength+1);
                        break;
                    }
                }
            }
        }
            
        if (target && target->GetLabeledFrame(str.ToCStr(), pframeNumber))
            success = true;                  
    }
    else if (frameValue.GetType() == GASValue::OBJECT)
    {
        // This is a no-op; see TestGotoFrame.Swf
    }
    else if (frameValue.IsNumber())
    {
        // MA: Unlike other places, this frame number is NOT zero-based,
        // because it is computed by action script; so subtract 1.
        *pframeNumber = int(frameValue.ToNumber(env) - 1);
        success = true;
    }
    if (success && pptarget)
        *pptarget = target;

    return success;
}

// Interpret the actions in this action buffer, and evaluate
// them in the given environment.  Execute our whole buffer,
// without any arguments passed in.
void    GASActionBuffer::Execute(GASEnvironment* env)
{
    //int   LocalStackTop = env->GetLocalFrameTop();
    //env->AddFrameBarrier(); //??AB, should I create a local frame here??? AddFrameBarrier does nothing.
    //GPtr<GASLocalFrame> curLocalFrame = env->CreateNewLocalFrame();
    
    Execute(env, 0, GetLength(), NULL, 0, Exec_Unknown /* not function2 */);

    //curLocalFrame->ReleaseFramesForLocalFuncs ();
    //env->SetLocalFrameTop(LocalStackTop);
}

#ifdef GFXACTION_COUNT_OPS
// Debug counters for performance statistics.
UInt Execute_OpCounters[256] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

UInt Execute_PushDataTypeCounter[16] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

UInt Execute_NotType[16] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
#endif

class GASExecutionContext
{
public:
    GASEnvironment* const   pEnv;
    GFxASCharacter* const   pOriginalTarget;
    const UByte* const      pBuffer;

    GFxActionLogger*        pPrevLog;

    int StopPC;
    int NextPC;
    int PC;

    // Use a custom WithStackHolder to ensure that WithStack is allocated
    // only when necessary and in correct heap.
    struct WithStackHolder
    {
        GMemoryHeap*       pHeap;
        GASWithStackArray* pWithStackArray;

        WithStackHolder(GMemoryHeap* pheap, const GASWithStackArray *pinit)
            : pHeap(pheap)
        {
            pWithStackArray = pinit ? (GHEAP_NEW(pheap) GASWithStackArray(*pinit)) : 0;
        }
        ~WithStackHolder()
        {
            if (pWithStackArray)
                delete pWithStackArray;
        }

        inline operator GASWithStackArray* () const       { return pWithStackArray; }
        inline operator const GASWithStackArray* () const { return pWithStackArray; }

        void PushBack(const GASWithStackEntry& entry)
        {
            if (!pWithStackArray)
                pWithStackArray = GHEAP_NEW(pHeap) GASWithStackArray();
            pWithStackArray->PushBack(entry);
        }

        inline UPInt GetSize() const { return pWithStackArray ? pWithStackArray->GetSize() : 0; }
    };

//     GASString       TempStr;
//     GASString       TempStr1;
//     GASValue        TempVal;
//     GASValue        OwnerVal;
//     GASValue        ResultVal;
//     GASValue        FunctionVal;
    WithStackHolder WithStack;
    GFxActionLogger Log;

    UInt8           Version;
    UInt8           ExecType;
    bool            VerboseActionErrors :1;
    bool            VerboseAction       :1;
    bool            IsFunction2         :1;

    GINLINE GASExecutionContext(GASEnvironment* penv, 
                        const GASWithStackArray *pinitialWithStack, 
                        GASActionBuffer::ExecuteType execType,
                        const UByte* pbuffer,
                        const char* fileName) 
      : pEnv(penv), pOriginalTarget(pEnv->GetTarget()), pBuffer(pbuffer), 
        //TempStr(pEnv->GetBuiltin(GASBuiltin_empty_)), TempStr1(TempStr),
        WithStack(penv->GetHeap(), pinitialWithStack), 
        Log(pOriginalTarget, fileName)
    {
        pPrevLog    = pEnv->GetASLogger();
        Version     = (UInt8)pEnv->GetTarget()->GetVersion();
        ExecType    = (UInt8)execType;
        IsFunction2 = (execType == GASActionBuffer::Exec_Function2);
        VerboseAction        = Log.IsVerboseAction();
        VerboseActionErrors  = Log.IsVerboseActionErrors();
    }
    GINLINE ~GASExecutionContext()
    {
        pEnv->SetASLogger(pPrevLog); 
    }

    void SetTargetOpCode();
    void StartDragOpCode();
    void CastObjectOpCode();
    void ImplementsOpCode();
    void EnumerateOpCode(int actionId);
    void ExtendsOpCode();
    void InstanceOfOpCode();
    bool MethodCallOpCode();
    void WaitForFrameOpCode(GASActionBuffer* pActions, int actionId);
    void Function1OpCode(GASActionBuffer* pActions);
    void Function2OpCode(GASActionBuffer* pActions);

    GASExecutionContext& operator=(const GASExecutionContext&) { return *this; }
};

void GASExecutionContext::SetTargetOpCode()
{
    GFxASCharacter* target = 0;

    GASValue targetVal = pEnv->Top();
    if (!targetVal.IsString() && !targetVal.IsCharacter())
    {
        targetVal.SetString(targetVal.ToStringVersioned(pEnv, Version));
    }

    if (targetVal.IsString())
    {
        if (targetVal.ToString(pEnv).IsEmpty())
        {
            // special case for empty string - originalTarget will be used
            target = pOriginalTarget;
        }
        else
        {
            // target is specified as a string, like /mc1/nmc1/mmm1
            GASValue val;
            pEnv->GetVariable(pEnv->Top().ToString(pEnv), &val, WithStack, &target);

            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
            if (VerboseAction) 
            {
                GASString s1(pEnv->Top().ToDebugString(pEnv));

                if (target && target->IsASCharacter())
                    Log.LogAction("-- ActionSetTarget2: %s (%d)\n",
                        s1.ToCStr(),
                        target->ToASCharacter()->GetId().GetIdIndex());
                else
                    Log.LogAction("-- ActionSetTarget2: %s - no target found\n",
                        s1.ToCStr());
            }
            #endif
        }
    }
    else if (targetVal.IsCharacter())
    {
        target = pEnv->Top().ToASCharacter(pEnv);
    }
    else
    {   
        GASSERT(0);
    }

    if (!target) 
    {
        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
        if (VerboseActionErrors)
        {
            GASString sv(targetVal.ToDebugString(pEnv));                            
            Log.LogScriptError("Error: SetTarget2(tellTarget) with invalid target '%s'.\n",
                sv.ToCStr());
        }
        #endif
        //!AB: if target is invalid, then set originalTarget as a target and
        // mark it as "invalid" target. This means that frame-related
        // functions (such as gotoAndStop, etc) should do nothing
        // inside this tellTarget.
        pEnv->SetInvalidTarget(pOriginalTarget);
    }
    else
    {
        pEnv->SetTarget(target);
    }
    pEnv->Drop1();
}

void GASExecutionContext::StartDragOpCode()
{
    GFxMovieRoot::DragState st;
    bool lockCenter = pEnv->Top1().ToBool(pEnv);

    st.pCharacter = pEnv->FindTargetByValue(pEnv->Top());
    if (st.pCharacter == NULL)
    {
        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
        if (VerboseActionErrors)
        {
            GASString s0(pEnv->Top().ToDebugString(pEnv));
            Log.LogScriptError("Error: StartDrag of invalid target '%s'.\n",
                s0.ToCStr());
        }
        #endif
    }

    st.Bound = pEnv->Top(2).ToBool(pEnv);
    if (st.Bound)
    {
        st.BoundLT.x = GFC_PIXELS_TO_TWIPS((Float) pEnv->Top(6).ToNumber(pEnv));
        st.BoundLT.y = GFC_PIXELS_TO_TWIPS((Float) pEnv->Top(5).ToNumber(pEnv));
        st.BoundRB.x = GFC_PIXELS_TO_TWIPS((Float) pEnv->Top(4).ToNumber(pEnv));
        st.BoundRB.y = GFC_PIXELS_TO_TWIPS((Float) pEnv->Top(3).ToNumber(pEnv));
        pEnv->Drop(4);
    }

    if (st.pCharacter)
    {
        // Init mouse offsets based on LockCenter flag.
        st.InitCenterDelta(lockCenter);

        GFxMovieRoot* pmovieRoot = pEnv->GetTarget()->GetMovieRoot();
        GASSERT(pmovieRoot);

        if (pmovieRoot)
            pmovieRoot->SetDragState(st);   
    }

    pEnv->Drop3();
}

void GASExecutionContext::CastObjectOpCode()
{
    // Pop object, pop constructor function
    // Make sure o1 is an instance of s2.
    // If the cast succeeds, push object back, else push NULL.
    const GASValue& objVal      = pEnv->Top();
    const GASValue& ctorFuncVal = pEnv->Top1();
    GASValue rv(GASValue::NULLTYPE);

    if (ctorFuncVal.IsFunction())
    {
        GASFunctionRef ctorFunc = ctorFuncVal.ToFunction(pEnv);
        if (!ctorFunc.IsNull())
        {
            GASObjectInterface* obj = objVal.ToObjectInterface(pEnv);
            if (obj != 0)
            {
                GASValue prototypeVal;
                if (ctorFunc->GetMemberRaw(pEnv->GetSC(), pEnv->GetBuiltin(GASBuiltin_prototype), &prototypeVal))
                {
                    GASObject* prototype = prototypeVal.ToObject(pEnv);
                    if (obj->InstanceOf(pEnv, prototype))
                    {
                        rv.SetAsObjectInterface(obj);
                    }
                }
                else
                {
                    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                    if (VerboseActionErrors)
                        Log.LogScriptError("Error: The constructor function in 'cast' should have 'prototype'.\n");
                    #endif
                }
            }
        }
    }
    else
    {
        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
        if (VerboseActionErrors)
            Log.LogScriptError("Error: The parameter of 'cast' should be a function.\n");
        #endif
    }

    pEnv->Drop2();
    pEnv->Push(rv);
}

void GASExecutionContext::ImplementsOpCode()
{
    // Declare that a class s1 implements one or more
    // Interfaces (i2 == number of interfaces, s3..Sn are the names
    // of the interfaces).
    GASValue    ctorFuncVal(pEnv->Top());
    int         intfNum = pEnv->Top1().ToInt32(pEnv);
    pEnv->Drop2();

    if (ctorFuncVal.IsFunction())
    {
        GASFunctionRef ctorFunc = ctorFuncVal.ToFunction(pEnv);
        if (!ctorFunc.IsNull())
        {
            GASValue protoVal;
            if (ctorFunc->GetMemberRaw(pEnv->GetSC(), pEnv->GetBuiltin(GASBuiltin_prototype), &protoVal))
            {
                GASObject* proto = protoVal.ToObject(pEnv);
                if (proto != 0)
                {
                    proto->AddInterface(pEnv->GetSC(), intfNum, NULL);
                    for (int i = 0; i < intfNum; ++i)
                    {
                        const GASValue& intfVal = pEnv->Top(i);
                        if (intfVal.IsFunction())
                        {
                            GASFunctionRef intfFunc = intfVal.ToFunction(pEnv);
                            if (!intfFunc.IsNull())
                            {
                                proto->AddInterface(pEnv->GetSC(), i, intfFunc.GetObjectPtr());
                            }
                        }
                    }
                }
            }
            else
            {
                #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                if (VerboseActionErrors)
                    Log.LogScriptError("Error: The constructor function in 'implements' should have 'prototype'.\n");
                #endif
            }
        }
    }
    else
    {
        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
        if (VerboseActionErrors)
            Log.LogScriptError("Error: The parameter of 'implements' should be a function.\n");
        #endif
    }
    pEnv->Drop(intfNum);
}

void GASExecutionContext::EnumerateOpCode(int actionId)
{                   
    const GASObjectInterface* piobj     = 0;
    GASValue                  varName   = pEnv->Top();
    pEnv->Drop1();

    // The end of the enumeration
    GASValue nullvalue;
    nullvalue.SetNull();
    pEnv->Push(nullvalue);

    if (actionId == 0x55)
    {   
        // For opcode 0x55, top of stack IS the object.                 
        piobj = varName.ToObjectInterface(pEnv);
        if (!piobj)
            return;
    }
    else
    {
        GASString   varString(varName.ToString(pEnv));
        GASValue    variable;
        if (pEnv->GetVariable(varString, &variable, WithStack))
        {
            piobj = variable.ToObjectInterface(pEnv);
            if (!piobj)
                return;
        }
        else
            return;
    }

    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
    if (VerboseAction) 
        Log.LogAction("---enumerate - Push: NULL\n");
    #endif

    // Use enumerator class to handle members other then GASObject.
    // For example, we may enumerate members added to sprite.
    struct EnumerateOpVisitor : public GASObjectInterface::MemberVisitor
    {
        GASEnvironment*     pEnv;
        GFxActionLogger*    pLog;

        EnumerateOpVisitor(GASEnvironment* penv, GFxActionLogger* plog)
        { pEnv = penv; pLog = plog; }
        virtual void    Visit(const GASString& name, const GASValue&, UByte flags)
        {
            GUNUSED(flags);
            pEnv->Push(name);
            if (pLog)
                pLog->LogAction("---enumerate - Push: %s\n", name.ToCStr());
        }
    };

    // Visit all members, including prototype & child clips.
    EnumerateOpVisitor memberVisitor(pEnv, &Log);
    piobj->VisitMembers(pEnv->GetSC(), &memberVisitor, 
        GASObjectInterface::VisitMember_Prototype|
        GASObjectInterface::VisitMember_ChildClips|
        GASObjectInterface::VisitMember_NamesOnly);
}

void GASExecutionContext::ExtendsOpCode()
{   
    // Extends actually does the following:
    // Pop(Superclass)
    // Pop(Subclass)
    // Subclass.prototype = new Object();
    // Subclass.prototype.__proto__ = Superclass.prototype;
    // Subclass.prototype.__constructor__ = Superclass;                 
    GASValue superClassCtorVal = pEnv->Top();
    GASValue subClassCtorVal = pEnv->Top1();
    GASFunctionRef superClassCtor = superClassCtorVal.ToFunction(pEnv);
    GASFunctionRef subClassCtor = subClassCtorVal.ToFunction(pEnv);
    if (!superClassCtor.IsNull() && !subClassCtor.IsNull())
    {
        GASValue superProtoVal;
        if (!superClassCtor->GetMemberRaw(pEnv->GetSC(), pEnv->GetBuiltin(GASBuiltin_prototype), &superProtoVal) ||
            !superProtoVal.IsObject())
        {
            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
            if (VerboseActionErrors)
                Log.LogScriptError("Error: can't extend by the class w/o prototype.\n");
            #endif
        }
        else
        {
            GPtr<GASObject> superProto = superProtoVal.ToObject(pEnv);
            //!AB: need to create instance of ObjectProto to make sure "implements" and
            // "instanceof" work correctly.
            GPtr<GASObjectProto> newSubclassProto = *GHEAP_NEW(pEnv->GetHeap()) GASObjectProto(pEnv->GetSC(), superProto);
            //GPtr<GASObject> newSubclassProto = *GHEAP_NEW(pEnv->GetHeap()) GASObject(pEnv->GetSC(), superProto);

            subClassCtor->SetPrototype(pEnv->GetSC(), newSubclassProto);
            newSubclassProto->Set__constructor__(pEnv->GetSC(), superClassCtor);
        }
    }
    else 
    {
        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
        if (VerboseActionErrors)
        {
            if (superClassCtor.IsNull())
            {
                Log.LogScriptError("Error: can't extend with unknown super class.\n");
            }
            else
            {
                Log.LogScriptError("Error: can't extend the unknown class.\n");
            }
        }
        #endif
    }

    pEnv->Drop2();
}

void GASExecutionContext::InstanceOfOpCode()
{   
    const GASValue& ctorFuncVal = pEnv->Top();
    const GASValue& objVal = pEnv->Top1();
    bool rv = false;

    if (ctorFuncVal.IsFunction())
    {
        GASFunctionRef ctorFunc = ctorFuncVal.ToFunction(pEnv);
        if (!ctorFunc.IsNull())
        {
            GASObjectInterface* obj = objVal.ToObjectInterface(pEnv);
            if (obj != 0)
            {
                GASValue prototypeVal;
                if (ctorFunc->GetMemberRaw(pEnv->GetSC(), pEnv->GetBuiltin(GASBuiltin_prototype), &prototypeVal))
                {
                    GASObject* prototype = prototypeVal.ToObject(pEnv);
                    rv = obj->InstanceOf(pEnv, prototype);
                }
                else
                {
                    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                    if (VerboseActionErrors)
                        Log.LogScriptError("Error: The constructor function in InstanceOf should have 'prototype'.\n");
                    #endif
                }
            }
        }
    }
    else
    {
        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
        if (VerboseActionErrors)
            Log.LogScriptError("Error: The parameter of InstanceOf should be a function.\n");
        #endif
    }

    pEnv->Drop2();
    pEnv->Push(rv);
}                

void GASExecutionContext::WaitForFrameOpCode(GASActionBuffer* pActions, int actionId)
{   
    UInt frame = 0;
    UInt skipCount = 0;
    GFxSprite* ptargetSprite = pEnv->IsTargetValid() ? pEnv->GetTarget()->ToSprite() : 0;
    bool frameResolved = false;

    if (actionId == 0x8A )
    {
        frame  = pBuffer[PC + 3] | (pBuffer[PC + 4] << 8);
        skipCount = pBuffer[PC + 5];
        frameResolved = true;
    }
    else
    {
        frameResolved = pActions->ResolveFrameNumber(pEnv, pEnv->Top(), NULL, &frame);
        skipCount = pBuffer[PC + 3];
        pEnv->Drop1();
    }

    if (ptargetSprite && frameResolved)
    {           
        // if frame is bigger than totalFrames - cut it down to totalFrames
        UInt totalFrames = ptargetSprite->GetFrameCount();
        if (totalFrames > 0 && frame >= totalFrames)
            frame = totalFrames - 1;

        // If we haven't loaded a specified frame yet, then we're supposed to skip
        // some specified number of actions.
        if (frame >= ptargetSprite->GetLoadingFrame())
        {
            // need to calculate the offset by counting actions
            UInt len = pActions->GetLength();
            UInt curpc = (UInt)NextPC;
            for(UInt i = 0; i < skipCount && curpc < len; ++i)
            {
                UByte cmd = pBuffer[curpc++];
                if (cmd & 0x80) // complex action?
                {
                    // skip the content of action by getting its length
                    curpc += (pBuffer[curpc] | (pBuffer[curpc + 1] << 8));
                    curpc += 2; // skip the length itself
                }
            }
            UInt nextOffset = curpc;

            // Check ActionBuffer bounds.
            if (nextOffset >= len)
            {
                #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                if (VerboseActionErrors)
                    Log.LogScriptError(
                    "Error: WaitForFrame branch to offset %d - this section only runs to %d\n",
                    NextPC, StopPC);
                #endif
            }
            else
            {
                NextPC = (int)nextOffset;
            }
        }
    }

    // Fall through if this frame is loaded.
}

void GASExecutionContext::Function1OpCode(GASActionBuffer* pActions)
{
    GPtr<GASAsFunctionObject> funcObj = 
        *GHEAP_NEW(pEnv->GetHeap()) GASAsFunctionObject (pEnv, pActions, NextPC, 0, WithStack);

    int i = PC;
    i += 3;

    // Extract name.
    // @@ security: watch out for possible missing terminator here!
    GASString   name = pEnv->CreateString((const char*) &pBuffer[i]);
    i += (int)name.GetSize() + 1;

    // Get number of arguments.
    int nargs = pBuffer[i] | (pBuffer[i + 1] << 8);
    GASSERT((i + nargs*2) <= (int)pActions->GetLength());
    i += 2;

    // Get the names of the arguments.
    for (int n = 0; n < nargs; n++)
    {
        // @@ security: watch out for possible missing terminator here!
        GASString argstr(pEnv->CreateString((const char*) &pBuffer[i]));
        i += (int)funcObj->AddArg(0, argstr).Name.GetSize() + 1;
    }

    // Get the length of the actual function code.
    int length = pBuffer[i] | (pBuffer[i + 1] << 8);
    i += 2;
    funcObj->SetLength(length);

    // Skip the function Body (don't interpret it now).
    NextPC += length;

    // If we have a name, then save the function in this
    // environment under that name. FunctionValue will AddRef to the function.
    GASFunctionRef funcRef (funcObj);
    GASLocalFrame* plocalFrame = pEnv->GetTopLocalFrame ();
    if (plocalFrame)
    {
        funcRef.SetLocalFrame (plocalFrame);
#ifdef GFC_NO_GC
        plocalFrame->LocallyDeclaredFuncs.Set(funcObj.GetPtr(), 0);
#endif
    }
    GASValue    FunctionValue(funcRef);
    if (!name.IsEmpty())
    {
        // @@ NOTE: should this be Target->SetVariable()???
        //pEnv->SetMember(name, FunctionValue);
        pEnv->GetTarget()->SetMemberRaw(pEnv->GetSC(), name, FunctionValue);
        //pEnv->SetVariable(name, FunctionValue, WithStack);
    }

    // AB, Set prototype property for function
    // The prototype for function is the instance of FunctionProto. 
    // Also, the "constructor" property of this prototype
    // instance should point to GASAsFunction instance (funcDef)
    GPtr<GASFunctionProto> funcProto = 
        *GHEAP_NEW(pEnv->GetHeap()) GASFunctionProto(pEnv->GetSC(), pEnv->GetPrototype(GASBuiltin_Object), funcRef, false);
    funcRef->SetProtoAndCtor(pEnv->GetSC(), pEnv->GetPrototype(GASBuiltin_Function));
    funcObj->SetPrototype(pEnv->GetSC(), funcProto);

    if (name.IsEmpty())
    {
        // Also leave it on the stack, if function is anonymous
        pEnv->Push(FunctionValue);
    }

#ifdef GFX_AMP_SERVER
    if (GFxAmpServer::GetInstance().IsEnabled() && !name.IsEmpty())
    {
        // Actionscript profiling
        pEnv->GetMovieRoot()->AdvanceStats->RegisterScriptFunction(pActions->GetActionBufferData()->GetSwdHandle(), 
            pActions->GetActionBufferData()->GetSWFFileOffset() + funcObj->GetStartPC(), 
            name.ToCStr(), funcObj->GetLength());
    }
#endif  // GFX_AMP_SERVER
}

void GASExecutionContext::Function2OpCode(GASActionBuffer* pActions)
{
    GPtr<GASAsFunctionObject> funcObj = 
        *GHEAP_NEW(pEnv->GetHeap()) GASAsFunctionObject(pEnv, pActions, NextPC, 0, WithStack, GASActionBuffer::Exec_Function2);

    int i = PC;
    i += 3;

    // Extract name.
    // @@ security: watch out for possible missing terminator here!
    GASString   name = pEnv->CreateString((const char*) &pBuffer[i]);
    i += (int)name.GetSize() + 1;

    // Get number of arguments.
    int nargs = pBuffer[i] | (int(pBuffer[i + 1]) << 8);
    i += 2;

    // Get the count of local registers used by this function.
    UByte   RegisterCount = pBuffer[i];
    i += 1;
    funcObj->SetLocalRegisterCount(RegisterCount);

    // Flags, for controlling register assignment of implicit args.
    UInt16  flags = pBuffer[i] | (UInt16(pBuffer[i + 1]) << 8);
    i += 2;
    funcObj->SetFunction2Flags(flags);

    // Get the register assignments and names of the arguments.
    for (int n = 0; n < nargs; n++)
    {
        int ArgRegister = pBuffer[i];
        i++;

        UInt     length = (UInt)strlen((const char*) &pBuffer[i]);
        GASSERT((length + i) <= pActions->GetLength());
        GASString argstr(pEnv->CreateString((const char*) &pBuffer[i], length));

        // @@ security: watch out for possible missing terminator here!
        i += (int)funcObj->AddArg(ArgRegister, argstr).Name.GetSize() + 1;                    
    }

    // Get the length of the actual function code.
    int length = pBuffer[i] | (pBuffer[i + 1] << 8);
    i += 2;
    funcObj->SetLength(length);

    // Skip the function Body (don't interpret it now).
    NextPC += length;

    // If we have a name, then save the function in this
    // environment under that name. FunctionValue will AddRef to the function.
    GASFunctionRef funcRef (funcObj);
    GASLocalFrame* plocalFrame = pEnv->GetTopLocalFrame ();
    if (plocalFrame)
    {
        funcRef.SetLocalFrame (plocalFrame);
        #ifdef GFC_NO_GC
        plocalFrame->LocallyDeclaredFuncs.Set(funcObj.GetPtr(), 0);
        #endif
    }

    GASValue    FunctionValue(funcRef);
    if (!name.IsEmpty())
    {
        // @@ NOTE: should this be Target->SetVariable()???
        //pEnv->SetMember(name, FunctionValue);
        pEnv->GetTarget()->SetMemberRaw(pEnv->GetSC(), name, FunctionValue);
    }

    // AB, Set prototype property for function
    // The prototype for function is the instance of FunctionProto. 
    // Also, the "constructor" property of this prototype
    // instance should point to GASAsFunction instance (funcDef)
    GASStringContext*   psc = pEnv->GetSC();
    GPtr<GASFunctionProto> funcProto = 
        *GHEAP_NEW(psc->GetHeap()) GASFunctionProto(psc, pEnv->GetPrototype(GASBuiltin_Object), funcRef, false);
    funcRef->SetProtoAndCtor(psc, pEnv->GetPrototype(GASBuiltin_Function));

    funcObj->SetPrototype(psc, funcProto.GetPtr());

    if (name.IsEmpty())
    {
        // Also leave it on the stack if function is anonymous.
        pEnv->Push(FunctionValue);   
    }

#ifdef GFX_AMP_SERVER
    if (GFxAmpServer::GetInstance().IsEnabled() && !name.IsEmpty())
    {
        // Actionscript profiling
        pEnv->GetMovieRoot()->AdvanceStats->RegisterScriptFunction(pActions->GetActionBufferData()->GetSwdHandle(), 
            pActions->GetActionBufferData()->GetSWFFileOffset() + funcObj->GetStartPC(), 
            name.ToCStr(), funcObj->GetLength());
    }
#endif  // GFX_AMP_SERVER
    
}

#ifndef GFC_BUILD_DEBUG
#define GASInitBuffer(p)                (void)0
#define GASCheckBuffer(p)               (void)0

#define GASStringConstruct(source,p)    *::new(p) GASString(source)
#define GASStringDeconstruct(r)         (r).~GASString()

#define GASValueConstruct(source,p)     *::new(p) GASValue(source)
#define GASValueConstruct0(p)           *::new(p) GASValue()
#define GASValueDeconstruct(r)          (r).~GASValue()

#define GASFunctionRefConstruct(source,p)    *::new(p) GASFunctionRef(source)
#define GASFunctionRefDeconstruct(r)         (r).~GASFunctionRef()

#define GASFnCallConstruct(p1,p2,p3,p4,p5,pbuf)    *::new(pbuf) GASFnCall(p1,p2,p3,p4,p5)
#define GASFnCallDeconstruct(r)         (r).~GASFnCall()

#else
static void gfxbufassert(void* p) { GASSERT( *((UInt32*)p) == 0xABCDEF01u); }

#define GASInitBuffer(p)                *((UInt32*)p) = 0xABCDEF01;
#define GASCheckBuffer(p)               gfxbufassert(p)

#define GASStringConstruct(source,p)    (gfxbufassert(p),*::new(p) GASString(source))
#define GASStringDeconstruct(r)         do { (r).~GASString(); *((UInt32*)&r) = 0xABCDEF01; } while(0)

#define GASValueConstruct(source,p)     (gfxbufassert(p),*::new(p) GASValue(source))
#define GASValueConstruct0(p)           (gfxbufassert(p),*::new(p) GASValue())
#define GASValueDeconstruct(r)          do { (r).~GASValue(); *((UInt32*)&r) = 0xABCDEF01; } while(0)

#define GASFunctionRefConstruct(source,p)    (gfxbufassert(p),*::new(p) GASFunctionRef(source))
#define GASFunctionRefDeconstruct(r)         do { (r).~GASFunctionRef(); *((UInt32*)&r) = 0xABCDEF01; } while(0)

#define GASFnCallConstruct(p1,p2,p3,p4,p5,pbuf)    (gfxbufassert(pbuf),*::new(pbuf) GASFnCall(p1,p2,p3,p4,p5))
#define GASFnCallDeconstruct(r)         do { (r).~GASFnCall(); *((UInt32*)&r) = 0xABCDEF01; } while(0)
#endif

// Interpret the specified subset of the actions in our
// buffer.  Caller is responsible for cleaning up our local
// stack Frame (it may have passed its arguments in via the
// local stack frame).
// 
// The execType option determines whether to use global or local registers.
void    GASActionBuffer::Execute(
                        GASEnvironment* env,
                        int startPc,
                        int execBytes,
                        GASValue* retval,
                        const GASWithStackArray *pinitialWithStack,
                        ExecuteType execType)
{
    GASSERT(env);
    if (env->NeedTermination(execType))
        return;
    int tryCount = 0;
    env->EnteringExecution();

    GASExecutionContext execContext(env, pinitialWithStack, execType, GetBufferPtr(), pBufferData->GetFileName());

    bool isOriginalTargetValid = env->IsTargetValid();

    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
    if (execContext.VerboseAction) 
        execContext.Log.LogAction("\n");
    #endif

    execContext.StopPC = startPc + G_Min(execBytes, (int)GetLength());
    execContext.NextPC = startPc;
    execContext.PC     = startPc;

    // local variables used inside the 'switch' below.
    // DO NOT declare local variables inside the 'case-break' if possible.
    // This cause Execute to be very greedy for the stack size. It is 
    // critical for nested calls, since every call of ActionScript function
    // may consume up to 4K of stack, if local variables are declared inside
    // 'case-break' statements.
    GPtr<GASObject> pobj;
    union
    {
        UInt64          timerMs;
        GASNumber       nargsf;
        char            buf[8];
        wchar_t         wbuf[4];
    };
    char tmpStr1Buf[sizeof(GASString)];
    char tmpStr2Buf[sizeof(GASString)];
    char valBuf1[sizeof(GASValue)];
    char valBuf2[sizeof(GASValue)];
    union
    {
        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
        char tmpStr3Buf[sizeof(GASString)];
        #endif
        char valBuf3[sizeof(GASValue)];
    };
    char funcBuf[sizeof(GASFunctionRef)];
    char fnCallBuf[sizeof(GASFnCall)];

    GASInitBuffer(tmpStr1Buf);
    GASInitBuffer(tmpStr2Buf);
    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
    GASInitBuffer(tmpStr3Buf);
    #endif
    GASInitBuffer(valBuf1);
    GASInitBuffer(valBuf2);
    GASInitBuffer(valBuf3);
    GASInitBuffer(funcBuf);
    GASInitBuffer(fnCallBuf);

#ifdef GFX_AMP_SERVER
    GArrayLH<UInt64>* bufferTimes = NULL;
    bool shouldProfile = GFxAmpServer::GetInstance().IsProfiling();
    UInt32 samplingPeriod = GFxAmpServer::GetInstance().IsInstructionSampling() ? 50 : 0;
    GFxAmpViewStats* viewStats = env->GetMovieRoot()->AdvanceStats;

    // Function profiling
    ScopeFunctionTimer funcTimer(shouldProfile ? viewStats : NULL, pBufferData->GetSwdHandle(), 
                                 startPc + pBufferData->GetSWFFileOffset(), Amp_Profile_Level_Medium);
    
    if (shouldProfile)
    {
        if (GFxAmpServer::GetInstance().IsInstructionProfiling())
        {
            bufferTimes = &viewStats->LockBufferInstructionTimes(
                pBufferData->GetSwdHandle(), 
                pBufferData->GetSWFFileOffset(), 
                pBufferData->GetLength());

            viewStats->GetInstructionTime(samplingPeriod);
        }
    }

#endif  // GFX_AMP_SERVER

    while (execContext.PC < execContext.StopPC)
    {
        // Cleanup any expired "with" blocks.
        if (execContext.WithStack.pWithStackArray)
        {
            UPInt i, n = execContext.WithStack.pWithStackArray->GetSize();
            for (i = 0; i < n && execContext.PC >= execContext.WithStack.pWithStackArray->Back().GetBlockEndPc(); ++i)
                ;
            if (i > 0)
                execContext.WithStack.pWithStackArray->Resize(n - i);
        }

        // Get the opcode.
        int actionId = execContext.pBuffer[execContext.PC];

#ifdef GFXACTION_COUNT_OPS
        Execute_OpCounters[actionId]++;
#endif

        if ((actionId & 0x80) == 0)
        {
            execContext.NextPC = execContext.PC + 1;   // advance to next action.

            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
            if (execContext.VerboseAction) 
            {
                execContext.Log.LogAction("EX: %4.4X\t", execContext.PC);
                execContext.Log.LogDisasm(&execContext.pBuffer[execContext.PC]);
            }
            #endif

            // Simple action; no extra data.
            switch (actionId)
            {
                default:
                    break;

                case 0x00:  // end of actions.
                    execContext.NextPC = execContext.StopPC;
                    tryCount = 0; // prevents from executing "finally" blocks since if it is
                                  // normal termination (end-of-action code means "normal termination")
                                  // then all "finally" blocks should be already executed.
                    break;

                case 0x04:  // next frame.
                    if (env->IsTargetValid())
                        env->GetTarget()->GotoFrame(env->GetTarget()->GetCurrentFrame() + 1);
                    // check if it is necessary to terminate the execution, because the target 
                    // is unloaded or unloading. See comments for Bmination for more details.
                    if (env->NeedTermination(execType))
                        execContext.NextPC = execContext.StopPC;
                    break;

                case 0x05:  // prev frame.
                    if (env->IsTargetValid())
                        env->GetTarget()->GotoFrame(env->GetTarget()->GetCurrentFrame() - 1);
                    // check if it is necessary to terminate the execution, because the target 
                    // is unloaded or unloading. See comments for NeedTermination for more details.
                    if (env->NeedTermination(execType))
                        execContext.NextPC = execContext.StopPC;
                    break;

                case 0x06:  // action play
                    if (env->IsTargetValid())
                        env->GetTarget()->SetPlayState(GFxMovie::Playing);
                    break;

                case 0x07:  // action stop
                    if (env->IsTargetValid())
                        env->GetTarget()->SetPlayState(GFxMovie::Stopped);
                    break;

                case 0x08:  // toggle quality
                    break;

                case 0x09:  // stop sounds
                    {
#ifndef GFC_NO_SOUND
                        GFxMovieRoot* proot = env->GetMovieRoot();
                        if (proot)
                        {
                            GFxSprite* root = proot->GetLevelMovie(0);
                            if (root)
                                root->StopActiveSounds();
                        }
#endif
                    }
                    break;

                case 0x0A:  // add
                    env->Top1().Add(env, env->Top());
                    env->Drop1();
                    break;

                case 0x0B:  // subtract
                    env->Top1().Sub(env, env->Top());
                    env->Drop1();
                    break;

                case 0x0C:  // multiply
                    env->Top1().Mul(env, env->Top());
                    env->Drop1();
                    break;

                case 0x0D:  // divide
                    env->Top1().Div(env, env->Top());
                    env->Drop1();
                    break;

                case 0x0E:  // equal
                    env->Top1().SetBool(env->Top1().IsEqual (env, env->Top()));
                    env->Drop1();
                    break;

                case 0x0F:  // less than
                case 0x48:  // less Than (typed) (should they be implemented the same way?)
                    {
                        GASValue& result = GASValueConstruct(env->Top1().Compare (env, env->Top(), -1), valBuf1);
                        env->Top1() = result;
                        env->Drop1();
                        GASValueDeconstruct(result);
                    }
                    break;

                case 0x10:  // logical and
                    env->Top1().SetBool(env->Top1().ToBool(env) && env->Top().ToBool(env));
                    env->Drop1();
                    break;

                case 0x11:  // logical or
                    env->Top1().SetBool(env->Top1().ToBool(env) || env->Top().ToBool(env));
                    env->Drop1();
                    break;

                case 0x12:  // logical not
                    {
                        GASValue& topVal = env->Top();
                        // LOgical NOT is a very frequently executed op (used to invert conditions 
                        // for branching?). Optimize it for most common values.                    
                        if (topVal.GetType() == GASValue::BOOLEAN)
                        {
                            // Here, BOOLEAN is by far the most common type.
                            topVal.V.BooleanValue = !topVal.V.BooleanValue;
                        }
                        else  if (topVal.IsUndefined())
                        {
                            topVal.T.Type = GASValue::BOOLEAN;
                            topVal.V.BooleanValue = true;
                        }
                        else 
                        {
                            topVal.SetBool(!topVal.ToBool(env));
                        }
                        break;
                    }

                case 0x13:  // string equal
                {
                    GASString& s1 = GASStringConstruct(env->Top1().ToStringVersioned(env, execContext.Version), tmpStr1Buf);
                    GASString& s0 = GASStringConstruct(env->Top().ToStringVersioned(env, execContext.Version), tmpStr2Buf);
                    env->Top1().SetBool(s1 == s0);
                    env->Drop1();
                    GASStringDeconstruct(s1);
                    GASStringDeconstruct(s0);
                    break;
                }

                case 0x14:  // string length
                {
                    GASString& s1 = GASStringConstruct(env->Top().ToStringVersioned(env, execContext.Version), tmpStr1Buf);
                    env->Top().SetInt(s1.GetLength());
                    GASStringDeconstruct(s1);
                    break;
                }

                case 0x15:  // substring
                {
                    GASString& s2 = GASStringConstruct(env->Top(2).ToStringVersioned(env, execContext.Version), tmpStr1Buf);
                    GASString& retVal = GASStringConstruct(GASStringProto::StringSubstring(
                                        s2,
                                        env->Top1().ToInt32(env) - 1,
                                        env->Top().ToInt32(env) ), tmpStr2Buf);
                    env->Drop2();
                    env->Top().SetString(retVal);
                    GASStringDeconstruct(s2);
                    GASStringDeconstruct(retVal);
                    break;
                }

                case 0x17:  // pop
                    //!AB, here is something I can't understand for now: if class is
                    // declared as com.package.Class then AS code contains
                    // creation of "com" and "com.package" objects as "new Object".
                    // But, the problem is in one extra "pop" command after that, when
                    // the stack is already empty. Thus, I commented out the assert below
                    // for now....
                    //GASSERT(env->GetTopIndex() >= 0); //!AB
                    env->Drop1();
                    break;

                case 0x18:  // int
                    env->Top().SetInt(env->Top().ToInt32(env));
                    break;

                case 0x1C:  // get variable
                    {                    
                        // Use a reference to Top() here for efficiency to avoid value assignment overhead,
                        // as this is a very frequently executed op. This value is overwritten by the result.
                        GASValue&   variable = env->Top();
                        GASString&  varString = GASStringConstruct(variable.ToString(env), tmpStr1Buf);

                        // ATTN: Temporary hack to recognize "NaN", "Infinity" and "-Infinity".
                        // We want this to be as efficient as possible, so do a separate branch for case sensitivity.
                        if (env->IsCaseSensitive())
                        {
                            if (varString == env->GetBuiltin(GASBuiltin_NaN))
                                variable.SetNumber(GASNumberUtil::NaN());
                            else if (varString == env->GetBuiltin(GASBuiltin_Infinity))
                                variable.SetNumber(GASNumberUtil::POSITIVE_INFINITY());
                            else if (varString == env->GetBuiltin(GASBuiltin_minusInfinity_)) // "-Infinity"
                                variable.SetNumber(GASNumberUtil::NEGATIVE_INFINITY());
                            else
                            {
                                variable.SetUndefined();
                                env->GetVariable(varString, &variable, execContext.WithStack);
                            }
                        }
                        else
                        {
                            if (env->GetBuiltin(GASBuiltin_NaN).CompareBuiltIn_CaseInsensitive(varString))
                                variable.SetNumber(GASNumberUtil::NaN());
                            else if (env->GetBuiltin(GASBuiltin_Infinity).CompareBuiltIn_CaseInsensitive(varString))
                                variable.SetNumber(GASNumberUtil::POSITIVE_INFINITY());
                            else if (env->GetBuiltin(GASBuiltin_minusInfinity_).CompareBuiltIn_CaseInsensitive(varString))
                                variable.SetNumber(GASNumberUtil::NEGATIVE_INFINITY());
                            else
                            {
                                variable.SetUndefined();
                                env->GetVariable(varString, &variable, execContext.WithStack);
                            }
                        }

                        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                        if (execContext.VerboseAction) 
                        {    
                            GASString&  s0 = GASStringConstruct(variable.ToDebugString(env), tmpStr2Buf);
                            if (!variable.ToObjectInterface(env))
                            {
                                execContext.Log.LogAction("-- get var: %s=%s\n",
                                    varString.ToCStr(), s0.ToCStr());
                            }
                            else
                            {
                                execContext.Log.LogAction("-- get var: %s=%s at %p\n",
                                    varString.ToCStr(), s0.ToCStr(),
                                    variable.ToObjectInterface(env));
                            }
                            GASStringDeconstruct(s0);
                        }
                        #endif

                        GASStringDeconstruct(varString);
                        break;
                    }

                case 0x1D:  // set variable
                    {
                        GASString&  varString = GASStringConstruct(env->Top1().ToString(env), tmpStr1Buf);
                        env->SetVariable(varString, env->Top(), execContext.WithStack);
                        
                        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                        if (execContext.VerboseAction) 
                        {
                            GASString&  s0 = GASStringConstruct(env->Top1().ToDebugString(env), tmpStr2Buf);
                            execContext.Log.LogAction("-- set var: %s \n", s0.ToCStr());
                            GASStringDeconstruct(s0);
                        }
                        #endif

                        env->Drop2();
                        GASStringDeconstruct(varString);
                        break;
                    }

                case 0x20:  // set target expression (used with tellTarget)
                    execContext.SetTargetOpCode();
                    break;

                case 0x21:  // string concat
                    {
                        env->Top1().ConvertToStringVersioned(env, execContext.Version);
                        GASString&  str = GASStringConstruct(env->Top().ToStringVersioned(env, execContext.Version), tmpStr1Buf);
                        env->Top1().StringConcat(env, str);
                        env->Drop1();
                        GASStringDeconstruct(str);
                    }
                    break;

                case 0x22:  // get property
                {
                    GFxASCharacter* ptarget = env->FindTargetByValue(env->Top1());
                    if (ptarget)
                    {
                        // Note: GetStandardMember does bounds checks for opcodes.
                        ptarget->GetStandardMember((GFxASCharacter::StandardMember) env->Top().ToInt32(env), &env->Top1(), 1);
                    }
                    else
                    {
                        env->Top1().SetUndefined();
                    }
                    env->Drop1();
                    break;
                }

                case 0x23:  // set property
                {
                    GFxASCharacter* ptarget = env->FindTargetByValue(env->Top(2));
                    if (ptarget)
                    {
                        // Note: SetStandardMember does bounds checks for opcodes.
                        ptarget->SetStandardMember((GFxASCharacter::StandardMember) env->Top1().ToInt32(env), env->Top(), 1);
                    }
                    env->Drop3();
                    break;
                }

                case 0x24:  // duplicate Clip (sprite?)
                {
                    // MA: The target object can be a string, a path, or an object. 
                    // The duplicated clip will be created within its parent.
                    GFxASCharacter* ptarget = env->FindTargetByValue(env->Top(2));
                    if (ptarget)
                    {
                        // !AB: we don't need to add 16384 to the depth, like
                        // we do for MovieClip.duplicateMovieClip method, since
                        // this value already has been added: Flash generates "add_t 16384"
                        // for global function duplicateMovieClip call.
                        GASString&  str = GASStringConstruct(env->Top1().ToString(env), tmpStr1Buf);
                        ptarget->CloneDisplayObject(
                            str,
                            env->Top().ToInt32(env), 0);
                        GASStringDeconstruct(str);
                    }
                    env->Drop3();
                    break;
                }

                case 0x25:  // remove clip
                {
                    GFxASCharacter* ptarget = env->FindTargetByValue(env->Top());
                    if (ptarget)
                    {
                        if (ptarget->GetDepth() < 16384)
                        {
                            execContext.Log.LogScriptWarning("removeMovieClip(\"%s\") failed - depth must be >= 0\n",
                                ptarget->GetName().ToCStr());
                        }
                        else
                            ptarget->RemoveDisplayObject();
                    }
                    env->Drop1();
                    break;
                }

                case 0x26:  // trace
                {
                    // Log the stack val.
                    const GASFnCall& fnCall = GASFnCallConstruct(&env->Top(), NULL, env, 1, env->GetTopIndex(), fnCallBuf);
                    GAS_GlobalTrace(fnCall);
                    env->Drop1();
                    GASFnCallDeconstruct(fnCall);
                    break;
                }

                case 0x27:  // start drag GFxASCharacter
                    execContext.StartDragOpCode();
                    break;

                case 0x28:  // stop drag GFxASCharacter
                    {
                        GFxMovieRoot* proot = env->GetTarget()->GetMovieRoot();
                        GASSERT(proot);

                        proot->StopDrag();
                        break;
                    }

                case 0x29:  // string less than
                    {
                        GASString& s1 = GASStringConstruct(env->Top1().ToStringVersioned(env, execContext.Version), tmpStr1Buf);
                        GASString& s0 = GASStringConstruct(env->Top().ToStringVersioned(env, execContext.Version), tmpStr2Buf);
                        env->Top1().SetBool(s1 < s0);
                        env->Drop1();
                        GASStringDeconstruct(s0);
                        GASStringDeconstruct(s1);
                        break;
                    }

                case 0x2A:  // throw
                {
                    env->CheckTryBlocks(execContext.PC, &tryCount);
                    env->Throw(env->Top());
                    env->Drop1();
                    
                    execContext.NextPC = env->CheckExceptions(this, 
                        execContext.NextPC, 
                        &tryCount, 
                        retval,
                        execContext.WithStack,
                        execType);
                    break;
                }

                case 0x2B:  // CastObject
                    execContext.CastObjectOpCode();
                    break;

                case 0x2C:  // implements
                    execContext.ImplementsOpCode();
                    break;

                case 0x30:  // random
                {
                    SInt32 max = env->Top().ToInt32(env);
                    if (max < 1) 
                        max = 1;
                    env->Top().SetInt(GASMath::GetNextRandom(env->GetMovieRoot()) % max);
                    break;
                }

                case 0x31:  // mb length
                {
                    // Must use execContext.Version check here, for SWF <= 6, "undefined" length is 0.              
                    GASString& s = GASStringConstruct(env->Top().ToStringVersioned(env, execContext.Version), tmpStr1Buf);
                    UInt length = s.GetLength();
                    env->Top().SetInt(length);
                    GASStringDeconstruct(s);
                    break;
                }

                case 0x32:  // ord
                {
                    // ASCII code of first GFxCharacter
                    GASString& s = GASStringConstruct(env->Top().ToString(env), tmpStr1Buf);
                    env->Top().SetInt(s[0]);
                    GASStringDeconstruct(s);
                    break;
                }

                case 0x33:  // chr
                    buf[0] = char(env->Top().ToInt32(env));
                    buf[1] = 0;
                    env->Top().SetString(env->CreateString(buf));
                    break;

                case 0x34:  // get timer
                    // Push milliseconds since we started playing.
                    timerMs = env->GetTarget()->GetMovieRoot()->GetASTimerMs();
                    env->Push((GASNumber)timerMs);
                    break;

                case 0x35:  // mbsubstring
                {
                    GASString& s2     = GASStringConstruct(env->Top(2).ToStringVersioned(env, execContext.Version), tmpStr1Buf);
                    GASString& retVal = GASStringConstruct(GASStringProto::StringSubstring(
                        s2,
                        env->Top1().ToInt32(env) - 1,
                        env->Top().ToInt32(env) ), tmpStr2Buf);
                    env->Drop2();
                    env->Top().SetString(retVal);
                    GASStringDeconstruct(s2);
                    GASStringDeconstruct(retVal);
                    break;
                }

                case 0x36: // mbord
                {
                    // Convert first character to its numeric value.
                    // MA: This is correct for SWF6+ versions, but there is also some strange rounding
                    // that occurs if SWF 5 is selected. (i.e. mbord(mbchr(450)) returns 1). TBD.
                    GASString& s = GASStringConstruct(env->Top().ToStringVersioned(env, execContext.Version), tmpStr1Buf);
                    // Note: GetUTF8CharAt does bounds check, so return 0 to avoid accessing empty string.
                    env->Top().SetInt( (s.GetSize() == 0) ? 0 : s.GetCharAt(0) );
                    GASStringDeconstruct(s);
                    break;
                }

                case 0x37:  // mbchr
                    // Convert number to a multi-byte aware character string
                    wbuf[0] = (wchar_t)env->Top().ToUInt32(env);
                    wbuf[1] = 0;
                    env->Top().SetString(env->CreateString(wbuf));
                    break;

                case 0x3A:  // delete
                {                   
                    // This is used to remove properties from an object.
                    // AB: For Flash 6, if Top1 is null or undefined then delete works by the same way
                    // as delete2
                    bool retVal = false;
                    const GASValue& top1 = env->Top1();
                    const GASString& memberName = GASStringConstruct(env->Top().ToString(env), tmpStr1Buf);
                    if (execContext.Version <= 6 && (top1.IsNull() || top1.IsUndefined()))
                    {
                        GASValue& ownerVal = GASValueConstruct0(valBuf1);
                        if (env->FindOwnerOfMember(memberName, &ownerVal, execContext.WithStack))
                        {
                            GASObjectInterface *piobj = ownerVal.ToObjectInterface(env);
                            if (piobj)
                            {                                       
                                retVal = piobj->DeleteMember(env->GetSC(), memberName);
                            }
                        }
                        GASValueDeconstruct(ownerVal);
                    }
                    else
                    {
                        GASObjectInterface* piobj = env->Top1().ToObjectInterface(env);
                        if (piobj)
                            retVal = piobj->DeleteMember(env->GetSC(), memberName);
                    }
                    // Delete pops two parameters and push the return value (bool) into the stack.
                    // Usually it is removed by explicit "pop" operation, if return value is not used.
                    env->Drop1();
                    env->Top().SetBool(retVal);
                    GASStringDeconstruct(memberName);
                    break;
                }

                case 0x3B:  // delete2
                {
                    GASString& memberName       = GASStringConstruct(env->Top().ToString(env), tmpStr1Buf);
                    GASValue& ownerVal          = GASValueConstruct0(valBuf1);
                    GASObjectInterface *piobj   = NULL;

                    // !AB: 'delete' may be invoked with string parameter containing a whole
                    // path to the member being deleted: delete "obj.mem1.mem2". Or,
                    // it might be used with eval: delete eval("/anObject:member")
                    bool found = false;
                    if (memberName.IsNotPath() || !env->IsPath(memberName))
                    {
                        found = env->FindOwnerOfMember(memberName, &ownerVal, execContext.WithStack);
                    }
                    else
                    {
                        // In the case, if path is specified we need to resolve it,
                        // get the owner and modify the memberName to be relative to the
                        // found owner.
                        bool retVal = env->FindVariable(
                            GASEnvironment::GetVarParams(memberName, NULL, execContext.WithStack, NULL, &ownerVal), 
                            false, &memberName);
                        found = (retVal && !ownerVal.IsUndefined());
                    }

                    if (found)
                    {
                        piobj = ownerVal.ToObjectInterface(env);
                        if (piobj)
                        {                                       
                            env->Top().SetBool(piobj->DeleteMember(env->GetSC(), memberName));
                        }
                    }
                    if (!piobj)
                    {
                        env->Top().SetBool(false);
                    }
                    GASStringDeconstruct(memberName);
                    GASValueDeconstruct(ownerVal);
                    break;
                }

                case 0x3C:  // set local
                {                 
                    GASString& varname = GASStringConstruct(env->Top1().ToString(env), tmpStr1Buf);

                    // This is used in 'var' declarations.
                    // For functions, the value becomes local; for frame actions and events, it is global to clip.
                    if (execContext.IsFunction2 || (execType == GASActionBuffer::Exec_Function))
                        env->SetLocal(varname, env->Top());
                    else
                        env->SetVariable(varname, env->Top(), execContext.WithStack);

                    env->Drop2();
                    GASStringDeconstruct(varname);
                    break;
                }

                case 0x3D:  // call function
                {
                    GASValue*   pfunction = &GASValueConstruct0(valBuf1);
                    GASValue&   owner     = GASValueConstruct0(valBuf2);
                    bool        invokeFunction = true; 

                    if (env->Top().GetType() == GASValue::STRING)
                    {
                        // Function is a string; lookup the function.
                        const GASString& functionName = GASStringConstruct(env->Top().ToString(env), tmpStr1Buf);
                        env->GetVariable(functionName, pfunction, execContext.WithStack, 0, &owner);

                        if (!pfunction->IsFunction())
                        {
                            if (pfunction->IsObject() && (pobj = pfunction->ToObject(env)) && pobj->IsSuper())
                            {
                                // special case for "super": call to super.__constructor__ (SWF6+)
                                GASValueDeconstruct(*pfunction);

                                GASFunctionRef& ctor = GASFunctionRefConstruct(pobj->Get__constructor__(env->GetSC()), funcBuf);
                                pfunction = &GASValueConstruct(ctor, valBuf1);
                                GASFunctionRefDeconstruct(ctor);

                                //printf ("!!! function = %s\n", (const char*)env->GetGC()->FindClassName(function.ToObject()));
                                owner.SetAsObject(pobj);
                            }
                            else
                            {
                                #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                                if (execContext.VerboseActionErrors)
                                    execContext.Log.LogScriptError("Error: CallFunction - '%s' is not a function\n",
                                        functionName.ToCStr());
                                #endif
                                invokeFunction=false;
                            }
                            pobj = NULL; // dropref
                        }
                        GASStringDeconstruct(functionName);
                    }
                    else
                    {
                        // Hopefully the actual function object is here.
                        *pfunction = env->Top();
                    }

            
                    nargsf = env->Top1().ToNumber(env);
                    int nargs = (!GASNumberUtil::IsNaNOrInfinity(nargsf)) ? (int)nargsf : 0;
                    GASSERT(nargs >= 0);

                    GASValue&   result    = GASValueConstruct0(valBuf3);
                    if (invokeFunction)
                    {           
                        //!AB, Sometimes methods can be invoked as functions. For example,
                        // a movieclip "mc" has the method "foo" (mc.foo). If this method
                        // is invoked from "mc"'s timeline as "foo();" (not "this.foo();"!)
                        // then call_func opcode (0x3D) will be generated instead of call_method (0x52).
                        // In this case, "this" should be set to current target ("mc").
                        // Similar issue exists, if method is invoked from "with" statement:
                        // with (mc) { foo(); } // inside foo "this" should be set to "mc"
                        GASObjectInterface *ownerObj = owner.ToObjectInterface(env);
                        //GAS_Invoke(function, &result, ownerObj, env, nargs, env->GetTopIndex() - 2, NULL);
                        GASFunctionRef& func = GASFunctionRefConstruct(pfunction->ToFunction(env), funcBuf);
                        if (func != NULL)
                        {
                            const GASFnCall& fnCall = GASFnCallConstruct(&result, ownerObj, env, nargs, env->GetTopIndex() - 2, fnCallBuf);
                            func.Function->Invoke(fnCall, func.LocalFrame, NULL);
                            GASFnCallDeconstruct(fnCall);
                        }
                        else
                        {           
                            if (env && env->IsVerboseActionErrors())
                                env->LogScriptError("Error: CallFunction - attempt to call invalid function\n");
                        }
                        GASFunctionRefDeconstruct(func);
                    }
                    env->Drop(nargs + 1);
                    env->Top() = result;
                    GASValueDeconstruct(owner);
                    GASValueDeconstruct(*pfunction);
                    GASValueDeconstruct(result);

                    if (env->IsThrowing())
                    {
                        // exception handling
                        env->CheckTryBlocks(execContext.PC, &tryCount);
                        execContext.NextPC = env->CheckExceptions(this, 
                            execContext.NextPC, 
                            &tryCount, 
                            retval,
                            execContext.WithStack,
                            execType);
                    }
                    // check if it is necessary to terminate the execution, because the target 
                    // is unloaded or unloading. See comments for NeedTermination for more details.
                    if (env->NeedTermination(execType))
                        execContext.NextPC = execContext.StopPC;
                    break;
                }

                case 0x3E:  // return
                    // Put top of stack in the provided return slot, if
                    // it's not NULL.
                    if (retval)
                    {
                        *retval = env->Top();
                    }
                    env->Drop1();

                    // Skip the rest of this execContext.pBuffer (return from this GASActionBuffer).
                    execContext.NextPC = execContext.StopPC;

                    break;

                case 0x3F:  // modulo
                {
                    GASNumber   result;
                    GASNumber   y = env->Top().ToNumber(env);
                    GASNumber   x = env->Top1().ToNumber(env);
                    if (y != 0)
                        result = (GASNumber) fmod(x, y);
                    else
                        result = GASNumberUtil::NaN();

                    env->Drop2();
                    env->Push(result);
                    break;
                }
                case 0x40:  // new
                {                    
                    const GASString& classname = GASStringConstruct(env->Top().ToString(env), tmpStr1Buf);

                    nargsf = env->Top1().ToNumber(env);
                    int nargs = (!GASNumberUtil::IsNaNOrInfinity(nargsf)) ? (int)nargsf : 0;
                    GASSERT(nargs >= 0);

                    env->Drop2();

                    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                    if (execContext.VerboseAction) 
                        execContext.Log.LogAction("---new object: %s\n", classname.ToCStr());
                    #endif
                    
                    pobj = NULL; //reset
                    GASValue& constructor = GASValueConstruct0(valBuf1);

                    if (env->GetVariable(classname, &constructor, execContext.WithStack) && constructor.IsFunction())
                    {
                        GASFunctionRef& func = GASFunctionRefConstruct(constructor.ToFunction(env), funcBuf);
                        GASSERT (!func.IsNull ());

                        pobj = *env->OperatorNew(func, nargs);

                        if (!pobj)
                        {
                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                            if (execContext.VerboseActionErrors)
                                execContext.Log.LogScriptError("Error: can't create object with unknown class '%s'\n",
                                                   classname.ToCStr());
                            #endif
                        }
                        GASFunctionRefDeconstruct(func);
                    }
                    else
                    {
                        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                        if (execContext.VerboseActionErrors)
                            execContext.Log.LogScriptError("Error: can't create object with unknown class '%s'\n",
                                               classname.ToCStr());
                        #endif
                    }

                    env->Drop(nargs);
                    if (pobj)
                        env->Push(pobj);
                    else
                    {
                        GASValue& empty = GASValueConstruct0(valBuf2);
                        env->Push(empty);
                        GASValueDeconstruct(empty);
                    }
                    
                    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                    if (execContext.VerboseAction)
                        execContext.Log.LogAction("New object %s at %p\n", classname.ToCStr(), pobj.GetPtr());
                    #endif
                    pobj = NULL; //dropref
                    GASValueDeconstruct(constructor);
                    GASStringDeconstruct(classname);
                    
                    if (env->IsThrowing())
                    {
                        // exception handling
                        env->CheckTryBlocks(execContext.PC, &tryCount);
                        execContext.NextPC = env->CheckExceptions(this, 
                            execContext.NextPC, 
                            &tryCount, 
                            retval,
                            execContext.WithStack,
                            execType);
                    }
                    // check if it is necessary to terminate the execution, because the target 
                    // is unloaded or unloading. See comments for NeedTermination for more details.
                    if (env->NeedTermination(execType))
                        execContext.NextPC = execContext.StopPC;
                    break;
                }

                case 0x41:  // declare local
                {
                    const GASString& varname = GASStringConstruct(env->Top().ToString(env), tmpStr1Buf);
                    // This is used in 'var' declarations.
                    // For functions, the value becomes local; for frame actions and events, it is global to clip.
                    if (execContext.IsFunction2 || (execType == GASActionBuffer::Exec_Function))                
                        env->DeclareLocal(varname);
                    env->Drop1();
                    GASStringDeconstruct(varname);
                    break;
                }

                case 0x42:  // init array
                {               
                    int arraySize = (int) env->Top().ToNumber(env);
                    env->Drop1();

                    // Call the array constructor with the given elements as args
                    GASValue&   result = GASValueConstruct0(valBuf1);
                    const GASFnCall& fnCall = GASFnCallConstruct(&result, NULL, env, arraySize, env->GetTopIndex(), fnCallBuf);
                    GASArrayCtorFunction::DeclareArray(fnCall);
                    GASFnCallDeconstruct(fnCall);

                    if (arraySize > 0) 
                    {
                        env->Drop(arraySize);
                    }
                    // Push array 
                    env->Push(result);
                    GASValueDeconstruct(result);
                    break;
                }

                case 0x43:  // declare object
                {
                    // Use an initialize list to build an object.
                    int argCount    = (int) env->Top().ToNumber(env);
                    pobj            = *env->OperatorNew(env->GetBuiltin(GASBuiltin_Object));
                    env->Drop1();
                    
                    if (pobj)
                    {
                        for(int i = 0; i < argCount; i++)
                        {
                            // Pop {value, name} pairs.
                            if (env->GetTopIndex() >=1)
                            {
                                // Validity/null check.
                                if (env->Top1().GetType() == GASValue::STRING)
                                {
                                    const GASString& s = GASStringConstruct(env->Top1().ToString(env), tmpStr1Buf);
                                    pobj->SetMember(env, s, env->Top());
                                    GASStringDeconstruct(s);
                                }
                                env->Drop2();
                            }
                        }
                    }

                    env->Push(pobj.GetPtr());
                    pobj = NULL; // drop ref

                    if (env->IsThrowing())
                    {
                        // exception handling
                        env->CheckTryBlocks(execContext.PC, &tryCount);
                        execContext.NextPC = env->CheckExceptions(this, 
                            execContext.NextPC, 
                            &tryCount, 
                            retval,
                            execContext.WithStack,
                            execType);
                    }
                    // check if it is necessary to terminate the execution, because the target 
                    // is unloaded or unloading. See comments for NeedTermination for more details.
                    if (env->NeedTermination(execType))
                        execContext.NextPC = execContext.StopPC;
                    break;
                }
                    
                case 0x44:  // type of
                {
                    GASBuiltinType typeStrIndex = GASBuiltin_undefined;
                    const GASValue& top = env->Top();
                    switch(top.GetType())
                    {
                        case GASValue::UNDEFINED:
                        case GASValue::UNSET:
                            //typeStrIndex = GASBuiltin_undefined;                            
                            break;
                        case GASValue::STRING:
                            typeStrIndex = GASBuiltin_string;
                            break;
                        case GASValue::BOOLEAN:
                            typeStrIndex = GASBuiltin_boolean;
                            break;
                        case GASValue::CHARACTER:
                            {                           
                                GFxASCharacter* pchar = env->Top().ToASCharacter(env);
                                // ActionScript returns "movieclip" for null clips and sprite types only.
                                if (!pchar || (pchar && pchar->IsSprite()))
                                    typeStrIndex = GASBuiltin_movieclip;
                                else
                                    typeStrIndex = GASBuiltin_object;
                            }
                            break;

                        case GASValue::OBJECT:
                            typeStrIndex = GASBuiltin_object;
                            break;
                        case GASValue::NULLTYPE:
                            typeStrIndex = GASBuiltin_null;
                            break;
                        case GASValue::FUNCTION:
                            typeStrIndex = GASBuiltin_function;
                            break;
                        default:
                            if (top.IsNumber())
                                typeStrIndex = GASBuiltin_number;
                            else
                            {
                                #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                                if (execContext.VerboseActionErrors)
                                    execContext.Log.LogScriptError("Error: typeof unknown: %02X\n", env->Top().GetType());
                                #endif
                            }
                            break;
                        }

                    env->Top().SetString(env->GetBuiltin(typeStrIndex));
                    break;
                }

                case 0x45:  // get target path
                {
                    GFxASCharacter* ptarget = env->Top().ToASCharacter(env);
                    if (ptarget)             
                        env->Top().SetString(ptarget->GetCharacterHandle()->GetNamePath());
                    else
                        env->Top().SetUndefined();
                    break;
                }

                case 0x46:  // enumerate
                case 0x55:  // enumerate object 2
                    execContext.EnumerateOpCode(actionId);
                    break;

                case 0x47:  // AddT (typed)
                    env->Top1().Add (env, env->Top());
                    env->Drop1();
                    break;

                //case 0x48:  // less Than (typed)
                // see case 0x0F:

                case 0x49:  // Equal (typed)
                    // @@ identical to untyped equal, as far as I can tell...
                    env->Top1().SetBool(env->Top1().IsEqual (env, env->Top()));
                    env->Drop1();
                    break;

                case 0x4A:  // to number
                    env->Top().ConvertToNumber(env);
                    break;

                case 0x4B:  // to string
                    env->Top().ConvertToStringVersioned(env, execContext.Version);
                    break;

                case 0x4C:  // dup
                    env->Push(env->Top());
                    break;
                
                case 0x4D:  // swap
                {
                    GASValue& temp = GASValueConstruct(env->Top1(), valBuf1);
                    env->Top1() = env->Top();
                    env->Top() = temp;
                    GASValueDeconstruct(temp);
                    break;
                }
                
                case 0x4E:  // get member
                {               
                    // Use a reference to Top1 access since stack does not change here,
                    // and this is one of the most common ops.
                    GASValue&          top1Ref = env->Top1();
                    GASValue&          topRef  = env->Top();
                    GASObjectInterface *piobj = top1Ref.ToObjectInterface(env);

                    if (!piobj)
                    {
                        // get member for non-object case
                        // try to create temporary object and get member from it.
                        GASValue& objVal = GASValueConstruct(env->PrimitiveToTempObject(1), valBuf1);
                        if ((piobj = objVal.ToObject(env))!=0)
                        {
                            GASString& memberName = GASStringConstruct(topRef.ToString(env), tmpStr1Buf);
                            if (!env->GetMember(piobj, memberName, &top1Ref))
                            {
                                top1Ref.SetUndefined();
                            }
                            GASStringDeconstruct(memberName);
                        }
                        else
                            top1Ref.SetUndefined();
                        GASValueDeconstruct(objVal);
                    }
                    else
                    {
                        // A special case for handling indices in array
                        if (topRef.IsNumber() && piobj->GetObjectType() == GASObjectInterface::Object_Array)
                        {
                            GASArrayObject* parr = static_cast<GASArrayObject*>(piobj);
                            
                            // Save object ref just in case its released by SetUndefined; does not matter for characters.
                            pobj = parr;

                            SInt32 index = (SInt32)topRef.ToNumber(env);
                            if (index < 0)
                            {
                                // negative index - just use regular GetMember
                                GASString& memberName = GASStringConstruct(topRef.ToString(env), tmpStr1Buf);
                                if (!parr->GASObject::GetMember(env, memberName, &top1Ref))
                                    top1Ref.SetUndefined();
                                GASStringDeconstruct(memberName);
                            }
                            else if (index < parr->GetSize())
                            {
                                GASValue* v = parr->GetElementPtr(index);
                                if (v)
                                    top1Ref = *v;
                                else
                                    top1Ref.SetUndefined();
                            }
                            else
                                top1Ref.SetUndefined();

                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                            if (execContext.VerboseAction) 
                            {     
                                GASString& s1 = GASStringConstruct(top1Ref.ToDebugString(env), tmpStr2Buf);
                                execContext.Log.LogAction("-- GetMember %p[%d]=%s\n",
                                   parr, (int)index, s1.ToCStr());
                                GASStringDeconstruct(s1);
                            }
                            #endif
                            pobj = NULL; //drop
                        }
                        else
                        {
                            GASString& memberName = GASStringConstruct(topRef.ToString(env), tmpStr1Buf);
                            // Save object ref just in case its released by SetUndefined; does not matter for characters.
                            if (top1Ref.IsObject() || top1Ref.IsFunction()) // Avoid debug warning.
                                pobj = top1Ref.ToObject(env);

                            top1Ref.SetUndefined();

                            //printf ("!!! piobj.__proto__ = %s\n", (const char*)env->GetGC()->FindClassName(piobj->Get__proto__(env)));
                            env->GetMember(piobj, memberName, &top1Ref);

                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                            if (execContext.VerboseAction) 
                            {     
                                GASString& s1 = GASStringConstruct(top1Ref.ToDebugString(env), tmpStr2Buf);

                                if (top1Ref.ToObjectInterface(env) == NULL)
                                {                           
                                    execContext.Log.LogAction("-- GetMember %s=%s\n",
                                        memberName.ToCStr(), s1.ToCStr());
                                }
                                else
                                {                           
                                    execContext.Log.LogAction("-- GetMember %s=%s at %p\n",
                                        memberName.ToCStr(), s1.ToCStr(),
                                        top1Ref.ToObjectInterface(env));
                                }
                                GASStringDeconstruct(s1);
                            }
                            #endif
                            GASStringDeconstruct(memberName);
                            pobj = NULL; //drop
                        }
                    }

                    env->Drop1();
                    if (env->IsThrowing())
                    {
                        // exception handling
                        env->CheckTryBlocks(execContext.PC, &tryCount);
                        execContext.NextPC = env->CheckExceptions(this, 
                            execContext.NextPC, 
                            &tryCount, 
                            retval,
                            execContext.WithStack,
                            execType);
                    }
                    // Note: get member doesn't cause AS termination
                    break;                  
                }
                case 0x4F:  // set member
                {
                    GASObjectInterface* piobj = env->Top(2).ToObjectInterface(env);
                    if (piobj)
                    {
                        GASValue& top1Ref = env->Top1();
                        // A special case for handling indices in array
                        if (top1Ref.IsNumber() && piobj->GetObjectType() == GASObjectInterface::Object_Array)
                        {
                            GASArrayObject* parr = static_cast<GASArrayObject*>(piobj);
                            SInt32 index = (SInt32)top1Ref.ToNumber(env);
                            if (index >= 0)
                                parr->SetElementSafe((int)index, env->Top());
                            else
                            {
                                GASString& memberName = GASStringConstruct(top1Ref.ToString(env), tmpStr1Buf);
                                parr->GASObject::SetMember(env, memberName, env->Top());
                                GASStringDeconstruct(memberName);
                            }
                            
                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                            if (execContext.VerboseAction)
                            {
                                GASString& s0 = GASStringConstruct(env->Top().ToDebugString(env), tmpStr3Buf);
                                execContext.Log.LogAction("-- SetMember %p[%d]=%s\n",
                                    parr, 
                                    (int)index, 
                                    s0.ToCStr());
                                GASStringDeconstruct(s0);
                            }
                            #endif
                        }
                        else
                        {
                            GASString& memberName = GASStringConstruct(top1Ref.ToString(env), tmpStr1Buf);
                            piobj->SetMember(env, memberName, env->Top());

                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                            if (execContext.VerboseAction)
                            {
                                GASString& s2 = GASStringConstruct(env->Top(2).ToDebugString(env), tmpStr2Buf);
                                GASString& s0 = GASStringConstruct(env->Top().ToDebugString(env), tmpStr3Buf);
                                execContext.Log.LogAction("-- SetMember %s.%s=%s\n",
                                    s2.ToCStr(), 
                                    memberName.ToCStr(), 
                                    s0.ToCStr());
                                GASStringDeconstruct(s2);
                                GASStringDeconstruct(s0);
                            }
                            #endif
                            GASStringDeconstruct(memberName);
                        }
                    }
                    else
                    {
                        // Invalid object, can't set.
                        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                        if (execContext.VerboseAction)
                        {
                            GASString& s2 = GASStringConstruct(env->Top(2).ToDebugString(env), tmpStr1Buf);
                            GASString& s1 = GASStringConstruct(env->Top1().ToDebugString(env), tmpStr2Buf);
                            GASString& s0 = GASStringConstruct(env->Top().ToDebugString(env), tmpStr3Buf);
                            execContext.Log.LogAction(
                                    "- SetMember %s.%s=%s on invalid object\n",
                                    s2.ToCStr(), 
                                    s1.ToCStr(), 
                                    s0.ToCStr());
                            GASStringDeconstruct(s2);
                            GASStringDeconstruct(s1);
                            GASStringDeconstruct(s0);
                        }
                        #endif
                    }
                    env->Drop3();
                    if (env->IsThrowing())
                    {
                        // exception handling
                        env->CheckTryBlocks(execContext.PC, &tryCount);
                        execContext.NextPC = env->CheckExceptions(this, 
                            execContext.NextPC, 
                            &tryCount, 
                            retval,
                            execContext.WithStack,
                            execType);
                    }
                    // Note: set member doesn't cause AS termination
                    break;
                }

                case 0x50:  // increment
                    env->Top().Add(env, 1);
                    break;
                
                case 0x51:  // decrement
                    env->Top().Sub(env, 1);
                    break;
                
                case 0x52:  // call method
                {
                    nargsf = env->Top(2).ToNumber(env);
                    int nargs = (!GASNumberUtil::IsNaNOrInfinity(nargsf)) ? (int)nargsf : 0;
                    GASSERT(nargs >= 0);

                    GASValue&           result     = GASValueConstruct0(valBuf1);
                    const GASString&    methodName = GASStringConstruct(env->Top().ToString(env), tmpStr1Buf);
                    GASObjectInterface* piobj = 0;
                    GASValueGuard       valStorage(env, env->Top1());   

                    // If the method name is blank or undefined, the object is taken to be a function object
                    // that should be invoked.
                    if (env->Top().IsUndefined() || methodName.IsEmpty())
                    {
                        //!AB
                        // This code is invoked when nested function is called from parent function, for example:
                        //foo = function() {
                        //  var local = 10;
                        //  var nestedFoo = function () {}
                        //  nestedFoo(); // <- here
                        //}
                        // NOTE: in this case, "this" inside the "nestedFoo" is reported as "undefined",
                        // but this is not true: it is kinda "stealth" object. typeof(this) says this is 
                        // an "object". It is possible to get access to "local" by using "this.local". Also,
                        // it is possible to create local variable by setting "this.newLocal = 100;"
                        GASValue* pfunction = &GASValueConstruct(env->Top1(), valBuf2);

                        pobj = NULL; // reset
                        GASValue& thisVal = GASValueConstruct0(valBuf3);

                        if (pfunction->IsObject() && (pobj = pfunction->ToObject(env)) && pobj->IsSuper())
                        {
                            // special case for "super": call to super.__constructor__
                            GASValueDeconstruct(*pfunction);

                            GASFunctionRef& ctor = GASFunctionRefConstruct(pobj->Get__constructor__(env->GetSC()), funcBuf);
                            pfunction = &GASValueConstruct(ctor, valBuf2);
                            GASFunctionRefDeconstruct(ctor);

                            thisVal.SetAsObject(pobj);
                        }
                        else
                        {
                            env->GetVariable(env->GetBuiltin(GASBuiltin_this), &thisVal, execContext.WithStack);
                        }

                        if (pfunction->IsFunction())
                        {
                            //GAS_Invoke(function, &result, thisVal, env, nargs, env->GetTopIndex() - 3, NULL);
                            GASFunctionRef& func   = GASFunctionRefConstruct(pfunction->ToFunction(env), funcBuf);
                            if (func != NULL)
                            {
                                GASFnCall&      fnCall = GASFnCallConstruct(&result, thisVal, env, nargs, env->GetTopIndex() - 3, fnCallBuf); 
                                func.Invoke(fnCall, NULL);
                                GASFnCallDeconstruct(fnCall);
                            }
                            GASFunctionRefDeconstruct(func);
                        }
                        else
                        {
                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                            if (execContext.VerboseActionErrors)
                                execContext.Log.LogScriptError("Error: CallMethod \"as a function\" - pushed object is not a function object\n");
                            #endif
                        }
                        GASValueDeconstruct(thisVal);
                        GASValueDeconstruct(*pfunction);
                        pobj = NULL; // dropref
                    }

                    // Object or character.
                    else if (!env->Top1().IsFunction() && (piobj = env->Top1().ToObjectInterface(env)) != 0)
                    {
                        GASValue&    method = GASValueConstruct0(valBuf2);

                        // check, is the piobj super or not. If yes, get the correct "this" (piobj)
                        if (piobj->IsSuper())
                        {
                            //!AB: looks like we are calling something like "super.func()".
                            // The super object now contains a __proto__ pointing to the nearest
                            // base class. But, the nearest base class might not have the "func()"
                            // method, but the base of base call might have. 
                            // In this case we need to find the appropriate base class containing
                            // the calling method. Once we found it we set it as alternative prototype
                            // to the super object. It will be reseted to original prototype automatically 
                            // after call is completed.
                            GPtr<GASSuperObject> superObj = static_cast<GASSuperObject*>(piobj);
                            //printf ("!!! superObj->GetSuperProto() = %s\n", (const char*)env->GetGC()->FindClassName(env->GetSC(), superObj->GetSuperProto()).ToCStr());
                            pobj = superObj->GetSuperProto()->FindOwner(env->GetSC(), methodName); //newProto
                            if (pobj)
                            {
                                //printf ("!!! newProto = %s\n", (const char*)env->GetGC()->FindClassName(env->GetSC(), newProto).ToCStr());
                                superObj->SetAltProto(pobj);
                                pobj = NULL; //dropref
                            }
                            else
                                piobj = 0; // looks like there is no such method in super class
                        }
                        //printf ("!!! piobj.__proto__ = %s\n", (const char*)env->GetGC()->FindClassName(env->GetSC(), piobj->Get__proto__()).ToCStr());

                        if (piobj)
                        {
                            if (env->GetMember(piobj, methodName, &method))
                            {
                                if (!method.IsFunction())
                                {
                                    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                                    if (execContext.VerboseActionErrors)
                                    {
                                        if (piobj->IsASCharacter())
                                            execContext.Log.LogScriptError("Error: CallMethod - '%s.%s' is not a method\n",
                                                piobj->ToASCharacter()->GetCharacterHandle()->GetNamePath().ToCStr(), methodName.ToCStr());
                                        else
                                            execContext.Log.LogScriptError("Error: CallMethod - '%s' is not a method\n",
                                                methodName.ToCStr());                                      
                                    }
                                    #endif
                                }
                                else
                                {
                                    //                     GAS_Invoke(
                                    //                         method,
                                    //                         &result,
                                    //                         piobj,
                                    //                         env,
                                    //                         nargs,
                                    //                         env->GetTopIndex() - 3,
                                    //                         methodName.ToCStr());

                                    GASFunctionRef&  func = GASFunctionRefConstruct(method.ToFunction(env), funcBuf);
                                    if (func != NULL)
                                    {
                                        GASFnCall& fnCall = GASFnCallConstruct(&result, piobj, env, nargs, env->GetTopIndex() - 3, fnCallBuf); 
                                        func.Function->Invoke(fnCall, func.LocalFrame, methodName.ToCStr());
                                        GASFnCallDeconstruct(fnCall);
                                    }
                                    else
                                    {           
                                        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                                        if (execContext.VerboseActionErrors)
                                        {
                                            if (piobj->IsASCharacter())
                                                execContext.Log.LogScriptError(
                                                    "Error: Invoked method (%s.%s) is null\n", 
                                                    piobj->ToASCharacter()->GetCharacterHandle()->GetNamePath().ToCStr(), 
                                                    methodName.ToCStr());
                                            else
                                                execContext.Log.LogScriptError(
                                                    "Error: Invoked method (%s) is null\n", methodName.ToCStr());
                                        }
                                        #endif
                                    }
                                    GASFunctionRefDeconstruct(func);
                                }
                            }
                            else
                            {
                                #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                                if (execContext.VerboseActionErrors)
                                {
                                    if (piobj->IsASCharacter())
                                        execContext.Log.LogScriptError("Error: CallMethod - can't find method %s.%s\n",
                                        piobj->ToASCharacter()->GetCharacterHandle()->GetNamePath().ToCStr(), methodName.ToCStr());
                                    else
                                        execContext.Log.LogScriptError("Error: CallMethod - can't find method %s\n",
                                        methodName.ToCStr());                                      

                                }
                                #endif
                            }
                        }
                        GASValueDeconstruct(method);
                    }
                    else if (env->Top1().IsFunction())
                    {
                        // Looks like we want to call a static method.

                        // get the function object first
                        const GASValue& thisVal = env->Top1();
                        piobj = thisVal.ToObject(env);
                        if (piobj != 0) 
                        {
                            GASValue& method = GASValueConstruct0(valBuf2);
                            if (env->GetMember(piobj, methodName, &method))
                            {                                                       
                                // invoke method
                                //                 GAS_Invoke(
                                //                     method,
                                //                     &result,
                                //                     thisVal,
                                //                     env,
                                //                     //NULL, // this is null for static calls
                                //                     nargs,
                                //                     env->GetTopIndex() - 3,
                                //                     methodName.ToCStr());
                                GASFunctionRef&  func = GASFunctionRefConstruct(method.ToFunction(env), funcBuf);
                                if (func != NULL)
                                {
                                    GASFnCall& fnCall = GASFnCallConstruct(&result, thisVal, env, nargs, env->GetTopIndex() - 3, fnCallBuf); 
                                    func.Function->Invoke(fnCall, func.LocalFrame, methodName.ToCStr());
                                    GASFnCallDeconstruct(fnCall);
                                }
                                else
                                {           
                                    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS

                                    if (execContext.VerboseActionErrors)
                                    {
                                        if (piobj->IsASCharacter())
                                            execContext.Log.LogScriptError("Error: Invoked method (%s.%s) is not a function\n",
                                            piobj->ToASCharacter()->GetCharacterHandle()->GetNamePath().ToCStr(), methodName.ToCStr());
                                        else
                                            execContext.Log.LogScriptError("Error: Invoked method (%s) is not a function\n", methodName.ToCStr());

                                    }
                                    #endif
                                }
                                GASFunctionRefDeconstruct(func);
                            }
                            else
                            {
                                #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                                if (execContext.VerboseActionErrors)
                                {
                                    if (piobj->IsASCharacter())
                                        execContext.Log.LogScriptError("Error: Static method '%s.%s' is not found.\n",
                                            piobj->ToASCharacter()->GetCharacterHandle()->GetNamePath().ToCStr(), methodName.ToCStr());
                                    else
                                        execContext.Log.LogScriptError("Error: Static method '%s' is not found.\n",
                                                               methodName.ToCStr());
                                } 
                                #endif
                            }
                            GASValueDeconstruct(method);
                        }
                        else 
                        {
                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                            if (execContext.VerboseActionErrors)
                                execContext.Log.LogScriptError("Error: Function is not an object for static method '%s'.\n",
                                                                methodName.ToCStr());
                            #endif
                        }
                    }
                    else
                    {
                        // Handle methods on primitive types (string, boolean, number).
                        // In this case, an appropriate temporary object is created
                        // (String, Boolean, Number) and method is called for it.
                        GASValue& objVal = GASValueConstruct(env->PrimitiveToTempObject(1), valBuf2);
                        if (objVal.IsObject())
                        {
                            piobj = objVal.ToObject(env);
                            GASValue& method = GASValueConstruct0(valBuf3);
                            if (env->GetMember(piobj, methodName, &method))
                            {
                                if (method.GetType() != GASValue::FUNCTION)
                                {
                                    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                                    if (execContext.VerboseActionErrors)
                                    {
                                        if (piobj && piobj->IsASCharacter())
                                            execContext.Log.LogScriptError("Error: CallMethod - '%s.%s' is not a method\n",
                                                piobj->ToASCharacter()->GetCharacterHandle()->GetNamePath().ToCStr(),
                                                methodName.ToCStr());
                                        else
                                            execContext.Log.LogScriptError("Error: CallMethod - '%s' is not a method\n",
                                                methodName.ToCStr());
                                    }
                                    #endif
                                }
                                else
                                {
                                    //                     GAS_Invoke(
                                    //                         method,
                                    //                         &result,
                                    //                         piobj,
                                    //                         env,
                                    //                         nargs,
                                    //                         env->GetTopIndex() - 3,
                                    //                         methodName.ToCStr());
                                    GASFunctionRef&  func = GASFunctionRefConstruct(method.ToFunction(env), funcBuf);
                                    if (func != NULL)
                                    {
                                        GASFnCall& fnCall = GASFnCallConstruct(&result, piobj, env, nargs, env->GetTopIndex() - 3, fnCallBuf); 
                                        func.Function->Invoke(fnCall, func.LocalFrame, methodName.ToCStr());
                                        GASFnCallDeconstruct(fnCall);
                                    }
                                    else
                                    {           
                                        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                                        if (execContext.VerboseActionErrors)
                                        {
                                            if (piobj && piobj->IsASCharacter())
                                                execContext.Log.LogScriptError
                                                    ("Error: Invoked method (%s.%s) is not a function\n", 
                                                    piobj->ToASCharacter()->GetCharacterHandle()->GetNamePath().ToCStr(),
                                                    methodName.ToCStr());
                                            else
                                                execContext.Log.LogScriptError
                                                    ("Error: Invoked method (%s) is not a function\n", methodName.ToCStr());
                                        }
                                        #endif
                                    }
                                    GASFunctionRefDeconstruct(func);
                                }
                            }
                            else
                            {
                                #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                                if (piobj && piobj->IsASCharacter())
                                    execContext.Log.LogScriptError("Error: CallMethod - can't find method %s.%s\n",
                                        piobj->ToASCharacter()->GetCharacterHandle()->GetNamePath().ToCStr(), methodName.ToCStr());
                                else
                                    execContext.Log.LogScriptError("Error: CallMethod - can't find method %s\n",
                                        methodName.ToCStr());                                  
                                #endif
                            }
                            GASValueDeconstruct(method);
                            piobj = NULL;
                        }
                        else
                        {
                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                            if (execContext.VerboseActionErrors)
                                execContext.Log.LogScriptError("Error: CallMethod - '%s' on invalid object.\n",
                                                                methodName.ToCStr());
                            #endif
                        }
                        GASValueDeconstruct(objVal);
                    }

                    env->Drop(nargs + 2);
                    env->Top() = result;

                    if (env->IsThrowing())
                    {
                        // exception handling
                        env->CheckTryBlocks(execContext.PC, &tryCount);
                        execContext.NextPC = env->CheckExceptions(this, 
                            execContext.NextPC, 
                            &tryCount, 
                            retval,
                            execContext.WithStack,
                            execType);
                    }
                    // check if it is necessary to terminate the execution, because the target 
                    // is unloaded or unloading. See comments for NeedTermination for more details.
                    // "Call method" may terminate the ActionScript only if "this" is resolved
                    // and if it is a character - if it is not unloaded.
                    if (piobj)
                    {
                        GFxASCharacter* pasch = piobj->ToASCharacter();
                        if (!pasch || !pasch->IsUnloaded())
                        {
                            if (env->NeedTermination(execType))
                                execContext.NextPC = execContext.StopPC;
                        }
                    }
                    GASValueDeconstruct(result);
                    GASStringDeconstruct(methodName);
                    break;
                }
                case 0x53:  // new method
                {
                    GASValue&   constructorName = GASValueConstruct(env->Top(), valBuf1);
                    GASValue&   object          = GASValueConstruct(env->Top1(), valBuf2);
                    nargsf = env->Top(2).ToNumber(env);
                    int nargs = (!GASNumberUtil::IsNaNOrInfinity(nargsf)) ? (int)nargsf : 0;
                    GASSERT(nargs >= 0);
                    env->Drop3();

                    GASString&  constructorNameStr = GASStringConstruct(constructorName.ToString(env), tmpStr1Buf);

                    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                    if (execContext.VerboseAction) 
                        execContext.Log.LogAction("---new method: %s\n", constructorNameStr.ToCStr());
                    #endif

                    GASValue*   pconstructor;
                    if (constructorName.IsUndefined() || (constructorName.IsString() && constructorNameStr.IsEmpty()))
                    {
                        // if constructor method's name is blank use the "object" as function object
                        GASFunctionRef& ctor = GASFunctionRefConstruct(object.ToFunction(env), funcBuf);
                        pconstructor = &GASValueConstruct(ctor, valBuf3);
                        GASFunctionRefDeconstruct(ctor);
                    }
                    else
                    {
                        // MA: Can object be null?
                        // get the method
                        pconstructor = &GASValueConstruct0(valBuf3);
                        GASObjectInterface* piobj = object.ToObjectInterface(env);
                        if (!piobj || !env->GetMember(piobj, constructorNameStr, pconstructor))
                        {
                            // method not found!
                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                            if (execContext.VerboseActionErrors)
                            {
                                if (piobj && piobj->IsASCharacter())
                                    execContext.Log.LogScriptError("Error: Method '%s.%s' is not found.\n",
                                        piobj->ToASCharacter()->GetCharacterHandle()->GetNamePath().ToCStr(), 
                                        constructorNameStr.ToCStr());
                                else
                                    execContext.Log.LogScriptError("Error: Method '%s' is not found.\n",
                                        constructorNameStr.ToCStr());
                            }
                            #endif
                        }
                    }

                    if (pconstructor->IsFunction())
                    {
                        GASFunctionRef& func = GASFunctionRefConstruct(pconstructor->ToFunction(env), funcBuf);
                        GASSERT (func != NULL);

                        pobj = *env->OperatorNew(func, nargs);
                        GASFunctionRefDeconstruct(func);
                    }
                    else
                    {
                        pobj = NULL;
                        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                        if (execContext.VerboseActionErrors)
                            execContext.Log.LogScriptError("Error: can't create object with unknown ctor\n");
                        #endif
                    }

                    env->Drop(nargs);
                    env->Push(pobj);
                    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                    if (execContext.VerboseAction) 
                        execContext.Log.LogAction("New object created at %p\n", pobj.GetPtr());
                    #endif

                    pobj = NULL; //drop ref
                    GASValueDeconstruct(constructorName);
                    GASValueDeconstruct(object);
                    GASValueDeconstruct(*pconstructor);
                    GASStringDeconstruct(constructorNameStr);

                    if (env->IsThrowing())
                    {
                        // exception handling
                        env->CheckTryBlocks(execContext.PC, &tryCount);
                        execContext.NextPC = env->CheckExceptions(this, 
                            execContext.NextPC, 
                            &tryCount, 
                            retval,
                            execContext.WithStack,
                            execType);
                    }
                    // check if it is necessary to terminate the execution, because the target 
                    // is unloaded or unloading. See comments for NeedTermination for more details.
                    if (env->NeedTermination(execType))
                        execContext.NextPC = execContext.StopPC;
                    break;
                }
                case 0x54:  // instance of
                    execContext.InstanceOfOpCode();
                    break;

                //case 0x55: // enumerate object 2: see op code 0x46
                
                case 0x60:  // bitwise and
                    env->Top1().And (env, env->Top());
                    env->Drop1();
                    break;

                case 0x61:  // bitwise or
                    env->Top1().Or (env, env->Top());
                    env->Drop1();
                    break;

                case 0x62:  // bitwise xor
                    env->Top1().Xor (env, env->Top());
                    env->Drop1();
                    break;

                case 0x63:  // shift left
                    env->Top1().Shl(env, env->Top());
                    env->Drop1();
                    break;

                case 0x64:  // shift Right (signed)
                    env->Top1().Asr(env, env->Top());
                    env->Drop1();
                    break;

                case 0x65:  // shift Right (unsigned)
                    env->Top1().Lsr(env, env->Top());
                    env->Drop1();
                    break;

                case 0x66:  // strict equal
                    if (!env->Top1().TypesMatch(env->Top()))
                    {
                        // Types don't match.
                        env->Top1().SetBool(false);
                        env->Drop1();
                    }
                    else
                    {
                        env->Top1().SetBool(env->Top1().IsEqual (env, env->Top()));
                        env->Drop1();
                    }
                    break;

                case 0x67:  // Gt (typed)
                {
                    GASValue& result = GASValueConstruct(env->Top1().Compare (env, env->Top(), 1), valBuf1);
                    env->Top1() = result;
                    env->Drop1();
                    GASValueDeconstruct(result);
                    break;
                }

                case 0x68:  // string gt
                {
                    GASString& s1 = GASStringConstruct(env->Top1().ToStringVersioned(env, execContext.Version), tmpStr1Buf);
                    GASString& s0 = GASStringConstruct(env->Top().ToStringVersioned(env, execContext.Version), tmpStr2Buf);
                    env->Top1().SetBool(s1 > s0);
                    env->Drop1();
                    GASStringDeconstruct(s1);
                    GASStringDeconstruct(s0);
                    break;
                }

                case 0x69:  // extends
                    execContext.ExtendsOpCode();
                    break;
            }
        }
        else
        {
            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
            if (execContext.VerboseAction) 
            {
                execContext.Log.LogAction("EX: %4.4X\t", execContext.PC); 
                execContext.Log.LogDisasm(&execContext.pBuffer[execContext.PC]);
            }
            #endif

            // Action containing extra data.
            int actionLength = execContext.pBuffer[execContext.PC + 1] | (execContext.pBuffer[execContext.PC + 2] << 8);
            execContext.NextPC = execContext.PC + actionLength + 3;

            switch (actionId)
            {
            default:
                break;

            case 0x81:  // goto frame
            {
                // Used by gotoAndPlay(n), gotoAndStop(n) where n is a constant.
                // If n is a "string" use 0x8C, if variable - 0x9F.
                // Produced frame is already zero-based.
                if (env->IsTargetValid())
                {
                    int frame = execContext.pBuffer[execContext.PC + 3] | (execContext.pBuffer[execContext.PC + 4] << 8);
                    if (env->GetTarget())
                        env->GetTarget()->GotoFrame(frame);
                }
                // check if it is necessary to terminate the execution, because the target 
                // is unloaded or unloading. See comments for NeedTermination for more details.
                if (env->NeedTermination(execType))
                    execContext.NextPC = execContext.StopPC;
                break;
            }

            case 0x83:  // get url
            {               
                // Two strings as args.
                const char*     purl        = (const char*) &(execContext.pBuffer[execContext.PC + 3]);
                UInt            urlLen      = (UInt)strlen(purl);
                const char*     ptargetPath = (const char*) &(execContext.pBuffer[execContext.PC + 3 + urlLen + 1]);

                #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                if (execContext.VerboseAction) 
                    execContext.Log.LogAction("GetURL - path: %s  URL: %s", ptargetPath, purl);
                #endif
                
                // If the url starts with "FSCommand:" then this is a message for the host app,
                // so call the callback handler, if any.
                if (strncmp(purl, "FSCommand:", 10) == 0)
                {
                    ScopeFunctionTimer fsCommandTimer(env->GetMovieRoot()->AdvanceStats, 
                                            NativeCodeSwdHandle, Func_GFxFSCommandHandler_Callback, Amp_Profile_Level_Low);
                    GFxFSCommandHandler *phandler =
                        execContext.pOriginalTarget->GetMovieRoot()->pFSCommandHandler;
                    if (phandler)
                    {
                        // Call into the app.
                        phandler->Callback(env->GetTarget()->GetMovieRoot(), purl + 10, ptargetPath);
                    }
                }
                else
                {
                    // This is a loadMovie/loadMovieNum/unloadMovie/unloadMovieNum call.
                    env->GetMovieRoot()->AddLoadQueueEntry(ptargetPath, purl, env);
                }
                break;
            }

            case 0x87:  // StoreRegister
            {
                int reg = execContext.pBuffer[execContext.PC + 3];
                // Save top of stack in specified register.
                if (execContext.IsFunction2)
                {
                    *(env->LocalRegisterPtr(reg)) = env->Top();

                    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                    if (execContext.VerboseAction) 
                    {
                        GASString& s0 = GASStringConstruct(env->Top().ToDebugString(env), tmpStr1Buf);
                        execContext.Log.LogAction(
                                "-------------- local register[%d] = '%s'",
                                reg, s0.ToCStr());

                        GASObjectInterface* piobj = env->Top().ToObjectInterface(env);
                        if (piobj)
                           execContext.Log.LogAction(" at %p", piobj);
                        execContext.Log.LogAction("\n");
                        GASStringDeconstruct(s0);
                    }
                    #endif
                }
                else if (reg >= 0 && reg < 4)
                {
                    env->GetGlobalRegister(reg) = env->Top();
                
                    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                    if (execContext.VerboseAction)
                    {
                        GASString& s0 = GASStringConstruct(env->Top().ToDebugString(env), tmpStr1Buf);
                        execContext.Log.LogAction(
                                "-------------- global register[%d] = '%s'",
                                reg, s0.ToCStr());
                        
                        GASObjectInterface* piobj = env->Top().ToObjectInterface(env);
                        if (piobj)
                           execContext.Log.LogAction(" at %p", piobj);
                        execContext.Log.LogAction("\n");
                        GASStringDeconstruct(s0);
                    }
                    #endif
                }
                else
                {
                    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                    if (execContext.VerboseActionErrors)
                        execContext.Log.LogScriptError("Error: StoreRegister[%d] - register out of bounds!", reg);
                    #endif
                }

                break;
            }

            case 0x88:  // DeclDict: declare dictionary
            {
                int i = execContext.PC;
                //int   count = execContext.pBuffer[execContext.PC + 3] | (execContext.pBuffer[execContext.PC + 4] << 8);
                i += 2;

                ProcessDeclDict(env->GetSC(), execContext.PC, execContext.NextPC, execContext.Log);

                break;
            }

            case 0x8A:  // WaitForFrame
            case 0x8D:  // WaitForFrame2 (stack based)
                execContext.WaitForFrameOpCode(this, actionId);
                break;

            case 0x8B:  // set target (used for non-stacked tellTarget)
            {
                // Change the GFxASCharacter we're working on.
                const char* ptargetName = (const char*) &execContext.pBuffer[execContext.PC + 3];
                if (ptargetName[0] == 0)
                {
                    env->SetTarget(execContext.pOriginalTarget);
                }
                else
                {
                    GASString& targetName = GASStringConstruct(env->CreateString(ptargetName), tmpStr1Buf);
                    GFxASCharacter* ptarget = env->FindTarget(targetName);

                    //!AB: if ptarget is NULL, we need to set kind of fake target
                    // to avoid any side effects. Like:
                    // tellTarget(bullshit)
                    // {  gotoAndStop(1); } // if 'bullshit' is not found here,
                    // then gotoAndStop should do nothing
                    if (!ptarget) 
                    {
                        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                        if (execContext.VerboseActionErrors)
                            execContext.Log.LogScriptError("Error: SetTarget(tellTarget) with invalid target '%s'.\n",
                                               ptargetName);
                        #endif
                        env->SetInvalidTarget(execContext.pOriginalTarget);
                    }
                    else
                    {
                        env->SetTarget(ptarget);
                    }
                    GASStringDeconstruct(targetName);
                }
                break;
            }

            case 0x8C:  // go to labeled frame, GotoFrameLbl
            {
                // MA: This op does NOT interpret numbers in a string, so "4" is actually 
                // treated as a label and NOT as frame. This is different from ops and
                // functions with stack arguments, which DO try to parse frame number first.
                if (env->IsTargetValid())
                {
                    char*       pframeLabel = (char*) &execContext.pBuffer[execContext.PC + 3];
                    GFxSprite*  psprite = env->GetTarget()->ToSprite();
                    if (psprite)
                        psprite->GotoLabeledFrame(pframeLabel);
                }
                // check if it is necessary to terminate the execution, because the target 
                // is unloaded or unloading. See comments for NeedTermination for more details.
                if (env->NeedTermination(execType))
                    execContext.NextPC = execContext.StopPC;
                break;
            }

            case 0x8E:  // function2
                execContext.Function2OpCode(this);
                break;

            case 0x8F:  // try
            {
                env->CheckTryBlocks(execContext.PC, &tryCount);
                ++tryCount;
                GASEnvironment::TryDescr descr;
                descr.pTryBlock     = &execContext.pBuffer[execContext.PC + 3];
                descr.TryBeginPC    = execContext.NextPC;
                descr.TopStackIndex = env->GetTopIndex();
                env->PushTryBlock(descr);
                break;
            }

            case 0x94:  // with
            {
                //int frame = execContext.pBuffer[execContext.PC + 3] | (execContext.pBuffer[execContext.PC + 4] << 8);
                
                #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                if (execContext.VerboseAction) 
                    execContext.Log.LogAction("-------------- with block start: stack size is %d\n", (int)execContext.WithStack.GetSize());
                #endif
                if (execContext.WithStack.GetSize() < 8)
                {
                    int BlockLength = execContext.pBuffer[execContext.PC + 3] | (execContext.pBuffer[execContext.PC + 4] << 8);
                    int BlockEnd = execContext.NextPC + BlockLength;
                    
                    if (env->Top().IsCharacter())
                    {
                        GFxASCharacter* pwithChar = env->Top().ToASCharacter(env);
                        execContext.WithStack.PushBack(GASWithStackEntry(pwithChar, BlockEnd));
                    }
                    else
                    {
                        GASObject* pwithObj = env->Top().ToObject(env);
                        execContext.WithStack.PushBack(GASWithStackEntry(pwithObj, BlockEnd));
                    }               
                }
                env->Drop1();
                break;
            }

            case 0x96:  // PushData
            {
                SInt i = execContext.PC;

                // MA: Length must be greater then 0 here, otherwise push would make no sense;
                // so it shouldn't happen in practice. Hence, use do {} while for efficiency,
                // as PushData is the *most* common op. This assertion can be checked for by the 
                // bytecode verifier in the future (during action buffer Read, etc).
                GASSERT(actionLength > 0);
               
                do
                {
                    SPInt   type = execContext.pBuffer[3 + i];
                    i++;

                    // Push register is the most common value type.
                    // Push dictionary is the second common type.
                    if (type == 4)
                    {
                        // contents of register
                        SInt    reg = execContext.pBuffer[3 + i];
                        i++;
                        if (execContext.IsFunction2)
                        {
                            env->Push(*(env->LocalRegisterPtr(reg)));
                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                            if (execContext.VerboseAction) 
                            {
                                GASString& s = GASStringConstruct(env->Top().ToDebugString(env), tmpStr1Buf);
                                execContext.Log.LogAction(
                                    "-------------- pushed local register[%d] = '%s'",
                                    reg, s.ToCStr());

                                GASObjectInterface* piobj = env->Top().ToObjectInterface(env);
                                if (piobj)
                                    execContext.Log.LogAction(" at %p", piobj);                          
                                execContext.Log.LogAction("\n");
                                GASStringDeconstruct(s);
                            }
                            #endif
                        }
                        else if (reg < 0 || reg >= 4)
                        {
                            GASValue& empty = GASValueConstruct0(valBuf1);
                            env->Push(empty);
                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                            if (execContext.VerboseActionErrors)
                                execContext.Log.LogScriptError("Error: push register[%d] - register out of bounds\n", reg);
                            #endif
                            GASValueDeconstruct(empty);
                        }
                        else
                        {
                            env->Push(env->GetGlobalRegister(reg));
                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                            if (execContext.VerboseAction) 
                            {
                                GASString& s = GASStringConstruct(env->Top().ToDebugString(env), tmpStr1Buf);
                                execContext.Log.LogAction(
                                    "-------------- pushed global register[%d] = '%s'",
                                    reg, s.ToCStr());
                                GASObjectInterface* piobj = env->Top().ToObjectInterface(env);
                                if (piobj)
                                    execContext.Log.LogAction(" at %p", piobj);  
                                execContext.Log.LogAction("\n");
                                GASStringDeconstruct(s);
                            }
                            #endif
                        }

                    }
                    else if (type == 8)
                    {
                        UInt    id = execContext.pBuffer[3 + i];
                        i++;
                        if (id < Dictionary.GetSize())
                        {
                            // Push string directly with a copy constructor.
                            env->Push(Dictionary[id]);

                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                            if (execContext.VerboseAction) 
                                execContext.Log.LogAction("-------------- pushed '%s'\n", Dictionary[id].ToCStr());
                            #endif
                        }
                        else
                        {
                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                            if (execContext.VerboseActionErrors)
                                execContext.Log.LogScriptError("Error: DictLookup(%d) is out of bounds\n", id);
                            #endif
                            GASValue& empty = GASValueConstruct0(valBuf1);
                            env->Push(empty);
                            GASValueDeconstruct(empty);
                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                            if (execContext.VerboseAction) 
                                execContext.Log.LogAction("-------------- pushed 0\n");
                            #endif
                        }
                    }
                    else if (type == 0)
                    {
                        // string
                        GASString& str = GASStringConstruct(env->CreateString((const char*) &execContext.pBuffer[3 + i]), tmpStr1Buf);
                        i += (SInt)str.GetSize() + 1;
                        env->Push(str);

                        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                        if (execContext.VerboseAction) 
                            execContext.Log.LogAction("-------------- pushed '%s'\n", str.ToCStr());
                        #endif
                        GASStringDeconstruct(str);
                    }
                    else if (type == 1)
                    {
                        // Float (little-endian)
                        union {
                            Float   F;
                            UInt32  I;
                        } u;
                        GCOMPILER_ASSERT(sizeof(u) == sizeof(u.I));

                        memcpy(&u.I, &execContext.pBuffer[3 + i], 4);
                        u.I = GByteUtil::LEToSystem(u.I);
                        i += 4;

                        env->Push(u.F);

                        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                        if (execContext.VerboseAction) 
                            execContext.Log.LogAction("-------------- pushed '%f'\n", u.F);
                        #endif
                    }
                    else if (type == 2)
                    {                           
                        GASValue& nullValue = GASValueConstruct0(valBuf1);
                        nullValue.SetNull();
                        env->Push(nullValue);
                        GASValueDeconstruct(nullValue);

                        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                        if (execContext.VerboseAction) 
                            execContext.Log.LogAction("-------------- pushed NULL\n");
                        #endif
                    }
                    else if (type == 3)
                    {
                        GASValue& empty = GASValueConstruct0(valBuf1);
                        env->Push(empty);
                        GASValueDeconstruct(empty);

                        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                        if (execContext.VerboseAction) 
                            execContext.Log.LogAction("-------------- pushed UNDEFINED\n");
                        #endif
                    }
                    else if (type == 5)
                    {
                        bool    boolVal = execContext.pBuffer[3 + i] ? true : false;
                        i++;
                        env->Push(boolVal);

                        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                        if (execContext.VerboseAction) 
                            execContext.Log.LogAction("-------------- pushed %s\n", boolVal ? "true" : "false");
                        #endif
                    }
                    else if (type == 6)
                    {
                        // double
                        // wacky format: 45670123
#ifdef GFC_NO_DOUBLE
                        union
                        {
                            Float   F;
                            UInt32  I;
                        } u;

                        // convert ieee754 64bit to 32bit for systems without proper double
                        SInt    sign = (execContext.pBuffer[3 + i + 3] & 0x80) >> 7;
                        SInt    expo = ((execContext.pBuffer[3 + i + 3] & 0x7f) << 4) + ((execContext.pBuffer[3 + i + 2] & 0xf0) >> 4);
                        SInt    mant = ((execContext.pBuffer[3 + i + 2] & 0x0f) << 19) + (execContext.pBuffer[3 + i + 1] << 11) +
                            (execContext.pBuffer[3 + i + 0] << 3) + ((execContext.pBuffer[3 + i + 7] & 0xf8) >> 5);

                        if (expo == 2047)
                            expo = 255;
                        else if (expo - 1023 > 127)
                        {
                            expo = 255;
                            mant = 0;
                        }
                        else if (expo - 1023 < -126)
                        {
                            expo = 0;
                            mant = 0;
                        }
                        else
                            expo = expo - 1023 + 127;

                        u.I = (sign << 31) + (expo << 23) + mant;
                        i += 8;

                        env->Push((GASNumber)u.F);
                        
                        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                        if (execContext.VerboseAction) 
                            execContext.Log.LogAction("-------------- pushed double %f\n", u.F);
                        #endif
#else
                        union {
                            double  D;
                            UInt64  I;
                            struct {
                                UInt32  Lo;
                                UInt32  Hi;
                            } Sub;
                        } u;
                        GCOMPILER_ASSERT(sizeof(UInt32) == 4);
                        GCOMPILER_ASSERT(sizeof(u) == sizeof(u.I));

                        memcpy(&u.Sub.Hi, &execContext.pBuffer[3 + i], 4);
                        memcpy(&u.Sub.Lo, &execContext.pBuffer[3 + i + 4], 4);
                        u.I = GByteUtil::LEToSystem(u.I);
                        i += 8;

                        env->Push((GASNumber)u.D);

                        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                        if (execContext.VerboseAction) 
                            execContext.Log.LogAction("-------------- pushed double %f\n", u.D);
                        #endif
#endif
                    }
                    else if (type == 7)
                    {
                        // int32
                        SInt32  val = execContext.pBuffer[3 + i]
                            | (execContext.pBuffer[3 + i + 1] << 8)
                            | (execContext.pBuffer[3 + i + 2] << 16)
                            | (execContext.pBuffer[3 + i + 3] << 24);
                        i += 4;
                    
                        env->Push(val);

                        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                        if (execContext.VerboseAction) 
                            execContext.Log.LogAction("-------------- pushed int32 %d\n", (int)val);
                        #endif
                    }
                    else if (type == 9)
                    {
                        UInt    id = execContext.pBuffer[3 + i] | (execContext.pBuffer[4 + i] << 8);
                        i += 2;
                        if (id < Dictionary.GetSize())
                        {                            
                            env->Push(Dictionary[id]);
                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                            if (execContext.VerboseAction) 
                                execContext.Log.LogAction("-------------- pushed '%s'\n", Dictionary[id].ToCStr());
                            #endif
                        }
                        else
                        {
                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                            if (execContext.VerboseActionErrors)
                                execContext.Log.LogScriptError("Error: DictLookup(%d) is out of bounds\n", id);
                            #endif
                            GASValue& empty = GASValueConstruct0(valBuf1);
                            env->Push(empty);
                            GASValueDeconstruct(empty);

                            #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                            if (execContext.VerboseAction) 
                                execContext.Log.LogAction("-------------- pushed 0");
                            #endif
                        }
                    }

                } while (i - execContext.PC < actionLength);
                
                break;
            }
            case 0x99:  // branch Always (goto)
            {
                SInt16  offset = execContext.pBuffer[execContext.PC + 3] | (SInt16(execContext.pBuffer[execContext.PC + 4]) << 8);
                execContext.NextPC += offset;
                                  
                // Range checks.
                if (((UInt)execContext.NextPC) >= GetLength())
                {
                    // MA: Seems that ActionScript can actually jump outside of action buffer
                    // bounds into the binary content stored within other tags. Handling this
                    // would complicate things. Ex: f1.swf 
                    // Perhaps the entire SWF is stored within one chunk of memory and
                    // ActionScript goto instruction is welcome to seek arbitrarily in it ?!?

                    env->SetTarget(execContext.pOriginalTarget);
                    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                    if (execContext.VerboseActionErrors)
                        execContext.Log.LogScriptError(" Error: Branch destination %d is out of action buffer bounds!\n",
                                            execContext.NextPC);
                    #endif
                    return;
                }
                break;
            }
            case 0x9A:  // get url 2
            {
                UByte   method = execContext.pBuffer[execContext.PC + 3];

                GASString&   targetPath = GASStringConstruct(env->Top().ToString(env), tmpStr1Buf);
                GASString&   url        = GASStringConstruct(env->Top1().ToString(env), tmpStr2Buf);

                #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
                if (execContext.VerboseAction) 
                    execContext.Log.LogAction("GetURL2 - path: %s  URL: %s", targetPath.ToCStr(), url.ToCStr());
                #endif

                // If the url starts with "FSCommand:", then this is
                // a message for the host app.
                if (strncmp(url.ToCStr(), "FSCommand:", 10) == 0)
                {
                    GFxFSCommandHandler *phandler =
                        execContext.pOriginalTarget->GetMovieRoot()->pFSCommandHandler;
                    if (phandler)
                    {
                        // Call into the app.
                        phandler->Callback(env->GetTarget()->GetMovieRoot(),
                                           url.ToCStr() + 10, targetPath.ToCStr());
                    }
                }
                else
                {
                    GFxLoadQueueEntry::LoadMethod loadMethod = GFxLoadQueueEntry::LM_None;
                    switch(method & 3)
                    {
                        case 1: loadMethod = GFxLoadQueueEntry::LM_Get; break;
                        case 2: loadMethod = GFxLoadQueueEntry::LM_Post; break;
                    }

                    // 0x40 is set if target is a path to a clip; otherwise it is a path to a window
                    // 0x40 -> loadMovie(); if not loadURL()
                    // Note: loadMovieNum() can call this without target flag, so check url for _levelN.
                    const char* ptail = "";
                    int         level = GFxMovieRoot::ParseLevelName(
                                                targetPath.ToCStr(), &ptail,
                                                env->GetTarget()->IsCaseSensitive());
                    if (method & 0x80)
                    {   
                        // loadVars\loadVarsNum
                        env->GetMovieRoot()->AddVarLoadQueueEntry(targetPath.ToCStr(), url.ToCStr(), loadMethod);
                    }
                    else if ((method & 0x40) || ((level != -1) && (*ptail==0)))
                    {
                        // This is a loadMovie/loadMovieNum/unloadMovie/unloadMovieNum call.
                        env->GetMovieRoot()->AddLoadQueueEntry(targetPath.ToCStr(), url.ToCStr(), env, loadMethod);
                    }
                    else
                    {   
                        // ??? not sure what to do.
                    }
                
                }
                env->Drop2();
                GASStringDeconstruct(targetPath);
                GASStringDeconstruct(url);
                break;
            }

            case 0x9B:  // declare function
                execContext.Function1OpCode(this);
                break;

            case 0x9D:  // branch if true
            {
                SInt16  offset = execContext.pBuffer[execContext.PC + 3] | (SInt16(execContext.pBuffer[execContext.PC + 4]) << 8);
                
                bool    test = env->Top().ToBool(env);
                env->Drop1();
                if (test)
                {
                    execContext.NextPC += offset;

                    if (execContext.NextPC > execContext.StopPC)
                    {
                        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                        if (execContext.VerboseActionErrors)
                            execContext.Log.LogScriptError("Error: branch to offset %d - this section only runs to %d\n",
                                                execContext.NextPC,
                                                execContext.StopPC);
                        #endif
                    }
                }
                break;
            }
            case 0x9E:  // call frame
            {
                // Note: no extra data in this instruction!
                GFxASCharacter* ptarget  = env->GetTarget();
                GASSERT(ptarget);
                if (env->IsTargetValid())
                {
                    UInt frameNumber = 0;   
                    if (ResolveFrameNumber(env, env->Top(), &ptarget, &frameNumber))
                    {
                        GFxSprite* psprite = ptarget->ToSprite();
                        if (psprite)
                            psprite->CallFrameActions(frameNumber);
                    }
                    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
                    else if (execContext.VerboseActionErrors)
                    {
                        execContext.Log.LogScriptError("Error: %s, CallFrame('%s') - unknown frame\n", 
                            ptarget->GetCharacterHandle()->GetNamePath().ToCStr(), env->Top().ToString(env).ToCStr());
                    }
                    #endif

                    env->Drop1();
                }
           
                break;
            }

            case 0x9F:  // goto frame expression, GotoFrameExp
            {
                // From Alexi's SWF ref:
                //
                // Pop a value or a string and jump to the specified
                // frame. When a string is specified, it can include a
                // path to a sprite as in:
                // 
                //   /Test:55
                // 
                // When F_play is ON, the action is to play as soon as that 
                // frame is reached. Otherwise, the frame is shown in stop mode.

                UByte               optionFlag  = execContext.pBuffer[execContext.PC + 3];
                GFxMovie::PlayState state       = (optionFlag & 1) ?
                                                    GFxMovie::Playing : GFxMovie::Stopped;
                SInt                sceneOffset = 0;

                // SceneOffset is used if multiple scenes are created in Flash studio,
                // and then gotoAndPlay("Scene 2", val) is executed. The name of the
                // scene is converted into offset and passed here.
                if (optionFlag & 0x2)
                    sceneOffset = execContext.pBuffer[execContext.PC + 4] | (execContext.pBuffer[execContext.PC + 5] << 8);

                GFxASCharacter* target  = env->GetTarget();
                UInt frameNumber = 0;   
                if (ResolveFrameNumber(env, env->Top(), &target, &frameNumber))
                {
                    // There actually is a bug here *In Flash*. SceneOffset should not be 
                    // considered if we go to label, as labels already have absolute
                    // scene-inclusive offsets. However, we replicate their player bug.
                    // MA: Verified correct.
                    target->GotoFrame(frameNumber + sceneOffset);
                    target->SetPlayState(state);
                }
                
                env->Drop1();
                // check if it is necessary to terminate the execution, because the target 
                // is unloaded or unloading. See comments for NeedTermination for more details.
                if (env->NeedTermination(execType))
                    execContext.NextPC = execContext.StopPC;
                break;
            }
            }
        }

#ifdef GFX_AMP_SERVER
        if (bufferTimes != NULL)
        {
            GASSERT(execContext.PC < (int)bufferTimes->GetSize());
            (*bufferTimes)[execContext.PC] += viewStats->GetInstructionTime(samplingPeriod);
        }
#endif

        execContext.PC = execContext.NextPC;

        #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
        if (execContext.VerboseAction) 
            execContext.Log.LogAction ("==== stack top index %d ====\n", env->GetTopIndex());
        #endif
    }

#ifdef GFX_AMP_SERVER
    // Unlock the instruction times buffer
    if (bufferTimes)
    {
        env->GetMovieRoot()->AdvanceStats->ReleaseBufferInstructionTimes();
    }
#endif

    GASCheckBuffer(tmpStr1Buf);
    GASCheckBuffer(tmpStr2Buf);
    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
    GASCheckBuffer(tmpStr3Buf);
    #endif
    GASCheckBuffer(valBuf1);
    GASCheckBuffer(valBuf2);
    GASCheckBuffer(valBuf3);
    GASCheckBuffer(funcBuf);
    GASCheckBuffer(fnCallBuf);

    // Exception handling
    // check, if we still have finally blocks to execute.
    // It may end up here when "return" occurs from inside of the "try" or "catch" blocks
    if (tryCount > 0)
    {
        for (int tc = tryCount; tc > 0; --tc)
        {
            GASEnvironment::TryDescr tryDescr = env->PopTryBlock();

            // restore stack pointer
            UInt topIndex = env->GetTopIndex();
            if (topIndex > tryDescr.TopStackIndex)
                env->Drop(topIndex - tryDescr.TopStackIndex);
            if (tryDescr.IsFinallyBlockFlag())
            {
                // ok, we have a "finally" block. Need to redirect execution to there 
                // and then continue throwing.
                execContext.PC = tryDescr.TryBeginPC + tryDescr.GetTrySize() + tryDescr.GetCatchSize();
                env->SetUnrolling();
                Execute(env, execContext.PC, tryDescr.GetFinallySize(), retval, execContext.WithStack, execType);
                env->ClearUnrolling();
            }
        }
    }

    env->LeavingExecution();
    if (env->IsExecutionNestingLevelZero() && env->IsThrowing())
    {
        // unhandled exception?
        env->ClearThrowing();
    }
    
    // AB: if originally set target was "invalid" (means tellTarget was invoked with incorrect
    // target as a parameter) then we need restore it as "invalid"...
    if (isOriginalTargetValid)
        env->SetTarget(execContext.pOriginalTarget);
    else
        env->SetInvalidTarget(execContext.pOriginalTarget);
    #ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
    if (execContext.VerboseAction) 
        execContext.Log.LogAction("\n");
    #endif
}


//
// GASEnvironment
//
void GASEnvironment::CheckTryBlocks(int pc, int* plocalTryBlockCount)
{
    // Check, are we still inside the try blocks or not.
    // If not - remove them from TryBlocks array.
    GASSERT(plocalTryBlockCount);
    int localTryBlockCount = *plocalTryBlockCount;
    if (localTryBlockCount > 0)
    {
        for (; TryBlocks.GetSize() > 0 && localTryBlockCount >= 0; --localTryBlockCount)
        {
            const TryDescr& tryDescr = TryBlocks[TryBlocks.GetSize() - 1];
            if ((UInt)pc < tryDescr.TryBeginPC || (UInt)pc >= tryDescr.TryBeginPC + tryDescr.GetTrySize())
            {
                // need to remove this block since pc is out-of-bound this try-block
                --(*plocalTryBlockCount);
                TryBlocks.Resize(TryBlocks.GetSize() - 1);
            }
            else
                break;
        }
    }
}

// Handle exception (invoke catch, finally, etc) if necessary.
int GASEnvironment::CheckExceptions(GASActionBuffer* pactBuf, 
                                    int nextPc, 
                                    int* plocalTryBlockCount, 
                                    GASValue* retval,
                                    const GASWithStackArray* pwithStack,
                                    GASActionBuffer::ExecuteType execType)
{
    GASSERT(pactBuf);
    bool needContinueExecution;
    do 
    {
        needContinueExecution = false;
        if (IsThrowing())
        {
            for (int tc = *plocalTryBlockCount; tc > 0 && !needContinueExecution; --tc)
            {
                GASEnvironment::TryDescr tryDescr = PopTryBlock();
                --(*plocalTryBlockCount);

                // restore stack pointer
                UInt topIndex = GetTopIndex();
                if (topIndex > tryDescr.TopStackIndex)
                    Drop(topIndex - tryDescr.TopStackIndex);

                // do we have catch here?
                if (tryDescr.IsCatchBlockFlag())
                {
                    int localStackTop = 0;
                    // yes - put the thrown object in var/reg, 
                    // redirect execution to the catch block, clear throw state.
                    // It will run "finally" automatically.
                    if (tryDescr.IsCatchInRegister())
                        *(LocalRegisterPtr(tryDescr.GetCatchRegister())) = GetThrowingValue();
                    else
                    {
                        // create new local frame and save catch variable there
                        localStackTop = GetLocalFrameTop();
                        GPtr<GASLocalFrame> prevLocalFrame = GetTopLocalFrame();
                        GPtr<GASLocalFrame> curLocalFrame  = CreateNewLocalFrame();
                        curLocalFrame->PrevFrame = prevLocalFrame;

                        SetLocal(tryDescr.GetCatchName(this), GetThrowingValue());
                    }
                    ClearThrowing();
                    int catchPc = tryDescr.TryBeginPC + tryDescr.GetTrySize();

                    pactBuf->Execute(this, catchPc, tryDescr.GetCatchSize(), retval, pwithStack, execType);

                    nextPc = tryDescr.TryBeginPC + tryDescr.GetTrySize() + tryDescr.GetCatchSize() + tryDescr.GetFinallySize();

                    needContinueExecution = true;

                    if (tryDescr.IsCatchInRegister())
                        *(LocalRegisterPtr(tryDescr.GetCatchRegister())) = GASValue();
                    else
                    {
                        // Clean up stack frame.
                        SetLocalFrameTop(localStackTop);
                    }
                }
                if (tryDescr.IsFinallyBlockFlag())
                {
                    // ok, we have a "finally" block. Need to redirect execution to there 
                    // and then continue throwing.
                    int finallyPc = tryDescr.TryBeginPC + tryDescr.GetTrySize() + tryDescr.GetCatchSize();
                    SetUnrolling();
                    pactBuf->Execute(this, finallyPc, tryDescr.GetFinallySize(), retval, pwithStack, execType);
                    ClearUnrolling();
                }
            }
            if (!needContinueExecution)
                return pactBuf->GetLength(); // forces to exit
            // continue
        }
    } while (needContinueExecution);
    return nextPc;
}

void GASEnvironment::PushTryBlock(const TryDescr& descr)
{
    TryBlocks.PushBack(descr);
}
GASEnvironment::TryDescr GASEnvironment::PopTryBlock()
{
    GASSERT(TryBlocks.GetSize() > 0);
    TryDescr descr = TryBlocks[TryBlocks.GetSize()-1];
    TryBlocks.Resize(TryBlocks.GetSize()-1);
    return descr;
}
GASEnvironment::TryDescr& GASEnvironment::PeekTryBlock()
{
    GASSERT(TryBlocks.GetSize() > 0);
    return TryBlocks[TryBlocks.GetSize()-1];
}

bool GASEnvironment::IsInsideTryBlock(int pc)
{
    if (pc < 0 || TryBlocks.GetSize() == 0)
        return false;
    const TryDescr& tryDescr = TryBlocks[TryBlocks.GetSize()-1];
    return ((UInt)pc >= tryDescr.TryBeginPC && (UInt)pc < tryDescr.TryBeginPC + tryDescr.GetTrySize());
}

bool GASEnvironment::IsInsideCatchBlock(int pc)
{
    if (pc < 0 || TryBlocks.GetSize() == 0)
        return false;
    const TryDescr& tryDescr = TryBlocks[TryBlocks.GetSize()-1];
    UInt catchBeginPc = tryDescr.TryBeginPC + tryDescr.GetTrySize();
    return ((UInt)pc >= catchBeginPc && (UInt)pc < catchBeginPc + tryDescr.GetCatchSize());
}

bool GASEnvironment::IsInsideFinallyBlock(int pc)
{
    if (pc < 0 || TryBlocks.GetSize() == 0)
        return false;
    const TryDescr& tryDescr = TryBlocks[TryBlocks.GetSize()-1];
    UInt finallyBeginPc = tryDescr.TryBeginPC + tryDescr.GetTrySize() + tryDescr.GetCatchSize();
    return ((UInt)pc >= finallyBeginPc && (UInt)pc < finallyBeginPc + tryDescr.GetFinallySize());
}

// This GetMember method resolves properties (invokes getter methods) 
// and members (in the case if _resolve handler is set).
bool GASEnvironment::GetMember(GASObjectInterface* pthisObj, 
                               const GASString& memberName, 
                               GASValue* pdestVal)
{
    GASSERT(pthisObj && pdestVal);

    bool rv = pthisObj->GetMember(this, memberName, pdestVal);
    if (rv && pdestVal->IsProperty())
    {
        // resolve property - invoke getter
        // resolve property
        // check, is the object 'this' came from GFxASCharacter. If so,
        // use original GFxASCharacter as this.
        GPtr<GFxASCharacter> paschar = pthisObj->ToASCharacter();
        pdestVal->GetPropertyValue(this, (paschar)?(GASObjectInterface*)paschar.GetPtr():pthisObj, pdestVal);
    }
    else if (pdestVal->IsResolveHandler())
    {
        GASFunctionRef resolveHandler = pdestVal->ToResolveHandler();
        if (resolveHandler != NULL)
        {
            Push(memberName);
            GPtr<GFxASCharacter> paschar = pthisObj->ToASCharacter();
            pdestVal->SetUndefined();
            resolveHandler.Invoke(GASFnCall
                (pdestVal, 
                (paschar)?(GASObjectInterface*)paschar.GetPtr():pthisObj,  
                this, 1, GetTopIndex()));
            Drop1();
            rv = true;
        }
        else
            rv = false;
    }
    return rv;
}

// Target functions placed here because GFxAction.h does not know GFxASCharacter
void    GASEnvironment::SetTarget(GFxASCharacter* ptarget) 
{ 
    Target          = ptarget; 
    IsInvalidTarget = false;
    StringContext.UpdateVersion(ptarget->GetVersion());
}
void    GASEnvironment::SetInvalidTarget(GFxASCharacter* ptarget) 
{ 
    Target          = ptarget; 
    IsInvalidTarget = true;
    StringContext.UpdateVersion(ptarget->GetVersion());
}

// Used to set target right after construction
void    GASEnvironment::SetTargetOnConstruct(GFxASCharacter* ptarget)
{    
    SetTarget(ptarget);
    StringContext.pContext = ptarget->GetGC();  
}

// GFxLogBase overrides to support logging correctly
void        GASEnvironment::LogScriptError(const char* pfmt,...) const
{
    va_list argList; va_start(argList, pfmt);
    if(pASLogger)
        pASLogger->LogScriptMessageVarg(Log_ScriptError,pfmt,argList);
    else if (GetLog())
        GetLog()->LogMessageVarg(Log_ScriptError, pfmt, argList);
    va_end(argList);
}

void        GASEnvironment::LogScriptWarning(const char* pfmt,...) const
{
    va_list argList; va_start(argList, pfmt);
    if(pASLogger)
        pASLogger->LogScriptMessageVarg(Log_ScriptWarning,pfmt,argList);
    else if (GetLog())
        GetLog()->LogMessageVarg(Log_ScriptWarning, pfmt, argList);
    va_end(argList);
}

void        GASEnvironment::LogScriptMessage(const char* pfmt,...) const
{
    va_list argList; va_start(argList, pfmt);
    if(pASLogger)
        pASLogger->LogScriptMessageVarg(Log_ScriptMessage, pfmt, argList);
    else if (GetLog())
        GetLog()->LogMessageVarg(Log_ScriptMessage,pfmt,argList);
    va_end(argList);
}

GFxLog*     GASEnvironment::GetLog() const
{
    return Target->GetLog();
}

bool        GASEnvironment::IsVerboseAction() const
{   
    return Target->GetMovieRoot()->IsVerboseAction();
}

bool        GASEnvironment::IsVerboseActionErrors() const
{   
    return !Target->GetMovieRoot()->IsSuppressActionErrors();
}

GFxMovieRoot*   GASEnvironment::GetMovieRoot() const
{   
    return Target->GetMovieRoot();
}

void GASEnvironment::InvalidateOptAdvanceList() const
{
    if (Target)
        Target->GetMovieRoot()->InvalidateOptAdvanceList();
}

GASRefCountCollector* GASEnvironment::GetCollector()
{
    if (Target)
        return Target->GetMovieRoot()->GetASGC();
    return NULL;
}

bool        GASEnvironment::FindAndGetVariableRaw(const GASEnvironment::GetVarParams& params) const
{
    // GFxPath lookup rigmarole.
    // NOTE: IsPath caches 'NotPath' bit in varname string for efficiency.
    bool retVal;
    if (params.VarName.IsNotPath() || !IsPath(params.VarName))
    {
        retVal = GetVariableRaw(params);
    }
    else
    {
        GASValue owner;
        retVal = FindVariable(GetVarParams(params.VarName, params.pResult, params.pWithStack, params.ppNewTarget, &owner, params.ExcludeFlags), false);
        if (owner.IsUndefined())
        {
            if (!(params.ExcludeFlags & NoLogOutput))
                LogScriptError("Error: GetVariable failed: can't resolve the path \"%s\"\n", 
                params.VarName.ToCStr());
            retVal = false;
        }
        else
        {
            if (params.pOwner)
                *params.pOwner = owner;
        }
    }
    return retVal;
}

// Return the value of the given var, if it's defined.
// *ppnewTarget receives a new value if it changes.
// excludeFlags - mask consisted of ExcludeFlags bits.
bool        GASEnvironment::GetVariable(const GASString& varname, 
                                        GASValue* presult,
                                        const GASWithStackArray* pwithStack, 
                                        GFxASCharacter **ppnewTarget,
                                        GASValue* powner,
                                        UInt excludeFlags)
{
    // GFxPath lookup rigmarole.
    // NOTE: IsPath caches 'NotPath' bit in varname string for efficiency.
    bool retVal = FindAndGetVariableRaw(GetVarParams(varname,
                                                     presult,
                                                     pwithStack,
                                                     ppnewTarget,
                                                     powner,
                                                     excludeFlags));
    if (retVal && presult->IsProperty())
    {
        // resolve property. There is no explicit object here, so we assume
        // the owner of the property is "this".
        // check, is the object 'this' came from GFxASCharacter. If so,
        // use original GFxASCharacter as this.
        GASObjectInterface*  pobj = NULL;
        GPtr<GFxASCharacter> paschar;
        GASValue             thisVal;
        if (GetVariableRaw(GetVarParams(GetBuiltin(GASBuiltin_this), &thisVal, pwithStack)))
        {
            paschar = thisVal.ToASCharacter(this);
            if (!paschar)
                pobj    = thisVal.ToObject(this);            
        }
        else
            paschar = GetTarget();
        presult->GetPropertyValue(this, (paschar)?(GASObjectInterface*)paschar.GetPtr():pobj, presult);
    }
    else if (presult->IsResolveHandler())
    {
        GASObjectInterface*  pobj = NULL;
        GPtr<GFxASCharacter> paschar;
        GASValue             thisVal;
        if (GetVariable(GetBuiltin(GASBuiltin_this), &thisVal, pwithStack))
        {
            paschar = thisVal.ToASCharacter(this);
            if (!paschar)
                pobj    = thisVal.ToObject(this);            
        }
        else
            paschar = GetTarget();

        GASFunctionRef resolveHandler = presult->ToResolveHandler();
        Push(varname);
        presult->SetUndefined();
        resolveHandler.Invoke(GASFnCall
            (presult, 
            (paschar)?(GASObjectInterface*)paschar.GetPtr():pobj,  
            this, 1, GetTopIndex()));
        Drop1();
    }
    return retVal;
}


// Determine if a specified variable is available for access.
bool    GASEnvironment::IsAvailable(const GASString& varname, const GASWithStackArray* pwithStack) const
{
    if(varname.IsEmpty())
        return false;

    // *** GFxPath lookup.
    GFxASCharacter* target = Target;
    GASString       path(GetBuiltin(GASBuiltin_empty_));
    GASString       var(GetBuiltin(GASBuiltin_empty_));
    GASValue        val;
    GASStringContext* psc = GetSC();

    if (FindAndGetVariableRaw(GetVarParams(varname, &val, pwithStack, NULL, NULL, NoLogOutput)))
        return true;

    //!AB: do we still need this here after GetVariable?
    if (ParsePath(psc, varname, &path, &var))
    {
        if ((target = FindTarget(path, NoLogOutput))==0)
            return 0;
        return target->GetMemberRaw(psc, var, &val);
    }

    // *** NOT Path, do RAW variable check.

    if (pwithStack)
    {
        // Check the with-stack.
        for (int i = (int)pwithStack->GetSize() - 1; i >= 0; i--)
        {
            GASObjectInterface* obj = (*pwithStack)[i].GetObjectInterface();
            // Found the var in this context ?
            if (obj && obj->GetMemberRaw(psc, varname, &val))
                return true;
        }
    }

    if (FindLocal(varname))
        return true;
    if (Target && Target->GetMemberRaw(psc, varname, &val))
        return true;
    
    // Pre-canned names.
    if (IsCaseSensitive())
    {
        if (GetBuiltin(GASBuiltin_this) == varname ||
            GetBuiltin(GASBuiltin__root) == varname ||
            GetBuiltin(GASBuiltin__global) == varname)
            return true;
    }
    else
    {
        varname.ResolveLowercase();
        if (GetBuiltin(GASBuiltin_this).CompareBuiltIn_CaseInsensitive_Unchecked(varname) ||
            GetBuiltin(GASBuiltin__root).CompareBuiltIn_CaseInsensitive_Unchecked(varname) ||
            GetBuiltin(GASBuiltin__global).CompareBuiltIn_CaseInsensitive_Unchecked(varname))
            return true;
    }

    // Check for _levelN.
    if (varname[0] == '_')
    {
        const char* ptail = 0;
        SInt        level = GFxMovieRoot::ParseLevelName(varname.ToCStr(), &ptail,
                                                         IsCaseSensitive());
        if ((level != -1) && !*ptail )
        {
            if (Target->GetLevelMovie(level))
                return true;
        }
    }

    GASObject* pglobal = GetGC()->pGlobal;
    if (pglobal && pglobal->GetMemberRaw(psc, varname, &val))       
        return true;

    //  Not found -> not available.
    return false;
}

// excludeFlags: IgnoreWithoutOwners, IgnoreLocals
// varname must be a plain variable name; no path parsing.
bool    GASEnvironment::GetVariableRaw(const GASEnvironment::GetVarParams& params) const
{
    // Debug checks: these should never be called for path variables.
    GASSERT(strchr(params.VarName.ToCStr(), ':') == NULL);
    GASSERT(strchr(params.VarName.ToCStr(), '/') == NULL);
    GASSERT(strchr(params.VarName.ToCStr(), '.') == NULL);

    if (!params.pResult) return false;

    if (params.pOwner)
        *params.pOwner = 0;
    // Check the with-stack.
    if (params.pWithStack)
    {
        for (int i = (int)params.pWithStack->GetSize() - 1; i >= 0; i--)
        {
            GASObjectInterface* obj = (*params.pWithStack)[i].GetObjectInterface();
            
            if (obj == NULL)
            {
                //!AB, looks like, invalid obj in withStack should stop lookup
                // and return "undefined"
                return false;
            }

            if (obj && obj->GetMember(const_cast<GASEnvironment*>(this), params.VarName, params.pResult))
            {
                // Found the var in this context.
                if (params.pOwner)
                {
                    // Object interfaces can be characters or objects only.
                    if (obj->IsASCharacter())
                        params.pOwner->SetAsCharacter((GFxASCharacter*) obj);
                    else
                        params.pOwner->SetAsObject((GASObject*) obj);
                }
                return true;
            }
        }
    }

    if (!(params.ExcludeFlags & IgnoreLocals))
    {
        // Check locals.
        if (FindLocal(params.VarName, params.pResult))
        {
            // Get local var.
            return true;
        }
        else if (GetVersion() >= 5)
        {
            if (GetBuiltin(GASBuiltin_arguments).CompareBuiltIn_CaseCheck(params.VarName, IsCaseSensitive()))  
            {
                // lazy initialization of "arguments" array, required for Function1.
                GASLocalFrame* pLocFr = GetTopLocalFrame(0);
                if (pLocFr)
                {
                    GASEnvironment* penv = const_cast<GASEnvironment*>(this);
                    GASStringContext *psc = GetSC();

                    GPtr<GASArrayObject> pargArray = *GHEAP_NEW(GetHeap()) GASArrayObject(penv);
                    pargArray->Resize(pLocFr->NArgs);
                    for (int i = 0; i < pLocFr->NArgs; i++)
                        pargArray->SetElement(i, pLocFr->Arg(i));

                    penv->AddLocal(psc->GetBuiltin(GASBuiltin_arguments), GASValue(pargArray.GetPtr()));
                    pargArray->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_callee), pLocFr->Callee, 
                        GASPropFlags::PropFlag_DontEnum | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_ReadOnly);
                    pargArray->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_caller), pLocFr->Caller, 
                        GASPropFlags::PropFlag_DontEnum | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_ReadOnly);
                    params.pResult->SetAsObject(pargArray);
                    return true;
                }
            }
            else if (GetVersion() >= 6)
            {
                // Looking for "super" in SWF6?            
                if (GetBuiltin(GASBuiltin_super).CompareBuiltIn_CaseCheck(params.VarName, IsCaseSensitive()))                
                {
                    GASLocalFrame* pLocFr = GetTopLocalFrame(0);
                    if (pLocFr && pLocFr->SuperThis)
                    {
                        GASObjectInterface* const iobj = pLocFr->SuperThis;
                        GPtr<GASObject> superObj;
                        GPtr<GASObject> proto = iobj->Get__proto__();
                        //printf ("!!! savedThis.__proto__ = %s\n", (const char*)GetGC()->FindClassName(proto));
                        //printf ("!!! real pthis.__proto__ = %s\n", (const char*)GetGC()->FindClassName(pthis->Get__proto__(this)));

                        if (proto)
                        {
                            // need to get a real "this" to save in "super"
                            GASValue thisVal;
                            FindAndGetVariableRaw(GetVarParams(GetBuiltin(GASBuiltin_this), &thisVal, params.pWithStack));

                            GASFunctionRef __ctor__ = proto->Get__constructor__(GetSC());
                            //printf ("!!! __proto__.__ctor__ = %s\n", (const char*)pourEnv->GetGC()->FindClassName(__ctor__.GetObjectPtr()));
                            //printf ("!!! __proto__.__proto__ = %s\n", (const char*)pourEnv->GetGC()->FindClassName(proto->Get__proto__(pourEnv)));
                            superObj = *GHEAP_NEW(GetHeap()) GASSuperObject(proto->Get__proto__(), thisVal.ToObjectInterface(this), __ctor__);

                            params.pResult->SetAsObject(superObj);
                            //!AB, don't like this hack, but it is better to cache the "super"...
                            // don't want to remove the constness from GetVariable/Raw either
                            const_cast<GASEnvironment*>(this)->SetLocal(GetBuiltin(GASBuiltin_super), *params.pResult);
                            return true;
                        }
                    }
                }
            }
        }

        // Looking for "this"?
        if (GetBuiltin(GASBuiltin_this).CompareBuiltIn_CaseCheck(params.VarName, IsCaseSensitive()))
        {
            params.pResult->SetAsCharacter(Target);
            return true;
        }
    }

    // Check GFxASCharacter members.
    if (!Target) // Target should not be null, but just in case..
        return false;
    if (Target->GetMemberRaw(GetSC(), params.VarName, params.pResult))
    {
        if (params.pOwner)
            *params.pOwner = Target;
        return true; 
    }

    GASObject* pglobal = GetGC()->pGlobal;
    if (!(params.ExcludeFlags & IgnoreContainers))
    {
        // Check built-in constants.
        if (params.VarName.GetLength() > 0 && params.VarName[0] == '_')
        {
            Bool3W rv = CheckGlobalAndLevels(params);
            if (rv.IsDefined())
                return rv.IsTrue();
        }
    }

    if (pglobal && pglobal->GetMember(const_cast<GASEnvironment*>(this), params.VarName, params.pResult))
    {
        if (params.pOwner)
            *params.pOwner = pglobal;
        return true;
    }

    // Fallback.
    if (!(params.ExcludeFlags & NoLogOutput))
        LogAction("GetVariableRaw(\"%s\") failed, returning UNDEFINED.\n", params.VarName.ToCStr());
    return false;
}

Bool3W    GASEnvironment::CheckGlobalAndLevels(const GASEnvironment::GetVarParams& params) const
{
    // Handle _root and _global.
    if (IsCaseSensitive())
    {
        if (GetBuiltin(GASBuiltin__root) == params.VarName)
        {
            params.pResult->SetAsCharacter(Target->GetASRootMovie());
            return Bool3W(true);
        }
        else if (GetBuiltin(GASBuiltin__global) == params.VarName)
        {
            GASObject* pglobal = GetGC()->pGlobal;
            params.pResult->SetAsObject(pglobal);
            return Bool3W(true);
        }
    }
    else
    {
        if (GetBuiltin(GASBuiltin__root).CompareBuiltIn_CaseInsensitive(params.VarName))
        {
            params.pResult->SetAsCharacter(Target->GetASRootMovie());
            return Bool3W(true);
        }
        else if (GetBuiltin(GASBuiltin__global).CompareBuiltIn_CaseInsensitive(params.VarName))
        {
            GASObject* pglobal = GetGC()->pGlobal;
            params.pResult->SetAsObject(pglobal);
            return Bool3W(true);
        }
    }

    // Check for _levelN.
    const char* ptail = 0;
    SInt        level = GFxMovieRoot::ParseLevelName(params.VarName.ToCStr(), &ptail,
        IsCaseSensitive());
    if ((level != -1) && !*ptail)
    {
        GFxASCharacter* lm = Target->GetLevelMovie(level);
        if (lm)
        {
            params.pResult->SetAsCharacter(lm);
            return Bool3W(true);
        }
        return Bool3W(false);
    }
    return Bool3W();
}

// varname must be a plain variable name; no path parsing.
bool    GASEnvironment::FindOwnerOfMember(const GASString& varname,
                                          GASValue* presult,
                                          const GASWithStackArray* pwithStack) const
{
    GASSERT(strchr(varname.ToCStr(), ':') == NULL);
    GASSERT(strchr(varname.ToCStr(), '/') == NULL);
    GASSERT(strchr(varname.ToCStr(), '.') == NULL); 

    if (!presult) return false;

    if (pwithStack)
    {
        // Check the with-stack.
        for (int i = (int)pwithStack->GetSize() - 1; i >= 0; i--)
        {
            GASObjectInterface* obj = (*pwithStack)[i].GetObjectInterface();
            if (obj && obj->HasMember(GetSC(), varname, false))
            {
                // Found the var in this context.
                // Object interfaces can be characters or objects only.
                if (obj->IsASCharacter())
                    presult->SetAsCharacter((GFxASCharacter*) obj);
                else
                    presult->SetAsObject((GASObject*) obj);
                return true;
            }
        }
    }

    // Check GFxMovieSub members.
    if (!Target) // Target should not be null, but just in case..
        return false;
    if (Target->HasMember(GetSC(), varname, false))  
    {
        presult->SetAsCharacter(Target);
        return true;  
    }

    GASObject* pglobal = GetGC()->pGlobal;
    if (pglobal && pglobal->HasMember(GetSC(), varname, false))
    {
        presult->SetAsObject(pglobal);
        return true;  
    }
    return false;
}

// Given a path to variable, set its value.
bool    GASEnvironment::SetVariable(
                const GASString& varname,
                const GASValue& val,
                const GASWithStackArray* pwithStack,
                bool doDisplayErrors)
{
    if (IsVerboseAction())
    {
        GASString vs(GASValue(val).ToDebugString(this));
        LogAction("-------------- %s = %s\n", varname.ToCStr(), vs.ToCStr());
    }

    // GFxPath lookup rigamarole.
    // NOTE: IsPath caches 'NotPath' bit in varname string for efficiency.
    if (varname.IsNotPath() || !IsPath(varname))
    {
        SetVariableRaw(varname, val, pwithStack);
        // Essentially SetMember, which should not fail.
        return true;
    }
    else
    {
        GASValue owner;
        GASString var(GetBuiltin(GASBuiltin_empty_));
        GASValue curval;
        FindVariable(GetVarParams(varname, &curval, pwithStack, 0, &owner), false, &var);
        if (!owner.IsUndefined())
        {
            GASObjectInterface* pobj = owner.ToObjectInterface(this);
            GASSERT(pobj); // owner always should be an object!
            if (pobj)
            {
                pobj->SetMember(this, var, val);
                return true;
            }
        }
        else
        {
            if (doDisplayErrors && IsVerboseActionErrors())
                LogScriptError("Error: SetVariable failed: can't resolve the path \"%s\"\n", varname.ToCStr());
        }
    }    
    return false;
}

// No path rigamarole.
void    GASEnvironment::SetVariableRaw(
            const GASString& varname, const GASValue& val,
            const GASWithStackArray* pwithStack)
{
    if (pwithStack)
    {
        // Check the with-stack.
        for (int i = (int)pwithStack->GetSize() - 1; i >= 0; i--)
        {
            GASObjectInterface* obj = (*pwithStack)[i].GetObjectInterface();
            GASValue    dummy;
            if (obj && obj->GetMember(this, varname, &dummy))
            {
                // This object has the member; so set it here.
                obj->SetMember(this, varname, val);
                return;
            }
        }
    }

    // Check locals.
    GASValue* value = FindLocal(varname);
    if (value)
    {
        // Set local var.
        *value = val;
        return;
    }

    GASSERT(Target);
    Target->SetMember(this, varname, val);
}


// Set/initialize the value of the local variable.
void    GASEnvironment::SetLocal(const GASString& varname, const GASValue& val)
{
    if (LocalFrames.GetSize() == 0) //!AB if no local frame exist - do nothing
        return; 
    if (!LocalFrames[LocalFrames.GetSize()-1])
        return; 

    // Is it in the current frame already?
    GASValue* pvalue = FindLocal(varname);
    if (!pvalue)
    {
        GASSERT(!varname.IsEmpty());   // null varnames are invalid!
        
        // Not in frame; create a new local var.
        AddLocal (varname, val);
        //LocalFrames.PushBack(GASFrameSlot(varname, val));
    }
    else
    {
        // In frame already; modify existing var.
        //LocalFrames[index].Value = val;
        *pvalue = val;
    }
}


// Add a local var with the given name and value to our
// current local frame.  Use this when you know the var
// doesn't exist yet, since it's faster than SetLocal();
// e.G. when setting up args for a function.
void    GASEnvironment::AddLocal(const GASString& varname, const GASValue& val)
{
    GASSERT(varname.GetSize() > 0);

    //if (LocalFrames.GetSize() == 0) //!AB Create frame if no frame exists
    //  CreateNewLocalFrame();
    //!AB Don't do this, local frame should be created intentionally. If no local frame
    // exists - ASSERT.
    GASSERT(LocalFrames.GetSize() > 0);

    GPtr<GASLocalFrame> frame = LocalFrames[LocalFrames.GetSize()-1];
    if (frame)
        frame->Variables.SetCaseCheck(varname, val, IsCaseSensitive());
}


// Create the specified local var if it doesn't exist already.
void    GASEnvironment::DeclareLocal(const GASString& varname)
{
    if (LocalFrames.GetSize() == 0) //!AB if no local frame exist - do nothing
        return; 
    if (!LocalFrames[LocalFrames.GetSize()-1])
        return; 

    // Is it in the current frame already?
    GASValue* pvalue = FindLocal(varname);
    if (!pvalue)
    {
        // Not in frame; create a new local var.
        GASSERT(varname.GetSize() > 0);   // null varnames are invalid!
        
        AddLocal(varname, GASValue());
    }
    else
    {
        // In frame already; don't mess with it.
    }
}

GASValue*   GASEnvironment::LocalRegisterPtr(UInt reg)
// Return a pointer to the specified local register.
// Local registers are numbered starting with 0.
// MA: Although DefineFunction2 index 0 has a special meaning,
// ActionScript code store_register still references it occasionally.
//
// Return value will never be NULL.  If reg is out of bounds,
// we log an error, but still return a valid Pointer (to
// global reg[0]).  So the behavior is a bit undefined, but
// not dangerous.
{
    // We index the registers from the end of the register
    // GArray, so we don't have to keep base/frame
    // pointers.

    if (reg >= LocalRegister.GetSize())
    {       
        LogError("Invalid local register %d, stack only has %d entries\n",
                 reg, (int)LocalRegister.GetSize());

        // Fallback: use global 0.
        return &GlobalRegister[0];
    }

    return &LocalRegister[LocalRegister.GetSize() - reg - 1];
}

const GASValue* GASEnvironment::FindLocal(const GASString& varname) const
{
    if (LocalFrames.GetSize() > 0) 
    {
        GPtr<GASLocalFrame> localFrame = GetTopLocalFrame();
        if (!localFrame) return false;
        do {
            const GASValue* ptr = localFrame->Variables.GetCaseCheck(varname, IsCaseSensitive());
            if (ptr)
                return ptr;
            else 
            {
                int version = GetVersion();
                if ((version >= 5 && GetBuiltin(GASBuiltin_arguments).CompareBuiltIn_CaseCheck(varname, IsCaseSensitive())) ||
                    (version >= 6 && GetBuiltin(GASBuiltin_super).CompareBuiltIn_CaseCheck(varname, IsCaseSensitive())))  
                {
                    // do not propagate requests for "arguments" and "super" in upper local frames;
                    // they would be substituted in GetVariableRaw.
                    break;
                }
            }

            localFrame = localFrame->PrevFrame;
        } while (localFrame);
    }
    return NULL;
}

GASLocalFrame* GASEnvironment::GetTopLocalFrame (int off) const 
{ 
    if (LocalFrames.GetSize() - off > 0)
    {
        return LocalFrames[LocalFrames.GetSize() - off - 1]; 
    }
    return 0;
}

GASLocalFrame*  GASEnvironment::CreateNewLocalFrame ()
{
    GPtr<GASLocalFrame> frame = *GHEAP_NEW(GetHeap()) GASLocalFrame(GetCollector());
    LocalFrames.PushBack(frame);
    return frame;
}


// See if the given variable name is actually a sprite path
// followed by a variable name.  These come in the format:
//
//  /path/to/some/sprite/:varname
//
// (or same thing, without the last '/')
//
// or
//  path.To.Some.Var
//
// If that's the format, puts the path Part (no colon or
// trailing slash) in *path, and the varname Part (no color)
// in *var and returns true.
//
// If no colon, returns false and leaves *path & *var alone.
bool    GASEnvironment::ParsePath(GASStringContext* psc, const GASString& varPath, GASString* ppath, GASString* pvar)
{
    // Search for colon.
    int         colonIndex = -1;
    const char* cstr = varPath.ToCStr();
    const char* p;
    
    if ((p = strchr(cstr, ':')) != 0)
        colonIndex = int(p - cstr);
    else
    {
        if ((p = strrchr(cstr, '.')) != 0)
            colonIndex = int(p - cstr);
        else
        {
            if ((p = strrchr(cstr, '/')) == 0)
                return false;
        }
    }

    // Make the subparts.

    // Var.
    if (colonIndex >= 0)
        *pvar = psc->CreateString(varPath.ToCStr() + colonIndex + 1);
    else
        *pvar = psc->GetBuiltin(GASBuiltin_empty_);

    // GFxPath.
    if (colonIndex > 0)
    {
        if (varPath[colonIndex - 1] == '/')
        {
            // Trim off the extraneous trailing slash.
            colonIndex--;
        }
    }
    
    // @@ could be better.  This whole usage of GString is very flabby...
    if (colonIndex >= 0)
        *ppath = psc->CreateString(varPath.ToCStr(), colonIndex);
    else
        *ppath = varPath;

    return true;
}

bool    GASEnvironment::IsPath(const GASString& varPath)
{
    // This should be checked for externally, for efficiency.   
    //if (varPath.IsNotPath())
    //    return 0;
    GASSERT(varPath.IsNotPath() == false);
    if (varPath.GetHashFlags() & GASString::Flag_PathCheck) // IS a path!
        return 1;  

    // This string was not checked for being a path yet, so do the check.
    const char* pstr = varPath.ToCStr();
    if (strchr(pstr, ':') != NULL ||
        strchr(pstr, '/') != NULL ||
        strchr(pstr, '.') != NULL)
    {
        // This *IS* a path
        varPath.SetPathFlags(GASString::Flag_PathCheck|0);
        return 1;
    }
    else
    {
        varPath.SetPathFlags(GASString::Flag_PathCheck|GASString::Flag_IsNotPath);
        return 0;
    }
}

// Find the sprite/Movie represented by the given value.  The
// value might be a reference to the object itself, or a
// string giving a relative path name to the object.
GFxASCharacter* GASEnvironment::FindTargetByValue(const GASValue& val)
{
    if (val.GetType() == GASValue::OBJECT)
    {   
        // TBD: Can object be convertible to string path? Should it be?
    }
    if (val.GetType() == GASValue::CHARACTER)
    {
        return val.ToASCharacter(this);
    }
    else if (val.GetType() == GASValue::STRING)
    {
        return FindTarget(val.ToString(this));
    }
    else
    {
        LogScriptError("Error: Invalid movie clip path; neither string nor object\n");
    }
    return NULL;
}

// This method determines necessity of actions' termination. In certain cases
// the actions being executed might be terminated. This is happening when the
// environment owner is unloading or already unloaded. This unloading may 
// happen because of gotoFrame, where the target frame doesn't contain the
// environment owner sprite. Though, Flash demonstrates different behavior that
// depends on type of executing code and presence of on-unload handler:
// 1) if the executing code is frame actions or the on()-type event handler (on(press),
//    for example) then the actions are terminated if the owner sprite is either 
//    UNLOADING or UNLOADED. 
//    This means, that regardless to presence or absence of the onUnload event handler
//    the execution of frame actions and on()-events is terminated, once the owner is 
//    going to die. But this is not true for onClipEvent(unload) handlers: their
//    execution is not terminated.
// 2) If the executing code is a function or function style event handler (such as
//    OnEnterFrame) then execution is terminated only if the owner sprite doesn't 
//    have the onUnload handler. If it has the onUnload handler and is going to die
//    then execution continues.
// Note, Flash (and GFx) checks the currently set target for being unloaded. But if the
// target is changed by the "tellTarget" then the ActionScript is not terminated, even
// if the real target ("this") is unloaded already. The "this" character becomes
// completely wiped out: there are no name, no methods, no local variables exist for it.
// The only accessible things are _global and _root.
// ActionScript execution may be terminated later, if call methods on existing characters
// in _root. It is not terminated if we try to invoke methods on that wiped out "this".
bool GASEnvironment::NeedTermination(GASActionBuffer::ExecuteType execType) const
{
    GASSERT(Target); 
    if ((execType == GASActionBuffer::Exec_Unknown || execType == GASActionBuffer::Exec_Event) && 
        Target->IsUnloading())
        return true;
    else if (Target->IsUnloaded())
        return true;
    return false;
}

// Search for next '.' or '/' GFxCharacter in this word.  Return
// a pointer to it, or to NULL if it wasn't found.
static const char*  GAS_NextSlashOrDot(const char* pword)
{
    for (const char* p = pword; *p; p++)
    {
        if (*p == '.' && p[1] == '.')
        {
            p++;
        }
        else if (*p == '.' || *p == '/')
        {
            return p;
        }
    }

    return NULL;
}

class StringTokenizer 
{
    const char* Str;
    const char* const EndStr;
    const char* Delimiters;
    GASString   Token;

    bool IsDelimiter(int ch) const
    {
        return strchr(Delimiters, ch) != 0;
    }

    const char* SkipDelimiters()
    {
        while (Str < EndStr) {
            if (!IsDelimiter(*Str))
                break;
            Str++;
        }
        return Str;
    }

public:
    StringTokenizer(GASStringContext *psc, const char* str, const char* delim) :
        Str(str), EndStr(str + strlen(str)), Delimiters(delim), Token(psc->GetBuiltin(GASBuiltin_empty_))
    {
    }
    StringTokenizer(GASStringContext *psc, const char* str, UPInt strSize, const char* delim) :
        Str(str), EndStr(str + strSize), Delimiters(delim), Token(psc->GetBuiltin(GASBuiltin_empty_))
    {
    }
    const char* SetDelimiters(const char* delim)
    {
        Delimiters = delim;
        return delim;
    }
    bool NextToken(char* sep)
    {
        if (Str >= EndStr) return false;
        const char* pToken;

        pToken = Str;// = SkipDelimiters();

        while (Str < EndStr) {
            if (IsDelimiter(*Str))
                break;
            Str++;
        }
        *sep = *Str;
        
        if (pToken != Str && Str <= EndStr) 
        {
            UPInt lastTokenSize = (UPInt)(Str - pToken);
            Token = Token.GetManager()->CreateString(pToken, lastTokenSize);
        }
        else
        {
            Token = Token.GetManager()->CreateEmptyString();
        }

        ++Str; // skip delimiter
        return true;
    }

    const GASString& GetToken() const { return Token; }

    StringTokenizer& operator=(const StringTokenizer&) { return *this; }
};

static bool GAS_IsRelativePathToken(GASStringContext* psc, const GASString& pathComponent)
{
    // return (pathComponent == ".." || pathComponent == "_parent");
    return (psc->GetBuiltin(GASBuiltin_dotdot_) == pathComponent ||
            psc->GetBuiltin(GASBuiltin__parent).CompareBuiltIn_CaseCheck(pathComponent, psc->IsCaseSensitive()));
}

// Find the sprite/Movie referenced by the given path.
bool GASEnvironment::FindVariable(const GetVarParams& params, bool onlyTargets, GASString* varName) const
{
    if (params.VarName.IsEmpty())
    {
        if (params.pResult)
            params.pResult->SetAsCharacter(Target);
        return true;
    }

    GASValue                    current;  
    bool                        currentFound        = false;
    const char*                 p                   = params.VarName.ToCStr();
    UPInt                       plength             = params.VarName.GetSize();
    static const char*          onlySlashesDelim    = ":/";
    static const char* const    regularDelim        = ":./";
    const char*                 delim               = regularDelim;

    if (params.pOwner) // reset owner if necessary
        params.pOwner->SetUndefined();
    if (params.ppNewTarget) // reset last target
        *params.ppNewTarget = 0;

    GASSERT(Target);
    
    if (*p == '/') // if the first symbol is '/' - root level is the beginning
    {
        // Absolute path; start at the _root.
        current.SetAsCharacter(Target->GetASRootMovie());
        currentFound = true;
        p++;
        plength--;
        delim = onlySlashesDelim;
        if (params.pOwner)
            *params.pOwner = current;
    }
    else if (*p == '.') // if the first symbol is '.' use only slashes delim
    {
        delim = onlySlashesDelim;
    }

    StringTokenizer parser(GetSC(), p, plength, delim);
    char            sep = 0;

    bool first_token = true;
    while(parser.NextToken(&sep))
    {
        const GASString& token = parser.GetToken();
        
        if (token.GetSize())
        {
            if (varName)
            {
                *varName = token;
            }
            GASValue member;
            bool     memberFound = false;
            if (current.IsCharacter() || (!currentFound && GAS_IsRelativePathToken(GetSC(), token)))
            {
                if (!currentFound)
                {
                    // the special case, if relative path is used and current is not set yet:
                    // check withStack first, or use the Target as a current
                    if (params.pWithStack && params.pWithStack->GetSize() > 0)
                    {
                        GASObjectInterface* obj = (*params.pWithStack)[params.pWithStack->GetSize()-1].GetObjectInterface();
                        if (obj->IsASCharacter())
                            current.SetAsCharacter(obj->ToASCharacter());
                    }
                    if (current.IsUndefined())
                        current.SetAsCharacter(Target);
                    currentFound = true;
                }
                GFxASCharacter* m = current.ToASCharacter(this);
                if (m)
                {
                    m = m->GetRelativeTarget(token, first_token);
                    if (m)
                    {
                        member.SetAsCharacter(m);
                        memberFound = true;
                    }
                }
            }   
            if (!memberFound)
            {
                if (!currentFound)
                {
                    // if no current set yet - check local vars in environment
                    memberFound = GetVariableRaw(GetVarParams(token, &member, params.pWithStack));
                }
                else
                {
                    if (current.IsNumber() || current.IsBoolean() || current.IsString())
                    {
                        // create a temp object to be able to resolve builtin properties like "length".
                        // TODO: revise const_cast
                        current = const_cast<GASEnvironment*>(this)->PrimitiveToTempObject(current);
                    }
                    if (!current.IsObject() && !current.IsCharacter() && !current.IsFunction())
                    {
                        member.SetUndefined();
                        memberFound = false;
                    }
                    else
                    {
                        GASObjectInterface* iobj = current.ToObjectInterface(this);
                        if (iobj)
                        {
                            // TODO: revise const_cast
                            memberFound = iobj->GetMember(const_cast<GASEnvironment*>(this), token, &member);
                            if (!memberFound)
                            {
                                member.SetUndefined();
                                memberFound = false;
                            }
                        }
                    }
                }
            }
            if (params.pOwner) // save owner if necessary
                *params.pOwner = current;
            if ((onlyTargets && !member.IsCharacter()) || !memberFound)
            {
                current.SetUndefined();
                currentFound = false;
                if (parser.NextToken(&sep))
                {   // if this is not the last token - set owner and target to null
                    if (params.pOwner) params.pOwner->SetUndefined();
                    if (params.ppNewTarget) *params.ppNewTarget = 0;
                    if (varName) *varName = GetBuiltin(GASBuiltin_empty_);
                }
                break; // abort the loop
            }

            if (member.IsProperty())
            {
                // resolve the property to its value
                // TODO: revise const_cast
                member.GetPropertyValue(const_cast<GASEnvironment *>(this),
                    current.ToObjectInterface(this), &current);
            }
            else
            {
                current = member;
            }

            currentFound = memberFound;
        }
        if (delim == onlySlashesDelim)
        {
            if (sep == ':')
            {
                delim = parser.SetDelimiters(regularDelim);
                if (params.ppNewTarget && current.IsCharacter())
                {
                    *params.ppNewTarget = current.ToASCharacter(this);
                }
            }
        }
        else
        {
            if (sep == '.')
            {
                if (params.ppNewTarget && current.IsCharacter())
                {
                    *params.ppNewTarget = current.ToASCharacter(this);
                }
            }
        }

        if (sep == '/')
        {
            delim = parser.SetDelimiters(onlySlashesDelim);
        }
        first_token = false;
    }
    if (params.ppNewTarget && current.IsCharacter())
    {
        *params.ppNewTarget = current.ToASCharacter(this);
    }
    if (params.pOwner && (!params.pOwner->IsObject() && 
        !params.pOwner->IsCharacter() && !params.pOwner->IsFunction()))
    {
        // owner can only be an object/character/function object
        params.pOwner->SetUndefined();
    }
    if (!currentFound)
        return false;
    if (params.pResult)
        *params.pResult = current;
    return true;
}

// Find the sprite/Movie referenced by the given path.
// excludeFlags - 0 or NoLogOutput
//@TODO: should it be redesigned to work with FindVariable? (AB)
GFxASCharacter* GASEnvironment::FindTarget(const GASString& path, UInt excludeFlags) const
{
    if (path.IsEmpty())    
        return IsInvalidTarget ? NULL : Target;
    GASSERT(path.GetSize() > 0);

    GFxASCharacter* env = Target;
    GASSERT(env);
    
    const char* p = path.ToCStr();
    GASString   subpart(GetBuiltin(GASBuiltin_empty_));

    if (*p == '/')
    {
        // Absolute path; start at the _root.
        env = env->GetASRootMovie();
        p++;
    }
    bool first_call = true;
    for (;;)
    {
        const char* pnextSlash = GAS_NextSlashOrDot(p);    
        if (pnextSlash == p)
        {
            if (!(excludeFlags & NoLogOutput))
                LogError("Error: invalid path '%s'\n", path.ToCStr());
            break;
        }
        else if (pnextSlash)
        {
            // Cut off the slash and everything after it.
            subpart = CreateString(p, (UPInt)(pnextSlash - p));
        }
        else
        {
            subpart = CreateString(p);
        }

        if (subpart.GetSize())
        {
            // Handle: '.', '..', this, _root, _parent, _levelN, display list characters
            env = env->GetRelativeTarget(subpart, first_call);
        }       

        if (env == NULL || pnextSlash == NULL)
        {
            break;
        }

        p = pnextSlash + 1;
        first_call = false;
    }
    return env;
}

GASObject* GASEnvironment::OperatorNew(const GASFunctionRef& constructor, int nargs, int argsTopOff)
{
    GASValue    newObjVal;

    GASSERT (!constructor.IsNull ());

    if (argsTopOff < 0) argsTopOff = GetTopIndex();

    GPtr<GASObject> pnewObj;                    

    //!AB, special undocumented(?) case: if Object's ctor is invoked
    // with a parameter of type:
    // 1. number - it will create instance of Number;
    // 2. boolean - it will create instance of Boolean;
    // 3. array - it will create instance of Array;
    // 4. string - it will create instance of String;
    // 5. object - it will just return the same object.
    // "null" and "undefined" are ignored.
    if (nargs == 1 && constructor == GetConstructor(GASBuiltin_Object))
    {
        const GASValue& arg0 = Top();
        GASValue res;
        if (arg0.IsNumber() || arg0.IsBoolean() || arg0.IsString())
        {
            res = PrimitiveToTempObject(0);
        }
        else if (arg0.IsObject() || arg0.IsCharacter())
        {
            res = arg0;
        }
        if (!res.IsUndefined())
        {
            GASObject* pnewObj = res.ToObject(this);
            if (pnewObj)
                pnewObj->AddRef();
            return pnewObj;
        }
    }

    // get the prototype
    GASValue prototypeVal;
    if (!constructor->GetMemberRaw(GetSC(), GetBuiltin(GASBuiltin_prototype), &prototypeVal))
    {
        prototypeVal.SetAsObject(GetPrototype(GASBuiltin_Object));
    }

    GASObject* prototype = prototypeVal.ToObject(this);
    GASFunctionRef ctor(constructor);
    GASValue __ctor__Val;

    if (prototype && prototype->GetMemberRaw(GetSC(), GetBuiltin(GASBuiltin_constructor), &__ctor__Val))
    {
        if (__ctor__Val.IsFunction() && !__ctor__Val.ToFunction(this).IsNull())
            ctor = __ctor__Val.ToFunction(this);
    }

    pnewObj = *ctor->CreateNewObject(this);
    if (pnewObj)
    {
        // object is allocated by CreateNewObject - set __proto__ and __ctor__

        pnewObj->Set__proto__(GetSC(), prototypeVal.ToObject(this));
        pnewObj->Set__constructor__(GetSC(), constructor);
    }
    else
    {
        GASString thisClassName = GetGC()->FindClassName(this, constructor.GetObjectPtr());
        GASString baseClassName = GetGC()->FindClassName(this, ctor.GetObjectPtr());
        LogScriptError("Error: %s::CreateNewObject returned NULL during creation of %s class instance.\n", 
            baseClassName.ToCStr(), thisClassName.ToCStr());
        return NULL;
    }

    GASValue result;
    //GAS_Invoke(constructor, &result, pnewObj.GetPtr(), this, nargs, argsTopOff, NULL);
    constructor.Function->Invoke(GASFnCall(&result, pnewObj, this, nargs, argsTopOff), constructor.LocalFrame, NULL);

    if (!pnewObj)
    {
        pnewObj = result.ToObject(this);
        if (pnewObj)
        {
            GASFunctionRef ctor = pnewObj->Get_constructor(GetSC());
            if (ctor.IsNull() || ctor == constructor) // this is necessary for Object ctor that can return object of different type
                                                      // depending on parameter (see GASObjectProto::GlobalCtor) (!AB)
            {
                GPtr<GASObject> prototype = prototypeVal.ToObject(this);

                // if constructor returned an object - set __proto__ and __ctor__ for it
                pnewObj->Set__proto__(GetSC(), prototype);
                pnewObj->Set__constructor__(GetSC(), constructor);
            }
        }
    }

    if (pnewObj)
        pnewObj->AddRef();
    return pnewObj;
}

GASObject* GASEnvironment::OperatorNew(GASObject* ppackageObj, const GASString &className, int nargs, int argsTopOff)
{
    GASSERT(className != "");

    GASValue   ctor;

    if (ppackageObj->GetMember(this, className, &ctor))
    {
        if (ctor.IsFunction())
        {
            return OperatorNew(ctor.ToFunction(this), nargs, argsTopOff);
        }
    }
    return 0;
}

GASFunctionRef GASEnvironment::GetConstructor(GASBuiltinType className)
{    
    GASValue ctor;
    if (GetGC()->pGlobal->GetMemberRaw(GetSC(), GetBuiltin(className), &ctor) &&
        ctor.IsFunction())
    {
        return ctor.ToFunction(this);
    }
    return 0;
}

// Creates an appropriate temporary object for primitive type at env->Top(index)
// and return it as GASValue.
GASValue GASEnvironment::PrimitiveToTempObject(int index)
{    
    GASBuiltinType  ctorName;    

    const GASValue& top = Top(index);
    switch (top.GetType())
    {
    case GASValue::BOOLEAN:
        ctorName = GASBuiltin_Boolean;
        break;
    case GASValue::STRING:
        ctorName = GASBuiltin_String;
        break;
    default:
        if (top.IsNumber())
            ctorName = GASBuiltin_Number;        
        else
            return GASValue(); // undefined
    }
    
    GPtr<GASObject> obj = *OperatorNew(GetBuiltin(ctorName), 1, GetTopIndex() - index);
    return GASValue(obj);
}

GASValue GASEnvironment::PrimitiveToTempObject(const GASValue& v) 
{    
    GASBuiltinType  ctorName;    

    switch (v.GetType())
    {
    case GASValue::BOOLEAN:
        ctorName = GASBuiltin_Boolean;
        break;
    case GASValue::STRING:
        ctorName = GASBuiltin_String;
        break;
    default:
        if (v.IsNumber())
            ctorName = GASBuiltin_Number;        
        else
            return GASValue(); // undefined
    }

    Push(v);
    GPtr<GASObject> obj = *OperatorNew(GetBuiltin(ctorName), 1, GetTopIndex());
    Drop1();
    return GASValue(obj);
}

void GASEnvironment::Reset()
{
    Stack.Reset();
    UInt i;
    for (i = 0; i < sizeof(GlobalRegister)/sizeof(GlobalRegister[0]); ++i)
    {
        GlobalRegister[i].SetUndefined();
    }
    LocalRegister.Resize(0);
    IsInvalidTarget = Unrolling = false;

    CallStack.Reset(); // stack of calls
    LocalFrames.Resize(0);
    FuncCallNestingLevel = 0;
    TryBlocks.Resize(0);
    ThrowingValue.SetUndefined();
}

//
// GFxEventId
//

GASBuiltinType      GFxEventId::GetFunctionNameBuiltinType() const
{
    // Note: There are no function names for Event_KeyPress, Event_Initialize, and 
    // Event_Construct. We use "@keyPress@", "@initialize@", and "@construct@" strings.
    static GASBuiltinType  functionTypes[Event_COUNT] =
    {        
        GASBuiltin_INVALID,             // Event_Invalid "INVALID"
        GASBuiltin_onLoad,              // Event_Load
        GASBuiltin_onEnterFrame,        // Event_EnterFrame
        GASBuiltin_onUnload,            // Event_Unload
        GASBuiltin_onMouseMove,         // Event_MouseMove
        GASBuiltin_onMouseDown,         // Event_MouseDown
        GASBuiltin_onMouseUp,           // Event_MouseUp
        GASBuiltin_onKeyDown,           // Event_KeyDown
        GASBuiltin_onKeyUp,             // Event_KeyUp
        GASBuiltin_onData,              // Event_Data
        GASBuiltin_ainitializea_,       // Event_Initialize "@initialize@"
        GASBuiltin_onPress,             // Event_Press
        GASBuiltin_onRelease,           // Event_Release
        GASBuiltin_onReleaseOutside,    // Event_ReleaseOutside
        GASBuiltin_onRollOver,          // Event_RollOver
        GASBuiltin_onRollOut,           // Event_RollOut
        GASBuiltin_onDragOver,          // Event_DragOver
        GASBuiltin_onDragOut,           // Event_DragOut
        GASBuiltin_akeyPressa_,         // Event_KeyPress  "@keyPress@"
        GASBuiltin_onConstruct,        // Event_Construct

        GASBuiltin_onPressAux,
        GASBuiltin_onReleaseAux,
        GASBuiltin_onReleaseOutsideAux,
        GASBuiltin_onDragOverAux,
        GASBuiltin_onDragOutAux,

        // These are for the MoveClipLoader ActionScript only
        GASBuiltin_onLoadStart,         // Event_LoadStart
        GASBuiltin_onLoadError,         // Event_LoadError
        GASBuiltin_onLoadProgress,      // Event_LoadProgress
        GASBuiltin_onLoadInit,          // Event_LoadInit
        // These are for the XMLSocket ActionScript only
        GASBuiltin_onSockClose,         // Event_SockClose
        GASBuiltin_onSockConnect,       // Event_SockConnect
        GASBuiltin_onSockData,          // Event_SockData
        GASBuiltin_onSockXML,           // Event_SockXML
        // These are for the XML ActionScript only
        GASBuiltin_onXMLLoad,           // Event_XMLLoad
        GASBuiltin_onXMLData,           // Event_XMLData

        //!AB:? GFXSTATICSTRING("onTimer"),  // setInterval GFxTimer expired
    };  
    GASSERT(GetEventsCount() == 1);
    UInt idx;
    if (Id <= Event_LastCombined)
        idx = GBitsUtil::BitCount32(Id);
    else
        idx = (Id - Event_NextAfterCombined) + 20 + 5; // PPS: 5 for the Aux events
    GASSERT(idx > Event_Invalid && idx < Event_COUNT);
    //DBG:GASSERT(Id != Event_Press);

    if (idx > Event_Invalid && idx < Event_COUNT)
        return functionTypes[idx];
    return GASBuiltin_unknown_;
}

GASString   GFxEventId::GetFunctionName(GASStringContext *psc) const
{
    return psc->GetBuiltin(GetFunctionNameBuiltinType()); 
}

// converts keyCode/asciiCode from this event to the on(keyPress <>) format
int GFxEventId::ConvertToButtonKeyCode() const
{
    int kc = 0;

    // convert keycode/ascii to button's keycode
    switch (KeyCode) 
    {
        case GFxKey::Left:      kc = 1; break;
        case GFxKey::Right:     kc = 2; break;
        case GFxKey::Home:      kc = 3; break;
        case GFxKey::End:       kc = 4; break;
        case GFxKey::Insert:    kc = 5; break;
        case GFxKey::Delete:    kc = 6; break;
        case GFxKey::Backspace: kc = 8; break;
        case GFxKey::Return:    kc = 13; break;
        case GFxKey::Up:        kc = 14; break;
        case GFxKey::Down:      kc = 15; break;
        case GFxKey::PageUp:    kc = 16; break;
        case GFxKey::PageDown:  kc = 17; break;
        case GFxKey::Tab:       kc = 18; break;
        case GFxKey::Escape:    kc = 19; break;
        default:
            if (AsciiCode >= 32)
                kc = AsciiCode;
    }
    return kc;
}

// Scan for functions in local frame and convert them to internal ones.
void GASLocalFrame::ReleaseFramesForLocalFuncs()
{
#ifdef GFC_NO_GC
    for (GASStringHash<GASValue>::Iterator iter = Variables.Begin(); iter != Variables.End(); ++iter)
    {
        //GFxStringIHash<GASValue>::Node& var = *iter;
        GASValue& value = iter->Second;
        if (value.IsFunction ())
        {
            // check, is this function declared locally or not. Only if the function
            // was declared locally, then we need to "weak" it by SetInternal. 
            // Otherwise, it might be just a local pointer to non-local function.
            if (LocallyDeclaredFuncs.Get(value.V.FunctionValue.GetObjectPtr()) != 0)
                value.V.FunctionValue.SetInternal(true);
        }
    }
    SuperThis = 0;
#endif
}

#ifndef GFC_NO_GC
template <class Functor> 
void GASLocalFrame::ForEachChild_GC(Collector* prcc) const
{
    GASStringHash_GC<GASValue>::ConstIterator it = Variables.Begin();
    while(it != Variables.End())
    {   
        const GASValue& value = it->Second;
        value.template ForEachChild_GC<Functor>(prcc);
        ++it;
    }
    if (PrevFrame)
        Functor::Call(prcc, PrevFrame);
    Callee.template ForEachChild_GC<Functor>(prcc);
    Caller.template ForEachChild_GC<Functor>(prcc);
}
GFC_GC_PREGEN_FOR_EACH_CHILD(GASLocalFrame)

void GASLocalFrame::ExecuteForEachChild_GC(Collector* prcc, OperationGC operation) const
{
    GASRefCountBaseType::CallForEachChild<GASLocalFrame>(prcc, operation);
}

void GASLocalFrame::Finalize_GC()
{
    Variables.~GASStringHash_GC<GASValue>();
}
#endif //GFC_NO_GC

//
// Disassembler
//


#if !defined(GFC_NO_FXPLAYER_VERBOSE_ACTION) || !defined(GFC_NO_FXPLAYER_VERBOSE_PARSE_ACTION)

#if defined(GFC_NO_FXPLAYER_VERBOSE_ACTION)
// No disassembler in this version...
void    GFxDisasm::LogDisasm(const unsigned char*)
{
    Log("<disasm is disabled>\n");
}

#else // !defined(GFC_NO_FXPLAYER_VERBOSE_ACTION)

// Disassemble one instruction to the log.
void    GFxDisasm::LogDisasm(const unsigned char* InstructionData)
{
    enum ArgFormatType {
        ARG_NONE = 0,
        ARG_STR,
        ARG_HEX,    // default hex dump, in case the format is unknown or unsupported
        ARG_U8,
        ARG_U16,
        ARG_S16,
        ARG_PUSH_DATA,
        ARG_DECL_DICT,
        ARG_FUNCTION,
        ARG_FUNCTION2
    };
    class GASInstInfo
    {
    public:
        int actionId;
        const char* Instruction;

        ArgFormatType   ArgFormat;
    };

    static GASInstInfo  InstructionTable[] = {
        { 0x04, "next_frame", ARG_NONE },
        { 0x05, "prev_frame", ARG_NONE },
        { 0x06, "play", ARG_NONE },
        { 0x07, "stop", ARG_NONE },
        { 0x08, "toggle_qlty", ARG_NONE },
        { 0x09, "stop_sounds", ARG_NONE },
        { 0x0A, "add", ARG_NONE },
        { 0x0B, "sub", ARG_NONE },
        { 0x0C, "mul", ARG_NONE },
        { 0x0D, "div", ARG_NONE },
        { 0x0E, "equ", ARG_NONE },
        { 0x0F, "lt", ARG_NONE },
        { 0x10, "and", ARG_NONE },
        { 0x11, "or", ARG_NONE },
        { 0x12, "not", ARG_NONE },
        { 0x13, "str_eq", ARG_NONE },
        { 0x14, "str_len", ARG_NONE },
        { 0x15, "substr", ARG_NONE },
        { 0x17, "pop", ARG_NONE },
        { 0x18, "floor", ARG_NONE },
        { 0x1C, "get_var", ARG_NONE },
        { 0x1D, "set_var", ARG_NONE },
        { 0x20, "set_target2", ARG_NONE },
        { 0x21, "str_cat", ARG_NONE },
        { 0x22, "get_prop", ARG_NONE },
        { 0x23, "set_prop", ARG_NONE },
        { 0x24, "dup_sprite", ARG_NONE },
        { 0x25, "rem_sprite", ARG_NONE },
        { 0x26, "trace", ARG_NONE },
        { 0x27, "start_drag", ARG_NONE },
        { 0x28, "stop_drag", ARG_NONE },
        { 0x29, "str_lt", ARG_NONE },
        { 0x2A, "throw", ARG_NONE },
        { 0x2B, "cast_object", ARG_NONE },
        { 0x2C, "implements", ARG_NONE },
        { 0x30, "random", ARG_NONE },
        { 0x31, "mb_length", ARG_NONE },
        { 0x32, "ord", ARG_NONE },
        { 0x33, "chr", ARG_NONE },
        { 0x34, "get_timer", ARG_NONE },
        { 0x35, "substr_mb", ARG_NONE },
        { 0x36, "ord_mb", ARG_NONE },
        { 0x37, "chr_mb", ARG_NONE },
        { 0x3A, "delete", ARG_NONE },
        { 0x3B, "delete2", ARG_STR },
        { 0x3C, "set_local", ARG_NONE },
        { 0x3D, "call_func", ARG_NONE },
        { 0x3E, "return", ARG_NONE },
        { 0x3F, "mod", ARG_NONE },
        { 0x40, "new", ARG_NONE },
        { 0x41, "decl_local", ARG_NONE },
        { 0x42, "decl_array", ARG_NONE },
        { 0x43, "decl_obj", ARG_NONE },
        { 0x44, "type_of", ARG_NONE },
        { 0x45, "get_target", ARG_NONE },
        { 0x46, "enumerate", ARG_NONE },
        { 0x47, "add_t", ARG_NONE },
        { 0x48, "lt_t", ARG_NONE },
        { 0x49, "eq_t", ARG_NONE },
        { 0x4A, "number", ARG_NONE },
        { 0x4B, "string", ARG_NONE },
        { 0x4C, "dup", ARG_NONE },
        { 0x4D, "swap", ARG_NONE },
        { 0x4E, "get_member", ARG_NONE },
        { 0x4F, "set_member", ARG_NONE },
        { 0x50, "inc", ARG_NONE },
        { 0x51, "dec", ARG_NONE },
        { 0x52, "call_method", ARG_NONE },
        { 0x53, "new_method", ARG_NONE },
        { 0x54, "is_inst_of", ARG_NONE },
        { 0x55, "enum_object", ARG_NONE },
        { 0x60, "bit_and", ARG_NONE },
        { 0x61, "bit_or", ARG_NONE },
        { 0x62, "bit_xor", ARG_NONE },
        { 0x63, "shl", ARG_NONE },
        { 0x64, "asr", ARG_NONE },
        { 0x65, "lsr", ARG_NONE },
        { 0x66, "eq_strict", ARG_NONE },
        { 0x67, "gt_t", ARG_NONE },
        { 0x68, "gt_str", ARG_NONE },
        { 0x69, "extends", ARG_NONE },
        
        { 0x81, "goto_frame", ARG_U16 },
        { 0x83, "get_url", ARG_STR },
        { 0x87, "store_register", ARG_U8 },
        { 0x88, "decl_dict", ARG_DECL_DICT },
        { 0x8A, "wait_for_frame", ARG_HEX },
        { 0x8B, "set_target", ARG_STR },
        { 0x8C, "goto_frame_lbl", ARG_STR },
        { 0x8D, "wait_for_fr_exp", ARG_HEX },
        { 0x8E, "function2", ARG_FUNCTION2 },
        { 0x8F, "try", ARG_HEX },
        { 0x94, "with", ARG_U16 },
        { 0x96, "push_data", ARG_PUSH_DATA },
        { 0x99, "goto", ARG_S16 },
        { 0x9A, "get_url2", ARG_HEX },
        { 0x9B, "function", ARG_FUNCTION },
        { 0x9D, "branch_if_true", ARG_S16 },
        { 0x9E, "call_frame", ARG_HEX },
        { 0x9F, "goto_frame_exp", ARG_HEX },
        { 0x00, "<end>", ARG_NONE }
    };

    int actionId = InstructionData[0];
    GASInstInfo*    info = NULL;

    for (int i = 0; ; i++)
    {
        if (InstructionTable[i].actionId == actionId)
        {
            info = &InstructionTable[i];
        }

        if (InstructionTable[i].actionId == 0)
        {
            // Stop at the end of the table and give up.
            break;
        }
    }

    ArgFormatType   fmt = ARG_HEX;

    // Show instruction.
    if (info == NULL)
    {
        Log("<unknown>[0x%02X]", actionId);
    }
    else
    {
        Log("%-15s", info->Instruction);
        fmt = info->ArgFormat;
    }

    // Show instruction Argument(s).
    if (actionId & 0x80)
    {
        GASSERT(fmt != ARG_NONE);

        int length = InstructionData[1] | (InstructionData[2] << 8);

        // Log(" [%d]", length);

        if (fmt == ARG_HEX)
        {
            for (int i = 0; i < length; i++)
            {
                Log(" 0x%02X", InstructionData[3 + i]);
            }
            Log("\n");
        }
        else if (fmt == ARG_STR)
        {
            Log(" \"");
            for (int i = 0; i < length; i++)
            {
                Log("%c", InstructionData[3 + i]);
            }
            Log("\"\n");
        }
        else if (fmt == ARG_U8)
        {
            int val = InstructionData[3];
            Log(" %d\n", val);
        }
        else if (fmt == ARG_U16)
        {
            int val = InstructionData[3] | (InstructionData[4] << 8);
            Log(" %d\n", val);
        }
        else if (fmt == ARG_S16)
        {
            int val = InstructionData[3] | (InstructionData[4] << 8);
            if (val & 0x8000) val |= ~0x7FFF;   // sign-extend
            Log(" %d\n", val);
        }
        else if (fmt == ARG_PUSH_DATA)
        {
            Log("\n");
            int i = 0;
            while (i < length)
            {
                int type = InstructionData[3 + i];
                i++;
                Log("\t\t");    // indent
                if (type == 0)
                {
                    // string
                    Log("\"");
                    while (InstructionData[3 + i])
                    {
                        Log("%c", InstructionData[3 + i]);
                        i++;
                    }
                    i++;
                    Log("\"\n");
                }
                else if (type == 1)
                {
                    // Float (little-endian)
                    union {
                        Float   F;
                        UInt32  I;
                    } u;
                    //compiler_assert(sizeof(u) == sizeof(u.I));

                    memcpy(&u.I, InstructionData + 3 + i, 4);
                    u.I = GByteUtil::LEToSystem(u.I);
                    i += 4;

                    Log("(Float) %f\n", u.F);
                }
                else if (type == 2)
                {
                    Log("NULL\n");
                }
                else if (type == 3)
                {
                    Log("undef\n");
                }
                else if (type == 4)
                {
                    // contents of register
                    int reg = InstructionData[3 + i];
                    i++;
                    Log("reg[%d]\n", reg);
                }
                else if (type == 5)
                {
                    int boolVal = InstructionData[3 + i];
                    i++;
                    Log("bool(%d)\n", boolVal);
                }
                else if (type == 6)
                {
                    // double
                    // wacky format: 45670123
#ifdef GFC_NO_DOUBLE
                    union
                    {
                        Float   F;
                        UInt32  I;
                    } u;

                    // convert ieee754 64bit to 32bit for systems without proper double
                    SInt    sign = (InstructionData[3 + i + 3] & 0x80) >> 7;
                    SInt    expo = ((InstructionData[3 + i + 3] & 0x7f) << 4) + ((InstructionData[3 + i + 2] & 0xf0) >> 4);
                    SInt    mant = ((InstructionData[3 + i + 2] & 0x0f) << 19) + (InstructionData[3 + i + 1] << 11) +
                        (InstructionData[3 + i + 0] << 3) + ((InstructionData[3 + i + 7] & 0xf8) >> 5);

                    if (expo == 2047)
                        expo = 255;
                    else if (expo - 1023 > 127)
                    {
                        expo = 255;
                        mant = 0;
                    }
                    else if (expo - 1023 < -126)
                    {
                        expo = 0;
                        mant = 0;
                    }
                    else
                        expo = expo - 1023 + 127;

                    u.I = (sign << 31) + (expo << 23) + mant;
                    i += 8;

                    Log("(double) %f\n", u.F);
#else
                    union {
                        double  D;
                        UInt64  I;
                        struct {
                            UInt32  Lo;
                            UInt32  Hi;
                        } Sub;
                    } u;
                    GCOMPILER_ASSERT(sizeof(u) == sizeof(u.I));

                    memcpy(&u.Sub.Hi, InstructionData + 3 + i, 4);
                    memcpy(&u.Sub.Lo, InstructionData + 3 + i + 4, 4);
                    u.I = GByteUtil::LEToSystem(u.I);
                    i += 8;

                    Log("(double) %f\n", u.D);
#endif
                }
                else if (type == 7)
                {
                    // int32
                    SInt32  val = InstructionData[3 + i]
                        | (InstructionData[3 + i + 1] << 8)
                        | (InstructionData[3 + i + 2] << 16)
                        | (InstructionData[3 + i + 3] << 24);
                    i += 4;
                    Log("(int) %d\n", val);
                }
                else if (type == 8)
                {
                    int id = InstructionData[3 + i];
                    i++;
                    Log("DictLookup[%d]\n", id);
                }
                else if (type == 9)
                {
                    int id = InstructionData[3 + i] | (InstructionData[3 + i + 1] << 8);
                    i += 2;
                    Log("DictLookupLg[%d]\n", id);
                }
            }
        }
        else if (fmt == ARG_DECL_DICT)
        {
            int i = 0;
            int count = InstructionData[3 + i] | (InstructionData[3 + i + 1] << 8);
            i += 2;

            Log(" [%d]\n", count);

            // Print strings.
            for (int ct = 0; ct < count; ct++)
            {
                Log("\t\t");    // indent

                Log("\"");
                while (InstructionData[3 + i])
                {
                    // safety check.
                    if (i >= length)
                    {
                        Log("<disasm error -- length exceeded>\n");
                        break;
                    }

                    Log("%c", InstructionData[3 + i]);
                    i++;
                }
                Log("\"\n");
                i++;
            }
        }
        else if (fmt == ARG_FUNCTION2)
        {
            // Signature info for a function2 opcode.
            int i = 0;
            const char* functionName = (const char*) &InstructionData[3 + i];
            i += (int)strlen(functionName) + 1;

            int argCount = InstructionData[3 + i] | (InstructionData[3 + i + 1] << 8);
            i += 2;

            int regCount = InstructionData[3 + i];
            i++;

            Log("\n\t\tname = '%s', ArgCount = %d, RegCount = %d\n",
                functionName, argCount, regCount);

            UInt16  flags = (InstructionData[3 + i]) | (InstructionData[3 + i + 1] << 8);
            i += 2;

            // @@ What is the difference between "super" and "Parent"?

            bool    preloadGlobal = (flags & 0x100) != 0;
            bool    preloadParent = (flags & 0x80) != 0;
            bool    preloadRoot   = (flags & 0x40) != 0;
            bool    suppressSuper = (flags & 0x20) != 0;
            bool    preloadSuper  = (flags & 0x10) != 0;
            bool    suppressArgs  = (flags & 0x08) != 0;
            bool    preloadArgs   = (flags & 0x04) != 0;
            bool    suppressThis  = (flags & 0x02) != 0;
            bool    preloadThis   = (flags & 0x01) != 0;

            Log("\t\t        pg = %d\n"
                "\t\t        pp = %d\n"
                "\t\t        pr = %d\n"
                "\t\tss = %d, ps = %d\n"
                "\t\tsa = %d, pa = %d\n"
                "\t\tst = %d, pt = %d\n",
                int(preloadGlobal),
                int(preloadParent),
                int(preloadRoot),
                int(suppressSuper),
                int(preloadSuper),
                int(suppressArgs),
                int(preloadArgs),
                int(suppressThis),
                int(preloadThis));

            for (int argi = 0; argi < argCount; argi++)
            {
                int argRegister = InstructionData[3 + i];
                i++;
                const char* argName = (const char*) &InstructionData[3 + i];
                i += (int)strlen(argName) + 1;

                Log("\t\targ[%d] - reg[%d] - '%s'\n", argi, argRegister, argName);
            }

            int functionLength = InstructionData[3 + i] | (InstructionData[3 + i + 1] << 8);
            i += 2;

            Log("\t\tfunction length = %d\n", functionLength);
        }
        else if (fmt == ARG_FUNCTION)
        {
            // Signature info for a function opcode.
            int i = 0;
            const char* functionName = (const char*) &InstructionData[3 + i];
            i += (int)strlen(functionName) + 1;

            int argCount = InstructionData[3 + i] | (InstructionData[3 + i + 1] << 8);
            i += 2;

            Log("\n\t\tname = '%s', ArgCount = %d\n",
                functionName, argCount);

            for (int argi = 0; argi < argCount; argi++)
            {
                const char* argName = (const char*) &InstructionData[3 + i];
                i += (int)strlen(argName) + 1;

                Log("\t\targ[%d] - '%s'\n", argi, argName);
            }

            int functionLength = InstructionData[3 + i] | (InstructionData[3 + i + 1] << 8);
            i += 2;

            Log("\t\tfunction length = %d\n", functionLength);
        }

    }
    else
    {
        Log("\n");
    }
}
#endif // !defined(GFC_NO_FXPLAYER_VERBOSE_ACTION)
#endif // #if !defined(GFC_NO_FXPLAYER_VERBOSE_ACTION) && !defined(GFC_NO_FXPLAYER_VERBOSE_PARSE_ACTION)

void GASStringContext::InvalidateOptAdvanceList()
{
    if (pContext)
    {
        pContext->GetMovieRoot()->InvalidateOptAdvanceList();
    }
}


#ifndef GFC_NO_GC
//////////////////////////////////////////////////////////////////////////
// GASRefCountCollection
//
#ifdef GFC_BUILD_DEBUG
//#define GFC_TRACE_COLLECTIONS
#endif

GASRefCountCollector::GASRefCountCollector()
{
    FrameCnt            = 0;
    PeakRootCount       = 0;
    LastRootCount       = 0;
    LastCollectedRoots  = 0;
    LastPeakRootCount   = 0;
    TotalFramesCount    = 0;
    LastCollectionFrameNum = 0;

    SetParams(~0u, ~0u);
}

void GASRefCountCollector::SetParams(UInt frameBetweenCollections, UInt maxRootCount)
{
    // max num of roots before collection
    if (frameBetweenCollections != ~0u)
        MaxFramesBetweenCollections = frameBetweenCollections;
    else
        MaxFramesBetweenCollections = 0; // off by default

    // force collection for every N-frames
    if (maxRootCount != ~0u)
        PresetMaxRootCount  = MaxRootCount = maxRootCount;
    else
        PresetMaxRootCount  = MaxRootCount = 1000;
}

void GASRefCountCollector::AdvanceFrame(UInt* movieFrameCnt, UInt* movieLastCollectFrame)
{
    // Is this the first advance since a collection by a different MovieView?
    if (*movieLastCollectFrame != LastCollectionFrameNum)
    {
        *movieLastCollectFrame = LastCollectionFrameNum;
        *movieFrameCnt = 1;
        return;
    }

    // Make sure we don't collect multiple times per frame
    // in the case where several views share the same GC
    if (*movieFrameCnt < FrameCnt)
    {
        // the calling MovieView is advancing the GC during a frame where
        // the GC has already been advanced more times by different MovieView
        ++(*movieFrameCnt);
        return;
    }

    UInt curRootCount = (UInt)GetRootsCount();
    // Do collection only every 10th frame for now
    ++TotalFramesCount;
    ++FrameCnt;
    PeakRootCount = G_Max(PeakRootCount, curRootCount);

    // Collection occurs if:
    // 1) if number of root exceeds currently set MaxRootCount;
    // 2) if MaxFramesBetweenCollections is set to value higher than 0 and the
    //    frame counter (FrameCnt) exceeds this value, and number of roots
    //    exceeds PresetMaxRootCount.
    if ((PresetMaxRootCount != 0 && curRootCount > MaxRootCount) || 
        (MaxFramesBetweenCollections != 0 && 
            FrameCnt >= MaxFramesBetweenCollections && 
            curRootCount > PresetMaxRootCount))
    {
        GASRefCountCollector::Stats stats;
        Collect(&stats);

#ifdef GFC_TRACE_COLLECTIONS        
        printf("Collect! Roots %d, MaxRoots %d, Peak %d, Roots collected %d, frames between %d ", 
            curRootCount, MaxRootCount, PeakRootCount, stats.RootsFreedTotal,
            TotalFramesCount - LastCollectionFrameNum);
#endif

        // If number of roots exceeds the preset max root count then we need to reset the PeakRootCount
        // in order to decrease currently set MaxRootCount.
        if (stats.RootsFreedTotal > PresetMaxRootCount)
        {
            PeakRootCount = curRootCount; // reset peak count
            MaxRootCount = PresetMaxRootCount;
        }

        // MaxRootCount has been updated every collection event
        //MaxRootCount = G_Max(PresetMaxRootCount, PeakRootCount - stats.RootsFreedTotal);
        MaxRootCount = G_Max(MaxRootCount, curRootCount - stats.RootsFreedTotal);

        if (PeakRootCount < (UInt)(MaxRootCount * 0.7))
            MaxRootCount = (UInt)(MaxRootCount * 0.7);

#ifdef GFC_TRACE_COLLECTIONS        
        GASSERT((int)MaxRootCount >= 0);
        printf("new maxroots %d\n", MaxRootCount);
#endif

        LastCollectionFrameNum = TotalFramesCount;
        
        FrameCnt          = 0;
        LastPeakRootCount = PeakRootCount;
        LastCollectedRoots= stats.RootsFreedTotal;
    }
    LastRootCount = curRootCount;
    *movieFrameCnt = FrameCnt;
    *movieLastCollectFrame = LastCollectionFrameNum;
}

void GASRefCountCollector::ForceCollect()
{
    UInt curRootCount = (UInt)GetRootsCount();

    GASRefCountCollector::Stats stats;
    Collect(&stats);

#ifdef GFC_TRACE_COLLECTIONS        
    printf("Forced collect! Roots %d, MaxRoots %d, Peak %d, Roots collected %d\n", 
        curRootCount, MaxRootCount, PeakRootCount, stats.RootsFreedTotal);
#endif
    FrameCnt  = 0;
    PeakRootCount = G_Max(PeakRootCount, curRootCount);
    LastRootCount = curRootCount;
}

void GASRefCountCollector::ForceEmergencyCollect()
{
    ForceCollect();

    // DO NOT shrink roots, if this was called while in adding roots in Release
    if (!IsAddingRoot())
        ShrinkRoots();
    
    // Reset peak and max root counters.
    PeakRootCount = 0;
    MaxRootCount  = PresetMaxRootCount;
}

#endif // GFC_NO_GC
