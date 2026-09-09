//------------------------------------------------------------------------------
// dungen
// Room.cpp
//------------------------------------------------------------------------------

#include "Room.hpp"
#include <iostream>

using namespace std;

//------------------------------------------------------------------------------
quetzal::dungen::Room::Room(id_type id, const string& name, const box_type& rectangle) :
    m_id(id),
    m_name(name),
    m_rectangle(rectangle),
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
const quetzal::dungen::box_type& quetzal::dungen::Room::rectangle() const
{
    return m_rectangle;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Room::set_rectangle(const box_type& rectangle)
{
    m_rectangle = rectangle;
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
    os << "Room " << m_id << "\t" << m_name << "\tposition (" << m_rectangle.position()[0] << ", " << m_rectangle.position()[1] << ")\tsize (" << m_rectangle.extent()[0] << ", " << m_rectangle.extent()[1] << ")" << endl;
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
