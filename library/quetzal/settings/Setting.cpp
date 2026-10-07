//------------------------------------------------------------------------------
// settings
// Setting.cpp
//------------------------------------------------------------------------------

#include "Setting.hpp"

//------------------------------------------------------------------------------
std::vector<std::string_view> quetzal::settings::sections_of(std::string_view key)
{
    std::vector<std::string_view> sections;
    for (size_t end = key.find('.'); end != std::string_view::npos; end = key.find('.', end + 1))
    {
        sections.push_back(key.substr(0, end));
    }

    return sections;
}

//------------------------------------------------------------------------------
bool quetzal::settings::is_setting_key(std::string_view key)
{
    return key.find('.') != std::string_view::npos && !key.starts_with('.') && !key.ends_with('.')
        && key.find("..") == std::string_view::npos;
}
