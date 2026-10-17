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
#include <string>
#include <string_view>

namespace quetzal::settings
{

    //--------------------------------------------------------------------------
    // Where a setting's value is saved: the user settings file, or the game file of the game in progress
    enum class SettingScope
    {
        User,
        Game
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
    // A setting's one declaration in C++, which holds no value
    template<SettingValue T>
    struct Setting
    {
        std::string_view key; // "gameplay.random_seeding_on_open"
        T default_value {};
        SettingScope scope = SettingScope::User;
        bool restart_required = false;
    };

} // namespace quetzal::settings

#endif // QUETZAL_SETTINGS_SETTING_HPP
