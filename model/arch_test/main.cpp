//------------------------------------------------------------------------------
// arch_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/math/math_util_dx.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include "quetzal/model/mesh_attributes.hpp"
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
    template<typename M>
    void create_column(M& mesh, size_t nAzimuth, size_t nz, value_type radius, value_type dz, value_type azimuth0, value_type azimuth1)
    {
        M m;
        model::create_cylinder(m, nAzimuth, nz, radius, radius, 0.0f, dz, azimuth0, azimuth1);
        mesh.insert(m);

        mesh.check();
        return;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void create_columns(M& mesh, size_t nAzimuth, size_t nz, value_type radius, value_type dz, value_type azimuth, value_type separation)
    {
        model::ExtentSideFlat<float> extentAzimuth(0.125f, 1.125f);

        M m;
        model::create_cylinder(m, nAzimuth, nz, radius, radius, 0.0f, dz, 0.0f, azimuth, extentAzimuth);
        model::transform(m, math::translation(-separation / 2.0f, 0.0f, 0.0f));
        mesh.insert(m);

        m.clear();
        model::create_cylinder(m, nAzimuth, nz, radius, radius, 0.0f, dz, 0.0f, -azimuth, extentAzimuth);
        model::transform(m, math::translation(separation / 2.0f, 0.0f, 0.0f));
        mesh.insert(m);

        mesh.check();
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

    name = "column.obj";
    cout << name << endl;
    mesh.clear();
    create_column(mesh, 4, 12, 10.0f, 60.0f, 0.0f, 0.5f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "columns.obj";
    cout << name << endl;
    mesh.clear();
    create_columns(mesh, 4, 12, 10.0f, 80.0f, 0.5, 60.0f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    return 0;
}
