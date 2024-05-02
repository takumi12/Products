/*********************************************************************

Filename    :   GFxTextLineBuffer.cpp
Content     :   Text line buffer
Created     :   May, 2007
Authors     :   Artem Bolgar

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#include "GUTF8Util.h"

#include "GRenderer.h"
#include "GFxDisplayContext.h"
#include "GFxFontCacheManager.h"
#include "Text/GFxTextLineBuffer.h"
#include "GAlgorithm.h"

#ifdef GFC_BUILD_DEBUG
//#define DEBUG_MASK
//#define DEBUG_LINE_WIDTHS
//#define DEBUG_VIS_RECT
#endif //GFC_BUILD_DEBUG

#ifndef GFC_NO_3D
////////////////////////////////////////////////
// 3D Text tesselation logic
// - Moose
// scale ramp code (Maxim)
// poly area code
//
class TextUtil3D
{
public:
    TextUtil3D() : bInitted(false), bScaledFor3D(false) { }

    void BeginDisplay(const GFxDisplayContext &context, GMatrix2D *m)
    {
        GASSERT(context.pChar || !context.Is3D);
        if (context.Is3D && context.pChar)
        {
            // in this case, context.pParentMatrix is identity and the context.pParentMatrix3D has the accumulated 3D transform
            const GFxCharacter *parent = context.pChar;
            // get 3d bounds in stage coords
            GMatrix3D worldMat = *context.pParentMatrix3D;
            worldMat.Prepend(*m);

            GPointF bounds3D[4];
            parent->GetProjectedBounds(bounds3D, &worldMat);
            float bounds3DArea = (float)PolygonArea(bounds3D, 4);
//            DrawBounds(bounds3D, context.pRenderConfig->GetRenderer(), context, GColor(GColor::Blue, 255));
            // get 2D bounds in pixel stage coords
            GRectF bounds2D = TwipsToPixels(parent->GetWorldBounds());
            GASSERT(bounds2D.Area() != 0);
//            DrawBounds(bounds2D, context.pRenderConfig->GetRenderer(), context, GColor(GColor::Red, 255));
            float scaleFactor = SnapToRamp(bounds3DArea / bounds2D.Area());
//            parent->GetLog()->LogMessage("ScaleF=%.3f, B2DArea=%f, B3DArea=%f\n", scaleFactor, bounds2D.Area(), bounds3DArea);

            if (scaleFactor > 1.0f)
            {
                float scaleFactorInv = 1.f/scaleFactor;

                //                parent->GetLog()->LogMessage("ScaleF=%.3f, B2D=[%.2f,%.2f %.2f,%.2f] B3D=[%.2f,%.2f %.2f,%.2f]\n", scaleFactor, 
                //                    bounds2D.Left, bounds2D.Top, bounds2D.Right, bounds2D.Bottom,
                //                    bounds3D.Left, bounds3D.Top, bounds3D.Right, bounds3D.Bottom);

                // scale the 2D matrix that will be used to decide whether to tessellate or not, to take into account the 3D projected size 
                m->AppendScaling(scaleFactor);

                // inverse scale the 3D matrix now, to account for the added 2D scale
                GMatrix3D scale3D;
                scale3D.Scaling(scaleFactorInv, scaleFactorInv, scaleFactorInv);
                oldParentMatrix3D = *context.pParentMatrix3D;
                context.pParentMatrix3D->Prepend(scale3D);      
                bScaledFor3D = true;
            }
        }
    }

    void EndDisplay(const GFxDisplayContext &context)
    {
        // restore 3D parent matrix after scaling 
        if (bScaledFor3D)
            *context.pParentMatrix3D = oldParentMatrix3D;
    }

    void Init() 
    {
        if (!bInitted)
        {
            float scaleDetail = sqrtf(2);  // two times more detailed ramp.
            // scaleDetail = 2: very rough ramp: 1,2,4,8...
            // scaleDetail = pow(2, 0.25): four times more detailed ramp
            CreateRamp(scaleDetail);
            bInitted = true;
        }
        bScaledFor3D = false;
    }

private:

    float SnapToRamp(float factor)
    {
        int n = sizeof(scaleRamp) / sizeof(float);
        int i = 0;
        int j = n - 1;
        int k;
        for(i = 0; (j - i) > 1; ) 
        {
            if(factor < scaleRamp[k = (i + j) >> 1]) 
                j = k; 
            else                                     
                i = k;
        }
        float rampBelow = scaleRamp[i];
        float rampAbove = scaleRamp[j];
        return (factor-rampBelow < rampAbove-factor) ? rampBelow : rampAbove;
    }

    double PolygonArea(GPointF *polygon,int n)
    {
        int i,j;
        double area = 0;

        for (i=0;i<n;i++) 
        {
            j = (i + 1) % n;
            area += polygon[i].x * polygon[j].y;
            area -= polygon[i].y * polygon[j].x;
        }

        area /= 2;
        return(area < 0 ? -area : area);
    }

#ifdef GFC_BUILD_DEBUG
    void DrawLines(SInt16 *Lines, GRenderer *prenderer, const GFxDisplayContext &context, GColor col)
    {
        prenderer->LineStyleColor(col);
        prenderer->SetVertexData(Lines, 5, GRenderer::Vertex_XY16i);
        prenderer->SetWorld3D(NULL);        
        prenderer->SetMatrix(GMatrix2D::Identity);          // this will get set correctly when the next object renders
        prenderer->DrawLineStrip(0, 4);
        prenderer->SetWorld3D(context.pParentMatrix3D);     // restore, may be unnecessary
        prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
    }
    void DrawBounds(GPointF *bounds, GRenderer *prenderer, const GFxDisplayContext &context, GColor col)
    {
        static SInt16 Lines[5 * 2];
        Lines[0] = (SInt16)PixelsToTwips(bounds[0].x);    Lines[1] = (SInt16)PixelsToTwips(bounds[0].y);
        Lines[2] = (SInt16)PixelsToTwips(bounds[1].x);    Lines[3] = (SInt16)PixelsToTwips(bounds[1].y);
        Lines[4] = (SInt16)PixelsToTwips(bounds[2].x);    Lines[5] = (SInt16)PixelsToTwips(bounds[2].y);
        Lines[6] = (SInt16)PixelsToTwips(bounds[3].x);    Lines[7] = (SInt16)PixelsToTwips(bounds[3].y);
        Lines[8] = (SInt16)PixelsToTwips(bounds[0].x);    Lines[9] = (SInt16)PixelsToTwips(bounds[0].y);
        DrawLines(Lines, prenderer, context, col);
    }
    void DrawBounds(GRectF bounds, GRenderer *prenderer, const GFxDisplayContext &context, GColor col)
    {
        static SInt16 Lines[5 * 2];
        bounds = PixelsToTwips(bounds);
        Lines[0] = (SInt16)bounds.Left;    Lines[1] = (SInt16)bounds.Bottom;
        Lines[2] = (SInt16)bounds.Right;    Lines[3] = (SInt16)bounds.Bottom;
        Lines[4] = (SInt16)bounds.Right;    Lines[5] = (SInt16)bounds.Top;
        Lines[6] = (SInt16)bounds.Left;    Lines[7] = (SInt16)bounds.Top;
        Lines[8] = (SInt16)bounds.Left;    Lines[9] = (SInt16)bounds.Bottom;
        DrawLines(Lines, prenderer, context, col);
    }
#endif
    void CreateRamp(float scaleDetail)
    {
        float val = 1.f;
        for(int i = 0; i < sizeof(scaleRamp) / sizeof(float); ++i)
        {
            scaleRamp[i] = val;
            val *= scaleDetail;
        }
    }

    float   scaleRamp[32];   
    bool    bInitted;
    GMatrix3D oldParentMatrix3D;
    bool    bScaledFor3D;
};

////////////////////////////////////////////////
#endif


GFxTextLineBuffer::Line* GFxTextLineBuffer::TextLineAllocator::AllocLine(UInt size, GFxTextLineBuffer::LineType lineType)
{
    //TODO: look for already allocated line in FreeLinesPool first
    Line* pline = (Line*)GHEAP_AUTO_ALLOC(this, size);// Was: GFxStatMV_Text_Mem);
    pline->SetMemSize(size);
    if (lineType == Line8)
        pline->InitLine8();
    else
        pline->InitLine32();
    return pline;
}

void GFxTextLineBuffer::TextLineAllocator::FreeLine(GFxTextLineBuffer::Line* ptr)
{
    //TODO: return line being freed in FreeLinesPool
    if (ptr)
    {
        ptr->Release();
        GFREE(ptr);
    }
}

void GFxTextLineBuffer::ReleasePartOfLine(GlyphEntry* pglyphs, UInt n, GFxFormatDataEntry* pnextFormatData)
{
    for(UInt i = 0; i < n; ++i, ++pglyphs)
    {
        GlyphEntry* pglyph = pglyphs;
        if (pglyph->IsNextFormat())
        {   
            if (pglyph->HasFmtFont())
            {
                GFxFontHandle* pfont = pnextFormatData->pFont;
                pfont->Release();
                ++pnextFormatData;
            }
            if (pglyph->HasFmtColor())
            {
                ++pnextFormatData;
            }
            if (pglyph->HasFmtImage())
            {
                GFxTextImageDesc *pimage = pnextFormatData->pImage;
                pimage->Release();
                ++pnextFormatData;
            }
        }
    }
}

void GFxTextLineBuffer::GlyphInserter::ResetTo(const GlyphInserter& savedPos)
{
    GASSERT(savedPos.GlyphIndex <= GlyphIndex);
    if (savedPos.GlyphIndex < GlyphIndex && GlyphsCount > 0)
    {
        // first of all, we need to release all fonts and image descriptors
        // beyond the savedPos.GlyphIndex; otherwise memory leak is possible.
        
        GlyphEntry* pglyphs = savedPos.pGlyphs + savedPos.GlyphIndex;
        UInt n = GlyphIndex - savedPos.GlyphIndex;
        GFxFormatDataEntry* pnextFormatData 
            = reinterpret_cast<GFxFormatDataEntry*>(savedPos.pNextFormatData + savedPos.FormatDataIndex);
        ReleasePartOfLine(pglyphs, n, pnextFormatData);
    }
    operator=(savedPos);
}

void GFxTextLineBuffer::GlyphIterator::UpdateDesc()
{
    pImage = NULL;
    if (!IsFinished())
    {
        if (pGlyphs->IsNextFormat())
        {   
            if (pGlyphs->HasFmtFont())
            {
                pFontHandle = pNextFormatData->pFont;
                ++pNextFormatData;
            }
            if (pGlyphs->HasFmtColor())
            {
                OrigColor = Color = pNextFormatData->Color;
                ++pNextFormatData;
            }
            if (pGlyphs->HasFmtImage())
            {
                pImage = pNextFormatData->pImage;
                ++pNextFormatData;
            }
        }
        if (pGlyphs->IsUnderline())
        {
            UnderlineStyle  = GFxTextHighlightInfo::Underline_Single;
            UnderlineColor  = Color;
        }
        else
            UnderlineStyle = GFxTextHighlightInfo::Underline_None;

        if (!HighlighterIter.IsFinished())
        {
            // get data from the highlighter
            const GFxTextHighlightDesc& hd = *HighlighterIter;
            
            Color           = OrigColor;
            if ((pGlyphs->GetLength() > 0 || pGlyphs->IsWordWrapSeparator()))
            {
                if (hd.Info.HasTextColor())
                    Color = hd.Info.GetTextColor().ToColor32();

                if (hd.Info.HasUnderlineStyle())
                    UnderlineStyle = hd.Info.GetUnderlineStyle();
                if (hd.Info.HasUnderlineColor())
                    UnderlineColor = hd.Info.GetUnderlineColor().ToColor32();
                else
                    UnderlineColor  = Color;
            }
        }
        else
        {
            if (pGlyphs->IsUnderline())
            {
                UnderlineColor = Color;
                UnderlineStyle  = GFxTextHighlightInfo::Underline_Single;
            }
        }
    }
}

GFxTextLineBuffer::GFxFormatDataEntry* GFxTextLineBuffer::Line::GetFormatData() const 
{ 
    UByte* p = reinterpret_cast<UByte*>(GetGlyphs() + GetNumGlyphs());
    // align to boundary of GFxFormatDataEntry
    UPInt pi = ((UPInt)p);
    UPInt delta = ((pi + sizeof(GFxFormatDataEntry*) - 1) & ~(sizeof(GFxFormatDataEntry*) - 1)) - pi;
    return reinterpret_cast<GFxFormatDataEntry*>(p + delta);
}

void GFxTextLineBuffer::Line::Release()
{
    if (!IsInitialized()) return;

    GlyphEntry* pglyphs = GetGlyphs();
    UInt n = GetNumGlyphs();
    GFxFormatDataEntry* pnextFormatData = GetFormatData();
    ReleasePartOfLine(pglyphs, n, pnextFormatData);
    SetNumGlyphs(0);
#ifdef GFC_BUILD_DEBUG
    memset(((UByte*)this) + sizeof(MemSize), 0xf0, GetMemSize() - sizeof(MemSize));
#endif
}

bool GFxTextLineBuffer::Line::HasNewLine() const
{
    UInt n = GetNumGlyphs();
    if (n > 0)
    {
        GlyphEntry* pglyphs = GetGlyphs();
        return pglyphs[n - 1].IsNewLineChar() && !pglyphs[n - 1].IsEOFChar();
    }
    return false;
}

//////////////////////////////////
// GFxTextLineBuffer
//
GFxTextLineBuffer::GFxTextLineBuffer()
{
    DummyStyle.SetColor(0xFFFFFFFF); // White, modified by CXForm.
    Geom.VisibleRect.Clear();
    pBatchPackage = 0;
    pCacheManager = 0;
    LastHScrollOffset = ~0u;
}

GFxTextLineBuffer::~GFxTextLineBuffer()
{
    if (pCacheManager)
        pCacheManager->ReleaseBatchPackage(pBatchPackage);
    ClearLines();
}

GFxTextLineBuffer::Line* GFxTextLineBuffer::GetLine(UInt lineIdx)
{
    if (lineIdx >= Lines.GetSize())
        return NULL;
    GFxTextLineBuffer::Line* pline = Lines[lineIdx];
    InvalidateCache();
    return pline;
}

const GFxTextLineBuffer::Line* GFxTextLineBuffer::GetLine(UInt lineIdx) const
{
    if (lineIdx >= Lines.GetSize())
        return NULL;
    const GFxTextLineBuffer::Line* pline = Lines[lineIdx];
    return pline;
}

bool GFxTextLineBuffer::DrawMask(GFxDisplayContext &context, 
                                 const GRenderer::Matrix& mat, 
                                 const GRectF& rect, 
                                 GRenderer::SubmitMaskMode maskMode)
{
    GRenderer*  prenderer = context.GetRenderer();

    // Only clear stencil if no masks were applied before us; otherwise
    // no clear is necessary because we are building a cumulative mask.
#ifndef DEBUG_MASK
    prenderer->BeginSubmitMask(maskMode);
#else
    GUNUSED(maskMode);
#endif

    GPointF                 coords[4];
    static const UInt16     indices[6] = { 0, 1, 2, 2, 1, 3 };

    // Show white background + black bounding box.
    //prenderer->SetCxform(cx); //?AB: do we need this for drawing mask?
#ifdef DEBUG_MASK
    GRenderer::Cxform colorCxform;
    GColor color(255, 0, 0, 128);
    colorCxform.M_[0][0] = (1.0f / 255.0f) * color.GetRed();
    colorCxform.M_[1][0] = (1.0f / 255.0f) * color.GetGreen();
    colorCxform.M_[2][0] = (1.0f / 255.0f) * color.GetBlue();
    colorCxform.M_[3][0] = (1.0f / 255.0f) * color.GetAlpha();
    prenderer->SetCxform(colorCxform); //?AB: do we need this for drawing mask?
#endif
    Matrix m(mat);
    GRectF newRect;
    GFx_RecalculateRectToFit16Bit(m, rect, &newRect);
    prenderer->SetMatrix(m);

    coords[0] = newRect.TopLeft();
    coords[1] = newRect.TopRight();
    coords[2] = newRect.BottomLeft();
    coords[3] = newRect.BottomRight();

    GRenderer::VertexXY16i icoords[4];
    icoords[0].x = (SInt16) coords[0].x;
    icoords[0].y = (SInt16) coords[0].y;
    icoords[1].x = (SInt16) coords[1].x;
    icoords[1].y = (SInt16) coords[1].y;
    icoords[2].x = (SInt16) coords[2].x;
    icoords[2].y = (SInt16) coords[2].y;
    icoords[3].x = (SInt16) coords[3].x;
    icoords[3].y = (SInt16) coords[3].y;

    prenderer->FillStyleColor(GColor(255, 255, 255, 255));
    prenderer->SetVertexData(icoords, 4, GRenderer::Vertex_XY16i);

    // Fill the inside
    prenderer->SetIndexData(indices, 6, GRenderer::Index_16);
    prenderer->DrawIndexedTriList(0, 0, 4, 0, 2);

    // Done
    prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
    prenderer->SetIndexData(0, 0, GRenderer::Index_None);

    // Done rendering mask.
#ifndef DEBUG_MASK
    prenderer->EndSubmitMask();
#endif
    return true;
}

UInt GFxTextLineBuffer::CalcLineSize
    (UInt glyphCount, UInt formatDataElementsCount, GFxTextLineBuffer::LineType lineType)
{
    UInt sz;
    if (lineType == Line8)
        sz = LineData8::GetLineStructSize();
    else
        sz = LineData32::GetLineStructSize();
    sz += sizeof(UInt32); // MemSize
    sz += sizeof(GlyphEntry) * glyphCount;
    // add alignment here
    sz = (sz + sizeof(GFxFormatDataEntry) - 1) & ~(sizeof(GFxFormatDataEntry) - 1);
    sz += sizeof(GFxFormatDataEntry) * formatDataElementsCount;
    return sz;
}


GFxTextLineBuffer::Line* GFxTextLineBuffer::InsertNewLine
    (UInt lineIdx, UInt glyphCount, UInt formatDataElementsCount, GFxTextLineBuffer::LineType lineType)
{
    UInt sz = CalcLineSize(glyphCount, formatDataElementsCount, lineType);
    Line* pline = LineAllocator.AllocLine(sz, lineType);
    if (!pline) return NULL;

    pline->SetNumGlyphs(glyphCount);
    Lines.InsertAt(lineIdx, pline);
    return pline;
}

void GFxTextLineBuffer::RemoveLines(UInt lineIdx, UInt num)
{
    Iterator it = Begin() + lineIdx;
    for(UInt i = 0; i < num && !it.IsFinished(); ++it, ++i)
    {
        LineAllocator.FreeLine(it.GetPtr());
    }
    Lines.RemoveMultipleAt(lineIdx, num);
}

UInt   GFxTextLineBuffer::GetVScrollOffsetInTwips() const
{
    SInt yOffset = 0;
    if (Geom.FirstVisibleLinePos != 0)
    {
        // calculate y-offset of first visible line
        ConstIterator visIt   = BeginVisible(0);
        ConstIterator linesIt = Begin();
        if (!visIt.IsFinished() && !linesIt.IsFinished())
        {
            yOffset = visIt->GetOffsetY() - linesIt->GetOffsetY();
            GASSERT(yOffset >= 0);
        }
    }
    return UInt(yOffset);
}

void GFxTextLineBuffer::ResetCache() 
{ 
    if (pCacheManager)
        pCacheManager->ReleaseBatchPackage(pBatchPackage);
    pBatchPackage = 0;
}


template<class Matrix>
static Float GFx_CalcHeightRatio(const Matrix& matrix)
{
    Matrix mat2(matrix);
    mat2.M_[0][2] = 0;
    mat2.M_[1][2] = 0;
    GPointF p01 = mat2.Transform(GPointF(0,1));
    GPointF p10 = mat2.Transform(GPointF(1,0));
    return fabsf(GMath2D::CalcLinePointDistance(0, 0, p10.x, p10.y, p01.x, p01.y));
}

void    GFxTextLineBuffer::DrawUnderline(
     Float offx, Float offy, Float singleUnderlineHeight, Float singlePointWidth, const GRectF& hscrolledViewRect, 
     SInt lineLength, GFxTextHighlightInfo::UnderlineStyle style, UInt32 color,
     const Matrix& globalMatrixDir, const Matrix& globalMatrixInv)
{
    // clip offx and lineLength by VisibleRect, since mask might be off
    GPointF topLeft     = hscrolledViewRect.ClampPoint(GPointF(offx, offy));
    GPointF bottomRight;
    if (style == GFxTextHighlightInfo::Underline_Thick || 
        style == GFxTextHighlightInfo::Underline_Thick)
    {
        bottomRight = hscrolledViewRect.ClampPoint(
            GPointF(offx + lineLength, offy + singleUnderlineHeight*2));
    }
    else
        bottomRight = hscrolledViewRect.ClampPoint(
            GPointF(offx + lineLength, offy + singleUnderlineHeight));
    bottomRight.x = floorf(bottomRight.x + 0.5f);
    bottomRight.y = floorf(bottomRight.y + 0.5f);

    if (!pUnderlineDrawing)
        pUnderlineDrawing = *GHEAP_AUTO_NEW(this) GFxDrawingContext;
    pUnderlineDrawing->SetFill(color);
    Float dashLen = (style == GFxTextHighlightInfo::Underline_Dotted) ? 2.f : 1.f;
    switch (style)
    {
    case GFxTextHighlightInfo::Underline_Single:
    case GFxTextHighlightInfo::Underline_Thick:
        pUnderlineDrawing->MoveTo(topLeft.x, topLeft.y);
        pUnderlineDrawing->LineTo(bottomRight.x, topLeft.y);
        pUnderlineDrawing->LineTo(bottomRight.x, bottomRight.y);
        pUnderlineDrawing->LineTo(topLeft.x, bottomRight.y);
        pUnderlineDrawing->LineTo(topLeft.x, topLeft.y);
        pUnderlineDrawing->AddPath();
        break;
    case GFxTextHighlightInfo::Underline_Dotted:
    case GFxTextHighlightInfo::Underline_DitheredSingle:
    {
        for (Float x = topLeft.x; x < bottomRight.x; x += singlePointWidth*dashLen*2)
        {
            // for dotted or dithered underlining we need to snap to pixel every pixel
            // since otherwise it doesn't work correctly.
            GPointF p(x, bottomRight.y);
            p   = globalMatrixDir.Transform(p);
            p.x = floorf(p.x + 0.5f);
            p.y = floorf(p.y + 0.5f);
            p    = globalMatrixInv.Transform(p);
            p.x = floorf(p.x + 0.5f);
            p.y = floorf(p.y + 0.5f);
            x = p.x;

            pUnderlineDrawing->MoveTo(p.x, topLeft.y);
            pUnderlineDrawing->LineTo(p.x + singlePointWidth*dashLen, topLeft.y);
            pUnderlineDrawing->LineTo(p.x + singlePointWidth*dashLen, bottomRight.y);
            pUnderlineDrawing->LineTo(p.x, bottomRight.y);
            pUnderlineDrawing->LineTo(p.x, topLeft.y);
            pUnderlineDrawing->AddPath();
        }
        break;
    }
    case GFxTextHighlightInfo::Underline_DitheredThick:
        // X X X X X X X X
        //  X X X X X X X X
        for (int i = 0; i < 2; ++i)
        {
            for (Float x = topLeft.x + i*singlePointWidth; x < bottomRight.x; x += singlePointWidth*2)
            {
                // for dotted or dithered underlining we need to snap to pixel every pixel
                // since otherwise it doesn't work correctly.
                GPointF p(x, topLeft.y);
                p   = globalMatrixDir.Transform(p);
                p.x = floorf(p.x + 0.5f);
                p.y = floorf(p.y + 0.5f);
                p    = globalMatrixInv.Transform(p);
                p.x = floorf(p.x + 0.5f);
                p.y = floorf(p.y + 0.5f);
                x = p.x;

                pUnderlineDrawing->MoveTo(p.x, p.y);
                pUnderlineDrawing->LineTo(p.x + singlePointWidth, p.y);
                pUnderlineDrawing->LineTo(p.x + singlePointWidth, p.y + singleUnderlineHeight);
                pUnderlineDrawing->LineTo(p.x, p.y + singleUnderlineHeight);
                pUnderlineDrawing->LineTo(p.x, p.y);
                pUnderlineDrawing->AddPath();
            }
            topLeft.y += singleUnderlineHeight;
        }
        break;
    default: GASSERT(0); // not supported
    }
}

#if defined(GFC_BUILD_DEBUG) && (defined(DEBUG_LINE_WIDTHS) || defined(DEBUG_VIS_RECT))
static void DrawRect(GRenderer*  prenderer, const GRenderer::Matrix& mat, const GRectF& rect, GColor color)
{
    GRenderer::Cxform colorCxform;
    colorCxform.M_[0][0] = (1.0f / 255.0f) * color.GetRed();
    colorCxform.M_[1][0] = (1.0f / 255.0f) * color.GetGreen();
    colorCxform.M_[2][0] = (1.0f / 255.0f) * color.GetBlue();
    colorCxform.M_[3][0] = (1.0f / 255.0f) * color.GetAlpha();
    prenderer->SetCxform(colorCxform); //?AB: do we need this for drawing mask?

    GPointF                 coords[4];
    static const UInt16     indices[6] = { 0, 1, 2, 2, 1, 3 };

    GRenderer::Matrix m(mat);
    GRectF newRect;
    GFx_RecalculateRectToFit16Bit(m, rect, &newRect);
    prenderer->SetMatrix(m);

    coords[0] = newRect.TopLeft();
    coords[1] = newRect.TopRight();
    coords[2] = newRect.BottomLeft();
    coords[3] = newRect.BottomRight();

    const SInt16  icoords[18] = 
    {
        // Strip (fill in)
        (SInt16) coords[0].x, (SInt16) coords[0].y,
        (SInt16) coords[1].x, (SInt16) coords[1].y,
        (SInt16) coords[2].x, (SInt16) coords[2].y,
        (SInt16) coords[3].x, (SInt16) coords[3].y
    };

    prenderer->FillStyleColor(GColor(255, 255, 255, 255));
    prenderer->SetVertexData(icoords, 4, GRenderer::Vertex_XY16i);

    // Fill the inside
    prenderer->SetIndexData(indices, 6, GRenderer::Index_16);
    prenderer->DrawIndexedTriList(0, 0, 4, 0, 2);

    // Done
    prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
    prenderer->SetIndexData(0, 0, GRenderer::Index_None);
}
#endif

void    GFxTextLineBuffer::Display(GFxDisplayContext &context,
                                   const Matrix& matrixIn, const Cxform& cxform,
                                   bool nextFrame,
                                   const GFxTextFieldParam& textFieldParam,
                                   const GFxTextHighlighter* phighlighter)
{
    GFxTextFieldParam param(textFieldParam);

//param.TextParam.SetOutline(1);

// DBG
//param.ShadowColor = 0xC0FF0000;
//param.ShadowParam.SetBlurX(2.f);
//param.ShadowParam.SetBlurY(2.f);
//param.ShadowParam.SetBlurStrength(5.0f).
//param.ShadowParam.SetFineBlur(false);
//param.ShadowParam.SetKnockOut(true);
//param.TextParam.SetOptRead(true);
//param.TextParam.SetBitmapFont(true);
//param.TextParam.SetFauxItalic(true);
//param.TextParam.SetFauxBold(true);

    GRenderer*  prenderer = context.GetRenderer();

    bool dynamicCacheEnabled = false; 
    bool Text3DVectorizationEnabled = false;
    bool hasUnderline = false;
                
    Float maxRasterScale = 1.25f;
    UInt  steadyCount = 0;
    GFxFontCacheManager* fm = context.pFontCacheManager;
    if (fm)
    {
        pCacheManager       = fm->GetImplementation();
        dynamicCacheEnabled = fm->IsDynamicCacheEnabled();
        Text3DVectorizationEnabled = fm->IsText3DVectorizationEnabled();
        maxRasterScale      = fm->GetMaxRasterScale();
        steadyCount         = fm->GetSteadyCount();
    }
    else
    {
        pCacheManager = 0;
    }

    Matrix matrix = matrixIn;
#ifndef GFC_NO_3D
    static TextUtil3D text3D;
    if (Text3DVectorizationEnabled)
    {
        text3D.Init();
        text3D.BeginDisplay(context, &matrix);        
    }
#endif

    Matrix mat(matrix);
    mat.PrependTranslation(-Float(Geom.HScrollOffset), 0);

    Float yOffset = -Float(GetVScrollOffsetInTwips());

    Float heightRatio = GFx_CalcHeightRatio(matrix) * 
                        PixelsToTwips(context.ViewportMatrix.M_[1][1]);

    if (heightRatio < 0.001f)
        heightRatio = 0.001f; // To prevent from Div by Zero

    if (steadyCount && !param.TextParam.IsOptRead())
    {
        bool animation = (matrix != Geom.SourceTextMatrix);
        if (animation)
        {
            Geom.SteadyCount = 0;
            if (Geom.IsReadability())
            {
                InvalidateCache();
                Geom.ClearReadability();
            }
        }
        else
        {
            if (nextFrame)
            {
                Geom.SteadyCount++;
                if (Geom.SteadyCount > steadyCount) 
                    Geom.SteadyCount = steadyCount;
            }
        }
        if (Geom.SteadyCount >= steadyCount)
        {
            param.TextParam.SetOptRead(true);
            param.ShadowParam.SetOptRead(true);
            if(!Geom.IsReadability())
            {
                InvalidateCache();
                Geom.SetReadability();
            }
        }
    }

    Matrix displayMatrix = matrix;
    bool   matrixChanged = false;
    if (param.TextParam.IsOptRead())
    {
        //// We have to check the 2x2 matrix. Theoretically it would be enough 
        //// to check for the heightRatio only, but it may result in using 
        //// a degenerate matrix for the first calculation, and so, 
        //// introducing huge inaccuracy. 
        ////------------------------------
        //matrixChanged = Geom.SourceTextMatrix.M_[0][0] != matrix.M_[0][0] ||
        //                Geom.SourceTextMatrix.M_[0][1] != matrix.M_[0][1] ||
        //                Geom.SourceTextMatrix.M_[1][0] != matrix.M_[1][0] ||
        //                Geom.SourceTextMatrix.M_[1][1] != matrix.M_[1][1] ||
        //                Geom.ViewportMatrix != context.ViewportMatrix;
        //Geom.SourceTextMatrix = matrix;


        // New method that eliminates text "shaking", when changing of 
        // coordinates is very small (less than 0.1 pixel).
        //----------------------------
        matrixChanged = matrix                 != Geom.SourceTextMatrix || 
                        context.ViewportMatrix != Geom.ViewportMatrix;

        if(matrixChanged)
        {
            GPointF p11 = GPointF(Geom.VisibleRect.Left,  Geom.VisibleRect.Top);
            GPointF p12 = GPointF(Geom.VisibleRect.Right, Geom.VisibleRect.Top);
            GPointF p13 = GPointF(Geom.VisibleRect.Right, Geom.VisibleRect.Bottom);
            GPointF p21 = p11;
            GPointF p22 = p12;
            GPointF p23 = p13;

            Geom.SourceTextMatrix.Transform2x2(&p11.x, &p11.y); p11 = Geom.ViewportMatrix.Transform(p11);
            Geom.SourceTextMatrix.Transform2x2(&p12.x, &p12.y); p12 = Geom.ViewportMatrix.Transform(p12);
            Geom.SourceTextMatrix.Transform2x2(&p13.x, &p13.y); p13 = Geom.ViewportMatrix.Transform(p13);

            matrix.Transform2x2(&p21.x, &p21.y); p21 = context.ViewportMatrix.Transform(p21);
            matrix.Transform2x2(&p22.x, &p22.y); p22 = context.ViewportMatrix.Transform(p22);
            matrix.Transform2x2(&p23.x, &p23.y); p23 = context.ViewportMatrix.Transform(p23);

            Float d = p11.DistanceL1(p21) + p12.DistanceL1(p22) + p13.DistanceL1(p23);
            matrixChanged = d > 0.3f;
        }

        if (matrixChanged)
        {
            Geom.SourceTextMatrix = matrix;
        }
        else
        {
            displayMatrix = Geom.SourceTextMatrix;
            GPointF transl(GPointF(matrix.M_[0][2] - Geom.Translation.x, 
                                   matrix.M_[1][2] - Geom.Translation.y));
            Float kx = context.ViewportMatrix.M_[0][0];
            Float ky = context.ViewportMatrix.M_[1][1];
            displayMatrix.M_[0][2] = Geom.Translation.x + floorf(transl.x * kx + 0.5f) / kx;
            displayMatrix.M_[1][2] = Geom.Translation.y + floorf(transl.y * ky + 0.5f) / ky;
        }
    }
    else
    {
        matrixChanged = Geom.ViewportMatrix != context.ViewportMatrix;
        if (Geom.NeedsCheckPreciseScale() && heightRatio != Geom.HeightRatio)
        {
            matrixChanged = true;
        }
        Geom.SourceTextMatrix = matrix;
    }

    Geom.HeightRatio      = heightRatio;
    Geom.ViewportMatrix   = context.ViewportMatrix;

    if (phighlighter)
    {
        hasUnderline = phighlighter->HasUnderlineHighlight();
        if (!phighlighter->IsValid())
            InvalidateCache(); // if highligher was invalidated then invalidate the cache too
    }

    if (IsCacheInvalid() ||
        matrixChanged || 
        LastHScrollOffset != Geom.HScrollOffset)
    {
        pUnderlineDrawing = NULL;
    }

    if (IsCacheInvalid() || 
        param != Param ||
        matrixChanged || 
       !pBatchPackage || !pCacheManager ||
        context.MaskRenderCount > 0 || // if text is used as a mask - don't use batches (!AB)
        LastHScrollOffset != Geom.HScrollOffset ||
       !pCacheManager->VerifyBatchPackage(pBatchPackage, 
                                          context, 
                                          param.TextParam.IsOptRead() ? 0 : heightRatio))
    {
        //printf("\nInvalidate%d matrixChanged%d Readability%d\n", InvalidCache, matrixChanged, GFxTextParam::GetOptRead(textParam));
        ResetCache();
        ValidateCache();
    }

    LastHScrollOffset = Geom.HScrollOffset;

    // Prepare a batch for optimized rendering.
    // if text is used as a mask - don't use batches (!AB)
    UInt numGlyphsInBatch = 0;
    if (!pBatchPackage && context.MaskRenderCount == 0)
    {      
        ClearBatchHasUnderline();
        Iterator     linesIt = BeginVisible(yOffset);
        Geom.ClearCheckPreciseScale();
        // set or clear "in-batch" flags for each glyph
        for(; linesIt.IsVisible(); ++linesIt)
        {
            GFxTextLineBuffer::Line& line = *linesIt;
            GFxTextLineBuffer::GlyphIterator glyphIt = line.Begin();
            for (; !glyphIt.IsFinished(); ++glyphIt)
            {
                GFxTextLineBuffer::GlyphEntry& glyph = glyphIt.GetGlyph();
                glyph.ClearInBatch();

                if (glyphIt.IsUnderline())
                    SetBatchHasUnderline();

                if (glyph.GetIndex() == ~0u || glyph.IsCharInvisible() || glyphIt.HasImage())
                    continue;

                GFxFontResource* presolvedFont = glyphIt.GetFont();
                GASSERT(presolvedFont);

                UInt fontSize = (UInt)glyph.GetFontSize(); //?AB, support for fractional font size(?)
                if (fontSize == 0) 
                    continue;

                const GFxTextureGlyphData* tgd = presolvedFont->GetTextureGlyphData();
                bool useGlyphTextures = false;
                if (tgd)
                {
                    Float textScreenHeight = heightRatio * fontSize;
                    Float maxGlyphHeight   = GFxFontPackParams::GetTextureGlyphMaxHeight(presolvedFont);
                    useGlyphTextures       = ((textScreenHeight <= maxGlyphHeight * maxRasterScale) || 
                                               presolvedFont->GlyphShapesStripped());
                    Geom.SetCheckPreciseScale();
                }
                else
                if (dynamicCacheEnabled && pCacheManager)
                {
                    GRectF b;
                    useGlyphTextures = 
                        pCacheManager->GlyphFits(presolvedFont->GetGlyphBounds(glyph.GetIndex(), &b),
                                                 fontSize, 
                                                 heightRatio,
                                                 param.TextParam,
                                                 maxRasterScale);
                }

                if (useGlyphTextures)
                {
                    glyph.SetInBatch();
                    ++numGlyphsInBatch;
                }
                else
                    Geom.SetCheckPreciseScale();
            }
        }

        Param = param;

        linesIt = BeginVisible(yOffset);
        GPointF offset = Geom.VisibleRect.TopLeft();
        offset.y += yOffset;

        // Create batches.
        // Value numGlyphsInBatch is just a hint to the allocator that
        // helps avoid extra reallocs. 
        if (pCacheManager)
        {
            linesIt.SetHighlighter(phighlighter);
            pBatchPackage = pCacheManager->CreateBatchPackage(GMemory::pGlobalHeap->GetAllocHeap(this),
                                                              pBatchPackage, 
                                                              linesIt, 
                                                              context, 
                                                              offset,
                                                              &Geom,
                                                              param,
                                                              numGlyphsInBatch);
        }
        Geom.Translation.x = matrix.M_[0][2];
        Geom.Translation.y = matrix.M_[1][2];
    }

    bool maskApplied = false;

#ifdef DEBUG_VIS_RECT
    DrawRect(prenderer, mat, Geom.VisibleRect, GColor(0, 0, 233, 128));
#endif

#ifdef DEBUG_LINE_WIDTHS
    // Draw bounds around every line
    {Iterator linesIt = BeginVisible(yOffset);
    for(; linesIt.IsVisible(); ++linesIt)
    {
        const Line& line = *linesIt;
        GPointF offset = line.GetOffset();
        offset += Geom.VisibleRect.TopLeft();
        offset.y += yOffset;

        GRectF rect(offset.x, offset.y, offset.x + line.GetWidth(), offset.y + line.GetHeight());
        DrawRect(prenderer, mat, rect, GColor(0, 222, 0, 128));
    }}
#endif //DEBUG_LINE_WIDTHS

    // Display batches
    if (pCacheManager)
    {
        pCacheManager->DisplayBatchPackage(pBatchPackage, 
                                           context, 
                                           displayMatrix,
                                           cxform);
    }

    if (!pCacheManager || 
         pCacheManager->IsVectorRenderingRequired(pBatchPackage))
    {
        // check for partially visible (vertically) line
        // if there is one - apply mask right here
        if (IsPartiallyVisible(yOffset))
        {
            maskApplied = ApplyMask(context, matrix, Geom.VisibleRect);
        }

        // We apply textColor through CXForm because that works for EdgeAA too.
        GRenderer::Cxform finalCxform = cxform;
        GColor prevColor(0u);
        Iterator linesIt = BeginVisible(yOffset);
        GRectF glyphBounds;
        for(; linesIt.IsVisible(); ++linesIt)
        {
            Line& line = *linesIt;
            GPointF offset;
            offset.x = (Float)line.GetOffsetX();
            offset.y = (Float)line.GetOffsetY();
            offset += Geom.VisibleRect.TopLeft();
            offset.y += yOffset + line.GetBaseLineOffset();
            SInt advance = 0;
            GlyphIterator glyphIt = line.Begin(phighlighter);
            for (; !glyphIt.IsFinished(); ++glyphIt, offset.x += advance)
            {
                const GlyphEntry& glyph = glyphIt.GetGlyph();

                advance = glyph.GetAdvance();
                if (glyph.IsInBatch() || (glyph.IsCharInvisible() && !glyphIt.IsUnderline() && !glyphIt.HasImage()))
                    continue;

                Float scale = 1.0f;
                Float approxSymW = 0;
                UInt  index = glyph.GetIndex();
                GFxTextImageDesc* pimage = NULL;
                GFxFontResource* presolvedFont = NULL;
                GPtr<GFxShapeBase> pglyphShape; 

                if (glyphIt.HasImage())
                {
                    pimage = glyphIt.GetImage();
                    approxSymW = pimage->GetScreenWidth();
                }
                else
                {
                    // render glyph as shape
                    Float fontSize = (Float)PixelsToTwips(glyph.GetFontSize());
                    scale = fontSize / 1024.0f; // the EM square is 1024 x 1024   

                    presolvedFont = glyphIt.GetFont();
                    GASSERT(presolvedFont);

                    if (pCacheManager)
                    {
                        // apply glyph offsets if they were provided by fontmap into the fonthandle...
                        GFxFontHandle* pfontHandle = glyphIt.GetFontHandle();
                        Float h = presolvedFont->GetAscent() + presolvedFont->GetDescent();
                        Float offx = pfontHandle->GetGlyphOffsetX() * h;
                        Float offy = pfontHandle->GetGlyphOffsetY() * h;
                        if (index != ~0u)
                            pglyphShape = *pCacheManager->GetGlyphShape(presolvedFont, index, 0,
                                                                       glyphIt.IsFauxBold() | param.TextParam.IsFauxBold(), 
                                                                       glyphIt.IsFauxItalic() | param.TextParam.IsFauxItalic(),
                                                                       offx, offy, param.TextParam.GetOutline(),
                                                                       context.pLog);
                        if (pglyphShape)
                            approxSymW = pglyphShape->GetBound().Right * scale;
                    }
                    else
                    {
                        if (index != ~0u)
                            pglyphShape = *presolvedFont->GetGlyphShape(index, 0);

                        if (!IsStaticText()) // no mask or partial visibility test for static text
                        {
                            approxSymW = presolvedFont->GetGlyphBounds(index, &glyphBounds).Right * scale;
                        }
                        else
                        {
                            // For static text GetGlyphBounds is expensive, and since we do not 
                            // need it, just don't get it for static text.
                            approxSymW = 0;
                        }
                    }
                }
                Float adjox = offset.x - Geom.HScrollOffset;

                // check for invisibility/partial visibility of the glyph in order to determine
                // necessity of setting mask.
                if (!IsStaticText()) // no mask or partial visibility test for static text
                {
                    // test for complete invisibility, left side
                    if (index != ~0u && adjox + approxSymW <= Geom.VisibleRect.Left)
                        continue;

                    // test for complete invisibility, right side
                    if (adjox >= Geom.VisibleRect.Right)
                        break; // we can finish here
                }
                if (glyphIt.IsUnderline())
                {
                    if (!glyph.IsNewLineChar())
                        hasUnderline = true;
                    if (glyph.IsCharInvisible())
                        continue;
                }

                // test for partial visibility
                if (!maskApplied && !IsStaticText() && !Geom.IsNoClipping() &&
                    ((adjox < Geom.VisibleRect.Left && adjox + approxSymW > Geom.VisibleRect.Left) || 
                    (adjox < Geom.VisibleRect.Right && adjox + approxSymW > Geom.VisibleRect.Right)))
                {
                    maskApplied = ApplyMask(context, matrix, Geom.VisibleRect);
                }

                Matrix  m(mat);

                m.PrependTranslation(offset.x, offset.y);

                if (pimage)
                {
                    if (pimage->pImageShape)
                    {                        
                        m.Prepend(pimage->Matrix);

                        // draw image
                        GFxDisplayParams params(context, m, cxform, GRenderer::Blend_None);
                        pimage->pImageShape->GetFillAndLineStyles(&params);
                        pimage->pImageShape->Display(params, false, 0);
                    }
                    continue;
                }

                GColor color(glyphIt.GetColor());
                if (color != prevColor)
                {
                    GRenderer::Cxform   colorCxform;
                    colorCxform.M_[0][0] = (1.0f / 255.0f) * color.GetRed();
                    colorCxform.M_[1][0] = (1.0f / 255.0f) * color.GetGreen();
                    colorCxform.M_[2][0] = (1.0f / 255.0f) * color.GetBlue();
                    colorCxform.M_[3][0] = (1.0f / 255.0f) * color.GetAlpha();

                    finalCxform = cxform;
                    finalCxform.Concatenate(colorCxform);
                }

                m.PrependScaling(scale);

                if (index == ~0u)
                {
                    // Invalid glyph; render it as an empty box.
                    prenderer->SetCxform(cxform);
                    prenderer->SetMatrix(m);
                    GColor transformedColor = cxform.Transform(glyphIt.GetColor());
                    prenderer->LineStyleColor(transformedColor);

                    // The EM square is 1024x1024, but usually isn't filled up.
                    // We'll use about half the width, and around 3/4 the height.
                    // Values adjusted by eye.
                    // The Y baseline is at 0; negative Y is up.
                    static const SInt16 EmptyCharBox[5 * 2] =
                    {
                        32,     32,
                        480,    32,
                        480,    -656,
                        32,     -656,
                        32,     32
                    };

                    prenderer->SetVertexData(EmptyCharBox, 5, GRenderer::Vertex_XY16i);
                    prenderer->DrawLineStrip(0, 4);
                    prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
                }
                else
                {               
                    // Draw the GFxCharacter using the filled outline.
                    // MA TBD: Is 0 ok for mask in characters?
                    if (pglyphShape)
                    {
                        // Blend arg doesn't matter because no stroke is used for glyphs.
                        pglyphShape->SetNonZeroFill(true);
                        GFxDisplayParams params(context, m, finalCxform, GRenderer::Blend_None);
                        params.FillStylesNum = 1;
                        params.pFillStyles = &DummyStyle;

                        pglyphShape->Display(params, false, 0);
                        //pglyphShape->Display(context, m, finalCxform, GRenderer::Blend_None,
                        //    0, DummyStyle, DummyLineStyle, 0);
                    }
                }
            }
        }
    }

    // draw underline if necessary
    if ((hasUnderline || HasBatchUnderline()) && !pUnderlineDrawing)
    {
        Iterator linesIt = BeginVisible(yOffset);
        //linesIt.SetHighlighter(phighlighter);

        // calculate the "hscrolled" view rect for clipping
        GRectF hscrolledViewRect(Geom.VisibleRect);
        hscrolledViewRect.OffsetX(Float(Geom.HScrollOffset));

        for(; linesIt.IsVisible(); ++linesIt)
        {
            Line& line = *linesIt;
            GPointF offset;
            offset.x = (Float)line.GetOffsetX();
            offset.y = (Float)line.GetOffsetY();
            offset += Geom.VisibleRect.TopLeft();

            bool newLine = true;

            Float offx = 0, offy = 0, singleUnderlineHeight = 0, singlePointWidth = 0;
            UInt32 lastColor = 0;
            SInt lineLength = 0;
            UInt lastIndex = ~0u;
            GFxTextHighlightInfo::UnderlineStyle lastStyle = GFxTextHighlightInfo::Underline_None;

            Matrix globalMatrixDir;
            Matrix globalMatrixInv;
            SInt advance = 0;
            GlyphIterator glyphIt = line.Begin(phighlighter);
            for (UInt i = 0; !glyphIt.IsFinished(); ++glyphIt, offset.x += advance, ++i)
            {
                const GlyphEntry& glyph = glyphIt.GetGlyph();

                advance = glyph.GetAdvance();
                if (glyphIt.IsUnderline() && !glyphIt.HasImage() && !glyph.IsNewLineChar())
                {
                    Float adjox = offset.x - Geom.HScrollOffset;

                    // check for invisibility/partial visibility of the glyph in order to determine
                    // necessity of setting mask.

                    // test for complete invisibility, right side
                    if (adjox >= Geom.VisibleRect.Right)
                        break; // we can finish here

                    // test for complete invisibility, left side
                    if (adjox + advance < Geom.VisibleRect.Left)
                        continue;

                    // if this is the first visible underlined char in the line - 
                    // do some init stuff
                    if (newLine)
                    {
                        Float descent = Float(line.GetHeight() - line.GetBaseLineOffset());
                        offset.y += yOffset + line.GetBaseLineOffset() + descent/2;

                        // snap to pixel the x and y values
                        //globalMatrixDir = Geom.SourceTextMatrix;

                        globalMatrixDir = mat;

                        globalMatrixDir.Append(context.ViewportMatrix);
                        globalMatrixInv = globalMatrixDir;
                        globalMatrixInv.Invert();

                        GPointF offset1   = globalMatrixDir.Transform(offset);
                        offset1.x = floorf(offset1.x + 0.5f);
                        offset1.y = floorf(offset1.y + 0.5f);
                        offset    = globalMatrixInv.Transform(offset1);
                        offset.x = floorf(offset.x + 0.5f);
                        offset.y = floorf(offset.y + 0.5f);

                        offset1.x += 1;
                        offset1.y += 1;
                        offset1    = globalMatrixInv.Transform(offset1);
                        offset1.x = floorf(offset1.x + 0.5f);
                        offset1.y = floorf(offset1.y + 0.5f);

                        singleUnderlineHeight = offset1.y - offset.y;
                        singlePointWidth      = offset1.x - offset.x;
                        offx = offset.x;
                        offy = offset.y;

                        newLine = false;
                    }

                    UInt32 curColor = glyphIt.GetUnderlineColor().ToColor32();
                    GFxTextHighlightInfo::UnderlineStyle curStyle = glyphIt.GetUnderlineStyle();
                    if (lineLength != 0 && 
                       (lastColor != curColor || lastIndex + 1 != i || curStyle != lastStyle))
                    {
                        DrawUnderline(offx, offy, singleUnderlineHeight, singlePointWidth, hscrolledViewRect, 
                            lineLength, lastStyle, lastColor, globalMatrixDir, globalMatrixInv);

                        lineLength = 0;
                        offx = offset.x;
                    }
                    lineLength += advance;
                    lastColor = curColor;
                    lastIndex = i;
                    lastStyle = curStyle;
                }
            }
            if (lineLength != 0)
            {
                DrawUnderline(offx, offy, singleUnderlineHeight, singlePointWidth, hscrolledViewRect, 
                    lineLength, lastStyle, lastColor, globalMatrixDir, globalMatrixInv);
            }
        }
    }
    if (pUnderlineDrawing)
        pUnderlineDrawing->Display(context, mat, cxform, GRenderer::Blend_None, false);

    // done DisplayRecord
    if (maskApplied)
    {
#ifndef DEBUG_MASK
        RemoveMask(context, matrix, Geom.VisibleRect);
#endif
    }

#ifndef GFC_NO_3D
    if (Text3DVectorizationEnabled)
        text3D.EndDisplay(context);
#endif
}

void    GFxTextLineBuffer::SetFirstVisibleLine(UInt line)
{
    Geom.FirstVisibleLinePos = line;
    InvalidateCache();
}

void    GFxTextLineBuffer::SetHScrollOffset(UInt offset)
{
    Geom.HScrollOffset = offset;
}

GFxTextLineBuffer::Iterator GFxTextLineBuffer::FindLineByTextPos(UPInt textPos)
{
    if (Lines.GetSize() > 0)
    {
        UPInt i = G_LowerBound(Lines, (UInt)textPos, LineIndexComparator::Less);
        if (i == Lines.GetSize())
            --i;
        Line* pline = Lines[(UInt)i];
        UInt lineTextPos = pline->GetTextPos();
        if (textPos >= lineTextPos && textPos <= lineTextPos + pline->GetTextLength())
            return Iterator(*this, (UInt)i);
    }
    return Iterator();
}

GFxTextLineBuffer::Iterator GFxTextLineBuffer::FindLineAtYOffset(Float yoff)
{
    if (Lines.GetSize() > 0)
    {
        UPInt i = G_LowerBound(Lines, yoff, LineYOffsetComparator::Less);
        if (i == Lines.GetSize())
            --i;
        Line* pline = Lines[(UInt)i];
        if (yoff >= pline->GetOffsetY() && yoff < pline->GetOffsetY() + pline->GetHeight() + pline->GetLeading())
        {
            return Iterator(*this, (UInt)i);
        }
    }
    return Iterator();
}

GFxTextLineBuffer::Iterator GFxTextLineBuffer::FindLineAtOffset(const GPointF& p)
{
    if (Lines.GetSize() > 0)
    {
        UPInt i = G_LowerBound(Lines, p.y, LineYOffsetComparator::Less);
        if (i == Lines.GetSize())
            --i;
        Line* pline = Lines[(UInt)i];
        SInt yoff = pline->GetOffsetY();
        while (pline)
        {
            if ( (p.y >= pline->GetOffsetY() && p.y < pline->GetOffsetY() + pline->GetHeight() + pline->GetLeading()) )
            {
                if (p.x >= pline->GetOffsetX() && p.x < pline->GetOffsetX() + pline->GetWidth())
                {
                    return Iterator(*this, (UInt)i);
                }
                if ( (++i) == Lines.GetSize() ) break;
                pline = Lines[(UInt)i];
                if (pline->GetOffsetY() != yoff)
                    break;
            }
            else
                break;
        }
    }
    return Iterator();
}

bool GFxTextLineBuffer::IsLineVisible(UInt lineIndex, Float yOffset) const
{
    const Line& line = *Lines[lineIndex];
    if (lineIndex == Geom.FirstVisibleLinePos)
    {
        // a special case for the first VISIBLE line: display it even if it is only
        // partially visible
        return (line.GetOffsetY() + yOffset <= Geom.VisibleRect.Height() + GFX_TEXT_GUTTER/2); 
    }
    // subtract leading of the line before checking its complete visibility
    return lineIndex > Geom.FirstVisibleLinePos  && 
        (line.GetOffsetY() + line.GetHeight() + yOffset <= Geom.VisibleRect.Height() + GFX_TEXT_GUTTER/2); 
}

bool GFxTextLineBuffer::IsPartiallyVisible(Float yOffset) const
{
    if (Geom.FirstVisibleLinePos < GetSize())
    {
        const Line& line = *Lines[Geom.FirstVisibleLinePos];
        if (line.GetWidth() != 0 && line.GetHeight() != 0)
        {
            // XBox360 VC9 compiler had a problem compiling original complex condition
            // (optimizer generated incorrect code). So, have splitted it by more
            // simple parts.
			Float lh     = (Float)line.GetHeight();
			Float vrectH = Geom.VisibleRect.Height() + GFX_TEXT_GUTTER/2;
			Float yf     = (Float)line.GetOffsetY() + yOffset;
            if (yf <= vrectH && (yf + lh) > vrectH)
			{
// 				bool b1 = (yf <= vrectH);
// 				bool b2 = ((yf + lh) > vrectH);
// 				printf("Mask: %f > %f,  %d && %d, yoff %f, ly  %f, lhei %f, visrHei %f\n", 
// 					float(yf + line.GetHeight()), float(vrectH), 
// 					(int)b1, (int)b2,
// 					float(yOffset), float(yf), float(lh), float(Geom.VisibleRect.Height()));
                return true;
			}
        }
    }
    return false;
}

void GFxTextLineBuffer::Scale(Float scaleFactor)
{
    Iterator it = Begin();
    for (;!it.IsFinished(); ++it)
    {
        Line& line = *it;
        GASSERT(line.IsData32());

        Float newLeading    = Float(line.GetLeading())*scaleFactor;
        Float newW          = Float(line.GetWidth())*scaleFactor;
        Float newH          = Float(line.GetHeight())*scaleFactor;
        line.SetLeading(SInt(newLeading));
        line.SetDimensions(SInt(newW), SInt(newH));
        line.SetBaseLineOffset(line.GetBaseLineOffset()*scaleFactor);
        line.SetOffsetX(SInt(Float(line.GetOffsetX())*scaleFactor));
        line.SetOffsetY(SInt(Float(line.GetOffsetY())*scaleFactor));

        GlyphIterator git = line.Begin();
        for(; !git.IsFinished(); ++git)
        {
            GlyphEntry& ge = git.GetGlyph();
            Float newAdv = Float(ge.GetAdvance())*scaleFactor;
            ge.SetAdvance(SInt(newAdv));
            ge.SetFontSize(ge.GetFontSize()*scaleFactor);
        }
    }
    InvalidateCache();
}

SInt GFxTextLineBuffer::GetMinLineHeight() const
{
    if (GetSize() == 0)
        return 0;
    ConstIterator it = Begin();
    SInt minH = GFC_MAX_SINT;
    for (;!it.IsFinished(); ++it)
    {
        const Line& line = *it;
        SInt h = line.GetHeight();
        if (h < minH)
            minH = h;
    }
    return minH;
}

#ifdef GFC_BUILD_DEBUG
void GFxTextLineBuffer::Dump() const
{
    printf("Dumping lines...\n");
    printf("VisibleRect: { %f, %f, {%f, %f}}\n", 
        Geom.VisibleRect.Left, Geom.VisibleRect.Top, Geom.VisibleRect.Width(), Geom.VisibleRect.Height());
    ConstIterator linesIt = Begin();
    UInt i = 0;
    for(; !linesIt.IsFinished(); ++linesIt)
    {
        const Line& line = *linesIt;
        printf("Line[%d]\n", i++);
        printf("   TextPos = %d, NumGlyphs = %d, Len = %d\n", line.GetTextPos(), line.GetNumGlyphs(), line.GetTextLength());
        printf("   BaseLine = %f, Leading = %d\n", line.GetBaseLineOffset(), line.GetLeading());
        printf("   Bounds = { %d, %d, {%d, %d}}\n", line.GetOffsetX(), line.GetOffsetY(), line.GetWidth(), line.GetHeight());
    }
    printf("...end\n\n");
}

void GFxTextLineBuffer::CheckIntegrity() const
{
    ConstIterator linesIt = Begin();
    UInt nextpos = 0;
    for(; !linesIt.IsFinished(); ++linesIt)
    {
        const Line& line = *linesIt;
        UInt l = 0;
        GlyphIterator git = const_cast<Line&>(line).Begin();
        for (; !git.IsFinished(); ++git)
        {
            const GFxTextLineBuffer::GlyphEntry& glyph = git.GetGlyph();
            l += glyph.GetLength();
        }
        if (!(line.GetTextPos() == nextpos || line.GetTextPos() == nextpos + 1) )
        {
            Dump();
            GASSERT(0);
        }
        nextpos = line.GetTextPos() + l;
    }
}
#endif

void GFx_RecalculateRectToFit16Bit(GRenderer::Matrix& matrix, const GRectF& srcRect, GRectF* pdestRect)
{
    matrix.PrependTranslation(srcRect.Left, srcRect.Top);

    Float xscale = 1, width = srcRect.Width();
    if (width > 32767)
    {
        xscale = width / 32767;
        width = 32767;
    }
    Float yscale = 1, height = srcRect.Height();
    if (height > 32767)
    {
        yscale = height / 32767;
        height = 32767;
    }
    matrix.PrependScaling(xscale, yscale);
    GASSERT(pdestRect);
    pdestRect->Top = pdestRect->Left = 0;
    pdestRect->SetWidth(width);
    pdestRect->SetHeight(height);
}


