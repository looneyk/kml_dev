#if !defined(QUETZAL_STRATUM_SIM_INSERT_DEFINITIONS_HPP)
#define QUETZAL_STRATUM_SIM_INSERT_DEFINITIONS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// insert_definitions.hpp
//------------------------------------------------------------------------------

#include "quetzal/settings/Settings.hpp"
#include "quetzal/settings/SettingsGroups.hpp"
#include "quetzal/stratum_format/SettingDefinitionsDocument.hpp"

namespace quetzal::stratum_sim
{

    // Registers the document's enumerations, then inserts its settings, then its groups, so each may refer to those before it
    // Throws std::runtime_error if a definition is invalid
    void insert_definitions(const stratum_format::SettingDefinitionsDocument& document, settings::Settings& settings, settings::SettingsGroups& groups);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_INSERT_DEFINITIONS_HPP
