﻿#pragma once
#include "wrGeo.h"
#include "ADFCollisionObject.h"
#include <CGAL\Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL\Delaunay_Triangulation_3.h>
#include <CGAL\Triangulation_vertex_base_with_info_3.h>
#include <CGAL\Polyhedron_3.h>
#include <CGAL\Point_3.h>
#include <CGAL\Triangle_3.h>


namespace WR
{
    //typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
    //typedef K::Point_3 Point_3;
    //typedef K::Vector_3 Vector_3;

    template <class Refs, class T = Tag_true, class Pln = Tag_false>
    struct FaceWithId : public CGAL::HalfedgeDS_face_base<Refs, T, Pln> {
        int idx;
        FaceWithId() {}
    };

    struct Items_FaceWithId : public CGAL::Polyhedron_items_3 {
        template <class Refs, class Traits>
        struct Face_wrapper {
            typedef typename Traits::Plane_3 Plane;
            typedef FaceWithId<Refs, CGAL::Tag_true, Plane> Face;
        };
    };
    
    typedef typename CGAL::Polyhedron_3<K, Items_FaceWithId>      Polyhedron_3_FaceWithId;

    template <class _Kernel>
    class wrTriangle : public CGAL::Triangle_3 <_Kernel>
    {
        typedef typename _Kernel::Point_3                                   Point_3;
        typedef typename _Kernel::Vector_3                                  Vector_3;
        typedef typename Polyhedron_3_FaceWithId::FaceHandle                FaceHandle;
        typedef typename WRG::PointTriangleDistInfo<typename _Kernel::FT>   Info;

    public:
        wrTriangle() {}

        wrTriangle(const CGAL::Triangle_3<_Kernel>& t)
            : CGAL::Triangle_3 <_Kernel>(t) {}

        wrTriangle(const Point_3& p, const Point_3& q, const Point_3& r) :
            CGAL::Triangle_3 <_Kernel>(p, q, r){}

        void initInfo()
        {
            E0 = vertex(1) - vertex(0);
            E1 = vertex(2) - vertex(0);

            infos.a = E0 * E0;
            infos.b = E0 * E1;
            infos.c = E1 * E1;

            normal = CGAL::normal(vertex(0), vertex(1), vertex(2));
            normal = normal / sqrt(normal.squared_length());
        }

        void computeInfo(const Point_3& p)
        {
            Vector_3 D = vertex(0) - p;
            infos.d = E0 * D;
            infos.e = E1 * D;
            infos.f = D * D;
        }

        Info                infos;
        Vector_3            E0, E1;
        Vector_3            normal;
        FaceHandle          fh;
    };


    class ADFOctree
    {
        typedef K::Iso_cuboid_3         Cube_3;
        typedef K::Point_3              Point_3;
        typedef K::Vector_3             Vector_3;
        typedef Polyhedron_3_FaceWithId Polyhedron_3;

    public:

        struct Node
        {
            //   0---------1
            //  /|        /|
            // 3---------2 |
            // | |       | |
            // | |       | |
            // | 4-------|-5
            // |/        |/
            // 7---------6
            //   
            //   z
            //   |
            //   •----y
            //  /
            // x

            Dt::Vertex_handle       vertices[8];
            std::vector<unsigned>   eList;
            size_t                  level;
            Cube_3                  bbox, triple;

            Node*                   pParent = nullptr;
            Node*                   children[8];

            bool hasChild() const { return children[0] == nullptr; }
        };

    public:
        ADFOctree();
        ~ADFOctree();

        void releaseExceptDt();

        bool construct(Polyhedron_3& geom, size_t maxLvl);
        Dt* pop_dt_structure() { Dt* res = dt; dt = nullptr; return res; }

        //float queryDistance(const Point_3& p) const;
        //bool queryGradient(const Point_3& p, Vector_3& grad) const;
        //float queryExactDistance(const Point_3& p) const;

    //public: // for debug
    //    int(*testSign)(const Point_3&);

    private:

        void constructChildren(Node*);
        Node* createRootNode(const Polyhedron_3&);
        void computeMinDistance(Node*);
        int determineSign(int type, const Point_3& p, const Vector_3& diff, size_t triIdx) const;

        template <class Iterator>
        double minSquaredDist(const Point_3& p, Iterator begin, Iterator end, Vector_3* diff = nullptr, size_t* tri = nullptr, int* type = nullptr) const;

        double minDist(const Cube_3& bbox, const Point_3& p);
        void computeGradient();

        int detSignOnFace(const Point_3& p, const Vector_3& diff, size_t triIdx) const;
        int detSignOnEdge(const Point_3& p, const Vector_3& diff, size_t triIdx, int seq) const;
        int detSignOnVertex(const Point_3& p, const Vector_3& diff, size_t triIdx, int seq) const;

        void release();
        Node* createNode();
        void computeTripleFromBbox(Cube_3&, const Cube_3&) const;

        Node*                           pRoot = nullptr;
        size_t                          nMaxLevel = 0;
        Dt*                             dt = nullptr;

        std::vector<Node*>              cellList;
        wrTriangle<K>*                  triList = nullptr;
        size_t                          nTriangles = 0;
        CGAL::Bbox_3                    box;
    private:

    };
}
