#if !defined(QUETZAL_STRATUM_SIM_CONFIG_HPP)
#define QUETZAL_STRATUM_SIM_CONFIG_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Config.hpp
//------------------------------------------------------------------------------

#include "ScriptLimits.hpp"
#include <cstddef>
#include <filesystem>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The program's configuration, which lasts as long as the host and is the same for every game. The program reads the
    // program directory and its assets, and writes only in the user directory
    struct Config
    {
        std::filesystem::path saves_directory;   // Each save is a subdirectory
        std::filesystem::path working_directory; // Contains only the game directory of the game in progress
        std::size_t zone_count_max = 0;          // 0 for no limit
        ScriptLimits pattern_limits{.memory_size_max = 16 << 20, .instruction_count_max = 1'000'000};
        std::filesystem::path assets_directory;
        std::filesystem::path user_directory;    // Everything the program creates while it runs
        std::filesystem::path logs_directory;
        std::size_t log_file_count_max = 0;      // 0 for no limit
        std::filesystem::path cache_directory;
        std::filesystem::path mods_directory;
        std::filesystem::path user_settings_filepath;
    };

    // The platform's user directory
    // Throws std::runtime_error on error
    std::filesystem::path default_user_directory();

    // Returns the config, overridden by the config file if any
    // Throws std::runtime_error on error
    Config load_config(const std::filesystem::path& program_directory);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_CONFIG_HPP
