/**********************************************************************

Filename    :   GFxFontCacheManager.h
Content     :   
Created     :   
Authors     :   Maxim Shemanarev, Artem Bolgar

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxFontCacheManager_H
#define INC_GFxFontCacheManager_H

#include "GArray.h"
#include "GListAlloc.h"
#include "Text/GFxTextLineBuffer.h" 
#include "GRenderer.h"
#include "GStroker.h"
#include "GCompoundShape.h"

#ifndef GFC_NO_GLYPH_CACHE
#include "GFxGlyphCache.h"
#endif

#include "GRendererEventHandler.h"

//------------------------------------------------------------------------
struct GFxBatchPackageData : public GNewOverrideBase<GFxStatFC_Batch_Mem>
{
    enum { SID = StatType };
    typedef GRenderer::BitmapDesc   BitmapDesc;

    // Batching
    //--------------------------------------------------------------------
    struct BatchInfo
    {   
        UInt                    Index;          // Index of first BitmapDesc
        UInt                    ImageUseCount;  // Expected image use count
        UInt                    Count;          // Running count        
        const GTexture*         pTexture;
        UInt                    DrawFlags;

        void Clear()
        { 
            Count = ImageUseCount = Index = 0; 
            pTexture = 0; 
            DrawFlags = 0;
        }
    };

    //--------------------------------------------------------------------
    struct GlyphVerifier
    {
        GFxGlyphParam    GlyphParam;
#ifndef GFC_NO_GLYPH_CACHE
        GFxGlyphNode*    pGlyph;
        UInt16           FontSize;
        UInt16           GlyphScale;
#endif
        const GTexture*  pTexture;
        UInt             DrawFlags;
        UInt16           TextureWidth;
        UInt16           TextureHeight;
    };

    //--------------------------------------------------------------------
    struct BatchInfoKey
    {
    public:
        const GTexture* pTexture;
        UInt            Layer;      // 0=Shadow, 1=Text

        BatchInfoKey(): pTexture(0) {}
        BatchInfoKey(const GTexture* pt, UInt layer) : pTexture(pt), Layer(layer) {}

        UPInt  operator()(const BatchInfoKey& data) const
        {
            return (((UPInt)data.pTexture) >> 6) ^ 
                     (UPInt)data.pTexture ^
                     (UPInt)data.Layer;
        }
        bool operator== (const BatchInfoKey& bik) const 
        { 
            return pTexture == bik.pTexture && Layer == bik.Layer; 
        }
    };

    typedef GHashLH<BatchInfoKey, BatchInfo,BatchInfoKey, SID> BatchDescHash;

    //--------------------------------------------------------------------
    BatchDescHash                   BatchDesc;
    GArrayLH<BitmapDesc, SID>       Batch;
    GArrayLH<GlyphVerifier, SID>    BatchVerifier;
    GRenderer::CachedData           BatchCache;
    GRenderer::DistanceFieldParams  DistFieldParams;
    bool                            VectorRenderingRequired;
    bool                            FailedGlyphs;

    void Clear() 
    { 
        BatchCache.ReleaseData(GRenderer::Cached_BitmapList);
        BatchDesc.Clear();
        Batch.Resize(0);
        BatchVerifier.Resize(0);
        VectorRenderingRequired = false;
        FailedGlyphs = false;
    }

    GFxBatchPackageData() : 
        VectorRenderingRequired(false), FailedGlyphs(false) {}

    GINLINE ~GFxBatchPackageData()
    {
        BatchCache.ReleaseData(GRenderer::Cached_BitmapList);
    }
};




//------------------------------------------------------------------------
struct GFxBatchPackage : public GListNode<GFxBatchPackage>
{
    GFxBatchPackageData*     Package;
    GFxFontCacheManagerImpl* Owner;
};

//------------------------------------------------------------------------
struct GFxFontHashOp
{
    UPInt operator()(const GFxFontResource* ptr)
    {
        return (((UPInt)ptr) >> 6) ^ (UPInt)ptr;
    }
};


//------------------------------------------------------------------------
class GFxFontCacheManagerImpl : public GNewOverrideBase<GFxStatFC_Mem>
{
    enum 
    { 
#ifndef GFC_NO_GLYPH_CACHE
        SubpixelSizeScale   = GFxGlyphRasterCache::SubpixelSizeScale,
#endif
        ShapePageSize       = 256-2 - 8-4
    };

    class FontDisposeHandler : public GFxFontResource::DisposeHandler
    {
    public:
        void Bind(GFxFontCacheManagerImpl* cache) { pCache = cache; }
        virtual void OnDispose(GFxFontResource* font)
        {
            pCache->CleanUpFont(font);
        }

    private:
        GFxFontCacheManagerImpl* pCache;
    };

    struct RenEventHandler : public GRendererEventHandler
    {
        RenEventHandler() {}

        void Bind(GFxFontCacheManagerImpl* self) { pSelf = self; }
        
        virtual void OnEvent(GRenderer* prenderer, GRendererEventHandler::EventType changeType);

        GFxFontCacheManagerImpl* pSelf;
    };
public: // for Wii compiler v4.3 145
    struct VectorGlyphShape : public GListNode<VectorGlyphShape>
    {
        enum { LockGlyphFlag = 0x80 };

        GFxFontResource*        pFont;
        UInt16                  GlyphIndex;
        UInt8                   HintedGlyphSize;
        UInt8                   Flags;
        UInt8                   Outline;
        GPtr<GFxShapeNoStyles>  pShape;
    };
private:
    struct VectorGlyphKey
    {
        GFxFontResource* pFont;
        UInt16           GlyphIndex;
        UInt8            HintedGlyphSize;
        UInt8            Flags;
        UInt8            Outline;

        VectorGlyphKey() {}
        VectorGlyphKey(GFxFontResource* font, UInt glyphIndex, UInt hintedGlyphSize, UInt flags, UInt8 outline):
            pFont(font), GlyphIndex((UInt16)glyphIndex), 
            HintedGlyphSize((UInt8)hintedGlyphSize), 
            Flags((UInt8)flags & (GFxFont::FF_Bold | GFxFont::FF_Italic)),
            Outline(outline) {}

        UPInt operator()(const VectorGlyphKey& key) const
        {
            return (((UPInt)key.pFont) >> 6) ^ (UPInt)key.pFont ^ 
                     (UPInt)key.GlyphIndex ^
                     (UPInt)key.HintedGlyphSize ^
                     (UPInt)key.Flags ^
                     (UPInt)key.Outline;
        }

        bool operator == (const VectorGlyphKey& key) const 
        { 
            return  pFont           == key.pFont && 
                    GlyphIndex      == key.GlyphIndex &&
                    HintedGlyphSize == key.HintedGlyphSize &&
                    Flags           == key.Flags &&
                    Outline         == key.Outline;
        }
    };

public:
    typedef GRenderer::Matrix       Matrix;
    typedef GRenderer::Cxform       Cxform;
    typedef GRenderer::BitmapDesc   BitmapDec;

    GFxFontCacheManagerImpl(GMemoryHeap* pheap);
   ~GFxFontCacheManagerImpl();

    void                Init(const GFxFontCacheManager::TextureConfig& config);
    void                SetMaxVectorCacheSize(UInt n) { MaxVectorCacheSize = n; }
    void                SetFauxItalicAngle(Float a);
    void                SetFauxBoldRatio(Float r)     { FauxBoldRatio = r; }
    void                SetOutlineRatio(Float r)      { OutlineRatio = r; }
    Float               GetOutlineRatio() const       { return OutlineRatio; }

    void                SetNumLockedFrames(UInt num)  { NumLockedFrames = LockedFrame = num; }

    UInt                GetTextureWidth() const;
    UInt                GetTextureHeight() const;
    GTexture*           GetTexture(UInt i) const;

    // Init glyph textures. In some cases it may be desirable to initialize
    // the textures in advance. The function may be called once before the 
    // main display loop. Without this call the textures will be 
    // initialized on demand as necessary.
    void                InitTextures(GRenderer* ren);

    void                InvalidateAll();
    void                Clear();

    bool                VerifyBatchPackage(const GFxBatchPackage* bp, 
                                           GFxDisplayContext &context,
                                           Float heightRatio);

    GFxBatchPackage*    CreateBatchPackage(GMemoryHeap* pheap,
                                           GFxBatchPackage* bp, 
                                           const GFxTextLineBuffer::Iterator& linesIt, 
                                           GFxDisplayContext &context, 
                                           const GPointF& lnOffset,
                                           GFxLineBufferGeometry* geom,
                                           const GFxTextFieldParam& param,
                                           UInt numGlyphsInBatch);

    void                ReleaseBatchPackage(GFxBatchPackage* bp);
    void                CleanUpFont(GFxFontResource* font);

    void                DisplayBatchPackage(GFxBatchPackage* bp, 
                                            GFxDisplayContext &context, 
                                            const Matrix& displayMatrix,
                                            const Cxform& cx);

    bool                GlyphFits(const GRectF& bounds, 
                                  UInt  fontSize, 
                                  Float heightRatio, 
                                  const GFxGlyphParam& param,
                                  Float maxRasterScale) const;

    bool                IsVectorRenderingRequired(GFxBatchPackage* bp)
    {
        return !bp || (bp->Package && bp->Package->VectorRenderingRequired);
    }

    UInt GetTextureGlyphMaxHeight() const
    {
#ifndef GFC_NO_GLYPH_CACHE
        return Cache.GetMaxGlyphHeight(); 
#else
        return 0;
#endif
    }

    GFxShapeBase* GetGlyphShape(GFxFontResource* font, UInt index, UInt glyphSize,
                                        bool fauxBold, bool fauxItalic,
                                        Float offx, Float offy, UInt outline,
                                        GFxLog* log);
    
    GFxShapeBase* GetGlyphShape_NoLock(GFxFontResource* font, UInt index, UInt glyphSize,
                                              bool fauxBold, bool fauxItalic,
                                              Float offx, Float offy, UInt outline,
                                              GFxLog* log);

    const GRectF& AdjustBounds(GRectF* bounds, bool fauxBold, bool fauxItalic) const;

    void  UnlockAllGlyphs();

    GMemoryHeap* GetHeap() const { return pHeap; }

    UInt GetNumRasterizedGlyphs() const 
    { 
#ifndef GFC_NO_GLYPH_CACHE
        return Cache.GetNumRasterizedGlyphs(); 
#else
        return 0;
#endif
    }

    UInt GetNumTextures() const 
    { 
#ifndef GFC_NO_GLYPH_CACHE
        return Cache.GetNumTextures(); 
#else
        return 0;
#endif
    }

    void VisitGlyphs(GFxGlyphCacheVisitor* visitor) const;
    void SetEventHandler(class GFxGlyphCacheEventHandler* h);
    UInt ComputeUsedArea() const;


private:
    // Prohibit copying
    GFxFontCacheManagerImpl(const GFxFontCacheManagerImpl&);
    const GFxFontCacheManagerImpl& operator = (const GFxFontCacheManagerImpl&);

    void setRenderer(GRenderer* pRenderer);
    bool resolveTextureGlyph(GFxBatchPackageData::GlyphVerifier* gv, 
                             const GFxGlyphParam& gp,
                             bool  canUseRaster,
                             const GFxShapeBase* shape,
                             GFxDisplayContext &context,
                             bool canUseTgd);

    void fillBatchPackage(GFxBatchPackageData* pd, 
                          const GFxTextLineBuffer::Iterator& linesIt, 
                          GFxDisplayContext &context, 
                          const GPointF& lnOffset,
                          GFxLineBufferGeometry* geom,
                          const GFxTextFieldParam& textFieldParam,
                          UInt numGlyphsInBatch);

    bool isOuterContourCW(const GCompoundShape& shape) const;
    void copyAndTransformShape(GFxShapeNoStyles* dst, 
                               const GFxShapeBase* src,
                               GRectF* bbox,
                               bool fauxBold, bool fauxItalic,
                               Float offx, Float offy,
                               UInt outline);

    inline UInt snapFontSizeToRamp(UInt fontSize) const;

    typedef GHashSetLH<
        GFxFontResource*, 
        GFxFontHashOp, 
        GFxFontHashOp, 
        GFxStatFC_Other_Mem>        FontSetType;

    typedef GListAllocLH_POD<
        GFxBatchPackage, 
        127, 
        GFxStatFC_Batch_Mem>        BatchPackageStorageType;

    typedef GList<GFxBatchPackage>  BatchPackageListType;

    typedef GHashLH<
        VectorGlyphKey, 
        VectorGlyphShape*, 
        VectorGlyphKey,
        GFxStatFC_Other_Mem>        VectorGlyphCacheType;

    typedef GListAllocLH<
        VectorGlyphShape, 
        127,
        GFxStatFC_Other_Mem>        VectorGlyphShapeStorageType;

    // FontCache manager gets its own heap.
    GMemoryHeap*                    pHeap;

    GRenderer*                      pRenderer;
    FontDisposeHandler              FontDisposer;
    RenEventHandler                 FrameHandler;
    bool                            LockAllInFrame;
    bool                            RasterCacheWarning;
    bool                            VectorCacheWarning;
    UInt                            NumLockedFrames;
    UInt                            LockedFrame;
    BatchPackageStorageType         BatchPackageStorage;
    BatchPackageListType            BatchPackageQueue;
#ifndef GFC_NO_GLYPH_CACHE
    GFxGlyphRasterCache             Cache;
    static UInt                     FontSizeRamp[];
    UByte                           FontSizeMap[256];
#endif
    FontSetType                     KnownFonts;
    VectorGlyphShapeStorageType     VectorGlyphShapeStorage;
    GList<VectorGlyphShape>         VectorGlyphShapeList;
    VectorGlyphCacheType            VectorGlyphCache;
#ifndef GFC_NO_FXPLAYER_STROKER
    GStroker                        Stroker;
    GCompoundShape                  TmpShape1;
    GCompoundShape                  TmpShape2;
#endif

    UInt                            MaxVectorCacheSize;
    Float                           FauxItalicAngle;
    Float                           FauxBoldRatio;
    Float                           OutlineRatio;
    GMatrix2D                       ItalicMtx;
    mutable GLock                   StateLock;
};


#endif
