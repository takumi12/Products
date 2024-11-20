/**********************************************************************

Filename    :   GASNumberUtils.h
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

#ifndef INC_GASNumberUtilS_H
#define INC_GASNumberUtilS_H

#include "GFxActionTypes.h"


// ***** Declared Classes
class GASNumberUtil
{
public:
    static GASNumber GCDECL NaN();
    static GASNumber GCDECL POSITIVE_INFINITY();
    static GASNumber GCDECL NEGATIVE_INFINITY();
    static GASNumber GCDECL MIN_VALUE();
    static GASNumber GCDECL MAX_VALUE();
    static GASNumber GCDECL POSITIVE_ZERO();
    static GASNumber GCDECL NEGATIVE_ZERO();

#ifndef GFC_NO_DOUBLE
    static inline bool GSTDCALL IsNaN(GASNumber v)
    {
        GCOMPILER_ASSERT(sizeof(GASNumber) == sizeof(UInt64));
        union
        {
            UInt64  I;
            Double  D;
        } u;
        u.D = v;
        return ((u.I & GUINT64(0x7FF0000000000000)) == GUINT64(0x7FF0000000000000) && (u.I & GUINT64(0xFFFFFFFFFFFFF)));
    }
    static inline bool GSTDCALL IsPOSITIVE_INFINITY(GASNumber v) 
    { 
        GCOMPILER_ASSERT(sizeof(GASNumber) == sizeof(UInt64));
        union
        {
            UInt64  I;
            Double  D;
        } u;
        u.D = v;
        return (u.I == GUINT64(0x7FF0000000000000));
    }
    static inline bool GSTDCALL IsNEGATIVE_INFINITY(GASNumber v) 
    { 
        GCOMPILER_ASSERT(sizeof(GASNumber) == sizeof(UInt64));
        union
        {
            UInt64  I;
            Double  D;
        } u;
        u.D = v;
        return (u.I == GUINT64(0xFFF0000000000000));
    }
    static inline bool GSTDCALL IsNaNOrInfinity(GASNumber v)
    {
        GCOMPILER_ASSERT(sizeof(GASNumber) == sizeof(UInt64));
        union
        {
            UInt64  I;
            Double  D;
        } u;
        u.D = v;
        return ((u.I & GUINT64(0x7FF0000000000000)) == GUINT64(0x7FF0000000000000));
    }
#else
    static inline bool GSTDCALL IsNaN(GASNumber v)
    {
        GCOMPILER_ASSERT(sizeof(GASNumber) == sizeof(UInt32));
        union
        {
            UInt32  I;
            Float   F;
        } u;
        u.F = v;
        return ((u.I & 0x7F800000u) == 0x7F800000u && (u.I & 0x7FFFFFu));
    }
    static inline bool GSTDCALL IsPOSITIVE_INFINITY(GASNumber v) 
    { 
        GCOMPILER_ASSERT(sizeof(GASNumber) == sizeof(UInt32));
        union
        {
            UInt32  I;
            Float   F;
        } u;
        u.F = v;
        return (u.I == 0x7F800000u);
    }
    static inline bool GSTDCALL IsNEGATIVE_INFINITY(GASNumber v) 
    { 
        GCOMPILER_ASSERT(sizeof(GASNumber) == sizeof(UInt32));
        union
        {
            UInt32  I;
            Float   F;
        } u;
        u.F = v;
        return (u.I == 0xFF800000u);
    }
    static inline bool GSTDCALL IsNaNOrInfinity(GASNumber v) 
    { 
        GCOMPILER_ASSERT(sizeof(GASNumber) == sizeof(UInt32));
        union
        {
            UInt32  I;
            Float   F;
        } u;
        u.F = v;
        return ((u.I & 0x7F800000u) == 0x7F800000u);
    }
#endif

    static bool GSTDCALL IsPOSITIVE_ZERO(GASNumber v);
    static bool GSTDCALL IsNEGATIVE_ZERO(GASNumber v);

    enum 
    {
        TOSTRING_BUF_SIZE = 64
    };
    // radixPrecision: > 0  - radix
    //                 < 0  - precision (= -radixPrecision)
    // Returns: a pointer to the first character in the destStr buffer.
    // The recommended size for the destStr buffer is TOSTRING_BUF_SIZE.
    // IMPORTANT: use returned pointer rather than the buffer itself: ToString & IntToString
    // fill buffer from the end to the beginning and DO NOT move the resulting string to the
    // beginning of the buffer; thus, the resulting string may start not from the beginning of
    // the buffer.
    static const char* GSTDCALL ToString(GASNumber value, char destStr[], size_t destStrSize, int radixPrecision = 10);
    // radix - 2, 8, 10, 16
    static const char* GSTDCALL IntToString(SInt32 value, char destStr[], size_t destStrSize, int radix);
    // radix is always 10
    static const char* GSTDCALL IntToString(SInt32 value, char destStr[], size_t destStrSize);
};

#endif

