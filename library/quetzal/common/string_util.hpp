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

    // Reflects an ostream's currently active precision and fixed/scientific formatting flags into
    // an equivalent std::format spec fragment (precision before the presentation type, matching
    // std::format's grammar), for use by operator<< implementations that want to stay aware of
    // manipulators like std::setprecision/std::fixed/std::scientific.
    inline std::string array_format_spec(const std::ostream& os)
    {
        std::string spec = "[,_.";
        spec += std::to_string(os.precision());

        if (os.flags() & std::ios::fixed)
        {
            spec += 'f';
        }
        else if (os.flags() & std::ios::scientific)
        {
            spec += 'e';
        }

        return spec;
    }

    // Stream-state-aware form of to_array_string: mirrors os's current precision and fixed/scientific
    // flags into the format spec, so operator<< implementations built on this respect manipulators
    // like std::setprecision/std::fixed/std::scientific the way plain ostream insertion always has.
    template<typename T>
    std::string to_array_string(const T& t, const std::ostream& os)
    {
        return std::vformat("{:" + array_format_spec(os) + "}", std::make_format_args(t));
    }

    //--------------------------------------------------------------------------
    template<typename T>
    std::string opt_string(const std::optional<T>& opt)
    {
        return opt ? to_string(*opt) : "nullopt";
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
    std::optional<T> opt_type(const std::string& str)
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

#endif // QUETZAL_STRING_UTIL_HPP
