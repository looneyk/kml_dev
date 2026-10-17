#if !defined(QUETZAL_SETTINGS_SETTINGS_HPP)
#define QUETZAL_SETTINGS_SETTINGS_HPP
//------------------------------------------------------------------------------
// settings
// Settings.hpp
//------------------------------------------------------------------------------

#include "Setting.hpp"
#include "quetzal/common/KeyValueStore.hpp"
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

        // Registers the types boolean, integer, real, and text
        Settings();
        Settings(const Settings&) = delete;
        Settings(Settings&&) = delete;
        ~Settings() = default;

        Settings& operator=(const Settings&) = delete;
        Settings& operator=(Settings&&) = delete;

        // Type names must be unique
        template<SettingValue T>
        void register_type(std::string_view type_name);
        bool registered_type(std::string_view type_name) const;

        const SettingType& type(std::string_view type_name) const;

        // Throws std::runtime_error if the key is invalid, already used, unregistered, or the default is invalid
        void insert(std::string_view key, std::string_view type_name, std::string_view default_value, SettingScope scope, bool restart_required);
        bool contains(std::string_view key) const;

        const Setting& setting(std::string_view key) const;

        template<SettingValue T>
        T value(std::string_view key) const;
        template<SettingValue T>
        void set_value(std::string_view key, const std::type_identity_t<T>& value);

        // Throws std::runtime_error if the key is not found or the value is invalid for its type
        std::string text(std::string_view key) const;
        void set_text(std::string_view key, std::string_view value);

        // Returns each setting in the scope
        KeyValueStore values(SettingScope scope) const;

        // Change nothing on any set_text failure
        void set_values(const KeyValueStore& values);

        // Sets each setting in the scope to its default
        void reset(SettingScope scope);

        // The listener is called once for each set_value, set_text, set_values or reset that changes any setting, after
        // every setting it changes has changed, with their keys
        void add_change_listener(std::function<void(const std::vector<std::string>& keys_changed)> listener);

        // Retains only built-in registered types
        void clear();

    private:

        template<SettingValue T>
        static std::function<bool(std::string_view text)> accepts_function();

        void register_built_in_types();

        std::string current_text(std::string_view key) const;
        bool assign(std::string_view key, std::string_view text); // Returns true if the value changed

        // Calls each change listener, unless no key changed
        void notify(const std::vector<std::string>& keys_changed);

        std::map<std::string, SettingType, std::less<>> m_registered_types; // <type name, type>
        std::map<std::string, Setting, std::less<>> m_settings; // <key, setting>
        std::vector<std::function<void(const std::vector<std::string>& keys_changed)>> m_change_listeners;
    };

    // Returns the process-wide store, constructed on first use
    Settings& store();

} // namespace quetzal::settings

//------------------------------------------------------------------------------
template<quetzal::settings::SettingValue T>
void quetzal::settings::Settings::register_type(std::string_view type_name)
{
    assert(!registered_type(type_name));
    m_registered_types.emplace(std::string(type_name), SettingType {std::type_index(typeid(T)), accepts_function<T>()});
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
void quetzal::settings::Settings::set_value(std::string_view key, const std::type_identity_t<T>& value)
{
    assert(type(setting(key).type_name).cpp_type == std::type_index(typeid(T)));

    if (assign(key, quetzal::to_string(value)))
    {
        notify({std::string(key)});
    }

    return;
}

//------------------------------------------------------------------------------
template<quetzal::settings::SettingValue T>
std::function<bool(std::string_view text)> quetzal::settings::Settings::accepts_function()
{
    return [](std::string_view text) { return quetzal::try_type<T>(text).has_value(); };
}

#endif // QUETZAL_SETTINGS_SETTINGS_HPP
