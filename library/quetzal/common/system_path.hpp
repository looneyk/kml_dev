#if !defined(QUETZAL_SYSTEM_PATH_HPP)
#define QUETZAL_SYSTEM_PATH_HPP
//------------------------------------------------------------------------------
// system_path.hpp
//------------------------------------------------------------------------------

#include <filesystem>
#include <optional>

#if defined(_WIN32)
#include <shlobj.h>
#endif

namespace quetzal
{

#if defined(_WIN32)
    std::filesystem::path system_path(REFKNOWNFOLDERID rfid);
#endif

    // Returns std::nullopt if the environment variable is not set or empty
    std::optional<std::filesystem::path> environment_path(const char* name);

    // Throws std::runtime_error if the environment variable is not set or empty
    std::filesystem::path required_environment_path(const char* name);

    // Returns the directory in which the platform stores user application data
    // Throws std::runtime_error if the environment variable that locates it is not set
    std::filesystem::path application_data_directory();

} // namespace quetzal

#endif // QUETZAL_SYSTEM_PATH_HPP
