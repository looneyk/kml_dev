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

    // set of intersection sections, each of which can be vertex, edge segment, or a set of paths (polygon)
    // polygon is a pair of branches or edge segments, the first element of which is the one that follows the edges of the face's submesh, the second is the path where the other face's submesh intersects
    // (shared edges complicate this determination)
    // the current trace is just edge segments, and only a single halfedge_id is needed because we can follow the border

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

    // For each intersection point in the trace
    template<typename Traits>
    struct HalfedgeIntersectionPoint
    {
        typename Traits::point_type m_position;
        std::vector<std::array<id_type, 2>> m_halfedge_ids; // {halfedge_id_a, halfedge_id_b} for each path
    };

    // Treats halfedges as half-open intervals, unlike segment intersections
    template<typename Traits>
    bool intersects(const Halfedge<Traits>& halfedge_a, const Halfedge<Traits>& halfedge_b);

    template<typename Traits>
    bool intersects(const Face<Traits>& face_a, const Face<Traits>& face_b);

    // Treats halfedges as half-open intervals, unlike segment intersections
    template<typename Traits>
    geometry::Intersection<typename Traits::vector_traits> intersection(const Halfedge<Traits>& halfedge_a, const Halfedge<Traits>& halfedge_b);

    // Find the intersection between two non-coplanar faces
    // 0: no intersection
    // 1: single point intersection: HalfedgeIntersectionPoint
    // 2: segment intersection: HalfedgeIntersectionPoint for each endpoint
    // Note that the intersection can be a floating point or floating at either or both ends
    template<typename Traits>
    std::vector<HalfedgeIntersectionPoint<Traits>> intersection_linear(const Face<Traits>& face_a, const Face<Traits>& face_b);

    // this has different semantics from the intersection functions in geometry; with brep, areas are halfedge loops (pairs of halfedge paths), not polygons
    // 0: no intersection
    // 1 1: single point intersection: HalfedgeIntersectionPoint
    // 1 1: inclusion: HalfedgeIntersectionPoint with only a or b, the other null
    // 2 1: edge segment intersection: HalfedgeIntersectionPoint for each endpoint
    // 2 2: polygon intersection: HalfedgeIntersectionPoint for each intersection point and for each path (path positions will be duplicative)
    // Note that the intersection can never be floating
    template<typename Traits>
    std::vector<HalfedgeIntersectionPoint<Traits>> intersection_coplanar(const Face<Traits>& face_a, const Face<Traits>& face_b);
    
/* staging for what will eventually move to IntersectionSplitter ...
    template<typename Traits>
    IntersectionSplitter<Traits>::connections_type split_intersection_(const Face<Traits>& face_a, const Face<Traits>& face_b);

    template<typename Traits>
    IntersectionSplitter<Traits>::connections_type split_intersection_(const Face<Traits>& face_a, const Face<Traits>& face_b, const std::vector<HalfedgeIntersectionPoint<Traits>>& ix);
*/
namespace detail
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    id_type half_open_endpoint(const Halfedge<Traits>& halfedge, const geometry::Point<typename Traits::vector_traits>& position)
    {
        return halfedge.next().vertex_attributes().position() == position ? halfedge.next_id() : halfedge.id();
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    id_type float_half_open_endpoint(const Halfedge<Traits>& halfedge, const geometry::Point<typename Traits::vector_traits>& position)
    {
        return float_eq(halfedge.next().vertex_attributes().position(), position) ? halfedge.next_id() : halfedge.id();
    }

} // namespace detail

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
    auto ix = geometry::intersection(to_segment(halfedge_a), to_segment(halfedge_b)); // make sure that float_eq results are coerced to exact values (but what if they are different?) ...
    if (ix.empty())
    {
        return ix;
    }

    if (ix.locus() == geometry::Locus::Point)
    {
        auto position = ix.point();
        //if (float_eq(position, halfedge_a.next().vertex_attributes().position()) || float_eq(position, halfedge_b.next().vertex_attributes().position())) // Reject open end of halfedge interval
        if (detail::half_open_endpoint(halfedge_a, position) != halfedge_a.id() || detail::half_open_endpoint(halfedge_b, position) != halfedge_b.id()) // float_half_open_endpoint? ...
        {
            return {};
        }

        return ix;
    }

    assert(ix.locus() == geometry::Locus::Segment);
    return ix;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::vector<quetzal::brep::HalfedgeIntersectionPoint<Traits>> quetzal::brep::intersection_linear(const Face<Traits>& face_a, const Face<Traits>& face_b)
{
    using geometry::Locus;

    auto trace_direction = normalize(cross(face_a.attributes().normal(), face_b.attributes().normal()));
const auto plane_a = to_plane(face_a);
    const auto plane_b = to_plane(face_b);

    auto ix_a = face_plane_intersection(face_a, plane_b, trace_direction);
    if (!ix_a)
    {
std::cout << "face_plane_intersection a returned nullopt" << std::endl;
        return {};
    }

    auto [id_a0, id_a1, segment_a] = *ix_a;
std::cout << std::format("face_plane_intersection a returned a0 {}, a1 {}, segment {}", id_string(id_a0), id_string(id_a1), segment_a) << std::endl;

    auto ix_b = face_plane_intersection(face_b, plane_a, trace_direction);
    if (!ix_b)
    {
std::cout << "face_plane_intersection b returned nullopt" << std::endl;
        return {};
    }

    auto [id_b0, id_b1, segment_b] = *ix_b;
std::cout << std::format("face_plane_intersection b returned b0 {}, b1 {}, segment {}", id_string(id_b0), id_string(id_b1), segment_b) << std::endl;

    if (!intersects(segment_a, segment_b)) // "skew" polygons
    {
        return {};
    }

    // Handle the single point intersection case (would be nice not to have such duplicative data for this case) ...
    if (id_a1 == nullid)
    {
        assert(id_b1 == nullid);
        auto position = segment_a.endpoint(0);
        assert(float_eq(segment_a.endpoint(1), position));
        assert(float_eq(segment_b.endpoint(0), position));
        assert(float_eq(segment_b.endpoint(1), position));
        return {{position, {{id_a0, id_b0}}}};
    }

    // Intersect the two resulting segments with endpoints ordered by trace_direction

    assert(float_eq(segment_a.direction(), trace_direction));
    assert(float_eq(segment_b.direction(), trace_direction));

    geometry::Segment<typename Traits::vector_traits> segment;

    if (float_eq(segment_a.endpoint(0), segment_b.endpoint(0)))
    {
        if (id_a0 != nullid && vertex_position(face_a.mesh().halfedge(id_a0)) == segment_a.endpoint(0))
        {
            segment.endpoint(0) = segment_a.endpoint(0);
        }
        else if (id_b0 != nullid && vertex_position(face_b.mesh().halfedge(id_b0)) == segment_b.endpoint(0))
        {
            segment.endpoint(0) = segment_b.endpoint(0);
        }
        else
        {
            segment.endpoint(0) = segment_a.endpoint(0);
        }
    }
    else if (farther(segment_b.endpoint(0), segment_a.endpoint(0), trace_direction))
    {
        segment.endpoint(0) = segment_b.endpoint(0);
        id_a0 = nullid;
    }
    else
    {
        segment.endpoint(0) = segment_a.endpoint(0);
        id_b0 = nullid;
    }

    if (float_eq(segment_a.endpoint(1), segment_b.endpoint(1)))
    {
        if (id_a1 != nullid && vertex_position(face_a.mesh().halfedge(id_a1)) == segment_a.endpoint(1))
        {
            segment.endpoint(1) = segment_a.endpoint(1);
        }
        else if (id_b1 != nullid && vertex_position(face_b.mesh().halfedge(id_b1)) == segment_b.endpoint(1))
        {
            segment.endpoint(1) = segment_b.endpoint(1);
        }
        else
        {
            segment.endpoint(1) = segment_a.endpoint(1);
        }
    }
    else if (farther(segment_b.endpoint(1), segment_a.endpoint(1), trace_direction))
    {
        segment.endpoint(1) = segment_a.endpoint(1);
        id_b1 = nullid;
    }
    else
    {
        segment.endpoint(1) = segment_b.endpoint(1);
        id_a1 = nullid;
    }
//std::cout << std::format("s0 {}", segment.endpoint(0)) << std::endl;
//std::cout << std::format("s1 {}", segment.endpoint(1)) << std::endl;

    assert(float_eq(segment.direction(), trace_direction));
    HalfedgeIntersectionPoint<Traits> ix0 = {segment.endpoint(0), {{id_a0, id_b0}}};
    HalfedgeIntersectionPoint<Traits> ix1 = {segment.endpoint(1), {{id_a1, id_b1}}};
    return {ix0, ix1};
}

//------------------------------------------------------------------------------
template<typename Traits>
std::vector<quetzal::brep::HalfedgeIntersectionPoint<Traits>> quetzal::brep::intersection_coplanar(const Face<Traits>& face_a, const Face<Traits>& face_b)
{
    using geometry::Locus;

    std::vector<quetzal::brep::HalfedgeIntersectionPoint<Traits>> intersections;

    for (const auto& halfedge_a : face_a.halfedges())
    {
        for (const auto& halfedge_b : face_b.halfedges())
        {
            auto ix = intersection(halfedge_a, halfedge_b);
            switch (ix.locus())
            {
                case Locus::Empty:
                    continue;

                case Locus::Point:
                    assert(intersections.size() < 2); // only convex faces supported ...
                    intersections.push_back({ix.point(), {{halfedge_a.id(), halfedge_b.id()}}});
                    break;

                case Locus::Segment:
                {
                    auto segment = ix.segment();

                    HalfedgeIntersectionPoint<Traits> intersection_point0 = {segment.endpoint(0), {{halfedge_a.id(), halfedge_b.id()}}};
                    intersections.push_back(intersection_point0);

                    auto position1 = segment.endpoint(1);
                    id_type halfedge_id_a1 = detail::half_open_endpoint(halfedge_a, position1);
                    id_type halfedge_id_b1 = detail::half_open_endpoint(halfedge_b, position1);
                    HalfedgeIntersectionPoint<Traits> intersection_point1 = {segment.endpoint(1), {{halfedge_id_a1, halfedge_id_b1}}};
                    intersections.push_back(intersection_point1);
                    break;
                }

                default:
                    assert(false);
            }
        }
    }

    // Check for the inclusion case
    if (intersections.empty())
    {
        id_type halfedge_id_a = nullid;
        id_type halfedge_id_b = nullid;
        id_type halfedge_id = face_a.halfedge_id();
        if (to_polygon(face_b).contains(vertex_position(face_a.mesh().halfedge(halfedge_id))))
        {
            halfedge_id_a = halfedge_id;
        }
        else
        {
            halfedge_id = face_b.halfedge_id();
            assert(to_polygon(face_b).contains(vertex_position(face_b.mesh().halfedge(halfedge_id))));
            halfedge_id_b = halfedge_id;
        }

        auto position = vertex_position(face_b.mesh().halfedge(halfedge_id));
        intersections.push_back({position, {{halfedge_id_a, halfedge_id_b}}});
    }

    return intersections;
}

/*
//------------------------------------------------------------------------------
template<typename Traits>
quetzal::brep::IntersectionSplitter<Traits>::connections_type quetzal::brep::split_intersection_(const Face<Traits>& face_a, const Face<Traits>& face_b)
{
    std::vector<HalfedgeIntersectionPoint<Traits>> intersection_trace;

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
                // point intersection is only relevant if one face is entirely inside the other except for that point (does this apply here? same detail issue with empty) ...
                return intersection2(face_a, face_b, ix.point());
            }

            assert(ix.locus() == geometry::Locus::Segment);

            // discard or cache any point intersections ...
            // for each endpoint that intersects both face vertices, trace it forward and backward until that is not true ...
            // if halfedges are in the same direction, this is an detail (possibly not exclusively though) intersection, return the polygon ...
            // in the detail case, discard or cache any point intersections ...
            //return intersection_trace of elements {halfedge_a, halfedge_b, position}; // ...
//            return intersection2(-- array of 2 intersection_points --); // ...
return {};
        }
    }


    return intersection_trace.empty() ? std::nullopt : intersection_trace;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::brep::IntersectionSplitter<Traits>::connections_type quetzal::brep::split_intersection_(const Face<Traits>& face_a, const Face<Traits>& face_b, const std::vector<HalfedgeIntersectionPoint<Traits>>& ix)
{
    std::vector<HalfedgeIntersectionPoint<Traits>> intersection_trace;

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
            // if halfedges are in the same direction, this is an detail (possibly not exclusively though) intersection, return the polygon ...
            // in the detail case, discard or cache any point intersections ...
//            return intersection_trace of elements {halfedge_a, halfedge_b, position}; ...
return {};
        }
    }


    return intersection_trace.empty() ? std::nullopt : intersection_trace;
}
*/

#endif // QUETZAL_BREP_INTERSECT_HPP
