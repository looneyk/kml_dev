//------------------------------------------------------------------------------
// settings
// SettingsSnapshot.cpp
//------------------------------------------------------------------------------

#include "SettingsSnapshot.hpp"
#include <cassert>
#include <format>
#include <stdexcept>
#include <string_view>

namespace
{

    //--------------------------------------------------------------------------
    std::string_view scope_name(quetzal::settings::SettingScope scope)
    {
        return scope == quetzal::settings::SettingScope::Game ? "game" : "user";
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::settings::SettingsSnapshot quetzal::settings::SettingsSnapshot::capture(SettingScope scope, const Settings& store,
    const SettingGroups& group_store)
{
    SettingsSnapshot snapshot;
    for (const auto& [key, value] : store.values(scope))
    {
        if (!group_store.has_value_from_preset(key))
        {
            snapshot.settings.insert(key, value);
        }
    }

    snapshot.groups = group_store.values(scope);
    return snapshot;
}

//------------------------------------------------------------------------------
void quetzal::settings::SettingsSnapshot::restore(SettingScope scope, Settings& store, SettingGroups& group_store) const
{
    for (const auto& [key, preset_name] : groups)
    {
        if (!group_store.contains(key) || group_store.group(key).scope() != scope)
        {
            throw std::runtime_error(std::format("'{}' is not a {} setting group", key, scope_name(scope)));
        }

        if (!group_store.group(key).contains(preset_name))
        {
            throw std::runtime_error(std::format("'{}' is not a preset of the setting group '{}'", preset_name, key));
        }
    }

    for (const auto& [key, value] : settings)
    {
        if (!store.contains(key) || store.setting(key).scope != scope)
        {
            throw std::runtime_error(std::format("'{}' is not a {} setting", key, scope_name(scope)));
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

    group_store.set_values(groups);
    [[maybe_unused]] const bool set = store.set_values(settings);
    assert(set);
    return;
}
