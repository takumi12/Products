/**********************************************************************

Filename    :   GFxASString.h
Content     :   String manager implementation for Action Script
Created     :   Movember 7, 2006
Authors     :   Michael Antonov

Notes       :    Implements optimized GASString class, which acts as a
hash key for strings allocated from GASStringManager.

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXASSTRING_H
#define INC_GFXASSTRING_H

#include "GTypes.h"
#include "GArray.h"
#include "GHash.h"
#include "GRefCount.h"

#include "GUTF8Util.h"
#include "GFxString.h"

#include "GFxPlayerStats.h"

#include <string.h>
#ifdef GFC_OS_PS3
# include <wctype.h>
#endif // GFC_OS_PS3


// ***** Classes Declared

class   GASString;
class   GASStringManager;
struct  GASStringNode;

// Log forward declaration - for leak report
class   GFxLog;


// String node - stored in the manager table.

struct GASStringNode
{
    const char*         pData;
    union {
        GASStringNode*  pLower;
        GASStringNode*  pNextAlloc;
    };
    UInt32              RefCount;
    UInt32              HashFlags;
    UInt                Size;


    // *** Utility functions

    inline UInt32  GetHashCode()
    {
        return HashFlags & 0x00FFFFFF;
    }

    void    AddRef()
    {
        RefCount++;
    }
    void    Release()
    {
        if (--RefCount == 0)
            ReleaseNode();
    }

    // Releases the node to the manager.
    void    ReleaseNode();    
    // Resolves pLower to a valid value.    
    void    ResolveLowercase_Impl();

    void    ResolveLowercase()
    {
        // This should not be called if lowercase was already resolved,
        // or we would overwrite it.
        GASSERT(pLower == 0);
        ResolveLowercase_Impl();
    }

    inline GASStringManager* GetManager() const;
};




// ***** GASString - ActionScript string implementation.

// GASString is represented as a pointer to a unique node so that
// comparisons can be implemented very fast. Nodes are allocated by
// and stored within GASStringManager.
// GASString objects can be created only by string manager; default
// constructor is not implemented.

struct GASStringNodeHolder
{
    GASStringNode*  pNode;
};

// Do not derive from GNewOverrideBase and do not new!
class GASString : public GASStringNodeHolder
{
    friend class GASStringManager;
    friend class GASStringBuiltinManager;

public:

    // *** Create/Destroy: can

    explicit GASString(GASStringNode *pnode)
    {
        pNode = pnode;
        pNode->AddRef();
    }

    GASString(const GASString& src)
    {
        pNode = src.pNode;
        pNode->AddRef();
    }
    ~GASString()
    {        
        pNode->Release();
    }

    // *** General Functions

    void        Clear();

    // Pointer to raw buffer.
    const char* ToCStr() const          { return pNode->pData; }
    const char* GetBuffer() const       { return pNode->pData; }

    // Size of string characters without 0. Raw count, not UTF8.
    UInt        GetSize() const         { return pNode->Size; }
    bool        IsEmpty() const         { return pNode->Size == 0; }

    UInt        GetHashFlags() const    { return pNode->HashFlags; }
    UInt        GetHash() const         { return GetHashFlags() & Flag_HashMask; }

    GASStringNode* GetNode() const      { return pNode; }
    GASStringManager* GetManager() const { return pNode->GetManager(); }


    static UInt32   HashFunction(const char *pchar, UPInt length);


    // *** UTF8 Aware functions.

    // Returns length in characters.
    UInt        GetLength() const;
    UInt32      GetCharAt(UInt index) const;

    // The rest of the functions here operate in UTF8. For example,    
    GASString   AppendChar(UInt32 ch);

    /* No Insert or Remove for now - not necessary.

    GASString& Insert(const GASString& substr, int posAt, int len = -1);
    GString&   Insert(const char* substr, int posAt, int len = -1);
    GString&   Insert(char ch, int posAt);

    void        Remove(int posAt, int len = 1);
    */  

    // Returns a GString that's a substring of this.
    //  -start is the index of the first UTF8 character you want to include.    
    //  -end is the index one past the last UTF8 character you want to include.
    GASString   Substring(int start, int end) const;

    // Case-converted strings.
    GASString   ToUpper() const; 
    GASString   ToLower() const;



    // *** Operators

    // Assignment.
    void    operator = (const char* str);
    GINLINE void    operator = (const GASString& src)
    {
        AssignNode(src.pNode);        
    }

    // Concatenation of string / UTF8.
    void        operator += (const char* str);
    void        operator += (const GASString& str);
    GASString   operator + (const char* str) const;
    GASString   operator + (const GASString& str) const;

    // Comparison.
    bool    operator == (const GASString& str) const
    {
        return pNode == str.pNode;
    }    
    bool    operator != (const GASString& str) const
    {
        return pNode != str.pNode;
    }
    bool    operator == (const char* str) const
    {
        return G_strcmp(pNode->pData, str) == 0;
    }
    bool    operator != (const char* str) const
    {
        return !operator == (str);
    }

    // Compares provide the same ordering for UTF8 due to
    // the nature of data.
    bool    operator<(const char* pstr) const
    {
        return G_strcmp(ToCStr(), pstr) < 0;
    }
    bool    operator<(const GASString& str) const
    {
        return *this < str.ToCStr();
    }
    bool    operator>(const char* pstr) const
    {
        return G_strcmp(ToCStr(), pstr) > 0;
    }
    bool    operator>(const GASString& str) const
    {
        return *this > str.ToCStr();
    }

    // These functions compare strings with using current locale. They might be used
    // for sorting arrays of strings. By default, strings are sorted by Unicode order,
    // however this is not always acceptable. For example, in French, sorting of array
    // ['a','A','Z','z','e','ê','é'] should give the result ['a','A','e','é','ê','z','Z'], 
    // if locale is used rather than just Unicode order.
    int LocaleCompare_CaseCheck(const char* pstr, UPInt len, bool caseSensitive) const;
    int LocaleCompare_CaseCheck(const GASString& str, bool caseSensitive) const
    {
        return LocaleCompare_CaseCheck(str.ToCStr(), str.GetLength(), caseSensitive);
    }
    static int LocaleCompare_CaseCheck(const GASString& s1, const GASString& s2, bool caseSensitive)
    {
        return s1.LocaleCompare_CaseCheck(s2, caseSensitive);
    }

    int LocaleCompare(const char* pstr, UPInt len = GFC_MAX_UPINT) const
    {
        return LocaleCompare_CaseCheck(pstr, len, true);
    }
    int LocaleCompare(const GASString& str) const
    {
        return LocaleCompare(str.ToCStr(), str.GetLength());
    }
    static int LocaleCompare(const GASString& s1, const GASString& s2)
    {
        return s1.LocaleCompare(s2);
    }
    int LocaleCompare_CaseInsensitive(const char* pstr, UPInt len = GFC_MAX_UPINT) const
    {
        return LocaleCompare_CaseCheck(pstr, len, false);
    }
    int LocaleCompare_CaseInsensitive(const GASString& str) const
    {
        return LocaleCompare_CaseInsensitive(str.ToCStr(), str.GetLength());
    }
    static int LocaleCompare_CaseInsensitive(const GASString& s1, const GASString& s2)
    {
        return s1.LocaleCompare_CaseInsensitive(s2);
    }

    // Accesses raw bytes returned by GetSize.
    char operator [] (UInt index) const
    {
        GASSERT(index < pNode->Size);
        return pNode->pData[index];
    }




    // ***** Custom Methods

    // Path flag access inlines.
    bool    IsNotPath() const                 { return (GetHashFlags() & GASString::Flag_IsNotPath) != 0;  }
    void    SetPathFlags(UInt32 flags) const  { pNode->HashFlags |= flags; }

    // Determines if this string is a built-in.
    bool    IsBuiltin() const           { return (GetHashFlags() & GASString::Flag_Builtin) != 0;  }
    // Determines if this string is a standard member.
    bool    IsStandardMember() const    { return (GetHashFlags() & GASString::Flag_StandardMember) != 0;  }

    bool    IsCaseInsensitive() const    
    { 
        return (GetHashFlags() & Flag_CaseInsensitive) != 0;  
    }


    GINLINE void    AssignNode(GASStringNode *pnode)
    {
        pnode->AddRef();
        pNode->Release();
        pNode = pnode;
    }

    enum FlagConstants
    {
        Flag_HashMask       = 0x00FFFFFF,
        Flag_FlagMask       = 0xFF000000,
        // Flags
        Flag_Builtin        = 0x80000000,
        Flag_ConstData      = 0x40000000,
        Flag_StandardMember = 0x20000000,
        Flag_CaseInsensitive= 0x10000000, // used with StandardMembers

        // This flag is set if GetLength() == GetSize() for a string.
        // Avoid extra scanning is Substring and indexing logic.
        Flag_LengthIsSize   = 0x08000000,

        // Determining whether a string a path is pricey, so we cache this information
        // in a string node. Flag_PathCheck is set if a check was made for a path.
        // Flag_IsNotPath is set if we have determined that this string is not to be a path.
        // If a check was not made yet, Flag_IsNotPath is always cleared.
        Flag_PathCheck      = 0x04000000,
        Flag_IsNotPath      = 0x02000000
    };

    inline void     ResolveLowercase() const
    {
        if (pNode->pLower == 0)
            pNode->ResolveLowercase();
    }


    inline bool    Compare_CaseCheck(const GASString &str, bool caseSensitive) const
    {
        if (caseSensitive)
            return pNode == str.pNode;
        // For case in-sensitive strings we need to resolve lowercase.
        ResolveLowercase();
        str.ResolveLowercase();
        return pNode->pLower == str.pNode->pLower;
    }

    // Compares constants, assumes that 
    inline bool    CompareBuiltIn_CaseCheck(const GASString &str, bool caseSensitive) const
    {
        GASSERT((pNode->HashFlags & Flag_Builtin) != 0);
        if (caseSensitive)
            return pNode == str.pNode;
        // For case in-sensitive strings we need to resolve lowercase.
        str.ResolveLowercase();
        return pNode->pLower == str.pNode->pLower;
    }


    // Compares constants case-insensitively
    inline bool    CompareBuiltIn_CaseInsensitive(const GASString &str) const
    {
        GASSERT((pNode->HashFlags & Flag_Builtin) != 0);        
        // For case in-sensitive strings we need to resolve lowercase.
        str.ResolveLowercase();        
        return pNode->pLower == str.pNode->pLower;
    }

    // Compares constants case-insensitively
    inline bool    Compare_CaseInsensitive_Resolved(const GASString &str) const
    {
        GASSERT(pNode->pLower != 0);
        // For case in-sensitive strings we need to resolve lowercase.
        str.ResolveLowercase();        
        return pNode->pLower == str.pNode->pLower;
    }

    // Compares constants case-insensitively.
    // Assumes that pLower MUST be resolved in both strings.
    inline bool    CompareBuiltIn_CaseInsensitive_Unchecked(const GASString &str) const
    {
        GASSERT((pNode->HashFlags & Flag_Builtin) != 0);
        GASSERT(str.pNode->pLower != 0);
        return pNode->pLower == str.pNode->pLower;
    }


    // ***** Case Insensitive wrapper support

    // Case insensitive keys are used to look up insensitive string in hash tables
    // for SWF files with version before SWF 7.
    struct NoCaseKey
    {
        const GASString *pStr;        
        NoCaseKey(const GASString &str) : pStr(&str)
        {
            str.ResolveLowercase();
        }
    };

    bool    operator == (const NoCaseKey& strKey) const
    {
        return strKey.pStr->Compare_CaseInsensitive_Resolved(*this);
    }    
    bool    operator != (const NoCaseKey& strKey) const
    {
        return !strKey.pStr->Compare_CaseInsensitive_Resolved(*this);
    }

    // Raw compare is used to look up a string is extremely rare cases when no 
    // string context is available for GASString creation.    
    struct RawCompareKey
    {
        const char *pStr;
        UPInt       Hash;

        RawCompareKey(const char *pstr, UPInt length)
        {
            pStr = pstr;
            Hash = GASString::HashFunction(pstr, length);            
        }
    };

    bool    operator == (const RawCompareKey& strKey) const
    {
        return operator == (strKey.pStr);
    }    
    bool    operator != (const RawCompareKey& strKey) const
    {
        return operator != (strKey.pStr);
    }

};



// ***** String Manager implementation

struct GASStringKey
{
    const char* pStr;
    UPInt       HashValue;
    UPInt       Length;

    GASStringKey(const GASStringKey &src)
        : pStr(src.pStr), HashValue(src.HashValue), Length(src.Length)
    { }
    GASStringKey(const char* pstr, UPInt hashValue, UPInt length)
        : pStr(pstr), HashValue(hashValue), Length(length)
    { }
};


template<class C>
class GASStringNodeHashFunc
{
public:
    typedef C ValueType;

    // Hash code is stored right in the node
    UPInt  operator() (const C& data) const
    {
        return data->HashFlags;
    }

    UPInt  operator() (const GASStringKey &str) const
    {
        return str.HashValue;
    }

    // Hash update - unused.
    static UPInt    GetCachedHash(const C& data)                { return data->HashFlags; }
    static void     SetCachedHash(C& data, UPInt hashValue)     { GUNUSED2(data, hashValue); }
    // Value access.
    static C&       GetValue(C& data)                           { return data; }
    static const C& GetValue(const C& data)                     { return data; }

};

// String node hash set - keeps track of all strings currently existing in the manager.
typedef GHashSetUncachedLH<GASStringNode*,
                           GASStringNodeHashFunc<GASStringNode*>,
                           GASStringNodeHashFunc<GASStringNode*>,
                           GFxStatMV_ActionScript_Mem>              GASStringNodeHash;




class GASStringManager : public GRefCountBase<GASStringManager, GFxStatMV_ActionScript_Mem>
{    
    friend class GASString;  
    friend struct GASStringNode;

    GASStringNodeHash StringSet;
    GMemoryHeap*      pHeap;

    // Allocation Page structures, used to avoid many small allocations.
    // The page is aligned to StringNodePageAlign to allow only one 'GASStringManager*' to
    // be shared among multiple nodes.
    struct StringNodePage
    {
        enum {
            StringNodePageAlign = sizeof(void*) * 256, // 1K on 32-bit system.
            StringNodeCount     = (StringNodePageAlign - sizeof(void*)*4) / sizeof(GASStringNode)
        };
        GASStringManager*   pManager;
        // Next allocated page; save so that it can be released.
        StringNodePage*     pNext;        
        // Node array starts here.
        GASStringNode       Nodes[StringNodeCount];
    };

    // Strings text is also allocated in pages, to make small string management efficient.
    struct TextPage
    {
        // The size of buffer is usually 12 or 16, depending on platform.
        enum  {
            BuffSize   = (sizeof(void*) <= 4) ? (sizeof(void*) * 3) : (sizeof(void*) * 2),
            // Use -2 because we do custom alignment and we want to fit in allocator block.
            BuffCount  = (2048 / BuffSize) - 2
        };

        struct Entry
        {
            union
            {   // Entry for free node list.
                Entry*  pNextAlloc;
                char    Buff[BuffSize];
            };
        };

        Entry       Entries[BuffCount];
        TextPage*   pNext;
        void*       pMem;
    };


    // Free string nodes that can be used.
    GASStringNode*      pFreeStringNodes;
    // List of allocated string node pages, so that they can be released.
    // Note that these are allocated for the duration of GASStringManager lifetime.
    StringNodePage*     pStringNodePages;

    // Free string buffers that can be used, together with their owner nodes.
    TextPage::Entry*    pFreeTextBuffers;
    TextPage*           pTextBufferPages;

    // Pointer to the available string node.
    GASStringNode*      pEmptyStringNode;

    // Log object used for reporting AS leaks.
    GPtr<GFxLog>        pLog;
    GStringLH           FileName;


    void            AllocateStringNodes();
    void            AllocateTextBuffers();

    GASStringNode*  AllocStringNode()
    {
        if (!pFreeStringNodes)
            AllocateStringNodes();
        // Grab next node from list.
        GASStringNode* pnode = pFreeStringNodes;
        if (pFreeStringNodes)
            pFreeStringNodes = pnode->pNextAlloc;
        return pnode;
    }

    void            FreeStringNode(GASStringNode* pnode)
    {
        if (pnode->pData)
        {
            if (!(pnode->HashFlags & GASString::Flag_ConstData))
                FreeTextBuffer(const_cast<char*>(pnode->pData), pnode->Size);
            pnode->pData = 0;
        }
        // Insert into free list.
        pnode->pNextAlloc = pFreeStringNodes;
        pFreeStringNodes  = pnode;
    }

    // Get buffer for text (adds 0 to length).
    char*           AllocTextBuffer(UPInt length);
    // Allocates text buffer and copies length characters into it. Appends 0.
    char*           AllocTextBuffer(const char* pbuffer, UPInt length);
    void            FreeTextBuffer(char* pbuffer, UPInt length);


    // Various functions for creating string nodes.
    // Returns a node copy/reference to text in question.
    // All lengths specified must be exact and not include trailing characters.

    GASStringNode*  CreateConstStringNode(const char* pstr, UPInt length, UInt32 stringFlags);
    // Allocates node owning buffer.
    GASStringNode*  CreateStringNode(const char* pstr);
    GASStringNode*  CreateStringNode(const char* pstr, UPInt length);
    // These functions also perform string concatenation.
    GASStringNode*  CreateStringNode(const char* pstr1, UPInt l1,
                                     const char* pstr2, UPInt l2);
    GASStringNode*  CreateStringNode(const char* pstr1, UPInt l1,
                                     const char* pstr2, UPInt l2,
                                     const char* pstr3, UPInt l3);
    // Wide character; use special type of counting.
    GASStringNode*  CreateStringNode(const wchar_t* pwstr);  

    GASStringNode* GetEmptyStringNode() { return pEmptyStringNode;  }

public:

    GASStringManager(GMemoryHeap* pheap);
    ~GASStringManager();

    GMemoryHeap* GetHeap() const { return pHeap; }

    // Sets the log that will be used to report leaks during destructor.    
    void        SetLeakReportLog(GFxLog *plog, const char *pfilename);

    GASString   CreateEmptyString()
    {
        return GASString(GetEmptyStringNode());
    }

    // Create a string from a buffer.
    GASString   CreateString(const char *pstr)
    {
        return GASString(CreateStringNode(pstr));
    } 
    GASString   CreateString(const char *pstr, UPInt length)
    {
        return GASString(CreateStringNode(pstr, length));
    }     
    GASString   CreateString(const GString& str)
    {
        return GASString(CreateStringNode(str.ToCStr(), str.GetSize()));
    }
    GASString   CreateConstString(const char *pstr, UPInt length, UInt32 stringFlags = 0)
    {
        return GASString(CreateConstStringNode(pstr, length, stringFlags));
    }
    GASString   CreateConstString(const char *pstr)
    {
        return GASString(CreateConstStringNode(pstr, G_strlen(pstr), 0));
    }
    // Wide character; use special type of counting.
    GASString  CreateString(const wchar_t* pwstr)
    {
        return GASString(CreateStringNode(pwstr));
    }  
};

// *** GASStringNode inlines

GASStringManager*   GASStringNode::GetManager() const
{
    GASStringManager::StringNodePage* ppage = (GASStringManager::StringNodePage*)
        (((UPInt)this) & ~(GASStringManager::StringNodePage::StringNodePageAlign-1));
    return ppage->pManager;
}


// ***** Hash table used for member Strings

struct GASStringHashFunctor
{
    inline UPInt  operator() (const GASString& str) const
    {
        // No need to mask out hashFlags since flags are in high bits that
        // will be masked out automatically by the hash table.
        return str.GetHashFlags();
    }

    // Hash lookup for custom keys.
    inline UPInt  operator() (const GASString::NoCaseKey& ikey) const
    {        
        return ikey.pStr->GetHashFlags();
    }
    inline UPInt  operator() (const GASString::RawCompareKey& ikey) const
    {
        return ikey.Hash;
    }
};

template<class C, class HashF>
class GHashsetNodeEntry_GC
{
public:
    // Internal chaining for collisions.
    SPInt NextInChain;
    C     Value;

    GHashsetNodeEntry_GC()
        : NextInChain(-2) { }
    GHashsetNodeEntry_GC(const GHashsetNodeEntry_GC& e)
        : NextInChain(e.NextInChain), Value(e.Value) { }
    GHashsetNodeEntry_GC(const C& key, SPInt next)
        : NextInChain(next), Value(key) { }    
    GHashsetNodeEntry_GC(const typename C::NodeRef& keyRef, SPInt next)
        : NextInChain(next), Value(keyRef) { }

    bool    IsEmpty() const             { return NextInChain == -2;  }
    bool    IsEndOfChain() const        { return NextInChain == -1;  }
    UPInt   GetCachedHash(UPInt maskValue) const  { return HashF()(Value) & maskValue; }
    void    SetCachedHash(UPInt hashValue)        { GUNUSED(hashValue); }

    void    Clear()
    {        
        Value.~C(); // placement delete
        NextInChain = -2;
    }
    void    Free()
    {   
        typedef typename C::FirstType FirstDtor;
        typedef typename C::SecondType SecondDtor;
        Value.First.~FirstDtor();
        Value.Second.Finalize_GC(); // placement delete
        //Value.Second.~SecondDtor(); // placement delete

        //Value.~C();
        NextInChain = -2;
    }
};

template<class C, class U, class HashF = GFixedSizeHash<C>, int StatId = GStat_Default_Mem>
class GHashUncachedLH_GC
    : public GHash<C, U, HashF, GAllocatorLH<C, StatId>, GHashNode<C,U,HashF>,
    GHashsetNodeEntry_GC<GHashNode<C,U,HashF>, typename GHashNode<C,U,HashF>::NodeHashF> >
{
public:
    typedef GHashUncachedLH_GC<C, U, HashF, StatId> SelfType;
    typedef GHash<C, U, HashF, GAllocatorLH<C, StatId>, GHashNode<C,U,HashF>,
        GHashsetNodeEntry_GC<GHashNode<C,U,HashF>, typename GHashNode<C,U,HashF>::NodeHashF> >  BaseType;
    //GHashsetEntry<C, HashF> > BaseType;

    // Delegated constructors.
    GHashUncachedLH_GC()                                        { }
    GHashUncachedLH_GC(int sizeHint) : BaseType(sizeHint)       { }
    GHashUncachedLH_GC(const SelfType& src) : BaseType(src)     { }
    ~GHashUncachedLH_GC()                                       
    { 
        //FreeTable();
    }
    void operator = (const SelfType& src)                    { BaseType::operator = (src); }
};

// Case-insensitive string hash.
template<class U, class BaseType>
class GASStringHashBase : public BaseType
{
    typedef GASStringHashBase<U, BaseType>  SelfType;

public:
    // Delegated constructors.
    GASStringHashBase()                                     { }
    GASStringHashBase(int sizeHint) : BaseType(sizeHint)    { }
    GASStringHashBase(const SelfType& src) : BaseType(src)  { }
    ~GASStringHashBase()                                    { }

    void    operator = (const SelfType& src)               { BaseType::operator = (src); }


    // *** Case-Insensitive / Case-Selectable 'get' lookups.

    bool    GetCaseInsensitive(const GASString& key, U* pvalue) const
    {
        GASString::NoCaseKey ikey(key);
        return BaseType::GetAlt(ikey, pvalue);
    }
    // Pointer-returning get variety.
    const U* GetCaseInsensitive(const GASString& key) const   
    {
        GASString::NoCaseKey ikey(key);
        return BaseType::GetAlt(ikey);
    }
    U*  GetCaseInsensitive(const GASString& key)
    {
        GASString::NoCaseKey ikey(key);
        return BaseType::GetAlt(ikey);
    }

    // Case-checking based on argument.
    bool    GetCaseCheck(const GASString& key, U* pvalue, bool caseSensitive) const
    {
        return caseSensitive ? BaseType::Get(key, pvalue) : GetCaseInsensitive(key, pvalue);
    }
    const U* GetCaseCheck(const GASString& key, bool caseSensitive) const   
    {
        return caseSensitive ? BaseType::Get(key) : GetCaseInsensitive(key);
    }
    U*      GetCaseCheck(const GASString& key, bool caseSensitive)
    {
        return caseSensitive ? BaseType::Get(key) : GetCaseInsensitive(key);
    }


    // *** Case-Insensitive / Case-Selectable find.

    typedef typename BaseType::ConstIterator  ConstIterator;
    typedef typename BaseType::Iterator       Iterator;

    Iterator    FindCaseInsensitive(const GASString& key)
    {
        GASString::NoCaseKey ikey(key);
        return BaseType::FindAlt(ikey);
    }
    Iterator    FindCaseCheck(const GASString& key, bool caseSensitive)
    {
        return caseSensitive ? BaseType::Find(key) : FindCaseInsensitive(key);
    }


    // *** Case-Selectable set.

    // Set just uses a find and assigns value if found.
    // The key is not modified - this behavior is identical to Flash string variable assignment.    
    void    SetCaseCheck(const GASString& key, const U& value, bool caseSensitive)
    {
        Iterator it = FindCaseCheck(key, caseSensitive);
        if (it != BaseType::End())
        {
            it->Second = value;
        }
        else
        {
            BaseType::Add(key, value);
        }
    } 


    // *** Case-Insensitive / Case-Selectable remove.

    void     RemoveCaseInsensitive(const GASString& key)
    {   
        GASString::NoCaseKey ikey(key);
        BaseType::RemoveAlt(ikey);
    }

    void     RemoveCaseCheck(const GASString& key, bool caseSensitive)
    {   
        if (caseSensitive)
            BaseType::Remove(key);
        else
            RemoveCaseInsensitive(key);
    }
};

template <class U>
class GASStringHash : public GASStringHashBase<U, GHashUncachedLH<GASString, U, GASStringHashFunctor, GFxStatMV_ActionScript_Mem> >
{
    typedef GASStringHash<U>                                                                  SelfType;
    typedef GASStringHashBase<U, GHashUncachedLH<GASString, U, GASStringHashFunctor, GFxStatMV_ActionScript_Mem> >   BaseType;

};

#ifndef GFC_NO_GC
// special version for GRefCountCollection
template <class U>
class GASStringHash_GC : public GASStringHashBase<U, GHashUncachedLH_GC<GASString, U, GASStringHashFunctor, GFxStatMV_ActionScript_Mem> >
{
    typedef GASStringHash_GC<U>                                                               SelfType;
    typedef GASStringHashBase<U, GHashUncachedLH_GC<GASString, U, GASStringHashFunctor, GFxStatMV_ActionScript_Mem> >   BaseType;

};
#else
#define GASStringHash_GC GASStringHash
#endif // !GFC_NO_GC

#endif
