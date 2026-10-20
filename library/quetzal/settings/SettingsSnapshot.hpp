#if !defined(QUETZAL_SETTINGS_SETTINGS_SNAPSHOT_HPP)
#define QUETZAL_SETTINGS_SETTINGS_SNAPSHOT_HPP
//------------------------------------------------------------------------------
// settings
// SettingsSnapshot.hpp
//------------------------------------------------------------------------------

#include "Setting.hpp"
#include "Settings.hpp"
#include "SettingGroups.hpp"
#include "quetzal/common/KeyValueStore.hpp"

namespace quetzal::settings
{

    //--------------------------------------------------------------------------
    // One scope's values, as they are saved and restored: each setting that is set and that no preset sets, and each group's
    // selected preset that is not its default, as text
    struct SettingsSnapshot
    {
        KeyValueStore settings; // <key, value>
        KeyValueStore groups; // <group key, preset name>

        static SettingsSnapshot capture(SettingScope scope, const Settings& store, const SettingGroups& group_store);

        // Selects the groups' presets first, then sets the settings, so an editable preset is selected before its members are
        // set. Checks every key, preset and value before changing any. Throws std::runtime_error if a key is not a setting's or
        // a group's of the scope, or a preset or value is invalid or out of range
        void restore(SettingScope scope, Settings& store, SettingGroups& group_store) const;
    };

} // namespace quetzal::settings

#endif // QUETZAL_SETTINGS_SETTINGS_SNAPSHOT_HPP
