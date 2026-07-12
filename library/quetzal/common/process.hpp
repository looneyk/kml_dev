#if !defined(QUETZAL_COMMON_PROCESS_HPP)
#define QUETZAL_COMMON_PROCESS_HPP
//------------------------------------------------------------------------------
// common
// process.hpp
//------------------------------------------------------------------------------

#include <filesystem>
#include <string>
#include <vector>

namespace quetzal
{

    // Launches app_path as a child process with the given arguments, waits for it to exit, and returns its exit code
    int run_process(const std::filesystem::path& app_path, const std::vector<std::string>& args = {});

    // Opens path with the OS's default application for its file type
    void open_with_default_application(const std::filesystem::path& path);

} // namespace quetzal

#endif // QUETZAL_COMMON_PROCESS_HPP
