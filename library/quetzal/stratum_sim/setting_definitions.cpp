//------------------------------------------------------------------------------
// stratum_sim
// setting_definitions.cpp
//------------------------------------------------------------------------------

#include "setting_definitions.hpp"
#include <format>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

    using document_type = quetzal::stratum_format::SettingDefinitionsDocument;

    //--------------------------------------------------------------------------
    std::string setting_scope(document_type::Scope scope)
    {
        return std::string(scope == document_type::Scope::Game ? quetzal::stratum_sim::game_setting_scope : quetzal::stratum_sim::user_setting_scope);
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::stratum_sim::insert_definitions(const stratum_format::SettingDefinitionsDocument& document, settings::Settings& store)
{
    for (const auto& enumeration : document.enumerations)
    {
        store.register_enumeration(enumeration.name, enumeration.values);
    }

    for (const auto& definition : document.settings)
    {
        const bool user_setting = definition.scope == document_type::Scope::User;
        if (user_setting && definition.modifiable)
        {
            throw std::runtime_error(std::format("the user setting '{}' has modifiable, which only a game setting may have", definition.key));
        }

        store.insert(definition.key, {.type_name = definition.type_name, .default_value = definition.default_value,
            .scope = setting_scope(definition.scope), .restart_required = definition.restart_required,
            .modifiable = user_setting || definition.modifiable, .display_name = definition.display_name, .description = definition.description, .category = definition.category,
            .minimum = definition.minimum, .maximum = definition.maximum});
    }

    for (const auto& definition : document.groups)
    {
        const bool user_group = definition.scope == document_type::Scope::User;
        if (user_group && definition.modifiable)
        {
            throw std::runtime_error(std::format("the user setting group '{}' has modifiable, which only a game setting group may have",
                definition.key));
        }

        std::vector<std::pair<std::string, settings::Preset>> presets;
        for (const auto& preset_definition : definition.presets)
        {
            presets.emplace_back(preset_definition.name, settings::Preset {preset_definition.display_name, preset_definition.modifiable,
                preset_definition.values});
        }

        store.insert_group(definition.key, setting_scope(definition.scope), user_group || definition.modifiable, definition.default_preset_name,
            definition.member_keys, std::move(presets), definition.display_name, definition.category);
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::load_setting_definitions(const std::filesystem::path& assets_directory, settings::Settings& store)
{
    insert_definitions(stratum_format::read_setting_definitions(assets_directory / stratum_format::SettingDefinitionsDocument::filepath), store);
    return;
}
