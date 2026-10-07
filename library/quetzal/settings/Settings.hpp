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
#include <typeindex>
#include <typeinfo>
#include <vector>
#include <cassert>

namespace quetzal::settings
{

    //--------------------------------------------------------------------------
    // What the store keeps for every declared setting, whether declared in C++ or in data
    struct SettingDeclaration
    {
        std::type_index type; // The C++ type of the setting's value
        std::string default_text;
        SettingScope scope = SettingScope::User;
        bool restart_required = false;
        std::function<bool(std::string_view text)> accepts; // Returns true if the text is a value of the setting's type
    };

    //--------------------------------------------------------------------------
    // Every setting's declaration and value, by key. A setting that is not set has its default. A subsystem applies its
    // settings from the store into structures of its own, and applies them again when notified of a change. Every value
    // is text, in the store and in a file
    class Settings
    {
    public:

        // Registers the type names a declaration in data may give: bool, integer, real and text
        Settings();
        Settings(const Settings&) = delete;
        Settings(Settings&&) = delete;
        ~Settings() = default;

        Settings& operator=(const Settings&) = delete;
        Settings& operator=(Settings&&) = delete;

        // The name a declaration in data gives for the type. Requires a name that is not registered yet
        template<SettingValue T>
        void register_type(std::string_view type_name);
        bool registers_type(std::string_view type_name) const;

        // Requires a setting key that is not declared yet
        template<SettingValue T>
        void declare(const Setting<T>& setting);

        // A declaration in data. Throws std::runtime_error if the key is not a setting key or is declared, the type name
        // is not registered, or the default is no value of the type
        void declare(std::string_view key, std::string_view type_name, std::string_view default_text, SettingScope scope,
            bool restart_required);

        bool declares(std::string_view key) const;

        // Requires a declared key
        const SettingDeclaration& declaration(std::string_view key) const;

        // Each requires a declared setting of this type
        template<SettingValue T>
        T value(const Setting<T>& setting) const;
        template<SettingValue T>
        void set_value(const Setting<T>& setting, const T& value);

        // By key, as text. Throws std::runtime_error if the key is not declared, or the text is no value of its type
        std::string text(std::string_view key) const;
        void set_text(std::string_view key, std::string_view text);

        // Returns each setting of the scope that is set, by key, as text
        KeyValueStore values(SettingScope scope) const;

        // Sets each value, as set_text does, changing none if any key is not declared or any text is no value of its
        // type, and notifies each section changed once
        void set_values(const KeyValueStore& values);

        // Returns each setting of the scope to its default
        void reset(SettingScope scope);

        // apply is called after a setting in the section changes, a setting in a section nested in it included, once
        // for each set_value, set_text, set_values or reset that changes any
        void add_section_listener(std::string_view section, std::function<void()> apply);

        // Removes every declaration, value, listener and registered type but the four the constructor registers
        void clear();

    private:

        struct RegisteredType
        {
            std::type_index type;
            std::function<bool(std::string_view text)> accepts;
        };

        template<SettingValue T>
        static std::function<bool(std::string_view text)> accepts_function();

        void register_built_in_types();

        // Each requires a declared key. assign returns true if the value changed
        std::string value_text(std::string_view key) const;
        bool assign(std::string_view key, std::string_view text);

        // Calls each listener of a section that one of the keys is in, once
        void notify(const std::vector<std::string>& keys_changed);

        std::map<std::string, RegisteredType, std::less<>> m_registered_types; // <type name, type>
        std::map<std::string, SettingDeclaration, std::less<>> m_declarations; // <key, declaration>
        std::map<std::string, std::string, std::less<>> m_values; // <key, text>
        std::multimap<std::string, std::function<void()>, std::less<>> m_section_listeners; // <section, apply>
    };

    // Returns the process-wide store, constructed on first use
    Settings& store();

} // namespace quetzal::settings

//------------------------------------------------------------------------------
template<quetzal::settings::SettingValue T>
void quetzal::settings::Settings::register_type(std::string_view type_name)
{
    assert(!registers_type(type_name));
    m_registered_types.emplace(std::string(type_name), RegisteredType {std::type_index(typeid(T)), accepts_function<T>()});
    return;
}

//------------------------------------------------------------------------------
template<quetzal::settings::SettingValue T>
void quetzal::settings::Settings::declare(const Setting<T>& setting)
{
    assert(!declares(setting.key) && is_setting_key(setting.key));

    m_declarations.emplace(std::string(setting.key), SettingDeclaration {std::type_index(typeid(T)), quetzal::to_string(setting.default_value),
        setting.scope, setting.restart_required, accepts_function<T>()});
    return;
}

//------------------------------------------------------------------------------
template<quetzal::settings::SettingValue T>
T quetzal::settings::Settings::value(const Setting<T>& setting) const
{
    assert(declaration(setting.key).type == std::type_index(typeid(T)));

    std::optional<T> value = quetzal::try_type<T>(value_text(setting.key));
    assert(value.has_value());
    return *value;
}

//------------------------------------------------------------------------------
template<quetzal::settings::SettingValue T>
void quetzal::settings::Settings::set_value(const Setting<T>& setting, const T& value)
{
    assert(declaration(setting.key).type == std::type_index(typeid(T)));

    if (assign(setting.key, quetzal::to_string(value)))
    {
        notify({std::string(setting.key)});
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
