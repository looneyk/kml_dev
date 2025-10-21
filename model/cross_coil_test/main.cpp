//------------------------------------------------------------------------------
// coil_lattice
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/brep/validation.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/math/Vector.hpp"
//#include "quetzal/math/math_util.hpp"
#include "quetzal/math/math_util_dx.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <algorithm>
#include <iostream>

using namespace std;
using namespace quetzal;

namespace
{

	using value_type = float;
    using mesh_type = brep::Mesh<brep::MeshTraits<value_type>>;
    using vector3_type = mesh_type::vector3_type;

    const std::string dirOut = "out/";

    //--------------------------------------------------------------------------
    model::Path<float> create_coil_path_spherical(size_t n, float radius, const math::Interval<value_type>& azimuth = math::Interval(value_type(1)), const math::Interval<value_type>& elevation = math::Interval(value_type(1)))
    {
        model::Path<float> path;

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

            vector3_type position = {radius * cos(phi) * cos(theta), radius * sin(phi) * cos(theta), radius * sin(theta)};
            vector3_type direction = math::normalize<float, 3>({radius * (-sin(phi) * dphi * cos(theta) + cos(phi) * -sin(theta) * dtheta), radius * (cos(phi) * dphi * cos(theta) + sin(phi) * -sin(theta) * dtheta), radius * cos(theta) * dtheta});
            vector3_type right = normalize(position);

            geometry::OrientedPoint op{position, {direction, right}};
            path.emplace_back(op);
        }

        return path;
    }

    //--------------------------------------------------------------------------
    void position_coil(mesh_type& mesh, float phi)
    {
        auto matrix = math::rotation_z3(phi);
        model::transform(mesh, matrix, matrix);
        return;
    }

    //--------------------------------------------------------------------------
    void create_coils_spherical(mesh_type& mesh, size_t nCoils, size_t nAzimuth, size_t nz, float radius)
    {
        geometry::OrientedPolygon<float, 3> circle(nAzimuth);
        for (size_t i = 0; i < nAzimuth; ++i)
        {
            float phi = math::PiTwo<float> * i / nAzimuth;
            circle[i] = {radius * cos(phi), radius * sin(phi), 0.0f};
        }

        model::Path<float> pathRight = create_coil_path_spherical(nz, 75.0f, 1.0f); // 75 ...
        model::Path<float> pathLeft = create_coil_path_spherical(nz, 75.0f, -1.0f); // 75 ...
        mesh_type mRight;
        model::create_swept_volume(mRight, "coil", circle, pathRight, [](const geometry::OrientedPolygon<float, 3>& in, float t)->geometry::OrientedPolygon<float, 3>{geometry::OrientedPolygon<float, 3> out(in.size()); float scale = 1.0f - 3.0f * (t - 0.5f) * (t - 0.5f); for (size_t i = 0; i < in.size(); ++i) {out[i] = scale * in[i];} return out;});
        mesh_type mLeft;
        model::create_swept_volume(mLeft, "coil", circle, pathLeft, [](const geometry::OrientedPolygon<float, 3>& in, float t)->geometry::OrientedPolygon<float, 3>{geometry::OrientedPolygon<float, 3> out(in.size()); float scale = 1.0f - 3.0f * (t - 0.5f) * (t - 0.5f); for (size_t i = 0; i < in.size(); ++i) {out[i] = scale * in[i];} return out;});

        mesh_type m;

        for (size_t i = 0; i < nCoils; ++i)
        {
            m = mRight;
            m.rename_surface(m.surface_id("coil"), "coil_right_" + to_string(i)); // submesh ...
            position_coil(m, math::PiTwo<float> * i / nCoils);
            triangulate(m);
            mesh.insert(m);

            m = mLeft;
            m.rename_surface(m.surface_id("coil"), "coil_left_" + to_string(i)); // submesh ...
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
