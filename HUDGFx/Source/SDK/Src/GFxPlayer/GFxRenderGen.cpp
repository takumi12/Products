/*********************************************************************

Filename    :   GFxRenderGen.cpp
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

#include "GFxRenderGen.h"
#include "GFxLoader.h"
#include "GHeapNew.h"

GFxRenderGen::GFxRenderGen(GMemoryHeap* pheap)
  : VertexArray(pheap), NumTessellatedShapes(0),  
#if defined(GFC_OS_WIN32) || defined(GFC_OS_DARWIN) || defined(GFC_OS_LINUX)
  MemLimit(256*1024)
#else // Use a lower RenderGen limit on consoles.
  MemLimit(64*1024)
#endif
{
}

UPInt GFxRenderGen::GetNumBytes() const
{
    return 
        Shape1.GetNumBytes() +
        Shape2.GetNumBytes() +
#if !defined(GFC_NO_FXPLAYER_STROKER)
        Stroker.GetNumBytes() +
#endif
#if !defined(GFC_NO_FXPLAYER_STROKERAA)
        StrokerAA.GetNumBytes() +
#endif
#ifndef GFC_NO_FXPLAYER_EDGEAA
        EdgeAA_Gen.GetNumBytes() +
#endif
        Tessellator.GetNumBytes() +
        VertexArray.GetNumBytes();
}

void GFxRenderGen::ClearAndRelease()
{
    Shape1.ClearAndRelease();
    Shape2.ClearAndRelease();
#if !defined(GFC_NO_FXPLAYER_STROKER)
    Stroker.ClearAndRelease();
#endif
#if !defined(GFC_NO_FXPLAYER_STROKERAA)
    StrokerAA.ClearAndRelease();
#endif
#ifndef GFC_NO_FXPLAYER_EDGEAA
    EdgeAA_Gen.ClearAndRelease();
#endif
    Tessellator.ClearAndRelease();
    VertexArray.ClearAndRelease();
}
