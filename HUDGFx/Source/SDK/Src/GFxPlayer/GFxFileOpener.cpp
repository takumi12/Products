/**********************************************************************

Filename    :   GFxFileOpener.cpp
Content     :   
Created     :   April 15, 2008
Authors     :   

Notes       :   Redesigned to use inherited state objects for GFx 2.2.

Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxLoader.h"
#include "GSysFile.h"
#include "GFxLog.h"


// ***** GFxFileOpener implementation

// Default implementation - use GSysFile.
GFile* GFxFileOpener::OpenFile(const char *purl, SInt flags, SInt modes)
{
    // Buffered file wrapper is faster to use because it optimizes seeks.
    return new GSysFile(purl, flags, modes);
}

SInt64  GFxFileOpener::GetFileModifyTime(const char *purl)
{
    GFileStat fileStat;
    if (GSysFile::GetFileStat(&fileStat, purl))
        return fileStat.ModifyTime; 
    return -1;
}


// Implementation that allows us to override the log.
GFile*      GFxFileOpener::OpenFileEx(const char *pfilename, GFxLog *plog, 
                                      SInt flags, SInt modes)
{   
    GFile*  pin = OpenFile(pfilename, flags, modes);
    SInt errCode = 16;
    if (pin)
        errCode = pin->GetErrorCode();

    if (!pin || errCode)
    {
        if (plog)
            plog->LogError("Error: GFxLoader failed to open '%s'\n", pfilename);
        if (pin)
        {
            //plog->LogError("Error: Code '%d'\n", errCode);
            pin->Release();
        }
        return 0;
    }
    return pin;
}

