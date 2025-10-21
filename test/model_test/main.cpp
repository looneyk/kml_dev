//------------------------------------------------------------------------------
// model_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/Attributes.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/primitives.hpp"
#include "quetzal/model/obj_io.hpp"
#include <string>
#include <iostream>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
//    using value_type = double;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
//    using point_type = typename mesh_type::point_type;
//    using vector_type = point_type::vector_type;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "model_test";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    FileValidator validator(dir_out, dir_ref, app_name);

    string name;
    mesh_type mesh;

    name = "cylinder";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 5, 2, 20.0f, 10.0f, -5.0f, 25.0f);
//    triangulate(mesh);
//    model::write_obj(mesh, to_filename(name));

    cout << "0" << endl;
    for (const auto& vertex : mesh.vertices())
    {
        if (!vertex.deleted())
        {
            cout << "v " << vertex.id() << " " << vertex.halfedge_count() << " " << vertex.halfedges().size() << endl;
        }
    }

    mesh.delete_face(11);
    cout << "1" << endl;
    for (const auto& vertex : mesh.vertices())
    {
        if (!vertex.deleted())
        {
            cout << "v " << vertex.id() << " " << vertex.halfedge_count() << " " << vertex.halfedges().size() << endl;
        }
    }

    mesh.delete_face(6);
    cout << "2" << endl;
    for (const auto& vertex : mesh.vertices())
    {
        if (!vertex.deleted())
        {
            cout << "v " << vertex.id() << " " << vertex.halfedge_count() << " " << vertex.halfedges().size() << endl;
        }
    }

//    mesh.delete_face(3);
    cout << endl;

    for (auto& face : mesh.faces())
    {
        cout << "f " << face.id() << " " << face.attributes().normal() << endl;
    }

    cout << endl;

    for (auto& face : mesh.faces())
    {
        for (auto& halfedge : face.halfedges())
        {
            auto normal = model::surface_vertex_normal(halfedge.vertex());
            halfedge.vertex_attributes().set_normal(normal);
            cout << "f " << face.id() << " h " << halfedge.id() << " v " << halfedge.vertex_id() << " (" << halfedge.vertex_attributes().position() << ") n (" << normal << ")" << endl;
        }
    }

    cout << endl;

    for (const auto& face : mesh.faces())
    {
        assert(!face.deleted());
        for (const auto& halfedge : face.halfedges())
        {
            assert(!halfedge.deleted());
            size_t n = halfedge.vertex().halfedge_count();
            size_t i = 0;

            for (const auto& h : halfedge.vertex().halfedges())
            {
                assert(!h.deleted());
                cout << "s " << h.surface_id() << " f " << face.id() << " h " << halfedge.id() << " vh " << h.id() << " v " << h.vertex_id() << " (" << h.vertex_attributes().position() << ") n (" << h.vertex_attributes().normal() << ")" << endl;
                cout << i << " / " << n << endl;
                ++i;
            }
        }
    }
/* results, vertex halfedge_count
v 0 3
v 1 3
v 2 3
v 3 3
v 4 3
v 5 3
v 6 3
v 7 3
v 8 3
v 9 3
v 10 3
v 11 3
v 12 4
v 13 4
v 14 4
v 15 4
v 16 3
v 17 3
v 18 4
v 19 4
v 20 4
v 22 3
v 23 4
v 24 4
v 25 3
v 27 4
v 28 4
v 29 4
v 30 1
v 32 2
v 33 2
v 34 2
v 35 2
v 37 1
v 38 2
v 39 2
v 40 3
v 41 3
v 42 3
v 43 3
v 44 3
*/
    validator.report_results();
    return 0;
}
