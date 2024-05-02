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
#include "GFile.h"

// ***** GFile interface


// ***** GBufferedFile

// Buffered file adds buffering to an existing file
// FILEBUFFER_SIZE defines the size of internal buffer, while
// FILEBUFFER_TOLERANCE controls the amount of data we'll effectively try to buffer
#ifdef GFC_OS_WII
#define FILEBUFFER_SIZE         8192
#else
#define FILEBUFFER_SIZE         (8192-8)
#endif
#define FILEBUFFER_TOLERANCE    4096

// ** Constructor/Destructor

// Hidden constructor
// Not supposed to be used
GBufferedFile::GBufferedFile() : GDelegatedFile(0)
{
    // WII OS requires file buffer to be 32-byte aligned.
    pBuffer     = (UByte*)GMEMALIGN(FILEBUFFER_SIZE, 32, GStat_Default_Mem);
    BufferMode  = NoBuffer;
    FilePos     = 0;
    Pos         = 0;
    DataSize    = 0;
}

// Takes another file as source
GBufferedFile::GBufferedFile(GFile *pfile) : GDelegatedFile(pfile)
{
    // WII OS requires file buffer to be 32-byte aligned.
    pBuffer     = (UByte*)GMEMALIGN(FILEBUFFER_SIZE, 32, GStat_Default_Mem);
    BufferMode  = NoBuffer;
    FilePos     = pfile->LTell();
    Pos         = 0;
    DataSize    = 0;
}


// Destructor
GBufferedFile::~GBufferedFile()
{
    // Flush in case there's data
    if (pFile)
        FlushBuffer();
    // Get rid of buffer
    if (pBuffer)
        GFREE_ALIGN(pBuffer);
}

/*
bool    GBufferedFile::VCopy(const GObject &source)
{
    if (!GDelegatedFile::VCopy(source))
        return 0;

    // Data members
    GBufferedFile *psource = (GBufferedFile*)&source;

    // Buffer & the mode it's in
    pBuffer         = psource->pBuffer;
    BufferMode      = psource->BufferMode;
    Pos             = psource->Pos;
    DataSize        = psource->DataSize;
    return 1;
}
*/

// Initializes buffering to a certain mode
bool    GBufferedFile::SetBufferMode(BufferModeType mode)
{
    if (!pBuffer)
        return false;
    if (mode == BufferMode)
        return true;
     
    FlushBuffer();

    // Can't set write mode if we can't write
    if ((mode==WriteBuffer) && (!pFile || !pFile->IsWritable()) )
        return 0;

    // And SetMode
    BufferMode = mode;
    Pos        = 0;
    DataSize   = 0;
    return 1;
}

// Flushes buffer
void    GBufferedFile::FlushBuffer()
{
    switch(BufferMode)
    {
        case WriteBuffer:
            // Write data in buffer
            FilePos += pFile->Write(pBuffer,Pos);
            Pos = 0;
            break;

        case ReadBuffer:
            // Seek back & reset buffer data
            if ((DataSize-Pos)>0)
                FilePos = pFile->LSeek(-(SInt)(DataSize-Pos), Seek_Cur);
            DataSize = 0;
            Pos      = 0;
            break;
        default:
            // not handled!
            break;
    }
}

// Reloads data for ReadBuffer
void    GBufferedFile::LoadBuffer()
{
    if (BufferMode == ReadBuffer)
    {
        // We should only reload once all of pre-loaded buffer is consumed.
        GASSERT(Pos == DataSize);

        // WARNING: Right now LoadBuffer() assumes the buffer's empty
        SInt sz  = pFile->Read(pBuffer,FILEBUFFER_SIZE);
        DataSize = sz<0 ? 0 : (UInt)sz;
        Pos      = 0;
        FilePos  += DataSize;
    }
}


// ** Overridden functions

// We override all the functions that can possibly
// require buffer mode switch, flush, or extra calculations

// Tell() requires buffer adjustment
SInt    GBufferedFile::Tell()
{
    if (BufferMode == ReadBuffer)
        return SInt(FilePos - DataSize + Pos);

    SInt pos = pFile->Tell();
    // Adjust position based on buffer mode & data
    if (pos!=-1)
    {
        GASSERT(BufferMode != ReadBuffer);
        if (BufferMode == WriteBuffer)
            pos += Pos;
    }
    return pos;
}

SInt64  GBufferedFile::LTell()
{
    if (BufferMode == ReadBuffer)
        return FilePos - DataSize + Pos;

    SInt64 pos = pFile->LTell();
    if (pos!=-1)
    {
        GASSERT(BufferMode != ReadBuffer);
        if (BufferMode == WriteBuffer)
            pos += Pos;
    }
    return pos;
}

SInt    GBufferedFile::GetLength()
{
    SInt len = pFile->GetLength();
    // If writing through buffer, file length may actually be bigger
    if ((len!=-1) && (BufferMode==WriteBuffer))
    {
        SInt currPos = pFile->Tell() + Pos;
        if (currPos>len)
            len = currPos;
    }
    return len;
}
SInt64  GBufferedFile::LGetLength()
{
    SInt64 len = pFile->LGetLength();
    // If writing through buffer, file length may actually be bigger
    if ((len!=-1) && (BufferMode==WriteBuffer))
    {
        SInt64 currPos = pFile->LTell() + Pos;
        if (currPos>len)
            len = currPos;
    }
    return len;
}

/*
bool    GBufferedFile::Stat(GFileStats *pfs)
{
    // Have to fix up length is stat
    if (pFile->Stat(pfs))
    {
        if (BufferMode==WriteBuffer)
        {
            SInt64 currPos = pFile->LTell() + Pos;
            if (currPos > pfs->Size)
            {
                pfs->Size   = currPos;
                // ??
                pfs->Blocks = (pfs->Size+511) >> 9;
            }
        }
        return 1;
    }
    return 0;
}
*/

SInt    GBufferedFile::Write(const UByte *psourceBuffer, SInt numBytes)
{
    if ( (BufferMode==WriteBuffer) || SetBufferMode(WriteBuffer))
    {
        // If not data space in buffer, flush
        if ((FILEBUFFER_SIZE-(SInt)Pos)<numBytes)
        {
            FlushBuffer();
            // If bigger then tolerance, just write directly
            if (numBytes>FILEBUFFER_TOLERANCE)
            {
                SInt sz = pFile->Write(psourceBuffer,numBytes);
                if (sz > 0)
                    FilePos += sz;
                return sz;
            }
        }

        // Enough space in buffer.. so copy to it
        memcpy(pBuffer+Pos, psourceBuffer, numBytes);
        Pos += numBytes;
        return numBytes;
    }
    SInt sz = pFile->Write(psourceBuffer,numBytes);
    if (sz > 0)
        FilePos += sz;
    return sz;
}

SInt    GBufferedFile::Read(UByte *pdestBuffer, SInt numBytes)
{
    if ( (BufferMode==ReadBuffer) || SetBufferMode(ReadBuffer))
    {
#ifdef GFC_OS_WII
        SInt totalRead = 0;
        while (numBytes > 0)
        {
            // Data in buffer... copy it
            if (((SInt)DataSize-(SInt)Pos)>=numBytes)
            {
                memcpy(pdestBuffer, pBuffer+Pos, numBytes);
                Pos += numBytes;
                return totalRead + numBytes;
            }

            // Not enough data in buffer, copy buffer
            SInt    readBytes = DataSize-Pos;
            memcpy(pdestBuffer, pBuffer+Pos, readBytes);
            numBytes    -= readBytes;
            pdestBuffer += readBytes;
            totalRead   += readBytes;
            Pos = DataSize;

            LoadBuffer();
            if (FilePos >= GetLength() && Pos == DataSize)
                return totalRead;
        }
        return totalRead;
#else        
        
        // Data in buffer... copy it
        if ((SInt)(DataSize-Pos) >= numBytes)
        {
            memcpy(pdestBuffer, pBuffer+Pos, numBytes);
            Pos += numBytes;
            return numBytes;
        }

        // Not enough data in buffer, copy buffer
        SInt    readBytes = DataSize-Pos;
        memcpy(pdestBuffer, pBuffer+Pos, readBytes);
        numBytes    -= readBytes;
        pdestBuffer += readBytes;
        Pos = DataSize;

        // Don't reload buffer if more then tolerance
        // (No major advantage, and we don't want to write a loop)
        if (numBytes>FILEBUFFER_TOLERANCE)
        {
            numBytes = pFile->Read(pdestBuffer,numBytes);
            if (numBytes > 0)
            {
                FilePos += numBytes;
                Pos = DataSize = 0;
            }
            return readBytes + ((numBytes==-1) ? 0 : numBytes);
        }

        // Reload the buffer
        // WARNING: Right now LoadBuffer() assumes the buffer's empty
        LoadBuffer();
        if ((SInt)(DataSize-Pos) < numBytes)
            numBytes = (SInt)DataSize-Pos;

        memcpy(pdestBuffer, pBuffer+Pos, numBytes);
        Pos += numBytes;
        return numBytes + readBytes;
        
        /*
        // Alternative Read implementation. The one above is probably better
        // due to FILEBUFFER_TOLERANCE.
        SInt    total = 0;

        do {
            SInt    bufferBytes = (SInt)(DataSize-Pos);
            SInt    copyBytes = (bufferBytes > numBytes) ? numBytes : bufferBytes;

            memcpy(pdestBuffer, pBuffer+Pos, copyBytes);
            numBytes    -= copyBytes;
            pdestBuffer += copyBytes;
            Pos         += copyBytes;
            total       += copyBytes;

            if (numBytes == 0)
                break;
            LoadBuffer();

        } while (DataSize > 0);

        return total;
        */     
        
#endif
    }
    SInt sz = pFile->Read(pdestBuffer,numBytes);
    if (sz > 0)
        FilePos += sz;
    return sz;
}


SInt    GBufferedFile::SkipBytes(SInt numBytes)
{
    SInt skippedBytes = 0;

    // Special case for skipping a little data in read buffer
    if (BufferMode==ReadBuffer)
    {
        skippedBytes = (((SInt)DataSize-(SInt)Pos) >= numBytes) ? numBytes : (DataSize-Pos);
        Pos          += skippedBytes;
        numBytes     -= skippedBytes;
    }

    if (numBytes)
    {
        numBytes = pFile->SkipBytes(numBytes);
        // Make sure we return the actual number skipped, or error
        if (numBytes!=-1)
        {
            skippedBytes += numBytes;
            FilePos += numBytes;
            Pos = DataSize = 0;
        }
        else if (skippedBytes <= 0)
            skippedBytes = -1;
    }
    return skippedBytes;
}

SInt    GBufferedFile::BytesAvailable()
{
    SInt available = pFile->BytesAvailable();
    // Adjust available size based on buffers
    switch(BufferMode)
    {
        case ReadBuffer:
            available += DataSize-Pos;
            break;
        case WriteBuffer:
            available -= Pos;
            if (available<0)
                available= 0;
            break;
        default:
            break;
    }
    return available;
}

bool    GBufferedFile::Flush()
{
    FlushBuffer();
    return pFile->Flush();
}

// Seeking could be optimized better..
SInt    GBufferedFile::Seek(SInt offset, SInt origin)
{    
    if (BufferMode == ReadBuffer)
    {
        if (origin == Seek_Cur)
        {
            // Seek can fall either before or after Pos in the buffer,
            // but it must be within bounds.
            if (((UInt(offset) + Pos)) <= DataSize)
            {
                Pos += offset;
                return SInt(FilePos - DataSize + Pos);
            }

            // Lightweight buffer "Flush". We do this to avoid an extra seek
            // back operation which would take place if we called FlushBuffer directly.
            origin = Seek_Set;
            GASSERT(((FilePos - DataSize + Pos) + (UInt64)offset) < ~(UInt64)0);
            offset = (SInt)(FilePos - DataSize + Pos) + offset;
            Pos = DataSize = 0;
        }
        else if (origin == Seek_Set)
        {
            if (((UInt)offset - (FilePos-DataSize)) <= DataSize)
            {
                GASSERT((FilePos-DataSize) < ~(UInt64)0);
                Pos = (UInt)offset - (UInt)(FilePos-DataSize);
                return offset;
            }
            Pos = DataSize = 0;
        }
        else
        {
            FlushBuffer();
        }
    }
    else
    {
        FlushBuffer();
    }    

    /*
    // Old Seek Logic
    if (origin == Seek_Cur && offset + Pos < DataSize)
    {
        //GASSERT((FilePos - DataSize) >= (FilePos - DataSize + Pos + offset));
        Pos += offset;
        GASSERT(SInt(Pos) >= 0);
        return SInt(FilePos - DataSize + Pos);
    }
    else if (origin == Seek_Set && UInt(offset) >= FilePos - DataSize && UInt(offset) < FilePos)
    {
        Pos = UInt(offset - FilePos + DataSize);
        GASSERT(SInt(Pos) >= 0);
        return SInt(FilePos - DataSize + Pos);
    }   
    
    FlushBuffer();
    */


    FilePos = pFile->Seek(offset,origin);
    return SInt(FilePos);
}

SInt64  GBufferedFile::LSeek(SInt64 offset, SInt origin)
{
    if (BufferMode == ReadBuffer)
    {
        if (origin == Seek_Cur)
        {
            // Seek can fall either before or after Pos in the buffer,
            // but it must be within bounds.
            if (((UInt(offset) + Pos)) <= DataSize)
            {
                Pos += (UInt)offset;
                return SInt64(FilePos - DataSize + Pos);
            }

            // Lightweight buffer "Flush". We do this to avoid an extra seek
            // back operation which would take place if we called FlushBuffer directly.
            origin = Seek_Set;            
            offset = (SInt64)(FilePos - DataSize + Pos) + offset;
            Pos = DataSize = 0;
        }
        else if (origin == Seek_Set)
        {
            if (((UInt64)offset - (FilePos-DataSize)) <= DataSize)
            {                
                Pos = (UInt)((UInt64)offset - (FilePos-DataSize));
                return offset;
            }
            Pos = DataSize = 0;
        }
        else
        {
            FlushBuffer();
        }
    }
    else
    {
        FlushBuffer();
    }

/*
    GASSERT(BufferMode != NoBuffer);

    if (origin == Seek_Cur && offset + Pos < DataSize)
    {
        Pos += SInt(offset);
        return FilePos - DataSize + Pos;
    }
    else if (origin == Seek_Set && offset >= SInt64(FilePos - DataSize) && offset < SInt64(FilePos))
    {
        Pos = UInt(offset - FilePos + DataSize);
        return FilePos - DataSize + Pos;
    }

    FlushBuffer();
    */

    FilePos = pFile->LSeek(offset,origin);
    return FilePos;
}

bool    GBufferedFile::ChangeSize(SInt newSize)
{
    FlushBuffer();
    return pFile->ChangeSize(newSize);
}

SInt    GBufferedFile::CopyFromStream(GFile *pstream, SInt byteSize)
{
    // We can't rely on overridden Write()
    // because delegation doesn't override virtual pointers
    // So, just re-implement
    UByte   buff[0x4000];
    SInt    count = 0;
    SInt    szRequest, szRead, szWritten;

    while(byteSize)
    {
        szRequest = (byteSize > SInt(sizeof(buff))) ? SInt(sizeof(buff)) : byteSize;

        szRead    = pstream->Read(buff,szRequest);
        szWritten = 0;
        if (szRead > 0)
            szWritten = Write(buff,szRead);

        count   +=szWritten;
        byteSize-=szWritten;
        if (szWritten < szRequest)
            break;
    }
    return count;
}

// Closing files
bool    GBufferedFile::Close()
{
    switch(BufferMode)
    {
        case WriteBuffer:
            FlushBuffer();
            break;
        case ReadBuffer:
            // No need to seek back on close
            BufferMode = NoBuffer;
            break;
        default:
            break;
    }
    return pFile->Close();
}

/*
bool    GBufferedFile::CloseCancel()
{
    // If operation's being canceled
    // There's not need to flush buffers at all
    BufferMode = NoBuffer;
    return pFile->CloseCancel();
}
*/

// ***** Global path helpers

// Find trailing short filename in a path.
const char* G_GetShortFilename(const char* purl)
{    
    UPInt len = G_strlen(purl);
    for (UPInt i=len; i>0; i--) 
        if (purl[i]=='\\' || purl[i]=='/')
            return purl+i+1;
    return purl;
}



