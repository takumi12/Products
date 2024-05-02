#ifndef INC_GSCREENTOWORLD_H
#define INC_GSCREENTOWORLD_H

/*****************************************************************

Filename    :   GScreenToWorld.h
Content     :   Screen to World reverse transform helper class 
Created     :   Jan 15, 2010
Authors     :   Mustafa Thamer

History     :   
Copyright   :   (c) 2010 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GMatrix3D.h"

//
// This class computes a screen to world transform.
// It requires the following inputs to be set:
//      normalized screen coords - these are from the mouse generally and should be -1 to 1
//      view, perspective and word matrices - these are what are set in the renderer and on the object
//
class GScreenToWorld
{
public:

    GINLINE GScreenToWorld() : Sx(FLT_MAX), Sy(FLT_MAX), LastX(FLT_MAX), LastY(FLT_MAX),
        MatProj(NULL), MatView(NULL), MatWorld(NULL) { }

    // required inputs
    GINLINE void SetNormalizedScreenCoords(Float nsx, Float nsy) { Sx=nsx; Sy=-nsy; }   // note: the Y is inverted
    GINLINE void SetView(const GMatrix3D &mView) { MatView = &mView; }
    GINLINE void SetPerspective(const GMatrix3D &mProj) { MatProj = &mProj; }
    GINLINE void SetWorld(const GMatrix3D &mWorld) { MatWorld = &mWorld; }

    // computes the answer
    void GetWorldPoint(GPointF *ptOut);
    void GetWorldPoint(GPoint3F *ptOut);
    
    GPointF GetLastWorldPoint() const { return GPointF(LastX, LastY); }
private:
    void VectorMult(float *po, const float *pa, float x, float y, float z, float w);
    void VectorMult(float *po, const float *pa, const float *v);
    void VectorInvHomog(float *v);

    Float Sx, Sy;
    Float LastX, LastY;
    const GMatrix3D *MatProj;
    const GMatrix3D *MatView;
    const GMatrix3D *MatWorld;
};

#endif  // INC_GSCREENTOWORLD_H
