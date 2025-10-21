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
quetzal::FileValidator::FileValidator(const std::filesystem::path& lhs, const std::filesystem::path& rhs, const std::string& app_name, bool clear) :
    m_lhs(lhs),
    m_rhs(rhs),
    m_t0()
{
    if (!app_name.empty())
    {
        write_debug_string(app_name + '\n');
        cout << format(escape_rendition_pattern, 27);
        cout << app_name;
        cout << escape_reset_string << endl;
    }

    init_directory(m_lhs, clear);
    init_directory(m_rhs, false);
}

//------------------------------------------------------------------------------
int quetzal::FileValidator::report_results(std::function<void()> report_extra)
{
    double t = Timestamp::since(m_t0);

    ostringstream oss;
    auto [difference_count, missing_count, total_count] = directory_differences();
    int color_code = 0;
    int n = 0;

    if (missing_count == 0 && difference_count == 0)
    {
        color_code = 34;
        oss << "OK: " << total_count << " files equal (" << t << " seconds)";
        n = int(total_count);
    }
    else
    {
        color_code = 196;
        oss << "Error: " << missing_count << " missing, " << difference_count << " different, " << total_count << " files (" << t << " seconds)";
        n = -int(difference_count);
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
void quetzal::FileValidator::init_directory(const std::filesystem::path& dir, bool clear)
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
void quetzal::FileValidator::clear_directory(const std::filesystem::path& dir)
{
    for (const auto& entry : std::filesystem::directory_iterator(dir)) 
    {
        std::filesystem::remove_all(entry.path());
    }

    return;
}

//------------------------------------------------------------------------------
std::array<size_t, 3> quetzal::FileValidator::directory_differences()
{
    size_t missing_count = 0;
    size_t difference_count = 0;
    size_t total_count = 0;

    for (auto& reference_filepath : filesystem::directory_iterator(m_rhs))
    {
        filesystem::path filepath = m_lhs / reference_filepath.path().filename();
        if (!exists(filepath))
        {
            ++missing_count;
        }
        else
        {
            bool b = file_equal(reference_filepath, filepath);
            if (!b)
            {
                ++difference_count;
            }
        }

        ++total_count;
    }

    return {difference_count, missing_count, total_count};
}

//------------------------------------------------------------------------------
bool quetzal::FileValidator::file_equal(const std::filesystem::path& lhs, const std::filesystem::path& rhs)
{
    size_t size_remaining = static_cast<size_t>(file_size(lhs));
    if (!exists(lhs) || !exists(rhs) || size_remaining != file_size(rhs))
    {
        return false;
    }

    ifstream ifs_lhs(lhs, ios_base::binary);
    ifstream ifs_rhs(rhs, ios_base::binary);

    const size_t size_buffer = 4096;
    char buffer_lhs[size_buffer];
    char buffer_rhs[size_buffer];

    while (size_remaining > 0)
    {
        size_t size_read = min(size_buffer, size_remaining);

        ifs_lhs.read(buffer_lhs, size_read);
        ifs_rhs.read(buffer_rhs, size_read);

        if (memcmp(buffer_lhs, buffer_rhs, size_read) != 0)
        {
            return false;
        }

        size_remaining -= size_read;
    }

    return true;
}
