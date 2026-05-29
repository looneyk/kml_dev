#if !defined(QUETZAL_GEOMETRY_GEOMETRY_UTIL_HPP)
#define QUETZAL_GEOMETRY_GEOMETRY_UTIL_HPP
//------------------------------------------------------------------------------
// geometry
// geometry_util.hpp
//------------------------------------------------------------------------------

#include "HalfPlane.hpp"
#include "Line.hpp"
#include "quetzal/math/Vector.hpp"
#include <cassert>

namespace quetzal::geometry
{

    // Returns true if vectors a and b are in counterclockwise order around normal with respect to ref vector
    // a, b, and ref are vertices of a polygon in a plane with the given normal
    template<typename V> requires (V::dimension == 3)
    bool ccw_order(const V& a, const V& b, const V& ref, const V& normal);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename V> requires (V::dimension == 3)
bool quetzal::geometry::ccw_order(const V& a, const V& b, const V& ref, const V& normal)
{
    HalfPlane halfplane(Line<typename V::traits_type>({}, ref.direction()), normalize(cross(ref, normal)));

    auto contains_a = halfplane.contains(a) ? 0 : 1;
    auto contains_b = halfplane.contains(b) ? 0 : 1;
    if (contains_a != contains_b)
    {
        return contains_a < contains_b;
    }

    return dot(normal, cross(a, b)) > 0;
}

#endif // QUETZAL_GEOMETRY_GEOMETRY_UTIL_HPP
