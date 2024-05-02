/**********************************************************************

Filename    :   GZLibFile.h
Content     :   Header for z-lib wrapped file input 
Created     :   June 24, 2005
Authors     :   Michael Antonov

Notes       :   GZLibFile is currently Read Only

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GZLibFile_H
#define INC_GZLibFile_H

#include "GFile.h"

// ZLib functionality is only available if GFC_USE_ZLIB is defined
#ifdef GFC_USE_ZLIB

// ***** Declared Classes
class   GZLibFile;

class GZLibFile : public GFile
{
    friend class GZLibFileImpl;
    friend class GFxZlibSupport;

    class GZLibFileImpl   *pImpl;
    
    // ZLib Helper: Initializes z_stream by setting allocators and calling inflateInit.
    //  - pallocOwner is used as 'this' for GALLOC_AUTO_HEAP.
    //  - pbuffer, buffer size are used to initialize the output buffer.
    // Returns zlib error code, which is Z_OK for success.
    static int    ZLib_InitStream(struct z_stream_s* pstream, void* pallocowner,
                                  void* pbuffer = 0, unsigned int bufferSize = 0);

public:

    // GZLibFile must be constructed with a source file 
    GZLibFile(GFile *psourceFile = 0);
    ~GZLibFile();

    // ** File Information
    virtual const char* GetFilePath();

    // Return 1 if file's usable (open)
    virtual bool        IsValid();
    // Return 0; ZLib files are not writable for now
    virtual bool        IsWritable() { return 0; }

    // Return position
    // Position position is reported in relation to the compressed stream, NOT the source file
    virtual SInt        Tell ();
    virtual SInt64      LTell ();
    virtual SInt        GetLength ();
    virtual SInt64      LGetLength ();
    // Return errno-based error code
    virtual SInt        GetErrorCode();

    // ** GFxStream implementation & I/O
    
    virtual SInt        Write(const UByte *pbufer, SInt numBytes);  
    virtual SInt        Read(UByte *pbufer, SInt numBytes);
    virtual SInt        SkipBytes(SInt numBytes);           
    virtual SInt        BytesAvailable();
    virtual bool        Flush();
    // Returns new position, -1 for error
    // Position seeking works in relation to the compressed stream, NOT the source file
    virtual SInt        Seek(SInt offset, SInt origin=SEEK_SET);
    virtual SInt64      LSeek(SInt64 offset, SInt origin=SEEK_SET);
    // Writing not supported..
    virtual bool        ChangeSize(SInt newSize);       
    virtual SInt        CopyFromStream(GFile *pstream, SInt byteSize);
    // Closes the file  
    virtual bool        Close();    
};

#endif // GFC_USE_ZLIB


#endif
