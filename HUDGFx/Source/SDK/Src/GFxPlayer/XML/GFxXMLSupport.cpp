/**********************************************************************

Filename    :   GFxXMLParser.cpp
Content     :   SAX2 compliant interface
Created     :   March 7, 2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GConfig.h"

#ifndef GFC_NO_XML_SUPPORT

#include <GFxXMLSupport.h>
#include "GFxAction.h"
#include "XML/GFxASXmlNode.h"
#include "XML/GFxASXml.h"


//
// Load file given the provided file opener and DOM builder.
//
bool    GFxXMLSupport::ParseFile(const char* pfilename, 
                                 GFxFileOpenerBase* pfo, 
                                 GFxXMLParserHandler* pdb)
{
    GASSERT(pParser.GetPtr());
    return pParser->ParseFile(pfilename, pfo, pdb);
}

//
// Load a string using the provided DOM builder
//
bool    GFxXMLSupport::ParseString(const char* pdata, 
                                   UPInt len, 
                                   GFxXMLParserHandler* pdb)
{
    GASSERT(pParser.GetPtr());
    return pParser->ParseString(pdata, len, pdb);
}

//
// Register the XML and XMLNode actionscript classes
//
void    GFxXMLSupport::RegisterASClasses(GASGlobalContext& gc, 
                                         GASStringContext& sc)
{
    gc.AddBuiltinClassRegistry<GASBuiltin_XML, GASXmlCtorFunction>(sc, gc.pGlobal);
    gc.AddBuiltinClassRegistry<GASBuiltin_XMLNode, GASXmlNodeCtorFunction>(sc, gc.pGlobal);
}


#endif  // GFC_NO_XML_SUPPORT
