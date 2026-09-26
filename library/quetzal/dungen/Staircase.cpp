//------------------------------------------------------------------------------
// dungen
// Staircase.cpp
//------------------------------------------------------------------------------

#include "Staircase.hpp"
#include <iostream>

using namespace std;

//------------------------------------------------------------------------------
std::string quetzal::dungen::to_string(Rise rise)
{
    return rise == Rise::Up ? "up" : "down";
}

//------------------------------------------------------------------------------
quetzal::dungen::Staircase::Staircase(id_type id, const string& name, directed_point_type directed_position, Rise rise) :
    m_id(id),
    m_name(name),
    m_directed_position(directed_position),
    m_rise(rise)
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
quetzal::dungen::directed_point_type quetzal::dungen::Staircase::directed_position() const
{
    return m_directed_position;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Staircase::set_directed_position(directed_point_type directed_position)
{
    m_directed_position = directed_position;
    return;
}

//------------------------------------------------------------------------------
quetzal::dungen::point_type quetzal::dungen::Staircase::position() const
{
    return m_directed_position.position();
}

//------------------------------------------------------------------------------
quetzal::dungen::vector_type quetzal::dungen::Staircase::direction() const
{
    return m_directed_position.direction();
}

//------------------------------------------------------------------------------
quetzal::dungen::Rise quetzal::dungen::Staircase::rise() const
{
    return m_rise;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Staircase::print(ostream& os) const
{
    os << "Staircase " << m_id << "\t" << m_name << "\t" << to_string(m_rise)
        << "\tdirected_position (" << m_directed_position << ")" << endl;
    return;
}

//------------------------------------------------------------------------------
ostream& quetzal::dungen::operator<<(ostream& os, const Staircase& staircase)
{
    staircase.print(os);
    return os;
}
