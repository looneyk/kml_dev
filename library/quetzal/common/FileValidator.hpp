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
#include <map>

namespace quetzal
{

    // File counts:
    // Different: files exist in both directories but differ in content
    // Unchanged: files exist in both directories and have identical timestamps (test did not generate new output)
    // Missing: files exist in reference directory but not in test output directory
    // Unchecked: files exist in test output directory but not in reference directory
    // Total: total files in the union of the test output and reference directories

    // Unchanged files count as identical

    //--------------------------------------------------------------------------
    class FileValidator
    {
    public:

        // Outcome of a comparison, ordered least to most severe
        enum class ResultStatus
        {
            Equal,
            Unchanged,
            Missing,
            Different
        };

        // out_path is directory for generated test files, reference_path is directory for reference files
        // clear empties the out_path directory at the start for more comprehensive testing
        // out_path: test output directory, reference_path: reference directory
        // get rid of obsolete clear option now that we're checking file timestamps? ...
        FileValidator(const std::filesystem::path& out_path, const std::filesystem::path& reference_path, const std::string& app_name = "", bool clear = false);
        FileValidator(const FileValidator&) = default;
        FileValidator(FileValidator&&) = default;
        ~FileValidator() = default;

        FileValidator& operator=(const FileValidator&) = default;
        FileValidator& operator=(FileValidator&&) = default;

        // Returns total_count if everything lines up and there are no differences
        // Returns negative count of different, unchanged, and missing otherwise
        int report_results(std::function<void()> report_extra = []{});

        ResultStatus result() const; // The overall worst case result value

    private:

        void init_directory(const std::filesystem::path& dir, bool clear = false) const;
        void clear_directory(const std::filesystem::path& dir) const;

        void store_out_file_timestamps();

        // Returns counts {different, unchanged, missing, unchecked, total}
        std::array<size_t, 5> directory_differences() const;

        bool file_equal(const std::filesystem::path& out_path, const std::filesystem::path& reference_path) const;

        std::filesystem::path m_out_path;
        std::filesystem::path m_reference_path;
        std::map<std::string, std::filesystem::file_time_type> m_out_file_timestamps;
        Timestamp m_t0;
        ResultStatus m_result;
    };

} // namespace quetzal

#endif // QUETZAL_COMMON_FILEVALIDATOR_HPP
