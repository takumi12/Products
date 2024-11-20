/**********************************************************************

Filename    :   GRenderer.cpp
Content     :   Vector graphics 2D renderer implementation
Created     :   June 29, 2005
Authors     :   

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GRenderer.h"

#include "GImage.h"
#include "GMath.h"
#include "GStd.h"
#include "GResizeImage.h"
#include "GMsgFormat.h"
#include <stdio.h>
#include <string.h>
#include "GFxAmpServer.h"

#define GFX_STREAM_BUFFER_SIZE  512



// ***** Renderer Stat Constants

// GRenderer Memory.
GDECLARE_MEMORY_STAT_SUM_GROUP(GStatRender_Mem, "Renderer", GStat_Mem)

// Video Memory
GDECLARE_MEMORY_STAT_AUTOSUM_GROUP(GStatRender_VMem, "Video Mem", GStatGroup_Default)
GDECLARE_MEMORY_STAT(GStatRender_Texture_VMem,       "Texture", GStatRender_VMem)
GDECLARE_MEMORY_STAT(GStatRender_Buffer_VMem,        "Buffer",  GStatRender_VMem)

// GRenderer Counters.
GDECLARE_COUNTER_STAT_SUM_GROUP(GStatRender_Counters,   "Counters",         GStatGroup_Default)
GDECLARE_COUNTER_STAT(GStatRender_TextureUpload_Cnt,    "TextureUpload",    GStatRender_Counters)
GDECLARE_COUNTER_STAT(GStatRender_TextureUpdate_Cnt,    "TextureUpdate",    GStatRender_Counters)
GDECLARE_COUNTER_STAT_AUTOSUM_GROUP(GStatRender_DP_Cnt, "DP",               GStatRender_Counters)
GDECLARE_COUNTER_STAT(GStatRender_DP_Line_Cnt,          "Line DP",          GStatRender_DP_Cnt)
GDECLARE_COUNTER_STAT(GStatRender_DP_Triangle_Cnt,      "Triangle DP",      GStatRender_DP_Cnt)
GDECLARE_COUNTER_STAT(GStatRender_Triangle_Cnt,         "Triangle",         GStatRender_Counters)
GDECLARE_COUNTER_STAT(GStatRender_Line_Cnt,             "Line",             GStatRender_Counters)
GDECLARE_COUNTER_STAT(GStatRender_Mask_Cnt,             "Mask",             GStatRender_Counters)
GDECLARE_COUNTER_STAT(GStatRender_Filter_Cnt,           "Filter",           GStatRender_Counters)


// ***** Nested types

//
// cxform
//


GRenderer::Cxform   GRenderer::Cxform::Identity;


// Initialize to identity transform.
GRenderer::Cxform::Cxform()
{
    SetIdentity();
}

// Concatenate c's transform onto ours.  When
// transforming colors, c's transform is applied
// first, then ours.
void    GRenderer::Cxform::Concatenate(const GRenderer::Cxform& c)
{
    M_[0][1] += M_[0][0] * c.M_[0][1];
    M_[1][1] += M_[1][0] * c.M_[1][1];
    M_[2][1] += M_[2][0] * c.M_[2][1];
    M_[3][1] += M_[3][0] * c.M_[3][1];

    M_[0][0] *= c.M_[0][0];
    M_[1][0] *= c.M_[1][0];
    M_[2][0] *= c.M_[2][0];
    M_[3][0] *= c.M_[3][0];
}


// Apply our transform to the given color; return the result.
GColor  GRenderer::Cxform::Transform(const GColor in) const
{
    GColor  result(
        (UByte) G_Clamp<Float>(in.GetRed() * M_[0][0] + M_[0][1], 0, 255),
        (UByte) G_Clamp<Float>(in.GetGreen() * M_[1][0] + M_[1][1], 0, 255),
        (UByte) G_Clamp<Float>(in.GetBlue() * M_[2][0] + M_[2][1], 0, 255),
        (UByte) G_Clamp<Float>(in.GetAlpha() * M_[3][0] + M_[3][1], 0, 255) );

    return result;
}


// Debug log.
void    GRenderer::Cxform::Format(char *pbuffer) const
{

    G_Format(GStringDataPtr(pbuffer, GFX_STREAM_BUFFER_SIZE),
        "    *         +\n"
        "| {0:4.4} {1:4.4}|\n"
        "| {2:4.4} {3:4.4}|\n"
        "| {4:4.4} {5:4.4}|\n"
        "| {6:4.4} {7:4.4}|\n",
        M_[0][0], M_[0][1],
        M_[1][0], M_[1][1],
        M_[2][0], M_[2][1],
        M_[3][0], M_[3][1]
        );
}

void    GRenderer::Cxform::SetIdentity()
{
    M_[0][0] = 1;
    M_[1][0] = 1;
    M_[2][0] = 1;
    M_[3][0] = 1;
    M_[0][1] = 0;
    M_[1][1] = 0;
    M_[2][1] = 0;
    M_[3][1] = 0;
}

bool    GRenderer::Cxform::IsIdentity() const
{
    return (M_[0][0] == 1 &&  M_[1][0] == 1 && M_[2][0] == 1 && M_[3][0] == 1 &&
            M_[0][1] == 0 &&  M_[1][1] == 0 && M_[2][1] == 0 && M_[3][1] == 0);
}

GRenderer::GRenderer()
{
#ifndef GFC_NO_3D
    S3DDisplay = StereoCenter;     // default to non-stereo operation
#endif
}

GRenderer::~GRenderer() 
{
    CallHandlers(GRendererEventHandler::Event_RendererReleased);    
    GRendererEventHandler* pcur = Handlers.GetFirst(), *pnext = pcur;
    if (pcur)
    {
        while (!Handlers.IsNull(pnext)) 
        {
            pcur = pnext;
            pnext = pcur->pNext;
            pcur->RemoveAndClear();
        }
    }
}


// This is a compatibility function. The renderers should call 
// GResizeImageBilinear() directly.
void GRenderer::ResizeImage(UByte* pDst, 
                            int dstWidth, int dstHeight, int dstPitch,
                            const UByte* pSrc, 
                            int srcWidth, int srcHeight, int srcPitch,
                            ResizeImageType type)
{
    switch(type)
    {
    case ResizeRgbToRgb:
        GResizeImageBilinear(pDst, dstWidth, dstHeight, dstPitch,
                             pSrc, srcWidth, srcHeight, srcPitch,
                             GResizeRgbToRgb);
        break;


    case ResizeRgbaToRgba: 
        GResizeImageBilinear(pDst, dstWidth, dstHeight, dstPitch,
                             pSrc, srcWidth, srcHeight, srcPitch,
                             GResizeRgbaToRgba);
        break;

    case ResizeRgbToRgba:
        GResizeImageBilinear(pDst, dstWidth, dstHeight, dstPitch,
                             pSrc, srcWidth, srcHeight, srcPitch,
                             GResizeRgbToRgba);
        break;

    case ResizeGray:
        GResizeImageBilinear(pDst, dstWidth, dstHeight, dstPitch,
                             pSrc, srcWidth, srcHeight, srcPitch,
                             GResizeGray);
        break;
    }
}

void GRenderer::EndFrame()
{
    CallHandlers(GRendererEventHandler::Event_EndFrame);

    // Placing the AMP update call here avoids the need 
    // to explicitly call it every frame from the main loop
    // Make sure that platform-specific renderers call their base class EndFrame
#ifdef GFX_AMP_SERVER
    GFxAmpServer::GetInstance().AdvanceFrame();
#endif
}

#ifndef GFC_NO_3D
// create default view and perspective matrices
void GRenderer::MakeViewAndPersp3D(const GRectF &visFrameRectInTwips, GMatrix3D &matView, GMatrix3D &matPersp, Float perspFOV, bool bInvertY)
{
    const float nearZ = 1;
    const float farZ  = 100000;

    // calculate view matrix
    Float DisplayHeight = fabsf(visFrameRectInTwips.Bottom - visFrameRectInTwips.Top);
    Float DisplayWidth = fabsf(visFrameRectInTwips.Right - visFrameRectInTwips.Left);
    Float fovYAngle = (Float)GFC_DEGTORAD(perspFOV);

    Float centerX = (visFrameRectInTwips.Left + visFrameRectInTwips.Right)/2.f;
    Float centerY = (visFrameRectInTwips.Top + visFrameRectInTwips.Bottom)/2.f;

    Float pixelPerUnitRatio = DisplayWidth*.5f;
    Float eyeZ = (perspFOV <= 0) ? pixelPerUnitRatio : pixelPerUnitRatio/tanf(fovYAngle/2.f);     // focalLength

    // compute view matrix
    GPoint3F vEyePt( centerX, centerY, (-eyeZ < -farZ) ? -farZ : -eyeZ);
    GPoint3F vLookatPt(centerX, centerY, 0);
    GPoint3F vUpVec( 0, bInvertY ? 1.f : -1.f, 0 );
    if (bInvertY)
        matView.ViewLH(vEyePt, vLookatPt, vUpVec );
    else
        matView.ViewRH(vEyePt, vLookatPt, vUpVec );

    if (perspFOV <= 0)
    {
        // make orthographic projection
        if (bInvertY)
            matPersp.OrthoLH(DisplayWidth, DisplayHeight, nearZ, farZ);
        else
            matPersp.OrthoRH(DisplayWidth, DisplayHeight, nearZ, farZ);
    }
    else
    {
        // compute perspective matrix    
        if (bInvertY) 
            matPersp.PerspectiveFocalLengthLH(eyeZ, DisplayWidth, DisplayHeight, nearZ, farZ);
        else
            matPersp.PerspectiveFocalLengthRH(eyeZ, DisplayWidth, DisplayHeight, nearZ, farZ);
    }
}

void GRenderer::Adjust3DMatrixForRT(GMatrix3D &UVPMatrix, const GMatrix3D &oldProjMatrix, const GMatrix2D &oldViewMatrix, const GMatrix2D &newViewMatrix)
{
    GMatrix3D oldView, oldProj;
    //Float oldWidth = oldViewRect.Width;

    Float FrameRectLeft   =  (oldViewMatrix.M_[0][2] + 1.0f) / oldViewMatrix.M_[0][0];
    Float FrameRectTop    = -(oldViewMatrix.M_[1][2] - 1.0f) / oldViewMatrix.M_[1][1];
    Float FrameRectWidth  =  2.0f / oldViewMatrix.M_[0][0];
    Float FrameRectHeight = -2.0f / oldViewMatrix.M_[1][1];

    Float fovRad = 2.0f * atanf(1.0f / oldProjMatrix.M_[0][0]);
    Float fovDeg = (Float)GFC_RADTODEG(fovRad);

    MakeViewAndPersp3D(GRectF(FrameRectLeft,FrameRectTop, GSizeF(FrameRectWidth,FrameRectHeight)),
                       oldView, oldProj, fovDeg, false);

    oldProj.Invert();
    UVPMatrix.Append(oldProj);
    oldView.Invert();
    UVPMatrix.Append(oldView);

    FrameRectLeft   = -(newViewMatrix.M_[0][2] + 1.0f) / newViewMatrix.M_[0][0];
    FrameRectTop    = -(newViewMatrix.M_[1][2] - 1.0f) / newViewMatrix.M_[1][1];
    FrameRectWidth  =  2.0f / newViewMatrix.M_[0][0];
    FrameRectHeight = -2.0f / newViewMatrix.M_[1][1];

    MakeViewAndPersp3D(GRectF(FrameRectLeft,FrameRectTop, GSizeF(FrameRectWidth,FrameRectHeight)),
        oldView, oldProj, fovDeg, false);

    UVPMatrix.Append(oldView);
    UVPMatrix.Append(oldProj);
}
#endif

//////////////////////////////////////////////////////////////////////////
bool    GRenderer::AddEventHandler(GRendererEventHandler *phandler)
{
    if (!phandler->pNext && !phandler->pPrev)
    {
        Handlers.PushBack(phandler);
        phandler->BindRenderer(this);
        return true;
    }
    return false;
}

void    GRenderer::RemoveEventHandler(GRendererEventHandler *phandler)
{
    phandler->RemoveAndClear();
    phandler->pRenderer = NULL;
}

// Routine to call handlers
void    GRenderer::CallHandlers(GRendererEventHandler::EventType event)
{
    GRendererEventHandler* pcur = Handlers.GetFirst(), *pnext = pcur;
    if (pcur)
    {
        while (!Handlers.IsNull(pnext)) 
        {
            pcur = pnext;
            pnext = pcur->pNext;
            pcur->OnEvent(this, event);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void GRendererEventHandler::RemoveFromList()
{
    if (pRenderer)
        pRenderer->RemoveEventHandler(this);
    else
        RemoveAndClear();
}

