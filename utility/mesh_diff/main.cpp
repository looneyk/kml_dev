//------------------------------------------------------------------------------
// mesh_diff
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/Exception.hpp"
#include "quetzal/common/file_util.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/model/mesh_diff.hpp"
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <map>
#include <string>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
//    using value_type = double;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;

    static const string app_name = "mesh_diff";

    //--------------------------------------------------------------------------
    enum class FileResult
    {
        identical,
        equivalent,
        different,
        error
    };

    //--------------------------------------------------------------------------
    struct Results
    {
        size_t n_total = 0;
        size_t n_identical = 0;
        size_t n_equivalent = 0; // Includes identical
        size_t n_different = 0;
        size_t n_error = 0;
        size_t n_unpaired = 0;       // Present only on the left
        size_t n_unpaired_other = 0; // Present only on the right
    };

    //--------------------------------------------------------------------------
    void accumulate(Results& results, FileResult result)
    {
        ++results.n_total;

        switch (result)
        {
            case FileResult::identical:
                ++results.n_identical;
                ++results.n_equivalent;
                break;

            case FileResult::equivalent:
                ++results.n_equivalent;
                break;

            case FileResult::different:
                ++results.n_different;
                break;

            case FileResult::error:
                ++results.n_error;
                break;
        }

        return;
    }

    //--------------------------------------------------------------------------
    void report_differences(const model::Comparison& comparison, const model::Options& options)
    {
        for (const auto& difference : comparison.differences)
        {
            const auto& counts = difference.differences;

            string where = difference.face_id_other
                ? std::format("faces {} and {}", difference.face_id, *difference.face_id_other)
                : std::format("face {}", difference.face_id);

            string detail;
            if (!counts.empty())
            {
                detail = std::format(", {} position, {} normal, {} texcoord", counts.positions, counts.normals, counts.texcoords);
            }

            string surface;
            if (options.compare_surfaces && !difference.surface.empty() && !difference.surface_other.empty() && difference.surface != difference.surface_other)
            {
                surface = std::format(", surface {} != {}", difference.surface, difference.surface_other);
            }

            cout << "    " << where << " " << model::to_string(difference.match) << detail << surface << endl;
        }

        if (comparison.n_differences_elided > 0)
        {
            cout << "    ... " << comparison.n_differences_elided << " more" << endl;
        }

        return;
    }

    //--------------------------------------------------------------------------
    FileResult compare_files(const filesystem::path& path_lhs, const filesystem::path& path_rhs, const model::Options& options)
    {
        model::Comparison comparison;

        try
        {
            comparison = model::mesh_diff<vector_traits>(path_lhs, path_rhs, options);
        }
        catch (const std::exception& e)
        {
            cout << "Error reading: " << e.what() << endl;
            return FileResult::error;
        }

        if (comparison.identical())
        {
            cout << "Identical: " << comparison.n_faces << " faces" << endl;
            return FileResult::identical;
        }

        if (comparison.equivalent() && comparison.n_surface_differences == 0)
        {
            cout << "Equivalent: " << comparison.n_faces << " faces (" << comparison.n_identical << " identical)" << endl;
            return FileResult::equivalent;
        }

        if (comparison.n_faces != comparison.n_faces_other)
        {
            cout << "Different face counts: " << comparison.n_faces << " and " << comparison.n_faces_other << endl;
        }
        else
        {
            cout << std::format("Different: {} identical, {} equivalent, {} reversed, {} different, {} unmatched, {} extra, {} surface, {} total faces",
                comparison.n_identical, comparison.n_equivalent, comparison.n_reversed, comparison.n_different,
                comparison.n_unmatched, comparison.n_unmatched_other, comparison.n_surface_differences, comparison.n_faces) << endl;
        }

        report_differences(comparison, options);
        return FileResult::different;
    }

    //--------------------------------------------------------------------------
    void report_results(const Results& results)
    {
        if (results.n_error > 0 || results.n_different > 0)
        {
            cout << std::format("Error: {} different, {} unreadable, {} files compared",
                results.n_different, results.n_error, results.n_total) << endl;
        }
        else if (results.n_identical == results.n_total)
        {
            cout << "OK: " << results.n_total << " identical files" << endl;
        }
        else
        {
            cout << "OK: " << results.n_total << " equivalent files (" << results.n_identical << " identical)" << endl;
        }

        if (results.n_unpaired > 0 || results.n_unpaired_other > 0)
        {
            cout << std::format("Unpaired: {} only on the left, {} only on the right", results.n_unpaired, results.n_unpaired_other) << endl;
        }

        return;
    }

    //--------------------------------------------------------------------------
    using file_index_type = map<string, filesystem::path>;

    //--------------------------------------------------------------------------
    file_index_type index_files(const filesystem::path& path)
    {
        file_index_type index;

        for (const auto& entry : filesystem::directory_iterator{path})
        {
            if (entry.is_regular_file())
            {
                index[entry.path().filename().string()] = entry.path();
            }
        }

        return index;
    }

    //--------------------------------------------------------------------------
    Results compare_directories(const filesystem::path& path_lhs, const filesystem::path& path_rhs, const model::Options& options)
    {
        file_index_type index_lhs = index_files(path_lhs);
        file_index_type index_rhs = index_files(path_rhs);

        Results results;

        auto i_lhs = index_lhs.begin();
        auto i_rhs = index_rhs.begin();

        while (i_lhs != index_lhs.end() || i_rhs != index_rhs.end())
        {
            if (i_rhs == index_rhs.end() || (i_lhs != index_lhs.end() && i_lhs->first < i_rhs->first))
            {
                cout << i_lhs->first << "\tOnly in " << path_lhs.string() << endl;
                ++results.n_unpaired;
                ++i_lhs;
                continue;
            }

            if (i_lhs == index_lhs.end() || i_rhs->first < i_lhs->first)
            {
                cout << i_rhs->first << "\tOnly in " << path_rhs.string() << endl;
                ++results.n_unpaired_other;
                ++i_rhs;
                continue;
            }

            cout << i_lhs->first << "\t" << std::flush;
            accumulate(results, compare_files(i_lhs->second, i_rhs->second, options));

            ++i_lhs;
            ++i_rhs;
        }

        return results;
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

    set_output_file_lf_endings();

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

    cout << path_lhs.string() << " <=> " << path_rhs.string() << endl;

    model::Options options;

    Results results;

    if (filesystem::is_regular_file(path_lhs) && filesystem::is_regular_file(path_rhs))
    {
        accumulate(results, compare_files(path_lhs, path_rhs, options));
    }
    else if (filesystem::is_directory(path_lhs) && filesystem::is_directory(path_rhs))
    {
        results = compare_directories(path_lhs, path_rhs, options);
    }
    else
    {
        cerr << "Error: Paths must be either both files or both directories." << endl;
        return -1;
    }

    report_results(results);
    return results.n_different > 0 || results.n_error > 0 ? 1 : 0;
}
