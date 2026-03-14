#if !defined(QUETZAL_BREP_GEOMETRY_HPP)
#define QUETZAL_BREP_GEOMETRY_HPP
//------------------------------------------------------------------------------
// brep
// geometry.hpp
//------------------------------------------------------------------------------

#include "id.hpp"
#include "Face.hpp"
#include "Halfedge.hpp"
#include "Mesh.hpp"
#include "Submesh.hpp"
#include "Surface.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include "quetzal/geometry/HalfSpace.hpp"
#include "quetzal/geometry/Line.hpp"
#include "quetzal/geometry/Plane.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/Projection.hpp"
#include "quetzal/geometry/spatial_properties.hpp"
#include "quetzal/geometry/Segment.hpp"
#include "quetzal/geometry/intersect.hpp"
#include "quetzal/geometry/triangle_util.hpp"
#include "quetzal/math/floating_point.hpp"
#include <algorithm>
#include <compare>
#include <optional>
#include <ranges>
#include <vector>

namespace quetzal::brep
{

    // The position of the vertex associated with this halfedge
    template<typename Traits>
    typename Traits::point_type vertex_position(const Halfedge<Traits>& halfedge);

    // The normal of the face associated with this halfedge
    template<typename Traits>
    typename Traits::vector_type face_normal(const Halfedge<Traits>& halfedge);

    // Returns -1, 0, 1: reflex, straight, convex
    // The angle at this halfedge vertex with its next and prev vertices
    // Requires a valid vertex normal for reference
    template<typename Traits> requires (Traits::dimension >= 2 && Traits::dimension <= 3)
    std::partial_ordering orientation(const Halfedge<Traits>& halfedge);

    // The angle at this halfedge with its next and prev
    template<typename Traits>
    bool convex(const Halfedge<Traits>& halfedge);

    // This halfedge and its next and prev
    template<typename Traits>
    bool colinear(const Halfedge<Traits>& halfedge);

    // The angle at this halfedge with its next and prev
    template<typename Traits>
    bool reflex(const Halfedge<Traits>& halfedge);

    // The value of the angle at this halfedge with its next and prev
    template<typename Traits>
    bool angle(const Halfedge<Traits>& halfedge);

    // The triangle at this halfedge with its next and prev
    template<typename Traits>
    bool ear(const Halfedge<Traits>& halfedge);

    // this can't tell if the angle is convex or reflex ...
    template<typename Traits>
    typename Traits::value_type halfedge_vertex_angle(const Halfedge<Traits>& halfedge);

    template<typename Traits>
    typename Traits::value_type length(const Halfedge<Traits>& halfedge);

    // Unit vector
    template<typename Traits>
    typename Traits::vector_type direction(const Halfedge<Traits>& halfedge);

    template<typename Traits>
    typename Traits::vector_type to_vector(const Halfedge<Traits>& halfedge);

    template<typename Traits>
    geometry::Line<typename Traits::vector_traits> to_line(const Halfedge<Traits>& halfedge);

    template<typename Traits>
    geometry::Segment<typename Traits::vector_traits> to_segment(const Halfedge<Traits>& halfedge);

    template<typename Traits>
    bool planar(const Face<Traits>& face);

    template<typename Traits>
    bool convex(const Face<Traits>& face);

    // Returns the halfedge id of the first reflex vertex found
    template<typename Traits>
    std::optional<id_type> reflex_vertex(const Face<Traits>& face);

    template<typename Traits>
    bool degenerate(const Face<Traits>& face);

    // could use geometry functions for these two if we exposed normls the way we expose positions ...
    // Simple wrapper around the corresponding geometry function
    template<typename Traits>
    bool parallel(const Face<Traits>& face_a, const Face<Traits>& face_b);

    // Simple wrapper around the corresponding geometry function
    template<typename Traits>
    bool coplanar(const Face<Traits>& face_a, const Face<Traits>& face_b);

    // This can be used to quickly reject many non-intersections, but if this returns false, there still may not be an actual intersection
    template<typename Traits>
    bool disjoint_plane_crossings(const Face<Traits>& face_a, const Face<Traits>& face_b);

    // all vertices of face_b are not intersectiong and on the same side of the plane of face_a
    template<typename Traits>
    bool disjoint_plane_crossing_unilateral(const Face<Traits>& face_a, const Face<Traits>& face_b);

    template<typename Traits>
    geometry::Plane<typename Traits::vector_traits> to_plane(const Face<Traits>& face);

    template<typename Traits>
    geometry::HalfSpace<typename Traits::vector_traits> to_halfspace(const Face<Traits>& face);

    template<typename Traits>
    geometry::Polygon<typename Traits::vector_traits> to_polygon(const Face<Traits>& face);

    template<typename Traits>
    auto to_polygon(const Face<Traits>& face, const geometry::Projection<typename Traits::vector_traits>& projection) -> geometry::Polygon<typename geometry::Projection<typename Traits::vector_traits>::projected_traits>;

    template<typename Traits>
    geometry::Polygon<typename Traits::vector_traits> to_polygon(const Hole<Traits>& hole);

    template<typename Traits>
    geometry::Plane<typename Traits::vector_traits> to_plane(const Surface<Traits>& face);

    template<typename Traits>
    geometry::HalfSpace<typename Traits::vector_traits> to_halfspace(const Surface<Traits>& face);

    // This follows the seams of the surface and returns a polygon for each. There is no expectation that the returned "polygon" is planar.
    template<typename Traits>
    std::vector<geometry::Polygon<typename Traits::vector_traits>> to_polygon(const Surface<Traits>& surface);

    // This follows the seam of the surface. There is no expectation that the returned "polygon" is planar.
    // This is for surfaces that have a single seam
    template<typename Traits>
    geometry::Polygon<typename Traits::vector_traits> to_polygon(const Surface<Traits>& surface);

    // This follows a seam of a surface. There is no expectation that the returned "polygon" is planar.
    template<typename Traits>
    geometry::Polygon<typename Traits::vector_traits> to_polygon(const Seam<Traits>& seam);
/*
Instead of this, add optional or default identity projection to the functions above (std::ranges pattern) ...
    template<typename Traits>
    geometry::Polygon<typename geometry::Projection<typename Traits::vector_traits>::projected_traits> to_polygon(const Surface<Traits>& surface, const geometry::Projection<typename Traits::vector_traits>& projection);
*/
    // area functions calculate the total signed area based on winding order (CCW positive, CW negative)

    template<typename Traits>
    typename Traits::value_type face_area(const Face<Traits>& face);

    // Works with Surface, Submesh, and Mesh
    template<typename U>
    typename U::value_type surface_area(const U& u);

    // Returns -1, 0, 1: interior, boundary, exterior
    template<typename Traits>
    std::partial_ordering face_compare(const Face<Traits>& face, const typename Traits::point_type& point);

    template<typename Traits>
    bool face_contains(const Face<Traits>& face, const typename Traits::point_type& point);

    // Works with Surface, Submesh, and Mesh
    // Assumes surface is planar
    // No, the way it's currently written, each face in the surface needs to be planar
    template<typename U>
    std::partial_ordering surface_compare(const U& u, const typename U::attributes_type::point_type& point);

    // Works with Surface, Submesh, and Mesh
    template<typename U>
    bool surface_contains(const U& u, const typename U::attributes_type::point_type& point);

// solid_... does not work with bConvex false ...

    // Returns -1, 0, 1: interior, boundary, exterior
    // Works with Submesh and Mesh
    template<typename U>
    std::partial_ordering solid_compare(const U& u, const typename U::attributes_type::point_type& point, bool bConvex = true);

    // Works with Submesh and Mesh
    template<typename U>
    bool solid_contains(const U& u, const typename U::attributes_type::point_type& point, bool bConvex = true);

    // deprecated, now a halfedge method ...
    template<typename Traits>
    geometry::AxisAlignedBoundingBox<typename Traits::vector_traits> bounds(const Halfedge<Traits>& halfedge);

    // deprecated, now supported by spatial_properties, but this is more performant, fix PositionAccessor for Face to be constant_time_access ...
    template<typename Traits>
    geometry::AxisAlignedBoundingBox<typename Traits::vector_traits> bounds(const Face<Traits>& face);

    // Works with Surface, Submesh, and Mesh
    template<typename U>
    geometry::AxisAlignedBoundingBox<typename U::traits_type::vector_traits> bounds(const U& u);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::point_type quetzal::brep::vertex_position(const Halfedge<Traits>& halfedge)
{
    return halfedge.vertex_attributes().position();
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::vector_type quetzal::brep::face_normal(const Halfedge<Traits>& halfedge)
{
    return halfedge.face().attributes().normal();
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 2 && Traits::dimension <= 3)
std::partial_ordering quetzal::brep::orientation(const Halfedge<Traits>& halfedge)
{
    if (colinear(halfedge))
    {
        return std::partial_ordering::equivalent;
    }

    const auto& a = halfedge.prev().vertex_attributes().position();
    const auto& b = halfedge.vertex_attributes().position();
    const auto& c = halfedge.next().vertex_attributes().position();

    if constexpr (Traits::dimension == 3)
    {
        const auto& normal = halfedge.vertex_attributes().normal();
        return math::float_compare0(dot(c - b, cross(normal, b - a)));
    }
    else if constexpr (Traits::dimension == 2)
    {
        return math::float_compare0(dot(c - b, perp(b - a)));
    }
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::brep::convex(const Halfedge<Traits>& halfedge)
{
    return orientation(halfedge) > 0;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::brep::colinear(const Halfedge<Traits>& halfedge)
{
    const auto& a = halfedge.prev().vertex_attributes().position();
    const auto& b = halfedge.vertex_attributes().position();
    const auto& c = halfedge.next().vertex_attributes().position();
    return geometry::colinear(a, b, c);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::brep::reflex(const Halfedge<Traits>& halfedge)
{
    return orientation(halfedge) < 0;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::brep::angle(const Halfedge<Traits>& halfedge)
{
    const auto& a = halfedge.prev().vertex_attributes().position();
    const auto& b = halfedge.vertex_attributes().position();
    const auto& c = halfedge.next().vertex_attributes().position();
    return angle_full(c - b, a - b, halfedge.vertex_attributes().normal());
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::brep::ear(const Halfedge<Traits>& halfedge)
{
    if (!convex(halfedge))
    {
        return false;
    }

    id_type idHalfedgePrev = halfedge.prev_id();

    const auto& position0 = halfedge.prev().vertex_attributes().position();
    const auto& position1 = halfedge.vertex_attributes().position();
    const auto& position2 = halfedge.next().vertex_attributes().position();

    // this looks like implementation for face, not halfedge ...
    const auto& mesh = halfedge.mesh();
    for (id_type id = halfedge.next().next_id(); id != idHalfedgePrev; id = mesh.halfedge(id).next_id())
    {
        if (triangle_contains(position0, position1, position2, mesh.halfedge(id).vertex_attributes().position()))
        {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type quetzal::brep::halfedge_vertex_angle(const Halfedge<Traits>& halfedge)
{
    const auto& position0 = vertex_position(halfedge.prev());
    const auto& position1 = vertex_position(halfedge);
    const auto& position2 = vertex_position(halfedge.next());
    auto d = dot(position0 - position1, position2 - position1);
    return acos(d);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type quetzal::brep::length(const Halfedge<Traits>& halfedge)
{
    return (halfedge.next().vertex_attributes().position() - halfedge.vertex_attributes().position()).norm();
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::vector_type quetzal::brep::direction(const Halfedge<Traits>& halfedge)
{
    return normalize(denoise0(to_vector(halfedge)));
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::vector_type quetzal::brep::to_vector(const Halfedge<Traits>& halfedge)
{
    return halfedge.next().vertex_attributes().position() - halfedge.vertex_attributes().position();
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Line<typename Traits::vector_traits> quetzal::brep::to_line(const Halfedge<Traits>& halfedge)
{
    return {halfedge.vertex_attributes().position(), direction(halfedge)};
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Segment<typename Traits::vector_traits> quetzal::brep::to_segment(const Halfedge<Traits>& halfedge)
{
    return {halfedge.vertex_attributes().position(), halfedge.next().vertex_attributes().position()};
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::brep::planar(const Face<Traits>& face)
{
    return geometry::planar(face, face.attributes().normal());
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::brep::convex(const Face<Traits>& face)
{
    for (const auto& halfedge : face.halfedges())
    {
        if (!convex(halfedge))
        {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::optional<quetzal::id_type> quetzal::brep::reflex_vertex(const Face<Traits>& face)
{
    // this needs to take face normal into account, but is that even reliable in the concave face case? ...
    for (const auto& halfedge : face.halfedges())
    {
        if (reflex(halfedge))
        {
            return halfedge.id();
        }
    }

    return std::nullopt;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::brep::degenerate(const Face<Traits>& face)
{
    for (const auto& halfedge : face.halfedges())
    {
        if (!colinear(halfedge))
        {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::brep::parallel(const Face<Traits>& face_a, const Face<Traits>& face_b)
{
    return geometry::parallel(to_plane(face_a), to_plane(face_b));
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::brep::coplanar(const Face<Traits>& face_a, const Face<Traits>& face_b)
{
    return geometry::coplanar(to_plane(face_a), to_plane(face_b));
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::brep::disjoint_plane_crossings(const Face<Traits>& face_a, const Face<Traits>& face_b)
{
    return disjoint_plane_crossing_unilateral(face_a, face_b) && disjoint_plane_crossing_unilateral(face_b, face_a);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::brep::disjoint_plane_crossing_unilateral(const Face<Traits>& face_a, const Face<Traits>& face_b)
{
    auto plane_a = to_plane(face_a);        
    std::optional<int> d_reference;
    for (const auto& halfedge : face_b.halfedges())
    {
        int d = plane_a.compare(halfedge.vertex_attributes().position());
        if (d == 0)
        {
            return false;
        }

        if (!d_reference)
        {
            d_reference = d;
        }
        else if (d != d_reference)
        {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Plane<typename Traits::vector_traits> quetzal::brep::to_plane(const Face<Traits>& face)
{
    return {face.halfedge().vertex_attributes().position(), face.attributes().normal()};
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::HalfSpace<typename Traits::vector_traits> quetzal::brep::to_halfspace(const Face<Traits>& face)
{
    return {{face.halfedge().vertex_attributes().position(), face.attributes().normal()}};
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Polygon<typename Traits::vector_traits> quetzal::brep::to_polygon(const Face<Traits>& face)
{
    geometry::Polygon<typename Traits::vector_traits> polygon(face.halfedge_count());

    size_t i = 0;
    for (const auto& halfedge : face.halfedges())
    {
        polygon.set_vertex(i++, halfedge.vertex_attributes().position());
    }

    return polygon;
}

//------------------------------------------------------------------------------
template<typename Traits>
auto quetzal::brep::to_polygon(const Face<Traits>& face, const geometry::Projection<typename Traits::vector_traits>& projection) -> geometry::Polygon<typename geometry::Projection<typename Traits::vector_traits>::projected_traits>
{
    using projected_traits = geometry::Projection<typename Traits::vector_traits>::projected_traits;
    geometry::Polygon<projected_traits> polygon(face.halfedge_count());

    size_t i = 0;
    for (const auto& halfedge : face.halfedges())
    {
        polygon.set_vertex(i++, projection(halfedge.vertex_attributes().position()));
    }

    return polygon;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Polygon<typename Traits::vector_traits> quetzal::brep::to_polygon(const Hole<Traits>& hole)
{
    geometry::Polygon<typename Traits::vector_traits> polygon(hole.halfedge_count());

    size_t i = 0;
    for (const auto& halfedge : hole.halfedges())
    {
        polygon.set_vertex(i++, halfedge.vertex_attributes().position());
    }

    return polygon;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Plane<typename Traits::vector_traits> quetzal::brep::to_plane(const Surface<Traits>& surface)
{
    return {surface.faces().front().halfedge().vertex_attributes().position(), surface.attributes().normal()};
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::HalfSpace<typename Traits::vector_traits> quetzal::brep::to_halfspace(const Surface<Traits>& surface)
{
    return {{surface.faces().front().halfedge().vertex_attributes().position(), surface.attributes().normal()}};
}

//------------------------------------------------------------------------------
template<typename Traits>
std::vector<quetzal::geometry::Polygon<typename Traits::vector_traits>> quetzal::brep::to_polygon(const Surface<Traits>& surface)
{
    std::vector<geometry::Polygon<typename Traits::vector_traits>> polygons(surface.seam_count());

    size_t i = 0;
    for (const auto& seam : surface.seams())
    {
        polygons[i++] = to_polygon(seam);
    }

    return polygons;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Polygon<typename Traits::vector_traits> quetzal::brep::to_polygon(const Surface<Traits>& surface)
{
    assert(surface.seam_count() == 1);
    return to_polygon(surface.seams().front());
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Polygon<typename Traits::vector_traits> quetzal::brep::to_polygon(const Seam<Traits>& seam)
{
    geometry::Polygon<typename Traits::vector_traits> polygon(seam.halfedge_count());

    size_t i = 0;
    for (const auto& halfedge : seam.halfedges())
    {
        polygon.set_vertex(i++, halfedge.vertex_attributes().position());
    }

    return polygon;
}
/*
//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Polygon<typename quetzal::geometry::Projection<typename Traits::vector_traits>::projected_traits> quetzal::brep::to_polygon(const Surface<Traits>& surface, const quetzal::geometry::Projection<typename Traits::vector_traits>& projection)
{
    assert(surface.seam_count() == 1);

    using projected_traits = geometry::Projection<typename Traits::vector_traits>::projected_traits;
    const auto& seam = surface.seams().front();
    geometry::Polygon<projected_traits> polygon(seam.halfedge_count());

    size_t i = 0;
    for (const auto& halfedge : seam.halfedges())
    {
        polygon.set_vertex(i++, projection(halfedge.vertex_attributes().position()));
    }

    return polygon;
}
*/
//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type quetzal::brep::face_area(const Face<Traits>& face)
{
    assert(!face.deleted());
    assert(face.halfedge_count() >= 3);

    const auto normal = face.attributes().normal();

    if (face.halfedge_count() == 3)
    {
        const auto& halfedge = face.halfedge();
        const auto& position0 = halfedge.prev().vertex_attributes().position();
        const auto& position1 = halfedge.vertex_attributes().position();
        const auto& position2 = halfedge.next().vertex_attributes().position();

        return triangle_area(position0, position1, position2, normal);
    }

    return to_polygon(face).signed_area(normal);
}

//------------------------------------------------------------------------------
template<typename U>
typename U::value_type quetzal::brep::surface_area(const U& u)
{
    typename U::value_type area = U::val(0);

    for (const auto& face : u.faces())
    {
        area += face_area(face);
    }

    return area;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::partial_ordering quetzal::brep::face_compare(const Face<Traits>& face, const typename Traits::point_type& point)
{
    assert(!face.deleted());
    assert(face.halfedge_count() >= 3);

    if (face.halfedge_count() == 3)
    {
        const auto& halfedge = face.halfedge();
        return geometry::triangle_compare(halfedge.prev().vertex_attributes().position(), halfedge.vertex_attributes().position(), halfedge.next().vertex_attributes().position(), point);
    }

    if (!to_polygon(face).planar() || !to_plane(face).contains(point))
    {
        return std::partial_ordering::greater;
    }

    geometry::Projection<typename Traits::vector_traits> projection(face.attributes().normal());
    return to_polygon(face, projection).compare(projection(point));
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::brep::face_contains(const Face<Traits>& face, const typename Traits::point_type& point)
{
    return face_compare(face, point) <= 0;
}

//------------------------------------------------------------------------------
template<typename U>
std::partial_ordering quetzal::brep::surface_compare(const U& u, const typename U::attributes_type::point_type& point)
{
    // Or do polygon_compare with surface seam ...

    for (const auto& face : u.faces())
    {
        auto c = face_compare(face, point);
        if (c == std::partial_ordering::greater)
        {
            continue;
        }

        if (c == std::partial_ordering::equivalent)
        {
            // check intersecting edge, if not on edge of surface, c = 0 ...
            // redo intersection tests for face edges or face_compare, polygon_compare that rerurns halfedge_id, i ...
        }

        return c;
    }

    return std::partial_ordering::greater;
}

//------------------------------------------------------------------------------
template<typename U>
bool quetzal::brep::surface_contains(const U& u, const typename U::attributes_type::point_type& point)
{
    for (const auto& face : u.faces())
    {
        if (face_contains(face, point))
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
template<typename U>
std::partial_ordering quetzal::brep::solid_compare(const U& u, const typename U::attributes_type::point_type& point, bool bConvex)
{
    if (bConvex)
    {
        bool bBoundary = false;

        for (const auto& face : u.faces())
        {
            auto d = dot(face.halfedge().vertex_attributes().position() - point, face.attributes().normal());
            if (math::float_eq0(d))
            {
                bBoundary = true;
            }
            else if (d < U::val(0))
            {
                return std::partial_ordering::greater;
            }
        }

        return bBoundary ? std::partial_ordering::equivalent : std::partial_ordering::less;
    }

// neeed better algorithm, like one that works ...

    using T = U::value_type;

    u.reset_all(); // reset_faces would work here ...

    geometry::Ray ray(point, {T(1), T(0), T(0)});
    size_t n = 0;

    for (const auto& face : u.faces())
    {
        if (intersects(ray, to_polygon(face)))
        {
            for (const auto& halfedge : face.halfedges())
            {
                // Skip next position so that each vertex is only checked once
                if (float_eq(halfedge.next().vertex_attributes().position(), point))
                {
                    continue;
                }

                // Check for vertex and edge intersection so that adjoining vertices or edge can be marked so that intersection will only count once

                if (float_eq(halfedge.vertex_attributes().position(), point))
                {
                    for (const auto& h : halfedge.vertex().halfedges())
                    {
                        h.face().set_marked();
                    }

                    break;
                }

                if (intersects(ray, to_segment(halfedge)))
                {
                    halfedge.partner().face().set_marked();
                    break;
                }
            }

            face.set_marked();
            ++n;
        }
    }

    return math::odd(n) ? std::partial_ordering::less : std::partial_ordering::greater;
}

//------------------------------------------------------------------------------
template<typename U>
bool quetzal::brep::solid_contains(const U& u, const typename U::attributes_type::point_type& point, bool bConvex)
{
    return solid_compare(u, point, bConvex) <= 0;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::AxisAlignedBoundingBox<typename Traits::vector_traits> quetzal::brep::bounds(const Halfedge<Traits>& halfedge)
{
    geometry::AxisAlignedBoundingBox<typename Traits::vector_traits> box{};
    box.insert(halfedge.vertex_attributes().position());
    box.insert(halfedge.next().vertex_attributes().position());
    return box;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::AxisAlignedBoundingBox<typename Traits::vector_traits> quetzal::brep::bounds(const Face<Traits>& face)
{
    geometry::AxisAlignedBoundingBox<typename Traits::vector_traits> box{};
    for (const auto& halfedge : face.halfedges())
    {
        box.insert(halfedge.vertex_attributes().position());
    }
    return box;
}

//------------------------------------------------------------------------------
template<typename U>
quetzal::geometry::AxisAlignedBoundingBox<typename U::traits_type::vector_traits> quetzal::brep::bounds(const U& u)
{
    geometry::AxisAlignedBoundingBox<typename U::traits_type> box{};
    for (const auto& face : u.faces())
    {
        for (const auto& halfedge : face.halfedges())
        {
            box.insert(halfedge.vertex_attributes().position());
        }
    }
    return box;
}

#endif // QUETZAL_BREP_GEOMETRY_HPP
