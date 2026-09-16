//------------------------------------------------------------------------------
// dungen
// remove_doors.cpp
//------------------------------------------------------------------------------

#include "remove_doors.hpp"
#include <utility>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
void quetzal::dungen::remove_doors(Level& level, double door_probability, random_type& random)
{
    Level::doors_type doors;
    swap(doors, level.doors());

    for (const auto& door : doors)
    {
        assert(level.cell(door.position()).type() == CellType::Door);

        if (random.gen() < door_probability)
        {
            level.create_door(door.position());
        }
        else
        {
            level.cell(door.position()).set_type(CellType::Corridor);
        }
    }

    return;
}
