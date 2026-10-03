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
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_CONFIG_HPP
