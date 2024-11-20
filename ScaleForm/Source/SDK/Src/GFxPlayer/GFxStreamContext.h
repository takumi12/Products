/**********************************************************************

Filename    :   GFxStreamContext.h
Content     :   
Created     :   
Authors     :   Artem Bolgar, Prasad Silva
Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFxStreamContext_H
#define INC_GFxStreamContext_H

#include <GTypes.h>
#include <GRenderer.h>

struct GFxStreamContext
{
    const UByte*    pData;
    UPInt           DataSize;
    UPInt           CurByteIndex;
    UInt            CurBitIndex;

    GFxStreamContext(const UByte* pdata, UPInt sz = GFC_MAX_UPINT) : 
    pData(pdata), DataSize(sz), CurByteIndex(0), CurBitIndex(0) {}

    // Primitive types
    UInt32      ReadU32();
    UInt16      ReadU16();
    UInt8       ReadU8();
    SInt8       ReadS8();
    UInt        ReadUInt4();
    UInt        ReadUInt5();
    UInt        ReadUInt(UInt bitcount);                    // Defn in GFxStream.cpp
    SInt        ReadSInt(UInt bitcount);
    bool        ReadUInt1();
    Float       ReadFloat();

    // Complex types
    void        ReadRgb(GColor *pc);
    void        ReadRgba(GColor *pc);
    void        ReadMatrix(GRenderer::Matrix *pm);          // Defn in GFxStream.cpp
    void        ReadCxformRgb(GRenderer::Cxform *pcxform);  // "
    void        ReadCxformRgba(GRenderer::Cxform *pcxform); // "

    // Utility
    void        Align();
    void        Skip(UInt32 d);

};

GINLINE void GFxStreamContext::Align()
{
    if (CurBitIndex != 0)
        ++CurByteIndex;
    CurBitIndex = 0;
}

GINLINE UInt32 GFxStreamContext::ReadU32()
{
    Align();

    GASSERT(pData && CurByteIndex + sizeof(UInt32) <= DataSize);
    UInt32 v = (UInt32(pData[CurByteIndex]))       | (UInt32(pData[CurByteIndex+1])<<8) |
        (UInt32(pData[CurByteIndex+2])<<16) | (UInt32(pData[CurByteIndex+3])<<24);
    CurByteIndex += sizeof(UInt32);
    return v;
}

GINLINE Float GFxStreamContext::ReadFloat()
{
    Align();

    GASSERT(pData && CurByteIndex + sizeof(UInt32) <= DataSize);
    union
    {
        UInt32 v;
        Float  f;
    };
    v = (UInt32(pData[CurByteIndex]))       | (UInt32(pData[CurByteIndex+1])<<8) |
        (UInt32(pData[CurByteIndex+2])<<16) | (UInt32(pData[CurByteIndex+3])<<24);
    CurByteIndex += sizeof(UInt32);
    return f;
}

GINLINE UInt16 GFxStreamContext::ReadU16()
{
    Align();

    GASSERT(pData && CurByteIndex + sizeof(UInt16) <= DataSize);
    UInt16 v = (UInt16((pData[CurByteIndex]) | (UInt32(pData[CurByteIndex+1])<<8)));
    CurByteIndex += sizeof(UInt16);
    return v;
}

GINLINE UInt8 GFxStreamContext::ReadU8()
{
    Align();

    GASSERT(pData && CurByteIndex + 1 <= DataSize);
    return pData[CurByteIndex++];
}

GINLINE SInt8 GFxStreamContext::ReadS8()
{
    return (SInt8) ReadU8();
}

GINLINE UInt GFxStreamContext::ReadUInt4()
{
    GASSERT(pData && CurByteIndex < DataSize);
    UInt v;
    switch(CurBitIndex)
    {
    case 0:
        v = ((pData[CurByteIndex] & 0xF0) >> 4);
        CurBitIndex += 4;
        break;
    case 1:
        v = ((pData[CurByteIndex] & 0x78) >> 3);
        CurBitIndex += 4;
        break;
    case 2:
        v = ((pData[CurByteIndex] & 0x3C) >> 2);
        CurBitIndex += 4;
        break;
    case 3:
        v = ((pData[CurByteIndex] & 0x1E) >> 1);
        CurBitIndex += 4;
        break;
    case 4:
        v = (pData[CurByteIndex] & 0x0F);
        CurBitIndex = 0;
        ++CurByteIndex;
        break;
    case 5:
        GASSERT(CurByteIndex + 1 < DataSize);
        v = ((pData[CurByteIndex] & 0x07) << 1) | (pData[CurByteIndex + 1] >> 7);
        CurBitIndex = 1;
        ++CurByteIndex;
        break;
    case 6:
        GASSERT(CurByteIndex + 1 < DataSize);
        v = ((pData[CurByteIndex] & 0x03) << 2) | (pData[CurByteIndex + 1] >> 6);
        CurBitIndex = 2;
        ++CurByteIndex;
        break;
    case 7:
        GASSERT(CurByteIndex + 1 < DataSize);
        v = ((pData[CurByteIndex] & 0x01) << 3) | (pData[CurByteIndex + 1] >> 5);
        CurBitIndex = 3;
        ++CurByteIndex;
        break;
    default: GASSERT(0); v = 0;
    }
    return v;
}

GINLINE UInt GFxStreamContext::ReadUInt5()
{
    GASSERT(pData && CurByteIndex < DataSize);
    UInt v;
    switch(CurBitIndex)
    {
    case 0:
        v = ((pData[CurByteIndex] & 0xF8) >> 3);
        CurBitIndex += 5;
        break;
    case 1:
        v = ((pData[CurByteIndex] & 0x7C) >> 2);
        CurBitIndex += 5;
        break;
    case 2:
        v = ((pData[CurByteIndex] & 0x3E) >> 1);
        CurBitIndex += 5;
        break;
    case 3:
        v = (pData[CurByteIndex] & 0x1F);
        CurBitIndex = 0;
        ++CurByteIndex;
        break;
    case 4:
        GASSERT(CurByteIndex + 1 < DataSize);
        v = ((pData[CurByteIndex] & 0x0F) << 1) | (pData[CurByteIndex + 1] >> 7);
        CurBitIndex = 1;
        ++CurByteIndex;
        break;
    case 5:
        GASSERT(CurByteIndex + 1 < DataSize);
        v = ((pData[CurByteIndex] & 0x07) << 2) | (pData[CurByteIndex + 1] >> 6);
        CurBitIndex = 2;
        ++CurByteIndex;
        break;
    case 6:
        GASSERT(CurByteIndex + 1 < DataSize);
        v = ((pData[CurByteIndex] & 0x03) << 3) | (pData[CurByteIndex + 1] >> 5);
        CurBitIndex = 3;
        ++CurByteIndex;
        break;
    case 7:
        GASSERT(CurByteIndex + 1 < DataSize);
        v = ((pData[CurByteIndex] & 0x01) << 4) | (pData[CurByteIndex + 1] >> 4);
        CurBitIndex = 4;
        ++CurByteIndex;
        break;
    default: GASSERT(0); v = 0;
    }
    return v;
}

GINLINE SInt GFxStreamContext::ReadSInt(UInt bitcount)
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

GINLINE bool GFxStreamContext::ReadUInt1()
{
    // Seems like the following code is faster than the commented one,
    // at least on PC....
    UInt v = pData[CurByteIndex] & (1 << (7 - CurBitIndex));
    ++CurBitIndex;
    if (CurBitIndex >= 8) 
    {
        CurBitIndex = 0;
        ++CurByteIndex;
    }
    //UInt v;
    //switch(CurBitIndex)
    //{
    //case 0: v = pData[CurByteIndex] & 0x80; ++CurBitIndex; break;
    //case 1: v = pData[CurByteIndex] & 0x40; ++CurBitIndex; break;
    //case 2: v = pData[CurByteIndex] & 0x20; ++CurBitIndex; break;
    //case 3: v = pData[CurByteIndex] & 0x10; ++CurBitIndex; break;
    //case 4: v = pData[CurByteIndex] & 0x08; ++CurBitIndex; break;
    //case 5: v = pData[CurByteIndex] & 0x04; ++CurBitIndex; break;
    //case 6: v = pData[CurByteIndex] & 0x02; ++CurBitIndex; break;
    //case 7: v = pData[CurByteIndex] & 0x01; CurBitIndex = 0; ++CurByteIndex; break;
    //default: GASSERT(0); v = 0;
    //}
    return v != 0;
}


GINLINE void GFxStreamContext::Skip(UInt32 d)
{
    Align();

    GASSERT(CurByteIndex + d <= DataSize);
    CurByteIndex += d;
}


GINLINE void GFxStreamContext::ReadRgb(GColor *pc)
{
    // Read: R, G, B
    pc->SetRed(ReadU8());
    pc->SetGreen(ReadU8());
    pc->SetBlue(ReadU8());
    pc->SetAlpha(0xFF); 
}

GINLINE void GFxStreamContext::ReadRgba(GColor *pc)
{
    // Read: RGB, A
    ReadRgb(pc);
    pc->SetAlpha(ReadU8()); 
}

#endif // INC_GFxStreamContext_H
