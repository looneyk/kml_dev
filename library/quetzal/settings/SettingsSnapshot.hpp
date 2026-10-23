#if !defined(QUETZAL_SETTINGS_SETTINGS_SNAPSHOT_HPP)
#define QUETZAL_SETTINGS_SETTINGS_SNAPSHOT_HPP
//------------------------------------------------------------------------------
// settings
// SettingsSnapshot.hpp
//------------------------------------------------------------------------------

#include "Setting.hpp"
#include "Settings.hpp"
#include "quetzal/common/KeyValueStore.hpp"

namespace quetzal::settings
{

    //--------------------------------------------------------------------------
    // One scope's values, as they are saved and restored: each setting that is set, groups' selected presets among them, and
    // the defined values of each modifiable preset, as text
    struct SettingsSnapshot
    {
        KeyValueStore settings; // <key, value>
        Settings::preset_values_type presets; // <{group key, preset name}, values>

        static SettingsSnapshot capture(SettingScope scope, const Settings& store);

        // Sets the presets' values first, then the settings, so a selected modifiable preset has its own values. Checks every
        // key, preset and value before changing any. Throws std::runtime_error if a key is not a setting's or a group's of the
        // scope, or a preset or value is invalid or out of range
        void restore(SettingScope scope, Settings& store) const;
    };

} // namespace quetzal::settings

#endif // QUETZAL_SETTINGS_SETTINGS_SNAPSHOT_HPP
