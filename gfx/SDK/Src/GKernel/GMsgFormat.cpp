/**********************************************************************

Filename    :   GMsgFormat.cpp 
Content     :   Formatting of strings
Created     :   January 26, 2009
Authors     :   Sergey Sikorskiy

Notes       :   

History     :   

Copyright   :   (c) 2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GMsgFormat.h"

#include <locale.h>
#include <math.h>
#include <float.h>

////////////////////////////////////////////////////////////////////////////////
// Disable warnings.
#if defined(GFC_CC_MSVC)
// Disable "unreferenced local function has been removed" warning
# pragma warning(disable : 4505)
#endif

////////////////////////////////////////////////////////////////////////////////
#define __countof(a)          int(sizeof(a) / sizeof(a[0]))

////////////////////////////////////////////////////////////////////////////////
// Check whether the whole string containts only spaces ...
bool IsSpace(GStringDataPtr str);

////////////////////////////////////////////////////////////////////////////////
UInt GFmtResource::Reflect(const TAttrs*& attrs) const
{
    attrs = NULL;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
class GSwitchFormatter : public GFormatter
{
public:
    class ValueType
    {
    public:
        typedef SInt Type;

    public:
        ValueType() : Value(0) {}
        ValueType(SInt value) : Value(value) {}

        SInt GetValue() const { return Value; }
        void SetValue(SInt value) { Value = value; }

        operator SInt () const { return GetValue(); }
        ValueType& operator = (SInt value) { SetValue(value); return *this; }

    private:
        SInt    Value;
    };

public:
    GSwitchFormatter(GMsgFormat& f, const ValueType& v);

public:
    virtual void            Parse(const GStringDataPtr& str);
    virtual void            Convert();
    virtual GStringDataPtr  GetResult() const;
    virtual UPInt           GetSize() const;

private:
    GSwitchFormatter& operator = (const GSwitchFormatter&);

private:
    SInt                        Value;
    GHash<SInt, GStringDataPtr> StringSet;

    GStringDataPtr              StrValue;
    GStringDataPtr              DefaultStrValue;
};

GSwitchFormatter::GSwitchFormatter(GMsgFormat& f, const ValueType& v)
: GFormatter(f), Value(v)
{
}

void GSwitchFormatter::Parse(const GStringDataPtr& str)
{
    GStringDataPtr tmpStr = str;
    GStringDataPtr curStr;
    enum State {stNumber, stStr};
    State state = stNumber;
    SInt valueNumber = 0;

    // Read number.
    do 
    {
        curStr = tmpStr.GetNextToken(); 
        tmpStr.TrimLeft(curStr.GetSize() + 1);

        if (state == stNumber)
        {
            const char* s = curStr.ToCStr();
            if (!curStr.IsEmpty() && s && isdigit(s[0]))
            {
                valueNumber = atoi(s);
                state = stStr;
            } else
            {
                // This is not a number. Let's make it a default value.
                DefaultStrValue = curStr;
                break;
            }
        } else
        {
            // state == stStr
            StringSet.Add(valueNumber, curStr);
            state = stNumber;
        }
    } while (!tmpStr.IsEmpty());
}

void GSwitchFormatter::Convert()
{
    if (Converted())
    {
        return;
    }

    if (!StringSet.Get(Value, &StrValue))
    {
        StrValue = DefaultStrValue;
    }

    SetConverted();
}

GStringDataPtr GSwitchFormatter::GetResult() const
{
    return StrValue;
}

UPInt GSwitchFormatter::GetSize() const
{
    return StrValue.GetSize();
}

////////////////////////////////////////////////////////////////////////////////
/*
GFormatter::GFormatter()
: pParentFmt(NULL)
, IsConverted(false)
{
}

GFormatter::GFormatter(GMsgFormat& f)
: pParentFmt(&f)
, IsConverted(false)
{
}
*/

GFormatter::~GFormatter() 
{
}

void GFormatter::Parse(const GStringDataPtr& /*str*/)
{
}

GFormatter::requirements_t GFormatter::GetRequirements() const
{
    return rtNone;
}

void GFormatter::SetPrevStr(const GStringDataPtr& /*ptr*/)
{
}

GStringDataPtr GFormatter::GetPrevStr() const
{
    return GStringDataPtr();
}

void GFormatter::SetNextStr(const GStringDataPtr& /*ptr*/)
{
}

GStringDataPtr GFormatter::GetNextStr() const
{
    return GStringDataPtr();
}

GFormatter::ParentRef GFormatter::GetParentRef() const
{
    GASSERT(false);
    return prNone;
}

unsigned char GFormatter::GetParentPos() const
{
    GASSERT(false);
    return 0;
}

void GFormatter::SetParent(const GFmtResource&)
{
    GASSERT(false);
}

void GFormatter::SetParentRef(ParentRef)
{
    GASSERT(false);
}

void GFormatter::SetParentPos(unsigned char)
{
    GASSERT(false);
}

////////////////////////////////////////////////////////////////////////////////
GStrFormatter::GStrFormatter(const char* v)
: Value(v)
{
}

GStrFormatter::GStrFormatter(const GStringDataPtr& v)
: Value(v)
{
}

GStrFormatter::GStrFormatter(const GString& v)
: Value(v)
{
}

GStrFormatter::GStrFormatter(GMsgFormat& f, const char* v)
: GFormatter(f)
, Value(v)
{
}

GStrFormatter::GStrFormatter(GMsgFormat& f, const GStringDataPtr& v)
: GFormatter(f)
, Value(v)
{
}

GStrFormatter::GStrFormatter(GMsgFormat& f, const GString& v)
: GFormatter(f)
, Value(v)
{
}

void GStrFormatter::Parse(const GStringDataPtr& str)
{
    GStringDataPtr token = str.GetNextToken(); 
    GFormatter* impl_ptr = NULL;

    // Create implementation ...
    if (HasParent() && GetParent().GetLocaleProvider())
    {
        GFormatterFactory::Args args(GetParent(), token, GResourceFormatter::ValueType(Value.ToCStr()));

        impl_ptr = GetParent().GetLocaleProvider()->MakeFormatter(args);
    }

    GASSERT(impl_ptr);
    if (impl_ptr)
    {
        // Calculate string, which should be passed to implemtation ...
        GStringDataPtr impl_param_str = str.GetTrimLeft(token.GetSize() + 1);

        // Pass string with parameters ...
        // Parse ...
        if (impl_param_str.GetSize() > 0)
        {
            impl_ptr->Parse(impl_param_str);
        }

        // Replace current formatter with a new implementation ...
        if (!GetParent().ReplaceFormatter(this, impl_ptr, true))
        {
            GASSERT(false);
        }

        // Our job here is done ...
    }
}

void GStrFormatter::Convert()
{
    SetConverted();
}

GStringDataPtr GStrFormatter::GetResult() const
{
    return GStringDataPtr(Value);
}

UPInt GStrFormatter::GetSize() const
{
    return Value.GetSize();
}


////////////////////////////////////////////////////////////////////////////////
GBoolFormatter::GBoolFormatter(GMsgFormat& f, bool v)
: GFormatter(f)
, Value(v)
, SwitchStmt(false)
{
}

void GBoolFormatter::Parse(const GStringDataPtr& str)
{
    // Create implementation ...

    GStringDataPtr token = str.GetNextToken(); 
    GFormatter* impl_ptr = NULL;
    const char* s = token.ToCStr();

    if (!s || token.IsEmpty())
        return;

    if (s[0] == 's' && s[1] == 'w')
    {
        // This is a switch formatter;
        GStringDataPtr tmpStr = str.GetTrimLeft(token.GetSize() + 1);

        result = tmpStr.GetNextToken();

        if (!Value)
        {
            tmpStr.TrimLeft(result.GetSize() + 1);
            result = tmpStr.GetNextToken();
        }

        SwitchStmt = true;
    } else if (GetParent().GetLocaleProvider())
    {
        GFormatterFactory::Args args(GetParent(), token, GResourceFormatter::ValueType(Value));

        impl_ptr = GetParent().GetLocaleProvider()->MakeFormatter(args);
    }

    if (impl_ptr)
    {
        // Calculate string, which should be passed to implemtation ...
        GStringDataPtr impl_param_str = str.GetTrimLeft(token.GetSize() + 1);

        // Pass string with parameters ...
        // Parse ...
        if (!impl_param_str.IsEmpty())
        {
            impl_ptr->Parse(impl_param_str);
        }

        // Replace current formatter with a new implementation ...
        if (!GetParent().ReplaceFormatter(this, impl_ptr, true))
            GASSERT(false);

        // Our job here is done ...
    }
}

void GBoolFormatter::Convert()
{
    if (Converted())
        return;

    if (!SwitchStmt)
        result = (Value ? GStringDataPtr("true", 4) : GStringDataPtr("false", 5));

    SetConverted();
}

GStringDataPtr GBoolFormatter::GetResult() const
{
    return result;
}

UPInt GBoolFormatter::GetSize() const
{
    return result.GetSize();
}



////////////////////////////////////////////////////////////////////////////////
// Return NULL on error.
static
char* AppendCharLeft(char* buff, char* value_ptr, UInt32 ucs_char)
{
    if (ucs_char)
    {
        value_ptr -= GUTF8Util::GetEncodeCharSize(ucs_char);

        if (buff > value_ptr)
            return NULL;

        SPInt index = 0;
        GUTF8Util::EncodeChar(value_ptr, &index, ucs_char);
    }

    return value_ptr;
}


////////////////////////////////////////////////////////////////////////////////
GNumericBase::GNumericBase()
: Precision(1)
, Width(1)
, PrefixChar(' ')
, SeparatorChar('\0')
, ShowSign(false)
, BigLetters(true)
, BlankPrefix(false)
, AlignLeft(false)
, SharpSign(false)
, ValueStr(NULL)
{
}

void GNumericBase::ReadPrintFormat(GStringDataPtr token)
{
    if (token.IsEmpty())
        return;

    const char* s = token.ToCStr();

    if (s == NULL)
        return;

    switch (*s)
    {
    case '-':
        SetAlignLeft();
        ReadPrintFormat(token.TrimLeft(1));
        break;
    case ' ':
        SetBlankPrefix();
        ReadPrintFormat(token.TrimLeft(1));
        break;
    case '.':
        SetPrecision(0); // In case there is no following integer ...
        SetPrecision(G_ReadInteger(token.TrimLeft(1), Precision));
        break;
    case '+':
        SetShowSign();
        ReadPrintFormat(token.TrimLeft(1));
        break;
    case '#':
        SetSharpSign();
        ReadPrintFormat(token.TrimLeft(1));
        break;
    case '0':
        SetPrefixChar('0');
        ReadPrintFormat(token.TrimLeft(1));
        break;
    default:
        ReadWidth(token);
    }
}

void GNumericBase::ReadWidth(GStringDataPtr token)
{
    if (token.IsEmpty())
        return;

    SPInt pos = token.FindChar('.');

    SetWidth(G_ReadInteger(token, Width));

    if (pos >= 0)
    {
        SetPrecision(0); // In case there is no following integer ...
        SetPrecision(G_ReadInteger(token.TrimLeft(1), Precision));
    }
}

////////////////////////////////////////////////////////////////////////////////
// We need two versions for optimization purposes.
// We are not using templates here to prevent inlining.

// UInt64 version
void GNumericBase::ULongLong2String(char* buff, UInt64 value, bool separator, UInt base)
{
    typedef UInt64 ValueType;

    const char* chars;
    ValueType quotient = value;
    unsigned char rem;
    UInt sep_char_left = (separator && base == 10 && SeparatorChar ? 3 : 1000);

    if (BigLetters)
        chars = "0123456789ABCDEF";
    else
        chars = "0123456789abcdef";

    // check that the base if valid
    if (base < 2 || base > 16) 
        return; 

    do 
    {
        if (buff == ValueStr)
            return;

        rem = static_cast<unsigned char>(quotient % base);
        quotient /= base;

        // Add separator.
        if (!sep_char_left)
        {
            sep_char_left = 3;
            *--ValueStr = SeparatorChar;
        }

        --sep_char_left;

        *--ValueStr = chars[rem];
    } while(quotient != 0);
}

// UPInt version
void GNumericBase::ULong2String(char* buff, UInt32 value, bool separator, UInt base)
{
    typedef UInt32 ValueType;

    const char* chars;
    ValueType quotient = value;
    unsigned char rem;
    UInt sep_char_left = (separator && base == 10 && SeparatorChar ? 3 : 1000);

    if (BigLetters)
        chars = "0123456789ABCDEF";
    else
        chars = "0123456789abcdef";

    // check that the base if valid
    if (base < 2 || base > 16) 
        return; 

    do 
    {
        if (buff == ValueStr)
            return;

        rem = static_cast<unsigned char>(quotient % base);
        quotient /= base;

        // Add separator.
        if (!sep_char_left)
        {
            sep_char_left = 3;
            *--ValueStr = SeparatorChar;
        }

        --sep_char_left;

        *--ValueStr = chars[rem];
    } while(quotient != 0);
}


////////////////////////////////////////////////////////////////////////////////
static const Double pow10_precalc[23] = 
{
    1.0, 
    1e+1, 
    1e+2, 
    1e+3, 
    1e+4, 
    1e+5, 
    1e+6, 
    1e+7, 
    1e+8, 
    1e+9, 
    1e+10, 
    1e+11, 
    1e+12, 
    1e+13, 
    1e+14, 
    1e+15, 
    1e+16, 
    1e+17, 
    1e+18, 
    1e+19, 
    1e+20, 
    1e+21, 
    1e+22, 
}; 


////////////////////////////////////////////////////////////////////////////////
GLongFormatter::GLongFormatter(int v)
: Base(10)
, SignedValue(true)
, IsLongLong(false)
, Value(v)
{
    ValueStr = (Buff + sizeof(Buff) - 1);
    Buff[sizeof(Buff) - 1] = '\0';
}

GLongFormatter::GLongFormatter(unsigned int v)
: Base(10)
, SignedValue(false)
, IsLongLong(false)
, Value(v)
{
    ValueStr = (Buff + sizeof(Buff) - 1);
    Buff[sizeof(Buff) - 1] = '\0';
}

GLongFormatter::GLongFormatter(long v)
: Base(10)
, SignedValue(true)
, IsLongLong(false)
, Value(v)
{
    ValueStr = (Buff + sizeof(Buff) - 1);
    Buff[sizeof(Buff) - 1] = '\0';
}

GLongFormatter::GLongFormatter(unsigned long v)
: Base(10)
, SignedValue(false)
, IsLongLong(false)
, Value(v)
{
    ValueStr = (Buff + sizeof(Buff) - 1);
    Buff[sizeof(Buff) - 1] = '\0';
}

#if !defined(GFC_OS_PS2) && !defined(GFC_64BIT_POINTERS) || defined(GFC_OS_WIN32) || (defined(GFC_OS_MAC) && defined(GFC_CPU_X86_64))
GLongFormatter::GLongFormatter(SInt64 v)
: Base(10)
, SignedValue(true)
, IsLongLong(true)
, Value(v)
{
    ValueStr = (Buff + sizeof(Buff) - 1);
    Buff[sizeof(Buff) - 1] = '\0';
}

GLongFormatter::GLongFormatter(UInt64 v)
: Base(10)
, SignedValue(false)
, IsLongLong(true)
, Value(v)
{
    ValueStr = (Buff + sizeof(Buff) - 1);
    Buff[sizeof(Buff) - 1] = '\0';
}
#endif

GLongFormatter::GLongFormatter(GMsgFormat& f, int v)
: GFormatter(f)
, Base(10)
, SignedValue(true)
, IsLongLong(false)
, Value(v)
{
    ValueStr = (Buff + sizeof(Buff) - 1);
    Buff[sizeof(Buff) - 1] = '\0';
}

GLongFormatter::GLongFormatter(GMsgFormat& f, unsigned int v)
: GFormatter(f)
, Base(10)
, SignedValue(false)
, IsLongLong(false)
, Value(v)
{
    ValueStr = (Buff + sizeof(Buff) - 1);
    Buff[sizeof(Buff) - 1] = '\0';
}

GLongFormatter::GLongFormatter(GMsgFormat& f, long v)
: GFormatter(f)
, Base(10)
, SignedValue(true)
, IsLongLong(false)
, Value(v)
{
    ValueStr = (Buff + sizeof(Buff) - 1);
    Buff[sizeof(Buff) - 1] = '\0';
}

GLongFormatter::GLongFormatter(GMsgFormat& f, unsigned long v)
: GFormatter(f)
, Base(10)
, SignedValue(false)
, IsLongLong(false)
, Value(v)
{
    ValueStr = (Buff + sizeof(Buff) - 1);
    Buff[sizeof(Buff) - 1] = '\0';
}

#if !defined(GFC_OS_PS2) && !defined(GFC_64BIT_POINTERS) || defined(GFC_OS_WIN32) || (defined(GFC_OS_MAC) && defined(GFC_CPU_X86_64))
GLongFormatter::GLongFormatter(GMsgFormat& f, SInt64 v)
: GFormatter(f)
, Base(10)
, SignedValue(true)
, IsLongLong(true)
, Value(v)
{
    ValueStr = (Buff + sizeof(Buff) - 1);
    Buff[sizeof(Buff) - 1] = '\0';
}

GLongFormatter::GLongFormatter(GMsgFormat& f, UInt64 v)
: GFormatter(f)
, Base(10)
, SignedValue(false)
, IsLongLong(true)
, Value(v)
{
    ValueStr = (Buff + sizeof(Buff) - 1);
    Buff[sizeof(Buff) - 1] = '\0';
}
#endif

void GLongFormatter::Parse(const GStringDataPtr& str)
{
    // Create implementation ...

    GStringDataPtr tmp_str = str;
    GStringDataPtr token; 
    GFormatter* impl_ptr = NULL;

    while (!tmp_str.IsEmpty())
    {
        token = tmp_str.GetNextToken(); 
        const char* s = token.ToCStr();

        if (!s || token.IsEmpty())
            return;

        tmp_str.TrimLeft(token.GetSize() + 1);

        if (isdigit(s[0]))
        {
            ReadPrintFormat(token);
            continue;
        }

        switch (s[0])
        {
        case '#':
        case '+':
        case '-':
        case '.':
        case ' ':
            ReadPrintFormat(token);
            break;
        case 'b':
            if (G_strncmp(s, "base", 4) == 0)
                // Read base ...
                SetBase(G_ReadInteger(tmp_str, 10));
            break;
        case 'o':
            SetBase(8);
            ReadPrintFormat(tmp_str.GetNextToken());
            break;
        case 'x':
            SetBigLetters(false);
        case 'X':
            SetBase(16);
            ReadPrintFormat(tmp_str.GetNextToken());
            break;
        case 's':
            if (s[1] == 'w')
            {
                // This is a switch formatter;
                impl_ptr = new (GetParent().GetMemoryPool()) GSwitchFormatter(GetParent(), static_cast<GSwitchFormatter::ValueType::Type>(Value));
                // Erase tmp_str. No further parsing is necessary.
                tmp_str.TrimLeft(tmp_str.GetSize());
            } else if (G_strncmp(s, "sep", 3) == 0)
            {
                GStringDataPtr sep_token = tmp_str.GetNextToken(); 

                if (!sep_token.IsEmpty())
                    SetSeparatorChar(*sep_token.ToCStr());

                tmp_str.TrimLeft(sep_token.GetSize());
            }
            break;
        default:
            if (GetParent().GetLocaleProvider())
            {
                GFormatterFactory::Args args(GetParent(), tmp_str, GResourceFormatter::ValueType(static_cast<UPInt>(Value)));

                impl_ptr = GetParent().GetLocaleProvider()->MakeFormatter(args);
            }
        }
    }


    if (impl_ptr)
    {
        // Calculate string, which should be passed to implementation ...
        GStringDataPtr impl_param_str = str.GetTrimLeft(token.GetSize() + 1);

        // Pass string with parameters ...
        // Parse ...
        if (!impl_param_str.IsEmpty())
            impl_ptr->Parse(impl_param_str);

        // Replace current formatter with a new implementation ...
        if (!GetParent().ReplaceFormatter(this, impl_ptr, true))
            GASSERT(false);

        // Our job here is done ...
    }
}

void GLongFormatter::Convert()
{
    if (Converted())
        return;

    if (!(Precision == 0 && Value == 0))
    {
        if (IsLongLong)
            // Use UInt64 version only if we really need that.
            ULongLong2String(Buff, G_Abs(Value), true, Base);
        else
            // Cast correctly ...
            if (SignedValue)
                ULong2String(Buff, G_Abs(static_cast<SInt32>(Value)), true, Base);
            else
                ULong2String(Buff, static_cast<UInt32>(Value), true, Base);
    }

    // Handle precision (put zeros in front).
    for (UPInt i = GetSizeInternal(); i < Precision; ++i)
        *--ValueStr = '0';

    // Do not use ZERO in case of Precision == 0 ...
    if (Precision == 0 || Value == 0)
        PrefixChar = ' ';

    // Only apply signs for base 10
    if (Base == 10) 
    {
        if (PrefixChar == '0')
        {
            // Handle width (put spaces/PrefixChar in front).
            for (UPInt i = GetSizeInternal(); i < static_cast<UPInt>(Width - ((ShowSign || BlankPrefix) ? 1 : 0)); ++i)
                *--ValueStr = PrefixChar;
        }

        // Add a sign if necessary.
        AppendSignCharLeft(Value < 0);

    } else if (Base == 8 || Base == 16)
    {
        if (Value != 0 && SharpSign)
        {
            // Put prefix.
            if (Base == 16)
            {
                *--ValueStr = (BigLetters ? 'X' : 'x');
            } 

            *--ValueStr = '0';
        }
    }

    // The blank is ignored if both the blank and + flags appear.
    if (BlankPrefix && !ShowSign)
    {
        PrefixChar = ' ';

        // Prefix the output value with a blank if the output value is signed and positive
        //if ((SignedValue && Value >= 0) || Base != 10)
        if (SignedValue && Value >= 0)
            *--ValueStr = ' ';
    }

    // Handle width (put spaces/PrefixChar in front).
    const UPInt size = GetSizeInternal();
    if (AlignLeft)
    {
        // Left-aligned ...
        if (size < Width)
        {
            // We realy need to shift the string ...
            char* str = Buff + sizeof(Buff) - 1 - Width;

            memmove(str, ValueStr, size);
            ValueStr = str;

            str += size;
            for (UPInt i = size; i < Width; ++i)
                //*str++ = PrefixChar;
                if (Base != 10)
                    *str++ = PrefixChar;
                else
                    *str++ = ' ';
        }
    }
    else
    {
        for (UPInt i = size; i < Width; ++i)
            //*--ValueStr = PrefixChar;
            if (Base != 10)
                *--ValueStr = PrefixChar;
            else
                *--ValueStr = ' ';
    }

    SetConverted();
}

GStringDataPtr GLongFormatter::GetResult() const
{
	return GStringDataPtr(ValueStr, GetSize());
}

UPInt GLongFormatter::GetSize() const
{
    return GetSizeInternal();
}

void GLongFormatter::InitString(char* pbuffer, UPInt size) const
{
    if (Converted())
        memcpy(pbuffer, ValueStr, size);
}

void GLongFormatter::AppendSignCharLeft(bool negative)
{
    if (HasParent() && GetParent().GetLocaleProvider())
    {
        const GLocale& locale = GetParent().GetLocaleProvider()->GetLocale();

        if (negative)
            ValueStr = AppendCharLeft(Buff, ValueStr, locale.GetNegativeSign());
        else if (ShowSign)
            ValueStr = AppendCharLeft(Buff, ValueStr, locale.GetPositiveSign());
    } else
    {
        if (negative)
            *--ValueStr = '-';
        else if (ShowSign)
            *--ValueStr = '+';
    }
}

////////////////////////////////////////////////////////////////////////////////
GDoubleFormatter::GDoubleFormatter(Double v)
: Type(FmtDecimal)
, Value(v)
, Len(0)
{
    ValueStr = (Buff + sizeof(Buff) - 1);
    Buff[sizeof(Buff) - 1] = '\0';

    SetPrecision(6);
}

GDoubleFormatter::GDoubleFormatter(GMsgFormat& f, Double v)
: GFormatter(f)
, Type(FmtDecimal)
, Value(v)
, Len(0)
{
    ValueStr = (Buff + sizeof(Buff) - 1);
    Buff[sizeof(Buff) - 1] = '\0';

    SetPrecision(6);
}

void GDoubleFormatter::Parse(const GStringDataPtr& str)
{
    // Create implementation ...

    GStringDataPtr tmp_str = str;
    GStringDataPtr token; 
    GFormatter* impl_ptr = NULL;

    while (!tmp_str.IsEmpty())
    {
        token = tmp_str.GetNextToken(); 
        const char* s = token.ToCStr();

        if (!s || token.IsEmpty())
            return;

        tmp_str.TrimLeft(token.GetSize() + 1);

        if (isdigit(s[0]))
        {
            ReadPrintFormat(token);
            continue;
        }

        switch (s[0])
        {
        case '-':
        case '+':
        case ' ':
        case '#':
        case '.':
            ReadPrintFormat(token);
            break;
        case 'f':
            Type = FmtDecimal;
            ReadPrintFormat(tmp_str.GetNextToken());
            break;
        case 'e':
            SetBigLetters(false);
        case 'E':
            Type = FmtScientific;
            ReadPrintFormat(tmp_str.GetNextToken());
            break;
        case 'g':
            SetBigLetters(false);
        case 'G':
            Type = FmtSignificant;
            ReadPrintFormat(tmp_str.GetNextToken());
            break;
        case 's':
            if (s[1] == 'w')
            {
                // This is a switch formatter;
                impl_ptr = new (GetParent().GetMemoryPool()) GSwitchFormatter(GetParent(), G_IRound(Value));
                // Erase tmp_str. No further parsing is necessary.
                tmp_str.TrimLeft(tmp_str.GetSize());
            } else if (G_strncmp(s, "sep", 3) == 0)
            {
                GStringDataPtr sep_token = tmp_str.GetNextToken(); 

                if (!sep_token.IsEmpty())
                    SetSeparatorChar(*sep_token.ToCStr());
            }
            break;
        default:
            if (GetParent().GetLocaleProvider())
            {
                GFormatterFactory::Args args(GetParent(), tmp_str, GResourceFormatter::ValueType(G_IRound(Value)));

                impl_ptr = GetParent().GetLocaleProvider()->MakeFormatter(args);
            }
        }
    }


    if (impl_ptr)
    {
        // Calculate string, which should be passed to implementation ...
        GStringDataPtr impl_param_str = str.GetTrimLeft(token.GetSize() + 1);

        // Pass string with parameters ...
        // Parse ...
        if (!impl_param_str.IsEmpty())
            impl_ptr->Parse(impl_param_str);

        // Replace current formatter with a new implementation ...
        if (!GetParent().ReplaceFormatter(this, impl_ptr, true))
            GASSERT(false);

        // Our job here is done ...
    }
}

#ifdef INTERNAL_D2A

void GDoubleFormatter::DecimalFormat(Double v)
{
    const Double thres_max = (Double)18446744073709551615ull;

    if (v > thres_max) 
    {
        // Overflow.
        // Switch to scientific format.
        ScientificFormat();
        return;
    }     

    if (Precision > __countof(pow10_precalc)) 
        // precision of >= 10 can lead to overflow errors
        Precision = __countof(pow10_precalc);

    // if input is larger than thres_max, revert to exponential
    Double diff = 0.0; 

    UInt64 whole = static_cast<UInt64>(G_Abs(v));
    Double tmp = (G_Abs(v) - whole) * pow10_precalc[Precision];
    UInt64 frac = static_cast<UInt64>(tmp);
    diff = tmp - frac; 

    if (diff > 0.5) {
        ++frac;
        // handle rollover, e.g.  case 0.99 with prec 1 is 1.0 
        if (frac >= pow10_precalc[Precision]) {
            frac = 0;
            ++whole;
        }
    } else if (diff == 0.5 && ((frac == 0) || (frac & 1))) {
        // if halfway, round up if odd, OR
        // if last digit is 0.  That last part is strange
        ++frac;
    }  

    unsigned char rem;

    if (Precision == 0) 
    {
        diff = v - whole;

        if (diff > 0.5) 
            // greater than 0.5, round up, e.g. 1.6 -> 2
            ++whole;
        else if (diff == 0.5 && (whole & 1)) 
            // exactly 0.5 and ODD, then round up 
            // 1.5 -> 2, but 2.5 -> 2 
            ++whole;
    }  

    if (whole > 17976931348623157ull && Type == FmtSignificant)
    {
        // Overflow.
        // Switch to scientific format.
        ScientificFormat();
        return;
    }

    if (Type == FmtDecimal || whole != 0 || (frac == 0 && Precision == 0) || v == 0.0)
    {
        // Output fraction part.
        for (UPInt i = 0; i < Precision; ++i)
        {
            rem = static_cast<unsigned char>(frac % 10);
            frac /= 10;
            *--ValueStr = rem + '0';
        }
    } else
    {
        // Underflow.
        // Switch to scientific format.
        ScientificFormat();
        return;
    }

    if (HasParent() && GetParent().GetLocaleProvider())
    {
        const GLocale& locale = GetParent().GetLocaleProvider()->GetLocale();

        if (Precision)
            // Take care of decimal separator.
            ValueStr = AppendCharLeft(Buff, ValueStr, locale.GetDecimalSeparator());

        // Do the whole part ...
        if (whole <= GFC_MAX_UINT32)
            ULong2String(Buff, static_cast<UInt32>(whole), true);
        else
            ULongLong2String(Buff, whole, true);
    } else
    {
        if (Precision)
            // Take care of decimal separator.
            *--ValueStr = '.';

        // Do the whole part ...
        if (whole <= GFC_MAX_UINT32)
            ULong2String(Buff, static_cast<UInt32>(whole), true);
        else
            ULongLong2String(Buff, whole, true);
    }

    // Take care of a sign 
    AppendSignCharLeft(Value < 0, false);
}

void GDoubleFormatter::ScientificFormat()
{
    Double v = G_Abs(Value);

    // Exponent.
    SInt exponent = 0;

    if (v != 0.0)
    {
        exponent = G_IRound(log10(v)); // Power of 10.

        // Normalize by power of 10.
        if (exponent >= 0 && exponent < __countof(pow10_precalc))
            v /= pow10_precalc[exponent];
        else
            v /= pow(10.0, exponent);
    }

    // Print exponent.
    ULong2String(Buff, G_Abs(exponent), false);

    // exponent should hace three digits (put zeros in front).
    for (UPInt i = GetSizeInternal(); i < 3; ++i)
        *--ValueStr = '0';

    // Print exponent sign.
    AppendSignCharLeft(exponent < 0, true);

    *--ValueStr = (BigLetters ? 'E' : 'e');

    // Print decimal part.
    DecimalFormat(v);
}

#endif

void GDoubleFormatter::Convert()
{
    if (Converted())
        return;

#ifdef INTERNAL_D2A

    // Original code ...
    if (Type == FmtDecimal || Type == FmtSignificant)
        DecimalFormat(Value);
    else
        ScientificFormat();

    // Handle width (put spaces in front).
    for (UPInt i = GetSizeInternal(); i < Width - (BlankPrefix ? 1u : 0); ++i)
        *--ValueStr = PrefixChar;

    if (BlankPrefix)
        *--ValueStr = ' ';

#else

    // Optimization ...
    /*
    if (Type == FmtSignificant)
    {
        SInt32 ival = (SInt32)Value;

        if ((double)ival == Value)
        {
            ULong2String(Buff, ival, true);
            //if (Value <= GFC_MAX_UINT32)
            //    ULong2String(Buff, static_cast<UInt32>(ival), true);
            //else
            //    ULongLong2String(Buff, ival, true);

            Len = GetSizeInternal();

            SetConverted();

            return;
        }
    }
    */

    char f = ' ';

    switch (Type)
    {
    case FmtDecimal:
        f = 'f';
        break;
    case FmtScientific:
        f = BigLetters ? 'E' : 'e';
        break;
    case FmtSignificant:
        f = BigLetters ? 'G' : 'g';
        break;
    };

    char fmt[32];
    char format[32];
    char* str = fmt;

    *str++ = '%'; // escape character ...
    *str++ = '%'; 

    if (ShowSign)
        *str++ = '+';

    if (SharpSign)
        *str++ = '#';

    if (BlankPrefix)
        *str++ = ' ';

    if (AlignLeft)
        *str++ = '-';

    if (PrefixChar == '0')
        *str++ = PrefixChar;

    if (Width != 1)
    {
        *str++ = '%';
        *str++ = 'd';
        *str++ = '.';
        *str++ = '%';
        *str++ = 'd';
        *str++ = f;
        *str++ = '\0';

        G_sprintf(format, sizeof(format), fmt, Width, Precision);
    } 
    else
    {
        *str++ = '.';
        *str++ = '%';
        *str++ = 'd';
        *str++ = f;
        *str++ = '\0';

        G_sprintf(format, sizeof(format), fmt, Precision);
    }

    //G_sprintf(format, sizeof(format), fmt, Precision);

    // Actual formatting ...
    Len = G_sprintf(Buff, sizeof(Buff), format, Value);
    
#ifndef GFC_OS_WINCE
    // Get rid of a possible comma ...
    for (ValueStr = Buff; *ValueStr != 0; ++ValueStr)
    {
        if (*ValueStr == ',')
        {
            *ValueStr = '.';
            break;
        }
    }
#endif

    ValueStr = Buff;

#endif

    SetConverted();
}

GStringDataPtr GDoubleFormatter::GetResult() const
{
	return GStringDataPtr(ValueStr, GetSize());
}

UPInt GDoubleFormatter::GetSize() const
{
#ifdef INTERNAL_D2A
    return GetSizeInternal();
#else
    return Len;
#endif
}

void GDoubleFormatter::InitString(char* pbuffer, UPInt size) const
{
    if (Converted())
        memcpy(pbuffer, ValueStr, size);
}

void GDoubleFormatter::AppendSignCharLeft(bool negative, bool show_sign)
{
    if (HasParent() && GetParent().GetLocaleProvider())
    {
        const GLocale& locale = GetParent().GetLocaleProvider()->GetLocale();

        if (negative)
            ValueStr = AppendCharLeft(Buff, ValueStr, locale.GetNegativeSign());
        else if (show_sign)
            ValueStr = AppendCharLeft(Buff, ValueStr, locale.GetPositiveSign());
    } else
    {
        if (negative)
            *--ValueStr = '-';
        else if (show_sign)
            *--ValueStr = '+';
    }
}

////////////////////////////////////////////////////////////////////////////////
GResourceFormatter::ValueType::ValueType(UPInt rc)
: IsString(false)
, RC_Provider(NULL)
{
    Resource.RLong = rc;
}

GResourceFormatter::ValueType::ValueType(UPInt rc, const GResouceProvider& provider)
: IsString(false)
, RC_Provider(&provider)
{
    Resource.RLong = rc;
}

GResourceFormatter::ValueType::ValueType(SInt rc)
: IsString(true)
, RC_Provider(NULL)
{
    Resource.RStr = reinterpret_cast<const char*>(rc);
}

GResourceFormatter::ValueType::ValueType(SInt rc, const GResouceProvider& provider)
: IsString(true)
, RC_Provider(&provider)
{
    Resource.RStr = reinterpret_cast<const char*>(rc);
}

GResourceFormatter::ValueType::ValueType(const char* rc)
: IsString(true)
, RC_Provider(NULL)
{
    Resource.RStr = rc;
}

GResourceFormatter::ValueType::ValueType(const char* rc, const GResouceProvider& provider)
: IsString(true)
, RC_Provider(&provider)
{
    Resource.RStr = rc;
}

////////////////////////////////////////////////////////////////////////////////
GResourceFormatter::GResourceFormatter(GMsgFormat& f, const GResourceFormatter::ValueType& v)
: GFormatter(f)
, Value(v)
, pRP(NULL)
{
    pRP = v.GetResouceProvider();

    if (!pRP && GetParent().GetLocaleProvider())
    {
        pRP = GetParent().GetLocaleProvider()->GetDefaultRCProvider();
    } 
}

GResourceFormatter::~GResourceFormatter()
{
}

GStringDataPtr GResourceFormatter::MakeString(const TAttrs& attrs) const
{
    if (pRP)
    {
        return pRP->MakeString(Value, attrs);
    }

    return GStringDataPtr();
}

void GResourceFormatter::Parse(const GStringDataPtr& str)
{
    GStringDataPtr impl_param_str; // implementation-specific parameters.
    GStringDataPtr token = str.GetNextToken(); 
    GFormatter* impl_ptr = NULL;

    // Create implementation ...
    if (GetParent().GetLocaleProvider())
    {
        GFormatterFactory::Args args(GetParent(), token, Value);

        impl_ptr = GetParent().GetLocaleProvider()->MakeFormatter(args);
    }

    // Calculate string, which should be passed to implemtation ...
    impl_param_str = str.GetTrimLeft(token.GetSize() + 1);

    GASSERT(impl_ptr);
    if (impl_ptr)
    {
        // Pass string with parameters ...
        // Parse ...
        if (impl_param_str.GetSize() > 0)
        {
            impl_ptr->Parse(impl_param_str);
        }

        // Replace current formatter with a new implementation ...
        GetParent().ReplaceFormatter(static_cast<GFormatter*>(this), static_cast<GFormatter*>(impl_ptr), true);

        // Our job here is done ...
    }
}

void GResourceFormatter::Convert()
{
    if (!Converted())
    {
        Result = MakeString(TAttrs());
        SetConverted();
    }
}

GStringDataPtr GResourceFormatter::GetResult() const
{
    return Result;
}

UPInt GResourceFormatter::GetSize() const
{
    return Result.GetSize();
}

////////////////////////////////////////////////////////////////////////////////
enum
{
    NotInitializedFmtrInd = 0xFFFF
};

GMsgFormat::GMsgFormat(const GMsgFormat::Sink& r)
    : EscapeChar('%'), FirstArgNum(0), NonPosParamNum(0), UnboundFmtrInd(NotInitializedFmtrInd), 
    StrSize(0), pLocaleProvider(NULL), Result(r)
{
}

GMsgFormat::GMsgFormat(const GMsgFormat::Sink& r, const GLocaleProvider& loc)
    : EscapeChar('%'), FirstArgNum(0), NonPosParamNum(0), UnboundFmtrInd(NotInitializedFmtrInd), 
    StrSize(0), pLocaleProvider(&loc), Result(r)
{
}

GMsgFormat::~GMsgFormat()
{
    const UPInt data_size = Data.GetSize();

    for (UPInt i = 0; i < data_size; ++i)
    {
        fmt_record& rec = Data[i];

        if (rec.GetType() == eFmtType && rec.GetValue().Formatter.Allocated)
        {
            GFormatter* formatter = rec.GetValue().Formatter.Formatter;

            if (formatter)
            {
                // Placement destructor.
                formatter->~GFormatter();
                operator delete (formatter, GetMemoryPool());
            }
        }
    }
}

void GMsgFormat::AddStringRecord(const GStringDataPtr& str)
{
    fmt_value value;
    value.String.Str = str.ToCStr();
    value.String.Len = static_cast<unsigned char>(str.GetSize());

    Data.PushBack(fmt_record(eStrType, value));
}

void GMsgFormat::AddFormatterRecord(GFormatter* f, bool allocated)
{
    fmt_value value;

    value.Formatter.Formatter = f;
    value.Formatter.Allocated = allocated;

    // New fmt_record ...
    Data.PushBack(fmt_record(eFmtType, value));
}

void GMsgFormat::Parse(const char* fmt)
{
    enum {stInitial, stParam} state = stInitial;
    const char* str = fmt;
    const char* str_ptr = fmt;
    UnboundFmtrInd = NotInitializedFmtrInd;
    bool escape = false;

    if (!fmt)
        return;

    NonPosParamNum = 0;

    while (*str)
    {
        switch (state)
        {
            case stInitial:
                if (escape)
                {
                    escape = false;
                    ++str;
                } else if (*str == '{')
                {
                    // Store string ...
                    if (str_ptr != str)
                        AddStringRecord(GStringDataPtr(str_ptr, str - str_ptr));

                    // We found parameter
                    str_ptr = ++str;
                    state = stParam;
                } else if (*str == GetEscapeChar() && *(str + 1) != 0)
                {
                    // It is an escape character ...

                    // Store string ...
                    if (str_ptr != str)
                        AddStringRecord(GStringDataPtr(str_ptr, str - str_ptr));

                    str_ptr = ++str;
                    escape = true;
                } else
                {
                    ++str;
                }
                break;
            case stParam:
                if (*str == '}')
                {
                    // Store parameter string ...
                    if (str_ptr != str)
                    {
                        // There is something to store ...

                        int arg_num = 0xFF;

                        // Skip spaces ...
                        while (isspace(*str_ptr))
                        {
                            ++str_ptr;
                        }

                        if (isdigit(*str_ptr))
                        {
                            // Try to read a number in front of a parameter string ...
                            arg_num = atoi(str_ptr);

                            // Try to find a separator ...
                            for (; *str_ptr && *str_ptr != ':' && *str_ptr != '}'; ++str_ptr)
                            {
                                ;
                            }

                            if (*str_ptr && *str_ptr == ':')
                            {
                                ++str_ptr;
                            }
                        } else
                        {
                            // Non-positional parameter ...
                            ++NonPosParamNum;
                        }

                        fmt_value value;
                        value.String.Str = str_ptr;
                        value.String.Len = static_cast<unsigned char>(str - str_ptr);
                        value.String.ArgNum = static_cast<unsigned char>(arg_num);

                        Data.PushBack(fmt_record(eParamStrType, value));

                        if (UnboundFmtrInd == NotInitializedFmtrInd)
                        {
                            // This is the very first parameter.
                            UnboundFmtrInd = static_cast<UInt16>(Data.GetSize() - 1);
                        }
                    }

                    // We found parameter
                    str_ptr = ++str;
                    state = stInitial;
                } else
                {
                    ++str;
                }
        }
    }

    if (state != stInitial)
    {
        GASSERT(false);
    } else
    {
        // Store string ...
        if (str_ptr != str)
        {
            fmt_value value;
            value.String.Str = str_ptr;
            value.String.Len = static_cast<unsigned char>(str - str_ptr);

            Data.PushBack(fmt_record(eStrType, value));
        }
    }
}

void GMsgFormat::FormatF(const GStringDataPtr& fmt, va_list argList)
{
    enum FieldType {ftNone, ftInt, ftLong, ftDouble, ftString, ftPointer};

    if (fmt.IsEmpty())
        return;

    GStringDataPtr cur_fmt = fmt;
    
    while (!cur_fmt.IsEmpty())
    {
        FieldType fieldType = ftNone;
        SPInt pos = cur_fmt.FindChar('%');

        if (pos < 0)
        {
            // No formatters, just plain text. This is the last piece of text.
            AddStringRecord(cur_fmt);

            // This is the last piece of text.
            break;
        } else
        {
            // Unsigned version of position.
            UPInt upos = UPInt(pos);

            // Formatter or an escape character.
            if (cur_fmt.GetSize() > upos + 1)
            {
                // This is not the last character.
                ++upos;

                // In case it is not an escape character.
                const char* str = cur_fmt.ToCStr();
                if (str[upos] != '%')
                {
                    union DataType
                    {
                        SInt    v_sint;
                        SInt64  v_sint64;
                        UInt    v_uint;
                        UInt64  v_uint64;
                        Double  v_double;
                        char*   v_str;
                    };

                    DataType data;
                    UInt base = 10;
                    bool bigLetters = false;
                    bool unsigned_type = false;
                    GDoubleFormatter::PresentationType dpType = GDoubleFormatter::FmtDecimal;

                    if (upos > 0)
                    {
                        // Let's store previously found string.
                        AddStringRecord(cur_fmt.GetTrimRight(cur_fmt.GetSize() - pos));
                    }

                    UPInt startPos = upos;
                    data.v_uint = 0;
                    enum prefix_t {prefix_none, prefix_h, prefix_l, prefix_I};
                    prefix_t prefix = prefix_none;
                    UInt prefix_size = 0;

                    // Locate the end of the formatting sequence.
                    for (; upos < cur_fmt.GetSize(); ++upos)
                    {
                        switch (str[upos])
                        {
                        case 'c':
                            // Ignore for the time being.
                            break;
                        case 'C':
                            // Ignore for the time being.
                            break;
                        case 'h':
                            prefix = prefix_h;
                            ++prefix_size;
                            break;
                        case 'l':
                            prefix = prefix_l;
                            ++prefix_size;
                            break;
                        case 'I':
                            prefix = prefix_I;
                            ++prefix_size;
                            break;
                        case 's':
                            fieldType = ftString;
                            data.v_str = va_arg(argList, char*);
                            break;
                        case 'S':
                            // Ignore for the time being.
                            break;

                        case 'd':
                        case 'i':
                            if (prefix == prefix_l)
                            {
                                fieldType = ftLong;
                                data.v_sint64 = va_arg(argList, SInt);
                            }
                            else
                            {
                                fieldType = ftInt;
                                data.v_sint = va_arg(argList, SInt);
                            }
                            break;
                        case 'n':
                            // Pointer to integer ...
                            fieldType = ftInt;
                            data.v_sint = *va_arg(argList, SInt*);
                            break;
                        case 'u':
                            if (prefix == prefix_l)
                            {
                                fieldType = ftLong;
                                unsigned_type = true;
                                data.v_uint = va_arg(argList, UInt);
                            }
                            else
                            {
                                fieldType = ftInt;
                                unsigned_type = true;
                                data.v_uint = va_arg(argList, UInt);
                            }
                            break;
                        case 'o':
                            fieldType = ftInt;
                            base = 8;
                            unsigned_type = true;
                            data.v_uint = va_arg(argList, UInt);
                            break;
                        case 'X':
                            bigLetters = true;
                        case 'x':
                            fieldType = ftInt;
                            unsigned_type = true;
                            data.v_uint = va_arg(argList, UInt);
                            base = 16;
                            break;
                        case 'p':
                            // Same as 'x'. Value is passed as a pointer.
                            fieldType = ftInt;
                            unsigned_type = true;
                            data.v_uint = *va_arg(argList, UInt*);
                            base = 16;
                            break;

                        case 'E':
                            bigLetters = true;
                        case 'e':
                            dpType = GDoubleFormatter::FmtScientific;
                            fieldType = ftDouble;
                            data.v_double = Double(va_arg(argList, double));
                            break;
                        case 'G':
                            bigLetters = true;
                        case 'g':
                            dpType = GDoubleFormatter::FmtSignificant;
                            fieldType = ftDouble;
                            data.v_double = Double(va_arg(argList, double));
                            break;
                        case 'f':
                            dpType = GDoubleFormatter::FmtDecimal;
                            fieldType = ftDouble;
                            data.v_double = Double(va_arg(argList, double));
                            break;

                        default:
                            break;
                        }

                        if (fieldType != ftNone)
                        {
                            // Leave loop. Do not increase position.
                            break;
                        }
                    }

                    // Create formatter object.
                    switch (fieldType)
                    {
                    case ftInt:
                        {
                            GLongFormatter* pf = NULL;
                            GStringDataPtr attr(cur_fmt.ToCStr() + startPos, upos - startPos - prefix_size);

                            if (unsigned_type)
                                pf = new (GetMemoryPool()) GLongFormatter(data.v_uint);
                            else
                                pf = new (GetMemoryPool()) GLongFormatter(data.v_sint);

                            pf->SetBase(base);
                            pf->SetBigLetters(bigLetters);
                            pf->Parse(attr);

                            AddFormatterRecord(pf, true);
                        }
                        break;
                    case ftLong:
                        {
                            GLongFormatter* pf = NULL;
                            GStringDataPtr attr(cur_fmt.ToCStr() + startPos, upos - startPos - prefix_size);

                            if (unsigned_type)
                                pf = new (GetMemoryPool()) GLongFormatter(data.v_uint64);
                            else
                                pf = new (GetMemoryPool()) GLongFormatter(data.v_sint64);

                            pf->SetBase(base);
                            pf->SetBigLetters(bigLetters);
                            pf->Parse(attr);

                            AddFormatterRecord(pf, true);
                        }
                        break;
                    case ftDouble:
                        {
                            GDoubleFormatter* pf = new (GetMemoryPool()) GDoubleFormatter(data.v_double);
                            GStringDataPtr attr(cur_fmt.ToCStr() + startPos, upos - startPos);

                            pf->SetBigLetters(bigLetters);
                            pf->SetType(dpType);
                            pf->Parse(attr);

                            AddFormatterRecord(pf, true);
                        }
                        break;
                    case ftString:
                        {
                            GStrFormatter* pf = new (GetMemoryPool()) GStrFormatter(data.v_str);

                            AddFormatterRecord(pf, true);
                        }
                    case ftPointer:
                        // We do not handle it yet.
                        break;
                    case ftNone:
                        // We couldn't find anything.
                        break;
                    };

                } else
                {
                    // Skip extra '%' character.
                    AddStringRecord(cur_fmt.GetTrimRight(cur_fmt.GetSize() - pos - 1));
                }
            } else
            {
                // We found last character in a string. 
                // It is not a formatter specification.
                AddStringRecord(cur_fmt);
            } // else

            // Restore signed version of position.
            pos = upos;

            // Trim processed data ...
            cur_fmt.TrimLeft(pos + 1);
        } // else

    } // while loop

    // Finally, make a string.
    MakeString();
}

bool GMsgFormat::ReplaceFormatter(GFormatter* oldf, GFormatter* newf, bool allocated)
{
    const UPInt data_size = Data.GetSize();

    for (UPInt i = 0; i < data_size; ++i)
    {
        fmt_record& rec = Data[i];

        if (rec.GetType() == eFmtType && rec.GetValue().Formatter.Formatter == oldf)
        {
            // New value ...
            fmt_value value;

            value.Formatter.Formatter = newf;
            value.Formatter.Allocated = allocated;

            // New fmt_record ...
            rec = fmt_record(eFmtType, value);

            return true;
        }
    }

    return false;
}

bool GMsgFormat::NextFormatter()
{
    bool shift_unbound_fmtr = true;
    const UPInt data_size = Data.GetSize();

    DataInd = -1;

    for (UPInt i = UnboundFmtrInd; i < data_size; ++i)
    {
        const fmt_record& rec = Data[i];

        if (rec.GetType() == eParamStrType)
        {
            str_ptr sp = rec.GetValue().String;

            if (sp.ArgNum == GetFirstArgNum())
            {
                if (shift_unbound_fmtr)
                {
                    ++UnboundFmtrInd;
                }

                DataInd = i;

                return true;

            } else if (shift_unbound_fmtr)
            {
                shift_unbound_fmtr = false;
            }
        } else if (shift_unbound_fmtr)
        {
            ++UnboundFmtrInd;
        }
    }

    return false;
}

void GMsgFormat::Bind(GFormatter* formatter, const bool allocated)
{
    GASSERT(DataInd >= 0);

    const fmt_record& rec = Data[DataInd];
    // We have to make a copy of str_ptr here ...
    str_ptr sp = rec.GetValue().String;

    // Init fmt_value ...
    fmt_value value;
    value.Formatter.Formatter = formatter;
    // value.formatter.arg_num = GetFirstArgNum();
    value.Formatter.Allocated = allocated;

    // Make new record ...
    Data[DataInd] = fmt_record(eFmtType, value);

    // Call formatter->Parse if necessary ...
    if (sp.Len > 0)
    {
        formatter->Parse(GStringDataPtr(sp.Str, sp.Len));
    }
}

void GMsgFormat::BindNonPos()
{
    GFmtInfo<GResourceFormatter::ValueType>::formatter fr(*this, GResourceFormatter::ValueType(0));

    if (NextFormatter())
    {
        Bind(&fr, false);
    }

    if (--NonPosParamNum)
        BindNonPos();
    else 
        MakeString();
}

void GMsgFormat::Evaluate(UPInt ind)
{
    if (Data[ind].GetType() != eFmtType)
        return;

    GFormatter& formatter = *Data[ind].GetValue().Formatter.Formatter;

    GFormatter::requirements_t req = formatter.GetRequirements();

    if (req == GFormatter::rtNone)
    {
        // Nothing to evaluate. Just convert Data ...
        if (!formatter.Converted())
            formatter.Convert();

        return;
    }

    // Previous string sentence separator is required ...
    if (req & GFormatter::rtPrevStrSS)
    {
        UPInt       prev_ind = ind - 1;
        const UPInt overflow = 0U - 1;
        bool        found = false;

        for (; prev_ind != overflow && !found; --prev_ind)
        {
            switch (Data[prev_ind].GetType())
            {
                case eStrType:
                    // Previous record is a string ...
                    {
                        const str_ptr& sp = Data[prev_ind].GetValue().String;
                        GStringDataPtr str(sp.Str, sp.Len);

                        if (IsSpace(str))
                        {
                            // Empty string. Skip it ...
                            break;
                        }

                        formatter.SetPrevStr(str);
                    }

                    found = true;
                    break;
                case eFmtType:
                    // Previous record is a formatter ...
                    {
                        const GFormatter& parent_formatter = *Data[prev_ind].GetValue().Formatter.Formatter;

                        if (parent_formatter.GetRequirements() & GFormatter::rtNextStr)
                        {
                            // Cyclic dependency, but sentence separator ...
                            formatter.SetPrevStr(GStringDataPtr("stub"));

                            found = true;
                            break;
                        }

                        if ((parent_formatter.GetRequirements() & GFormatter::rtParent) &&
                            parent_formatter.GetParentRef() == GFormatter::prNext)
                        {
                            // Cyclic dependency, but sentence separator ...
                            formatter.SetPrevStr(GStringDataPtr("stub"));

                            found = true;
                            break;
                        }

                        Evaluate(prev_ind);

                        if (IsSpace(parent_formatter.GetResult()))
                        {
                            // Empty string. Skip it ...
                            break;
                        }

                        formatter.SetPrevStr(parent_formatter.GetResult());
                    }

                    found = true;
                    break;
                default:
                    GASSERT(false);
            };
        }

        if (!found)
        {
            // This is the very first string ...
            // Set previous string to an empty string ...
            formatter.SetPrevStr(GStringDataPtr());
        }
    }

    // Previous string is required ...
    if (req & GFormatter::rtPrevStr)
    {
        UPInt       prev_ind = ind - 1;
        const UPInt overflow = 0U - 1;
        bool        found = false;

        for (; prev_ind != overflow && !found; --prev_ind)
        {
            switch (Data[prev_ind].GetType())
            {
                case eStrType:
                    // Previous record is a string ...
                    {
                        const str_ptr& sp = Data[prev_ind].GetValue().String;
                        GStringDataPtr str(sp.Str, sp.Len);

                        if (IsSpace(str))
                        {
                            // Empty string. Skip it ...
                            break;
                        }

                        formatter.SetPrevStr(str);
                    }

                    found = true;
                    break;
                case eFmtType:
                    // Previous record is a formatter ...
                    {
                        const GFormatter& parent_formatter = *Data[prev_ind].GetValue().Formatter.Formatter;

                        if (parent_formatter.GetRequirements() & GFormatter::rtNextStr)
                        {
                            // Cyclic dependency ...
                            GASSERT(false);
                        }

                        Evaluate(prev_ind);

                        if (IsSpace(parent_formatter.GetResult()))
                        {
                            // Empty string. Skip it ...
                            break;
                        }

                        formatter.SetPrevStr(parent_formatter.GetResult());
                    }

                    found = true;
                    break;
                default:
                    GASSERT(false);
            };
        }

        if (!found)
        {
            // This is the very first string ...
            // Set previous string to an empty string ...
            formatter.SetPrevStr(GStringDataPtr());
        }
    }

    // Next string is required ...
    if (req & GFormatter::rtNextStr)
    {
        UPInt   next_ind = ind + 1;
        bool    found = false;
        const UPInt data_size = Data.GetSize();

        for (; next_ind < data_size && !found; ++next_ind)
        {
            switch (Data[next_ind].GetType())
            {
                case eStrType:
                    // Next record is a string ...
                    {
                        const str_ptr& sp = Data[next_ind].GetValue().String;
                        GStringDataPtr str(sp.Str, sp.Len);

                        if (IsSpace(str))
                        {
                            // Empty string. Skip it ...
                            break;
                        }

                        formatter.SetNextStr(str);
                    }

                    found = true;
                    break;
                case eFmtType:
                    // Next record is a formatter ...
                    {
                        const GFormatter& parent_formatter = *Data[next_ind].GetValue().Formatter.Formatter;

                        if (parent_formatter.GetRequirements() & GFormatter::rtPrevStr)
                        {
                            // Cyclic dependency ...
                            GASSERT(false);
                        }

                        Evaluate(next_ind);

                        if (IsSpace(parent_formatter.GetResult()))
                        {
                            // Empty string. Skip it ...
                            break;
                        }

                        formatter.SetNextStr(parent_formatter.GetResult());
                    }

                    found = true;
                    break;
                default:
                    GASSERT(false);
            };
        }

        if (!found)
        {
            // This is the very last string, or no non-empty strings  ...
            // Set next string to an empty string ...
            formatter.SetNextStr(GStringDataPtr());
        }
    }

    // Parent is required ...
    if (req & GFormatter::rtParent)
    {
        UPInt       parent_ind = ind;
        const UPInt overflow = 0U - 1;
        bool        found = false;

        switch (formatter.GetParentRef())
        {
        case GFormatter::prPrev:
            // Locate previous formatter ...
            for (--parent_ind; parent_ind != overflow && !found; --parent_ind)
            {
                if (Data[parent_ind].GetType() == eFmtType)
                {
                    found = true;
                    break;
                }
            }

            if (found)
            {
                const GFormatter& parent_formatter = *Data[parent_ind].GetValue().Formatter.Formatter;

                if (parent_formatter.GetRequirements() & GFormatter::rtNextStr)
                    // Cyclic dependency ...
                    GASSERT(false);

                Evaluate(parent_ind);

                formatter.SetParent(parent_formatter);
            } else
                GASSERT(false);

            break;
        case GFormatter::prNext:
            // Locate next formatter ...
            for (++parent_ind; parent_ind < Data.GetSize() && !found; ++parent_ind)
            {
                if (Data[parent_ind].GetType() == eFmtType)
                {
                    found = true;
                    break;
                }
            }

            if (found)
            {
                GFormatter& parent_formatter = *Data[parent_ind].GetValue().Formatter.Formatter;

                if (parent_formatter.GetRequirements() & GFormatter::rtPrevStr)
                    // Cyclic dependency ...
                    GASSERT(false);

                Evaluate(parent_ind);

                formatter.SetParent(parent_formatter);
            } else
                GASSERT(false);

            break;
        case GFormatter::prPos:
            {
                const unsigned char param_position = formatter.GetParentPos();
                unsigned char cur_param_position = 0;
                const UPInt data_size = Data.GetSize();

                // Locate positional formatter ...
                for (parent_ind = 0; parent_ind < data_size && !found; ++parent_ind)
                {
                    if (Data[parent_ind].GetType() == eFmtType)
                    {
                        if (cur_param_position == param_position)
                        {
                            // We found it ...
                            found = true;
                            break;
                        } else
                            ++cur_param_position;
                    }
                }

                if (found)
                {
                    // Do not refer to itself ...
                    GASSERT(parent_ind != ind);

                    GFormatter& parent_formatter = *Data[parent_ind].GetValue().Formatter.Formatter;

                    Evaluate(parent_ind);

                    formatter.SetParent(parent_formatter);
                } else
                    GASSERT(false);
            }
            break;
        case GFormatter::prNone:
            break;
        };
    }

    // Finally convert Data ...
    if (!formatter.Converted())
        formatter.Convert();
}

void GMsgFormat::MakeString()
{
    StrSize = 0;
    const UPInt data_size = Data.GetSize();

    // Resolve dependencies ...
    for (UPInt i = 0; i < data_size; ++i)
    {
        const fmt_value& value = Data[i].GetValue();

        switch (Data[i].GetType())
        {
            case eStrType:
                StrSize += value.String.Len;
                break;
            case eFmtType:
                {
                    // Evaluate formatter to resolve dependencies.
                    Evaluate(i);

                    const GFormatter* const fr = value.Formatter.Formatter;

                    if (fr) 
                        StrSize += fr->GetSize();
                }
                break;
            case eParamStrType:
                // There is no corresponding argument.
                //StrSize += 0;
                //GASSERT(false);
                break;
            default:
                GASSERT(false);
        }
    }

    // Put resulting data into a sink.
    switch (Result.Type)
    {
    case Sink::tStr:
        Result.SinkData.pStr->AssignString(*this, GetStrSize());
        break;
    case Sink::tStrBuffer:
        {
            GStringBuffer& buffer = *Result.SinkData.pStrBuffer;
            const UPInt data_size = Data.GetSize();

            // Reserve ...
            buffer.Reserve(buffer.GetSize() + GetStrSize());

            // Put data into the buffer.
            for (UPInt i = 0; i < data_size; ++i)
            {
                const fmt_value& value = Data[i].GetValue();

                switch (Data[i].GetType())
                {
                    case eStrType:
                        buffer.AppendString(value.String.Str, value.String.Len);
                        break;
                    case eFmtType:
                        {
                            const GFormatter* const fr = value.Formatter.Formatter;

                            if (fr) 
                            {
                                GStringDataPtr r = fr->GetResult();
                                buffer.AppendString(r.ToCStr(), r.GetSize());
                            }
                        }
                        break;
                    default:
                        GASSERT(false);
                }
            }
        }
        break;
    case Sink::tDataPtr:
        {
            const Sink::StrDataType& str = Result.SinkData.DataPtr;
            char* s = const_cast<char*>(str.pStr);
            InitString(s, str.Size);

            // Put finalizing '\0' character.
            const UPInt size = G_Min(str.Size - 1, GetStrSize());
            s[size] = '\0';
        }
    };
}

void GMsgFormat::InitString(char* pbuffer, UPInt size) const
{
    UPInt left_size = size;
    UPInt count = 0;
    const UPInt data_size = Data.GetSize();

    // Put Data ...
    for (UPInt i = 0; left_size != 0 && i < data_size; ++i)
    {
        const fmt_value& value = Data[i].GetValue();

        switch (Data[i].GetType())
        {
            case eStrType:
                count = G_Min(left_size, UPInt(value.String.Len));
                memcpy(pbuffer, value.String.Str, count);
                left_size -= count;
                pbuffer += count;

                break;
            case eFmtType:
                {
                    const GFormatter* const fr = value.Formatter.Formatter;

                    if (fr) 
                    {
                        GStringDataPtr r = fr->GetResult();

                        count = G_Min(left_size, r.GetSize());
                        memcpy(pbuffer, r.ToCStr(), count);
                        left_size -= count;
                        pbuffer += count;
                    }
                }
                break;
            default:
                GASSERT(false);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
bool IsSpace(GStringDataPtr s)
{
    UInt32 c = 0;
    const char* pstr = s.ToCStr();
    const char* pstr_end = pstr + s.GetSize();

    if (pstr != pstr_end)
    {
        do 
        {
            c = GUTF8Util::DecodeNextChar(&pstr);

            if (c == 0 || !G_iswspace(static_cast<wchar_t>(c)))
                return false;
        } while (pstr < pstr_end);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
UPInt G_SPrintF(const GMsgFormat::Sink& result, const char* fmt, ...)
{
    GMsgFormat parsed_format(result);
    va_list argList;

    va_start(argList, fmt);
    parsed_format.FormatF(GStringDataPtr(fmt), argList);
    va_end(argList);

    return parsed_format.GetStrSize();
}

////////////////////////////////////////////////////////////////////////////////
SInt G_ReadInteger(GStringDataPtr& str, SInt defaultValue, char separator)
{
    GStringDataPtr token = str.GetNextToken(separator); 
    const char* s = token.ToCStr();

    if (!token.IsEmpty() && s && isdigit(s[0]))
    {
        // Strip found number.
        UPInt i = 1;
        for (; i < token.GetSize() && isdigit(s[i]); ++i) { ; }
        str.TrimLeft(i);

        return atoi(s);
    }

    return defaultValue;
}

////////////////////////////////////////////////////////////////////////////////
void GMsgFormat::FinishFormatD()
{
    if (NonPosParamNum)
    {
        SetFirstArgNum(0xFF);
        BindNonPos();
    } else
    {
        MakeString();
    }
}

