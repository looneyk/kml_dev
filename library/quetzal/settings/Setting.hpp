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
    // The built-in type a setting type is based on
    enum class SettingBaseType
    {
        Boolean,
        Integer,
        Real,
        Text
    };

    // The type of a setting's value
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
        std::function<bool(std::string_view text)> accepts; // Returns true if text is an exact representation of the type
        bool group = false; // The enumeration values are preset names
    };

    //--------------------------------------------------------------------------
    struct Setting
    {
        std::string type_name;
        std::string default_value; // Value used if value is not given
        std::string scope; // Client domain
        bool restart_required = false;
        bool modifiable = false; // True if set_values may change it; initialize_values and reset change it either way
        std::string display_name;
        std::string description;
        std::string category; // Client domain
        std::optional<std::string> minimum; // Bounds for numeric setting types
        std::optional<std::string> maximum;
        std::optional<std::string> value;
    };

} // namespace quetzal::settings

#endif // QUETZAL_SETTINGS_SETTING_HPP
