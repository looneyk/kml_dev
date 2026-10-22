//------------------------------------------------------------------------------
// to_utf8_test
// main.cpp
//------------------------------------------------------------------------------

#include "to_utf8.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <format>
#include <string>

using namespace std;

namespace
{

    //--------------------------------------------------------------------------
    void check_conversion(const string& bytes, const string& expected_text, const string& expected_encoding_label)
    {
        utf8_conversion conversion = to_utf8(bytes);

        CHECK_CASE("the text is converted", conversion.text == expected_text);
        CHECK_CASE_EQUAL("the encoding label", conversion.encoding_label, expected_encoding_label);
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("empty input")
    {
        check_conversion("", "", "utf-8");
        return;
    }

    TEST_CASE("plain utf-8, no bom")
    {
        check_conversion("hello world", "hello world", "utf-8");
        return;
    }

    TEST_CASE("utf-8 bom is stripped")
    {
        check_conversion("\xEF\xBB\xBF" "hello", "hello", "utf-8 (bom removed)");
        return;
    }

    // "hi" as UTF-16LE: FF FE (bom) 68 00 69 00
    TEST_CASE("utf-16le bom")
    {
        check_conversion(string("\xFF\xFE\x68\x00\x69\x00", 6), "hi", "utf-16le -> utf-8");
        return;
    }

    // "hi" as UTF-16BE: FE FF (bom) 00 68 00 69
    TEST_CASE("utf-16be bom")
    {
        check_conversion(string("\xFE\xFF\x00\x68\x00\x69", 6), "hi", "utf-16be -> utf-8");
        return;
    }

    // 0x92 is not valid standalone UTF-8; as windows-1252 it is U+2019 (e2 80 99 in utf-8)
    TEST_CASE("windows-1252 fallback")
    {
        check_conversion("quote: " "\x92" "test" "\x92", "quote: " "\xE2\x80\x99" "test" "\xE2\x80\x99", "windows-1252 -> utf-8");
        return;
    }

} // namespace
