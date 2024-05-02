/**********************************************************************

Filename    :   GFxASXmlNode.h
Content     :   XMLNode reference class for ActionScript 2.0
Created     :   11/30/2007
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#ifndef INC_GFXASXMLNODE_H
#define INC_GFXASXMLNODE_H

#include "GConfig.h"

#ifndef GFC_NO_XML_SUPPORT

#include "AS/GASObject.h"
#include "AS/GASObjectProto.h"

#include <GFxXMLDocument.h>
#include "XML/GFxXMLShadowRef.h"


// ***** Declared Classes
class GASXmlNodeObject;
class GASXmlNodeProto;
class GASXmlNodeCtorFunction;


// Forward decl
void GFx_Xml_CreateIDMap(GASEnvironment* penv, GFxXMLElementNode* elemNode, GFxASXMLRootNode* proot, GASObject* pobj);

// 
// Actionscript XMLNode object declaration
//

class GASXmlNodeObject : public GASObject
{
    friend class GASXmlNodeProto;
    friend class GASXmlNodeCtorFunction;

protected:

#ifndef GFC_NO_GC
    void Finalize_GC()
    {
        // Remove itself as the realnode's (C++) shadow reference
        if (pRealNode && pRealNode->pShadow)
        {
            GFxXMLShadowRef* pshadow = pRealNode->pShadow;
            pshadow->pASNode = NULL;
        }
        pRootNode = NULL;
        GASObject::Finalize_GC();
    }
#endif // GFC_NO_GC

    GASXmlNodeObject(GASStringContext *psc, GASObject* pprototype) 
        : GASObject(psc), pRealNode(NULL)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }

    virtual             ~GASXmlNodeObject();

public:

    GPtr<GFxASXMLRootNode>  pRootNode;
    GFxXMLNode*             pRealNode;

    GASXmlNodeObject(GASEnvironment* penv);

    virtual ObjectType  GetObjectType() const { return Object_XMLNode; }

    virtual bool        SetMember(GASEnvironment* penv, 
                            const GASString& name, const GASValue& val, 
                            const GASPropFlags& flags = GASPropFlags());
    virtual bool        GetMember(GASEnvironment* penv, 
                            const GASString& name, GASValue* val);

    //
    // Enumeration of XMLNode properties for fast comparison
    // in GetMember/SetMember
    //
    enum StandardMember
    {
        M_InvalidMember = -1,
        M_attributes    = 0,
        M_childNodes,
        M_firstChild,
        M_lastChild,
        M_localName,
        M_namespaceURI,
        M_nextSibling,
        M_nodeName,
        M_nodeType,
        M_nodeValue,
        M_parentNode,
        M_prefix,
        M_previousSibling,

        M_XMLNodeMemberCount
    };

    // Looks up a standard member and returns M_InvalidMember if it is not found.
    StandardMember      GetStandardMemberConstant(GASEnvironment* penv, 
                                                  const GASString& memberName) const;

    // Initialize the XMLNode standard member hash
    static void         InitializeStandardMembers(GASGlobalContext& gc, 
                                                  GASStringHash<SByte>& hash);
};


class GASXmlNodeProto : public GASPrototype<GASXmlNodeObject>
{
#ifndef GFC_NO_GC
protected:
    void Finalize_GC()
    {
        XMLNodeMemberMap.~GASStringHash<SByte>();
        GASPrototype<GASXmlNodeObject>::Finalize_GC();
    }
#endif // GFC_NO_GC
public:
    // A map of XMLNode member names for efficient access. The values 
    //in it are of GFxASCharacter::StandardMember type.
    GASStringHash<SByte>        XMLNodeMemberMap;

    GASXmlNodeProto(GASStringContext *psc, GASObject* prototype, 
                    const GASFunctionRef& constructor);

    static const GASNameFunction FunctionTable[];

    //
    // Default XMLNode object functions
    //
    static void         AppendChild(const GASFnCall& fn);
    static void         CloneNode(const GASFnCall& fn);
    static void         GetNamespaceForPrefix(const GASFnCall& fn);
    static void         GetPrefixForNamespace(const GASFnCall& fn);
    static void         HasChildNodes(const GASFnCall& fn);
    static void         InsertBefore(const GASFnCall& fn);
    static void         RemoveNode(const GASFnCall& fn);
    static void         ToString(const GASFnCall& fn);
};


class GASXmlNodeCtorFunction : public GASCFunctionObject
{
public:
    GASXmlNodeCtorFunction(GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);
    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;

    static GASFunctionRef Register(GASGlobalContext* pgc);
};

#endif  // #ifndef GFC_NO_XML_SUPPORT

#endif  // INC_GFXASXMLNODE_H
