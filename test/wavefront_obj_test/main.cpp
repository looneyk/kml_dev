//------------------------------------------------------------------------------
// wavefront_obj_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/model/mesh_diff.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <vector>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
//    using value_type = double;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;

    static const string dirOut = "out/";
    static const string dirReference = "reference/";

    //--------------------------------------------------------------------------
    string to_filename(const string& name)
    {
        return dirOut + name + ".obj";
    }

    //--------------------------------------------------------------------------
    string to_filename_rewrite(const string& name)
    {
        return to_filename(name + "_");
    }

    //--------------------------------------------------------------------------
    enum class RewriteMatch
    {
        identical,
        equivalent,
        different,
        error
    };

    //--------------------------------------------------------------------------
    const string& to_string(RewriteMatch match)
    {
        static const string identical = "identical";
        static const string equivalent = "equivalent";
        static const string different = "different";
        static const string error = "error";

        switch (match)
        {
            case RewriteMatch::identical: return identical;
            case RewriteMatch::equivalent: return equivalent;
            case RewriteMatch::different: return different;
            case RewriteMatch::error: break;
        }

        return error;
    }

    //--------------------------------------------------------------------------
    struct RewriteComparison
    {
        string filename;
        model::Comparison comparison;
        string error;
    };

    //--------------------------------------------------------------------------
    RewriteMatch rewrite_match(const RewriteComparison& rewrite_comparison)
    {
        if (!rewrite_comparison.error.empty())
        {
            return RewriteMatch::error;
        }
        else if (rewrite_comparison.comparison.identical())
        {
            return RewriteMatch::identical;
        }
        else if (rewrite_comparison.comparison.equivalent() && rewrite_comparison.comparison.n_surface_differences == 0)
        {
            return RewriteMatch::equivalent;
        }

        return RewriteMatch::different;
    }

    //--------------------------------------------------------------------------
    string comparison_note(const RewriteComparison& rewrite_comparison)
    {
        if (!rewrite_comparison.error.empty())
        {
            return rewrite_comparison.error;
        }

        const model::Comparison& comparison = rewrite_comparison.comparison;
        string note;

        if (comparison.n_faces_other != comparison.n_faces)
        {
            note = format("{} faces rewritten", comparison.n_faces_other);
        }

        if (comparison.n_surface_differences > 0)
        {
            if (!note.empty())
            {
                note += ", ";
            }

            note += format("{} surface differences", comparison.n_surface_differences);
        }

        return note;
    }

    //--------------------------------------------------------------------------
    void rewrite(const string& name)
    {
        mesh_type mesh;
        model::read_obj(mesh, to_filename(name));
        model::write_obj(mesh, to_filename_rewrite(name));
        return;
    }

    //--------------------------------------------------------------------------
    RewriteComparison compare_files(const string& filename, const string& filename_rewrite)
    {
        RewriteComparison rewrite_comparison;
        rewrite_comparison.filename = filename;

        try
        {
            rewrite_comparison.comparison = model::mesh_diff<vector_traits>(filename, filename_rewrite);
        }
        catch (const std::exception& e)
        {
            rewrite_comparison.error = e.what();
        }

        return rewrite_comparison;
    }

    //--------------------------------------------------------------------------
    void report_comparisons(const vector<RewriteComparison>& comparisons)
    {
        constexpr const char* row_format = "{:<20}{:>7}{:>11}{:>11}{:>10}{:>10}{:>10}  {}";

        cout << endl << "Rewritten files compared to the files they were read from" << endl;
        cout << format(row_format, "name", "faces", "identical", "equivalent", "reversed", "different", "unmatched", "result") << endl;

        size_t identical_count = 0;
        size_t equivalent_count = 0;
        size_t different_count = 0;
        size_t error_count = 0;

        for (const auto& rewrite_comparison : comparisons)
        {
            RewriteMatch match = rewrite_match(rewrite_comparison);

            switch (match)
            {
                case RewriteMatch::identical: ++identical_count; break;
                case RewriteMatch::equivalent: ++equivalent_count; break;
                case RewriteMatch::different: ++different_count; break;
                case RewriteMatch::error: ++error_count; break;
            }

            string result = to_string(match);
            string note = comparison_note(rewrite_comparison);
            if (!note.empty())
            {
                result += ": " + note;
            }

            const model::Comparison& comparison = rewrite_comparison.comparison;
            cout << format(row_format, filesystem::path(rewrite_comparison.filename).stem().string(), comparison.n_faces, comparison.n_identical,
                comparison.n_equivalent, comparison.n_reversed, comparison.n_different, comparison.n_unmatched, result) << endl;
        }

        cout << format("{} files: {} identical, {} equivalent, {} different, {} error",
            comparisons.size(), identical_count, equivalent_count, different_count, error_count) << endl;
        return;
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
    id_type submesh_id = nullid;
    vector<RewriteComparison> comparisons;

    name = "box";
    cout << name << endl;
    mesh.clear();
    model::create_box(mesh, name, 5.0f, 10.0f, 20.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));
    
    name = "cylinder";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 12, 10, 5.0f, 5.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    name = "cylinder_1";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 12, 1, 5.0f, 5.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    name = "cone_up";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 12, 6, 7.5f, 0.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    name = "cone_down";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 12, 6, 0.0f, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    name = "cone_up_1";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 6, 1, 7.5f, 0.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    name = "cone_down_1";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 6, 1, 0.0f, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    name = "pyramid_up";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 12, 6, 7.5f, 0.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    name = "pyramid_down";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 12, 6, 0.0f, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    name = "pyramid_up_1";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 6, 1, 7.5f, 0.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    name = "pyramid_down_1";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 6, 1, 0.0f, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    name = "sphere";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 24, 12, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    name = "torus";
    cout << name << endl;
    mesh.clear();
    model::create_torus(mesh, name, 24, 12, 15.0f, 7.5f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    name = "tetrahedron";
    cout << name << endl;
    mesh.clear();
    model::create_tetrahedron(mesh, name, 10.0f);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    name = "octahedron_vertex";
    cout << name << endl;
    mesh.clear();
    model::create_octahedron(mesh, name, 10.0f, true);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    name = "octahedron_face";
    cout << name << endl;
    mesh.clear();
    model::create_octahedron(mesh, name, 10.0f, false);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    name = "dodecahedron";
    cout << name << endl;
    mesh.clear();
    submesh_id = model::create_dodecahedron(mesh, name, 10.0f);
    triangulate_central_vertices(mesh.submesh(submesh_id));
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    name = "icosahedron_vertex";
    cout << name << endl;
    mesh.clear();
    model::create_icosahedron(mesh, name, 10.0f);
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    name = "icosahedron_face";
    cout << name << endl;
    mesh.clear();
    model::create_icosahedron(mesh, name, 10.0f, false, {{"smooth", "false"}});
    model::write_obj(mesh, to_filename(name));
    rewrite(name);
    comparisons.push_back(compare_files(to_filename(name), to_filename_rewrite(name)));

    validator.report_results([&comparisons]{ report_comparisons(comparisons); });
    return 0;
}
