/**********************************************************************

Filename    :   GFxFontManager.cpp
Content     :   Font manager functinality
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
#include "GFxFontManager.h"
#include "GFxMovieDef.h"
#include "GFxFontLib.h"
#include "GFxCharacter.h"
#include "GMsgFormat.h"

GFxState* GFxFontManagerStates::GetStateAddRef(GFxState::StateType state) const
{
    if (state == GFxState::State_FontLib)
    {
        if (pFontLib)
            pFontLib->AddRef();
        return pFontLib.GetPtr();
    }
    else if (state == GFxState::State_FontMap)
    {
        if (pFontMap)
            pFontMap->AddRef();
        return pFontMap.GetPtr();
    }
    else if (state == GFxState::State_FontProvider)
    {
        if (pFontProvider)
            pFontProvider->AddRef();
        return pFontProvider.GetPtr();
    }
    if (state == GFxState::State_Translator)
    {
        if (pTranslator)
            pTranslator->AddRef();
        return pTranslator.GetPtr();
    }
    GASSERT(pDelegate);
    return pDelegate->GetStateAddRef(state);
}

UInt8 GFxFontManagerStates::CheckStateChange(GFxFontLib* pfontLib, GFxFontMap* pfontMap,
                                             GFxFontProvider* pfontProvider, GFxTranslator* ptranslator)
{
    UInt8 stateChangeFlag = 0u;

    if (pFontLib.GetPtr() != pfontLib)
    {
        stateChangeFlag |= GFxCharacter::StateChanged_FontLib;
        pFontLib = pfontLib;
    }
    if (pFontMap.GetPtr() != pfontMap)
    {
        stateChangeFlag |=  GFxCharacter::StateChanged_FontMap;
        pFontMap = pfontMap;
    }
    if (pFontProvider.GetPtr() != pfontProvider)
    {
        stateChangeFlag |=  GFxCharacter::StateChanged_FontProvider;
        pFontProvider = pfontProvider;
    }
    if (pTranslator.GetPtr() != ptranslator)
    {
        stateChangeFlag |=  GFxCharacter::StateChanged_Translator;
        pTranslator = ptranslator;
    }
    return stateChangeFlag;
}

//////////////////////////////////
// GFxFontManager
//
GFxFontManager::GFxFontManager(GFxMovieDefImpl *pdefImpl,
                               GFxFontManagerStates *pstate)
{
    pDefImpl      = pdefImpl;
    pWeakLib      = NULL;
    pState        = pstate;
    commonInit();
}

GFxFontManager::GFxFontManager(GFxResourceWeakLib *pweakLib,
                               GFxFontManagerStates *pstate)
{
    pDefImpl      = NULL;
    pWeakLib      = pweakLib;
    pState        = pstate;
    commonInit();
}

void GFxFontManager::commonInit()
{
    GPtr<GFxFontData> pfontData = *GHEAP_AUTO_NEW(this) GFxFontData();
    GPtr<GFxFontResource> pemptyFont = *GHEAP_AUTO_NEW(this) GFxFontResource(pfontData, 0);
    pEmptyFont = *GHEAP_AUTO_NEW(this) GFxFontHandle(0, pemptyFont);
}

GFxFontManager::~GFxFontManager()
{
    // CachedFonts must be empty here.
    GASSERT(CreatedFonts.GetSize() == 0);
}


void GFxFontManager::RemoveFontHandle(GFxFontHandle *phandle)
{
    if (phandle != pEmptyFont)
    {
        UInt fontCount = (UInt)CreatedFonts.GetSize();
        GUNUSED(fontCount);
        //if (!fontCount)
        //    return;
        CreatedFonts.Remove(phandle);
        // This should not fail but has been seen to do so due to font style mask
        // mismatch. The new operator == which compares pointers resolves that.
        GASSERT(CreatedFonts.GetSize() == fontCount - 1);
    }
}

static const char *StrFlags[] = 
{
    "", "[Bold]", "[Italic]", "[Bold,Italic]", "[Device]", "[Bold,Device]", "[Italic,Device]", "[Bold,Italic,Device]"
};

static inline const char* FontFlagsToString(UInt matchFontFlags)
{
    if (!matchFontFlags)
        return StrFlags[0];
    if (matchFontFlags & GFxFont::FF_DeviceFont)
    {
        if((matchFontFlags & GFxFont::FF_BoldItalic) == GFxFont::FF_BoldItalic)
            return StrFlags[7];
        if(matchFontFlags & GFxFont::FF_Bold)
            return StrFlags[5];
        if(matchFontFlags & GFxFont::FF_Italic)
            return StrFlags[6];
        return StrFlags[4];
    }
    else
    {
        if((matchFontFlags & GFxFont::FF_BoldItalic) == GFxFont::FF_BoldItalic)
            return StrFlags[3];
        if(matchFontFlags & GFxFont::FF_Bold)
            return StrFlags[1];
        if(matchFontFlags & GFxFont::FF_Italic)
            return StrFlags[2];
    }
    return StrFlags[0];
}

static inline void AddSearchInfo(GFxFontManager::FontSearchPathInfo* psearchInfo, const char* line)
{
    if (!psearchInfo)
        return;
    GString sindent("   ");
    for (SInt i = 0; i < psearchInfo->Indent; ++i)
        psearchInfo->Info += sindent;
    psearchInfo->Info += line;
    psearchInfo->Info += "\n";
}

static inline void AddSearchInfo(GFxFontManager::FontSearchPathInfo* psearchInfo, const char* pfontname, UInt flags, bool fontlib_installed,
                                 const GFxMovieDefImpl::SearchInfo& resSearchInfo)
{
    char buff[1024];
    GStringDataPtr buffData(buff, sizeof(buff));

    if (!psearchInfo)
        return;
    if(resSearchInfo.Status == GFxMovieDefImpl::SearchInfo::FoundInResources)
    {
        G_Format(buffData, "Movie resource: \"{0}\" {1} found.", pfontname, FontFlagsToString(flags));
        AddSearchInfo(psearchInfo, buff);
        return;
    }
    if(resSearchInfo.Status == GFxMovieDefImpl::SearchInfo::FoundInResourcesNeedFaux)
    {
        G_Format(buffData, "Movie resource: \"{0}\" {1} found, requires faux", pfontname, FontFlagsToString(flags));
        AddSearchInfo(psearchInfo, buff);
        return;
    }
    if(resSearchInfo.Status == GFxMovieDefImpl::SearchInfo::FoundInResourcesNoGlyphs)
    {
        G_Format(buffData, "Movie resource: \"{0}\" {1} ref found, requires FontLib/Map/Provider.",
                            pfontname, FontFlagsToString(flags));
        AddSearchInfo(psearchInfo, buff);
        return;
    }

    G_Format(buffData, "Movie resource: \"{0}\" {1} not found.", pfontname, FontFlagsToString(flags));
    AddSearchInfo(psearchInfo, buff);
    if(resSearchInfo.Status == GFxMovieDefImpl::SearchInfo::FoundInImports || 
       (resSearchInfo.Status == GFxMovieDefImpl::SearchInfo::FoundInImportsFontLib && !fontlib_installed) )
    {
        G_Format(buffData, "Imports       : \"{0}\" {1} found in \"{2}\".", 
                 pfontname, FontFlagsToString(flags), resSearchInfo.ImportFoundUrl);
        AddSearchInfo(psearchInfo, buff);
        return;
    }
    if(resSearchInfo.Status == GFxMovieDefImpl::SearchInfo::FoundInImportsFontLib)
    {
        G_Format(buffData, "Imports       : \"{0}\" {1} import delegates to font library.", 
                 pfontname, FontFlagsToString(flags));
        AddSearchInfo(psearchInfo, buff);
        return;
    }
    G_Format(buffData, "Imports       : \"{0}\" {1} not found.", pfontname, FontFlagsToString(flags));
    AddSearchInfo(psearchInfo, buff);
    if (resSearchInfo.ImportSearchUrls.GetSize() > 0)
    {
        GString tmp;
        for(GFxMovieDefImpl::SearchInfo::StringSet::ConstIterator it = resSearchInfo.ImportSearchUrls.Begin(); 
                it != resSearchInfo.ImportSearchUrls.End(); ++it)
        {
            if (it != resSearchInfo.ImportSearchUrls.Begin())
                tmp += ", ";
            tmp += GString("\"") + *it + "\"";
        }
        G_Format(buffData, "              : {0}.", tmp);
        AddSearchInfo(psearchInfo, buff);
    }
    if(resSearchInfo.Status == GFxMovieDefImpl::SearchInfo::FoundInExports)
    {
        G_Format(buffData, "Exported      : \"{0}\" {1} found.", pfontname, FontFlagsToString(flags));
        AddSearchInfo(psearchInfo, buff);
        return;
    }
    G_Format(buffData, "Exported      : \"{0}\" {1} not found.", pfontname, FontFlagsToString(flags));
    AddSearchInfo(psearchInfo, buff);
}

static inline void AddSearchInfo(GFxFontManager::FontSearchPathInfo* psearchInfo, const char* str1,
                                 const char* str2, const char* str3)
{
    if (!psearchInfo)
        return;
    GStringBuffer buf;
    buf += str1;
    buf += str2;
    buf += str3;
    AddSearchInfo(psearchInfo, buf);
}
static inline void AddSearchInfo(GFxFontManager::FontSearchPathInfo* psearchInfo, const char* str1, const char* str2, 
                                 const char* str3, UInt flags, const char* str4)
{
    if (!psearchInfo)
        return;
    GStringBuffer buf;
    buf += str1;
    buf += str2;
    buf += str3;
    buf += FontFlagsToString(flags);
    buf += str4;
    AddSearchInfo(psearchInfo, buf);
}
static inline void AddSearchInfo(GFxFontManager::FontSearchPathInfo* psearchInfo, const char* str1, const char* str2, const char* str3, 
                                 UInt flags1, const char* str4, const char* str5, const char* str6, UInt flags2)
{
    if (!psearchInfo)
        return;
    GStringBuffer buf;
    buf += str1;
    buf += str2;
    buf += str3;
    buf += FontFlagsToString(flags1);
    buf += str4;
    buf += str5;
    buf += str6;
    buf += FontFlagsToString(flags2);
    AddSearchInfo(psearchInfo, buf);
}

static inline void AddSearchInfo(GFxFontManager::FontSearchPathInfo* psearchInfo, const char* str1, const char* str2,
                                 const char* str3, const char* str4, const char* str5, UInt flags)
{
    if (!psearchInfo)
        return;
    GStringBuffer buf;
    buf += str1;
    buf += str2;
    buf += str3;
    buf += str4;
    buf += str5;
    buf += FontFlagsToString(flags);
    AddSearchInfo(psearchInfo, buf);
}
// If searchInfo is not NULL then the search algorithm will not check internal cache and any new created FontHandles 
// will not be cached
GFxFontHandle* GFxFontManager::FindOrCreateHandle(const char* pfontName, UInt matchFontFlags, 
                                                  GFxFontResource** ppfoundFont,
                                                  FontSearchPathInfo* searchInfo)
{
// DBG
//GASSERT(!(matchFontFlags & GFxFont::FF_BoldItalic));
//if (matchFontFlags & GFxFont::FF_BoldItalic)
//    return 0;

    FontKey          key(pfontName, matchFontFlags);
    GFxFontHandle*   phandle       = 0;
    GFxFontLib*      pfontLib      = pState->pFontLib.GetPtr();
    GFxFontProvider* pfontProvider = pState->pFontProvider.GetPtr();
    GFxFontMap*      pfontMap      = pState->pFontMap.GetPtr();

    // Our matchFontFlag argument may include FF_DeviceFont flag if device preference is
    // requested. The biggest part this is handled correctly by MatchFont used in our
    // CreatedFonts.Get(), pDefImpl->GetFontResource and GFxFontLib lookups. However,
    // if device font match is not found we may need to do a second cycle to find an embedded
    // font instead (default Flash behavior, since DeviceFont is only a hint).
    SInt indentDif = 0;
    const NodePtr* psetNode = NULL;
    GFxMovieDefImpl::SearchInfo resSearchInfo;
    GFxMovieDefImpl::SearchInfo* presSearchInfo = NULL;
    bool secondLoop                             = false;

#ifndef GFC_NO_IME_SUPPORT
    if (pIMECandidateFont && !GString::CompareNoCase(pIMECandidateFont->GetFontName(), pfontName))
    {
        pIMECandidateFont->AddRef();
        return pIMECandidateFont;
    }
#endif //#ifndef GFC_NO_IME_SUPPORT

retry_font_lookup:
    if (searchInfo)
    {
        indentDif++;
        searchInfo->Indent++;
        presSearchInfo = &resSearchInfo;
    } 
    else
    {
        // If the font has already been created, return it.
        psetNode = CreatedFonts.Get(key);
        if (psetNode)
        {
            psetNode->pNode->AddRef();
            return psetNode->pNode;
        }
    }

    // Search MovieDef for the desired font and create it if necessary.
    // The resource is not AddRefed here, so don't do extra release.
    GFxFontResource* pfontRes = 
        (pDefImpl) ? pDefImpl->GetFontResource(pfontName, matchFontFlags, presSearchInfo) : NULL;
    AddSearchInfo(searchInfo, pfontName, matchFontFlags, pState->pFontLib.GetPtr() != NULL, resSearchInfo);
    if (pfontRes)
    {
        // If NotResolve flag for the found font is set we have to search it through 
        // FontLib and FontProvider
        if (pfontRes->GetFont()->IsResolved())
        {
            // If we are looking for bold/italic, and there is an empty font found, 
            // then return NULL to give opportunity to faux bold/italic.
            if ((matchFontFlags & GFxFont::FF_BoldItalic) && !pfontRes->GetFont()->HasVectorOrRasterGlyphs())
            {
                // need to pass the found font out, just in case if font manager
                // fails looking for a regular font for faux bold/italic
                if (ppfoundFont) 
                    *ppfoundFont = pfontRes;
                return 0;
            }
            // The above lookup can return a font WITH A DIFFERENT name in
            // case export names were used for a search. If that happens,
            // the passed name will be stored and used in GFxFontHandle.
            phandle = GHEAP_AUTO_NEW(this) GFxFontHandle(searchInfo? NULL : this, pfontRes, pfontName);
        }
    }

    // Lookup an alternative font name for the font. Font flags can
    // also be altered if the font map forces a style.
    const char* plookupFontName = pfontName;
    UInt        lookupFlags     = matchFontFlags;
    Float       scaleFactor     = 1.0f;
    UInt        overridenFlags  = 0;
    Float       offx = 0, offy = 0;

    if (!phandle && pfontMap)
    {       
        if (pfontMap->GetFontMapping(&FontMapEntry, pfontName))
        {
            plookupFontName = FontMapEntry.Name.ToCStr();
            lookupFlags     = FontMapEntry.UpdateFontFlags(matchFontFlags);
            scaleFactor     = FontMapEntry.ScaleFactor;
            offx            = FontMapEntry.GlyphOffsetX;
            offy            = FontMapEntry.GlyphOffsetY;
            overridenFlags  = FontMapEntry.GetOverridenFlags();
            if (searchInfo)
                AddSearchInfo(searchInfo, "Applying GFxFontMap: \"", pfontName, "\"  mapped to \"", 
                                            plookupFontName, "\"" , lookupFlags);
            else
            {
                // When we substitute a font name, it is possible that
                // this name already existed - so do a hash lookup again.
                FontKey    lookupKey(plookupFontName, lookupFlags);
                psetNode = CreatedFonts.Get(lookupKey);
                if (psetNode)
                {
                    // If font scale factor of a fount font is not the same as in FontMapEntry we 
                    // need to create a new font handle
                    if (psetNode->pNode->FontScaleFactor != scaleFactor)
                    {
                        phandle = GHEAP_AUTO_NEW(this)
                            GFxFontHandle(searchInfo? NULL : this, psetNode->pNode->pFont, 
                                          pfontName, matchFontFlags, psetNode->pNode->pSourceMovieDef );
                        phandle->FontScaleFactor = scaleFactor;
                        phandle->SetGlyphOffsets(offx, offy);
                    }
                    else
                    {
                        psetNode->pNode->AddRef();
                        return psetNode->pNode;
                    }
                }

                // Well, we need to check for (pfontName, changed lookupFlags) also,
                // since in the case if a normal font with name "name" is substituted by 
                // the bold one, that substitution will be placed in the hash table as (name, Bold),
                // even though we were (and will be next time!) looking for the (name, 0).
                FontKey    lookupKey2(pfontName, lookupFlags);
                psetNode = CreatedFonts.Get(lookupKey2);
                if (psetNode)
                {
                    // If font scale factor of a fount font is not the same as in FontMapEntry we 
                    // need to create a new font handle
                    if (psetNode->pNode->FontScaleFactor != scaleFactor)
                    {
                        phandle = GHEAP_AUTO_NEW(this) GFxFontHandle(searchInfo? NULL :this, psetNode->pNode->pFont, 
                            pfontName, matchFontFlags, psetNode->pNode->pSourceMovieDef );
                        phandle->FontScaleFactor = scaleFactor;
                        phandle->SetGlyphOffsets(offx, offy);
                    }
                    else
                    {
                        psetNode->pNode->AddRef();
                        return psetNode->pNode;
                    }
                }
            }
        }
    }

    if (!phandle && pfontLib)
    {
        // Find/create compatible font through FontLib.
        GFxFontLib::FontResult fr;
        if (pfontLib->FindFont(&fr, plookupFontName, lookupFlags,
            pDefImpl, pState, pWeakLib))
        {
            if (overridenFlags)
            {
                // if FauxBold or/and FauxItalic attributes are set (overridenFlags != 0)
                // then need to look for already existing font handle first.
                // If not found - create one with using overridenFlags.
                FontKey    lookupKey(pfontName, overridenFlags);
                psetNode = CreatedFonts.Get(lookupKey);
                if (!psetNode)
                {
                    // Use font handle to hold reference to resource's MovieDef.
                    phandle = GHEAP_AUTO_NEW(this) GFxFontHandle(searchInfo? NULL :this, fr.GetFontResource(),
                        pfontName, overridenFlags, fr.GetMovieDef());
                    phandle->FontScaleFactor = scaleFactor;
                    phandle->SetGlyphOffsets(offx, offy);
                }
                else
                {
                    psetNode->pNode->AddRef();
                    return psetNode->pNode;
                }
                AddSearchInfo(searchInfo, "Searching GFxFontLib: \"", pfontName, "\" ", overridenFlags, " found.");
            }
            else
            {
                // Use font handle to hold reference to resource's MovieDef.
                phandle = GHEAP_AUTO_NEW(this) GFxFontHandle(searchInfo? NULL :this, fr.GetFontResource(),
                    pfontName, 0, fr.GetMovieDef());
                phandle->FontScaleFactor = scaleFactor;
                phandle->SetGlyphOffsets(offx, offy);
                AddSearchInfo(searchInfo, "Searching GFxFontLib: \"", plookupFontName, "\" ", lookupFlags, " found.");
            }
        }
        else
            AddSearchInfo(searchInfo, "Searching GFxFontLib: \"", plookupFontName, "\" ", lookupFlags, " not found.");
    }
    if (!secondLoop && searchInfo && !phandle && !pfontLib && resSearchInfo.Status == GFxMovieDefImpl::SearchInfo::FoundInResourcesNoGlyphs)
        AddSearchInfo(searchInfo, "GFxFontLib not installed.");

    if (!phandle && pfontProvider)
    {
        // Create or lookup a system font if system is available.
        GASSERT(pDefImpl || pWeakLib);
        GPtr<GFxFontResource> pfont = 
            *GFxFontResource::CreateFontResource(plookupFontName, lookupFlags,
            pfontProvider, ((pDefImpl) ? pDefImpl->GetWeakLib() : pWeakLib));
        if (pfont)
        {
            AddSearchInfo(searchInfo, "Searching GFxFontProvider: \"", plookupFontName, "\" ", lookupFlags, " found.");
            phandle = GHEAP_AUTO_NEW(this) GFxFontHandle(searchInfo? NULL : this, pfont, pfontName);
            phandle->FontScaleFactor = scaleFactor;
            phandle->SetGlyphOffsets(offx, offy);
        }
        else
            AddSearchInfo(searchInfo, "Searching GFxFontProvider: \"", plookupFontName, "\" ", lookupFlags, " not found.");
    }
    if (!secondLoop && searchInfo && !phandle && !pfontProvider && resSearchInfo.Status == GFxMovieDefImpl::SearchInfo::FoundInResourcesNoGlyphs)
        AddSearchInfo(searchInfo, "GFxFontProvider not installed.");


    // If device font was not found, try fontLib without that flag,
    // as GFxFontLib is allowed to masquerade as a device font source.
    if (!phandle && pfontLib && (lookupFlags & GFxFont::FF_DeviceFont))
    {
        // Find/create compatible font through FontLib.
        UInt lf = lookupFlags & ~GFxFont::FF_DeviceFont;
        GFxFontLib::FontResult fr;
        if (pfontLib->FindFont(&fr, plookupFontName, lf, pDefImpl, pState))
        {
            // Use font handle to hold reference to resource's MovieDef.
            phandle = GHEAP_AUTO_NEW(this) GFxFontHandle(searchInfo? NULL : this, fr.GetFontResource(),
                                        pfontName, GFxFont::FF_DeviceFont, fr.GetMovieDef());
            phandle->FontScaleFactor = scaleFactor;
            phandle->SetGlyphOffsets(offx, offy);
            AddSearchInfo(searchInfo, "Searching GFxFontLib without [Device] flag: \"", plookupFontName, "\" ", lf, " found.");
        }
        else
            AddSearchInfo(searchInfo, "Searching GFxFontLib without [Device] flag: \"", plookupFontName, "\" ", lf, " not found.");
    }


    // If font handle was successfully created, add it to list and return.
    if (phandle)
    {
        if (searchInfo)
            searchInfo->Indent -= indentDif;
        else 
        {
            GASSERT(CreatedFonts.Get(phandle) == 0);
            CreatedFonts.Add(phandle);
        }
        return phandle;
    }

    // If DeviceFont was not found, search for an embedded font.
    if (matchFontFlags & GFxFont::FF_DeviceFont)
    {
        AddSearchInfo(searchInfo, "Searching again without [Device] flag:");
        matchFontFlags &= ~GFxFont::FF_DeviceFont;
        // Must modify key flags value otherwise incorrect multiple copies of the same
        // font would get created and put into a hash (causing waste and crash later).
        key.FontStyle  = matchFontFlags;
        // Do not search font provider in a second pass since failure in
        // the first pass would imply failure in the second one as well.
        pfontProvider = 0;
        pfontLib = 0;
        secondLoop = true;
        goto retry_font_lookup;
    }
    if (searchInfo)
        searchInfo->Indent -= indentDif;
    return 0;
}

GFxFontHandle* GFxFontManager::CreateFontHandle(const char* pfontName, UInt matchFontFlags, 
                                                bool allowListOfFonts, FontSearchPathInfo* searchInfo)
{
    if (!allowListOfFonts)
        return CreateFontHandleFromName(pfontName, matchFontFlags, searchInfo);
    else
    {
        GFxFontHandle* fh = NULL;
        // traverse through the comma-separated list of fonts in pfontName,
        // until font is created.
        const char* pe = NULL;
        char buf[128];
        do 
        {
            pe = G_strchr(pfontName, ',');
            const char* curName;
            UPInt len;
            if (pe)
            {
                len = pe - pfontName;
                if (len > sizeof(buf) - 1)
                    continue; // skip too long font name - it doesn't fit to our buffer
                G_strncpy(buf, sizeof(buf)-1, pfontName, len);
                buf[len] = '\0';
                curName = buf;
                pfontName += len + 1; // skip the name + comma
            }
            else
            {
                curName = pfontName;
                len = G_strlen(pfontName);
            }
            fh = CreateFontHandleFromName(curName, matchFontFlags, searchInfo);
        } while (!fh && pe != NULL);
        return fh;
    }
}

GFxFontHandle* GFxFontManager::CreateFontHandleFromName(const char* pfontName, UInt matchFontFlags, 
                                                        FontSearchPathInfo* searchInfo)
{
    SInt saveIndent = 0;
    if (searchInfo)
    {
        saveIndent = searchInfo->Indent;
        AddSearchInfo(searchInfo, "Searching for font: \"", pfontName, "\" ", matchFontFlags, "");
    }
    GFxFontResource*    pfoundFont = NULL;
    GFxFontHandle*      phandle = FindOrCreateHandle(pfontName, matchFontFlags, &pfoundFont, searchInfo);

    if (!phandle)
    {
        // if Bold or/and Italic was not found, search for a regular font to 
        // make faux bold/italic
        if (matchFontFlags & GFxFont::FF_BoldItalic)
        {
            UInt newMatchFontFlags = matchFontFlags & (~GFxFont::FF_BoldItalic);
            if (searchInfo)
            {
                searchInfo->Indent++;
                AddSearchInfo(searchInfo, "Searching for font: \"", pfontName, "\" " , newMatchFontFlags, "");
            }
            GPtr<GFxFontHandle> pplainHandle = *FindOrCreateHandle(pfontName, newMatchFontFlags, NULL, searchInfo);
            if (pplainHandle)
            {
                // copy the handle, modify with faux bold/italic flags and add
                // it to a table.
                phandle = GHEAP_AUTO_NEW(this) GFxFontHandle(*pplainHandle);
                phandle->OverridenFontFlags |= (matchFontFlags & GFxFont::FF_BoldItalic);
                
                // If font handle was successfully created, add it to list and return.
                if (!searchInfo)
                {
                    GASSERT(CreatedFonts.Get(phandle) == 0);
                    CreatedFonts.Add(phandle);
                }
                else
                    AddSearchInfo(searchInfo, "Font \"", pfontName, "\" ", matchFontFlags, " will be generated from \"", pfontName, "\"", newMatchFontFlags);
            }
        }
    }

    // if handle is not created, but the original font (empty) was found - 
    // use it.
    if (searchInfo)
        searchInfo->Indent = saveIndent;
    if (!phandle && pfoundFont)
    {
        AddSearchInfo(searchInfo, "Empty font: \"", pfontName, "\" is created");
        phandle = GHEAP_AUTO_NEW(this) GFxFontHandle(searchInfo? NULL : this, pfoundFont, pfontName);
        if (!searchInfo)
        {
            GASSERT(CreatedFonts.Get(phandle) == 0);
            CreatedFonts.Add(phandle);
        }
    }
    if(!phandle)
        AddSearchInfo(searchInfo, "Font not found.");
    return phandle;
}

#ifndef GFC_NO_IME_SUPPORT
void GFxFontManager::SetIMECandidateFont(GFxFontHandle* pfont)
{
    pIMECandidateFont = pfont;
}
#endif //#ifndef GFC_NO_IME_SUPPORT

/*
GFxFontHandle* GFxFontManager::CreateAnyFontHandle(const char* pfontName, FontSearchPathInfo* searchInfo) 
{
    GFxFontHandle* pfontHandle = CreateFontHandle(pfontName, 0, searchInfo);
    if (!pfontHandle)
    {
        pEmptyFont->AddRef();
        AddSearchInfo(searchInfo, "Returning \"EmptyFont\".");
        return pEmptyFont;
    }
    return pfontHandle;    
}
*/

GFxFontHandle* GFxFontManager::GetEmptyFont()
{
    pEmptyFont->AddRef();
    return pEmptyFont;
}

void GFxFontManager::CleanCache()
{
    for(FontSet::Iterator it = CreatedFonts.Begin(); it != CreatedFonts.End(); ++it)
    {
        GFxFontHandle* phandle = it->pNode;
        phandle->pFontManager = NULL;
    }
    CreatedFonts.Clear();
}
