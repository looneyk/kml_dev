//------------------------------------------------------------------------------
// wavefront_obj_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/model/mesh_diff.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include "quetzal/testing/named_checks.hpp"
#include "quetzal/testing/reference_files.hpp"
#include <doctest/doctest.h>
#include <filesystem>
#include <format>
#include <functional>
#include <string>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
//    using value_type = double;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;

    //--------------------------------------------------------------------------
    struct Primitive
    {
        string name;
        function<void(mesh_type& mesh, const string& name)> create;
    };

    //--------------------------------------------------------------------------
    string comparison_counts(const model::Comparison& comparison)
    {
        return format("{} faces, {} rewritten: {} identical, {} equivalent, {} reversed, {} different, {} unmatched, {} surface differences",
            comparison.n_faces, comparison.n_faces_other, comparison.n_identical, comparison.n_equivalent, comparison.n_reversed,
            comparison.n_different, comparison.n_unmatched, comparison.n_surface_differences);
    }

    //--------------------------------------------------------------------------
    // Each primitive is written, read back, and written again; both files match their references, and the rewritten mesh is
    // equivalent to the original
    TEST_CASE("primitives")
    {
        const Primitive primitives[]
        {
            {"box", [](mesh_type& mesh, const string& name) { model::create_box(mesh, name, 5.0f, 10.0f, 20.0f); triangulate(mesh); }},
            {"cylinder", [](mesh_type& mesh, const string& name) { model::create_cylinder(mesh, name, 12, 10, 5.0f, 5.0f, -10.0f, 10.0f);
                triangulate(mesh); }},
            {"cylinder_1", [](mesh_type& mesh, const string& name) { model::create_cylinder(mesh, name, 12, 1, 5.0f, 5.0f, -10.0f, 10.0f);
                triangulate(mesh); }},
            {"cone_up", [](mesh_type& mesh, const string& name) { model::create_cylinder(mesh, name, 12, 6, 7.5f, 0.0f, -10.0f, 10.0f);
                triangulate(mesh); }},
            {"cone_down", [](mesh_type& mesh, const string& name) { model::create_cylinder(mesh, name, 12, 6, 0.0f, 7.5f, -10.0f, 10.0f);
                triangulate(mesh); }},
            {"cone_up_1", [](mesh_type& mesh, const string& name) { model::create_cylinder(mesh, name, 6, 1, 7.5f, 0.0f, -10.0f, 10.0f);
                triangulate(mesh); }},
            {"cone_down_1", [](mesh_type& mesh, const string& name) { model::create_cylinder(mesh, name, 6, 1, 0.0f, 7.5f, -10.0f, 10.0f);
                triangulate(mesh); }},
            {"pyramid_up", [](mesh_type& mesh, const string& name) { model::create_prism(mesh, name, 12, 6, 7.5f, 0.0f, -10.0f, 10.0f);
                triangulate(mesh); }},
            {"pyramid_down", [](mesh_type& mesh, const string& name) { model::create_prism(mesh, name, 12, 6, 0.0f, 7.5f, -10.0f, 10.0f);
                triangulate(mesh); }},
            {"pyramid_up_1", [](mesh_type& mesh, const string& name) { model::create_prism(mesh, name, 6, 1, 7.5f, 0.0f, -10.0f, 10.0f);
                triangulate(mesh); }},
            {"pyramid_down_1", [](mesh_type& mesh, const string& name) { model::create_prism(mesh, name, 6, 1, 0.0f, 7.5f, -10.0f, 10.0f);
                triangulate(mesh); }},
            {"sphere", [](mesh_type& mesh, const string& name) { model::create_sphere(mesh, name, 24, 12, 10.0f); triangulate(mesh); }},
            {"torus", [](mesh_type& mesh, const string& name) { model::create_torus(mesh, name, 24, 12, 15.0f, 7.5f); triangulate(mesh); }},
            {"tetrahedron", [](mesh_type& mesh, const string& name) { model::create_tetrahedron(mesh, name, 10.0f); }},
            {"octahedron_vertex", [](mesh_type& mesh, const string& name) { model::create_octahedron(mesh, name, 10.0f, true); }},
            {"octahedron_face", [](mesh_type& mesh, const string& name) { model::create_octahedron(mesh, name, 10.0f, false); }},
            {"dodecahedron", [](mesh_type& mesh, const string& name)
                {
                    id_type submesh_id = model::create_dodecahedron(mesh, name, 10.0f);
                    triangulate_central_vertices(mesh.submesh(submesh_id));
                    return;
                }},
            {"icosahedron_vertex", [](mesh_type& mesh, const string& name) { model::create_icosahedron(mesh, name, 10.0f); }},
            {"icosahedron_face", [](mesh_type& mesh, const string& name) { model::create_icosahedron(mesh, name, 10.0f, false, {{"smooth", "false"}}); }}
        };

        filesystem::create_directories(testing::out_directory);

        for (const Primitive& primitive : primitives)
        {
            SUBCASE(primitive.name.c_str())
            {
                const string filename = primitive.name + ".obj";
                const string filename_rewrite = primitive.name + "_.obj";

                mesh_type mesh;
                primitive.create(mesh, primitive.name);
                model::write_obj(mesh, testing::out_directory / filename);

                mesh_type mesh_read;
                model::read_obj(mesh_read, testing::out_directory / filename);
                model::write_obj(mesh_read, testing::out_directory / filename_rewrite);

                testing::check_reference(filename);
                testing::check_reference(filename_rewrite);

                const model::Comparison comparison = model::mesh_diff<vector_traits>(testing::out_directory / filename,
                    testing::out_directory / filename_rewrite);
                CHECK_CASE("rewrite is equivalent", comparison.equivalent() && comparison.n_surface_differences == 0,
                    comparison_counts(comparison));
            }
        }

        return;
    }

} // namespace
