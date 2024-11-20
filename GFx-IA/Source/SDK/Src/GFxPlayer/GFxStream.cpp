/**********************************************************************

Filename    :   GFxStream.cpp
Content     :   Byte/bit packed stream used for reading SWF files.
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   wrapper class, for loading variable-length data from a
                GFxStream, and keeping track of SWF tag boundaries.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#include "GFile.h"
#include "GFxStream.h"
#include "GFxLoader.h"
#include "GFxLog.h"
#include "GMsgFormat.h"

#include "GHeapNew.h"

#include "GFxStreamContext.h"

#include <string.h>


GFxStream::GFxStream(GFile* pinput, GMemoryHeap* pheap,
                     GFxLog *plog, GFxParseControl *pparseControl) 
    : FileName(pheap)
{
    BufferSize  = sizeof(BuiltinBuffer);
    pBuffer     = BuiltinBuffer;

    Initialize(pinput, plog, pparseControl);
}

GFxStream::GFxStream(const UByte* pbuffer, UInt bufSize, GMemoryHeap* pheap,
                     GFxLog *plog, GFxParseControl *pparseControl)
    : FileName(pheap)
{
    pBuffer     = const_cast<UByte*>(pbuffer);
    if (pBuffer)
        BufferSize  = bufSize;
    else
        BufferSize  = 0;

    Initialize(NULL, plog, pparseControl);

    DataSize    = BufferSize;
    FilePos     = BufferSize;
}

GFxStream::~GFxStream()
{
}

void    GFxStream::Initialize(GFile* pinput, GFxLog *plog, GFxParseControl *pparseControl)
{    
    pInput      = pinput;
    pLog        = plog;
    pParseControl = pparseControl;
    ParseFlags  = pparseControl ? pparseControl->GetParseFlags() : 0;
    CurrentByte = 0;
    UnusedBits  = 0;

    if (pinput)
        FileName = pinput->GetFilePath();
    else
        FileName.Clear();

    TagStackEntryCount = 0;
    TagStack[0] = 0;
    TagStack[1] = 0;

    Pos         = 0;
    DataSize    = 0;
    ResyncFile  = false;
    FilePos     = pinput ? pinput->Tell() : 0;
}


void    GFxStream::ShutDown()
{
    // Clear FileName since it contains allocation from the users heap.
    FileName.Clear();
    pInput = 0;
    pLog   = 0;
    pParseControl = 0;
}
    
// Reads a bit-packed unsigned integer from the GFxStream
// and returns it.  The given bitcount determines the
// number of bits to read.
UInt    GFxStream::ReadUInt(UInt bitcount)
{
    GASSERT(bitcount <= 32);
        
    UInt32  value      = 0;
    int     bitsNeeded = bitcount;

    while (bitsNeeded > 0)
    {
        if (UnusedBits) 
        {
            if (bitsNeeded >= UnusedBits) 
            {
                // Consume all the unused bits.
                value |= (CurrentByte << (bitsNeeded - UnusedBits));

                bitsNeeded -= UnusedBits;                
                UnusedBits = 0;
            } 
            else 
            {
                int ubits = UnusedBits - bitsNeeded;

                // Consume some of the unused bits.
                value |= (CurrentByte >> ubits);
                // mask off the bits we consumed.
                CurrentByte &= ((1 << ubits) - 1);
                
                // We're done.
                UnusedBits = (UByte)ubits;
                bitsNeeded = 0;
            }
        } 
        else 
        {
            CurrentByte = ReadU8();
            UnusedBits  = 8;
        }
    }

    GASSERT(bitsNeeded == 0);

    return value;
}


UInt    GFxStream::ReadUInt1()
{    
    UInt32  value;
   
    if (UnusedBits == 0)
    {
        CurrentByte = ReadU8();
        UnusedBits  = 7;
        value       = (CurrentByte >> 7);
        CurrentByte &= ((1 << 7) - 1);
    }
    else
    {
        UnusedBits--;
        // Consume some of the unused bits.
        value       = (CurrentByte >> UnusedBits);
        CurrentByte &= ((1 << UnusedBits) - 1);
    }
    
    return value;
}


// Reads a bit-packed little-endian signed integer
// from the GFxStream.  The given bitcount determines the
// number of bits to read.
SInt    GFxStream::ReadSInt(UInt bitcount)
{
    GASSERT(bitcount <= 32);

    SInt32  value = (SInt32) ReadUInt(bitcount);

    // Sign extend...
    if (value & (1 << (bitcount - 1)))
    {
        value |= -1 << bitcount;
    }

    return value;
}


// Makes data available in buffer. Stores zeros if not available.
bool    GFxStream::PopulateBuffer(SInt size)
{
    if (DataSize == 0)
    {
        // In case Underlying file position was changed.
        if (pInput)
        {
            FilePos    = pInput->Tell();
            ResyncFile = false;
        }
    }
    GASSERT(ResyncFile == false);

    // move data to front
    if (Pos < DataSize)
    {
        memmove(pBuffer, pBuffer + Pos, DataSize - Pos);
        DataSize = DataSize - Pos;
        Pos      = 0;        
    }
    else if (Pos >= DataSize)
    {
        GASSERT(Pos == DataSize);
        DataSize = 0;
        Pos      = 0;
    }
    bool rv;
    if (pInput)
    {
        SInt readSize = BufferSize - (SInt)DataSize;
        SInt bytes    = pInput->Read(pBuffer + DataSize, readSize);

        if (bytes < readSize)
        {
            if (bytes > 0)
            {
                DataSize += (UInt) bytes;
                FilePos  += (UInt) bytes;
            }
            // Did not read enough. Set to zeros - error case.
            memset(pBuffer + DataSize, 0, BufferSize - DataSize);

            rv = (size <= (SInt)(DataSize - Pos)); //!AB: what if 'size' is greater than BufferSize???
            if ((SInt)(DataSize - Pos) < size)
            {
                SInt extraBytes = size - (SInt)(DataSize - Pos);
                // There is an attempt to read beyond the end-of-file. To avoid
                // assertion we simulate reading of all zeros.
                GFC_DEBUG_ERROR3(1, "Read error: attempt to read %d bytes beyond the EOF, file %s, filepos %d", 
                    extraBytes, FileName.ToCStr(), (int)FilePos);
                DataSize += (UInt) extraBytes;
            }
        }
        else
        {
            DataSize += (UInt) bytes;
            FilePos  += (UInt) bytes;
            rv       = true;
        } 
    }
    else 
    {
        // there is no file set to the stream - just 
        pBuffer     = BuiltinBuffer;
        BufferSize  = sizeof(BuiltinBuffer);
        memset(pBuffer, 0, BufferSize);
        Pos         = 0;
        DataSize    = BufferSize;
        FilePos    += DataSize;
        rv          = false;
    }
    GASSERT(DataSize <= BufferSize );
    return rv;
}

bool    GFxStream::PopulateBuffer1()
{
    // Non-inline to simplify the call in ReadU8.
    return PopulateBuffer(1);
}

// Reposition the underlying file to the desired location
void    GFxStream::SyncFileStream()
{
    SInt pos = pInput->Seek(Tell());
    if (pos != -1)
    {
        Pos       = 0;
        FilePos   = (UInt) pos;
        DataSize  = 0;
    }   
}



bool   GFxStream::ReadString(GString* pstr)
{
    Align();

    char         c;
    GArray<char> buffer;
    
    while ((c = ReadU8()) != 0)    
        buffer.PushBack(c);    
    buffer.PushBack(0);

    if (buffer.GetSize() == 0)
    {
        pstr->Clear();
        return false;
    }

    pstr->AssignString(&buffer[0], buffer.GetSize()-1);
    return true;
}

bool   GFxStream::ReadStringWithLength(GString* pstr)
{
    // Use special initializer to avoid allocation copy.
    struct StringReader : public GString::InitStruct
    {
        GFxStream* pStream;
        StringReader(GFxStream* pstream) : pStream(pstream) { }
        StringReader(StringReader&) { } // avoid warning

        virtual void InitString(char* pbuffer, UPInt size) const
        {
            for (UPInt i = 0; i < size; i++)
                pbuffer[i] = pStream->ReadU8();    
        }
    };

    Align();

    int len = ReadU8();
    if (len <= 0)
    {
        pstr->Clear();
        return false;
    }

    // String data is read in from InitStrng callback.
    StringReader sreader(this);
    pstr->AssignString(sreader, len);
    return true;
}

// Reads *and GALLOC()'s* the string from the given file.
// Ownership passes to the caller; caller must delete[] the
// string when it is done with it.
char* GFxStream::ReadString(GMemoryHeap* pheap)
{
    Align();

    char         c;
    GArray<char> buffer;

    while ((c = ReadU8()) != 0)
        buffer.PushBack(c);    
    buffer.PushBack(0);

    if (!buffer.GetSize())
        return 0;

    char* retval = (char*)GHEAP_ALLOC(pheap, buffer.GetSize(), GStat_Default_Mem);
    memcpy(retval, &buffer[0], buffer.GetSize());
    return retval;
}

// Reads *and new[]'s* the string from the given file.
// Ownership passes to the caller; caller must delete[] the
// string when it is done with it.
char*   GFxStream::ReadStringWithLength(GMemoryHeap* pheap)
{
    Align();

    int len = ReadU8();
    if (len <= 0)   
        return 0;
    
    char*   buffer = (char*)GHEAP_ALLOC(pheap, len + 1, GStat_Default_Mem);
    int     i;
    for (i = 0; i < len; i++)
        buffer[i] = ReadU8();
    buffer[i] = 0;
    return buffer;
}



int   GFxStream::ReadToBuffer(UByte* pdestBuf, UInt sz)
{
    UInt bytes_read = 0;
    if (DataSize == 0)
    {
        // In case Underlying file position was changed.
        FilePos    = pInput->Tell();
        ResyncFile = false;
    }
    GASSERT(ResyncFile == false);

    // move existing data to front of destination buffer
    if (Pos < DataSize)
    {
        UInt szFromBuf = G_Min(sz, DataSize - Pos);
        memmove(pdestBuf, pBuffer + Pos, szFromBuf);
        Pos += szFromBuf;
        sz  -= szFromBuf;
        pdestBuf += szFromBuf;
        bytes_read += szFromBuf;
    }
    if (Pos >= DataSize)
    {
        DataSize = 0;
        Pos      = 0;
    }

    if (sz)
    {
        SInt bytes = pInput->Read(pdestBuf, (SInt)sz);
        bytes_read += bytes;
        FilePos  += (UInt) bytes;
        if (bytes < (SInt)sz)
        {
            // Did not read enough. Set to zeros - error case.
            memset(pdestBuf + bytes, 0, sz - bytes);
        }
    }
    return (int)bytes_read;
}



// Set the file position to the given value.
void    GFxStream::SetPosition(int pos)
{
    Align();

    // If we're in a tag, make sure we're not seeking outside the tag.
    if (TagStackEntryCount > 0)
    {
        GASSERT(pos <= GetTagEndPosition());
        // @@ check start pos somehow???
    }
            
    if ((pos >= (int)(FilePos - DataSize) && (pos < (int)FilePos)))
    {
        // If data falls within the buffer, reposition the pointer.
        // Note that if we are here we know that (DataSize != 0) because
        // otherwise the (pos < (int)FilePos) condition would fail.
        GASSERT((DataSize != 0) && (ResyncFile == false));
        Pos = UInt(pos - FilePos + DataSize);
    }
    else
    {
        if (!ResyncFile && (Tell() == pos))
        {
            // If we are already at the right location, nothing to do.
            // This may happen frequently if we have just read all data and
            // are being repositioned to the same location, so just return.
            GASSERT(FilePos == (UInt)pInput->Tell());
        }
        else if (pInput->Seek(pos) >= 0)
        {
            ResyncFile  = false;

            // MA DEBUG
            //GFC_DEBUG_MESSAGE3(1, "GFxStream::SetPosition(%d) - seek out of buffer; Tell() == %d, BuffStart = %d",
            //                      pos, Tell(), FilePos - DataSize);        
            Pos         = 0;
            DataSize    = 0;
            FilePos     = pos;             
        }
    }
}



// *** Tag handling.

// Return the tag type.
GFxTagType GFxStream::OpenTag(GFxTagInfo* pTagInfo)
{
    Align();
    int tagOffset   = Tell();
    int tagHeader   = ReadU16();
    int tagType     = tagHeader >> 6;
    int tagLength   = tagHeader & 0x3F;
    
    GASSERT(UnusedBits == 0);
    if (tagLength == 0x3F)
        tagLength = ReadS32();
    
    pTagInfo->TagOffset     = tagOffset;
    pTagInfo->TagType       = (GFxTagType)tagType;
    pTagInfo->TagLength     = tagLength;
    pTagInfo->TagDataOffset = Tell();

    if (IsVerboseParse())
        LogParse("---------------Tag type = %d, Tag length = %d, offset = %d\n", tagType, tagLength, tagOffset);
        
    // Remember where the end of the tag is, so we can
    // fast-forward past it when we're done reading it.
    GASSERT(TagStackEntryCount < Stream_TagStackSize);
    TagStack[TagStackEntryCount] = Tell() + tagLength;
    TagStackEntryCount++;

    GASSERT((TagStackEntryCount == 1) ||
            (TagStack[TagStackEntryCount-1] >= TagStack[TagStackEntryCount]) );
    
    return (GFxTagType)tagType;
}

// Simplified OpenTag - no info reported back.
GFxTagType GFxStream::OpenTag()
{
    Align();
    int tagHeader   = ReadU16();
    int tagType     = tagHeader >> 6;
    int tagLength   = tagHeader & 0x3F;

    GASSERT(UnusedBits == 0);
    if (tagLength == 0x3F)
        tagLength = ReadS32();
    
    if (IsVerboseParse())
        LogParse("---------------Tag type = %d, Tag length = %d\n", tagType, tagLength);

    // Remember where the end of the tag is, so we can
    // fast-forward past it when we're done reading it.
    GASSERT(TagStackEntryCount < Stream_TagStackSize);
        TagStack[TagStackEntryCount] = Tell() + tagLength;
    TagStackEntryCount++;

    return (GFxTagType)tagType;
}

// Seek to the end of the most-recently-opened tag.
void    GFxStream::CloseTag()
{
    GASSERT(TagStackEntryCount > 0);
    TagStackEntryCount--;
    SetPosition(TagStack[TagStackEntryCount]);
    UnusedBits = 0;
}

// Return the file position of the end of the current tag.
int GFxStream::GetTagEndPosition()
{
    // Bounds check - assumes UInt.
    GASSERT((TagStackEntryCount-1) < Stream_TagStackSize); 
    return ((TagStackEntryCount-1) < Stream_TagStackSize) ?
           TagStack[TagStackEntryCount-1] : 0;
}



// *** Reading SWF data types

// Loading functions
void    GFxStream::ReadMatrix(GRenderer::Matrix *pm)
{
    Align();
    pm->SetIdentity();

    int hasScale = ReadUInt1();
    if (hasScale)
    {
        UInt    ScaleNbits = ReadUInt(5);
        pm->M_[0][0] = ReadSInt(ScaleNbits) / 65536.0f;
        pm->M_[1][1] = ReadSInt(ScaleNbits) / 65536.0f;
    }
    int hasRotate = ReadUInt1();
    if (hasRotate)
    {
        UInt    rotateNbits = ReadUInt(5);
        pm->M_[1][0] = ReadSInt(rotateNbits) / 65536.0f;
        pm->M_[0][1] = ReadSInt(rotateNbits) / 65536.0f;
    }

    int translateNbits = ReadUInt(5);
    if (translateNbits > 0)
    {
        pm->M_[0][2] = (Float) ReadSInt(translateNbits);
        pm->M_[1][2] = (Float) ReadSInt(translateNbits);
    }
}

void    GFxStream::ReadCxformRgb(GRenderer::Cxform *pcxform)
{
    Align();
    UInt    hasAdd = ReadUInt1();
    UInt    hasMult = ReadUInt1();
    UInt    nbits = ReadUInt(4);

    if (hasMult)
    {
        // The divisor value must be 256,
        // since multiply factor 1.0 has value 0x100, not 0xFF
        pcxform->M_[0][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[1][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[2][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[3][0] = 1.0f;
    }
    else
    {
        for (UInt i = 0; i < 4; i++) { pcxform->M_[i][0] = 1.0f; }
    }
    if (hasAdd)
    {
        pcxform->M_[0][1] = (Float) ReadSInt(nbits);
        pcxform->M_[1][1] = (Float) ReadSInt(nbits);
        pcxform->M_[2][1] = (Float) ReadSInt(nbits);
        pcxform->M_[3][1] = 1.0f;
    }
    else
    {
        for (UInt i = 0; i < 4; i++) { pcxform->M_[i][1] = 0.0f; }
    }
}

void    GFxStream::ReadCxformRgba(GRenderer::Cxform *pcxform)
{
    Align();

    UInt    hasAdd = ReadUInt1();
    UInt    hasMult = ReadUInt1();
    UInt    nbits = ReadUInt(4);

    if (hasMult)
    {
        // The divisor value must be 256,
        // since multiply factor 1.0 has value 0x100, not 0xFF
        pcxform->M_[0][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[1][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[2][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[3][0] = ReadSInt(nbits) / 256.0f;
    }
    else
    {
        for (int i = 0; i < 4; i++) { pcxform->M_[i][0] = 1.0f; }
    }
    if (hasAdd)
    {
        pcxform->M_[0][1] = (Float) ReadSInt(nbits);
        pcxform->M_[1][1] = (Float) ReadSInt(nbits);
        pcxform->M_[2][1] = (Float) ReadSInt(nbits);
        pcxform->M_[3][1] = (Float) ReadSInt(nbits);
    }
    else
    {
        for (int i = 0; i < 4; i++) { pcxform->M_[i][1] = 0.0f; }
    }       
}


void    GFxStream::ReadRect(GRectF *pr)
{
    Align();
    int nbits = ReadUInt(5);
    pr->Left    = (Float) ReadSInt(nbits);
    pr->Right   = (Float) ReadSInt(nbits);
    pr->Top     = (Float) ReadSInt(nbits);
    pr->Bottom  = (Float) ReadSInt(nbits);
}


void    GFxStream::ReadPoint(GPointF *ppt)
{
    GUNUSED(ppt);
    GFC_DEBUG_ERROR(1, "GFxStream::ReadPoint not implemented");
}


// Color
void    GFxStream::ReadRgb(GColor *pc)
{
    // Read: R, G, B
    pc->SetRed(ReadU8());
    pc->SetGreen(ReadU8());
    pc->SetBlue(ReadU8());
    pc->SetAlpha(0xFF); 
}

void    GFxStream::ReadRgba(GColor *pc)
{
    // Read: RGB, A
    ReadRgb(pc);
    pc->SetAlpha(ReadU8()); 
}




// GFxLogBase impl
GFxLog* GFxStream::GetLog() const
    { return pLog; }
bool    GFxStream::IsVerboseParse() const
    { return (ParseFlags & GFxParseControl::VerboseParse) != 0; }
bool    GFxStream::IsVerboseParseShape() const
    { return (ParseFlags & GFxParseControl::VerboseParseShape) != 0; }
bool    GFxStream::IsVerboseParseMorphShape() const
    { return (ParseFlags & GFxParseControl::VerboseParseMorphShape) != 0; }
bool    GFxStream::IsVerboseParseAction() const
    { return (ParseFlags & GFxParseControl::VerboseParseAction) != 0; }


    
// *** Helper functions to log contents or required types

#ifndef GFC_NO_FXPLAYER_VERBOSE_PARSE

#define GFX_STREAM_BUFFER_SIZE  512

void    GFxStream::LogParseClass(GColor color)
{
    char buff[GFX_STREAM_BUFFER_SIZE];
    color.Format(buff);
    LogParse("%s", buff);
}

void    GFxStream::LogParseClass(const GRectF &rc)
{
    char buff[GFX_STREAM_BUFFER_SIZE];


    G_sprintf(buff, GFX_STREAM_BUFFER_SIZE,
        "xmin = %g, ymin = %g, xmax = %g, ymax = %g\n",
        TwipsToPixels(rc.Left),
        TwipsToPixels(rc.Top),
        TwipsToPixels(rc.Right),
        TwipsToPixels(rc.Bottom));

    //G_Format(
    //    GStringDataPtr(buff, sizeof(buff)),
    //    "xmin = {0:g}, ymin = {1:g}, xmax = {2:g}, ymax = {3:g}\n",
    //    TwipsToPixels(rc.Left),
    //    TwipsToPixels(rc.Top),
    //    TwipsToPixels(rc.Right),
    //    TwipsToPixels(rc.Bottom)
    //    );

    LogParse("%s", buff);
}

void    GFxStream::LogParseClass(const GRenderer::Cxform &cxform)
{
    char buff[GFX_STREAM_BUFFER_SIZE];
    cxform.Format(buff);
    LogParse("%s", buff);
}
void    GFxStream::LogParseClass(const GRenderer::Matrix &matrix)
{
    char buff[GFX_STREAM_BUFFER_SIZE];
    matrix.Format(buff);
    LogParse("%s", buff);
}


// Log the contents of the current tag, in hex.
void    GFxStream::LogTagBytes()
{
    static const int    ROW_BYTES = 16;

    char    rowBuf[ROW_BYTES];
    int     rowCount = 0;

    while(Tell() < GetTagEndPosition())
    {
        UInt    c = ReadU8();
        LogParse("%02X", c);

        if (c < 32) c = '.';
        if (c > 127) c = '.';
        rowBuf[rowCount] = (UByte)c;
        
        rowCount++;
        if (rowCount >= ROW_BYTES)
        {
            LogParse("    ");
            for (int i = 0; i < ROW_BYTES; i++)
            {
                LogParse("%c", rowBuf[i]);
            }

            LogParse("\n");
            rowCount = 0;
        }
        else
        {
            LogParse(" ");
        }
    }

    if (rowCount > 0)
    {
        LogParse("\n");
    }
}
#endif //GFC_NO_FXPLAYER_VERBOSE_PARSE


UInt GFxStreamContext::ReadUInt(UInt bitcount)
{
    static const UInt8 bytesInBits[] = { 0,1,1,1,1,1,1,1,1, 
        2,2,2,2,2,2,2,2,
        3,3,3,3,3,3,3,3,
        4,4,4,4,4,4,4,4 };
    UInt v;
    UInt mask = ((1 << (8 - CurBitIndex)) - 1);
    UInt rshift;
    UInt lastBitIndex = CurBitIndex + bitcount;
    GASSERT(bitcount < sizeof(bytesInBits)/sizeof(bytesInBits[0]));
    switch(bytesInBits[bitcount])
    {
    case 0: return 0;
    case 1: // 1 octet 1..8 bits
        if (lastBitIndex <= 8) // whole value fits in 1 bytes
        {
            v = pData[CurByteIndex] & mask;
            rshift = (8 - lastBitIndex);
        }
        else
        {
            // assemble a preliminary value using 2 bytes
            // and init rshift accordingly
            v = (UInt(pData[CurByteIndex] & mask) << 8) | 
                UInt(pData[CurByteIndex + 1]);
            ++CurByteIndex;
            rshift = (16 - lastBitIndex);
        }
        break;
    case 2: // 2 octets 9..16 bits
        if (lastBitIndex <= 16) // whole value fits in 2 bytes
        {
            v = ((UInt(pData[CurByteIndex]) & mask) << 8) | UInt(pData[CurByteIndex + 1]);
            ++CurByteIndex;
            rshift = (16 - lastBitIndex);
        }
        else
        {
            // assemble a preliminary value using 3 bytes
            // and init rshift accordingly
            v = (UInt(pData[CurByteIndex] & mask) << 16) | 
                (UInt(pData[CurByteIndex + 1]) << 8) | 
                UInt(pData[CurByteIndex + 2]);
            CurByteIndex += 2;
            rshift = (24 - lastBitIndex);
        }
        break;
    case 3: // 3 octets 17..24 bits
        if (lastBitIndex <= 24) // whole value fits in 3 bytes 
        {
            v = (UInt(pData[CurByteIndex] & mask) << 16) | 
                (UInt(pData[CurByteIndex + 1]) << 8) | 
                UInt(pData[CurByteIndex + 2]);
            CurByteIndex += 2;
            rshift = (24 - lastBitIndex);
        }
        else
        {
            // assemble a preliminary value using 4 bytes
            // and init rshift accordingly
            v = (UInt(pData[CurByteIndex] & mask) << 24) | 
                (UInt(pData[CurByteIndex + 1]) << 16) | 
                (UInt(pData[CurByteIndex + 2]) << 8) | 
                UInt(pData[CurByteIndex + 3]);
            CurByteIndex += 3;
            rshift = (32 - lastBitIndex);
        }
        break;
    case 4: // 4 octets 25..32 bits
        if (lastBitIndex <= 32) // whole value fits in 4 bytes 
        {
            v = (UInt(pData[CurByteIndex] & mask) << 24) | 
                (UInt(pData[CurByteIndex + 1]) << 16) | 
                (UInt(pData[CurByteIndex + 2]) << 8) | 
                UInt(pData[CurByteIndex + 3]);
            CurByteIndex += 3;
            rshift = (32 - lastBitIndex);
        }
        else
        {
            // assemble a preliminary value using 4 bytes
            v = (UInt(pData[CurByteIndex] & mask) << 24) | 
                (UInt(pData[CurByteIndex + 1]) << 16) | 
                (UInt(pData[CurByteIndex + 2]) << 8) | 
                UInt(pData[CurByteIndex + 3]);
            CurByteIndex += 4;

            // need to read 5th byte and take "extraBits" bits from it
            UInt extraBits = (lastBitIndex - 32);
            v <<= extraBits;
            v |= UInt(pData[CurByteIndex]) >> (8 - extraBits);

            // value is already shifted correctly.
            // just modify CurBitIndex and return
            CurBitIndex = extraBits;
            GASSERT(CurBitIndex < 8);
            return v;
        }
        break;
    default: GASSERT(0); v = 0; rshift = 0; // only up to 32-bits values supported
    }
    if (rshift)
    {
        v >>= rshift;
        CurBitIndex = 8 - rshift;
    }
    else
    {
        ++CurByteIndex;
        CurBitIndex = 0;
    }
    GASSERT(CurBitIndex < 8);
    return v;
}

void GFxStreamContext::ReadMatrix(GRenderer::Matrix *pm)
{
    Align();
    pm->SetIdentity();

    int hasScale = ReadUInt1();
    if (hasScale)
    {
        UInt    ScaleNbits = ReadUInt(5);
        pm->M_[0][0] = ReadSInt(ScaleNbits) / 65536.0f;
        pm->M_[1][1] = ReadSInt(ScaleNbits) / 65536.0f;
    }
    int hasRotate = ReadUInt1();
    if (hasRotate)
    {
        UInt    rotateNbits = ReadUInt(5);
        pm->M_[1][0] = ReadSInt(rotateNbits) / 65536.0f;
        pm->M_[0][1] = ReadSInt(rotateNbits) / 65536.0f;
    }

    int translateNbits = ReadUInt(5);
    if (translateNbits > 0)
    {
        pm->M_[0][2] = (Float) ReadSInt(translateNbits);
        pm->M_[1][2] = (Float) ReadSInt(translateNbits);
    }
}

void GFxStreamContext::ReadCxformRgb(GRenderer::Cxform *pcxform)
{
    Align();
    UInt    hasAdd = ReadUInt1();
    UInt    hasMult = ReadUInt1();
    UInt    nbits = ReadUInt(4);

    if (hasMult)
    {
        // The divisor value must be 256,
        // since multiply factor 1.0 has value 0x100, not 0xFF
        pcxform->M_[0][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[1][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[2][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[3][0] = 1.0f;
    }
    else
    {
        for (UInt i = 0; i < 4; i++) { pcxform->M_[i][0] = 1.0f; }
    }
    if (hasAdd)
    {
        pcxform->M_[0][1] = (Float) ReadSInt(nbits);
        pcxform->M_[1][1] = (Float) ReadSInt(nbits);
        pcxform->M_[2][1] = (Float) ReadSInt(nbits);
        pcxform->M_[3][1] = 1.0f;
    }
    else
    {
        for (UInt i = 0; i < 4; i++) { pcxform->M_[i][1] = 0.0f; }
    }
}

void GFxStreamContext::ReadCxformRgba(GRenderer::Cxform *pcxform)
{
    Align();

    UInt    hasAdd = ReadUInt1();
    UInt    hasMult = ReadUInt1();
    UInt    nbits = ReadUInt(4);

    if (hasMult)
    {
        // The divisor value must be 256,
        // since multiply factor 1.0 has value 0x100, not 0xFF
        pcxform->M_[0][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[1][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[2][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[3][0] = ReadSInt(nbits) / 256.0f;
    }
    else
    {
        for (int i = 0; i < 4; i++) { pcxform->M_[i][0] = 1.0f; }
    }
    if (hasAdd)
    {
        pcxform->M_[0][1] = (Float) ReadSInt(nbits);
        pcxform->M_[1][1] = (Float) ReadSInt(nbits);
        pcxform->M_[2][1] = (Float) ReadSInt(nbits);
        pcxform->M_[3][1] = (Float) ReadSInt(nbits);
    }
    else
    {
        for (int i = 0; i < 4; i++) { pcxform->M_[i][1] = 0.0f; }
    }       
}


