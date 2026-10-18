#if !defined(QUETZAL_SETTINGS_SETTINGS_GROUP_HPP)
#define QUETZAL_SETTINGS_SETTINGS_GROUP_HPP
//------------------------------------------------------------------------------
// settings
// SettingsGroup.hpp
//------------------------------------------------------------------------------

#include "Setting.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace quetzal::settings
{

    //--------------------------------------------------------------------------
    struct Preset
    {
        std::string name;
        std::string display_name;
        bool editable = false;
        KeyValueStore values;
    };

    //--------------------------------------------------------------------------
    class SettingsGroup
    {
    public:

        // Creates a group with the default preset selected. Throws std::runtime_error if there is no default preset or a preset is invalid
        SettingsGroup(std::string_view key, SettingScope scope, std::string_view default_preset_name, std::vector<Preset> presets,
            std::string_view display_name = {}, std::string_view category = {});

        const std::string& key() const;
        SettingScope scope() const;
        const std::string& display_name() const;
        const std::string& category() const;
        const std::string& default_preset_name() const;
        const std::vector<Preset>& presets() const;
        const Preset& selected() const;

        bool editable() const;
        const Preset& editable_preset() const;

        std::vector<std::string> setting_keys() const;

        bool contains(std::string_view preset_name) const;

        // Throws std::runtime_error if the preset's name is empty or used, or it is a second editable preset or an editable preset with values
        void insert(Preset preset);

        // Returns true if the selected preset changed
        bool select(std::string_view preset_name);

    private:

        std::string m_key;
        SettingScope m_scope;
        std::string m_display_name;
        std::string m_category; // The menu page that shows it
        std::string m_default_preset_name;
        std::string m_selected_preset_name;
        std::vector<Preset> m_presets;
    };

} // namespace quetzal::settings

#endif // QUETZAL_SETTINGS_SETTINGS_GROUP_HPP
