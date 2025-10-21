#if !defined(QUETZAL_GEOMETRY_TRIANGLE_UTIL_HPP)
#define QUETZAL_GEOMETRY_TRIANGLE_UTIL_HPP
//------------------------------------------------------------------------------
// geometry
// triangle_util.hpp
//------------------------------------------------------------------------------

#include "Plane.hpp"
#include "Projection.hpp"
#include "Segment.hpp"
#include <tuple>
#include <cmath>

namespace quetzal::geometry
{

    // Returns -1, 0, 1: interior, boundary, exterior
    // Exterior is defined by the direction of the normal as defined by the winding order
    // This is more accurate than contains for points on the polygon edges
    template<typename P>
    int triangle_compare(const P& a, const P& b, const P& c, const P& point);

    // Point is in the region defined by the triangle
    // For points on triangle edges, containment is not defined
    template<typename P>
    bool triangle_contains(const P& a, const P& b, const P& c, const P& point);

    template<typename P> requires (P::dimension == 2)
    bool triangle_ccw(const P& a, const P& b, const P& c);

    template<typename P> requires (P::dimension == 3)
    bool triangle_ccw(const P& a, const P& b, const P& c, const typename P::vector_type& normal);

    template<typename P>
    bool triangle_degenerate(const P& a, const P& b, const P& c);

    template<typename P>
    P triangle_centroid(const P& a, const P& b, const P& c);

    template<typename P>
    std::tuple<P, typename P::value_type> triangle_incenter(const P& p0, const P& p1, const P& p2);

    template<typename P> requires (P::dimension == 2)
    typename P::value_type triangle_area(const P& a, const P& b, const P& c);

    template<typename P> requires (P::dimension == 3)
    typename P::value_type triangle_area(const P& a, const P& b, const P& c);

    // Based on implied winding around positive z-axis
    template<typename P> requires (P::dimension == 2)
    typename P::value_type triangle_signed_area(const P& a, const P& b, const P& c);

    template<typename P> requires (P::dimension == 3)
    typename P::value_type triangle_signed_area(const P& a, const P& b, const P& c, const typename P::vector_type& normal);

    // Points should be in CCW order
    template<typename P> requires (P::dimension == 3)
    typename P::vector_type triangle_normal(const P& a, const P& b, const P& c);

namespace internal
{

    // Check if point is in region defined by triangle
    template<typename P> requires (P::dimension == 2)
    bool triangle_region_contains(const P& a, const P& b, const P& c, const P& point);

} // namespace internal

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename P>
int quetzal::geometry::triangle_compare(const P& a, const P& b, const P& c, const P& point)
{
    using segment_type = Segment<typename P::traits_type>;
    if (segment_type(a, b).contains(point) || segment_type(b, c).contains(point) || segment_type(c, a).contains(point))
    {
        return 0;
    }

    if (triangle_degenerate(a, b, c))
    {
        return 1;
    }

    if constexpr (P::dimension == 2)
    {
        return internal::triangle_region_contains(a, b, c, point) ? -1 : 1;
    }
    else if constexpr (P::dimension == 3)
    {
//        auto normal = triangle_normal(a, b, c);
        auto normal = normalize(cross(b - a, c - a));

        Plane<typename P::traits_type> plane(a, normal);
        if (!plane.contains(point))
        {
            return 1;
        }

        Projection<typename P::traits_type> projection(normal);
        return internal::triangle_region_contains(projection(a), projection(b), projection(c), projection(point)) ? -1 : 1;
    }
}

//------------------------------------------------------------------------------
template<typename P>
bool quetzal::geometry::triangle_contains(const P& a, const P& b, const P& c, const P& point)
{
    return triangle_compare(a, b, c, point) <= 0;
}

//------------------------------------------------------------------------------
template<typename P> requires (P::dimension == 2)
bool quetzal::geometry::triangle_ccw(const P& a, const P& b, const P& c)
{
    return dot(c - a, perp(b - a)) >= 0;
}

//------------------------------------------------------------------------------
template<typename P> requires (P::dimension == 3)
bool quetzal::geometry::triangle_ccw(const P& a, const P& b, const P& c, const typename P::vector_type& normal)
{
    return dot(c - a, cross(normal, b - a)) >= 0;
}

//------------------------------------------------------------------------------
template<typename P>
bool quetzal::geometry::triangle_degenerate(const P& a, const P& b, const P& c)
{
    return colinear(a, b, c);
}

//------------------------------------------------------------------------------
template<typename P>
P quetzal::geometry::triangle_centroid(const P& a, const P& b, const P& c)
{
    return (a.vector() + b.vector() + c.vector()) / P::value_type(3);
}

//------------------------------------------------------------------------------
template<typename P>
std::tuple<P, typename P::value_type> quetzal::geometry::triangle_incenter(const P& p0, const P& p1, const P& p2)
{
    auto a = (p2 - p1).norm();
    auto b = (p0 - p2).norm();
    auto c = (p1 - p0).norm();

    auto perimeter = (a + b + c);
    auto s = perimeter / typename P::value_type(2);

    auto v0 = p0.vector();
    auto v1 = p1.vector();
    auto v2 = p2.vector();

    auto incenter = (a * v0 + b * v1 + c * v2) / perimeter;
    auto r = sqrt((s - a) * (s - b) * (s - c) / s);
//    return {to_point(incenter), r};
    return {incenter, r};
}

//------------------------------------------------------------------------------
template<typename P> requires (P::dimension == 2)
typename P::value_type quetzal::geometry::triangle_area(const P& a, const P& b, const P& c)
{
    return std::abs(triangle_signed_area(a, b, c));
}

//------------------------------------------------------------------------------
template<typename P> requires (P::dimension == 3)
typename P::value_type quetzal::geometry::triangle_area(const P& a, const P& b, const P& c)
{
    return cross(b - a, c - a).norm() * typename P::value_type(0.5);
}

//------------------------------------------------------------------------------
template<typename P> requires (P::dimension == 2)
typename P::value_type quetzal::geometry::triangle_signed_area(const P& a, const P& b, const P& c)
{
    return (a.x() * (b.y() - c.y()) + b.x() * (c.y() - a.y()) + c.x() * (a.y() - b.y())) * typename P::value_type(0.5);
}

//------------------------------------------------------------------------------
template<typename P> requires (P::dimension == 3)
typename P::value_type quetzal::geometry::triangle_signed_area(const P& a, const P& b, const P& c, const typename P::vector_type& normal)
{
    return dot(cross(b - a, c - a), normal) * typename P::value_type(0.5);
}

//------------------------------------------------------------------------------
template<typename P> requires (P::dimension == 3)
typename P::vector_type quetzal::geometry::triangle_normal(const P& a, const P& b, const P& c)
{
//    return normalize(denoise0(cross(b - a, c - a)));
    return normalize(cross(b - a, c - a));
}

//------------------------------------------------------------------------------
// Area signs implementation
template<typename P> requires (P::dimension == 2)
bool quetzal::geometry::internal::triangle_region_contains(const P& a, const P& b, const P& c, const P& point)
{
    using T = P::value_type;

    T signa = dot(perp(b - a), point - a);
    T signb = dot(perp(c - b), point - b);
    T signc = dot(perp(a - c), point - c);

    return (signa > T(0) && signb > T(0) && signc > T(0)) || (signa < T(0) && signb < T(0) && signc < T(0));
}

/*
//------------------------------------------------------------------------------
// Barycentric implementation
template<typename P> requires (P::dimension == 2)
bool quetzal::geometry::internal::triangle_region_contains(const P& a, const P& b, const P& c, const P& point)
{
    using T = P::value_type;

    auto v0 = a;
    auto v1 = b - a;
    auto v2 = c - a;

    T denomFactor = T(1) / (v1.x() * v2.y() - v1.y() * v2.x());
    T u = ((point.x() - v0.x()) * v2.y() - (point.y() - v0.y()) * v2.x()) * denomFactor;
    T v = ((point.y() - v0.y()) * v1.x() - (point.x() - v0.x()) * v1.y()) * denomFactor;

    return u > 0 && v > 0 && u + v < 1;
}
*/

#endif // QUETZAL_GEOMETRY_TRIANGLE_UTIL_HPP
