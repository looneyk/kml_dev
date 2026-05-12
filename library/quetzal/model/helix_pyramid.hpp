#if !defined(QUETZAL_MODEL_HELIX_PYRAMID_HPP)
#define QUETZAL_MODEL_HELIX_PYRAMID_HPP
//------------------------------------------------------------------------------
// model
// helix_pyramid.hpp
//------------------------------------------------------------------------------

#include "primitives_util.hpp"
#include "quetzal/math/math_util.hpp"
#include <string>
#include <cassert>

namespace quetzal::model
{

    // nAzimuth is number of sides
    // Surfaces: per side "body<i>", "end0"
    template<typename M>
    id_type create_helix_pyramid(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> rBase, data_value_type<M> dz, data_value_type<M> revs);

    // polygon is the base of the pyramid
    // Surfaces: per side "body<i>", "end0"
    template<typename M>
    id_type create_helix_pyramid(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, size_t nz, data_value_type<M> dz, data_value_type<M> revs);

    //--------------------------------------------------------------------------
    template<typename Traits>
    bool body_surface(const brep::Surface<Traits>& surface)
    {
        return surface.name().starts_with("body");
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    void triangulate_body_surfaces(brep::Submesh<Traits>& submesh)
    {
        for (auto& surface : submesh.surfaces())
        {
            if (!body_surface(surface))
            {
                continue;
            }

            triangulate_central_vertex_positions(surface);
            calculate_surface_vertex_normals(surface);
        }

        return;
    }

} // namespace quetzal::model

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_helix_pyramid(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, data_value_type<M> r0, data_value_type<M> dz, data_value_type<M> revs)
{
    using T = M::value_type;

    CylinderSpecification<T> spec(nAzimuth, nz, r0, T(0), T(0), dz, T(0), revs, {{"azimuth_smooth", "false"}});

    assert(nAzimuth > 2);
    assert(nz > 0);
    assert(r0 > T(0));

    T t = T(1) / T(nz);

    T r1 = spec.axis_radius_interval.lerp(t);

    T z0 = spec.axis_z_interval.lower();
    T z1 = spec.axis_z_interval.lerp(t);

    T revs0 = spec.axis_azimuth_offset_interval.lower();
    T revs1 = spec.axis_azimuth_offset_interval.lerp(t);

    math::Interval azimuth(T(1));
    math::Interval azimuth0 = shift(azimuth, revs0);
    math::Interval azimuth1 = shift(azimuth, revs1);

    auto ts0 = texture_span<M>(0, nz, false, true);
    auto ts1 = texture_span<M>(1, nz, false, true);

    auto face_count = spec.azimuth.edge_count;

    id_type halfedge_id0 = mesh.halfedge_store_count();

    id_type submesh_id = mesh.create_submesh(name);
    id_type surface_id0 = create_section_surfaces(mesh, submesh_id, face_count, [&spec](size_t i) { return surface_name(spec.shell_name, i); });

    id_type j0_orig = mesh.halfedge_store_count();

    if (nz == 1)
    {
        create_apex_cusp_sections(mesh, nAzimuth, r0, z0, z1, azimuth0, ts0, surface_id0);
    }
    else
    {
        create_band_sections(mesh, nAzimuth, r0, r1, z0, z1, azimuth0, azimuth1, ts0, ts1, surface_id0);

        for (size_t i = 2; i < nz; ++i)
        {
            t = T(i) / T(nz);
            r1 = spec.axis_radius_interval.lerp(t);
            z1 = spec.axis_z_interval.lerp(t);
            ts1 = texture_span<M>(i, nz, false, true);
            revs1 = spec.axis_azimuth_offset_interval.lerp(t);
            azimuth1 = shift(azimuth, revs1);

            connect_band_sections(mesh, nAzimuth, r1, z1, azimuth1, ts1, surface_id0);
        }

        j0_orig = mesh.halfedge_store_count();

        connect_apex_cusp_sections(mesh, nAzimuth, dz, azimuth1, surface_id0);
    }

    calculate_apex_cusp_surface_vertex_normals_sequential(mesh, j0_orig + 2 * face_count, face_count);
    seal_cylinder(mesh, submesh_id, spec, halfedge_id0, false, true);

//    triangulate_body_surfaces(mesh.submesh(submesh_id));
    return submesh_id;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_helix_pyramid(M& mesh, const std::string& name, const geometry::Polygon<typename M::vector_traits>& polygon, size_t nz, data_value_type<M> dz, data_value_type<M> revs)
{
    using T = M::value_type;

    CylinderSpecification<T> spec(polygon.edge_count(), nz, T(0), T(0), T(0), dz, T(0), revs, {{"azimuth_smooth", "false"}});

    auto face_count = spec.azimuth.edge_count;
    assert(face_count > 2);
    assert(nz > 0);

    T t = T(1) / T(nz);

    T z0 = spec.axis_z_interval.lower();
    T z1 = spec.axis_z_interval.lerp(t);

    T revs0 = spec.axis_azimuth_offset_interval.lower();
    T revs1 = spec.axis_azimuth_offset_interval.lerp(t);

    T scale0 = T(1);
    T scale1 = T(1) - t;

    auto ts0 = texture_span<M>(0, nz, false, true);
    auto ts1 = texture_span<M>(1, nz, false, true);

    bool closed = true;

    id_type halfedge_id0 = mesh.halfedge_store_count();

    id_type submesh_id = mesh.create_submesh(name);
    id_type surface_id0 = create_section_surfaces(mesh, submesh_id, face_count, [&spec](size_t i) { return surface_name(spec.shell_name, i); });

    auto avss0 = vertices_attributes_edges<M>(polygon, z0, scale0, revs0, ts0);

    id_type j0_orig = mesh.halfedge_store_count();

    if (nz == 1)
    {
        auto avss1 = cusp_vertices_attributes_edges<M>(polygon, z1, ts1);
        create_apex_cusp(mesh, avss0, avss1, surface_id0, closed);
    }
    else
    {
        auto avss1 = vertices_attributes_edges<M>(polygon, z1, scale1, revs1, ts1);
        create_band(mesh, avss0, avss1, surface_id0, closed);

        for (size_t i = 2; i < nz; ++i)
        {
            t = T(i) / T(nz);
            z1 = spec.axis_z_interval.lerp(t);
            revs1 = spec.axis_azimuth_offset_interval.lerp(t);
            scale1 = T(1) - t;
            ts1 = texture_span<M>(i, nz, false, true);

            avss1 = vertices_attributes_edges<M>(polygon, z1, scale1, revs1, ts1);
            connect_band(mesh, avss1, surface_id0, closed);
        }

        j0_orig = mesh.halfedge_store_count();

        ts1 = texture_span<M>(nz, nz, false, true);
        avss1 = cusp_vertices_attributes_edges<M>(polygon, dz, ts1);
        connect_apex_cusp(mesh, avss1, surface_id0, closed);
    }

    calculate_apex_cusp_surface_vertex_normals_sequential(mesh, j0_orig + 2 * face_count, face_count);
    seal_cylinder(mesh, submesh_id, spec, halfedge_id0, false, true);

//    triangulate_body_surfaces(mesh.submesh(submesh_id));
    return submesh_id;
}

#endif // QUETZAL_MODEL_HELIX_PYRAMID_HPP
