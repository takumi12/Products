/**********************************************************************

Filename    :   GFxTextCore.h
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

#ifndef INC_TEXT_GFXTEXTCORE_H
#define INC_TEXT_GFXTEXTCORE_H

#include "GUTF8Util.h"
#include "GArray.h"
#include "GRange.h"
#include "Text/GFxTextLineBuffer.h"

#define MAX_FONT_NAME_LEN           50
#define GFX_ACTIVE_SEL_BKCOLOR      0xFF000000u
#define GFX_INACTIVE_SEL_BKCOLOR    0xFF808080u
#define GFX_ACTIVE_SEL_TEXTCOLOR    0xFFFFFFFFu
#define GFX_INACTIVE_SEL_TEXTCOLOR  0xFFFFFFFFu

class GFxTextParagraph;

template <typename T>
SInt RoundTwips(T v)
{
    return G_IRound(v);
}

template <typename T>
SInt AlignTwipsToPixel(T v)
{
    return G_IRound(v);
}

#ifdef GFC_BUILD_DEBUG
template <class T>
void _dump(GRangeDataArray<T>& ranges)
{
    printf("-----\n");
    typename GRangeDataArray<T>::Iterator it = ranges.Begin();
    for(; !it.IsFinished(); ++it)
    {
        GRangeData<T>& r = *it;
        printf ("i = %d, ei = %d\n", r.Index, r.NextIndex());
    }
    printf("-----\n");
}
#endif

// A descriptor that is used by the GFxTextFormat for holding
// images (<IMG> tag).
struct GFxTextHTMLImageTagDesc : public GFxTextImageDesc
{
    GStringLH    Url;
    GStringLH    Id;
    SInt         VSpace, HSpace; // in twips
    UInt         ParaId;
    enum
    {
        Align_BaseLine,
        Align_Right,
        Align_Left
    };
    UByte     Alignment;

    GFxTextHTMLImageTagDesc() : VSpace(0), HSpace(0), 
        ParaId(~0u), Alignment(Align_BaseLine) {}
    
    UPInt GetHash() const;

    bool operator == (const GFxTextHTMLImageTagDesc& f) const
    {
        return Url == f.Url && Id == f.Id && VSpace == f.VSpace && 
               HSpace == f.HSpace && ParaId == f.ParaId && Alignment == f.Alignment;
    }
};

// Format descriptor, applying to rich text
// GFxTextFormat ref counting is Thread-Safe on purpose
class GFxTextFormat : public GNewOverrideBase<GFxStatMV_Text_Mem>
{
    // Ranges:
    // fontSize         [0..2500] pts
    // letterSpacing    [-60..60] pts
public:
    friend class HashFunctor;
    struct HashFunctor
    {
        UPInt  operator()(const GFxTextFormat& data) const;
    };
private:
    mutable unsigned                RefCount;
protected:
    GStringDH                       FontList; // comma separated list of font names
    GStringDH                       Url;
    GPtr<GFxTextHTMLImageTagDesc>   pImageDesc;
    GPtr<GFxFontHandle>             pFontHandle; // required if font was referenced by id

    UInt32      Color; // format AA RR GG BB.
    SInt16      LetterSpacing; // in twips!
    UInt16      FontSize;      // in twips!

    enum
    {
        Format_Bold =       0x1,
        Format_Italic =     0x2,
        Format_Underline =  0x4,
        Format_Kerning =    0x8
    };
    UInt8       FormatFlags;

    enum
    {
        PresentMask_Color         = 1,
        PresentMask_LetterSpacing = 2,
        PresentMask_FontList      = 4,
        PresentMask_FontSize      = 8,
        PresentMask_Bold          = 0x10,
        PresentMask_Italic        = 0x20,
        PresentMask_Underline     = 0x40,
        PresentMask_Kerning       = 0x80,
        PresentMask_Url           = 0x100,
        PresentMask_ImageDesc     = 0x200,
        PresentMask_Alpha         = 0x400,
        PresentMask_FontHandle    = 0x800,
        PresentMask_SingleFontName= 0x1000 // indicating that the single font name is stored in FontList
    };
    UInt16       PresentMask;

    //    void InvalidateCachedFont() { pCachedFont = 0; }
public:

    GFxTextFormat(GMemoryHeap *pheap)
        : RefCount(1), FontList(pheap), Url(pheap),
          Color(0xFF000000u), LetterSpacing(0), FontSize(0), FormatFlags(0), PresentMask(0) 
    { 
    }
    GFxTextFormat(const GFxTextFormat& srcfmt, GMemoryHeap* pheap = NULL)
        : RefCount(1), FontList((pheap) ? pheap:srcfmt.GetHeap(), srcfmt.FontList),
          Url((pheap) ? pheap:srcfmt.GetHeap(), srcfmt.Url),
          pImageDesc(srcfmt.pImageDesc), pFontHandle(srcfmt.pFontHandle),
          Color(srcfmt.Color), LetterSpacing(srcfmt.LetterSpacing),
          FontSize(srcfmt.FontSize), FormatFlags(srcfmt.FormatFlags), PresentMask(srcfmt.PresentMask)
    {
    }
    ~GFxTextFormat() 
    {
        //GASSERT(!pFontHandle || !pFontHandle->GetFont() || pFontHandle->GetFont()->GetFont());
    }
    void AddRef() const  { ++RefCount; }
    void Release() const { if (--RefCount == 0) delete this; }
    unsigned GetRefCount() const { return RefCount; }

    GMemoryHeap* GetHeap() const { return FontList.GetHeap(); }

    void SetBold(bool bold = true);
    void ClearBold()    { FormatFlags &= ~Format_Bold; PresentMask &= ~PresentMask_Bold; }
    bool IsBold() const { return (FormatFlags & Format_Bold) != 0; }

    void SetItalic(bool italic = true);
    void ClearItalic()    { FormatFlags &= ~Format_Italic; PresentMask &= ~PresentMask_Italic; }
    bool IsItalic() const { return (FormatFlags & Format_Italic) != 0; }

    void SetUnderline(bool underline = true);
    void ClearUnderline()    { FormatFlags &= ~Format_Underline; PresentMask &= ~PresentMask_Underline; }
    bool IsUnderline() const { return (FormatFlags & Format_Underline) != 0; }

    void SetKerning(bool kerning = true);
    void ClearKerning()    { FormatFlags &= ~Format_Kerning; PresentMask &= ~PresentMask_Kerning; }
    bool IsKerning() const { return (FormatFlags & Format_Kerning) != 0; }

    // This method sets whole color, including the alpha
    void   SetColor(const GColor& color) { Color = color.ToColor32(); PresentMask |= PresentMask_Color; }
    GColor GetColor() const              { return GColor(Color); }
    // This method sets only RGB values of color
    void SetColor32(UInt32 color) { Color = (color & 0xFFFFFF) | (Color & 0xFF000000u); PresentMask |= PresentMask_Color; }
    UInt32 GetColor32() const     { return Color; }
    void ClearColor()             { Color = 0xFF000000u; PresentMask &= ~PresentMask_Color; }

    void SetAlpha(UInt8 alpha) { Color = (Color & 0xFFFFFFu) | (UInt32(alpha)<<24); PresentMask |= PresentMask_Alpha; }
    UInt8 GetAlpha() const  { return UInt8((Color >> 24) & 0xFF); }
    void ClearAlpha()       { Color |= 0xFF000000u; PresentMask &= ~PresentMask_Alpha; }

    // letterSpacing is specified in pixels, e.g.: 2.6 pixels, but stored in twips
    void SetLetterSpacing(Float letterSpacing) 
    {
        GASSERT(PixelsToTwips(letterSpacing) >= -32768 && PixelsToTwips(letterSpacing) < 32768); 
        LetterSpacing = (SInt16)PixelsToTwips(letterSpacing); PresentMask |= PresentMask_LetterSpacing; 
    }
    Float GetLetterSpacing() const             { return TwipsToPixels(Float(LetterSpacing)); }
    void SetLetterSpacingInTwips(SInt letterSpacing) 
    { 
        GASSERT(letterSpacing >= -32768 && letterSpacing < 32768); 
        LetterSpacing = (SInt16)letterSpacing; PresentMask |= PresentMask_LetterSpacing; 
    }
    SInt16 GetLetterSpacingInTwips() const     { return LetterSpacing; }
    void ClearLetterSpacing()                  { LetterSpacing = 0; PresentMask &= ~PresentMask_LetterSpacing; }

    void SetFontSizeInTwips(UInt fontSize) 
    { 
        FontSize = (fontSize <= 65536) ? (UInt16)fontSize : (UInt16)65535u; 
        PresentMask |= PresentMask_FontSize; 
    }
    UInt GetFontSizeInTwips() const { return FontSize; }
    void SetFontSize(Float fontSize) 
    { 
        FontSize = (fontSize < TwipsToPixels(65536.0)) ? (UInt16)PixelsToTwips(fontSize) : (UInt16)65535u; 
        PresentMask |= PresentMask_FontSize; 
    }
    Float GetFontSize(Float scaleFactor) const { return TwipsToPixels((Float)FontSize)*scaleFactor; }
    Float GetFontSize() const       { return TwipsToPixels((Float)FontSize); }
    void ClearFontSize()            { FontSize = 0; PresentMask &= ~PresentMask_FontSize; }

    void SetFontName(const GString& fontName);
    void SetFontName(const char* pfontName, UPInt  fontNameSz = GFC_MAX_UPINT);
    void SetFontName(const wchar_t* pfontName, UPInt fontNameSz = GFC_MAX_UPINT);
    void SetFontList(const GString& fontList);
    void SetFontList(const char* pfontList, UPInt  fontListSz = GFC_MAX_UPINT);
    void SetFontList(const wchar_t* pfontList, UPInt  fontListSz = GFC_MAX_UPINT);
    const GString& GetFontList() const;
    void ClearFontList() { PresentMask &= ~(PresentMask_FontList|PresentMask_SingleFontName); }

    void SetFontHandle(GFxFontHandle* pfontHandle); 
    GFxFontHandle* GetFontHandle() const { return (IsFontHandleSet()) ? pFontHandle : NULL; }
    void ClearFontHandle() { pFontHandle = NULL; PresentMask &= ~PresentMask_FontHandle; }

    void SetImageDesc(GFxTextHTMLImageTagDesc* pimage) { pImageDesc = pimage; PresentMask |= PresentMask_ImageDesc;}
    GFxTextHTMLImageTagDesc* GetImageDesc() const { return (IsImageDescSet()) ? pImageDesc : NULL; }
    void ClearImageDesc() { pImageDesc = NULL; PresentMask &= ~PresentMask_ImageDesc; }

    void SetUrl(const char* purl, UPInt  urlSz = GFC_MAX_UPINT);
    void SetUrl(const wchar_t* purl, UPInt  urlSz = GFC_MAX_UPINT);
    void SetUrl(const GString& url);
    const GString& GetUrl() const    { return Url; }
    const char* GetUrlCStr() const     { return (IsUrlSet()) ? Url.ToCStr() : ""; }
    void ClearUrl()                    { Url.Clear(); PresentMask &= ~PresentMask_Url; }

    bool IsBoldSet() const      { return (PresentMask & PresentMask_Bold) != 0; }
    bool IsItalicSet() const    { return (PresentMask & PresentMask_Italic) != 0; }
    bool IsUnderlineSet() const { return (PresentMask & PresentMask_Underline) != 0; }
    bool IsKerningSet() const   { return (PresentMask & PresentMask_Kerning) != 0; }
    bool IsFontSizeSet() const  { return (PresentMask & PresentMask_FontSize) != 0; }
    bool IsFontListSet() const  { return (PresentMask & PresentMask_FontList) != 0; }
    bool IsSingleFontNameSet() const { return (PresentMask & PresentMask_SingleFontName) != 0; }
    bool IsFontHandleSet() const{ return (PresentMask & PresentMask_FontHandle) != 0; }
    bool IsImageDescSet() const { return (PresentMask & PresentMask_ImageDesc) != 0; }
    bool IsColorSet() const     { return (PresentMask & PresentMask_Color) != 0; }
    bool IsAlphaSet() const     { return (PresentMask & PresentMask_Alpha) != 0; }
    bool IsLetterSpacingSet() const { return (PresentMask & PresentMask_LetterSpacing) != 0; }
    bool IsUrlSet() const       { return (PresentMask & PresentMask_Url) != 0 && Url.GetLength() != 0; }
    bool IsUrlCleared() const   { return (PresentMask & PresentMask_Url) != 0 && Url.GetLength() == 0; }

    GFxTextFormat Merge(const GFxTextFormat& fmt) const;
    GFxTextFormat Intersection(const GFxTextFormat& fmt) const;

    bool operator == (const GFxTextFormat& f) const
    {
        return (PresentMask == f.PresentMask && FormatFlags == f.FormatFlags && Color == f.Color && 
            FontSize == f.FontSize && 
            (IsFontListSet() == f.IsFontListSet() && (!IsFontListSet() || FontList.CompareNoCase(f.FontList) == 0)) && 
            LetterSpacing == f.LetterSpacing &&
            (IsFontHandleSet() == f.IsFontHandleSet() && (!IsFontHandleSet() || pFontHandle == f.pFontHandle || 
            (pFontHandle && f.pFontHandle && *pFontHandle == *f.pFontHandle))) &&
            (IsUrlSet() == f.IsUrlSet() && (!IsUrlSet() || Url.CompareNoCase(f.Url) == 0)) && 
            ((pImageDesc && f.pImageDesc && *pImageDesc == *f.pImageDesc) || (pImageDesc == f.pImageDesc)));
    }

    bool IsFontSame(const GFxTextFormat& fmt) const; 
    bool IsHTMLFontTagSame(const GFxTextFormat& fmt) const;
    void InitByDefaultValues();
};

template <class T>
class GFxTextFormatPtrWrapper
{
public:
    GPtr<T> pFormat;

    GFxTextFormatPtrWrapper() {}
    GFxTextFormatPtrWrapper(GPtr<T>& ptr) : pFormat(ptr) {}
    GFxTextFormatPtrWrapper(const GPtr<T>& ptr) : pFormat(ptr) {}
    GFxTextFormatPtrWrapper(T* ptr) : pFormat(ptr) {}
    GFxTextFormatPtrWrapper(const T* ptr) : pFormat(const_cast<T*>(ptr)) {}
    GFxTextFormatPtrWrapper(T& ptr) : pFormat(ptr) {}
    GFxTextFormatPtrWrapper(const GFxTextFormatPtrWrapper& orig) : pFormat(orig.pFormat) {}

    GPtr<T>& operator*() { return pFormat; }
    const GPtr<T>& operator*() const { return pFormat; }

    operator GFxTextFormat*() { return pFormat; }
    operator const GFxTextFormat*() const { return pFormat; }

    T* operator-> () { return pFormat; }
    const T* operator-> () const { return pFormat; }

    T* GetPtr() { return pFormat; }
    const T* GetPtr() const { return pFormat; }

    GFxTextFormatPtrWrapper<T>& operator=(const GFxTextFormatPtrWrapper<T>& p)
    {
        pFormat = (*p).GetPtr();
        return *this;
    }
    bool operator==(const GFxTextFormatPtrWrapper<T>& p) const
    {
        return *pFormat == *p.pFormat;
    }
    bool operator==(const T* p) const
    {
        return *pFormat == *p;
    }
    bool operator==(const GPtr<T> p) const
    {
        return *pFormat == *p;
    }

    // hash functor
    struct HashFunctor
    {
        UPInt  operator()(const GFxTextFormatPtrWrapper<T>& data) const
        {
            typename T::HashFunctor h;
            return h.operator()(*data.pFormat);
        }
    };
};

typedef GFxTextFormatPtrWrapper<GFxTextFormat> GFxTextFormatPtr;

// Format descriptor, applying to whole paragraph
class GFxTextParagraphFormat : public GNewOverrideBase<GFxStatMV_Text_Mem>
{
public:
    friend class HashFunctor;
    struct HashFunctor
    {
        UPInt  operator()(const GFxTextParagraphFormat& data) const;
    };

    // ranges:
    // indent   [-720..720] px
    // leading  [-360..720] px
    // margins  [0..720] px
public:
    enum AlignType
    {
        Align_Left      = 0,
        Align_Right     = 1,
        Align_Justify   = 2,
        Align_Center    = 3
    };
    enum DisplayType
    {
        Display_Inline = 0,
        Display_Block  = 1,
        Display_None   = 2
    };
private:
    mutable unsigned RefCount;
protected:
    UInt*  pTabStops;   // First elem is total num of tabstops, followed by tabstops in twips
    UInt16 BlockIndent;
    SInt16 Indent;
    SInt16 Leading;
    UInt16 LeftMargin;
    UInt16 RightMargin;
    enum
    {
        PresentMask_Alignment     = 0x01,
        PresentMask_BlockIndent   = 0x02,
        PresentMask_Indent        = 0x04,
        PresentMask_Leading       = 0x08,
        PresentMask_LeftMargin    = 0x10,
        PresentMask_RightMargin   = 0x20,
        PresentMask_TabStops      = 0x40,
        PresentMask_Bullet        = 0x80,
        PresentMask_Display       = 0x100,

        Mask_Align      = 0x600, // (0x400 | 0x200)
        Shift_Align     = 9,
        Mask_Display    = 0x1800,// (0x1000 | 0x800)
        Shift_Display   = 11,
        Mask_Bullet     = 0x8000
    };
    UInt16 PresentMask;
public:
    GFxTextParagraphFormat() : RefCount(1), pTabStops(NULL), BlockIndent(0), Indent(0), Leading(0), 
        LeftMargin(0), RightMargin(0), PresentMask(0) 
    {}

    GFxTextParagraphFormat(const GFxTextParagraphFormat& src) : RefCount(1), pTabStops(NULL), 
        BlockIndent(src.BlockIndent), Indent(src.Indent), Leading(src.Leading), 
        LeftMargin(src.LeftMargin), RightMargin(src.RightMargin), 
        PresentMask(src.PresentMask)
    {
        CopyTabStops(src.pTabStops);
    }
    ~GFxTextParagraphFormat()
    {
        FreeTabStops();
    }
    void AddRef() const  { ++RefCount; }
    void Release() const { if (--RefCount == 0) delete this; }
    unsigned GetRefCount() const { return RefCount; }

    void SetAlignment(AlignType align) 
    { 
        PresentMask = (UInt16)((PresentMask & (~Mask_Align)) | ((align << Shift_Align) & Mask_Align)); 
        PresentMask |= PresentMask_Alignment; 
    }
    void ClearAlignment()              { PresentMask &= ~Mask_Align; PresentMask &= ~PresentMask_Alignment; }
    bool IsAlignmentSet() const        { return (PresentMask & PresentMask_Alignment) != 0; }
    bool IsLeftAlignment() const       { return IsAlignmentSet() && GetAlignment() == Align_Left; }
    bool IsRightAlignment() const      { return IsAlignmentSet() && GetAlignment() == Align_Right; }
    bool IsCenterAlignment() const     { return IsAlignmentSet() && GetAlignment() == Align_Center; }
    bool IsJustifyAlignment() const    { return IsAlignmentSet() && GetAlignment() == Align_Justify; }
    AlignType GetAlignment() const     { return (AlignType)((PresentMask & Mask_Align) >> Shift_Align); }

    void SetBullet(bool bullet = true) 
    { 
        if (bullet)
            PresentMask |= Mask_Bullet; 
        else
            PresentMask &= ~Mask_Bullet; 
        PresentMask |= PresentMask_Bullet; 
    }
    void ClearBullet()                 { PresentMask &= ~Mask_Bullet; PresentMask &= ~PresentMask_Bullet; }
    bool IsBulletSet() const           { return (PresentMask & PresentMask_Bullet) != 0; }
    bool IsBullet() const              { return IsBulletSet() && (PresentMask & Mask_Bullet) != 0; }

    void SetBlockIndent(UInt value)    { GASSERT(value < 65536); BlockIndent = (UInt16)value; PresentMask |= PresentMask_BlockIndent; }
    UInt GetBlockIndent() const        { return BlockIndent; }
    void ClearBlockIndent()            { BlockIndent = 0; PresentMask &= ~PresentMask_BlockIndent; }
    bool IsBlockIndentSet() const      { return (PresentMask & PresentMask_BlockIndent) != 0; }

    void SetIndent(SInt value)    { GASSERT(value >= -32768 && value < 32768); Indent = (SInt16)value; PresentMask |= PresentMask_Indent; }
    SInt GetIndent() const        { return Indent; }
    void ClearIndent()            { Indent = 0; PresentMask &= ~PresentMask_Indent; }
    bool IsIndentSet() const      { return (PresentMask & PresentMask_Indent) != 0; }

    void SetLeading(SInt value)    { GASSERT(value >= -32768 && value < 32768); Leading = (SInt16)value; PresentMask |= PresentMask_Leading; }
    SInt GetLeading() const        { return Leading; }
    void ClearLeading()            { Leading = 0; PresentMask &= ~PresentMask_Leading; }
    bool IsLeadingSet() const      { return (PresentMask & PresentMask_Leading) != 0; }

    void SetLeftMargin(UInt value)    { GASSERT(value < 65536); LeftMargin = (UInt16)value; PresentMask |= PresentMask_LeftMargin; }
    UInt GetLeftMargin() const        { return LeftMargin; }
    void ClearLeftMargin()            { LeftMargin = 0; PresentMask &= ~PresentMask_LeftMargin; }
    bool IsLeftMarginSet() const      { return (PresentMask & PresentMask_LeftMargin) != 0; }

    void SetRightMargin(UInt value)    { GASSERT(value < 65536); RightMargin = (UInt16)value; PresentMask |= PresentMask_RightMargin; }
    UInt GetRightMargin() const        { return RightMargin; }
    void ClearRightMargin()            { RightMargin = 0; PresentMask &= ~PresentMask_RightMargin; }
    bool IsRightMarginSet() const      { return (PresentMask & PresentMask_RightMargin) != 0; }

    void SetTabStops(UInt num, ...);
    const UInt* GetTabStops(UInt* pnum) const;
    void  SetTabStops(const UInt* psrcTabStops);
    const UInt* GetTabStops() const { return pTabStops; }
    void SetTabStopsNum(UInt num)   { AllocTabStops(num); PresentMask |= PresentMask_TabStops; }
    void SetTabStopsElement(UInt idx, UInt val);
    void ClearTabStops()            { FreeTabStops(); PresentMask &= ~PresentMask_TabStops; }
    bool IsTabStopsSet() const      { return (PresentMask & PresentMask_TabStops) != 0; }

    void SetDisplay(DisplayType display) 
    { 
        PresentMask = (UInt16)((PresentMask & (~Mask_Display)) | ((display << Shift_Display) & Mask_Display)); 
        PresentMask |= PresentMask_Display; 
    }
    void ClearDisplay()                  { PresentMask &= ~Mask_Display; PresentMask &= ~PresentMask_Display; }
    bool IsDisplaySet() const            { return (PresentMask & PresentMask_Display) != 0; }
    DisplayType GetDisplay() const       { return (DisplayType)((PresentMask & Mask_Display) >> Shift_Display); }

    GFxTextParagraphFormat Merge(const GFxTextParagraphFormat& fmt) const;
    GFxTextParagraphFormat Intersection(const GFxTextParagraphFormat& fmt) const;

    GFxTextParagraphFormat& operator=(const GFxTextParagraphFormat& src);

    bool operator == (const GFxTextParagraphFormat& f) const
    {
        return (PresentMask == f.PresentMask && BlockIndent == f.BlockIndent && Indent == f.Indent && 
            Leading == f.Leading && LeftMargin == f.LeftMargin &&
            RightMargin == f.RightMargin && TabStopsEqual(f.pTabStops));
    }

    void InitByDefaultValues();
protected:
    void AllocTabStops(UInt num);
    void FreeTabStops();
    bool TabStopsEqual(const UInt* psrcTabStops) const;
    void CopyTabStops(const UInt* psrcTabStops);
};

typedef GFxTextFormatPtrWrapper<GFxTextParagraphFormat> GFxTextParagraphFormatPtr;

// Text allocator. Allocates text, text and paragraph formats.
class GFxTextAllocator : public GRefCountBaseNTS<GFxTextAllocator, GFxStatMV_Text_Mem>
{
public:
    enum FlagsEnum
    {
        Flags_Global = 0x1 // global allocator, do not save font handles (since they might be destroyed)
    };
protected:
    enum 
    {
        InitialFormatCacheCap = 100,
        FormatCacheCapacityDelta = 10
    };
    typedef GHashSetLH<GFxTextFormatPtr, GFxTextFormatPtr::HashFunctor,
                       GFxTextFormatPtr::HashFunctor, GFxStatMV_Text_Mem>                   TextFormatStorageType;

    typedef GHashSetLH<GFxTextParagraphFormatPtr, GFxTextParagraphFormatPtr::HashFunctor,
                       GFxTextParagraphFormatPtr::HashFunctor, GFxStatMV_Text_Mem>          ParagraphFormatStorageType;

    TextFormatStorageType           TextFormatStorage;
    ParagraphFormatStorageType      ParagraphFormatStorage;
    UInt                            TextFormatStorageCap;
    UInt                            ParagraphFormatStorageCap;
    UInt32                          NewParagraphId;
    GMemoryHeap*                    pHeap;
   
    // Default text format used for heap-correct format allocation.
    GFxTextFormat                   EntryTextFormat;
    UInt8                           Flags;

public:
    GFxTextAllocator(GMemoryHeap* pheap, UInt8 flags = 0)
      : TextFormatStorageCap(InitialFormatCacheCap),ParagraphFormatStorageCap(InitialFormatCacheCap), 
        NewParagraphId(1), pHeap(pheap), EntryTextFormat(pheap), Flags(flags) {}
    ~GFxTextAllocator() { GASSERT(1); }

    GMemoryHeap* GetHeap() const { return pHeap; }

    wchar_t* AllocText(UPInt numChar)
    {
        return (wchar_t*)GHEAP_ALLOC(pHeap, numChar * sizeof(wchar_t), GFxStatMV_Text_Mem);
    }
    wchar_t* ReallocText(wchar_t* pstr, UPInt oldLength, UPInt newLength)
    {
        GUNUSED(oldLength);
        if (!pstr) return AllocText(newLength);
        GASSERT(GMemory::GetHeapByAddress(pstr) == GetHeap());
        return (wchar_t*)GREALLOC(pstr, newLength*sizeof(wchar_t), GFxStatMV_Text_Mem);
    }
    void FreeText(wchar_t* pstr)
    {
        GHEAP_FREE(pHeap, pstr);
    }
    void* AllocRaw(UPInt numBytes)
    {
        return GHEAP_ALLOC(pHeap, numBytes, GFxStatMV_Text_Mem);
    }
    void FreeRaw(void* pstr)
    {
        GHEAP_FREE(pHeap, pstr);
    }

    GFxTextFormat* AllocateTextFormat(const GFxTextFormat& srcfmt);

    GFxTextFormat* AllocateDefaultTextFormat()
    {
        return AllocateTextFormat(EntryTextFormat);
    }

    GFxTextParagraphFormat* AllocateParagraphFormat(const GFxTextParagraphFormat& srcfmt);

    // Flushes text format cache. Returns 'true' if cache was not empty.
    // Set 'noAllocationsAllowed' to true if need to prevent new allocations
    // (for example, when calling from OnExceedLimit handler).
    bool FlushTextFormatCache(bool noAllocationsAllowed = false);

    // Flushes paragraph format cache. Returns 'true' if cache was not empty.
    // Set 'noAllocationsAllowed' to true if need to prevent new allocations
    // (for example, when calling from OnExceedLimit handler).
    bool FlushParagraphFormatCache(bool noAllocationsAllowed = false);

    GFxTextParagraph* AllocateParagraph();
    GFxTextParagraph* AllocateParagraph(const GFxTextParagraph& srcPara);

    UInt32  AllocateParagraphId() { return NewParagraphId++; }
    UInt32  GetNextParagraphId()  { return NewParagraphId; }
};

template <typename T>
struct CachedPrimValue
{
    mutable T       Value;
    mutable UInt16  FormatCounter;

    CachedPrimValue():FormatCounter(0) {}
    CachedPrimValue(T v):Value(v), FormatCounter(0) {}
    CachedPrimValue(T v, UInt16 counter):Value(v), FormatCounter(counter) {}

    void SetValue(T v, UInt16 counter)
    {
        Value = v; FormatCounter = counter;
    }
    operator T() const { return Value; }
    bool IsValid(UInt16 fmtCnt)
    {
        return FormatCounter == fmtCnt;
    }
};

template <typename T>
struct CachedValue
{
    mutable T       Value;
    mutable UInt16  FormatCounter;

    CachedValue():FormatCounter(0) {}
    CachedValue(const T& v):Value(v), FormatCounter(0) {}
    CachedValue(const T& v, UInt16 counter):Value(v), FormatCounter(counter) {}

    void SetValue(const T& v, UInt16 counter)
    {
        Value = v; FormatCounter = counter;
    }
    T& GetValue() const { return Value; }
    T& operator*() const { return Value; }
    T* operator->() const { return &Value; }
    //operator T&() const { return Value; }
    bool IsValid(UInt16 fmtCnt)
    {
        return FormatCounter == fmtCnt;
    }
    void Invalidate() { --FormatCounter; }
};

#endif //INC_TEXT_GFXTEXTCORE_H
