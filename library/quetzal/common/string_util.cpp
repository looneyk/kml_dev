//------------------------------------------------------------------------------
// string_util.cpp
//------------------------------------------------------------------------------

#include "string_util.hpp"
#include "utf8.h"
#include "utf8/cpp11.h"

using namespace std;

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
