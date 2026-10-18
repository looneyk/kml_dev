#if !defined(QUETZAL_STRATUM_SIM_LOAD_SETTING_DEFINITIONS_HPP)
#define QUETZAL_STRATUM_SIM_LOAD_SETTING_DEFINITIONS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// load_setting_definitions.hpp
//------------------------------------------------------------------------------

#include "quetzal/settings/Settings.hpp"
#include "quetzal/settings/SettingsGroups.hpp"
#include <filesystem>

namespace quetzal::stratum_sim
{

    // Inserts the game's setting definition file from the assets directory. Called once, when the program starts. Throws
    // std::runtime_error if the file cannot be read or a definition is invalid
    void load_setting_definitions(const std::filesystem::path& assets_directory, settings::Settings& settings, settings::SettingsGroups& groups);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_LOAD_SETTING_DEFINITIONS_HPP
