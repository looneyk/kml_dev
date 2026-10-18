//------------------------------------------------------------------------------
// settings
// SettingsGroups.cpp
//------------------------------------------------------------------------------

#include "SettingsGroups.hpp"
#include <algorithm>
#include <cassert>
#include <format>
#include <stdexcept>

//------------------------------------------------------------------------------
quetzal::settings::SettingsGroups::SettingsGroups(Settings& settings) :
    m_settings(settings),
    m_pre_change_listener_id(settings.add_pre_change_listener([this](const KeyValueStore& values) { select_editable_presets(values); })),
    m_groups(),
    m_parent_groups(),
    m_selection_listeners()
{
}

//------------------------------------------------------------------------------
quetzal::settings::SettingsGroups::~SettingsGroups()
{
    m_settings.remove_listener(m_pre_change_listener_id);
}

//------------------------------------------------------------------------------
void quetzal::settings::SettingsGroups::insert(SettingsGroup group)
{
    const std::string key = group.key();
    if (key.empty())
    {
        throw std::runtime_error("a settings group key is empty");
    }

    if (contains(key))
    {
        throw std::runtime_error(std::format("the settings group '{}' is defined twice", key));
    }

    for (const auto& preset : group.presets())
    {
        check_preset_values(key, group.scope(), preset);
    }

    for (const auto& member_key : group.setting_keys())
    {
        m_parent_groups.emplace(member_key, key);
    }

    const std::string default_preset_name = group.default_preset_name();
    m_groups.emplace(key, std::move(group));
    select_presets({{key, default_preset_name}});
    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::SettingsGroups::insert_preset(std::string_view group_key, Preset preset)
{
    if (!contains(group_key))
    {
        throw std::runtime_error(std::format("'{}' is not a settings group", group_key));
    }

    SettingsGroup& group = m_groups.find(group_key)->second;
    check_preset_values(group_key, group.scope(), preset);

    KeyValueStore values = preset.values;
    group.insert(std::move(preset));

    for (const auto& [key, value] : values)
    {
        m_parent_groups.insert_or_assign(key, std::string(group_key));
    }

    return;
}

//------------------------------------------------------------------------------
bool quetzal::settings::SettingsGroups::contains(std::string_view key) const
{
    return m_groups.contains(key);
}

//------------------------------------------------------------------------------
const quetzal::settings::SettingsGroup& quetzal::settings::SettingsGroups::group(std::string_view key) const
{
    auto i = m_groups.find(key);
    assert(i != m_groups.end());
    return i->second;
}

//------------------------------------------------------------------------------
void quetzal::settings::SettingsGroups::select(std::string_view group_key, std::string_view preset_name)
{
    KeyValueStore selections;
    selections.insert(std::string(group_key), std::string(preset_name));
    set_values(selections);
    return;
}

//------------------------------------------------------------------------------
bool quetzal::settings::SettingsGroups::has_value_from_preset(std::string_view key) const
{
    auto parent = m_parent_groups.find(key);
    return parent != m_parent_groups.end() && !group(parent->second).selected().editable;
}

//------------------------------------------------------------------------------
quetzal::KeyValueStore quetzal::settings::SettingsGroups::values(SettingScope scope) const
{
    KeyValueStore scope_values;
    for (const auto& [key, key_group] : m_groups)
    {
        if (key_group.scope() == scope && key_group.selected().name != key_group.default_preset_name())
        {
            scope_values.insert(key, key_group.selected().name);
        }
    }

    return scope_values;
}

//------------------------------------------------------------------------------
void quetzal::settings::SettingsGroups::set_values(const KeyValueStore& values)
{
    for (const auto& [key, value] : values)
    {
        if (!contains(key))
        {
            throw std::runtime_error(std::format("'{}' is not a settings group", key));
        }

        if (!group(key).contains(value))
        {
            throw std::runtime_error(std::format("'{}' is not a preset of the settings group '{}'", value, key));
        }
    }

    select_presets(values);
    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::SettingsGroups::reset(SettingScope scope)
{
    KeyValueStore selections;
    for (const auto& [key, key_group] : m_groups)
    {
        if (key_group.scope() == scope)
        {
            selections.insert(key, key_group.default_preset_name());
        }
    }

    select_presets(selections);
    return;
}

//------------------------------------------------------------------------------
quetzal::settings::Settings::listener_id_type quetzal::settings::SettingsGroups::add_selection_listener(
    std::function<void(const std::vector<std::string>& group_keys)> listener)
{
    m_selection_listeners.emplace(m_next_listener_id, std::move(listener));
    return m_next_listener_id++;
}

//------------------------------------------------------------------------------
void quetzal::settings::SettingsGroups::remove_listener(Settings::listener_id_type listener_id)
{
    assert(m_selection_listeners.contains(listener_id));

    m_selection_listeners.erase(listener_id);
    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::SettingsGroups::clear()
{
    m_groups.clear();
    m_parent_groups.clear();
    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::SettingsGroups::check_preset_values(std::string_view group_key, SettingScope scope, const Preset& preset) const
{
    for (const auto& [key, value] : preset.values)
    {
        if (!m_settings.contains(key))
        {
            throw std::runtime_error(std::format("the preset '{}' of the settings group '{}' sets '{}', which is not a setting", preset.name,
                group_key, key));
        }

        if (m_settings.setting(key).scope != scope)
        {
            throw std::runtime_error(std::format("the preset '{}' of the settings group '{}' sets '{}', whose scope is not the group's",
                preset.name, group_key, key));
        }

        auto parent = m_parent_groups.find(key);
        if (parent != m_parent_groups.end() && parent->second != group_key)
        {
            throw std::runtime_error(std::format("the preset '{}' of the settings group '{}' sets '{}', a member of the group '{}'", preset.name,
                group_key, key, parent->second));
        }

        if (!m_settings.type(m_settings.setting(key).type_name).accepts(value))
        {
            throw std::runtime_error(std::format("the preset '{}' of the settings group '{}' sets '{}' to '{}', which is no value of its type",
                preset.name, group_key, key, value));
        }

        if (!m_settings.bounded_inclusive(key, value))
        {
            throw std::runtime_error(std::format("the preset '{}' of the settings group '{}' sets '{}' to '{}', which is out of its range",
                preset.name, group_key, key, value));
        }
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::SettingsGroups::select_presets(const KeyValueStore& selections)
{
    KeyValueStore member_values;
    for (const auto& [group_key, preset_name] : selections)
    {
        const SettingsGroup& selection_group = group(group_key);
        const Preset& preset = *std::ranges::find(selection_group.presets(), preset_name, &Preset::name);
        if (preset.editable)
        {
            continue;
        }

        for (const auto& key : selection_group.setting_keys())
        {
            member_values.insert(key, preset.values.value_or(key, m_settings.setting(key).default_value));
        }
    }

    m_setting_members = true;
    try
    {
        m_settings.set_values(member_values);
    }
    catch (...)
    {
        m_setting_members = false;
        throw;
    }

    m_setting_members = false;

    std::vector<std::string> group_keys;
    for (const auto& [group_key, preset_name] : selections)
    {
        if (m_groups.find(group_key)->second.select(preset_name))
        {
            group_keys.push_back(group_key);
        }
    }

    notify(group_keys);
    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::SettingsGroups::select_editable_presets(const KeyValueStore& values)
{
    if (m_setting_members)
    {
        return;
    }

    KeyValueStore selections;
    for (const auto& [key, value] : values)
    {
        auto parent = m_parent_groups.find(key);
        if (parent == m_parent_groups.end() || group(parent->second).selected().editable)
        {
            continue;
        }

        if (!group(parent->second).editable())
        {
            throw std::runtime_error(std::format("the setting '{}' is a member of the settings group '{}', which has no editable preset", key,
                parent->second));
        }

        selections.insert(parent->second, group(parent->second).editable_preset().name);
    }

    std::vector<std::string> group_keys;
    for (const auto& [group_key, preset_name] : selections)
    {
        if (m_groups.find(group_key)->second.select(preset_name))
        {
            group_keys.push_back(group_key);
        }
    }

    notify(group_keys);
    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::SettingsGroups::notify(const std::vector<std::string>& group_keys)
{
    if (group_keys.empty())
    {
        return;
    }

    for (const auto& [listener_id, listener] : m_selection_listeners)
    {
        listener(group_keys);
    }

    return;
}

//------------------------------------------------------------------------------
quetzal::settings::SettingsGroups& quetzal::settings::groups()
{
    static SettingsGroups s_groups(store());
    return s_groups;
}
