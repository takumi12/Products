/**********************************************************************

Filename    :   GFxXMLObject.cpp
Content     :   XML DOM support
Created     :   March 17, 2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GConfig.h"

#ifndef GFC_NO_XML_SUPPORT

#include <GFxXMLObject.h>
#include <GFxXMLDocument.h>
#include "XML/GFxXMLShadowRef.h"


#include "GFxPlayerImpl.h"


#ifdef GFC_BUILD_DEBUG
//
// Trace XML object reference counts in debug output
// (Warning: This generates a lot of messages)
//
// #define GFC_XML_STRING_REFCOUNTING_TRACE
//
#endif  // GFC_BUILD_DEBUG

//
// Frees the node whose refCount has reached 0.
//
void   GFxXMLDOMStringNode::ReleaseNode()
{
#ifdef GFC_XML_STRING_REFCOUNTING_TRACE
    GFC_DEBUG_MESSAGE1(1, "GFxXMLStringNode::~ReleaseNode : '%.16s'", pData);
#endif

    GASSERT(RefCount == 0);

    pManager->StringSet.Remove(this);
    pManager->FreeStringNode(this);
}


GFxXMLDOMString::GFxXMLDOMString(GFxXMLDOMStringNode *pnode)
{
    pNode = pnode;
    pNode->AddRef();
}

GFxXMLDOMString::GFxXMLDOMString(const GFxXMLDOMString& src)
{
    pNode = src.pNode;
    pNode->AddRef();
}
GFxXMLDOMString::~GFxXMLDOMString()
{        
    pNode->Release();
}

void    GFxXMLDOMString::AssignNode(GFxXMLDOMStringNode *pnode)
{
    pnode->AddRef();
    pNode->Release();
    pNode = pnode;
}

//
// Static hash function calculation routine
//
UInt32  GFxXMLDOMString::HashFunction(const char *pchar, UPInt length)
{
    return (UInt32)GString::BernsteinHashFunction(pchar, length) & GFxXMLDOMStringNode::Flag_HashMask;
}


GFxXMLDOMStringManager::GFxXMLDOMStringManager()
{
    pHeap = GMemory::GetHeapByAddress(this);

    // Allocate initial data.    
    pStringNodePages = 0;
    pFreeStringNodes = 0;

    pFreeTextBuffers = 0;
    pTextBufferPages = 0;

    // Empty data - refcount 1, so never released.
    EmptyStringNode.RefCount    = 1;
    EmptyStringNode.Size        = 0;
    EmptyStringNode.HashFlags   = GFxXMLDOMString::HashFunction("", 0);
    EmptyStringNode.pData       =  "";
    EmptyStringNode.pManager    = this;

    StringSet.Add(&EmptyStringNode);
}

GFxXMLDOMStringManager::~GFxXMLDOMStringManager()
{    
    // Release string nodes.
    while(pStringNodePages)
    {
        StringNodePage* ppage = pStringNodePages;
        pStringNodePages = ppage->pNext;

        // Free text in all active XMLString objects. Technically this is a bug, since 
        // all XMLString should have already died before XMLStringManager is destroyed.
        // However, if ActionScript leaks due to circular references, this will at least
        // release all of the string content involved.

        // In the future, once we have GC this may be changed to throw asserts if
        // unreleased data / RefCount is detected at this point.

        for (UInt i=0; i< ppage->StringNodeCount; i++)
        {
            if (ppage->Nodes[i].pData)
                FreeTextBuffer(const_cast<char*>(ppage->Nodes[i].pData), ppage->Nodes[i].Size);
        }
        GFREE(ppage);
    }

    // Free text pages.
    while(pTextBufferPages)
    {
        void *pmem = pTextBufferPages->pMem;
        pTextBufferPages = pTextBufferPages->pNext;        
        GFREE(pmem);
    }
}


void    GFxXMLDOMStringManager::AllocateStringNodes()
{
    StringNodePage* ppage = (StringNodePage*) GHEAP_ALLOC(pHeap, sizeof(StringNodePage), GFxStatMV_XML_Mem);

    if (ppage)
    {
        ppage->pNext = pStringNodePages;        
        pStringNodePages = ppage;
        // Add nodes to free pool.
        for (UInt i=0; i < StringNodePage::StringNodeCount; i++)
        {
            GFxXMLDOMStringNode *pnode = &ppage->Nodes[i];
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

void   GFxXMLDOMStringManager::AllocateTextBuffers()
{
    // Align data (some of our allocation routines add size counters
    // that may case this to be misaligned at a multiple of 4 instead).
    // Seems to make a difference on P4 chips.
    void*     pp    = GHEAP_ALLOC(pHeap, sizeof(TextPage) + 8, GFxStatMV_XML_Mem);    
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
char*   GFxXMLDOMStringManager::AllocTextBuffer(UPInt length)
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
    return (char*)GHEAP_ALLOC(pHeap, length + 1, GFxStatMV_XML_Mem);
}

char*   GFxXMLDOMStringManager::AllocTextBuffer(const char* pbuffer, UPInt length)
{
    char *pbuff = AllocTextBuffer(length);
    if (pbuff)
    {
        memcpy(pbuff, pbuffer, length);
        pbuff[length] = 0;
    }
    return pbuff;
}

void    GFxXMLDOMStringManager::FreeTextBuffer(char* pbuffer, UPInt length)
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

    GFREE(pbuffer);
}


bool operator == (GFxXMLDOMStringNode* pnode, const GFxXMLDOMStringKey &str)
{
    if (pnode->Size != str.Length)
        return 0;
    return strncmp(pnode->pData, str.pStr, str.Length) == 0;
}


GFxXMLDOMStringNode*  GFxXMLDOMStringManager::CreateStringNode(const char* pstr, UPInt length)
{
    GFxXMLDOMStringNode*  pnode = 0;
    GFxXMLDOMStringKey    key(pstr, GFxXMLDOMString::HashFunction(pstr, length), length);

    if (StringSet.GetAlt(key, &pnode))
        return pnode;

    if (length == 0)
        return &EmptyStringNode;

    if ((pnode = AllocStringNode()) != 0)
    { 
        pnode->pData = AllocTextBuffer(pstr, length);

        if (pnode->pData)
        {
            pnode->RefCount = 0;
            pnode->Size     = (UInt)length;
            pnode->HashFlags= (UInt32)key.HashValue;

            StringSet.Add(pnode);
            return pnode;
        }
        else
        {
            FreeStringNode(pnode);
        }
    }

    return &EmptyStringNode;
}


// These functions also perform string concatenation.
GFxXMLDOMStringNode*  GFxXMLDOMStringManager::CreateStringNode(const char* pstr1, UPInt l1,
                                                               const char* pstr2, UPInt l2)
{
    GFxXMLDOMStringNode*  pnode = 0;
    UPInt           length = l1 + l2;    
    char*           pbuffer = AllocTextBuffer(length);

    if (pbuffer)
    {
        if (l1 > 0) memcpy(pbuffer, pstr1, l1);
        if (l2 > 0) memcpy(pbuffer + l1, pstr2, l2);
        pbuffer[length] = 0;

        GFxXMLDOMStringKey   key(pbuffer, GFxXMLDOMString::HashFunction(pbuffer, length), length);

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

            StringSet.Add(pnode);
            return pnode;
        }
        else
        {
            FreeTextBuffer(pbuffer, length);
        }
    }

    return &EmptyStringNode;
}


// --------------------------------------------------------------------

//
// Ctor
//
GFxXMLObjectManager::GFxXMLObjectManager(GMemoryHeap* pheap, GFxMovieRoot* powner)
: GFxExternalLibPtr(powner), pHeap(pheap)
{
#ifdef GFC_XML_OBJECT_MANAGER_REFCOUNTING_TRACE
    GFC_DEBUG_MESSAGE(1, "GFxXMLObjectManager::GFxXMLObjectManager");
#endif
}
GFxXMLObjectManager::GFxXMLObjectManager(GFxMovieRoot* powner)
: GFxExternalLibPtr(powner)
{
#ifdef GFC_XML_OBJECT_MANAGER_REFCOUNTING_TRACE
    GFC_DEBUG_MESSAGE(1, "GFxXMLObjectManager::GFxXMLObjectManager");
#endif
    pHeap = GMemory::GetHeapByAddress(this);
}


//
// Dtor
//
GFxXMLObjectManager::~GFxXMLObjectManager()
{
#ifdef GFC_XML_OBJECT_MANAGER_REFCOUNTING_TRACE
    GFC_DEBUG_MESSAGE(1, "GFxXMLObjectManager::~GFxXMLObjectManager");
#endif

    // Unregister from owner (if one exists)
    if (pOwner)
        pOwner->pXMLObjectManager = NULL;

}

//
// Thread safe object allocation 
//
GFxXMLDocument* GFxXMLObjectManager::CreateDocument()
{
    return GHEAP_NEW(pHeap) GFxXMLDocument(this);
}
GFxXMLElementNode* GFxXMLObjectManager::CreateElementNode(GFxXMLDOMString value)
{
    return GHEAP_NEW(pHeap) GFxXMLElementNode(this, value);
}
GFxXMLTextNode* GFxXMLObjectManager::CreateTextNode(GFxXMLDOMString value)
{
    return GHEAP_NEW(pHeap) GFxXMLTextNode(this, value);
}
GFxXMLAttribute* GFxXMLObjectManager::CreateAttribute(GFxXMLDOMString name, GFxXMLDOMString value)
{
    return GHEAP_NEW(pHeap) GFxXMLAttribute(name, value);
}
GFxXMLPrefix* GFxXMLObjectManager::CreatePrefix(GFxXMLDOMString name, GFxXMLDOMString value)
{
    return GHEAP_NEW(pHeap) GFxXMLPrefix(name, value);
}
GFxXMLShadowRef* GFxXMLObjectManager::CreateShadowRef()
{
    return GHEAP_NEW(pHeap) GFxXMLShadowRef();
}
GFxASXMLRootNode* GFxXMLObjectManager::CreateRootNode(GFxXMLNode* pdom)
{
    return GHEAP_NEW(pHeap) GFxASXMLRootNode(pdom);
}


#endif // #ifndef GFC_NO_XML_SUPPORT 
