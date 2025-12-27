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

    // add support for submesh for the rest ...

    template<typename M>
    void undulate(M& mesh, typename M::value_type amplitude, typename M::value_type period, typename M::value_type z0, typename M::value_type z1);

    template<typename M>
    void swirl(M& mesh, typename M::value_type angle, typename M::value_type z0, typename M::value_type z1);

    template<typename M>
    void swirl(M& mesh, typename M::value_type angle);

    template<typename M>
    void swirler(M& mesh, typename M::value_type angle_z, typename M::value_type angle_r, typename M::value_type r_max);

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
template<typename M>
void quetzal::model::undulate(M& mesh, typename M::value_type amplitude, typename M::value_type period, typename M::value_type z0, typename M::value_type z1)
{
    period;

    using T = M::value_type;

    T dz = z1 - z0;

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
*/
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::swirl(M& mesh, typename M::value_type angle, typename M::value_type z0, typename M::value_type z1)
{
    using T = M::value_type;

    T dz = z1 - z0;
    auto axis = M::vector_type::z_direction();

    for (auto& vertex : mesh.vertices())
    {
        T phi = angle * (vertex.attributes().position().z() - z0) / dz;

        vertex.attributes().position() *= geometry::rotation_axis_unit(axis, phi); // Rotate an amount proportional to z position from z0 to z1
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::swirl(M& mesh, typename M::value_type angle)
{
    using T = M::value_type;

    // Find lowest and highest points in mesh
    T z0 = mesh.vertex(0).attributes().position().z();
    T z1 = z0;

    for (const auto& vertex : mesh.vertices())
    {
        const auto& av = vertex.attributes();

        if (av.position().z() < z0)
        {
            z0 = av.position().z();
        }
        else if (av.position().z() > z1)
        {
            z1 = av.position().z();
        }
    }

    swirl(mesh, angle, z0, z1);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::swirler(M& mesh, typename M::value_type angle_z, typename M::value_type angle_r, typename M::value_type r_max)
{
    using T = M::value_type;

    // Find lowest and highest points in mesh
    T z0 = mesh.vertex(0).attributes().position().z();
    T z1 = z0;

    for (const auto& vertex : mesh.vertices())
    {
        const auto& av = vertex.attributes();

        if (av.position().z() < z0)
        {
            z0 = av.position().z();
        }
        else if (av.position().z() > z1)
        {
            z1 = av.position().z();
        }
    }

    T dz = z1 - z0;
    auto axis = typename M::vector_type::z_direction();

    for (auto& vertex : mesh.vertices())
    {
        T r = sqrt(vertex.attributes().position().x() * vertex.attributes().position().x() + vertex.attributes().position().y() * vertex.attributes().position().y());
        T phi = angle_z * (vertex.attributes().position().z() - z0) / dz + angle_r * r / r_max;

        vertex.attributes().position() *= rotation_axis_unit(axis, phi);
    }

    return;
}

#endif // QUETZAL_MODEL_TRANSFORMS_HPP
