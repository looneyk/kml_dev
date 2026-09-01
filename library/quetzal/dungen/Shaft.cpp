//------------------------------------------------------------------------------
// dungen
// Shaft.cpp
//------------------------------------------------------------------------------

#include "Shaft.hpp"
#include <iostream>

using namespace std;

//------------------------------------------------------------------------------
quetzal::dungen::Shaft::Shaft(id_type id, const string& name, position_type position) :
    m_id(id),
    m_name(name),
    m_position(position)
{
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::dungen::Shaft::id() const
{
    return m_id;
}

//------------------------------------------------------------------------------
string quetzal::dungen::Shaft::name() const
{
    return m_name;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Shaft::set_name(const string& name)
{
    m_name = name;
    return;
}

//------------------------------------------------------------------------------
quetzal::dungen::position_type quetzal::dungen::Shaft::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Shaft::set_position(position_type position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Shaft::print(ostream& os) const
{
    os << "Shaft " << m_id << "\t" << m_name << "\tposition (" << m_position[0] << ", " << m_position[1] << ")" << endl;
    return;
}

//------------------------------------------------------------------------------
ostream& quetzal::dungen::operator<<(ostream& os, const Shaft& shaft)
{
    shaft.print(os);
    return os;
}
