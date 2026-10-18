//------------------------------------------------------------------------------
// settings
// SettingsGroup.cpp
//------------------------------------------------------------------------------

#include "SettingsGroup.hpp"
#include <algorithm>
#include <cassert>
#include <format>
#include <set>
#include <stdexcept>

//------------------------------------------------------------------------------
quetzal::settings::SettingsGroup::SettingsGroup(std::string_view key, SettingScope scope, std::string_view default_preset_name,
    std::vector<Preset> presets, std::string_view display_name, std::string_view category) :
    m_key(key),
    m_scope(scope),
    m_display_name(display_name),
    m_category(category),
    m_default_preset_name(default_preset_name),
    m_selected_preset_name(default_preset_name),
    m_presets()
{
    for (auto& preset : presets)
    {
        insert(std::move(preset));
    }

    if (!contains(m_default_preset_name))
    {
        throw std::runtime_error(std::format("the settings group '{}' has no default preset '{}'", m_key, m_default_preset_name));
    }
}

//------------------------------------------------------------------------------
const std::string& quetzal::settings::SettingsGroup::key() const
{
    return m_key;
}

//------------------------------------------------------------------------------
quetzal::settings::SettingScope quetzal::settings::SettingsGroup::scope() const
{
    return m_scope;
}

//------------------------------------------------------------------------------
const std::string& quetzal::settings::SettingsGroup::display_name() const
{
    return m_display_name;
}

//------------------------------------------------------------------------------
const std::string& quetzal::settings::SettingsGroup::category() const
{
    return m_category;
}

//------------------------------------------------------------------------------
const std::string& quetzal::settings::SettingsGroup::default_preset_name() const
{
    return m_default_preset_name;
}

//------------------------------------------------------------------------------
const std::vector<quetzal::settings::Preset>& quetzal::settings::SettingsGroup::presets() const
{
    return m_presets;
}

//------------------------------------------------------------------------------
const quetzal::settings::Preset& quetzal::settings::SettingsGroup::selected() const
{
    auto i = std::ranges::find(m_presets, m_selected_preset_name, &Preset::name);
    assert(i != m_presets.end());
    return *i;
}

//------------------------------------------------------------------------------
bool quetzal::settings::SettingsGroup::editable() const
{
    return std::ranges::any_of(m_presets, &Preset::editable);
}

//------------------------------------------------------------------------------
const quetzal::settings::Preset& quetzal::settings::SettingsGroup::editable_preset() const
{
    auto i = std::ranges::find_if(m_presets, &Preset::editable);
    assert(i != m_presets.end());
    return *i;
}

//------------------------------------------------------------------------------
std::vector<std::string> quetzal::settings::SettingsGroup::setting_keys() const
{
    std::set<std::string> keys;
    for (const auto& preset : m_presets)
    {
        for (const auto& [key, value] : preset.values)
        {
            keys.insert(key);
        }
    }

    return {keys.begin(), keys.end()};
}

//------------------------------------------------------------------------------
bool quetzal::settings::SettingsGroup::contains(std::string_view preset_name) const
{
    return std::ranges::find(m_presets, preset_name, &Preset::name) != m_presets.end();
}

//------------------------------------------------------------------------------
void quetzal::settings::SettingsGroup::insert(Preset preset)
{
    if (preset.name.empty())
    {
        throw std::runtime_error(std::format("a preset of the settings group '{}' has no name", m_key));
    }

    if (contains(preset.name))
    {
        throw std::runtime_error(std::format("the settings group '{}' has the preset '{}' twice", m_key, preset.name));
    }

    if (preset.editable && editable())
    {
        throw std::runtime_error(std::format("the settings group '{}' has more than one editable preset", m_key));
    }

    if (preset.editable && preset.values.begin() != preset.values.end())
    {
        throw std::runtime_error(std::format("the editable preset '{}' of the settings group '{}' has values", preset.name, m_key));
    }

    m_presets.push_back(std::move(preset));
    return;
}

//------------------------------------------------------------------------------
bool quetzal::settings::SettingsGroup::select(std::string_view preset_name)
{
    assert(contains(preset_name));

    if (m_selected_preset_name == preset_name)
    {
        return false;
    }

    m_selected_preset_name = preset_name;
    return true;
}
