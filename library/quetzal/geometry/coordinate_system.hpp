#if !defined(QUETZAL_GEOMETRY_COORDINATE_SYSTEM_HPP)
#define QUETZAL_GEOMETRY_COORDINATE_SYSTEM_HPP
//------------------------------------------------------------------------------
// geometry
// coordinate_system.hpp
//
// Cartesian (x, y, z)
// Cylindrical (r, phi, z), phi azimuth
// Spherical (r, phi, theta), phi azimuth, theta elevation
// Angles in radians
//------------------------------------------------------------------------------

#include "quetzal/math/math_util.hpp"
#include <cmath>

namespace quetzal::geometry
{

    template<typename P> requires (P::dimension == 3)
    P cartesian_to_cylindrical(const P& p);

    template<typename P> requires (P::dimension == 3)
    P cartesian_to_spherical(const P& p);

    template<typename P> requires (P::dimension == 3)
    P cylindrical_to_cartesian(const P& p);

    template<typename P> requires (P::dimension == 3)
    P cylindrical_to_spherical(const P& p);

    template<typename P> requires (P::dimension == 3)
    P spherical_to_cartesian(const P& p);

    template<typename P> requires (P::dimension == 3)
    P spherical_to_cylindrical(const P& p);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename P> requires (P::dimension == 3)
P quetzal::geometry::cartesian_to_cylindrical(const P& p)
{
    return {azimuth_radius(p), azimuth_angle(p), p.z()};
}

//------------------------------------------------------------------------------
template<typename P> requires (P::dimension == 3)
P quetzal::geometry::cartesian_to_spherical(const P& p)
{
    return {p.norm(), azimuth_angle(p), elevation_angle(p)};
}

//------------------------------------------------------------------------------
template<typename P> requires (P::dimension == 3)
P quetzal::geometry::cylindrical_to_cartesian(const P& p)
{
    return {p[0] * cos(p[1]), p[0] * sin(p[1]), p[2]};
}

//------------------------------------------------------------------------------
template<typename P> requires (P::dimension == 3)
P quetzal::geometry::cylindrical_to_spherical(const P& p)
{
    return {sqrt(p[0] * p[0] + p[2] * p[2]), p[1], atan2(p[2], p[0])};
}

//------------------------------------------------------------------------------
template<typename P> requires (P::dimension == 3)
P quetzal::geometry::spherical_to_cartesian(const P& p)
{
    return {p[0] * cos(p[2]) * cos(p[1]), p[0] * cos(p[2]) * sin(p[1]), p[0] * sin(p[2])};
}

//------------------------------------------------------------------------------
template<typename P> requires (P::dimension == 3)
P quetzal::geometry::spherical_to_cylindrical(const P& p)
{
    return {p[0] * cos(p[2]), p[1], p[0] * sin(p[2])};
}

#endif // QUETZAL_GEOMETRY_COORDINATE_SYSTEM_HPP
