#if !defined(QUETZAL_MODEL_PRIMITIVES_UTIL_HPP)
#define QUETZAL_MODEL_PRIMITIVES_UTIL_HPP
//------------------------------------------------------------------------------
// model
// primitives_util.hpp
//------------------------------------------------------------------------------

#include "mesh_attributes.hpp"
#include "primitives_specification.hpp"
#include "vertices_attributes.hpp"
#include "quetzal/brep/geometry.hpp"
#include "quetzal/brep/texcoord.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/PositionAccessor.hpp"
#include "quetzal/geometry/spatial_properties.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/Interval.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/math/math_util.hpp"
#include <algorithm>
#include <array>
#include <functional>
#include <optional>
#include <ranges>
#include <tuple>

#include <iomanip>

namespace quetzal::model
{

    // The following create/connect functions can be used to construct cylinders, cones, spheres, and contoured cylinders by succesive layering
    // All angles and intervals that represent angles are measured in turns, 0..1 representing one turn (circle) and/or full range
    // Cylindrical coordinates (r, phi, z) with azimuth interval 0..1 -> phi 0..2Pi
    // Spherical coordinates (r, phi, theta) with azimuth interval 0..1 -> phi 0..2Pi; elevation interval 0..1 -> theta -Pi/2..Pi/2
    // A surface_id parameter references a single surface for the whole band
    // A surface_id0 parameter references a set of separate surface for each section. surface_id0 is the first of a numerically contiguous set of surface ids

    // Single surface
    template<typename M>
    void create_apex_cusp(M& mesh, size_t nAzimuth, data_value_type<M> r0, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts0, id_type surface_id, std::optional<vertex_normal_function_type<M>> vertex_normal = std::nullopt);

    template<typename M>
    void create_apex_cusp(M& mesh, size_t nAzimuth, data_value_type<M> r0, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts0, id_type surface_id, vertex_normal_function_type<M> vertex_normal0, vertex_normal_function_type<M> vertex_normal1);

    // Separate surfaces for each azimuth edge
    template<typename M>
    void create_apex_cusp_sections(M& mesh, size_t nAzimuth, data_value_type<M> r0, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth0, texture_span_type<M> ts0, id_type surface_id0);

    template<typename M>
    void create_apex_cusp(M& mesh, size_t nAzimuth, std::function<typename M::point_type(data_value_type<M>)> fr0, data_value_type<M> z1, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts0, id_type surface_id, std::optional<vertex_normal_function_type<M>> vertex_normal1 = std::nullopt);

    // Single surface
    template<typename M>
    void create_apex_cusp(M& mesh, const vertices_attributes_type<M>& avs0, const vertices_attributes_type<M>& avs1, id_type surface_id, bool closed, bool calculate_normals = true);

    // Separate surfaces for each section
    template<typename M>
    void create_apex_cusp(M& mesh, const vertices_attributes_sections_type<M>& avss0, const vertices_attributes_sections_type<M>& avss1, id_type surface_id0, bool closed);

    template<typename M>
    void create_base_cusp(M& mesh, size_t nAzimuth, data_value_type<M> r1, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts1, id_type surface_id, std::optional<vertex_normal_function_type<M>> vertex_normal = std::nullopt);

    // Single surface
    // would making the second optional make sense, then the first could be used for both levels? ...
    template<typename M>
    void create_base_cusp(M& mesh, size_t nAzimuth, data_value_type<M> r1, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts1, id_type surface_id, vertex_normal_function_type<M> vertex_normal0, vertex_normal_function_type<M> vertex_normal1);

    // Separate surfaces for each azimuth edge
    template<typename M>
    void create_base_cusp_sections(M& mesh, size_t nAzimuth, data_value_type<M> r1, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts1, id_type surface_id0);

    // Single surface
    template<typename M>
    void create_base_cusp(M& mesh, size_t nAzimuth, data_value_type<M> z0, std::function<typename M::point_type(data_value_type<M>)> f1, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts1, id_type surface_id, std::optional<vertex_normal_function_type<M>> vertex_normal0 = std::nullopt);

    // Single surface
    template<typename M>
    void create_base_cusp(M& mesh, const vertices_attributes_type<M>& avs0, const vertices_attributes_type<M>& avs1, id_type surface_id, bool closed, bool calculate_normals = true);

    // Separate surfaces for each section
    template<typename M>
    void create_base_cusp(M& mesh, const vertices_attributes_sections_type<M>& avss0, const vertices_attributes_sections_type<M>& avss1, id_type surface_id0, bool closed);
    
    // Single surface
    // Create an open cylindrical section with nAzimuth edges
    template<typename M>
    void create_band(M& mesh, size_t nAzimuth, data_value_type<M> r0, data_value_type<M> r1, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth0, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts0, texture_span_type<M> ts1, id_type surface_id, std::optional<vertex_normal_function_type<M>> vertex_normal = std::nullopt);

    template<typename M>
    void create_band(M& mesh, size_t nAzimuth, data_value_type<M> r0, data_value_type<M> r1, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth0, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts0, texture_span_type<M> ts1, id_type surface_id, vertex_normal_function_type<M> vertex_normal0, vertex_normal_function_type<M> vertex_normal1);

    // Separate surfaces for each azimuth edge
    // Create an open cylindrical section with nAzimuth edges
    template<typename M>
    void create_band_sections(M& mesh, size_t nAzimuth, data_value_type<M> r0, data_value_type<M> r1, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth0, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts0, texture_span_type<M> ts1, id_type surface_id0);

    // Single surface
    // Create an open cylindrical section with nAzimuth edges
    template<typename M>
    void create_band(M& mesh, size_t nAzimuth, std::function<typename M::point_type(data_value_type<M>)> f0, std::function<typename M::point_type(data_value_type<M>)> f1, azimuth_interval_type<M> azimuth0, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts0, texture_span_type<M> ts1, id_type surface_id);

    // Single surface
    template<typename M>
    void create_band(M& mesh, const vertices_attributes_type<M>& avs0, const vertices_attributes_type<M>& avs1, id_type surface_id, bool closed, bool calculate_normals = true);

    // A surface for each polygon edge
    // surface_id0 must be the id of the first surface in a contiguous set of surfaces
    template<typename M>
    void create_band_edges(M& mesh, const vertices_attributes_type<M>& avs0, const vertices_attributes_type<M>& avs1, id_type surface_id0, bool closed, bool calculate_normals = true);

    // Separate surfaces for each section
    // Create an open cylindrical section between two vertex value sets
    // surface_id0 must be the start of a contiguous set of surfaces, one for each set of vertex attributes
    template<typename M>
    void create_band(M& mesh, const vertices_attributes_sections_type<M>& avss0, const vertices_attributes_sections_type<M>& avss1, id_type surface_id0, bool closed);

    // Create an open antiprism section between two vertex attribute sets
    // Starts with a face with the horizontal edge at the bottom (triangle pointing up)
    template<typename M>
    void create_antiband(M& mesh, const vertices_attributes_type<M>& avs0, const vertices_attributes_type<M>& avs1, id_type surface_id);

    // Create an open antiprism section between two vertex attribute sets
    // Starts with a face with the horizontal edge at the top (triangle pointing down)
    template<typename M>
    void create_antiband_inverted(M& mesh, const vertices_attributes_type<M>& avs0, const vertices_attributes_type<M>& avs1, id_type surface_id);

    // Create an open antiprism section between two vertex attribute sets
    template<typename M>
    void create_antiband_faces(M& mesh, const vertices_attributes_type<M>& avs0, const vertices_attributes_type<M>& avs1, id_type submesh_id);

    // Append a cone with base connected to previous layer in mesh with apex at z1 and with nAzimuth edges
    // Single surface
    template<typename M>
    void connect_apex_cusp(M& mesh, size_t nAzimuth, data_value_type<M> z1, azimuth_interval_type<M> azimuth1, id_type surface_id, std::optional<vertex_normal_function_type<M>> vertex_normal = std::nullopt);

    // Append a cone with base connected to previous layer in mesh with apex at z1 and with nAzimuth edges
    // Separate surfaces for each azimuth edge
    template<typename M>
    void connect_apex_cusp_sections(M& mesh, size_t nAzimuth, data_value_type<M> z1, azimuth_interval_type<M> azimuth1, id_type surface_id0);

    // Single surface
    template<typename M>
    void connect_apex_cusp(M& mesh, const vertices_attributes_type<M>& avs1, id_type surface_id, bool closed, bool calculate_normals = true);

    // Append a cone with base connected to previous layer
    // Separate surfaces for each section
    template<typename M>
    void connect_apex_cusp(M& mesh, const vertices_attributes_sections_type<M>& avss1, id_type surface_id0, bool closed);

    // Single surface
    template<typename M>
    void connect_band(M& mesh, size_t nAzimuth, data_value_type<M> r1, data_value_type<M> z1, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts1, id_type surface_id, std::optional<vertex_normal_function_type<M>> vertex_normal = std::nullopt);

    // Separate surfaces for each azimuth edge
    template<typename M>
    void connect_band_sections(M& mesh, size_t nAzimuth, data_value_type<M> r1, data_value_type<M> z1, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts1, id_type surface_id0);

    // Single surface
    template<typename M>
    void connect_band(M& mesh, size_t nAzimuth, std::function<typename M::point_type(data_value_type<M>)> f1, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts1, id_type surface_id);

    // Create an open cylindrical section connected to the previous section
    // Single surface
    template<typename M>
    void connect_band(M& mesh, const vertices_attributes_type<M>& avs1, id_type surface_id, bool closed, bool calculate_normals = true);

    // Create an open cylindrical section connected to the previous section
    // Separate surfaces for each section
    template<typename M>
    void connect_band(M& mesh, const vertices_attributes_sections_type<M>& avss1, id_type surface_id0, bool closed);

    // Create an open cylindrical section connected to an arbitrary open border section
    // better way to do this? ...
    template<typename M>
    void connect_band(M& mesh, const vertices_attributes_type<M>& avs1, const std::vector<id_type>& idPartners, id_type idSurface0, bool bClosed);

    template<typename M>
    void connect_antiband(M& mesh, const vertices_attributes_type<M>& avs1, id_type surface_id);

    template<typename M>
    void connect_antiband_inverted(M& mesh, const vertices_attributes_type<M>& avs1, id_type surface_id);

    template<typename M>
    void connect_antiband_faces(M& mesh, const vertices_attributes_type<M>& avs1, id_type submesh_id);

    // Connect the borders of an open side with a flat surface divided into nb faces
    template<typename M>
    void connect_side_flat(M& mesh, size_t na, size_t nb, bool cusped_lower, bool cusped_upper, id_type surface_id, bool bPlanar);

    // Connect the borders of an open side with two surfaces that extend to the vertical center, each divided into nb faces
    // This assumes vertical orientation and flat ends ...
    // need positions for torus ...
    // need triangular, centered faces for sphere with ends center ...
    template<typename M>
    void connect_side_center(M& mesh, size_t na, size_t nb, bool cusped_lower, bool cusped_upper, id_type surface_id0, id_type surface_id1, std::function<typename M::point_type(const typename M::point_type&)> center_position, bool bPlanar);

    // Connect the borders of an open side with a flat surface consisting of a single face
    // Normals are undefined if side face is not planar
    template<typename M>
    void close_side_flat(M& mesh, size_t na, size_t nb, bool cusped_lower, bool cusped_upper, id_type surface_id, bool bPlanar);

    // Connect the borders of an open side with two surfaces that extend to the vertical center, each consisting of a single face
    // Normals are undefined if side faces are not planar
    template<typename M>
    void close_side_center(M& mesh, size_t na, size_t nb, bool cusped_lower, bool cusped_upper, id_type surface_id0, id_type surface_id1, std::function<typename M::point_type(const typename M::point_type&)> center_position, bool bPlanar);

    // Close off all open extents
    template<typename M>
    void seal_cylinder(M& mesh, id_type submesh_id, const CylinderSpecification<data_value_type<M>>& spec, id_type halfedge_id0, bool cusped_lower, bool cusped_upper, normal_vector_type<M> normal_lower = -normal_vector_type<M>::z_direction(), normal_vector_type<M> normal_upper = normal_vector_type<M>::z_direction());

    // Close off all open extents
    template<typename M>
    void seal_sphere(M& mesh, id_type submesh_id, const SphereSpecification<data_value_type<M>>& spec, id_type halfedge_id0, bool cusped_lower, bool cusped_upper);

    // Close off all open extents
    template<typename M>
    void seal_torus(M& mesh, id_type submesh_id, const TorusSpecification<data_value_type<M>>& spec, id_type halfedge_id0, bool cusped_lower, bool cusped_upper, data_value_type<M> rMajor);

    // Triangulate a triangle with segmented edges to produce a uniform triangulation
    // With rows arranged with the largest across the bottom and getting successively shorter as you go up:
    // id is interior halfedge at right side of longest row
    // n is segments per triangle side
    // Creates a trapezoid from the bottom row, triangulate it, call recursively for remaining area above
    template<typename M>
    void triangulate_rows(M& mesh, id_type idHalfedge, size_t n);

    // Project vertices onto sphere and calculate vertex attributes
    // Internally used for geodesic sphere
    // Works with Submesh and Mesh
    // Works with Surface, Submesh, and Mesh? ...
    template<typename U>
    void project_vertices(U& u, typename U::value_type radius);

    std::string surface_name(const std::string& base, size_t n);

    template<typename M>
    std::string next_surface_name(M& mesh, const std::string& base_name = "body");

    template<typename M>
    void create_facet(M& mesh, typename M::face_type& face);

    template<typename M>
    id_type create_section_surfaces(M& mesh, id_type submesh_id, size_t n, std::function<std::string(size_t)> namer);

    // deprecated
    template<typename M>
    void check_normal(size_t n, const quetzal::model::normal_vector_type<M>& normal, const typename M::vertex_type& vertex, const typename M::face_type& face);

    // These calculate normals for a single section of a band having n faces
    // id0 is the id of the first halfedge in the section
    template<typename M>
    void calculate_lower_surface_vertex_normals_sequential(M& mesh, id_type id0, size_t n);

    template<typename M>
    void calculate_upper_surface_vertex_normals_sequential(M& mesh, id_type id0, size_t n);

    // These calculate normals for an entire cusp or a single section of a cusp having n faces
    // id0 is the halfedge id of the first vertex in the sequence
    // This currently sets each cusp vertex normal to its face normal
    // These should only be called if calculate_normals is set, otherwise, ideal cylinder normals should have been precalculated in avs generation, and so the current calculation here is fine
    template<typename M>
    void calculate_apex_cusp_surface_vertex_normals_sequential(M& mesh, id_type id0, size_t n);

    template<typename M>
    void calculate_base_cusp_surface_vertex_normals_sequential(M& mesh, id_type id0, size_t n);

    template<typename M>
    quetzal::model::texture_span_type<M> texture_span(data_value_type<M> t, bool cusped_lower, bool cusped_upper, data_value_type<M> alignment = data_value_type<M>(0.5));

    template<typename M>
    quetzal::model::texture_span_type<M> texture_span(size_t i, size_t n, bool cusped_lower, bool cusped_upper, data_value_type<M> alignment = data_value_type<M>(0.5));

    template<typename M>
    quetzal::model::data_value_type<M> texture_span_v(size_t i, size_t n);

} // namespace quetzal::model

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_apex_cusp(M& mesh, size_t nAzimuth, data_value_type<M> r0, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts0, id_type surface_id, std::optional<vertex_normal_function_type<M>> vertex_normal)
{
//std::cout << "    create_apex_cusp azimuth single" << std::endl;
    assert(nAzimuth > 2);

    using T = data_value_type<M>;

    auto avs0 = vertices_attributes<M>(nAzimuth, r0, z0, azimuth_interval, ts0, vertex_normal);
    auto avs1 = cusp_vertices_attributes<M>(nAzimuth, z1, azimuth_interval, texture_span_type<M>{T(0.5), T(0.5), T(0)}, vertex_normal);

    bool closed = azimuth_interval.unit();
    create_apex_cusp(mesh, avs0, avs1, surface_id, closed, false);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_apex_cusp(M& mesh, size_t nAzimuth, data_value_type<M> r0, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts0, id_type surface_id, vertex_normal_function_type<M> vertex_normal0, vertex_normal_function_type<M> vertex_normal1)
{
//std::cout << "    create_apex_cusp azimuth single" << std::endl;
    assert(nAzimuth > 2);

    using T = data_value_type<M>;

    auto avs0 = vertices_attributes<M>(nAzimuth, r0, z0, azimuth_interval, ts0, vertex_normal0);
    auto avs1 = cusp_vertices_attributes<M>(nAzimuth, z1, azimuth_interval, texture_span_type<M>{T(0.5), T(0.5), T(0)}, vertex_normal1);

    bool closed = azimuth_interval.unit();
    create_apex_cusp(mesh, avs0, avs1, surface_id, closed, false);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_apex_cusp_sections(M& mesh, size_t nAzimuth, data_value_type<M> r0, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth0, texture_span_type<M> ts0, id_type surface_id0)
{
//std::cout << "    create_apex_cusp azimuth sections" << std::endl;
    assert(nAzimuth > 0);

    using T = data_value_type<M>;

    auto avss0 = vertices_attributes_edges<M>(nAzimuth, r0, z0, azimuth0, ts0);
    auto avss1 = cusp_vertices_attributes_edges<M>(nAzimuth, z1, texture_span_type<M>{T(0.5), T(0.5), T(0)});

    bool closed = azimuth0.unit();
    create_apex_cusp(mesh, avss0, avss1, surface_id0, closed);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_apex_cusp(M& mesh, size_t nAzimuth, std::function<typename M::point_type(data_value_type<M>)> fr0, data_value_type<M> z1, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts0, id_type surface_id, std::optional<vertex_normal_function_type<M>> vertex_normal1)
{
    assert(nAzimuth > 2);

    using T = data_value_type<M>;

    auto avs0 = vertices_attributes<M>(nAzimuth, fr0, azimuth_interval, ts0);
    auto avs1 = cusp_vertices_attributes<M>(nAzimuth, z1, azimuth_interval, texture_span_type<M>{T(0.5), T(0.5), T(0)}, vertex_normal1);

    bool closed = azimuth_interval.unit();
    create_apex_cusp(mesh, avs0, avs1, surface_id, closed, false);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_apex_cusp(M& mesh, const vertices_attributes_type<M>& avs0, const vertices_attributes_type<M>& avs1, id_type surface_id, bool closed, bool calculate_normals)
{
//std::cout << "    create_apex_cusp avs single" << std::endl;
    assert(avs1.size() > 2);
    assert(avs1.size() == avs0.size() - 1);

    size_t nh = mesh.halfedge_store_count();
    size_t nv = mesh.vertex_store_count();

    size_t face_count = avs1.size();
    assert(face_count > 2);

    mesh.halfedge_store().resize(nh + 3 * face_count);
    mesh.vertex_store().resize(nv + 3 * face_count);

    id_type j0_orig = nh;
    id_type j0 = j0_orig;

    for (size_t j = 0; j < face_count; ++j)
    {
        id_type j1 = j0 + face_count;
        id_type j2 = j1 + face_count;

        vertices_attributes_type<M> avs =
        {
            avs0[j],
            avs0[j + 1],
            avs1[j],
        };

        auto normal = geometry::normal(avs);
        id_type face_id = mesh.create_face(surface_id, j0, {normal});

        id_type partner_id = nullid;
        mesh.halfedge(j0) = {mesh, j0, partner_id, j1, j2, j0, face_id};
        partner_id = (j < face_count - 1 ? j2 + 1 : (closed ? j2 - face_count + 1 : nullid));
        mesh.halfedge(j1) = {mesh, j1, partner_id, j2, j0, j1, face_id};
        partner_id = (j > 0 ? j1 - 1 : (closed ? j1 + face_count - 1 : nullid));
        mesh.halfedge(j2) = {mesh, j2, partner_id, j0, j1, j2, face_id};

        mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
        mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
        mesh.vertex(j2) = {mesh, j2, j2, avs[2]};

        ++j0;
    }

    // Calculate normals for the lower vertices
    if (calculate_normals)
    {
        calculate_lower_surface_vertex_normals_sequential(mesh, j0_orig, face_count);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_apex_cusp(M& mesh, const vertices_attributes_sections_type<M>& avss0, const vertices_attributes_sections_type<M>& avss1, id_type surface_id0, bool closed)
{
//std::cout << "    create_apex_cusp avs sections" << std::endl;
    assert(avss1.size() > 0);
    assert(avss1.size() == avss0.size());

    size_t nh = mesh.halfedge_store_count();
    size_t nv = mesh.vertex_store_count();

    size_t section_count = avss0.size();
    size_t face_count = 0;
    for (size_t i = 0; i < section_count; ++i)
    {
        face_count += avss0[i].size() - 1;
    }

    mesh.halfedge_store().resize(nh + 3 * face_count);
    mesh.vertex_store().resize(nv + 3 * face_count);

    id_type surface_id = surface_id0;
    id_type j0_orig = nh;
    id_type j0 = j0_orig;

    for (size_t i = 0; i < section_count; ++i)
    {
        const auto& avs0 = avss0[i];
        const auto& avs1 = avss1[i];
        assert(avs1.size() == avs0.size() - 1);

        size_t section_face_count = avs1.size();
        for (size_t j = 0; j < section_face_count; ++j)
        {
            id_type j1 = j0 + face_count;
            id_type j2 = j1 + face_count;

            vertices_attributes_type<M> avs =
            {
                avs0[j],
                avs0[j + 1],
                avs1[j],
            };

            auto normal = geometry::normal(avs);
            id_type face_id = mesh.create_face(surface_id, j0, {normal});

            id_type partner_id = nullid;
            mesh.halfedge(j0) = {mesh, j0, partner_id, j1, j2, j0, face_id};
            partner_id = (i <section_count - 1 || j < section_face_count - 1 ? j2 + 1 : (closed ? j2 - face_count + 1 : nullid));
            mesh.halfedge(j1) = {mesh, j1, partner_id, j2, j0, j1, face_id};
            partner_id = (i > 0 || j > 0 ? j1 - 1 : (closed ? j1 + face_count - 1 : nullid));
            mesh.halfedge(j2) = {mesh, j2, partner_id, j0, j1, j2, face_id};

            mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
            mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
            mesh.vertex(j2) = {mesh, j2, j2, avs[2]};

            ++j0;
        }

        ++surface_id;
    }

    // Calculate normals for the lower vertices
    id_type id = j0_orig;
    for (size_t i = 0; i < section_count; ++i)
    {
        size_t n = avss0[i].size() - 1;
        calculate_lower_surface_vertex_normals_sequential(mesh, id, n);
        id += n;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_base_cusp(M& mesh, size_t nAzimuth, data_value_type<M> r1, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts1, id_type surface_id, std::optional<vertex_normal_function_type<M>> vertex_normal)
{
//std::cout << "    create_base_cusp azimuth single" << std::endl;
    using T = data_value_type<M>;

    auto avs0 = cusp_vertices_attributes<M>(nAzimuth, z0, azimuth1, texture_span_type<M>{T(0.5), T(0.5), T(1)}, vertex_normal);
    auto avs1 = vertices_attributes<M>(nAzimuth, r1, z1, azimuth1, ts1, vertex_normal);

    bool closed = azimuth1.unit();
    create_base_cusp(mesh, avs0, avs1, surface_id, closed, false);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_base_cusp(M& mesh, size_t nAzimuth, data_value_type<M> r1, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts1, id_type surface_id, vertex_normal_function_type<M> vertex_normal0, vertex_normal_function_type<M> vertex_normal1)
{
//std::cout << "    create_base_cusp azimuth single" << std::endl;
    using T = data_value_type<M>;

    auto avs0 = cusp_vertices_attributes<M>(nAzimuth, z0, azimuth1, texture_span_type<M>{T(0.5), T(0.5), T(1)}, vertex_normal0);
    auto avs1 = vertices_attributes<M>(nAzimuth, r1, z1, azimuth1, ts1, vertex_normal1);

    bool closed = azimuth1.unit();
    create_base_cusp(mesh, avs0, avs1, surface_id, closed, false);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_base_cusp_sections(M& mesh, size_t nAzimuth, data_value_type<M> r1, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts1, id_type surface_id0)
{
//std::cout << "    create_base_cusp azimuth sections" << std::endl;
    using T = data_value_type<M>;

    auto avss0 = cusp_vertices_attributes_edges<M>(nAzimuth, z0, texture_span_type<M>{T(0.5), T(0.5), T(1)});
    auto avss1 = vertices_attributes_edges<M>(nAzimuth, r1, z1, azimuth1, ts1);

    bool closed = azimuth1.unit();
    create_base_cusp(mesh, avss0, avss1, surface_id0, closed);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_base_cusp(M& mesh, size_t nAzimuth, data_value_type<M> z0, std::function<typename M::point_type(data_value_type<M>)> f1, azimuth_interval_type<M> azimuth_interval, texture_span_type<M> ts1, id_type surface_id, std::optional<vertex_normal_function_type<M>> vertex_normal0)
{
    using T = data_value_type<M>;

    auto avs0 = cusp_vertices_attributes<M>(nAzimuth, z0, azimuth_interval, texture_span_type<M>{T(0.5), T(0.5), T(1)}, vertex_normal0);
    auto avs1 = vertices_attributes<M>(nAzimuth, f1, azimuth_interval, ts1);

    bool closed = azimuth_interval.unit();
    create_base_cusp(mesh, avs0, avs1, surface_id, closed, false);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_base_cusp(M& mesh, const vertices_attributes_type<M>& avs0, const vertices_attributes_type<M>& avs1, id_type surface_id, bool closed, bool calculate_normals)
{
//std::cout << "    create_base_cusp avs single" << std::endl;
    assert(avs0.size() > 2);
    assert(avs1.size() == avs0.size() + 1);

    size_t nh = mesh.halfedge_store_count();
    size_t nv = mesh.vertex_store_count();

    size_t face_count = avs0.size();
    assert(face_count > 2);

    mesh.halfedge_store().resize(nh + 3 * face_count);
    mesh.vertex_store().resize(nv + 3 * face_count);

    id_type j0_orig = nh;
    id_type j0 = j0_orig;

    for (size_t j = 0; j < face_count; ++j)
    {
        size_t j1 = j0 + face_count;
        size_t j2 = j1 + face_count;

        vertices_attributes_type<M> avs =
        {
            avs0[j],
            avs1[j + 1],
            avs1[j],
        };

        auto normal = geometry::normal(avs);
        id_type face_id = mesh.create_face(surface_id, j0, {normal});
        if (calculate_normals)
        {
            avs[0].set_normal(normal); // This is fine unless we want to use vertex_normal function to be applied to the base cusp
        }

        id_type partner_id = (j < face_count - 1 ? j2 + 1 : (closed ? j2 - face_count + 1 : nullid));
        mesh.halfedge(j0) = {mesh, j0, partner_id, j1, j2, j0, face_id};
        partner_id = nullid;
        mesh.halfedge(j1) = {mesh, j1, partner_id, j2, j0, j1, face_id};
        partner_id = (j > 0 ? j0 - 1 : (closed ? j0 + face_count - 1 : nullid));
        mesh.halfedge(j2) = {mesh, j2, partner_id, j0, j1, j2, face_id};

        mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
        mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
        mesh.vertex(j2) = {mesh, j2, j2, avs[2]};

        ++j0;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_base_cusp(M& mesh, const vertices_attributes_sections_type<M>& avss0, const vertices_attributes_sections_type<M>& avss1, id_type surface_id0, bool closed)
{
//std::cout << "    create_base_cusp avs sections" << std::endl;
    assert(avss0.size() > 0);
    assert(avss1.size() == avss0.size());
    assert(avss1[0].size() == avss0[0].size() + 1);

    size_t nh = mesh.halfedge_store_count();
    size_t nv = mesh.vertex_store_count();

    size_t section_count = avss1.size();
    size_t face_count = 0;
    for (size_t i = 0; i < section_count; ++i)
    {
        face_count += avss1[i].size() - 1;
    }

    mesh.halfedge_store().resize(nh + 3 * face_count);
    mesh.vertex_store().resize(nv + 3 * face_count);

    id_type surface_id = surface_id0;
    id_type j0_orig = nh;
    id_type j0 = j0_orig;

    for (size_t i = 0; i < section_count; ++i)
    {
        const auto& avs0 = avss0[i];
        const auto& avs1 = avss1[i];
        assert(avs1.size() == avs0.size() + 1);

        size_t section_face_count = avs0.size();
        for (size_t j = 0; j < section_face_count; ++j)
        {
            id_type j1 = j0 + face_count;
            id_type j2 = j1 + face_count;

            vertices_attributes_type<M> avs =
            {
                avs0[j],
                avs1[j + 1],
                avs1[j],
            };

            auto normal = geometry::normal(avs);
            id_type face_id = mesh.create_face(surface_id, j0, {normal});
            avs[0].set_normal(normal); // This is fine unless we want to use vertex_normal function to be applied to the base cusp

            id_type partner_id = (i < section_count - 1 || j < section_face_count - 1 ? j2 + 1 : (closed ? j2 - face_count + 1 : nullid));
            mesh.halfedge(j0) = {mesh, j0, partner_id, j1, j2, j0, face_id};
            partner_id = nullid;
            mesh.halfedge(j1) = {mesh, j1, partner_id, j2, j0, j1, face_id};
            partner_id = (i > 0 || j > 0 ? j0 - 1 : (closed ? j0 + face_count - 1 : nullid));
            mesh.halfedge(j2) = {mesh, j2, partner_id, j0, j1, j2, face_id};

            mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
            mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
            mesh.vertex(j2) = {mesh, j2, j2, avs[2]};
     
            ++j0;
        }

        ++surface_id;
    }

/*
    // Calculate normals for the lower vertices
    // this just recalculates what was already done above ...
    id_type id = j0_orig;
    for (size_t i = 0; i < section_count; ++i)
    {
        size_t n = avss0[i].size() - 1;
        calculate_base_cusp_surface_vertex_normals_sequential(mesh, id, n);
        id += n;
    }
*/
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_band(M& mesh, size_t nAzimuth, data_value_type<M> r0, data_value_type<M> r1, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth0, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts0, texture_span_type<M> ts1, id_type surface_id,  std::optional<vertex_normal_function_type<M>> vertex_normal)
{
//std::cout << "    create_band azimuth single" << std::endl;
    assert(azimuth0.unit() == azimuth1.unit());

    auto avs0 = vertices_attributes<M>(nAzimuth, r0, z0, azimuth0, ts0, vertex_normal);
    auto avs1 = vertices_attributes<M>(nAzimuth, r1, z1, azimuth1, ts1, vertex_normal);

    bool closed = azimuth1.unit();
    create_band(mesh, avs0, avs1, surface_id, closed, false);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_band(M& mesh, size_t nAzimuth, data_value_type<M> r0, data_value_type<M> r1, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth0, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts0, texture_span_type<M> ts1, id_type surface_id, vertex_normal_function_type<M> vertex_normal0, vertex_normal_function_type<M> vertex_normal1)
{
//std::cout << "    create_band azimuth single" << std::endl;
    assert(azimuth0.unit() == azimuth1.unit());

    auto avs0 = vertices_attributes<M>(nAzimuth, r0, z0, azimuth0, ts0, vertex_normal0);
    auto avs1 = vertices_attributes<M>(nAzimuth, r1, z1, azimuth1, ts1, vertex_normal1);

    bool closed = azimuth1.unit();
    create_band(mesh, avs0, avs1, surface_id, closed, false);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_band_sections(M& mesh, size_t nAzimuth, data_value_type<M> r0, data_value_type<M> r1, data_value_type<M> z0, data_value_type<M> z1, azimuth_interval_type<M> azimuth0, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts0, texture_span_type<M> ts1, id_type surface_id0)
{
//std::cout << "    create_band azimuth sections" << std::endl;
    assert(azimuth0.unit() == azimuth1.unit());

    auto avss0 = vertices_attributes_edges<M>(nAzimuth, r0, z0, azimuth0, ts0);
    auto avss1 = vertices_attributes_edges<M>(nAzimuth, r1, z1, azimuth1, ts1);

    bool closed = azimuth1.unit();
    create_band(mesh, avss0, avss1, surface_id0, closed);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_band(M& mesh, size_t nAzimuth, std::function<typename M::point_type(data_value_type<M>)> f0, std::function<typename M::point_type(data_value_type<M>)> f1, azimuth_interval_type<M> azimuth0, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts0, texture_span_type<M> ts1, id_type surface_id)
{
    assert(azimuth0.unit() == azimuth1.unit());

    auto avs0 = vertices_attributes<M>(nAzimuth, f0, azimuth0, ts0);
    auto avs1 = vertices_attributes<M>(nAzimuth, f1, azimuth1, ts1);

    bool closed = azimuth1.unit();
    create_band(mesh, avs0, avs1, surface_id, closed, true);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_band(M& mesh, const vertices_attributes_type<M>& avs0, const vertices_attributes_type<M>& avs1, id_type surface_id, bool closed, bool calculate_normals)
{
//std::cout << "    create_band avs single" << std::endl;
    assert(avs0.size() > 2);
    assert(avs1.size() == avs0.size());

    size_t face_count = avs0.size() - 1;
    assert(face_count > 2);

    size_t nh = mesh.halfedge_store_count();
    size_t nv = mesh.vertex_store_count();

    mesh.halfedge_store().resize(nh + 4 * face_count);
    mesh.vertex_store().resize(nv + 4 * face_count);

    id_type j0_orig = nh;
    id_type j0 = j0_orig;

    for (size_t j = 0; j < face_count; ++j)
    {
        size_t j1 = j0 + face_count;
        size_t j2 = j1 + face_count;
        size_t j3 = j2 + face_count;

        vertices_attributes_type<M> avs =
        {
            avs0[j],
            avs0[j + 1],
            avs1[j + 1],
            avs1[j],
        };

        auto normal = geometry::normal(avs);
        id_type face_id = mesh.create_face(surface_id, j0, {normal});

        id_type partner_id = nullid;
        mesh.halfedge(j0) = {mesh, j0, partner_id, j1, j3, j0, face_id};
        partner_id = (j < face_count - 1 ? j3 + 1 : (closed ? j3 - face_count + 1 : nullid));
        mesh.halfedge(j1) = {mesh, j1, partner_id, j2, j0, j1, face_id};
        partner_id = nullid;
        mesh.halfedge(j2) = {mesh, j2, partner_id, j3, j1, j2, face_id};
        partner_id = (j > 0 ? j1 - 1 : (closed ? j1 + face_count - 1 : nullid));
        mesh.halfedge(j3) = {mesh, j3, partner_id, j0, j2, j3, face_id};

        mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
        mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
        mesh.vertex(j2) = {mesh, j2, j2, avs[2]};
        mesh.vertex(j3) = {mesh, j3, j3, avs[3]};

        ++j0;
    }

    // Calculate normals for the lower vertices
    if (calculate_normals)
    {
        calculate_lower_surface_vertex_normals_sequential(mesh, j0_orig, face_count);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_band_edges(M& mesh, const vertices_attributes_type<M>& avs0, const vertices_attributes_type<M>& avs1, id_type surface_id0, bool closed, bool calculate_normals)
{
//std::cout << "    create_band_edges avs single" << std::endl;
    assert(avs0.size() > 2);
    assert(avs1.size() == avs0.size());

    size_t face_count = avs0.size() - 1;
    assert(face_count > 2);

    size_t nh = mesh.halfedge_store_count();
    size_t nv = mesh.vertex_store_count();

    mesh.halfedge_store().resize(nh + 4 * face_count);
    mesh.vertex_store().resize(nv + 4 * face_count);

    id_type surface_id = surface_id0;

    id_type j0_orig = nh;
    id_type j0 = j0_orig;

    for (size_t j = 0; j < face_count; ++j)
    {
        size_t j1 = j0 + face_count;
        size_t j2 = j1 + face_count;
        size_t j3 = j2 + face_count;

        vertices_attributes_type<M> avs =
        {
            avs0[j],
            avs0[j + 1],
            avs1[j + 1],
            avs1[j],
        };

        auto normal = geometry::normal(avs);
        id_type face_id = mesh.create_face(surface_id, j0, {normal});

        id_type partner_id = nullid;
        mesh.halfedge(j0) = {mesh, j0, partner_id, j1, j3, j0, face_id};
        partner_id = (j < face_count - 1 ? j3 + 1 : (closed ? j3 - face_count + 1 : nullid));
        mesh.halfedge(j1) = {mesh, j1, partner_id, j2, j0, j1, face_id};
        partner_id = nullid;
        mesh.halfedge(j2) = {mesh, j2, partner_id, j3, j1, j2, face_id};
        partner_id = (j > 0 ? j1 - 1 : (closed ? j1 + face_count - 1 : nullid));
        mesh.halfedge(j3) = {mesh, j3, partner_id, j0, j2, j3, face_id};

        mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
        mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
        mesh.vertex(j2) = {mesh, j2, j2, avs[2]};
        mesh.vertex(j3) = {mesh, j3, j3, avs[3]};

        ++j0;
        ++surface_id;
    }

    // Calculate normals for the lower vertices
    if (calculate_normals)
    {
        calculate_lower_surface_vertex_normals_sequential(mesh, j0_orig, face_count);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_band(M& mesh, const vertices_attributes_sections_type<M>& avss0, const vertices_attributes_sections_type<M>& avss1, id_type surface_id0, bool closed)
{
//std::cout << "    create_band avss" << std::endl;
//std::cout << std::setprecision(10) << avss0;
//std::cout << std::setprecision(10) << avss1;
    assert(avss0.size() > 0);
    assert(avss1.size() == avss0.size());

    size_t nh = mesh.halfedge_store_count();
    size_t nv = mesh.vertex_store_count();

    size_t section_count = avss0.size();
    size_t face_count = 0;
    for (size_t i = 0; i < section_count; ++i)
    {
        assert(avss1[i].size() == avss0[i].size());
        face_count += avss0[i].size() - 1;
    }

    mesh.halfedge_store().resize(nh + 4 * face_count);
    mesh.vertex_store().resize(nv + 4 * face_count);

    id_type surface_id = surface_id0;
    id_type j0_orig = nh;
    id_type j0 = j0_orig;

    for (size_t i = 0; i < section_count; ++i)
    {
        const auto& avs0 = avss0[i];
        const auto& avs1 = avss1[i];

        size_t section_face_count = avs0.size() - 1;
        for (size_t j = 0; j < section_face_count; ++j)
        {
            id_type j1 = j0 + face_count;
            id_type j2 = j1 + face_count;
            id_type j3 = j2 + face_count;

            vertices_attributes_type<M> avs =
            {
                avs0[j],
                avs0[j + 1],
                avs1[j + 1],
                avs1[j],
            };

            auto normal = geometry::normal(avs);
            id_type face_id = mesh.create_face(surface_id, j0, {normal});

            id_type partner_id = nullid;
            mesh.halfedge(j0) = {mesh, j0, partner_id, j1, j3, j0, face_id};
            partner_id = (i < section_count - 1 || j < section_face_count - 1 ? j3 + 1 : (closed ? j3 - face_count + 1 : nullid));
            mesh.halfedge(j1) = {mesh, j1, partner_id, j2, j0, j1, face_id};
            partner_id = nullid;
            mesh.halfedge(j2) = {mesh, j2, partner_id, j3, j1, j2, face_id};
            partner_id = (i > 0 || j > 0 ? j1 - 1 : (closed ? j1 + face_count - 1 : nullid));
            mesh.halfedge(j3) = {mesh, j3, partner_id, j0, j2, j3, face_id};

            mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
            mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
            mesh.vertex(j2) = {mesh, j2, j2, avs[2]};
            mesh.vertex(j3) = {mesh, j3, j3, avs[3]};
     
            ++j0;
        }

        ++surface_id;
    }

    // Calculate normals for the lower vertices
    id_type id = j0_orig;
    for (size_t i = 0; i < section_count; ++i)
    {
        size_t n = avss0[i].size() - 1;
        calculate_lower_surface_vertex_normals_sequential(mesh, id, n);
        id += n;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_antiband(M& mesh, const vertices_attributes_type<M>& avs0, const vertices_attributes_type<M>& avs1, id_type surface_id)
{
    using T = data_value_type<M>;

    assert(avs1.size() == avs0.size());

    size_t nAzimuth = avs0.size() - 1;
    assert(nAzimuth > 2);

    size_t nh = mesh.halfedge_store_count();

    mesh.halfedge_store().resize(nh + 6 * nAzimuth);
    mesh.vertex_store().resize(nh + 6 * nAzimuth);

    for (size_t j = 0; j < nAzimuth; ++j)
    {
        size_t j0 = nh;
        size_t j1 = j0 + nAzimuth;
        size_t j2 = j1 + nAzimuth;
        size_t j3 = j2 + nAzimuth;
        size_t j4 = j3 + nAzimuth;
        size_t j5 = j4 + nAzimuth;

        id_type face_id = mesh.create_face(surface_id, j0);

        id_type idPartner = nullid;
        mesh.halfedge(j0) = {mesh, j0, idPartner, j1, j2, j0, face_id};
        idPartner = j5;
        mesh.halfedge(j1) = {mesh, j1, idPartner, j2, j0, j1, face_id};
        idPartner = (j > 0 ? j3 - 1 : j3 + nAzimuth - 1);
        mesh.halfedge(j2) = {mesh, j2, idPartner, j0, j1, j2, face_id};

        mesh.vertex(j0) = {mesh, j0, j0, avs0[j]};
        mesh.vertex(j1) = {mesh, j1, j1, avs0[j + 1]};
        mesh.vertex(j2) = {mesh, j2, j2, avs1[j]};

        auto normal = geometry::normal(mesh.face(face_id));
        mesh.face(face_id).attributes().set_normal(normal);

        face_id = mesh.create_face(surface_id, j3);

        idPartner = (j < nAzimuth - 1 ? j2 + 1 : j2 - nAzimuth + 1);
        mesh.halfedge(j3) = {mesh, j3, idPartner, j4, j5, j3, face_id};
        idPartner = nullid;
        mesh.halfedge(j4) = {mesh, j4, idPartner, j5, j3, j4, face_id};
        idPartner = j1;
        mesh.halfedge(j5) = {mesh, j5, idPartner, j3, j4, j5, face_id};

        mesh.vertex(j3) = {mesh, j3, j3, avs0[j + 1]};
        mesh.vertex(j4) = {mesh, j4, j4, avs1[j + 1]};
        mesh.vertex(j5) = {mesh, j5, j5, avs1[j]};

        normal = geometry::normal(mesh.face(face_id));
        mesh.face(face_id).attributes().set_normal(normal);

        ++nh;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_antiband_inverted(M& mesh, const vertices_attributes_type<M>& avs0, const vertices_attributes_type<M>& avs1, id_type surface_id)
{
    using T = data_value_type<M>;

    assert(avs1.size() == avs0.size());

    size_t nAzimuth = avs0.size() - 1;
    assert(nAzimuth > 2);

    size_t nh = mesh.halfedge_store_count();

    mesh.halfedge_store().resize(nh + 6 * nAzimuth);
    mesh.vertex_store().resize(nh + 6 * nAzimuth);

    for (size_t j = 0; j < nAzimuth; ++j)
    {
        size_t j0 = nh;
        size_t j1 = j0 + nAzimuth;
        size_t j2 = j1 + nAzimuth;
        size_t j3 = j2 + nAzimuth;
        size_t j4 = j3 + nAzimuth;
        size_t j5 = j4 + nAzimuth;

        id_type face_id = mesh.create_face(surface_id, j0);

        id_type idPartner = nullid;
        mesh.halfedge(j0) = {mesh, j0, idPartner, j1, j2, j0, face_id};
        idPartner =  (j < nAzimuth - 1 ? j5 + 1 : j5 - nAzimuth + 1);
        mesh.halfedge(j1) = {mesh, j1, idPartner, j2, j0, j1, face_id};
        idPartner = j3;
        mesh.halfedge(j2) = {mesh, j2, idPartner, j0, j1, j2, face_id};

        mesh.vertex(j0) = {mesh, j0, j0, avs0[j]};
        mesh.vertex(j1) = {mesh, j1, j1, avs0[j + 1]};
        mesh.vertex(j2) = {mesh, j2, j2, avs1[j + 1]};

        auto normal = geometry::normal(mesh.face(face_id));
        mesh.face(face_id).attributes().set_normal(normal);

        face_id = mesh.create_face(surface_id, j3);

        idPartner = j2;
        mesh.halfedge(j3) = {mesh, j3, idPartner, j4, j5, j3, face_id};
        idPartner = nullid;
        mesh.halfedge(j4) = {mesh, j4, idPartner, j5, j3, j4, face_id};
        idPartner = (j > 0 ? j1 - 1 : j1 + nAzimuth - 1);
        mesh.halfedge(j5) = {mesh, j5, idPartner, j3, j4, j5, face_id};

        mesh.vertex(j3) = {mesh, j3, j3, avs0[j]};
        mesh.vertex(j4) = {mesh, j4, j4, avs1[j + 1]};
        mesh.vertex(j5) = {mesh, j5, j5, avs1[j]};

        normal = geometry::normal(mesh.face(face_id));
        mesh.face(face_id).attributes().set_normal(normal);

        ++nh;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_antiband_faces(M& mesh, const vertices_attributes_type<M>& avs0, const vertices_attributes_type<M>& avs1, id_type submesh_id)
{
    using T = data_value_type<M>;

    assert(avs1.size() == avs0.size());

    size_t nAzimuth = avs0.size() - 1;
    assert(nAzimuth > 2);

    size_t nh = mesh.halfedge_store_count();

    mesh.halfedge_store().resize(nh + 6 * nAzimuth);
    mesh.vertex_store().resize(nh + 6 * nAzimuth);

    for (size_t j = 0; j < nAzimuth; ++j)
    {
        size_t j0 = nh;
        size_t j1 = j0 + nAzimuth;
        size_t j2 = j1 + nAzimuth;
        size_t j3 = j2 + nAzimuth;
        size_t j4 = j3 + nAzimuth;
        size_t j5 = j4 + nAzimuth;

        id_type surface_id = mesh.create_surface(submesh_id, next_surface_name(mesh));
        id_type face_id = mesh.create_face(surface_id, j0);

        id_type idPartner = nullid;
        mesh.halfedge(j0) = {mesh, j0, idPartner, j1, j2, j0, face_id};
        idPartner = j5;
        mesh.halfedge(j1) = {mesh, j1, idPartner, j2, j0, j1, face_id};
        idPartner = (j > 0 ? j3 - 1 : j3 + nAzimuth - 1);
        mesh.halfedge(j2) = {mesh, j2, idPartner, j0, j1, j2, face_id};

        mesh.vertex(j0) = {mesh, j0, j0, avs0[j]};
        mesh.vertex(j1) = {mesh, j1, j1, avs0[j + 1]};
        mesh.vertex(j2) = {mesh, j2, j2, avs1[j]};

        set_facet_normal(mesh.face(face_id));
        set_triangular_texcoords(mesh, face_id);

        surface_id = mesh.create_surface(submesh_id, next_surface_name(mesh));
        face_id = mesh.create_face(surface_id, j3);

        idPartner = (j < nAzimuth - 1 ? j2 + 1 : j2 - nAzimuth + 1);
        mesh.halfedge(j3) = {mesh, j3, idPartner, j4, j5, j3, face_id};
        idPartner = nullid;
        mesh.halfedge(j4) = {mesh, j4, idPartner, j5, j3, j4, face_id};
        idPartner = j1;
        mesh.halfedge(j5) = {mesh, j5, idPartner, j3, j4, j5, face_id};

        mesh.vertex(j3) = {mesh, j3, j3, avs0[j + 1]};
        mesh.vertex(j4) = {mesh, j4, j4, avs1[j + 1]};
        mesh.vertex(j5) = {mesh, j5, j5, avs1[j]};

        set_facet_normal(mesh.face(face_id));
// A v up vector would allow for this ...
//        set_triangular_texcoords(mesh, face_id);
        mesh.vertex(j3).attributes().set_texcoord({T(0.5), T(1)});
        mesh.vertex(j4).attributes().set_texcoord({T(1), T(0)});
        mesh.vertex(j5).attributes().set_texcoord({T(0), T(0)});

        ++nh;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_apex_cusp(M& mesh, size_t nAzimuth, data_value_type<M> z1, azimuth_interval_type<M> azimuth1, id_type surface_id, std::optional<vertex_normal_function_type<M>> vertex_normal)
{
//std::cout << "    connect_apex_cusp azimuth single" << std::endl;
    assert(nAzimuth > 0);

    using T = data_value_type<M>;

    auto avs1 = cusp_vertices_attributes<M>(nAzimuth, z1, azimuth1, {T(0.5), T(0.5), T(0)}, vertex_normal);

    bool closed = azimuth1.unit();
    bool calculate_normals = false; // smooth, but we don't have this info; needs to be true in the sphere case? but seems to be working anyway ...
    connect_apex_cusp(mesh, avs1, surface_id, closed, calculate_normals);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_apex_cusp_sections(M& mesh, size_t nAzimuth, data_value_type<M> z1, azimuth_interval_type<M> azimuth1, id_type surface_id0)
{
//std::cout << "    connect_apex_cusp azimuth sections" << std::endl;
    assert(nAzimuth > 0);

    using T = data_value_type<M>;

    auto avss1 = cusp_vertices_attributes_edges<M>(nAzimuth, z1, {T(0.5), T(0.5), T(0)});

    bool closed = azimuth1.unit();
    connect_apex_cusp(mesh, avss1, surface_id0, closed);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_apex_cusp(M& mesh, const vertices_attributes_type<M>& avs1, id_type surface_id, bool closed, bool calculate_normals)
{
//std::cout << "    connect_apex_cusp avs single" << std::endl;
    size_t nh = mesh.halfedge_store_count();
    size_t nv = mesh.vertex_store_count();

    size_t face_count = avs1.size();
    assert(face_count > 2);
    assert(mesh.halfedge_count() >= 2 * face_count);

    mesh.halfedge_store().resize(nh + 3 * face_count);
    mesh.vertex_store().resize(nv + 3 * face_count);

    id_type j0_orig = nh;
    id_type j0 = j0_orig;
    id_type partner_id0 = j0 - 2 * face_count;

    for (size_t j = 0; j < face_count; ++j)
    {
        id_type j1 = j0 + face_count;
        id_type j2 = j1 + face_count;

        vertices_attributes_type<M> avs =
        {
            mesh.halfedge(partner_id0).next().vertex_attributes(),
            mesh.halfedge(partner_id0).vertex_attributes(),
            avs1[j],
        };

        auto normal = geometry::normal(avs);
        id_type face_id = mesh.create_face(surface_id, j0, {normal});

        mesh.halfedge(partner_id0).set_partner_id(j0);

        id_type partner_id = partner_id0;
        mesh.halfedge(j0) = {mesh, j0, partner_id, j1, j2, j0, face_id};
        partner_id = (j < face_count - 1 ? j2 + 1 : (closed ? j2 - face_count + 1 : nullid));
        mesh.halfedge(j1) = {mesh, j1, partner_id, j2, j0, j1, face_id};
        partner_id = (j > 0 ? j1 - 1 : (closed ? j1 + face_count - 1 : nullid));
        mesh.halfedge(j2) = {mesh, j2, partner_id, j0, j1, j2, face_id};

        mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
        mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
        mesh.vertex(j2) = {mesh, j2, j2, avs[2]};

        ++j0;
        ++partner_id0;
    }

    // Calculate normals for the lower vertices just connected
    if (calculate_normals)
    {
        calculate_lower_surface_vertex_normals_sequential(mesh, j0_orig, face_count);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_apex_cusp(M& mesh, const vertices_attributes_sections_type<M>& avss1, id_type surface_id0, bool closed)
{
//std::cout << "    connect_apex_cusp avss" << std::endl;
//std::cout << avss1;
    size_t nh = mesh.halfedge_store_count();
    size_t nv = mesh.vertex_store_count();

    size_t section_count = avss1.size();
    size_t face_count = 0;
    for (size_t i = 0; i < section_count; ++i)
    {
        face_count += avss1[i].size();
    }

    mesh.halfedge_store().resize(nh + 3 * face_count);
    mesh.vertex_store().resize(nv + 3 * face_count);

    id_type surface_id = surface_id0;
    id_type j0_orig = nh;
    id_type j0 = j0_orig;
    id_type partner_id0 = j0_orig - 2 * face_count;

    for (size_t i = 0; i < section_count; ++i)
    {
        const auto& avs1 = avss1[i];

        size_t section_face_count = avs1.size();
        for (size_t j = 0; j < section_face_count; ++j)
        {
            id_type j1 = j0 + face_count;
            id_type j2 = j1 + face_count;

            vertices_attributes_type<M> avs =
            {
                mesh.halfedge(partner_id0).next().vertex_attributes(),
                mesh.halfedge(partner_id0).vertex_attributes(),
                avs1[j],
            };

            auto normal = geometry::normal(avs);
            id_type face_id = mesh.create_face(surface_id, j0, {normal});
            avs[2].set_normal(normal); // ...

            mesh.halfedge(partner_id0).set_partner_id(j0);

            mesh.halfedge(j0) = {mesh, j0, partner_id0, j1, j2, j0, face_id};
            id_type partner_id1 = (i < section_count - 1 || j < section_face_count - 1 ? j2 + 1 : (closed ? j2 - face_count + 1 : nullid));
            mesh.halfedge(j1) = {mesh, j1, partner_id1, j2, j0, j1, face_id};
            id_type  partner_id2 = (i > 0 || j > 0 ? j1 - 1 : (closed ? j1 + face_count - 1 : nullid));
            mesh.halfedge(j2) = {mesh, j2, partner_id2, j0, j1, j2, face_id};

            mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
            mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
            mesh.vertex(j2) = {mesh, j2, j2, avs[2]};
     
            ++j0;
            ++partner_id0;
        }

        ++surface_id;
    }

    // Calculate normals for the lower vertices just connected
    id_type id = j0_orig;
    for (size_t i = 0; i < section_count; ++i)
    {
        size_t n = avss1[i].size();
        calculate_lower_surface_vertex_normals_sequential(mesh, id, n);
        id += n;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_band(M& mesh, size_t nAzimuth, data_value_type<M> r1, data_value_type<M> z1, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts1, id_type surface_id, std::optional<vertex_normal_function_type<M>> vertex_normal)
{
//std::cout << "    connect_band azimuth single" << std::endl;
    assert(nAzimuth > 0);

    auto avs1 = vertices_attributes<M>(nAzimuth, r1, z1, azimuth1, ts1, vertex_normal);
    bool closed = azimuth1.unit();
    connect_band(mesh, avs1, surface_id, closed, false);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_band_sections(M& mesh, size_t nAzimuth, data_value_type<M> r1, data_value_type<M> z1, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts1, id_type surface_id0)
{
//std::cout << "    connect_band azimuth sections" << std::endl;
    assert(nAzimuth > 0);

    auto avss1 = vertices_attributes_edges<M>(nAzimuth, r1, z1, azimuth1, ts1);
    bool closed = azimuth1.unit();
    connect_band(mesh, avss1, surface_id0, closed);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_band(M& mesh, size_t nAzimuth, std::function<typename M::point_type(data_value_type<M>)> f1, azimuth_interval_type<M> azimuth1, texture_span_type<M> ts1, id_type surface_id)
{
    assert(nAzimuth > 0);

    auto avs1 = vertices_attributes<M>(nAzimuth, f1, azimuth1, ts1);
    bool closed = azimuth1.unit();
    connect_band(mesh, avs1, surface_id, closed, true);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_band(M& mesh, const vertices_attributes_type<M>& avs1, id_type surface_id, bool closed, bool calculate_normals)
{
//std::cout << "    connect_band avs single" << std::endl;
    size_t nh = mesh.halfedge_store_count();
    size_t nv = mesh.vertex_store_count();

    size_t face_count = avs1.size() - 1;
    assert(face_count > 2);
    assert(mesh.halfedge_count() >= 2 * face_count);

    mesh.halfedge_store().resize(nh + 4 * face_count);
    mesh.vertex_store().resize(nv + 4 * face_count);

    id_type j0_orig = nh;
    id_type j0 = j0_orig;

    for (size_t j = 0; j < face_count; ++j)
    {
        size_t j1 = j0 + face_count;
        size_t j2 = j1 + face_count;
        size_t j3 = j2 + face_count;

        vertices_attributes_type<M> avs =
        {
            mesh.vertex(j3 - 4 * face_count).attributes(),
            mesh.vertex(j2 - 4 * face_count).attributes(),
            avs1[j + 1],
            avs1[j],
        };

        auto normal = geometry::normal(avs);
        id_type face_id = mesh.create_face(surface_id, j0, {normal});

        id_type idPartner = j2 - 4 * face_count;
        mesh.halfedge(idPartner).set_partner_id(j0);

        mesh.halfedge(j0) = {mesh, j0, idPartner, j1, j3, j0, face_id};
        idPartner = (j < face_count - 1 ? j3 + 1 : (closed ? j3 - face_count + 1 : nullid));
        mesh.halfedge(j1) = {mesh, j1, idPartner, j2, j0, j1, face_id};
        idPartner = nullid;
        mesh.halfedge(j2) = {mesh, j2, idPartner, j3, j1, j2, face_id};
        idPartner = (j > 0 ? j1 - 1 : (closed ? j1 + face_count - 1 : nullid));
        mesh.halfedge(j3) = {mesh, j3, idPartner, j0, j2, j3, face_id};

        mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
        mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
        mesh.vertex(j2) = {mesh, j2, j2, avs[2]};
        mesh.vertex(j3) = {mesh, j3, j3, avs[3]};

        ++j0;
    }

    // Calculate normals for the lower vertices just connected
    if (calculate_normals)
    {
        calculate_lower_surface_vertex_normals_sequential(mesh, j0_orig, face_count);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_band(M& mesh, const vertices_attributes_sections_type<M>& avss1, id_type surface_id0, bool closed)
{
//std::cout << "    connect_band avss" << std::endl;
//std::cout << avss1;
    size_t nh = mesh.halfedge_store_count();
    size_t nv = mesh.vertex_store_count();

    size_t section_count = avss1.size();
    size_t face_count = 0;
    for (size_t i = 0; i < section_count; ++i)
    {
        face_count += avss1[i].size() - 1;
    }

    mesh.halfedge_store().resize(nh + 4 * face_count);
    mesh.vertex_store().resize(nv + 4 * face_count);

    id_type surface_id = surface_id0;
    id_type j0_orig = nh;
    id_type j0 = j0_orig;
    id_type partner_id0 = j0_orig - 2 * face_count;

    for (size_t i = 0; i < section_count; ++i)
    {
        const auto& avs1 = avss1[i];

        size_t section_face_count = avs1.size() - 1;
        for (size_t j = 0; j < section_face_count; ++j)
        {
            id_type j1 = j0 + face_count;
            id_type j2 = j1 + face_count;
            id_type j3 = j2 + face_count;

            vertices_attributes_type<M> avs =
            {
                mesh.halfedge(partner_id0).next().vertex_attributes(),
                mesh.halfedge(partner_id0).vertex_attributes(),
                avs1[j + 1],
                avs1[j],
            };

            auto normal = geometry::normal(avs);
            id_type face_id = mesh.create_face(surface_id, j0, {normal});

            mesh.halfedge(partner_id0).set_partner_id(j0);

            id_type partner_id = partner_id0;
            mesh.halfedge(j0) = {mesh, j0, partner_id, j1, j3, j0, face_id};
            partner_id = (i < section_count - 1 || j < section_face_count - 1 ? j3 + 1 : (closed ? j3 - face_count + 1 : nullid));
            mesh.halfedge(j1) = {mesh, j1, partner_id, j2, j0, j1, face_id};
            partner_id = nullid;
            mesh.halfedge(j2) = {mesh, j2, partner_id, j3, j1, j2, face_id};
            partner_id = (i > 0 || j > 0 ? j1 - 1 : (closed ? j1 + face_count - 1 : nullid));
            mesh.halfedge(j3) = {mesh, j3, partner_id, j0, j2, j3, face_id};

            mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
            mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
            mesh.vertex(j2) = {mesh, j2, j2, avs[2]};
            mesh.vertex(j3) = {mesh, j3, j3, avs[3]};

            ++j0;
            ++partner_id0;
        }

        ++surface_id;
    }

    // Calculate normals for the lower vertices just connected
    id_type id = j0_orig;
    for (size_t i = 0; i < section_count; ++i)
    {
        size_t n = avss1[i].size() - 1;
        calculate_lower_surface_vertex_normals_sequential(mesh, id, n);
        id += n;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_band(M& mesh, const vertices_attributes_type<M>& avs1, const std::vector<id_type>& idPartners, id_type idSurface0, bool bClosed)
{
//std::cout << "    connect_band avs partners" << std::endl;
    size_t nAzimuth = avs1.size() - 1;
    assert(nAzimuth > 2);
    assert(mesh.halfedge_count() >= 2 * nAzimuth);

    size_t nh = mesh.halfedge_store_count();
    size_t nv = mesh.vertex_store_count();

    mesh.halfedge_store().resize(nh + 4 * nAzimuth);
    mesh.vertex_store().resize(nv + 4 * nAzimuth);

    for (size_t j = 0; j < nAzimuth; ++j)
    {
        size_t j0 = nh;
        size_t j1 = j0 + nAzimuth;
        size_t j2 = j1 + nAzimuth;
        size_t j3 = j2 + nAzimuth;

        id_type idSurface = idSurface0;
        // smooth, single surface, sections not supported, but they need to be, add avss version ...
/*
        if (!bSmooth && idSurface != nullid)
        {
            idSurface += j;
        }
*/
        vertices_attributes_type<M> avs =
        {
            mesh.vertex(idPartners[j]).attributes(), // different ...
            mesh.vertex(idPartners[(j + 1) % nAzimuth]).attributes(), // different ...
            avs1[j + 1],
            avs1[j],
        };

        auto normal = geometry::normal(avs);
        id_type idFace = mesh.create_face(idSurface, j0, {normal});

        id_type idPartner = idPartners[j]; // different ...
        mesh.halfedge(idPartner).set_partner_id(j0);

        mesh.halfedge(j0) = {mesh, j0, idPartner, j1, j3, j0, idFace};
        idPartner = (j < nAzimuth - 1 ? j3 + 1 : (bClosed ? j3 - nAzimuth + 1 : nullid));
        mesh.halfedge(j1) = {mesh, j1, idPartner, j2, j0, j1, idFace};
        idPartner = nullid;
        mesh.halfedge(j2) = {mesh, j2, idPartner, j3, j1, j2, idFace};
        idPartner = (j > 0 ? j1 - 1 : (bClosed ? j1 + nAzimuth - 1 : nullid));
        mesh.halfedge(j3) = {mesh, j3, idPartner, j0, j2, j3, idFace};

        mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
        mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
        mesh.vertex(j2) = {mesh, j2, j2, avs[2]};
        mesh.vertex(j3) = {mesh, j3, j3, avs[3]};

        ++nh;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_antiband(M& mesh, const vertices_attributes_type<M>& avs1, id_type surface_id)
{
    // create_antiband is leveraged here ...
    using T = data_value_type<M>;

    size_t nAzimuth = avs1.size() - 1;
    assert(nAzimuth > 2);

    size_t nh = mesh.halfedge_store_count();

    vertices_attributes_type<M> avs0(nAzimuth + 1);
    for (size_t i = 0; i < nAzimuth; ++i)
    {
        avs0[i] = mesh.halfedge(nh - nAzimuth + i).vertex_attributes();
    }

    avs0[nAzimuth] = avs0[0];
    avs0[nAzimuth].texcoord().set_x(T(1));

    nh = mesh.halfedge_store_count();

    create_antiband(mesh, avs0, avs1, surface_id);

    for (size_t i = 0; i < nAzimuth; ++i)
    {
        size_t idHalfedge0 = nh - 2 * nAzimuth + i;
        size_t idHalfedge1 = nh + i;

        mesh.halfedge(idHalfedge0).set_partner_id(idHalfedge1);
        mesh.halfedge(idHalfedge1).set_partner_id(idHalfedge0);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_antiband_inverted(M& mesh, const vertices_attributes_type<M>& avs1, id_type surface_id)
{
    // create_antiband is leveraged here ...
    using T = data_value_type<M>;

    size_t nAzimuth = avs1.size() - 1;
    assert(nAzimuth > 2);
    assert(mesh.halfedge_count() >= 6 * nAzimuth);

    size_t nh = mesh.halfedge_store_count();

    vertices_attributes_type<M> avs0(nAzimuth + 1);
    for (size_t i = 0; i < nAzimuth; ++i)
    {
        avs0[i] = mesh.halfedge(nh - nAzimuth + i).vertex_attributes();
    }

    avs0[nAzimuth] = avs0[0];
    avs0[nAzimuth].texcoord().set_x(T(1) + T(0.5) / nAzimuth);

    nh = mesh.halfedge_store_count();

    create_antiband_inverted(mesh, avs0, avs1, surface_id);

    for (size_t i = 0; i < nAzimuth; ++i)
    {
        size_t idHalfedge0 = nh - 2 * nAzimuth + i;
        size_t idHalfedge1 = nh + i;

        mesh.halfedge(idHalfedge0).set_partner_id(idHalfedge1);
        mesh.halfedge(idHalfedge1).set_partner_id(idHalfedge0);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_antiband_faces(M& mesh, const vertices_attributes_type<M>& avs1, id_type submesh_id)
{
    // create_antiband_faces is leveraged here ...
    using T = data_value_type<M>;

    size_t nAzimuth = avs1.size() - 1;
    assert(nAzimuth > 2);
    assert(mesh.halfedge_count() >= 6 * nAzimuth);

    size_t nh = mesh.halfedge_store_count();

    vertices_attributes_type<M> avs0(nAzimuth + 1);
    for (size_t i = 0; i < nAzimuth; ++i)
    {
        avs0[i] = mesh.halfedge(nh - nAzimuth + i).vertex_attributes();
    }

    avs0[nAzimuth] = avs0[0];

    nh = mesh.halfedge_store_count();

    create_antiband_faces(mesh, avs0, avs1, submesh_id);

    for (size_t i = 0; i < nAzimuth; ++i)
    {
        size_t idHalfedge0 = nh - 2 * nAzimuth + i;
        size_t idHalfedge1 = nh + i;

        mesh.halfedge(idHalfedge0).set_partner_id(idHalfedge1);
        mesh.halfedge(idHalfedge1).set_partner_id(idHalfedge0);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_side_flat(M& mesh, size_t na, size_t nb, bool cusped_lower, bool cusped_upper, id_type surface_id, bool bPlanar)
{
    using T = data_value_type<M>;

    size_t nh = mesh.halfedge_store_count();
    size_t nv = mesh.vertex_store_count();

    size_t n = 4 * nb;
    if (cusped_lower)
    {
        --n;
    }

    if (cusped_upper)
    {
        --n;
    }

    mesh.halfedge_store().resize(nh + n);
    mesh.vertex_store().resize(nv + n);

    id_type idPartner0 = nullid;
    id_type idPartner1 = 3 * na;
    id_type idPartner2 = nullid;
    id_type idPartner3 = 2 * na - 1;

    typename M::vector_type normal;

    for (size_t j = 0; j < nb; ++j)
    {
        size_t j0 = nh + 0;
        size_t j1 = nh + 1;
        size_t j2 = nh + 2;
        size_t j3 = nh + 3;

        if (idPartner0 != nullid)
        {
            mesh.halfedge(idPartner0).set_partner_id(j0);
        }

        id_type face_id = mesh.create_face(surface_id, j0);

        if (j == 0 && cusped_lower)
        {
            idPartner1 -= na;
            idPartner3 -= na;

            mesh.halfedge(j0) = {mesh, j0, idPartner1, j1, j2, j0, face_id};
            mesh.halfedge(j1) = {mesh, j1, idPartner2, j2, j0, j1, face_id};
            mesh.halfedge(j2) = {mesh, j2, idPartner3, j0, j1, j2, face_id};

            mesh.halfedge(idPartner1).set_partner_id(j0);
            mesh.halfedge(idPartner3).set_partner_id(j2);

            vertices_attributes_type<M> avs =
            {
                mesh.halfedge(idPartner1).next().vertex_attributes(),
                mesh.halfedge(idPartner1).vertex_attributes(),
                mesh.halfedge(idPartner3).next().vertex_attributes(),
            };

            normal = geometry::normal(avs);
            avs[0].set_normal(normal);
            avs[1].set_normal(normal);
            avs[2].set_normal(normal);

            texture_span_type<M> ts = texture_span<M>(j + 1, nb, cusped_lower, cusped_upper);
            avs[0].set_texcoord({T(0.5), T(1)});
            avs[1].set_texcoord({ts[1], ts[2]});
            avs[2].set_texcoord({ts[0], ts[2]});
            assert(mesh.halfedge(idPartner3).next().vertex_attributes().texcoord().y() == ts[2]);
            assert(mesh.halfedge(idPartner1).vertex_attributes().texcoord().y() == ts[2]);

            mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
            mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
            mesh.vertex(j2) = {mesh, j2, j2, avs[2]};
        }
        else if (j == nb - 1 && cusped_upper)
        {
            idPartner1 -= na;

            mesh.halfedge(j0) = {mesh, j0, idPartner0, j1, j2, j0, face_id};
            mesh.halfedge(j1) = {mesh, j1, idPartner1, j2, j0, j1, face_id};
            mesh.halfedge(j2) = {mesh, j2, idPartner3, j0, j1, j2, face_id};

            mesh.halfedge(idPartner1).set_partner_id(j1);
            mesh.halfedge(idPartner3).set_partner_id(j2);

            vertices_attributes_type<M> avs =
            {
                mesh.halfedge(idPartner3).vertex_attributes(),
                mesh.halfedge(idPartner1).next().vertex_attributes(),
                mesh.halfedge(idPartner3).next().vertex_attributes(),
            };

            if (!bPlanar || normal.zero())
            {
                normal = geometry::normal(avs);
            }

            if (!bPlanar && idPartner0 != nullid)
            {
                avs[0].set_normal(normalize(mesh.halfedge(idPartner0).face_attributes().normal() + normal));
                avs[1].set_normal(avs[0].normal());

                mesh.halfedge(idPartner0).vertex_attributes().set_normal(avs[0].normal());
                mesh.halfedge(idPartner0).next().vertex_attributes().set_normal(avs[0].normal());
            }
            else
            {
                avs[0].set_normal(normal);
                avs[1].set_normal(normal);
            }

            avs[2].set_normal(normal);

            if (idPartner0 != nullid)
            {
                avs[0].set_texcoord(mesh.halfedge(idPartner0).next().vertex_attributes().texcoord());
                avs[1].set_texcoord(mesh.halfedge(idPartner0).vertex_attributes().texcoord());
            }
            else
            {
                avs[0].set_texcoord({T(0), T(1)});
                avs[1].set_texcoord({T(1), T(1)});
            }

            avs[2].set_texcoord({T(0.5), T(0)});

            mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
            mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
            mesh.vertex(j2) = {mesh, j2, j2, avs[2]};
        }
        else
        {
            mesh.halfedge(j0) = {mesh, j0, idPartner0, j1, j3, j0, face_id};
            mesh.halfedge(j1) = {mesh, j1, idPartner1, j2, j0, j1, face_id};
            mesh.halfedge(j2) = {mesh, j2, idPartner2, j3, j1, j2, face_id};
            mesh.halfedge(j3) = {mesh, j3, idPartner3, j0, j2, j3, face_id};

            mesh.halfedge(idPartner1).set_partner_id(j1);
            mesh.halfedge(idPartner3).set_partner_id(j3);

            vertices_attributes_type<M> avs =
            {
                mesh.halfedge(idPartner3).vertex_attributes(),
                mesh.halfedge(idPartner1).next().vertex_attributes(),
                mesh.halfedge(idPartner1).vertex_attributes(),
                mesh.halfedge(idPartner3).next().vertex_attributes(),
            };

            if (!bPlanar || normal.zero())
            {
                normal = geometry::normal(avs);
            }

            // normal here should be calculated like new create_cylinder, botton in the loop, the last top edge at the end, linear propagation ...

            if (!bPlanar && idPartner0 != nullid)
            {
                avs[0].set_normal(normalize(mesh.halfedge(idPartner0).face_attributes().normal() + normal));
                avs[1].set_normal(avs[0].normal());

                mesh.halfedge(idPartner0).vertex_attributes().set_normal(avs[0].normal());
                mesh.halfedge(idPartner0).next().vertex_attributes().set_normal(avs[0].normal());
            }
            else
            {
                avs[0].set_normal(normal);
                avs[1].set_normal(normal);
            }

            avs[2].set_normal(normal);
            avs[3].set_normal(normal);

            std::swap(avs[0].texcoord(), avs[1].texcoord());
            std::swap(avs[2].texcoord(), avs[3].texcoord());

            mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
            mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
            mesh.vertex(j2) = {mesh, j2, j2, avs[2]};
            mesh.vertex(j3) = {mesh, j3, j3, avs[3]};
        }

        mesh.face(face_id).attributes().set_normal(normal);

        idPartner0 = j2;
        idPartner1 += 4 * na;
        idPartner3 += 4 * na;

        nh += 4;

        if (j == 0 && cusped_lower)
        {
            idPartner0 = j1;
            --nh;
        }
    }

    // this is junk ...
    // without this is probably how doing this with sections will work, if so, just remove this ...
    if (!bPlanar)
    {
        size_t nf = mesh.face_count();
        typename M::vector_type sum{};
        for (size_t i = 1; i <= nb; ++i)
        {
            sum += mesh.face(nf - i).attributes().normal();
        }

        normal = normalize(sum);
    }

    // surface normal is just not meaningful if not planar, do away with it entirely if possible ...
    mesh.surface(surface_id).attributes().set_normal(normal);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::connect_side_center(M& mesh, size_t na, size_t nb, bool cusped_lower, bool cusped_upper, id_type surface_id0, id_type surface_id1, std::function<typename M::point_type(const typename M::point_type&)> center_position, bool bPlanar)
{
    using T = data_value_type<M>;

    size_t nh = mesh.halfedge_store_count();
    size_t nv = mesh.vertex_store_count();

    size_t n = 4 * nb;
    if (cusped_lower)
    {
        --n;
    }

    if (cusped_upper)
    {
        --n;
    }

    mesh.halfedge_store().resize(nh + 2 * n);
    mesh.vertex_store().resize(nv + 2 * n);

    size_t nhOrig = nh;

    // Lower

    id_type idPartner0 = nullid;
    id_type idPartner1 = 3 * na;
    id_type idPartner2 = nullid;
    id_type idPartner3 = nullid;

    typename M::vector_type normal;

    for (size_t j = 0; j < nb; ++j)
    {
        size_t j0 = nh + 0;
        size_t j1 = nh + 1;
        size_t j2 = nh + 2;
        size_t j3 = nh + 3;

        if (idPartner0 != nullid)
        {
            mesh.halfedge(idPartner0).set_partner_id(j0);
        }

        id_type face_id = mesh.create_face(surface_id0, j0);

        if (j == 0 && cusped_lower)
        {
            idPartner1 -= na;

            mesh.halfedge(j0) = {mesh, j0, idPartner1, j1, j2, j0, face_id};
            mesh.halfedge(j1) = {mesh, j1, idPartner2, j2, j0, j1, face_id};
            mesh.halfedge(j2) = {mesh, j2, idPartner3, j0, j1, j2, face_id};

            mesh.halfedge(idPartner1).set_partner_id(j0);

            auto av1 = mesh.halfedge(idPartner1).vertex_attributes();
            vertices_attributes_type<M> avs =
            {
                mesh.halfedge(idPartner1).next().vertex_attributes(),
                av1,
                av1,
            };

            avs[2].set_position(center_position(avs[2].position()));

            normal = geometry::normal(avs);
            avs[0].set_normal(normal);
            avs[1].set_normal(normal);
            avs[2].set_normal(normal);

            texture_span_type<M> ts = texture_span<M>(j + 1, nb, cusped_lower, cusped_upper, T(0));
            avs[0].set_texcoord({T(0), T(1)});
            avs[1].set_texcoord({ts[1], ts[2]});
            avs[2].set_texcoord({ts[0], ts[2]});
            assert(mesh.halfedge(idPartner1).vertex_attributes().texcoord().y() == ts[2]);

            mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
            mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
            mesh.vertex(j2) = {mesh, j2, j2, avs[2]};

            --nh;
        }
        else if (j == nb - 1 && cusped_upper)
        {
            idPartner1 -= na;

            mesh.halfedge(j0) = {mesh, j0, idPartner0, j1, j2, j0, face_id};
            mesh.halfedge(j1) = {mesh, j1, idPartner1, j2, j0, j1, face_id};
            mesh.halfedge(j2) = {mesh, j2, idPartner3, j0, j1, j2, face_id};

            mesh.halfedge(idPartner1).set_partner_id(j1);

            auto av0 = mesh.halfedge(idPartner1).next().vertex_attributes();
            vertices_attributes_type<M> avs =
            {
                av0,
                av0,
                mesh.halfedge(idPartner1).vertex_attributes(),
            };

            avs[0].set_position(center_position(avs[0].position()));

            if (!bPlanar || normal.zero())
            {
                normal = geometry::normal(avs);
            }

            if (!bPlanar && idPartner0 != nullid)
            {
                avs[0].set_normal(normalize(mesh.halfedge(idPartner0).face_attributes().normal() + normal));
                avs[1].set_normal(avs[0].normal());

                mesh.halfedge(idPartner0).vertex_attributes().set_normal(avs[0].normal());
                mesh.halfedge(idPartner0).next().vertex_attributes().set_normal(avs[0].normal());
            }
            else
            {
                avs[0].set_normal(normal);
                avs[1].set_normal(normal);
            }

            avs[2].set_normal(normal);

            if (idPartner0 != nullid)
            {
                avs[0].set_texcoord(mesh.halfedge(idPartner0).next().vertex_attributes().texcoord());
                avs[1].set_texcoord(mesh.halfedge(idPartner0).vertex_attributes().texcoord());
            }
            else
            {
                avs[0].set_texcoord({T(0), T(1)});
                avs[1].set_texcoord({T(1), T(1)});
            }

            avs[2].set_texcoord({T(0), T(0)});

            mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
            mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
            mesh.vertex(j2) = {mesh, j2, j2, avs[2]};

            mesh.vertex(j0).set_attributes(avs[0]);
            mesh.vertex(j1).set_attributes(avs[1]);
            mesh.vertex(j2).set_attributes(avs[2]);

            --nh;
        }
        else
        {
            mesh.halfedge(j0) = {mesh, j0, idPartner0, j1, j3, j0, face_id};
            mesh.halfedge(j1) = {mesh, j1, idPartner1, j2, j0, j1, face_id};
            mesh.halfedge(j2) = {mesh, j2, idPartner2, j3, j1, j2, face_id};
            mesh.halfedge(j3) = {mesh, j3, idPartner3, j0, j2, j3, face_id};

            mesh.halfedge(idPartner1).set_partner_id(j1);

            auto av0 = mesh.halfedge(idPartner1).next().vertex_attributes();
            auto av2 = mesh.halfedge(idPartner1).vertex_attributes();
            vertices_attributes_type<M> avs =
            {
                av0,
                av0,
                av2,
                av2,
            };

            avs[0].set_position(center_position(avs[0].position()));
            avs[3].set_position(center_position(avs[3].position()));

            if (!bPlanar || normal.zero())
            {
                normal = geometry::normal(avs);
            }

            if (!bPlanar && idPartner0 != nullid)
            {
                avs[0].set_normal(normalize(mesh.halfedge(idPartner0).face_attributes().normal() + normal));
                avs[1].set_normal(avs[0].normal());

                mesh.halfedge(idPartner0).vertex_attributes().set_normal(avs[0].normal());
                mesh.halfedge(idPartner0).next().vertex_attributes().set_normal(avs[0].normal());
            }
            else
            {
                avs[0].set_normal(normal);
                avs[1].set_normal(normal);
            }

            avs[2].set_normal(normal);
            avs[3].set_normal(normal);

            if (idPartner0 != nullid)
            {
                avs[0].set_texcoord(mesh.halfedge(idPartner0).next().vertex_attributes().texcoord());
                avs[1].set_texcoord(mesh.halfedge(idPartner0).vertex_attributes().texcoord());
            }
            else
            {
                avs[0].set_texcoord({T(0), T(1)});
                avs[1].set_texcoord({T(1), T(1)});
            }

            texture_span_type<M> ts = texture_span<M>(j + 1, nb, cusped_lower, cusped_upper, T(0));
            avs[2].set_texcoord({ts[1], ts[2]});
            avs[3].set_texcoord({ts[0], ts[2]});
            assert(mesh.halfedge(idPartner1).vertex_attributes().texcoord().y() == ts[2]);

            mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
            mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
            mesh.vertex(j2) = {mesh, j2, j2, avs[2]};
            mesh.vertex(j3) = {mesh, j3, j3, avs[3]};
        }

        mesh.face(face_id).attributes().set_normal(normal);

        idPartner0 = j2;
        idPartner1 += 4 * na;

        nh += 4;

        if (j == 0 && cusped_lower)
        {
            idPartner0 = j1;
        }
    }

    if (!bPlanar)
    {
        size_t nf = mesh.face_count();
        typename M::vector_type sum{};
        for (size_t i = 1; i <= nb; ++i)
        {
            sum += mesh.face(nf - i).attributes().normal();
        }

        normal = normalize(sum);
    }

    // surface normal is not meaningful if not planar ...
    mesh.surface(surface_id0).attributes().set_normal(normal);

    // Upper

    idPartner0 = nullid;
    idPartner1 = nhOrig + 3;
    idPartner2 = nullid;
    idPartner3 = 2 * na - 1;

    normal.clear();

    for (size_t j = 0; j < nb; ++j)
    {
        size_t j0 = nh + 0;
        size_t j1 = nh + 1;
        size_t j2 = nh + 2;
        size_t j3 = nh + 3;

        if (idPartner0 != nullid)
        {
            mesh.halfedge(idPartner0).set_partner_id(j0);
        }

        id_type face_id = mesh.create_face(surface_id1, j0);

        if (j == 0 && cusped_lower)
        {
            idPartner1 -= 1;
            idPartner3 -= na;

            mesh.halfedge(j0) = {mesh, j0, idPartner1, j1, j2, j0, face_id};
            mesh.halfedge(j1) = {mesh, j1, idPartner2, j2, j0, j1, face_id};
            mesh.halfedge(j2) = {mesh, j2, idPartner3, j0, j1, j2, face_id};

            mesh.halfedge(idPartner1).set_partner_id(j0);
            mesh.halfedge(idPartner3).set_partner_id(j2);

            vertices_attributes_type<M> avs =
            {
                mesh.halfedge(idPartner1).next().vertex_attributes(),
                mesh.halfedge(idPartner1).vertex_attributes(),
                mesh.halfedge(idPartner3).next().vertex_attributes(),
            };

            avs[1].set_position(center_position(avs[1].position()));

            normal = geometry::normal(avs);
            avs[0].set_normal(normal);
            avs[1].set_normal(normal);
            avs[2].set_normal(normal);

            texture_span_type<M> ts = texture_span<M>(j + 1, nb, cusped_lower, cusped_upper, T(1));
            avs[0].set_texcoord({T(1), T(1)});
            avs[1].set_texcoord({ts[1], ts[2]});
            avs[2].set_texcoord({ts[0], ts[2]});
            assert(mesh.halfedge(idPartner3).next().vertex_attributes().texcoord().y() == ts[2]);
            assert(mesh.halfedge(idPartner1).vertex_attributes().texcoord().y() == ts[2]);

            mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
            mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
            mesh.vertex(j2) = {mesh, j2, j2, avs[2]};

            --nh;
        }
        else if (j == nb - 1 && cusped_upper)
        {
            idPartner1 -= 1;

            mesh.halfedge(j0) = {mesh, j0, idPartner0, j1, j2, j0, face_id};
            mesh.halfedge(j1) = {mesh, j1, idPartner1, j2, j0, j1, face_id};
            mesh.halfedge(j2) = {mesh, j2, idPartner3, j0, j1, j2, face_id};

            mesh.halfedge(idPartner1).set_partner_id(j1);
            mesh.halfedge(idPartner3).set_partner_id(j2);

            vertices_attributes_type<M> avs =
            {
                mesh.halfedge(idPartner3).vertex_attributes(),
                mesh.halfedge(idPartner1).next().vertex_attributes(),
                mesh.halfedge(idPartner3).next().vertex_attributes(),
               };

            avs[1].set_position(center_position(avs[1].position()));



            if (!bPlanar || normal.zero())
            {
                normal = geometry::normal(avs);
            }

            if (!bPlanar && idPartner0 != nullid)
            {
                avs[0].set_normal(normalize(mesh.halfedge(idPartner0).face_attributes().normal() + normal));
                avs[1].set_normal(avs[0].normal());

                mesh.halfedge(idPartner0).vertex_attributes().set_normal(avs[0].normal());
                mesh.halfedge(idPartner0).next().vertex_attributes().set_normal(avs[0].normal());
            }
            else
            {
                avs[0].set_normal(normal);
                avs[1].set_normal(normal);
            }

            avs[2].set_normal(normal);

            if (idPartner0 != nullid)
            {
                avs[0].set_texcoord(mesh.halfedge(idPartner0).next().vertex_attributes().texcoord());
                avs[1].set_texcoord(mesh.halfedge(idPartner0).vertex_attributes().texcoord());
            }
            else
            {
                avs[0].set_texcoord({T(0), T(1)});
                avs[1].set_texcoord({T(1), T(1)});
            }

            avs[2].set_texcoord({T(1), T(0)});

            mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
            mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
            mesh.vertex(j2) = {mesh, j2, j2, avs[2]};

            --nh;
        }
        else
        {
            mesh.halfedge(j0) = {mesh, j0, idPartner0, j1, j3, j0, face_id};
            mesh.halfedge(j1) = {mesh, j1, idPartner1, j2, j0, j1, face_id};
            mesh.halfedge(j2) = {mesh, j2, idPartner2, j3, j1, j2, face_id};
            mesh.halfedge(j3) = {mesh, j3, idPartner3, j0, j2, j3, face_id};

            mesh.halfedge(idPartner1).set_partner_id(j1);
            mesh.halfedge(idPartner3).set_partner_id(j3);

            vertices_attributes_type<M> avs =
            {
                mesh.halfedge(idPartner3).vertex_attributes(),
                mesh.halfedge(idPartner1).next().vertex_attributes(),
                mesh.halfedge(idPartner1).vertex_attributes(),
                mesh.halfedge(idPartner3).next().vertex_attributes(),
            };

            if (!bPlanar || normal.zero())
            {
                normal = geometry::normal(avs);
            }

            if (!bPlanar && idPartner0 != nullid)
            {
                avs[0].set_normal(normalize(mesh.halfedge(idPartner0).face_attributes().normal() + normal));
                avs[1].set_normal(avs[0].normal());

                mesh.halfedge(idPartner0).vertex_attributes().set_normal(avs[0].normal());
                mesh.halfedge(idPartner0).next().vertex_attributes().set_normal(avs[0].normal());
            }
            else
            {
                avs[0].set_normal(normal);
                avs[1].set_normal(normal);
            }

            avs[2].set_normal(normal);
            avs[3].set_normal(normal);

            if (idPartner0 != nullid)
            {
                avs[0].set_texcoord(mesh.halfedge(idPartner0).next().vertex_attributes().texcoord());
                avs[1].set_texcoord(mesh.halfedge(idPartner0).vertex_attributes().texcoord());
            }
            else
            {
                avs[0].set_texcoord({T(0), T(1)});
                avs[1].set_texcoord({T(1), T(1)});
            }

            texture_span_type<M> ts = texture_span<M>(j + 1, nb, cusped_lower, cusped_upper, T(1));
            avs[2].set_texcoord({ts[1], ts[2]});
            avs[3].set_texcoord({ts[0], ts[2]});
            assert(mesh.halfedge(idPartner3).next().vertex_attributes().texcoord().y() == ts[2]);
            assert(mesh.halfedge(idPartner1).vertex_attributes().texcoord().y() == ts[2]);

            mesh.vertex(j0) = {mesh, j0, j0, avs[0]};
            mesh.vertex(j1) = {mesh, j1, j1, avs[1]};
            mesh.vertex(j2) = {mesh, j2, j2, avs[2]};
            mesh.vertex(j3) = {mesh, j3, j3, avs[3]};
        }

        mesh.face(face_id).attributes().set_normal(normal);

        idPartner0 = j2;
        idPartner1 += 4;
        idPartner3 += 4 * na;

        nh += 4;

        if (j == 0 && cusped_lower)
        {
            idPartner0 = j1;
        }
    }

    if (!bPlanar)
    {
        size_t nf = mesh.face_count();
        typename M::vector_type sum{};
        for (size_t i = 1; i <= nb; ++i)
        {
            sum += mesh.face(nf - i).attributes().normal();
        }

        normal = normalize(sum);
    }

    // surface normal is not meaningful if not planar ...
    mesh.surface(surface_id1).attributes().set_normal(normal);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::close_side_flat(M& mesh, size_t na, size_t nb, bool cusped_lower, bool cusped_upper, id_type surface_id, bool bPlanar)
{
    assert(bPlanar);

    using T = data_value_type<M>;

    size_t nh = mesh.halfedge_store_count();
    size_t nv = mesh.vertex_store_count();

    id_type idPartnerLower0 = cusped_lower ? 2 * na : 3 * na;
    id_type idPartnerLower1 = cusped_lower ? na - 1 : 2 * na - 1;
    id_type idPartnerUpper0 = nh - na;
    id_type idPartnerUpper1 = cusped_upper ? nh - na - 1 : nh - 2 * na - 1;

    id_type idUpper = nullid;

    id_type idPrev0 = nh;
    id_type idNext1 = nh;

    id_type idMid0 = idPartnerLower0;
    id_type idMid1 = idPartnerLower1 - (cusped_lower ? na : 0);
    idMid0 += (nb / 2) * 4 * na;
    idMid1 += (nb / 2) * 4 * na;

    auto positionMid0 = mesh.halfedge(idMid0).next().vertex_attributes().position();
    auto positionMid1 = mesh.halfedge(idMid1).vertex_attributes().position();
    auto positionUpper0 = mesh.halfedge(idPartnerUpper0).vertex_attributes().position();
    auto normal = normalize(cross(positionMid1 - positionUpper0, positionMid0 - positionUpper0));
// can normal take a span of points? ...
//    auto normal = geometry::normal({positionMid1, positionMid0, positionUpper0});

    id_type face_id = mesh.create_face(surface_id, nh, {normal});

    if (cusped_lower)
    {
        idPrev0 = nh + 1;
        idNext1 = nh;
    }
    else
    {
        typename M::vertex_attributes_type av = {mesh.halfedge(idPartnerLower1).vertex_attributes().position(), normal, {T(0), T(1)}}; // 0.5, 1 ...
        mesh.create_halfedge_vertex(nullid, nh + 1, nh + 2, face_id, av);

        ++nh;
        ++nv;
    }

    if (!cusped_upper)
    {
        typename M::vertex_attributes_type av = {mesh.halfedge(idPartnerUpper0).vertex_attributes().position(), normal, {T(1), T(0)}}; // 0.5, 0 ...
        idUpper = mesh.create_halfedge_vertex(nullid, nh + 2 * nb - 1, nh + 2 * nb - 2, face_id, av);

        ++nh;
        ++nv;
    }

    id_type idPartner0 = idPartnerLower0;
    id_type idPartner1 = idPartnerLower1;

    for (size_t j = 0; j < nb; ++j)
    {
        id_type idNext0 = nh + 2;
        id_type idPrev1 = nh + 3;
        if (j == nb - 1)
        {
            if (cusped_upper)
            {
                idPartner0 -= na;
            }

            assert(idPartner0 == idPartnerUpper0);
            assert(idPartner1 == idPartnerUpper1);

            idNext0 = cusped_upper ? nh + 1 : idUpper;
            idPrev1 = cusped_upper ? nh : idUpper;
        }

        idPrev0 = mesh.create_halfedge(idPartner0, idNext0, idPrev0, nv, face_id);
        idNext1 = mesh.create_halfedge(idPartner1, idNext1, idPrev1, nv + 1, face_id);

        mesh.halfedge(idPartner0).set_partner_id(nh);
        mesh.halfedge(idPartner1).set_partner_id(nh + 1);

        auto av0 = mesh.halfedge(idPartner1).next().vertex_attributes();
        auto av1 = mesh.halfedge(idPartner0).next().vertex_attributes();

        av0.set_normal(normal);
        av1.set_normal(normal);

        texture_span_type<M> ts = texture_span<M>(j, nb, cusped_lower, cusped_upper);
        av0.set_texcoord({ts[1], ts[2]});
        assert(mesh.halfedge(idPartner0).next().vertex_attributes().texcoord().y() == ts[2]);
        
        ts = texture_span<M>(j + 1, nb, cusped_lower, cusped_upper);
        av1.set_texcoord({ts[0], ts[2]});
        assert(mesh.halfedge(idPartner1).next().vertex_attributes().texcoord().y() == ts[2]);

        mesh.create_vertex(nh, av0);
        mesh.create_vertex(nh + 1, av1);

        idPartner0 += 4 * na;
        idPartner1 += 4 * na;

        nh += 2;
        nv += 2;
    }

    mesh.surface(surface_id).attributes().set_normal(normal);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::close_side_center(M& mesh, size_t na, size_t nb, bool cusped_lower, bool cusped_upper, id_type surface_id0, id_type surface_id1, std::function<typename M::point_type(const typename M::point_type&)> center_position, bool bPlanar)
{
    assert(bPlanar);

    using T = data_value_type<M>;

    size_t nh = mesh.halfedge_store_count();

    id_type idPartnerLower0 = cusped_lower ? 2 * na : 3 * na;
    id_type idPartnerLower1 = cusped_lower ? na - 1 : 2 * na - 1;
    id_type idPartnerUpper0 = nh - na;
    id_type idPartnerUpper1 = cusped_upper ? nh - na - 1 : nh - 2 * na - 1;

    id_type idMid0 = idPartnerLower0;
    id_type idMid1 = idPartnerLower1;
    idMid0 += (nb / 2) * 4 * na;
    idMid1 += (nb / 2) * 4 * na;

    auto positionMid0 = mesh.halfedge(idMid0).next().vertex_attributes().position();
    auto positionMid1 = mesh.halfedge(idMid1).vertex_attributes().position();

    auto positionCenterUpper = center_position(mesh.halfedge(idPartnerUpper0).vertex_attributes().position());
    auto positionCenterLower = center_position(mesh.halfedge(idPartnerLower1).vertex_attributes().position());

    // geometry::normal with span ...
    auto normal0 = normalize(cross(positionCenterUpper - positionMid0, positionCenterLower - positionMid0));
    auto normal1 = normalize(cross(positionCenterLower - positionMid1, positionCenterUpper - positionMid1));

    typename M::vertex_attributes_type av0 = {positionCenterUpper, normal0, {T(0), T(0)}};
    typename M::vertex_attributes_type av1 = {positionCenterLower, normal1, {T(1), T(1)}};

    id_type idFace0 = mesh.create_face(surface_id0, nh, {normal0});
    id_type idFace1 = mesh.create_face(surface_id1, nh + 1, {normal1});

    id_type idCenter0 = mesh.create_halfedge_vertex(nh + 1, nh + 2, nullid, idFace0, av0);
    id_type idCenter1 = mesh.create_halfedge_vertex(nh, nullid, nh + 3, idFace1, av1);

    nh += 2;

    id_type idPrev0 = idCenter0;
    id_type idNext1 = idCenter1;

    if (!cusped_lower)
    {
        av0 = {positionCenterLower, normal0, {T(0), T(1)}};
        idPrev0 = mesh.create_halfedge_vertex(nullid, nh + 2, idPrev0, idFace0, av0);

        av1 = {mesh.halfedge(idPartnerLower1).vertex_attributes().position(), normal1, {T(0), T(1)}};
        idNext1 = mesh.create_halfedge_vertex(nullid, idNext1, nh + 3, idFace1, av1);

        nh += 2;
    }

    id_type idPartner0 = idPartnerLower0;
    id_type idPartner1 = idPartnerLower1;

    for (size_t j = 0; j < nb; ++j)
    {
        id_type idNext0 = nh + 2;
        id_type idPrev1 = nh + 3;
        if (j == nb - 1)
        {
            if (cusped_upper)
            {
                idPartner0 -= na;
            }

            assert(idPartner0 == idPartnerUpper0);
            assert(idPartner1 == idPartnerUpper1);

            idNext0 = idCenter0;
            idPrev1 = idCenter1;
        }

        av0 = mesh.halfedge(idPartner0).next().vertex_attributes();
        av1 = mesh.halfedge(idPartner1).next().vertex_attributes();

        av0.set_normal(normal0);
        av1.set_normal(normal1);

        texture_span_type<M> ts = texture_span<M>(j, nb, cusped_lower, cusped_upper, T(0));
        av0.set_texcoord({ts[1], ts[2]});
        assert(mesh.halfedge(idPartner0).next().vertex_attributes().texcoord().y() == ts[2]);
        
        ts = texture_span<M>(j + 1, nb, cusped_lower, cusped_upper, T(1));
        av1.set_texcoord({ts[0], ts[2]});
        assert(mesh.halfedge(idPartner1).next().vertex_attributes().texcoord().y() == ts[2]);

        idPrev0 = mesh.create_halfedge_vertex(idPartner0, idNext0, idPrev0, idFace0, av0);
        idNext1 = mesh.create_halfedge_vertex(idPartner1, idNext1, idPrev1, idFace1, av1);

        mesh.halfedge(idPartner0).set_partner_id(idPrev0);
        mesh.halfedge(idPartner1).set_partner_id(idNext1);

        idPartner0 += 4 * na;
        idPartner1 += 4 * na;

        nh += 2;
    }

    if (!cusped_upper)
    {
        av0 = {mesh.halfedge(idPartnerUpper0).vertex_attributes().position(), normal0, {T(1), T(0)}};
        id_type idUpper0 = mesh.create_halfedge_vertex(nullid, idCenter0, idPrev0, idFace0, av0);

        av1 = {positionCenterUpper, normal1, {T(1), T(0)}};
        id_type idUpper1 = mesh.create_halfedge_vertex(nullid, idNext1, idCenter1, idFace1, av1);

        mesh.halfedge(idPrev0).set_next_id(idUpper0);
        mesh.halfedge(idNext1).set_prev_id(idUpper1);

        mesh.halfedge(idCenter0).set_prev_id(idUpper0);
        mesh.halfedge(idCenter1).set_next_id(idUpper1);

        nh += 2;
    }
    else
    {
        mesh.halfedge(idCenter0).set_prev_id(idPrev0);
        mesh.halfedge(idCenter1).set_next_id(idNext1);
    }

    mesh.surface(surface_id0).attributes().set_normal(normal0);
    mesh.surface(surface_id1).attributes().set_normal(normal1);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::seal_cylinder(M& mesh, id_type submesh_id, const CylinderSpecification<data_value_type<M>>& spec, id_type halfedge_id0, bool cusped_lower, bool cusped_upper, normal_vector_type<M> normal_lower, normal_vector_type<M> normal_upper)
{
    using T = data_value_type<M>;

    size_t nAzimuth = spec.azimuth.edge_count;
    size_t nz = spec.axis_edge_count;

    // Save halfedge id's before potential side closure
    id_type idLowerFirst = halfedge_id0;
    id_type idUpperFirst = mesh.halfedge_store_count() - 2 * nAzimuth; // should have halfedge_id1 too, but should be fine in the current use case ...

    bool bOpenSide = !spec.azimuth.interval.unit();
    if (bOpenSide)
    {
        if (spec.azimuth.termination.style == TerminationStyle::Butt)
        {
            id_type surface_id = mesh.create_surface(submesh_id, spec.azimuth.termination.names[0]);
            if (spec.azimuth.termination.subdivision)
            {
                connect_side_flat(mesh, nAzimuth, nz, cusped_lower, cusped_upper, surface_id, true);
            }
            else
            {
                close_side_flat(mesh, nAzimuth, nz, cusped_lower, cusped_upper, surface_id, true);
            }
            bOpenSide = false;
        }
        else if (spec.azimuth.termination.style == TerminationStyle::Center)
        {
            id_type surface_id0 = mesh.create_surface(submesh_id, spec.azimuth.termination.names[0]);
            id_type surface_id1 = mesh.create_surface(submesh_id, spec.azimuth.termination.names[1]);
            if (spec.azimuth.termination.subdivision)
            {
                connect_side_center(mesh, nAzimuth, nz, cusped_lower, cusped_upper, surface_id0, surface_id1,
                        [](const typename M::point_type& point) -> typename M::point_type { return {T(0), T(0), point.z()}; },
                        true);
            }
            else
            {
                close_side_center(mesh, nAzimuth, nz, cusped_lower, cusped_upper, surface_id0, surface_id1,
                        [](const typename M::point_type& point) -> typename M::point_type { return {T(0), T(0), point.z()}; },
                        true);
            }
            bOpenSide = false;
        }
    }

    if (!cusped_lower)
    {
        if (spec.axis_termination_lower.style == TerminationStyle::Butt)
        {
            id_type surface_id = mesh.create_surface(submesh_id, spec.axis_termination_lower.names[0], {normal_lower});
            id_type face_id = nullid;
            if (!bOpenSide)
            {
                face_id = create_border_face(mesh, idLowerFirst, normal_lower, surface_id);
            }
            else
            {
                face_id = create_border_face(mesh, idLowerFirst + nAzimuth - 1, idLowerFirst, normal_lower, surface_id);
            }

            calculate_face_texcoords(mesh, face_id);
        }
    }

    if (!cusped_upper)
    {
        if (spec.axis_termination_upper.style == TerminationStyle::Butt)
        {
            id_type surface_id = mesh.create_surface(submesh_id, spec.axis_termination_upper.names[0], {normal_upper});
            id_type face_id = nullid;
            if (!bOpenSide)
            {
                face_id = create_border_face(mesh, idUpperFirst, normal_upper, surface_id);
            }
            else
            {
                face_id = create_border_face(mesh, idUpperFirst, idUpperFirst + nAzimuth - 1, normal_upper, surface_id);
            }

            calculate_face_texcoords(mesh, face_id);
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::seal_sphere(M& mesh, id_type submesh_id, const SphereSpecification<data_value_type<M>>& spec, id_type halfedge_id0, bool cusped_lower, bool cusped_upper)
{
    using T = data_value_type<M>;

    size_t nAzimuth = spec.azimuth.edge_count;
    size_t nElevation = spec.elevation_edge_count;

    // Save halfedge id's before potential side closure
    id_type idLowerFirst = halfedge_id0;
    id_type idUpperFirst = mesh.halfedge_store_count() - 2 * nAzimuth; // should have halfedge_id1 too, but should be fine in the current use case ...

    bool bOpenSide = !spec.azimuth.interval.unit();
    if (bOpenSide) // Elevation TerminationStyle::Center will affect side generation ...
    {
        if (spec.azimuth.termination.style == TerminationStyle::Butt)
        {
            id_type surface_id = mesh.create_surface(submesh_id, spec.azimuth.termination.names[0]);
            bool bPlanar = math::float_eq(spec.azimuth.interval.length(), T(0.5)); // don't think this is sufficient ...
            connect_side_flat(mesh, nAzimuth, nElevation, cusped_lower, cusped_upper, surface_id, bPlanar);
            if (bPlanar)
            {
                calculate_planar_surface_texcoords(mesh, surface_id); // really just need x ...
            }
            else
            {
                calculate_surface_texcoords(mesh, surface_id); // really just need x ...
            }
            bOpenSide = false;
        }
        else if (spec.azimuth.termination.style == TerminationStyle::Center)
        {
            id_type surface_id0 = mesh.create_surface(submesh_id, spec.azimuth.termination.names[0]);
            id_type surface_id1 = mesh.create_surface(submesh_id, spec.azimuth.termination.names[1]);
            if (spec.azimuth.termination.subdivision)
            {
                connect_side_center(mesh, nAzimuth, nElevation, cusped_lower, cusped_upper, surface_id0, surface_id1,
                        [](const typename M::point_type& point) -> typename M::point_type { return {T(0), T(0), point.z()}; },
                        true);
            }
            else
            {
                close_side_center(mesh, nAzimuth, nElevation, cusped_lower, cusped_upper, surface_id0, surface_id1,
                        [](const typename M::point_type& point) -> typename M::point_type { return {T(0), T(0), point.z()}; },
                        true);
            }
            calculate_planar_surface_texcoords(mesh, surface_id0); // really just need right x ...
            calculate_planar_surface_texcoords(mesh, surface_id1); // really just need left x ...
            bOpenSide = false;
        }
    }

    if (!cusped_lower)
    {
        if (spec.elevation_termination_lower.style == TerminationStyle::Butt)
        {
            typename M::vector_type normal {T(0), T(0), T(-1)};
            id_type surface_id = mesh.create_surface(submesh_id, spec.elevation_termination_lower.names[0], {normal});
            id_type face_id = nullid;
            if (!bOpenSide)
            {
                face_id = create_border_face(mesh, idLowerFirst, normal, surface_id);
            }
            else
            {
                face_id = create_border_face(mesh, idLowerFirst + nAzimuth - 1, idLowerFirst, normal, surface_id);
            }

            calculate_face_texcoords(mesh, face_id);
        }
        else if (spec.elevation_termination_lower.style == TerminationStyle::Center)
        {
            assert(false); // TerminationStyle::Center ...
        }
    }

    if (!cusped_upper)
    {
        if (spec.elevation_termination_upper.style == TerminationStyle::Butt)
        {
            typename M::vector_type normal = M::vector_type::z_direction();
            id_type surface_id = mesh.create_surface(submesh_id, spec.elevation_termination_upper.names[0], {normal});
            id_type face_id = nullid;
            if (!bOpenSide)
            {
                face_id = create_border_face(mesh, idUpperFirst, normal, surface_id);
            }
            else
            {
                face_id = create_border_face(mesh, idUpperFirst, idUpperFirst + nAzimuth - 1, normal, surface_id);
            }

            calculate_face_texcoords(mesh, face_id);
        }
        else if (spec.elevation_termination_upper.style == TerminationStyle::Center)
        {
            assert(false); // TerminationStyle::Center ...
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::seal_torus(M& mesh, id_type submesh_id, const TorusSpecification<data_value_type<M>>& spec, id_type halfedge_id0, bool cusped_lower, bool cusped_upper, data_value_type<M> rMajor)
{
    using T = data_value_type<M>;

    T theta0 = math::PiTwo<T> * spec.major_azimuth_interval.lower();
    T theta1 = math::PiTwo<T> * spec.major_azimuth_interval.upper();
    size_t nMajor = spec.major_edge_count;
    size_t nMinor = spec.minor.edge_count;

    // why not rMajor from spec? ...
    //data_value_type<M> rMajor = spec.major_radius_interval ... assert constant? ...

    // Save halfedge id's before potential side closure
    id_type idLowerFirst = halfedge_id0;
    id_type idUpperFirst = mesh.halfedge_store_count() - 2 * nMinor; // should have halfedge_id1 too, but should be fine in the current use case ...
    size_t idSideFirst = mesh.halfedge_store_count();

    bool bOpenMajor = !spec.major_azimuth_interval.unit();
    bool bOpenMinor = !spec.minor.interval.unit();
    if (bOpenMinor)
    {
        if (spec.minor.termination.style == TerminationStyle::Butt)
        {
            id_type idSurfaceSide = mesh.create_surface(submesh_id, spec.minor.termination.names[0]);
T sum = spec.minor.interval.lower() + spec.minor.interval.upper();
sum -= floor(sum);
//auto sum_not = spec.minor.interval.length();
//assert(math::float_eq(sum, spec.minor.interval.length()));
bool bPlanar = math::float_eq(sum, T(0.5)); // abs? ...
// only planar with horizontal slices ...
//            bool bPlanar = math::float_eq(spec.minor.interval.length(), T(0.5)); // abs? ...
            connect_side_flat(mesh, nMinor, nMajor, cusped_lower, cusped_upper, idSurfaceSide, bPlanar);
            bOpenMinor = false;

            if (!bOpenMajor)
            {
                id_type id0 = idSideFirst;
                id_type id2 = idSideFirst + 4 * nMajor - 2;
                mesh.halfedge(id0).set_partner_id(id2);
                mesh.halfedge(id2).set_partner_id(id0);
//                surface_normal(mesh.vertex(id0)); // ...
            }
        }
        else if (spec.minor.termination.style == TerminationStyle::Center)
        {
            id_type idSurfaceSideLower = mesh.create_surface(submesh_id, spec.minor.termination.names[0]);
            id_type idSurfaceSideUpper = mesh.create_surface(submesh_id, spec.minor.termination.names[1]);
            connect_side_center(mesh, nMinor, nMajor, cusped_lower, cusped_upper, idSurfaceSideLower, idSurfaceSideUpper,
                    [rMajor](typename M::point_type point) -> typename M::point_type
                    {
                        point.set_z(T(0)); // z interval? ...
                        return direction(point) * rMajor;
                    },
                    false);
            bOpenMinor = false;

            if (!bOpenMajor)
            {
                id_type id0 = idSideFirst;
                id_type id2 = idSideFirst + 4 * nMajor - 2;
                mesh.halfedge(id0).set_partner_id(id2);
                mesh.halfedge(id2).set_partner_id(id0);
//                surface_normal(mesh.vertex(id0)); // ...

                idSideFirst += 4 * nMajor;
                id0 = idSideFirst;
                id2 = idSideFirst + 4 * nMajor - 2;
                mesh.halfedge(id0).set_partner_id(id2);
                mesh.halfedge(id2).set_partner_id(id0);
//                surface_normal(mesh.vertex(id0)); // ...
            }
        }
    }

    if (!bOpenMajor)
    {
        // Connect ends

        for (size_t j = 0; j < nMinor; ++j)
        {
            id_type j0 = idLowerFirst + j;
            id_type j2 = idUpperFirst + j;
            assert(mesh.halfedge(j0).border());
            assert(mesh.halfedge(j2).border());

            mesh.halfedge(j0).set_partner_id(j2);
            mesh.halfedge(j2).set_partner_id(j0);
//            surface_normal(mesh.vertex(j0)); // ...
        }
    }
    else
    {
        if (!cusped_lower)
        {
            if (spec.major_termination_lower.style == TerminationStyle::Butt)
            {
                typename M::vector_type normal {math::denoise011(sin(theta0)), -math::denoise011(cos(theta0)), T(0)};
                id_type surface_id = mesh.create_surface(submesh_id, spec.major_termination_lower.names[0], {normal});
                id_type face_id = nullid;
                if (!bOpenMinor)
                {
                    face_id = create_border_face(mesh, idLowerFirst, normal, surface_id);
                }
                else
                {
                    face_id = create_border_face(mesh, idLowerFirst + nMinor - 1, idLowerFirst, normal, surface_id);
                }

                calculate_face_texcoords(mesh, face_id);
            }
        }

        if (!cusped_upper)
        {
            if (spec.major_termination_upper.style == TerminationStyle::Butt)
            {
                typename M::vector_type normal = {-math::denoise011(sin(theta1)), math::denoise011(cos(theta1)), T(0)};
                normal.denoise0();
                id_type surface_id = mesh.create_surface(submesh_id, spec.major_termination_upper.names[0], {normal});
                id_type face_id = nullid;
                if (!bOpenMinor)
                {
                    face_id = create_border_face(mesh, idUpperFirst, normal, surface_id);
                }
                else
                {
                    face_id = create_border_face(mesh, idUpperFirst, idUpperFirst + nMinor - 1, normal, surface_id);
                }

                calculate_face_texcoords(mesh, face_id);
            }
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::triangulate_rows(M& mesh, id_type idHalfedge, size_t n)
{
    if (mesh.halfedge(idHalfedge).face().halfedge_count() == 3)
    {
        assert(n == 1);
        return;
    }

    assert(n > 1);

    // Ensure that halfedge is at the right side of a bottom row trapezoid
    while (colinear(mesh.halfedge(idHalfedge)))
    {
        idHalfedge = mesh.halfedge(idHalfedge).next_id();
    }

    const id_type idNext = mesh.halfedge(idHalfedge).next_id();
    id_type idOpposite = idHalfedge;
    for (size_t i = 0; i <= n; ++i)
    {
        idOpposite = mesh.halfedge(idOpposite).prev_id();
    }

    split_face(mesh, idNext, idOpposite);
    split_edge(mesh, mesh.halfedge(idHalfedge).next_id(), n - 1);

    // Triangulate trapezoid
    for (size_t i = 0; i < n - 1; ++i)
    {
        split_face(mesh, mesh.halfedge(idHalfedge).next_id(), mesh.halfedge(idHalfedge).prev_id());
        split_face(mesh, mesh.halfedge(idHalfedge).next().partner().next().next_id(), mesh.halfedge(idHalfedge).next().partner_id());
        idHalfedge = mesh.halfedge(idHalfedge).next().partner().prev().partner_id();
    }

    triangulate_rows(mesh, idNext, n - 1);
    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::project_vertices(U& u, typename U::value_type radius)
{
    using T = typename U::value_type;
    using vertex_attributes_type = U::traits_type::vertex_attributes_type;
    using texcoord_type = vertex_attributes_type::texcoord_type;

    u.reset_all();

    vertex_attributes_type av;
    for_each_surface_vertex(u,
        [&av, radius](const auto& vertex) -> bool
        {
            auto normal = direction(vertex.attributes().position());
            av.set_position(normal * radius);
            av.set_normal(normal);

            // Derive texture coordinates from spherical coordinates of position
            T phi = azimuth_angle(av.position());
            if (errno == EDOM) // Both x and y are 0
            {
                phi = math::Pi<T>;
            }
            if (phi < T(0))
            {
                phi += math::PiTwo<T>;
            }

            // handle seam crossover values, wraparound uses u > 1  ...

            // theta [0, Pi] -> texcoord [0, 1]
            T theta = acos(math::clamp11(av.position().z() / radius));
            av.set_texcoord(denoise01(texcoord_type({phi / math::PiTwo<T>, theta / math::Pi<T>})));
            return true;
        },
        [&av](auto& vertex) -> void
        {
            vertex.set_attributes(av);
            return;
        }
    );

    return;
}

//--------------------------------------------------------------------------
std::string quetzal::model::surface_name(const std::string& base, size_t n)
{
    return std::format("{}{}", base, n);
}

//--------------------------------------------------------------------------
template<typename M>
std::string quetzal::model::next_surface_name(M& mesh, const std::string& base_name)
{
    return surface_name(base_name, mesh.surface_store_count());
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::create_facet(M& mesh, typename M::face_type& face)
{
    auto normal = geometry::normal(face);
    face.attributes().set_normal(normal);

    if (face.surface().face_count() > 1)
    {
        id_type submesh_id = face.submesh_id();
        assert(submesh_id != nullid);
        id_type surface_id = mesh.create_surface(submesh_id, next_surface_name(mesh), {normal});
        mesh.unlink_face(face.id());
        mesh.link(face.id(), surface_id, submesh_id);
    }

    for (auto& halfedge : face.halfedges())
    {
        auto& av = halfedge.vertex_attributes();
        av.set_normal(normal);
    }

    if (face.halfedge_count() == 3)
    {
        set_triangular_texcoords(mesh, face.id());
    }
    else
    {
        calculate_face_texcoords(mesh, face.id());
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::model::create_section_surfaces(M& mesh, id_type submesh_id, size_t n, std::function<std::string(size_t)> namer)
{
    id_type surface_id0 = mesh.surface_store_count();
    for (size_t i = 0; i < n; ++i)
    {
        mesh.create_surface(submesh_id, namer(i));
    }

    return surface_id0;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::check_normal(size_t n, const quetzal::model::normal_vector_type<M>& normal, const typename M::vertex_type& vertex, const typename M::face_type& face)
{
//    std::cout << "check_normal <" << n << "> [" << normal << "]" << std::endl;
//    std::cout << "c v n " << vertex.id() << " normal [" << vertex.attributes().normal() << "]" << std::endl;
//    std::cout << "c f n " << face.id() << " normal [" << face.attributes().normal() << "]" << std::endl;
//    std::cout << "c dot " << dot(normal, face.attributes().normal()) << std::endl;

//    assert(quetzal::math::float_gt0(dot(normal, face.attributes().normal())));
// This can only be guaranteed with surface changes at singular points, as with polygon sections, for example ...
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::calculate_lower_surface_vertex_normals_sequential(M& mesh, id_type id0, size_t n)
{
//std::cout << "calculate_lower_surface_vertex_normals_sequential " << id0 << " " << n << std::endl;
    // j0..j3 are the virtual halfedge positions in each of the four faces that share this vertex
    auto& halfedge0 = mesh.halfedge(id0);
    id_type surface_id = halfedge0.surface_id();
    bool border_z = halfedge0.border() || halfedge0.partner().surface_id() != surface_id; // This will be the same for the whole sequence

    auto normal = surface_normal(halfedge0.vertex());
    if (!border_z)
    {
        auto normal_prev_z = halfedge0.partner().next().next().vertex_attributes().normal();
        if (float_eq(normal, normal_prev_z))
        {
            normal = normal_prev_z;
        }
    }

    halfedge0.vertex_attributes().set_normal(normal); // j0
    if (!border_z)
    {
        halfedge0.partner().next().vertex_attributes().set_normal(normal); // j3
    }
//std::cout << "0 (" << normal << ")" << std::endl;

    id_type id = id0;

    for (size_t j = 1; j < n; ++j)
    {
        ++id;
        auto& halfedge = mesh.halfedge(id);
        assert(halfedge.surface_id() == surface_id);

        normal = surface_normal(halfedge.vertex());
        if (!border_z)
        {
            auto normal_prev_z = halfedge.partner().next().next().vertex_attributes().normal();
            if (float_eq(normal, normal_prev_z))
            {
                normal = normal_prev_z;
            }
        }
        auto normal_prev_a = halfedge.prev().partner().prev().vertex_attributes().normal();
        if (float_eq(normal, normal_prev_a))
        {
            normal = normal_prev_a;
        }

        halfedge.vertex_attributes().set_normal(normal); // j0
        halfedge.prev().partner().vertex_attributes().set_normal(normal); // j1

        if (!border_z)
        {
            halfedge.prev().partner().prev().partner().vertex_attributes().set_normal(normal); // j2
            halfedge.partner().next().vertex_attributes().set_normal(normal); // j3
        }
//std::cout << j << " (" << normal << ")" << std::endl;
    }

    auto& halfedge = mesh.halfedge(id).next();
    assert(halfedge.surface_id() == surface_id);

    normal = surface_normal(halfedge.vertex());
    if (!border_z)
    {
        auto normal_prev_z = halfedge.prev().partner().prev().vertex_attributes().normal();
        if (float_eq(normal, normal_prev_z))
        {
            normal = normal_prev_z;
        }
    }
    auto normal_prev_a = halfedge.prev().vertex_attributes().normal();
    if (float_eq(normal, normal_prev_a))
    {
        normal = normal_prev_a;
    }

    halfedge.vertex_attributes().set_normal(normal); // j1
    if (!border_z)
    {
        halfedge.prev().partner().vertex_attributes().set_normal(normal); // j2
    }
//std::cout << n << " (" << normal << ")" << std::endl;

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::calculate_upper_surface_vertex_normals_sequential(M& mesh, id_type id0, size_t n)
{
//std::cout << "calculate_upper_surface_vertex_normals_sequential " << id0 << " " << n << std::endl;
    // j0..j3 are the virtual halfedge positions in each of the four faces that share this vertex

    auto& halfedge0 = mesh.halfedge(id0).next();
    id_type surface_id = halfedge0.surface_id();

    auto normal = surface_normal(halfedge0.vertex());
    auto normal_prev_z = halfedge0.next().vertex_attributes().normal();
    if (float_eq(normal, normal_prev_z))
    {
        normal = normal_prev_z;
    }

    halfedge0.vertex_attributes().set_normal(normal); // j3

    id_type id = id0;

    for (size_t j = 0; j < n - 1; ++j)
    {
        auto& halfedge = mesh.halfedge(id);
        assert(halfedge.border());
        assert(halfedge.surface_id() == surface_id);

        normal = surface_normal(halfedge.vertex());
        normal_prev_z = halfedge.prev().vertex_attributes().normal();
        if (float_eq(normal, normal_prev_z))
        {
            normal = normal_prev_z;
        }
        auto normal_prev_a = halfedge.next().vertex_attributes().normal();
        if (float_eq(normal, normal_prev_a))
        {
            normal = normal_prev_a;
        }
        // resolve two conditions? ...

        halfedge.vertex_attributes().set_normal(normal); // j2
        halfedge.prev().partner().vertex_attributes().set_normal(normal); // j3

        ++id;
    }

    auto& halfedge = mesh.halfedge(id);
    assert(halfedge.border());
    assert(halfedge.surface_id() == surface_id);

    normal = surface_normal(halfedge.vertex());
    normal_prev_z = halfedge.prev().vertex_attributes().normal();
    if (float_eq(normal, normal_prev_z))
    {
        normal = normal_prev_z;
    }
    auto normal_prev_a = halfedge.next().vertex_attributes().normal();
    if (float_eq(normal, normal_prev_a))
    {
        normal = normal_prev_a;
    }

    halfedge.vertex_attributes().set_normal(normal); // j2

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::calculate_apex_cusp_surface_vertex_normals_sequential(M& mesh, id_type id0, size_t n)
{
//std::cout << "calculate_apex_cusp_surface_vertex_normals_sequential " << id0 << " " << n << std::endl;
    auto id = id0;

    for (size_t i = 0; i < n; ++i)
    {
        auto& halfedge = mesh.halfedge(id);

        auto normal = halfedge.face_attributes().normal();
// instead of doing this here, can we ensure that corresopnding vertex_attributes and face normals are consistent on creation? ...
        auto normal_prev = halfedge.prev().vertex_attributes().normal();
        if (float_eq(normal, normal_prev))
        {
            normal = normal_prev;
        }

        halfedge.vertex_attributes().set_normal(normal);

        ++id;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::calculate_base_cusp_surface_vertex_normals_sequential(M& mesh, id_type id0, size_t n)
{
//std::cout << "calculate_base_cusp_surface_vertex_normals_sequential " << id0 << " " << n << std::endl;
    auto id = id0;

    for (size_t i = 0; i < n; ++i)
    {
        auto& halfedge = mesh.halfedge(id);
        auto normal = halfedge.face_attributes().normal();
        halfedge.vertex_attributes().set_normal(normal);

        ++id;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::texture_span_type<M> quetzal::model::texture_span(data_value_type<M> t, bool cusped_lower, bool cusped_upper, data_value_type<M> alignment)
{
    using T = data_value_type<M>;

    assert(math::clamped01(t));

    T v = math::denoise01(T(1) - t);
    T w = T(1);

    // for partial azimuth sphere, width might instead be the fraction of the maximum equatorial width of the side closure etc. ...
    // partial elevation sphere should have same width throughout even as it may be missing cusps, so can't use only cusped for logic here  ...
    // that width might instead be the fraction of the maximum equatorial width minus the side closure ...
    if (cusped_lower && cusped_upper)
    {
        return {T(0), w, v};
    }
    else if (cusped_lower)
    {
        w = t;
    }
    else if (cusped_upper)
    {
        w = v;
    }

    T u0 = math::denoise01(alignment * (T(1) - w));
    assert(math::clamped01(u0));

    T u1 = math::denoise01(u0 + w); // T(1) - u0;
    assert(math::clamped01(u1));

    return {u0, u1, v};
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::texture_span_type<M> quetzal::model::texture_span(size_t i, size_t n, bool cusped_lower, bool cusped_upper, data_value_type<M> alignment)
{
    using T = data_value_type<M>;

    assert(n > 0);
    assert(i <= n);

    T v = texture_span_v<M>(i, n);
    T w = T(1);

    // Too bad we can't just ...
    //return texture_span(T(1) - v, cusped_lower, cusped_upper, alignment);
    // unless texture_span(t) takes v instead of t ...

    // for partial azimuth sphere, width might instead be the fraction of the maximum equatorial width of the side closure etc. ...
    // partial elevation sphere should have same width throughout even as it may be missing cusps, so can't use only cusped for logic here  ...
    // that width might instead be the fraction of the maximum equatorial width minus the side closure ...
    if (cusped_lower && cusped_upper)
    {
        return {T(0), w, v};
    }
    else if (cusped_lower)
    {
        w = T(1) - v;
    }
    else if (cusped_upper)
    {
        w = v;
    }

    T u0 = math::denoise01(alignment * (T(1) - w));
    assert(math::clamped01(u0));

    T u1 = math::denoise01(u0 + w); // T(1) - u0;
    assert(math::clamped01(u1));

    return {u0, u1, v};
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::data_value_type<M> quetzal::model::texture_span_v(size_t i, size_t n)
{
//    T t = T(i) / T(n);
//    T v = T(1) - t; // lerp(t, 0, 1) result
//assert(t == T(1) - v);
    return data_value_type<M>(n - i) / n;
}

#endif // QUETZAL_MODEL_PRIMITIVES_UTIL_HPP
