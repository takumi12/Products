/**********************************************************************

Filename    :   GFxFontCompactor.cpp
Content     :   
Created     :   2007
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   Compact font data storage

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

For information regarding Commercial License Agreements go to:
online - http://www.scaleform.com/licensing.html or
email  - sales@scaleform.com 

**********************************************************************/

#ifndef INC_GFxFontCompactor_H
#define INC_GFxFontCompactor_H

#include "GConfig.h"
#if !defined(GFC_NO_COMPACTED_FONT_SUPPORT) || !defined(GFC_NO_FONTCOMPACTOR_SUPPORT)
#include "GArray.h"
#include "GHash.h"
#include "GTypes2DF.h"
#include "GArrayUnsafe.h"
#include "GArrayPaged.h"
#include "GRefCount.h"
#include "GFxFont.h"
#include "GFxPathDataStorage.h"


// Font collection data structure:
//------------------------------------------------------------------------
//
// Font Collection:
//
//      FontType            Font[];         // while(pos < size)
//
// FontType:
//      Char                Name[];         // Null-terminated, possibly UTF-8 encoded
//      UInt16fixlen        Flags;
//      UInt16fixlen        nominalSize;
//      SInt16fixlen        Ascent;
//      SInt16fixlen        Descent;
//      SInt16fixlen        Leading;
//      UInt32fixlen        NumGlyphs;
//      UInt32fixlen        TotalGlyphBytes;                // To quckly jump to the tables
//      GlyphType           Glyphs[NumGlyphs];
//      GlyphInfoType       GlyphInfoTable[NumGlyphs];      // Ordered by GlyphCode
//      UInt30              KerningTableSize;
//      KerningPairType     KerningPairs[KerningTableSize]; // Ordered by Code1,Code2
//      
// GlyphType:
//      SInt15              BoundingBox[4];
//      UInt15              NumContours;
//      ContourType         Contours[NumContours];
//      
// ContourType:
//      SInt15              MoveToX;
//      SInt15              MoveToY;
//      UInt30              NumEdges OR Reference;
//          if  (NumEdges & 1) Go To (NumEdges >> 1) and read edges
//          else NumEdges >>= 1
//      EdgeType            Edges[NumEdges];    // No edgess in case of reference.
//      
// EdgeType:
//      See GFxPathDataStorage.cpp, edge description
//
// GlyphInfoType:
//      UInt16fixlen        GlyphCode;
//      SInt16fixlen        AdvanceX;
//      UInt32fixlen        GlobalOffset;       // Starting from BoundingBox
//
// KerningPair:
//      UInt16fixlen        Char1;
//      UInt16fixlen        Char2;
//      SInt16fixlen        Adjustment;
//
//------------------------------------------------------------------------




//------------------------------------------------------------------------
template<class ContainerType> class GFxGlyphPathIterator
{
public:
    typedef GFxPathDataDecoder<ContainerType> PathDataDecoderType;

    GFxGlyphPathIterator(const ContainerType& data) : Data(data) {}

    void ReadBounds(UInt pos)
    {
        SInt t;
        Pos  = pos;
        Pos += Data.ReadSInt15(Pos, &t); XMin = SInt16(t);
        Pos += Data.ReadSInt15(Pos, &t); YMin = SInt16(t);
        Pos += Data.ReadSInt15(Pos, &t); XMax = SInt16(t);
        Pos += Data.ReadSInt15(Pos, &t); YMax = SInt16(t);
    }

    bool ValidBounds() const
    {
        return XMin < XMax && YMin < YMax;
    }

    void SetSpaceBounds(UInt advanceX)
    {
        XMin = YMin = YMax = 0;
        XMax = (SInt16)advanceX;
    }

    void StartGlyph(UInt pos)
    {
        ReadBounds(pos);
        Pos += Data.ReadUInt15(Pos, &NumContours);
        readPathHeader();
    }

    GRectF& GetBounds(GRectF* r) const { *r = GRectF(XMin, YMin, XMax, YMax); return *r; }
    SInt    GetWidth()   const { return XMax - XMin; }
    SInt    GetHeight()  const { return YMax - YMin; }

    bool    IsFinished() const { return NumContours == 0; }
    void    AdvancePath() { --NumContours; readPathHeader(); }

    SInt    GetMoveX() const { return MoveX; }
    SInt    GetMoveY() const { return MoveY; }

    bool    IsPathFinished() const { return NumEdges == 0; }
    void    ReadEdge(SInt* edge);

private:
    void readPathHeader();

    PathDataDecoderType Data;
    UInt   Pos;
    SInt16 XMin, YMin, XMax, YMax;
    SInt   MoveX, MoveY;
    UInt   NumContours;
    UInt   NumEdges;
    UInt   EdgePos;
    bool   JumpToPos;
};

//------------------------------------------------------------------------
template<class ContainerType>
void GFxGlyphPathIterator<ContainerType>::readPathHeader()
{
    if (NumContours)
    {
        Pos += Data.ReadSInt15(Pos, &MoveX);
        Pos += Data.ReadSInt15(Pos, &MoveY);
        Pos += Data.ReadUInt30(Pos, &NumEdges);

        EdgePos = Pos;
        JumpToPos = true;
        if (NumEdges & 1)
        {
            // Go to the referenced contour
            EdgePos   = NumEdges >> 1;
            EdgePos  += Data.ReadUInt30(EdgePos, &NumEdges);
            JumpToPos = false;
        }
        NumEdges >>= 1;
    }
}

//------------------------------------------------------------------------
template<class ContainerType>
void GFxGlyphPathIterator<ContainerType>::ReadEdge(SInt* edge)
{
    EdgePos += Data.ReadEdge(EdgePos, edge);
    switch(edge[0])
    {
    case PathDataDecoderType::Edge_HLine:
        MoveX += edge[1];
        edge[0] = PathDataDecoderType::Edge_Line;
        edge[1] = MoveX;
        edge[2] = MoveY;
        break;

    case PathDataDecoderType::Edge_VLine:
        MoveY += edge[1];
        edge[0] = PathDataDecoderType::Edge_Line;
        edge[1] = MoveX;
        edge[2] = MoveY;
        break;

    case PathDataDecoderType::Edge_Line:
        MoveX += edge[1];
        MoveY += edge[2];
        edge[1] = MoveX;
        edge[2] = MoveY;
        break;

    case PathDataDecoderType::Edge_Quad:
        MoveX += edge[1];
        MoveY += edge[2];
        edge[1] = MoveX;
        edge[2] = MoveY;
        MoveX += edge[3];
        MoveY += edge[4];
        edge[3] = MoveX;
        edge[4] = MoveY;
        break;
    }
    if (NumEdges)
        --NumEdges;

    if(NumEdges == 0 && JumpToPos)
        Pos = EdgePos;
}




//------------------------------------------------------------------------
template<class ContainerType>
class GFxCompactedFont : public GRefCountBaseNTS<GFxCompactedFont<ContainerType>, GFxStatMD_Fonts_Mem >
{
public:
    typedef GFxPathDataDecoder<ContainerType>   PathDataDecoderType;
    typedef GFxGlyphPathIterator<ContainerType> GlyphPathIteratorType;

    GFxCompactedFont(const ContainerType& fontData) : Decoder(fontData) {}
    unsigned    AcquireFont(unsigned startPos);

    UInt        GetNumGlyphs() const    { return NumGlyphs; }
    int         GetGlyphIndex(UInt16 code) const;
    UInt        GetGlyphCode(UInt glyphIndex) const;
    UInt        GetAdvanceInt(UInt glyphIndex) const;
    Float       GetAdvance(UInt glyphIndex) const;
    Float       GetKerningAdjustment(UInt lastCode, UInt thisCode) const;
    Float       GetGlyphWidth(UInt glyphIndex) const;
    Float       GetGlyphHeight(UInt glyphIndex) const;
    GRectF&     GetGlyphBounds(UInt glyphIndex, GRectF* prect) const;
    void        GetGlyphShape(UInt glyphIndex, GlyphPathIteratorType* glyph) const;

    Float       GetAscent()    const    { return Ascent;  }
    Float       GetDescent()   const    { return Descent; }
    Float       GetLeading()   const    { return Leading; }
    
    const char* GetName()        const  { return &Name[0]; }
    UInt        GetFontFlags()   const  { return Flags; }
    UInt        GetNominalSize() const  { return NominalSize; }

    bool        MatchFont(const char* name, UInt flags) const;

private:
    GFxCompactedFont(const GFxCompactedFont<ContainerType>&);
    const GFxCompactedFont<ContainerType>& operator = (const GFxCompactedFont<ContainerType>&);

    UInt getGlyphPos(UInt glyphIndex) const
    {
        return Decoder.ReadUInt32fixlen(GlyphInfoTablePos + 
                                        glyphIndex * (2+2+4) + 2+2);
    }

    const PathDataDecoderType   Decoder;
    UInt                        NumGlyphs;
    UInt                        GlyphInfoTablePos;
    UInt                        KerningTableSize;
    UInt                        KerningTablePos;

    GArrayUnsafePOD<char>       Name;
    UInt                        Flags;
    UInt                        NominalSize;
    Float                       Ascent;
    Float                       Descent;
    Float                       Leading;
};

//------------------------------------------------------------------------
template<class ContainerType> inline
UInt GFxCompactedFont<ContainerType>::GetGlyphCode(UInt glyphIndex) const
{
    UInt pos = GlyphInfoTablePos + glyphIndex * (2+2+4);
    return Decoder.ReadUInt16fixlen(pos);
}

//------------------------------------------------------------------------
template<class ContainerType> inline
int GFxCompactedFont<ContainerType>::GetGlyphIndex(UInt16 code) const
{
    int end = (int)NumGlyphs - 1;
    int beg = 0;
    while(beg <= end)
    {
        int  mid = (end + beg) / 2;
        UInt pos = GlyphInfoTablePos + mid * (2+2+4);
        UInt chr = Decoder.ReadUInt16fixlen(pos);
        if (chr == code)
        {
            return mid;
        }
        else
        {
            if (code < chr) end = mid - 1;
            else            beg = mid + 1;
        }
    }
    return -1;
}

//------------------------------------------------------------------------
template<class ContainerType> inline
UInt GFxCompactedFont<ContainerType>::GetAdvanceInt(UInt glyphIndex) const
{
    UInt pos = GlyphInfoTablePos + glyphIndex * (2+2+4) + 2;
    return Decoder.ReadSInt16fixlen(pos);
}

//------------------------------------------------------------------------
template<class ContainerType> inline
Float GFxCompactedFont<ContainerType>::GetAdvance(UInt glyphIndex) const
{
    return (Float)GetAdvanceInt(glyphIndex);
}


//------------------------------------------------------------------------
template<class ContainerType> inline
Float GFxCompactedFont<ContainerType>::GetKerningAdjustment(UInt lastCode, UInt thisCode) const
{
    int end = (int)KerningTableSize - 1;
    int beg = 0;
    while(beg <= end)
    {
        int  mid   = (end + beg) / 2;
        UInt pos   = KerningTablePos + mid * (2+2+2);
        UInt char1 = Decoder.ReadUInt16fixlen(pos);
        UInt char2 = Decoder.ReadUInt16fixlen(pos + 2);
        if (char1 == lastCode && char2 == thisCode)
        {
            return Float(Decoder.ReadSInt16fixlen(pos + 4));
        }
        else
        {
            bool pairLess = (lastCode != char1) ? (lastCode < char1) : (thisCode < char2);
            if (pairLess) end = mid - 1;
            else          beg = mid + 1;
        }
    }
    return 0;
}

//------------------------------------------------------------------------
template<class ContainerType> inline
Float GFxCompactedFont<ContainerType>::GetGlyphWidth(UInt glyphIndex) const
{
    GlyphPathIteratorType it(Decoder.GetData());
    it.ReadBounds(getGlyphPos(glyphIndex));
    if(!it.ValidBounds()) 
        it.SetSpaceBounds(GetAdvanceInt(glyphIndex));
    return Float(it.GetWidth());
}

//------------------------------------------------------------------------
template<class ContainerType> inline
Float GFxCompactedFont<ContainerType>::GetGlyphHeight(UInt glyphIndex) const
{
    GlyphPathIteratorType it(Decoder.GetData());
    it.ReadBounds(getGlyphPos(glyphIndex));
    if(!it.ValidBounds()) 
        it.SetSpaceBounds(GetAdvanceInt(glyphIndex));
    return Float(it.GetHeight());
}

//------------------------------------------------------------------------
template<class ContainerType> inline
GRectF& GFxCompactedFont<ContainerType>::GetGlyphBounds(UInt glyphIndex, 
                                                        GRectF* prect) const
{
    GlyphPathIteratorType it(Decoder.GetData());
    it.ReadBounds(getGlyphPos(glyphIndex));
    if(!it.ValidBounds()) 
        it.SetSpaceBounds(GetAdvanceInt(glyphIndex));
    return it.GetBounds(prect);
}

//------------------------------------------------------------------------
template<class ContainerType>
void GFxCompactedFont<ContainerType>::GetGlyphShape(UInt glyphIndex, 
                                                    GlyphPathIteratorType* glyph) const
{
    glyph->StartGlyph(getGlyphPos(glyphIndex));
}

//------------------------------------------------------------------------
template<class ContainerType>
bool GFxCompactedFont<ContainerType>::MatchFont(const char* name, UInt flags) const
{
    return GString::CompareNoCase(&Name[0], name) == 0 && 
        (flags & GFxFont::FF_Style_Mask) == (Flags & GFxFont::FF_Style_Mask);
}

//------------------------------------------------------------------------
template<class ContainerType>
unsigned GFxCompactedFont<ContainerType>::AcquireFont(unsigned startPos)
{
    enum { MinFontDataSize = 15 };

    if (Decoder.GetSize() < startPos+MinFontDataSize)
        return 0;

    unsigned pos = startPos;
    UInt i;
    // Read font name
    //--------------------
    i = 0;
    while(Decoder.ReadChar(pos + i)) 
        ++i;

    Name.Resize(i + 1);
    for(i = 0; i < Name.GetSize(); ++i)
        Name[i] = Decoder.ReadChar(pos + i);

    pos += (unsigned)Name.GetSize();
    //---------------------

    // Read attributes and sizes;
    Flags           =        Decoder.ReadUInt16fixlen(pos); pos += 2;
    NominalSize     =        Decoder.ReadUInt16fixlen(pos); pos += 2;
    Ascent          = (Float)Decoder.ReadSInt16fixlen(pos); pos += 2;
    Descent         = (Float)Decoder.ReadSInt16fixlen(pos); pos += 2;
    Leading         = (Float)Decoder.ReadSInt16fixlen(pos); pos += 2;
    NumGlyphs       =        Decoder.ReadUInt32fixlen(pos); pos += 4;
    UInt glyphBytes =        Decoder.ReadUInt32fixlen(pos); pos += 4;

    // Navigate to glyph info table (code, advanceX, globalOffset)
    pos += glyphBytes;
    GlyphInfoTablePos = pos;

    // Navigate to kerning table
    pos += NumGlyphs * (2+2+4);
    pos += Decoder.ReadUInt30(pos, &KerningTableSize);
    KerningTablePos = pos;

    // Navigate to next font
    pos += KerningTableSize * (2+2+2);

    return pos - startPos;
}


#ifndef GFC_NO_FONTCOMPACTOR_SUPPORT
//------------------------------------------------------------------------
class GFxFontCompactor : public GRefCountBaseNTS<GFxFontCompactor, GFxStatMD_Fonts_Mem>
{
public:
    enum { SID = GFxStatMD_Fonts_Mem };
    typedef GArrayPagedLH_POD<UByte, 12, 256, SID>  ContainerType;
    typedef GFxPathDataEncoder<ContainerType>       PathDataEncoderType;
    typedef GFxPathDataDecoder<ContainerType>       PathDataDecoderType;
    typedef GFxGlyphPathIterator<ContainerType>     GlyphPathIteratorType;
    typedef GFxCompactedFont<ContainerType>         CompactedFontType;

    // Create the container as:
    // GFxFontCompactor::ContainerType container(GfxFontCompactor::NumBlocksInc);

     GFxFontCompactor(ContainerType& data);
    ~GFxFontCompactor();

    void Clear();

    // Font creation and packing
    //-----------------------
    void StartFont(const char* name, UInt flags, UInt nominalSize, 
                   SInt ascent, SInt descent, SInt leading);

    void StartGlyph();
    void MoveTo(SInt16 x, SInt16 y);
    void LineTo(SInt16 x, SInt16 y);
    void QuadTo(SInt16 cx, SInt16 cy, SInt16 ax, SInt16 ay);
    void EndGlyph(bool mergeContours);
    void EndGlyph(UInt glyphCode, SInt advanceX, bool mergeContours);

    void AssignGlyphInfo(UInt glyphIndex, UInt glyphCode, SInt advanceX);
    void AssignGlyphCode(UInt glyphIndex, UInt glyphCode);
    void AssignGlyphAdvance(UInt glyphIndex, SInt advanceX);

    void AddKerningPair(UInt char1, UInt char2, SInt adjustment);

    void UpdateFlags(UInt flags);
    void UpdateMetrics(SInt ascent, SInt descent, SInt leading);
    void EndFont();
    //-----------------------

    // Serialization
    //-----------------------
    UPInt GetDataSize() const { return Encoder.GetSize(); }
    void  Serialize(void* ptr, UInt start, UInt size) const
    {
        Encoder.Serialize(ptr, start, size);
    }

    UInt32 ComputePathHash(UInt pos) const;
    bool   PathsEqual(UInt pos, const GFxFontCompactor& cmpPath, UInt cmpPos) const;

    UInt32 ComputeGlyphHash(UInt pos) const;
    bool   GlyphsEqual(UInt pos, const GFxFontCompactor& cmpFont, UInt cmpPos) const;

private:
    GFxFontCompactor(const GFxFontCompactor&);
    const GFxFontCompactor& operator = (GFxFontCompactor&);

    struct VertexType
    {
        SInt16 x,y;
    };

    struct ContourType
    {
        UInt DataStart;
        UInt DataSize;
    };

    struct GlyphInfoType
    {
        UInt16 GlyphCode;
        SInt16 AdvanceX;
        UInt   GlobalOffset;
    };

    struct ContourKeyType
    {
        const GFxFontCompactor* pCoord;
        UInt32                  HashValue;
        UInt                    DataStart;

        ContourKeyType() : pCoord(0), HashValue(0), DataStart(0) {}
        ContourKeyType(const GFxFontCompactor* coord, UInt32 hash, UInt start) :
            pCoord(coord), HashValue(hash), DataStart(start) {}

        UPInt operator()(const ContourKeyType& data) const
        {
            return (UPInt)data.HashValue;
        }

        bool operator== (const ContourKeyType& cmpWith) const 
        {
            return pCoord->PathsEqual(DataStart, *cmpWith.pCoord, cmpWith.DataStart);
        }
    };

    struct GlyphKeyType
    {
        const GFxFontCompactor* pFont;
        UInt32                  HashValue;
        UInt                    DataStart;

        GlyphKeyType() : pFont(0), HashValue(0), DataStart(0) {}
        GlyphKeyType(const GFxFontCompactor* font, UInt32 hash, UInt start) :
            pFont(font), HashValue(hash), DataStart(start) {}

        UPInt operator()(const GlyphKeyType& data) const
        {
            return (UPInt)data.HashValue;
        }

        bool operator== (const GlyphKeyType& cmpWith) const 
        {
            return pFont->GlyphsEqual(DataStart, *cmpWith.pFont, cmpWith.DataStart);
        }
    };

    struct KerningPairType
    {
        UInt16 Char1, Char2;
        SInt   Adjustment;
    };

    static bool cmpGlyphCodes(const GlyphInfoType& a, const GlyphInfoType& b)
    {
        return a.GlyphCode < b.GlyphCode;
    }

    static bool cmpKerningPairs(const KerningPairType& a, const KerningPairType& b)
    {
        if (a.Char1 != b.Char1) return a.Char1 < b.Char1;
        return a.Char2 < b.Char2;
    }

    void normalizeLastContour();
    static void extendBounds(SInt* x1, SInt* y1, SInt* x2, SInt* y2, SInt x, SInt y);
    void computeBounds(SInt* x1, SInt* y1, SInt* x2, SInt* y2) const;
    UInt navigateToEndGlyph(UInt pos) const;


    // NOTE; MD_Fonts_Mem is not fully correct here
    typedef GAllocatorGH<ContourKeyType, SID>                                                   ContainerGlobalAllocator;
    typedef GHashSet<ContourKeyType, ContourKeyType, ContourKeyType, ContainerGlobalAllocator>  ContourHashType;
    typedef GHashSet<GlyphKeyType,   GlyphKeyType,   GlyphKeyType, ContainerGlobalAllocator>    GlyphHashType;

    PathDataEncoderType                             Encoder;
    PathDataDecoderType                             Decoder;
    ContourHashType                                 ContourHash;
    GlyphHashType                                   GlyphHash;
    GArrayPagedPOD<VertexType, 6, 64, SID>          TmpVertices;
    GArrayPagedPOD<ContourType, 6, 64, SID>         TmpContours;
    GArrayPagedPOD<VertexType, 6, 64, SID>          TmpContour;

    GHashSet<UInt16>                                GlyphCodes;
    GArrayPagedPOD<GlyphInfoType, 6, 64, SID>       GlyphInfoTable;
    GArrayPagedPOD<KerningPairType, 6, 64, SID>     KerningTable;

    UInt                                            FontMetricsPos;
    UInt                                            FontNumGlyphs;
    UInt                                            FontTotalGlyphBytes;
    UInt                                            FontStartGlyphs;
};
#endif //GFC_NO_FONTCOMPACTOR_SUPPORT


#endif //!defined(GFC_NO_COMPACTED_FONT_SUPPORT) || !defined(GFC_NO_FONTCOMPACTOR_SUPPORT)

#endif


