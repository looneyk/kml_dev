//------------------------------------------------------------------------------
// common
// FileValidator.cpp
//------------------------------------------------------------------------------

#include "FileValidator.hpp"
#include "error_util.hpp"
#include <iostream>
#include <format>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

using namespace std;

namespace
{
    
    static constexpr auto escape_reset_string = "\x1b[0m";
    static constexpr auto escape_rendition_pattern = "\x1b[38;5;{}m";

} // namespace

//------------------------------------------------------------------------------
quetzal::FileValidator::FileValidator(const std::filesystem::path& out_path, const std::filesystem::path& reference_path, const std::string& app_name, bool clear) :
    m_out_path(out_path),
    m_reference_path(reference_path),
    m_t0()
{
    if (!app_name.empty())
    {
        write_debug_string(app_name + '\n');
        cout << format(escape_rendition_pattern, 27);
        cout << app_name;
        cout << escape_reset_string << endl;
    }

    init_directory(m_out_path, clear);
    init_directory(m_reference_path, false);
}

//------------------------------------------------------------------------------
int quetzal::FileValidator::report_results(std::function<void()> report_extra)
{
    double t = Timestamp::since(m_t0);

    ostringstream oss;
    auto [difference_count, unchanged_count, missing_count, unchecked_count, total_count] = directory_differences();
    int color_code = 0;
    int n = 0;

//    if (difference_count == 0 && unchanged_count == 0 && missing_count == 0 && unchecked_count == 0)
    if (difference_count == 0 && unchecked_count == 0)
    {
        if (missing_count == 0)
        {
            if (unchanged_count == 0)
            {
                color_code = 34;
                oss << "OK: " << total_count << " files equal (" << t << " seconds)";
            }
            else
            {
                color_code = 76;
                oss << "OK: " << total_count << " files equal, " << unchanged_count << " unchanged (" << t << " seconds)";
            }
        }
        else
        {
            color_code = 148;
            oss << "Warn: " << (total_count - missing_count) << " files equal, " << unchanged_count << " unchanged, " << missing_count << " missing (" << t << " seconds)";
        }
        n = int(total_count);
    }
    else
    {
        color_code = 196;
        oss << "Error: " << difference_count << " different, " << unchanged_count << " unchanged, " << missing_count << " missing, " << unchecked_count << " unchecked, " << total_count << " files (" << t << " seconds)";
        n = -int(difference_count + unchanged_count + missing_count);
    }

    write_debug_string(oss.str() + '\n');
    cout << format(escape_rendition_pattern, color_code);
    cout << oss.str();
    cout << escape_reset_string << endl;
    report_extra();
    this_thread::sleep_for(1s);
    return n;
}

//------------------------------------------------------------------------------
void quetzal::FileValidator::init_directory(const std::filesystem::path& dir, bool clear) const
{
    if (!exists(dir))
    {
        create_directory(dir);
    }
    else if (clear)
    {
        clear_directory(dir);
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::FileValidator::clear_directory(const std::filesystem::path& dir) const
{
    for (const auto& entry : std::filesystem::directory_iterator(dir)) 
    {
        std::filesystem::remove_all(entry.path());
    }

    return;
}

//------------------------------------------------------------------------------
std::array<size_t, 5> quetzal::FileValidator::directory_differences() const
{
    size_t difference_count = 0;
    size_t unchanged_count = 0;
    size_t missing_count = 0;
    size_t unchecked_count = 0;
    size_t total_count = 0;

    for (auto& reference_filepath : filesystem::directory_iterator(m_reference_path))
    {
        filesystem::path filepath = m_out_path / reference_filepath.path().filename();
        if (!exists(filepath))
        {
            ++missing_count;
            cout << std::format("Missing {} {}", reference_filepath.path().filename().string(), filesystem::file_size(reference_filepath)) << std::endl;
        }
        else
        {
            auto ts_out_path = filesystem::last_write_time(filepath);
            auto ts_reference_path = filesystem::last_write_time(reference_filepath.path());
            if (ts_out_path == ts_reference_path)
            {
                ++unchanged_count;
            }
            else
            {
                bool b = file_equal(filepath, reference_filepath);
                if (!b)
                {
                    ++difference_count;
                }
            }
        }

        ++total_count;
    }

    for (auto& reference_filepath : filesystem::directory_iterator(m_out_path))
    {
        filesystem::path filepath = m_reference_path / reference_filepath.path().filename();
        if (!exists(filepath))
        {
            ++unchecked_count;
            ++total_count;
        }
    }

    return {difference_count, unchanged_count, missing_count, unchecked_count, total_count};
}

//------------------------------------------------------------------------------
bool quetzal::FileValidator::file_equal(const std::filesystem::path& out_path, const std::filesystem::path& reference_path) const
{
    size_t size_remaining = static_cast<size_t>(file_size(out_path));
    if (!exists(out_path) || !exists(reference_path) || size_remaining != file_size(reference_path))
    {
        return false;
    }

    ifstream ifs_out_path(out_path, ios_base::binary);
    ifstream ifs_reference_path(reference_path, ios_base::binary);

    const size_t size_buffer = 4096;
    char buffer_out_path[size_buffer];
    char buffer_reference_path[size_buffer];

    while (size_remaining > 0)
    {
        size_t size_read = min(size_buffer, size_remaining);

        ifs_out_path.read(buffer_out_path, size_read);
        ifs_reference_path.read(buffer_reference_path, size_read);

        if (memcmp(buffer_out_path, buffer_reference_path, size_read) != 0)
        {
            return false;
        }

        size_remaining -= size_read;
    }

    return true;
}
