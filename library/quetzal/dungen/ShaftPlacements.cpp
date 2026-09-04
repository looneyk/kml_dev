//------------------------------------------------------------------------------
// dungen
// ShaftPlacements.cpp
//------------------------------------------------------------------------------

#include "ShaftPlacements.hpp"
#include <format>
#include <iostream>
#include <stdexcept>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
string quetzal::dungen::to_string(ShaftPlacements placements)
{
    switch (placements)
    {
        case ShaftPlacements::None:
            return "none";

        case ShaftPlacements::Paired:
            return "paired";

        case ShaftPlacements::Multiple:
            return "multiple";

        default:
            assert(false);
    }

    return "";
}

//------------------------------------------------------------------------------
quetzal::dungen::ShaftPlacements quetzal::dungen::to_shaft_placements(string_view s)
{
    if (s == "none")
    {
        return ShaftPlacements::None;
    }
    else if (s == "paired")
    {
        return ShaftPlacements::Paired;
    }
    else if (s == "multiple")
    {
        return ShaftPlacements::Multiple;
    }

    throw invalid_argument(format("\"{}\" is not none, paired or multiple", s));
}

//------------------------------------------------------------------------------
ostream& quetzal::dungen::operator<<(ostream& os, const ShaftPlacements& placements)
{
    os << to_string(placements);
    return os;
}
