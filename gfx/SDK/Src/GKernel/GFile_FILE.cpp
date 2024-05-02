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

#include "GTypes.h"

// Standard C library
#include <stdio.h>
#ifndef GFC_OS_WINCE
#include <sys/stat.h>
#endif

#include "GSysFile.h"

#ifndef GFC_OS_WINCE
#include <errno.h>
#endif

// unistd provides stat() function
#ifdef GFC_OS_PS3
#include <unistd.h>
#endif

#include "GHeapNew.h"



// ***** GFile interface

// ***** GFILEFile - C streams file

static int GFC_error ()
{
#if !defined(GFC_OS_WINCE) && !defined(GFC_CC_RENESAS)
    if (errno == ENOENT)
        return GFileConstants::Error_FileNotFound;
    else if (errno == EACCES || errno == EPERM)
        return GFileConstants::Error_Access;
    else if (errno == ENOSPC)
        return GFileConstants::Error_DiskFull;
    else
#endif
        return GFileConstants::Error_IOError;
};

#ifdef GFC_OS_WIN32
#include "windows.h"
// A simple helper class to disable/enable system error mode, if necessary
// Disabling happens conditionally only if a drive name is involved
class SysErrorModeDisabler
{
    BOOL    Disabled;
    UINT    OldMode;
public:
    SysErrorModeDisabler(const char* pfileName)
    {
        if (pfileName && (pfileName[0]!=0) && pfileName[1]==':')
        {
            Disabled = 1;
            OldMode = ::SetErrorMode(SEM_FAILCRITICALERRORS);
        }
        else
            Disabled = 0;
    }

    ~SysErrorModeDisabler()
    {
        if (Disabled) ::SetErrorMode(OldMode);
    }
};
#else
class SysErrorModeDisabler
{
public:
    SysErrorModeDisabler(const char* pfileName) { }
};
#endif // GFC_OS_WIN32


// This macro enables verification of I/O results after seeks against a pre-loaded
// full file buffer copy. This is generally not necessary, but can been used to debug
// memory corruptions; we've seen this fail due to EAX2/DirectSound corrupting memory
// under FMOD with XP64 (32-bit) and Realtek HA Audio driver.
//#define GFILE_VERIFY_SEEK_ERRORS


// This is the simplest possible file implementation, it wraps around the descriptor
// This file is delegated to by GSysFile.

class GFILEFile : public GFile
{
protected:

    // Allocated filename
    GString     FileName;

    // File handle & open mode
    bool        Opened;
    FILE*       fs;
    SInt        OpenFlags;
    // Error code for last request
    SInt        ErrorCode;

    SInt        LastOp;

#ifdef GFILE_VERIFY_SEEK_ERRORS
    UByte*      pFileTestBuffer;
    UInt        FileTestLength;
    UInt        TestPos; // File pointer position during tests.
#endif

public:

    GFILEFile()
    {
        Opened = 0; FileName = "";

#ifdef GFILE_VERIFY_SEEK_ERRORS
        pFileTestBuffer =0;
        FileTestLength  =0;
        TestPos         =0;
#endif
    }
    // Initialize file by opening it
    GFILEFile(const GString& fileName, SInt flags, SInt Mode);
    // The 'pfileName' should be encoded as UTF-8 to support international file names.
    GFILEFile(const char* pfileName, SInt flags, SInt Mode);

    ~GFILEFile()
    {
        if (Opened)
            Close();
    }

    virtual const char* GetFilePath();

    // ** File Information
    virtual bool        IsValid();
    virtual bool        IsWritable();
    //virtual bool      IsRecoverable();
    // Return position / file size
    virtual SInt        Tell();
    virtual SInt64      LTell();
    virtual SInt        GetLength();
    virtual SInt64      LGetLength();

//  virtual bool        Stat(GFileStats *pfs);
    virtual SInt        GetErrorCode();

    // ** GFxStream implementation & I/O
    virtual SInt        Write(const UByte *pbuffer, SInt numBytes);
    virtual SInt        Read(UByte *pbuffer, SInt numBytes);
    virtual SInt        SkipBytes(SInt numBytes);
    virtual SInt        BytesAvailable();
    virtual bool        Flush();
    virtual SInt        Seek(SInt offset, SInt origin);
    virtual SInt64      LSeek(SInt64 offset, SInt origin);

    virtual bool        ChangeSize(SInt newSize);
    virtual SInt        CopyFromStream(GFile *pStream, SInt byteSize);
    virtual bool        Close();
    //virtual bool      CloseCancel();
private:
    void                init();
};


// Initialize file by opening it
GFILEFile::GFILEFile(const GString& fileName, SInt flags, SInt mode)
  : FileName(fileName), OpenFlags(flags)
{
    GUNUSED(mode);
    init();
}

// The 'pfileName' should be encoded as UTF-8 to support international file names.
GFILEFile::GFILEFile(const char* pfileName, SInt flags, SInt mode)
  : FileName(pfileName), OpenFlags(flags)
{
    GUNUSED(mode);
    init();
}

void GFILEFile::init()
{
    // Open mode for file's open
    const char *omode = "rb";

    if (OpenFlags & Open_Truncate)
    {
        if (OpenFlags & Open_Read)
            omode = "w+b";
        else
            omode = "wb";
    }
    else if (OpenFlags & Open_Create)
    {
        if (OpenFlags & Open_Read)
            omode = "a+b";
        else
            omode = "ab";
    }
    else if (OpenFlags & Open_Write)
        omode = "r+b";

#ifdef GFC_OS_WIN32
    SysErrorModeDisabler disabler(FileName.ToCStr());
#endif

#if defined(GFC_CC_MSVC) && (GFC_CC_MSVC >= 1400) && !defined(GFC_OS_WINCE)
    wchar_t womode[16];
    wchar_t *pwFileName = (wchar_t*)GALLOC((GUTF8Util::GetLength(FileName.ToCStr())+1) * sizeof(wchar_t), GStat_Default_Mem);
    GUTF8Util::DecodeString(pwFileName, FileName.ToCStr());
    GASSERT(strlen(omode) < sizeof(womode)/sizeof(womode[0]));
    GUTF8Util::DecodeString(womode, omode);
    _wfopen_s(&fs, pwFileName, womode);
    GFREE(pwFileName);
#else
    fs = fopen(FileName.ToCStr(), omode);
#endif
#ifdef GFC_OS_WINCE
    if (fs)
        fseek(fs, 0, SEEK_SET);
#else
    if (fs)
        rewind (fs);
#endif
    Opened = (fs != NULL);
    // Set error code
    if (!Opened)
        ErrorCode = GFC_error();
    else
    {
        // If we are testing file seek correctness, pre-load the entire file so
        // that we can do comparison tests later.
#ifdef GFILE_VERIFY_SEEK_ERRORS        
        TestPos         = 0;
        fseek(fs, 0, SEEK_END);
        FileTestLength  = ftell(fs);
        fseek(fs, 0, SEEK_SET);
        pFileTestBuffer = (UByte*)GHEAP_AUTO_ALLOC(this, FileTestLength);        
        if (pFileTestBuffer)
        {
            GASSERT(FileTestLength == (UInt)Read(pFileTestBuffer, FileTestLength));
            Seek(0, Seek_Set);
        }        
#endif

        ErrorCode = 0;
    }
    LastOp = 0;
}


const char* GFILEFile::GetFilePath()
{
    return FileName.ToCStr();
}


// ** File Information
bool    GFILEFile::IsValid()
{
    return Opened;
}
bool    GFILEFile::IsWritable()
{
    return IsValid() && (OpenFlags&Open_Write);
}
/*
bool    GFILEFile::IsRecoverable()
{
    return IsValid() && ((OpenFlags&GFC_FO_SAFETRUNC) == GFC_FO_SAFETRUNC);
}
*/

// Return position / file size
SInt    GFILEFile::Tell()
{
    int pos = ftell (fs);
    if (pos < 0)
        ErrorCode = GFC_error();
    return pos;
}

SInt64  GFILEFile::LTell()
{
    int pos = ftell(fs);
    if (pos < 0)
        ErrorCode = GFC_error();
    return pos;
}

SInt    GFILEFile::GetLength()
{
    SInt pos = Tell();
    if (pos >= 0)
    {
        Seek (0, Seek_End);
        SInt size = Tell();
        Seek (pos, Seek_Set);
        return size;
    }
    return -1;
}
SInt64  GFILEFile::LGetLength()
{
    return GetLength();
}

SInt    GFILEFile::GetErrorCode()
{
    return ErrorCode;
}

// ** GFxStream implementation & I/O
SInt    GFILEFile::Write(const UByte *pbuffer, SInt numBytes)
{
    if (LastOp && LastOp != Open_Write)
        fflush(fs);
    LastOp = Open_Write;
    int written = (int) fwrite(pbuffer, 1, numBytes, fs);
    if (written < numBytes)
        ErrorCode = GFC_error();

#ifdef GFILE_VERIFY_SEEK_ERRORS
    if (written > 0)
        TestPos += written;
#endif

    return written;
}

SInt    GFILEFile::Read(UByte *pbuffer, SInt numBytes)
{
    if (LastOp && LastOp != Open_Read)
        fflush(fs);
    LastOp = Open_Read;
    int read = (int) fread(pbuffer, 1, numBytes, fs);
    if (read < numBytes)
        ErrorCode = GFC_error();

#ifdef GFILE_VERIFY_SEEK_ERRORS
    if (read > 0)
    {
        // Read-in data must match our pre-loaded buffer data!
        UByte* pcompareBuffer = pFileTestBuffer + TestPos;
        for (int i=0; i< read; i++)
        {
            GASSERT(pcompareBuffer[i] == pbuffer[i]);
        }

        //GASSERT(!memcmp(pFileTestBuffer + TestPos, pbuffer, read));
        TestPos += read;
        GASSERT(ftell(fs) == (int)TestPos);
    }
#endif

    return read;
}

// Seeks ahead to skip bytes
SInt    GFILEFile::SkipBytes(SInt numBytes)
{
    SInt64 pos    = LTell();
    SInt64 newPos = LSeek(numBytes, Seek_Cur);

    // Return -1 for major error
    if ((pos==-1) || (newPos==-1))
    {
        return -1;
    }
    //ErrorCode = ((NewPos-Pos)<numBytes) ? errno : 0;

    return SInt(newPos-(SInt)pos);
}

// Return # of bytes till EOF
SInt    GFILEFile::BytesAvailable()
{
    SInt64 pos    = LTell();
    SInt64 endPos = LGetLength();

    // Return -1 for major error
    if ((pos==-1) || (endPos==-1))
    {
        ErrorCode = GFC_error();
        return 0;
    }
    else
        ErrorCode = 0;

    return SInt(endPos-(SInt)pos);
}

// Flush file contents
bool    GFILEFile::Flush()
{
    return !fflush(fs);
}

SInt    GFILEFile::Seek(SInt offset, SInt origin)
{
    int newOrigin = 0;
    switch(origin)
    {
    case Seek_Set: newOrigin = SEEK_SET; break;
    case Seek_Cur: newOrigin = SEEK_CUR; break;
    case Seek_End: newOrigin = SEEK_END; break;
    }

    if (newOrigin == SEEK_SET && offset == Tell())
        return Tell();

    if (fseek (fs, offset, newOrigin))
    {
#ifdef GFILE_VERIFY_SEEK_ERRORS
        GASSERT(0);
#endif
        return -1;
    }
    
#ifdef GFILE_VERIFY_SEEK_ERRORS
    // Track file position after seeks for read verification later.
    switch(origin)
    {
    case Seek_Set:  TestPos = offset;       break;
    case Seek_Cur:  TestPos += offset;      break;    
    case Seek_End:  TestPos = FileTestLength + offset; break;
    }
    GASSERT((SInt)TestPos == Tell());
#endif

    return (SInt)Tell();
}

SInt64  GFILEFile::LSeek(SInt64 offset, SInt origin)
{
    return Seek((SInt)offset,origin);
}

bool    GFILEFile::ChangeSize(SInt newSize)
{
    GUNUSED(newSize);
    GFC_DEBUG_WARNING(1, "GFile::ChangeSize not supported");
    ErrorCode = Error_IOError;
    return 0;
}

SInt    GFILEFile::CopyFromStream(GFile *pstream, SInt byteSize)
{
    UByte   buff[0x4000];
    SInt    count = 0;
    SInt    szRequest, szRead, szWritten;

    while (byteSize)
    {
        szRequest = (byteSize > SInt(sizeof(buff))) ? SInt(sizeof(buff)) : byteSize;

        szRead    = pstream->Read(buff, szRequest);
        szWritten = 0;
        if (szRead > 0)
            szWritten = Write(buff, szRead);

        count    += szWritten;
        byteSize -= szWritten;
        if (szWritten < szRequest)
            break;
    }
    return count;
}


bool    GFILEFile::Close()
{
#ifdef GFILE_VERIFY_SEEK_ERRORS
    if (pFileTestBuffer)
    {
        GFREE(pFileTestBuffer);
        pFileTestBuffer = 0;
        FileTestLength  = 0;
    }
#endif

    bool closeRet = !fclose(fs);

    if (!closeRet)
    {
        ErrorCode = GFC_error();
        return 0;
    }
    else
    {
        Opened    = 0;
        fs        = 0;
        ErrorCode = 0;
    }

    // Handle safe truncate
    /*
    if ((OpenFlags & GFC_FO_SAFETRUNC) == GFC_FO_SAFETRUNC)
    {
        // Delete original file (if it existed)
        DWORD oldAttributes = GFileUtilWin32::GetFileAttributes(FileName);
        if (oldAttributes!=0xFFFFFFFF)
            if (!GFileUtilWin32::DeleteFile(FileName))
            {
                // Try to remove the readonly attribute
                GFileUtilWin32::SetFileAttributes(FileName, oldAttributes & (~FILE_ATTRIBUTE_READONLY) );
                // And delete the file again
                if (!GFileUtilWin32::DeleteFile(FileName))
                    return 0;
            }

        // Rename temp file to real filename
        if (!GFileUtilWin32::MoveFile(TempName, FileName))
        {
            //ErrorCode = errno;
            return 0;
        }
    }
    */
    return 1;
}

/*
bool    GFILEFile::CloseCancel()
{
    bool closeRet = (bool)::CloseHandle(fd);

    if (!closeRet)
    {
        //ErrorCode = errno;
        return 0;
    }
    else
    {
        Opened    = 0;
        fd        = INVALID_HANDLE_VALUE;
        ErrorCode = 0;
    }

    // Handle safe truncate (delete tmp file, leave original unchanged)
    if ((OpenFlags&GFC_FO_SAFETRUNC) == GFC_FO_SAFETRUNC)
        if (!GFileUtilWin32::DeleteFile(TempName))
        {
            //ErrorCode = errno;
            return 0;
        }
    return 1;
}
*/

GFile *GFileFILEOpen(const GString& path, SInt flags, SInt mode)
{
    return GNEW GFILEFile(path, flags, mode);
}

#if !defined(GFC_OS_PS2) && !defined(GFC_OS_PSP) && !defined(GFC_OS_WII) 
// Helper function: obtain file information time.
bool    GSysFile::GetFileStat(GFileStat* pfileStat, const GString& path)
{
#if defined(GFC_OS_WINCE)
    return false;

#else
#if defined(GFC_OS_WIN32)
    // 64-bit implementation on Windows.
    struct __stat64 fileStat;
    // Stat returns 0 for success.
    wchar_t *pwpath = (wchar_t*)GALLOC((GUTF8Util::GetLength(path.ToCStr())+1)*sizeof(wchar_t), GStat_Default_Mem);
    GUTF8Util::DecodeString(pwpath, path.ToCStr());

    int ret = _wstat64(pwpath, &fileStat);
    GFREE(pwpath);
    if (ret) return false;
#else
    struct stat fileStat;
    // Stat returns 0 for success.
    if (stat(path, &fileStat) != 0)
        return false;
#endif
    pfileStat->AccessTime = fileStat.st_atime;
    pfileStat->ModifyTime = fileStat.st_mtime;
    pfileStat->FileSize   = fileStat.st_size;
    return true;
#endif
}
#endif

