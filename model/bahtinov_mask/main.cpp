//------------------------------------------------------------------------------
// bahtinov mask
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/Submesh.hpp"
//#include "quetzal/brep/mesh_operations.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/brep/validation.hpp"
#include "quetzal/math/Vector.hpp"
//#include "quetzal/math/math_util.hpp"
#include "quetzal/math/math_util_dx.hpp" // ...
#include "quetzal/geometry/PolygonWithHoles.hpp"
#include "quetzal/geometry/polygon_util.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/primitives.hpp"
#include <string>
#include <iostream>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using mesh_type = brep::Mesh<brep::MeshTraits<value_type>>;
    using polygons_type = geometry::PolygonWithHoles<typename mesh_type::vector_traits>;

    const std::string dirOut = "out/";

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    string name;
    mesh_type mesh;

    math::Vector<value_type, 3> center = {372.047546386719f / 2.0f, 372.047546386719f / 2.0f, 0.0f};
    value_type scale = 25.4f / 96;

    polygons_type polygonsMask;
    geometry::load_polygon("bahtinov_mask.xml", polygonsMask);
for (auto& hole : polygonsMask.holes())
{
    model::reverse(hole); // in file, hole polygons are cw ...
    for (auto& vertex : hole.vertices())
    {
        vertex -= center;
        vertex *= scale;
    }
}
for (auto& vertex : polygonsMask.polygon().vertices())
{
    vertex -= center;
    vertex *= scale;
}

    polygons_type polygonsCollar;
    polygonsCollar.set_polygon(polygonsMask.polygon());
    polygonsCollar.holes().push_back(polygonsMask.polygon());
    for (auto& vertex : polygonsCollar.polygon())
    {
        vertex *= 1.1f;
    }

    name = "mask.obj";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, 1, 0.0f, 2.0f, polygonsMask);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "collar.obj";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, 1, 0.0f, 27.0f, polygonsCollar);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "mask_collar.obj";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, 1, 0.0f, 2.0f, polygonsMask);
    model::create_cylinder(mesh, 1, 0.0f, 27.0f, polygonsCollar);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    return 0;
}
