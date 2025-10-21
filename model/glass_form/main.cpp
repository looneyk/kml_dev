//------------------------------------------------------------------------------
// glass_form
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/validation.hpp"
#include "quetzal/math/math_util_dx.hpp"
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

    mesh_type::vector3_type point = {0.0f, 0.0f, -10.0f};
    mesh_type::vector3_type direction = {0.0f, 0.0f, -1.0f};
    geometry::Plane plane(point, direction);

    float phi = 0.19591327601530363508542777961122f;
    float phi2 = phi / 2.0f;

    name = "tetra_cradle_3.obj";
    cout << name << endl;
    mesh.clear();

//    model::create_sphere(mesh, 4, 2, 70.0f, model::ExtentSideCenter(phi2, 1.0f - phi2));
model::create_sphere(mesh, 4, 2, 70.0f);
//cout << "create_sphere" << endl;
//brep::dump(mesh);
    model::transform(mesh, math::rotation_normal({0.0f, 1.0f, 0.0f}, -0.25f * math::PiTwo<float>));
//cout << "rotation_normal" << endl;
//brep::dump(mesh);
    model::transform(mesh, math::scaling(0.75f, 1.0f, 1.0f));
//    model::transform(mesh, math::translation(0.0f, 10.0f, 0.0f));
//cout << "scaling" << endl;
//brep::dump(mesh);
    brep::clip(mesh, plane, "slice");

    triangulate(mesh);
    write_obj(mesh, dirOut + name);


    name = "tetra_cradle_flat.obj";
    cout << name << endl;
    mesh.clear();

    model::create_sphere(mesh, 4, 2, 70.0f, model::ExtentSideFlat(phi2, 1.0f - phi2));
cout << "create_sphere" << endl;
brep::dump(mesh);
    model::transform(mesh, math::rotation_normal({0.0f, 1.0f, 0.0f}, -0.25f * math::PiTwo<float>));
cout << "rotation_normal" << endl;
brep::dump(mesh);
    model::transform(mesh, math::scaling(0.75f, 1.0f, 1.0f));
//    model::transform(mesh, math::translation(0.0f, 10.0f, 0.0f));
cout << "scaling" << endl;
brep::dump(mesh);
// bad last face normals here ...
    brep::clip(mesh, plane, "slice");

    triangulate(mesh);
    write_obj(mesh, dirOut + name);


    name = "tetra_cradle_3_bad.obj";
    cout << name << endl;
    mesh.clear();

    model::create_sphere(mesh, 4, 2, 70.0f, model::ExtentSideCenter(phi2, 1.0f - phi2));
//model::create_sphere(mesh, 4, 2, 70.0f);
cout << "create_sphere" << endl;
brep::dump(mesh);
    model::transform(mesh, math::rotation_normal({0.0f, 1.0f, 0.0f}, 0.25f * math::PiTwo<float>)); // -0.25f
cout << "rotation_normal" << endl;
brep::dump(mesh);
    model::transform(mesh, math::scaling(0.75f, 1.0f, 1.0f));
//    model::transform(mesh, math::translation(0.0f, 10.0f, 0.0f));
cout << "scaling" << endl;
brep::dump(mesh);
    brep::clip(mesh, plane, "slice");

    triangulate(mesh);
    write_obj(mesh, dirOut + name);

    return 0;
}
