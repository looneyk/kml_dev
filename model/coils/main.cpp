//------------------------------------------------------------------------------
// coils
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/clip.hpp"
#include "quetzal/brep/validation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/HalfSpace.hpp"
#include "quetzal/geometry/OrientedPoint.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/Matrix.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <algorithm>
#include <iostream>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
    using point_type = mesh_type::point_type;
    using vector_type = point_type::vector_type;
    using vertex_type = mesh_type::vertex_type;
    using vertex_attributes_type = mesh_type::vertex_attributes_type;
    using halfspace_type = geometry::HalfSpace<vector_traits>;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "coils";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

    //--------------------------------------------------------------------------
    vertex_attributes_type coilify(vertex_attributes_type av, value_type r, value_type dphi, value_type z0, value_type z1, value_type azimuthArc)
    {
        value_type dz = z1 - z0;
        value_type t = (av.position().z() - z0) / dz;
        value_type phi = t * dphi;

        r *= sin(t * azimuthArc);

        auto matrix_rotation = geometry::rotation_z(phi);
        auto matrix_position = matrix_rotation * geometry::translation(r * cos(phi), r * sin(phi), 0.0f);
        auto matrix_direction = geometry::direction_matrix(matrix_position); // should simply be same as matrix_rotation? ...

        av.position() *= matrix_position;
        av.normal() *= matrix_direction;
        return av;
    }

    //--------------------------------------------------------------------------
    point_type coilify_position(const point_type&position, value_type r, value_type z0, value_type z1, value_type x, value_type y, value_type azimuth0, value_type azimuthArc)
    {
        value_type dz = z1 - z0;
        value_type t = (position.z() - z0) / dz;
        value_type phi = azimuth0 + t * azimuthArc;

        auto matrix = geometry::translation(x + r * cos(phi), y + r * sin(phi), 0.0f);
        return position * matrix;
    }

    //--------------------------------------------------------------------------
    geometry::OrientedPoint<vector_traits> curvulate(const geometry::OrientedPoint<vector_traits>& op, value_type factor, value_type theta0, value_type theta1, value_type z0, value_type z1)
    {
        value_type t = (op.position().z() - z0) / (z1 - z0);
        value_type theta = theta0 + t * (theta1 - theta0);
        value_type scale = factor * sin(theta);
        value_type trans = 1.0f * factor * sin(theta) - t * t * 2.0f;
//        value_type trans = (1.0f - t * t) * 3.0f * sin(theta);

        auto matrix_position = geometry::scaling(scale, scale, 1.0f);
        matrix_position *= geometry::translation(trans * op.position().x(), trans * op.position().y(), op.position().z());
//        auto matrix_direction = math::scaling(1.0f / f, 1.0f / f, 0.0f);

//        return {op.position() * matrix_position, {normalize(op.orientation().direction() * matrix_direction), normalize(op.orientation().right() * matrix_direction)}};
        return {op.position() * matrix_position, op.orientation()};
    }

    //--------------------------------------------------------------------------
    geometry::Path<vector_traits> create_coil_path_linear(size_t n, value_type r0, value_type r1, value_type x0, value_type x1, value_type z0, value_type z1, const math::Interval<value_type>& azimuth)
    {
        value_type dr = r1 - r0;
        value_type dx = x1 - x0;
        value_type dz = z1 - z0;
        value_type phi0 = math::PiTwo<value_type> * azimuth.lower();
        value_type dphi = math::PiTwo<value_type> * azimuth.length();

        geometry::Path<vector_traits> path(n + 1);

        for (size_t i = 0; i <= n; ++i)
        {
            value_type t = value_type(i) / value_type(n);
            value_type r = r0 + t * dr;
            value_type x = x0 + t * dx;
            value_type z = z0 + t * dz;
            value_type phi = phi0 + t * dphi;

            value_type cos_phi = cos(phi);
            value_type sin_phi = sin(phi);

            point_type position = {x + r * cos_phi, r * sin_phi, z};
            vector_type direction = {dx + r * -sin_phi * dphi + dr * cos_phi, dr * sin_phi + r * cos_phi * dphi, dz};
            vector_type up = {dx, 0.0f, dz};
            vector_type right = normalize(cross(direction, up));

            path[i] = {position, {math::normalize(direction), right}};
        }

        return path;
    }

    //--------------------------------------------------------------------------
    geometry::Path<vector_traits> create_coil_path_parabolic(size_t n, value_type rFactor, value_type r0, value_type x0, value_type z0, value_type z1, const math::Interval<value_type>& azimuth)
    {
        value_type dz = z1 - z0;
        value_type phi0 = math::PiTwo<value_type> * azimuth.lower();
        value_type dphi = math::PiTwo<value_type> * azimuth.length();

        geometry::Path<vector_traits> path(n + 1);

        for (size_t i = 0; i <= n; ++i)
        {
            value_type t = static_cast<value_type>(i) / n;
            value_type z = z0 + t * dz;
            value_type phi = phi0 + t * dphi;

            value_type cos_phi = cos(phi);
            value_type sin_phi = sin(phi);

            value_type a = -4.5f * rFactor + 3.0f;
            value_type b = 4.5f * rFactor - 4.0f;
            value_type c = 1.0f;
            value_type r = r0 * (a * t * t + b * t + c);
            value_type dr = r0 * (2.0f * a * t + b);
            value_type x = r * cos_phi;
            value_type y = r * sin_phi;
            value_type dx = r * -sin_phi * dphi + dr * cos_phi;
            value_type dy = r * cos_phi * dphi + dr * sin_phi;

            value_type xx = x0 * rFactor * (a * t * t + b * t + c);
            value_type dxx = x0 * rFactor * (2.0f * a * t + b);

            point_type position = {xx + x, y, z};
            vector_type direction = {dxx + dx, dy, dz};
            vector_type up = {dxx, 0.0f, dz};
            vector_type right = normalize(cross(direction, up));

            path[i] = {position, {math::normalize(direction), right}};
        }

        return path;
    }

    //--------------------------------------------------------------------------
    void position_coil(mesh_type& mesh, value_type phi)
    {
        auto matrix = geometry::rotation_z(phi);
//        model::transform(mesh, matrix, matrix);
        model::transform(mesh.submesh(0), matrix, matrix);
        return;
    }

    //--------------------------------------------------------------------------
    halfspace_type halfspace_upper(value_type z = value_type(0))
    {
        return {{{value_type(0), value_type(0), z}, vector_type::z_direction()}, -vector_type::z_direction()};
    }

    //--------------------------------------------------------------------------
    void create_coils_linear(mesh_type& mesh, size_t nCoils, size_t nAzimuth, size_t nz, value_type radius)
    {
        auto circle = geometry::regular_polygon<vector_traits>(nAzimuth, radius);
        circle.set_final();

        math::Interval<value_type> azimuth_even = {0.0f, 2.0f};
        math::Interval<value_type> azimuth_odd = shift(azimuth_even, 0.5f);
        geometry::Path<vector_traits> path_even = create_coil_path_linear(nz, 20.0f, 0.25f, 30.0f, 01.5f, 0.0f, 100.0f, azimuth_even);
        geometry::Path<vector_traits> path_odd = create_coil_path_linear(nz, 20.0f, 0.25f, 30.0f, 01.5f, 0.0f, 100.0f, azimuth_odd);
        auto scaler = [](const geometry::Polygon<vector_traits>& in, value_type t) -> geometry::Polygon<vector_traits>
        {
            return scale(in, (1.0f - 0.85f * t));
        };
        // change to single m0 with separate submesh_id's ...
        mesh_type m0_even;
        model::create_swept_volume(m0_even, "coil", circle, path_even, scaler);
        mesh_type m0_odd;
        model::create_swept_volume(m0_odd, "coil", circle, path_odd, scaler);
// replace ...
        mesh_type m;
        for (size_t i = 0; i < nCoils; ++i)
        {
            m = math::even(i) ? m0_even : m0_odd;
            m.rename_submesh(m.submesh_id("coil"), "coil_" + to_string(i));
            position_coil(m, math::PiTwo<value_type> * i / nCoils);
            clip(m, halfspace_upper());
            triangulate(m);
            mesh.insert(m);
        }
/* with this ...
        for (size_t i = 0; i < nCoils; ++i)
        {
            value_type phi = math::PiTwo<value_type> * i / nCoils;
            auto matrix = geometry::rotation_z(phi);
            id_type submesh_id_coil = mesh.insert(math::even(i) ? m0_even : m0_odd, 0, "coil_" + to_string(i), [=](const typename mesh_type::vertex_type& vertex) -> typename mesh_type::vertex_type
            {
                vertex_type v = vertex;
                v.attributes().transform(matrix, matrix);
                return v;
            });
            clip(mesh.submesh(submesh_id_coil), halfspace_upper());
            triangulate(mesh.submesh(submesh_id_coil));
        }
*/
        // Pinnacle
        id_type submesh_id_pinnacle = model::create_geodesic_sphere(mesh, "pinnacle", 3.2f, 5);
// this needed for printing, but incorporate in a better way ...
//        clip(mesh.submesh(submesh_id_pinnacle), halfspace_upper(-2.4f));
        model::transform(mesh.submesh(submesh_id_pinnacle), geometry::scaling(0.75f, 0.75f, 1.0f) * geometry::translation(0.0f, 0.0f, 99.0f));
        triangulate(mesh.submesh(submesh_id_pinnacle));

        // Base
        id_type submesh_id_base = model::create_geodesic_sphere(mesh, "base", 60.0f, 5);
        clip(mesh.submesh(submesh_id_base), halfspace_upper());
        model::transform(mesh.submesh(submesh_id_base), geometry::scaling(1.0f, 1.0f, 0.1f));
        triangulate(mesh.submesh(submesh_id_base));

        return;
    }

    //--------------------------------------------------------------------------
    void create_coils_parabolic(mesh_type& mesh, size_t nCoils, size_t nAzimuth, size_t nz, value_type radius)
    {
        auto circle = geometry::regular_polygon<vector_traits>(nAzimuth, radius);
        circle.set_final();

        math::Interval<value_type> azimuth_even = {0.0f, 2.0f};
        math::Interval<value_type> azimuth_odd = shift(azimuth_even, 0.5f);

        geometry::Path<vector_traits> path_even = create_coil_path_parabolic(nz, 1.5f, 20.0f, 20.0f, 0.0f, 150.0f, azimuth_even);
        geometry::Path<vector_traits> path_odd = create_coil_path_parabolic(nz, 1.5f, 20.0f, 20.0f, 0.0f, 150.0f, azimuth_odd);
        auto scaler = [](const geometry::Polygon<vector_traits>& in, value_type t) -> geometry::Polygon<vector_traits>
        {
            return scale(in, (1.0f - 0.75f * t));
        };
        mesh_type m0_even;
        model::create_swept_volume(m0_even, "coil", circle, path_even, scaler);
        mesh_type m0_odd;
        model::create_swept_volume(m0_odd, "coil", circle, path_odd, scaler);

// replace ...
        mesh_type m;
        for (size_t i = 0; i < nCoils; ++i)
        {
            m = math::even(i) ? m0_even : m0_odd;
            m.rename_submesh(m.submesh_id("coil"), "coil_" + to_string(i));
            position_coil(m, math::PiTwo<value_type> * i / nCoils);
            clip(m, halfspace_upper());
            triangulate(m);
            mesh.insert(m);
        }

        // Pinnacle
        id_type submesh_id_pinnacle = model::create_geodesic_sphere(mesh, "pinnacle", 3.2f, 5);
        model::transform(mesh.submesh(submesh_id_pinnacle), geometry::scaling(0.75f, 0.75f, 1.0f) * geometry::translation(0.0f, 0.0f, 149.0f));
        triangulate(mesh.submesh(submesh_id_pinnacle));

        // Base
        id_type submesh_id_base = model::create_geodesic_sphere(mesh, "base", 70.0f, 5);
        clip(mesh.submesh(submesh_id_base), halfspace_upper());
        model::transform(mesh.submesh(submesh_id_base), geometry::scaling(1.0f, 1.0f, 0.1f));
        triangulate(mesh.submesh(submesh_id_base));

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    FileValidator validator(dir_out, dir_ref, app_name);

    mesh_type mesh;
    string name;

    name = "coil_cone";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, "cylinder", 6, 100, 5.0f, 0.0f, -10.0f, 10.0f);
    triangulate(mesh);
//    validate(mesh);
//    check_normal_consistency(mesh);
    model::transform_vertices(mesh, [](vertex_attributes_type av) -> vertex_attributes_type { return coilify(av, 5.0f, math::PiTwo<value_type>, -10.0f, 10.0f, math::PiTwo<value_type>); });
    model::write_obj(mesh, to_filename(name));
//validate(mesh);

    name = "coil_prism";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, "prism", 6, 100, 5.0f, 0.0f, -10.0f, 10.0f);
    triangulate(mesh);
//    validate(mesh);
//    check_normal_consistency(mesh);
    model::transform_vertices(mesh, [](vertex_attributes_type av) -> vertex_attributes_type { return coilify(av, 5.0f, math::PiTwo<value_type>, -10.0f, 10.0f, math::PiTwo<value_type>); });
    model::write_obj(mesh, to_filename(name));
//validate(mesh);

    name = "entwine_8";
    cout << name << endl;
    mesh.clear();
    mesh_type m0;
    id_type submesh_id0 = model::create_cylinder(m0, "cylinder", 12, 100, 3.75f, 3.75f, -50.0f, 50.0f);
    for (size_t i = 0; i < 8; ++i)
    {
        value_type x = 30.0f * cos(math::PiTwo<value_type> * i / 8);
        value_type y = 30.0f * sin(math::PiTwo<value_type> * i / 8);

        id_type submesh_id = mesh.insert(m0, submesh_id0, std::format("cylinder_{}", i), [=](const typename mesh_type::vertex_type& vertex) -> typename mesh_type::vertex_type
        {
            vertex_type v = vertex;
            point_type position = vertex.attributes().position();
            v.attributes().position() = coilify_position(position, 20.0f, -50.0f, 50.0f, x, y, math::Pi<value_type> * i + math::PiTwo<value_type> * i / 8, 2.0f * math::PiTwo<value_type>);
            return v;
        });
        triangulate(mesh.submesh(submesh_id));
    }
    model::write_obj(mesh, to_filename(name));
//validate(mesh);

    name = "entwine_linear_8_100";
    cout << name << endl;
    mesh.clear();
    create_coils_linear(mesh, 8, 10, 100, 3.5f);
    model::write_obj(mesh, to_filename(name));
//validate(mesh);

    name = "entwine_linear_10_100";
    cout << name << endl;
    mesh.clear();
    create_coils_linear(mesh, 10, 10, 100, 3.5f);
    model::write_obj(mesh, to_filename(name));
//validate(mesh);
/*
    name = "entwine_linear_8_300";
    cout << name << endl;
    mesh.clear();
    create_coils_linear(mesh, 8, 10, 300, 3.5f);
    model::write_obj(mesh, to_filename(name));

    name = "entwine_linear_10_300";
    cout << name << endl;
    mesh.clear();
    create_coils_linear(mesh, 10, 10, 300, 3.5f);
    model::write_obj(mesh, to_filename(name));
*/
    name = "entwine_parabolic_8_100";
    cout << name << endl;
    mesh.clear();
    create_coils_parabolic(mesh, 8, 10, 100, 5.0f);
    model::write_obj(mesh, to_filename(name));
//validate(mesh);

    name = "entwine_parabolic_10_100";
    cout << name << endl;
    mesh.clear();
    create_coils_parabolic(mesh, 10, 10, 100, 5.0f);
    model::write_obj(mesh, to_filename(name));
//validate(mesh);
/*
    name = "entwine_parabolic_8_300";
    cout << name << endl;
    mesh.clear();
    create_coils_parabolic(mesh, 8, 10, 300, 5.0f);
    model::write_obj(mesh, to_filename(name));

    name = "entwine_parabolic_10_300";
    cout << name << endl;
    mesh.clear();
    create_coils_parabolic(mesh, 10, 10, 300, 5.0f);
    model::write_obj(mesh, to_filename(name));
*/

    return validator.report_results();
}
