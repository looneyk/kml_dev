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

//------------------------------------------------------------------------------
quetzal::settings::Settings::Settings() :
    m_registered_types(),
    m_declarations(),
    m_values(),
    m_section_listeners()
{
    register_built_in_types();
}

//------------------------------------------------------------------------------
bool quetzal::settings::Settings::registers_type(std::string_view type_name) const
{
    return m_registered_types.contains(type_name);
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::declare(std::string_view key, std::string_view type_name, std::string_view default_text,
    SettingScope scope, bool restart_required)
{
    if (!is_setting_key(key))
    {
        throw std::runtime_error(std::format("'{}' is not a setting key: it has no section, or an empty segment", key));
    }

    if (declares(key))
    {
        throw std::runtime_error(std::format("the setting '{}' is declared twice", key));
    }

    auto i = m_registered_types.find(type_name);
    if (i == m_registered_types.end())
    {
        throw std::runtime_error(std::format("the setting '{}' has the type '{}', which is no type a setting may have", key, type_name));
    }

    if (!i->second.accepts(default_text))
    {
        throw std::runtime_error(std::format("the setting '{}' has the default '{}', which is no value of its type", key, default_text));
    }

    m_declarations.emplace(std::string(key), SettingDeclaration {i->second.type, std::string(default_text), scope, restart_required,
        i->second.accepts});
    return;
}

//------------------------------------------------------------------------------
bool quetzal::settings::Settings::declares(std::string_view key) const
{
    return m_declarations.contains(key);
}

//------------------------------------------------------------------------------
const quetzal::settings::SettingDeclaration& quetzal::settings::Settings::declaration(std::string_view key) const
{
    auto i = m_declarations.find(key);
    assert(i != m_declarations.end());
    return i->second;
}

//------------------------------------------------------------------------------
std::string quetzal::settings::Settings::text(std::string_view key) const
{
    if (!declares(key))
    {
        throw std::runtime_error(std::format("'{}' is not a setting", key));
    }

    return value_text(key);
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::set_text(std::string_view key, std::string_view text)
{
    KeyValueStore values;
    values.insert(std::string(key), std::string(text));
    set_values(values);
    return;
}

//------------------------------------------------------------------------------
quetzal::KeyValueStore quetzal::settings::Settings::values(SettingScope scope) const
{
    KeyValueStore scope_values;
    for (const auto& [key, text] : m_values)
    {
        if (declaration(key).scope == scope)
        {
            scope_values.insert(key, text);
        }
    }

    return scope_values;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::set_values(const KeyValueStore& values)
{
    for (const auto& [key, text] : values)
    {
        if (!declares(key))
        {
            throw std::runtime_error(std::format("'{}' is not a setting", key));
        }

        if (!declaration(key).accepts(text))
        {
            throw std::runtime_error(std::format("'{}' is not a value of the setting '{}'", text, key));
        }
    }

    std::vector<std::string> keys_changed;
    for (const auto& [key, text] : values)
    {
        if (assign(key, text))
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

    for (auto i = m_values.begin(); i != m_values.end();)
    {
        const SettingDeclaration& key_declaration = declaration(i->first);
        if (key_declaration.scope != scope)
        {
            ++i;
            continue;
        }

        if (i->second != key_declaration.default_text)
        {
            keys_changed.push_back(i->first);
        }

        i = m_values.erase(i);
    }

    notify(keys_changed);
    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::add_section_listener(std::string_view section, std::function<void()> apply)
{
    m_section_listeners.emplace(std::string(section), std::move(apply));
    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::clear()
{
    m_registered_types.clear();
    m_declarations.clear();
    m_values.clear();
    m_section_listeners.clear();
    register_built_in_types();
    return;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::register_built_in_types()
{
    register_type<bool>("bool");
    register_type<std::int64_t>("integer");
    register_type<double>("real");
    register_type<std::string>("text");
    return;
}

//------------------------------------------------------------------------------
std::string quetzal::settings::Settings::value_text(std::string_view key) const
{
    auto i = m_values.find(key);
    return i != m_values.end() ? i->second : declaration(key).default_text;
}

//------------------------------------------------------------------------------
bool quetzal::settings::Settings::assign(std::string_view key, std::string_view text)
{
    assert(declaration(key).accepts(text));

    bool changed = value_text(key) != text;
    m_values.insert_or_assign(std::string(key), std::string(text));
    return changed;
}

//------------------------------------------------------------------------------
void quetzal::settings::Settings::notify(const std::vector<std::string>& keys_changed)
{
    std::set<std::string_view> sections_changed;
    for (const auto& key : keys_changed)
    {
        for (std::string_view section : sections_of(key))
        {
            sections_changed.insert(section);
        }
    }

    for (std::string_view section : sections_changed)
    {
        auto [first, last] = m_section_listeners.equal_range(section);
        for (auto i = first; i != last; ++i)
        {
            i->second();
        }
    }

    return;
}

//------------------------------------------------------------------------------
quetzal::settings::Settings& quetzal::settings::store()
{
    static Settings s_store;
    return s_store;
}
