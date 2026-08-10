//------------------------------------------------------------------------------
// to_utf8
// main.cpp
//------------------------------------------------------------------------------

#include "to_utf8.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

namespace
{

    struct options
    {
        fs::path root_path;
        vector<string> extensions = { ".cpp", ".hpp", ".h", ".c", ".cc", ".cxx", ".inl", ".txt", ".md", ".cmake", ".sh", ".ps1", ".py", ".vert", ".frag" };
        vector<string> filenames = { "notes", "misc" };
        vector<string> excluded_directories = { ".git", ".vs", "build", "x64", "Debug", "Release", "bin", "obj" };
        bool dry_run = false;
        bool verbose = false;
    };

    //--------------------------------------------------------------------------
    void usage()
    {
        cout << endl;
        cout << "to_utf8" << endl;
        cout << endl;
        cout << "Normalizes text file encoding (to UTF-8 without BOM) and line endings" << endl;
        cout << "(to LF) recursively under a directory." << endl;
        cout << endl;
        cout << "to_utf8 <root_path> [options]" << endl;
        cout << endl;
        cout << "options:" << endl;
        cout << "    --ext <.ext1,.ext2,...>    file extensions to process" << endl;
        cout << "                               (default: .cpp,.hpp,.h,.c,.cc,.cxx,.inl,.txt,.cmake,.sh,.ps1,.md)" << endl;
        cout << "    --filenames <name1,name2,...>  extensionless filenames to process" << endl;
        cout << "                               (default: notes,misc)" << endl;
        cout << "    --exclude <dir1,dir2,...>  directory names to skip" << endl;
        cout << "                               (default: .git,.vs,build,x64,Debug,Release,bin,obj)" << endl;
        cout << "    --dry-run                  report what would change without modifying files" << endl;
        cout << "    --verbose                  print a line for every file examined, not just changed ones" << endl;

        return;
    }

    //--------------------------------------------------------------------------
    vector<string> split(const string& s, char delim)
    {
        vector<string> tokens;
        stringstream ss(s);
        string token;
        while (getline(ss, token, delim))
        {
            if (!token.empty())
            {
                tokens.push_back(token);
            }
        }

        return tokens;
    }

    //--------------------------------------------------------------------------
    string normalize_line_endings(const string& text)
    {
        string result;
        result.reserve(text.size());

        for (size_t i = 0; i < text.size(); ++i)
        {
            if (text[i] == '\r')
            {
                result += '\n';
                if (i + 1 < text.size() && text[i + 1] == '\n')
                {
                    ++i;
                }
            }
            else
            {
                result += text[i];
            }
        }

        return result;
    }

    //--------------------------------------------------------------------------
    bool should_process(const fs::path& file_path, const options& opts)
    {
        string filename = file_path.filename().string();
        if (ranges::contains(opts.filenames, filename))
        {
            return true;
        }

        string extension = file_path.extension().string();
        return ranges::contains(opts.extensions, extension);
    }

    //--------------------------------------------------------------------------
    bool is_excluded_directory(const fs::path& dir_path, const options& opts)
    {
        string name = dir_path.filename().string();
        return ranges::contains(opts.excluded_directories, name);
    }

    //--------------------------------------------------------------------------
    bool process_file(const fs::path& file_path, const options& opts)
    {
        ifstream in(file_path, ios::binary);
        if (!in)
        {
            cerr << "Could not open " << file_path.string() << endl;
            return false;
        }

        string bytes((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        in.close();

        utf8_conversion conversion = to_utf8(bytes);
        string normalized = normalize_line_endings(conversion.text);
        bool changed = (normalized != bytes);

        if (opts.verbose || changed)
        {
            cout << (changed ? "[changed] " : "[ok]      ") << file_path.string();
            if (changed)
            {
                cout << "  (" << conversion.encoding_label << ")";
            }
            cout << endl;
        }

        if (changed && !opts.dry_run)
        {
            ofstream out(file_path, ios::binary | ios::trunc);
            out.write(normalized.data(), static_cast<streamsize>(normalized.size()));
        }

        return changed;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    options opts;

    if (argc < 2)
    {
        usage();
        return 0;
    }

    opts.root_path = argv[1];

    for (int i = 2; i < argc; ++i)
    {
        string arg = argv[i];
        if (arg == "--ext" && i + 1 < argc)
        {
            opts.extensions = split(argv[++i], ',');
        }
        else if (arg == "--filenames" && i + 1 < argc)
        {
            opts.filenames = split(argv[++i], ',');
        }
        else if (arg == "--exclude" && i + 1 < argc)
        {
            opts.excluded_directories = split(argv[++i], ',');
        }
        else if (arg == "--dry-run")
        {
            opts.dry_run = true;
        }
        else if (arg == "--verbose")
        {
            opts.verbose = true;
        }
        else
        {
            usage();
            return 0;
        }
    }

    if (!fs::exists(opts.root_path))
    {
        cerr << "Path does not exist: " << opts.root_path.string() << endl;
        return 1;
    }

    size_t files_examined = 0;
    size_t files_changed = 0;

    for (auto it = fs::recursive_directory_iterator(opts.root_path); it != fs::recursive_directory_iterator(); ++it)
    {
        if (it->is_directory())
        {
            if (is_excluded_directory(it->path(), opts))
            {
                it.disable_recursion_pending();
            }
            continue;
        }

        if (!should_process(it->path(), opts))
        {
            continue;
        }

        ++files_examined;
        if (process_file(it->path(), opts))
        {
            ++files_changed;
        }
    }

    cout << endl;
    cout << files_examined << " files examined, " << files_changed << " changed" << (opts.dry_run ? " (dry run)" : "") << "." << endl;

    return 0;
}
