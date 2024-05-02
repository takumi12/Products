/**********************************************************************

Filename    :   AS/GASTextFormat.h
Content     :   TextFormat object functinality
Created     :   April 17, 2007
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXTEXTFORMAT_H
#define INC_GFXTEXTFORMAT_H

#include "GConfig.h"
#ifndef GFC_NO_FXPLAYER_AS_TEXTFORMAT
#include "GFxAction.h"
#include "Text/GFxTextCore.h"
#include "AS/GASObjectProto.h"


// ***** Declared Classes
class GASTextFormatObject;
class GASTextFormatProto;
class GASTextFormatCtorFunction;


class GASTextFormatObject : public GASObject
{
protected:
    GASTextFormatObject(GASStringContext *psc, GASObject* pprototype)
        : GASObject(psc), TextFormat(psc->GetHeap())
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }
#ifndef GFC_NO_GC
protected:
    virtual void Finalize_GC()
    {
        TextFormat.~GFxTextFormat();
        ParagraphFormat.~GFxTextParagraphFormat();
        GASObject::Finalize_GC();
    }
#endif //GFC_NO_GC
public:
    GFxTextFormat           TextFormat;
    GFxTextParagraphFormat  ParagraphFormat;

    GASTextFormatObject(GASStringContext* psc) 
        : GASObject(psc), TextFormat(psc->GetHeap()) { GUNUSED(psc); }
    GASTextFormatObject(GASEnvironment* penv);

    ObjectType      GetObjectType() const   { return Object_TextFormat; }

    virtual bool    SetMember(GASEnvironment *penv, const GASString& name, 
        const GASValue& val, const GASPropFlags& flags = GASPropFlags());

    void SetTextFormat(GASStringContext* psc, const GFxTextFormat& textFmt);
    void SetParagraphFormat(GASStringContext* psc, const GFxTextParagraphFormat& paraFmt);
};

class GASTextFormatProto : public GASPrototype<GASTextFormatObject>
{
public:
    GASTextFormatProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);

    static void GetTextExtent(const GASFnCall& fn);
};

class GASTextFormatCtorFunction : public GASCFunctionObject
{
public:
    GASTextFormatCtorFunction(GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);
    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;

    static GASFunctionRef Register(GASGlobalContext* pgc);
};
#endif //#ifndef GFC_NO_FXPLAYER_AS_TEXTFORMAT

#endif //TEXTFORMAT

