/**********************************************************************

Filename    :   AS/GASBitmapData.h
Content     :   Implementation of BitmapData class
Created     :   March, 2007
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXBITMAPDATA_H
#define INC_GFXBITMAPDATA_H

#include "GConfig.h"
#ifndef GFC_NO_FXPLAYER_AS_BITMAPDATA
#include "GFxAction.h"
#include "GFxCharacter.h"
#include "AS/GASObjectProto.h"


// ***** Declared Classes
class GASBitmapData;
class GASBitmapDataProto;
class GASBitmapDataCtorFunction;

// ***** External Classes
class GASArrayObject;
class GASEnvironment;



class GASBitmapData : public GASObject
{
    GPtr<GFxImageResource> pImageRes;
    GPtr<GFxMovieDef>      pMovieDef; // holder for MovieDef to avoid "_Images" heap release

    void commonInit (GASEnvironment* penv);
#ifndef GFC_NO_GC
protected:
    virtual void Finalize_GC()
    {
        pImageRes = NULL;
        pMovieDef = NULL;
        GASObject::Finalize_GC();
    }
#endif //GFC_NO_GC
protected:
    GASBitmapData(GASStringContext* psc, GASObject* pprototype) 
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }
public:
    GASBitmapData(GASEnvironment* penv);
    ~GASBitmapData() {}

    ObjectType      GetObjectType() const   { return Object_BitmapData; }

    void              SetImage(GASEnvironment* penv, GFxImageResource* pimg, GFxMovieDef* pmovieDef);
    GFxImageResource* GetImage() const                 { return pImageRes; }

    bool            SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, 
                              const GASPropFlags& flags = GASPropFlags());
    bool            GetMember(GASEnvironment* penv, const GASString& name, GASValue* val);

    static GASBitmapData* LoadBitmap(GASEnvironment* penv, const GString& linkageId);
};

class GASBitmapDataProto : public GASPrototype<GASBitmapData>
{
public:
    GASBitmapDataProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);

};

class GASBitmapDataCtorFunction : public GASCFunctionObject
{
    static const GASNameFunction StaticFunctionTable[];
public:
    GASBitmapDataCtorFunction (GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);
    static void LoadBitmap(const GASFnCall& fn);
    virtual GASObject* CreateNewObject(GASEnvironment*) const { return 0; }

    static GASFunctionRef Register(GASGlobalContext* pgc);
};

#endif //#ifndef GFC_NO_FXPLAYER_AS_BITMAPDATA

#endif // INC_GFXBITMAPDATA_H
