#if !defined(QUETZAL_GEOMETRY_INTERSECT_HPP)
#define QUETZAL_GEOMETRY_INTERSECT_HPP
//------------------------------------------------------------------------------
// geometry
// intersect.hpp
//
// Functions that have polygon parameters only support planar convex polygons
// Polygon intersections that return segments could be edge intersections (ok) or internal intersections (not ok)
// We have yet to be able to make the distinction ...
//
//------------------------------------------------------------------------------

#include "HalfSpace.hpp"
#include "Intersection.hpp"
#include "Line.hpp"
#include "Plane.hpp"
#include "Point.hpp"
#include "Polygon.hpp"
#include "Ray.hpp"
#include "Segment.hpp"
#include "distance.hpp"
#include "relationships.hpp"
#include "quetzal/math/floating_point.hpp"
#include <map>
#include <vector>
#include <cassert>

//#include "quetzal/geometry/Projection.hpp"
//#include "quetzal/math/CartesianTraits.hpp"

namespace quetzal::geometry
{

    // With polygons, there is no expectation that these will work with non-convex, and certainly none with non-planar, polygons
    // consider supporting the return of a set of intersections for polygons so that the non-convex case can be covered ...
    // now have intersection of intersections, might also be useful to have union (sort of), the super-segment containing all linear intersections ...
    // split it out of intersection_non_convex np ...

    template<typename Traits>
    bool intersects(const Point<Traits>& point_a, const Point<Traits>& point_b);

    template<typename Traits>
    bool intersects(const Point<Traits>& point, const Line<Traits>& line);

    template<typename Traits>
    bool intersects(const Point<Traits>& point, const Ray<Traits>& ray);

    template<typename Traits>
    bool intersects(const Point<Traits>& point, const Segment<Traits>& segment);

    template<typename Traits> requires (Traits::dimension >= 3)
    bool intersects(const Point<Traits>& point, const Plane<Traits>& plane);

    template<typename Traits>
    bool intersects(const Point<Traits>& point, const Polygon<Traits>& polygon);

    template<typename Traits>
    bool intersects(const Line<Traits>& line, const Point<Traits>& point);

    template<typename Traits>
    bool intersects(const Line<Traits>& line_a, const Line<Traits>& line_b);

    template<typename Traits>
    bool intersects(const Line<Traits>& line, const Ray<Traits>& ray);

    template<typename Traits>
    bool intersects(const Line<Traits>& line, const Segment<Traits>& segment);

    template<typename Traits> requires (Traits::dimension >= 3)
    bool intersects(const Line<Traits>& line, const Plane<Traits>& plane);

    template<typename Traits>
    bool intersects(const Line<Traits>& line, const Polygon<Traits>& polygon);

    template<typename Traits>
    bool intersects(const Ray<Traits>& ray, const Point<Traits>& point);

    template<typename Traits>
    bool intersects(const Ray<Traits>& ray, const Line<Traits>& line);

    template<typename Traits>
    bool intersects(const Ray<Traits>& ray_a, const Ray<Traits>& ray_b);

    template<typename Traits>
    bool intersects(const Ray<Traits>& ray, const Segment<Traits>& segment);

    template<typename Traits> requires (Traits::dimension >= 3)
    bool intersects(const Ray<Traits>& ray, const Plane<Traits>& plane);

    template<typename Traits>
    bool intersects(const Ray<Traits>& ray, const Polygon<Traits>& polygon);

    template<typename Traits>
    bool intersects(const Segment<Traits>& segment, const Point<Traits>& point);

    template<typename Traits>
    bool intersects(const Segment<Traits>& segment, const Line<Traits>& line);

    template<typename Traits>
    bool intersects(const Segment<Traits>& segment, const Ray<Traits>& ray);

    template<typename Traits>
    bool intersects(const Segment<Traits>& segment_a, const Segment<Traits>& segment_b);

    template<typename Traits> requires (Traits::dimension >= 3)
    bool intersects(const Segment<Traits>& segment, const Plane<Traits>& plane);

    template<typename Traits>
    bool intersects(const Segment<Traits>& segment, const Polygon<Traits>& polygon);

    template<typename Traits> requires (Traits::dimension >= 3)
    bool intersects(const Plane<Traits>& plane, const Point<Traits>& point);

    template<typename Traits> requires (Traits::dimension >= 3)
    bool intersects(const Plane<Traits>& plane, const Line<Traits>& line);

    template<typename Traits> requires (Traits::dimension >= 3)
    bool intersects(const Plane<Traits>& plane, const Ray<Traits>& ray);

    template<typename Traits> requires (Traits::dimension >= 3)
    bool intersects(const Plane<Traits>& plane, const Segment<Traits>& segment);

    template<typename Traits> requires (Traits::dimension >= 3)
    bool intersects(const Plane<Traits>& plane_a, const Plane<Traits>& plane_b);

    template<typename Traits> requires (Traits::dimension >= 3)
    bool intersects(const Plane<Traits>& plane, const Polygon<Traits>& polygon);

    template<typename Traits>
    bool intersects(const Polygon<Traits>& polygon, const Point<Traits>& point);

    template<typename Traits>
    bool intersects(const Polygon<Traits>& polygon, const Line<Traits>& line);

    template<typename Traits>
    bool intersects(const Polygon<Traits>& polygon, const Ray<Traits>& ray);

    template<typename Traits>
    bool intersects(const Polygon<Traits>& polygon, const Segment<Traits>& segment);

    template<typename Traits> requires (Traits::dimension >= 3)
    bool intersects(const Polygon<Traits>& polygon, const Plane<Traits>& plane);

    template<typename Traits>
    bool intersects(const Polygon<Traits>& polygon_a, const Polygon<Traits>& polygon_b);

    template<typename Traits>
    Intersection<Traits> intersection(const Point<Traits>& point_a, const Point<Traits>& point_b);

    template<typename Traits>
    Intersection<Traits> intersection(const Point<Traits>& point, const Line<Traits>& line);

    template<typename Traits>
    Intersection<Traits> intersection(const Point<Traits>& point, const Ray<Traits>& ray);

    template<typename Traits>
    Intersection<Traits> intersection(const Point<Traits>& point, const Segment<Traits>& segment);

    template<typename Traits> requires (Traits::dimension >= 3)
    Intersection<Traits> intersection(const Point<Traits>& point, const Plane<Traits>& plane);

    template<typename Traits>
    Intersection<Traits> intersection(const Point<Traits>& point, const Polygon<Traits>& polygon);

    template<typename Traits>
    Intersection<Traits> intersection(const Line<Traits>& line, const Point<Traits>& point);

    template<typename Traits>
    Intersection<Traits> intersection(const Line<Traits>& line_a, const Line<Traits>& line_b);

    template<typename Traits>
    Intersection<Traits> intersection(const Line<Traits>& line, const Ray<Traits>& ray);

    template<typename Traits>
    Intersection<Traits> intersection(const Line<Traits>& line, const Segment<Traits>& segment);

    template<typename Traits> requires (Traits::dimension >= 3)
    Intersection<Traits> intersection(const Line<Traits>& line, const Plane<Traits>& plane);

    template<typename Traits>
    Intersection<Traits> intersection(const Line<Traits>& line, const Polygon<Traits>& polygon);

    template<typename Traits>
    Intersection<Traits> intersection(const Ray<Traits>& ray, const Point<Traits>& point);

    template<typename Traits>
    Intersection<Traits> intersection(const Ray<Traits>& ray, const Line<Traits>& line);

    template<typename Traits>
    Intersection<Traits> intersection(const Ray<Traits>& ray_a, const Ray<Traits>& ray_b);

    template<typename Traits>
    Intersection<Traits> intersection(const Ray<Traits>& ray, const Segment<Traits>& segment);

    template<typename Traits> requires (Traits::dimension >= 3)
    Intersection<Traits> intersection(const Ray<Traits>& ray, const Plane<Traits>& plane);

    template<typename Traits>
    Intersection<Traits> intersection(const Ray<Traits>& ray, const Polygon<Traits>& polygon);

    template<typename Traits>
    Intersection<Traits> intersection(const Segment<Traits>& segment, const Point<Traits>& point);

    template<typename Traits>
    Intersection<Traits> intersection(const Segment<Traits>& segment, const Line<Traits>& line);

    template<typename Traits>
    Intersection<Traits> intersection(const Segment<Traits>& segment, const Ray<Traits>& ray);

    template<typename Traits>
    Intersection<Traits> intersection(const Segment<Traits>& segment_a, const Segment<Traits>& segment_b);

    template<typename Traits> requires (Traits::dimension >= 3)
    Intersection<Traits> intersection(const Segment<Traits>& segment, const Plane<Traits>& plane);

    // Returned segment intersections preserve the original segment direction
    template<typename Traits>
    Intersection<Traits> intersection(const Segment<Traits>& segment, const Polygon<Traits>& polygon);

    template<typename Traits> requires (Traits::dimension >= 3)
    Intersection<Traits> intersection(const Plane<Traits>& plane, const Point<Traits>& point);

    template<typename Traits> requires (Traits::dimension >= 3)
    Intersection<Traits> intersection(const Plane<Traits>& plane, const Line<Traits>& line);

    template<typename Traits> requires (Traits::dimension >= 3)
    Intersection<Traits> intersection(const Plane<Traits>& plane, const Ray<Traits>& ray);

    template<typename Traits> requires (Traits::dimension >= 3)
    Intersection<Traits> intersection(const Plane<Traits>& plane, const Segment<Traits>& segment);

    template<typename Traits> requires (Traits::dimension >= 3)
    Intersection<Traits> intersection(const Plane<Traits>& plane_a, const Plane<Traits>& plane_b);

    template<typename Traits> requires (Traits::dimension >= 3)
    Intersection<Traits> intersection(const Plane<Traits>& plane, const Polygon<Traits>& polygon);

    template<typename Traits> requires (Traits::dimension >= 3)
    std::vector<Intersection<Traits>> intersection_non_convex(const Plane<Traits>& plane, const Polygon<Traits>& polygon);

    template<typename Traits>
    Intersection<Traits> intersection(const Polygon<Traits>& polygon, const Point<Traits>& point);

    template<typename Traits>
    Intersection<Traits> intersection(const Polygon<Traits>& polygon, const Line<Traits>& line);

    template<typename Traits>
    Intersection<Traits> intersection(const Polygon<Traits>& polygon, const Ray<Traits>& ray);

    template<typename Traits>
    Intersection<Traits> intersection(const Polygon<Traits>& polygon, const Segment<Traits>& segment);

    template<typename Traits> requires (Traits::dimension >= 3)
    Intersection<Traits> intersection(const Polygon<Traits>& polygon, const Plane<Traits>& plane);

    template<typename Traits>
    Intersection<Traits> intersection(const Polygon<Traits>& polygon_a, const Polygon<Traits>& polygon_b);

    template<typename Traits>
    std::vector<Intersection<Traits>> intersection_non_convex(const Polygon<Traits>& polygon_a, const Polygon<Traits>& polygon_b);

    template<typename Traits> requires (Traits::dimension >= 3)
    Intersection<Traits> intersection(const Plane<Traits>& plane_a, const Plane<Traits>& plane_b, const Plane<Traits>& plane_c);

    // Supports finite extent: point, segment, polygon
    template<typename Traits>
    Intersection<Traits> intersection(const Intersection<Traits>& ix_a, const Intersection<Traits>& ix_b);

    // Supports finite extent: point, segment, polygon
    // All intersection psitions should be colinear
    template<typename Traits>
    std::vector<Intersection<Traits>> intersection_colinear(std::span<const Intersection<Traits>> ixs_a, std::span<const Intersection<Traits>> ixs_b);

    template<typename Traits>
    bool intersects_coplanar(const Point<Traits>& point, const Polygon<Traits>& polygon);

    // line_util? ...

    template<typename Traits>
    Line<Traits> to_line(const Ray<Traits>& ray)
    {
        return Line<Traits>(ray.endpoint(), ray.direction());
    }

    template<typename Traits>
    Line<Traits> to_line(const Segment<Traits>& segment)
    {
        return Line<Traits>(segment.endpoint(0), segment.direction());
    }

    // Plane, plane_util? ...

    template<typename Traits> requires (Traits::dimension >= 3)
    bool coplanar(const Plane<Traits>& plane_a, const Plane<Traits>& plane_b)
    {
        return parallel(plane_a.normal(), plane_b.normal()) && plane_a.contains(plane_b.point());
    }

    // try to remove the constraint ...
    template<typename Traits> requires (Traits::dimension == 3)
    std::array<typename Traits::value_type, Traits::dimension> components(math::Vector<Traits> v)
    {
        return {v.data()[0], v.data()[1], v.data()[2]};
    }

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Point<Traits>& point_a, const Point<Traits>& point_b)
{
    return float_eq(point_a, point_b);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Point<Traits>& point, const Line<Traits>& line)
{
    return line.contains(point);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Point<Traits>& point, const Ray<Traits>& ray)
{
    return ray.contains(point);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Point<Traits>& point, const Segment<Traits>& segment)
{
    return segment.contains(point);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
bool quetzal::geometry::intersects(const Point<Traits>& point, const Plane<Traits>& plane)
{
    return plane.contains(point);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Point<Traits>& point, const Polygon<Traits>& polygon)
{
    return polygon.contains(point);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Line<Traits>& line, const Point<Traits>& point)
{
    return intersects(point, line);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Line<Traits>& line_a, const Line<Traits>& line_b)
{
    if (parallel(line_a, line_b))
    {
        return line_b.contains(line_a.point());
    }

    if constexpr (Traits::dimension == 3)
    {
        return math::float_eq0(distance(line_a, line_b));
    }
    else if constexpr (Traits::dimension == 2)
    {
        return true;
    }
    else
    {
        static_assert(Traits::dimension == 2 || Traits::dimension == 3, "Dimension not supported.");
    }
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Line<Traits>& line, const Ray<Traits>& ray)
{
    if (parallel(line, ray))
    {
        return line.contains(ray.point());
    }

    auto [s, t] = closest_point_parameters(line, to_line(ray));
    return math::float_ge0(t) && math::float_eq0((line.point(s) - ray.endpoint(t)).norm_squared()); // point_a == point_b ...
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Line<Traits>& line, const Segment<Traits>& segment)
{
    if (parallel(line, segment))
    {
        return line.contains(segment.endpoint(0));
    }

    auto [s, t] = closest_point_parameters(line.point(), line.direction(), segment.endpoint(0), segment.vector());
    return math::float_clamped01(t) && math::float_eq0((line.point(s) - segment.point(t)).norm_squared()); // point_a == point_b ...
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
bool quetzal::geometry::intersects(const Line<Traits>& line, const Plane<Traits>& plane)
{
    return !parallel(line, plane) || plane.contains(line.point());
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Line<Traits>& line, const Polygon<Traits>& polygon)
{
    if constexpr (Traits::dimension >= 3)
    {
        Intersection<Traits> ix = intersection(line, polygon.plane());
        if (ix.empty())
        {
            return false;
        }
        else if (ix.locus() == Locus::Point)
        {
            return polygon.contains(ix.point()); // but no need for plane check ...
        }

        assert(ix.locus() == Locus::Line);
    }

    for (size_t i = 0; i < polygon.edge_count(); ++i)
    {
        if (intersects(line, polygon.edge(i)))
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Ray<Traits>& ray, const Point<Traits>& point)
{
    return intersects(point, ray);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Ray<Traits>& ray, const Line<Traits>& line)
{
    return intersects(line, ray);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Ray<Traits>& ray_a, const Ray<Traits>& ray_b)
{
    if (parallel(ray_a, ray_b))
    {
        return ray_a.contains(ray_b.point()) || ray_b.contains(ray_a.point());
    }

    auto [s, t] = closest_point_parameters(to_line(ray_a), to_line(ray_b));
    return math::float_ge0(s) && math::float_ge0(t) && math::float_eq0((ray_a.point(s) - ray_b.point(t)).norm_squared()); // point_a == point_b ...
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Ray<Traits>& ray, const Segment<Traits>& segment)
{
    if (parallel(ray, segment))
    {
        return ray.contains(segment.endpoint(0)) || ray.contains(segment.endpoint(1));
    }

    auto [s, t] = closest_point_parameters(ray.endpoint(), ray.direction(), segment.endpoint(0), segment.vector());
    return math::float_ge0(s) && math::float_clamped01(t) && math::float_eq0((ray.point(s) - segment.point(t)).norm_squared()); // point_a == point_b ...
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
bool quetzal::geometry::intersects(const Ray<Traits>& ray, const Plane<Traits>& plane)
{
    if (parallel(ray, plane))
    {
        return plane.contains(ray.endpoint());
    }

    auto d0 = dot(plane.normal(), ray.endpoint() - plane.point());
    auto d1 = dot(plane.normal(), ray.direction());
    assert(!math::float_eq0(d0));
    assert(!math::float_eq0(d1));
    return d0 < 0 && d1 > 0 || d0 > 0 && d1 < 0;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Ray<Traits>& ray, const Polygon<Traits>& polygon)
{
    if constexpr (Traits::dimension >= 3)
    {
        Intersection<Traits> ix = intersection(ray, polygon.plane());
        if (ix.empty())
        {
            return false;
        }
        else if (ix.locus() == Locus::Point)
        {
            return polygon.contains(ix.point()); // but no need for plane check ...
        }

        assert(ix.locus() == Locus::Ray);
    }

    for (size_t i = 0; i < polygon.edge_count(); ++i)
    {
        if (intersects(ray, polygon.edge(i)))
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Segment<Traits>& segment, const Point<Traits>& point)
{
    return intersects(point, segment);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Segment<Traits>& segment, const Line<Traits>& line)
{
    return intersects(line, segment);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Segment<Traits>& segment, const Ray<Traits>& ray)
{
    return intersects(ray, segment);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Segment<Traits>& segment_a, const Segment<Traits>& segment_b)
{
    if (parallel(segment_a, segment_b))
    {
        return segment_a.contains(segment_b.endpoint(0)) || segment_a.contains(segment_b.endpoint(1))
            || segment_b.contains(segment_a.endpoint(0)) || segment_b.contains(segment_a.endpoint(1));
    }

    auto [s, t] = closest_point_parameters(segment_a.endpoint(0), segment_a.vector(), segment_b.endpoint(0), segment_b.vector());
    return math::float_clamped01(s) && math::float_clamped01(t) && math::float_eq0((segment_a.point(s) - segment_b.point(t)).norm_squared()); // point_a == point_b ...
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
bool quetzal::geometry::intersects(const Segment<Traits>& segment, const Plane<Traits>& plane)
{
    if constexpr(Traits::dimension == 2)
    {
        return true;
    }
    else if constexpr(Traits::dimension >= 3)
    {
        HalfSpace<Traits> halfspace{plane, plane.normal()};
        auto p = halfspace.compare(segment.endpoint(0));
        auto q = halfspace.compare(segment.endpoint(1));
        return p != q || p == 0;
    }
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Segment<Traits>& segment, const Polygon<Traits>& polygon)
{
    if constexpr (Traits::dimension >= 3)
    {
        Intersection<Traits> ix = intersection(segment, polygon.plane());
        if (ix.empty())
        {
            return false;
        }
        else if (ix.locus() == Locus::Point)
        {
            return polygon.contains(ix.point());
        }

        assert(ix.locus() == Locus::Segment);
    }

    if (polygon.contains(segment.endpoint(0)) || polygon.contains(segment.endpoint(1)))
    {
        return true;
    }

    for (size_t i = 0; i < polygon.edge_count(); ++i)
    {
        if (intersects(segment, polygon.edge(i)))
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
bool quetzal::geometry::intersects(const Plane<Traits>& plane, const Point<Traits>& point)
{
    return intersects(point, plane);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
bool quetzal::geometry::intersects(const Plane<Traits>& plane, const Line<Traits>& line)
{
    return intersects(line, plane);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
bool quetzal::geometry::intersects(const Plane<Traits>& plane, const Ray<Traits>& ray)
{
    return intersects(ray, plane);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
bool quetzal::geometry::intersects(const Plane<Traits>& plane, const Segment<Traits>& segment)
{
    return intersects(segment, plane);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
bool quetzal::geometry::intersects(const Plane<Traits>& plane_a, const Plane<Traits>& plane_b)
{
    return !parallel(plane_a, plane_b) || plane_a.contains(plane_b.point());
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
bool quetzal::geometry::intersects(const Plane<Traits>& plane, const Polygon<Traits>& polygon)
{
    for (size_t i = 0; i < polygon.edge_count(); ++i)
    {
        if (intersects(polygon.edge(i), plane))
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Polygon<Traits>& polygon, const Point<Traits>& point)
{
    return intersects(point, polygon);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Polygon<Traits>& polygon, const Line<Traits>& line)
{
    return intersects(line, polygon);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Polygon<Traits>& polygon, const Ray<Traits>& ray)
{
    return intersects(ray, polygon);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Polygon<Traits>& polygon, const Segment<Traits>& segment)
{
    return intersects(segment, polygon);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
bool quetzal::geometry::intersects(const Polygon<Traits>& polygon, const Plane<Traits>& plane)
{
    return intersects(plane, polygon);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects(const Polygon<Traits>& polygon_a, const Polygon<Traits>& polygon_b)
{
    if (disjoint(polygon_a.bounds(), polygon_b.bounds()))
    {
        return false;
    }

    // Need to check both ways as one polygon's intersection may be entirely within the other

    for (size_t i = 0; i < polygon_a.edge_count(); ++i)
    {
        if (intersects(polygon_a.edge(i), polygon_b))
        {
            return true;
        }
    }

    for (size_t i = 0; i < polygon_b.edge_count(); ++i)
    {
        if (intersects(polygon_b.edge(i), polygon_a))
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Point<Traits>& point_a, const Point<Traits>& point_b)
{
    if (float_eq(point_a, point_b))
    {
        return {point_a};
    }

    return {};
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Point<Traits>& point, const Line<Traits>& line)
{
    if (line.contains(point))
    {
        return {point};
    }

    return {};
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Point<Traits>& point, const Ray<Traits>& ray)
{
    if (ray.contains(point))
    {
        return {point};
    }

    return {};
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Point<Traits>& point, const Segment<Traits>& segment)
{
    if (segment.contains(point))
    {
        return {point};
    }

    return {};
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Point<Traits>& point, const Plane<Traits>& plane)
{
    if (plane.contains(point))
    {
        return {point};
    }

    return {};
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Point<Traits>& point, const Polygon<Traits>& polygon)
{
    if (polygon.contains(point))
    {
        return {point};
    }

    return {};
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Line<Traits>& line, const Point<Traits>& point)
{
    return intersection(point, line);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Line<Traits>& line_a, const Line<Traits>& line_b)
{
    auto [s, t] = closest_point_parameters(line_a, line_b);
    if (math::float_eq0((line_a.point(s) - line_b.point(t)).norm_squared())) // point_a == point_b ...
    {
        assert(line_a.contains(line_b.point(t)));
        assert(line_b.contains(line_a.point(s)));

        if (parallel(line_a, line_b))
        {
            return {line_a};
        }

        return {line_a.point(s)};
    }

    return {};
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Line<Traits>& line, const Ray<Traits>& ray)
{
    auto [s, t] = closest_point_parameters(line, to_line(ray));
    if (math::float_ge0(t) && math::float_eq0((line.point(s) - ray.point(t)).norm_squared())) // point_a == point_b ...
    {
        assert(line.contains(ray.point(t)));
        assert(ray.contains(line.point(s)));

        if (parallel(line, ray))
        {
            return {ray};
        }

        return {line.point(s)};
    }

    return {};
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Line<Traits>& line, const Segment<Traits>& segment)
{
    auto [s, t] = closest_point_parameters(line.point(), line.direction(), segment.endpoint(0), segment.vector());
    if (math::float_clamped01(t) && math::float_eq0((line.point(s) - segment.point(t)).norm_squared())) // point_a == point_b ...
    {
        assert(line.contains(segment.point(t)));
        assert(segment.contains(line.point(s)));

        if (parallel(line, segment))
        {
            return {segment};
        }

        return {line.point(s)};
    }

    return {};
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Line<Traits>& line, const Plane<Traits>& plane)
{
    assert(plane.normal().unit());
    assert(line.direction().unit());

    auto num = dot(plane.point() - line.point(), plane.normal());
    auto denom = dot(line.direction(), plane.normal());

    if (math::float_eq0(denom)) // parallel
    {
        if (math::float_eq0(num)) // in plane
        {
            return {line};
        }

        return {};
    }

    auto t = num / denom;
    auto point = line.point(t);

    assert(plane.contains(point));
    return {point};
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Line<Traits>& line, const Polygon<Traits>& polygon)
{
    if constexpr (Traits::dimension >= 3)
    {
        Intersection<Traits> ix = intersection(line, polygon.plane());
        if (ix.empty())
        {
            return {};
        }
        else if (ix.locus() == Locus::Point)
        {
            return intersection(ix.point(), polygon); // just return ix if intersects_coplanar, else return {}? ...
        }

        assert(ix.locus() == Locus::Line);
    }

    std::vector<Intersection<Traits>> intersections;
    for (size_t i = 0; i < polygon.edge_count(); ++i)
    {
        auto segment = polygon.edge(i);
        Intersection<Traits> ix = intersection(line, segment);
        if (ix.locus() == Locus::Segment)
        {
//            assert(intersections.empty() || (intersections.size() == 1 && intersections.front().locus() == Locus::Point && float_eq(intersections.front().point(), segment.endpoint(1))));
            assert(ix.segment() == segment);
            return ix;
        }

        // Don't consider second segment endpoint so vertex intersections don't get counted twice
        if (ix.locus() == Locus::Point && !line.contains(segment.endpoint(1)))
        {
            if (float_eq(ix.point(), segment.endpoint(0)))
            {
                ix = segment.endpoint(0); // denoise
            }
            intersections.push_back(ix);
        }
    }

    if (intersections.size() == 1)
    {
        assert(intersections[0].locus() == Locus::Point);
        return intersections[0];
    }

    if (intersections.size() == 2)
    {
        assert(intersections[0].locus() == Locus::Point);
        assert(intersections[1].locus() == Locus::Point);

        // Align segment with line direction
        auto segment = Segment<Traits>(intersections[0].point(), intersections[1].point());
        if (direction_compare(segment.direction(), line.direction()) < 0)
        {
            segment.reverse();
        }

        return segment;
    }

    if (intersections.size() > 2) // Handle the non-convex case ...
    {
        using T = Traits::value_type;
        using point_type = Line<Traits>::point_type;
        using vector_type = point_type::vector_type;
        T d_lowest = std::numeric_limits<T>::max();
        T d_highest = std::numeric_limits<T>::lowest();
        point_type point_lowest;
        point_type point_highest;

        auto order = [&](const vector_type& direction, const point_type& position, const point_type& position_reference) -> void
        {
            auto d = dot(direction, (position - position_reference));
            if (d < d_lowest)
            {
                d_lowest = d;
                point_lowest = position;
            }
            else if (d > d_highest)
            {
                d_highest = d;
                point_highest = position;
            }
        };

        for (const auto& ix : intersections)
        {
            if (ix.locus() == Locus::Point)
            {
                order(line.direction(), ix.point(), line.point());
            }
            else if (ix.locus() == Locus::Segment)
            {
                order(line.direction(), ix.segment().point(0), line.point());
                order(line.direction(), ix.segment().point(1), line.point());
            }
        }
        return Segment<Traits>(point_lowest, point_highest);
    }

    assert(intersections.empty());
    return {};
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Ray<Traits>& ray, const Point<Traits>& point)
{
    return intersection(point, ray);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Ray<Traits>& ray, const Line<Traits>& line)
{
    return intersection(line, ray);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Ray<Traits>& ray_a, const Ray<Traits>& ray_b)
{
    auto [s, t] = closest_point_parameters(to_line(ray_a), to_line(ray_b));
//    if (math::float_ge0(s) && math::float_ge0(t) && math::float_eq0((ray_a.point(s) - ray_b.point(t)).norm_squared())) // point_a == point_b ...
    if (math::float_ge0(s) && math::float_ge0(t))
    {
        assert(float_eq(ray_a.point(s), ray_b.point(t)));
        assert(ray_a.contains(ray_b.point(t)));
        assert(ray_b.contains(ray_a.point(s)));

        if (parallel(ray_a, ray_b))
        {
            bool intersects_a_b = ray_a.contains(ray_b.point());
            bool intersects_b_a = ray_b.contains(ray_a.point());

            if (intersects_b_a && intersects_a_b)
            {
                return {Segment<Traits>(ray_a.point(), ray_b.point())};
            }

            return intersects_b_a ? Intersection<Traits>(ray_a) : Intersection<Traits>(ray_b);
        }

        return {ray_a.point(s)};
    }

    return {};
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Ray<Traits>& ray, const Segment<Traits>& segment)
{
    auto [s, t] = closest_point_parameters(ray.endpoint(), ray.direction(), segment.endpoint(0), segment.vector());
    if (math::float_ge0(s) && math::float_clamped01(t) && math::float_eq0((ray.point(s) - segment.point(t)).norm_squared())) // point_a == point_b ...
    {
        assert(ray.contains(segment.point(t)));
        assert(segment.contains(ray.point(s)));

        if (parallel(ray, segment))
        {
            if (segment.contains(ray.endpoint()))
            {
                if (ray.contains(segment.endpoint(0)))
                {
                    return {Segment<Traits>(segment.endpoint(0), ray.endpoint())};
                }

                return {Segment<Traits>(segment.endpoint(1), ray.endpoint())};
            }

            return {segment};
        }

        return {ray.point(s)};
    }

    return {};
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Ray<Traits>& ray, const Plane<Traits>& plane)
{
    if (intersects(ray, plane))
    {
        auto ix = intersection(Line<Traits>(ray.endpoint(), ray.direction()), plane);
        if (ix.locus() == Locus::Line)
        {
            return {ray};
        }

        assert(ix.locus() == Locus::Point);
        return ix;
    }

    return {};
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Ray<Traits>& ray, const Polygon<Traits>& polygon)
{
    if constexpr (Traits::dimension >= 3)
    {
        Intersection<Traits> ix = intersection(ray, polygon.plane());
        if (ix.empty())
        {
            return {};
        }
        else if (ix.locus() == Locus::Point)
        {
            return intersection(ix.point(), polygon());
        }

        assert(ix.locus() == Locus::Ray);
    }

    std::vector<Intersection<Traits>> intersections;
    for (size_t i = 0; i < polygon.edge_count(); ++i)
    {
        auto segment = polygon.edge(i);

        Intersection<Traits> ix = intersection(ray, segment);
        if (ix.locus() == Locus::Segment)
        {
            assert(intersections.empty());
            return ix;
        }

        // Don't consider second segment endpoint so vertex intersections don't get counted twice
        if (ix.locus() == Locus::Point && !ray.contains(segment.endpoint(1)))
        {
            if (float_eq(ix.point(), segment.endpoint(0)))
            {
                ix = segment.endpoint(0); // denoise
            }
            intersections.push_back(ix);
        }
    }

    if (intersections.size() == 1)
    {
        assert(intersections[0].locus() == Locus::Point);
        if (intersections[0].point() == ray.endpoint())
        {
            return intersections[0];
        }

        return {Segment<Traits>(ray.endpoint(), intersections[0].point())};
    }

    if (intersections.size() == 2)
    {
        assert(intersections[0].locus() == Locus::Point);
        assert(intersections[1].locus() == Locus::Point);
        return Segment<Traits>(intersections[0].point(), intersections[1].point());
    }

    assert(intersections.empty());
    return {};
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Segment<Traits>& segment, const Point<Traits>& point)
{
    return intersection(point, segment);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Segment<Traits>& segment, const Line<Traits>& line)
{
    return intersection(line, segment);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Segment<Traits>& segment, const Ray<Traits>& ray)
{
    return intersection(ray, segment);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Segment<Traits>& segment_a, const Segment<Traits>& segment_b)
{
    if (parallel(segment_a, segment_b))
    {
        bool a0b = segment_b.contains(segment_a.endpoint(0));
        bool a1b = segment_b.contains(segment_a.endpoint(1));
        bool ab0 = segment_a.contains(segment_b.endpoint(0));
        bool ab1 = segment_a.contains(segment_b.endpoint(1));

        if (!a0b && !a1b && !ab0 && !ab1)
        {
            return {};
        }

        if (a0b && a1b)
        {
            return {segment_a};
        }

        if (ab0 && ab1)
        {
            return {segment_b};
        }

        Segment<Traits> segment(a0b ? segment_a.endpoint(0) : segment_a.endpoint(1), ab0 ? segment_b.endpoint(0) : segment_b.endpoint(1));
        if (float_eq(segment.endpoint(0), segment.endpoint(1)))
        {
            return {segment.endpoint(0)};
        }

        return {segment};
    }

    auto a0 = segment_a.endpoint(0).rep();
    auto a1 = segment_a.endpoint(1).rep();
    auto b0 = segment_b.endpoint(0).rep();
    auto b1 = segment_b.endpoint(1).rep();
    auto [s, t] = closest_point_parameters(segment_a.endpoint(0), segment_a.vector(), segment_b.endpoint(0), segment_b.vector());
//    auto ps = segment_a.point(s);
//    auto pt = segment_b.point(t);
//    if (math::float_clamped01(s) && math::float_clamped01(t) && math::float_eq(segment_a.point(s), segment_b.point(t))) // point_a == point_b ...
    if (math::float_clamped01(s) && math::float_clamped01(t) && math::float_eq0((segment_a.point(s) - segment_b.point(t)).norm_squared())) // point_a == point_b ...
    {
        assert(segment_a.contains(segment_b.point(t)));
        assert(segment_b.contains(segment_a.point(s)));

        if (t == 0 || t == 1) // Use the exact value
        {
            return {segment_a.point(t)};
        }
        return {segment_a.point(s)};
    }

    return {};
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Segment<Traits>& segment, const Plane<Traits>& plane)
{
    if (!intersects(segment, plane))
    {
        return {};
    }

    Intersection<Traits> ix = intersection(to_line(segment), plane);
    if (ix.locus() == Locus::Line)
    {
        return segment;
    }

    assert(ix.locus() == Locus::Point);

    // Denoise a segment endpoint intersection
    if (float_eq(ix.point(), segment.endpoint(0)))
    {
        return segment.endpoint(0);
    }
    if (float_eq(ix.point(), segment.endpoint(1)))
    {
        return segment.endpoint(1);
    }

    return ix;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Segment<Traits>& segment, const Polygon<Traits>& polygon)
{
    if constexpr (Traits::dimension >= 3)
    {
        Intersection<Traits> ix = intersection(segment, polygon.plane());
        if (ix.empty())
        {
            return {};
        }
        else if (ix.locus() == Locus::Point)
        {
            return intersection(ix.point(), polygon);
        }

        assert(ix.locus() == Locus::Segment);
    }

    std::vector<Intersection<Traits>> intersections;
    for (size_t i = 0; i < polygon.edge_count(); ++i)
    {
        auto edge = polygon.edge(i);

        Intersection<Traits> ix = intersection(segment, edge);
        if (ix.locus() == Locus::Segment)
        {
            assert(intersections.empty());
            return ix;
        }

        // Don't consider second edge endpoint (half-open interval) so vertex intersections don't get counted twice
        if (ix.locus() == Locus::Point && !segment.contains(edge.endpoint(1)))
        {
            if (float_eq(ix.point(), segment.endpoint(0)))
            {
                ix = segment.endpoint(0); // denoise
            }
            intersections.push_back(ix);
        }
    }

    if (intersections.size() == 1)
    {
        assert(intersections[0].locus() == Locus::Point);
        if (float_eq(intersections[0].point(), segment.endpoint(0)))
        {
            if (polygon.contains(segment.endpoint(1)))
            {
                return {segment};
            }
            return segment.endpoint(0);
        }

        if (float_eq(intersections[0].point(), segment.endpoint(1)))
        {
            if (polygon.contains(segment.endpoint(0)))
            {
                return {segment};
            }
            return segment.endpoint(1);
        }

        if (polygon.contains(segment.endpoint(0)))
        {
            return {Segment<Traits>(segment.endpoint(0), intersections[0].point())};
        }
        else
        {
            return {Segment<Traits>(intersections[0].point(), segment.endpoint(1))};
        }
    }
    else if (intersections.size() == 2)
    {
        assert(intersections[0].locus() == Locus::Point);
        assert(intersections[1].locus() == Locus::Point);

        auto s = Segment<Traits>(intersections[0].point(), intersections[1].point());
        if (direction_compare(s.direction(), segment.direction()) < 0)
        {
            s.reverse();
        }

        if (float_eq(s.endpoint(0), segment.endpoint(0)))
        {
            s.endpoint(0) = segment.endpoint(0);
        }
        if (float_eq(s.endpoint(1), segment.endpoint(1)))
        {
            s.endpoint(1) = segment.endpoint(1);
        }

        return s;
    }

    assert(intersections.empty());
    return {};
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Plane<Traits>& plane, const Point<Traits>& point)
{
    return intersection(point, plane);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Plane<Traits>& plane, const Line<Traits>& line)
{
    return intersection(line, plane);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Plane<Traits>& plane, const Ray<Traits>& ray)
{
    return intersection(ray, plane);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Plane<Traits>& plane, const Segment<Traits>& segment)
{
    return intersection(segment, plane);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Plane<Traits>& plane_a, const Plane<Traits>& plane_b)
{
    using T = Traits::value_type;

    if (parallel(plane_a, plane_b))
    {
        if (plane_a.contains(plane_b.point()))
        {
            return {plane_a};
        }

        return {};
    }

    auto direction = normalize(cross(plane_a.normal(), plane_b.normal()));

    // Zero the max coord, and solve for the other two

    auto [a1, b1, c1] = components(plane_a.normal());
    auto [a2, b2, c2] = components(plane_b.normal());

    T d1 = -dot(plane_a.normal(), plane_a.point().vector());
    T d2 = -dot(plane_b.normal(), plane_b.point().vector());

    Point<Traits> point;
    T denom;

    size_t i_max = direction.longest_component();
    switch (i_max)
    {
        case 0: // intersection point with x == 0
            denom = b1 * c2 - b2 * c1;
            point[1] = (d2 * c1 - d1 * c2) / denom;
            point[2] = (d1 * b2 - d2 * b1) / denom;
            break;

        case 1: // intersection point with y == 0
            denom = c1 * a2 - c2 * a1;
            point[2] = (d2 * a1 - d1 * a2) / denom;
            point[0] = (d1 * c2 - d2 * c1) / denom;
            break;

        case 2: // intersection point with z == 0
            denom = a1 * b2 - a2 * b1;
            point[0] = (d2 * b1 - d1 * b2) / denom;
            point[1] = (d1 * a2 - d2 * a1) / denom;
            break;

        default:
            assert(false);
    }

    // point comes from a cross product and a 2x2 linear solve, so its distance from either plane accumulates more rounding
    // error than the default tolerance allows
    constexpr int intersection_tolerance = 4096;
    assert(plane_a.contains(point, intersection_tolerance));
    assert(plane_b.contains(point, intersection_tolerance));

    return {Line<Traits>(point, direction)};
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Plane<Traits>& plane, const Polygon<Traits>& polygon)
{
    if (parallel(plane, polygon))
    {
        if (plane.contains(polygon.vertex(0)))
        {
            return {polygon};
        }
        return {};
    }

    // this might be more robust if the plane was checked against each edge for intersection directly rather than line/segment checks ...
    // line/segment checks are more susceptible to precision issues ...

    Intersection<Traits> ix = intersection(plane, polygon.plane());
    assert(ix.locus() == Locus::Line);

    return intersection(ix.line(), polygon);
/*
    std::vector<Intersection<Traits>> intersections;
    for (size_t i = 0; i < polygon.edge_count(); ++i)
    {
        auto segment = polygon.edge(i);
        Intersection<Traits> ix = intersection(segment, plane);
        if (ix.locus() == Locus::Segment)
        {
            assert(ix.segment() == segment);
            return ix;
        }

        // Don't consider second segment endpoint so vertex intersections don't get counted twice
        if (ix.locus() == Locus::Point && !float_eq(ix.point(), segment.endpoint(1)))
        {
            if (float_eq(ix.point(), segment.endpoint(0)))
            {
                ix = segment.endpoint(0); // denoise
            }
            intersections.push_back(ix);
        }
    }

    if (intersections.size() == 1)
    {
        assert(intersections[0].locus() == Locus::Point);
        return intersections[0];
    }

    if (intersections.size() == 2)
    {
        assert(intersections[0].locus() == Locus::Point);
        assert(intersections[1].locus() == Locus::Point);
        return {Segment<Traits>(intersections[0].point(), intersections[1].point())};
    }

    assert(intersections.empty());
    return {};
*/
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
std::vector<quetzal::geometry::Intersection<Traits>> quetzal::geometry::intersection_non_convex(const Plane<Traits>& plane, const Polygon<Traits>& polygon)
{
    if (parallel(plane, polygon))
    {
        if (plane.contains(polygon.vertex(0)))
        {
            return {{polygon}};
        }
        return {};
    }

    std::vector<Intersection<Traits>> intersections;
    for (size_t i = 0; i < polygon.edge_count(); ++i)
    {
        auto segment = polygon.edge(i);
        Intersection<Traits> ix = intersection(segment, plane);
////std::cout << std::format("**  segment {}, ix {}", segment, ix) << std::endl;
        if (ix.locus() == Locus::Segment)
        {
            assert(ix.segment() == segment);
            intersections.push_back(ix);
////std::cout << std::format("**      adding segment") << std::endl;
        }
        else if (ix.locus() == Locus::Point && !float_eq(ix.point(), segment.endpoint(1))) // Don't consider second segment endpoint so vertex intersections don't get counted twice
        {
            if (float_eq(ix.point(), segment.endpoint(0)))
            {
                ix = segment.endpoint(0); // denoise
            }
            intersections.push_back(ix);
////std::cout << std::format("**      adding point") << std::endl;
        }
    }
//std::cout << std::format("intersection_non_convex np intersections {}", intersections.size()) << std::endl;
////for (const auto ix : intersections) std::cout << std::format("    intersections {}", ix) << std::endl;

    // Sort intersections

    auto ix0 = intersections.front();
    auto position0 = ix0.locus() == Locus::Point ? ix0.point() : ix0.segment().endpoint(0); // not error checked, midpoint? ...
    auto direction = normalize(cross(plane.normal(), polygon.plane().normal()));
    std::map<typename Traits::value_type, Intersection<Traits>> ordered_intersections;
    for (const auto& ix : intersections)
    {
        if (ix.locus() == Locus::Point)
        {
            ordered_intersections[dot(direction, ix.point() - position0)] = ix;
        }
        else if (ix.locus() == Locus::Segment)
        {
            auto segment = ix.segment();
            if (direction_compare(segment.direction(), direction) < 0)
            {
                segment.reverse();
            }
            ordered_intersections[dot(direction, segment.midpoint() - position0)] = segment;
        }
    }
//std::cout << std::format("intersection_non_convex np ordered_intersections {}", ordered_intersections.size()) << std::endl;
//for (const auto i : ordered_intersections) std::cout << std::format("    ordered_intersections {}", i.second) << std::endl;

    // Merge adjacent segment intersections, edge or point-point
    // (exact comparisons since denoise is either done or checked above)

    Intersection<Traits> ix_prev;

    intersections.clear();
    for (const auto& i : ordered_intersections)
    {
        auto ix = i.second;
//std::cout << std::format("    loop {}", i.second) << std::endl;
        if (ix_prev.empty())
        {
            ix_prev = ix;    
            continue;
        }

        if (ix.locus() == Locus::Segment)
        {
            if (ix_prev.locus() == Locus::Segment)
            {
                if (ix.segment().endpoint(0) == ix_prev.segment().endpoint(1))
                {
                    ix_prev = Segment<Traits>(ix_prev.segment().endpoint(0), ix.segment().endpoint(1));
                }
                else if (polygon.contains(Segment<Traits>(ix_prev.segment().endpoint(1), ix.segment().endpoint(0)).midpoint())) // make sure this can handle the degenerate case (same endpoints) ...
                {
                    ix_prev = Segment<Traits>(ix_prev.segment().endpoint(0), ix.segment().endpoint(1));
                }
                else
                {
                    intersections.push_back(ix_prev);
                    ix_prev = ix;
                }
            }
            else
            {
                assert(ix_prev.locus() == Locus::Point);
                if (ix.segment().endpoint(0) == ix_prev.point())
                {
                    ix_prev = ix;
                }
                else if (polygon.contains(Segment<Traits>(ix_prev.point(), ix.segment().endpoint(0)).midpoint())) // remove the above if this can handle that case ...
                {
                    ix_prev = Segment<Traits>(ix_prev.point(), ix.segment().endpoint(1));
                }
                else
                {
                    intersections.push_back(ix_prev);
                    ix_prev = ix;
                }
            }
        }
        else if (ix.locus() == Locus::Point)
        {
            if (ix_prev.locus() == Locus::Segment)
            {
                if (ix.point() == ix_prev.segment().endpoint(1))
                {
                    // Done
                }
                else if (polygon.contains(Segment<Traits>(ix_prev.segment().endpoint(1), ix.point()).midpoint())) // remove the above if this can handle that case ...
                {
                    ix_prev = Segment<Traits>(ix_prev.segment().endpoint(0), ix.point());
                }
                else
                {
                    intersections.push_back(ix_prev);
                    ix_prev = ix;
                }
            }
            else
            {
                assert(ix_prev.locus() == Locus::Point);
                if (ix.point() == ix_prev.point())
                {
                    // Done
                }
                else if (polygon.contains(Segment<Traits>(ix_prev.point(), ix.point()).midpoint()))
                {
                    ix_prev = Segment<Traits>(ix_prev.point(), ix.point());
                }
                else
                {
                    intersections.push_back(ix_prev);
                    ix_prev = ix;
                }
            }
        }
    }

    if (!ix_prev.empty())
    {
        intersections.push_back(ix_prev);
    }
//std::cout << std::format("intersection_non_convex np intersections {}", intersections.size()) << std::endl;
//for (const auto ix : intersections) std::cout << std::format("    intersections {}", ix) << std::endl;

    return intersections;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Polygon<Traits>& polygon, const Point<Traits>& point)
{
    return intersection(point, polygon);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Polygon<Traits>& polygon, const Line<Traits>& line)
{
    return intersection(line, polygon);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Polygon<Traits>& polygon, const Ray<Traits>& ray)
{
    return intersection(ray, polygon);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Polygon<Traits>& polygon, const Segment<Traits>& segment)
{
    return intersection(segment, polygon);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Polygon<Traits>& polygon, const Plane<Traits>& plane)
{
    return intersection(plane, polygon);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Polygon<Traits>& polygon_a, const Polygon<Traits>& polygon_b)
{
//std::cout << std::format("intersection pp") << std::endl;
    auto ix_a = intersection(polygon_b.plane(), polygon_a);
//std::cout << std::format("intersection np returned ix_a {}", ix_a) << std::endl;
    if (ix_a.empty())
    {
        return {};
    }

    auto ix_b = intersection(polygon_a.plane(), polygon_b);
//std::cout << std::format("intersection np returned ix_b {}", ix_b) << std::endl;
    if (ix_b.empty())
    {
        return {};
    }

    return intersection(ix_a, ix_b);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::vector<quetzal::geometry::Intersection<Traits>> quetzal::geometry::intersection_non_convex(const Polygon<Traits>& polygon_a, const Polygon<Traits>& polygon_b)
{
auto direction = normalize(cross(polygon_a.normal(), polygon_b.normal()));
//std::cout << std::format("intersection_non_convex pp direction (+/-) {}", direction) << std::endl;
    auto ixs_a = intersection_non_convex(polygon_b.plane(), polygon_a);
//std::cout << std::format("intersection_non_convex np returned n {}", ixs_a.size()) << std::endl;
//for (const auto ix : ixs_a) std::cout << std::format("    ix_a {}", ix) << std::endl;
    if (ixs_a.empty())
    {
        return {};
    }

    auto ixs_b = intersection_non_convex(polygon_a.plane(), polygon_b);
//std::cout << std::format("intersection_non_convex np returned n {}", ixs_b.size()) << std::endl;
//for (const auto ix : ixs_b) std::cout << std::format("    ix_b {}", ix) << std::endl;
    if (ixs_b.empty())
    {
        return {};
    }

    auto ixs = intersection_colinear<Traits>(ixs_a, ixs_b);
//std::cout << std::format("intersection_colinear returned n {}", ixs.size()) << std::endl;
//for (const auto ix : ixs) std::cout << std::format("    ix {}", ix) << std::endl;
    return ixs;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Plane<Traits>& plane_a, const Plane<Traits>& plane_b, const Plane<Traits>& plane_c)
{
plane_a;
plane_b;
plane_c;
    // none
    // point
    /*
        typename Traits::value_type d_a = plane_a.distance({});
        typename Traits::value_type d_b = plane_b.distance({});
        typename Traits::value_type d_c = plane_c.distance({});
        return (-d_a * cross(plane_b.normal(), plane_c.normal()) - d_b * cross(plane_c.normal(), plane_a.normal()) - d_c * cross(plane_a.normal(), plane_b.normal()))
            / dot(plane_a.normal(), cross(plane_b.normal(), plane_c.normal()));
    */
    // line(s)
    // plane
    assert(false); // ...
    return {}; // ...
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Intersection<Traits>& ix_a, const Intersection<Traits>& ix_b)
{
//std::cout << std::format("intersection ix_a {}, ix_b {}", ix_a, ix_b) << std::endl;
    if (ix_a.locus() == Locus::Point)
    {
        if (ix_b.locus() == Locus::Point)
        {
            return intersection(ix_a.point(), ix_b.point());
        }
        else if (ix_b.locus() == Locus::Segment)
        {
            return intersection(ix_a.point(), ix_b.segment());
        }
        else if (ix_b.locus() == Locus::Polygon)
        {
            return intersection(ix_a.point(), ix_b.polygon());
        }
    }
    else if (ix_a.locus() == Locus::Segment)
    {
        if (ix_b.locus() == Locus::Point)
        {
            return intersection(ix_a.segment(), ix_b.point());
        }
        else if (ix_b.locus() == Locus::Segment)
        {
            return intersection(ix_a.segment(), ix_b.segment());
        }
        else if (ix_b.locus() == Locus::Polygon)
        {
            return intersection(ix_a.segment(), ix_b.polygon());
        }
    }
    else if (ix_a.locus() == Locus::Polygon)
    {
        if (ix_b.locus() == Locus::Point)
        {
            return intersection(ix_a.polygon(), ix_b.point());
        }
        else if (ix_b.locus() == Locus::Segment)
        {
            return intersection(ix_a.polygon(), ix_b.segment());
        }
        else if (ix_b.locus() == Locus::Polygon)
        {
            return intersection(ix_a.polygon(), ix_b.polygon());
        }
    }

    return {};
}

//------------------------------------------------------------------------------
template<typename Traits>
std::vector<quetzal::geometry::Intersection<Traits>> quetzal::geometry::intersection_colinear(std::span<const Intersection<Traits>> ixs_a, std::span<const Intersection<Traits>> ixs_b)
{
    // brute force here, if necessary, order each set of intersections and step through in order ...
//std::cout << std::format("intersection_colinear na {}, nb {}", ixs_a.size(), ixs_b.size()) << std::endl;

    std::vector<Intersection<Traits>> intersections;
    for (const auto& ix_a : ixs_a)
    {
        for (const auto& ix_b : ixs_b)
        {
            auto ix = intersection(ix_a, ix_b);
//std::cout << std::format("intersection returned ix {}", ix) << std::endl;
            if (!ix.empty())
            {
                intersections.push_back(ix);
            }
        }
    }
    return intersections;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::intersects_coplanar(const Point<Traits>& point, const Polygon<Traits>& polygon)
{
/*
    using planar_point_type = geometry::Point<math::CartesianTraits<typename Traits::value_type, 2>>;
    std::vector<planar_point_type> vertices;
    planar_point_type initial_point;

    if constexpr (Traits::dimension == 3)
    {
        geometry::Projection<Traits> projection(polygon.normal());
        for (const auto& vertex : polygon.vertices())
        {
            vertices.emplace_back(projection(vertex));
        }

        initial_point = projection(point);
    }
    else if constexpr (Traits::dimension == 2)
    {
        vertices = polygon.vertices();
        initial_point = point;
    }
    else
    {
        static_assert(false, "Dimension not supported.");
    }
*/
    Ray ray(point, normalize(polygon.vertex(1) - polygon.vertex(0)));

    size_t n = 0;
    for (size_t i = 0; i < polygon.edge_count(); ++i)
    {
        auto segment = polygon.edge(i);
        auto ix = intersection(ray, segment);
        if (ix.locus() == Locus::Segment)
        {
            if (intersects(ray.endpoint(), segment))
            {
                return true;
            }

            ++n;
        }
        else if (ix.locus() == Locus::Point)
        {
            if (float_eq(ix.point(), ray.endpoint())) // for this check exact equality probably ok, only if denoised to ray endpoint ...
            {
//                assert(ix.point() == ray.endpoint());
                return true;
            }

            // Don't consider second segment endpoint so vertex intersections don't get counted twice
            if (!float_eq(ix.point(), segment.endpoint(1), 10000)) // see if this can be reduced ...
            {
                ++n;
            }
        }
        else
        {
            assert(ix.empty());
        }

        // Also handle the edge case where the ray is tangent to the polygon at a vertex ...
        // tangent points should not be counted ...
        // if point0, then don't count if prev and next are both to the right or left of the ray ...
    }

assert(polygon.contains(point) == math::odd(n));
    return math::odd(n);
}

/*
//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits> quetzal::geometry::intersection(const Segment<Traits>& segment, const Polygon<Traits>& polygon)
{
    vector_type normal = normalize(cross(polygon.vertex(1) - polygon.vertex(0), polygon.vertex(2) - polygon.vertex(0))); // polygon.normal()? ...

    auto v0 = segment.endpoint(0) - polygon.vertex(0);
    auto v1 = segment.endpoint(1) - polygon.vertex(0);
    typename Traits::value_type dot0 = dot(v0, normal);
    typename Traits::value_type dot1 = dot(v1, normal);

    // Check for both points on the same side of the polygon, no intersection
    if (math::float_gt0(dot0) && math::float_gt0(dot1) || math::float_lt0(dot0) && math::float_lt0(dot1))
    {
        return {};
    }

    // Segment is in plane of polygon
    if (math::float_eq0(dot0) && math::float_eq0(dot1))
    {
        // count crossings ...
        // if crossings == 0, no intersection ...
        // find crossing points ...
        // if same, then point intersection ...
        // if not convex polygon (crossings > 2), then could be multiple segments ...
        // if 2 and endpoints are interior, then 2 segments ...
        return {segment};
    }

    // Segment crosses plane of polygon. Check if segment/plane intersection point is within polygon
    Intersection<Traits> i = intersection(Line<Traits>(segment.endpoint(0), segment.direction()), Plane<Traits>(polygon.vertex(1), normal));
    assert(i.locus() == Locus::Point);

    if (intersects(i.point(), polygon))
    {
        return i;
    }

    return {};
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Plane<Traits>::line_type quetzal::geometry::Plane<Traits>::intersection(const Plane& plane) const
{
    value_type h0 = distance(Plane<Traits>::point_type());
    value_type h1 = plane.distance(Plane<Traits>::point_type());

    value_type ndotn = dot(m_normal, plane.normal());
    value_type denom = Traits::val(1) - ndotn * ndotn;

    value_type c0 = (h0 - h1 * ndotn) / denom;
    value_type c1 = (h1 - h0 * ndotn) / denom;

    return Line(c0 * m_normal + c1 * plane.normal(), cross(m_normal, plane.m_normal));
}
*/

#endif // QUETZAL_GEOMETRY_INTERSECT_HPP
