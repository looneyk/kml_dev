//------------------------------------------------------------------------------
// string_util.cpp
//------------------------------------------------------------------------------

#include "string_util.hpp"
#include "utf8.h"
#include "utf8/cpp11.h"
#include <ios>
#include <cassert>

using namespace std;

namespace
{

    //--------------------------------------------------------------------------
    int array_object_delim_index()
    {
        static int index = ios_base::xalloc();
        return index;
    }

    //--------------------------------------------------------------------------
    int array_element_delim_index()
    {
        static int index = ios_base::xalloc();
        return index;
    }

    //--------------------------------------------------------------------------
    // pword is a raw void*, so this callback copies it on copyfmt and deletes it on stream destruction
    void array_element_delim_callback(ios_base::event event, ios_base& stream, int index)
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
    void set_element_delim(ios_base& stream, const string& value)
    {
        if (value.empty()) // leave whatever element delimiter is already in effect unchanged
        {
            return;
        }

        int index = array_element_delim_index();
        if (stream.pword(index) == nullptr)
        {
            stream.register_callback(array_element_delim_callback, index);
        }

        delete static_cast<string*>(stream.pword(index));
        stream.pword(index) = new string(value);
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::object_delim_manip quetzal::object_delim(char c)
{
    assert(c == '\0' || c == '[' || c == '(');
    return {c};
}

//------------------------------------------------------------------------------
ostream& quetzal::operator<<(ostream& os, object_delim_manip manip)
{
    os.iword(array_object_delim_index()) = manip.value;
    return os;
}

//------------------------------------------------------------------------------
quetzal::element_delim_manip quetzal::element_delim(string s)
{
    return {std::move(s)};
}

//------------------------------------------------------------------------------
ostream& quetzal::operator<<(ostream& os, const element_delim_manip& manip)
{
    set_element_delim(os, manip.value);
    return os;
}

//------------------------------------------------------------------------------
istream& quetzal::operator>>(istream& is, object_delim_manip manip)
{
    is.iword(array_object_delim_index()) = manip.value;
    return is;
}

//------------------------------------------------------------------------------
istream& quetzal::operator>>(istream& is, const element_delim_manip& manip)
{
    set_element_delim(is, manip.value);
    return is;
}

//------------------------------------------------------------------------------
string quetzal::array_element_delimiter(ios_base& stream, const string& default_delimiter)
{
    void* p = stream.pword(array_element_delim_index());
    return p != nullptr ? *static_cast<string*>(p) : default_delimiter;
}

//------------------------------------------------------------------------------
const char* quetzal::skip_whitespace(const char* first, const char* last)
{
    while (first != last && (*first == ' ' || *first == '\t'))
    {
        ++first;
    }
    return first;
}

//------------------------------------------------------------------------------
istream& quetzal::skip_object_delimiter(istream& is, char delimiter)
{
    is >> ws;
    istream::int_type c = is.peek();
    if (c == istream::traits_type::to_int_type(delimiter))
    {
        is.ignore();
    }
    else
    {
        is.setstate(ios::failbit);
    }
    return is;
}

//------------------------------------------------------------------------------
istream& quetzal::skip_element_delimiter(istream& is, const string& delimiter_pattern)
{
    for (char c : delimiter_pattern)
    {
        if (c == ' ' || c == '\t')
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
const char* quetzal::skip_element_delimiter(const char* first, const char* last, const string& delimiter_pattern)
{
    for (char c : delimiter_pattern)
    {
        if (c == ' ' || c == '\t')
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
string quetzal::array_format_spec(const ostream& os, char default_object_delim, const string& default_element_delim, bool precision)
{
    auto& mutable_os = const_cast<ostream&>(os); // iword/pword aren't const, though this only reads

    long object_delim_value = mutable_os.iword(array_object_delim_index());
    char object_delim_char = object_delim_value != 0 ? static_cast<char>(object_delim_value) : default_object_delim;

    string element_delim_value = array_element_delimiter(mutable_os, default_element_delim);

    string spec;
    if (object_delim_char != '\0')
    {
        spec += object_delim_char;
    }
    for (char c : element_delim_value)
    {
        spec += (c == ' ') ? '_' : c;
    }
    if (precision)
    {
        spec += '.';
        spec += std::to_string(os.precision());
    }

    if (os.flags() & ios::fixed)
    {
        spec += 'f';
    }
    else if (os.flags() & ios::scientific)
    {
        spec += 'e';
    }

    return spec;
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
template<>
string quetzal::to_type<string>(const string& str)
{
    return str;
}

//------------------------------------------------------------------------------
template<>
wstring quetzal::to_type<wstring>(const string& str)
{
    if constexpr (sizeof(wchar_t) == 2)
    {
        u16string utf16 = utf8::utf8to16(str);
        return wstring(utf16.begin(), utf16.end());
    }
    else
    {
        u32string utf32 = utf8::utf8to32(str);
        return wstring(utf32.begin(), utf32.end());
    }
}

//------------------------------------------------------------------------------
template<>
bool quetzal::to_type<bool>(const string& str)
{
    return(to_bool(str));
}

//------------------------------------------------------------------------------
bool quetzal::to_bool(const string& str)
{
    if (str == "true" || str == "1")
    {
        return true;
    }

    if (str != "false" && str != "0")
    {
        throw std::domain_error("Invalid boolean string value: " + str);
    }

    return false;
}
