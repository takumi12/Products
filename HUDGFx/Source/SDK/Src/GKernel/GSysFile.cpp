/**********************************************************************

Filename    :   GFile_sys.cpp
Content     :   GFile wrapper class implementation (Win32)

Created     :   April 5, 1999
Authors     :   Michael Antonov

History     :   8/27/2001 MA    Reworked file and directory interface

Copyright   :   (c) 1999-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#define  GFILE_CXX

// Standard C library
#include <stdio.h>

// GKernel
#include "GSysFile.h"
#include "GHeapNew.h"

// ***** GFile interface


// ***** GUnopenedFile

// This is - a dummy file that fails on all calls.

class GUnopenedFile : public GFile
{
public:
    GUnopenedFile()  { }
    ~GUnopenedFile() { }

    virtual const char* GetFilePath()               { return 0; }

    // ** File Information
    virtual bool        IsValid()                   { return 0; }
    virtual bool        IsWritable()                { return 0; }
    //virtual bool      IsRecoverable()             { return 0; }
    // Return position / file size
    virtual SInt        Tell()                      { return 0; }
    virtual SInt64      LTell()                     { return 0; }
    virtual SInt        GetLength()                 { return 0; }
    virtual SInt64      LGetLength()                { return 0; }

//  virtual bool        Stat(GFileStats *pfs)       { return 0; }
    virtual SInt        GetErrorCode()              { return Error_FileNotFound; }

    // ** GFxStream implementation & I/O
    virtual SInt        Write(const UByte *pbuffer, SInt numBytes)      { return -1; GUNUSED2(pbuffer, numBytes); }
    virtual SInt        Read(UByte *pbuffer, SInt numBytes)             { return -1; GUNUSED2(pbuffer, numBytes); }
    virtual SInt        SkipBytes(SInt numBytes)                        { return 0;  GUNUSED(numBytes); }
    virtual SInt        BytesAvailable()                                { return 0; }
    virtual bool        Flush()                                         { return 0; }
    virtual SInt        Seek(SInt offset, SInt origin)                  { return -1; GUNUSED2(offset, origin); }
    virtual SInt64      LSeek(SInt64 offset, SInt origin)               { return -1; GUNUSED2(offset, origin); }

    virtual bool        ChangeSize(SInt newSize)                        { return 0;  GUNUSED(newSize); }
    virtual SInt        CopyFromStream(GFile *pstream, SInt byteSize)   { return -1; GUNUSED2(pstream, byteSize); }
    virtual bool        Close()                                         { return 0; }
    virtual bool        CloseCancel()                                   { return 0; }
};



// ***** System File

// System file is created to access objects on file system directly
// This file can refer directly to path

// ** Constructor
GSysFile::GSysFile() : GDelegatedFile(0)
{
    pFile = *new GUnopenedFile;
}

#ifdef GFC_OS_WII
GFile* GFileWiiDvdOpen(const char *ppath, SInt flags=GFile::Open_Read);
#elif defined(GFC_OS_PS2)
GFile* GFilePS2Open(const char *ppath, SInt flags=GFile::Open_Read);
#elif defined(GFC_OS_PSP)
GFile* GFilePSPOpen(const char *ppath, SInt flags=GFile::Open_Read);
#else
GFile* GFileFILEOpen(const GString& path, SInt flags, SInt mode);
#endif

// Opens a file
GSysFile::GSysFile(const GString& path, SInt flags, SInt mode) : GDelegatedFile(0)
{
    Open(path, flags, mode);
}


// ** Open & management
// Will fail if file's already open
bool    GSysFile::Open(const GString& path, SInt flags, SInt mode)
{
#ifdef GFC_OS_WII
    pFile = *GFileWiiDvdOpen(path.ToCStr());
#elif defined(GFC_OS_PS2)
    pFile = *GFilePS2Open(path.ToCStr());
#elif defined(GFC_OS_PSP)
    pFile = *GFilePSPOpen(path.ToCStr());
#else
    pFile = *GFileFILEOpen(path, flags, mode);
#endif
    if ((!pFile) || (!pFile->IsValid()))
    {
        pFile = *GNEW GUnopenedFile;
        return 0;
    }
#if defined(GFC_OS_WII) || defined(GFC_OS_PS2) || defined(GFC_OS_PSP)
    pFile = *GNEW GBufferedFile(pFile);
#else    
    //pFile = *GNEW GDelegatedFile(pFile); // MA Testing
    if (flags & Open_Buffered)
        pFile = *GNEW GBufferedFile(pFile);
#endif
    return 1;
}


// ** Overrides

SInt    GSysFile::GetErrorCode()
{
    return pFile ? pFile->GetErrorCode() : Error_FileNotFound;
}


// Overrides to provide re-open support
bool    GSysFile::IsValid()
{
    return pFile && pFile->IsValid();
}
bool    GSysFile::Close()
{
    if (IsValid())
    {
        GDelegatedFile::Close();
        pFile = *GNEW GUnopenedFile;
        return 1;
    }
    return 0;
}

/*
bool    GSysFile::CloseCancel()
{
    if (IsValid())
    {
        GBufferedFile::CloseCancel();
        pFile = *GNEW GUnopenedFile;
        return 1;
    }
    return 0;
}
*/
