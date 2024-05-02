/**********************************************************************

Filename    :   GString.cpp
Content     :   GString UTF8 string implementation with copy-on
                write semantics (thread-safe for assignment but not
                modification).
Created     :   April 27, 2007
Authors     :   Ankur Mohan, Michael Antonov

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxString.h"

#include "GUTF8Util.h"
#include "GDebug.h"
#include "GFunctions.h"

#include <stdlib.h>
#include <ctype.h>

#ifdef GFC_OS_QNX
# include <strings.h>
#endif


// ***** GFxWStringBuffer class


GFxWStringBuffer::GFxWStringBuffer(const GFxWStringBuffer& other)
    : pText(0), Length(0), Reserved(0,0)
{
    // Our reserve is 0. It's ok, although not effient. Should we add
    // a different constructor?
    if (other.pText && Resize(other.Length+1))    
        memcpy(pText, other.pText, (other.Length+1)*sizeof(wchar_t));
}

GFxWStringBuffer::~GFxWStringBuffer()
{
    if ((pText != Reserved.pBuffer) && pText)
        GFREE(pText);
}

bool     GFxWStringBuffer::Resize(UPInt size)
{    
    if ((size > Length) && (size >= Reserved.Size))
    {
        wchar_t* palloc = (wchar_t*) GALLOC(sizeof(wchar_t)*(size+1), GStat_Default_Mem);
        if (palloc)
        {
            if (pText)
                memcpy(palloc, pText, (Length+1)*sizeof(wchar_t));
            palloc[size] = 0;

            if ((pText != Reserved.pBuffer) && pText)
                GFREE(pText);
            pText  = palloc;
            Length = size;
            return true;
        }
        return false;
    }

    if (pText)
        pText[size] = 0;
    Length = size;
    return true;
}


// Assign buffer data.
GFxWStringBuffer& GFxWStringBuffer::operator = (const GFxWStringBuffer& buff)
{
    SetString(buff.pText, buff.Length);
    return *this;
}

GFxWStringBuffer& GFxWStringBuffer::operator = (const GString& str)
{
    UPInt size = str.GetLength();
    if (Resize(size) && size)    
        GUTF8Util::DecodeString(pText, str.ToCStr(), size);
    return *this;
}

GFxWStringBuffer& GFxWStringBuffer::operator = (const char* putf8str)
{
    UPInt size = GUTF8Util::GetLength(putf8str);
    if (Resize(size) && size)    
        GUTF8Util::DecodeString(pText, putf8str, size);
    return *this;
}

GFxWStringBuffer& GFxWStringBuffer::operator = (const wchar_t *pstr)
{
    UPInt length = 0;
    for (const wchar_t *pstr2 = pstr; *pstr2 != 0; pstr2++)
        length++;     
    SetString(pstr, length);
    return *this;
}

void GFxWStringBuffer::SetString(const char* putf8str, UPInt length)
{
    if (length == GFC_MAX_UPINT)
        length = G_strlen(putf8str);
    if (Resize(length) && length)
        GUTF8Util::DecodeString(pText, putf8str, length);    
}

void GFxWStringBuffer::SetString(const wchar_t* pstr, UPInt length)
{
    if (length == GFC_MAX_UPINT)
        length = G_wcslen(pstr);
    if (Resize(length) && length)
        memcpy(pText, pstr, (length+1)*sizeof(wchar_t));    
}

void GFxWStringBuffer::StripTrailingNewLines()
{
    SPInt len = SPInt(Length);
    // check, is the content already null terminated
    if (len > 0 && pText[len -1] == 0)
        --len; //if yes, skip the '\0'
    for (SPInt i = len - 1; i >= 0 && (pText[i] == '\n' || pText[i] == '\r'); --i)
    {
        --Length;
        pText[i] = 0;
    }
}
