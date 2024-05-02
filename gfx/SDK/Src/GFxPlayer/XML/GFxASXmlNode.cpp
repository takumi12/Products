/**********************************************************************

Filename    :   GFxASXmlNode.cpp
Content     :   XMLNode reference class for ActionScript 2.0
Created     :   11/30/2007
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

//
// Known issues and notes:
//
// - Custom properties cannot be set and retrieved. The properties will
//   be destroyed once the ref count goes to 0.
// - After appending a child that already has parent, the child's 
//   namespace is not resolved in Flash. GFx fixes this obvious 'bug'.
//

#include "GConfig.h"

#ifndef GFC_NO_XML_SUPPORT

#include "GFxPlayerImpl.h"
#include "GFxAction.h"
#include "XML/GFxXMLShadowRef.h"
#include "XML/GFxASXmlNode.h"
#include "AS/GASArrayObject.h"

#include "GHeapNew.h"


//
// Temporary descriptor of XMLNode properties
//
struct GASXmlNodeObject_MemberTableType
{
    const char *                        pName;
    GASXmlNodeObject::StandardMember    Id;
};

//
// Mapping between const char* and standard member enum
//
GASXmlNodeObject_MemberTableType GASXmlNodeObject_MemberTable[] =
{
    { "attributes",         GASXmlNodeObject::M_attributes },
    { "childNodes",         GASXmlNodeObject::M_childNodes },
    { "firstChild",         GASXmlNodeObject::M_firstChild },
    { "lastChild",          GASXmlNodeObject::M_lastChild },
    { "localName",          GASXmlNodeObject::M_localName },
    { "namespaceURI",       GASXmlNodeObject::M_namespaceURI },
    { "nextSibling",        GASXmlNodeObject::M_nextSibling },
    { "nodeName",           GASXmlNodeObject::M_nodeName },
    { "nodeType",           GASXmlNodeObject::M_nodeType },
    { "nodeValue",          GASXmlNodeObject::M_nodeValue },
    { "parentNode",         GASXmlNodeObject::M_parentNode },
    { "prefix",             GASXmlNodeObject::M_prefix },
    { "previousSibling",    GASXmlNodeObject::M_previousSibling },

    // Done.
    { 0,                    GASXmlNodeObject::M_InvalidMember }
};

//
// Looks up an XMLNode standard member and returns M_InvalidMember if it is not found.
//
GASXmlNodeObject::StandardMember  GASXmlNodeObject::GetStandardMemberConstant(GASEnvironment* penv, 
                                                                              const GASString& memberName) const
{
    SByte   memberConstant = M_InvalidMember; 
    if (memberName.IsStandardMember())
    {        
        GASXmlNodeProto* xmlNodeProto = 
            static_cast<GASXmlNodeProto*>(penv->GetPrototype(GASBuiltin_XMLNode));
        GASSERT(xmlNodeProto);
        xmlNodeProto->XMLNodeMemberMap.GetCaseCheck(memberName, &memberConstant, true);
    }        
    return (StandardMember) memberConstant;
}

//
// Initialize the XMLNode members in the global contexts
//
// This will be used in GetMember/SetMember for fast member comparison
//
void GASXmlNodeObject::InitializeStandardMembers(GASGlobalContext& gc, GASStringHash<SByte>& hash)
{
    GCOMPILER_ASSERT( (sizeof(GASXmlNodeObject_MemberTable)/sizeof(GASXmlNodeObject_MemberTable[0]))
        == M_XMLNodeMemberCount + 1);

    // Add all standard members.
    GASXmlNodeObject_MemberTableType* pentry;
    GASStringManager* pstrManager = gc.GetStringManager();

    hash.SetCapacity(M_XMLNodeMemberCount);  

    for (pentry = GASXmlNodeObject_MemberTable; pentry->pName; pentry++)
    {
        GASString name(pstrManager->CreateConstString(pentry->pName, strlen(pentry->pName),
            GASString::Flag_StandardMember));
        hash.Add(name, (SByte)pentry->Id);
    }
}


// 
// Setup the attributes of an element node. This method offloads any
// attributes defined in the C++ side to the AS side for correct behavior.
//
static void SetupAttributes(GASEnvironment* penv, GFxXMLElementNode* preal)
{
    // Special logic to handle attributes
    // Since attributes can hold object references as values, the functionality 
    // provided by the C++ DOM is inadaquete and not compatible. Therefore we will
    // offload all attributes to the AS Object.

    GFxXMLShadowRef* pshadow = preal->pShadow;
    GASStringContext* psc = penv->GetSC();
    pshadow->pAttributes = *GHEAP_NEW(penv->GetHeap()) GASObject(penv);
    if (preal->HasAttributes())
    {
        // this code will execute once for unprocessed nodes
        for (GFxXMLAttribute* attr = preal->FirstAttribute; attr != NULL; attr = attr->Next)
        {
            pshadow->pAttributes->SetMember(penv, psc->CreateString(attr->Name.ToCStr()), 
                psc->CreateString(attr->Value.ToCStr()));
        }
        preal->ClearAttributes();
    }
}


//
// Setup the given AS xml node object as a shadow for a real node.
//
static void SetupShadow(GASEnvironment* penv, GFxXMLNode* preal, GASXmlNodeObject* asobj)
{
    GASSERT(preal);
    if (!preal->pShadow)
    {
        preal->pShadow = preal->MemoryManager->CreateShadowRef();
        if (preal->Type == GFxXMLElementNodeType)
        {
            GFxXMLElementNode* pnode = static_cast<GFxXMLElementNode*>(preal);
            SetupAttributes(penv, pnode);
        }
    }
    GFxXMLShadowRef* pshadow = preal->pShadow;
    pshadow->pASNode = asobj;
    asobj->pRealNode = preal;
}


//
// Create an actionscript shadow for a real node (C++).
//
static GPtr<GASXmlNodeObject> CreateShadow(GASEnvironment* penv, GFxXMLNode* preal, GFxASXMLRootNode* proot)
{
    GASSERT(preal);
    GPtr<GASXmlNodeObject> asobj = *GHEAP_NEW(penv->GetHeap()) GASXmlNodeObject(penv);
    SetupShadow(penv, preal, asobj);
    // Setup root node
    if (proot)
        asobj->pRootNode = proot;
    else
        asobj->pRootNode = *preal->MemoryManager->CreateRootNode(preal);
    return asobj;
}


//
// Fill the GASObject with id-name -> XMLNode mappings
//
void GFx_Xml_CreateIDMap(GASEnvironment* penv, GFxXMLElementNode* elemNode, GFxASXMLRootNode* proot, GASObject* pobj) 
{
    GFxXMLDOMStringCompareWrapper idstr("id", 2);
    for (GFxXMLNode* child = elemNode->FirstChild; child != NULL; child = child->NextSibling)
    {
        if (child->Type == GFxXMLElementNodeType)
        {
            GFx_Xml_CreateIDMap(penv, (GFxXMLElementNode*)child, proot, pobj);

            // Find attribute with id name
            GFxXMLElementNode* pnode = static_cast<GFxXMLElementNode*>(child);
            for (GFxXMLAttribute* attr = pnode->FirstAttribute; attr != NULL; attr = attr->Next)
            {
                if (attr->Name == idstr)
                {
					GFxString value(attr->Value.ToCStr());
                    GPtr<GASXmlNodeObject> pchildobj = NULL;
                    // pnode does not have a shadow; create one
                    if (NULL == pnode->pShadow)
                        pchildobj = CreateShadow(penv, pnode, proot);
                    else
                    {
                        // pnode does not have an AS counterpart
                        GFxXMLShadowRef* pshadow = pnode->pShadow;
                        pchildobj = pshadow->pASNode;
                        if (NULL == pchildobj.GetPtr())
                        {
                            pchildobj = *GHEAP_NEW(penv->GetHeap()) GASXmlNodeObject(penv);
                            SetupShadow(penv, pnode, pchildobj);
                        }
                    }

                    pobj->SetMember(penv, penv->CreateString(value), 
                        GASValue(pchildobj));

                    // Break on first attribute matched
                    break;
                }
            }
        }
    }
}


//
// Resolve the namespace of the given element node. This method first looks
// at the attributes of the node itself to determine a namespace declaration.
// If a matching namespace is not found, then it traverses its parent chain to 
// find one.
//
static void ResolveNamespace(GASEnvironment* penv, GFxXMLElementNode* elemNode, GFxASXMLRootNode* proot)
{
    GASStringContext* psc = penv->GetSC();
    // Create the property query
    GASString pfxquery = psc->CreateString("xmlns", 5);
    if (elemNode->Prefix.GetSize() > 0)
    {
        pfxquery += ":";
        pfxquery += elemNode->Prefix.ToCStr();
    }
    GASValue qval(GASValue::UNDEFINED);

    // -- Reset the namespace
    GASSERT(elemNode);
    elemNode->Namespace = elemNode->MemoryManager->EmptyString();

    // -- Look for namespace declaration in self first
    GASSERT(elemNode->pShadow);
    GFxXMLShadowRef* pshadow = elemNode->pShadow;
    GASSERT(pshadow);
    GASObject* pattribs = pshadow->pAttributes;
    GASSERT(pattribs);
    pattribs->GetMember(penv, pfxquery, &qval);
    if (!qval.IsUndefined())
    {
        // Found ns declaration in self; bail
        GASString ns = qval.ToString(penv);
        elemNode->Namespace = elemNode->MemoryManager->CreateString(ns.ToCStr(), 
            ns.GetSize());
        return;
    }

    // -- Traverse up parent chain and look for a namespace
    GFxXMLElementNode* parent = static_cast<GFxXMLElementNode*>(elemNode->Parent);
    while (parent)
    {
        // create a shadow if one does not exist
        if (NULL == parent->pShadow)
            CreateShadow(penv, parent, proot);
        pshadow = parent->pShadow;
        GASSERT(pshadow);
        GASObject* pattribs = pshadow->pAttributes;
        GASSERT(pattribs);
        // Lookup prefix in properties
        pattribs->GetMember(penv, pfxquery, &qval);
        if (!qval.IsUndefined())
        {
            GASString ns = qval.ToString(penv);
            elemNode->Namespace = elemNode->MemoryManager->CreateString(ns.ToCStr(), 
                ns.GetSize());
            break;
        }
        parent = static_cast<GFxXMLElementNode*>(parent->Parent);
    }

}


// 
// Constructor
//
GASXmlNodeObject::GASXmlNodeObject(GASEnvironment* penv) 
: GASObject(penv), pRealNode(NULL)
{
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_XMLNode));    
}


// 
// Destructor
//
GASXmlNodeObject::~GASXmlNodeObject()
{
    // Remove itself as the realnode's (C++) shadow reference
    if (pRealNode && pRealNode->pShadow)
    {
        GFxXMLShadowRef* pshadow = pRealNode->pShadow;
        pshadow->pASNode = NULL;
    }
}


// 
// Overloaded SetMember function to intercept and process builtin properties
// Since the actual data is stored in the realnode, each property needs to be
// intercepted and processed individually.
//
bool GASXmlNodeObject::SetMember(GASEnvironment* penv, const GASString& name, 
                                 const GASValue& val, 
                                 const GASPropFlags& flags)
{
    GFxLog* log = penv->GetLog();
    // Try to handle this property
    if (pRealNode)
    {         
        StandardMember member = GetStandardMemberConstant(penv, name);

        switch (member)
        {

        // ***** XMLNode.nodeName
        case M_nodeName:

            if (pRealNode)
            {
                // Flash Doc: If the XML object is an XML element (nodeType == 1), nodeName is 
                // the name of the tag that represents the node in the XML file. For example, 
                // TITLE is the nodeName of an HTML TITLE tag. If the XML object is a text node 
                // (nodeType == 3), nodeName is null.
                //
                if (pRealNode->Type == GFxXMLElementNodeType)
                {
                    GFxXMLElementNode* elemNode = static_cast<GFxXMLElementNode*>(pRealNode);
                    // -- Look for prefix declaration, if one exists in node name
                    GFxXMLDOMString prefix( pRealNode->MemoryManager->EmptyString() );
                    GASString str = val.ToString(penv);
                    const char* data = str.ToCStr();
                    const char* colon = strchr(data, ':');
                    if (colon)
                    {
                        // Check prefix
                        prefix = pRealNode->MemoryManager->CreateString(data, colon-data);
                        // Set the localname
                        pRealNode->Value = pRealNode->MemoryManager->CreateString(
                            colon+1, G_strlen(colon+1));
                    }
                    else
                    {
                        // If node already contains default namespace, it is preserved.
                        pRealNode->Value = pRealNode->MemoryManager->CreateString(str.ToCStr(), 
                            str.GetSize());
                    }
                    elemNode->Prefix = prefix;

                    ResolveNamespace(penv, elemNode, pRootNode);
                }
                else
                {
                    if (log != NULL)
                        log->LogScriptWarning("Warning: XMLNodeObject::SetMember"
                            " - cannot set nodeName of node type %d. Only type 1 allowed\n", 
                            pRealNode->Type);
                }
            }
            else
            {
                if (log != NULL)
                    log->LogScriptWarning("Warning: XMLNodeObject::SetMember"
                        " - cannot set nodeName of a malformed node\n");
            }
            return true;


        // ***** XMLNode.nodeValue
        case M_nodeValue:

            if (pRealNode && pRealNode->Type != GFxXMLElementNodeType)
            {
                // Flash Doc: If the XML object is a text node, the nodeType is 3, and the nodeValue 
                // is the text of the node. If the XML object is an XML element (nodeType is 1), 
                // nodeValue is null and read-only
                //
                GASString str = val.ToString(penv);
                pRealNode->Value = pRealNode->MemoryManager->CreateString(str.ToCStr(), 
                    str.GetSize());
            }
            else
            {
                if (log != NULL)
                    log->LogScriptWarning("Warning! XMLNodeObject::SetMember"
                        " - cannot set nodeValue of a malformed node\n");
            }
            return true;


        // ***** XMLNode.attributes
        case M_attributes:

            if (pRealNode)
            {
                // Flash Doc: The XML.attributes object contains one variable for each attribute 
                // of the XML instance. Because these variables are defined as part of the 
                // object, they are generally referred to as properties of the object. The value 
                // of each attribute is stored in the corresponding property as a string.
                //
                if (pRealNode->Type == GFxXMLElementNodeType)
                {
                    GASSERT(pRealNode);
                    GFxXMLShadowRef* pshadow = pRealNode->pShadow;
                    GASSERT(pshadow);
                    pshadow->pAttributes = val.ToObject(penv);
                }
                else
                {
                    if (log != NULL)
                        log->LogScriptWarning("Warning: XMLNodeObject::SetMember"
                            " - cannot set attributes of node type %d. Only type 1 allowed\n", 
                            pRealNode->Type);
                }
            }
            else
            {
                if (log != NULL)
                    log->LogScriptWarning("Warning: XMLNodeObject::SetMember"
                        " - cannot set attributes of a malformed node\n");
            }
            return true;

        default:
            {
                // Pass along to GASObject
            }
        }

    }
    return GASObject::SetMember(penv, name, val, flags);
}


// 
// Overloaded GetMember function to intercept and process builtin properties
// Since the actual data is stored in the realnode, each property needs to be
// intercepted and processed individually.
//
bool GASXmlNodeObject::GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
{
    GASStringContext* psc = penv->GetSC();
    // Try to handle this property
    if (pRealNode)
    {
        StandardMember member = GetStandardMemberConstant(penv, name);

        switch (member)
        {

        // ***** XMLNode.firstChild
        case M_firstChild:
        
            if (pRealNode) 
            {
                if (pRealNode->Type == GFxXMLElementNodeType)
                {
                    // Flash Doc: This property is null if the node does not have 
                    // children. This property is undefined if the node is a text node. 
                    // This is a read-only property and cannot be used to manipulate 
                    // child nodes; use the appendChild(), insertBefore(), and 
                    // removeNode() methods to manipulate child nodes.
                    //
                    GFxXMLElementNode* pnode = static_cast<GFxXMLElementNode*>(pRealNode);
                    if (pnode->HasChildren())
                    {
                        GFxXMLShadowRef* pshadow = pnode->FirstChild->pShadow;
                        if (NULL == pshadow || NULL == pshadow->pASNode)
                            val->SetAsObject(CreateShadow(penv, pnode->FirstChild, pRootNode));
                        else
                            val->SetAsObject(static_cast<GASObject*>(pshadow->pASNode));
                    }
                    else
                        val->SetNull();
                }
                else
                    val->SetNull();
            }
            else
                val->SetUndefined();
            return true;
        

        // ***** XMLNode.nextSibling
        case M_nextSibling:
        
            if (pRealNode)
            {
                // Flash Doc: This property is null if the node does not have 
                // a next sibling node. This property cannot be used to manipulate 
                // child nodes; use the appendChild(), insertBefore(), and 
                // removeNode() methods to manipulate child nodes.
                //
                GFxXMLNode* pnode = static_cast<GFxXMLNode*>(pRealNode);
                if (pnode->NextSibling)
                {
                    GFxXMLShadowRef* pshadow = pnode->NextSibling->pShadow;
                    if (NULL == pshadow || NULL == pshadow->pASNode)
                        val->SetAsObject(CreateShadow(penv, pnode->NextSibling, pRootNode));
                    else
                        val->SetAsObject(static_cast<GASObject*>(pshadow->pASNode));
                }
                else
                    val->SetNull();
            }
            else
                val->SetUndefined();
            return true;
        

        // ***** XMLNode.childNodes
        case M_childNodes:
        
            if (pRealNode && pRealNode->Type == GFxXMLElementNodeType)
            {
                // Flash Doc: Each element in the array is a reference to an XML object 
                // that represents a child node. This is a read-only property and cannot 
                // be used to manipulate child nodes. Use the appendChild(), insertBefore(), 
                // and removeNode() methods to manipulate child nodes. This property is 
                // undefined for text nodes (nodeType == 3).
                //
                // fill array with children
                GPtr<GASArrayObject> parr = *GHEAP_NEW(penv->GetHeap()) GASArrayObject(penv);
                GFxXMLElementNode* pnode = static_cast<GFxXMLElementNode*>(pRealNode);
                for (GFxXMLNode* child = pnode->FirstChild; child != NULL; 
                    child = child->NextSibling)
                {
                    GFxXMLShadowRef* pshadow = child->pShadow;
                    if (NULL == pshadow || NULL == pshadow->pASNode)
                        parr->PushBack(GASValue(CreateShadow(penv, child, pRootNode)));
                    else
                        parr->PushBack( pshadow->pASNode );
                }
                val->SetAsObject(parr);
            }
            else
                val->SetUndefined();
            return true;
        

        // ***** XMLNode.nodeName
        case M_nodeName:
        
            if (pRealNode)
            {
                // Flash Doc: If the XML object is an XML element (nodeType == 1), nodeName is 
                // the name of the tag that represents the node in the XML file. For example, 
                // TITLE is the nodeName of an HTML TITLE tag. If the XML object is a text node 
                // (nodeType == 3), nodeName is null.
                //
                if (pRealNode->Type == GFxXMLElementNodeType)
                {
                    GFxXMLElementNode* pelem = static_cast<GFxXMLElementNode*>(pRealNode);
                    if (pelem->Prefix.GetSize() > 0)
                    {
                        GASString str = psc->CreateString(pelem->Prefix.ToCStr());
                        //GASString str = psc->CreateConstString(pelem->Prefix.ToCStr());
                        str += ":";
                        str += pelem->Value.ToCStr();
                        val->SetString(str);
                    }
                    else
                    {
                        val->SetString(psc->CreateString(pelem->Value.ToCStr()));
                        //val->SetString(psc->CreateConstString(pelem->Value.ToCStr()));
                    }
                }
                else
                    val->SetNull();
            }
            else
                val->SetUndefined();
            return true;
        

        // ***** XMLNode.nodeValue
        case M_nodeValue:
        
            if (pRealNode)
            {
                // Flash Doc: If the XML object is a text node, the nodeType is 3, and the nodeValue 
                // is the text of the node. If the XML object is an XML element (nodeType is 1), 
                // nodeValue is null and read-only
                //
                if (pRealNode->Type == GFxXMLElementNodeType)
                    val->SetNull();
                else
                    val->SetString(psc->CreateString(pRealNode->Value.ToCStr()));
                    //val->SetString(psc->CreateConstString(pRealNode->Value.ToCStr()));
            }
            else
                val->SetUndefined();
            return true;
        

        // ***** XMLNode.attributes
        case M_attributes:
        
            if (pRealNode) 
            {             
                // Flash Doc: The XML.attributes object contains one variable for each attribute 
                // of the XML instance. Because these variables are defined as part of the 
                // object, they are generally referred to as properties of the object. The value 
                // of each attribute is stored in the corresponding property as a string.
                //
                GASSERT(pRealNode);
                GFxXMLShadowRef* pshadow = pRealNode->pShadow;
                GASSERT(pshadow);
                if (NULL == pshadow->pAttributes.GetPtr())
                    pshadow->pAttributes = *GHEAP_NEW(penv->GetHeap()) GASObject(penv);
                val->SetAsObject(pshadow->pAttributes);
            }
            else
                val->SetUndefined();
            return true;

        

        // ***** XMLNode.lastChild
        case M_lastChild:
        
            if (pRealNode)
            {
                if (pRealNode->Type == GFxXMLElementNodeType)
                {
                    // Flash Doc: The XML.lastChild property is null if the node 
                    // does not have children. This property cannot be used to 
                    // manipulate child nodes; use the appendChild(), insertBefore(), 
                    // and removeNode() methods to manipulate child nodes.
                    //
                    GFxXMLElementNode* pnode = static_cast<GFxXMLElementNode*>(pRealNode);
                    if (pnode->HasChildren())
                    {
                        GFxXMLShadowRef* pshadow = pnode->LastChild->pShadow;
                        if (NULL == pshadow || NULL == pshadow->pASNode)
                            val->SetAsObject(CreateShadow(penv, pnode->LastChild, pRootNode));
                        else
                            val->SetAsObject(static_cast<GASObject*>(pshadow->pASNode));
                    }
                    else
                        val->SetNull();
                }
                else
                    val->SetNull();
            }
            else
                val->SetUndefined();
            return true;
        

        // ***** XMLNode.previousSibling
        case M_previousSibling:
        
            if (pRealNode)
            {
                // Flash Doc: The property has a value of null if the node does not 
                // have a previous sibling node. This property cannot be used to 
                // manipulate child nodes; use the appendChild(), insertBefore(), 
                // and removeNode() methods to manipulate child nodes.
                //
                GFxXMLNode* pnode = static_cast<GFxXMLNode*>(pRealNode);
                if (pnode->PrevSibling)
                {
                    GFxXMLShadowRef* pshadow = pnode->PrevSibling->pShadow;
                    if (NULL == pshadow || NULL == pshadow->pASNode)
                        val->SetAsObject(CreateShadow(penv, pnode->PrevSibling, pRootNode));
                    else
                        val->SetAsObject(static_cast<GASObject*>(pshadow->pASNode));
                }
                else
                    val->SetNull();
            }
            else
                val->SetUndefined();
            return true;
        

        // ***** XMLNode.parentNode
        case M_parentNode:
        
            if (pRealNode)
            {
                // Flash Doc: An XMLNode value that references the parent node of 
                // the specified XML object, or returns null if the node has no 
                // parent. This is a read-only property and cannot be used to 
                // manipulate child nodes; use the appendChild(), insertBefore(), 
                // and removeNode() methods to manipulate child nodes.
                //
                GFxXMLNode* pnode = static_cast<GFxXMLNode*>(pRealNode);
                if (pnode->Parent)
                {
                    GFxXMLShadowRef* pshadow = pnode->Parent->pShadow;
                    if (NULL == pshadow || NULL == pshadow->pASNode)
                        val->SetAsObject(CreateShadow(penv, pnode->Parent, pRootNode));
                    else
                        val->SetAsObject(static_cast<GASObject*>(pshadow->pASNode));
                }
                else
                    val->SetNull();
            }
            else
                val->SetUndefined();
            return true;
        

        // ***** XMLNode.localName
        case M_localName:
        
            if (pRealNode && pRealNode->Type == GFxXMLElementNodeType)
            {
                // Flash Doc: This is the element name without the namespace 
                // prefix. For example, the node 
                // <contact:mailbox/>bob@example.com</contact:mailbox> has the 
                // local name "mailbox", and the prefix "contact", which comprise 
                // the full element name "contact.mailbox". You can access the 
                // namespace prefix via the prefix property of the XML node object. 
                // The nodeName property returns the full name (including the 
                // prefix and the local name).
                //
                val->SetString(psc->CreateString(pRealNode->Value.ToCStr()));
                //val->SetString(psc->CreateConstString(pRealNode->Value.ToCStr()));
            }
            else
                val->SetUndefined();
            return true;
        

        // ***** XMLNode.namespaceURI
        case M_namespaceURI:
        
            if (pRealNode && pRealNode->Type == GFxXMLElementNodeType)
            {
                // Flash Doc: If the XML node has a prefix, namespaceURI is the 
                // value of the xmlns declaration for that prefix (the URI), 
                // which is typically called the namespace URI. The xmlns 
                // declaration is in the current node or in a node higher in 
                // the XML hierarchy. If the XML node does not have a prefix, 
                // the value of the namespaceURI property depends on whether 
                // there is a default namespace defined (as in 
                // xmlns="http://www.example.com/"). If there is a default 
                // namespace, the value of the namespaceURI property is the 
                // value of the default namespace. If there is no default 
                // namespace, the namespaceURI property for that node is an 
                // empty string (""). You can use the getNamespaceForPrefix() 
                // method to identify the namespace associated with a specific 
                // prefix. The namespaceURI* [*PPS: this should be 'prefix'] 
                // property returns the prefix associated with the node name.
                //
                GFxXMLElementNode* pnode = static_cast<GFxXMLElementNode*>(pRealNode);
                val->SetString(psc->CreateString(pnode->Namespace.ToCStr()));
                //val->SetString(psc->CreateConstString(pnode->Namespace.ToCStr()));
            }
            else
                val->SetUndefined();
            return true;
        

        // ***** XMLNode.prefix
        case M_prefix:
        
            if (pRealNode && pRealNode->Type == GFxXMLElementNodeType)
            {
                // Flash Doc: For example, the node 
                // <contact:mailbox/>bob@example.com</contact:mailbox> 
                // prefix "contact" and the local name "mailbox", which comprise 
                // the full element name "contact.mailbox". The nodeName property 
                // of an XML node object returns the full name (including the 
                // prefix and the local name). You can access the local name 
                // portion of the element's name via the localName property. 
                //
                GFxXMLElementNode* pnode = static_cast<GFxXMLElementNode*>(pRealNode);
                val->SetString(psc->CreateString(pnode->Prefix.ToCStr()));
                //val->SetString(psc->CreateConstString(pnode->Prefix.ToCStr()));
            }
            else
                val->SetUndefined();
            return true;
        

        // ***** XMLNode.nodeType
        case M_nodeType:
        
            // Flash Doc: A nodeType value, either 1 for an XML element or 3 for a text node. 
            // The nodeType is a numeric value from the NodeType enumeration in the W3C DOM 
            // Level 1 recommendation: 
            // www.w3.org/tr/1998/REC-DOM-Level-1-19981001/level-one-core.html.
            //
            if (pRealNode)
                val->SetNumber(pRealNode->Type);
            else
                val->SetUndefined();
            return true;
        
        default:
            {
                // Pass along to GASObject
            }
            
        }
    }

    // if property wasn't gobbled up by the custom handler, pass it along to the base 
    // class
    return GASObject::GetMember(penv, name, val);
}


// 
// AS to GFx function mapping
//
const GASNameFunction GASXmlNodeProto::FunctionTable[] = 
{
    { "appendChild", &GASXmlNodeProto::AppendChild },
    { "cloneNode", &GASXmlNodeProto::CloneNode },
    { "getNamespaceForPrefix", &GASXmlNodeProto::GetNamespaceForPrefix },
    { "getPrefixForNamespace", &GASXmlNodeProto::GetPrefixForNamespace },
    { "hasChildNodes", &GASXmlNodeProto::HasChildNodes },
    { "insertBefore", &GASXmlNodeProto::InsertBefore },
    { "removeNode", &GASXmlNodeProto::RemoveNode },
    { "toString", &GASXmlNodeProto::ToString },
    { 0, 0 }
};


// 
// GASXmlNode prototype constructor
//
GASXmlNodeProto::GASXmlNodeProto(GASStringContext *psc, GASObject* prototype, 
                                 const GASFunctionRef& constructor) :
    GASPrototype<GASXmlNodeObject>(psc, prototype, constructor)
{
    // make the functions enumerable
    InitFunctionMembers(psc, FunctionTable, GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete);

    GASXmlNodeObject::SetMemberRaw(psc, psc->CreateConstString("attributes"), 
        GASValue::UNDEFINED, GASPropFlags::PropFlag_DontDelete);
    GASXmlNodeObject::SetMemberRaw(psc, psc->CreateConstString("childNodes"), 
        GASValue::UNDEFINED, GASPropFlags::PropFlag_DontDelete | 
        GASPropFlags::PropFlag_ReadOnly);
    GASXmlNodeObject::SetMemberRaw(psc, psc->CreateConstString("firstChild"), 
        GASValue::UNDEFINED, GASPropFlags::PropFlag_DontDelete | 
        GASPropFlags::PropFlag_ReadOnly);
    GASXmlNodeObject::SetMemberRaw(psc, psc->CreateConstString("lastChild"), 
        GASValue::UNDEFINED, GASPropFlags::PropFlag_DontDelete | 
        GASPropFlags::PropFlag_ReadOnly);  
    GASXmlNodeObject::SetMemberRaw(psc, psc->CreateConstString("localName"), 
        GASValue::UNDEFINED, GASPropFlags::PropFlag_DontDelete | 
        GASPropFlags::PropFlag_ReadOnly);
    GASXmlNodeObject::SetMemberRaw(psc, psc->CreateConstString("namespaceURI"), 
        GASValue::UNDEFINED, GASPropFlags::PropFlag_DontDelete | 
        GASPropFlags::PropFlag_ReadOnly);
    GASXmlNodeObject::SetMemberRaw(psc, psc->CreateConstString("nextSibling"), 
        GASValue::UNDEFINED, GASPropFlags::PropFlag_DontDelete | 
        GASPropFlags::PropFlag_ReadOnly);
    GASXmlNodeObject::SetMemberRaw(psc, psc->CreateConstString("nodeName"), 
        GASValue::UNDEFINED, GASPropFlags::PropFlag_DontDelete);
    GASXmlNodeObject::SetMemberRaw(psc, psc->CreateConstString("nodeType"), 
        GASValue::UNDEFINED, GASPropFlags::PropFlag_DontDelete | 
        GASPropFlags::PropFlag_ReadOnly);
    GASXmlNodeObject::SetMemberRaw(psc, psc->CreateConstString("nodeValue"), 
        GASValue::UNDEFINED, GASPropFlags::PropFlag_DontDelete);  
    GASXmlNodeObject::SetMemberRaw(psc, psc->CreateConstString("parentNode"), 
        GASValue::UNDEFINED, GASPropFlags::PropFlag_DontDelete | 
        GASPropFlags::PropFlag_ReadOnly);
    GASXmlNodeObject::SetMemberRaw(psc, psc->CreateConstString("prefix"), 
        GASValue::UNDEFINED, GASPropFlags::PropFlag_DontDelete | 
        GASPropFlags::PropFlag_ReadOnly);
    GASXmlNodeObject::SetMemberRaw(psc, psc->CreateConstString("previousSibling"), 
        GASValue::UNDEFINED, GASPropFlags::PropFlag_DontDelete | 
        GASPropFlags::PropFlag_ReadOnly);
}


// 
// XMLNode.appendChild(newChild:XMLNode) : Void
//
void GASXmlNodeProto::AppendChild(const GASFnCall& fn)
{
    // Flash Doc: Appends the specified node to the XML object's child list. This method 
    // operates directly on the node referenced by the childNode parameter; it does not 
    // append a copy of the node. If the node to be appended already exists in another 
    // tree structure, appending the node to the new location will remove it from its 
    // current location. If the childNode parameter refers to a node that already exists 
    // in another XML tree structure, the appended child node is placed in the new tree 
    // structure after it is removed from its existing parent node.
    //
    CHECK_THIS_PTR2(fn, XMLNode, XML);
    GASXmlNodeObject* pthis = (GASXmlNodeObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    GFxLog* log = fn.GetLog();
    if (pthis->pRealNode)
    {        
        if (pthis->pRealNode->Type == GFxXMLElementNodeType)
        {
            // Only works for element nodes
            GFxXMLElementNode* elemNode = (GFxXMLElementNode*) pthis->pRealNode;
            // Get new child
            if (fn.NArgs > 0)
            {
                GASObject* p = fn.Arg(0).ToObject(fn.Env);
                if ( NULL == p || p->GetObjectType() != Object_XMLNode )
                {
                    if (log != NULL)
                        log->LogScriptWarning("Warning! XMLNode::appendChild"
                            " - trying to add a child that is not of type XMLNode\n");
                    // Do nothing (not an XMLNode object)
                    return;
                }
                GASXmlNodeObject* pchild = static_cast<GASXmlNodeObject*>(p);
                if (!pchild->pRealNode)
                {
                    // Do nothing (malformed object)
                    return;
                }
                // Check to see if child is parent of this node
                // If so, it is a circular reference
                GFxXMLNode* ptopmost = pthis->pRealNode->Parent;
                while ( ptopmost && ptopmost->Parent != NULL)
                {
                    ptopmost = ptopmost->Parent;
                }
                if (ptopmost == pchild->pRealNode)
                {
                    // If child contains this node, then circular reference
                    // if appended.
                    if (log != NULL)
                        log->LogScriptWarning("Warning! XMLNode::appendChild"
                        " - trying to add a child that is the root of the current tree\n");
                    return;
                }
                // Keep a ref here else subtree will be nuked
                GPtr<GFxXMLNode> selfptr = pchild->pRealNode;
                // Check if child already has a parent. 
                if (pchild->pRealNode->Parent)
                {
                    // Flash removes the child from its tree and allows
                    // it to be appended to the new tree
                    pchild->pRealNode->Parent->RemoveChild(pchild->pRealNode);
                }
                elemNode->AppendChild(pchild->pRealNode);
                // Setup root node
                pchild->pRootNode = pthis->pRootNode;

                // Resolve floating prefix of child
                if (pchild->pRealNode->Type == GFxXMLElementNodeType)
                {
                    GFxXMLElementNode* elemChild = 
                        static_cast<GFxXMLElementNode*>(pchild->pRealNode);
                    // [PPS] Flash appendChild does not resolve namespace, if namespace 
                    // is already assigned to the node
                    if (elemChild->Namespace.GetSize() == 0)
                    {
                        ResolveNamespace(fn.Env, elemChild, pthis->pRootNode);
                    }
                }
            }
        }
        else
        {
            // Adding to text nodes is non-deterministic behavior
            if (log != NULL)
                log->LogScriptWarning("Warning: XMLNode::appendChild"
                    " - trying to add a child to a text node\n");
        }
    }
}


// 
// XMLNode.cloneNode(deep:Boolean) : XMLNode
//
void GASXmlNodeProto::CloneNode(const GASFnCall& fn)
{
    // Flash Doc: Constructs and returns a new XML node of the same type, 
    // name, value, and attributes as the specified XML object. If deep is 
    // set to true, all child nodes are recursively cloned, resulting in an 
    // exact copy of the original object's document tree. The clone of the 
    // node that is returned is no longer associated with the tree of the 
    // cloned item. Consequently, nextSibling, parentNode, and previousSibling 
    // all have a value of null. If the deep parameter is set to false, or 
    // the my_xml node has no child nodes, firstChild and lastChild are also 
    // null.
    //
    CHECK_THIS_PTR2(fn, XMLNode, XML);
    GASXmlNodeObject* pthis = (GASXmlNodeObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    if (pthis->pRealNode)
    {        
        bool deepCopy = false;
        if ( fn.NArgs > 0 )
            deepCopy = fn.Arg(0).ToBool(fn.Env);

        if (pthis->pRealNode->Type == GFxXMLElementNodeType)
        {
            // Set the real node
            GPtr<GFxXMLNode> realclone = *pthis->pRealNode->Clone(deepCopy);
            // Make a copy of the AS node
            GPtr<GASXmlNodeObject> clone = CreateShadow(fn.Env, realclone, NULL);
            // Set return value
            fn.Result->SetAsObject(clone);
        }
        else
        {
            // Text nodes and unknown nodes have no deep copy semantics
            GFxXMLTextNode* textNode = (GFxXMLTextNode*) pthis->pRealNode;
            // Set the real node
            GPtr<GFxXMLNode> realclone = *pthis->pRealNode->Clone(deepCopy);
            // Make a copy of the AS node
            GPtr<GASXmlNodeObject> clone = CreateShadow(fn.Env, realclone, NULL);
            // Set type since node may not be text node
            realclone->Type = textNode->Type;
            // Set return value
            fn.Result->SetAsObject(clone);
        }
    }
}


// 
// XMLNode.getNamespaceForPrefix(prefix:String) : String
//
void GASXmlNodeProto::GetNamespaceForPrefix(const GASFnCall& fn)
{
    // Flash Doc: Returns the namespace URI that is associated with the specified 
    // prefix for the node. To determine the URI, getPrefixForNamespace() searches up 
    // the XML hierarchy from the node, as necessary, and returns the namespace URI 
    // of the first xmlns declaration for the given prefix. If no namespace is defined 
    // for the specified prefix, the method returns null. If you specify an empty string 
    // ("") as the prefix and there is a default namespace defined for the node (as 
    // in xmlns="http://www.example.com/"), the method returns that default namespace 
    // URI. 

    CHECK_THIS_PTR2(fn, XMLNode, XML);
    GASXmlNodeObject* pthis = (GASXmlNodeObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    fn.Result->SetNull();

    GFxLog* log = fn.GetLog();
    if (pthis->pRealNode)
    {        
        if (pthis->pRealNode->Type == GFxXMLElementNodeType)
        {
            GFxXMLElementNode* elemNode = 
                (GFxXMLElementNode*) pthis->pRealNode;

            // Get query string
            if (fn.NArgs < 1)               
                return;

            GASString str = fn.Arg(0).ToString(fn.Env);         
            // Create the property query
            GASString pfxquery = fn.Env->GetSC()->CreateString("xmlns", 5);
            if (str.GetSize() > 0)
            {
                pfxquery += ":";
                pfxquery += str.ToCStr();
            }
            GASValue qval(GASValue::UNDEFINED);

            // -- Look for namespace declaration in self first
            GASSERT(elemNode);
            GASSERT(elemNode->pShadow);
            GFxXMLShadowRef* pshadow = elemNode->pShadow;
            GASSERT(pshadow);
            GASObject* pattribs = pshadow->pAttributes;
            GASSERT(pattribs);
            pattribs->GetMember(fn.Env, pfxquery, &qval);
            if (!qval.IsUndefined())
            {
                fn.Result->SetString(qval.ToString(fn.Env));
                return;
            }

            // -- Traverse up parent chain and look for a namespace
            GFxXMLElementNode* parent = 
                static_cast<GFxXMLElementNode*>(elemNode->Parent);
            while (parent)
            {
                // Create a shadow if one does not exist
                if (NULL == parent->pShadow)
                    CreateShadow(fn.Env, parent, pthis->pRootNode);
                pshadow = parent->pShadow;
                GASSERT(pshadow);
                pattribs = pshadow->pAttributes;
                GASSERT(pattribs);
                // lookup prefix in properties
                pattribs->GetMember(fn.Env, pfxquery, &qval);
                if (!qval.IsUndefined())
                {
                    fn.Result->SetString(qval.ToString(fn.Env));
                    return;
                }
                parent = static_cast<GFxXMLElementNode*>(parent->Parent);
            }
        }
        else
        {
            if (log != NULL)
                log->LogScriptWarning("Warning: XMLNodeProto::"
                    "GetNamespaceForPrefix - only element nodes support this method.\n");
        }
    }
}


// 
// Member visitor to traverse the attributes.
// This is required because the namespace declarations are in the
// attributes.
//
struct XMLPrefixQuerier : public GASObject::MemberVisitor
{
    GASEnvironment* pEnv;
    GASString &pKey;
    GASValue &pVal;
    XMLPrefixQuerier(GASEnvironment* penv, GASString& pkey, GASValue& pval) 
        : pEnv(penv), pKey(pkey), pVal(pval) {}
    XMLPrefixQuerier& operator=( const XMLPrefixQuerier& x )
    {
        pEnv = x.pEnv;
        pKey = x.pKey;
        pVal = x.pVal;
        return *this;
    }
    void Visit(const GASString& name, const GASValue& val, UByte flags)
    {
        GUNUSED(flags);
        GASString valstr = val.ToString(pEnv);
        if (valstr.Compare_CaseCheck(pKey, true) && 
            !strncmp(name.ToCStr(), "xmlns", 5))
        {
            pVal.SetString(name);
        }
    }
};

// 
// XMLNode.getPrefixForNamespace(nsURI:String) : String
//
void GASXmlNodeProto::GetPrefixForNamespace(const GASFnCall& fn) 
{
    // Flash Doc: Returns the prefix that is associated with the specified 
    // namespace URI for the node. To determine the prefix, 
    // getPrefixForNamespace() searches up the XML hierarchy from the node, 
    // as necessary, and returns the prefix of the first xmlns declaration 
    // with a namespace URI that matches nsURI. If there is no xmlns 
    // assignment for the given URI, the method returns null. If there is an 
    // xmlns assignment for the given URI but no prefix is associated with the 
    // assignment, the method returns an empty string (""). 

    CHECK_THIS_PTR2(fn, XMLNode, XML);
    GASXmlNodeObject* pthis = (GASXmlNodeObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    fn.Result->SetNull();

    GASStringContext* psc = fn.Env->GetSC();
    GFxLog* log = fn.GetLog();
    if (pthis->pRealNode)
    {        
        if (pthis->pRealNode->Type == GFxXMLElementNodeType)
        {
            GFxXMLElementNode* elemNode = 
                (GFxXMLElementNode*) pthis->pRealNode;

            // Get query string
            if (fn.NArgs < 1)               
                return;

            GASString str = fn.Arg(0).ToString(fn.Env);  
            GASValue qval(GASValue::UNDEFINED);
            XMLPrefixQuerier pq(fn.Env, str, qval);

            // -- Look for prefix declaration in self first
            GASSERT(elemNode);
            GASSERT(elemNode->pShadow);
            GFxXMLShadowRef* pshadow = elemNode->pShadow;
            GASSERT(pshadow);
            GASObject* pattribs = pshadow->pAttributes;
            GASSERT(pattribs);            
            pattribs->VisitMembers(psc, &pq, 0);
            if (!qval.IsUndefined())
            {
                fn.Result->SetString(qval.ToString(fn.Env));
            }
            // Get root node ref
            // -- Traverse up parent chain and look for a namespace
            GFxXMLElementNode* parent = 
                static_cast<GFxXMLElementNode*>(elemNode->Parent);
            while (fn.Result->IsNull() && parent)
            {
                // Create a shadow if one does not exist
                if (NULL == parent->pShadow)
                    CreateShadow(fn.Env, parent, pthis->pRootNode);
                pshadow = parent->pShadow;
                GASSERT(pshadow);
                pattribs = pshadow->pAttributes;
                GASSERT(pattribs);
                // lookup prefix in properties
                pattribs->VisitMembers(psc, &pq, 0);
                if (!qval.IsUndefined())
                {
                    fn.Result->SetString(qval.ToString(fn.Env));
                }
                parent = static_cast<GFxXMLElementNode*>(parent->Parent);
            }

            if (!fn.Result->IsNull())
            {
                // -- Remove the xmlns part
                GASString pfx = fn.Result->ToString(fn.Env);
                const char* data = pfx.ToCStr();
                const char* colon = strchr(data, ':');
                // Bounds checking is not needed because the string is at least 6 chars if
                // there is a colon and at least 5 chars without. this is guaranteed by the
                // member visitor constraint.
                if (colon)
                    fn.Result->SetString(psc->CreateString(data+6, pfx.GetSize()-6));
                else
                    fn.Result->SetString(psc->CreateString(data+5, pfx.GetSize()-5));
            }
        }
        else
        {
            if (log != NULL)
                log->LogScriptWarning("Warning: XMLNodeProto::"
                    "GetNamespaceForPrefix - only element nodes support this method.\n");
        }
    }
}


// 
// XMLNode.hasChildNodes() : Boolean
//
void GASXmlNodeProto::HasChildNodes(const GASFnCall& fn)
{
    CHECK_THIS_PTR2(fn, XMLNode, XML);
    GASXmlNodeObject* pthis = (GASXmlNodeObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    fn.Result->SetBool(false);

    if (pthis->pRealNode)
    {        
        if (pthis->pRealNode->Type == GFxXMLElementNodeType)
        {
            GFxXMLElementNode* elemNode = (GFxXMLElementNode*) pthis->pRealNode;
            fn.Result->SetBool(elemNode->HasChildren());
        }
    }
}


// 
// XMLNode.insertBefore(newChild:XMLNode, insertPoint:XMLNode) : Void
//
void GASXmlNodeProto::InsertBefore(const GASFnCall& fn)
{
    CHECK_THIS_PTR2(fn, XMLNode, XML);
    GASXmlNodeObject* pthis = (GASXmlNodeObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    // Flash Doc: Inserts a newChild node into the XML object's child list, 
    // before the insertPoint node. If insertPoint is not a child of the 
    // XMLNode object, the insertion fails

    if (pthis->pRealNode && pthis->pRealNode->Type == GFxXMLElementNodeType)
    {
        // Only works for element nodes
        GFxXMLElementNode* elemNode = (GFxXMLElementNode*) pthis->pRealNode;
        // Get new child and insertion point
        if (fn.NArgs > 1)
        {
            GASObject* child = fn.Arg(0).ToObject(fn.Env);
            GASObject* insert = fn.Arg(1).ToObject(fn.Env);
            if ( NULL == child || child->GetObjectType() != Object_XMLNode )
            {
                // Do nothing (not an XMLNode object)
                return;
            }
            // If insert is invalid, the child is appended. This is 
            // Flash behavior.
            GASXmlNodeObject* pchild = static_cast<GASXmlNodeObject*>(child);
            GASXmlNodeObject* pinsert = static_cast<GASXmlNodeObject*>(insert);
            if ( pinsert && pinsert->GetObjectType() == Object_XMLNode &&
                pinsert->pRealNode && pinsert->pRealNode->Parent &&
                pinsert->pRealNode->Parent == elemNode)
            {
                // Insertion node is a child of this node
                if (pchild->pRealNode)
                {
                    // Keep a ref here else subtree will be nuked
                    GPtr<GFxXMLNode> selfptr = pchild->pRealNode;
                    if (pchild->pRealNode->Parent)
                    {
                        // Flash removes the child from its tree and allows
                        // it to be appended to the new tree
                        pchild->pRealNode->Parent->RemoveChild(pchild->pRealNode);
                    }
                    elemNode->InsertBefore(pchild->pRealNode, pinsert->pRealNode);
                    pchild->pRootNode = pthis->pRootNode;
                }
            }
            else if (pchild->pRealNode)
            {
                // Keep a ref here else subtree will be nuked
                GPtr<GFxXMLNode> selfptr = pchild->pRealNode;
                // Simply append
                if (pchild->pRealNode->Parent)
                {
                    // Flash removes the child from its tree and allows
                    // it to be appended to the new tree
                    pchild->pRealNode->Parent->RemoveChild(pchild->pRealNode);
                }
                elemNode->AppendChild(pchild->pRealNode);
                pchild->pRootNode = pthis->pRootNode;
            }
        }
    }
}


// 
// XMLNode.removeNode() : Void
//
void GASXmlNodeProto::RemoveNode(const GASFnCall& fn)
{
    CHECK_THIS_PTR2(fn, XMLNode, XML);
    GASXmlNodeObject* pthis = (GASXmlNodeObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    // Flash Doc: Removes the specified XML object from its parent. Also 
    // deletes all descendants of the node.

    if (pthis->pRealNode)
    {        
        GFxXMLElementNode* parent = pthis->pRealNode->Parent;
        if (parent)
        {
            pthis->pRootNode = *parent->MemoryManager->CreateRootNode(pthis->pRealNode);
            parent->RemoveChild(pthis->pRealNode);
        }
    }
}



// 
// Member visitor to traverse the attributes
//
struct XMLAttributeStringBuilder : public GASObject::MemberVisitor
{
    GASEnvironment* pEnv;
    GStringBuffer& Dest;
    XMLAttributeStringBuilder(GASEnvironment* penv, GStringBuffer& dest) 
        : pEnv(penv), Dest(dest) {}
    XMLAttributeStringBuilder& operator=( const XMLAttributeStringBuilder& x )
    {
        pEnv = x.pEnv;
        Dest = x.Dest;
        return *this;
    }
    void Visit(const GASString& name, const GASValue& val, UByte flags)
    {
        GUNUSED(flags);
        Dest += " ";
        Dest += name.ToCStr();
        Dest += "=\"";
        Dest += val.ToString(pEnv).ToCStr();
        Dest += "\"";        
    }
};


// 
// Create an XML string by traversing the DOM tree 
//
static void BuildXMLString(GASEnvironment* penv, GFxXMLNode* proot, GStringBuffer& dest)
{
    switch (proot->Type)
    {
    case GFxXMLElementNodeType: 
        {
            GFxXMLElementNode* pnode = static_cast<GFxXMLElementNode*>(proot);
            GFxXMLShadowRef* pshadow = proot->pShadow;

            // If document node, do not print it out - just visit children
            if (pshadow && pshadow->pASNode)
            {
                if (pshadow->pASNode->GetObjectType() == GASObject::Object_XML)
                {
                    GASXmlNodeObject* pobj = pshadow->pASNode;
                    // Print xmldecl
                    GASValue xmldecl;
                    pobj->GetMember(penv, 
                        penv->GetSC()->CreateConstString("xmlDecl"), 
                        &xmldecl);
                    if (!xmldecl.IsUndefined())
                    {
                        dest += xmldecl.ToString(penv).ToCStr();
                        GASValue ignorews;
                        pobj->GetMember(penv, 
                            penv->GetSC()->CreateConstString("ignoreWhite"), 
                            &ignorews);
                        if (!ignorews.ToBool(penv))
                        {
                            dest += "\n";
                        }
                    }
                    // Print children
                    for (GFxXMLNode* child = pnode->FirstChild; 
                            child != NULL; child = child->NextSibling)
                        BuildXMLString(penv, child, dest);
                    break;
                }
            }

            dest += "<";

            if (pnode->Prefix.GetSize() > 0)
            {
                dest += pnode->Prefix.ToCStr();
                dest += ":";
            }
            dest += proot->Value.ToCStr();
            
            if (pshadow && pshadow->pAttributes)
            {
                // Shadow is present, get the attributes from the shadow
                XMLAttributeStringBuilder attrvis(penv, dest);
                GASSERT(pshadow->pAttributes);
                GASObject* attrs = pshadow->pAttributes;
                if (attrs)
                    attrs->VisitMembers(penv->GetSC(), &attrvis, 0);
            }
            else
            {
                // Shadow is not present, get attributes from real node
                for (GFxXMLAttribute* attr = pnode->FirstAttribute; 
                        attr != NULL; attr = attr->Next)
                {
                    dest += " ";
                    dest += attr->Name.ToCStr();
                    dest += "=\"";
                    dest += attr->Value.ToCStr();
                    dest += "\"";                    
                }
            }

            if (pnode->HasChildren())
                dest += ">";
            else
                dest += " />";

            for (GFxXMLNode* child = pnode->FirstChild; child != NULL; 
                    child = child->NextSibling)
                BuildXMLString(penv, child, dest);
            if (pnode->HasChildren())
            {
                dest += "</";
                if (pnode->Prefix.GetSize() > 0)
                {
                    dest += pnode->Prefix.ToCStr();
                    dest += ":";
                }
                dest += proot->Value.ToCStr();
                dest += ">";
            }
            break;
        }
    default:    // Text nodes and unknowns
        {
            dest += proot->Value.ToCStr();
        }
    }
}


// 
// XMLNode.toString() : String
//
void GASXmlNodeProto::ToString(const GASFnCall& fn)
{
    CHECK_THIS_PTR2(fn, XMLNode, XML);
    GASXmlNodeObject* pthis = (GASXmlNodeObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    GStringBuffer str;
    if (pthis->pRealNode)
    {        
        if (pthis->pRealNode->Type == GFxXMLElementNodeType)
        {
            GFxXMLElementNode* elemNode = 
                (GFxXMLElementNode*) pthis->pRealNode;
            BuildXMLString(fn.Env, elemNode, str);
        }
        else // Text nodes and unknown types
        {
            str += pthis->pRealNode->Value.ToCStr();
        }
        fn.Result->SetString(fn.Env->CreateString(str.ToCStr(),str.GetSize()));
    }
    else
    {
        // Flash has inconsistent behavior when a node is created
        // with a type but no value. This malformed node returns
        // '[type Object] : undefined' when toString() is called on it.
        // GFx simply returns undefined, since this case should not happen
        // in production code.
        fn.Result->SetUndefined();
    }
}



// 
// Constructor func ctor
//
GASXmlNodeCtorFunction::GASXmlNodeCtorFunction(GASStringContext *psc) 
    : GASCFunctionObject(psc, GlobalCtor)
{
    GUNUSED(psc);
}

// 
// Called when the constructor is invoked for the XMLNode class 
// (new XMLNode(...))
//
void GASXmlNodeCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASXmlNodeObject> pnode;
    // Need to check if object is of type XML as well
    if (fn.ThisPtr && 
        (fn.ThisPtr->GetObjectType() == GASObject::Object_XMLNode || 
         fn.ThisPtr->GetObjectType() == GASObject::Object_XML))
        pnode = static_cast<GASXmlNodeObject*>(fn.ThisPtr);
    else
        pnode = *GHEAP_NEW(fn.Env->GetHeap()) GASXmlNodeObject(fn.Env);

    GFxLog* log = fn.GetLog();

    // Handle the constructor parameters
    GASValue nodeType = GASValue::UNDEFINED;
    GASValue nodeValue = GASValue::UNDEFINED;
    if (fn.NArgs > 0)
    {
        nodeType = fn.Arg(0);
        if (fn.NArgs > 1)
            nodeValue = fn.Arg(1);
    }

    // Since the XMLNode is created from scratch, a real node must be 
    // explictly created
    if ( !nodeType.IsUndefined() )
    {
        GASNumber nt = nodeType.ToNumber(fn.Env);
        // If node type is defined, a node value is required. if not,
        // default to GASObject
        if (!nodeValue.IsUndefined())
        {
            // A shadow should not already be assigned to this node.
            GASSERT(pnode->pRealNode == NULL);

            // Get the global (to movie root) object manager
            GFxMovieRoot* pmovieroot = fn.Env->GetMovieRoot();
            GPtr<GFxXMLObjectManager> memMgr;
            if (!pmovieroot->pXMLObjectManager)
            {
                memMgr = *GHEAP_NEW(fn.Env->GetHeap()) GFxXMLObjectManager(pmovieroot);
                pmovieroot->pXMLObjectManager = memMgr;
            }
            else
                memMgr = (GFxXMLObjectManager*)pmovieroot->pXMLObjectManager;

            GFxXMLDOMString localname(memMgr->EmptyString());
            GFxXMLDOMString prefix(memMgr->EmptyString());

            GASString s = nodeValue.ToString(fn.Env);

            if (nt == GFxXMLElementNodeType)
            {            
                // Name may have a prefix
                const char* pfx = strchr(s.ToCStr(), ':');
                if (pfx)
                {
                    // Create a prefix mapping local to this node.
                    // this will have to be resolved if node is attached
                    // to a tree.
                    prefix = memMgr->CreateString(s.ToCStr(), pfx-s.ToCStr());
                    localname = memMgr->CreateString(pfx+1, G_strlen(pfx));
                }
                else
                    localname = memMgr->CreateString(s.ToCStr(), s.GetSize());

                GPtr<GFxXMLElementNode> en = *memMgr->CreateElementNode(localname);
                pnode->pRealNode = en;
                pnode->pRootNode = *memMgr->CreateRootNode(pnode->pRealNode);
                GFxXMLElementNode* elemNode = 
                    static_cast<GFxXMLElementNode*>(pnode->pRealNode);
                elemNode->Prefix = prefix;
            }
            else if (nt == GFxXMLTextNodeType)
            {
                localname = memMgr->CreateString(s.ToCStr(), s.GetSize());
                GPtr<GFxXMLTextNode> tn = *memMgr->CreateTextNode(localname);
                pnode->pRealNode = tn;
                pnode->pRootNode = *memMgr->CreateRootNode(pnode->pRealNode);
            }
            else
            {
                // If node type is neither 1 or 3, then the values 
                // should not matter. But AS 2.0 converts them into text nodes..
                // Flash Docs: "In Flash Player, the XML class only supports 
                // node types 1 (ELEMENT_NODE) and 3 (TEXT_NODE)." 
                GPtr<GFxXMLTextNode> tn = *memMgr->CreateTextNode(localname);
                pnode->pRealNode = tn;
                pnode->pRootNode = *memMgr->CreateRootNode(pnode->pRealNode);
                pnode->pRealNode->Type = static_cast<UByte>(nt);
            }
            SetupShadow(fn.Env, pnode->pRealNode, pnode);
        }
        else
        {
            if (log != NULL)
                log->LogScriptWarning("Warning: XMLNodeCtorFunction::"
                    "GlobalCtor - malformed XMLNode object\n");            
        }
    }
    else
    {
        if (log != NULL)
            log->LogScriptWarning("Warning: XMLNodeCtorFunction::"
                "GlobalCtor - node type not specified\n");
    }

    fn.Result->SetAsObject(pnode.GetPtr());
}

GASObject* GASXmlNodeCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{
    return GHEAP_NEW(penv->GetHeap()) GASXmlNodeObject(penv);
}


GASFunctionRef GASXmlNodeCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef ctor(*GHEAP_NEW(pgc->GetHeap()) GASXmlNodeCtorFunction(&sc));
    GPtr<GASXmlNodeProto> proto = 
        *GHEAP_NEW(pgc->GetHeap()) GASXmlNodeProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_XMLNode, proto);
    pgc->pGlobal->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_XMLNode), GASValue(ctor));

    // Preload the XML and XMLNode member names as GASStrings and 
    // store them locally for fast access
    GASXmlNodeObject::InitializeStandardMembers(*pgc, proto->XMLNodeMemberMap);

    return ctor;
}

#endif // #ifndef GFC_NO_XML_SUPPORT
