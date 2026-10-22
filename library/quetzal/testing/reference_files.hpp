#if !defined(QUETZAL_TESTING_REFERENCE_FILES_HPP)
#define QUETZAL_TESTING_REFERENCE_FILES_HPP
//------------------------------------------------------------------------------
// testing
// reference_files.hpp
//------------------------------------------------------------------------------

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace quetzal::testing
{

    // Relative to a test executable's working directory, its own directory
    inline const std::filesystem::path out_directory = "out";
    inline const std::filesystem::path reference_directory = "reference";

    // Returns nothing if the file a test case wrote matches its reference file, or a description of the difference
    using comparison_function_type = std::function<std::optional<std::string>(const std::filesystem::path& out_filepath,
        const std::filesystem::path& reference_filepath)>;

    // Returns nothing if the two files are identical, byte for byte, or the first line on which they differ
    std::optional<std::string> compare_identical(const std::filesystem::path& out_filepath, const std::filesystem::path& reference_filepath);

    // Returns true if the run was given --update-references
    bool updating_references();

    // Checks that the file a test case wrote to out_directory matches the reference file at the same relative path in
    // reference_directory, or, when updating references, copies it over the reference file
    void check_reference(const std::filesystem::path& relative_filepath, const comparison_function_type& compare = compare_identical);

    namespace detail
    {

        void set_updating_references(bool updating);

    } // namespace detail

} // namespace quetzal::testing

#endif // QUETZAL_TESTING_REFERENCE_FILES_HPP
