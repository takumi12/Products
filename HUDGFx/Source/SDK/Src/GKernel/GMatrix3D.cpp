/*****************************************************************

Filename    :   GMatrix3D.cpp
Content     :   3D Matrix class implementation 
Created     :   January 15, 2010
Authors     :   Mustafa Thamer

History     :   
Copyright   :   (c) 2010 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GMatrix3D.h"

#define M(x,r,c) x[(r)*4+(c)]

// static
GMatrix3D   GMatrix3D::Identity;

Float GMatrix3D::Cofactor(const Float *pa, int i, int j) const
{
    const int subs[4][3] = {{1,2,3},{0,2,3},{0,1,3},{0,1,2}};

#define SUBM(m,a,b) M(m,subs[i][a],subs[j][b])

    Float a;
    a =  SUBM(pa, 0,0) * SUBM(pa, 1,1) * SUBM(pa, 2,2);
    a += SUBM(pa, 1,0) * SUBM(pa, 2,1) * SUBM(pa, 0,2);
    a += SUBM(pa, 2,0) * SUBM(pa, 0,1) * SUBM(pa, 1,2);
    a -= SUBM(pa, 0,0) * SUBM(pa, 2,1) * SUBM(pa, 1,2);
    a -= SUBM(pa, 1,0) * SUBM(pa, 0,1) * SUBM(pa, 2,2);
    a -= SUBM(pa, 2,0) * SUBM(pa, 1,1) * SUBM(pa, 0,2);

    return ((i + j) & 1) ? -a : a;
}

void GMatrix3D::MatrixInverse(Float *po, const Float *pa) const
{
    Float c[16];
    Float det = 0;

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            M(c,i,j) = Cofactor(pa, i,j);

    for (int i = 0; i < 4; i++)
        det += M(c, 0,i) * M(pa, 0,i);

    if (det == 0.0f)
    {
        // Not invertible - this happens sometimes (ie. sample6.Swf)
        // Arbitrary fallback, set to identity and negate translation
        memcpy(po, GMatrix3D::Identity.M_, sizeof(GMatrix3D::Identity.M_));  
        po[12] = -pa[12];
        po[13] = -pa[13];
        po[14] = -pa[14];
    }
    else
    {
        det = 1.0f / det;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                M(po, j,i) = det * M(c, i,j);
    }
}

//////////////////////////////////////////////////////////////////////////

GMatrix3D::GMatrix3D(const GMatrix2D &m)
{
    M_[0][0] = m.M_[0][0];   // SX
    M_[1][0] = m.M_[0][1];
    M_[2][0] = 0;
    M_[3][0] = m.M_[0][2];   // TX

    M_[0][1] = m.M_[1][0];
    M_[1][1] = m.M_[1][1];   // SY
    M_[2][1] = 0;
    M_[3][1] = m.M_[1][2];   // TY

    M_[0][2] = 0;
    M_[1][2] = 0;
    M_[2][2] = 1.0f;
    M_[3][2] = 0;

    M_[0][3] = 0;
    M_[1][3] = 0;
    M_[2][3] = 0;
    M_[3][3] = 1.0f;
}

bool    GMatrix3D::IsValid() const
{
    return GFC_ISFINITE(M_[0][0])
        && GFC_ISFINITE(M_[0][1])
        && GFC_ISFINITE(M_[0][2])
        && GFC_ISFINITE(M_[0][3])

        && GFC_ISFINITE(M_[1][0])
        && GFC_ISFINITE(M_[1][1])
        && GFC_ISFINITE(M_[1][2])
        && GFC_ISFINITE(M_[1][3])
        
        && GFC_ISFINITE(M_[2][0])
        && GFC_ISFINITE(M_[2][1])
        && GFC_ISFINITE(M_[2][2])
        && GFC_ISFINITE(M_[2][3])
        
        && GFC_ISFINITE(M_[3][0])
        && GFC_ISFINITE(M_[3][1])
        && GFC_ISFINITE(M_[3][2])
        && GFC_ISFINITE(M_[3][3]);
}

void GMatrix3D::Transpose()
{
    GMatrix3D matDest;
    int i,j;
    for(i=0;i<4;i++)
        for(j=0;j<4;j++)
            matDest.M_[j][i] = M_[i][j];
    *this = matDest;
}

void GMatrix3D::SetIdentity()
{
    Clear();

    M_[0][0] = 1;
    M_[1][1] = 1;
    M_[2][2] = 1;
    M_[3][3] = 1;
}

void GMatrix3D::MultiplyMatrix(const GMatrix3D &matrixA, const GMatrix3D &matrixB)
{
//    SetIdentity();

    M_[0][0] = matrixA.M_[0][0] * matrixB.M_[0][0] + matrixA.M_[0][1] * matrixB.M_[1][0] + matrixA.M_[0][2] * matrixB.M_[2][0] + matrixA.M_[0][3] * matrixB.M_[3][0];
    M_[0][1] = matrixA.M_[0][0] * matrixB.M_[0][1] + matrixA.M_[0][1] * matrixB.M_[1][1] + matrixA.M_[0][2] * matrixB.M_[2][1] + matrixA.M_[0][3] * matrixB.M_[3][1];
    M_[0][2] = matrixA.M_[0][0] * matrixB.M_[0][2] + matrixA.M_[0][1] * matrixB.M_[1][2] + matrixA.M_[0][2] * matrixB.M_[2][2] + matrixA.M_[0][3] * matrixB.M_[3][2];
    M_[0][3] = matrixA.M_[0][0] * matrixB.M_[0][3] + matrixA.M_[0][1] * matrixB.M_[1][3] + matrixA.M_[0][2] * matrixB.M_[2][3] + matrixA.M_[0][3] * matrixB.M_[3][3];
    M_[1][0] = matrixA.M_[1][0] * matrixB.M_[0][0] + matrixA.M_[1][1] * matrixB.M_[1][0] + matrixA.M_[1][2] * matrixB.M_[2][0] + matrixA.M_[1][3] * matrixB.M_[3][0];
    M_[1][1] = matrixA.M_[1][0] * matrixB.M_[0][1] + matrixA.M_[1][1] * matrixB.M_[1][1] + matrixA.M_[1][2] * matrixB.M_[2][1] + matrixA.M_[1][3] * matrixB.M_[3][1];
    M_[1][2] = matrixA.M_[1][0] * matrixB.M_[0][2] + matrixA.M_[1][1] * matrixB.M_[1][2] + matrixA.M_[1][2] * matrixB.M_[2][2] + matrixA.M_[1][3] * matrixB.M_[3][2];
    M_[1][3] = matrixA.M_[1][0] * matrixB.M_[0][3] + matrixA.M_[1][1] * matrixB.M_[1][3] + matrixA.M_[1][2] * matrixB.M_[2][3] + matrixA.M_[1][3] * matrixB.M_[3][3];
    M_[2][0] = matrixA.M_[2][0] * matrixB.M_[0][0] + matrixA.M_[2][1] * matrixB.M_[1][0] + matrixA.M_[2][2] * matrixB.M_[2][0] + matrixA.M_[2][3] * matrixB.M_[3][0];
    M_[2][1] = matrixA.M_[2][0] * matrixB.M_[0][1] + matrixA.M_[2][1] * matrixB.M_[1][1] + matrixA.M_[2][2] * matrixB.M_[2][1] + matrixA.M_[2][3] * matrixB.M_[3][1];
    M_[2][2] = matrixA.M_[2][0] * matrixB.M_[0][2] + matrixA.M_[2][1] * matrixB.M_[1][2] + matrixA.M_[2][2] * matrixB.M_[2][2] + matrixA.M_[2][3] * matrixB.M_[3][2];
    M_[2][3] = matrixA.M_[2][0] * matrixB.M_[0][3] + matrixA.M_[2][1] * matrixB.M_[1][3] + matrixA.M_[2][2] * matrixB.M_[2][3] + matrixA.M_[2][3] * matrixB.M_[3][3];
    M_[3][0] = matrixA.M_[3][0] * matrixB.M_[0][0] + matrixA.M_[3][1] * matrixB.M_[1][0] + matrixA.M_[3][2] * matrixB.M_[2][0] + matrixA.M_[3][3] * matrixB.M_[3][0];
    M_[3][1] = matrixA.M_[3][0] * matrixB.M_[0][1] + matrixA.M_[3][1] * matrixB.M_[1][1] + matrixA.M_[3][2] * matrixB.M_[2][1] + matrixA.M_[3][3] * matrixB.M_[3][1];
    M_[3][2] = matrixA.M_[3][0] * matrixB.M_[0][2] + matrixA.M_[3][1] * matrixB.M_[1][2] + matrixA.M_[3][2] * matrixB.M_[2][2] + matrixA.M_[3][3] * matrixB.M_[3][2];
    M_[3][3] = matrixA.M_[3][0] * matrixB.M_[0][3] + matrixA.M_[3][1] * matrixB.M_[1][3] + matrixA.M_[3][2] * matrixB.M_[2][3] + matrixA.M_[3][3] * matrixB.M_[3][3];
}

void GMatrix3D::GetEulerAngles(Float *eX, Float *eY, Float *eZ) const
{
    // Unscale the matrix before extracting term
    GMatrix3D copy(*this);

    // knock out the scale
    copy.SetXScale(1.0f);
    copy.SetYScale(1.0f);
    copy.SetZScale(1.0f);

    // Assuming the angles are in radians.
    if (copy.M_[0][1] > 0.998f) {                       // singularity at north pole
        if (eY)
            *eY = atan2f(copy.M_[2][0],copy.M_[2][2]);  // heading
        if (eZ)
            *eZ = (Float)GFC_MATH_PI/2.f;               // attitude
        if (eX)
            *eX = 0;                                    // bank
        return;
    }
    if (copy.M_[0][1] < -0.998f) {                      // singularity at south pole
        if (eY)
            *eY = atan2f(copy.M_[2][0],copy.M_[2][2]);  // heading
        if (eZ)
            *eZ = (Float)-GFC_MATH_PI/2.f;              // attitude
        if (eX)
            *eX = 0;                                    // bank
        return;
    }

    if (eY)
        *eY = atan2f(-copy.M_[0][2],copy.M_[0][0]);     // heading
    if (eX)
        *eX = atan2f(-copy.M_[2][1],copy.M_[1][1]);     // bank
    if (eZ)
        *eZ = asinf(copy.M_[0][1]);                     // attitude
}

// bank
void GMatrix3D::RotateX(Float angle) 
{
    SetIdentity();

    Float c = cosf(angle);
    Float s = sinf(angle);
    M_[0][0] = 1.0f;
    M_[0][1] = 0.0f;
    M_[0][2] = 0.0f;
    M_[1][0] = 0.0f;
    M_[1][1] = c;
    M_[1][2] = s;
    M_[2][0] = 0.0f;
    M_[2][1] = -s;
    M_[2][2] = c;
}

// heading
void GMatrix3D::RotateY(Float angle) 
{
    SetIdentity();

    Float c = cosf(angle);
    Float s = sinf(angle);
    M_[0][0] = c;
    M_[0][1] = 0.0f;
    M_[0][2] = -s;
    M_[1][0] = 0.0f;
    M_[1][1] = 1;
    M_[1][2] = 0.0f;
    M_[2][0] = s;
    M_[2][1] = 0.0f;
    M_[2][2] = c;
}

// attitude
void GMatrix3D::RotateZ(Float angle) 
{
    SetIdentity();

    Float c = cosf(angle);
    Float s = sinf(angle);
    M_[0][0] = c;
    M_[0][1] = s;
    M_[0][2] = 0.0f;
    M_[1][0] = -s;
    M_[1][1] = c;
    M_[1][2] = 0.0f;
    M_[2][0] = 0.0f;
    M_[2][1] = 0.0f;
    M_[2][2] = 1.0f;
}

void GMatrix3D::GetTranslation(Float *tX, Float *tY, Float *tZ) const
{
    if (tX)
        *tX = GetX();
    if (tY)
        *tY = GetY();
    if (tZ)
        *tZ = GetZ();
}

void GMatrix3D::GetScale(Float *tX, Float *tY, Float *tZ) const
{
    if (tX)
        *tX = GetXScale();
    if (tY)
        *tY = GetYScale();
    if (tZ)
        *tZ = GetZScale();
}

// create camera view matrix
void GMatrix3D::ViewRH(const GPoint3F& eyePt, const GPoint3F& lookAtPt, const GPoint3F& upVec)
{
    Clear();

    // view direction
    GPoint3F zAxis = (eyePt - lookAtPt);
    zAxis.Normalize();

    // right direction
    GPoint3F xAxis;
    xAxis.Cross(upVec, zAxis);
    xAxis.Normalize();

    // up direction
    GPoint3F yAxis;
    yAxis.Cross(zAxis, xAxis);
//    yAxis.Normalize();

    M_[0][0] = xAxis.x;
    M_[1][0] = xAxis.y;
    M_[2][0] = xAxis.z;
    M_[3][0] = -xAxis.Dot(eyePt);

    M_[0][1] = yAxis.x;
    M_[1][1] = yAxis.y;
    M_[2][1] = yAxis.z;
    M_[3][1] = -yAxis.Dot(eyePt);

    M_[0][2] = zAxis.x;
    M_[1][2] = zAxis.y;
    M_[2][2] = zAxis.z;
    M_[3][2] = -zAxis.Dot(eyePt);

    M_[3][3] = 1;
}

// create camera view matrix
void GMatrix3D::ViewLH(const GPoint3F& eyePt, const GPoint3F& lookAtPt, const GPoint3F& upVec)
{
    Clear();

    // view direction
    GPoint3F zAxis = (lookAtPt - eyePt);
    zAxis.Normalize();

    // right direction
    GPoint3F xAxis;
    xAxis.Cross(upVec, zAxis);
    xAxis.Normalize();

    // up direction
    GPoint3F yAxis;
    yAxis.Cross(zAxis, xAxis);
    //    yAxis.Normalize();

    M_[0][0] = xAxis.x;
    M_[1][0] = xAxis.y;
    M_[2][0] = xAxis.z;
    M_[3][0] = -xAxis.Dot(eyePt);

    M_[0][1] = yAxis.x;
    M_[1][1] = yAxis.y;
    M_[2][1] = yAxis.z;
    M_[3][1] = -yAxis.Dot(eyePt);

    M_[0][2] = zAxis.x;
    M_[1][2] = zAxis.y;
    M_[2][2] = zAxis.z;
    M_[3][2] = -zAxis.Dot(eyePt);

    M_[3][3] = 1;
}

// create right handed perspective matrix
void GMatrix3D::PerspectiveRH(Float fovYRad, Float fAR, Float fNearZ, float fFarZ)
{
    Clear();            // zero out

    float dz = fNearZ - fFarZ;
    float yScale = cosf(fovYRad*0.5f) / sinf(fovYRad*0.5f);
    float xScale = yScale / fAR;

    M_[0][0] = xScale;
    M_[1][1] = yScale;
    M_[2][2] = fFarZ / dz;
    M_[2][3] = -1;
    M_[3][2] = fNearZ * fFarZ / dz;
}

// create left handed perspective matrix
void GMatrix3D::PerspectiveLH(Float fovYRad, Float fAR, Float fNearZ, float fFarZ)
{
    Clear();            // zero out

    float dz = fFarZ - fNearZ;
    float yScale = cosf(fovYRad*0.5f) / sinf(fovYRad*0.5f);
    float xScale = yScale / fAR;

    M_[0][0] = xScale;
    M_[1][1] = yScale;
    M_[2][2] = fFarZ / dz;
    M_[2][3] = 1;
    M_[3][2] = -fNearZ * fFarZ / dz;
}

// create right handed perspective matrix based on focal length
void GMatrix3D::PerspectiveFocalLengthRH(float focalLength, float DisplayWidth, float DisplayHeight, float fNearZ, float fFarZ)
{
    Clear();            // zero out

    float dz = fNearZ - fFarZ;
    float xScale = 2.f*focalLength/DisplayWidth;
    float yScale = 2.f*focalLength/DisplayHeight;

    M_[0][0] = xScale;
    M_[1][1] = yScale;
    M_[2][2] = fFarZ / dz;
    M_[2][3] = -1;
    M_[3][2] = fNearZ * fFarZ / dz;
}

// create left handed perspective matrix based on focal length
void GMatrix3D::PerspectiveFocalLengthLH(float focalLength, float DisplayWidth, float DisplayHeight, float fNearZ, float fFarZ)
{
    Clear();            // zero out

    float dz = fFarZ - fNearZ;
    float xScale = 2.f*focalLength/DisplayWidth;
    float yScale = 2.f*focalLength/DisplayHeight;

    M_[0][0] = xScale;
    M_[1][1] = yScale;
    M_[2][2] = fFarZ / dz;
    M_[2][3] = 1;
    M_[3][2] = -fNearZ * fFarZ / dz;
}


// create right handed perspective matrix based on view vol
void GMatrix3D::PerspectiveViewVolRH(Float viewW, Float viewH, Float fNearZ, float fFarZ)
{
    Clear();            // zero out

    float dz = fNearZ - fFarZ;
    M_[0][0] = 2.f * fNearZ / viewW;
    M_[1][1] = 2.f * fNearZ / viewH;
    M_[2][2] = fFarZ / dz;
    M_[2][3] = -1;
    M_[3][2] = fNearZ * fFarZ / dz;
}

// create left handed perspective matrix based on view vol
void GMatrix3D::PerspectiveViewVolLH(Float viewW, Float viewH, Float fNearZ, float fFarZ)
{
    Clear();            // zero out

    float dz = fFarZ - fNearZ;
    M_[0][0] = 2.f * fNearZ / viewW;
    M_[1][1] = 2.f * fNearZ / viewH;
    M_[2][2] = fFarZ / dz;
    M_[2][3] = 1;
    M_[3][2] = fNearZ * fFarZ / (fNearZ-fFarZ);
}

// create customized right handed perspective matrix based on view vol
void GMatrix3D::PerspectiveOffCenterRH(Float viewMinX, Float viewMaxX, Float viewMinY, Float viewMaxY, Float fNearZ, float fFarZ)
{
    Clear();            // zero out

    float dz = fNearZ - fFarZ;
    M_[0][0] = 2.f * fNearZ / (viewMaxX-viewMinX);
    M_[1][1] = 2.f * fNearZ / (viewMaxY-viewMinY);
    M_[2][2] = fFarZ / dz;
    M_[2][3] = -1;
    M_[3][2] = fNearZ * fFarZ / dz;
    M_[2][0] = (viewMinX+viewMaxX) / (viewMaxX-viewMinX);
    M_[2][1] = (viewMinY+viewMaxY) / (viewMaxY-viewMinY);
}

// create customized left handed perspective matrix based on view vol
void GMatrix3D::PerspectiveOffCenterLH(Float viewMinX, Float viewMaxX, Float viewMinY, Float viewMaxY, Float fNearZ, float fFarZ)
{
    Clear();            // zero out

    M_[0][0] = 2.f * fNearZ / (viewMaxX-viewMinX);
    M_[1][1] = 2.f * fNearZ / (viewMaxY-viewMinY);
    M_[2][2] = fFarZ / (fFarZ - fNearZ);
    M_[2][3] = 1;
    M_[3][2] = fNearZ * fFarZ / (fNearZ - fFarZ);
    M_[2][0] = (viewMinX+viewMaxX) / (viewMinX-viewMaxX);
    M_[2][1] = (viewMinY+viewMaxY) / (viewMinY-viewMaxY);
}

// create orthographic projection matrix.  Right handed.
void GMatrix3D::OrthoRH(Float viewW, Float viewH, Float fNearZ, float fFarZ)
{
    Clear();            // zero out

    float dz = fNearZ - fFarZ;
    M_[0][0] = 2.f / viewW;
    M_[1][1] = 2.f / viewH;
    M_[2][2] = 1.f / dz;
    M_[3][2] = fNearZ / dz;
    M_[3][3] = 1;
}

// create orthographic projection matrix.  Left handed.
void GMatrix3D::OrthoLH(Float viewW, Float viewH, Float fNearZ, float fFarZ)
{
    Clear();            // zero out

    float dz = fFarZ - fNearZ;
    M_[0][0] = 2.f / viewW;
    M_[1][1] = 2.f / viewH;
    M_[2][2] = 1.f / dz;
    M_[3][2] = -fNearZ / dz;
    M_[3][3] = 1;
}

// create customized orthographic projection matrix.  Right handed.
void GMatrix3D::OrthoOffCenterRH(Float viewMinX, Float viewMaxX, Float viewMinY, Float viewMaxY, Float fNearZ, float fFarZ)
{
    Clear();            // zero out

    float dz = fNearZ - fFarZ;
    M_[0][0] = 2.f / (viewMaxX-viewMinX);
    M_[1][1] = 2.f / (viewMaxY-viewMinY);
    M_[2][2] = 1.f / dz;
    M_[3][2] = fNearZ / dz;
    M_[3][3] = 1;
    M_[3][0] = (viewMinX+viewMaxX) / (viewMinX-viewMaxX);
    M_[3][1] = (viewMinY+viewMaxY) / (viewMinY-viewMaxY);
}

// create customized orthographic projection matrix.  Left handed.
void GMatrix3D::OrthoOffCenterLH(Float viewMinX, Float viewMaxX, Float viewMinY, Float viewMaxY, Float fNearZ, float fFarZ)
{
    Clear();            // zero out

    M_[0][0] = 2.f / (viewMaxX-viewMinX);
    M_[1][1] = 2.f / (viewMaxY-viewMinY);
    M_[2][2] = 1.f / (fFarZ - fNearZ);
    M_[3][2] = fNearZ / (fNearZ - fFarZ);
    M_[3][3] = 1;
    M_[3][0] = (viewMinX+viewMaxX) / (viewMinX-viewMaxX);
    M_[3][1] = (viewMinY+viewMaxY) / (viewMinY-viewMaxY);
}

// This is an axial bound of an oriented (and/or
// sheared, scaled, etc) box.
void    GMatrix3D::EncloseTransform(GRectF *pr, const GRectF& r, bool bDivideByW) const
{
    // Get the transformed bounding box.
    GPointF p0, p1, p2, p3;
    Transform(&p0, r.TopLeft(), bDivideByW);
    Transform(&p1, r.TopRight(), bDivideByW);
    Transform(&p2, r.BottomRight(), bDivideByW);
    Transform(&p3, r.BottomLeft(), bDivideByW);

    pr->SetRect(p0, p0);
    pr->ExpandToPoint(p1);
    pr->ExpandToPoint(p2);
    pr->ExpandToPoint(p3);
}

void    GMatrix3D::EncloseTransform(GPointF *pts, const GRectF& r, bool bDivideByW) const
{
    // Get the transformed bounding box.
    Transform(&pts[0], r.TopLeft(), bDivideByW);
    Transform(&pts[1], r.TopRight(), bDivideByW);
    Transform(&pts[2], r.BottomRight(), bDivideByW);
    Transform(&pts[3], r.BottomLeft(), bDivideByW);
}
