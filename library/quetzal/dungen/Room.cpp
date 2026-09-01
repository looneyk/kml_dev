//------------------------------------------------------------------------------
// dungen
// Room.cpp
//------------------------------------------------------------------------------

#include "Room.hpp"
#include <iostream>

using namespace std;

//------------------------------------------------------------------------------
quetzal::dungen::Room::Room(id_type id, const string& name, position_type position, dimensions_type size) :
    m_id(id),
    m_name(name),
    m_position(position),
    m_size(size),
    m_door_ids()
{
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::dungen::Room::id() const
{
    return m_id;
}

//------------------------------------------------------------------------------
string quetzal::dungen::Room::name() const
{
    return m_name;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Room::set_name(const string& name)
{
    m_name = name;
    return;
}

//------------------------------------------------------------------------------
quetzal::dungen::position_type quetzal::dungen::Room::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Room::set_position(position_type position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
quetzal::dungen::dimensions_type quetzal::dungen::Room::size() const
{
    return m_size;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Room::set_size(dimensions_type size)
{
    m_size = size;
    return;
}

//------------------------------------------------------------------------------
const quetzal::dungen::Room::door_ids_type& quetzal::dungen::Room::door_ids() const
{
    return m_door_ids;
}

//------------------------------------------------------------------------------
quetzal::dungen::Room::door_ids_type& quetzal::dungen::Room::door_ids()
{
    return m_door_ids;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Room::attach_door(id_type door_id)
{
    m_door_ids.push_back(door_id);
    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Room::print(ostream& os) const
{
    os << "Room " << m_id << "\t" << m_name << "\tposition (" << m_position[0] << ", " << m_position[1] << ")\tsize (" << m_size[0] << ", " << m_size[1] << ")" << endl;
    os << "    doors: [";
    bool first = true;
    for (auto id : m_door_ids)
    {
        if (!first)
        {
            os << ", ";
        }

        first = false;
        os << id;
    }
    os << "]" << endl;
    return;
}

//------------------------------------------------------------------------------
ostream& quetzal::dungen::operator<<(ostream& os, const Room& room)
{
    room.print(os);
    return os;
}
