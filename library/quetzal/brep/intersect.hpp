#if !defined(QUETZAL_BREP_INTERSECT_HPP)
#define QUETZAL_BREP_INTERSECT_HPP
//------------------------------------------------------------------------------
// brep
// face_operations.hpp
//------------------------------------------------------------------------------

#include "Mesh.hpp"
#include "Face.hpp"
#include "border.hpp"
#include "geometry.hpp"
#include "quetzal/common/id.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include "quetzal/geometry/Segment.hpp"
#include "quetzal/geometry/intersect.hpp"
#include "quetzal/geometry/intersection.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/math/math_util.hpp"
#include <array>
#include <algorithm>
#include <span>
#include <vector>

namespace quetzal::brep
{

    // Faces should be planar and coplanar

    // Returns a set of intersections as with geometry, except with specific vertex ordering and where polygons track the face that each section originated from ...
    // these results will be interpreted by the caller, so only the intersection polygon sections are needed ...
    // this is the analog of intersection operations from geometry, but with brep elements ...

    // set of intersection sections, each of which can be vertex (tangent can be ignored?), edge segment, or polygon
    // polygon is a pair of branches or edge segments, the first element of which is the one that follows the edges of the face's submesh, the second is the path where the other face's submesh intersects
    // (shared edges complicate this determination)
    // the current trace is just edge segments, and only a single halfedge_id is needed because we can follor the border

    // first make sure that the current implementation supports internal face intersections that share edges ...
    // try with fully interior, 1 edge shared, 2 adjacent edges shared, 2 non-adjacent edges shared, ..., all edges shared

    // face locus can be empty, segment, polygon
    // polygon can be normal intersection, completely interior, or interior sharing some or all of the edges
    // segment: halfedge_id_a, position_a, halfedge_id_b, position_b, treat just like the existing intersection trace
    // normal polygon intersection: halfedge_id_a0, halfedge_id_b0, position_a0, halfedge_id_a1, halfedge_id_b1, position_a1
    // completely interior: halfedge_id or face_id of the interior, hole should be produced in the other
    // interior with shared edges: same as normal intersection but the interior face requires no edges added, hint as to which face is interior would be helpful
    // this last case, shared edges, can occur in cases where there is a normal polygon intersection and this should simply be handled there
    // the interior hole case is the only real edge case for polygon intersection result


    // intersection should return a polygon whose vertices are actual and virtual intersection positions
    // split_intersection should work like IntersectionSplitter returning a set of four id's for each uncoupled trace


    // Considers halfedges as half-open intervals
    template<typename Traits>
    bool intersects(const Halfedge<Traits>& halfedge_a, const Halfedge<Traits>& halfedge_b);

    template<typename Traits>
    bool intersects(const Face<Traits>& face_a, const Face<Traits>& face_b);

    // Treats halfedges as half-open intervals
    template<typename Traits>
    geometry::Intersection<typename Traits::vector_traits> intersection(const Halfedge<Traits>& halfedge_a, const Halfedge<Traits>& halfedge_b);

    // halfedge_id_a, halfedge_id_b, position
    template<typename Traits>
    using halfedge_intersection_position = std::tuple<id_type, id_type, typename Traits::point_type>;

    // This just finds the intersection points/segment
    // But with that, we can use the halfedge_id/s returned to trace/uncouple the polygonal intersection, one path following next, and the other following the prev partners for other face ...
    // result can be vertex (position0 and position1 are the same) (ignored), edge segment (id1 == is0 or id0 next_id), or polygon (otherwise)
    template<typename Traits>
    std::optional<std::array<halfedge_intersection_position<Traits>, 2>> intersection(const Face<Traits>& face_a, const Face<Traits>& face_b);

    template<typename Traits>
    IntersectionSplitter<Traits>::connections_type split_intersection_(const Face<Traits>& face_a, const Face<Traits>& face_b);

    template<typename Traits>
    IntersectionSplitter<Traits>::connections_type split_intersection_(const Face<Traits>& face_a, const Face<Traits>& face_b, const halfedge_intersection_position<Traits>& ix);

//    template<typename Traits>
//    IntersectionSplitter<Traits>::connections_type split_intersection(const Surface<Traits>& surface_a, const Surface<Traits>& surface_b);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::brep::intersects(const Halfedge<Traits>& halfedge_a, const Halfedge<Traits>& halfedge_b)
{
    auto inter = geometry::intersection(to_segment(halfedge_a), to_segment(halfedge_b));
    if (!inter)
    {
        return false;
    }

    if (inter.locus() == geometry::Locus::Point)
    {
        auto position = inter.point();
        if (float_eq(position, halfedge_a.next().vertex_attributes().position()) || float_eq(position, halfedge_b.next().vertex_attributes().position()))
        {
            return false; // Reject open end of halfedge interval
        }

        return true;
    }
    
    assert(inter.locus() == geometry::Locus::Segment);
    return true;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::brep::intersects(const Face<Traits>& face_a, const Face<Traits>& face_b)
{
    return intersects(to_polygon(face_a), to_polygon(face_b));
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<typename Traits::vector_traits> quetzal::brep::intersection(const Halfedge<Traits>& halfedge_a, const Halfedge<Traits>& halfedge_b)
{
    auto inter = geometry::intersection(to_segment(halfedge_a), to_segment(halfedge_b));
    if (inter.empty())
    {
        return {};
    }

    if (inter.locus() == geometry::Locus::Point)
    {
        auto position = inter.point();
        if (float_eq(position, halfedge_a.next().vertex_attributes().position()) || float_eq(position, halfedge_b.next().vertex_attributes().position())) // Reject open end of halfedge interval
        {
            return {};
        }

        return inter;
    }

    assert(inter.locus() == geometry::Locus::Segment);
    return inter;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::optional<std::array<quetzal::brep::halfedge_intersection_position<Traits>, 2>> quetzal::brep::intersection(const Face<Traits>& face_a, const Face<Traits>& face_b)
{
    std::array<quetzal::brep::halfedge_intersection_position<Traits>, 2> endpoints;
    size_t intersection_count = 0;

    for (const auto& halfedge_a : face_a.halfedges())
    {
        for (const auto& halfedge_b : face_b.halfedges())
        {
            auto ix = intersection(halfedge_a, halfedge_b);
            if (ix.locus() != geometry::Locus::Empty)
            {
                continue;
            }

            if (ix.locus() == geometry::Locus::Segment)
            {
                // limit segment endpoints to halfedge vertices ...

                auto segment = ix.segment();

                endpoints[0] = {halfedge_a.id(), halfedge_b.id(), segment.endpoint(0)};

                id_type halfedge_id_a1 = halfedge_a.id();
                if (segment.endpoint(1) == halfedge_a.next().vertex_attributes().position())
                {
                    halfedge_id_a1 = halfedge_a.next_id();
                }

                id_type halfedge_id_b1 = halfedge_b.id();
                if (segment.endpoint(1) == halfedge_b.next().vertex_attributes().position())
                {
                    halfedge_id_b1 = halfedge_b.next_id();
                }

                endpoints[1] = {halfedge_id_a1, halfedge_id_b1, segment.endpoint(1)};

                return endpoints;
            }

            if (ix.locus() == geometry::Locus::Point)
            {
                assert(intersection_count < 2);
                endpoints[intersection_count++] = {halfedge_a.id(), halfedge_b.id(), ix.point()};
            }
        }
    }

    if (intersection_count == 0)
    {
        return std::nullopt;
    }

    return endpoints;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::brep::IntersectionSplitter<Traits>::connections_type quetzal::brep::split_intersection_(const Face<Traits>& face_a, const Face<Traits>& face_b)
{
    std::vector<halfedge_intersection_position<Traits>> intersection_trace;

    for (const auto& halfedge_a : face_a.halfedges())
    {
        for (const auto& halfedge_b : face_b.halfedges())
        {
            auto ix = intersection(halfedge_a, halfedge_b);
            if (ix.locus() != geometry::Locus::Empty)
            {
                continue;
            }

            if (ix.locus() == geometry::Locus::Point)
            {
                // point intersection is only relevant if one face is entirely inside the other except for that point (does this apply here? same internal issue with empty) ...
                return intersection2(face_a, face_b, ix.point());
            }

            assert(ix.locus() == geometry::Locus::Segment);

            // discard or cache any point intersections ...
            // for each endpoint that intersects both face vertices, trace it forward and backward until that is not true ...
            // if halfedges are in the same direction, this is an internal (possibly not exclusively though) intersection, return the polygon ...
            // in the internal case, discard or cache any point intersections ...
            //return intersection_trace of elements {halfedge_a, halfedge_b, position}; // ...
//            return intersection2(/*array of 2 intersection_points*/); // ...
return {};
        }
    }


    return intersection_trace.empty() ? std::nullopt : intersection_trace;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::brep::IntersectionSplitter<Traits>::connections_type quetzal::brep::split_intersection_(const Face<Traits>& face_a, const Face<Traits>& face_b, const halfedge_intersection_position<Traits>& ix)
{
    std::vector<halfedge_intersection_position<Traits>> intersection_trace;

    // start at ix, we have a starting point here ...
    // we can work with it, but we don't know if it is a starting point or an ending point ...

    for (const auto& halfedge_a : face_a.halfedges())
    {
        for (const auto& halfedge_b : face_b.halfedges())
        {
            auto inter = intersection(halfedge_a, halfedge_b);
            if (inter.locus() != geometry::Locus::Empty)
            {
                continue;
            }

            if (inter.locus() == geometry::Locus::Point)
            {
                intersection_trace.emplace_back(halfedge_a.id(), halfedge_b.id(), inter.point());
                continue;
            }

            assert(inter.locus() == geometry::Locus::Segment);

            // discard or cache any point intersections ...
            // for each endpoint that intersects both face vertices, trace it forward and backward until that is not true ...
            // if halfedges are in the same direction, this is an internal (possibly not exclusively though) intersection, return the polygon ...
            // in the internal case, discard or cache any point intersections ...
//            return intersection_trace of elements {halfedge_a, halfedge_b, position}; ...
return {};
        }
    }


    return intersection_trace.empty() ? std::nullopt : intersection_trace;
}

#endif // QUETZAL_BREP_INTERSECT_HPP
