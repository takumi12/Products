/**********************************************************************

Filename    :   GFxEdgeAAGen.cpp
Content     :   EdgeAA vertex buffer initialization for meshes.
Created     :   May 2, 2006
Authors     :   Michael Antonov

Copyright   :   (c) 2006 Scaleform Corp. All Rights Reserved.
                Patent Pending. Contact Scaleform for more information.

Notes       :

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxShape.h"
#include "GFxEdgeAAGen.h"
#include "GTessellator.h"


#ifndef GFC_NO_FXPLAYER_EDGEAA



// ***** GFxEdgeAAGenerator Implementation

// Specify pfills = 0 (or empty) to generate a solid.
GFxEdgeAAGenerator::GFxEdgeAAGenerator() :
    pFills(0), FillsNum(0), UsingTextures(false), DefFactorRGB(0)
{
}

GFxEdgeAAGenerator::~GFxEdgeAAGenerator()
{
}

// Processes & sorts edges if necessary
void    GFxEdgeAAGenerator::ProcessAndSortEdges(GTessellator& tess, const GFxFillStyle *pfills, UPInt fillsNum, 
                                                GCoordType width, GEdgeAA::AA_Method aaMethod,
                                                UInt32 defFactorRGB)
{
    EdgeAA.Clear();

    // Fill size of 0 means no texture fills.
    DefFactorRGB    = defFactorRGB;
    pFills          = pfills;
    FillsNum        = fillsNum;
    if (pFills && FillsNum == 0)
        pFills = 0;

    UsingTextures = 0;

    // Add vertices
    UInt i;
    for(i = 0; i < tess.GetNumVertices(); i++)
        EdgeAA.AddVertex(tess.GetVertex(i));

    int fillSize = pFills ? (int)FillsNum : 0;

    // And triangles
    for(i = 0; i < tess.GetNumMonotones(); i++)
    {
        // Get the info about the i-th monotone.
        // Well, basically m.style is all you need.
        const GTessellator::MonotoneType& m = tess.GetMonotone(i);

        // Mark textures
        // The tessellator uses zero-based styles, so, make it minus-one-based
        // to conform to the existing policy.
        int style = (int)m.style - 1;

        // 0's are solid fills, the rest depend on images
        // NOTE: Fills can be empty for fonts, so check bounds.
        if ((style < fillSize) && (pFills[style].GetType() != 0))
        {
            style |= StyleTextureBit;
            UsingTextures = 1;
        }

        // Triangulate the i-th monotone polygon. After this call
        // you can iterate through the triangles for this particular monotone.
        tess.TriangulateMonotone(i);

        // Add triangles to AA (with m.style)
        UInt    triangleCount = tess.GetNumTriangles();
        UInt    triangle;
        // Fetch triangle indices for each triangle
        for(triangle = 0; triangle<triangleCount; triangle++)
        {
            const GTessellator::TriangleType& t = tess.GetTriangle(triangle);
            EdgeAA.AddTriangle(t.v1, t.v2, t.v3, style);

            GASSERT(style != -1);
        }
    }

    EdgeAA.ProcessEdges(width, aaMethod);
    if (UsingTextures)
        EdgeAA.SortTrianglesByStyle();
}

// Returns the number of vertices generated,.
UInt    GFxEdgeAAGenerator::GetVertexCount() const
{
    return EdgeAA.GetNumVertices();
}





// Helper class: collects new vertices, duplicating them if so required for Factor
// uniqueness. This class is used to check if the vertices factor has been assigned
// based on a triangle. If that is the case, the other triangles must either match,
// or a new accommodating vertex will be inserted.
struct GFxEdgeAAGenerator_VertexCollector
{
    typedef GRenderer::VertexXY16iCF32 VertexXY16iCF32;

    GFxVertexArray &    NewVertices;
    // UsedVertex is used both as a vertex-to factor allocated flag and index
    // to the next replicated vertex with the same factor.
    GArray<UInt>        UsedVertices;

    GFxEdgeAAGenerator_VertexCollector(GFxVertexArray *pvertices, UInt size)
        : NewVertices (*pvertices)
    {
        NewVertices.Resize(size);
        UsedVertices.Resize(size);
        for (UInt i=0; i<UsedVertices.GetSize(); i++)
            UsedVertices[i] = GFC_MAX_UINT;
    }

    const GFxEdgeAAGenerator_VertexCollector& operator = (const GFxEdgeAAGenerator_VertexCollector&)
    { return *this; }

    GFxVertexRef operator [] (UInt index) const
    { return NewVertices[index]; }

    // Assigns a factor value to a vertex. Returns a potentially different index.
    UInt    AssignFactor(UInt index, UInt32 factor)
    {
        // If vertex does not yet have an assigned factor, use it directly
        GFxVertexRef vertexRef = NewVertices[index];

        if (UsedVertices[index] == GFC_MAX_UINT)
        {
            UsedVertices[index] = index;
            vertexRef.SetFactor(factor);
            return index;
        }

        if (vertexRef.GetFactor() == factor)
            return index;

        // Search index list for next use.
        while (UsedVertices[index] != index)
        {
            index = UsedVertices[index];
            if (NewVertices[index].GetFactor() == factor)
                return index;
        }

        // Use not found, create new vertex.
        vertexRef = NewVertices[index];

        Float   x, y;
        UInt32  color = vertexRef.GetColor();
        vertexRef.GetXY(&x, &y);

        // Adjust list and index.
        UsedVertices[index] = NewVertices.GetSize();
        index = NewVertices.GetSize();

        // Push new values.
        UsedVertices.PushBack(index);
        vertexRef = NewVertices.AppendVertex();
        vertexRef.InitVertex(x, y, color);
        vertexRef.SetFactor(factor);
        return index;
    }

    void    AssignColor(UInt dstIndex, UInt srcIndex)
    {
        GFxVertexRef srcVertex = NewVertices[srcIndex];
        GFxVertexRef dstVertex = NewVertices[dstIndex];
        dstVertex.SetColor(srcVertex.GetColor());
    }

};


GINLINE bool GFxEdgeAAGenerator::SetVertexColor(const GEdgeAA::VertexType &v,
                                                GFxVertexRef& cv,
                                                GRenderer::VertexFormat format, 
                                                int fillsSize)
{
    int     id = v.id & StyleMask;
    bool    styleMissing = (id >= fillsSize);
    bool    solidColorFlag = styleMissing || (pFills[id].GetType() == 0);
    if (v.id & TransparencyBit)
    {
        if (format == GRenderer::Vertex_XY16iC32)
        {
            if (styleMissing)
            {
                // Font, don't have fill
                cv.SetColor(0x00FFFFFF);
            }
            else if (solidColorFlag)
            {
                cv.SetColor(GetProcessedColor(id) & 0x00FFFFFF);
            }
        }
        else
        {
            if (styleMissing)
            {
                // Font, don't have fill
                cv.SetColor(0xFFFFFFFF);
                cv.SetFactor(DefFactorRGB);
            }
            else 
            {
                if (solidColorFlag)
                {
                    cv.SetColor(GetProcessedColor(id));
                    cv.SetFactor(DefFactorRGB);
                }
                else
                {
                    cv.SetFactor(0x00FFFFFF);
                }
            }
        }
    }
    else
    {
        // NOTE: Issue with font: we don't know color ahead of time.
        // Store while for now, but will need another font factor later.
        if (styleMissing)
        {
            // Font, don't have fill
            cv.SetColor(0xFFFFFFFF);
        }
        else if (solidColorFlag)
        {
            cv.SetColor(GetProcessedColor(id));
        }

        // Set EdgeAA constant to 1.0f, texture mix factor of 0.
        cv.SetFactor(DefFactorRGB | 0xFF000000);
    }
    return solidColorFlag;
}




bool    GFxEdgeAAGenerator::GenerateSolidMesh(GFxVertexArray *pvertices, GFxMesh *pmesh, Float scaleMultiplier)
{
    // Generate AA meshes
    pvertices->Resize(EdgeAA.GetNumVertices());
    GRenderer::VertexFormat vf = pvertices->GetFormat();

    // Should we copy vertices first,
    // and initialize them with color later
    int  fillsSize = pFills ? (int)FillsNum : 0;

    UInt i;
    for (i=0; i< EdgeAA.GetNumVertices(); i++)
    {
        const GEdgeAA::VertexType &v  = EdgeAA.GetVertex(i);
        GFxVertexRef              cv  = pvertices->GetVertexRef(i);
        cv.InitVertex(v.x * scaleMultiplier, v.y * scaleMultiplier);
        SetVertexColor(v, cv, vf, fillsSize);
    }

    // Generate a triangle mesh.
    for (i=0; i< EdgeAA.GetNumTriangles(); i++)
    {
        const GEdgeAA::TriangleType &t = EdgeAA.GetTriangle(i);
        pmesh->AddTriangle((UInt16)t.v1, (UInt16)t.v2, (UInt16)t.v3);
    }
    return 1;
}


bool    GFxEdgeAAGenerator::GenerateSolidMesh(GFxVertexArray *pvertices, GFxMesh *pmesh, const GRenderer::Matrix &mat)
{
    // Generate AA meshes
    pvertices->Resize(EdgeAA.GetNumVertices());
    GRenderer::VertexFormat vf = pvertices->GetFormat();

    // Should we copy vertices first,
    // and initialize them with color later
    int     fillsSize = pFills ? (int)FillsNum : 0;
    UInt    i;
    GPointF pt;

    for (i=0; i< EdgeAA.GetNumVertices(); i++)
    {
        const GEdgeAA::VertexType &v  = EdgeAA.GetVertex(i);
        GFxVertexRef              cv  = pvertices->GetVertexRef(i);

        // Transform back (usually to shape space from screen space)
        mat.Transform(&pt, GPointF(v.x, v.y));

    #ifdef GFC_BUILD_DEBUG
        if ((pt.x > 32767.0f) || (pt.y > 32767.0f) || (pt.y < -32768.0f) || (pt.x < -32768.0f))
        {
            static bool RangeOverflowWarned = 0;
            if (!RangeOverflowWarned)
            {
                GFC_DEBUG_WARNING2(1,
                        "Stroker target range overflow for EdgeAA: (x,y) = (%g,%g).",
                        pt.x, pt.y);
                RangeOverflowWarned = 1;
            }
            // This clamping should be faster for debug
            if (pt.x > 32767.0f)    pt.x = 32767.0f;
            if (pt.x < -32768.0f)   pt.x = -32768.0f;
            if (pt.y > 32767.0f)    pt.y = 32767.0f;
            if (pt.y < -32768.0f)   pt.y = -32768.0f;
        }

    #else
        // Safety clamping for the case above. CPU should have an fabsf() op.
        if (fabsf(pt.x) > 32767.0f)
            pt.x = (pt.x > 32767.0f) ? 32767.0f : -32768.0f;
        if (fabsf(pt.y) > 32767.0f)
            pt.y = (pt.y > 32767.0f) ? 32767.0f : -32768.0f;
    #endif


        cv.InitVertex(pt.x, pt.y, 0);
        SetVertexColor(v, cv, vf, fillsSize);
    }

    // Generate a triangle mesh.
    for (i=0; i< EdgeAA.GetNumTriangles(); i++)
    {
        const GEdgeAA::TriangleType &t = EdgeAA.GetTriangle(i);
        pmesh->AddTriangle((UInt16)t.v1, (UInt16)t.v2, (UInt16)t.v3);
    }
    return 1;
}



// Generates textured meshes, returns number of mesh sets if fit, 0 if over 65K (fails).
// If fails, Mesh array remains untouched and 0 is returned.
UInt    GFxEdgeAAGenerator::GenerateTexturedMeshes(GFxVertexArray *pvertices, 
                                                   GArrayLH<GFxMesh, GStatRG_MeshFill_Mem> *pmeshes, 
                                                   Float scaleMultiplier)
{
    GASSERT(pmeshes != NULL);

    GRenderer::VertexFormat vf = pvertices->GetFormat();
    GFxEdgeAAGenerator_VertexCollector  vertices(pvertices, EdgeAA.GetNumVertices());
    bool haveColors = 0;

    // Should we copy vertices first,
    // and initialize them with color later
    UInt i;
    for (i=0; i< EdgeAA.GetNumVertices(); i++)
    {
        const GEdgeAA::VertexType &v  = EdgeAA.GetVertex(i);
        GFxVertexRef              cv  = pvertices->GetVertexRef(i);

        cv.InitVertex(v.x * scaleMultiplier, v.y * scaleMultiplier);
        haveColors |= SetVertexColor(v, cv, vf, (int)FillsNum);
    }

    // Assign correct colors to corner vertices of each triangle.
    //UpdateEdgeAATriangleColors(pvertices);


    // Styles used in these vertices.
    int s1 = -1;
    int s2 = -1;
    int s3 = -1;

    // Store initial mesh size in case we fail.
    //UInt startMeshSize    = pmeshes->GetSize();
    UInt meshCount      = 0;
    GRenderer::GouraudFillType lastgft = GRenderer::GFill_Color;

    // Triangles come out consecutive by style, so we can generate
    // a special MeshSet for each style change
    for (i=0; i< EdgeAA.GetNumTriangles(); i++)
    {
        const GEdgeAA::TriangleType &t = EdgeAA.GetTriangle(i);
        // Style tripple
        const GEdgeAA::VertexType &v1 = EdgeAA.GetVertex(t.v1);
        const GEdgeAA::VertexType &v2 = EdgeAA.GetVertex(t.v2);
        const GEdgeAA::VertexType &v3 = EdgeAA.GetVertex(t.v3);

        bool    v1Texture = isTexture(v1.id);
        bool    v2Texture = isTexture(v2.id);
        bool    v3Texture = isTexture(v3.id);

        bool    haveTextures = v1Texture | v2Texture | v3Texture;

        if ((!stylesEq(s1, v1.id) && v1Texture) ||
            (!stylesEq(s2, v2.id) && v2Texture) ||
            (!stylesEq(s3, v3.id) && v3Texture) ||
            (i == 0) )
        {
            // Determine shade mode.
            GRenderer::GouraudFillType gft = GRenderer::GFill_Color;
            if (haveTextures)
            {
                gft = haveColors ? GRenderer::GFill_1TextureColor : GRenderer::GFill_1Texture;

                if (v1Texture && v2Texture && v3Texture)
                {
                    if (!stylesEq(v3.id, v2.id) || !stylesEq(v3.id, v1.id))
                        gft = GRenderer::GFill_2Texture;
                }
            }

            // Detect Textured cases where style change does not require us to generate
            // a new mesh, and can thus share DrawPrimitive calls.
            // Note that due to sorting, we may not detect all combinable cases here.
            bool    sharePrimitives = 0;

            if ((gft == GRenderer::GFill_1TextureColor) ||
                (gft == GRenderer::GFill_1Texture) ||
                (gft == GRenderer::GFill_2Texture) )
            {
                // Change from 2 styles with texture and one style with texture can share DrawPrimitive.
                bool    s1IsTexture = isTexture(s1);

                // If s3 did not change and s1 did not change texture
                if ( stylesEq(s3, v3.id) &&
                    (stylesEq(s1, v1.id) || (stylesEq(v1.id, v3.id) || (gft == GRenderer::GFill_1TextureColor && (!v1Texture || !s1IsTexture))))  )
                {
                    // This must be true after the check above.
                    GASSERT (!stylesEq(v2.id, s2) || !stylesEq(v1.id, s1));

                    // The following transitions are allowed (back and forth):
                    //
                    //  v1:   C    < = >    T
                    //  v2:   C    < = >    T
                    //  v3:   T             T
                    //
                    //  or
                    //
                    //  v1:   T1            T1
                    //  v2:   T1   < = >    T2
                    //  v3:   T2            T2

                    // If it is a texture, v2 must now equal either T1 or T2, in order to share primitive.
                    if (v2Texture)
                    {
                        if (stylesEq(v2.id, v3.id) || stylesEq(v2.id, v1.id))
                        {
                            bool s2IsTexture = isTexture(s2);

                            if (!s2IsTexture)
                            {
                                sharePrimitives = 1;
                            }
                            else
                            {
                                // s2IsTetexture check is necessary to avoid transitions such as:
                                //  T1, T2, T3 -> T1, T3, T3
                                //  TBD: This may not be necessary once we handle 3-texture cases?
                                //  GFill_2Texture check allows sow simple 3-different corner
                                //  cases to be batched "incorrectly" (single corner triangle, but less DPs).
                                if (stylesEq(s2, v3.id) || stylesEq(s2, v1.id) || (gft != GRenderer::GFill_2Texture))
                                    sharePrimitives = 1;
                            }

                        }
                    }
                    else
                    {
                        // Otherwise, if we are a color, the following must be true
                        //   - s1 must also be a color
                        //   - s2 must have been the same texture as s3

                        if (!s1IsTexture && stylesEq(s1, s3))
                        {
                            sharePrimitives = 1;
                        }
                    }
                }
            }

            // Triangles added to new mesh.
            // We do this even if we are sharing primitives, so that matching above can work faster.
            s1 = v1.id;
            s2 = v2.id;
            s3 = v3.id;

            if (!sharePrimitives)
            {
                // If texture style changed, new MeshSet
                pmeshes->Resize(pmeshes->GetSize()+1);
                meshCount++;
                // Different style tables are used based on different fill rule.
                pmeshes->Back().SetGouraudFillType(gft);
                lastgft = gft;

                switch(gft)
                {
                case GRenderer::GFill_1Texture:
                case GRenderer::GFill_1TextureColor:
                    // 1 texture: will always be style 3 due to sorting.
                    pmeshes->Back().SetEdgeAAStyles(1, isTransparent(s3) ? (UInt)GFxPathConsts::Style_None : (s3 & StyleMask),
                                                    (UInt)GFxPathConsts::Style_None, (UInt)GFxPathConsts::Style_None);
                    break;

                case GRenderer::GFill_2Texture:
                    // 2 textures.
                    // Here, second texture can come from either s2 or s3.
                    {
                        int secondStyle = stylesEq(s3, s2) ? s1 : s2;
                        pmeshes->Back().SetEdgeAAStyles(2,
                                isTransparent(s3) ? (UInt)GFxPathConsts::Style_None : (s3 & StyleMask),
                                isTransparent(secondStyle) ? (UInt)GFxPathConsts::Style_None : (secondStyle & StyleMask),
                                (UInt)GFxPathConsts::Style_None);
                    }
                    break;

                case GRenderer::GFill_Color:
                default:
                    // No textures.
                    pmeshes->Back().SetEdgeAAStyles(0, (UInt)GFxPathConsts::Style_None, (UInt)GFxPathConsts::Style_None, (UInt)GFxPathConsts::Style_None);
                    break;
                }
            }
        }


        // Keep processing vertex: Assign factors.
        if (haveTextures)
        {
            UInt    iv1 = t.v1;
            UInt    iv2 = t.v2;
            UInt    iv3 = t.v3;

            UInt32  v1Factor = vertices[iv1].GetFactor() & 0xFF000000,
                    v2Factor = vertices[iv2].GetFactor() & 0xFF000000,
                    v3Factor = vertices[iv3].GetFactor() & 0xFF000000;

            // 1. Figure out what the factors should be for each vertex
            //    (only necessary if there are ANY textures)

            /* Complex - case blending, need to handle channels differently
            if (v1Texture)
                v1Factor = 0x0000FF00; // G = 1.0
            else
                v1Factor = 0;

            if (v2Texture)
                v2Factor = 0x00FF0000; // B = 1.0
            else
                v2Factor = 0;
            */

            if (v3Texture)
            {
                v3Factor |= 0x00FFFFFF; // RGB = 1.0

                // Account for identical texture cases, where the same texture is
                // assigned to more then one corner.

                // Replicate factors for same texture use in different corner vertices.
                if (stylesEq(v2.id, v3.id) || isTransparent(v2.id))
                {
                    v2Factor |= 0x00FFFFFF;
                    if (stylesEq(v1.id, v2.id) || isTransparent(v1.id))
                        v1Factor |= 0x00FFFFFF;
                }
                else
                {
                    // because of backwards sort v3 is primary texture and v2 secondary
                    // V3 will be texture

                    if (stylesEq(v1.id, v3.id) || isTransparent(v1.id))
                    {
                        // texture2 mode
                        v1Factor |= 0x00FFFFFF;
                    }

                    // Two or more textures we used, need to handle this.
                //  GASSERT(!isTexture(v2.id));
                //  GASSERT(!isTexture(v1.id));
                }
            }
            else
            {
                // v3Factor |= 0;
                GASSERT(0);
            }


            // 2. Assign factors to vertices, duplicating them if necessary
            //    (modify indices in process)

            if (v1Texture) 
                iv1 = vertices.AssignFactor(iv1, v1Factor);
            else
            {
                if (v2Texture) vertices.AssignColor(iv2, iv1);
                if (v3Texture) vertices.AssignColor(iv3, iv1);
            }

            if (v2Texture) 
                iv2 = vertices.AssignFactor(iv2, v2Factor);
            else
            {
                if (v1Texture) vertices.AssignColor(iv1, iv2);
                if (v3Texture) vertices.AssignColor(iv3, iv2);
            }

            if (v3Texture) 
                iv3 = vertices.AssignFactor(iv3, v3Factor);
            else
            {
                if (v1Texture) vertices.AssignColor(iv1, iv3);
                if (v2Texture) vertices.AssignColor(iv2, iv3);
            }
            pmeshes->Back().AddTriangle((UInt16)iv1, (UInt16)iv2, (UInt16)iv3);
        }
        else
        {
            UInt    iv1 = t.v1;
            UInt    iv2 = t.v2;
            UInt    iv3 = t.v3;

            // Need to mark factor as used.
    //      iv1 = vertices.AssignFactor(iv1, vertices[iv1].Factors & 0xFF000000);
    //      iv2 = vertices.AssignFactor(iv2, vertices[iv2].Factors & 0xFF000000);
    //      iv3 = vertices.AssignFactor(iv3, vertices[iv3].Factors & 0xFF000000);

            // Store triangle indices into mesh set.
            pmeshes->Back().AddTriangle((UInt16)iv1, (UInt16)iv2, (UInt16)iv3);
        }
    }
    return meshCount;
}


#endif // #ifndef GFC_NO_FXPLAYER_EDGEAA
