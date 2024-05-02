/**********************************************************************

Filename    :   AS/GASStyleSheet.h
Content     :   StyleSheet (Textfield.StyleSheet) object functionality
Created     :   May 20, 2008
Authors     :   Prasad Silva

Notes       :   
History     :   

Copyright   :   (c) 1998-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GASStyleSheet_H
#define INC_GASStyleSheet_H

#include "GFxAction.h"
#include "Text/GFxTextCore.h"
#include "Text/GFxStyleSheet.h"
#include "AS/GASObjectProto.h"

#ifndef GFC_NO_CSS_SUPPORT

// ***** Declared Classes
class GASStyleSheetObject;
class GASStyleSheetProto;
class GASStyleSheetCtorFunction;


//
// ActionScript CSS file loader interface. 
//
// This interface is used by the GFx core to load a CSS file to an 
// ActionScript StyleSheet object. An instance of this object is 
// provided by the CSS library and passed through the movie load 
// queues. The two methods are called at the appropriate moments from 
// within GFx. Its implementation is defined in the CSS library. This 
// interface only serves as a temporary wrapper for the duration of 
// passing through the GFx core.
//
class GFxASCSSFileLoader : public GRefCountBase<GFxASCSSFileLoader, GStat_Default_Mem>
{
public:
    GFxASCSSFileLoader() { }
    virtual         ~GFxASCSSFileLoader() {}

    //
    // Load a CSS file using the file opener
    //
    virtual void    Load(const GString& filename, GFxFileOpener* pfo) = 0;

    //
    // Initialize the ActionScript StyleSheet object with the loaded data
    //
    virtual void    Init(GASEnvironment* penv, GASObject* pTarget) = 0;
};



class GASStyleSheetObject : public GASObject
{
protected:
    GASStyleSheetObject(GASStringContext *psc, GASObject* pprototype)
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }

#ifndef GFC_NO_GC
protected:
    virtual void Finalize_GC()
    {
        CSS.~GFxTextStyleManager();
        GASObject::Finalize_GC();
    }
#endif //GFC_NO_GC
public:
    GFxTextStyleManager   CSS;

    GASStyleSheetObject(GASEnvironment* penv);

    ObjectType      GetObjectType() const   { return Object_StyleSheet; }

    void            NotifyOnLoad(GASEnvironment* penv, bool success);

    GFxTextStyleManager* GetTextStyleManager() { return &CSS; }
    const GFxTextStyleManager* GetTextStyleManager() const { return &CSS; }
};

class GASStyleSheetProto : public GASPrototype<GASStyleSheetObject>
{
public:
    GASStyleSheetProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);

    static void Clear(const GASFnCall& fn);
    static void GetStyle(const GASFnCall& fn);
    static void GetStyleNames(const GASFnCall& fn);
    static void Load(const GASFnCall& fn);
    static void ParseCSS(const GASFnCall& fn);
    static void SetStyle(const GASFnCall& fn);
    static void Transform(const GASFnCall& fn);
};

class GASStyleSheetCtorFunction : public GASCFunctionObject
{
public:
    GASStyleSheetCtorFunction(GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);
    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;

    static GASFunctionRef Register(GASGlobalContext* pgc);
};
#endif // GFC_NO_CSS_SUPPORT

#endif // INC_GASStyleSheet_H

