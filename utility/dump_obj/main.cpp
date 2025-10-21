
#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/validation.hpp"
#include "quetzal/common/file_util.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/model/obj_io.hpp"
#include <filesystem>
#include <iostream>
#include <regex>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;

    //--------------------------------------------------------------------------
    filesystem::path extract_base_path(const filesystem::path& test_path)
    {
        cout << "test_path: " << test_path << endl;

        string base_path = test_path.string();
        regex pattern("[/\\^]out($|[/\\])", regex::extended | regex::icase);
        smatch match;

        if (regex_search(base_path, match, pattern))
        {
            base_path = base_path.substr(0, match.position());
        }

        return base_path;
    }

    //--------------------------------------------------------------------------
    struct ResultPaths
    {
        ResultPaths(const filesystem::path& test_path)
        {
            // ../../model/wimble/out/cylinder_sections.obj
            auto base_path = extract_base_path(test_path);
            check_path = base_path / "check";
            dump_path = base_path / "dump";
            filesystem::create_directory(check_path);
            filesystem::create_directory(dump_path);
        }

        filesystem::path check_path;
        filesystem::path dump_path;
    };

    //--------------------------------------------------------------------------
    void validate(const mesh_type& mesh, const filesystem::path& result_path)
    {
        ofstream ofs(result_path, ios_base::binary);
        OstreamRedirect redirect(cout, ofs);

        brep::validate(mesh);
        return;
    }

    //--------------------------------------------------------------------------
    void dump(const mesh_type& mesh, const filesystem::path& result_path)
    {
        ofstream ofs(result_path, ios_base::binary);
        OstreamRedirect redirect(cout, ofs);

        brep::dump(mesh);
        return;
    }

    //--------------------------------------------------------------------------
    void check_file(const filesystem::path& file_path, const ResultPaths& result_paths)
    {
        if (file_path.extension() != ".obj")
        {
            return;
        }

        mesh_type mesh;
        model::read_obj(mesh, file_path);

        auto file_stem = file_path.stem();
        auto result_file = file_stem.replace_extension(".txt");

        cout << "validate " << (result_paths.check_path / result_file) << endl;
        validate(mesh, result_paths.check_path / result_file);
        cout << "dump " << (result_paths.dump_path / result_file) << endl;
        dump(mesh, result_paths.dump_path / result_file);
        cout << "done" << endl;
        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cerr << "Error: Requires exactly one file or directory path argument." << endl;
        return -1;
    }

    filesystem::path test_path(argv[1]);
    if (!filesystem::exists(test_path))
    {
        cerr << "Error: Path does not exist." << endl;
        return -1;
    }

    ResultPaths result_paths(test_path);

    if (filesystem::is_directory(test_path))
    {
        for (auto& entry : filesystem::directory_iterator(test_path))
        {
            check_file(entry, result_paths);
        }
    }
    else
    {
        check_file(test_path, result_paths);
    }

    return 0;
}
