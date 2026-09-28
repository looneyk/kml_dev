//------------------------------------------------------------------------------
// dungen
// remove_doors.cpp
//------------------------------------------------------------------------------

#include "remove_doors.hpp"
#include <utility>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
void quetzal::dungen::remove_doors(Zone& zone, double door_probability, random_type& random)
{
    Zone::doors_type doors;
    swap(doors, zone.doors());

    for (const auto& door : doors)
    {
        assert(zone.cell(door.position()).type() == CellType::Door);

        if (random.gen() < door_probability)
        {
            zone.create_door(door.position());
        }
        else
        {
            zone.cell(door.position()).set_type(CellType::Corridor);
        }
    }

    return;
}
