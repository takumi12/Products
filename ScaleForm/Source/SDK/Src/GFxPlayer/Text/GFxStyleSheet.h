/**********************************************************************

Filename    :   Text/GFxStyleSheet.h
Content     :   StyleSheet parser
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

#ifndef INC_GFxStyleSheet_H
#define INC_GFxStyleSheet_H

#include "Text/GFxTextCore.h"

#ifndef GFC_NO_CSS_SUPPORT

// 
// Forward declaration
//
class GFxTextStyleParserHandlerBase;

//
// Hash key for CSS style objects
//
struct GFxTextStyleKey
{
    enum KeyType
    {
        CSS_Tag = 0,
        CSS_Class = 1,
        CSS_None = 2 // Invailid initialization.
    };

    KeyType      Type;
    GStringLH    Value;
    UPInt        HashValue;

    GFxTextStyleKey() : Type(CSS_None) { }

    GFxTextStyleKey(KeyType t, const GString& val)
        : Type(t), Value(val) 
    {
        // case sensitive 
        HashValue = GString::BernsteinHashFunction(val.ToCStr(), val.GetSize());
        HashValue += Type;
    }

    void    Set(KeyType t, const GString& val)
    {
        Type  = t;
        Value = val;
        HashValue = GString::BernsteinHashFunction(val.ToCStr(), val.GetSize());
        HashValue += Type;
    }

};

bool operator== (const GFxTextStyleKey& key1, const GFxTextStyleKey& key2);


//
// CSS style object
//
struct GFxTextStyle : public GNewOverrideBase<GFxStatMV_Text_Mem>
{
    GFxTextFormat           TextFormat;
    GFxTextParagraphFormat  ParagraphFormat;

    GFxTextStyle(GMemoryHeap *pheap) : TextFormat(pheap) { }

    void                    Reset();
};


//
// Hash functions
//
template<class C>
class GFxTextStyleHashFunc
{
public:
    typedef C ValueType;

    // Hash code is stored right in the key
//     UPInt  operator() (const C& data) const
//     {
//         return data->HashValue;
//     }

    UPInt   operator() (const C &str) const
    {
        return str.HashValue;
    }

    // Hash update - unused.
    static UPInt    GetCachedHash(const C& data)                { return data->HashValue; }
    static void     SetCachedHash(C& data, UPInt hashValue)     { GUNUSED2(data, hashValue); }
    // Value access.
    static C&       GetValue(C& data)                           { return data; }
    static const C& GetValue(const C& data)                     { return data; }

};


//
// Text style hash set 
//
// Keeps track of all text styles currently existing in the manager.
//
typedef GHashUncachedLH<GFxTextStyleKey, GFxTextStyle*,
    GFxTextStyleHashFunc<GFxTextStyleKey>, GFxStatMV_Text_Mem> GFxTextStyleHash;


//
// CSS style sheet object
//
// Manages a set of text stylse
//
class GFxTextStyleManager //: public GRefCountBase<GFxTextStyleManager>
{
    friend class GFxTextStyleParserHandlerBase;

public:
    GFxTextStyleManager() {}
    ~GFxTextStyleManager();

    bool ParseCSS(const char* buffer, UPInt len);
    bool ParseCSS(const wchar_t* buffer, UPInt len);

    const GFxTextStyleHash& GetStyles() const;
    const GFxTextStyle*     GetStyle(GFxTextStyleKey::KeyType type, const GString& name) const;
    const GFxTextStyle*     GetStyle(GFxTextStyleKey::KeyType type, const char* name, UPInt len = GFC_MAX_UPINT) const;
    const GFxTextStyle*     GetStyle(GFxTextStyleKey::KeyType type, const wchar_t* name, UPInt len = GFC_MAX_UPINT) const;

    void                    AddStyle(const GFxTextStyleKey& key, GFxTextStyle* pstyle);

    void                    ClearStyles();
    void                    ClearStyle(GFxTextStyleKey::KeyType type, const GString& name);
    void                    ClearStyle(GFxTextStyleKey::KeyType type, const char* name, UPInt len = GFC_MAX_UPINT);

    // Obtains a temp key used for look; this is use to make sure that key is allocated
    // on the correct local heap.
    const GFxTextStyleKey&  GetTempStyleKey(GFxTextStyleKey::KeyType t, const GString& val) const
    {
        TempKey.Set(t, val);
        return TempKey;
    }

private:
    GFxTextStyleHash         Styles; 
    
    // Store key here to have a temporary for style lookup.
    mutable GFxTextStyleKey  TempKey;

    template <typename Char>
    bool ParseCSSImpl(const Char* buffer, UPInt len);
};


#endif // GFC_NO_CSS_SUPPORT

#endif // INC_GFxStyleSheet_H

