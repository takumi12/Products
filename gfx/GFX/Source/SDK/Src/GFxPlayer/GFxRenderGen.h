/*********************************************************************

Filename    :   GFxRenderGen.h
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

#ifndef INC_GFxRenderGen_H
#define INC_GFxRenderGen_H

#include "GCompoundShape.h"
#include "GStroker.h"
#include "GStrokerAA.h"
#include "GTessellator.h"
#include "GFxEdgeAAGen.h"

class GFxRenderGen: public GRefCountBaseNTS<GFxRenderGen, GStat_Default_Mem>
{
public:
    GCompoundShape      Shape1;
    GCompoundShape      Shape2;
#if !defined(GFC_NO_FXPLAYER_STROKER)
    GStroker            Stroker;
#endif
    GTessellator        Tessellator;
#if !defined(GFC_NO_FXPLAYER_STROKERAA)
    GStrokerAA          StrokerAA;
#endif
#ifndef GFC_NO_FXPLAYER_EDGEAA
    GFxEdgeAAGenerator  EdgeAA_Gen;
#endif
    GFxVertexArray      VertexArray;
    UInt                NumTessellatedShapes;
    UPInt               MemLimit;

    GFxRenderGen(GMemoryHeap* pheap);

    UPInt   GetMemLimit() const     { return MemLimit; }
    void    SetMemLimit(UPInt lim)  { MemLimit = lim;  }

    UPInt   GetNumBytes() const;
    void    ClearAndRelease();
};


class GFxRenderGenStroker : public GRefCountBaseNTS<GFxRenderGenStroker, GStat_Default_Mem>
{
#if !defined(GFC_NO_FXPLAYER_STROKER)
public:
    GStroker        Stroker;
    GCompoundShape  Shape;
    GCompoundShape  Stroke;
#endif
};

class GFxRenderGenShape : public GRefCountBaseNTS<GFxRenderGenStroker, GStat_Default_Mem>
{
public:
    GCompoundShape  Shape;
};

#endif

