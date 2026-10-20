#if !defined(QUETZAL_SETTINGS_SETTING_GROUPS_HPP)
#define QUETZAL_SETTINGS_SETTING_GROUPS_HPP
//------------------------------------------------------------------------------
// settings
// SettingGroups.hpp
//------------------------------------------------------------------------------

#include "Settings.hpp"
#include "SettingGroup.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace quetzal::settings
{

    //--------------------------------------------------------------------------
    // The setting groups of a store, which sets their members through its public functions
    class SettingGroups
    {
    public:

        explicit SettingGroups(Settings& settings);
        SettingGroups(const SettingGroups&) = delete;
        SettingGroups(SettingGroups&&) = delete;
        ~SettingGroups();

        SettingGroups& operator=(const SettingGroups&) = delete;
        SettingGroups& operator=(SettingGroups&&) = delete;

        // Selects the default preset. Throws std::runtime_error if the key is empty or already defined or a preset contains an invalid value
        void insert(SettingGroup group);

        // Throws std::runtime_error if there is no group defined for that key or the preset is invalid
        void insert_preset(std::string_view group_key, Preset preset);

        bool contains(std::string_view key) const;
        const SettingGroup& group(std::string_view key) const;

        // Unless the preset is editable, sets each member to its value or default. Throws std::runtime_error if the group or preset is not defined
        void select(std::string_view group_key, std::string_view preset_name);

        // Returns true if the key is a member of a group whose selected preset is not editable
        bool has_value_from_preset(std::string_view key) const;

        // Returns each group of the scope whose selected preset is not its default, with the preset's name
        KeyValueStore values(SettingScope scope) const;

        // Selects each preset, selecting none if any group or preset is unknown
        void set_values(const KeyValueStore& values);

        // Selects the default preset of each group of the scope
        void reset(SettingScope scope);

        // The listener is called once for each select, set_values or reset that changes any group's selected preset, after
        // its members are set, with the groups' keys
        Settings::listener_id_type add_selection_listener(std::function<void(const std::vector<std::string>& group_keys)> listener);
        void remove_listener(Settings::listener_id_type listener_id);

        // Removes every group, and keeps every listener
        // I don't like the "keeps every listener" part ...
        void clear();

    private:

        void check_preset_values(std::string_view group_key, SettingScope scope, const Preset& preset) const;

        // Selects each preset and sets the members of those that are not editable with one Settings::set_values
        void select_presets(const KeyValueStore& selections);

        // The pre-change listener
        void select_editable_presets(const KeyValueStore& values);

        void notify(const std::vector<std::string>& group_keys);

        Settings& m_settings;
        Settings::listener_id_type m_pre_change_listener_id;
        bool m_setting_members = false;
        std::map<std::string, SettingGroup, std::less<>> m_groups; // <group key, group>
        std::map<std::string, std::string, std::less<>> m_parent_groups; // <member key, group key>
        std::map<Settings::listener_id_type, std::function<void(const std::vector<std::string>& group_keys)>> m_selection_listeners; // <listener_id, listener>
        Settings::listener_id_type m_next_listener_id = 0;
    };

    // Returns the process-wide groups store, constructed on first use
    SettingGroups& group_store();

} // namespace quetzal::settings

#endif // QUETZAL_SETTINGS_SETTING_GROUPS_HPP
