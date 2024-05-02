/**********************************************************************

Filename    :   GFxASString.cpp
Content     :   String manager implementation for Action Script
Created     :   Movember 7, 2006
Authors     :   Michael Antonov

Notes       :   Implements optimized GASString class, which acts as a
                hash key for strings allocated from GASStringManager.

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxASString.h"

// Log for leak reports
#include "GFxLog.h"
#include "GFxPlayerStats.h"


// ***** GASString implementation

// Static hash function calculation routine
UInt32  GASString::HashFunction(const char *pchar, UPInt length)
{
    return (UInt32)GString::BernsteinHashFunctionCIS(pchar, length) & Flag_HashMask;
}


void    GASString::Clear()
{
    AssignNode(GetManager()->GetEmptyStringNode());  
}

// Returns length in characters.
UInt    GASString::GetLength() const
{
    UInt length, size = GetSize();    
   
    // Optimize length accesses for non-UTF8 character strings.
    if (GetHashFlags() & Flag_LengthIsSize)
        return size;    
    length = (UInt)GUTF8Util::GetLength(GetBuffer(), GetSize());
    if (length == size)
        pNode->HashFlags |= Flag_LengthIsSize;
    return length;
}

UInt32  GASString::GetCharAt(UInt index) const
{
    if (GetHashFlags() & Flag_LengthIsSize)
    {
        return (UInt32) operator [](index);
    }

    int   i = (int) index;
    const char* buf = GetBuffer();
    UInt32  c;

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

// The rest of the functions here operate in UTF8. For example,    
GASString    GASString::AppendChar(UInt32 ch)
{
    char    buff[8];
    SPInt   index = 0;
    GUTF8Util::EncodeChar(buff, &index, ch);
    // Assign us a concatenated node.
    return GASString(GetManager()->CreateStringNode(pNode->pData, pNode->Size, buff, index));
}

/*
void    GASString::Remove(int posAt, int len)
{
    GUNUSED2(posAt, len);
    GASSERT(0);
} */

// Returns a GString that's a substring of this.
//  -start is the index of the first UTF8 character you want to include.    
//  -end is the index one past the last UTF8 character you want to include.
GASString   GASString::Substring(int start, int end) const
{
    GASSERT(start <= end);

    // Special case, always return empty string.
    if (start == end)
        return GetManager()->CreateEmptyString();

    int         index = 0;
    const char* p = GetBuffer();   
    const char* start_pointer = p;

    const char* end_pointer = p;

    for (;;)
    {
        if (index == start)
        {
            start_pointer = p;
        }

        UInt32  c = GUTF8Util::DecodeNextChar(&p);
        index++;

        if (index == end)
        {
            end_pointer = p;
            break;
        }

        if (c == 0)
        {
            //!AB if the string is shorter than "end" param - then just
            // return a whole string as a substring.
            if (index < end)
            {
                end_pointer = p;
            }
            break;
        }
    }

    if (end_pointer < start_pointer)
    {
        end_pointer = start_pointer;
    }

    return GetManager()->CreateString(start_pointer, (int) (end_pointer - start_pointer));
}

// Case-converted strings.
GASString   GASString::ToUpper() const
{
    GString str = GString(pNode->pData).ToUpper();
    return GetManager()->CreateString(str, str.GetSize());
}

GASString   GASString::ToLower() const
{
    ResolveLowercase();
    return GASString(pNode->pLower ? pNode->pLower : GetManager()->GetEmptyStringNode());
}


// *** Operations

void    GASString::operator = (const char* pstr)
{
    AssignNode(GetManager()->CreateStringNode(pstr, strlen(pstr)));
}

void        GASString::operator += (const char* pstr)
{
    AssignNode(GetManager()->CreateStringNode(pNode->pData, pNode->Size,
                                              pstr, strlen(pstr)));
}
void        GASString::operator += (const GASString& str)
{
    AssignNode(GetManager()->CreateStringNode(pNode->pData, pNode->Size,
                                              str.pNode->pData, str.pNode->Size));
}

GASString   GASString::operator + (const char* pstr) const
{
    return GASString(GetManager()->CreateStringNode(pNode->pData, pNode->Size,
                                                    pstr, strlen(pstr)));
}
GASString   GASString::operator + (const GASString& str) const
{
    return GASString(GetManager()->CreateStringNode(pNode->pData, pNode->Size,
                                                    str.pNode->pData, str.pNode->Size));
}

int GASString::LocaleCompare_CaseCheck(const char* pstr, UPInt len, bool caseSensitive) const
{
    if (len == GFC_MAX_UPINT)
        len = G_strlen(pstr);

    wchar_t buf1[250], buf2[250];
    wchar_t *pwstra = NULL, *pwstrb = NULL;
    
    UPInt la = GetLength();
    if (la >= sizeof(buf1)/sizeof(buf1[0]))
        pwstra = (wchar_t*)GALLOC(sizeof(wchar_t) * (la + 1), GStat_Default_Mem);
    else
        pwstra = buf1;

    if (len >= sizeof(buf2)/sizeof(buf2[0]))
        pwstrb = (wchar_t*)GALLOC(sizeof(wchar_t) * (len + 1), GStat_Default_Mem);
    else
        pwstrb = buf2;

    GUTF8Util::DecodeString(pwstra, ToCStr(), la);
    GUTF8Util::DecodeString(pwstrb, pstr, len);

    int res;
    if (caseSensitive)
        res = G_wcscoll(pwstra, pwstrb);
    else
        res = G_wcsicoll(pwstra, pwstrb);
    if (pwstra != buf1)
        GFREE(pwstra);
    if (pwstrb != buf2)
        GFREE(pwstrb);
    return res;
}

// ***** Node implementation


// Resolves pLower to a valid value.
void    GASStringNode::ResolveLowercase_Impl()
{   
    GString             str = GString(pData).ToLower();
    GASStringManager*   pmanager = GetManager();
    GASStringNode*      pnode = pmanager->CreateStringNode(str);
    
    if (pnode != pmanager->GetEmptyStringNode())
    {
        pLower = pnode;
        if (pnode != this)
            pnode->AddRef();
    }    
}

// Frees the node whose refCount has reached 0.
void   GASStringNode::ReleaseNode()
{
    GASSERT(RefCount == 0);

    // If we had a lowercase version, release its ref count.
    if ((pLower != this) && pLower)
        pLower->Release();
    GASStringManager*  pmanager = GetManager();
    pmanager->StringSet.Remove(this);
    pmanager->FreeStringNode(this);
}




// ***** GASStringManager

GASStringManager::GASStringManager(GMemoryHeap* pheap)
    : pHeap(pheap)
{
    // Allocate initial data.    
    pStringNodePages = 0;
    pFreeStringNodes = 0;

    pFreeTextBuffers = 0;
    pTextBufferPages = 0;
    
    // Empty data - refcount 1, so it is never released.
    // This node must me allocated with its GetManager() implementation relies on alignment.
    pEmptyStringNode = AllocStringNode();
    if (pEmptyStringNode)
    {
        pEmptyStringNode->RefCount    = 1;
        pEmptyStringNode->Size        = 0;
        pEmptyStringNode->HashFlags   = GASString::HashFunction("", 0) |
                                        GASString::Flag_ConstData | GASString::Flag_Builtin;
        pEmptyStringNode->pData       =  "";
        pEmptyStringNode->pLower      = pEmptyStringNode;
    }    

    StringSet.Add(pEmptyStringNode);
}

GASStringManager::~GASStringManager()
{    
    // Release string nodes.
    GStringBuffer leakReport;
    UInt            ileaks = 0;

    while(pStringNodePages)
    {
        StringNodePage* ppage = pStringNodePages;
        pStringNodePages = ppage->pNext;

        // Free text in all active GASString objects. Technically this is a bug, since 
        // all GASString should have already died before GASStringManager is destroyed.
        // However, if ActionScript leaks due to circular references, this will at least
        // release all of the string content involved.

        // In the future, once we have GC this may be changed to throw asserts if
        // unreleased data / RefCount is detected at this point.

        for (UInt i=0; i< ppage->StringNodeCount; i++)
        {
            if (ppage->Nodes[i].pData && (&ppage->Nodes[i] != pEmptyStringNode))
            {   
                if (ileaks < 16)
                {
                    leakReport += (ileaks > 0) ? ", '" : "'";
                    leakReport += ppage->Nodes[i].pData;
                    leakReport += "'";
                }
                ileaks++;

                if (!(ppage->Nodes[i].HashFlags & GASString::Flag_ConstData))
                {
                    FreeTextBuffer(const_cast<char*>(ppage->Nodes[i].pData), ppage->Nodes[i].Size);
                }
            }
        }
        GHEAP_FREE(pHeap, ppage);
    }

    // Free text pages.
    while(pTextBufferPages)
    {
        void *pmem = pTextBufferPages->pMem;
        pTextBufferPages = pTextBufferPages->pNext;        
        GHEAP_FREE(pHeap, pmem);
    }


    if (ileaks && pLog.GetPtr())
    {
        pLog->LogScriptError("ActionScript Memory leaks in movie '%s', including %d string nodes\n",
                             FileName.ToCStr(), ileaks);
        pLog->LogScriptError("Leaked string content: %s\n", leakReport.ToCStr());
    }    
}


// Sets the log that will be used to report leaks during destructor.    
void    GASStringManager::SetLeakReportLog(GFxLog *plog, const char *pfilename)
{
    pLog    = plog;
    FileName= pfilename ? pfilename : "";
}

void    GASStringManager::AllocateStringNodes()
{
    StringNodePage* ppage = (StringNodePage*)
        GHEAP_MEMALIGN(pHeap, sizeof(StringNodePage), StringNodePage::StringNodePageAlign,
                       GFxStatMV_ActionScript_Mem);

    if (ppage)
    {
        ppage->pNext    = pStringNodePages;
        ppage->pManager = this;
        pStringNodePages = ppage;
        // Add nodes to free pool.
        for (UInt i=0; i < StringNodePage::StringNodeCount; i++)
        {
            GASStringNode *pnode = &ppage->Nodes[i];
            // Clear data so that release detection fails on it.
            pnode->pData    = 0;
            
            // Insert into free list.
            pnode->pNextAlloc = pFreeStringNodes;
            pFreeStringNodes  = pnode;
        }
    }
    else
    {
        // Not enough memory for string nodes!!
        GASSERT(1);
    }
}

void   GASStringManager::AllocateTextBuffers()
{
    // Align data (some of our allocation routines add size counters
    // that may case this to be misaligned at a multiple of 4 instead).
    // Seems to make a difference on P4 chips.
    void*     pp    = GHEAP_ALLOC(pHeap, sizeof(TextPage) + 8, GFxStatMV_ActionScript_Mem);    
    TextPage* ppage = (TextPage*) ((((UPInt)pp)+7) & ~7);

    if (ppage)
    {
        ppage->pMem = pp;

        ppage->pNext = pTextBufferPages;
        pTextBufferPages = ppage;
        // Add nodes to free pool.
        for (UInt i=0; i < TextPage::BuffCount; i++)
        {
            TextPage::Entry *pe = ppage->Entries + i;
            pe->pNextAlloc = pFreeTextBuffers;
            pFreeTextBuffers  = pe;
        }
    }
    else
    {
        // Not enough memory for text entries!!
        GASSERT(1);
    }
}


// Get buffer for text (adds 0 to length).
char*   GASStringManager::AllocTextBuffer(UPInt length)
{   
    if (length < TextPage::BuffSize)
    {
        if (!pFreeTextBuffers)
            AllocateTextBuffers();

        char *pbuff = 0;        
        if (pFreeTextBuffers)
        {
            pbuff = pFreeTextBuffers->Buff;
            pFreeTextBuffers = pFreeTextBuffers->pNextAlloc;
        }
        return pbuff;
    }
    // If bigger then the size of built-in buffer entry, allocate separately.
    return (char*)GHEAP_ALLOC(pHeap, length + 1, GFxStatMV_ActionScript_Mem);
}

void    GASStringManager::FreeTextBuffer(char* pbuffer, UPInt length)
{
    // This is never called with null pbuffer.
    GASSERT(pbuffer);

    if (length < TextPage::BuffSize)
    {
        // delta should be 0, but just in case of a strange compiler...
        ptrdiff_t        delta  = &reinterpret_cast<TextPage::Entry*>(0)->Buff[0] - (char*)0;
        TextPage::Entry* pentry = reinterpret_cast<TextPage::Entry*>(pbuffer - delta);

        pentry->pNextAlloc = pFreeTextBuffers;
        pFreeTextBuffers = pentry;
        return;
    }
    
    GHEAP_FREE(pHeap, pbuffer);
}

char*   GASStringManager::AllocTextBuffer(const char* pbuffer, UPInt length)
{
    char *pbuff = AllocTextBuffer(length);
    if (pbuff)
    {
        memcpy(pbuff, pbuffer, length);
        pbuff[length] = 0;
    }
    return pbuff;
}



// *** String node management


bool operator == (GASStringNode* pnode, const GASStringKey &str)
{
    if (pnode->Size != str.Length)
        return 0;
    return strncmp(pnode->pData, str.pStr, str.Length) == 0;
}


GASStringNode*  GASStringManager::CreateConstStringNode(const char* pstr, UPInt length, UInt32 stringFlags)
{    
    GASStringNode*  pnode;
    GASStringKey    key(pstr, GASString::HashFunction(pstr, length), length);

    if (StringSet.GetAlt(key, &pnode))
    {
        // Must set flags, otherwise setting could be lost quietly for strings
        // that are already registered, such as built-ins.
        pnode->HashFlags |= stringFlags;
        return pnode;
    }

    if ((pnode = AllocStringNode()) != 0)
    {        
        pnode->RefCount = 0;
        pnode->Size     = (UInt)length;
        pnode->pData    = pstr;
        pnode->HashFlags= (UInt32)(key.HashValue | GASString::Flag_ConstData | stringFlags);
        pnode->pLower   = 0;
        
        StringSet.Add(pnode);
        return pnode;
    }

    // Return dummy string node.
    return pEmptyStringNode;
}


GASStringNode*  GASStringManager::CreateStringNode(const char* pstr, UPInt length)
{
    GASStringNode*  pnode;
    GASStringKey    key(pstr, GASString::HashFunction(pstr, length), length);

    if (StringSet.GetAlt(key, &pnode))
        return pnode;

    if ((pnode = AllocStringNode()) != 0)
    { 
        pnode->pData = AllocTextBuffer(pstr, length);

        if (pnode->pData)
        {
            pnode->RefCount = 0;
            pnode->Size     = (UInt)length;
            pnode->HashFlags= (UInt32)key.HashValue;
            pnode->pLower   = 0;

            StringSet.Add(pnode);
            return pnode;
        }
        else
        {
            FreeStringNode(pnode);
        }
    }
    
    return pEmptyStringNode;
}

GASStringNode*  GASStringManager::CreateStringNode(const char* pstr)
{
    if (!pstr)
        return pEmptyStringNode;
    return CreateStringNode(pstr, strlen(pstr));
}

// Wide character; use special type of counting.
GASStringNode*  GASStringManager::CreateStringNode(const wchar_t* pwstr)
{
//    GString strBuff;
//    GString::EncodeUTF8FromWChar(&strBuff, pwstr);
    GString strBuff;
    strBuff = pwstr;
    return CreateStringNode(strBuff.ToCStr(), strBuff.GetSize());
}


// These functions also perform string concatenation.
GASStringNode*  GASStringManager::CreateStringNode(const char* pstr1, UPInt l1,
                                                   const char* pstr2, UPInt l2)
{
    GASStringNode*  pnode;
    UPInt           length = l1 + l2;    
    char*           pbuffer = AllocTextBuffer(length);
    
    if (pbuffer)
    {
        if (l1 > 0) memcpy(pbuffer, pstr1, l1);
        if (l2 > 0) memcpy(pbuffer + l1, pstr2, l2);
        pbuffer[length] = 0;

        GASStringKey   key(pbuffer, GASString::HashFunction(pbuffer, length), length);

        // If already exists, no need for node.
        if (StringSet.GetAlt(key, &pnode))
        {
            FreeTextBuffer(pbuffer, length);
            return pnode;
        }

        // Create node.
        if ((pnode = AllocStringNode()) != 0)
        {             
            pnode->RefCount = 0;
            pnode->Size     = (UInt)length;
            pnode->pData    = pbuffer;
            pnode->HashFlags= (UInt32)key.HashValue;
            pnode->pLower   = 0;

            StringSet.Add(pnode);
            return pnode;
        }
        else
        {
            FreeTextBuffer(pbuffer, length);
        }
    }

    return pEmptyStringNode;
}


GASStringNode*  GASStringManager::CreateStringNode(const char* pstr1, UPInt l1,
                                                   const char* pstr2, UPInt l2,
                                                   const char* pstr3, UPInt l3)
{
    GASStringNode*  pnode;
    UPInt           length = l1 + l2 + l3;
    char*           pbuffer = AllocTextBuffer(length);

    if (pbuffer)
    {
        if (l1 > 0) memcpy(pbuffer, pstr1, l1);
        if (l2 > 0) memcpy(pbuffer + l1, pstr2, l2);
        if (l3 > 0) memcpy(pbuffer + l1 + l2, pstr3, l3);
        pbuffer[length] = 0;

        GASStringKey   key(pbuffer, GASString::HashFunction(pbuffer, length), length);
        
        // If already exists, no need for node.
        if (StringSet.GetAlt(key, &pnode))
        {
            FreeTextBuffer(pbuffer, length);
            return pnode;
        }

        // Create node.
        if ((pnode = AllocStringNode()) != 0)
        {             
            pnode->RefCount = 0;
            pnode->Size     = (UInt)length;
            pnode->pData    = pbuffer;
            pnode->HashFlags= (UInt32)key.HashValue;
            pnode->pLower   = 0;

            StringSet.Add(pnode);
            return pnode;
        }
        else
        {
            FreeTextBuffer(pbuffer, length);
        }
    }

    return pEmptyStringNode;
}



