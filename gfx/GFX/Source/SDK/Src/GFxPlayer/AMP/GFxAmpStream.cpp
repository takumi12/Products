/**********************************************************************

Filename    :   GFxAmpStream.cpp
Content     :   Stream used by AMP to serialize messages
Created     :   December 2009
Authors     :   Alex Mantzaris

Copyright   :   (c) 2005-2009 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxAmpStream.h"

// Default constructor (for writing a new message)
GFxAmpStream::GFxAmpStream() : readPosition(0)
{
}

// Buffer constructor (for parsing a received message)
// Assumes that the input buffer includes the header information (length)
GFxAmpStream::GFxAmpStream(const UByte* buffer, UPInt bufferSize) : readPosition(0)
{
    Append(buffer, bufferSize);
}

// GFile virtual method override
SInt GFxAmpStream::SkipBytes(SInt numBytes)
{ 
    readPosition += numBytes; 
    return numBytes; 
}

// GFile virtual method override
SInt GFxAmpStream::BytesAvailable()
{ 
    return (static_cast<SInt>(Data.GetSize()) - readPosition); 
}

// GFile virtual method override
// Not supported for GFxAmpStream
SInt GFxAmpStream::Seek(SInt offset, SInt origin)
{ 
    GUNUSED2(offset, origin); 
    GASSERT(false); // Seek not supported
    return -1; 
}

// GFile virtual method override
// Not supported for GFxAmpStream
SInt64 GFxAmpStream::LSeek(SInt64 offset, SInt origin)
{ 
    GUNUSED2(offset, origin); 
    GASSERT(false); // LSeek not supported
    return -1;  
}

// GFile virtual method override
// Not supported for GFxAmpStream
bool GFxAmpStream::ChangeSize(SInt newSize)
{ 
    GUNUSED(newSize); 
    GASSERT(false); // ChangeSize not supported
    return false;
}

// GFile virtual method override
// Not supported for GFxAmpStream
SInt GFxAmpStream::CopyFromStream(GFile *pstream, SInt byteSize)
{ 
    GUNUSED2(pstream, byteSize);
    GASSERT(false); // Copy from stream not supported
    return -1;
}

// Write a number of bytes to the stream
// GFile virtual method override
// Updates the message size stored in the header
SInt GFxAmpStream::Write(const UByte *pbufer, SInt numBytes)
{
    IncreaseMessageSize(numBytes);
    memcpy(&Data[GetBufferSize() - numBytes], pbufer, numBytes);
    return numBytes;
}

// Read a number of bytes from the stream
// GFile virtual method override
SInt GFxAmpStream::Read(UByte *pbufer, SInt numBytes)
{
    memcpy(pbufer, &Data[readPosition], numBytes);
    readPosition += numBytes;
    return numBytes;
}

// Read a string
void GFxAmpStream::ReadString(GString* str)
{
    str->Clear();
    UInt32 stringLength = ReadUInt32();
    for (UInt32 i = 0; i < stringLength; ++i)
    {
        str->AppendChar(ReadSByte());
    }
}

// Write a string
void GFxAmpStream::WriteString(const GString& str)
{
    WriteUInt32(static_cast<UInt32>(str.GetLength()));
    for (UPInt i = 0; i < str.GetLength(); ++i)
    {
        WriteSByte(str[i]);
    }
}

void GFxAmpStream::ReadStream(GFxAmpStream* str)
{
    UInt32 streamLength = ReadUInt32();
    str->Data.Resize(streamLength);
    for (UInt32 i = 0; i < streamLength; ++i)
    {
        str->Data[i] = ReadUByte();
    }
    str->Rewind();
}

void GFxAmpStream::WriteStream(const GFxAmpStream& str)
{
    UInt32 streamLength = static_cast<UInt32>(str.GetBufferSize());
    WriteUInt32(streamLength);
    if (streamLength > 0)
    {
        Write(str.GetBuffer(), str.GetBufferSize());
    }
}

// Append a buffer
// The buffer should already be in the proper format
// i.e. with message sizes stored before each message
void GFxAmpStream::Append(const UByte* buffer, UPInt bufferSize)
{
    Data.Append(buffer, bufferSize);
    Rewind();
}

// Data accessor
const UByte* GFxAmpStream::GetBuffer() const
{
    return Data.GetDataPtr();
}

// Buffer size
UPInt GFxAmpStream::GetBufferSize() const
{
    return Data.GetSize();
}

// Update the message size
// Assumes that there is only one message currently in the stream
void GFxAmpStream::IncreaseMessageSize(UInt32 newSize)
{
    // Make space for header
    UInt32 sizePlusHeader;
    if (Data.GetSize() == 0)
    {
        sizePlusHeader = sizeof(UInt32) + newSize;
    }
    else
    {
        sizePlusHeader = static_cast<UInt32>(Data.GetSize()) + newSize;
    }
    Data.Resize(sizePlusHeader);

    // Write the header
    UInt32 sizeLE = GByteUtil::SystemToLE(sizePlusHeader);
    memcpy(&Data[0], reinterpret_cast<UByte*>(&sizeLE), sizeof(UInt32));
}

// Clear the buffer
void GFxAmpStream::Clear()
{
    Data.Clear();
    WriteUInt32(0);
    Rewind();
}

// Start reading from the beginning
void GFxAmpStream::Rewind()
{
    readPosition = sizeof(UInt32);  // skip the header
}

// The size of the data without the header
UPInt GFxAmpStream::FirstMessageSize()
{
    SInt oldIndex = readPosition;
    readPosition = 0;
    UInt32 msgSize = ReadUInt32();
    readPosition = oldIndex;
    return msgSize;
}

// Remove the first message from the stream
// Returns true if successful
bool GFxAmpStream::PopFirstMessage()
{
    UPInt messageSize = FirstMessageSize();
    UPInt bufferLength = GetBufferSize();
    if (messageSize > bufferLength)
    {
        // incomplete message
        return false;
    }
    for (UPInt i = messageSize; i < bufferLength; ++i)
    {
        Data[i - messageSize] = Data[i];
    }
    Data.Resize(bufferLength - messageSize);
    Rewind();
    return true;
}
