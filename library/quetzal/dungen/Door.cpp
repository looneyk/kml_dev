//------------------------------------------------------------------------------
// dungen
// Door.cpp
//------------------------------------------------------------------------------

#include "Door.hpp"
#include <iostream>

using namespace std;

//------------------------------------------------------------------------------
quetzal::dungen::Door::Door(id_type id, const string& name, position_type position) :
    m_id(id),
    m_name(name),
    m_position(position)
{
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::dungen::Door::id() const
{
    return m_id;
}

//------------------------------------------------------------------------------
string quetzal::dungen::Door::name() const
{
    return m_name;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Door::set_name(const string& name)
{
    m_name = name;
    return;
}

//------------------------------------------------------------------------------
quetzal::dungen::position_type quetzal::dungen::Door::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Door::set_position(position_type position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Door::print(ostream& os) const
{
    os << "Door " << m_id << "\t" << m_name << "\tposition (" << m_position[0] << ", " << m_position[1] << ")" << endl;
    return;
}

//------------------------------------------------------------------------------
ostream& quetzal::dungen::operator<<(ostream& os, const Door& door)
{
    door.print(os);
    return os;
}
