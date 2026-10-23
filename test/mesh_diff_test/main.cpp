//------------------------------------------------------------------------------
// mesh_diff_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/common/file_util.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/model/mesh_diff.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_traits = brep::MeshTraits<vector_traits>;
    using mesh_type = brep::Mesh<mesh_traits>;
    using model_type = model::detail::MeshDiffModel<vector_traits>;
    using point_type = model_type::point_type;
    using vector_type = model_type::vector_type;
    using texcoord_type = model_type::texcoord_type;

    using face_type = vector<point_type>;
    using faces_type = vector<face_type>;

    //--------------------------------------------------------------------------
    // A directory for one test case, so test cases running at once never write to the same directory
    filesystem::path scratch_directory(const string& name)
    {
        filesystem::path dir = filesystem::temp_directory_path() / "mesh_diff_test" / name;
        filesystem::create_directories(dir);
        return dir;
    }

    //--------------------------------------------------------------------------
    string counts(const model::Comparison& comparison)
    {
        return std::format("[identical {}, equivalent {}, reversed {}, different {}, unmatched {}/{}, surface {}]",
            comparison.n_identical, comparison.n_equivalent, comparison.n_reversed, comparison.n_different,
            comparison.n_unmatched, comparison.n_unmatched_other, comparison.n_surface_differences);
    }

    //--------------------------------------------------------------------------
    faces_type base_faces()
    {
        return {
            {point_type(0.0f, 0.0f, 0.0f), point_type(1.0f, 0.0f, 0.0f), point_type(0.0f, 1.0f, 0.0f)},
            {point_type(4.0f, 0.0f, 0.0f), point_type(5.0f, 0.0f, 0.0f), point_type(4.0f, 1.0f, 0.0f)},
            {point_type(0.0f, 4.0f, 0.0f), point_type(1.0f, 4.0f, 0.0f), point_type(0.0f, 5.0f, 0.0f)},
            {point_type(4.0f, 4.0f, 0.0f), point_type(5.0f, 4.0f, 0.0f), point_type(4.0f, 5.0f, 0.0f)}
        };
    }

    //--------------------------------------------------------------------------
    model_type make_model(const faces_type& faces, const string& submesh = "object", const string& surface = "body")
    {
        model_type model;

        id_type submesh_id = model.get_submesh_id(submesh);
        id_type surface_id = model.get_surface_id(submesh_id, surface);

        for (const face_type& face : faces)
        {
            id_type vertex_first = model.vertex_count();

            for (const point_type& position : face)
            {
                // Derived from the position so that the attributes travel with the
                // vertex when a loop is rotated or reversed
                model.add_vertex({position, vector_type(0.0f, 0.0f, 1.0f), texcoord_type(position.x(), position.y())});
            }

            model.create_face(surface_id, vertex_first, face.size());
        }

        return model;
    }

    //--------------------------------------------------------------------------
    faces_type rotate_loops(faces_type faces)
    {
        for (face_type& face : faces)
        {
            std::rotate(face.begin(), face.begin() + 1, face.end());
        }

        return faces;
    }

    //--------------------------------------------------------------------------
    faces_type reverse_loops(faces_type faces)
    {
        for (face_type& face : faces)
        {
            std::reverse(face.begin(), face.end());
        }

        return faces;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("comparison")
    {
        set_output_file_lf_endings();
        model_type a = make_model(base_faces());

        {
            model::Comparison c = model::detail::compare_models(a, a);
            CHECK_CASE("identical model", c.identical() && c.n_identical == 4, counts(c));
        }

        {
            faces_type faces = base_faces();
            std::reverse(faces.begin(), faces.end());
            model::Comparison c = model::detail::compare_models(a, make_model(faces));
            CHECK_CASE("face order ignored", c.identical(), counts(c));
        }

        {
            model::Comparison c = model::detail::compare_models(a, make_model(rotate_loops(base_faces())));
            CHECK_CASE("vertex loop rotation ignored", c.identical(), counts(c));
        }

        {
            model::Comparison c = model::detail::compare_models(a, make_model(reverse_loops(base_faces())));
            CHECK_CASE("reversed winding detected", !c.equivalent() && c.n_reversed == 4 && c.n_identical == 0, counts(c));
        }

        {
            // A single ulp of displacement is within the default tolerance
            faces_type faces = base_faces();
            faces[0][1].set_x(std::nextafter(faces[0][1].x(), 2.0f));
            model::Comparison c = model::detail::compare_models(a, make_model(faces));
            CHECK_CASE("position within tolerance is equivalent", c.equivalent() && !c.identical(), counts(c));
        }

        {
            faces_type faces = base_faces();
            faces[0][0].set_x(faces[0][0].x() + 0.5f);
            model::Comparison c = model::detail::compare_models(a, make_model(faces));
            CHECK_CASE("position beyond tolerance is not equivalent", !c.equivalent(), counts(c));
        }

        {
            // Displacing an entire face leaves it with no counterpart on either side
            faces_type faces = base_faces();
            for (point_type& position : faces[0])
            {
                position.set_z(position.z() + 100.0f);
            }

            model::Comparison c = model::detail::compare_models(a, make_model(faces));
            CHECK_CASE("displaced face is unmatched on both sides", c.n_unmatched == 1 && c.n_unmatched_other == 1, counts(c));
        }

        {
            model::Comparison c = model::detail::compare_models(a, make_model(base_faces(), "object", "other"));
            CHECK_CASE("surface name difference", c.n_surface_differences == 4 && !c.identical(), counts(c));
        }

        {
            model::Comparison c = model::detail::compare_models(a, make_model(base_faces(), "other", "body"));
            CHECK_CASE("submesh name difference", c.n_surface_differences == 4 && !c.identical(), counts(c));
        }

        {
            model::Options options;
            options.compare_surfaces = false;
            model::Comparison c = model::detail::compare_models(a, make_model(base_faces(), "object", "other"), options);
            CHECK_CASE("surface comparison can be disabled", c.identical(), counts(c));
        }

        {
            faces_type faces = base_faces();
            faces.push_back({point_type(9.0f, 9.0f, 0.0f), point_type(10.0f, 9.0f, 0.0f), point_type(9.0f, 10.0f, 0.0f)});
            model::Comparison c = model::detail::compare_models(a, make_model(faces));
            CHECK_CASE("extra face on the right", !c.comparable() && c.n_unmatched_other == 1, counts(c));
        }

        {
            faces_type faces = base_faces();
            faces.pop_back();
            model::Comparison c = model::detail::compare_models(a, make_model(faces));
            CHECK_CASE("missing face on the right", !c.comparable() && c.n_unmatched == 1, counts(c));
        }

        {
            model_type empty;
            model::Comparison c = model::detail::compare_models(empty, empty);
            CHECK_CASE("empty models are identical", c.identical() && c.n_faces == 0, counts(c));
        }

        {
            // A quadrilateral sharing the triangle's centroid still cannot correspond to it
            faces_type faces = base_faces();
            faces[0] = {
                point_type(0.0f, 0.0f, 0.0f),
                point_type(2.0f / 3.0f, 0.0f, 0.0f),
                point_type(2.0f / 3.0f, 2.0f / 3.0f, 0.0f),
                point_type(0.0f, 2.0f / 3.0f, 0.0f)
            };

            model_type b = make_model(faces);
            model::Comparison c = model::detail::compare_models(a, b);
            CHECK_CASE("differing vertex count at the same centroid is unmatched",
                c.n_unmatched == 1 && c.n_unmatched_other == 1 && c.n_identical == 3, counts(c));
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A face vertex reference in whichever of the four forms the flags select
    string vertex_reference(int v, int vt, bool texcoords, bool normals)
    {
        string s = std::to_string(v);

        if (texcoords)
        {
            s += "/" + std::to_string(vt);
        }
        else if (normals)
        {
            s += "/";
        }

        if (normals)
        {
            s += "/1";
        }

        return s;
    }

    //--------------------------------------------------------------------------
    // Laid out the way quetzal writes obj files: all vertex data first, then the
    // object, then each group immediately followed by its own faces
    string obj_source(bool texcoords, bool normals)
    {
        string s =
            "# Vertex Positions\n"
            "v 0 0 0\n"
            "v 1 0 0\n"
            "v 0 1 0\n"
            "v 4 0 0\n"
            "v 5 0 0\n"
            "v 4 1 0\n";

        if (texcoords)
        {
            s += "\n# Texture Coordinates\n"
                "vt 0 0\n"
                "vt 1 0\n"
                "vt 0 1\n";
        }

        if (normals)
        {
            s += "\n# Vertex Normals\n"
                "vn 0 0 1\n";
        }

        s += "\n# Faces\n"
            "o object\n";

        for (int face = 0; face < 2; ++face)
        {
            s += std::format("g body{}\n", face);

            s += "f";
            for (int i = 1; i <= 3; ++i)
            {
                s += " " + vertex_reference(face * 3 + i, i, texcoords, normals);
            }

            s += "\n";
        }

        return s;
    }

    //--------------------------------------------------------------------------
    // One model in both layouts, to pin that the reader accepts either. Declaring
    // the object and group ahead of the vertex data is unusual but valid
    const string obj_declarations_last =
        "# Vertex Positions\n"
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "v 4 0 0\n"
        "v 5 0 0\n"
        "v 4 1 0\n"
        "\n# Vertex Normals\n"
        "vn 0 0 1\n"
        "\n# Faces\n"
        "o object\n"
        "g body\n"
        "f 1//1 2//1 3//1\n"
        "f 4//1 5//1 6//1\n";

    const string obj_declarations_first =
        "o object\n"
        "g body\n"
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "v 4 0 0\n"
        "v 5 0 0\n"
        "v 4 1 0\n"
        "vn 0 0 1\n"
        "f 1//1 2//1 3//1\n"
        "f 4//1 5//1 6//1\n";

    //--------------------------------------------------------------------------
    filesystem::path write_file(const filesystem::path& dir, const string& name, const string& content)
    {
        filesystem::path path = dir / name;

        ofstream ofs(path, ios_base::binary);
        ofs << content;
        ofs.close();

        return path;
    }

    //--------------------------------------------------------------------------
    string to_crlf(const string& s)
    {
        string result;
        for (char c : s)
        {
            if (c == '\n')
            {
                result += '\r';
            }

            result += c;
        }

        return result;
    }

    //--------------------------------------------------------------------------
    void check_reader(const filesystem::path& dir)
    {
        // v/vt/vn, v//vn, v/vt, and v must all read and compare against themselves
        struct Form
        {
            string name;
            bool texcoords;
            bool normals;
        };

        const vector<Form> forms = {
            {"full", true, true},
            {"no_vt", false, true},
            {"no_vn", true, false},
            {"pos_only", false, false}
        };

        for (const Form& form : forms)
        {
            filesystem::path path = write_file(dir, form.name + ".obj", obj_source(form.texcoords, form.normals));

            model::Comparison c = model::mesh_diff<vector_traits>(path, path);
            CHECK_CASE("reference form " + form.name, c.n_faces == 2 && c.identical(), counts(c));
        }

        {
            model::Comparison c = model::mesh_diff<vector_traits>(dir / "full.obj", dir / "no_vn.obj");
            const model::FaceDifferences& d = c.differences[0].differences;
            CHECK_CASE("missing vn differs in all three normals only", d.normals == 3 && d.texcoords == 0 && d.positions == 0,
                std::format("[{} position, {} normal, {} texcoord]", d.positions, d.normals, d.texcoords));
        }

        {
            model::Comparison c = model::mesh_diff<vector_traits>(dir / "full.obj", dir / "no_vt.obj");
            const model::FaceDifferences& d = c.differences[0].differences;
            // The first vertex has vt 0 0, so this also tests a supplied attribute equal to the default
            CHECK_CASE("missing vt differs in all three texcoords only", d.texcoords == 3 && d.normals == 0 && d.positions == 0,
                std::format("[{} position, {} normal, {} texcoord]", d.positions, d.normals, d.texcoords));
        }

        {
            filesystem::path path = write_file(dir, "crlf.obj", to_crlf(obj_source(true, true)));
            model::Comparison c = model::mesh_diff<vector_traits>(dir / "full.obj", path);
            CHECK_CASE("crlf reads the same as lf", c.identical(), counts(c));
        }

        {
            // A cr carried into a name would leave the surface unmatched by name
            model_type crlf = model::detail::read_obj_model<model_type>(dir / "crlf.obj");
            bool b = crlf.submeshes().size() == 1 && crlf.submeshes()[0].name == "object"
                && crlf.surfaces().size() == 2
                && crlf.surfaces()[0].name == "body0" && crlf.surfaces()[1].name == "body1";
            CHECK_CASE("crlf leaves no cr in submesh and surface names", b);
        }

        {
            // Groups interleaved with their faces must be read into the right surface
            model_type full = model::detail::read_obj_model<model_type>(dir / "full.obj");
            bool b = full.surfaces().size() == 2
                && full.surfaces()[0].face_ids.size() == 1 && full.surfaces()[1].face_ids.size() == 1
                && full.face(full.surfaces()[0].face_ids[0]).surface_id == 0
                && full.face(full.surfaces()[1].face_ids[0]).surface_id == 1;
            CHECK_CASE("each group takes the faces that follow it", b,
                std::format("[{} surfaces]", full.surfaces().size()));
        }

        {
            filesystem::path last = write_file(dir, "declarations_last.obj", obj_declarations_last);
            filesystem::path first = write_file(dir, "declarations_first.obj", obj_declarations_first);

            model::Comparison c = model::mesh_diff<vector_traits>(last, first);
            CHECK_CASE("object and group declared before the vertex data reads the same", c.identical(), counts(c));
        }

        return;
    }

    //--------------------------------------------------------------------------
    void check_overloads(const filesystem::path& dir)
    {
        filesystem::path path = write_file(dir, "full.obj", obj_source(true, true));

        mesh_type mesh;
        model::read_obj(mesh, path);

        mesh_type mesh_other;
        model::read_obj(mesh_other, path);

        // read_obj normalizes vertex normals, so a mesh only matches the file
        // it was read from once the normals are left out of the comparison
        model::Options options;
        options.compare_normals = false;

        {
            model::Comparison c = model::mesh_diff<vector_traits>(path, path);
            CHECK_CASE("file against file", c.n_faces == 2 && c.identical(), counts(c));
        }

        {
            model::Comparison c = model::mesh_diff(mesh, mesh_other);
            CHECK_CASE("mesh against mesh", c.n_faces == 2 && c.identical(), counts(c));
        }

        {
            model::Comparison c = model::mesh_diff(path, mesh, options);
            CHECK_CASE("file against mesh", c.n_faces == 2 && c.identical(), counts(c));
        }

        {
            model::Comparison c = model::mesh_diff(mesh, path, options);
            CHECK_CASE("mesh against file", c.n_faces == 2 && c.identical(), counts(c));
        }

        {
            // The adapter has to reproduce the grouping, not just the geometry
            model_type adapted = model::detail::to_model(mesh);
            model_type direct = model::detail::read_obj_model<model_type>(path);
            CHECK_CASE("adapted grouping matches the file",
                adapted.face_count() == direct.face_count()
                    && adapted.surfaces().size() == direct.surfaces().size()
                    && adapted.submeshes().size() == direct.submeshes().size(),
                std::format("[{} faces, {} surfaces, {} submeshes]", adapted.face_count(), adapted.surfaces().size(), adapted.submeshes().size()));
        }

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("obj reader")
    {
        set_output_file_lf_endings();

        filesystem::path dir = scratch_directory("obj_reader");
        check_reader(dir);
        filesystem::remove_all(dir);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("mesh_diff overloads")
    {
        set_output_file_lf_endings();

        filesystem::path dir = scratch_directory("overloads");
        check_overloads(dir);
        filesystem::remove_all(dir);

        return;
    }

} // namespace
