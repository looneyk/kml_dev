//------------------------------------------------------------------------------
// dungen
// StairPlacements.cpp
//------------------------------------------------------------------------------

#include "StairPlacements.hpp"
#include <format>
#include <iostream>
#include <stdexcept>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
string quetzal::dungen::to_string(StairPlacements placements)
{
    switch (placements)
    {
        case StairPlacements::None:
            return "none";

        case StairPlacements::Paired:
            return "paired";

        case StairPlacements::Multiple:
            return "multiple";

        default:
            assert(false);
    }

    return "";
}

//------------------------------------------------------------------------------
quetzal::dungen::StairPlacements quetzal::dungen::to_stair_placements(string_view s)
{
    if (s == "none")
    {
        return StairPlacements::None;
    }
    else if (s == "paired")
    {
        return StairPlacements::Paired;
    }
    else if (s == "multiple")
    {
        return StairPlacements::Multiple;
    }

    throw invalid_argument(format("\"{}\" is not none, paired or multiple", s));
}

//------------------------------------------------------------------------------
ostream& quetzal::dungen::operator<<(ostream& os, const StairPlacements& placements)
{
    os << to_string(placements);
    return os;
}
