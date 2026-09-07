#if !defined(QUETZAL_STRING_UTIL_HPP)
#define QUETZAL_STRING_UTIL_HPP
//------------------------------------------------------------------------------
// string_util.hpp
// locale independent
//------------------------------------------------------------------------------

#include <charconv>
#include <format>
#include <istream>
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
    // Reading counterparts to the delimiters above, for the element sequences an array type
    // writes. skip_whitespace returns the first position that is neither a space nor a tab;
    // the two stream forms consume the delimiter they name and set failbit where it is absent,
    // an empty element delimiter pattern skipping nothing and succeeding
    const char* skip_whitespace(const char* first, const char* last);

    std::istream& skip_object_delimiter(std::istream& is, char delimiter);
    std::istream& skip_element_delimiter(std::istream& is, const std::string& delimiter_pattern);

    //--------------------------------------------------------------------------
    // The delimited component sequence of an array type, as its operator<< writes it. T supplies
    // dimension, a value_type the standard conversions accept, and indexing, which the vector and
    // point types all do, discrete and not. Homogeneous coordinates are dimension components and
    // so carry no weight here: supplying one on read and reducing before write belong to the
    // caller, which is where they already are
    // A component that cannot be read leaves the ones before it assigned and reports where it
    // stopped, which is what std::from_chars does with a single value
    template<typename T>
    std::from_chars_result array_from_chars(const char* first, const char* last, T& t)
    {
        std::from_chars_result result {first, std::errc {}};

        for (size_t i = 0; i < T::dimension; ++i)
        {
            result = std::from_chars(skip_whitespace(result.ptr, last), last, t[i]);
            if (result.ec != std::errc {})
            {
                return result;
            }
        }

        return result;
    }

    //--------------------------------------------------------------------------
    template<typename T>
    std::to_chars_result array_to_chars(char* first, char* last, const T& t, const std::string& element_delimiter = " ")
    {
        using element_type = typename T::value_type;

        std::to_chars_result result {first, std::errc {}};

        for (size_t i = 0; i < T::dimension; ++i)
        {
            if (i > 0)
            {
                if (static_cast<size_t>(last - result.ptr) < element_delimiter.size())
                {
                    return {result.ptr, std::errc::value_too_large};
                }

                for (char c : element_delimiter)
                {
                    *result.ptr++ = c;
                }
            }

            // The conditional is what keeps a negative zero from being written as one
            element_type element = t[i];
            result = std::to_chars(result.ptr, last, element == element_type(0) ? element_type(0) : element);
            if (result.ec != std::errc {})
            {
                return result;
            }
        }

        return result;
    }

    //--------------------------------------------------------------------------
    // Using this with array types (Vector, Point, and higher level objects) produces a string of space separated values
    template<typename T>
    std::string to_string(const T& t)
    {
        return std::format("{}", t);
    }

    //--------------------------------------------------------------------------
    // Only valid for array types (Vector, Point, and higher level objects) with a corresponding formatter
    // Produces a string of bracketed, comma-separated values
    template<typename T>
    std::string to_array_string(const T& t)
    {
        return std::format("{:[,_}", t);
    }

    //--------------------------------------------------------------------------
    // Reflects an ostream's currently active object_delim/element_delim manipulators, precision,
    // and fixed/scientific formatting flags into an equivalent std::format spec fragment.
    // The defaults match std::format's own empty-spec default (no bracket, single-space element delimiter),
    // so this is what puts operator<< and format({}) in agreement.
    std::string array_format_spec(const std::ostream& os, char default_object_delim = '\0', const std::string& default_element_delim = " ");

    //--------------------------------------------------------------------------
    // Stream-state-aware form of to_string: matches to_string(t)'s unbracketed, space-separated output
    // by default (same as format({}), see array_format_spec), but honors object_delim/element_delim manipulators
    // in addition to standard floating point manipulators, so stream output has the same functionality as std::format.
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
        return result.ec == std::errc{} ? std::make_optional(t) : std::nullopt; // result ? ...
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
