/**********************************************************************

Filename    :   GEdgeAA.h
Content     :
Created     :   2005-2006
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.
                Patent Pending. Contact Scaleform for more information.

Notes       :

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GEDGEAA_H
#define INC_GEDGEAA_H

#ifndef GFC_NO_FXPLAYER_EDGEAA

#include "GArrayUnsafe.h"
#include "GArrayPaged.h"
#include "GAlgorithm.h"
#include "GCompoundShape.h"



const GCoordType G_IntersectionEpsilonAA = (GCoordType)1e-2;

class GEdgeAA
{
public:
    enum { SID = GStatRG_EdgeAA_Mem };

    enum 
    {
        TransparencyBit = 0x80000000,
        StyleTextureBit = 0x40000000,
        VertexProcessed = 0x20000000,
        VertexUsed      = 0x10000000,
        NeedsDuplicate  = 0x08000000,
        FlagsMask       = 0xFF000000,
        StyleMask       = 0x00FFFFFF
    };

    enum AA_Method
    {
        AA_OuterEdges,
        AA_AllEdges
    };

    struct VertexType
    {
        GCoordType x, y; // Coordinates
        int id;          // Some abstract ID, such as style, index, etc

        VertexType() {}
        VertexType(GCoordType x_, GCoordType y_, int id_) : 
            x(x_), y(y_), id(id_) {}
    };

    struct EdgeType
    {
        unsigned v1, v2; // Vertex indices
        unsigned tri;    // Triangle index,
                         // plus edge in triangle (first two bits)
    };

    enum EdgeSatusFlags
    {
        EdgeModified     = 1,
        TrapezoidEmitted = 2
    };

    struct MeshTriType
    {
        unsigned    iniVer[3];      // Vertex indices
        unsigned    newVer[3];      // New vertices

        unsigned    startEdge;      // Index of the starting edge (ver[0]->ver[2]).
                                    // Two other edges are startEdge+1 and startEdge+2

        int         adjTri[3];      // Adjacent triangles to:
                                    // (ver[0]->[1]), etc.
                                    // -1 means no adjacent triangles.
                                    // First two bits indicate the edge
                                    // of the adjacent triangle.

        int         extVer[3];      // External vertices,
                                    // -1 means no external vertex (inner edge)

        char        edgeStat[3];    // Edge Status

        int         style;          // Style, -1 means "no fill"
    };

    struct TriangleType
    {
        unsigned v1, v2, v3; // Vertices
    };

    // Intersection miter limit; may be used outside to calculate max possible bounds
    static GCoordType GetIntersectionMiterLimit() { return 10.0f; }


    UPInt GetNumBytes() const;

    // Data processing functions
    //------------------------------------------
    void Clear();
    void ClearAndRelease();
    void AddVertex(const GPointType& v);
    void AddTriangle(unsigned v1, unsigned v2, unsigned v3, unsigned style);
    void ProcessEdges(GCoordType width, AA_Method aaMethod);
    void SortTrianglesByStyle();

    // Data access functions
    //------------------------------------------
    UInt GetNumVertices() const                    { return (UInt)Vertices.GetSize(); }
    const VertexType& GetVertex(UInt i) const      { return Vertices[i]; }

    UInt GetNumTriangles() const                   { return (UInt)Triangles.GetSize(); }
    const TriangleType& GetTriangle(UInt i) const  { return Triangles[i]; }

    GCoordType TriangleCrossProduct(const TriangleType& tri) const
    {
        return GMath2D::CrossProduct(Vertices[tri.v1],
                                     Vertices[tri.v2],
                                     Vertices[tri.v3]);
    }

private:
    typedef GArrayPagedLH_POD<VertexType, 8, 64, SID>  VertexArrayType;
    typedef GArrayPagedLH_POD<EdgeType, 10, 64, SID>   EdgeArrayType;


    struct EdgeIdxLess
    {
        const VertexArrayType& Vertices;
        const EdgeArrayType&   Edges;

        EdgeIdxLess(const VertexArrayType& v,
                    const EdgeArrayType& e) :
            Vertices(v), Edges(e)
        {}

        const EdgeIdxLess& operator = (const EdgeIdxLess&)
        { return *this; }

        bool operator () (unsigned a, unsigned b) const
        {
            const EdgeType& ea = Edges[a];
            const EdgeType& eb = Edges[b];
            if (ea.v1 != eb.v1)
                return ea.v1 < eb.v1;
            return ea.v2 < eb.v2;
        }
    };

    struct EdgeLess
    {
        const VertexArrayType& Vertices;
        const EdgeArrayType&   Edges;

        EdgeLess(const VertexArrayType& v,
                 const EdgeArrayType& e) :
            Vertices(v), Edges(e)
        {}

        const EdgeLess& operator = (const EdgeLess&)
        { return *this; }

        bool operator () (unsigned a, const EdgeType& eb) const
        {
            const EdgeType& ea = Edges[a];
            if (ea.v1 != eb.v1)
                return ea.v1 < eb.v1;
            return ea.v2 < eb.v2;
        }
    };

    struct TriangleLess
    {
        const VertexArrayType& Vertices;

        TriangleLess(const VertexArrayType& v) : Vertices(v)
        {}
        const TriangleLess& operator = (const TriangleLess&)
        { return *this; }

        bool operator () (const TriangleType& a, const TriangleType& b) const
        {
            int sa1 = Vertices[a.v1].id;
            int sa2 = Vertices[a.v2].id;
            int sb1 = Vertices[b.v1].id;
            int sb2 = Vertices[b.v2].id;
            if(sa1 != sb1) return sa1 < sb1;
            if(sa2 != sb2) return sa2 < sb2;
            return Vertices[a.v3].id < Vertices[b.v3].id;
        }
    };

    int adjacentTriangle(int triIdx) const
    {
        return MeshTriangles[triIdx >> 2].adjTri[triIdx & 3];
    }

    const VertexType& triangleVertex(unsigned triIdx) const
    {
        return Vertices[MeshTriangles[triIdx >> 2].iniVer[triIdx & 3]];
    }

    const VertexType& triangleVertex(unsigned triIdx, unsigned verInc) const
    {
        unsigned idx = VertexIdx[(triIdx & 3) + verInc];
        return Vertices[MeshTriangles[triIdx >> 2].iniVer[idx]];
    }

    unsigned triangleNewVerIdx(unsigned triIdx) const
    {
        return MeshTriangles[triIdx >> 2].newVer[triIdx & 3];
    }

    unsigned triangleNewVerIdx(unsigned triIdx, unsigned verInc) const
    {
        unsigned idx = VertexIdx[(triIdx & 3) + verInc];
        return MeshTriangles[triIdx >> 2].newVer[idx];
    }

    static unsigned triangleNextIdx(unsigned idx, unsigned inc)
    {
        return (idx & ~3) | VertexIdx[(idx & 3) + inc];
    }

    int  findAdjacentTriangle(unsigned ei) const;
    void buildAdjacencyTable();
    bool buildEdgesFan(unsigned triIdx);
    void calcIntersectionPoint(GCoordType width, GCoordType lim,
                               unsigned start, unsigned end,
                               GCoordType* x, GCoordType* y) const;
    void correctCrossIntersection(const VertexType& iniVer,
                                  const MeshTriType& tri,
                                  unsigned triVerIdx,
                                  VertexType* newVer,
                                  GCoordType width) const;
    int      findSameVertex(int id, GCoordType x, GCoordType y) const;
    unsigned assignStyle(unsigned v1, unsigned v2, unsigned v3);
    void     addTriangle(unsigned v1, unsigned v2, unsigned v3)
    {
        TriangleType tr = { v1, v2, v3 };
        Triangles.PushBack(tr);
    }

private:
    static const unsigned   VertexIdx[6];

    VertexArrayType                                 Vertices;
    EdgeArrayType                                   Edges;
    GArrayPagedLH_POD<MeshTriType, 8, 64, SID>      MeshTriangles;
    GArrayUnsafeLH_POD<unsigned, SID>               EdgeIdx;
    GArrayPagedLH_POD<int, 6, 16, SID>              FanEdges;
    GArrayPagedLH_POD<unsigned, 6, 16, SID>         TmpStarVer;
    GArrayPagedLH_POD<TriangleType, 8, 64, SID>     Triangles;
    GArrayUnsafeLH_POD<unsigned>                    VertexMap;
    unsigned                                        StartDuplicates;
    unsigned                                        DefaultStyle;
};

#endif // #ifndef GFC_NO_FXPLAYER_EDGEAA
#endif

