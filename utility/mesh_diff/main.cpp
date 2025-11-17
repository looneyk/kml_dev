//------------------------------------------------------------------------------
// mesh_diff
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/geometry/Attributes.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/model/obj_io.hpp"
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <ranges>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
//    using value_type = double;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
    using point_type = typename mesh_type::point_type;
//    using vector_type = point_type::vector_type;

    using face_index_type = map<typename point_type, id_type, math::PriorityLess<typename point_type>>;

    static const string app_name = "mesh_diff";

    bool verbose = true;

    static const int gulp = 1000;

    //--------------------------------------------------------------------------
    face_index_type index_faces(const mesh_type& mesh)
    {
        face_index_type index;
        for (const auto& face : mesh.faces())
        {
            index[vertex_centroid(face)] = face.id();
        }

        return index;
    }

    //--------------------------------------------------------------------------
    // Returns {position_differences, normal_differences, texcoord_differences}, nullopt if halfedge counts don't match
    optional<array<size_t, 3>> face_differences(const mesh_type::face_type& face_lhs, const mesh_type::face_type& face_rhs, bool exact)
    {
        array<size_t, 3> differences = {};

        if (face_lhs.halfedge_count() != face_rhs.halfedge_count())
        {
            return nullopt;
        }

        for (const auto& [halfedge_lhs, halfedge_rhs] : views::zip(face_lhs.halfedges(), face_rhs.halfedges()))
        {
            const auto& va_lhs = halfedge_lhs.vertex_attributes();
            const auto& va_rhs = halfedge_rhs.vertex_attributes();

            if (exact ? va_lhs.position() != va_rhs.position() : !float_eq(va_lhs.position(), va_rhs.position(), gulp))
            {
                ++differences[0];
                if (verbose && !exact) cout << halfedge_lhs.id() << " " << halfedge_rhs.id() << " p " << va_lhs.position() << " != " << va_rhs.position() << endl;
            }

            if (exact ? va_lhs.normal() != va_rhs.normal() : !float_eq(va_lhs.normal(), va_rhs.normal(), gulp))
            {
                ++differences[1];
                if (verbose && !exact) cout << halfedge_lhs.id() << " " << halfedge_rhs.id() << " n " << va_lhs.normal() << " != " << va_rhs.normal() << endl;
            }

            if (exact ? va_lhs.texcoord() != va_rhs.texcoord() : !float_eq(va_lhs.texcoord(), va_rhs.texcoord(), gulp))
            {
                ++differences[2];
                if (verbose && !exact) cout << halfedge_lhs.id() << " " << halfedge_rhs.id() << " t " << va_lhs.texcoord() << " != " << va_rhs.texcoord() << endl;
            }
        }

        return differences;
    }

    //--------------------------------------------------------------------------
    // Returns {faces_identical, faces_equivalent, faces_total}, nullopt if face counts don't match
    // identical counts as equivalent
    optional<array<size_t, 3>> mesh_stats(const mesh_type& mesh_lhs, const mesh_type& mesh_rhs)
    {
        array<size_t, 3> stats = {};
        auto& [n_identical, n_equivalent, n_total] = stats;

        if (mesh_lhs.face_count() != mesh_rhs.face_count())
        {
            return nullopt;
        }

        auto index_lhs = index_faces(mesh_lhs);
        auto index_rhs = index_faces(mesh_rhs);

        for (const auto& [entry_lhs, entry_rhs, i] : views::zip(index_lhs, index_rhs, views::iota(0)))
        {
            ++n_total;

            auto result = face_differences(mesh_lhs.face(entry_lhs.second), mesh_rhs.face(entry_rhs.second), true);
            if (!result)
            {
                continue;
            }

            if (result.value()[0] == 0 && result.value()[1] == 0 && result.value()[2] == 0)
            {
                ++n_identical;
                ++n_equivalent;
                continue;
            }

            result = face_differences(mesh_lhs.face(entry_lhs.second), mesh_rhs.face(entry_rhs.second), false);
            assert(result);

            if (result.value()[0] == 0 && result.value()[1] == 0 && result.value()[2] == 0)
            {
                ++n_equivalent;
            }
        }

        return stats;
    }

    //--------------------------------------------------------------------------
    // Returns {compared, equivalent, mesh_stats (null for face count mismatch)}
    tuple<bool, bool, optional<array<size_t, 3>>> mesh_diff(const filesystem::path& path_lhs, const filesystem::path& path_rhs)
    {
        mesh_type mesh_lhs;
        model::read_obj(mesh_lhs, path_lhs);
        mesh_type mesh_rhs;
        model::read_obj(mesh_rhs, path_rhs);

        if (mesh_lhs.face_count() > 100000)
        {
            cout << "Skipping, too many faces: " << path_lhs << " " << mesh_lhs.face_count() << endl;
            return {false, false, {}};
        }

        auto results = mesh_stats(mesh_lhs, mesh_rhs);
        if (!results)
        {
            cout << "Different face counts: " << mesh_lhs.face_count() << " and " << mesh_rhs.face_count() << endl;
            return {true, false, {}};
        }

        auto [n_identical, n_equivalent, n_total] = results.value();
//        assert(n_total == mesh_lhs.face_count());
        if (n_total != mesh_lhs.face_count())
        {
            cout << std::format("Error: total {} != face_count {}", n_total,  mesh_lhs.face_count()) << endl;
        }

        if (n_identical == n_total)
        {
            cout << "Identical: " << n_total << " faces" << endl;
            return {true, true, results};
        }

        if (n_equivalent == n_total)
        {
            cout << "Equivalent: " << n_total << " faces (" << n_identical << " identical)" << endl;
            return {true, true, results};
        }

        cout << "Different: " << n_identical << " identical, " << n_equivalent << " equivalent, " << n_total << " total faces" << endl;
        return {true, false, results};
    }

    //--------------------------------------------------------------------------
    struct Results
    {
        size_t n_skipped = 0;
        size_t n_uncomparable = 0;
        size_t n_identical = 0;
        size_t n_equivalent = 0;
        size_t n_total = 0;
    };

    //--------------------------------------------------------------------------
    void report_results(const Results& results)
    {
        if (results.n_identical == results.n_total)
        {
            cout << "OK: " << results.n_total << " identical files" << endl;
        }
        else if (results.n_equivalent == results.n_total)
        {
            cout << "OK: " << results.n_total << " equivalent files (" << results.n_identical << " identical)" << endl;
        }
        else if (results.n_equivalent == results.n_total - results.n_skipped)
        {
            cout << "OK: " << (results.n_total - results.n_skipped) << " equivalent files (" << results.n_skipped << " skipped, " << results.n_identical << " identical)" << endl;
        }
        else
        {
            cout << "Error: " << (results.n_total - results.n_skipped - results.n_equivalent) << " different, " << results.n_skipped << " skipped, " << results.n_total << " files" << endl;
        }

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    if (argc < 3 || argc > 4)
    {
        cerr << "Error: Requires two file or directory path arguments or model directory name and two relative directories." << endl;
        return -1;
    }

    // Check that paths exist

    filesystem::path path_lhs(argc == 3 ? argv[1] : filesystem::path(argv[1]) / argv[2]);
    if (!filesystem::exists(path_lhs))
    {
        cerr << "Error: Path " << path_lhs << " does not exist." << endl;
        return -1;
    }

    filesystem::path path_rhs(argc == 3 ? argv[2] : filesystem::path(argv[1]) / argv[3]);
    if (!filesystem::exists(path_rhs))
    {
        cerr << "Error: Path " << path_rhs << " does not exist." << endl;
        return -1;
    }

    if (argc == 4)
    {
        cout << argv[1] << endl;
    }
    cout << path_lhs.string() << " " << path_rhs.string() << endl;

    if (filesystem::is_regular_file(path_lhs) && filesystem::is_regular_file(path_rhs))
    {
        auto mesh_results = mesh_diff(path_lhs, path_rhs);
        
        Results final_results;
        ++final_results.n_total;
        if (!get<0>(mesh_results))
        {
            ++final_results.n_skipped;
        }
        else if (get<2>(mesh_results))
        {
            auto counts = get<2>(mesh_results).value();
            if (counts[0] == counts[2])
            {
                ++final_results.n_identical;
                ++final_results.n_equivalent;
            }
            else if (counts[1] == counts[2])
            {
                ++final_results.n_equivalent;
            }
        }

        report_results(final_results);
    }
    else if (filesystem::is_directory(path_lhs) && filesystem::is_directory(path_rhs))
    {
        using file_index_type = map<string, filesystem::path>;

        file_index_type file_index_lhs;
        ranges::for_each(filesystem::directory_iterator{path_lhs}, [&](const auto& entry)
        { 
            auto path = entry.path();
            file_index_lhs[path.filename().string()] = path;
        });

        file_index_type file_index_rhs;
        ranges::for_each(filesystem::directory_iterator{path_rhs}, [&](const auto& entry)
        { 
            auto path = entry.path();
            file_index_rhs[path.filename().string()] = path;
        });

        Results final_results;

        auto i_lhs = file_index_lhs.begin();
        auto i_rhs = file_index_rhs.begin();
        while (i_lhs != file_index_lhs.end() && i_rhs != file_index_rhs.end())
        {
            cout << i_lhs->first << "\t";

            if (i_lhs->first == i_rhs->first)
            {
                auto mesh_results = mesh_diff(i_lhs->second, i_rhs->second);
                ++final_results.n_total;
                if (!get<0>(mesh_results))
                {
                    ++final_results.n_skipped;
                }
                else if (get<2>(mesh_results))
                {
                    auto counts = get<2>(mesh_results).value();
                    if (counts[0] == counts[2])
                    {
                        ++final_results.n_identical;
                        ++final_results.n_equivalent;
                    }
                    else if (counts[1] == counts[2])
                    {
                        ++final_results.n_equivalent;
                    }
                }

                ++i_lhs;
                ++i_rhs;
                continue;
            }

            if (i_lhs->first > i_rhs->first)
            {
                ++i_rhs;
                continue;
            }

            ++i_lhs;
            continue;
        }

        report_results(final_results);
    }
    else
    {
        cerr << "Error: Paths must be either both files or both directories." << endl;
        return -1;
    }

    return 0;
}
