/**********************************************************************

Filename    :   AS/GASCapabilities.h
Content     :   System.capabilities reference class for ActionScript 2.0
Created     :   3/27/2007
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXCAPABILITIES_H
#define INC_GFXCAPABILITIES_H

#include "GConfig.h"
#ifndef GFC_NO_FXPLAYER_AS_CAPABILITES
#include "AS/GASObject.h"

// ****************************************************************************
// GAS Capabilities Constructor function class
//
class GASCapabilitiesCtorFunction : public GASCFunctionObject
{
public:
    GASCapabilitiesCtorFunction (GASStringContext *psc);

    virtual bool SetMemberRaw(GASStringContext *psc, const GASString& name, 
        const GASValue& val, const GASPropFlags& flags = GASPropFlags())
    {
        // nothing to set
        GUNUSED4(psc,name,val,flags);
        return false;
    }
    
    virtual bool GetMember(GASEnvironment *penv, const GASString& name, 
        GASValue* val);

    virtual GASObject* CreateNewObject(GASEnvironment*) const { return 0; }

    static GASFunctionRef Register(GASGlobalContext* pgc);
};
#endif //#ifndef GFC_NO_FXPLAYER_AS_CAPABILITES
#endif  // INC_GFXCAPABILITIES_H
