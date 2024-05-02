/**********************************************************************

Filename    :   GDebug.h
Content     :   General purpose debugging support
Created     :   July 18, 2001
Authors     :   Brendan Iribe, Michael Antonov

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   GFC Debug warning functionality is enabled
                if and only if GFC_BUILD_DEBUG is defined.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GDebug.h"

#include <stdio.h>
#include <string.h>
#include "GStd.h"

#if defined(GFC_OS_WIN32)
#include <windows.h>
#elif defined(GFC_OS_XBOX360)
#include <xtl.h>
#elif defined(GFC_OS_WII)
#include <revolution/os.h>
#elif defined(GFC_OS_ANDROID)
#include <android/log.h>
#endif


// ***** GDebug class implementation

#define GFC_DEBUG_BUFFER_SIZE   2048

// Pointer to handler function, if null default message output is used
static GDebug::MessageHandlerType GDebug_pMessageHandler = 0;
    
void    GDebug::SetMessageHandler(GDebug::MessageHandlerType phandler)
{
    GDebug_pMessageHandler = phandler;
}

void GDebug::Message(MessageType msgType, const char *pformat, ...)
{
    va_list  argList;
    va_start(argList, pformat);

    if (GDebug_pMessageHandler)
        GDebug_pMessageHandler(msgType, pformat, argList);
    else
    {
        // Generate a format string
        char    sourceBuff[GFC_DEBUG_BUFFER_SIZE];
        char    formatBuff[GFC_DEBUG_BUFFER_SIZE];

   
        G_vsprintf(formatBuff, GFC_DEBUG_BUFFER_SIZE, pformat, argList);
        switch(msgType)
        {
            case Message_Error:     G_strcpy(sourceBuff, GFC_DEBUG_BUFFER_SIZE, "GFC Error: ");     break;
            case Message_Warning:   G_strcpy(sourceBuff, GFC_DEBUG_BUFFER_SIZE, "GFC Warning: ");   break;
            case Message_Assert:    G_strcpy(sourceBuff, GFC_DEBUG_BUFFER_SIZE, "GFC Assert: ");    break;
            case Message_Note:      sourceBuff[0] = 0;
        }
        G_strcat(sourceBuff, GFC_DEBUG_BUFFER_SIZE, formatBuff);
        G_strcat(sourceBuff, GFC_DEBUG_BUFFER_SIZE, "\n");

//fputs(sourceBuff, stdout); // DBG

#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360)
        ::OutputDebugStringA(sourceBuff);
#elif defined(GFC_OS_WII)
        OSReport(sourceBuff);
#elif defined(GFC_OS_ANDROID)
        __android_log_write(ANDROID_LOG_INFO,"GFx",sourceBuff);
#else
        fputs(sourceBuff, stdout);
#endif

    }
    va_end(argList);
}

