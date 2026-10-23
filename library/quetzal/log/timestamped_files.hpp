#if !defined(QUETZAL_LOG_TIMESTAMPED_FILES_HPP)
#define QUETZAL_LOG_TIMESTAMPED_FILES_HPP
//------------------------------------------------------------------------------
// log
// timestamped_files.hpp
//------------------------------------------------------------------------------

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <system_error>
#include <vector>

namespace quetzal::log
{

    //--------------------------------------------------------------------------
    // A file that remove_oldest_timestamped_files could not remove, and the reason
    struct UnremovedFile
    {
        std::filesystem::path filepath;
        std::error_code error;
    };

    // Returns the base path with the time inserted before its extension, in ISO 8601 basic format in UTC to the millisecond:
    // logs/program.log at 2026-10-22 22:30:05.123 becomes logs/program_20261022T223005.123Z.log, so the
    // files of one base path sort by time
    std::filesystem::path timestamped_filepath(const std::filesystem::path& base_filepath, std::chrono::system_clock::time_point time);

    // Keeps the newest file_count_max files of the form timestamped_filepath returns for the base path, removes each older one,
    // and returns those it could not remove, such as a file another process has open. A file it could not remove takes no
    // newer file's place. A file_count_max of 0 removes nothing
    std::vector<UnremovedFile> remove_oldest_timestamped_files(const std::filesystem::path& base_filepath, std::size_t file_count_max);

} // namespace quetzal::log

#endif // QUETZAL_LOG_TIMESTAMPED_FILES_HPP
