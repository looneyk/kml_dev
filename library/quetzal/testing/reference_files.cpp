//------------------------------------------------------------------------------
// testing
// reference_files.cpp
//------------------------------------------------------------------------------

#include "reference_files.hpp"
#include <doctest/doctest.h>
#include <format>
#include <fstream>
#include <sstream>

using namespace std;

namespace
{

    bool g_updating_references = false;

    //--------------------------------------------------------------------------
    string read_file(const filesystem::path& filepath)
    {
        ifstream is(filepath, ios_base::binary);
        ostringstream os;
        os << is.rdbuf();
        return os.str();
    }

} // namespace

//------------------------------------------------------------------------------
optional<string> quetzal::testing::compare_identical(const filesystem::path& out_filepath, const filesystem::path& reference_filepath)
{
    istringstream out(read_file(out_filepath));
    istringstream reference(read_file(reference_filepath));

    string out_line;
    string reference_line;
    size_t line_number = 0;

    while (true)
    {
        bool out_read = static_cast<bool>(getline(out, out_line));
        bool reference_read = static_cast<bool>(getline(reference, reference_line));
        ++line_number;

        if (!out_read && !reference_read)
        {
            return nullopt;
        }

        if (out_read != reference_read || out_line != reference_line)
        {
            return format("line {} differs: expected \"{}\", got \"{}\"", line_number, reference_line, out_line);
        }
    }
}

//------------------------------------------------------------------------------
bool quetzal::testing::updating_references()
{
    return g_updating_references;
}

//------------------------------------------------------------------------------
void quetzal::testing::check_reference(const filesystem::path& relative_filepath, const comparison_function_type& compare)
{
    const filesystem::path out_filepath = out_directory / relative_filepath;
    const filesystem::path reference_filepath = reference_directory / relative_filepath;

    if (!filesystem::exists(out_filepath))
    {
        FAIL_CHECK(format("the test case wrote no {}", out_filepath.generic_string()));
        return;
    }

    if (updating_references())
    {
        filesystem::create_directories(reference_filepath.parent_path());
        filesystem::copy_file(out_filepath, reference_filepath, filesystem::copy_options::overwrite_existing);
        return;
    }

    if (!filesystem::exists(reference_filepath))
    {
        FAIL_CHECK(format("no reference is checked in at {}", reference_filepath.generic_string()));
        return;
    }

    optional<string> difference = compare(out_filepath, reference_filepath);
    CHECK_MESSAGE(!difference.has_value(), format("{} differs from its reference: {}", out_filepath.generic_string(), difference.value_or("")));
    return;
}

//------------------------------------------------------------------------------
void quetzal::testing::detail::set_updating_references(bool updating)
{
    g_updating_references = updating;
    return;
}
