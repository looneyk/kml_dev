//------------------------------------------------------------------------------
// suburbmesh
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include "quetzal/model/stl_io.hpp"
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

    static const string dirIn = "in/";
    static const string dirOut = "out/";
    static const string dirReference = "reference/";

    //--------------------------------------------------------------------------
    string to_filename_obj_out(const string& name)
    {
        return dirOut + name + ".obj";
    }

    //--------------------------------------------------------------------------
    string to_filename_stl_in(const string& name)
    {
        return dirIn + name + ".stl";
    }

    //--------------------------------------------------------------------------
    string to_filename_stl_out(const string& name)
    {
        return dirOut + name + ".stl";
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

    name = "housemesh_mloone21_11719";
    cout << name << endl;
    mesh.clear();
    cout << "read_stl" << endl;
    model::read_stl(mesh, to_filename_stl_in(name));

    auto position0 = mesh.vertex(0).attributes().position();
    position0 *= 25.4f;
    value_type xMin = position0.x();
    value_type xMax = position0.x();
    value_type yMin = position0.y();
    value_type yMax = position0.y();
    value_type zMin = position0.z();
    value_type zMax = position0.z();

    for (auto& vertex : mesh.vertices())
    {
        auto& position = vertex.attributes().position();
        position *= 25.4f;
        if (position.x() < xMin) xMin = position.x();
        if (position.x() > xMax) xMin = position.x();
        if (position.y() < yMin) yMin = position.y();
        if (position.y() > yMax) yMax = position.y();
        if (position.z() < zMin) zMin = position.z();
        if (position.z() > zMax) zMax = position.z();
    }

    cout << "x " << xMin << " .. " << xMax << endl;
    cout << "y " << yMin << " .. " << yMax << endl;
    cout << "z " << zMin << " .. " << zMax << endl;

    value_type xShift = -(xMin + xMax) / 2.0f;
    value_type yShift = -(yMin + yMax) / 2.0f;
    value_type zShift = -zMin + 0.75f; // ...

    for (auto& vertex : mesh.vertices())
    {
        auto& position = vertex.attributes().position();

        position.x() += xShift;
        position.y() += yShift;
        position.z() += zShift;
    }

    position0 = mesh.vertex(0).attributes().position();
    xMin = position0.x();
    xMax = position0.x();
    yMin = position0.y();
    yMax = position0.y();
    zMin = position0.z();
    zMax = position0.z();

    for (auto& vertex : mesh.vertices())
    {
        auto& position = vertex.attributes().position();
        if (position.x() < xMin) xMin = position.x();
        if (position.x() > xMax) xMin = position.x();
        if (position.y() < yMin) yMin = position.y();
        if (position.y() > yMax) yMax = position.y();
        if (position.z() < zMin) zMin = position.z();
        if (position.z() > zMax) zMax = position.z();
    }

    cout << "x " << xMin << " .. " << xMax << endl;
    cout << "y " << yMin << " .. " << yMax << endl;
    cout << "z " << zMin << " .. " << zMax << endl;

    mesh_type m;
    model::create_box(m, "base", xMax - xMin + 10.0f, yMax - yMin + 10.0f, 0.5f); // ...
    mesh.insert(m);

    name = "housemail";
    cout << name << endl;
    cout << "write_obj" << endl;
    model::write_obj(mesh, to_filename_obj_out(name));
/*
    cout << "write_stl text" << endl;
    model::write_stl(mesh, to_filename_stl_out(name + "_text"), name, false);
    cout << "write_stl binary" << endl;
    model::write_stl(mesh, to_filename_stl_out(name), name, true);

    mesh.clear();
    cout << "read_obj" << endl;
    model::read_obj(mesh, to_filename_obj_out(name));
    cout << "write_obj" << endl;
    model::write_obj(mesh, to_filename_obj_out(name + "_rewrite"));

    name = "suburbmesh";
    cout << name << endl;
    mesh.clear();
    cout << "read_stl" << endl;
    model::read_stl(mesh, to_filename_stl_in(name));
    cout << "write_obj" << endl;
    model::write_obj(mesh, to_filename_obj_out(name));
    cout << "write_stl text" << endl;
    model::write_stl(mesh, to_filename_stl_out(name + "_text"), name, false);
    cout << "write_stl binary" << endl;
    model::write_stl(mesh, to_filename_stl_out(name), name);
*/
    validator.report_results();
cin.ignore();
    return 0;
}
