/**********************************************************************

Filename    :   GFxAmpStream.h
Content     :   Stream used by AMP to serialize messages
Created     :   December 2009
Authors     :   Alex Mantzaris

Copyright   :   (c) 2005-2009 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INCLUDE_GFX_AMP_STREAM_H
#define INCLUDE_GFX_AMP_STREAM_H

#include "GString.h"
#include "GHash.h"
#include "GRefCount.h"
#include "GFile.h"

// Custom stream class used to send messages across the network
// The size of each message precedes the data so we can split messages into 
// packets and reconstruct them after they have been received on the other end
//
// The data are stored as a Little-endian array of bytes
// The object normally holds only one message for write operations
// because the first four bytes are used to hold the message size
// For read operations, multiple messages can be contained on the stream
// After a message has been processed, PopFirstMessage is called to remove it
class GFxAmpStream : public GFile
{
public:
    // Default constructor (for writing a new message)
    GFxAmpStream();
    // Buffer constructor (for parsing a received message)
    GFxAmpStream(const UByte* buffer, UPInt bufferSize);

    // GFile virtual method overrides
    virtual const char* GetFilePath()   { return ""; } 
    virtual bool        IsValid()       { return true; }
    virtual bool        IsWritable()    { return true; }
    virtual SInt        Tell ()         { return readPosition; }
    virtual SInt64      LTell ()        { return  Tell(); }
    virtual SInt        GetLength ()    { return static_cast<SInt>(Data.GetSize()); }
    virtual SInt64      LGetLength ()   { return GetLength(); }
    virtual SInt        GetErrorCode()  { return 0; }
    virtual bool        Flush()         { return true; }
    virtual bool        Close()         { return false; }
    virtual SInt        Write(const UByte *pbufer, SInt numBytes);
    virtual SInt        Read(UByte *pbufer, SInt numBytes);
    virtual SInt        SkipBytes(SInt numBytes);                     
    virtual SInt        BytesAvailable();
    virtual SInt        Seek(SInt offset, SInt origin=Seek_Set);
    virtual SInt64      LSeek(SInt64 offset, SInt origin=Seek_Set);
    virtual bool        ChangeSize(SInt newSize);
    virtual SInt        CopyFromStream(GFile *pstream, SInt byteSize);

    // Custom read/write methods for common types
    void                ReadString(GString* str);
    void                WriteString(const GString& str);
    void                ReadStream(GFxAmpStream* str);
    void                WriteStream(const GFxAmpStream& str);

    // Append an buffer that is already in the GFxAmpStream format
    void                Append(const UByte* buffer, UPInt bufferSize);

    // Data accessors
    const UByte*        GetBuffer() const;
    UPInt               GetBufferSize() const;

    // Clear the stream
    void                Clear();

    // Set the current read position to the beginning
    void                Rewind();

    // Message retrieval methods
    UPInt               FirstMessageSize();
    bool                PopFirstMessage();

private:
    typedef GArrayConstPolicy<0, 4, true> NeverShrinkPolicy;
    GArrayLH<UByte, GStat_Default_Mem, NeverShrinkPolicy>   Data;
    SInt                                                    readPosition;  // next location to be read

    // Update the header with the message size
    // Assumes that there is only one message currently in the stream
    void IncreaseMessageSize(UInt32 newSize);
};

#endif
