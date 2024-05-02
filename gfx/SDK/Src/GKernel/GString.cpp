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

#include "GString.h"

#include "GUTF8Util.h"
#include "GDebug.h"
#include "GFunctions.h"

#include <stdlib.h>
#include <ctype.h>

#ifdef GFC_OS_QNX
# include <strings.h>
#endif

#define GString_LengthIsSize (UPInt(1) << GString::Flag_LengthIsSizeShift)

GString::DataDesc GString::NullData = {GString_LengthIsSize, 1, {0} };



GString::GString()
{
    pData = &NullData;
    pData->AddRef();
};

GString::GString(const char* pdata)
{
    // Obtain length in bytes; it doesn't matter if _data is UTF8.
    UPInt size = pdata ? G_strlen(pdata) : 0; 
    pData = AllocDataCopy1(GMemory::GetGlobalHeap(), size, 0, pdata, size);
};

GString::GString(const char* pdata1, const char* pdata2, const char* pdata3)
{
    // Obtain length in bytes; it doesn't matter if _data is UTF8.
    UPInt size1 = pdata1 ? G_strlen(pdata1) : 0; 
    UPInt size2 = pdata2 ? G_strlen(pdata2) : 0; 
    UPInt size3 = pdata3 ? G_strlen(pdata3) : 0; 

    DataDesc *pdataDesc = AllocDataCopy2(GMemory::GetGlobalHeap(),
                                         size1 + size2 + size3, 0,
                                         pdata1, size1, pdata2, size2);
    memcpy(pdataDesc->Data + size1 + size2, pdata3, size3);   
    pData = pdataDesc;    
}

GString::GString(const char* pdata, UPInt size)
{
    GASSERT((size == 0) || (pdata != 0));
    pData = AllocDataCopy1(GMemory::GetGlobalHeap(), size, 0, pdata, size);
};


GString::GString(const InitStruct& src, UPInt size)
{
    pData = AllocData(GMemory::GetGlobalHeap(), size, 0);
    src.InitString(GetData()->Data, size);
}

GString::GString(const GString& src)
{
    DataDesc* pdata = src.GetData();   

    // We can only AddRef if string belongs to the same (global) heap.
    // This constructor should not be called for derived (non-global heap) classes.
    if (src.GetHeap() == GMemory::GetGlobalHeap())
    {
        pData = pdata;
        pData->AddRef();
    }
    else
    {
        UPInt size = pdata->GetSize();
        pData = AllocDataCopy1(GMemory::GetGlobalHeap(),size,pdata->GetLengthFlag(),
                               pdata->Data, size);
    }
}

GString::GString(const GStringBuffer& src)
{
    pData = AllocDataCopy1(GMemory::GetGlobalHeap(), src.GetSize(), 0, src.ToCStr(), src.GetSize());
}

GString::GString(const wchar_t* data)
{
    pData = &NullData;
    pData->AddRef();
    // Simplified logic for wchar_t constructor.
    if (data)    
        *this = data;    
}


GString::DataDesc* GString::AllocData(GMemoryHeap* pheap, UPInt size, UPInt lengthIsSize)
{
    GString::DataDesc* pdesc;

    if (size == 0)
    {
        pdesc = &NullData;
        pdesc->AddRef();
        return pdesc;
    }

    pdesc = (DataDesc*)GHEAP_ALLOC(pheap, sizeof(DataDesc)+ size, GStat_String_Mem);  
    pdesc->Data[size] = 0;
    pdesc->RefCount = 1;
    pdesc->Size     = size | lengthIsSize;  
    return pdesc;
}


GString::DataDesc* GString::AllocDataCopy1(GMemoryHeap* pheap, UPInt size, UPInt lengthIsSize,
                                              const char* pdata, UPInt copySize)
{
    GString::DataDesc* pdesc = AllocData(pheap, size, lengthIsSize);
    memcpy(pdesc->Data, pdata, copySize);
    return pdesc;
}

GString::DataDesc* GString::AllocDataCopy2(GMemoryHeap* pheap, UPInt size, UPInt lengthIsSize,
                                               const char* pdata1, UPInt copySize1,
                                               const char* pdata2, UPInt copySize2)
{
    GString::DataDesc* pdesc = AllocData(pheap, size, lengthIsSize);
    memcpy(pdesc->Data, pdata1, copySize1);
    memcpy(pdesc->Data + copySize1, pdata2, copySize2);
    return pdesc;
}



GMemoryHeap*    GString::GetHeap() const
{
    GMemoryHeap* pheap = 0;

    switch(GetHeapType())
    {
    case HT_Global:
        pheap = GMemory::GetGlobalHeap();
        break;
    case HT_Local:
        pheap = GMemory::GetHeapByAddress(this);
        break;
    case HT_Dynamic:
        pheap = ((const GStringDH*)this)->GetHeap();
        break;
    default:;
    }

    return pheap;
}



UPInt GString::GetLength() const 
{
    // Optimize length accesses for non-UTF8 character strings. 
    DataDesc* pdata = GetData();
    UPInt     length, size = pdata->GetSize();
    
    if (pdata->LengthIsSize())
        return size;    
    
    length = (UPInt)GUTF8Util::GetLength(pdata->Data, (UPInt)size);
    
    if (length == size)
        pdata->Size |= GString_LengthIsSize;
    
    return length;
}


//static UInt32 GString_CharSearch(const char* buf, )


UInt32 GString::GetCharAt(UPInt index) const 
{  
    SPInt       i = (SPInt) index;
    DataDesc*   pdata = GetData();
    const char* buf = pdata->Data;
    UInt32      c;
    
    if (pdata->LengthIsSize())
    {
        GASSERT(index < pdata->GetSize());
        buf += i;
        return GUTF8Util::DecodeNextChar(&buf);
    }

    // UTF-8 character search.
    do 
    {
        c = GUTF8Util::DecodeNextChar(&buf);
        i--;

        if (c == 0)
        {
            // We've hit the end of the string; don't go further.
            GASSERT(i == 0);
            return c;
        }
    } while (i >= 0);

    return c;
}

UInt32 GString::GetFirstCharAt(UPInt index, const char** offset) const
{
    DataDesc*   pdata = GetData();
    SPInt       i = (SPInt) index;
    const char* buf = pdata->Data;
    UInt32      c;

    do 
    {
        c = GUTF8Util::DecodeNextChar(&buf);
        i--;

        if (c == 0)
        {
            // We've hit the end of the string; don't go further.
            GASSERT(i == 0);
            return c;
        }
    } while (i >= 0);

    *offset = buf;

    return c;
}

UInt32 GString::GetNextChar(const char** offset) const
{
    return GUTF8Util::DecodeNextChar(offset);
}



void GString::AppendChar(UInt32 ch)
{
    DataDesc*   pdata = GetData();
    UPInt       size = pdata->GetSize();
    char        buff[8];
    SPInt       encodeSize = 0;

    // Converts ch into UTF8 string and fills it into buff.   
    GUTF8Util::EncodeChar(buff, &encodeSize, ch);
    GASSERT(encodeSize >= 0);

    SetData(AllocDataCopy2(GetHeap(), size + (UPInt)encodeSize, 0,
                           pdata->Data, size, buff, (UPInt)encodeSize));
    pdata->Release();
}


void GString::AppendString(const wchar_t* pstr, SPInt len)
{
    if (!pstr)
        return;

    DataDesc*   pdata = GetData();
    UPInt       oldSize = pdata->GetSize();    
    UPInt       encodeSize = (UPInt)GUTF8Util::GetEncodeStringSize(pstr, len);

    DataDesc*   pnewData = AllocDataCopy1(GetHeap(), oldSize + (UPInt)encodeSize, 0,
                                          pdata->Data, oldSize);
    GUTF8Util::EncodeString(pnewData->Data + oldSize,  pstr, len);

    SetData(pnewData);
    pdata->Release();
}


void GString::AppendString(const char* putf8str, SPInt utf8StrSz)
{
    if (!putf8str || !utf8StrSz)
        return;
    if (utf8StrSz == -1)
        utf8StrSz = (SPInt)G_strlen(putf8str);

    DataDesc*   pdata = GetData();
    UPInt       oldSize = pdata->GetSize();

    SetData(AllocDataCopy2(GetHeap(), oldSize + (UPInt)utf8StrSz, 0,
                           pdata->Data, oldSize, putf8str, (UPInt)utf8StrSz));
    pdata->Release();
}

void    GString::AssignString(const InitStruct& src, UPInt size)
{
    DataDesc*   poldData = GetData();
    DataDesc*   pnewData = AllocData(GetHeap(), size, 0);
    src.InitString(pnewData->Data, size);
    SetData(pnewData);
    poldData->Release();
}

void    GString::AssignString(const char* putf8str, UPInt size)
{
    DataDesc* poldData = GetData();
    SetData(AllocDataCopy1(GetHeap(), size, 0, putf8str, size));
    poldData->Release();
}

void    GString::operator = (const char* pstr)
{
    AssignString(pstr, pstr ? G_strlen(pstr) : 0);
}

void    GString::operator = (const wchar_t* pwstr)
{
    DataDesc*   poldData = GetData();
    UPInt       size = pwstr ? (UPInt)GUTF8Util::GetEncodeStringSize(pwstr) : 0;

    DataDesc*   pnewData = AllocData(GetHeap(), size, 0);
    GUTF8Util::EncodeString(pnewData->Data, pwstr);
    SetData(pnewData);
    poldData->Release();
}


void    GString::operator = (const GString& src)
{ 
    GMemoryHeap* pheap = GetHeap();
    DataDesc*    psdata = src.GetData();
    DataDesc*    pdata = GetData();    

    if (pheap == src.GetHeap())
    {        
        SetData(psdata);
        psdata->AddRef();
    }
    else
    {
        SetData(AllocDataCopy1(pheap, psdata->GetSize(), psdata->GetLengthFlag(),
                               psdata->Data, psdata->GetSize()));
    }

    pdata->Release();
}


void    GString::operator = (const GStringBuffer& src)
{ 
    DataDesc* polddata = GetData();    
    SetData(AllocDataCopy1(GetHeap(), src.GetSize(), 0, src.ToCStr(), src.GetSize()));
    polddata->Release();
}

void    GString::operator += (const GString& src)
{
    DataDesc   *pourData = GetData(),
               *psrcData = src.GetData();
    UPInt       ourSize  = pourData->GetSize(),
                srcSize  = psrcData->GetSize();
    UPInt       lflag    = pourData->GetLengthFlag() & psrcData->GetLengthFlag();

    SetData(AllocDataCopy2(GetHeap(), ourSize + srcSize, lflag,
                            pourData->Data, ourSize, psrcData->Data, srcSize));
    pourData->Release();
}


GString   GString::operator + (const char* str) const
{   
    GString tmp1(*this);
    tmp1 += (str ? str : "");
    return tmp1;
}

GString   GString::operator + (const GString& src) const
{ 
    GString tmp1(*this);
    tmp1 += src;
    return tmp1;
}

void    GString::Remove(UPInt posAt, SPInt removeLength)
{
    DataDesc*   pdata = GetData();
    UPInt       oldSize = pdata->GetSize();    
    // Length indicates the number of characters to remove. 
    UPInt       length = GetLength();

    // If index is past the string, nothing to remove.
    if (posAt >= length)
        return;
    // Otherwise, cap removeLength to the length of the string.
    if ((posAt + removeLength) > length)
        removeLength = length - posAt;

    // Get the byte position of the UTF8 char at position posAt.
    SPInt bytePos    = GUTF8Util::GetByteIndex(posAt, pdata->Data);
    SPInt removeSize = GUTF8Util::GetByteIndex(removeLength, pdata->Data + bytePos);

    SetData(AllocDataCopy2(GetHeap(), oldSize - removeSize, pdata->GetLengthFlag(),
                           pdata->Data, bytePos,
                           pData->Data + bytePos + removeSize, (oldSize - bytePos - removeSize)));
    pdata->Release();
}


GString   GString::Substring(UPInt start, UPInt end) const
{
    UPInt length = GetLength();
    if ((start >= length) || (start >= end))
        return GString();   

    DataDesc* pdata = GetData();
    
    // If size matches, we know the exact index range.
    if (pdata->LengthIsSize())
        return GString(pdata->Data + start, end - start);
    
    // Get position of starting character.
    SPInt byteStart = GUTF8Util::GetByteIndex(start, pdata->Data);
    SPInt byteSize  = GUTF8Util::GetByteIndex(end - start, pdata->Data + byteStart);
    return GString(pdata->Data + byteStart, (UPInt)byteSize);
}

void GString::Clear()
{   
    NullData.AddRef();
    GetData()->Release();
    SetData(&NullData);
}


GString   GString::ToUpper() const 
{       
    UInt32      c;
    const char* psource = GetData()->Data;
    GString   str;      
    SPInt       bufferOffset = 0;
    char        buffer[512];    
    
    do {        
        do {
            c = GUTF8Util::DecodeNextChar(&psource);
            // If we've hit the end of the string, don't go further.
            if (c == 0)
                break;
            GUTF8Util::EncodeChar(buffer, &bufferOffset, G_towupper(wchar_t(c)));
        } while (bufferOffset < SPInt(sizeof(buffer)-8));

        // Append string a piece at a time.
        str.AppendString(buffer, bufferOffset);
        bufferOffset = 0;
    } while(c != 0);

    return str;
}

GString   GString::ToLower() const 
{
    UInt32      c;
    const char* psource = GetData()->Data;
    GString   str;    
    SPInt       bufferOffset = 0;
    char        buffer[512];    

    do {
        do {
            c = GUTF8Util::DecodeNextChar(&psource);
            // If we've hit the end of the string, don't go further.
            if (c == 0)
                break;
            GUTF8Util::EncodeChar(buffer, &bufferOffset, G_towlower(wchar_t(c)));
        } while (bufferOffset < SPInt(sizeof(buffer)-8));

        // Append string a piece at a time.
        str.AppendString(buffer, bufferOffset);
        bufferOffset = 0;
    } while(c != 0);

    return str;
}



GString& GString::Insert(const char* substr, UPInt posAt, SPInt strSize)
{
    DataDesc* poldData   = GetData();
    UPInt     oldSize    = poldData->GetSize();
    UPInt     insertSize = (strSize < 0) ? G_strlen(substr) : (UPInt)strSize;    
    UPInt     byteIndex  =  (poldData->LengthIsSize()) ?
                            posAt : (UPInt)GUTF8Util::GetByteIndex(posAt, poldData->Data);

    GASSERT(byteIndex <= oldSize);
    
    DataDesc* pnewData = AllocDataCopy2(GetHeap(), oldSize + insertSize, 0,
                                        poldData->Data, byteIndex, substr, insertSize);
    memcpy(pnewData->Data + byteIndex + insertSize,
           poldData->Data + byteIndex, oldSize - byteIndex);
    SetData(pnewData);
    poldData->Release();
    return *this;
}

/*
GString& GString::Insert(const UInt32* substr, UPInt posAt, SPInt len)
{
    for (SPInt i = 0; i < len; ++i)
    {
        UPInt charw = InsertCharAt(substr[i], posAt);
        posAt += charw;
    }
    return *this;
}
*/

UPInt GString::InsertCharAt(UInt32 c, UPInt posAt)
{
    char    buf[8];
    SPInt   index = 0;
    GUTF8Util::EncodeChar(buf, &index, c);
    GASSERT(index >= 0);
    buf[(UPInt)index] = 0;

    Insert(buf, posAt, index);
    return (UPInt)index;
}


int  GString::CompareNoCase(const char* a, const char* b)
{
    return G_stricmp(a, b);
}

int  GString::CompareNoCase(const char* a, const char* b, SPInt len)
{
    if (len)
    {
        SPInt f,l;
        SPInt slen = len;
        const char *s = b;
        do {
            f = (SPInt)G_tolower((int)(*(a++)));
            l = (SPInt)G_tolower((int)(*(b++)));
        } while (--len && f && (f == l) && *b != 0);

        if (f == l && (len != 0 || *b != 0))
        {
            f = (SPInt)slen;
            l = (SPInt)G_strlen(s);
            return int(f - l);
        }

        return int(f - l);
    }
    else
        return (0-(int)G_strlen(b));
}

// ***** Implement hash static functions

// Hash function
UPInt GString::BernsteinHashFunction(const void* pdataIn, UPInt size, UPInt seed)
{
    const UByte*    pdata   = (const UByte*) pdataIn;
    UPInt           h       = seed;
    while (size > 0)
    {
        size--;
        h = ((h << 5) + h) ^ (UInt) pdata[size];
    }

    return h;
}

// Hash function, case-insensitive
UPInt GString::BernsteinHashFunctionCIS(const void* pdataIn, UPInt size, UPInt seed)
{
    const UByte*    pdata = (const UByte*) pdataIn;
    UPInt           h = seed;
    while (size > 0)
    {
        size--;
        h = ((h << 5) + h) ^ G_tolower(pdata[size]);
    }

    // Alternative: "sdbm" hash function, suggested at same web page above.
    // h = 0;
    // for bytes { h = (h << 16) + (h << 6) - hash + *p; }
    return h;
}


void GString::EscapeSpecialHTML(const char* psrc, UPInt length, GString* pescapedStr)
{
    GUNUSED(length);

    UInt32 ch;
    GStringBuffer buf;

    while((ch = GUTF8Util::DecodeNextChar(&psrc)) != 0)
    {
        if (ch == '<')
        {
            buf.AppendString("&lt;", 4);
        }
        else if (ch == '>')
        {
            buf.AppendString("&gt;", 4);
        }
        else if (ch == '\"')
        {
            buf.AppendString("&quot;", 6);
        }
        else if (ch == '\'')
        {
            buf.AppendString("&apos;", 6);
        }
        else if (ch == '&')
        {
            buf.AppendString("&amp;", 5);
        }
        else
        {
            buf.AppendChar(ch);
        }
    }
    *pescapedStr = buf;
}

void GString::UnescapeSpecialHTML(const char* psrc, UPInt length, GString* punescapedStr)
{
    GUNUSED(length);

    UInt32 ch;
    GStringBuffer buf;

    while ((ch = GUTF8Util::DecodeNextChar(&psrc)) != 0)
    {
        if (ch == '&')
        {
            if (strncmp(psrc, "quot;", 5) == 0)
            {
                buf.AppendChar('\"');
                psrc += 5;
                break;
            }
            else if (strncmp(psrc, "apos;", 5) == 0)
            {
                buf.AppendChar('\'');
                psrc += 5;
                break;
            }
            else if (strncmp(psrc, "amp;", 4) == 0)
            {
                buf.AppendChar('&');
                psrc += 4;
                break;
            }
            else if (strncmp(psrc, "lt;", 3) == 0)
            {
                buf.AppendChar('<');
                psrc += 3;
                break;
            }
            else if (strncmp(psrc, "gt;", 3) == 0)
            {
                buf.AppendChar('>');
                psrc += 3;
                break;
            }
            else
                buf.AppendChar(ch);
        }
        else
            buf.AppendChar(ch);
    }
    *punescapedStr = buf;
}



// ***** Local String - GStringLH

GStringLH::GStringLH() : GString(GString::NoConstructor())
{
    NullData.AddRef();
    SetDataLcl(&NullData);
};

GStringLH::GStringLH(const char* pdata) : GString(GString::NoConstructor())
{
    // Obtain length in bytes; it doesn't matter if _data is UTF8.
    UPInt size = pdata ? G_strlen(pdata) : 0; 
    SetDataLcl(AllocDataCopy1(GMemory::GetHeapByAddress(this), size, 0, pdata, size));
};

GStringLH::GStringLH(const char* pdata, UPInt size) : GString(GString::NoConstructor())
{
    GASSERT((size == 0) || (pdata != 0));
    SetDataLcl(AllocDataCopy1(GMemory::GetHeapByAddress(this), size, 0, pdata, size));
};

GStringLH::GStringLH(const InitStruct& src, UPInt size) : GString(GString::NoConstructor())
{
    SetDataLcl(AllocData(GMemory::GetHeapByAddress(this), size, 0));
    src.InitString(GetData()->Data, size);
}

// Called for both Copy constructor and GString initializer.
void GStringLH::CopyConstructHelper(const GString& src)
{
    DataDesc*       pdata = src.GetData();   
    GMemoryHeap*    pheap = GMemory::GetHeapByAddress(this);

    // We can only AddRef if string belongs to the same local heap.    
    if (src.GetHeap() == pheap)
    {   
        pdata->AddRef();
        SetDataLcl(pdata);
    }
    else
    {
        UPInt size = pdata->GetSize();
        SetDataLcl(AllocDataCopy1(pheap,size,pdata->GetLengthFlag(),
                                  pdata->Data, size));
    }
}

GStringLH::GStringLH(const wchar_t* pdata)
{
    // Allow BASE constructor for data.
    // NullData.AddRef();
    // SetDataLcl(&NullData);    
    if (pdata)
        *this = pdata;
}


// ***** Dynamic heap string - GStringDH

GStringDH::GStringDH(GMemoryHeap* pheap)
    : GString(GString::NoConstructor())
{
    pHeap = pheap;
    NullData.AddRef();
    SetDataLcl(&NullData);    
};

GStringDH::GStringDH(GMemoryHeap* pheap, const char* pdata)
    : GString(GString::NoConstructor())
{
    // Obtain length in bytes; it doesn't matter if _data is UTF8.
    UPInt size = pdata ? G_strlen(pdata) : 0; 
    pHeap = pheap;
    SetDataLcl(AllocDataCopy1(pheap, size, 0, pdata, size));
};

GStringDH::GStringDH(GMemoryHeap* pheap,
                     const char* pdata1, const char* pdata2, const char* pdata3)
{
     // Obtain length in bytes; it doesn't matter if _data is UTF8.
    UPInt size1 = pdata1 ? G_strlen(pdata1) : 0; 
    UPInt size2 = pdata2 ? G_strlen(pdata2) : 0; 
    UPInt size3 = pdata3 ? G_strlen(pdata3) : 0; 

    DataDesc *pdataDesc = AllocDataCopy2(pheap,
                                         size1 + size2 + size3, 0,
                                         pdata1, size1, pdata2, size2);
    memcpy(pdataDesc->Data + size1 + size2, pdata3, size3);
    pHeap = pheap;
    SetDataLcl(pdataDesc);    
}

GStringDH::GStringDH(GMemoryHeap* pheap, const char* pdata, UPInt size)
     : GString(GString::NoConstructor())
{
    GASSERT((size == 0) || (pdata != 0));
    pHeap = pheap;
    SetDataLcl(AllocDataCopy1(pheap, size, 0, pdata, size));
};

GStringDH::GStringDH(GMemoryHeap* pheap, const InitStruct& src, UPInt size)
     : GString(GString::NoConstructor())
{
    pHeap = pheap;
    SetDataLcl(AllocData(pheap, size, 0));
    src.InitString(GetData()->Data, size);
}

// Called for both Copy constructor and GString initializer.
void GStringDH::CopyConstructHelper(const GString& src, GMemoryHeap* pheap)
{
    DataDesc*     pdata = src.GetData();
    GMemoryHeap*  psrcHeap = src.GetHeap();

    // We allow null here to indicate heap copy.
    if (!pheap)
        pheap = psrcHeap;
    pHeap = pheap;

    // We can only AddRef if string belongs to the same local heap.    
    if (psrcHeap == pheap)
    {   
        pdata->AddRef();
        SetDataLcl(pdata);
    }
    else
    {
        UPInt size = pdata->GetSize();
        SetDataLcl(AllocDataCopy1(pheap,size,pdata->GetLengthFlag(),
                                  pdata->Data, size));
    }
}

GStringDH::GStringDH(GMemoryHeap* pheap, const wchar_t* pdata)
{
    // Allow BASE constructor for data.
    // NullData.AddRef();
    // SetDataLcl(&NullData);
    pHeap = pheap;
    if (pdata)
        *this = pdata;
}



// ***** GString Buffer used for Building Strings


#define DEFAULD_GROW_SIZE 512
// Constructors / Destructor.
GStringBuffer::GStringBuffer(GMemoryHeap* pheap)
    : pData(NULL), Size(0), BufferSize(0), GrowSize(DEFAULD_GROW_SIZE), LengthIsSize(false), pHeap(pheap)
{
}

GStringBuffer::GStringBuffer(UPInt growSize, GMemoryHeap* pheap)
    : pData(NULL), Size(0), BufferSize(0), GrowSize(DEFAULD_GROW_SIZE), LengthIsSize(false), pHeap(pheap)
{
    SetGrowSize(growSize);
}

GStringBuffer::GStringBuffer(const char* data, GMemoryHeap* pheap)
    : pData(NULL), Size(0), BufferSize(0), GrowSize(DEFAULD_GROW_SIZE), LengthIsSize(false), pHeap(pheap)
{
    *this = data;
}

GStringBuffer::GStringBuffer(const char* data, UPInt dataSize, GMemoryHeap* pheap)
    : pData(NULL), Size(0), BufferSize(0), GrowSize(DEFAULD_GROW_SIZE), LengthIsSize(false), pHeap(pheap)
{
    AppendString(data, dataSize);
}

GStringBuffer::GStringBuffer(const GString& src, GMemoryHeap* pheap)
    : pData(NULL), Size(0), BufferSize(0), GrowSize(DEFAULD_GROW_SIZE), LengthIsSize(false), pHeap(pheap)
{
    AppendString(src.ToCStr(), src.GetSize());
}

GStringBuffer::GStringBuffer(const GStringBuffer& src, GMemoryHeap* pheap)
    : pData(NULL), Size(0), BufferSize(src.GetGrowSize()), GrowSize(DEFAULD_GROW_SIZE), LengthIsSize(false), pHeap(pheap)
{
    AppendString(src.ToCStr(), src.GetSize());
    LengthIsSize = src.LengthIsSize;
}

GStringBuffer::GStringBuffer(const wchar_t* data, GMemoryHeap* pheap)
    : pData(NULL), Size(0), BufferSize(0), GrowSize(DEFAULD_GROW_SIZE), LengthIsSize(false), pHeap(pheap)
{
    *this = data;
}

GStringBuffer::~GStringBuffer()
{
    if (pData)
        GHEAP_FREE(pHeap, pData);
}
void GStringBuffer::SetGrowSize(UPInt growSize) 
{ 
    if (growSize <= 16)
        GrowSize = 16;
    else
    {
        UInt8 bits = GBitsUtil::BitCount32(UInt32(growSize-1));
        UPInt size = 1<<bits;
        GrowSize = size == growSize ? growSize : size;
    }
}

UPInt GStringBuffer::GetLength() const
{
    UPInt length, size = GetSize();
    if (LengthIsSize)
        return size;

    length = (UPInt)GUTF8Util::GetLength(pData, (UPInt)GetSize());

    if (length == GetSize())
        LengthIsSize = true;
    return length;
}

void    GStringBuffer::Reserve(UPInt _size)
{
    if (_size >= BufferSize) // >= because of trailing zero! (!AB)
    {
        BufferSize = (_size + 1 + GrowSize - 1)& ~(GrowSize-1);
        if (!pData)
            pData = (char*)GHEAP_ALLOC(pHeap, BufferSize,  GStat_String_Mem);
        else 
            pData = (char*)GREALLOC(pData, BufferSize, GStat_String_Mem);
    }
}
void    GStringBuffer::Resize(UPInt _size)
{
    Reserve(_size);
    LengthIsSize = false;
    Size = _size;
    if (pData)
        pData[Size] = 0;
}

void GStringBuffer::Clear()
{
    Resize(0);
    /*
    if (pData != pEmptyNullData)
    {
        GHEAP_FREE(pHeap, pData);
        pData = pEmptyNullData;
        Size = BufferSize = 0;
        LengthIsSize = false;
    }
    */
}
// Appends a character
void     GStringBuffer::AppendChar(UInt32 ch)
{
    char    buff[8];
    UPInt   origSize = GetSize();

    // Converts ch into UTF8 string and fills it into buff. Also increments index according to the number of bytes
    // in the UTF8 string.
    SPInt   srcSize = 0;
    GUTF8Util::EncodeChar(buff, &srcSize, ch);
    GASSERT(srcSize >= 0);
    
    UPInt size = origSize + srcSize;
    Resize(size);
    memcpy(pData + origSize, buff, srcSize);
}

// Append a string
void     GStringBuffer::AppendString(const wchar_t* pstr, SPInt len)
{
    if (!pstr)
        return;

    SPInt   srcSize     = GUTF8Util::GetEncodeStringSize(pstr, len);
    UPInt   origSize    = GetSize();
    UPInt   size        = srcSize + origSize;

    Resize(size);
    GUTF8Util::EncodeString(pData + origSize,  pstr, len);
}

void      GStringBuffer::AppendString(const char* putf8str, SPInt utf8StrSz)
{
    if (!putf8str || !utf8StrSz)
        return;
    if (utf8StrSz == -1)
        utf8StrSz = (SPInt)G_strlen(putf8str);

    UPInt   origSize    = GetSize();
    UPInt   size        = utf8StrSz + origSize;

    Resize(size);
    memcpy(pData + origSize, putf8str, utf8StrSz);
}


void      GStringBuffer::operator = (const char* pstr)
{
    pstr = pstr ? pstr : "";
    UPInt size = G_strlen(pstr);
    Resize(size);
    memcpy(pData, pstr, size);
}

void      GStringBuffer::operator = (const wchar_t* pstr)
{
    pstr = pstr ? pstr : L"";
    UPInt size = (UPInt)GUTF8Util::GetEncodeStringSize(pstr);
    Resize(size);
    GUTF8Util::EncodeString(pData, pstr);
}

void      GStringBuffer::operator = (const GString& src)
{
    Resize(src.GetSize());
    memcpy(pData, src.ToCStr(), src.GetSize());
}


// Inserts substr at posAt
void      GStringBuffer::Insert(const char* substr, UPInt posAt, SPInt len)
{
    UPInt     oldSize    = Size;
    UPInt     insertSize = (len < 0) ? G_strlen(substr) : (UPInt)len;    
    UPInt     byteIndex  = LengthIsSize ? posAt : (UPInt)GUTF8Util::GetByteIndex(posAt, pData);

    GASSERT(byteIndex <= oldSize);
    Reserve(oldSize + insertSize);

    memmove(pData + byteIndex + insertSize, pData + byteIndex, oldSize - byteIndex + 1);
    memcpy (pData + byteIndex, substr, insertSize);
    LengthIsSize = false;
    Size = oldSize + insertSize;
    pData[Size] = 0;
}

// Inserts character at posAt
UPInt     GStringBuffer::InsertCharAt(UInt32 c, UPInt posAt)
{
    char    buf[8];
    SPInt   len = 0;
    GUTF8Util::EncodeChar(buf, &len, c);
    GASSERT(len >= 0);
    buf[(UPInt)len] = 0;

    Insert(buf, posAt, len);
    return (UPInt)len;
}

