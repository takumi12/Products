/**********************************************************************

Filename    :   GFxXML.cpp
Content     :   DOM support
Created     :   December 13, 2007
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GConfig.h"

#ifndef GFC_NO_XML_SUPPORT

#include <GFxXML.h>


// 
// Debug trace/dump helper functions
//
#ifdef GFC_BUILD_DEBUG

//
// Trace the document builder's DOM tree construction in debug output
//
// #define GFC_XML_DOCBUILDER_TRACE
//

//
// Dump all constructed DOM trees to standard out
// if provided to the document builder
// (Warning: Avoid this for large files)
//
// #define GFC_XML_DOCBUILDER_DOM_TREE_DUMP
//


// Maximum size of strings used for debug printing
#define MAX_DEBUG_STRING_LENGTH 256
// 
// Helper to copy strings for debugging
//
void SafeStringCopyWithSentinel(const char* src, UPInt srcLen, 
                                char* dest, UPInt destSz)
{
    UPInt len = 0;
    if (srcLen  < destSz)
        len = srcLen;
    else
        len = (destSz - 1);
    G_strncpy(dest, len + 1, src, len);
    // explicit sentinel just in case
    dest[len] = '\0';
}

#ifdef GFC_XML_DOCBUILDER_DOM_TREE_DUMP
//
// Prints the DOM tree to stdout
//
char printDOMBuffer[128];
void PrintDOMTree(GFxXMLNode* proot, UPInt level = 0)
{
    for (UPInt i=0; i < level; i++)
        printf("  ");
    switch (proot->Type)
    {
    case GFxXMLElementNodeType: 
        {
            GFxXMLElementNode* pnode = static_cast<GFxXMLElementNode*>(proot);
            if (pnode->Prefix.GetSize() > 0)
            {
                printf("ELEM - '%.16s:%.16s' ns:'%.16s' prefix:'%.16s'"
                    " localname: '%.16s'",
                    pnode->Prefix.ToCStr(),
                    pnode->Value.ToCStr(),
                    pnode->Namespace.ToCStr(),
                    pnode->Prefix.ToCStr(),
                    pnode->Value.ToCStr());
            }
            else
            {
                printf("ELEM - '%.16s' ns:'%.16s' prefix:''"
                    " localname: '%.16s'",
                    pnode->Value.ToCStr(), 
                    pnode->Namespace.ToCStr(), 
                    pnode->Value.ToCStr());
            }
            for (GFxXMLAttribute* attr = pnode->FirstAttribute; 
                attr != NULL; attr = attr->Next)
            {
                printf(" {%.16s,%.16s}", attr->Name.ToCStr(), 
                attr->Value.ToCStr());
            }
            printf("\n");
            for (GFxXMLNode* child = pnode->FirstChild; child != NULL; 
                child = child->NextSibling)
                PrintDOMTree(child, level+1);
            break;
        }
    case GFxXMLTextNodeType: 
        {
            printf("TEXT - '%.16s'\n", proot->Value.ToCStr());
            break;
        }
    default: 
        {
            printf("UNKN\n");
        }
    }
}

#endif  // #ifdef GFC_XML_DOCBUILDER_DOM_TREE_DUMP

#endif  // #ifdef GFC_BUILD_DEBUG


// 
// Constructor
//
GFxXMLDOMBuilder::GFxXMLDOMBuilder(GPtr<GFxXMLSupportBase> pxmlParser, 
                                           bool ignorews)
    : pXMLParserState(pxmlParser), pLocator(NULL), pDoc(NULL), 
      bIgnoreWhitespace(ignorews), bError(false)
{
#ifdef GFC_XML_DOCBUILDER_TRACE
    GFC_DEBUG_MESSAGE(1, "GFxXMLDocumentBuilder::GFxXMLDocumentBuilder");
#endif
}


// 
// Parse an XML file and build a DOM tree out of it
//
GPtr<GFxXMLDocument> GFxXMLDOMBuilder::ParseFile(const char* pfilename, 
                                                      GFxFileOpenerBase* pfo, 
                                                      GPtr<GFxXMLObjectManager> objMgr /* = NULL */)
{
    // reset internal vars
    bError = false;
    TotalBytesToLoad = 0;
    LoadedBytes = 0;

    if (NULL == objMgr.GetPtr())
        objMgr = *new GFxXMLObjectManager;

    // create new document
    pDoc = *objMgr->CreateDocument();

    // commence parsing
    if (pXMLParserState)
        bError = !pXMLParserState->ParseFile(pfilename, pfo, this);
    // NOTE: The locator could be invalid at this point
    
    // Release internal reference to document
    GPtr<GFxXMLDocument> pret = pDoc;
    pDoc = 0;

    if (pret.GetPtr() && bIgnoreWhitespace)
        DropWhiteSpaceNodes(pret);

    return pret;
}

// 
// Parse an XML string and build a DOM tree out of it
//
GPtr<GFxXMLDocument> GFxXMLDOMBuilder::ParseString(const char* pdata, UPInt len, 
                                                        GPtr<GFxXMLObjectManager> objMgr /* = NULL */)
{
    // Reset internal vars
    bError = false;
    TotalBytesToLoad = 0;
    LoadedBytes = 0;

    if (NULL == objMgr.GetPtr())
        objMgr = *new GFxXMLObjectManager;

    // Create new document
    pDoc = *objMgr->CreateDocument();

    // Commence parsing
    if (pXMLParserState)
        bError = !pXMLParserState->ParseString(pdata, len, this);
    // NOTE: The locator could be invalid at this point

    // Release internal reference to document
    GPtr<GFxXMLDocument> pret = pDoc;
    pDoc = 0;

    if (pret.GetPtr() && bIgnoreWhitespace)
        DropWhiteSpaceNodes(pret);

    return pret;
}

// 
// Document begin reported by XML parser
//
void GFxXMLDOMBuilder::StartDocument()
{
#ifdef GFC_XML_DOCBUILDER_TRACE
    GFC_DEBUG_ASSERT(pLocator, "Locator not set!");
    GFC_DEBUG_MESSAGE(1, "GFxXMLDocumentBuilder::StartDocument");
#endif

    // Create a parse stack      
    ParseStack.PushBack(pDoc);

    // Copy the total number of bytes to load
    TotalBytesToLoad = pLocator->TotalBytesToLoad;
}


// 
// Document end reported by XML parser
//
void GFxXMLDOMBuilder::EndDocument()
{
#ifdef GFC_XML_DOCBUILDER_TRACE
    GFC_DEBUG_ASSERT(pLocator, "Locator not set!");
    char db0[MAX_DEBUG_STRING_LENGTH];
    char db1[MAX_DEBUG_STRING_LENGTH];
    SafeStringCopyWithSentinel(pLocator->XMLVersion.ToCStr(), 
        pLocator->XMLVersion.GetSize(),
        db0, MAX_DEBUG_STRING_LENGTH);
    SafeStringCopyWithSentinel(pLocator->Encoding.ToCStr(), 
        pLocator->Encoding.GetSize(),
        db1, MAX_DEBUG_STRING_LENGTH);
    GFC_DEBUG_MESSAGE3(1, 
        "GFxXMLDocumentBuilder::EndDocument('%.16s', '%.16s', %d)",
        db0, db1, pLocator->StandAlone);
#endif

    // Copy the number of bytes loaded
    LoadedBytes = pLocator->LoadedBytes;

    // Check parse stack
    GASSERT(ParseStack.GetSize() == 1);
    GASSERT(ParseStack[0].GetPtr() == pDoc.GetPtr());

    // Drop reference to document
    ParseStack.Clear();

    // Check prefix ownership stacks
    GASSERT(PrefixNamespaceStack.GetSize() == 0);
    GASSERT(DefaultNamespaceStack.GetSize() == 0);

    // Set the xml declaration values
    GPtr<GFxXMLObjectManager> memMgr = pDoc->MemoryManager;
    pDoc->XMLVersion = memMgr->CreateString(pLocator->XMLVersion.ToCStr(),
        pLocator->XMLVersion.GetSize());
    pDoc->Encoding = memMgr->CreateString(pLocator->Encoding.ToCStr(),
        pLocator->Encoding.GetSize());
    pDoc->Standalone = (SByte)pLocator->StandAlone;

#ifdef GFC_XML_DOCBUILDER_DOM_TREE_DUMP
    PrintDOMTree(pDoc);
#endif
}


// 
// Tag element begin reported by XML parser
//
void GFxXMLDOMBuilder::StartElement(const GFxXMLStringRef& prefix,
                                  const GFxXMLStringRef& localname,
                                  const GFxXMLParserAttributes& atts)
{
#ifdef GFC_XML_DOCBUILDER_TRACE
    GFC_DEBUG_ASSERT(pLocator, "Locator not set!");
    char db0[MAX_DEBUG_STRING_LENGTH];
    char db1[MAX_DEBUG_STRING_LENGTH];
    SafeStringCopyWithSentinel(prefix.ToCStr(), prefix.GetSize(), 
        db0, MAX_DEBUG_STRING_LENGTH);
    SafeStringCopyWithSentinel(localname.ToCStr(), localname.GetSize(), 
        db1, MAX_DEBUG_STRING_LENGTH);
    GFC_DEBUG_MESSAGE4(1, 
        "[%d, %d] GFxXMLDocumentBuilder::StartElement('%.16s', '%.16s')", 
        pLocator->Line, pLocator->Column, db0, db1);
    for (UPInt i=0; i < atts.Length; i++)
    {
        GFxXMLParserAttribute& att = atts.Attributes[i];
        SafeStringCopyWithSentinel(att.Name.ToCStr(), att.Name.GetSize(), 
            db0, MAX_DEBUG_STRING_LENGTH);
        SafeStringCopyWithSentinel(att.Value.ToCStr(), att.Value.GetSize(), 
            db1, MAX_DEBUG_STRING_LENGTH);
        GFC_DEBUG_MESSAGE2(1, "\tATTR: '%.16s' -> '%.16s'", db0, db1);
    }
#endif

    // Copy the number of bytes loaded
    LoadedBytes = pLocator->LoadedBytes;

    // Must have at least root node in stack
    GASSERT(ParseStack.GetSize() > 0);

    GPtr<GFxXMLObjectManager> memMgr = pDoc->MemoryManager;

    // If appending required, consolidate it
    if (pAppendChainRoot.GetPtr() != NULL)
    {
        GPtr<GFxXMLElementNode> pnode = ParseStack.Back();
        pnode->AppendChild(pAppendChainRoot);
        pAppendChainRoot->Value = memMgr->CreateString(AppendText.ToCStr(), AppendText.GetSize());
        // Reset append chain
        pAppendChainRoot = NULL;
        AppendText.Clear();
    }

    // Create new node
    GPtr<GFxXMLElementNode> elemNode = *memMgr->CreateElementNode(
        memMgr->CreateString(localname.ToCStr(), localname.GetSize()));

    // Add attributes
    for (UPInt i=0; i < atts.Length; i++)
    {
        GFxXMLParserAttribute& att = atts.Attributes[i];
        GFxXMLAttribute* patt = memMgr->CreateAttribute(
            memMgr->CreateString(att.Name.ToCStr(), att.Name.GetSize()),
            memMgr->CreateString(att.Value.ToCStr(), att.Value.GetSize()));
        elemNode->AddAttribute(patt);
    }

    // Assign ownership to any floating namespaces
    if (PrefixNamespaceStack.GetSize() > 0)
    {
        UPInt len = PrefixNamespaceStack.GetSize();
        for (SInt i=(SInt)len-1; i >= 0; i--)
        {
            PrefixOwnership& po = PrefixNamespaceStack[i];
            if (NULL != po.Owner.GetPtr())
                break;
            po.Owner = elemNode;
        }
    }
    if (DefaultNamespaceStack.GetSize() > 0)
    {
        PrefixOwnership& po = DefaultNamespaceStack.Back();
        if (NULL == po.Owner.GetPtr())
            po.Owner = elemNode;
    }

    // Assign appropriate namespace to node
    if (prefix.GetSize() > 0)
    {
        bool found = false;
        // Assign prefix namespace - it has higher precedence
        // TODO: change to hash lookup?
        if (PrefixNamespaceStack.GetSize() > 0)
        {
            UPInt len = PrefixNamespaceStack.GetSize();
            for (SInt i=(SInt)len-1; i >= 0; i--)
            {
                PrefixOwnership po = PrefixNamespaceStack[i];
                if (po.Prefix->Name == 
                    GFxXMLDOMStringCompareWrapper(prefix.ToCStr(), prefix.GetSize()))
                {
                    found = true;
                    elemNode->Prefix = po.Prefix->Name;
                    elemNode->Namespace = po.Prefix->Value;
                    break;
                }
            }
        }
        if (!found)
        {
            // Prefix was not found. create a dummy one.
            elemNode->Prefix = memMgr->CreateString(prefix.ToCStr(), 
                prefix.GetSize());
            elemNode->Namespace = memMgr->EmptyString();
        }
    }
    else if (DefaultNamespaceStack.GetSize() > 0)
    {
        // Assign default namespace
        PrefixOwnership os = DefaultNamespaceStack.Back();
        elemNode->Prefix = os.Prefix->Name;
        elemNode->Namespace = os.Prefix->Value;
    }

    // Attach to parent
    GPtr<GFxXMLElementNode> parentNode = ParseStack.Back();
    parentNode->AppendChild(elemNode);

    // Push to parse stack
    ParseStack.PushBack(elemNode);
}


// 
// Tag element end reported by XML parser
//
void GFxXMLDOMBuilder::EndElement(const GFxXMLStringRef& prefix,
                                const GFxXMLStringRef& localname)
{
    GUNUSED2(prefix, localname);

#ifdef GFC_XML_DOCBUILDER_TRACE
    GFC_DEBUG_ASSERT(pLocator, "Locator not set!");
    char db0[MAX_DEBUG_STRING_LENGTH];
    char db1[MAX_DEBUG_STRING_LENGTH];
    SafeStringCopyWithSentinel(prefix.ToCStr(), prefix.GetSize(), db0, 
        MAX_DEBUG_STRING_LENGTH);
    SafeStringCopyWithSentinel(localname.ToCStr(), localname.GetSize(), 
        db1, MAX_DEBUG_STRING_LENGTH);
    GFC_DEBUG_MESSAGE4(1, 
        "[%d, %d] GFxXMLDocumentBuilder::EndElement('%.16s', '%.16s')", 
        pLocator->Line, pLocator->Column, db0, db1);
#endif

    // Copy the number of bytes loaded
    LoadedBytes = pLocator->LoadedBytes;

    GASSERT(ParseStack.GetSize() > 1); // must be at least root node + 1

    GPtr<GFxXMLElementNode> pnode = ParseStack.Back();
    GASSERT(pnode.GetPtr() != pDoc.GetPtr());    // must not be root node    
   
    // If appending required, consolidate it
    if (pAppendChainRoot.GetPtr() != NULL)
    {
        GPtr<GFxXMLObjectManager> memMgr = pDoc->MemoryManager;
        pnode->AppendChild(pAppendChainRoot);
        // Accumulate append text while removing the nodes
        pAppendChainRoot->Value = memMgr->CreateString(AppendText.ToCStr(), AppendText.GetSize());
        // Reset append chain
        pAppendChainRoot = NULL;
        AppendText.Clear();
    }
    
    // Remove namespace ownership
    // A top-down traversal of the namespace stacks is guaranteed to 
    // return any owned namespaces. if not found, no ownership.
    if (PrefixNamespaceStack.GetSize() > 0)
    {
        UPInt len = PrefixNamespaceStack.GetSize();
        for (SInt i=(SInt)len-1; i >= 0; i--)
        {
            PrefixOwnership po = PrefixNamespaceStack[i];
            if (po.Owner != pnode)
                break;
            PrefixNamespaceStack.PopBack();
        }
    }
    if (DefaultNamespaceStack.GetSize() > 0)
    {
        PrefixOwnership po = DefaultNamespaceStack.Back();
        if (po.Owner == pnode)
            DefaultNamespaceStack.PopBack();
    }

    // Pop stack
    ParseStack.PopBack();
}


// 
// Namespace mapping reported by XML parser
//
// Next tag element will define the namespace scope.
//
void GFxXMLDOMBuilder::PrefixMapping(const GFxXMLStringRef& prefix, 
                                          const GFxXMLStringRef& uri)
{
#ifdef GFC_XML_DOCBUILDER_TRACE
    GFC_DEBUG_ASSERT(pLocator, "Locator not set!");
    char db0[MAX_DEBUG_STRING_LENGTH];
    char db1[MAX_DEBUG_STRING_LENGTH];
    SafeStringCopyWithSentinel(prefix.ToCStr(), prefix.GetSize(), 
        db0, MAX_DEBUG_STRING_LENGTH);
    SafeStringCopyWithSentinel(uri.ToCStr(), uri.GetSize(), db1, 
        MAX_DEBUG_STRING_LENGTH);
    GFC_DEBUG_MESSAGE4(1, 
        "[%d, %d] GFxXMLDocumentBuilder::StartPrefixMapping('%.16s', '%.16s')", 
        pLocator->Line, pLocator->Column, db0, db1);
#endif

    // Copy the number of bytes loaded
    LoadedBytes = pLocator->LoadedBytes;

    GPtr<GFxXMLObjectManager> memMgr = pDoc->MemoryManager;

    GPtr<GFxXMLPrefix> pPfxObj = *memMgr->CreatePrefix(
        memMgr->CreateString(prefix.ToCStr(), prefix.GetSize()),
        memMgr->CreateString(uri.ToCStr(), uri.GetSize()));

    // Add prefix namespaces to prefix stack
    if (prefix.GetSize() > 0)
    {
        // Owner will be set when next start element is encountered.
        PrefixNamespaceStack.PushBack(PrefixOwnership(pPfxObj, NULL));
    }
    // Add default namespaces to dns stack
    else
    {
        // Why is this in a separate stack?
        // Elements without a prefix may inherit a default namespace
        // from any of its ancestors. keeping it in a separate stack
        // makes lookup Theta(1).
        // Owner will be set when next start element is encountered.
        DefaultNamespaceStack.PushBack(PrefixOwnership(pPfxObj, NULL));
    }
}


// 
// Text data reported by XML parser
//
void GFxXMLDOMBuilder::Characters(const GFxXMLStringRef& text)
{
#ifdef GFC_XML_DOCBUILDER_TRACE
    GFC_DEBUG_ASSERT(pLocator, "Locator not set!");
    char db0[MAX_DEBUG_STRING_LENGTH];
    SafeStringCopyWithSentinel(text.ToCStr(), text.GetSize(), db0, 
        MAX_DEBUG_STRING_LENGTH);
    GFC_DEBUG_MESSAGE3(1, 
        "[%d, %d] GFxXMLDocumentBuilder::Characters('%.16s')", 
        pLocator->Line, pLocator->Column, db0);
#endif

    // Copy the number of bytes loaded
    LoadedBytes = pLocator->LoadedBytes;

    GASSERT(ParseStack.GetSize() > 0);

    GPtr<GFxXMLObjectManager> memMgr = pDoc->MemoryManager;

    if (pAppendChainRoot.GetPtr() == NULL)
    {
        pAppendChainRoot = *memMgr->CreateTextNode(memMgr->EmptyString());
        AppendText.AppendString(text.ToCStr(), text.GetSize());
    }
    else
    {
        AppendText.AppendString(text.ToCStr(), text.GetSize());
    }
}


// 
// Ignored whitespace reported by XML parser
//
void GFxXMLDOMBuilder::IgnorableWhitespace(const GFxXMLStringRef& ws)
{
    GUNUSED(ws);
#ifdef GFC_XML_DOCBUILDER_TRACE
    GFC_DEBUG_ASSERT(pLocator, "Locator not set!");
    char db0[MAX_DEBUG_STRING_LENGTH];
    SafeStringCopyWithSentinel(ws.ToCStr(), ws.GetSize(), db0, 
        MAX_DEBUG_STRING_LENGTH);
    GFC_DEBUG_MESSAGE3(1, 
        "[%d, %d] GFxXMLDocumentBuilder::IgnorableWhitespace('%.16s')", 
        pLocator->Line, pLocator->Column, db0);
#endif

    // copy the number of bytes loaded
    LoadedBytes = pLocator->LoadedBytes;

    // Do nothing
}


// 
// Skipped entity reported by XML parser
//
void GFxXMLDOMBuilder::SkippedEntity(const GFxXMLStringRef& name)
{
    GUNUSED(name);
#ifdef GFC_XML_DOCBUILDER_TRACE
    GFC_DEBUG_ASSERT(pLocator, "Locator not set!");
    char db0[MAX_DEBUG_STRING_LENGTH];
    SafeStringCopyWithSentinel(name.ToCStr(), name.GetSize(), 
        db0, MAX_DEBUG_STRING_LENGTH);
    GFC_DEBUG_MESSAGE3(1, 
        "[%d, %d] GFxXMLDocumentBuilder::SkippedEntity('%.16s')", 
        pLocator->Line, pLocator->Column, db0);
#endif

    // Copy the number of bytes loaded
    LoadedBytes = pLocator->LoadedBytes;

    // Do nothing
}


// 
// Document locater registration
//
void GFxXMLDOMBuilder::SetDocumentLocator(const GFxXMLParserLocator* plocator)
{
#ifdef GFC_XML_DOCBUILDER_TRACE
    GFC_DEBUG_MESSAGE(1, "GFxXMLDocumentBuilder::SetDocumentLocator");
#endif

    pLocator = plocator;
}


// 
// Comment found by XML parser
//
void GFxXMLDOMBuilder::Comment(const GFxXMLStringRef& text)
{
    GUNUSED(text);
#ifdef GFC_XML_DOCBUILDER_TRACE
    GFC_DEBUG_ASSERT(pLocator, "Locator not set!");
    char db0[MAX_DEBUG_STRING_LENGTH];
    SafeStringCopyWithSentinel(text.ToCStr(), text.GetSize(), db0, 
        MAX_DEBUG_STRING_LENGTH);
    GFC_DEBUG_MESSAGE3(1, "[%d, %d] GFxXMLDocumentBuilder::Comment('%.16s')", 
        pLocator->Line, pLocator->Column, db0);
#endif

    // Copy the number of bytes loaded
    LoadedBytes = pLocator->LoadedBytes;

    // Do nothing
}


// 
// Error reported by XML parser
//
void GFxXMLDOMBuilder::Error(const GFxXMLParserException& exception)
{
    GUNUSED(exception);
#ifdef GFC_XML_DOCBUILDER_TRACE
    GFC_DEBUG_ASSERT(pLocator, "Locator not set!");
    char db0[MAX_DEBUG_STRING_LENGTH];
    SafeStringCopyWithSentinel(exception.ErrorMessage.ToCStr(), 
        exception.ErrorMessage.GetSize(), db0, MAX_DEBUG_STRING_LENGTH);
    GFC_DEBUG_MESSAGE3(1, "[%d, %d] GFxXMLDocumentBuilder::Error('%.64s')",
        pLocator->Line, pLocator->Column, db0);
#endif

    // Copy the number of bytes loaded
    LoadedBytes = pLocator->LoadedBytes;

    GFC_DEBUG_ERROR3(1, "Document builder @ line:%d col:%d - %.64s\n",
        pLocator->Line,
        pLocator->Column,
        exception.ErrorMessage.ToCStr());

    bError = true;
}


// 
// Fatal error reported by XML parser
//
void GFxXMLDOMBuilder::FatalError(const GFxXMLParserException& exception)
{
    GUNUSED(exception);
#ifdef GFC_XML_DOCBUILDER_TRACE
    GFC_DEBUG_ASSERT(pLocator, "Locator not set!");
    char db0[MAX_DEBUG_STRING_LENGTH];
    SafeStringCopyWithSentinel(exception.ErrorMessage.ToCStr(), 
        exception.ErrorMessage.GetSize(), db0, MAX_DEBUG_STRING_LENGTH);
    GFC_DEBUG_MESSAGE3(1, "[%d, %d] GFxXMLDocumentBuilder::FatalError('%.64s')",
        pLocator->Line, pLocator->Column, db0);
#endif

    // Copy the number of bytes loaded
    LoadedBytes = pLocator->LoadedBytes;

    GFC_DEBUG_ERROR3(1, "Document builder @ line:%d col:%d - %.64s\n",
        pLocator->Line,
        pLocator->Column,
        exception.ErrorMessage.ToCStr());

    bError = true;
}


// 
// Warning reported by XML parser
//
void GFxXMLDOMBuilder::Warning(const GFxXMLParserException& exception)
{
    GUNUSED(exception);
#ifdef GFC_XML_DOCBUILDER_TRACE
    GFC_DEBUG_ASSERT(pLocator, "Locator not set!");
    char db0[MAX_DEBUG_STRING_LENGTH];
    SafeStringCopyWithSentinel(exception.ErrorMessage.ToCStr(), 
        exception.ErrorMessage.GetSize(), db0, MAX_DEBUG_STRING_LENGTH);
    GFC_DEBUG_MESSAGE3(1, "[%d, %d] GFxXMLDocumentBuilder::Warning('%.64s')",
        pLocator->Line, pLocator->Column, db0);
#endif

    // Copy the number of bytes loaded
    LoadedBytes = pLocator->LoadedBytes;

    GFC_DEBUG_WARNING3(1, "Document builder @ line:%d col:%d - %.64s\n",
        pLocator->Line,
        pLocator->Column,
        exception.ErrorMessage.ToCStr());
}


//
// Return true if the text node only contains whitespace
//
bool CheckWhiteSpaceNode(GFxXMLTextNode* textNode)
{
    const char* buffer = textNode->Value.GetBuffer();
    bool allws = true;
    UInt32 utf8char;
    while ( (utf8char = GUTF8Util::DecodeNextChar(&buffer)) != 0 )
    {
        if (!G_iswspace((wchar_t)utf8char))
        {
            allws = false;
            break;
        }
    }
    return allws;
}

//
// Helper function for DOM traversal
//
void DropWhiteSpaceNodesHelper(GFxXMLElementNode* elemNode) 
{
    GFxXMLNode* child = elemNode->FirstChild;
    GFxXMLNode* temp = child;
    while (child != NULL)
    {
        temp = child->NextSibling;
        if (child->Type == GFxXMLElementNodeType)
            DropWhiteSpaceNodesHelper((GFxXMLElementNode*)child);
        else if (child->Type == GFxXMLTextNodeType)
        {
            if (CheckWhiteSpaceNode((GFxXMLTextNode*)child))
            {
                // Remove it
                elemNode->RemoveChild(child);
            }
        }
        child = temp;
    }
}


// 
// Drop all text nodes containing only whitespace from the DOM tree.
// This emulates the behavior of not processing unneccessary whitespace 
// in the XML document.
//
void GFxXMLDOMBuilder::DropWhiteSpaceNodes(GFxXMLDocument* document)
{
    for (GFxXMLNode* child = document->FirstChild; child != NULL; 
        child = child->NextSibling)
    {
        if (child->Type == GFxXMLElementNodeType)
            DropWhiteSpaceNodesHelper((GFxXMLElementNode*)child);
    }
}


#endif // #ifndef GFC_NO_XML_SUPPORT 
