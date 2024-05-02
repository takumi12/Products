/**********************************************************************

Filename    :   GFxASXml.h
Content     :   XML reference class for ActionScript 2.0
Created     :   11/30/2007
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#ifndef INC_GFxASXML_H
#define INC_GFxASXML_H

#include "GConfig.h"

#ifndef GFC_NO_XML_SUPPORT

#include "GFile.h"
#include "AS/GASObject.h"
#include "AS/GASObjectProto.h"
#include "XML/GFxASXmlNode.h" 
#include "AS/GASAsBroadcaster.h"


//
// Actionscrip XML object declaration
//

// ***** Declared Classes
class GASXmlObject;
class GASXmlProto;
class GASXmlCtorFunction;


class GASXmlObject : public GASXmlNodeObject
{
    friend class GASXmlProto;
    friend class GASXmlCtorFunction;

    GASNumber       BytesLoadedCurrent;
    GASNumber       BytesLoadedTotal;

protected:
    GASXmlObject(GASStringContext *psc, GASObject* pprototype) : 
         GASXmlNodeObject(psc, pprototype)
    { }
public:
    GASXmlObject(GASEnvironment* penv);

    virtual ObjectType GetObjectType() const { return Object_XML; }

    //
    // XML object callbacks
    //
    void            NotifyOnData(GASEnvironment* penv, GASValue val);
    void            NotifyOnHTTPStatus(GASEnvironment* penv, 
                        GASNumber httpStatus);
    void            NotifyOnLoad(GASEnvironment* penv, bool success);

    void            SetLoadedBytes(GASNumber total, GASNumber loaded);

    void            AssignXMLDecl(GASEnvironment* penv, 
                        GFxXMLDocument* pdoc);
};


class GASXmlProto : public GASPrototype<GASXmlObject>
{
public:
    GASXmlProto(GASStringContext *psc, GASObject* prototype, 
                const GASFunctionRef& constructor);

    static const GASNameFunction FunctionTable[];

    //
    // Default XML object functions
    //
    static void     AddRequestHeader(const GASFnCall& fn);
    static void     CreateElement(const GASFnCall& fn);
    static void     CreateTextNode(const GASFnCall& fn);
    static void     GetBytesLoaded(const GASFnCall& fn);
    static void     GetBytesTotal(const GASFnCall& fn);
    static void     Load(const GASFnCall& fn);
    static void     ParseXML(const GASFnCall& fn);
    static void     Send(const GASFnCall& fn);
    static void     SendAndLoad(const GASFnCall& fn);

    static void     DefaultOnData(const GASFnCall& fn);
};


class GASXmlCtorFunction : public GASCFunctionObject
{
public:
    GASXmlCtorFunction(GASStringContext *psc);

    static void             GlobalCtor(const GASFnCall& fn);
    virtual GASObject*      CreateNewObject(GASEnvironment* penv) const;

    static GASFunctionRef   Register(GASGlobalContext* pgc);
};


#endif  // ##ifndef GFC_NO_XML_SUPPORT

#endif  // INC_GFxASXML_H
