#if !defined(QUETZAL_SETTINGS_SETTING_HPP)
#define QUETZAL_SETTINGS_SETTING_HPP
//------------------------------------------------------------------------------
// settings
// Setting.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/string_util.hpp"
#include <charconv>
#include <concepts>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <typeindex>
#include <vector>

namespace quetzal::settings
{

    //--------------------------------------------------------------------------
    // Where a setting's value is saved: the user settings file, or the game file of the game in progress
    enum class SettingScope
    {
        User,
        Game
    };

    //--------------------------------------------------------------------------
    // The built-in type a setting type is based on, which a script reads its values as
    enum class SettingBaseType
    {
        Boolean,
        Integer,
        Real,
        Text
    };

    // A type a setting may have: one that std::format writes, and that quetzal::try_type reads back through from_chars.
    // An enumeration's setting needs a std::formatter specialization and a from_chars overload in its namespace
    template<typename T>
    concept SettingValue = std::formattable<T, char>
        && (std::same_as<T, std::string> || requires(const char* text, T& value)
        {
            { from_chars(text, text, value) } -> std::same_as<std::from_chars_result>;
        });

    //--------------------------------------------------------------------------
    // A type registered by setting type name
    struct SettingType
    {
        std::type_index cpp_type;
        SettingBaseType base_type;
        std::vector<std::string> values; // An enumeration's values
        std::function<bool(std::string_view text)> accepts; // Returns true if the text is a value of the type
    };

    //--------------------------------------------------------------------------
    struct Setting
    {
        std::string type_name;
        std::string default_value;
        SettingScope scope = SettingScope::User;
        bool restart_required = false;
        std::optional<std::string> value; // If empty, has its default value
    };

} // namespace quetzal::settings

#endif // QUETZAL_SETTINGS_SETTING_HPP
