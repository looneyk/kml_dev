//------------------------------------------------------------------------------
// settings
// Settings.cpp
//------------------------------------------------------------------------------

#include "Settings.hpp"
#include <cstdint>
#include <format>
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
    m_pre_change_listeners(),
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
std::string quetzal::settings::Settings::text(std::string_view key) const
{
    if (!contains(key))
    {
        throw std::runtime_error(std::format("'{}' is not a defined setting", key));
    }

    return current_text(key);
}

//------------------------------------------------------------------------------
bool quetzal::settings::Settings::set_text(std::string_view key, std::string_view value)
{
    KeyValueStore values;
    values.insert(std::string(key), std::string(value));
    return set_values(values);
}

//------------------------------------------------------------------------------
quetzal::KeyValueStore quetzal::settings::Settings::values(SettingScope scope) const
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
    for (const auto& [key, value] : values)
    {
        if (!contains(key))
        {
            throw std::runtime_error(std::format("'{}' is not a defined setting", key));
        }

        if (!type(setting(key).type_name).accepts(value))
        {
            throw std::runtime_error(std::format("'{}' is not an valid value of the setting '{}'", value, key));
        }
    }

    for (const auto& [key, value] : values)
    {
        if (!bounded_inclusive(key, value))
        {
            return false;
        }
    }

    for (const auto& [listener_id, listener] : m_pre_change_listeners)
    {
        listener(values);
    }

    std::vector<std::string> keys_changed;
    for (const auto& [key, value] : values)
    {
        if (assign(key, value))
        {
            keys_changed.push_back(key);
        }
    }

    notify(keys_changed);
    return true;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::reset(SettingScope scope)
{
    std::vector<std::string> keys_changed;

    for (auto i = m_settings.begin(); i != m_settings.end(); ++i)
    {
        Setting& key_setting = i->second;
        if (key_setting.scope != scope || !key_setting.value.has_value())
        {
            continue;
        }

        if (*key_setting.value != key_setting.default_value)
        {
            keys_changed.push_back(i->first);
        }

        key_setting.value.reset();
    }

    notify(keys_changed);
    return;
}

//------------------------------------------------------------------------------
quetzal::settings::Settings::listener_id_type quetzal::settings::Settings::add_pre_change_listener(std::function<void(const KeyValueStore& values)> listener)
{
    m_pre_change_listeners.emplace(m_next_listener_id, std::move(listener));
    return m_next_listener_id++;
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
    assert(m_pre_change_listeners.contains(listener_id) || m_change_listeners.contains(listener_id));

    m_pre_change_listeners.erase(listener_id);
    m_change_listeners.erase(listener_id);
    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::clear()
{
    m_registered_types.clear();
    m_settings.clear();
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
std::string quetzal::settings::Settings::current_text(std::string_view key) const
{
    const Setting& key_setting = setting(key);
    return key_setting.value.value_or(key_setting.default_value);
}

//------------------------------------------------------------------------------
bool quetzal::settings::Settings::assign(std::string_view key, std::string_view value)
{
    auto i = m_settings.find(key);
    assert(i != m_settings.end() && type(i->second.type_name).accepts(value));

    bool changed = current_text(key) != value;
    i->second.value = std::string(value);
    return changed;
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
