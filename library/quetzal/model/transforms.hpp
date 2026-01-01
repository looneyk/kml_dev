#if !defined(QUETZAL_MODEL_TRANSFORMS_HPP)
#define QUETZAL_MODEL_TRANSFORMS_HPP
//------------------------------------------------------------------------------
// model
// transforms.hpp
//------------------------------------------------------------------------------

#include "quetzal/brep/geometry.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include <functional>

namespace quetzal::model
{

    // Works with Mesh and Submesh
    template<typename U>
    void tuskify(U& u, const typename U::traits_type::vector_type& axis, typename U::value_type angle, typename U::value_type z0, typename U::value_type z1);

    // unused ...
    template<typename U>
    void tuskify(U& u, const typename U::traits_type::vector_type& axis, typename U::value_type angle);

    template<typename U>
    void undulate(U& u, typename U::value_type amplitude, typename U::value_type period, typename U::value_type z0, typename U::value_type z1);

    template<typename U>
    void swirl(U& u, typename U::value_type angle, typename U::value_type z0, typename U::value_type z1);

    template<typename U>
    void swirl(U& u, typename U::value_type angle);

    template<typename U>
    void swirler(U& u, typename U::value_type angle_z, typename U::value_type angle_r, typename U::value_type r_max);

} // namespace quetzal::model

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::tuskify(U& u, const typename U::traits_type::vector_type& axis, typename U::value_type angle, typename U::value_type z0, typename U::value_type z1)
{
    auto dz = z1 - z0;

    transform(u,
        [axis, angle, z0, dz](const U::traits_type::point_type& position) -> U::traits_type::point_type
        {
            auto theta = -angle * (U::traits_type::val(0.5) * position.z() - z0) / dz;
            auto matrix = geometry::rotation_axis_unit(axis, theta);
            return position * matrix;
        });

    // defer this so that triangulation can be done first ...
    // with this, normals before and after triangulation should be float_eq ...
    for (auto& surface : u.surfaces())
    {
        calculate_normals(surface); // face and then vertex normals
    }

    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::tuskify(U& u, const typename U::traits_type::vector_type& axis, typename U::value_type angle){
    auto bounds = u.bounds();
    tuskify(u, axis, angle, bounds.lower().z(), bounds.upper().z());
    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::undulate(U& u, typename U::value_type amplitude, typename U::value_type period, typename U::value_type z0, typename U::value_type z1)
{
    period;

    using T = U::value_type;

    auto dz = z1 - z0;
/*
    for (auto& vertex : mesh.vertices())
    {
        size_t nLevel = 2; // needs to be passed in (or as a function of dz) ...

        if (nLevel == 3) amplitude *= T(0.6666667);
        if (nLevel == 2) amplitude *= T(0.3333333);

        T zRelative = (vertex.attributes().position().z() - z0) / dz;
        T rScale = sin(zRelative * amplitude);
        rScale *= T(1) - zRelative;
//        rScale *= sin(zRelative * math::Pi<T>);
        rScale = T(1) - T(0.5) * rScale;
        auto matrix = geometry::scaling(rScale, rScale, T(1));

        vertex.attributes().position() *= matrix;
*/
/* better? ...
        T tz = (vertex.attributes().position().z() - z0) / dz;
        T xyScale = T(1) - T(0.5) * amplitude * cos(tz * period);
        xyScale *= T(1) - tz;
        auto matrixPosition = geometry::scaling(xyScale, xyScale, T(1));
        auto matrixDirection = geometry::direction_matrix(matrixPosition);

        if (math::float_ge(vertex.attributes().position().z(), z0)) // better/smoother condition ...
        {
            vertex.attributes().position() *= matrixPosition;
            vertex.attributes().normal() *= matrixDirection;
        }
    }
*/
    model::transform_positions(u, [&amplitude, z0, dz](const U::traits_type::point_type& position) -> U::traits_type::point_type
    {
        size_t nLevel = 2; // needs to be passed in (or as a function of dz) ...
        if (nLevel == 3) amplitude *= T(0.6666667);
        if (nLevel == 2) amplitude *= T(0.3333333);

        auto zRelative = (position.z() - z0) / dz;
        auto rScale = sin(zRelative * amplitude); // PI? ...
        rScale *= T(1) - zRelative;
        rScale = T(1) - T(0.5) * rScale;
        auto matrix = geometry::scaling(rScale, rScale, T(1));
        return position * matrix;
    });

    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::swirl(U& u, typename U::value_type angle, typename U::value_type z0, typename U::value_type z1)
{
    auto dz = z1 - z0;
    auto axis = U::traits_type::vector_type::z_direction();

    model::transform_positions(u, [axis, angle, z0, dz](const U::traits_type::point_type& position) -> U::traits_type::point_type
    {
        auto phi = angle * (position.z() - z0) / dz;
        auto matrix = geometry::rotation_axis_unit(axis, phi);
        return position * matrix;
    });

    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::swirl(U& u, typename U::value_type angle)
{
    auto bounds = u.bounds();
    swirl(u, angle, bounds.lower().z(), bounds.upper().z());
    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::swirler(U& u, typename U::value_type angle_z, typename U::value_type angle_r, typename U::value_type r_max)
{
    auto bounds = u.bounds();
    auto z0 = bounds.lower().z();
    auto z1 = bounds.upper().z();

    auto dz = z1 - z0;
    auto axis = typename U::vector_type::z_direction();

    model::transform_positions(u, [axis, angle_z, angle_r, z0, dz, r_max](const U::traits_type::point_type& position) -> U::traits_type::point_type
    {
        auto r = sqrt(position.x() * position.x() + position.y() * position.y());
        auto phi = angle_z * (position.z() - z0) / dz + angle_r * r / r_max;
        auto matrix = geometry::rotation_axis_unit(axis, phi);
        return position * matrix;
    });

    return;
}

#endif // QUETZAL_MODEL_TRANSFORMS_HPP
