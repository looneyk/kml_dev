#if !defined(QUETZAL_SETTINGS_SETTING_GROUP_HPP)
#define QUETZAL_SETTINGS_SETTING_GROUP_HPP
//------------------------------------------------------------------------------
// settings
// SettingGroup.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/KeyValueStore.hpp"
#include <string>
#include <vector>

namespace quetzal::settings
{

    //--------------------------------------------------------------------------
    // A setting whose value is the name of its selected preset, and the settings its presets assign
    struct SettingGroup
    {
        std::string key;
        std::vector<std::string> member_keys;
    };

    //--------------------------------------------------------------------------
    struct Preset
    {
        std::string display_name;
        bool editable = false;
        KeyValueStore values; // <member key, value>; empty for an editable preset not yet defined
    };

} // namespace quetzal::settings

#endif // QUETZAL_SETTINGS_SETTING_GROUP_HPP
