/**********************************************************************

Filename    :   GFxGlyphFitter.h
Content     :   
Created     :   
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxGlyphFitter_H
#define INC_GFxGlyphFitter_H

#include "GArrayUnsafe.h"
#include "GArrayPaged.h"


//------------------------------------------------------------------------
class GFxGlyphFitter
{
    enum { SID = GStat_Default_Mem };

    enum DirType
    { 
        DirCW  = 1,
        DirCCW = 2
    };

    enum FitDir
    {
        FitX = 1,
        FitY = 2
    };

public:
    struct VertexType
    {
        SInt16 x,y;
    };

    struct ContourType
    {
        UInt StartVertex;
        UInt NumVertices;
    };

    GFxGlyphFitter(int nominalFontHeight=1024): 
        NominalFontHeight(nominalFontHeight) {}

    void SetNominalFontHeight(int height) { NominalFontHeight = height; }

    void Clear();
    void ClearAndRelease();
    void MoveTo(int x, int y);
    void LineTo(int x, int y);

    int  ComputeTopY() { computeBounds(); return MaxY; }
    void FitGlyph(int heightInPixels, int widthInPixels, 
                  int lowerCaseTop,   int upperCaseTop);

    int                GetNominalFontHeight() const { return NominalFontHeight; }
    int                GetUnitsPerPixelX()    const { return UnitsPerPixelX; }
    int                GetUnitsPerPixelY()    const { return UnitsPerPixelY; }
    int                GetSnappedHeight()     const { return SnappedHeight; }
    UPInt              GetNumContours()       const { return Contours.GetSize(); }
    const ContourType& GetContour(UInt i)     const { return Contours[i]; }
    const VertexType&  GetVertex(const ContourType& c, UInt i) const 
    { 
        return Vertices[c.StartVertex + i]; 
    }

    void SnapVertex(VertexType& v)
    {
        int i;

        i = v.y - MinY;
        if(i >= 0 && i < (int)LerpRampY.GetSize())
        {
            v.y = LerpRampY[i] + MinY;
        }

        i = v.x - MinX;
        if(i >= 0 && i < (int)LerpRampX.GetSize())
        {
            v.x = LerpRampX[i] + MinX;
        }
    }

private:
    void removeDuplicateClosures();
    void computeBounds();
    void detectEvents(FitDir dir);
    void computeLerpRamp(FitDir dir, int unitsPerPixel, int middle, int lowerCaseTop, int upperCaseTop);
    int  snapToPixel(int x, int unitsPerPixel)
    {
        return (x + SnappedHeight) / unitsPerPixel * unitsPerPixel - SnappedHeight;
    }

    int                                         NominalFontHeight;
    GArrayPagedLH_POD<ContourType, 4, 16, SID>  Contours;
    GArrayPagedLH_POD<VertexType, 6, 16, SID>   Vertices;
    GArrayUnsafeLH_POD<UByte, SID>              Events;
    GArrayPagedLH_POD<VertexType, 6, 16, SID>   LerpPairs;
    GArrayUnsafeLH_POD<SInt16, SID>             LerpRampX;
    GArrayUnsafeLH_POD<SInt16, SID>             LerpRampY;
    SInt16                                      MinX;
    SInt16                                      MinY;
    SInt16                                      MaxX;
    SInt16                                      MaxY;
    DirType                                     Direction;
    int                                         UnitsPerPixelX;
    int                                         UnitsPerPixelY;
    int                                         SnappedHeight;
};


#endif
