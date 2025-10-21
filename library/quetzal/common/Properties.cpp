//------------------------------------------------------------------------------
// quetzal
// Properties.cpp
//------------------------------------------------------------------------------

#include "Properties.hpp"
#include <iostream>

using namespace std;

//------------------------------------------------------------------------------
quetzal::Properties& quetzal::operator|=(Properties& lhs, const Properties& rhs)
{
    for (const auto& property : rhs)
    {
        lhs.insert(property);
    }

    return lhs;
}

//------------------------------------------------------------------------------
quetzal::Properties quetzal::operator|(Properties lhs, const Properties& rhs)
{
    return lhs |= rhs;
}

//------------------------------------------------------------------------------
istream& quetzal::operator>>(istream& is, Properties& properties)
{
    // read JSON ...
    properties;
    return is;
}

//------------------------------------------------------------------------------
ostream& quetzal::operator<<(ostream& os, const Properties& properties)
{
    bool bFirst = true;

    os << '{';
    for (const auto& property : properties)
    {
        if (!bFirst)
        {
            os << ", ";
        }

        os << "    \"" << property.first << "\": \"" << property.second << "\"\n";
        bFirst = false;
    }

    os << '}';
/*
    std::string properties_text = properties |
        std::transform([](auto property) { return std::format("    \"{}\": \"{}\"\n", property.first, property.second); }) |
        std::views::join_with(' ') |
        std::ranges::to<std::string>();
    os << std::format({{{}}}, properties_text);
*/
    return os;
}
