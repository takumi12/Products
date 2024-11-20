/**********************************************************************

Filename    :   GFxXMLDocument.cpp
Content     :   XML SAX and DOM support
Created     :   December 13, 2007
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GConfig.h"

#ifndef GFC_NO_XML_SUPPORT

#include <GFxXMLDocument.h>
#include "XML/GFxXMLShadowRef.h"


// --------------------------------------------------------------------


// 
// Base node
//

GFxXMLNode::GFxXMLNode(GFxXMLObjectManager* memMgr, UByte type)
: MemoryManager(memMgr), Value(MemoryManager->EmptyString()), 
Parent(NULL), PrevSibling(NULL), pShadow(NULL), Type(type) {} 

GFxXMLNode::GFxXMLNode(GFxXMLObjectManager* memMgr, UByte type, 
                       GFxXMLDOMString value)
: MemoryManager(memMgr), Value(value), Parent(NULL), 
PrevSibling(NULL), pShadow(NULL), Type(type) {}

//
// Freeing shadow memory
//
GFxXMLNode::~GFxXMLNode()
{
    if (pShadow)
        delete pShadow;
}


// --------------------------------------------------------------------


// 
// Element node
//

GFxXMLElementNode::GFxXMLElementNode(GFxXMLObjectManager* memMgr)
: GFxXMLNode(memMgr, GFxXMLElementNodeType),
Prefix(memMgr->EmptyString()), Namespace(memMgr->EmptyString()),
FirstAttribute(NULL), LastAttribute(NULL),
LastChild(NULL) {}

GFxXMLElementNode::GFxXMLElementNode(GFxXMLObjectManager* memMgr, 
                                     GFxXMLDOMString value)
: GFxXMLNode(memMgr, GFxXMLElementNodeType, value), 
Prefix(memMgr->EmptyString()), Namespace(memMgr->EmptyString()),
FirstAttribute(NULL), LastAttribute(NULL), LastChild(NULL) {}

//
// Debug reporting and freeing attribute memory
//
GFxXMLElementNode::~GFxXMLElementNode()
{
#ifdef GFC_XML_OBJECT_REFCOUNTING_TRACE
    GFC_DEBUG_MESSAGE1(1, "GFxXMLElementNode::~GFxXMLElementNode : '%.16s'", 
        Value.ToCStr());
#endif

    // Remove children explicitly to avoid stack overflow caused by
    // destructor chain
    GFxXMLNode* currChild = LastChild, *prevChild = NULL;
    while (currChild)
    {
        // Remove parent references
        currChild->Parent = NULL;
        prevChild = currChild->PrevSibling;
        if (prevChild)
            prevChild->NextSibling = NULL;
        currChild = prevChild;
    }
    if (FirstChild)
        FirstChild = NULL;

    // Remove attributes manually
    for (GFxXMLAttribute* attr = FirstAttribute; attr != NULL; )
    {
        GFxXMLAttribute* atemp = attr->Next;
        delete attr;
        attr = atemp;
    }
}


//
// Deep or shallow copy of the element node
//
GFxXMLNode* GFxXMLElementNode::Clone(bool deep)
{
    GFxXMLElementNode* node = MemoryManager->CreateElementNode(Value);
    CloneHelper(node, deep);
    node->Type = Type;
    return node;
}


//
// Clone helper
//
void GFxXMLElementNode::CloneHelper(GFxXMLElementNode* clone, bool deep)
{
    // set the same prefix
    clone->Prefix = Prefix;
    // copy attributes
    for (GFxXMLAttribute* attr = FirstAttribute; attr != NULL; attr = attr->Next)
    {
        GFxXMLAttribute* newattr = MemoryManager->CreateAttribute(attr->Name, 
            attr->Value);
        clone->AddAttribute(newattr);
    }
    if (deep)
    {
        // recursive descent for children
        for (GFxXMLNode* child = FirstChild; child != NULL; 
            child = child->NextSibling)
        {            
            GPtr<GFxXMLNode> childClone = *child->Clone(deep);
            clone->AppendChild(childClone);
        }
    }
}


//
// Append a child to the end of the children linked list
//
void GFxXMLElementNode::AppendChild(GFxXMLNode* xmlNode)
{
    // Precondition check
    GASSERT(xmlNode);
    GASSERT(NULL == xmlNode->Parent);
    GASSERT(NULL == xmlNode->PrevSibling);
    GASSERT(NULL == xmlNode->NextSibling.GetPtr());

    if (LastChild == NULL)
    {
        // No children
        FirstChild = xmlNode;
        LastChild = xmlNode;
        xmlNode->Parent = this;
    }
    else
    {
        GASSERT(NULL == LastChild->NextSibling.GetPtr());
        // Has children
        xmlNode->PrevSibling = LastChild;
        LastChild->NextSibling = xmlNode;
        LastChild = xmlNode;
        xmlNode->Parent = this;
    }
}


//
// Insert a child before the specified child
//
void GFxXMLElementNode::InsertBefore(GFxXMLNode* child, GFxXMLNode* insert)
{
    // Precondition check
    GASSERT(FirstChild);
    GASSERT(LastChild);
    GASSERT(insert);
    GASSERT(this == insert->Parent);
    GASSERT(child);
    GASSERT(NULL == child->Parent);
    GASSERT(NULL == child->PrevSibling);
    GASSERT(NULL == child->NextSibling.GetPtr());

    GFxXMLNode* temp = insert->PrevSibling;
    insert->PrevSibling = child;
    child->PrevSibling = temp;
    child->NextSibling = insert;
    if (temp)
        temp->NextSibling = child;
    if (insert == FirstChild)
    {
        FirstChild = child;
    }
    child->Parent = this;
}


//
// Remove a child from the children list
//
void GFxXMLElementNode::RemoveChild(GFxXMLNode* xmlNode)
{
    // Precondition check
    GASSERT(xmlNode);
    GASSERT(xmlNode->Parent == this);

    GPtr<GFxXMLNode> temp = xmlNode;
    if (xmlNode == FirstChild)
    {
        FirstChild = xmlNode->NextSibling;
    }
    if (xmlNode == LastChild)
    {
        LastChild = xmlNode->PrevSibling;
    }
    if (xmlNode->NextSibling)
        xmlNode->NextSibling->PrevSibling = xmlNode->PrevSibling;
    if (xmlNode->PrevSibling)
        xmlNode->PrevSibling->NextSibling = xmlNode->NextSibling;
    xmlNode->Parent = NULL;
    xmlNode->NextSibling = NULL;
    xmlNode->PrevSibling = NULL;
}


//
// Return true if there are children
//
bool GFxXMLElementNode::HasChildren()
{
    return (NULL != FirstChild.GetPtr());
}


//
// Add an attribute to the attributes list
//
void GFxXMLElementNode::AddAttribute(GFxXMLAttribute* xmlAttrib)
{
    // Precondition check
    GASSERT(xmlAttrib);
    GASSERT(NULL == xmlAttrib->Next);

    if (FirstAttribute == NULL)
    {
        // No items in attribute list
        FirstAttribute = xmlAttrib;
        LastAttribute = xmlAttrib;
    }
    else
    {
        // There are items in attribute list
        GASSERT(LastAttribute->Next == NULL);
        LastAttribute->Next = xmlAttrib;
        LastAttribute = xmlAttrib;
    }
}

//
// Remove an attribute from the attributes list
//
bool GFxXMLElementNode::RemoveAttribute(const char* str, UInt len)
{
    // Find the attribute
    GFxXMLAttribute *found = NULL, *prev = NULL;
    for (GFxXMLAttribute* attr = FirstAttribute; attr != NULL; attr = attr->Next)
    {
        if (strncmp(str, attr->Name.ToCStr(), len) == 0)
        {
            found = attr;
            break;
        }
        prev = attr;
    }
    if (found)
    {
        if (FirstAttribute == found)
        {
            FirstAttribute = found->Next;
        }
        if (LastAttribute == found)
        {
            LastAttribute = prev;
        }
        if (prev)
        {
            prev->Next = found->Next;
        }
        delete found;
        return true;
    }
    else 
        return false;
}


//
// Remove all attributes from the attribute list
//
void GFxXMLElementNode::ClearAttributes()
{
    GFxXMLAttribute* temp = FirstAttribute;
    for (GFxXMLAttribute* attr = FirstAttribute; attr != NULL; )
    {
        temp = attr->Next;
        delete attr;
        attr = temp;
    }
    FirstAttribute = NULL;
    LastAttribute = NULL;
}


//
// Return true if there are attributes in the attribute list
//
bool GFxXMLElementNode::HasAttributes()
{
    return (NULL != FirstAttribute);
}


// --------------------------------------------------------------------


// 
// Text node
//

GFxXMLTextNode::GFxXMLTextNode(GFxXMLObjectManager* memMgr, GFxXMLDOMString value)
: GFxXMLNode(memMgr, GFxXMLTextNodeType, value) {}

// Debug reporting
GFxXMLTextNode::~GFxXMLTextNode()
{
#ifdef GFC_XML_OBJECT_REFCOUNTING_TRACE
    GFC_DEBUG_MESSAGE1(1, "GFxXMLTextNode::~GFxXMLElementNode : '%.16s'", Value.ToCStr());
#endif
}

//
// Deep copy or shallow copy of the text node.
//
// Either performs the same operation.
//
GFxXMLNode* GFxXMLTextNode::Clone(bool deep)
{
    GUNUSED(deep);
    GFxXMLTextNode* clone = MemoryManager->CreateTextNode(Value);
    clone->Type = Type;
    return clone;
}


// --------------------------------------------------------------------


// 
// Document
//

GFxXMLDocument::GFxXMLDocument(GFxXMLObjectManager* memMgr)
: GFxXMLElementNode(memMgr), XMLVersion(MemoryManager->EmptyString()),
Encoding(MemoryManager->EmptyString()), Standalone(-1) {}

// Debug reporting
GFxXMLDocument::~GFxXMLDocument()
{
#ifdef GFC_XML_OBJECT_REFCOUNTING_TRACE
    GFC_DEBUG_MESSAGE(1, "GFxXMLDocument::~GFxXMLDocument");
#endif
}

//
// Deep or shallow copy of the DOM tree root
//
GFxXMLNode* GFxXMLDocument::Clone(bool deep)
{
    GFxXMLDocument* doc = MemoryManager->CreateDocument();
    GFxXMLElementNode::CloneHelper(doc, deep);
    doc->Encoding = Encoding;
    doc->Standalone = Standalone;
    doc->XMLVersion = XMLVersion;
    doc->Type = Type;
    return doc;
}

#endif // #ifndef GFC_NO_XML_SUPPORT 
