#pragma once
#include "ICollisionObject.h"
#include <CGAL\Delaunay_Triangulation_3.h>
#include <CGAL\Triangulation_vertex_base_with_info_3.h>
#include <limits>
#include "wrMacro.h"

namespace WR
{
    class ADFCollisionObject :
        public ICollisionObject
    {
        COMMON_PROPERTY(CGAL::Bbox_3, bbox);
        COMMON_PROPERTY(size_t, max_level);

        friend class ADFOctree;

        struct VInfo
        {
            float           minDist = std::numeric_limits<float>::max();
            Vector_3        gradient;
            //int             idx; // for dubug
        };

        typedef CGAL::Triangulation_vertex_base_with_info_3<VInfo, K>                       Vb;
        typedef CGAL::Triangulation_data_structure_3<Vb>                                    Tds;
        typedef CGAL::Delaunay_triangulation_3<K, Tds, CGAL::Location_policy<CGAL::Fast>>   Dt;
        typedef float(ADFCollisionObject::*ExtrapolateFunc)(const Point_3& p, const Point_3 v[], size_t infId, Dt::Cell_handle ch) const;

    public:
        ADFCollisionObject(Dt* stt, const CGAL::Bbox_3& box, size_t lvl) :
            pDt(stt), m_bbox(box), m_max_level(lvl){
            compute_gradient();
        }
        ADFCollisionObject(const wchar_t*);
        ~ADFCollisionObject() { release(); }

        virtual float query_distance(const Point_3& p) const { return query_distance_template(p, &ADFCollisionObject::no_extrapolate); }
        virtual float query_squared_distance(const Point_3& p) const;
        virtual bool exceed_threshhold(const Point_3& p, float thresh = 0.f) const;
        virtual bool position_correlation(const Point_3& p, Point_3* pCorrect, float thresh = 0.f) const;

        bool save_model(const wchar_t*) const;
        bool load_model(const wchar_t*);

        void compute_gradient();

    private:
        float query_distance_with_extrapolation(const Point_3& p) const { return query_distance_template(p, &ADFCollisionObject::extrapolate); }
        float query_distance_with_fake_extrapolation(const Point_3& p) const { return query_distance_template(p, &ADFCollisionObject::fake_extrapolate); }
        float query_distance_template(const Point_3& p, ExtrapolateFunc func) const;

        float no_extrapolate(const Point_3& p, const Point_3 v[], size_t infId, Dt::Cell_handle ch) const { return std::numeric_limits<float>::max(); }
        float extrapolate(const Point_3& p, const Point_3 v[], size_t infId, Dt::Cell_handle ch) const;
        float fake_extrapolate(const Point_3& p, const Point_3 v[], size_t infId, Dt::Cell_handle ch) const;

        void release();

        Dt* pDt = nullptr;
    };
}
