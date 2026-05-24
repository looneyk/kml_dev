//------------------------------------------------------------------------------
// ghost_pendant
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/brep/validation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/model/obj_io.hpp"
//#include "quetzal/model/primitives.hpp"
#include <iostream>
#include <string>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using mesh_type = brep::Mesh<brep::MeshTraits<value_type>>;

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
    mesh_type m;
    mesh_type::point_type point = {0.0f, 0.0f, 0.0f};

    mesh.clear();
    model::read_obj(mesh, "ghost_pendant.obj");
validate(mesh);

    name = "ghost_pendant_top";
    cout << name << endl;
    m = mesh;
    clip(m, geometry::Plane(point, normalize(-vector_type::z_direction())), "slice");
    triangulate(m);
    model::write_obj(m, to_filename(name));

    name = "ghost_pendant_bottom";
    cout << name << endl;
    m = mesh;
    clip(m, geometry::Plane(point, normalize(vector_type::z_direction())), "slice");
    triangulate(m);
    model::write_obj(m, to_filename(name));

//    validator.report_results();
    return 0;
}
