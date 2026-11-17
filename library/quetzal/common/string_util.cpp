//------------------------------------------------------------------------------
// string_util.cpp
//------------------------------------------------------------------------------

#include "string_util.hpp"
#include "utf8.h"
#include "utf8/cpp11.h"
#include <algorithm>
#include <cctype>
#include <format>
#include <ios>
#include <iterator>
#include <cassert>
#include <string_view>
#include <utility>

using namespace std;

namespace
{

    //--------------------------------------------------------------------------
    // A stream's iword at this index is 0 until cartesian or homogeneous sets it
    int coordinate_form_index()
    {
        static int index = ios_base::xalloc();
        return index;
    }

    constexpr long cartesian_form = 1;
    constexpr long homogeneous_form = 2;

    //--------------------------------------------------------------------------
    int separator_index()
    {
        static int index = ios_base::xalloc();
        return index;
    }

    //--------------------------------------------------------------------------
    int opening_bracket_index()
    {
        static int index = ios_base::xalloc();
        return index;
    }

    //--------------------------------------------------------------------------
    int closing_bracket_index()
    {
        static int index = ios_base::xalloc();
        return index;
    }

    //--------------------------------------------------------------------------
    int series_delimiter_index()
    {
        static int index = ios_base::xalloc();
        return index;
    }

    //--------------------------------------------------------------------------
    // pword is a raw void*, so this callback copies it on copyfmt and deletes it on stream destruction
    void delim_text_callback(ios_base::event event, ios_base& stream, int index)
    {
        if (event == ios_base::erase_event)
        {
            delete static_cast<string*>(stream.pword(index));
            stream.pword(index) = nullptr;
        }
        else if (event == ios_base::copyfmt_event)
        {
            auto* p = static_cast<string*>(stream.pword(index));
            stream.pword(index) = p != nullptr ? new string(*p) : nullptr;
        }
    }

    //--------------------------------------------------------------------------
    void set_delim_text(ios_base& stream, int index, const string& value)
    {
        if (stream.pword(index) == nullptr)
        {
            stream.register_callback(delim_text_callback, index);
        }

        delete static_cast<string*>(stream.pword(index));
        stream.pword(index) = new string(value);
    }

    //--------------------------------------------------------------------------
    optional<string> delim_text(ios_base& stream, int index)
    {
        void* p = stream.pword(index);
        return p != nullptr ? make_optional(*static_cast<string*>(p)) : nullopt;
    }

    //--------------------------------------------------------------------------
    bool whitespace(char c)
    {
        return isspace(static_cast<unsigned char>(c)) != 0;
    }

} // namespace

//------------------------------------------------------------------------------
ios_base& quetzal::cartesian(ios_base& stream)
{
    stream.iword(coordinate_form_index()) = cartesian_form;
    return stream;
}

//------------------------------------------------------------------------------
ios_base& quetzal::homogeneous(ios_base& stream)
{
    stream.iword(coordinate_form_index()) = homogeneous_form;
    return stream;
}

//------------------------------------------------------------------------------
quetzal::separator_delim_manip quetzal::separator_delim(string separator)
{
    return {std::move(separator)};
}

//------------------------------------------------------------------------------
ostream& quetzal::operator<<(ostream& os, const separator_delim_manip& manip)
{
    set_delim_text(os, separator_index(), manip.value);
    return os;
}

//------------------------------------------------------------------------------
istream& quetzal::operator>>(istream& is, const separator_delim_manip& manip)
{
    set_delim_text(is, separator_index(), manip.value);
    return is;
}

//------------------------------------------------------------------------------
quetzal::brackets_delim_manip quetzal::brackets_delim(string opening_bracket, string closing_bracket)
{
    return {std::move(opening_bracket), std::move(closing_bracket)};
}

//------------------------------------------------------------------------------
ostream& quetzal::operator<<(ostream& os, const brackets_delim_manip& manip)
{
    set_delim_text(os, opening_bracket_index(), manip.opening_bracket);
    set_delim_text(os, closing_bracket_index(), manip.closing_bracket);
    return os;
}

//------------------------------------------------------------------------------
istream& quetzal::operator>>(istream& is, const brackets_delim_manip& manip)
{
    set_delim_text(is, opening_bracket_index(), manip.opening_bracket);
    set_delim_text(is, closing_bracket_index(), manip.closing_bracket);
    return is;
}

//------------------------------------------------------------------------------
quetzal::series_delim_manip quetzal::series_delim(string series_delimiter)
{
    return {std::move(series_delimiter)};
}

//------------------------------------------------------------------------------
ostream& quetzal::operator<<(ostream& os, const series_delim_manip& manip)
{
    set_delim_text(os, series_delimiter_index(), manip.value);
    return os;
}

//------------------------------------------------------------------------------
istream& quetzal::operator>>(istream& is, const series_delim_manip& manip)
{
    set_delim_text(is, series_delimiter_index(), manip.value);
    return is;
}

//------------------------------------------------------------------------------
quetzal::FormatSpec quetzal::stream_format_spec(ios_base& stream)
{
    FormatSpec spec;

    if (const long form = stream.iword(coordinate_form_index()); form != 0)
    {
        spec.homogeneous = form == homogeneous_form;
    }

    spec.separator = delim_text(stream, separator_index());
    spec.brackets = {delim_text(stream, opening_bracket_index()), delim_text(stream, closing_bracket_index())};
    spec.series_delimiter = delim_text(stream, series_delimiter_index());
    return spec;
}

//------------------------------------------------------------------------------
bool quetzal::resolved_homogeneous(const FormatSpec& spec, bool default_homogeneous)
{
    return spec.homogeneous.value_or(default_homogeneous);
}

//------------------------------------------------------------------------------
string_view quetzal::resolved_separator(const FormatSpec& spec, string_view default_separator)
{
    return spec.separator ? string_view(*spec.separator) : default_separator;
}

//------------------------------------------------------------------------------
quetzal::brackets_type quetzal::resolved_brackets(const FormatSpec& spec, const brackets_type& default_brackets)
{
    return {spec.brackets[0] ? string_view(*spec.brackets[0]) : default_brackets[0],
        spec.brackets[1] ? string_view(*spec.brackets[1]) : default_brackets[1]};
}

//------------------------------------------------------------------------------
string_view quetzal::resolved_series_delimiter(const FormatSpec& spec, string_view default_series_delimiter)
{
    return spec.series_delimiter ? string_view(*spec.series_delimiter) : default_series_delimiter;
}

//------------------------------------------------------------------------------
string_view quetzal::trim_whitespace(string_view str)
{
    const auto text_first = find_if_not(str.begin(), str.end(), whitespace);
    const auto text_last = find_if_not(str.rbegin(), str.rend(), whitespace).base();
    return text_first < text_last ? string_view(text_first, text_last) : string_view{};
}

//------------------------------------------------------------------------------
const char* quetzal::skip_whitespace(const char* first, const char* last)
{
    return find_if_not(first, last, whitespace);
}

//------------------------------------------------------------------------------
istream& quetzal::skip_pattern(istream& is, string_view pattern)
{
    for (char c : pattern)
    {
        if (whitespace(c))
        {
            is >> ws;
            continue;
        }

        if (is.peek() != istream::traits_type::to_int_type(c))
        {
            is.setstate(ios::failbit);
            return is;
        }

        is.ignore();
    }

    return is;
}

//------------------------------------------------------------------------------
const char* quetzal::skip_pattern(const char* first, const char* last, string_view pattern)
{
    for (char c : pattern)
    {
        if (whitespace(c))
        {
            first = skip_whitespace(first, last);
            continue;
        }

        if (first == last || *first != c)
        {
            return nullptr;
        }

        ++first;
    }

    return first;
}

//------------------------------------------------------------------------------
const char* quetzal::find_pattern(const char* first, const char* last, string_view pattern)
{
    string_view text = trim_whitespace(pattern);
    if (text.empty())
    {
        return last;
    }

    return search(first, last, text.begin(), text.end());
}

//------------------------------------------------------------------------------
string quetzal::read_until(istream& is, string_view text)
{
    string read;
    char c = '\0';

    while (is.get(c))
    {
        read += c;
        if (read.ends_with(text))
        {
            read.resize(read.size() - text.size());
            return read;
        }
    }

    is.setstate(ios::failbit);
    return read;
}

//------------------------------------------------------------------------------
to_chars_result quetzal::write_pattern(char* first, char* last, string_view pattern)
{
    if (static_cast<size_t>(last - first) < pattern.size())
    {
        return {first, errc::value_too_large};
    }

    return {copy(pattern.begin(), pattern.end(), first), errc{}};
}

//------------------------------------------------------------------------------
string quetzal::to_string(const string& str)
{
    return str;
}

//------------------------------------------------------------------------------
string quetzal::to_string(const wstring& wstr)
{
    if constexpr (sizeof(wchar_t) == 2)
    {
        return utf8::utf16to8(u16string(wstr.begin(), wstr.end()));
    }
    else
    {
        return utf8::utf32to8(u32string(wstr.begin(), wstr.end()));
    }
}

//------------------------------------------------------------------------------
string quetzal::to_string(const bool& b)
{
    return b ? "true" : "false";
}

//------------------------------------------------------------------------------
from_chars_result quetzal::from_chars(const char* first, const char* last, bool& value)
{
    static constexpr pair<string_view, bool> tokens[] = {{"true", true}, {"false", false}, {"1", true}, {"0", false}};

    const string_view text(first, last);
    for (const auto& [token, token_value] : tokens)
    {
        if (text.starts_with(token))
        {
            value = token_value;
            return {first + token.size(), errc{}};
        }
    }

    return {first, errc::invalid_argument};
}

//------------------------------------------------------------------------------
template<>
optional<string> quetzal::try_type<string>(string_view str, const FormatSpec&)
{
    return string(str);
}

//------------------------------------------------------------------------------
template<>
optional<wstring> quetzal::try_type<wstring>(string_view str, const FormatSpec&)
{
    try
    {
        wstring wstr;
        if constexpr (sizeof(wchar_t) == 2)
        {
            utf8::utf8to16(str.begin(), str.end(), back_inserter(wstr));
        }
        else
        {
            utf8::utf8to32(str.begin(), str.end(), back_inserter(wstr));
        }

        return wstr;
    }
    catch (const utf8::exception&)
    {
        return nullopt;
    }
}

//------------------------------------------------------------------------------
bool quetzal::to_bool(string_view str)
{
    const optional<bool> b = try_type<bool>(str);
    if (!b)
    {
        throw std::runtime_error(format("Invalid boolean string value: {}", str));
    }

    return *b;
}
