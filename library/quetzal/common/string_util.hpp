#if !defined(QUETZAL_STRING_UTIL_HPP)
#define QUETZAL_STRING_UTIL_HPP
//------------------------------------------------------------------------------
// string_util.hpp
// locale independent
//------------------------------------------------------------------------------

#include <charconv>
#include <format>
#include <optional>
#include <string>

namespace quetzal
{

    //--------------------------------------------------------------------------
    template<typename T>
    std::string to_string(const T& t)
    {
        return std::format("{}", t);
    }

    std::string to_string(const std::string& str);
    std::string to_string(const std::wstring& wstr);
    std::string to_string(const bool& b);

    //--------------------------------------------------------------------------
    template<typename T>
    T to_type(const std::string& str)
    {
        T t{};
        auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), t);
        return t;
    }

    template<>
    std::string to_type<std::string>(const std::string& str);

    template<>
    std::wstring to_type<std::wstring>(const std::string& str);

    template<>
    bool to_type<bool>(const std::string& str);

    bool to_bool(const std::string& str);

    //--------------------------------------------------------------------------
    template<typename T>
    std::string opt_string(const std::optional<T>& opt)
    {
        return opt ? to_string(*opt) : "nullopt";
    }

    //--------------------------------------------------------------------------
    template<typename T>
    std::optional<T> opt_type(const std::string& str)
    {
        T t{};
        auto result = std::from_chars(str.data(), str.data() + str.size(), t);
        return result ? make_optional(t) : std::nullopt;
    }

// to_utf8 ...
//    std::string to_utf8(char16_t c);
//    std::string to_utf8(char32_t c);
//
//    std::u8string to_utf8(string_view<wchar_t> s);
//    std::u8string to_utf8(string_view<char16_t> s);
//    std::u8string to_utf8(string_view<char32_t> s);

} // namespace quetzal

#endif // QUETZAL_STRING_UTIL_HPP
