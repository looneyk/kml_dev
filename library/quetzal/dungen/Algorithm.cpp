//------------------------------------------------------------------------------
// dungen
// Algorithm.cpp
//------------------------------------------------------------------------------

#include "Algorithm.hpp"
#include <format>
#include <iostream>
#include <stdexcept>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
string quetzal::dungen::to_string(Algorithm algorithm)
{
    switch (algorithm)
    {
        case Algorithm::Maze1:
            return "maze1";

        case Algorithm::Grid1:
            return "grid1";

        case Algorithm::Cave1:
            return "cave1";

        case Algorithm::Surface1:
            return "surface1";

        default:
            assert(false);
    }

    return "";
}

//------------------------------------------------------------------------------
quetzal::dungen::Algorithm quetzal::dungen::to_algorithm(string_view s)
{
    if (s == "maze1")
    {
        return Algorithm::Maze1;
    }
    else if (s == "grid1")
    {
        return Algorithm::Grid1;
    }
    else if (s == "cave1")
    {
        return Algorithm::Cave1;
    }
    else if (s == "surface1")
    {
        return Algorithm::Surface1;
    }

    throw invalid_argument(format("\"{}\" is not maze1, grid1, cave1 or surface1", s));
}

//------------------------------------------------------------------------------
ostream& quetzal::dungen::operator<<(ostream& os, const Algorithm& algorithm)
{
    os << to_string(algorithm);
    return os;
}
