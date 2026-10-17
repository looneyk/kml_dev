//------------------------------------------------------------------------------
// settings
// Settings.cpp
//------------------------------------------------------------------------------

#include "Settings.hpp"
#include <cstdint>
#include <format>
#include <stdexcept>
#include <vector>

//------------------------------------------------------------------------------
quetzal::settings::Settings::Settings() :
    m_registered_types(),
    m_settings(),
    m_change_listeners()
{
    register_built_in_types();
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
void quetzal::settings::Settings::insert(std::string_view key, std::string_view type_name, std::string_view default_value,
    SettingScope scope, bool restart_required)
{
    if (key.empty())
    {
        throw std::runtime_error("a setting key is empty");
    }

    if (contains(key))
    {
        throw std::runtime_error(std::format("the setting '{}' is defined twice", key));
    }

    if (!registered_type(type_name))
    {
        throw std::runtime_error(std::format("the setting '{}' has the type '{}', which is no type a setting may have", key, type_name));
    }

    if (!type(type_name).accepts(default_value))
    {
        throw std::runtime_error(std::format("the setting '{}' has the default '{}', which is no value of its type", key, default_value));
    }

    m_settings.emplace(std::string(key), Setting {std::string(type_name), std::string(default_value), scope, restart_required, std::nullopt});
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
std::string quetzal::settings::Settings::text(std::string_view key) const
{
    if (!contains(key))
    {
        throw std::runtime_error(std::format("'{}' is not a setting", key));
    }

    return current_text(key);
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::set_text(std::string_view key, std::string_view value)
{
    KeyValueStore values;
    values.insert(std::string(key), std::string(value));
    set_values(values);
    return;
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
void quetzal::settings::Settings::set_values(const KeyValueStore& values)
{
    for (const auto& [key, value] : values)
    {
        if (!contains(key))
        {
            throw std::runtime_error(std::format("'{}' is not a setting", key));
        }

        if (!type(setting(key).type_name).accepts(value))
        {
            throw std::runtime_error(std::format("'{}' is not a value of the setting '{}'", value, key));
        }
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
    return;
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
void quetzal::settings::Settings::add_change_listener(std::function<void(const std::vector<std::string>& keys_changed)> listener)
{
    m_change_listeners.push_back(std::move(listener));
    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::clear()
{
    m_registered_types.clear();
    m_settings.clear();
    m_change_listeners.clear();
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
void quetzal::settings::Settings::notify(const std::vector<std::string>& keys_changed)
{
    if (keys_changed.empty())
    {
        return;
    }

    for (const auto& listener : m_change_listeners)
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
