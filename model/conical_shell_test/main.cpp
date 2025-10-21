//------------------------------------------------------------------------------
// intersection_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/math/math_util_dx.hpp" // ...
#include "quetzal/model/primitives.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <iostream>
#include <string>
#include <cassert>

#include "quetzal/common/Timestamp.hpp"

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

Timestamp t0;

    string name;
    mesh_type mesh;

    name = "torus";
    cout << name << endl;
    mesh.clear();
    model::create_torus(mesh, name, 24, 12, 15.0f, 15.0f, 7.5f, 7.5f, 0.0f, 0.0f, model::ExtentEndsFlat(0.0f, 1.0f), model::ExtentEndsFlat(0.0f, 1.0f));
Timestamp t1;
    triangulate(mesh);
Timestamp t2;
    model::write_obj(mesh, to_filename(name));
Timestamp t3;

    name = "conical_shell";
    cout << name << endl;
    mesh.clear();
    model::create_torus(mesh, name, 240, 24, 15.0f, 1.0f, 10.0f, 2.0f, 0.0f, 60.0f, model::ExtentEndsFlat(0.0f, 6.0f), model::ExtentEndsFlat(0.0f, 1.0f));
Timestamp t4;
    triangulate(mesh);
Timestamp t5;
    model::write_obj(mesh, to_filename(name));
Timestamp t6;

    validator.report_results();
Timestamp t7;

cout << "1 torus " << (t1 - t0) << endl;
cout << "1 triangulate " << (t2 - t1) << endl;
cout << "1 write " << (t3 - t2) << endl;
cout << "2 torus " << (t4 - t3) << endl;
cout << "2 triangulate " << (t5 - t4) << endl;
cout << "2 write " << (t6 - t5) << endl;
cout << "report " << (t7 - t6) << endl;

    return 0;
}
