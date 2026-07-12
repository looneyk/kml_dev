//------------------------------------------------------------------------------
// to_utf8
// to_utf8.cpp
//------------------------------------------------------------------------------

#include "to_utf8.hpp"

#include <windows.h>

using namespace std;

namespace
{

    //--------------------------------------------------------------------------
    bool is_valid_utf8(const string& bytes)
    {
        size_t i = 0;
        size_t n = bytes.size();

        while (i < n)
        {
            unsigned char c = static_cast<unsigned char>(bytes[i]);
            size_t extra = 0;

            if (c <= 0x7f)
            {
                extra = 0;
            }
            else if ((c & 0xe0) == 0xc0)
            {
                extra = 1;
            }
            else if ((c & 0xf0) == 0xe0)
            {
                extra = 2;
            }
            else if ((c & 0xf8) == 0xf0)
            {
                extra = 3;
            }
            else
            {
                return false;
            }

            if (i + extra >= n)
            {
                return false;
            }

            for (size_t j = 1; j <= extra; ++j)
            {
                if ((static_cast<unsigned char>(bytes[i + j]) & 0xc0) != 0x80)
                {
                    return false;
                }
            }

            i += extra + 1;
        }

        return true;
    }

    //--------------------------------------------------------------------------
    string utf16_to_utf8(const wstring& wide)
    {
        if (wide.empty())
        {
            return string();
        }

        int utf8_length = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
        string utf8(utf8_length, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), utf8.data(), utf8_length, nullptr, nullptr);

        return utf8;
    }

    //--------------------------------------------------------------------------
    string ansi_to_utf8(const string& bytes)
    {
        if (bytes.empty())
        {
            return bytes;
        }

        // CP_ACP is not used here: on systems with the "Beta: Use Unicode UTF-8 for
        // worldwide language support" option enabled, GetACP() returns 65001 (UTF-8),
        // which would just re-run invalid UTF-8 through itself as UTF-8. 1252 is the
        // legacy codepage these files were actually authored under.
        constexpr UINT legacy_codepage = 1252;

        int wide_length = MultiByteToWideChar(legacy_codepage, 0, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
        wstring wide(wide_length, L'\0');
        MultiByteToWideChar(legacy_codepage, 0, bytes.data(), static_cast<int>(bytes.size()), wide.data(), wide_length);

        return utf16_to_utf8(wide);
    }

} // namespace

//------------------------------------------------------------------------------
utf8_conversion to_utf8(const string& bytes)
{
    utf8_conversion result;
    result.encoding_label = "utf-8";

    // BOM/validity checks decide the source encoding; only one of these branches applies per file.
    if (bytes.starts_with("\xEF\xBB\xBF"))
    {
        result.text = bytes.substr(3);
        result.encoding_label = "utf-8 (bom removed)";
    }
    else if (bytes.starts_with("\xFF\xFE"))
    {
        wstring wide;
        for (size_t i = 2; i + 1 < bytes.size(); i += 2)
        {
            wchar_t lo = static_cast<unsigned char>(bytes[i]);
            wchar_t hi = static_cast<unsigned char>(bytes[i + 1]);
            wide += static_cast<wchar_t>(lo | (hi << 8));
        }
        result.text = utf16_to_utf8(wide);
        result.encoding_label = "utf-16le -> utf-8";
    }
    else if (bytes.starts_with("\xFE\xFF"))
    {
        wstring wide;
        for (size_t i = 2; i + 1 < bytes.size(); i += 2)
        {
            wchar_t hi = static_cast<unsigned char>(bytes[i]);
            wchar_t lo = static_cast<unsigned char>(bytes[i + 1]);
            wide += static_cast<wchar_t>(lo | (hi << 8));
        }
        result.text = utf16_to_utf8(wide);
        result.encoding_label = "utf-16be -> utf-8";
    }
    else if (is_valid_utf8(bytes))
    {
        result.text = bytes;
    }
    else
    {
        result.text = ansi_to_utf8(bytes);
        result.encoding_label = "windows-1252 -> utf-8";
    }

    return result;
}
