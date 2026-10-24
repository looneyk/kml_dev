//------------------------------------------------------------------------------
// settings
// SettingsSnapshot.cpp
//------------------------------------------------------------------------------

#include "SettingsSnapshot.hpp"
#include <cassert>
#include <format>
#include <stdexcept>
#include <string_view>

//------------------------------------------------------------------------------
quetzal::settings::SettingsSnapshot quetzal::settings::SettingsSnapshot::capture(std::string_view scope, const Settings& store)
{
    return {store.values(scope), store.preset_values(scope)};
}

//------------------------------------------------------------------------------
void quetzal::settings::SettingsSnapshot::restore(std::string_view scope, Settings& store) const
{
    for (const auto& [key, key_values] : presets)
    {
        const auto& [group_key, preset_name] = key;
        if (!store.contains_group(group_key) || store.setting(group_key).scope != scope)
        {
            throw std::runtime_error(std::format("'{}' is not a setting group of the scope '{}'", group_key, scope));
        }

        for (const auto& [member_key, value] : key_values)
        {
            if (store.contains(member_key) && store.type(store.setting(member_key).type_name).accepts(value)
                && !store.bounded_inclusive(member_key, value))
            {
                throw std::runtime_error(std::format("'{}' is out of the range of the setting '{}'", value, member_key));
            }
        }
    }

    for (const auto& [key, value] : settings)
    {
        if (!store.contains(key) || store.setting(key).scope != scope)
        {
            throw std::runtime_error(std::format("'{}' is not a setting of the scope '{}'", key, scope));
        }

        if (!store.type(store.setting(key).type_name).accepts(value))
        {
            throw std::runtime_error(std::format("'{}' is not a valid value of the setting '{}'", value, key));
        }

        if (!store.bounded_inclusive(key, value))
        {
            throw std::runtime_error(std::format("'{}' is out of the range of the setting '{}'", value, key));
        }
    }

    [[maybe_unused]] const bool set = store.initialize_values(settings, presets);
    assert(set);
    return;
}
