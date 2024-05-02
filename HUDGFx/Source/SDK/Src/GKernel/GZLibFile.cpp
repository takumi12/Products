/**********************************************************************

Filename    :   GZlibFile.cpp
Content     :   
Created     :   April 5, 1999
Authors     :   Michael Antonov

History     :   

Copyright   :   (c) 2004-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

// GKernel
#include "GZlibFile.h"
#include "GHeapNew.h"

#ifdef GFC_USE_ZLIB

#include <zlib.h>

// This flag can be set to verify that read data after seeks and Resets
// always matches the actual decompressed stream. We've seen this fail
// due to EAX2/DirectSound corrupting memory under FMOD with XP64 (32-bit)
// and Realtek HA Audio driver.
//#define GLIBFILE_VERIFY_DATA


// ***** GFile interface

// Internal class used to store data necessary for deflation
class GZLibFileImpl : public GNewOverrideBase<GStat_Default_Mem>
{
public:
    GPtr<GFile> pIn;
    
    z_stream    ZStream;
    SInt        InitialStreamPos;   // position of the input stream where we started inflating.
    SInt        LogicalStreamPos;   // current stream position of uncompressed data.
    bool        AtEofFlag;
    SInt        ErrorCode;

    enum ZLibConstants
    {
        ZLib_BuffSize       = 4096,
#ifdef GLIBFILE_VERIFY_DATA
        ZLib_BacktrackSize  = 256
#else
        // ZLib_BacktrackSize should be as big as JPEG_BufferSize in GJPEGUtil_jpeglib.cpp
        // to avoid expensive z-lib buffer restarts.
        ZLib_BacktrackSize  = 2048
#endif
    };

    // User position in file, can be < then LogicalStreamPos.
    SInt    UserPos;
    SInt    BacktrackTail; // Pointer to last byte of buffer + 1.
                           // Data can warp around circularly, spanning
                           // range from [BacktrackTail, BacktrackTail-1].
    SInt    BacktrackSize; // Number of usable bytes in backtrack buffer.

    // Last bytes output - circular buffer used to allow cheap seek-back.
    UByte   BacktrackBuffer[ZLib_BacktrackSize];
    // Data buffer used by ZLib for input.
    UByte   DataBuffer[ZLib_BuffSize];
    
#ifdef GLIBFILE_VERIFY_DATA
    UByte* pHugeBuffer;
    SInt   HugeSize;
#endif

    // Constructor.
    GZLibFileImpl(GFile* pin)       
    {       
        // Init file pointer and other values
        pIn                 = pin;
        InitialStreamPos    = pIn->Tell();
        LogicalStreamPos    = 0;
        AtEofFlag           = 0;
        ErrorCode           = 0;
        
        // Initialize ZStream to our allocator and no I/O data for now.
        if (GZLibFile::ZLib_InitStream(&ZStream, this) != Z_OK)
        {
            //log_error("Error: GZLibFileImpl::ctor() inflateInit() returned %d\n", err);
            ErrorCode = 1;
            return;
        }

        // No backtrack data available yet.
        BacktrackSize = 0;
        BacktrackTail= 0;
        UserPos       = 0;

#ifdef GLIBFILE_VERIFY_DATA
        pHugeBuffer = (UByte*)GALLOC(1024*1024*8, GStat_Default_Mem);
        HugeSize    = Inflate(pHugeBuffer, 1024*1024*8);
        Reset();
#endif        
    }

    ~GZLibFileImpl()
    {
#ifdef GLIBFILE_VERIFY_DATA
        GFREE(pHugeBuffer);
#endif
    }


    // Discard current results and rewind to the beginning.
    // Necessary in order to seek backwards.    
    void    Reset()
    {
        ErrorCode = 0;
        AtEofFlag = 0;
        int err = inflateReset(&ZStream);
        if (err != Z_OK)
        {
            ErrorCode = 1;
            return;
        }

        ZStream.next_in     = 0;
        ZStream.avail_in    = 0;
        ZStream.next_out    = 0;
        ZStream.avail_out   = 0;

        // Rewind the underlying stream.
        pIn->Seek(InitialStreamPos);

        LogicalStreamPos = 0;

        BacktrackSize = 0;
        BacktrackTail= 0;
        UserPos       = 0;
    }


    // General reading.
    // Inflate while updating backtrack buffer.
    // Can also be used to read data after backtrack.
    SInt    Inflate(void* dst, int bytes)
    {
        SInt bytesOutput = 0;

#ifdef GLIBFILE_VERIFY_DATA
        UByte * porigDst = (UByte*)dst;
        UInt   copySize1 = 0, copySize2 = 0;
        UInt    tailSpace1 = 0;
#endif

        // Check the backtrack part of the stream and read/copy it.
        if (UserPos < LogicalStreamPos)
        {
            // Data must be in buffer (otherwise there is a bug in SetPosition).
            GASSERT(UserPos >= (LogicalStreamPos - BacktrackSize));
            GASSERT(BacktrackTail < (ZLib_BacktrackSize+1));

            // Determine how many bytes we will take from backtrack buffer,
            // and staring where (backtrackDataPos counts back from current pointer).
            SInt backtrackDataPos = LogicalStreamPos - UserPos;            
            SInt backtrackData    = (backtrackDataPos > bytes) ? bytes : backtrackDataPos;
            SInt backtrackDataSave= backtrackData;
            
            if (backtrackDataPos > BacktrackTail)
            {
                // Wrap-around can only happen once we have full buffer.
                GASSERT(BacktrackSize == ZLib_BacktrackSize);

                // Need to grab data from tail first.
                SInt    copyOffset = BacktrackSize - (backtrackDataPos - BacktrackTail);
                SInt    copySize   = BacktrackSize - copyOffset;
                if (copySize > backtrackData)
                    copySize = backtrackData;

#ifdef GLIBFILE_VERIFY_DATA
                copySize1 = copySize;
#endif

                memcpy(dst, BacktrackBuffer + copyOffset, copySize);
                backtrackData -= copySize;
                backtrackDataPos -= copySize;
                dst = ((UByte*)dst) + copySize;
            }

            // The rest of the data comes from the first part before the start
            if (backtrackData > 0)
            {
                GASSERT(backtrackDataPos <= BacktrackTail);
                memcpy(dst, BacktrackBuffer + (BacktrackTail - backtrackDataPos), backtrackData);
                dst = ((UByte*)dst) + backtrackData;

#ifdef GLIBFILE_VERIFY_DATA
                copySize2 = backtrackData;
#endif
            }

            bytes       -= backtrackDataSave;
            bytesOutput += backtrackDataSave;
            UserPos     += backtrackDataSave;
        }

        // Read the data from ZLib.
        if (bytes > 0)
        {
            SInt bytesRead = InflateFromStream(dst, bytes);

            // If the request would not fit in its entirety in backtrack
            // buffer, remember its last chunk.
            if (bytesRead >= ZLib_BacktrackSize)
            {
                BacktrackTail = ZLib_BacktrackSize;
                BacktrackSize = ZLib_BacktrackSize;
                memcpy(BacktrackBuffer, ((UByte*)dst) + (bytesRead - BacktrackSize), BacktrackSize);
            }

            else if (bytesRead > 0)
            {
                UByte *psrc = (UByte*)dst;

                // Read the piece into the tail of the buffer.
                SInt tailSpace= ZLib_BacktrackSize - BacktrackTail;
                SInt copySize = (tailSpace < bytesRead) ? tailSpace : bytesRead;

#ifdef GLIBFILE_VERIFY_DATA
                tailSpace1 = tailSpace;
#endif

                if (copySize > 0)
                {
                    memcpy(BacktrackBuffer + BacktrackTail, psrc, copySize);
                    psrc += copySize;
                    BacktrackTail += copySize;
                }

                // If there is mode data, do wrap-around to beginning of buffer.
                if (bytesRead > copySize)
                {
                    BacktrackTail = bytesRead - copySize;
                    memcpy(BacktrackBuffer, psrc, BacktrackTail);
                }
                
                // Once maximum backtrack size is reached, it doesn't change.
                if (BacktrackSize < ZLib_BacktrackSize)
                {
                    BacktrackSize += bytesRead;
                    if (BacktrackSize > ZLib_BacktrackSize)
                        BacktrackSize = ZLib_BacktrackSize;
                }
            }

            UserPos = LogicalStreamPos;
            bytesOutput += bytesRead;
        }
        
#ifdef GLIBFILE_VERIFY_DATA
        if (UserPos < HugeSize) 
        {
            //GASSERT(!memcmp(pHugeBuffer + UserPos - bytesOutput, porigDst, bytesOutput));
            UByte* pcompareBuffer = pHugeBuffer + UserPos - bytesOutput;

            for (SInt i = 0; i < bytesOutput; i++)
            {
                GASSERT(pcompareBuffer[i] == porigDst[i]);
            }
        }
#endif

        return bytesOutput;
    }



    // Seek to the target position.
    SInt    SetPosition(SInt offset)
    {        
        if (offset < LogicalStreamPos)
        {
            // If we can seek within a backtrack buffer, do so.
            if (offset >= (LogicalStreamPos - BacktrackSize))
            {
                UserPos = offset;
                return UserPos;
            }

            // MA DEBUG
            // GFC_DEBUG_MESSAGE1(1, "GZLibFileImpl::SetPosition(%d) - Restarting", offset);

            // Otherwise we must re-read.
            Reset();
        }
        else if (offset > LogicalStreamPos)
        {
            //GFC_DEBUG_MESSAGE1(1, "GZLibFileImpl::SetPosition(%d) - Seeking forward", offset);
            UserPos = LogicalStreamPos;
        }

        UByte   temp[GZLibFileImpl::ZLib_BuffSize];
        
        // Now seek forwards, by just reading data in blocks.
        while (UserPos < offset)
        {
            SInt    toRead = offset - UserPos;
            SInt    toReadThisTime = G_Min<SInt>(toRead, GZLibFileImpl::ZLib_BuffSize);
            //assert(toReadThisTime > 0);

            SInt    bytesRead = Inflate(temp, toReadThisTime);
            GASSERT(bytesRead <= toReadThisTime);

            // Trouble; can't seek any further.
            if (bytesRead == 0)
                break;
        }

        // Return new location.
        return UserPos;
    }



    // Decompress from ZLib stream into specified buffer.
    SInt    InflateFromStream(void* dst, int bytes)
    {
        if (ErrorCode)          
            return 0;           

        ZStream.next_out = (unsigned char*) dst;
        ZStream.avail_out = bytes;

        while (1)
        {
            if (ZStream.avail_in == 0)
            {
                // Get more raw data.
                int newBytes = pIn->Read(DataBuffer, ZLib_BuffSize);                
                if (newBytes == 0)
                {
                    // The cupboard is bare!  We have nothing to feed to inflate().
                    break;
                }
                else
                {
                    GASSERT(newBytes > 0);
                    ZStream.next_in = DataBuffer;
                    ZStream.avail_in = newBytes;
                }
            }

            int err = inflate(&ZStream, Z_SYNC_FLUSH);
            if (err == Z_STREAM_END)
            {
                AtEofFlag = 1;
                break;
            }
            if (err != Z_OK)
            {
                // something's wrong.
                ErrorCode = 1;
                break;
            }

            if (ZStream.avail_out == 0)             
                break;              
        }

        int bytesRead = bytes - ZStream.avail_out;
        LogicalStreamPos += bytesRead;

        return bytesRead;
    }

    // If we have unused bytes in our input buffer, rewind
    // to before they started.
    void    RewindUnusedBytes() 
    {
        if (ZStream.avail_in > 0)
        {
            SInt    pos = pIn->Tell();
            SInt    rewoundPos = pos - ZStream.avail_in;
            // GASSERT(pos >= 0);
            // GASSERT(pos >= InitialStreamPos);
            // GASSERT(rewound_pos >= 0);
            // GASSERT(rewound_pos >= InitialStreamPos);

            pIn->Seek(rewoundPos);
        }
    }

};


// ***** GZLibFile Implementation

// GZLibFile must be constructed with a source file 
GZLibFile::GZLibFile(GFile *psourceFile)
{
    pImpl = 0;
    if (psourceFile && psourceFile->IsValid())
        pImpl = GHEAP_AUTO_NEW(this) GZLibFileImpl(psourceFile);
    else
    {
        GFC_DEBUG_WARNING(1, "GZLibFile constructor failed, psourceFile is not valid");
    }
}

GZLibFile::~GZLibFile()
{
    // If implementation still exists, release inflater
    if (pImpl)
    {
        pImpl->RewindUnusedBytes();
        inflateEnd(&(pImpl->ZStream));
        delete pImpl;   
    }
}


// ** File Information
const char* GZLibFile::GetFilePath()
{
    return pImpl ? pImpl->pIn->GetFilePath() : 0;
}

// Return 1 if file's usable (open)
bool    GZLibFile::IsValid()
{
    return pImpl ? 1 : 0;
}

// Return position
SInt    GZLibFile::Tell ()
{
    if (!pImpl) return -1;
    return pImpl->UserPos;
}
SInt64  GZLibFile::LTell ()
    { return Tell(); }


SInt    GZLibFile::GetLength ()
{
    if (!pImpl) return 0;
    if (pImpl->ErrorCode)
        return 0;

    // This is expensive..
    SInt oldPos = pImpl->UserPos;
    SInt endPos = SeekToEnd();
    Seek(oldPos);
    return endPos;
}

SInt64  GZLibFile::LGetLength ()
{
    return GetLength();
}

// Return errno-based error code
SInt    GZLibFile::GetErrorCode()
{
    // TODO : convert to ERRNO
    return pImpl ? pImpl->ErrorCode : 0;
}


// ** GFxStream implementation & I/O

SInt    GZLibFile::Write(const UByte *pbuffer, SInt numBytes)
{
    GUNUSED2(pbuffer, numBytes);
    GFC_DEBUG_WARNING(1, "GZLibFile::Write is not yet supported");
    return 0;
}

SInt    GZLibFile::Read(UByte *pbuffer, SInt numBytes)
{
    if (!pImpl) return -1;
    return pImpl->Inflate(pbuffer, numBytes);
}


SInt    GZLibFile::SkipBytes(SInt numBytes)
{
    return Seek(numBytes, SEEK_CUR);
}

SInt    GZLibFile::BytesAvailable()
{
    if (!pImpl) return 0;
    if (pImpl->ErrorCode)
        return 0;

    // This is pricey..
    SInt oldPos = pImpl->UserPos;
    SInt endPos = SeekToEnd();
    Seek(oldPos);

    return endPos - oldPos;
}

bool    GZLibFile::Flush()
{
    return 1;
}


// Returns new position, -1 for error
SInt    GZLibFile::Seek(SInt offset, SInt origin)
{
    if (!pImpl) return -1;

    //If there us an error, bail
    if (pImpl->ErrorCode)
        return pImpl->UserPos;

    switch(origin)
    {
        case Seek_Cur:
            // Adjust offset and fall through
            offset += pImpl->UserPos;

        case Seek_Set:
        zlib_seek_set:
            pImpl->SetPosition(offset);
            // Return new location
            break;
        
        case Seek_End:
            // Seek forward as far as possible (till end).
            pImpl->SetPosition(0x7FFFFFFF);

            // If offset is not at the end (i.e. usually negative), seek back
            if (offset != 0)
            {
                offset = pImpl->UserPos + offset;
                goto zlib_seek_set;
            }
            break;
    }

    return pImpl->UserPos;
}


SInt64  GZLibFile::LSeek(SInt64 offset, SInt origin)
{
    GFC_DEBUG_WARNING(offset > (SInt64)0x7FFFFFFF, "GZLibFile::LSeek offset out of range, 64bit seek not yet supported" );
    return Seek((SInt)offset, origin);
}

// Writing not supported..
bool    GZLibFile::ChangeSize(SInt newSize)
{
    GUNUSED(newSize);
    GFC_DEBUG_WARNING(1, "GZLibFile::ChangeSize is not yet supported");
    return 0;
}
SInt    GZLibFile::CopyFromStream(GFile *pstream, SInt byteSize)
{
    GUNUSED2(pstream, byteSize);
    GFC_DEBUG_WARNING(1, "GZLibFile::CopyFromStream is not yet supported");
    return 0;
}

// Closes the file  
bool    GZLibFile::Close()
{
    if (!pImpl) return 0;

    pImpl->RewindUnusedBytes();
    // Close really should not fail, since there is no way to handle that nicely
    int err = inflateEnd(&(pImpl->ZStream));

    //  Close & release the file
    pImpl->pIn->Close();
    delete pImpl;
    pImpl = 0;
    return (err == Z_OK);
}


// *** ZLib helper functions

// Memory allocation functions for ZLib.
static voidpf ZLibAllocFunc(voidpf opaque, uInt items, uInt size)
{
    void* pmem = GHEAP_AUTO_ALLOC(opaque, size * items);
   //  GFC_DEBUG_MESSAGE3(1, "Zlib Alloc 0x%08X - 0x%08X, %d bytes", pmem, ((UPInt)pmem) +  size * items - 1,  size * items);
    return pmem;
    //return GHEAP_AUTO_ALLOC(opaque, size * items);
}
static void   ZLibFreeFunc(voidpf, voidpf address)
{
   // GFC_DEBUG_MESSAGE1(1, "Zlib Free  0x%08X", address);
    GFREE(address);
}

int    GZLibFile::ZLib_InitStream(struct z_stream_s* pstream, void* pallocowner,
                                  void* pbuffer, unsigned int bufferSize)
{
    pstream->zalloc     = ZLibAllocFunc;
    pstream->zfree      = ZLibFreeFunc;
    pstream->opaque     = (voidpf)pallocowner;

    pstream->next_in    = 0;
    pstream->avail_in   = 0;
    pstream->next_out   = (Byte*)pbuffer;
    pstream->avail_out  = (uInt) bufferSize;

    pstream->data_type  =0;
    pstream->adler      =0;
    pstream->reserved   =0;

    return inflateInit(pstream);
}



#endif // GFC_USE_ZLIB
