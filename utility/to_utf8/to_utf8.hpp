//------------------------------------------------------------------------------
// to_utf8
// to_utf8.hpp
//------------------------------------------------------------------------------

#pragma once

#include <string>

struct utf8_conversion
{
    std::string text;
    std::string encoding_label;
};

utf8_conversion to_utf8(const std::string& bytes);
