//------------------------------------------------------------------------------
// dungen
// Stair.cpp
//------------------------------------------------------------------------------

#include "Stair.hpp"
#include <iostream>

using namespace std;

//------------------------------------------------------------------------------
quetzal::dungen::Stair::Stair(id_type id, const string& name, point_type position) :
    m_id(id),
    m_name(name),
    m_position(position)
{
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::dungen::Stair::id() const
{
    return m_id;
}

//------------------------------------------------------------------------------
string quetzal::dungen::Stair::name() const
{
    return m_name;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Stair::set_name(const string& name)
{
    m_name = name;
    return;
}

//------------------------------------------------------------------------------
quetzal::dungen::point_type quetzal::dungen::Stair::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Stair::set_position(point_type position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Stair::print(ostream& os) const
{
    os << "Stair " << m_id << "\t" << m_name << "\tposition (" << m_position[0] << ", " << m_position[1] << ")" << endl;
    return;
}

//------------------------------------------------------------------------------
ostream& quetzal::dungen::operator<<(ostream& os, const Stair& stair)
{
    stair.print(os);
    return os;
}
