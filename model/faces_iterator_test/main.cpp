//------------------------------------------------------------------------------
// faces_iterator_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/id.hpp"
#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/model/primitives.hpp"
#include <string>
#include <iostream>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::CartesianTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    mesh_type mesh;
    model::create_tetrahedron(mesh, "tetrahedron", 100.0f);

    const auto submesh = mesh.submesh("tetrahedron");

    cout << "submesh surfaces iterator" << endl;
    for (auto i = submesh.surfaces().begin(); i != submesh.surfaces().end(); ++i)
    {
        cout << "submesh " << submesh.id() << " " << submesh.name() << " surface " << i->id() << " " << i->name() << endl;
    }

    cout << "submesh surfaces range" << endl;
    for (const auto surface : submesh.surfaces())
    {
        cout << "submesh " << submesh.id() << " " << submesh.name() << " surface " << surface.id() << " " << surface.name() << endl;
    }

    cout << "submesh surfaces front" << endl;
    cout << submesh.surfaces().front();

    cout << "submesh face_ids" << endl;
    for (brep::id_type idFace : submesh.face_ids())
    {
        cout << "submesh " << submesh.id() << " " << submesh.name() << " face " << idFace << endl;
    }

    cout << "submesh faces iterator" << endl;
    for (auto i = submesh.faces().begin(); i != submesh.faces().end(); ++i)
    {
        cout << "submesh " << submesh.id() << " " << submesh.name() << " face " << i->id() << endl;
    }

    cout << "submesh faces range" << endl;
    for (const auto face : submesh.faces())
    {
        cout << "submesh " << submesh.id() << " " << submesh.name() << " face " << face.id() << endl;
    }

    cout << "submesh faces front" << endl;
    cout << submesh.faces().front();

    const auto surface = mesh.surface("tetrahedron/body0");

    cout << "surface faces iterator" << endl;
    for (auto i = surface.faces().begin(); i != surface.faces().end(); ++i)
    {
        cout << "surface " << surface.id() << " " << surface.name() << " face " << i->id() << endl;
    }

    cout << "surface faces range" << endl;
    for (const auto face : surface.faces())
    {
        cout << "surface " << surface.id() << " " << surface.name() << " face " << face.id() << endl;
    }

    cout << "surface faces front" << endl;
    cout << surface.faces().front();

    string s;
    getline(cin, s);

    return 0;
}
