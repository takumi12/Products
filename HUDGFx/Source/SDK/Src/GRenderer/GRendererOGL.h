/**********************************************************************

Filename    :   GRendererOGL.h
Content     :   Vector graphics 2D renderer implementation for 
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


#ifndef INC_GRENDEREROGL_H
#define INC_GRENDEREROGL_H

#include "GTypes.h"
#include "GRefCount.h"

#include "GRenderer.h"
#include "GRendererCommonImpl.h"


class GTextureOGL : public GTextureImplNode
{
public:
    GTextureOGL () {}
    GTextureOGL (GRendererNode *plistRoot) : GTextureImplNode(plistRoot) { }

    virtual bool    InitTexture(GImageBase* pim, UInt usage = Usage_Wrap)   = 0; 
    virtual bool    InitTexture(UInt texID, bool deleteTexture = 1)  = 0;
    virtual UInt    GetNativeTexture() const                                                      = 0;
};

class GRenderTargetOGL : public GRenderTargetImplNode
{
public:
    GRenderTargetOGL () {}
    GRenderTargetOGL (GRendererNode *plistRoot) : GRenderTargetImplNode(plistRoot) { }

    virtual bool       InitRenderTarget(GTexture *ptarget, GTexture* pdepth = 0, GTexture* pstencil = 0) = 0;
    virtual bool       InitRenderTarget(UInt FboId) = 0;
};


// Base class for OpenGL Renderer implementation.
// This class will be replaced with a new shape-centric version in the near future.

class GRendererOGL : public GRenderer
{
public:
    typedef GTextureOGL       TextureType;
    typedef UInt              NativeTextureType;
    typedef UInt              NativeRenderTargetType;

    // Creates a new renderer object
    static GRendererOGL* GSTDCALL CreateRenderer();

    // Returns created objects with a refCount of 1, must be user-released.
    virtual GTextureOGL* CreateTexture()                                                                = 0;    
    virtual GTextureOGL* CreateTextureYUV()                                                             = 0;

    virtual GTextureOGL* PushTempRenderTarget(const GRectF& frameRect, UInt targetW, UInt targetH, bool wantStencil = 0)      = 0;

    // *** Implement Dependent Video Mode configuration
    
    // Call SetDependentVideoMode with the desired gl context (HGLRC, GLXContext, etc.) current.
    // This context must be current for all further calls to this GRendererOGL instance.
    // If the context is deleted or reset externally, a new GRendererOGL instance must be created.
    virtual bool                SetDependentVideoMode()
    {
        GASSERT(0);
        return 0;
    }
                                        

    // No effect - Video mode is managed by the application
    virtual bool                ResetVideoMode()
    {
        GASSERT(0);
        return 0;
    }

    
    // Query display status
    enum DisplayStatus
    {
        DisplayStatus_Ok            = 0,
        DisplayStatus_NoModeSet     = 1,    // Video mode 
        DisplayStatus_Unavailable   = 2,    // Display is unavailable for rendering; check status again later
        DisplayStatus_NeedsReset    = 3     // May be returned in Dependent mode to indicate external action being required
    };

    virtual DisplayStatus       CheckDisplayStatus() const                                              = 0;


};
    


#endif 
