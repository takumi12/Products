/**********************************************************************

Filename    :   GFxTextCore.cpp
Content     :   Core text definitions
Created     :   April 29, 2008
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#include "Text/GFxTextCore.h"
#include "GStd.h"

UPInt GFxTextHTMLImageTagDesc::GetHash() const
{
    UPInt v[] = { 0, 0, 0, 0};
    v[0] = VSpace;
    v[1] = HSpace;
    v[2] = ParaId;
    v[3] = Alignment;
    UPInt hash = GFixedSizeHash<int>::SDBM_Hash(&v, sizeof(v));
    hash ^= GString::BernsteinHashFunctionCIS(Url, Url.GetSize());  
    hash ^= GString::BernsteinHashFunctionCIS(Id.ToCStr(), Id.GetSize());
    return hash;
}

//////////////////////////////////
// GFxTextFormat
//
void GFxTextFormat::SetFontList(const GString& fontList)
{
    if (IsFontHandleSet())
    {
        // if font handle is set and name being set is different - reset the handle
        if (FontList.GetLength() != fontList.GetLength() || 
            GString::CompareNoCase(FontList.ToCStr(), fontList.ToCStr()) != 0)
        {
            ClearFontHandle();
        }
    }
    FontList = fontList;
    PresentMask |= PresentMask_FontList; 

    if (!G_strchr(FontList, ','))
        PresentMask |= PresentMask_SingleFontName; 
    else
        PresentMask &= ~PresentMask_SingleFontName; 
}

void GFxTextFormat::SetFontList(const char* pfontList, UPInt  fontListSz)
{
    // CS5 may generate empty font names.
    GASSERT(pfontList);
    GFC_DEBUG_WARNING(fontListSz == 0, "Empty font name is set in GFxTextFormat::SetFontList\n");

    if (fontListSz == GFC_MAX_UPINT)
        fontListSz = G_strlen(pfontList);

    if (IsFontHandleSet())
    {
        // if font handle is set and name being set is different - reset the handle
        if (FontList.GetLength() != fontListSz || 
            GString::CompareNoCase(FontList.ToCStr(), pfontList, fontListSz) != 0)
        {
            ClearFontHandle();
        }
    }
    FontList.Clear();
    FontList.AppendString(pfontList, fontListSz);
    PresentMask |= PresentMask_FontList; 

    if (!G_strchr(FontList, ','))
        PresentMask |= PresentMask_SingleFontName; 
    else
        PresentMask &= ~PresentMask_SingleFontName; 
}

void GFxTextFormat::SetFontList(const wchar_t* pfontList, UPInt fontListSz)
{
    // CS5 may generate empty font names.
    GASSERT(pfontList);
    GFC_DEBUG_WARNING(fontListSz == 0, "Empty font name is set in GFxTextFormat::SetFontList\n");

    if (fontListSz == GFC_MAX_UPINT)
        fontListSz = G_wcslen(pfontList);

    if (IsFontHandleSet())
    {
        // if font handle is set and name being set is different - reset the handle
        if (FontList.GetLength() == fontListSz)
        {
            for (UPInt i = 0; i < fontListSz; ++i)
            {
                if (G_towlower(FontList[i]) != G_towlower(pfontList[i]))
                {
                    ClearFontHandle();
                    break;
                }
            }
        }
        else
            ClearFontHandle();
    }
    FontList.Clear();
    FontList.AppendString(pfontList, fontListSz);
    PresentMask |= PresentMask_FontList; 
    
    if (!G_strchr(FontList, ','))
        PresentMask |= PresentMask_SingleFontName; 
    else
        PresentMask &= ~PresentMask_SingleFontName; 
}

const GString& GFxTextFormat::GetFontList() const    
{
    static const GString emptyStr;
    return (IsFontListSet()) ? (const GString&)FontList : emptyStr; 
}

void GFxTextFormat::SetFontName(const GString& fontName)
{
#ifdef GFC_BUILD_DEBUG
    for (UPInt i = 0; i < fontName.GetSize(); i++)
        GASSERT(fontName[i] != ',');
#endif
    SetFontList(fontName);
}

void GFxTextFormat::SetFontName(const char* pfontName, UPInt  fontNameSz)
{
    // CS5 may generate empty font names.
    GASSERT(pfontName);
    GFC_DEBUG_WARNING(fontNameSz == 0, "Empty font name is set in GFxTextFormat::SetFontName\n");

    if (fontNameSz == GFC_MAX_UPINT)
        fontNameSz = G_strlen(pfontName);
#ifdef GFC_BUILD_DEBUG
    for (UPInt i = 0; i < fontNameSz; i++)
        GASSERT(pfontName[i] != ',');
#endif
    SetFontList(pfontName, fontNameSz);
}

void GFxTextFormat::SetFontName(const wchar_t* pfontName, UPInt  fontNameSz)
{
    // CS5 may generate empty font names.
    GASSERT(pfontName);
    GFC_DEBUG_WARNING(fontNameSz == 0, "Empty font name is set in GFxTextFormat::SetFontName\n");

    if (fontNameSz == GFC_MAX_UPINT)
        fontNameSz = G_wcslen(pfontName);
#ifdef GFC_BUILD_DEBUG
    for (UPInt i = 0; i < fontNameSz; i++)
        GASSERT(pfontName[i] != ',');
#endif
    SetFontList(pfontName, fontNameSz);
}

void GFxTextFormat::SetFontHandle(GFxFontHandle* pfontHandle) 
{ 
    pFontHandle = pfontHandle;
    PresentMask |= PresentMask_FontHandle; 
}

void GFxTextFormat::SetUrl(const char* purl, UPInt urlSz)
{
    GASSERT(purl && urlSz > 0);
    if (urlSz == GFC_MAX_UPINT)
        urlSz = G_strlen(purl);
    Url.Clear();
    Url.AppendString(purl, urlSz);
    PresentMask |= PresentMask_Url; 
}

void GFxTextFormat::SetUrl(const wchar_t* purl, UPInt urlSz)
{
    GASSERT(purl && urlSz > 0);
    if (urlSz == GFC_MAX_UPINT)
        urlSz = G_wcslen(purl);
    Url.Clear();
    Url.AppendString(purl, urlSz);
    PresentMask |= PresentMask_Url; 
}

void GFxTextFormat::SetUrl(const GString& url)
{
    Url = url;
    PresentMask |= PresentMask_Url; 
}


void GFxTextFormat::SetBold(bool bold)
{
    if (IsFontHandleSet())
    {
        // if font handle is set and boldness being set is different - reset the handle
        if (IsBold() != bold)
            ClearFontHandle();
    }
    if (bold) 
    {
        FormatFlags |= Format_Bold; 
    }
    else
    {
        FormatFlags &= ~Format_Bold; 
    }
    PresentMask |= PresentMask_Bold; 
}

void GFxTextFormat::SetItalic(bool italic)
{
    if (IsFontHandleSet())
    {
        // if font handle is set and italicness being set is different - reset the handle
        if (IsItalic() != italic)
            ClearFontHandle();
    }
    if (italic) 
    {
        FormatFlags |= Format_Italic; 
    }
    else
    {
        FormatFlags &= ~Format_Italic; 
    }
    PresentMask |= PresentMask_Italic; 
}

void GFxTextFormat::SetUnderline(bool underline)
{
    if (underline) 
    {
        FormatFlags |= Format_Underline; 
    }
    else
    {
        FormatFlags &= ~Format_Underline; 
    }
    PresentMask |= PresentMask_Underline; 
}

void GFxTextFormat::SetKerning(bool kerning)
{
    if (kerning) 
    {
        FormatFlags |= Format_Kerning; 
    }
    else
    {
        FormatFlags &= ~Format_Kerning; 
    }
    PresentMask |= PresentMask_Kerning; 
}

bool GFxTextFormat::IsFontSame(const GFxTextFormat& fmt) const 
{ 
    return (((IsFontListSet() && fmt.IsFontListSet() && FontList.CompareNoCase(fmt.FontList) == 0) ||
        (IsFontHandleSet() && fmt.IsFontHandleSet() && pFontHandle == fmt.pFontHandle)) &&
        IsBold() == fmt.IsBold() && IsItalic() == fmt.IsItalic()); 
}

bool GFxTextFormat::IsHTMLFontTagSame(const GFxTextFormat& fmt) const 
{ 
    return (((IsFontListSet() && fmt.IsFontListSet() && FontList.CompareNoCase(fmt.FontList) == 0) ||
        (IsFontHandleSet() && fmt.IsFontHandleSet() && pFontHandle == fmt.pFontHandle)) && 
        (GetColor32() & 0xFFFFFFu) == (fmt.GetColor32() & 0xFFFFFFu) && GetAlpha() == fmt.GetAlpha() &&
        GetFontSize() == fmt.GetFontSize() && 
        IsKerning() == fmt.IsKerning() && GetLetterSpacing() == fmt.GetLetterSpacing()); 
}

GFxTextFormat GFxTextFormat::Merge(const GFxTextFormat& fmt) const
{
    GFxTextFormat result(*this);
    if (fmt.IsBoldSet())
        result.SetBold(fmt.IsBold());
    if (fmt.IsItalicSet())
        result.SetItalic(fmt.IsItalic());
    if (fmt.IsUnderlineSet())
        result.SetUnderline(fmt.IsUnderline());
    if (fmt.IsKerningSet())
        result.SetKerning(fmt.IsKerning());
    if (fmt.IsColorSet())
        result.SetColor(fmt.GetColor());
    if (fmt.IsAlphaSet())
        result.SetAlpha(fmt.GetAlpha());
    if (fmt.IsLetterSpacingSet())
        result.SetLetterSpacingInTwips(fmt.GetLetterSpacingInTwips()); // avoid extra conversions
    if (fmt.IsFontSizeSet())
        result.SetFontSizeInTwips(fmt.GetFontSizeInTwips());
    if (fmt.IsFontListSet())
        result.SetFontList(fmt.GetFontList());
    if (fmt.IsFontHandleSet())
        result.SetFontHandle(fmt.GetFontHandle());
    if (fmt.IsUrlCleared())
        result.ClearUrl();
    else if (fmt.IsUrlSet())
        result.SetUrl(fmt.GetUrl());
    if (fmt.IsImageDescSet())
        result.SetImageDesc(fmt.GetImageDesc());
    return result;
}

GFxTextFormat GFxTextFormat::Intersection(const GFxTextFormat& fmt) const
{
    GFxTextFormat result(fmt.FontList.GetHeap());
    if (IsBoldSet() && fmt.IsBoldSet() && IsBold() == fmt.IsBold())
        result.SetBold(fmt.IsBold());
    if (IsItalicSet() && fmt.IsItalicSet() && IsItalic() == fmt.IsItalic())
        result.SetItalic(fmt.IsItalic());
    if (IsUnderlineSet() && fmt.IsUnderlineSet() && IsUnderline() == fmt.IsUnderline())
        result.SetUnderline(fmt.IsUnderline());
    if (IsKerningSet() && fmt.IsKerningSet() && IsKerning() == fmt.IsKerning())
        result.SetKerning(fmt.IsKerning());
    if (IsColorSet() && fmt.IsColorSet() && GetColor() == fmt.GetColor())
        result.SetColor(fmt.GetColor());
    if (IsAlphaSet() && fmt.IsAlphaSet() && GetAlpha() == fmt.GetAlpha())
        result.SetAlpha(fmt.GetAlpha());
    if (IsLetterSpacingSet() && fmt.IsLetterSpacingSet() && GetLetterSpacing() == fmt.GetLetterSpacing())
        result.SetLetterSpacingInTwips(fmt.GetLetterSpacingInTwips()); // avoid extra conversions
    if (IsFontSizeSet() && fmt.IsFontSizeSet() && GetFontSizeInTwips() == fmt.GetFontSizeInTwips())
        result.SetFontSizeInTwips(fmt.GetFontSizeInTwips());
    if (IsFontListSet() && fmt.IsFontListSet() && FontList.CompareNoCase(fmt.FontList) == 0)
        result.SetFontList(fmt.GetFontList());
    if (IsFontHandleSet() && fmt.IsFontHandleSet() && GetFontHandle() == fmt.GetFontHandle())
        result.SetFontHandle(fmt.GetFontHandle());
    if (IsUrlSet() && fmt.IsUrlSet() && Url.CompareNoCase(fmt.Url) == 0)
        result.SetUrl(fmt.GetUrl());
    if (IsImageDescSet() && fmt.IsImageDescSet() && GetImageDesc() == fmt.GetImageDesc())
        result.SetImageDesc(fmt.GetImageDesc());
    return result;
}

void GFxTextFormat::InitByDefaultValues()
{
    SetColor32(0);
    SetFontList("Times New Roman");
    SetFontSize(12);
    SetBold(false);
    SetItalic(false);
    SetUnderline(false);
    SetKerning(false);
    ClearAlpha();
    ClearLetterSpacing();
    ClearUrl();

}

UPInt GFxTextFormat::HashFunctor::operator()(const GFxTextFormat& data) const
{
    UPInt v[] = { 0, 0, 0, 0};
    if (data.IsColorSet() || data.IsAlphaSet()) v[0] |= data.Color;
    if (data.IsLetterSpacingSet())  v[1] |= data.LetterSpacing;
    if (data.IsFontSizeSet())       v[1] |= ((UPInt)data.FontSize << 16);
    v[0] |= UPInt(data.FormatFlags) << 24;
    v[1] |= UPInt(data.PresentMask) << 24;
    //!AB: do not use font handle ptr for hash code since it might be different
    // for the same font. GFxTextFormat::operator== compares internals of font handle.
    // We will just set a bit: 0 - if font handle is set, 1 - if not.
    //if (data.IsFontHandleSet())
    //    v[2] = (UPInt)data.pFontHandle.GetPtr();
    if (data.pFontHandle.GetPtr())
        v[2] |= 1;
    if (data.IsImageDescSet() && data.pImageDesc)
        v[3] = (UPInt)data.pImageDesc->GetHash();
    UPInt hash = GFixedSizeHash<int>::SDBM_Hash(&v, sizeof(v));
    if (data.IsFontListSet())
    {
        hash ^= GString::BernsteinHashFunctionCIS(data.FontList, data.FontList.GetSize());  
    }
    if (data.IsUrlSet())
    {
        hash ^= GString::BernsteinHashFunctionCIS(data.Url.ToCStr(), data.Url.GetSize());
    }
    return hash;
}

//////////////////////////////////
// GFxTextParagraphFormat
//
GFxTextParagraphFormat GFxTextParagraphFormat::Merge(const GFxTextParagraphFormat& fmt) const
{
    GFxTextParagraphFormat result(*this);
    if (fmt.IsAlignmentSet())
        result.SetAlignment(fmt.GetAlignment());
    if (fmt.IsBulletSet())
        result.SetBullet(fmt.IsBullet());
    if (fmt.IsBlockIndentSet())
        result.SetBlockIndent(fmt.GetBlockIndent());
    if (fmt.IsIndentSet())
        result.SetIndent(fmt.GetIndent());
    if (fmt.IsLeadingSet())
        result.SetLeading(fmt.GetLeading());
    if (fmt.IsLeftMarginSet())
        result.SetLeftMargin(fmt.GetLeftMargin());
    if (fmt.IsRightMarginSet())
        result.SetRightMargin(fmt.GetRightMargin());
    if (fmt.IsTabStopsSet())
        result.SetTabStops(fmt.GetTabStops());
    if (fmt.IsDisplaySet())
        result.SetDisplay(fmt.GetDisplay());
    return result;
}

GFxTextParagraphFormat GFxTextParagraphFormat::Intersection(const GFxTextParagraphFormat& fmt) const
{
    GFxTextParagraphFormat result;
    if (IsAlignmentSet() && fmt.IsAlignmentSet() && GetAlignment() == fmt.GetAlignment())
        result.SetAlignment(fmt.GetAlignment());
    if (IsBulletSet() && fmt.IsBulletSet() && IsBullet() == fmt.IsBullet())
        result.SetBullet(fmt.IsBullet());
    if (IsBlockIndentSet() && fmt.IsBlockIndentSet() && GetBlockIndent() == fmt.GetBlockIndent())
        result.SetBlockIndent(fmt.GetBlockIndent());
    if (IsIndentSet() && fmt.IsIndentSet() && GetIndent() == fmt.GetIndent())
        result.SetIndent(fmt.GetIndent());
    if (IsLeadingSet() && fmt.IsLeadingSet() && GetLeading() == fmt.GetLeading())
        result.SetLeading(fmt.GetLeading());
    if (IsLeftMarginSet() && fmt.IsLeftMarginSet() && GetLeftMargin() == fmt.GetLeftMargin())
        result.SetLeftMargin(fmt.GetLeftMargin());
    if (IsRightMarginSet() && fmt.IsRightMarginSet() && GetRightMargin() == fmt.GetRightMargin())
        result.SetRightMargin(fmt.GetRightMargin());
    if (IsTabStopsSet() && fmt.IsTabStopsSet() && TabStopsEqual(fmt.GetTabStops()))
        result.SetTabStops(fmt.GetTabStops());
    if (IsDisplaySet() && fmt.IsDisplaySet() && GetDisplay() == fmt.GetDisplay())
        result.SetDisplay(fmt.GetDisplay());
    return result;
}

void GFxTextParagraphFormat::InitByDefaultValues()
{
    SetAlignment(Align_Left);
    ClearBlockIndent();
    ClearBullet();
    ClearIndent();
    ClearLeading();
    ClearLeftMargin();
    ClearRightMargin();
    ClearTabStops();
}

void GFxTextParagraphFormat::SetTabStops(UInt num, ...)
{
    if (num > 0)
    {
        if (!pTabStops || pTabStops[0] != num)
        {
            FreeTabStops();
            AllocTabStops(num);
        }
        va_list vl;
        va_start(vl, num);

        for (UInt i = 0; i < num; ++i)
        {
            UInt arg = va_arg(vl, UInt);
            pTabStops[i + 1] = arg;
        }
        va_end(vl);
        PresentMask |= PresentMask_TabStops;
    }
    else
        ClearTabStops();
}

void  GFxTextParagraphFormat::SetTabStops(const UInt* psrcTabStops)
{
    if (psrcTabStops && psrcTabStops[0] > 0)
    {
        CopyTabStops(psrcTabStops);
        PresentMask |= PresentMask_TabStops;
    }
    else
        ClearTabStops();
}

const UInt* GFxTextParagraphFormat::GetTabStops(UInt* pnum) const
{
    if (pTabStops)
    {
        UInt cnt = pTabStops[0];
        if (pnum)
            *pnum = cnt;
        return pTabStops + 1;
    }
    return NULL;
}

void GFxTextParagraphFormat::SetTabStopsElement(UInt idx, UInt val)
{
    if (pTabStops && idx < pTabStops[0])
    {
        pTabStops[idx + 1] = val;
    }
}

void GFxTextParagraphFormat::AllocTabStops(UInt num)
{
    FreeTabStops();
    pTabStops = (UInt*)GALLOC((num + 1) * sizeof(UInt), GFxStatMV_Text_Mem);
    pTabStops[0] = num;
}

void GFxTextParagraphFormat::FreeTabStops()
{
    GFREE(pTabStops);
    pTabStops = NULL;
}

bool GFxTextParagraphFormat::TabStopsEqual(const UInt* psrcTabStops) const
{
    if (pTabStops == psrcTabStops)
        return true;
    if (pTabStops && psrcTabStops)
    {
        UInt c1 = pTabStops[0];
        UInt c2 = psrcTabStops[0];
        if (c1 == c2)
        {
            return (memcmp(pTabStops + 1, psrcTabStops + 1, sizeof(UInt) * c1) == 0);
        }
    }
    return false;
}

void GFxTextParagraphFormat::CopyTabStops(const UInt* psrcTabStops)
{
    if (psrcTabStops)
    {
        UInt n = psrcTabStops[0];
        if (!pTabStops || pTabStops[0] != n)
        {
            AllocTabStops(n);
        }
        memcpy(pTabStops + 1, psrcTabStops + 1, n * sizeof(UInt));
    }
    else
        FreeTabStops();
}

GFxTextParagraphFormat& GFxTextParagraphFormat::operator=(const GFxTextParagraphFormat& src)
{
    BlockIndent     = src.BlockIndent;
    Indent          = src.Indent;
    Leading         = src.Leading;
    LeftMargin      = src.LeftMargin;
    RightMargin     = src.RightMargin;
    PresentMask     = src.PresentMask;
    CopyTabStops(src.pTabStops);
    return *this;
}

UPInt  GFxTextParagraphFormat::HashFunctor::operator()(const GFxTextParagraphFormat& data) const
{
    UPInt hash = 0;
    if (data.IsTabStopsSet() && data.pTabStops)
        hash ^= GFixedSizeHash<UInt>::SDBM_Hash(data.pTabStops, (data.pTabStops[0] + 1) * sizeof(UInt));
    if (data.IsBlockIndentSet())
        hash ^= data.GetBlockIndent();
    if (data.IsIndentSet())
        hash ^= ((UPInt)data.GetIndent()) << 8;
    if (data.IsLeadingSet())
        hash ^= ((UPInt)data.GetLeading()) << 12;
    if (data.IsLeftMarginSet())
        hash ^= ((UPInt)data.GetLeftMargin()) << 16;
    if (data.IsRightMarginSet())
        hash ^= ((UPInt)data.GetRightMargin()) << 18;
    hash ^= ((((UPInt)data.PresentMask) << 9) | (((UPInt)data.GetAlignment()) << 1) | (UPInt)data.IsBullet());
    hash ^= (((UPInt)data.GetDisplay()) << 10);
    return hash;
}

//////////////////////////////////////////////////////////////////////////
// GFxTextAllocator
GFxTextFormat* GFxTextAllocator::AllocateTextFormat(const GFxTextFormat& srcfmt)
{
    if (!srcfmt.IsImageDescSet())
    {
        GFxTextFormatPtr* ppfmt = TextFormatStorage.Get(&srcfmt);
        if (ppfmt)
        {
            ppfmt->GetPtr()->AddRef();
            return ppfmt->GetPtr();
        }
        // check if we need to flush format cache
        if (TextFormatStorage.GetSize() >= TextFormatStorageCap)
            FlushTextFormatCache();
    }

    GFxTextFormat* pfmt = GHEAP_NEW(pHeap) GFxTextFormat(srcfmt, pHeap);
    if (Flags & Flags_Global && pfmt->IsFontHandleSet())
    {
        // do not save font handles, since they might be destroyed eventually.
        pfmt->ClearFontHandle();
    }
    if (!srcfmt.IsImageDescSet())
        TextFormatStorage.Set(pfmt);
    return pfmt;
}

GFxTextParagraphFormat* GFxTextAllocator::AllocateParagraphFormat(const GFxTextParagraphFormat& srcfmt)
{
    GFxTextParagraphFormatPtr* ppfmt = ParagraphFormatStorage.Get(&srcfmt);
    if (ppfmt)
    {
        ppfmt->GetPtr()->AddRef();
        return ppfmt->GetPtr();
    }
    // check if we need to flush format cache
    if (ParagraphFormatStorage.GetSize() >= ParagraphFormatStorageCap)
        FlushParagraphFormatCache();

    GFxTextParagraphFormat* pfmt = GHEAP_NEW(pHeap) GFxTextParagraphFormat(srcfmt);
    ParagraphFormatStorage.Set(pfmt);
    return pfmt;
}

bool GFxTextAllocator::FlushTextFormatCache(bool noAllocationsAllowed)
{
    UPInt tfSize = TextFormatStorage.GetSize();
    TextFormatStorageType::Iterator it = TextFormatStorage.Begin();
    for (;it != TextFormatStorage.End(); ++it)
    {
        GFxTextFormatPtr& fmt = *it;
        if (fmt.GetPtr()->GetRefCount() == 1)
        {
            // we can delete this format, since it is not used.
            //fmt.pFormat = NULL;
            it.Remove();
        }
    }
    // SetCapacity may allocate new memory; if no new allocations are allowed
    // (for example, when Flush... is called from OnExceedLimit handler) then
    // don't set new capacity.
    if (!noAllocationsAllowed)
        TextFormatStorage.SetCapacity(TextFormatStorage.GetSize());
    
    // correct cache cap, if necessary
    if (TextFormatStorage.GetSize() >= TextFormatStorageCap)
        TextFormatStorageCap = UInt(TextFormatStorage.GetSize() + FormatCacheCapacityDelta);
    else if (TextFormatStorage.GetSize() <= InitialFormatCacheCap)
        TextFormatStorageCap = InitialFormatCacheCap;

    return (tfSize != TextFormatStorage.GetSize());
}

bool GFxTextAllocator::FlushParagraphFormatCache(bool noAllocationsAllowed)
{
    UPInt pfSize = ParagraphFormatStorage.GetSize();
    ParagraphFormatStorageType::Iterator it = ParagraphFormatStorage.Begin();
    for (;it != ParagraphFormatStorage.End(); ++it)
    {
        GFxTextParagraphFormatPtr& fmt = *it;
        if (fmt.GetPtr()->GetRefCount() == 1)
        {
            // we can delete this format, since it is not used.
            //fmt.pFormat = NULL;
            it.Remove();
        }
    }
    // SetCapacity may allocate new memory; if no new allocations are allowed
    // (for example, when Flush... is called from OnExceedLimit handler) then
    // don't set new capacity.
    if (!noAllocationsAllowed)
        ParagraphFormatStorage.SetCapacity(ParagraphFormatStorage.GetSize());

    // correct cache cap, if necessary
    if (ParagraphFormatStorage.GetSize() >= ParagraphFormatStorageCap)
        ParagraphFormatStorageCap = UInt(ParagraphFormatStorage.GetSize() + FormatCacheCapacityDelta);
    else if (ParagraphFormatStorage.GetSize() <= InitialFormatCacheCap)
        ParagraphFormatStorageCap = InitialFormatCacheCap;

    return (pfSize != ParagraphFormatStorage.GetSize());
}
