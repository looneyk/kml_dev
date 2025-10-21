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

    // consider adding functionality to look at original and final file times, returning a count so that file modification can be checked ...
    // std::filesystem::file_time_type last_write_time(const std::filesystem::path& p);

    //--------------------------------------------------------------------------
    class FileValidator
    {
    public:

        // lhs is directory for test files, rhs is directory for reference files
        // clear empties the lhs directory at the start for more comprehensive testing
        // lhs: test directory, rhs: reference directory
        FileValidator(const std::filesystem::path& lhs, const std::filesystem::path& rhs, const std::string& app_name = "", bool clear = false);
        FileValidator(const FileValidator&) = default;
        FileValidator(FileValidator&&) = default;
        ~FileValidator() = default;

        FileValidator& operator=(const FileValidator&) = default;
        FileValidator& operator=(FileValidator&&) = default;

        int report_results(std::function<void()> report_extra = []{});

    private:

        void init_directory(const std::filesystem::path& dir, bool clear);
        void clear_directory(const std::filesystem::path& dir);

        // Returns missing, different, total
        std::array<size_t, 3> directory_differences();

        bool file_equal(const std::filesystem::path& lhs, const std::filesystem::path& rhs);

        std::filesystem::path m_lhs;
        std::filesystem::path m_rhs;
        Timestamp m_t0;
    };

} // namespace quetzal

#endif // QUETZAL_COMMON_FILEVALIDATOR_HPP
