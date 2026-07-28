#if !defined(QUETZAL_STRING_UTIL_HPP)
#define QUETZAL_STRING_UTIL_HPP
//------------------------------------------------------------------------------
// string_util.hpp
// locale independent
//------------------------------------------------------------------------------

#include <charconv>
#include <format>
#include <optional>
#include <ostream>
#include <string>

namespace quetzal
{

    //--------------------------------------------------------------------------
    // Stream manipulators for the object and element delimiters used by array_format_spec, giving
    // operator<< the same array-string vocabulary as a std::format spec: os << object_delim('[')
    // << element_delim(", ") << v matches std::format("{:[,_}", v). object_delim('\0') resets the
    // stream back to no object delimiter (the default). element_delim("") is a no-op, leaving
    // whatever element delimiter is already in effect unchanged.
    struct object_delim_manip
    {
        char value;
    };

    object_delim_manip object_delim(char c);
    std::ostream& operator<<(std::ostream& os, object_delim_manip manip);

    struct element_delim_manip
    {
        std::string value;
    };

    element_delim_manip element_delim(std::string s);
    std::ostream& operator<<(std::ostream& os, const element_delim_manip& manip);

    //--------------------------------------------------------------------------
    // Using this with array types (Vector, Point, and higher level objects) produces a string of space separated values
    template<typename T>
    std::string to_string(const T& t)
    {
        return std::format("{}", t);
    }

    //--------------------------------------------------------------------------
    // Only valid for array types (Vector, Point, and higher level objects) with a corresponding formatter
    // Produces a string of bracketed, comma-separated valuse
    template<typename T>
    std::string to_array_string(const T& t)
    {
        return std::format("{:[,_}", t);
    }

    //--------------------------------------------------------------------------
    // Reflects an ostream's currently active object_delim/element_delim manipulators (falling back to
    // default_object_delim/default_element_delim when unset, i.e. '\0' == no bracket), precision, and
    // fixed/scientific formatting flags into an equivalent std::format spec fragment (matching
    // std::format's grammar). The defaults match std::format's own empty-spec default (no bracket,
    // single-space element delimiter), so this is what puts operator<< and format({}) in agreement.
    std::string array_format_spec(const std::ostream& os, char default_object_delim = '\0', const std::string& default_element_delim = " ");

    //--------------------------------------------------------------------------
    // Stream-state-aware form of to_string: matches to_string(t)'s unbracketed, space-separated output
    // by default (same as format({}), see array_format_spec), but honors object_delim/element_delim
    // manipulators (so operator<< can be made to produce array-formatted output on request, the ostream
    // equivalent of passing a format spec) as well as std::setprecision/std::fixed/std::scientific, the
    // way plain ostream insertion always has.
    template<typename T>
    std::string to_string(const T& t, const std::ostream& os)
    {
        return std::vformat("{:" + array_format_spec(os) + "}", std::make_format_args(t));
    }

    //--------------------------------------------------------------------------
    template<typename T>
    std::string to_string(const std::optional<T>& opt)
    {
        return opt ? to_string(*opt) : "nullopt";
    }

    template<typename T>
    std::ostream& operator<<(std::ostream& os, const std::optional<T>& opt)
    {
        os << to_string(opt, os);
        return os;
    }

    std::string to_string(const std::string& str);
    std::string to_string(const std::wstring& wstr);
    std::string to_string(const bool& b);

    //--------------------------------------------------------------------------
    template<typename T>
    T to_type(const std::string& str)
    {
        T t{};
        using std::from_chars;
        auto result = from_chars(str.data(), str.data() + str.size(), t);
        return t;
    }

    //--------------------------------------------------------------------------
    template<typename T>
    std::optional<T> try_type(const std::string& str)
    {
        T t{};
        using std::from_chars;
        auto result = from_chars(str.data(), str.data() + str.size(), t);
//        return result ? make_optional(t) : std::nullopt; ...
        return result.ec == std::errc{} ? make_optional(t) : std::nullopt;
    }

    template<>
    std::string to_type<std::string>(const std::string& str);

    template<>
    std::wstring to_type<std::wstring>(const std::string& str);

    template<>
    bool to_type<bool>(const std::string& str);

    bool to_bool(const std::string& str);

// to_utf8 ...
//    std::string to_utf8(char16_t c);
//    std::string to_utf8(char32_t c);
//
//    std::u8string to_utf8(string_view<wchar_t> s);
//    std::u8string to_utf8(string_view<char16_t> s);
//    std::u8string to_utf8(string_view<char32_t> s);

} // namespace quetzal

//------------------------------------------------------------------------------
template<typename T>
struct std::formatter<std::optional<T>> : std::formatter<T>
{
    template<typename FormatContext>
    auto format(const std::optional<T>& opt, FormatContext& ctx) const
    {
        if (!opt)
        {
            return std::format_to(ctx.out(), "nullopt");
        }

        return std::formatter<T>::format(*opt, ctx);
    }
};

#endif // QUETZAL_STRING_UTIL_HPP
