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
    // The keys of the settings that make up the group identified by key
    struct SettingGroup
    {
        std::string key;
        std::vector<std::string> member_keys;
    };

    //--------------------------------------------------------------------------
    struct Preset
    {
        std::string display_name;
        bool modifiable = false;
        KeyValueStore values; // <member_key, value>
    };

} // namespace quetzal::settings

#endif // QUETZAL_SETTINGS_SETTING_GROUP_HPP
