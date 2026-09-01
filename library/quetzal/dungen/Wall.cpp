//------------------------------------------------------------------------------
// dungen
// Wall.cpp
//------------------------------------------------------------------------------

#include "Wall.hpp"
#include <iostream>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::dungen::Wall::Wall()
{
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::dungen::Wall::cell_id() const
{
    return m_cell_id;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Wall::set_cell_id(id_type cell_id)
{
    m_cell_id = cell_id;
    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Wall::print(ostream& os) const
{
    os << ""; // ...
}

//------------------------------------------------------------------------------
ostream& quetzal::dungen::operator<<(ostream& os, const Wall& wall)
{
    wall.print(os);
    return os;
}
