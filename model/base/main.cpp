//------------------------------------------------------------------------------
// base
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
//#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/brep/triangulation.hpp"
//#include "quetzal/math/math_util_dx.hpp" // ...
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

    string name;
    mesh_type mesh;

    name = "base_40mm";
    cout << name << endl;
    mesh.clear();
    model::create_antiprism(mesh, name, 240, 20.0f, 18.5f, 0.0f, 3.2f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "base_60mm";
    cout << name << endl;
    mesh.clear();
    model::create_antiprism(mesh, name, 360, 30.0f, 28.5f, 0.0f, 3.2f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    return 0;
}
