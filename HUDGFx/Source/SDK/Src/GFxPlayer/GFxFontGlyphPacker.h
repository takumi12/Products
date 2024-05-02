/**********************************************************************

Filename    :   GFxFontGlyphPacker.h
Content     :   GFxFontGlyphPacker implementation declaration
Created     :   6/14/2007
Authors     :   Maxim Shemanarev, Artyom Bolgar

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxFontGlyphPacker_H
#define INC_GFxFontGlyphPacker_H

#include "GArray.h"
#include "GTypes2DF.h"
#include "GFxLoader.h"
#include "GFxFontResource.h"

#ifndef GFC_NO_FONT_GLYPH_PACKER
#include "GCompoundShape.h"
#include "GRectPacker.h"
#include "GRasterizer.h"
#include "GFxShape.h"

//------------------------------------------------------------------------
class GFxFontGlyphPacker : public GRefCountBaseNTS<GFxFontGlyphPacker, GStat_Default_Mem>
{
    // This is for keeping track of our rendered glyphs, before
    // packing them into textures and registering with the GFxFontResource.
    struct GlyphInfo
    {
        GFxFontResource*    pFont;
        UInt     GlyphIndex;
        UInt     GlyphReuse;
        UInt     TextureIdx;
        GRectF   Bounds;
        GPointF  Origin;
    };

    struct GlyphGeometryKey
    {
        const GFxFontResource*      pFont;
        const GFxShapeBase*         pShape;
        UInt32                      Hash;

        GlyphGeometryKey(): pFont(0), pShape(0), Hash(0) {}
        GlyphGeometryKey(const GFxFontResource* font, 
                         const GFxShapeBase* shape, 
                         UInt32 hash): 
            pFont(font), pShape(shape), Hash(hash) {}
        UPInt operator()(const GlyphGeometryKey& data) const
        {
            return (UPInt)data.Hash ^ 
                  ((UPInt)data.pFont >> 6) ^ 
                   (UPInt)data.pFont;
        }
        bool operator== (const GlyphGeometryKey& cmpWith) const 
        { 
            return pFont == cmpWith.pFont && 
                   pShape->IsEqualGeometry(*cmpWith.pShape);
        }
    };

public:
    GFxFontGlyphPacker(GFxFontPackParams* params,
                       GFxImageCreator *pimageCreator,
                       GFxRenderConfig *prenderConfig,
                       GFxLog* plog,
                       GFxResourceId* ptextureIdGen,
                       GMemoryHeap* fontHeap,
                       bool threadedLoading);
   ~GFxFontGlyphPacker();

    void GenerateFontBitmaps(const GArray<GFxFontResource*>& fonts);


private:
    // Prohibit Copying
    GFxFontGlyphPacker(const GFxFontGlyphPacker&);
    const GFxFontGlyphPacker& operator = (const GFxFontGlyphPacker&);

    void generateGlyphInfo(GArray<GlyphInfo>* glyphs, GFxFontResource* f);
    UInt packGlyphRects(GArray<GlyphInfo>* glyphs, 
                        UInt start, UInt end, UInt texIdx);
    UInt packGlyphRects(GArray<GlyphInfo>* glyphs);
    void rasterizeGlyph(GImage* texImage, GlyphInfo* gi);
    void generateTextures(GArray<GlyphInfo>* glyphs, UInt numTextures);

    typedef GHashLH<GlyphGeometryKey, UInt, GlyphGeometryKey,  GStat_Default_Mem> GlyphGeometryHashType;

    GFxFontPackParams*                  pFontPackParams;
    // Texture glyph configuration cached from FontPackParams.
    GFxFontPackParams::TextureConfig    PackTextureConfig;

    // Texture id generator, specified externally.
    GFxResourceId*                      pTextureIdGen;
    GPtr<GFxImageCreator>               pImageCreator;
    GPtr<GFxRenderConfig>               pRenderConfig; // Passed for in CreateImage.
    GPtr<GFxLog>                        pLog;
    GMemoryHeap*                        pFontHeap;

    GRectPacker                         Packer;
    GRasterizer                         Rasterizer;
    GCompoundShape                      CompoundShape;
    GlyphGeometryHashType               GlyphGeometryHash;
    bool                                ThreadedLoading;
};

#endif // GFC_NO_FONT_GLYPH_PACKER



// Builds cached glyph textures from shape info.    
extern void GFx_GenerateFontBitmaps(GFxFontPackParams *params,
                                    const GArray<GFxFontResource*>& fonts,
                                    GFxImageCreator *pimageCreator,
                                    GFxRenderConfig *prenderConfig,
                                    GFxLog* plog,
                                    GFxResourceId* pidGenerator,
                                    GMemoryHeap* fontHeap,
                                    bool threadedLoading);

#endif // INC_GFXFONTLIBIMPL_H
