//------------------------------------------------------------------------------
// stratum_sim
// insert_definitions.cpp
//------------------------------------------------------------------------------

#include "insert_definitions.hpp"
#include <vector>

namespace
{

    using document_type = quetzal::stratum_format::SettingDefinitionsDocument;

    //--------------------------------------------------------------------------
    quetzal::settings::SettingScope setting_scope(document_type::Scope scope)
    {
        return scope == document_type::Scope::Game ? quetzal::settings::SettingScope::Game : quetzal::settings::SettingScope::User;
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::stratum_sim::insert_definitions(const stratum_format::SettingDefinitionsDocument& document, settings::Settings& settings,
    settings::SettingsGroups& groups)
{
    for (const auto& enumeration : document.enumerations)
    {
        settings.register_enumeration(enumeration.name, enumeration.values);
    }

    for (const auto& definition : document.settings)
    {
        settings.insert(definition.key, {.type_name = definition.type_name, .default_value = definition.default_value,
            .scope = setting_scope(definition.scope), .restart_required = definition.restart_required, .display_name = definition.display_name,
            .description = definition.description, .category = definition.category, .minimum = definition.minimum, .maximum = definition.maximum});
    }

    for (const auto& definition : document.groups)
    {
        std::vector<settings::Preset> presets;
        for (const auto& preset : definition.presets)
        {
            presets.push_back({preset.name, preset.display_name, preset.editable, preset.values});
        }

        groups.insert(settings::SettingsGroup(definition.key, setting_scope(definition.scope), definition.default_preset_name, std::move(presets),
            definition.display_name, definition.category));
    }

    return;
}
