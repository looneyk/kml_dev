//------------------------------------------------------------------------------
// main.cpp
//------------------------------------------------------------------------------

#include "to_utf8.hpp"
#include <iostream>
#include <string>

using namespace std;

//------------------------------------------------------------------------------
void test(const string& description, const string& bytes, const string& expected_text, const string& expected_encoding_label)
{
    utf8_conversion conversion = to_utf8(bytes);

    cout << description << endl;

    if (conversion.text != expected_text)
    {
        cout << "failed: text mismatch" << endl;
    }

    if (conversion.encoding_label != expected_encoding_label)
    {
        cout << "failed: encoding_label should be \"" << expected_encoding_label << "\", got \"" << conversion.encoding_label << "\"" << endl;
    }

    return;
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    test("empty input", "", "", "utf-8");

    test("plain utf-8, no bom", "hello world", "hello world", "utf-8");

    test("utf-8 bom is stripped", "\xEF\xBB\xBF" "hello", "hello", "utf-8 (bom removed)");

    // "hi" as UTF-16LE: FF FE (bom) 68 00 69 00
    test("utf-16le bom", string("\xFF\xFE\x68\x00\x69\x00", 6), "hi", "utf-16le -> utf-8");

    // "hi" as UTF-16BE: FE FF (bom) 00 68 00 69
    test("utf-16be bom", string("\xFE\xFF\x00\x68\x00\x69", 6), "hi", "utf-16be -> utf-8");

    // 0x92 is not valid standalone UTF-8; as windows-1252 it is U+2019 (e2 80 99 in utf-8)
    test("windows-1252 fallback", "quote: " "\x92" "test" "\x92", "quote: " "\xE2\x80\x99" "test" "\xE2\x80\x99", "windows-1252 -> utf-8");

    cout << "Done" << endl;

    char c;
    cin >> c;

    return 0;
}
