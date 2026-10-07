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
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace quetzal
{

    // array type here refer to objects such as vector and point types that have a sequence of values
    // along with their higher level constructs and which supply dimension, value_type, and element addressing

    //--------------------------------------------------------------------------
    // Stream manipulators for the object and element delimiters of array_format_spec, so that os << object_delim('[')
    // << element_delim(", ") << v matches std::format("{:[,_}", v). object_delim('\0') restores the default of no object
    // delimiter, and element_delim("") leaves the element delimiter unchanged
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

    // The same state on the reading side, so a stream reads back what its counterpart wrote
    std::istream& operator>>(std::istream& is, object_delim_manip manip);
    std::istream& operator>>(std::istream& is, const element_delim_manip& manip);

    // What the manipulators put on the stream, or the default where none was set
    std::string array_element_delimiter(std::ios_base& stream, std::string_view default_delimiter = " ");

    // Returns the first position that is neither a space nor a tab
    const char* skip_whitespace(const char* first, const char* last);

    std::istream& skip_object_delimiter(std::istream& is, char delimiter);
    std::istream& skip_element_delimiter(std::istream& is, std::string_view delimiter_pattern);

    // Matched exactly, except that whitespace in the pattern skips a run of it.
    const char* skip_element_delimiter(const char* first, const char* last, std::string_view delimiter_pattern);

    //--------------------------------------------------------------------------
    template<typename T>
    std::from_chars_result array_from_chars(const char* first, const char* last, T& t, std::string_view element_delimiter = " ")
    {
        std::from_chars_result result {first, std::errc {}};

        for (size_t i = 0; i < T::dimension; ++i)
        {
            const char* position = i == 0
                ? skip_whitespace(result.ptr, last)
                : skip_element_delimiter(result.ptr, last, element_delimiter);

            if (position == nullptr)
            {
                return {result.ptr, std::errc::invalid_argument};
            }

            result = std::from_chars(position, last, t[i]);
            if (result.ec != std::errc {})
            {
                return result;
            }
        }

        return result;
    }

    //--------------------------------------------------------------------------
    template<typename T>
    std::to_chars_result array_to_chars(char* first, char* last, const T& t, std::string_view element_delimiter = " ")
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

            // The conditional translates negative zero
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
    // The std::formatter base of an array type: the parsed spec and the loop that writes the elements
    //
    // Format-spec characters: an optional leading '[' or '(' selects a bracket pair to wrap the
    // elements ('[' -> "[ ]", '(' -> "( )"), followed by an optional run of characters typically drawn from
    // ",;: _" that becomes the literal element separator verbatim, with '_' representing a space
    // (e.g. "[,_" -> bracket '[' + separator ", "; "_:_" -> separator " : "). Whatever remains of
    // the spec after them is handed to the element type's own formatter
    template<typename T>
    struct array_formatter
    {
        std::formatter<T> m_element_formatter;
        std::optional<std::pair<char, char>> m_object_delimiters;
        std::string m_element_delimiter = " ";

        constexpr auto parse(std::format_parse_context& ctx)
        {
            auto it = ctx.begin();

            if (it != ctx.end() && (*it == '[' || *it == '('))
            {
                m_object_delimiters = (*it == '[') ? std::make_pair('[', ']') : std::make_pair('(', ')');
                ++it;
            }

            std::string delimiter;
            bool has_delimiter = false;
            while (it != ctx.end() && (*it == ',' || *it == ';' || *it == ':' || *it == ' ' || *it == '_'))
            {
                delimiter += (*it == '_') ? ' ' : *it;
                has_delimiter = true;
                ++it;
            }

            if (has_delimiter)
            {
                m_element_delimiter = std::move(delimiter);
            }

            ctx.advance_to(it);
            return m_element_formatter.parse(ctx);
        }

        template<typename A, typename FormatContext>
        auto format_elements(const A& a, FormatContext& ctx) const
        {
            auto out = ctx.out();

            if (m_object_delimiters)
            {
                *out++ = m_object_delimiters->first;
                ctx.advance_to(out);
            }

            for (size_t i = 0; i < A::dimension; ++i)
            {
                if (i > 0)
                {
                    for (char c : m_element_delimiter)
                    {
                        *out++ = c;
                    }

                    ctx.advance_to(out);
                }

                // The conditional translates negative zero
                T element = a[i];
                out = m_element_formatter.format(element == T(0) ? T(0) : element, ctx);
                ctx.advance_to(out);
            }

            if (m_object_delimiters)
            {
                *out++ = m_object_delimiters->second;
            }

            return out;
        }
    };

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
    // The std::format spec fragment equivalent to an ostream's delimiter manipulators, precision and fixed/scientific
    // flags. The defaults match the empty spec, so operator<< and format({}) produce the same output
    std::string array_format_spec(const std::ostream& os, char default_object_delim = '\0',
        std::string_view default_element_delim = " ", bool precision = true);

    //--------------------------------------------------------------------------
    // to_string following the stream's delimiter and floating point manipulators
    template<typename T>
    std::string to_string(const T& t, const std::ostream& os)
    {
        constexpr bool precision = !std::is_integral_v<typename T::value_type>;
        return std::vformat("{:" + array_format_spec(os, '\0', " ", precision) + "}", std::make_format_args(t));
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
    // At namespace scope, so that an unqualified call finds the standard overloads and quetzal's together
    using std::from_chars;

    // Reads true, false, 1 or 0, and stops after it, as the standard overloads stop after a number
    std::from_chars_result from_chars(const char* first, const char* last, bool& value);

    //--------------------------------------------------------------------------
    template<typename T>
    std::optional<T> try_type(std::string_view str)
    {
        T t{};
        const char* end = str.data() + str.size();
        auto result = from_chars(str.data(), end, t);
        return result.ec == std::errc{} && result.ptr == end ? std::make_optional(t) : std::nullopt; // result && ...
    }

    template<>
    std::optional<std::string> try_type<std::string>(std::string_view str);

    // Returns std::nullopt if the string is not valid UTF-8
    template<>
    std::optional<std::wstring> try_type<std::wstring>(std::string_view str);

    //--------------------------------------------------------------------------
    // Throws std::runtime_error if the whole string is not a value of the type
    template<typename T>
    T to_type(std::string_view str)
    {
        std::optional<T> t = try_type<T>(str);
        if (!t)
        {
            throw std::runtime_error(std::format("Invalid value for the type: \"{}\"", str));
        }

        return *t;
    }

    bool to_bool(std::string_view str);

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
