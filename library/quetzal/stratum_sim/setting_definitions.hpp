#if !defined(QUETZAL_STRATUM_SIM_SETTING_DEFINITIONS_HPP)
#define QUETZAL_STRATUM_SIM_SETTING_DEFINITIONS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// setting_definitions.hpp
//------------------------------------------------------------------------------

#include "quetzal/settings/Settings.hpp"
#include "quetzal/settings/SettingGroups.hpp"
#include "quetzal/stratum_format/SettingDefinitionsDocument.hpp"
#include <filesystem>

namespace quetzal::stratum_sim
{

    // Registers the document's enumerations, then inserts its settings, then its groups, so each may refer to those before it
    // Throws std::runtime_error if a definition is invalid
    void insert_definitions(const stratum_format::SettingDefinitionsDocument& document, settings::Settings& store, settings::SettingGroups& group_store);

    // Inserts the game's setting definition file from the assets directory. Called once, when the program starts
    // Throws std::runtime_error if the file cannot be read or a definition is invalid
    void load_setting_definitions(const std::filesystem::path& assets_directory, settings::Settings& store, settings::SettingGroups& group_store);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SETTING_DEFINITIONS_HPP
