//------------------------------------------------------------------------------
// contour_function_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/geometry/Properties.hpp"
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

    const unsigned int s_nz = 200;
    const float s_dz = 75.0f;

    const float rr0 = 30.0f;
    const float rr1 = 10.0f;
    const int n1 = 8;
    const int n2 = 19;

    //--------------------------------------------------------------------------
    float radius_test3(float t)
    {
        float theta = -math::PiFourth<value_type> + t * 3 * math::PiFourth<value_type>;
        return rr0 * cos(theta) - rr1 * sin(t * n1 * math::Pi<value_type>) * sin(t * n2 * math::Pi<value_type>) * sin(t * math::Pi<value_type>);
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    string name;
    mesh_type mesh;

    name = "contour_function_test_0";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, 6, s_nz, [](float) -> float { return 50.0f; }, [](float t) -> float { return t * s_dz; });
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name + ".obj");

    name = "contour_function_test_3";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, 6, s_nz, radius_test3, [](float t) -> float { return t * s_dz; });
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name + ".obj");

    return 0;
}
