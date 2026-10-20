//------------------------------------------------------------------------------
// settings
// SettingGroup.cpp
//------------------------------------------------------------------------------

#include "SettingGroup.hpp"
#include <algorithm>
#include <cassert>
#include <format>
#include <set>
#include <stdexcept>

//------------------------------------------------------------------------------
quetzal::settings::SettingGroup::SettingGroup(std::string_view key, SettingScope scope, std::string_view default_preset_name,
    std::vector<std::string> member_keys, std::vector<Preset> presets, std::string_view display_name, std::string_view category) :
    m_key(key),
    m_scope(scope),
    m_display_name(display_name),
    m_category(category),
    m_default_preset_name(default_preset_name),
    m_selected_preset_name(default_preset_name),
    m_member_keys(std::move(member_keys)),
    m_presets()
{
    if (m_member_keys.empty())
    {
        throw std::runtime_error(std::format("the setting group '{}' has no members", m_key));
    }

    if (std::set<std::string>(m_member_keys.begin(), m_member_keys.end()).size() != m_member_keys.size())
    {
        throw std::runtime_error(std::format("the setting group '{}' has a member more than once", m_key));
    }

    for (auto& preset : presets)
    {
        insert(std::move(preset));
    }

    if (!contains(m_default_preset_name))
    {
        throw std::runtime_error(std::format("the setting group '{}' has no default preset '{}'", m_key, m_default_preset_name));
    }

    if (std::ranges::find(m_presets, m_default_preset_name, &Preset::name)->editable)
    {
        throw std::runtime_error(std::format("the default preset '{}' of the setting group '{}' is editable", m_default_preset_name, m_key));
    }
}

//------------------------------------------------------------------------------
const std::string& quetzal::settings::SettingGroup::key() const
{
    return m_key;
}

//------------------------------------------------------------------------------
quetzal::settings::SettingScope quetzal::settings::SettingGroup::scope() const
{
    return m_scope;
}

//------------------------------------------------------------------------------
const std::string& quetzal::settings::SettingGroup::display_name() const
{
    return m_display_name;
}

//------------------------------------------------------------------------------
const std::string& quetzal::settings::SettingGroup::category() const
{
    return m_category;
}

//------------------------------------------------------------------------------
const std::string& quetzal::settings::SettingGroup::default_preset_name() const
{
    return m_default_preset_name;
}

//------------------------------------------------------------------------------
const std::vector<quetzal::settings::Preset>& quetzal::settings::SettingGroup::presets() const
{
    return m_presets;
}

//------------------------------------------------------------------------------
const quetzal::settings::Preset& quetzal::settings::SettingGroup::selected() const
{
    auto i = std::ranges::find(m_presets, m_selected_preset_name, &Preset::name);
    assert(i != m_presets.end());
    return *i;
}

//------------------------------------------------------------------------------
bool quetzal::settings::SettingGroup::editable() const
{
    return std::ranges::any_of(m_presets, &Preset::editable);
}

//------------------------------------------------------------------------------
const quetzal::settings::Preset& quetzal::settings::SettingGroup::editable_preset() const
{
    auto i = std::ranges::find_if(m_presets, &Preset::editable);
    assert(i != m_presets.end());
    return *i;
}

//------------------------------------------------------------------------------
const std::vector<std::string>& quetzal::settings::SettingGroup::member_keys() const
{
    return m_member_keys;
}

//------------------------------------------------------------------------------
bool quetzal::settings::SettingGroup::contains(std::string_view preset_name) const
{
    return std::ranges::find(m_presets, preset_name, &Preset::name) != m_presets.end();
}

//------------------------------------------------------------------------------
void quetzal::settings::SettingGroup::insert(Preset preset)
{
    if (preset.name.empty())
    {
        throw std::runtime_error(std::format("a preset of the setting group '{}' has no name", m_key));
    }

    if (contains(preset.name))
    {
        throw std::runtime_error(std::format("the setting group '{}' has the preset '{}' twice", m_key, preset.name));
    }

    if (preset.editable && editable())
    {
        throw std::runtime_error(std::format("the setting group '{}' has more than one editable preset", m_key));
    }

    if (preset.editable && preset.values.begin() != preset.values.end())
    {
        throw std::runtime_error(std::format("the editable preset '{}' of the setting group '{}' has values", preset.name, m_key));
    }

    if (!preset.editable)
    {
        for (const auto& member_key : m_member_keys)
        {
            if (!preset.values.contains(member_key))
            {
                throw std::runtime_error(std::format("the preset '{}' of the setting group '{}' does not set its member '{}'", preset.name, m_key,
                    member_key));
            }
        }

        for (const auto& [value_key, value] : preset.values)
        {
            if (std::ranges::find(m_member_keys, value_key) == m_member_keys.end())
            {
                throw std::runtime_error(std::format("the preset '{}' of the setting group '{}' sets '{}', which is not one of its members",
                    preset.name, m_key, value_key));
            }
        }
    }

    m_presets.push_back(std::move(preset));
    return;
}

//------------------------------------------------------------------------------
bool quetzal::settings::SettingGroup::select(std::string_view preset_name)
{
    assert(contains(preset_name));

    if (m_selected_preset_name == preset_name)
    {
        return false;
    }

    m_selected_preset_name = preset_name;
    return true;
}
