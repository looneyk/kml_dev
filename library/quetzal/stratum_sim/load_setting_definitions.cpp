//------------------------------------------------------------------------------
// stratum_sim
// load_setting_definitions.cpp
//------------------------------------------------------------------------------

#include "load_setting_definitions.hpp"
#include "insert_definitions.hpp"
#include "quetzal/stratum_format/read_setting_definitions.hpp"

//------------------------------------------------------------------------------
void quetzal::stratum_sim::load_setting_definitions(const std::filesystem::path& assets_directory, settings::Settings& settings, settings::SettingsGroups& groups)
{
    insert_definitions(stratum_format::read_setting_definitions(assets_directory / stratum_format::SettingDefinitionsDocument::filepath), settings, groups);
    return;
}
