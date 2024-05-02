/**********************************************************************

Filename    :   GFxMediaInterfaces.h
Content     :   SWF (Shockwave Flash) player library
Created     :   August, 2008
Authors     :   Maxim Didenko

Notes       :   
History     :   

Copyright   :   (c) 1998-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXMEDIAINTERFASES_H
#define INC_GFXMEDIAINTERFASES_H

#include "GConfig.h"
#ifndef GFC_NO_VIDEO

#include "GRefCount.h"
#include "GFxPlayerStats.h"


class GRenderer;
class GTexture;
class GFxVideoCharacter;
class GFxMovieView;

//////////////////////////////////////////////////////////////////////////
// GFxVideoProvider Interface

// Action script 2 has two types of video providers NetStream (for playing 
// video content) and Camera (for capturing video content. 
// For now NetStream is only implemented

class GFxVideoProvider : public GRefCountBaseWeakSupport<GFxVideoProvider, GFxStatMV_Other_Mem>
{
public:
    virtual ~GFxVideoProvider();

    virtual GTexture* GetTexture(SInt* pictureWidth, SInt* pictureHeight, 
                                 SInt* textureWidth, SInt* textureHeight) = 0;

    virtual void      Advance()            = 0;
    virtual void      Display(GRenderer* prenderer)  = 0;
    virtual void      Pause(bool pause)    = 0;
    virtual void      Close()              = 0;

    virtual bool      IsActive()           = 0;
    virtual Float     GetFrameTime()       = 0;

    virtual void               RegisterVideoCharacter(GFxVideoCharacter* pvideo) = 0;
    virtual GFxVideoCharacter* RemoveFirstVideoCharacter() = 0;

};

#endif // GFC_NO_VIDEO

#endif // INC_GFXMEDIAINTERFASES_H
