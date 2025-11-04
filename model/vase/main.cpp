//------------------------------------------------------------------------------
// vase
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/Orientation.hpp"
#include "quetzal/geometry/Path.hpp"
#include "quetzal/geometry/polygon_util.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/transformation_matrix.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <iostream>
#include <string>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::CartesianTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
    using point_type = typename mesh_type::point_type;
    using vector_type = typename mesh_type::vector_type;
    using size_type = typename mesh_type::size_type;

    static const string dirOut = "out/";
    static const string dirReference = "reference/";

    //--------------------------------------------------------------------------
    string to_filename(const string& name)
    {
        return dirOut + name + ".obj";
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    FileValidator validator(dirOut, dirReference);

    string name;
    mesh_type mesh;

    name = "vase";
    cout << name << endl;
    mesh.clear();

    geometry::Polygon<vector_traits> polygonOuter = {
        {0.5f, -0.5f, 0.0f},
        {1.5f, -0.5f, 0.0f},
        {1.5f, 0.5f, 0.0f},
        {0.5f, 0.5f, 0.0f},
        {0.5f, 1.5f, 0.0f},
        {-0.5f, 1.5f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
        {-1.5f, 0.5f, 0.0f},
        {-1.5f, -0.5f, 0.0f},
        {-0.5f, -0.5f, 0.0f},
        {-0.5f, -1.5f, 0.0f},
        {0.5f, -1.5f, 0.0f},
    };

    value_type height = 100.0;
    value_type diameter = 60.0;
    polygonOuter *= math::scaling(diameter / 3.0f);

    geometry::Polygon<vector_traits> polygonInner = expand(polygonOuter, -5.0f);

    value_type z0Outer = 0.0f;
    value_type z1Outer = height;
    value_type dzOuter = z1Outer - z0Outer;

    value_type z0Inner = 5.0f;
    value_type z1Inner = z1Outer;
    value_type dzInner = z1Inner - z0Inner;

    value_type revsOuter = 0.5f;
    value_type rev0Outer = 0.0f;
    value_type rev1Outer = rev0Outer + revsOuter;
    value_type rev0Inner = revsOuter * (z0Inner - z0Outer) / dzOuter;
    value_type rev1Inner = rev1Outer;

    size_type nzOuter = 100;
    size_type nzInner = static_cast<size_type>(nzOuter * dzInner / dzOuter);

    // Outer surface
    mesh_type m;
    model::create_cylinder(m, "outer", nzOuter, z0Outer, z1Outer, rev0Outer, rev1Outer, polygonOuter);
    mesh.insert(m);

    // Inner surface
    m.clear();
    model::create_cylinder(m, "inner", nzInner, z0Inner, z1Inner, rev0Inner, rev1Inner, polygonInner);
    invert(m);
    mesh.insert(m);

    // Connect outer and inner surfaces
    attach(mesh, mesh.surface("outer/end1").faces().front().id(), mesh.surface("inner/end1").faces().front().id());

    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    auto position_transformer = [&z0Outer, &z1Outer](typename mesh_type::vertex_attributes_type& av) -> void
    {
        value_type z = 2.0f * (av.position().z() - z0Outer) / (z1Outer - z0Outer) - 1.0f;
        value_type factor = z * z * 0.3f + 0.7f;

        av.position().x() *= factor;
        av.position().y() *= factor;
    };

    model::transform(mesh, position_transformer);
    model::write_obj(mesh, to_filename(name + "_contoured"));

    name = "helix_square";
    cout << name << endl;
    mesh.clear();

    value_type p2 = math::PiTwo<value_type>;
    value_type phi0 = 0.0f;
    value_type phi1 = math::PiTwo<value_type>;

    value_type r = 1.0f;
r *= 20.0f;

    value_type z0 = 0.0f;
    value_type z1 = 2.0f * height;
    value_type dz = z1 - z0;

    geometry::Path<vector_traits> path(101);
    for (size_t i = 0; i < path.size(); ++i)
    {
        value_type t = value_type(i) / (path.size() - 1);
        value_type z = math::lerp(z0, z1, t);
        value_type phi = math::lerp(phi0, phi1, t);

        value_type cos_phi = cos(phi);
        value_type sin_phi = sin(phi);

        point_type position = {r * cos_phi, r * sin(phi), z};
        vector_type normal = math::normalize<vector_traits>({cos_phi, sin_phi, 0.0f});
        vector_type tangent = math::normalize<vector_traits>({-r * p2 * sin_phi, r * p2 * cos_phi, dz});

        geometry::Orientation<vector_traits> orientation = {tangent, normal};
        geometry::OrientedPoint<vector_traits> point = {position, orientation};
        path[i] = point;
    }
/*
    geometry::Polygon<vector_traits> square = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.5f, 0.5f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
    };
*/
    geometry::Polygon<vector_traits> square = {
        {-0.5f, -1.2f, 0.0f},
        {1.0f, -0.6f, 0.0f},
        {1.0f, 0.6f, 0.0f},
        {-0.5f, 1.2f, 0.0f},
    };
    square *= math::scaling(diameter / 3.0f * 0.5f);

    square.set_orientation();

    m.clear();
    model::create_swept_volume(m, name, square, path);

    for (size_t i = 0; i < 4; ++i)
    {
        mesh_type mn = m;
        mn.rename_submesh(mn.submesh_id(name), name + to_string(i));
        model::transform(mn, math::rotation_z(i * math::PiHalf<value_type>));
        mesh.insert(mn);
    }

    z0Outer = 0.5f * height;
    z1Outer = 1.5f * height;

    geometry::Plane<vector_traits> planeBottom({0.0f, 0.0f, z0Outer}, {0.0f, 0.0f, -1.0f});
    clip(mesh, planeBottom, "bottom");
    geometry::Plane<vector_traits> planeTop({0.0f, 0.0f, z1Outer}, {0.0f, 0.0f, 1.0f});
    clip(mesh, planeTop, "top");

    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    model::transform(mesh, position_transformer);
    model::write_obj(mesh, to_filename(name + "_contoured"));

    validator.report_results();
    return 0;
}
