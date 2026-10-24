//------------------------------------------------------------------------------
// settings
// Settings.cpp
//------------------------------------------------------------------------------

#include "Settings.hpp"
#include <cstdint>
#include <format>
#include <set>
#include <stdexcept>
#include <vector>

namespace
{

    //--------------------------------------------------------------------------
    template<typename T>
    bool bounded_inclusive(std::string_view text, const std::optional<std::string>& minimum, const std::optional<std::string>& maximum)
    {
        T value = *quetzal::try_type<T>(text);
        return (!minimum.has_value() || *quetzal::try_type<T>(*minimum) <= value) && (!maximum.has_value() || value <= *quetzal::try_type<T>(*maximum));
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::settings::Settings::Settings() :
    m_registered_types(),
    m_settings(),
    m_groups(),
    m_member_groups(),
    m_presets(),
    m_change_listeners()
{
    register_built_in_types();
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::register_enumeration(std::string_view type_name, std::vector<std::string> values)
{
    if (registered_type(type_name))
    {
        throw std::runtime_error(std::format("the setting type '{}' is defined twice", type_name));
    }

    if (values.empty())
    {
        throw std::runtime_error(std::format("the enumeration '{}' has no values", type_name));
    }

    auto accepts = [values](std::string_view text) { return std::ranges::find(values, text) != values.end(); };
    m_registered_types.emplace(std::string(type_name), SettingType {std::type_index(typeid(std::string)), SettingBaseType::Text, std::move(values),
        accepts});
    return;
}

//------------------------------------------------------------------------------
bool quetzal::settings::Settings::registered_type(std::string_view type_name) const
{
    return m_registered_types.contains(type_name);
}

//------------------------------------------------------------------------------
const quetzal::settings::SettingType& quetzal::settings::Settings::type(std::string_view type_name) const
{
    auto i = m_registered_types.find(type_name);
    assert(i != m_registered_types.end());
    return i->second;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::insert(std::string_view key, Setting setting)
{
    assert(!setting.value.has_value());

    if (key.empty())
    {
        throw std::runtime_error("the setting key is empty");
    }

    if (contains(key))
    {
        throw std::runtime_error(std::format("the setting '{}' is already defined", key));
    }

    if (!registered_type(setting.type_name))
    {
        throw std::runtime_error(std::format("the setting '{}' has the invalid type '{}'", key, setting.type_name));
    }

    const SettingType& setting_type = type(setting.type_name);
    if (!setting_type.accepts(setting.default_value))
    {
        throw std::runtime_error(std::format("the setting '{}' has the invalid default value '{}'", key, setting.default_value));
    }

    if (setting.minimum.has_value() || setting.maximum.has_value())
    {
        if (setting_type.base_type != SettingBaseType::Integer && setting_type.base_type != SettingBaseType::Real)
        {
            throw std::runtime_error(std::format("the setting '{}' has a minimum or a maximum, which is not allowed for its type", key));
        }

        for (const auto& bound : {setting.minimum, setting.maximum})
        {
            if (bound.has_value() && !setting_type.accepts(*bound))
            {
                throw std::runtime_error(std::format("the setting '{}' has the invalid bound value '{}'", key, *bound));
            }
        }

        if (!bounded_inclusive(setting, setting.default_value))
        {
            throw std::runtime_error(std::format("the setting '{}' has an out of range default value '{}'", key, setting.default_value));
        }
    }

    m_settings.emplace(key, std::move(setting));
    return;
}

//------------------------------------------------------------------------------
bool quetzal::settings::Settings::contains(std::string_view key) const
{
    return m_settings.contains(key);
}

//------------------------------------------------------------------------------
const quetzal::settings::Setting& quetzal::settings::Settings::setting(std::string_view key) const
{
    auto i = m_settings.find(key);
    assert(i != m_settings.end());
    return i->second;
}

//------------------------------------------------------------------------------
bool quetzal::settings::Settings::bounded_inclusive(std::string_view key, std::string_view value) const
{
    const Setting& key_setting = setting(key);
    assert(type(key_setting.type_name).accepts(value));

    return bounded_inclusive(key_setting, value);
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::insert_group(std::string_view key, std::string_view scope, bool modifiable, std::string_view default_preset_name,
    std::vector<std::string> member_keys, std::vector<std::pair<std::string, Preset>> presets, std::string_view display_name,
    std::string_view category)
{
    if (key.empty())
    {
        throw std::runtime_error("the setting group key is empty");
    }

    if (contains(key) || registered_type(key))
    {
        throw std::runtime_error(std::format("the setting group '{}' is already defined as a setting or a type", key));
    }

    if (member_keys.empty())
    {
        throw std::runtime_error(std::format("the setting group '{}' has no members", key));
    }

    if (std::set<std::string>(member_keys.begin(), member_keys.end()).size() != member_keys.size())
    {
        throw std::runtime_error(std::format("the setting group '{}' has a member more than once", key));
    }

    for (const auto& member_key : member_keys)
    {
        if (!contains(member_key))
        {
            throw std::runtime_error(std::format("the member '{}' of the setting group '{}' is not a setting", member_key, key));
        }

        const Setting& member = setting(member_key);

        // Nested groups would be allowed here, with current_value resolving a member group's selection through its own group
        if (type(member.type_name).group)
        {
            throw std::runtime_error(std::format("the member '{}' of the setting group '{}' is a setting group", member_key, key));
        }

        if (member.scope != scope)
        {
            throw std::runtime_error(std::format("the member '{}' of the setting group '{}' is not of the group's scope", member_key, key));
        }

        if (auto member_group = m_member_groups.find(member_key); member_group != m_member_groups.end())
        {
            throw std::runtime_error(std::format("the member '{}' of the setting group '{}' is a member of the setting group '{}'", member_key, key,
                member_group->second));
        }

        assert(!member.value.has_value());
    }

    std::vector<std::string> preset_names;
    bool modifiable_found = false;
    for (const auto& [preset_name, key_preset] : presets)
    {
        if (preset_name.empty())
        {
            throw std::runtime_error(std::format("a preset of the setting group '{}' has no name", key));
        }

        if (std::ranges::find(preset_names, preset_name) != preset_names.end())
        {
            throw std::runtime_error(std::format("the setting group '{}' has the preset '{}' twice", key, preset_name));
        }

        if (key_preset.modifiable)
        {
            if (modifiable_found)
            {
                throw std::runtime_error(std::format("the setting group '{}' has more than one modifiable preset", key));
            }

            if (!key_preset.values.empty())
            {
                throw std::runtime_error(std::format("the modifiable preset '{}' of the setting group '{}' has values", preset_name, key));
            }

            modifiable_found = true;
        }
        else
        {
            check_preset_members(key, preset_name, member_keys, key_preset.values);
            for (const auto& [member_key, value] : key_preset.values)
            {
                if (!bounded_inclusive(member_key, value))
                {
                    throw std::runtime_error(std::format("the preset '{}' of the setting group '{}' sets '{}' to '{}', which is out of its range",
                        preset_name, key, member_key, value));
                }
            }
        }

        preset_names.push_back(preset_name);
    }

    auto default_preset = std::ranges::find(presets, default_preset_name, &std::pair<std::string, Preset>::first);
    if (default_preset == presets.end())
    {
        throw std::runtime_error(std::format("the setting group '{}' has no default preset '{}'", key, default_preset_name));
    }

    if (default_preset->second.modifiable)
    {
        throw std::runtime_error(std::format("the default preset '{}' of the setting group '{}' is modifiable", default_preset_name, key));
    }

    const std::string group_key(key);
    auto accepts = [preset_names](std::string_view text) { return std::ranges::find(preset_names, text) != preset_names.end(); };
    m_registered_types.emplace(group_key, SettingType {std::type_index(typeid(std::string)), SettingBaseType::Text, preset_names, accepts, true});
    m_settings.emplace(group_key, Setting {.type_name = group_key, .default_value = std::string(default_preset_name), .scope = std::string(scope),
        .modifiable = modifiable, .display_name = std::string(display_name), .category = std::string(category)});

    for (const auto& member_key : member_keys)
    {
        m_member_groups.emplace(member_key, group_key);
    }

    m_groups.emplace(group_key, SettingGroup {group_key, std::move(member_keys)});
    for (auto& [preset_name, key_preset] : presets)
    {
        m_presets.emplace(preset_key(group_key, preset_name), std::move(key_preset));
    }

    return;
}

//------------------------------------------------------------------------------
bool quetzal::settings::Settings::contains_group(std::string_view key) const
{
    return m_groups.contains(key);
}

//------------------------------------------------------------------------------
const quetzal::settings::SettingGroup& quetzal::settings::Settings::group(std::string_view key) const
{
    auto i = m_groups.find(key);
    assert(i != m_groups.end());
    return i->second;
}

//------------------------------------------------------------------------------
std::optional<std::string> quetzal::settings::Settings::member_group_key(std::string_view key) const
{
    auto i = m_member_groups.find(key);
    return i != m_member_groups.end() ? std::make_optional(i->second) : std::nullopt;
}

//------------------------------------------------------------------------------
const quetzal::settings::Preset& quetzal::settings::Settings::preset(std::string_view group_key, std::string_view preset_name) const
{
    auto i = m_presets.find(preset_key(group_key, preset_name));
    assert(i != m_presets.end());
    return i->second;
}

//------------------------------------------------------------------------------
std::string quetzal::settings::Settings::text(std::string_view key) const
{
    if (!contains(key))
    {
        throw std::runtime_error(std::format("'{}' is not a defined setting", key));
    }

    return current_value(key);
}

//------------------------------------------------------------------------------
bool quetzal::settings::Settings::set_text(std::string_view key, std::string_view value)
{
    KeyValueStore values;
    values.insert(std::string(key), std::string(value));
    return set_values(values);
}

//------------------------------------------------------------------------------
quetzal::KeyValueStore quetzal::settings::Settings::values(std::string_view scope) const
{
    KeyValueStore scope_values;
    for (const auto& [key, key_setting] : m_settings)
    {
        if (key_setting.scope == scope && key_setting.value.has_value())
        {
            scope_values.insert(key, *key_setting.value);
        }
    }

    return scope_values;
}

//------------------------------------------------------------------------------
bool quetzal::settings::Settings::set_values(const KeyValueStore& values)
{
    return set_values(values, {});
}

//------------------------------------------------------------------------------
bool quetzal::settings::Settings::set_values(const KeyValueStore& values, const preset_values_type& preset_values)
{
    check_modifiable(values, preset_values);
    return change_values(values, preset_values);
}

//------------------------------------------------------------------------------
bool quetzal::settings::Settings::initialize_values(const KeyValueStore& values, const preset_values_type& preset_values)
{
    return change_values(values, preset_values);
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::reset(std::string_view scope)
{
    std::vector<std::string> keys;
    for (const auto& [key, key_setting] : m_settings)
    {
        if (key_setting.scope == scope)
        {
            keys.push_back(key);
        }
    }

    const std::vector<std::string> original_values = current_values(keys);

    for (const auto& key : keys)
    {
        m_settings.find(key)->second.value.reset();
    }

    for (auto& [key, key_preset] : m_presets)
    {
        if (key_preset.modifiable && setting(key.first).scope == scope)
        {
            key_preset.values.clear();
        }
    }

    notify(changed_keys(keys, original_values));
    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::apply_initializers(std::string_view scope)
{
    KeyValueStore initial_values;
    for (const auto& [key, key_setting] : m_settings)
    {
        if (key_setting.scope != scope || !key_setting.initializer_setting.has_value() || !contains(*key_setting.initializer_setting))
        {
            continue;
        }

        const std::string& initializer_key = *key_setting.initializer_setting;
        const std::string value = current_value(initializer_key);
        if (!type(key_setting.type_name).accepts(value) || !bounded_inclusive(key_setting, value))
        {
            throw std::runtime_error(std::format("the initializer setting '{}' of the setting '{}' has the value '{}', which is not a valid value of it",
                initializer_key, key, value));
        }

        initial_values.insert(key, value);
    }

    [[maybe_unused]] const bool initialized = initialize_values(initial_values, {});
    assert(initialized);
    return;
}

//------------------------------------------------------------------------------
quetzal::settings::Settings::preset_values_type quetzal::settings::Settings::preset_values(std::string_view scope) const
{
    preset_values_type scope_values;
    for (const auto& [key, key_preset] : m_presets)
    {
        if (key_preset.modifiable && !key_preset.values.empty() && setting(key.first).scope == scope)
        {
            scope_values.emplace(key, key_preset.values);
        }
    }

    return scope_values;
}

//------------------------------------------------------------------------------
quetzal::settings::Settings::listener_id_type quetzal::settings::Settings::add_change_listener(
    std::function<void(const std::vector<std::string>& keys_changed)> listener)
{
    m_change_listeners.emplace(m_next_listener_id, std::move(listener));
    return m_next_listener_id++;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::remove_listener(listener_id_type listener_id)
{
    assert(m_change_listeners.contains(listener_id));

    m_change_listeners.erase(listener_id);
    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::clear()
{
    m_registered_types.clear();
    m_settings.clear();
    m_groups.clear();
    m_member_groups.clear();
    m_presets.clear();
    register_built_in_types();
    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::register_built_in_types()
{
    register_type<bool>("boolean");
    register_type<std::int64_t>("integer");
    register_type<double>("real");
    register_type<std::string>("text");
    return;
}

//------------------------------------------------------------------------------
quetzal::settings::Settings::preset_key_type quetzal::settings::Settings::preset_key(std::string_view group_key, std::string_view preset_name)
{
    return {std::string(group_key), std::string(preset_name)};
}

//------------------------------------------------------------------------------
std::string quetzal::settings::Settings::current_value(std::string_view key) const
{
    if (auto member_group = m_member_groups.find(key); member_group != m_member_groups.end())
    {
        const Preset& selected = selected_preset(member_group->second);
        assert(!selected.values.empty());
        return *selected.values.value(key);
    }

    const Setting& key_setting = setting(key);
    return key_setting.value.value_or(key_setting.default_value);
}

//------------------------------------------------------------------------------
bool quetzal::settings::Settings::bounded_inclusive(const Setting& setting, std::string_view text) const
{
    switch (type(setting.type_name).base_type)
    {
        case SettingBaseType::Integer:
            return ::bounded_inclusive<std::int64_t>(text, setting.minimum, setting.maximum);
        case SettingBaseType::Real:
            return ::bounded_inclusive<double>(text, setting.minimum, setting.maximum);
        default:
            return true;
    }
}

//------------------------------------------------------------------------------
const quetzal::settings::Preset& quetzal::settings::Settings::selected_preset(std::string_view group_key) const
{
    return preset(group_key, current_value(group_key));
}

//------------------------------------------------------------------------------
std::optional<std::string> quetzal::settings::Settings::modifiable_preset_name(std::string_view group_key) const
{
    for (const auto& preset_name : type(setting(group_key).type_name).values)
    {
        if (preset(group_key, preset_name).modifiable)
        {
            return preset_name;
        }
    }

    return std::nullopt;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::check_preset_members(std::string_view group_key, std::string_view preset_name,
    const std::vector<std::string>& member_keys, const KeyValueStore& values) const
{
    for (const auto& member_key : member_keys)
    {
        if (!values.contains(member_key))
        {
            throw std::runtime_error(std::format("the preset '{}' of the setting group '{}' does not set its member '{}'", preset_name, group_key,
                member_key));
        }
    }

    for (const auto& [key, value] : values)
    {
        if (std::ranges::find(member_keys, key) == member_keys.end())
        {
            throw std::runtime_error(std::format("the preset '{}' of the setting group '{}' sets '{}', which is not one of its members", preset_name,
                group_key, key));
        }

        if (!type(setting(key).type_name).accepts(value))
        {
            throw std::runtime_error(std::format("the preset '{}' of the setting group '{}' sets '{}' to '{}', which is no value of its type",
                preset_name, group_key, key, value));
        }
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::check_modifiable_preset_values(const preset_key_type& key, const KeyValueStore& values) const
{
    const auto& [group_key, preset_name] = key;
    if (!contains_group(group_key))
    {
        throw std::runtime_error(std::format("'{}' is not a setting group", group_key));
    }

    auto i = m_presets.find(key);
    if (i == m_presets.end())
    {
        throw std::runtime_error(std::format("'{}' is not a preset of the setting group '{}'", preset_name, group_key));
    }

    if (!i->second.modifiable)
    {
        throw std::runtime_error(std::format("the preset '{}' of the setting group '{}' is not modifiable", preset_name, group_key));
    }

    check_preset_members(group_key, preset_name, group(group_key).member_keys, values);
    return;
}

//------------------------------------------------------------------------------
bool quetzal::settings::Settings::check_values(const KeyValueStore& values) const
{
    for (const auto& [key, value] : values)
    {
        if (!contains(key))
        {
            throw std::runtime_error(std::format("'{}' is not a defined setting", key));
        }

        if (!type(setting(key).type_name).accepts(value))
        {
            throw std::runtime_error(std::format("'{}' is not a valid value of the setting '{}'", value, key));
        }

        std::optional<std::string> group_key = member_group_key(key);
        if (group_key.has_value() && !modifiable_preset_name(*group_key).has_value())
        {
            throw std::runtime_error(std::format("the setting '{}' is a member of the setting group '{}', which has no modifiable preset", key, *group_key));
        }
    }

    // Ranges are checked after every type, since a range check requires a value of the type
    return std::ranges::all_of(values, [this](const auto& key_value) { return bounded_inclusive(key_value.first, key_value.second); });
}

//------------------------------------------------------------------------------
bool quetzal::settings::Settings::check_presets(const preset_values_type& preset_values) const
{
    for (const auto& [key, key_values] : preset_values)
    {
        check_modifiable_preset_values(key, key_values);
    }

    return std::ranges::all_of(preset_values, [this](const auto& preset_key_values)
    {
        return std::ranges::all_of(preset_key_values.second, [this](const auto& key_value) { return bounded_inclusive(key_value.first, key_value.second); });
    });
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::check_modifiable(const KeyValueStore& values, const preset_values_type& preset_values) const
{
    for (const auto& [key, value] : values)
    {
        if (!contains(key))
        {
            continue;
        }

        if (!setting(key).modifiable)
        {
            throw std::runtime_error(std::format("the setting '{}' is not modifiable", key));
        }

        if (std::optional<std::string> group_key = member_group_key(key); group_key.has_value())
        {
            if (!setting(*group_key).modifiable)
            {
                throw std::runtime_error(std::format("the setting '{}' is not modifiable since its setting group '{}' is not modifiable", key, *group_key));
            }

            if (!modifiable_preset_name(*group_key).has_value())
            {
                throw std::runtime_error(std::format("the setting '{}' is not modifiable since its setting group '{}' has no modifiable preset", key, *group_key));
            }
        }
    }

    for (const auto& [key, key_values] : preset_values)
    {
        const std::string& group_key = key.first;
        if (!contains_group(group_key))
        {
            continue;
        }

        if (!setting(group_key).modifiable)
        {
            throw std::runtime_error(std::format("the setting group '{}' is not modifiable", group_key));
        }

        for (const auto& [member_key, value] : key_values)
        {
            if (contains(member_key) && !setting(member_key).modifiable)
            {
                throw std::runtime_error(std::format("the setting '{}' is not modifiable", member_key));
            }
        }
    }

    return;
}

//------------------------------------------------------------------------------
bool quetzal::settings::Settings::change_values(const KeyValueStore& values, const preset_values_type& preset_values)
{
    // Both are checked before either returns, so an invalid value throws even when another is out of range
    const bool values_in_range = check_values(values);
    const bool presets_in_range = check_presets(preset_values);
    if (!values_in_range || !presets_in_range)
    {
        return false;
    }

    const std::vector<std::string> keys = extract_keys(values, preset_values);
    const std::vector<std::string> original_values = current_values(keys);

    // Preset values are assigned before the settings, so a value that selects a modifiable preset selects its new values
    assign_presets(preset_values);
    assign_values(values);

    notify(changed_keys(keys, original_values));
    return true;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::assign_presets(const preset_values_type& preset_values)
{
    for (const auto& [key, key_values] : preset_values)
    {
        m_presets.find(key)->second.values = key_values;
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::assign_values(const KeyValueStore& values)
{
    // Selects each group's preset before any member is set, since a member is set in the preset its group then has
    for (const auto& [key, value] : values)
    {
        if (contains_group(key))
        {
            select(key, value);
        }
    }

    for (const auto& [key, value] : values)
    {
        if (member_group_key(key).has_value())
        {
            set_member(key, value);
        }
        else if (!contains_group(key))
        {
            m_settings.find(key)->second.value = value;
        }
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::select(std::string_view group_key, std::string_view preset_name)
{
    Preset& selection = m_presets.find(preset_key(group_key, preset_name))->second;
    if (selection.modifiable && selection.values.empty())
    {
        selection.values = selected_preset(group_key).values;
    }

    m_settings.find(group_key)->second.value = std::string(preset_name);
    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::set_member(std::string_view key, std::string_view value)
{
    const std::string group_key = *member_group_key(key);
    const std::string modifiable_name = *modifiable_preset_name(group_key);
    Preset& modifiable = m_presets.find(preset_key(group_key, modifiable_name))->second;

    if (current_value(group_key) != modifiable_name)
    {
        modifiable.values = selected_preset(group_key).values;
        m_settings.find(group_key)->second.value = modifiable_name;
    }

    modifiable.values.insert(std::string(key), std::string(value));
    return;
}

//------------------------------------------------------------------------------
std::vector<std::string> quetzal::settings::Settings::extract_keys(const KeyValueStore& values, const preset_values_type& preset_values) const
{
    std::set<std::string> keys;

    // Nested groups would add the members of each member that is a group here
    auto insert_group_keys = [this, &keys](const std::string& group_key)
    {
        keys.insert(group_key);
        keys.insert(group(group_key).member_keys.begin(), group(group_key).member_keys.end());
    };

    for (const auto& [key, value] : values)
    {
        keys.insert(key);
        if (contains_group(key))
        {
            insert_group_keys(key);
        }
        else if (std::optional<std::string> group_key = member_group_key(key); group_key.has_value())
        {
            insert_group_keys(*group_key);
        }
    }

    for (const auto& [key, key_values] : preset_values)
    {
        insert_group_keys(key.first);
    }

    return {keys.begin(), keys.end()};
}

//------------------------------------------------------------------------------
std::vector<std::string> quetzal::settings::Settings::current_values(const std::vector<std::string>& keys) const
{
    std::vector<std::string> texts;
    for (const auto& key : keys)
    {
        texts.push_back(current_value(key));
    }

    return texts;
}

//------------------------------------------------------------------------------
std::vector<std::string> quetzal::settings::Settings::changed_keys(const std::vector<std::string>& keys,
    const std::vector<std::string>& original_values) const
{
    std::vector<std::string> keys_changed;
    for (size_t i = 0; i < keys.size(); ++i)
    {
        if (current_value(keys[i]) != original_values[i])
        {
            keys_changed.push_back(keys[i]);
        }
    }

    return keys_changed;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::notify(const std::vector<std::string>& keys_changed)
{
    if (keys_changed.empty())
    {
        return;
    }

    for (const auto& [listener_id, listener] : m_change_listeners)
    {
        listener(keys_changed);
    }

    return;
}

//------------------------------------------------------------------------------
quetzal::settings::Settings& quetzal::settings::store()
{
    static Settings s_store;
    return s_store;
}
