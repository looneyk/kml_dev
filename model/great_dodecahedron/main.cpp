//------------------------------------------------------------------------------
// great_dodecahedron
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/math_util.hpp"
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
    using vector_traits = math::CartesianTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;

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

    double a = 4.0 / sqrt(10.0 + 2.0 * sqrt(5.0));
    double e = a * sin(math::PiFifth<double>) / sin(3.0 * math::PiFifth<double>);
    value_type h = static_cast<value_type>(sqrt(e * e - a * a / 3.0));

    value_type radius = 50.0f;

    name = "great_dodecahedron_vertex";
    cout << name << endl;
    mesh.clear();
    model::create_icosahedron(mesh, name, radius, true, true);
    for (size_t idFace = 0; idFace < 20; ++idFace)
    {
        const auto& face = mesh.face(idFace);
        auto position = vertex_centroid(face) - face.attributes().normal() * radius * h;
        triangulate_central_vertex(mesh, idFace, position, false);
    }
    model::write_obj(mesh, to_filename(name));

    name = "great_dodecahedron_face";
    cout << name << endl;
    mesh.clear();
    model::create_icosahedron(mesh, name, radius, false, true);
    for (size_t idFace = 0; idFace < 20; ++idFace)
    {
        const auto& face = mesh.face(idFace);
        auto position = vertex_centroid(face) - face.attributes().normal() * radius * h;
        triangulate_central_vertex(mesh, idFace, position, false);
    }
    model::write_obj(mesh, to_filename(name));

    validator.report_results();
    return 0;
}
