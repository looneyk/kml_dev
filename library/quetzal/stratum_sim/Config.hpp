#if !defined(QUETZAL_STRATUM_SIM_CONFIG_HPP)
#define QUETZAL_STRATUM_SIM_CONFIG_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Config.hpp
//------------------------------------------------------------------------------

#include <cstddef>
#include <filesystem>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The program's configuration, which lasts as long as the host and is the same for every game. The program reads the
    // program directory and its assets, and writes only in the user directory
    struct Config
    {
        std::filesystem::path saves_directory;   // Each save is a directory in it
        std::filesystem::path working_directory; // Contains only the game directory of the game in progress
        std::size_t zone_count_max = 0;          // The most zones the world keeps loaded at once
        std::filesystem::path assets_directory;  // The program's own data, which it only reads
        std::filesystem::path user_directory;    // Everything the program creates while it runs
        std::filesystem::path logs_directory;
        std::size_t log_file_count_max = 0;      // The most log files kept in the logs directory, the current run's included
        std::filesystem::path cache_directory;   // Data the program can make again, which may be deleted
        std::filesystem::path mods_directory;
        std::filesystem::path user_settings_filepath; // The user settings file
    };

    // Returns the platform's user directory
    // Throws std::runtime_error on error
    std::filesystem::path default_user_directory();

    // Returns the config, possibly overridden by the config file
    // Throws std::runtime_error on error
    Config load_config(const std::filesystem::path& program_directory);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_CONFIG_HPP
