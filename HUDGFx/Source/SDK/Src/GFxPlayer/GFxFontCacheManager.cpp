/**********************************************************************

Filename    :   GFxFontCacheManager.cpp
Content     :   
Created     :   
Authors     :   Maxim Shemanarev, Artem Bolgar

Copyright   :   (c) 2001-2010 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxFontCacheManager.h"
#include "AMP/GFxAmpViewStats.h"
#include "GFxTextureFont.h"

//------------------------------------------------------------------------
GFxFontCacheManager::GFxFontCacheManager(bool enableDynamicCache, bool debugHeap):
    GFxState(State_FontCacheManager),
    DynamicCacheEnabled(enableDynamicCache),
    Text3DVectorizationEnabled(false),
    MaxRasterScale(1),
    MaxVectorCacheSize(512),
    FauxItalicAngle(0.28f),
    FauxBoldRatio(0.055f),
    OutlineRatio(0.01f),
    NumLockedFrames(1),
    pCache(0)
{
    InitManager(TextureConfig(), debugHeap);
}

GFxFontCacheManager::GFxFontCacheManager(const TextureConfig& config,
                                         bool enableDynamicCache, bool debugHeap):
    GFxState(State_FontCacheManager),
    DynamicCacheEnabled(enableDynamicCache),
    Text3DVectorizationEnabled(false),
    MaxRasterScale(1),
    MaxVectorCacheSize(512),
    FauxItalicAngle(0.3f),
    FauxBoldRatio(0.055f),
    OutlineRatio(0.01f),
    NumLockedFrames(1),
    pCache(0)
{
    InitManager(config, debugHeap);
}

GFxFontCacheManager::~GFxFontCacheManager()
{
    delete pCache;
}

void GFxFontCacheManager::InitManager(const TextureConfig& config, bool debugHeap)
{   
    // Create a heap for the font manager and tie its lifetime to pCache.
    UInt         heapFlags = debugHeap ? GMemoryHeap::Heap_UserDebug : 0;
    GMemoryHeap::HeapDesc desc(heapFlags);
    desc.HeapId = GHeapId_FontCache;
    GMemoryHeap* pheap = GMemory::GetGlobalHeap()->CreateHeap("_Font_Cache", desc);
    pCache = GHEAP_NEW(pheap) GFxFontCacheManagerImpl(pheap);
    pheap->ReleaseOnFree(pCache);

    SetTextureConfig(config);
}

GMemoryHeap*    GFxFontCacheManager::GetHeap() const
{
    return pCache->GetHeap();
}


void GFxFontCacheManager::SetTextureConfig(const TextureConfig& config)
{
    CacheTextureConfig = config;
    pCache->Init(config);
    pCache->SetMaxVectorCacheSize(MaxVectorCacheSize);
    pCache->SetFauxItalicAngle(FauxItalicAngle);
    pCache->SetFauxBoldRatio(FauxBoldRatio);
    pCache->SetNumLockedFrames(NumLockedFrames);
}

void  GFxFontCacheManager::SetMaxVectorCacheSize(UInt n) 
{ 
    pCache->SetMaxVectorCacheSize(MaxVectorCacheSize = n);
}

void  GFxFontCacheManager::SetFauxItalicAngle(Float a) 
{ 
    pCache->SetFauxItalicAngle(FauxItalicAngle = a); 
}

void  GFxFontCacheManager::SetFauxBoldRatio(Float r) 
{ 
    pCache->SetFauxBoldRatio(FauxBoldRatio = r);
}

void  GFxFontCacheManager::SetOutlineRatio(Float r) 
{ 
    pCache->SetOutlineRatio(OutlineRatio = r);
}

void GFxFontCacheManager::InitTextures(GRenderer* ren)
{
    pCache->InitTextures(ren);
}

void GFxFontCacheManager::SetNumLockedFrames(UInt num)
{
    if (num == 0)
        num = 1;
    NumLockedFrames = num;
    if (pCache)
        pCache->SetNumLockedFrames(num);
}

UInt GFxFontCacheManager::GetNumRasterizedGlyphs() const
{ 
    return pCache ? pCache->GetNumRasterizedGlyphs() : 0;
}

UInt GFxFontCacheManager::GetNumTextures() const
{ 
    return pCache ? pCache->GetNumTextures() : 0;
}

GTexture* GFxFontCacheManager::GetTexture(UInt i) const
{
    return pCache ? pCache->GetTexture(i) : 0;
}

void GFxFontCacheManager::VisitGlyphs(GFxGlyphCacheVisitor* visitor) const
{
    if (pCache)
        pCache->VisitGlyphs(visitor);
}

void GFxFontCacheManager::SetEventHandler(class GFxGlyphCacheEventHandler* h)
{
    if (pCache)
        pCache->SetEventHandler(h);
}

UInt GFxFontCacheManager::ComputeUsedArea() const 
{ 
    return pCache ? pCache->ComputeUsedArea() : 0; 
}

UInt GFxFontCacheManager::ComputeTotalArea() const
{
    return CacheTextureConfig.MaxNumTextures * 
           CacheTextureConfig.TextureWidth * 
           CacheTextureConfig.TextureHeight;
}



#ifndef GFC_NO_GLYPH_CACHE
//UInt GFxFontCacheManagerImpl::FontSizeRamp[] = { 0,1,2,4,6,8,11,16,22,32,40,50,64,76,90,108,128,147,168,194,222,256,0};
UInt GFxFontCacheManagerImpl::FontSizeRamp[] = { 0,0,2,4,6,8,12,16,18,22,26,32,36,40,45,50,56,64,72,80,90,108,128,147,168,194,222,256,0};
//UInt GFxFontCacheManagerImpl::FontSizeRamp[] = { 0,1,2,4,8,16,32,64,128,256,0};
#endif


//------------------------------------------------------------------------
GFxFontCacheManagerImpl::GFxFontCacheManagerImpl(GMemoryHeap* pheap)
:   pHeap(pheap),
    pRenderer(0),
    LockAllInFrame(false),
    RasterCacheWarning(true),
    VectorCacheWarning(true),
    NumLockedFrames(1),
    LockedFrame(1),
    MaxVectorCacheSize(512),
    FauxItalicAngle(0.3f),
    FauxBoldRatio(0.055f),
    OutlineRatio(0.01f)
{
    ItalicMtx = GMatrix2D::Shearing(0, -FauxItalicAngle);

    FontDisposer.Bind(this);
    FrameHandler.Bind(this);
#ifndef GFC_NO_GLYPH_CACHE
    UInt i;
    UInt ramp = 0;
    for (i = 0; i < 256; ++i)
    {
        if (i > FontSizeRamp[ramp + 1])
            ++ramp;
        FontSizeMap[i] = (UByte)ramp;
    }
#endif
}

//------------------------------------------------------------------------
GFxFontCacheManagerImpl::~GFxFontCacheManagerImpl()
{
    Clear();
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::Clear()
{
    GLock::Locker lock(&StateLock);
    InvalidateAll();
    BatchPackageQueue.Clear();
    BatchPackageStorage.ClearAndRelease();
    FontSetType::Iterator it = KnownFonts.Begin();    
    for (; it != KnownFonts.End(); ++it)
    {
        (*it)->RemoveDisposeHandler(&FontDisposer);
    }
    KnownFonts.Clear();
    G_FreeListElements(VectorGlyphShapeList, VectorGlyphShapeStorage);
    VectorGlyphShapeStorage.ClearAndRelease();
    VectorGlyphCache.Clear();
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::InvalidateAll()
{
    GFxBatchPackage* bp = BatchPackageQueue.GetFirst();
    while(!BatchPackageQueue.IsNull(bp))
    {
        delete bp->Package;
        bp->Package = 0;
        bp = bp->pNext;
    }
#ifndef GFC_NO_GLYPH_CACHE
    Cache.Clear();
#endif
    if (pRenderer && LockAllInFrame) 
    {
        pRenderer->RemoveEventHandler(&FrameHandler);
        pRenderer = NULL;
    }
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::CleanUpFont(GFxFontResource* font)
{
    GLock::Locker lock(&StateLock);
    GFxFontResource** knownFont = KnownFonts.Get(font);
    if (knownFont)
    {
#ifndef GFC_NO_GLYPH_CACHE
        Cache.CleanUpFont(font);
#endif
        font->RemoveDisposeHandler(&FontDisposer);
        KnownFonts.Remove(font);
        GFxBatchPackage* bp = BatchPackageQueue.GetFirst();
        while(!BatchPackageQueue.IsNull(bp))
        {
            bool removeFlag = false;
            if (bp->Package)
            {
                for (UInt i = 0; i < bp->Package->BatchVerifier.GetSize(); ++i)
                {
                    if (bp->Package->BatchVerifier[i].GlyphParam.pFont == font)
                    {
                        removeFlag = true;
                        break;
                    }
                }
            }
            if (removeFlag)
            {
                // Invalidate BatchPackage.
                delete bp->Package;
                bp->Package = 0;
            }
            bp = bp->pNext;
        }
    }
    VectorGlyphShape* sh = VectorGlyphShapeList.GetFirst();
    while(!VectorGlyphShapeList.IsNull(sh))
    {
        VectorGlyphShape* next = sh->pNext;
        if (sh->pFont == font)
        {
            VectorGlyphCache.Remove(VectorGlyphKey(sh->pFont, sh->GlyphIndex, sh->HintedGlyphSize, sh->Flags, sh->Outline));
            VectorGlyphShapeList.Remove(sh);
            VectorGlyphShapeStorage.Free(sh);
        }
        sh = next;
    }
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::Init(const GFxFontCacheManager::TextureConfig& config)
{
#ifndef GFC_NO_GLYPH_CACHE
    Cache.Init(config.TextureWidth,
               config.TextureHeight,
               config.MaxNumTextures,
               config.MaxSlotHeight,
               config.SlotPadding,
               config.TexUpdWidth,
               config.TexUpdHeight);
#else
    GUNUSED(config);
#endif
}


#ifndef GFC_NO_GLYPH_CACHE

UInt GFxFontCacheManagerImpl::GetTextureWidth()  const { return Cache.GetTextureWidth(); }
UInt GFxFontCacheManagerImpl::GetTextureHeight() const { return Cache.GetTextureHeight();}
GTexture* GFxFontCacheManagerImpl::GetTexture(UInt i) const { return Cache.GetTexture(i);}

#else

UInt GFxFontCacheManagerImpl::GetTextureWidth()       const { return 0; }
UInt GFxFontCacheManagerImpl::GetTextureHeight()      const { return 0; }
GTexture* GFxFontCacheManagerImpl::GetTexture(UInt)   const { return 0; }

#endif

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::SetFauxItalicAngle(Float a) 
{ 
    FauxItalicAngle = a;
    ItalicMtx = GMatrix2D::Shearing(0, -FauxItalicAngle); 
}


//------------------------------------------------------------------------
inline UInt GFxFontCacheManagerImpl::snapFontSizeToRamp(UInt fontSize) const
{
#ifndef GFC_NO_GLYPH_CACHE
    fontSize = fontSize + ((fontSize + 3) >> 2);
    fontSize =(fontSize <= 255) ? FontSizeRamp[FontSizeMap[fontSize] + 1] : 255;
#endif
    return fontSize;
}

//------------------------------------------------------------------------
bool GFxFontCacheManagerImpl::GlyphFits(const GRectF& bounds, 
                                        UInt  fontSize, 
                                        Float heightRatio, 
                                        const GFxGlyphParam& param,
                                        Float maxRasterScale) const
{
    if (param.BlurX || param.BlurY)
         return true;

    fontSize = UInt(heightRatio * fontSize + 0.5f);
    if (!param.IsOptRead())
        fontSize = snapFontSizeToRamp(fontSize);

    UInt h = (UInt)(ceilf (bounds.Bottom * fontSize / 1024.0f) -
        floorf(bounds.Top    * fontSize / 1024.0f)) + 1;
    return h < GetTextureGlyphMaxHeight() * maxRasterScale;
}


//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::setRenderer(GRenderer* ren)
{
    if (pRenderer != ren)
    {
        if (pRenderer)
            InvalidateAll();

        pRenderer = ren;
        LockAllInFrame = false;

        GRenderer::RenderCaps caps;
        pRenderer->GetRenderCaps(&caps);
        if (caps.CapBits & (GRenderer::Cap_KeepVertexData | GRenderer::Cap_NoTexOverwrite))
        {
            LockAllInFrame = true;
            pRenderer->AddEventHandler(&FrameHandler);
        }
    }
}

//------------------------------------------------------------------------
bool GFxFontCacheManagerImpl::VerifyBatchPackage(const GFxBatchPackage* bp, 
                                                 GFxDisplayContext &context,
                                                 Float heightRatio)
{
    setRenderer(context.GetRenderer());

    // Verify the package and the owner
    if (!bp || bp->Owner != this || !bp->Package || bp->Package->FailedGlyphs)
        return false;

#ifndef GFC_NO_GLYPH_CACHE
    // Verify glyphs
    const GFxBatchPackageData* bpd = bp->Package;

    UInt i;
    bool verifySize = heightRatio != 0;
    for (i = 0; i < bpd->BatchVerifier.GetSize(); ++i)
    {
        const GFxBatchPackageData::GlyphVerifier& gv = bpd->BatchVerifier[i];

        if (verifySize && gv.pGlyph != 0)
        {
            UInt oldSize = gv.GlyphParam.GetFontSize();
            if (oldSize)
            {
                UInt newSize = snapFontSizeToRamp(UInt(gv.FontSize * heightRatio + 0.5f));
                if (newSize != oldSize)
                    return false;
            }
        }
        if (gv.pGlyph)
        {
            if (!Cache.VerifyGlyphAndSendBack(gv.GlyphParam, gv.pGlyph))
                return false;
            if (LockAllInFrame)
                Cache.LockGlyph(gv.pGlyph);
        }
    }
#else
    GUNUSED(heightRatio);
#endif
//printf("*"); // DBG
    return true;
}

//------------------------------------------------------------------------
GFxBatchPackage* 
GFxFontCacheManagerImpl::CreateBatchPackage(GMemoryHeap* pheap,
                                            GFxBatchPackage* bp, 
                                            const GFxTextLineBuffer::Iterator& linesIt, 
                                            GFxDisplayContext &context, 
                                            const GPointF& lnOffset,
                                            GFxLineBufferGeometry* geom,
                                            const GFxTextFieldParam& param,
                                            UInt numGlyphsInBatch)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timer(context.pStats, NativeCodeSwdHandle, Func_GFxFontCacheManagerImpl_CreateBatchPackage, Amp_Profile_Level_Low);
#endif

    GLock::Locker lock(&StateLock);

    if (pheap == 0)
        pheap = pHeap;

    setRenderer(context.GetRenderer());

    if (bp && bp->Owner == this)
    {
        //BatchPackageQueue.SendToBack(bp);
        if (bp->Package)
        {
            if (numGlyphsInBatch)
                bp->Package->Clear();
        }
        else
            bp->Package = GHEAP_NEW(pheap) GFxBatchPackageData;
    }
    else
    {
        if (bp) 
            delete bp->Package;

        bp = BatchPackageStorage.Alloc();
        bp->Owner   = this;
        bp->Package = GHEAP_NEW(pheap) GFxBatchPackageData;
        BatchPackageQueue.PushBack(bp);
    }

    fillBatchPackage(bp->Package, 
                     linesIt, 
                     context, 
                     lnOffset, 
                     geom, 
                     param, 
                     numGlyphsInBatch);
#ifndef GFC_NO_GLYPH_CACHE
    Cache.UpdateTextures(context.GetRenderer());
#endif
    return bp;
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::ReleaseBatchPackage(GFxBatchPackage* bp)
{
    GLock::Locker lock(&StateLock);
    if (bp)
    {
        if (bp->Owner != this)
        {
            delete bp->Package;
            bp->Package = 0;
        }
        else
        {
            delete bp->Package;
            BatchPackageQueue.Remove(bp);
            BatchPackageStorage.Free(bp);
        }
    }
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::DisplayBatchPackage(GFxBatchPackage* bp, 
                                                  GFxDisplayContext &context, 
                                                  const Matrix& displayMatrix,
                                                  const Cxform& cx)
{
    if (bp && bp->Owner == this && bp->Package)
    {
        GFxBatchPackageData* pd = bp->Package;

        if (pd->Batch.GetSize() > 0)
        {
            GRenderer*  prenderer = context.GetRenderer();
            prenderer->SetCxform(cx);

            // Render the batch piece by piece.
            GRenderer::CacheProvider cacheProvider(&pd->BatchCache);

            for (UInt layer = 0; layer < 2; ++layer)
            {
                GFxBatchPackageData::BatchDescHash::Iterator it = pd->BatchDesc.Begin();
                UInt batchIndex;
                for (batchIndex = 0; it != pd->BatchDesc.End(); ++it)
                {
                    if (it->First.Layer == layer)
                    {
                        if (it->Second.DrawFlags)
                        {
                            prenderer->DrawDistanceFieldBitmaps(&pd->Batch[0], (int)pd->Batch.GetSize(),
                                it->Second.Index, it->Second.Count,
                                it->Second.pTexture, displayMatrix, pd->DistFieldParams, &cacheProvider);
                        }
                        else
                            prenderer->DrawBitmaps(&pd->Batch[0], (int)pd->Batch.GetSize(),
                                it->Second.Index, it->Second.Count,
                                it->Second.pTexture, displayMatrix, &cacheProvider);
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------
inline bool
GFxFontCacheManagerImpl::resolveTextureGlyph(GFxBatchPackageData::GlyphVerifier* gv, 
                                             const GFxGlyphParam& gp,
                                             bool canUseRaster,
                                             const GFxShapeBase* shape,
                                             GFxDisplayContext &context,
                                             bool canUseTgd)
{
    gv->GlyphParam    = gp;
#ifndef GFC_NO_GLYPH_CACHE
    gv->pGlyph        = 0;
#else
    GUNUSED(canUseRaster);
#endif
    gv->pTexture      = 0;
    gv->DrawFlags     = 0;
    gv->TextureWidth  = 0;
    gv->TextureHeight = 0;
    GFxTextureGlyphData* tgData = 0;
    
    if (canUseTgd || shape == 0)
        tgData = gp.pFont->GetTextureGlyphData();

    if (tgData)
    {
#ifndef GFC_NO_GLYPH_CACHE
        gv->FontSize      = 0;
        gv->GlyphScale    = 256;
#endif
        const GFxTextureGlyph& tg = tgData->GetTextureGlyph(gp.GlyphIndex);
        GImageInfoBase* pimage = tg.GetImageInfo(gp.pFont->GetBinding());
        if (pimage)
        {
            gv->pTexture      = pimage->GetTexture(context.GetRenderer());
            gv->DrawFlags     = tgData->GetTextureFlags();
            gv->TextureWidth  = (UInt16)pimage->GetWidth();
            gv->TextureHeight = (UInt16)pimage->GetHeight();
        }
    }
#ifndef GFC_NO_GLYPH_CACHE
    else
    {
        gv->pGlyph = Cache.GetGlyph(context.GetRenderer(), 
                                    gp, 
                                    canUseRaster,
                                    shape, 
                                    gv->FontSize, 
                                    context.pLog);
        if (gv->pGlyph)
        {
            gv->pTexture = Cache.GetGlyphTexture(gv->pGlyph);
            Cache.LockGlyph(gv->pGlyph);
        }
        else
        {
            if (shape != 0 && context.pLog && RasterCacheWarning)
                context.pLog->LogWarning("Warning: Increase raster glyph cache capacity - TextureConfig.\n");
            RasterCacheWarning = false;
            return false;
        }
    }
#endif
    return true;
}


//------------------------------------------------------------------------
static bool GFx_ClipBitmapDesc(GFxBatchPackageData::BitmapDesc* bd, 
                               const GRectF& visibleRect)
{
    GRectF clipped = bd->Coords;
    clipped.Intersect(visibleRect);
    if (clipped.IsEmpty())
    {
        bd->Coords = visibleRect;
        bd->Coords.Right = bd->Coords.Left;
        bd->Coords.Bottom = bd->Coords.Top;
        bd->TextureCoords.Right = bd->TextureCoords.Left;
        bd->TextureCoords.Bottom = bd->TextureCoords.Top;
        return false;
    }

    if (clipped != bd->Coords)
    {
        GRectF clippedTexture = bd->TextureCoords;

        if (bd->Coords.Left != clipped.Left)
            clippedTexture.Left = bd->TextureCoords.Left + 
                                 (clipped.Left - bd->Coords.Left) * 
                                  bd->TextureCoords.Width() / 
                                  bd->Coords.Width();

        if (bd->Coords.Top != clipped.Top)
            clippedTexture.Top = bd->TextureCoords.Top + 
                                (clipped.Top - bd->Coords.Top) * 
                                 bd->TextureCoords.Height() / 
                                 bd->Coords.Height();

        if (bd->Coords.Right != clipped.Right)
            clippedTexture.Right = bd->TextureCoords.Right - 
                                  (bd->Coords.Right - clipped.Right) * 
                                   bd->TextureCoords.Width() / 
                                   bd->Coords.Width();

        if (bd->Coords.Bottom != clipped.Bottom)
            clippedTexture.Bottom = bd->TextureCoords.Bottom - 
                                   (bd->Coords.Bottom - clipped.Bottom) * 
                                    bd->TextureCoords.Height() / 
                                    bd->Coords.Height();
        bd->Coords = clipped;
        bd->TextureCoords = clippedTexture;
    }
    return true;    
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::fillBatchPackage(GFxBatchPackageData* pd, 
                                               const GFxTextLineBuffer::Iterator& linesIter, 
                                               GFxDisplayContext &context, 
                                               const GPointF& lnOffset, 
                                               GFxLineBufferGeometry* geom,
                                               const GFxTextFieldParam& textFieldParam,
                                               UInt numGlyphsInBatch)
{
// DBG
//printf("C");

#ifdef GFC_NO_GLYPH_CACHE
    GUNUSED(geom); 
#endif
    GFxFontResource* pprevFont = NULL;
    GFxResourceBindData fontData;

    if (textFieldParam.ShadowColor != 0)
        numGlyphsInBatch *= 2;

    // Value numGlyphsInBatch is just a hint to the allocator that
    // helps avoid extra reallocs. 
    pd->BatchVerifier.Resize(0);    // Avoid possible extra copying
    pd->BatchVerifier.Reserve(numGlyphsInBatch);

    pd->VectorRenderingRequired = false;
    pd->FailedGlyphs = false;

#ifndef GFC_NO_GLYPH_CACHE
    Matrix globalMatrixDir;
    Matrix globalMatrixInv;
    if (textFieldParam.TextParam.IsOptRead())
    {
        globalMatrixDir = geom->SourceTextMatrix;
        globalMatrixDir.Append(context.ViewportMatrix);
        globalMatrixInv = globalMatrixDir;
        globalMatrixInv.Invert();
    }

    // This flag is used to snap the glyphs to pixels in the X direction.
    // If the text is not oriented along the axes (freely rotated) it 
    // does not make sense to snap it.
    bool freeRotation = geom->SourceTextMatrix.IsFreeRotation();
#endif

    UInt pass;
    enum { PassShadow = 0, PassText = 1 };
    UInt passStart = PassText;
    UInt passEnd   = PassText;

    if (textFieldParam.ShadowColor != 0)
    {
        passStart = PassShadow;
        if (textFieldParam.ShadowParam.IsKnockOut() || 
            textFieldParam.ShadowParam.IsHiddenObject())
        {
            passEnd = PassShadow;
        }
    }

    geom->BlurX = 0;
    geom->BlurY = 0;
    UInt layer;

    GFxTextLineBuffer::Iterator linesIt;
    for (pass = passStart; pass <= passEnd; ++pass)
    {
        // When using a single texture it will be a single batch. 
        // If there are many textures and text has a shadow, it is 
        // possible than the shadow will be drawn after text. So that, 
        // it is necessary to break the batches so that they would have
        // separately shadow and glyph bitmaps
        //-----------------------
        layer = passStart;
#ifndef GFC_NO_GLYPH_CACHE
        const GFxGlyphParam& textParam = (pass == PassText)? 
            textFieldParam.TextParam: 
            textFieldParam.ShadowParam;

        if (Cache.GetMaxNumTextures() > 1)
            layer = pass;
#endif

        for(linesIt = linesIter; linesIt.IsVisible(); ++linesIt)
        {
            GFxTextLineBuffer::Line& line = *linesIt;
            GFxTextLineBuffer::GlyphIterator glyphIt = line.Begin();
            for (; !glyphIt.IsFinished(); ++glyphIt)
            {
                GFxTextLineBuffer::GlyphEntry& glyph = glyphIt.GetGlyph();

                if (pass == PassShadow)
                    glyph.ClearShadowInBatch();

                if (glyph.GetIndex() == ~0u)
                {
                    pd->VectorRenderingRequired = true;
                    continue;
                }

                if (glyph.IsCharInvisible())
                    continue;

                if (!glyph.IsInBatch() && pass == PassText)
                {
                    pd->VectorRenderingRequired = true;
                    continue;
                }

                GFxFontResource* presolvedFont = glyphIt.GetFont();
                GASSERT(presolvedFont);

                // Create a BatchDesc entry for each different image.
                if (presolvedFont != pprevFont)
                {
                    //fontData = context.pResourceBinding->GetResourceData(presolvedFont);
                    pprevFont = presolvedFont;
                    if (!KnownFonts.Get(presolvedFont))
                    {
                        KnownFonts.Add(presolvedFont);
                        presolvedFont->AddDisposeHandler(&FontDisposer);
                    }
                }

                GFxGlyphParam glyphParam;

                glyphParam.Clear();
                glyphParam.pFont      = presolvedFont;
                glyphParam.GlyphIndex = UInt16(glyph.GetIndex());

                GFxBatchPackageData::GlyphVerifier gv;
                GPtr<GFxShapeBase> shape;
                bool canUseRaster = false;

#ifndef GFC_NO_GLYPH_CACHE
                Float screenFontSize  = geom->HeightRatio * glyph.GetFontSize();
                if (screenFontSize < 1)
                    screenFontSize = 1;
                UInt  snappedFontSize = UInt(screenFontSize + 0.5f);

                if (glyphParam.pFont->GetTextureGlyphData() == 0)// || pass == PassShadow)
                {
                    canUseRaster = 
                        textParam.IsOptRead() && 
                        textParam.GetBlurX() == 0 && 
                        textParam.GetBlurY() == 0 &&
                        pass == PassText &&
                        glyphParam.pFont->IsHintedRasterGlyph(glyphParam.GlyphIndex, glyphParam.FontSize);

                    // apply glyph offsets if they were provided by fontmap into the fonthandle...
                    GFxFontHandle* pfontHandle = glyphIt.GetFontHandle();
                    Float h = presolvedFont->GetAscent() + presolvedFont->GetDescent();
                    Float offx = pfontHandle->GetGlyphOffsetX() * h;
                    Float offy = pfontHandle->GetGlyphOffsetY() * h;

                    shape = *GetGlyphShape_NoLock(glyphParam.pFont, 
                                                  glyphParam.GlyphIndex, 
                                                 (textParam.IsOptRead() && pass == PassText)? 
                                                        snappedFontSize : 0,
                                                  glyphIt.IsFauxBold() | textParam.IsFauxBold(), 
                                                  glyphIt.IsFauxItalic() | textParam.IsFauxItalic(),
                                                  offx, offy, textParam.GetOutline(),
                                                  context.pLog);
                }

                if (shape)
                {
                    GRectF bounds;
                    if (shape->HasValidBounds())
                        bounds = shape->GetBound();
                    else
                        presolvedFont->GetGlyphBounds(glyph.GetIndex(), &bounds);

                    if (!textParam.IsOptRead())
                        snappedFontSize = snapFontSizeToRamp(snappedFontSize);

                    Float blurX   = textParam.GetBlurX();
                    Float blurY   = textParam.GetBlurY();
                    if (blurX > geom->BlurX) geom->BlurX = blurX;
                    if (blurY > geom->BlurY) geom->BlurY = blurY;

                    Cache.CalcGlyphParam(screenFontSize,
                                         snappedFontSize,
                                         geom->HeightRatio,
                                         bounds.Bottom - bounds.Top,
                                         textParam,
                                         &glyphParam,
                                         &gv.FontSize,
                                         &gv.GlyphScale);

                    if ( textParam.IsOptRead() && 
                        !textParam.IsBitmapFont() &&
                         shape->GetHintedGlyphSize() == 0 &&
                        !canUseRaster &&
                         textParam.BlurX == 0)
                    {
                        Float glyphWidth = (bounds.Right - bounds.Left) * glyphParam.FontSize / 1024.0f;
                        if (3*glyphWidth < Cache.GetMaxGlyphHeight())
                        {
                            glyphParam.SetStretch(true);
                        }
                    }
                }
                else
                {
                    glyphParam.SetFontSize(snappedFontSize);
                    glyphParam.Flags = textParam.Flags;
                    gv.FontSize      = UInt16(snappedFontSize * SubpixelSizeScale);
                    gv.GlyphScale    = 256;
                }
#endif

                if (glyphIt.IsAutoFitDisabled())
                    glyphParam.SetAutoFit(false);
                glyphParam.SetFauxBold  (glyphIt.IsFauxBold() | textParam.IsFauxBold());
                glyphParam.SetFauxItalic(glyphIt.IsFauxItalic() | textParam.IsFauxItalic());
                glyphParam.FlagsEx = textParam.FlagsEx;
                if (!resolveTextureGlyph(&gv, glyphParam, canUseRaster, shape, context, true))//pass == PassText))
                    pd->FailedGlyphs = true;

                if (gv.pTexture)
                {
                    if ((gv.DrawFlags & GFxTextureFont::TF_DistanceFieldAlpha) == 0 || pass == PassText)
                    {
                        if (pass == PassShadow)
                            glyph.SetShadowInBatch();

                        pd->BatchVerifier.PushBack(gv);

                        // Find the batch with the same texture and color.
                        GFxBatchPackageData::BatchInfoKey bik(gv.pTexture, layer);
                        GFxBatchPackageData::BatchInfo* pbi = pd->BatchDesc.Get(bik);
                        if (pbi == NULL)
                        {
                            // Not found? Add new record.
                            GFxBatchPackageData::BatchInfo bi;
                            bi.Clear();
                            bi.pTexture = bik.pTexture;
                            bi.DrawFlags = gv.DrawFlags;
                            bi.ImageUseCount = 1;
                            pd->BatchDesc.Add(bik, bi);
                        }
                        else
                            ++pbi->ImageUseCount;
                    }
                }
                else
                {
                    if (pass == PassText)
                        glyph.ClearInBatch();

                    pd->VectorRenderingRequired = true;
                    geom->SetCheckPreciseScale();
                }
            }
        }
    }  // for(pass...)

    // Distance Field effects.
    bool UseDistFieldShadow = false;
    pd->DistFieldParams.ShadowColor = textFieldParam.ShadowColor;
    if (pd->DistFieldParams.ShadowColor != 0)
    {
        UseDistFieldShadow = true;

        Float blurSize = G_Min(3.f, textFieldParam.ShadowParam.GetBlurX());
        pd->DistFieldParams.ShadowWidth = 6.f * blurSize;
        pd->DistFieldParams.ShadowOffset = GPointF(textFieldParam.ShadowOffsetX*0.05f, textFieldParam.ShadowOffsetY*0.05f);
        Float Offset2 = pd->DistFieldParams.ShadowOffset.DistanceSquared();
        if (Offset2 > 4)
        {
            Float Offset = sqrtf(Offset2);
            pd->DistFieldParams.ShadowOffset.x *= (2.f/Offset);
            pd->DistFieldParams.ShadowOffset.y *= (2.f/Offset);
        }
    }
    pd->DistFieldParams.GlowColor = textFieldParam.GlowColor;
    if (pd->DistFieldParams.GlowColor != 0)
    {
        UseDistFieldShadow = true;
        pd->DistFieldParams.GlowSize[0] = 0;
        pd->DistFieldParams.GlowSize[1] = GFxTextFilter::Fixed44toFloat(textFieldParam.GlowSize);
    }

    // Assign start indices to batches.
    GFxBatchPackageData::BatchDescHash::Iterator it = pd->BatchDesc.Begin();
    UInt batchIndex;
    for (batchIndex = 0; it != pd->BatchDesc.End(); ++it)
    {
        it->Second.Index = batchIndex;
        batchIndex += it->Second.ImageUseCount;
    }

    // Allocate bitmap coordinate batch.
    pd->Batch.Resize(0);            // Avoid possible extra copying
    pd->Batch.Resize(pd->BatchVerifier.GetSize());

    // Assign texture coordinates to right slots in the batch.
    UInt gvIdx = 0;
    for (pass = passStart; pass <= passEnd; ++pass)
    {
        layer = passStart;

        GRectF visibleRect = geom->VisibleRect;

        if (textFieldParam.ShadowColor.GetAlpha() != 0)
        {
            if (pass == PassShadow || UseDistFieldShadow)
            {
                // Extend visual rectangle in case of shadow/glow.
                visibleRect.Left   -= geom->BlurX * 20;
                visibleRect.Top    -= geom->BlurY * 20;
                visibleRect.Right  += geom->BlurX * 20;
                visibleRect.Bottom += geom->BlurY * 20;
            }
            if (pass == PassShadow)
            {
                visibleRect.Left   += (Float)textFieldParam.ShadowOffsetX;
                visibleRect.Top    += (Float)textFieldParam.ShadowOffsetY;
                visibleRect.Right  += (Float)textFieldParam.ShadowOffsetX;
                visibleRect.Bottom += (Float)textFieldParam.ShadowOffsetY;
            }
        }

#ifndef GFC_NO_GLYPH_CACHE
        const GFxGlyphParam& textParam = (pass == PassText)? 
            textFieldParam.TextParam: 
            textFieldParam.ShadowParam;

        if (Cache.GetMaxNumTextures() > 1)
            layer = pass;
#endif

        for(linesIt = linesIter; linesIt.IsVisible(); ++linesIt)
        {
            GFxTextLineBuffer::Line& line = *linesIt;
            GPointF offset;
            offset.x = Float(line.GetOffsetX());
            offset.y = Float(line.GetOffsetY());

            offset += lnOffset;
            offset.x -= Float(geom->HScrollOffset);
            offset.y += line.GetBaseLineOffset();
            SInt advance = 0;

#ifndef GFC_NO_GLYPH_CACHE
            if (textParam.IsOptRead())
            {
                offset   = globalMatrixDir.Transform(offset);
                offset.x = floorf(offset.x + 0.5f);
                offset.y = floorf(offset.y + 0.5f);
                offset   = globalMatrixInv.Transform(offset);
            }
#endif
            GFxTextLineBuffer::GlyphIterator glyphIt = line.Begin(linesIt.GetHighlighter());
            for (; !glyphIt.IsFinished(); ++glyphIt, offset.x += advance)
            {
                GFxTextLineBuffer::GlyphEntry& glyph = glyphIt.GetGlyph();
                advance = glyph.GetAdvance();

                if (glyph.GetIndex() == ~0u)
                {
                    pd->VectorRenderingRequired = true;
                    continue;
                }

                if (glyph.IsCharInvisible())
                    continue;

                if (pass == PassShadow)
                {
                    if (!glyph.IsShadowInBatch())
                        continue;
                }
                else
                {
                    if (!glyph.IsInBatch())
                    {
                        pd->VectorRenderingRequired = true;
                        continue;
                    }
                }

                // do batching
                GFxBatchPackageData::GlyphVerifier& gv = pd->BatchVerifier[gvIdx++];

                // Find the batch to which the glyph belongs to.
                GFxBatchPackageData::BatchInfoKey bik(gv.pTexture, layer);
                GFxBatchPackageData::BatchInfo* pbi = pd->BatchDesc.Get(bik);
                GASSERT(pbi != NULL);

                GFxBatchPackageData::BitmapDesc& bd = pd->Batch[pbi->Index + pbi->Count];
                Float scale;

#ifndef GFC_NO_GLYPH_CACHE
                bool snapX  = false;
                if (gv.pGlyph)
                {
                    if (!LockAllInFrame)
                        Cache.UnlockGlyph(gv.pGlyph);

                    const GFxGlyphRect&   r = gv.pGlyph->Rect;
                    const GPoint<SInt16>& o = gv.pGlyph->Origin;

                    bd.TextureCoords.Left   = (r.x + 1) * Cache.GetScaleU();
                    bd.TextureCoords.Top    = (r.y + 1) * Cache.GetScaleV();
                    bd.TextureCoords.Right  =  bd.TextureCoords.Left + (r.w - 2) * Cache.GetScaleU();
                    bd.TextureCoords.Bottom =  bd.TextureCoords.Top  + (r.h - 2) * Cache.GetScaleV();

                    UInt stretch = gv.GlyphParam.GetStretch();
                    bd.Coords.Left   = (r.x + 1 - o.x) * 20.0f / stretch;
                    bd.Coords.Top    = (r.y + 1 - o.y) * 20.0f;
                    bd.Coords.Right  = bd.Coords.Left + (r.w - 2) * 20.0f / stretch;
                    bd.Coords.Bottom = bd.Coords.Top  + (r.h - 2) * 20.0f;
                    snapX = stretch == 1 && !freeRotation && gv.GlyphParam.BlurX == 0;

                    if (gv.GlyphParam.IsOptRead())
                    {
                        scale = 1.0f / (Float(gv.GlyphScale) * geom->HeightRatio / 256.0f);
                    }
                    else
                    {
                        scale = Float(glyph.GetFontSize() * SubpixelSizeScale) / Float(gv.FontSize);
                        snapX = false;
                    }

                    bd.Coords.Left   *= scale;
                    bd.Coords.Top    *= scale;
                    bd.Coords.Right  *= scale;
                    bd.Coords.Bottom *= scale;

                    if (pass == PassShadow)
                    {
                        bd.Coords.Left   += (Float)textFieldParam.ShadowOffsetX;
                        bd.Coords.Top    += (Float)textFieldParam.ShadowOffsetY;
                        bd.Coords.Right  += (Float)textFieldParam.ShadowOffsetX;
                        bd.Coords.Bottom += (Float)textFieldParam.ShadowOffsetY;
                    }
                    gv.FontSize = (UInt16)glyph.GetFontSize();
                }
                else
#endif
                {
                    const GFxTextureGlyphData *gd = gv.GlyphParam.pFont->GetTextureGlyphData();
                    const GFxTextureGlyph& tg = gd->GetTextureGlyph(glyph.GetIndex());

                    // Scale from uv coords to the 1024x1024 glyph square.
                    // @@ need to factor this out!
                    scale = PixelsToTwips(glyph.GetFontSize()) / 1024.0f; // the EM square is 1024 x 1024   
                    Float textureXScale = gd->GetTextureGlyphScale() * scale * gv.TextureWidth;
                    Float textureYScale = gd->GetTextureGlyphScale() * scale * gv.TextureHeight;
                    bd.Coords         = tg.UvBounds;
                    bd.TextureCoords  = tg.UvBounds;
                    bd.Coords        -= tg.UvOrigin;
                    bd.Coords.Left   *= textureXScale;
                    bd.Coords.Top    *= textureYScale;
                    bd.Coords.Right  *= textureXScale;
                    bd.Coords.Bottom *= textureYScale;

                    if (UseDistFieldShadow && gv.DrawFlags)
                    {
                        Float pad = (Float)5*(gd->GetCharPadding()-2);
                        Float blur = G_Min<Float>(pad, pd->DistFieldParams.ShadowWidth * 20.f/6.f);
                        Float offsetX = blur + G_Min<Float>(pad-blur, pd->DistFieldParams.ShadowOffset.x * 20.f);
                        Float offsetY = blur + G_Min<Float>(pad-blur, pd->DistFieldParams.ShadowOffset.y * 20.f);

                        bd.Coords.Left   -= blur;
                        bd.Coords.Top    -= blur;
                        bd.Coords.Right  += offsetX;
                        bd.Coords.Bottom += offsetY;

                        bd.TextureCoords.Left   -= blur / textureXScale;
                        bd.TextureCoords.Top    -= blur / textureYScale;
                        bd.TextureCoords.Right  += offsetX / textureXScale;
                        bd.TextureCoords.Bottom += offsetY / textureYScale;
                    }
                }

                GPointF p(offset);
#ifndef GFC_NO_GLYPH_CACHE
                if (snapX)
                {
                    p  = globalMatrixDir.Transform(p);
                    p.x = floorf(p.x + 0.5f);
                    p  = globalMatrixInv.Transform(p);
                }
#endif
                bd.Coords += p;
                bd.Color   = (pass == PassText) ? glyphIt.GetColor() : textFieldParam.ShadowColor;
// DBG
//bd.Color   = (pass == PassText) ? 0xFFFFFFFF : textFieldParam.ShadowColor;

                // The following logic simulates Flash shadow and glow clipping.
                // It's not possible to achieve 100% visual compatibility, so that,
                // it's a compromise between performance and visual consistency. 
                //
                // The idea is as follows.
                // All glyphs bitmaps are stored unclipped. Clipping is applied
                // to the bitmap rectangles. In case of glow/shadow we extend the 
                // visual rectangle. But it may draw extra glow/shadow glyphs. 
                // The best trade-off is to draw glow/shadow in case if more than a half
                // of it is visible in the original (non extended) visual rectangle.
                //--------------------------
                if (pass == PassText)
                {
                    if (!geom->IsNoClipping())
                        GFx_ClipBitmapDesc(&bd, visibleRect);
                }
                else
                {
                    GFxBatchPackageData::BitmapDesc tmp = bd;
                    GFx_ClipBitmapDesc(&tmp, geom->VisibleRect);
                    if (tmp.Coords.Width()*2 > bd.Coords.Width())
                    {
                        GFx_ClipBitmapDesc(&bd, visibleRect);
                    }
                    else
                    {
                        bd.Coords = geom->VisibleRect;
                        bd.Coords.Right = bd.Coords.Left;
                        bd.Coords.Bottom = bd.Coords.Top;
                        bd.TextureCoords.Right = bd.TextureCoords.Left;
                        bd.TextureCoords.Bottom = bd.TextureCoords.Top;
                    }
                }
                pbi->Count++;
            }
        }
    } // for(pass...)
}


//------------------------------------------------------------------------
bool GFxFontCacheManagerImpl::isOuterContourCW(const GCompoundShape& shape) const
{
    UInt i, j;

    Float minX1 =  1e10f;
    Float minY1 =  1e10f;
    Float maxX1 = -1e10f;
    Float maxY1 = -1e10f;
    Float minX2 =  1e10f;
    Float minY2 =  1e10f;
    Float maxX2 = -1e10f;
    Float maxY2 = -1e10f;
    bool cw = true;

    for(i = 0; i < shape.GetNumPaths(); ++i)
    {
        const GCompoundShape::SPath& path = shape.GetPath(i);
        if(path.GetNumVertices() > 2)
        {
            GPointType v1 = path.GetVertex(path.GetNumVertices() - 1);
            Float sum = 0;
            for(j = 0; j < path.GetNumVertices(); ++j)
            {
                const GPointType& v2 = path.GetVertex(j);
                if(v2.x < minX1) minX1 = v2.x;
                if(v2.y < minY1) minY1 = v2.y;
                if(v2.x > maxX1) maxX1 = v2.x;
                if(v2.y > maxY1) maxY1 = v2.y;
                sum += v1.x * v2.y - v1.y * v2.x;
                v1 = v2;
            }

            if(minX1 < minX2 || minY1 < minY2 || maxX1 > maxX2 || maxY1 > maxY2)
            {
                minX2 = minX1;
                minY2 = minY1;
                maxX2 = maxX1;
                maxY2 = maxY1;
                cw = sum > 0;
            }
        }
    }
    return cw;
}


//------------------------------------------------------------------------
const GRectF& GFxFontCacheManagerImpl::AdjustBounds(GRectF* bounds, 
                                                    bool fauxBold, bool fauxItalic) const
{
    if (fauxBold)
    {
        Float fauxBoldWidth = FauxBoldRatio * 1024.0f;
        bounds->Left  -= fauxBoldWidth;
        bounds->Right += fauxBoldWidth;
    }
    if (fauxItalic)
    {
        GPointF p1, p2;
        p1.x = bounds->Left;
        p1.y = bounds->Top;
        p2.x = bounds->Right;
        p2.y = bounds->Bottom;
        p1 = ItalicMtx.Transform(p1);
        p2 = ItalicMtx.Transform(p2);
        bounds->Left  = p1.x;
        bounds->Right = p2.x;
    }
    return *bounds;
}


//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::copyAndTransformShape(GFxShapeNoStyles* dst, 
                                                    const GFxShapeBase* src,
                                                    GRectF* bbox,
                                                    bool fauxBold, bool fauxItalic,
                                                    Float offx, Float offy, UInt outline)
{
    if (!src)
        return;
    GFxPathPacker path;
#ifndef GFC_NO_FXPLAYER_STROKER
    Float fauxBoldWidth = FauxBoldRatio * 1024.0f;
    GPtr<GFxShapeNoStyles> fauxBoldShape;
    if (fauxBold)
    {
        fauxBoldShape = *new GFxShapeNoStyles(ShapePageSize * 4);
        src->MakeCompoundShape(&TmpShape1, 10);
        TmpShape1.ScaleAndTranslate(1, 1000, 0, 0);
        Stroker.SetWidth(isOuterContourCW(TmpShape1) ? fauxBoldWidth : -fauxBoldWidth);
        Stroker.SetLineJoin(GStrokerTypes::MiterJoin);
        TmpShape2.Clear();
        Stroker.GenerateContour(TmpShape1, -1, TmpShape2);
        UInt i, j;
        for(i = 0; i < TmpShape2.GetNumPaths(); ++i)
        {
            const GCompoundShape::SPath& path2 = TmpShape2.GetPath(i);
            if (path2.GetNumVertices() > 2)
            {
                const GPointType& p = path2.GetVertex(0);
                path.Reset();
                path.SetFill0(1);
                path.SetFill1(0);
                path.SetMoveTo(SInt(p.x), SInt(p.y * 0.001f));
                for(j = 1; j < path2.GetNumVertices(); ++j)
                {
                    const GPointType& p = path2.GetVertex(j);
                    path.LineToAbs(SInt(p.x), SInt(p.y * 0.001f));
                }
                fauxBoldShape->AddPath(&path);
            }
        }
        src = fauxBoldShape;
    }
    else
    if (outline > 0.0f)
    {
        fauxBoldShape = *new GFxShapeNoStyles(ShapePageSize * 4);
        src->MakeCompoundShape(&TmpShape1, 10);
        Stroker.SetWidth(Float(outline) * OutlineRatio * 1024.0f);
        Stroker.SetLineJoin(GStrokerTypes::MiterJoin);
        TmpShape2.Clear();
        Stroker.GenerateStroke(TmpShape1, -1, TmpShape2);
        UInt i, j;
        for(i = 0; i < TmpShape2.GetNumPaths(); ++i)
        {
            const GCompoundShape::SPath& path2 = TmpShape2.GetPath(i);
            if (path2.GetNumVertices() > 2)
            {
                const GPointType& p = path2.GetVertex(0);
                path.Reset();
                path.SetFill0(1);
                path.SetFill1(0);
                path.SetMoveTo(SInt(p.x), SInt(p.y));
                for(j = 1; j < path2.GetNumVertices(); ++j)
                {
                    const GPointType& p = path2.GetVertex(j);
                    path.LineToAbs(SInt(p.x), SInt(p.y));
                }
                fauxBoldShape->AddPath(&path);
            }
        }
        src = fauxBoldShape;
    }

    

#endif //#ifndef GFC_NO_FXPLAYER_STROKER

    GPtr<GFxShapeBase::PathsIterator> ppathsIt = *src->GetPathsIterator();
    GFxShapeBase::PathsIterator::StateInfo info;

    GPointF p1, p2;

    while(ppathsIt->GetNext(&info))
    {
        switch (info.State)
        {
        case GFxShapeBase::PathsIterator::StateInfo::St_Setup:
            {
                Float ax = info.Setup.MoveX, ay = info.Setup.MoveY;

                path.Reset();
                path.SetFill0(1);
                path.SetFill1(0);

                p1.x = ax + offx;
                p1.y = ay + offy;
                if (fauxItalic)
                    p1 = ItalicMtx.Transform(p1);
                path.SetMoveTo(SInt(p1.x), SInt(p1.y));
            }
            break;
        case GFxShapeBase::PathsIterator::StateInfo::St_Edge:
            if (info.Edge.Curve)
            {
                p1.x = info.Edge.Cx + offx;
                p1.y = info.Edge.Cy + offy;
                p2.x = info.Edge.Ax + offx;
                p2.y = info.Edge.Ay + offy;
                if (fauxItalic)
                {
                    p1 = ItalicMtx.Transform(p1);
                    p2 = ItalicMtx.Transform(p2);
                }
                path.CurveToAbs(SInt(p1.x), SInt(p1.y), SInt(p2.x), SInt(p2.y));
            }
            else
            {
                p1.x = info.Edge.Ax + offx;
                p1.y = info.Edge.Ay + offy;
                if (fauxItalic)
                    p1 = ItalicMtx.Transform(p1);
                path.LineToAbs(SInt(p1.x), SInt(p1.y));
            }
            break;
        case GFxShapeBase::PathsIterator::StateInfo::St_NewPath:
        case GFxShapeBase::PathsIterator::StateInfo::St_NewShape:
            dst->AddPath(&path);
            break;
        default:;
        }
    }

    if (fauxBold || fauxItalic)
    {
        dst->ComputeBound(bbox);
    }
}

//------------------------------------------------------------------------
GFxShapeBase* 
GFxFontCacheManagerImpl::GetGlyphShape(GFxFontResource* font, UInt index, UInt size, 
                                       bool fauxBold, bool fauxItalic,
                                       Float offx, Float offy, UInt outline,
                                       GFxLog* log)
{
//return font->GetGlyphShape(index, 0); // DBG
    GLock::Locker guard(&StateLock);
    return GetGlyphShape_NoLock(font,index, size, fauxBold, fauxItalic, offx, offy, outline, log);
}
GFxShapeBase* 
GFxFontCacheManagerImpl::GetGlyphShape_NoLock(GFxFontResource* font, UInt index, UInt size, 
                                              bool fauxBold, bool fauxItalic,
                                              Float offx, Float offy, UInt outline,
                                              GFxLog* log)
{
//return font->GetGlyphShape(index, 0); // DBG
    if (!KnownFonts.Get(font))
    {
        KnownFonts.Add(font);
        font->AddDisposeHandler(&FontDisposer);
    }

    // At this point the font size makes sense only if the font
    // supports native hinting. Otherwise it will be just a waste 
    // of cache capacity. Also, check for the maximum hinted size.
    if (!font->HasNativeHinting() ||
        !font->IsHintedVectorGlyph(index, size))
    {
        size = 0;
    }

    UInt flags = (fauxBold   ? GFxFont::FF_Bold : 0) |
                 (fauxItalic ? GFxFont::FF_Italic : 0);

    VectorGlyphShape** shapePtr = VectorGlyphCache.Get(VectorGlyphKey(font, index, size, flags, UInt8(outline)));
    if (shapePtr)
    {
        VectorGlyphShapeList.SendToBack(*shapePtr);
        if (LockAllInFrame)
            (*shapePtr)->Flags |= VectorGlyphShape::LockGlyphFlag;
        (*shapePtr)->pShape->AddRef();
        return (*shapePtr)->pShape;
    }

    GPtr<GFxShapeBase> srcShapePtr = *font->GetGlyphShape(index, size);
    if (srcShapePtr.GetPtr() == 0)
        return 0;

    VectorGlyphShape* shape;
    if (VectorGlyphCache.GetSize() >= MaxVectorCacheSize)
    {
        shape = VectorGlyphShapeList.GetFirst();
        if (shape->Flags & VectorGlyphShape::LockGlyphFlag)
        {
            if (log && VectorCacheWarning)
                log->LogWarning("Warning: Increase vector glyph cache capacity - SetMaxVectorCacheSize().\n");
            VectorCacheWarning = false;
            return 0;
        }

        VectorGlyphCache.Remove(VectorGlyphKey(shape->pFont, 
                                               shape->GlyphIndex, 
                                               shape->HintedGlyphSize,
                                               shape->Flags,
                                               shape->Outline));
        VectorGlyphShapeList.Remove(shape);
        VectorGlyphShapeStorage.Free(shape);
    }

    shape = VectorGlyphShapeStorage.Alloc();
    shape->pFont            = font;
    shape->GlyphIndex       = (UInt16)index;
    shape->HintedGlyphSize  = (UInt8)size;
    shape->Flags            = (UInt8)flags;
    shape->Outline          = (UInt8)outline;
    shape->pShape           = *GHEAP_NEW(pHeap) GFxShapeNoStyles(ShapePageSize);

    GRectF bbox;

    if (srcShapePtr->GetHintedGlyphSize())
        srcShapePtr->ComputeBound(&bbox);
    else
        font->GetGlyphBounds(index, &bbox);

    copyAndTransformShape(shape->pShape, srcShapePtr, &bbox, fauxBold, fauxItalic, offx, offy, outline);
    shape->pShape->SetBound(bbox);
    shape->pShape->SetValidBoundsFlag(true);
    shape->pShape->SetNonZeroFill(true);
    shape->pShape->SetHintedGlyphSize(srcShapePtr->GetHintedGlyphSize());

    VectorGlyphShapeList.PushBack(shape);
    VectorGlyphCache.Add(VectorGlyphKey(font, index, size, flags, UInt8(outline)), shape);

    if (LockAllInFrame)
        shape->Flags |= VectorGlyphShape::LockGlyphFlag;

    shape->pShape->AddRef();
    return shape->pShape;
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::UnlockAllGlyphs()
{
    GLock::Locker guard(&StateLock);

    if (LockAllInFrame)
    {
        if (--LockedFrame == 0)
        {
            LockedFrame = NumLockedFrames;
            VectorGlyphShape* shape = VectorGlyphShapeList.GetFirst();
            while(!VectorGlyphShapeList.IsNull(shape))
            {
                shape->Flags &= ~VectorGlyphShape::LockGlyphFlag;
                shape = VectorGlyphShapeList.GetNext(shape);
            }
#ifndef GFC_NO_GLYPH_CACHE
            Cache.UnlockAllGlyphs();
#endif
        }
    }
    RasterCacheWarning = true;
    VectorCacheWarning = true;
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::RenEventHandler::OnEvent(GRenderer* prenderer, GRendererEventHandler::EventType eventType)
{
    pSelf->UnlockAllGlyphs();
    if (eventType == GRendererEventHandler::Event_RendererReleased)
    {
        if (pSelf->pRenderer && pSelf->pRenderer == prenderer)
        {
            pSelf->pRenderer->RemoveEventHandler(&pSelf->FrameHandler);
            pSelf->pRenderer = NULL;
        }
    }
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::InitTextures(GRenderer* ren)
{
    setRenderer(ren);
#ifndef GFC_NO_GLYPH_CACHE
    Cache.InitTextures(ren);
#endif
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::VisitGlyphs(GFxGlyphCacheVisitor* visitor) const
{
    GUNUSED(visitor);
#ifndef GFC_NO_GLYPH_CACHE
    GLock::Locker locker(&StateLock);
    Cache.VisitGlyphs(visitor);
#endif
}

//------------------------------------------------------------------------
void GFxFontCacheManagerImpl::SetEventHandler(class GFxGlyphCacheEventHandler* h)
{
    GUNUSED(h);
#ifndef GFC_NO_GLYPH_CACHE
    GLock::Locker locker(&StateLock);
    Cache.SetEventHandler(h);
#endif
}

//------------------------------------------------------------------------
UInt GFxFontCacheManagerImpl::ComputeUsedArea() const
{
#ifndef GFC_NO_GLYPH_CACHE
    GLock::Locker locker(&StateLock);
    return Cache.ComputeUsedArea();
#else
    return 0;
#endif
}




