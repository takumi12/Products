/**********************************************************************

Filename    :   AS/GASAmpMarker.h
Content     :   Implementation of marker class for AMP
Created     :   May, 2010
Authors     :   Alex Mantzaris

Notes       :   
History     :   

Copyright   :   (c) 1998-2010 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GAS_AMP_MARKER_H
#define INC_GAS_AMP_MARKER_H

#include "GConfig.h"
#include "GASObject.h"
#include "AS/GASObjectProto.h"

// ***** External Classes
class GASArrayObject;
class GASEnvironment;

class GASAmpMarker : public GASObject
{
protected:
    GASAmpMarker(GASStringContext *sc, GASObject* prototype)
        : GASObject(sc)
    { 
        Set__proto__(sc, prototype); // this ctor is used for prototype obj only
    }

    void commonInit(GASEnvironment* env);

public:
    GASAmpMarker(GASEnvironment* env);
};

class GASAmpMarkerProto : public GASPrototype<GASAmpMarker>
{
public:
    GASAmpMarkerProto(GASStringContext* sc, GASObject* prototype, const GASFunctionRef& constructor);

};

//
// AMP Marker static class
//
// A constructor function object for Object
class GASAmpMarkerCtorFunction : public GASCFunctionObject
{
    static const GASNameFunction StaticFunctionTable[];

    static void AddMarker(const GASFnCall& fn);

public:
    GASAmpMarkerCtorFunction(GASStringContext *sc);

    static void GlobalCtor(const GASFnCall& fn);
    virtual GASObject* CreateNewObject(GASEnvironment*) const { return 0; }

    virtual bool GetMember(GASEnvironment* penv, const GASString& name, GASValue* pval);
    virtual bool SetMember(GASEnvironment *penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags());

    static GASFunctionRef Register(GASGlobalContext* pgc);
};

#endif // INC_GAS_AMP_MARKER_H
