/**********************************************************************

Filename    :   GFxFontManager.h
Content     :   Font manager functionality
Created     :   May 18, 2007
Authors     :   Artyom Bolgar, Michael Antonov

Notes       :   
History     :   

Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXFONTMANAGER_H
#define INC_GFXFONTMANAGER_H

#include "GUTF8Util.h"
#include "GArray.h"
#include "GFxFontResource.h"
#include "GFxStringHash.h"
#include "GFxFontLib.h"


class GFxFontManager;
class GFxMovieDefImpl;

class GFxFontHandle : public GNewOverrideBase<GFxStatMV_Other_Mem>
{
    friend class GFxFontManager;    

    // Our ref-count. This ref-count is not thread-safe
    // because font managers are local to 'root' sprites.
    SInt                    RefCount;    

    // Font manager which we come from. Can be null for static text fields.
    GFxFontManager*         pFontManager;

    // A font device flag applied to a non-device font, as can be done
    // if the font is taken from GFxFontLib due to its precedence.
    UInt                    OverridenFontFlags;   

    // Search FontName - could be an export name instead of the
    // name of the font.
    GStringLH               FontName;

    Float                   FontScaleFactor;

    Float                   GlyphOffsetX, GlyphOffsetY;

public:
    // Font we are holding.
    GPtr<GFxFontResource>   pFont;

    enum FontFlags
    {
        // General font style flags.
        FF_Italic               = GFxFont::FF_Italic,
        FF_Bold                 = GFxFont::FF_Bold,
        FF_BoldItalic           = GFxFont::FF_BoldItalic,
        FF_Style_Mask           = GFxFont::FF_Style_Mask,

        FF_NoAutoFit            = 0x20
    };
    // A strong pointer to GFxMovieDefImpl from which the font was created.
    // Font manager needs to hold these pointers for those fonts which came
    // from the GFxFontProvider, since our instance can be the only reference
    // to them. Null for all other cases.
    GPtr<GFxMovieDef>   pSourceMovieDef;

    GFxFontHandle(GFxFontManager *pmanager, GFxFontResource *pfont,
                  const char* pfontName = 0, UInt overridenFontFlags = 0,
                  GFxMovieDef* pdefImpl = 0)
        : RefCount(1), pFontManager(pmanager), OverridenFontFlags(overridenFontFlags),
          FontScaleFactor(1.0f), pFont(pfont), pSourceMovieDef(pdefImpl)
    {
        GlyphOffsetX = GlyphOffsetY = 0;
        // If font name doesn't match, store it locally.   
        if (pfontName && (GString::CompareNoCase(pfont->GetName(), pfontName) != 0))
            FontName = pfontName;
    }
    GFxFontHandle(const GFxFontHandle& f) : RefCount(1), pFontManager(f.pFontManager),
        OverridenFontFlags(f.OverridenFontFlags), FontName(f.FontName),
        FontScaleFactor(f.FontScaleFactor), GlyphOffsetX(f.GlyphOffsetX), GlyphOffsetY(f.GlyphOffsetY), 
        pFont(f.pFont), pSourceMovieDef(f.pSourceMovieDef)
    {
    }
    inline ~GFxFontHandle();
    
    GFxFontResource*    GetFont() const { return pFont; }
    const char*         GetFontName() const
    {
        return FontName.IsEmpty() ? GetFont()->GetName() : FontName.ToCStr();
    }
    UInt                GetFontFlags() const
    {
        return GetFont()->GetFontFlags() | OverridenFontFlags;
    }
    bool                IsFauxBold() const       { return (OverridenFontFlags & FF_Bold) != 0; }
    bool                IsFauxItalic() const     { return (OverridenFontFlags & FF_Italic) != 0; }
    bool                IsFauxBoldItalic() const { return (OverridenFontFlags & FF_BoldItalic) == FF_BoldItalic; }
    UInt                GetFauxFontStyle() const { return (OverridenFontFlags & FF_Style_Mask); }
    UInt                GetFontStyle() const     
    { 
        return ((GetFont()->GetFontFlags() | GetFauxFontStyle()) & FF_Style_Mask);
    }
    bool                IsAutoFitDisabled() const { return (OverridenFontFlags & FF_NoAutoFit) != 0; }

    Float               GetFontScaleFactor() const { return FontScaleFactor; }

    void                AddRef()    { RefCount++; }
    void                Release()   { if (--RefCount == 0) delete this; }

    bool                operator==(const GFxFontHandle& f) const
    {
        return pFontManager == f.pFontManager && pFont == f.pFont && 
               OverridenFontFlags == f.OverridenFontFlags && FontName == f.FontName &&
               FontScaleFactor == f.FontScaleFactor && pSourceMovieDef == f.pSourceMovieDef &&
               GlyphOffsetX == f.GlyphOffsetX && GlyphOffsetY == f.GlyphOffsetY;
    }

    void                SetGlyphOffsets(Float x, Float y) 
    { 
        GlyphOffsetX = x;
        GlyphOffsetY = y;
    }
    Float               GetGlyphOffsetX() const { return GlyphOffsetX; }
    Float               GetGlyphOffsetY() const { return GlyphOffsetY; }
};


class GFxFontManagerStates : public GRefCountBaseNTS<GFxFontManagerStates, GFxStatMV_Other_Mem>, public GFxStateBag
{
public:
    GFxFontManagerStates(GFxStateBag* pdelegate) : pDelegate(pdelegate) {}

    virtual GFxStateBag*    GetStateBagImpl() const { return pDelegate; }

    virtual GFxState*       GetStateAddRef(GFxState::StateType state) const;

    UInt8                   CheckStateChange(GFxFontLib*, GFxFontMap*, GFxFontProvider*, GFxTranslator*);

    GPtr<GFxFontLib>      pFontLib;
    GPtr<GFxFontMap>      pFontMap;
    GPtr<GFxFontProvider> pFontProvider;
    GPtr<GFxTranslator>   pTranslator;

private:
    GFxStateBag*       pDelegate;
};

class GFxFontManager : public GRefCountBaseNTS<GFxFontManager, GFxStatMV_Other_Mem>
{
    friend class GFxFontHandle;    
public:

    // Structure for retrieving a font's full search path. It is 
    // used in cases if a font is not found for generating more
    // informative error message
    struct FontSearchPathInfo
    {
        FontSearchPathInfo(SInt indent = 0) : Indent(indent) {}
        SInt            Indent;
        GStringBuffer   Info;
    };

protected:
    
    // Make a hash-set entry that tracks font handle nodes.
    struct FontKey 
    {
        const char*    pFontName;
        UInt           FontStyle;

        FontKey() : FontStyle(0) {}
        FontKey(const char* pfontName, UInt style)
            : pFontName(pfontName), FontStyle(style) { }

        static UPInt CalcHashCode(const char* pname, UPInt namelen, UInt fontFlags)
        {
            UPInt hash = GString::BernsteinHashFunctionCIS(pname, namelen);
            return (hash ^ (fontFlags & GFxFont::FF_Style_Mask));
        }
    };

    struct NodePtr
    {
        GFxFontHandle* pNode;
               
        NodePtr() { }
        NodePtr(GFxFontHandle* pnode) : pNode(pnode) { }
        NodePtr(const NodePtr& other) : pNode(other.pNode) { }
        
        // Two nodes are identical if their fonts match.
        bool operator == (const NodePtr& other) const
        {
            if (pNode == other.pNode)
                return 1;
            // For node comparisons we have to handle DeviceFont strictly ALL the time,
            // because it is possible to have BOTH versions of font it the hash table.
            enum { FontFlagsMask = GFxFont::FF_CreateFont_Mask | GFxFont::FF_DeviceFont | GFxFont::FF_BoldItalic};
            UInt ourFlags   = pNode->GetFontFlags() & FontFlagsMask;
            UInt otherFlags = other.pNode->GetFontFlags() & FontFlagsMask;
            return ((ourFlags == otherFlags) &&
                    !GString::CompareNoCase(pNode->GetFontName(), other.pNode->GetFontName()));
        }        

        // Key search uses MatchFont to ensure that DeviceFont flag is handled correctly.
        bool operator == (const FontKey &key) const
        {
            return (GFxFont::MatchFontFlags_Static(pNode->GetFontFlags(), key.FontStyle) &&
                    !GString::CompareNoCase(pNode->GetFontName(), key.pFontName));
        }
    };
    
    struct NodePtrHashOp
    {                
        // Hash code is computed based on a state key.
        UPInt  operator() (const NodePtr& other) const
        {            
            GFxFontHandle* pnode = other.pNode;
            const char* pfontName = pnode->GetFontName();
            return (UPInt) FontKey::CalcHashCode(pfontName, G_strlen(pfontName), 
                pnode->pFont->GetFontFlags() | pnode->GetFontStyle());
        }
        UPInt  operator() (const GFxFontHandle* pnode) const
        {
            GASSERT(pnode != 0);            
            const char* pfontName = pnode->GetFontName();
            return (UPInt) FontKey::CalcHashCode(pfontName, G_strlen(pfontName), 
                pnode->pFont->GetFontFlags() | pnode->GetFontStyle());
        }
        UPInt  operator() (const FontKey& data) const
        {
            return (UPInt) FontKey::CalcHashCode(data.pFontName, G_strlen(data.pFontName), data.FontStyle);
        }
    };


    // State hash
    typedef GHashSetLH<NodePtr, NodePtrHashOp, NodePtrHashOp> FontSet;

    // Keep a hash of all allocated nodes, so that we can find one when necessary.
    // Nodes automatically remove themselves when all their references die.
    FontSet                 CreatedFonts;

    // GFxMovieDefImpl which this font manager is associated with. We look up
    // fonts here by default before considering pFontProvider. Note that this
    // behavior changes if device flags are passed.
    GFxMovieDefImpl*        pDefImpl;
    // pWeakLib might be set and used in the case if pDefImpl == NULL
    GFxResourceWeakLib*     pWeakLib;
    // Shared state for the instance, comes from GFxMovieRoot. Shouldn't need 
    // to AddRef since nested sprite (and font manager) should die before MovieRoot.    
    GFxFontManagerStates*   pState;


    // Empty fake font returned if requested font is not found.
    GPtr<GFxFontHandle>      pEmptyFont; 

    // FontMap Entry temporary; here to avoid extra constructor/destructor calls.
    GFxFontMap::MapEntry     FontMapEntry;

#ifndef GFC_NO_IME_SUPPORT
    GPtr<GFxFontHandle>      pIMECandidateFont;
#endif //#ifndef GFC_NO_IME_SUPPORT

   
    // Helper function to remove handle from CreatedFonts when it dies.
    void                     RemoveFontHandle(GFxFontHandle *phandle);

    GFxFontHandle*           FindOrCreateHandle(const char* pfontName, UInt matchFontFlags, 
                                                GFxFontResource** ppfoundFont, FontSearchPathInfo* searchInfo);

    GFxFontHandle*           CreateFontHandleFromName(const char* pfontName, UInt matchFontFlags, 
                                                      FontSearchPathInfo* searchInfo);
private:
    void commonInit();
public:
    GFxFontManager(GFxMovieDefImpl *pdefImpl,
                   GFxFontManagerStates* pState);
    GFxFontManager(GFxResourceWeakLib *pweakLib,
                   GFxFontManagerStates* pState);
    ~GFxFontManager();


    // Returns font by name and style. If a non-NULL searchInfo passed to the method
    // it assumes that it was called only for a diagnostic purpose and in this case font
    // will not be searched in the internal cache and a created font handle will not be cached.
    GFxFontHandle*          CreateFontHandle(const char* pfontName, UInt matchFontFlags, 
                                             bool allowListOfFonts = true, FontSearchPathInfo* searchInfo = NULL); 

    GFxFontHandle*          CreateFontHandle(const char* pfontName, bool bold, bool italic, bool device, 
                                             bool allowListOfFonts = true, FontSearchPathInfo* searchInfo = NULL)
    { 
        UInt matchFontFlags = ((bold) ? GFxFont::FF_Bold : 0) |
                              ((italic) ? GFxFont::FF_Italic : 0) |
                              ((device) ? GFxFont::FF_DeviceFont : 0);
        return CreateFontHandle(pfontName, matchFontFlags, allowListOfFonts, searchInfo);
    }

    // Returns any font with the font name 'pfontName' or the first one
    // in the hash.
    //GFxFontHandle*          CreateAnyFontHandle(const char* pfontName, FontSearchPathInfo* searchInfo = NULL);

    GFxFontHandle* GetEmptyFont();

    // Clean internal cache. This method is called from Advance method of 
    // MovieRoot then it detects that FontLib, FontMap, FontProvider or Translator 
    // is changed
    void                    CleanCache();

#ifndef GFC_NO_IME_SUPPORT
    void                    SetIMECandidateFont(GFxFontHandle* pfont);
#endif //#ifndef GFC_NO_IME_SUPPORT

};


// Font handle inlined after font manager because it uses manager's APIs.
inline GFxFontHandle::~GFxFontHandle()
{        
    // Remove from hash.
    if (pFontManager)
        pFontManager->RemoveFontHandle(this);

    // Must release pFont before its containing GFxMovieDefImpl destructor,
    // since otherwise we'd be accessing a dying heap.
    pFont = 0;
}


#endif //INC_GFXFONTMANAGER_H
