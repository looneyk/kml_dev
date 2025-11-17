//------------------------------------------------------------------------------
// coil_lattice
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/brep/validation.hpp"
#include "quetzal/geometry/OrientedPoint.hpp"
#include "quetzal/geometry/Path.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/Vector.hpp"
#include "quetzal/math/math_util.hpp"
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
    using mesh_traits = brep::MeshTraits<vector_traits>;
    using mesh_type = brep::Mesh<mesh_traits>;
    using point_type = mesh_type::point_type;
    using vector_type = point_type::vector_type;

    const std::string dirOut = "out/";

    //--------------------------------------------------------------------------
    geometry::Path<vector_traits> create_coil_path_spherical(size_t n, float radius, const math::Interval<value_type>& azimuth = math::Interval(value_type(1)), const math::Interval<value_type>& elevation = math::Interval(value_type(1)))
    {
        geometry::Path<vector_traits> path;

        for (size_t i = 0; i <= n; ++i)
        {
            float t = value_type(i) / value_type(n);
            float phi = math::PiTwo<value_type> * (azimuth.lower() + t * azimuth.length());
            float theta = math::Pi<value_type> * (elevation.lower() + t * elevation.length() - 0.5f);
            if (i == n)
            {
                phi = math::PiTwo<value_type> * azimuth.upper();
                theta = math::Pi<value_type> * (elevation.upper() - 0.5f);
            }

            float dphi = math::PiTwo<value_type> * azimuth.length();
            float dtheta = math::Pi<value_type> * elevation.length();

            vector_type position = {radius * cos(phi) * cos(theta), radius * sin(phi) * cos(theta), radius * sin(theta)};
            vector_type direction = math::normalize<vector_traits>({radius * (-sin(phi) * dphi * cos(theta) + cos(phi) * -sin(theta) * dtheta), radius * (cos(phi) * dphi * cos(theta) + sin(phi) * -sin(theta) * dtheta), radius * cos(theta) * dtheta});
            vector_type right = normalize(position);

            geometry::OrientedPoint<vector_traits> op{position, {direction, right}};
            path.emplace_back(op);
        }

        return path;
    }

    //--------------------------------------------------------------------------
    void position_coil(mesh_type& mesh, float phi)
    {
        auto matrix = geometry::rotation_z(phi);
        model::transform(mesh, matrix, matrix);
        return;
    }

    //--------------------------------------------------------------------------
    void create_coils_spherical(mesh_type& mesh, size_t nCoils, size_t nAzimuth, size_t nz, float radius)
    {
        auto circle = geometry::regular_polygon<vector_traits>(nAzimuth, radius);

        geometry::Path<vector_traits> path_right = create_coil_path_spherical(nz, 75.0f, math::Interval(value_type(1)));
        geometry::Path<vector_traits> path_left = create_coil_path_spherical(nz, 75.0f, math::Interval(value_type(-1)));
        mesh_type mesh_right;
        model::create_swept_volume(mesh_right, "coil", circle, path_right, [](const geometry::Polygon<vector_traits>& in, float t) -> geometry::Polygon<vector_traits> {geometry::Polygon<vector_traits> out(in.vertex_count()); float scale = 1.0f - 3.0f * (t - 0.5f) * (t - 0.5f); for (size_t i = 0; i < in.vertex_count(); ++i) {out.vertex(i) = scale * in.vertex(i);} return out;});
        mesh_type mesh_left;
        model::create_swept_volume(mesh_left, "coil", circle, path_left, [](const geometry::Polygon<vector_traits>& in, float t) -> geometry::Polygon<vector_traits> {geometry::Polygon<vector_traits> out(in.vertex_count()); float scale = 1.0f - 3.0f * (t - 0.5f) * (t - 0.5f); for (size_t i = 0; i < in.vertex_count(); ++i) {out.vertex(i) = scale * in.vertex(i);} return out;});

        mesh_type m;

        for (size_t i = 0; i < nCoils; ++i)
        {
            m = mesh_right;
            m.rename_submesh(m.submesh_id("coil"), "coil_right_" + to_string(i));
            position_coil(m, math::PiTwo<float> * i / nCoils);
            triangulate(m);
            mesh.insert(m);

            m = mesh_left;
            m.rename_submesh(m.submesh_id("coil"), "coil_left_" + to_string(i));
            position_coil(m, math::PiTwo<float> * i / nCoils);
            triangulate(m);
            mesh.insert(m);
        }

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    string name;
    mesh_type mesh;

    name = "cross_coils_spherical.obj";
    cout << name << endl;
    mesh.clear();
    create_coils_spherical(mesh, 12, 4, 100, 3.0f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    return 0;
}
