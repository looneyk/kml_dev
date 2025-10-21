//------------------------------------------------------------------------------
// contour_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/geometry/Polyline.hpp"
#include "quetzal/geometry/Properties.hpp"
#include "quetzal/model/obj_io.hpp"
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

    const std::string dirOut = "out/";

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    string name;
    mesh_type mesh;
    geometry::Polyline<value_type, 2> contour;

    name = "contour_test_0";
    cout << name << endl;
    mesh.clear();
    contour.load(name + ".xml");
    model::create_cylinder(mesh, 6, contour);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name + ".obj");

    name = "contour_test_1";
    cout << name << endl;
    mesh.clear();
    contour.load(name + ".xml");
    model::create_cylinder(mesh, 6, contour);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name + ".obj");

    name = "angular_pawn";
    cout << name << endl;
    mesh.clear();
    contour.load(name + ".xml");
    model::create_cylinder(mesh, 6, contour);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name + ".obj");

    return 0;
}
