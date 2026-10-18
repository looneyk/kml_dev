#if !defined(QUETZAL_SETTINGS_SETTINGS_HPP)
#define QUETZAL_SETTINGS_SETTINGS_HPP
//------------------------------------------------------------------------------
// settings
// Settings.hpp
//------------------------------------------------------------------------------

#include "Setting.hpp"
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
#include <vector>
#include <cassert>

namespace quetzal::settings
{

    //--------------------------------------------------------------------------
    class Settings
    {
    public:

        using listener_id_type = std::size_t;

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

        template<SettingValue T>
        T value(std::string_view key) const;

        // Returns false if a value is out of range
        template<SettingValue T>
        bool set_value(std::string_view key, const std::type_identity_t<T>& value);

        // Throws std::runtime_error if the key is not found or the value is invalid for its type
        std::string text(std::string_view key) const;
        bool set_text(std::string_view key, std::string_view value);

        // Returns each setting in the scope
        KeyValueStore values(SettingScope scope) const;

        // Change nothing on any set_text failure. Returns false, setting nothing, if a value is out of range
        bool set_values(const KeyValueStore& values);

        // Sets each setting in the scope to its default
        void reset(SettingScope scope);

        // The listener is called by set_values, set_value and set_text with the values after they are checked and before any
        // is set, and may throw to reject them. It must not set a setting
        listener_id_type add_pre_change_listener(std::function<void(const KeyValueStore& values)> listener);

        // The listener is called once for each set_value, set_text, set_values or reset that changes any setting, after
        // every setting it changes has changed, with their keys
        listener_id_type add_change_listener(std::function<void(const std::vector<std::string>& keys_changed)> listener);

        void remove_listener(listener_id_type listener_id);

        // Retains only built-in registered types, and every listener
        void clear();

    private:

        template<SettingValue T>
        static SettingBaseType base_type();

        template<SettingValue T>
        static std::function<bool(std::string_view text)> accepts_function();

        void register_built_in_types();

        std::string current_text(std::string_view key) const;
        bool assign(std::string_view key, std::string_view text); // Returns true if the value changed

        // Requires a value of the setting's type
        bool bounded_inclusive(const Setting& setting, std::string_view text) const;

        // Calls each change listener, unless no key changed
        void notify(const std::vector<std::string>& keys_changed);

        std::map<std::string, SettingType, std::less<>> m_registered_types; // <type name, type>
        std::map<std::string, Setting, std::less<>> m_settings; // <key, setting>
        std::map<listener_id_type, std::function<void(const KeyValueStore& values)>> m_pre_change_listeners; // <listener_id, listener>
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

    m_registered_types.emplace(std::string(type_name), SettingType {std::type_index(typeid(T)), base_type<T>(), std::move(values),
        accepts_function<T>()});
    return;
}

//------------------------------------------------------------------------------
template<quetzal::settings::SettingValue T>
T quetzal::settings::Settings::value(std::string_view key) const
{
    assert(type(setting(key).type_name).cpp_type == std::type_index(typeid(T)));

    std::optional<T> value = quetzal::try_type<T>(current_text(key));
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
