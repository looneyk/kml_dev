//------------------------------------------------------------------------------
// wavefront_obj_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
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
//    using value_type = double;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;

    static const string dirOut = "out/";
    static const string dirReference = "reference/";

    //--------------------------------------------------------------------------
    string to_filename(const string& name)
    {
        return dirOut + name + ".obj";
    }

    //--------------------------------------------------------------------------
    void rewrite(string name)
    {
        mesh_type mesh;
        model::read_obj(mesh, to_filename(name));

        name += "_";
        model::write_obj(mesh, to_filename(name));
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

    name = "box";
    cout << name << endl;
    mesh.clear();
    model::create_box(mesh, name, 5.0f, 10.0f, 20.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    
    name = "cylinder";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 12, 10, 5.0f, 5.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    name = "cylinder_1";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 12, 1, 5.0f, 5.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    name = "cone_up";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 12, 6, 7.5f, 0.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    name = "cone_down";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 12, 6, 0.0f, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    name = "cone_up_1";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 6, 1, 7.5f, 0.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    name = "cone_down_1";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 6, 1, 0.0f, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    name = "pyramid_up";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 12, 6, 7.5f, 0.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    name = "pyramid_down";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 12, 6, 0.0f, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    name = "pyramid_up_1";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 6, 1, 7.5f, 0.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    name = "pyramid_down_1";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 6, 1, 0.0f, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    name = "sphere";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 24, 12, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    name = "torus";
    cout << name << endl;
    mesh.clear();
    model::create_torus(mesh, name, 24, 12, 15.0f, 7.5f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    name = "tetrahedron";
    cout << name << endl;
    mesh.clear();
    model::create_tetrahedron(mesh, name, 10.0f);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    name = "octahedron_vertex";
    cout << name << endl;
    mesh.clear();
    model::create_octahedron(mesh, name, 10.0f, true);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    name = "octahedron_face";
    cout << name << endl;
    mesh.clear();
    model::create_octahedron(mesh, name, 10.0f, false);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    name = "dodecahedron";
    cout << name << endl;
    mesh.clear();
    model::create_dodecahedron(mesh, name, 10.0f);
    triangulate_central_vertex(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    name = "icosahedron_vertex";
    cout << name << endl;
    mesh.clear();
    model::create_icosahedron(mesh, name, 10.0f);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    name = "icosahedron_face";
    cout << name << endl;
    mesh.clear();
    model::create_icosahedron(mesh, name, 10.0f, false, false);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);

    validator.report_results();
    return 0;
}
