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
    // The program's configuration, which lasts as long as the host and is the same for every game
    struct Config
    {
        std::filesystem::path saves_directory;   // Each save is a directory in it
        std::filesystem::path working_directory; // The game directory of the game in progress is in it, and nothing else
        std::size_t zone_count_max = 0;          // The most zones the world keeps loaded at once
        std::filesystem::path logs_directory;
        std::filesystem::path cache_directory;          // Data the program can make again, which may be deleted
        std::filesystem::path user_settings_filepath;   // The user settings file
    };

    // Returns the program's data directory, the one location in code, which has the config file: KML/Stratum in the
    // platform's directory for application data. Throws std::runtime_error if the environment does not give it
    std::filesystem::path data_directory();

    // Returns the config from the config file in the directory, after writing one with the defaults if there is none.
    // A relative location in the file is relative to the directory. Throws std::runtime_error if the file cannot be read
    // or written
    Config load_config(const std::filesystem::path& directory);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_CONFIG_HPP
