#if !defined(QUETZAL_STRING_UTIL_HPP)
#define QUETZAL_STRING_UTIL_HPP
//------------------------------------------------------------------------------
// string_util.hpp
// locale independent
//------------------------------------------------------------------------------

#include <array>
#include <charconv>
#include <concepts>
#include <format>
#include <istream>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace quetzal
{

    // array type here refer to objects such as vector and point types that have a sequence of values
    // along with their higher level constructs and which supply dimension, value_type, and element addressing

    // Each manipulator sets the same state on an output and an input stream, so stream reads and writes are symmetrical

    //--------------------------------------------------------------------------
    // The coordinate form of a homogeneous type's text: its components divided by the weight, or every stored value
    inline constexpr bool homogeneous_default = false;

    std::ios_base& cartesian(std::ios_base& stream);
    std::ios_base& homogeneous(std::ios_base& stream);

    //--------------------------------------------------------------------------
    inline constexpr std::string_view separator_default = " ";

    struct separator_delim_manip
    {
        std::string value;
    };

    separator_delim_manip separator_delim(std::string separator);
    std::ostream& operator<<(std::ostream& os, const separator_delim_manip& manip);
    std::istream& operator>>(std::istream& is, const separator_delim_manip& manip);

    //--------------------------------------------------------------------------
    // Opening and closing brackets
    using brackets_type = std::array<std::string_view, 2>;

    inline constexpr brackets_type brackets_default = {"", ""};

    struct brackets_delim_manip
    {
        std::string opening_bracket;
        std::string closing_bracket;
    };

    brackets_delim_manip brackets_delim(std::string opening_bracket, std::string closing_bracket);
    std::ostream& operator<<(std::ostream& os, const brackets_delim_manip& manip);
    std::istream& operator>>(std::istream& is, const brackets_delim_manip& manip);

    //--------------------------------------------------------------------------
    inline constexpr std::string_view series_delimiter_default = ", ";
    inline constexpr std::string_view bounds_delimiter_default = " .. ";

    struct series_delim_manip
    {
        std::string value;
    };

    series_delim_manip series_delim(std::string series_delimiter);
    std::ostream& operator<<(std::ostream& os, const series_delim_manip& manip);
    std::istream& operator>>(std::istream& is, const series_delim_manip& manip);

    //--------------------------------------------------------------------------
    // The settings of a value's text, parsed from a std::format spec, set on a stream, or passed to to_chars and from_chars.
    // A setting that is std::nullopt selects the type's default
    struct FormatSpec
    {
        std::optional<bool> homogeneous;
        std::optional<std::string> separator;
        std::array<std::optional<std::string>, 2> brackets;
        std::optional<std::string> series_delimiter;

        // Sets each member that the other spec sets
        constexpr void merge(const FormatSpec& other)
        {
            if (other.homogeneous)
            {
                homogeneous = other.homogeneous;
            }

            if (other.separator)
            {
                separator = other.separator;
            }

            for (size_t i = 0; i < brackets.size(); ++i)
            {
                if (other.brackets[i])
                {
                    brackets[i] = other.brackets[i];
                }
            }

            if (other.series_delimiter)
            {
                series_delimiter = other.series_delimiter;
            }

            return;
        }
    };

    // Returns the settings the manipulators set on the stream
    FormatSpec stream_format_spec(std::ios_base& stream);

    // Each returns the spec's setting, or the type's default if the spec has none
    bool resolved_homogeneous(const FormatSpec& spec, bool default_homogeneous = homogeneous_default);
    std::string_view resolved_separator(const FormatSpec& spec, std::string_view default_separator = separator_default);
    brackets_type resolved_brackets(const FormatSpec& spec, const brackets_type& default_brackets = brackets_default);
    std::string_view resolved_series_delimiter(const FormatSpec& spec, std::string_view default_series_delimiter = series_delimiter_default);

    //--------------------------------------------------------------------------

    const char* skip_whitespace(const char* first, const char* last);

    std::istream& skip_delimiter(std::istream& is, std::string_view delimiter_pattern);

    // Matched exactly, except that whitespace in the pattern skips a run of it.
    const char* skip_delimiter(const char* first, const char* last, std::string_view delimiter_pattern);

    // Returns the first occurrence of the delimiter's text without its leading and trailing whitespace, or last if there is
    // none or the delimiter is only whitespace. A value read up to it cannot consume the delimiter's first character, as
    // a real number would the '.' of ".." in "2..5"
    const char* find_delimiter(const char* first, const char* last, std::string_view delimiter);

    // Delimiter with  whitespace trimmed
    std::string_view trim_ws(std::string_view delimiter);

    // Reads characters from the stream through the first occurrence of the text, and returns those before it. Sets failbit
    // if the stream ends before the text occurs. A value read from what it returns cannot consume the text's first character
    std::string read_until(std::istream& is, std::string_view text);

    // Copies the delimiter to the range. Returns std::errc::value_too_large if it does not fit
    std::to_chars_result write_delimiter(char* first, char* last, std::string_view delimiter);

    //--------------------------------------------------------------------------
    // The letters that follow '/' in a format spec: homogeneous, separator, a pair of single-character brackets, opening and
    // closing bracket, series delimiter
    inline constexpr std::string_view format_spec_prefixes = "hsbocn";

    // Parses quetzal's part of a format spec into the spec. The part begins with '/' and ends at the first unescaped ':', the
    // end of the spec, or the end of the range. Each setting is a prefix, '/' and a letter of format_spec_prefixes; a
    // delimiter's literal text follows its prefix, '/b' is followed by the opening and the closing bracket, and '/h' has no
    // text. '/' followed by any other character is that character. Returns the position of the element type's spec, which is
    // it if the spec does not begin with '/'. Throws std::format_error for an unknown prefix, a setting given twice, text
    // after '/h', or text after '/b' that is not two characters
    constexpr std::format_parse_context::iterator parse_format_spec(std::format_parse_context::iterator it,
        std::format_parse_context::iterator end, FormatSpec& spec)
    {
        while (it != end && *it == '/')
        {
            if (++it == end)
            {
                throw std::format_error("a format spec ends with '/'");
            }

            const char prefix = *it;
            if (format_spec_prefixes.find(prefix) == std::string_view::npos)
            {
                throw std::format_error(std::format("'{}' is not a prefix in a format spec", prefix));
            }

            auto set_delimiter = [](std::optional<std::string>& delimiter, std::string_view name, std::string_view text)
            {
                if (delimiter)
                {
                    throw std::format_error(std::format("a format spec sets the {} twice", name));
                }

                delimiter = std::string(text);
                return;
            };

            std::string text;
            for (++it; it != end && *it != ':' && *it != '}'; ++it)
            {
                if (*it == '/')
                {
                    if (it + 1 == end)
                    {
                        throw std::format_error("a format spec ends with '/'");
                    }

                    if (format_spec_prefixes.find(*(it + 1)) != std::string_view::npos)
                    {
                        break;
                    }

                    ++it;
                }

                text += *it;
            }

            switch (prefix)
            {
                case 'h':
                    if (spec.homogeneous)
                    {
                        throw std::format_error("a format spec sets the coordinate form twice");
                    }

                    if (!text.empty())
                    {
                        throw std::format_error(std::format("the prefix '/h' in a format spec is followed by \"{}\"", text));
                    }

                    spec.homogeneous = true;
                    break;
                case 's':
                    set_delimiter(spec.separator, "separator", text);
                    break;
                case 'b':
                    if (text.size() != 2)
                    {
                        throw std::format_error(std::format("the prefix '/b' in a format spec is followed by \"{}\", not two characters", text));
                    }

                    set_delimiter(spec.brackets[0], "opening bracket", std::string_view(text).substr(0, 1));
                    set_delimiter(spec.brackets[1], "closing bracket", std::string_view(text).substr(1, 1));
                    break;
                case 'o':
                    set_delimiter(spec.brackets[0], "opening bracket", text);
                    break;
                case 'c':
                    set_delimiter(spec.brackets[1], "closing bracket", text);
                    break;
                case 'n':
                    set_delimiter(spec.series_delimiter, "series delimiter", text);
                    break;
            }
        }

        if (it != end && *it == ':')
        {
            ++it;
        }

        return it;
    }

    //--------------------------------------------------------------------------
    // At namespace scope, so that an unqualified call finds the standard overloads and quetzal's together
    using std::from_chars;

    // Reads true, false, 1 or 0, and stops after it, as the standard overloads stop after a number
    std::from_chars_result from_chars(const char* first, const char* last, bool& value);

    template<typename E>
    struct series_formatter;

    //--------------------------------------------------------------------------
    // A type whose text has a series delimiter of its own: a series type or a composite type
    template<typename C>
    concept series_convertible = requires { typename std::formatter<C>::element_type; }
        && std::derived_from<std::formatter<C>, series_formatter<typename std::formatter<C>::element_type>>;

    //--------------------------------------------------------------------------
    // Returns the number of values in an array type's text: its dimension, or every stored component if the text is homogeneous
    template<typename T>
    constexpr size_t text_value_count(bool homogeneous)
    {
        if constexpr (requires { T::component_count; })
        {
            return homogeneous ? T::component_count : T::dimension;
        }
        else
        {
            return T::dimension;
        }
    }

    //--------------------------------------------------------------------------
    // Reads the value from the stream's text before the next occurrence of the delimiter's text, consuming that text, so the
    // value cannot consume the text's first character. Sets failbit if the value is not all of the text before it
    template<typename T>
    std::istream& read_before_delimiter(std::istream& is, T& value, std::string_view text)
    {
        is >> std::ws;
        std::istringstream value_stream(read_until(is, text));
        value_stream.copyfmt(is);
        value_stream >> value;

        if (!value_stream.fail() && !value_stream.eof())
        {
            value_stream >> std::ws;
        }

        if (value_stream.fail() || !value_stream.eof())
        {
            is.setstate(std::ios::failbit);
        }

        return is;
    }

    //--------------------------------------------------------------------------
    // Reads the values of an array type with the stream's separator and brackets
    template<typename T>
    std::istream& read_array(std::istream& is, T& t)
    {
        const FormatSpec spec = stream_format_spec(is);
        const std::string_view separator = resolved_separator(spec);
        const brackets_type brackets = resolved_brackets(spec);

        if (!brackets[0].empty() && !skip_delimiter(is >> std::ws, brackets[0]))
        {
            return is;
        }

        for (size_t i = 0; i < text_value_count<T>(resolved_homogeneous(spec)); ++i)
        {
            if (i > 0 && !skip_delimiter(is, separator))
            {
                return is;
            }

            is >> t[i];
        }

        if (!brackets[1].empty())
        {
            skip_delimiter(is >> std::ws, brackets[1]);
        }

        return is;
    }

    //--------------------------------------------------------------------------
    // Reads each element of a series, with the stream's series delimiter, or the default if the stream has none, between
    // them, each with its own operator>>. Each element but the last is read from the text before the next delimiter, so it
    // cannot consume the delimiter's first character
    template<std::ranges::random_access_range R>
    std::istream& read_series(std::istream& is, R& elements, std::string_view default_series_delimiter = series_delimiter_default)
    {
        const std::string series_delimiter(resolved_series_delimiter(stream_format_spec(is), default_series_delimiter));
        std::string_view text = trim_ws(series_delimiter);

        for (size_t i = 0; i + 1 < elements.size(); ++i)
        {
            if (text.empty())
            {
                is >> elements[i];
                skip_delimiter(is, series_delimiter);
                continue;
            }

            read_before_delimiter(is, elements[i], text);
        }

        if (!elements.empty())
        {
            is >> elements.back();
        }

        return is;
    }

    //--------------------------------------------------------------------------
    // Reads the components of a composite type, in order, with the stream's series delimiter, or the default if the stream
    // has none, between them, each with its own operator>>. Each component but the last that has no series delimiter of its
    // own is read from the text before the next delimiter
    template<typename... C>
    std::istream& read_components(std::istream& is, C&... components)
    {
        const std::string series_delimiter(resolved_series_delimiter(stream_format_spec(is), series_delimiter_default));
        const std::string_view text = trim_ws(series_delimiter);

        size_t i = 0;
        bool delimiter_read = false;
        auto read_component = [&](auto& component)
        {
            if (i > 0 && !delimiter_read)
            {
                skip_delimiter(is, series_delimiter);
            }

            delimiter_read = false;
            if (++i < sizeof...(C) && !series_convertible<std::remove_cvref_t<decltype(component)>> && !text.empty())
            {
                read_before_delimiter(is, component, text);
                delimiter_read = true;
            }
            else
            {
                is >> component;
            }

            return !is.fail();
        };

        (read_component(components) && ...);
        return is;
    }

    //--------------------------------------------------------------------------
    // Reads one component or element: a type with from_chars of its own, or a number after any whitespace
    template<typename C>
    std::from_chars_result component_from_chars(const char* first, const char* last, C& component, const FormatSpec& spec)
    {
        if constexpr (requires { from_chars(first, last, component, spec); })
        {
            return from_chars(first, last, component, spec);
        }
        else
        {
            return from_chars(skip_whitespace(first, last), last, component);
        }
    }

    //--------------------------------------------------------------------------
    // Writes one component or element, as component_from_chars reads it
    template<typename C>
    std::to_chars_result component_to_chars(char* first, char* last, const C& component, const FormatSpec& spec)
    {
        if constexpr (requires { to_chars(first, last, component, spec); })
        {
            return to_chars(first, last, component, spec);
        }
        else
        {
            // The conditional translates negative zero
            return std::to_chars(first, last, component == C(0) ? C(0) : component);
        }
    }

    //--------------------------------------------------------------------------
    // Reads each element of a series with the series delimiter between them. Each element but the last is read from the text
    // before the next delimiter
    template<std::ranges::random_access_range R>
    std::from_chars_result series_from_chars(const char* first, const char* last, R& elements, const FormatSpec& spec,
        std::string_view default_series_delimiter = series_delimiter_default)
    {
        const std::string_view series_delimiter = resolved_series_delimiter(spec, default_series_delimiter);
        std::from_chars_result result {first, std::errc {}};

        for (size_t i = 0; i < elements.size(); ++i)
        {
            if (i > 0)
            {
                const char* position = skip_delimiter(result.ptr, last, series_delimiter);
                if (position == nullptr)
                {
                    return {result.ptr, std::errc::invalid_argument};
                }

                result.ptr = position;
            }

            const char* element_last = i + 1 < elements.size() ? find_delimiter(result.ptr, last, series_delimiter) : last;
            result = component_from_chars(result.ptr, element_last, elements[i], spec);
            if (result.ec != std::errc {})
            {
                return result;
            }
        }

        return result;
    }

    //--------------------------------------------------------------------------
    // Writes each element of a series with the series delimiter between them
    template<std::ranges::random_access_range R>
    std::to_chars_result series_to_chars(char* first, char* last, const R& elements, const FormatSpec& spec,
        std::string_view default_series_delimiter = series_delimiter_default)
    {
        const std::string_view series_delimiter = resolved_series_delimiter(spec, default_series_delimiter);
        std::to_chars_result result {first, std::errc {}};

        for (size_t i = 0; i < elements.size(); ++i)
        {
            if (i > 0)
            {
                result = write_delimiter(result.ptr, last, series_delimiter);
                if (result.ec != std::errc {})
                {
                    return result;
                }
            }

            result = component_to_chars(result.ptr, last, elements[i], spec);
            if (result.ec != std::errc {})
            {
                return result;
            }
        }

        return result;
    }

    //--------------------------------------------------------------------------
    // Reads the components of a composite type, in order, with the series delimiter between them. Each component but the
    // last that has no series delimiter of its own is read from the text before the next delimiter
    template<typename... C>
    std::from_chars_result components_from_chars(const char* first, const char* last, const FormatSpec& spec, C&... components)
    {
        const std::string_view series_delimiter = resolved_series_delimiter(spec, series_delimiter_default);
        std::from_chars_result result {first, std::errc {}};

        size_t i = 0;
        auto read_component = [&](auto& component)
        {
            if (i > 0)
            {
                const char* position = skip_delimiter(result.ptr, last, series_delimiter);
                if (position == nullptr)
                {
                    result = {result.ptr, std::errc::invalid_argument};
                    return false;
                }

                result.ptr = position;
            }

            const bool bounded = ++i < sizeof...(C) && !series_convertible<std::remove_cvref_t<decltype(component)>>;
            const char* component_last = bounded ? find_delimiter(result.ptr, last, series_delimiter) : last;
            result = component_from_chars(result.ptr, component_last, component, spec);
            return result.ec == std::errc {};
        };

        (read_component(components) && ...);
        return result;
    }

    //--------------------------------------------------------------------------
    // Writes the components of a composite type, in order, with the series delimiter between them
    template<typename... C>
    std::to_chars_result components_to_chars(char* first, char* last, const FormatSpec& spec, const C&... components)
    {
        const std::string_view series_delimiter = resolved_series_delimiter(spec, series_delimiter_default);
        std::to_chars_result result {first, std::errc {}};

        size_t i = 0;
        auto write_component = [&](const auto& component)
        {
            if (i++ > 0)
            {
                result = write_delimiter(result.ptr, last, series_delimiter);
                if (result.ec != std::errc {})
                {
                    return false;
                }
            }

            result = component_to_chars(result.ptr, last, component, spec);
            return result.ec == std::errc {};
        };

        (write_component(components) && ...);
        return result;
    }

    //--------------------------------------------------------------------------
    template<typename T>
    std::from_chars_result array_from_chars(const char* first, const char* last, T& t, const FormatSpec& spec)
    {
        const std::string_view separator = resolved_separator(spec);
        const brackets_type brackets = resolved_brackets(spec);
        std::from_chars_result result {skip_whitespace(first, last), std::errc {}};

        if (!brackets[0].empty())
        {
            const char* position = skip_delimiter(result.ptr, last, brackets[0]);
            if (position == nullptr)
            {
                return {result.ptr, std::errc::invalid_argument};
            }

            result.ptr = position;
        }

        for (size_t i = 0; i < text_value_count<T>(resolved_homogeneous(spec)); ++i)
        {
            const char* position = i == 0
                ? skip_whitespace(result.ptr, last)
                : skip_delimiter(result.ptr, last, separator);

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

        if (!brackets[1].empty())
        {
            const char* position = skip_delimiter(skip_whitespace(result.ptr, last), last, brackets[1]);
            if (position == nullptr)
            {
                return {result.ptr, std::errc::invalid_argument};
            }

            result.ptr = position;
        }

        return result;
    }

    //--------------------------------------------------------------------------
    template<typename T>
    std::to_chars_result array_to_chars(char* first, char* last, const T& t, const FormatSpec& spec)
    {
        using element_type = typename T::value_type;

        const std::string_view separator = resolved_separator(spec);
        const brackets_type brackets = resolved_brackets(spec);

        std::to_chars_result result = write_delimiter(first, last, brackets[0]);
        if (result.ec != std::errc {})
        {
            return result;
        }

        for (size_t i = 0; i < text_value_count<T>(resolved_homogeneous(spec)); ++i)
        {
            if (i > 0)
            {
                result = write_delimiter(result.ptr, last, separator);
                if (result.ec != std::errc {})
                {
                    return result;
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

        return write_delimiter(result.ptr, last, brackets[1]);
    }

    //--------------------------------------------------------------------------
    // The std::formatter base of an array type: the parsed spec and the loop that writes the elements
    //
    // The spec's homogeneous flag, separator, and brackets apply, and its series delimiter is ignored
    // (e.g. "/s, /b[]:.2f" -> "[1.50, 2.25, 3.00]"). The element type's own formatter parses what follows it
    template<typename T>
    struct array_formatter
    {
        std::formatter<T> m_element_formatter;
        FormatSpec m_format_spec;

        array_formatter() = default;

        explicit constexpr array_formatter(const FormatSpec& default_format_spec) :
            m_format_spec(default_format_spec)
        {
        }

        constexpr void set_separator(std::string_view separator)
        {
            m_format_spec.separator = std::string(separator);
            return;
        }

        constexpr void set_brackets(std::string_view opening_bracket, std::string_view closing_bracket)
        {
            m_format_spec.brackets = {std::string(opening_bracket), std::string(closing_bracket)};
            return;
        }

        constexpr void set_format_spec(const FormatSpec& spec)
        {
            m_format_spec.merge(spec);
            return;
        }

        constexpr auto parse(std::format_parse_context& ctx)
        {
            FormatSpec spec;
            ctx.advance_to(parse_format_spec(ctx.begin(), ctx.end(), spec));
            set_format_spec(spec);
            return m_element_formatter.parse(ctx);
        }

        template<typename A, typename FormatContext>
        auto format_elements(const A& a, FormatContext& ctx) const
        {
            const std::string_view separator = resolved_separator(m_format_spec);
            const brackets_type brackets = resolved_brackets(m_format_spec);
            auto out = ctx.out();

            for (char c : brackets[0])
            {
                *out++ = c;
            }

            ctx.advance_to(out);

            for (size_t i = 0; i < text_value_count<A>(resolved_homogeneous(m_format_spec)); ++i)
            {
                if (i > 0)
                {
                    for (char c : separator)
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

            for (char c : brackets[1])
            {
                *out++ = c;
            }

            return out;
        }
    };

    //--------------------------------------------------------------------------
    // Returns a formatter of the array type A with the format spec and element format of the array formatter, such as a
    // vector formatter that writes a direction as a point formatter writes a position
    template<typename A, typename T>
    std::formatter<A> matching_formatter(const array_formatter<T>& formatter)
    {
        std::formatter<A> result;
        static_cast<array_formatter<T>&>(result) = formatter;
        return result;
    }

    //--------------------------------------------------------------------------
    // The std::formatter base of a series type, whose elements are of type E: the parsed spec and the series it writes
    //
    // The spec's series delimiter applies to the series, and the rest of the spec to an element type that has a format
    // spec of its own (e.g. "/n.." -> "2..5"; "/n../b[]/s, " -> "[1, 2, 3]..[4, 5, 6]"). The element type's own formatter
    // parses what follows it
    template<typename E>
    struct series_formatter
    {
        using element_type = E;

        std::formatter<E> m_element_formatter;
        FormatSpec m_format_spec;

        series_formatter() = default;

        explicit constexpr series_formatter(const FormatSpec& default_format_spec)
        {
            set_format_spec(default_format_spec);
        }

        constexpr void set_series_delimiter(std::string_view series_delimiter)
        {
            m_format_spec.series_delimiter = std::string(series_delimiter);
            return;
        }

        constexpr void set_format_spec(const FormatSpec& spec)
        {
            m_format_spec.merge(spec);

            if constexpr (requires { m_element_formatter.set_format_spec(spec); })
            {
                m_element_formatter.set_format_spec(spec);
            }

            return;
        }

        constexpr auto parse(std::format_parse_context& ctx)
        {
            FormatSpec spec;
            ctx.advance_to(parse_format_spec(ctx.begin(), ctx.end(), spec));

            auto it = m_element_formatter.parse(ctx);
            set_format_spec(spec);
            return it;
        }

        template<typename FormatContext>
        auto format_series(std::span<const E> elements, FormatContext& ctx) const
        {
            auto out = ctx.out();

            for (size_t i = 0; i < elements.size(); ++i)
            {
                if (i > 0)
                {
                    out = format_series_delimiter(ctx);
                }

                out = m_element_formatter.format(elements[i], ctx);
                ctx.advance_to(out);
            }

            return out;
        }

        template<typename FormatContext>
        auto format_series_delimiter(FormatContext& ctx) const
        {
            auto out = ctx.out();

            for (char c : resolved_series_delimiter(m_format_spec))
            {
                *out++ = c;
            }

            ctx.advance_to(out);
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
    // The std::format spec fragment equivalent to an ostream's precision and fixed/scientific flags, for one number. The
    // defaults match the empty spec, so operator<< and format({}) produce the same output
    std::string numeric_format_string(const std::ostream& os, bool precision = true);

    //--------------------------------------------------------------------------
    // A value and the formatter, already parsed and given its format spec, that writes it
    template<typename T>
    struct formatted_value
    {
        const T& value;
        const std::formatter<T>& formatter;
    };

    //--------------------------------------------------------------------------
    // to_string following the stream's format spec and floating point manipulators. The format spec is set on the formatter
    // directly, so any delimiter can be written, including one that a std::format spec cannot represent
    template<typename T>
    std::string to_string(const T& t, const std::ostream& os)
    {
        constexpr bool precision = !std::is_integral_v<typename T::value_type>;
        const std::string spec = numeric_format_string(os, precision);

        std::formatter<T> formatter;
        std::format_parse_context parse_context(spec);
        formatter.parse(parse_context);
        formatter.set_format_spec(stream_format_spec(const_cast<std::ostream&>(os))); // pword isn't const, though this only reads

        return std::format("{}", formatted_value<T> {t, formatter});
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
struct std::formatter<quetzal::formatted_value<T>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const quetzal::formatted_value<T>& formatted, FormatContext& ctx) const
    {
        return formatted.formatter.format(formatted.value, ctx);
    }
};

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
