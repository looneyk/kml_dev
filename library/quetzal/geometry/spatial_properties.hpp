#if !defined(QUETZAL_GEOMETRY_SPATIAL_PROPERTIES_HPP)
#define QUETZAL_GEOMETRY_SPATIAL_PROPERTIES_HPP
//------------------------------------------------------------------------------
// geometry
// spatial_properties.hpp
//------------------------------------------------------------------------------

#include "AxisAlignedBoundingBox.hpp"
#include "Plane.hpp"
#include "PositionAccessor.hpp"
#include "triangle_util.hpp"
#include <optional>
#include <cmath>

namespace quetzal::geometry
{

    // The point sets here should form a polygon
    // In these functions, points refers to a container of objects, each with an embedded position

    // Undefined if all points are colinear
    template<typename T>
    typename PositionAccessor<T>::vector_type normal(const T& points);

    template<typename T>
    typename PositionAccessor<T>::point_type centroid(const T& points);

    template<typename T>
    typename PositionAccessor<T>::point_type vertex_centroid(const T& points);

    template<typename T>
    AxisAlignedBoundingBox<typename PositionAccessor<T>::point_type::traits_type> bounds(const T& points);

    template<typename T> requires (PositionAccessor<T>::dimension == 2)
    typename PositionAccessor<T>::value_type area(const T& points);

    template<typename T> requires (PositionAccessor<T>::dimension == 3)
    typename PositionAccessor<T>::value_type area(const T& points, const typename PositionAccessor<T>::vector_type& normal);

    template<typename T> requires (PositionAccessor<T>::dimension == 2)
    typename PositionAccessor<T>::value_type signed_area(const T& points);

    template<typename T> requires (PositionAccessor<T>::dimension == 3)
    typename PositionAccessor<T>::value_type signed_area(const T& points, const typename PositionAccessor<T>::vector_type& normal);

    // optional normal in case the normal is already known and does not need to be calculated
    template<typename T>
    bool planar(const T& points, const std::optional<typename PositionAccessor<T>::vector_type>& normal = std::nullopt);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename T>
typename quetzal::geometry::PositionAccessor<T>::vector_type quetzal::geometry::normal(const T& points)
{
    auto n = PositionAccessor<T>::position_count(points);
    assert(n >= 3);

    if (n == 3)
    {
        return triangle_normal(PositionAccessor<T>::position(points, 0), PositionAccessor<T>::position(points, 1), PositionAccessor<T>::position(points, 2));
    }

    typename PositionAccessor<T>::vector_type sum{};
    for (size_t i = 0; i < n; ++i)
    {
        size_t i1 = i == n - 1 ? 0 : i + 1;
        auto p0 = PositionAccessor<T>::position(points, i);
        auto p1 = PositionAccessor<T>::position(points, i1);

        sum.x() += (p0.y() - p1.y()) * (p0.z() + p1.z());
        sum.y() += (p0.z() - p1.z()) * (p0.x() + p1.x());
        sum.z() += (p0.x() - p1.x()) * (p0.y() + p1.y());
    }

// just for trinket_box_cover, fix it there ...
// reconsider this, is there a better way to handle this edge case? ...
if (math::float_eq0(sum.norm_squared()))
{
//assert(false);
    sum *= typename PositionAccessor<T>::value_type(1000);
}

    if (math::float_eq0(sum.norm_squared()))
    {
        return {};
    }

    return normalize(denoise0(sum));
}

//------------------------------------------------------------------------------
template<typename T>
typename quetzal::geometry::PositionAccessor<T>::point_type quetzal::geometry::centroid(const T& points)
{
    auto n = PositionAccessor<T>::position_count(points);
    if (n <= 3)
    {
        return vertex_centroid(points);
    }

    auto reference_normal = geometry::normal(points);

    typename PositionAccessor<T>::value_type areaTotal = 0;
    typename PositionAccessor<T>::vector_type weightedTotal{};
    auto p0 = PositionAccessor<T>::position(points, 0);
    for (size_t i = 1; i < n - 1; ++i)
    {
        auto p1 = PositionAccessor<T>::position(points, i);
        auto p2 = PositionAccessor<T>::position(points, i + 1);
        auto area = triangle_signed_area(p0, p1, p2, reference_normal);
        areaTotal += area;
        weightedTotal += area * triangle_centroid(p0, p1, p2).vector();
    }

    return weightedTotal / areaTotal;
}

//------------------------------------------------------------------------------
template<typename T>
typename quetzal::geometry::PositionAccessor<T>::point_type quetzal::geometry::vertex_centroid(const T& points)
{
    typename PositionAccessor<T>::vector_type total{};
    auto n = PositionAccessor<T>::position_count(points);

    for (size_t i = 0; i < n; ++i)
    {
        total += PositionAccessor<T>::position(points, i).vector();
    }

    return total / typename PositionAccessor<T>::value_type(n);
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::geometry::AxisAlignedBoundingBox<typename quetzal::geometry::PositionAccessor<T>::point_type::traits_type> quetzal::geometry::bounds(const T& points)
{
    quetzal::geometry::AxisAlignedBoundingBox<typename PositionAccessor<T>::point_type::traits_type> bounds;
    auto n = PositionAccessor<T>::position_count(points);

    for (size_t i = 0; i < n; ++i)
    {
        bounds.insert(PositionAccessor<T>::position(points, i));
    }

    return bounds;
}

//------------------------------------------------------------------------------
template<typename T> requires (PositionAccessor<T>::dimension == 2)
typename quetzal::geometry::PositionAccessor<T>::value_type quetzal::geometry::area(const T& points)
{
    return std::abs(signed_area(points));
}

//------------------------------------------------------------------------------
template<typename T> requires (PositionAccessor<T>::dimension == 3)
typename quetzal::geometry::PositionAccessor<T>::value_type quetzal::geometry::area(const T& points, const typename PositionAccessor<T>::vector_type& normal)
{
    return std::abs(signed_area(points, normal));
}

//------------------------------------------------------------------------------
template<typename T> requires (PositionAccessor<T>::dimension == 2)
typename quetzal::geometry::PositionAccessor<T>::value_type quetzal::geometry::signed_area(const T& points)
{
    size_t n = PositionAccessor<T>::position_count(points);
    assert(n >= 3);
    if (n == 3)
    {
        return triangle_signed_area(PositionAccessor<T>::position(points, 0), PositionAccessor<T>::position(points, 1), PositionAccessor<T>::position(points, 2));
    }

    typename PositionAccessor<T>::value_type area = 0;
    for (size_t i = 2; i < n; ++i)
    {
        area += triangle_signed_area(PositionAccessor<T>::position(points, 0), PositionAccessor<T>::position(points, i - 1), PositionAccessor<T>::position(points, i));
    }

    return area;
}

//------------------------------------------------------------------------------
template<typename T> requires (PositionAccessor<T>::dimension == 3)
typename quetzal::geometry::PositionAccessor<T>::value_type quetzal::geometry::signed_area(const T& points, const typename PositionAccessor<T>::vector_type& normal)
{
    size_t n = PositionAccessor<T>::position_count(points);
    assert(n >= 3);
    if (n == 3)
    {
        return triangle_signed_area(PositionAccessor<T>::position(points, 0), PositionAccessor<T>::position(points, 1), PositionAccessor<T>::position(points, 2), normal);
    }

    typename PositionAccessor<T>::value_type area = 0;
    for (size_t i = 2; i < n; ++i)
    {
        area += triangle_signed_area(PositionAccessor<T>::position(points, 0), PositionAccessor<T>::position(points, i - 1), PositionAccessor<T>::position(points, i), normal);
    }

    return area;
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::geometry::planar(const T& points, const std::optional<typename PositionAccessor<T>::vector_type>& normal)
{
    if constexpr (PositionAccessor<T>::dimension >= 3)
    {
        size_t n = PositionAccessor<T>::position_count(points);
        if (n <= 3)
        {
            return true;
        }

        Plane<typename PositionAccessor<T>::point_type::traits_type> plane(PositionAccessor<T>::position(points, 0), normal.value_or(geometry::normal(points)));

        for (size_t i = 0; i < n; ++i)
        {
            const auto& point = PositionAccessor<T>::position(points, i);
            if (!plane.contains(point))
            {
                return false;
            }
        }
    }

    return true;
}

#endif // QUETZAL_GEOMETRY_SPATIAL_PROPERTIES_HPP
