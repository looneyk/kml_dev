#if !defined(QUETZAL_SETTINGS_SETTINGS_HPP)
#define QUETZAL_SETTINGS_SETTINGS_HPP
//------------------------------------------------------------------------------
// settings
// Settings.hpp
//------------------------------------------------------------------------------

#include "Setting.hpp"
#include "SettingGroup.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include <algorithm>
#include <concepts>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <vector>
#include <cassert>

namespace quetzal::settings
{

    //--------------------------------------------------------------------------
    class Settings
    {
    public:

        using listener_id_type = std::size_t;
        using preset_key_type = std::pair<std::string, std::string>; // {group key, preset name}
        using preset_values_type = std::map<preset_key_type, KeyValueStore>; // <{group key, preset name}, values>

        // Registers the types boolean, integer, real, and text
        Settings();
        Settings(const Settings&) = delete;
        Settings(Settings&&) = delete;
        ~Settings() = default;

        Settings& operator=(const Settings&) = delete;
        Settings& operator=(Settings&&) = delete;

        // Type names must be unique. values are for enumerations in code as opposed to data
        template<SettingValue T>
        void register_type(std::string_view type_name, std::vector<std::string> values = {});

        // An enumeration defined in data
        // Throws std::runtime_error if the type name is already registered or values is empty
        void register_enumeration(std::string_view type_name, std::vector<std::string> values);

        bool registered_type(std::string_view type_name) const;

        const SettingType& type(std::string_view type_name) const;

        // Requires a setting with no value. Throws std::runtime_error if the key is invalid or already used, the type is
        // unregistered, the default is invalid or out of range, or a minimum or maximum is invalid or of a type with none
        void insert(std::string_view key, Setting setting);
        bool contains(std::string_view key) const;

        const Setting& setting(std::string_view key) const;

        // Checks if the value is in the range [minimum, maximum]
        bool bounded_inclusive(std::string_view key, std::string_view value) const;

        // Inserts a group type setting and its presets with values for its members. The group is modifiable if set_values
        // may change its selection
        // Requires the members to be inserted already, with no value
        // Throws std::runtime_error if the key is empty or used, a member is not a setting of the group's scope or is
        // another group's member or a group, the default preset is missing or modifiable, or a preset is invalid
        void insert_group(std::string_view key, std::string_view scope, bool modifiable, std::string_view default_preset_name,
            std::vector<std::string> member_keys, std::vector<std::pair<std::string, Preset>> presets, std::string_view display_name = {},
            std::string_view category = {});

        bool contains_group(std::string_view key) const;
        const SettingGroup& group(std::string_view key) const;

        // Returns the key of the member's parent group setting, nullopt if the key is not a member
        std::optional<std::string> member_group_key(std::string_view key) const;

        const Preset& preset(std::string_view group_key, std::string_view preset_name) const;

        template<SettingValue T>
        T value(std::string_view key) const;

        // Returns false if a value is out of range
        template<SettingValue T>
        bool set_value(std::string_view key, const std::type_identity_t<T>& value);

        // Throws std::runtime_error if the key is not found or the value is invalid for its type
        std::string text(std::string_view key) const;
        bool set_text(std::string_view key, std::string_view value);

        // Returns each setting in the scope
        KeyValueStore values(std::string_view scope) const;

        // Sets the values of multiple settings in one operation with a single change notification
        bool set_values(const KeyValueStore& values);

        // Sets the values of presets and then settings in one operation with a single change notification
        // These settings, including presets, must be modifiable
        bool set_values(const KeyValueStore& values, const preset_values_type& preset_values);

        // Sets the values of presets and then settings in one operation
        // These settings, including presets, need not be modifiable, so this can be used for initialization
        bool initialize_values(const KeyValueStore& values, const preset_values_type& preset_values);

        // Sets the value of each setting in the scope to its default
        void reset(std::string_view scope);

        // Returns the defined values of the modifiable presets of the scope's groups
        preset_values_type preset_values(std::string_view scope) const;

        // Called once for each function that changes setting values after the values have been changed
        listener_id_type add_change_listener(std::function<void(const std::vector<std::string>& keys_changed)> listener);

        void remove_listener(listener_id_type listener_id);

        // Retains only built-in registered types and listeners
        void clear();

    private:

        template<SettingValue T>
        static SettingBaseType base_type();

        template<SettingValue T>
        static std::function<bool(std::string_view text)> accepts_function();

        void register_built_in_types();

        static preset_key_type preset_key(std::string_view group_key, std::string_view preset_name);

        std::string current_value(std::string_view key) const;

        // text should exactly represent a value of the setting's type
        bool bounded_inclusive(const Setting& setting, std::string_view text) const;

        const Preset& selected_preset(std::string_view group_key) const;
        std::optional<std::string> modifiable_preset_name(std::string_view group_key) const;

        // Throws std::runtime_error unless the values set each of the members and are values of their types
        void check_preset_members(std::string_view group_key, std::string_view preset_name, const std::vector<std::string>& member_keys,
            const KeyValueStore& values) const;

        // Throws std::runtime_error unless the preset is a modifiable preset of a group and the values are valid for it
        void check_modifiable_preset_values(const preset_key_type& key, const KeyValueStore& values) const;

        // Each throws std::runtime_error for a value that is invalid, and returns false if a value is out of range
        bool check_values(const KeyValueStore& values) const;
        bool check_presets(const preset_values_type& preset_values) const;

        // Throws std::runtime_error if a value is for a setting that is not modifiable, or for a member of a group that is not
        // modifiable or has no modifiable preset, or if preset values are for a group that is not modifiable or set a member
        // that is not. Skips keys that are not found in the store
        void check_modifiable(const KeyValueStore& values, const preset_values_type& preset_values) const;

        // Checks the values and the presets' values, assigns them, and notifies the listeners of the keys changed
        bool change_values(const KeyValueStore& values, const preset_values_type& preset_values);

        // Each assigns values that are already checked, without notifying the listeners
        void assign_presets(const preset_values_type& preset_values);
        void assign_values(const KeyValueStore& values);

        // A modifiable preset with undefined values takes the values of the preset selected before it
        void select(std::string_view group_key, std::string_view preset_name);
        void set_member(std::string_view key, std::string_view value);

        // Returns the keys whose values a change to the values and preset values can affect, in key order. These are the key of
        // each value, and the key and member keys of each group involved. A group is involved if a value is for the group or
        // one of its members, or the preset values are for one of its presets
        std::vector<std::string> extract_keys(const KeyValueStore& values, const preset_values_type& preset_values) const;

        // Returns the current value of each key as text, in the order of the keys
        std::vector<std::string> current_values(const std::vector<std::string>& keys) const;

        // Returns the keys whose current values differ from the original values, in the order of the keys
        std::vector<std::string> changed_keys(const std::vector<std::string>& keys, const std::vector<std::string>& original_values) const;

        // Calls each change listener
        void notify(const std::vector<std::string>& keys_changed);

        std::map<std::string, SettingType, std::less<>> m_registered_types; // <type name, type>
        std::map<std::string, Setting, std::less<>> m_settings; // <key, setting>
        std::map<std::string, SettingGroup, std::less<>> m_groups; // <group key, group>
        std::map<std::string, std::string, std::less<>> m_member_groups; // <member key, group key>
        std::map<preset_key_type, Preset> m_presets; // <{group key, preset name}, preset>
        std::map<listener_id_type, std::function<void(const std::vector<std::string>& keys_changed)>> m_change_listeners; // <listener_id, listener>
        listener_id_type m_next_listener_id = 0;
    };

    // Returns the process-wide store, constructed on first use
    Settings& store();

} // namespace quetzal::settings

//------------------------------------------------------------------------------
template<quetzal::settings::SettingValue T>
void quetzal::settings::Settings::register_type(std::string_view type_name, std::vector<std::string> values)
{
    assert(!registered_type(type_name) && std::is_enum_v<T> == !values.empty());
    assert(std::ranges::all_of(values, [](const std::string& value) { return quetzal::try_type<T>(value).has_value(); }));

    m_registered_types.emplace(std::string(type_name), SettingType{std::type_index(typeid(T)), base_type<T>(), std::move(values),
        accepts_function<T>()});
    return;
}

//------------------------------------------------------------------------------
template<quetzal::settings::SettingValue T>
T quetzal::settings::Settings::value(std::string_view key) const
{
    assert(type(setting(key).type_name).cpp_type == std::type_index(typeid(T)));

    std::optional<T> value = quetzal::try_type<T>(current_value(key));
    assert(value.has_value());
    return *value;
}

//------------------------------------------------------------------------------
template<quetzal::settings::SettingValue T>
bool quetzal::settings::Settings::set_value(std::string_view key, const std::type_identity_t<T>& value)
{
    assert(type(setting(key).type_name).cpp_type == std::type_index(typeid(T)));

    return set_values({{std::string(key), quetzal::to_string(value)}});
}

//------------------------------------------------------------------------------
template<quetzal::settings::SettingValue T>
quetzal::settings::SettingBaseType quetzal::settings::Settings::base_type()
{
    if constexpr (std::same_as<T, bool>)
    {
        return SettingBaseType::Boolean;
    }
    else if constexpr (std::integral<T>)
    {
        return SettingBaseType::Integer;
    }
    else if constexpr (std::floating_point<T>)
    {
        return SettingBaseType::Real;
    }
    else
    {
        return SettingBaseType::Text;
    }
}

//------------------------------------------------------------------------------
template<quetzal::settings::SettingValue T>
std::function<bool(std::string_view text)> quetzal::settings::Settings::accepts_function()
{
    return [](std::string_view text) { return quetzal::try_type<T>(text).has_value(); };
}

#endif // QUETZAL_SETTINGS_SETTINGS_HPP
