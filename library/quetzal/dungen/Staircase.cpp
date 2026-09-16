//------------------------------------------------------------------------------
// dungen
// Staircase.cpp
//------------------------------------------------------------------------------

#include "Staircase.hpp"
#include <iostream>

using namespace std;

//------------------------------------------------------------------------------
quetzal::dungen::Staircase::Staircase(id_type id, const string& name, point_type position, StaircaseDirection direction) :
    m_id(id),
    m_name(name),
    m_position(position),
    m_direction(direction),
    m_facing()
{
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::dungen::Staircase::id() const
{
    return m_id;
}

//------------------------------------------------------------------------------
string quetzal::dungen::Staircase::name() const
{
    return m_name;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Staircase::set_name(const string& name)
{
    m_name = name;
    return;
}

//------------------------------------------------------------------------------
quetzal::dungen::point_type quetzal::dungen::Staircase::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Staircase::set_position(point_type position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
quetzal::dungen::StaircaseDirection quetzal::dungen::Staircase::direction() const
{
    return m_direction;
}

//------------------------------------------------------------------------------
const optional<quetzal::dungen::vector_type>& quetzal::dungen::Staircase::facing() const
{
    return m_facing;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Staircase::set_facing(vector_type facing)
{
    m_facing = facing;
    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Staircase::print(ostream& os) const
{
    os << "Staircase " << m_id << "\t" << m_name << "\t" << (m_direction == StaircaseDirection::Down ? "down" : "up")
        << "\tposition (" << m_position[0] << ", " << m_position[1] << ")" << endl;
    return;
}

//------------------------------------------------------------------------------
ostream& quetzal::dungen::operator<<(ostream& os, const Staircase& staircase)
{
    staircase.print(os);
    return os;
}
