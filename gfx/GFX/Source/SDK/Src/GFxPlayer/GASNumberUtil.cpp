/**********************************************************************

Filename    :   GASNumberUtils.cpp
Content     :   Utility number functinality
Created     :   Sep 21, 2009
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GASNumberUtil.h"

#include <stdio.h>
#include <stdlib.h>


#if GFC_BYTE_ORDER == GFC_LITTLE_ENDIAN

#ifdef GFC_NO_DOUBLE
static const UByte GFxNaN_Bytes[]               = {0x00, 0x00, 0xC0, 0xFF};
static const UByte GFxQNaN_Bytes[]              = {0x00, 0x00, 0xC0, 0x7F};
static const UByte GFxPOSITIVE_INFINITY_Bytes[] = {0x00, 0x00, 0x80, 0x7F};
static const UByte GFxNEGATIVE_INFINITY_Bytes[] = {0x00, 0x00, 0x80, 0xFF};
static const UByte GFxMIN_VALUE_Bytes[]         = {0x00, 0x00, 0x80, 0x00};
static const UByte GFxMAX_VALUE_Bytes[]         = {0xFF, 0xFF, 0x7F, 0x7F};
static const UByte GFxNEGATIVE_ZERO_Bytes[]     = {0x00, 0x00, 0x00, 0x80};
static const UByte GFxPOSITIVE_ZERO_Bytes[]     = {0x00, 0x00, 0x00, 0x00};
#else
static const UByte GFxNaN_Bytes[]               = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF};
static const UByte GFxQNaN_Bytes[]              = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x7F};
static const UByte GFxPOSITIVE_INFINITY_Bytes[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x7F};
static const UByte GFxNEGATIVE_INFINITY_Bytes[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF};
static const UByte GFxMIN_VALUE_Bytes[]         = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00};
static const UByte GFxMAX_VALUE_Bytes[]         = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F};
static const UByte GFxNEGATIVE_ZERO_Bytes[]     = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80};
static const UByte GFxPOSITIVE_ZERO_Bytes[]     = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif

#else

#ifdef GFC_NO_DOUBLE
static const UByte GFxNaN_Bytes[]               = {0xFF, 0xC0, 0x00, 0x00};
static const UByte GFxQNaN_Bytes[]              = {0x7F, 0xC0, 0x00, 0x00};
static const UByte GFxPOSITIVE_INFINITY_Bytes[] = {0x7F, 0x80, 0x00, 0x00};
static const UByte GFxNEGATIVE_INFINITY_Bytes[] = {0xFF, 0x80, 0x00, 0x00};
static const UByte GFxMIN_VALUE_Bytes[]         = {0x00, 0x80, 0x00, 0x00};
static const UByte GFxMAX_VALUE_Bytes[]         = {0x7F, 0x7F, 0xFF, 0xFF};
static const UByte GFxNEGATIVE_ZERO_Bytes[]     = {0x80, 0x00, 0x00, 0x00};
static const UByte GFxPOSITIVE_ZERO_Bytes[]     = {0x00, 0x00, 0x00, 0x00};
#else
static const UByte GFxNaN_Bytes[]               = {0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UByte GFxQNaN_Bytes[]              = {0x7F, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UByte GFxPOSITIVE_INFINITY_Bytes[] = {0x7F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UByte GFxNEGATIVE_INFINITY_Bytes[] = {0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UByte GFxMIN_VALUE_Bytes[]         = {0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UByte GFxMAX_VALUE_Bytes[]         = {0x7F, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const UByte GFxNEGATIVE_ZERO_Bytes[]     = {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UByte GFxPOSITIVE_ZERO_Bytes[]     = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif

#endif

GASNumber GASNumberUtil::NaN()               { GASNumber v; memcpy(&v, GFxNaN_Bytes              , sizeof(v)); return v; }
GASNumber GASNumberUtil::POSITIVE_INFINITY() { GASNumber v; memcpy(&v, GFxPOSITIVE_INFINITY_Bytes, sizeof(v)); return v; }
GASNumber GASNumberUtil::NEGATIVE_INFINITY() { GASNumber v; memcpy(&v, GFxNEGATIVE_INFINITY_Bytes, sizeof(v)); return v; }
GASNumber GASNumberUtil::POSITIVE_ZERO()     { GASNumber v; memcpy(&v, GFxPOSITIVE_ZERO_Bytes    , sizeof(v)); return v; }
GASNumber GASNumberUtil::NEGATIVE_ZERO()     { GASNumber v; memcpy(&v, GFxNEGATIVE_ZERO_Bytes    , sizeof(v)); return v; }
GASNumber GASNumberUtil::MIN_VALUE()         { GASNumber v; memcpy(&v, GFxMIN_VALUE_Bytes        , sizeof(v)); return v; }
GASNumber GASNumberUtil::MAX_VALUE()         { GASNumber v; memcpy(&v, GFxMAX_VALUE_Bytes        , sizeof(v)); return v; }

bool GASNumberUtil::IsPOSITIVE_ZERO(GASNumber v)
{
    return (memcmp(&v, GFxPOSITIVE_ZERO_Bytes,  sizeof(v)) == 0);
}

bool GASNumberUtil::IsNEGATIVE_ZERO(GASNumber v)
{
    return (memcmp(&v, GFxNEGATIVE_ZERO_Bytes,  sizeof(v)) == 0);
}

// radixPrecision: > 10 - radix
//                 < 0  - precision (= -radixPrecision)
const char* GASNumberUtil::ToString(GASNumber value, char destStr[], size_t destStrSize, int radix)
{
    GASSERT(destStrSize > 0);

#if 1
#ifndef GFC_NO_DOUBLE
    // MA: not sure about precision, but "%.14g" 
    // makes test_rotation.swf display too much rounding            
    static const char* const fmt[] = { 
        "%.1g", "%.2g", "%.3g", "%.4g", 
        "%.5g", "%.6g", "%.7g", "%.8g", 
        "%.9g", "%.10g", "%.11g", "%.12g", "%.13g", "%.14g" };
#else  //GFC_NO_DOUBLE
    static const char* const fmt[] = { 
        "%.1f", "%.2f", "%.3f", "%.4f", 
        "%.5f", "%.6f", "%.7f", "%.8f", 
        "%.9f", "%.10f", "%.11f", "%.12f", "%.13f", "%.14f" };
#endif //GFC_NO_DOUBLE
    const char* fmtStr = fmt[14 -1];

    if(radix <= 0)
    {
        fmtStr = fmt[((radix >= -14) ? -radix : 14) - 1];
        radix = 10;
    }

    if (GASNumberUtil::IsNaNOrInfinity(value))
    {
        if(GASNumberUtil::IsNaN(value))
        {
            G_strcpy(destStr, destStrSize, "NaN");
        }
        else if(GASNumberUtil::IsPOSITIVE_INFINITY(value))
        {
            G_strcpy(destStr, destStrSize, "Infinity");
        }
        else if(GASNumberUtil::IsNEGATIVE_INFINITY(value))
        {
            G_strcpy(destStr, destStrSize, "-Infinity");
        }
    }
    else
    {
        if (radix == 10) 
        {
            SInt32 ival = (SInt32)value;
            if ((GASNumber)ival == value)
                return IntToString(ival, destStr, destStrSize);
            G_sprintf(destStr, destStrSize, fmtStr, (GASNumber)value);

#ifndef GFC_OS_WINCE
            // Get rid of a possible comma ...
            for (char* s = destStr; *s != 0; ++s)
            {
                if (*s == ',' || *s == '.')
                {
                    *s = '.';
                    break;
                }
            }
#endif
        }
        else
            return IntToString((int)value, destStr, destStrSize, radix);
    }

#else
 
    // MA: not sure about precision, but "%.14g" 
    // makes test_rotation.swf display too much rounding            
    UInt precision = 14;

    if(radix <= 0)
    {
        precision = ((radix >= -14) ? -radix : 14);
        radix = 10;
    }

    if(GASNumberUtil::IsNaN(value))
    {
        destStr = "NaN";
    }
    else if(GASNumberUtil::IsPOSITIVE_INFINITY(value))
    {
        destStr = "Infinity";
    }
    else if(GASNumberUtil::IsNEGATIVE_INFINITY(value))
    {
        destStr = "-Infinity";
    }
    else
    {
        switch (radix) {
            case 10:
                {
                    //SInt32 ival = (SInt32)value;
                    //if ((double)ival == value)
                    //    return IntToString(ival, destStr, destStrSize);

                    GDoubleFormatter d((double)value);

                    d.SetPrecision(precision).SetType(GDoubleFormatter::FmtSignificant).Convert();
                    GStringDataPtr result = d.GetResult();

                    size_t size = G_Min(destStrSize, result.GetSize());
                    memcpy(destStr, result.ToCStr(), size - 1);
                    destStr[size] = '\0';
                }
                break;
            case 16:  
            case 8:
            case 2:
                {
                    GLongFormatter l((int)value);

                    l.SetBase(radix).Convert();
                    GStringDataPtr result = l.GetResult();

                    size_t size = G_Min(destStrSize, result.GetSize());
                    memcpy(destStr, result.ToCStr(), size - 1);
                    destStr[size] = '\0';
                }
        }
    }

#endif

    return destStr;
}

// radixPrecision: > 10 - radix
//                 < 0  - precision (= -radixPrecision)
const char* GASNumberUtil::IntToString(SInt32 value, char destStr[], size_t destStrSize, int radix)
{
    GASSERT(destStrSize > 0);
    size_t      bufSize = destStrSize - 1;
    char*       pbuf  = &destStr[bufSize];
    *pbuf = '\0';

    switch (radix) {
        case 16:  
            {
                unsigned    ival = (unsigned)value;
                for (unsigned i = 0; i < bufSize; i++) 
                {
                    pbuf--;
                    UByte c = (UByte)(ival & 0xF);
                    *pbuf = (c <= 9) ? c + '0' : c - 10 + 'a';
                    ival >>= 4;
                    if (ival == 0)
                        break;
                }
            }
            break;
        case 8:
            {
                unsigned    ival = (unsigned)value;
                for (unsigned i = 0; i < bufSize; i++) 
                {
                    pbuf--;
                    UByte c = (UByte)(ival & 0x7);
                    *pbuf = c + '0';
                    ival >>= 3;
                    if (ival == 0)
                        break;
                }
            }
            break;
        case 2:
            {
                unsigned    ival = (unsigned)value;
                unsigned mask = 1;
                char* plastSignificantBit = 0;
                for (unsigned i = 0; i < sizeof (int)*8 && i < bufSize; i++) 
                {
                    pbuf--;
                    if (ival & mask) {
                        *pbuf = '1';
                        plastSignificantBit = pbuf;
                    }
                    else {
                        *pbuf = '0';
                    }
                    mask <<= 1;
                    if (mask == 0) mask = 1;
                }
                if (plastSignificantBit == 0)
                    pbuf = &destStr[destStrSize - 2];
                else
                    pbuf = plastSignificantBit;
            }
            break;
        default:
            return IntToString(value, destStr, destStrSize);
    }
    return pbuf;
}

const char* GASNumberUtil::IntToString(SInt32 value, char destStr[], size_t destStrSize)
{
    GASSERT(destStrSize > 0);
    size_t      bufSize = destStrSize - 1;
    char*       pbuf  = &destStr[bufSize];
    *pbuf = '\0';

    unsigned i, absValue = G_Abs(value);
    for (i = 0; i < bufSize; i++) 
    {
        pbuf--;
        UByte c = (UByte)(absValue % 10);
        *pbuf = c + '0';
        absValue /= 10;
        if (absValue == 0)
            break;
    }
    if (i < bufSize && value < 0)
        *(--pbuf) = '-';
    return pbuf;
}

