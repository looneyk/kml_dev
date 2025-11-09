#if !defined(QUETZAL_COMMON_FILEVALIDATOR_HPP)
#define QUETZAL_COMMON_FILEVALIDATOR_HPP
//------------------------------------------------------------------------------
// common
// FileValidator.hpp
//------------------------------------------------------------------------------

#include "Timestamp.hpp"
#include <array>
#include <filesystem>
#include <functional>

namespace quetzal
{

    // File counts:
    // Different: files exist in both directories but differ in content
    // Unchanged: files exist in both directories and have identical timestamps (test did not generate new output)
    // Missing: files exist in reference directory but not in test output directory
    // Unchecked: files exist in test output directory but not in reference directory
    // Total: total files in the union of the test output and reference directories

    //--------------------------------------------------------------------------
    class FileValidator
    {
    public:

        // lhs is directory for generated test files, rhs is directory for reference files
        // clear empties the lhs directory at the start for more comprehensive testing
        // lhs: test output directory, rhs: reference directory
        // get rid of obsolete clear option now that we're checking file timestamps? ...
        FileValidator(const std::filesystem::path& lhs, const std::filesystem::path& rhs, const std::string& app_name = "", bool clear = false);
        FileValidator(const FileValidator&) = default;
        FileValidator(FileValidator&&) = default;
        ~FileValidator() = default;

        FileValidator& operator=(const FileValidator&) = default;
        FileValidator& operator=(FileValidator&&) = default;

        // Returns total_count if everything lines up and there are no differences
        // Returns negative count of different, unchanged, and missing otherwise
        int report_results(std::function<void()> report_extra = []{});

    private:

        void init_directory(const std::filesystem::path& dir, bool clear) const;
        void clear_directory(const std::filesystem::path& dir) const;

        // Returns counts {different, unchanged, missing, unchecked, total}
        std::array<size_t, 5> directory_differences() const;

        bool file_equal(const std::filesystem::path& lhs, const std::filesystem::path& rhs) const;

        std::filesystem::path m_lhs;
        std::filesystem::path m_rhs;
        Timestamp m_t0;
    };

} // namespace quetzal

#endif // QUETZAL_COMMON_FILEVALIDATOR_HPP
