//------------------------------------------------------------------------------
// dungen
// CellType.cpp
//------------------------------------------------------------------------------

#include "CellType.hpp"
#include <iostream>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
ostream& quetzal::dungen::operator<<(ostream& os, const CellType& type)
{
    switch (type)
    {
        case CellType::Null:
            os << "Null";
            break;

        case CellType::Solid:
            os << "Solid";
            break;

        case CellType::Room:
            os << "Room";
            break;

        case CellType::Corridor:
            os << "Corridor";
            break;

        case CellType::Door:
            os << "Door";
            break;

        case CellType::Shaft:
            os << "Shaft";
            break;

        default:
            assert(false);
    }

    return os;
}
