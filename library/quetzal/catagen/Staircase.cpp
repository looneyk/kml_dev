//------------------------------------------------------------------------------
// catagen
// Staircase.cpp
//------------------------------------------------------------------------------

#include "Staircase.hpp"
#include <iostream>

using namespace std;

//------------------------------------------------------------------------------
std::string quetzal::catagen::to_string(Rise rise)
{
    return rise == Rise::Up ? "up" : "down";
}

//------------------------------------------------------------------------------
quetzal::catagen::Staircase::Staircase(id_type id, const string& name, directed_point_type placement, Rise rise) :
    m_id(id),
    m_name(name),
    m_placement(placement),
    m_rise(rise)
{
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::catagen::Staircase::id() const
{
    return m_id;
}

//------------------------------------------------------------------------------
string quetzal::catagen::Staircase::name() const
{
    return m_name;
}

//------------------------------------------------------------------------------
quetzal::catagen::directed_point_type quetzal::catagen::Staircase::placement() const
{
    return m_placement;
}

//------------------------------------------------------------------------------
quetzal::catagen::Rise quetzal::catagen::Staircase::rise() const
{
    return m_rise;
}

//------------------------------------------------------------------------------
void quetzal::catagen::Staircase::print(ostream& os) const
{
    os << "Staircase " << m_id << "\t" << m_name << "\t" << to_string(m_rise)
        << "\tplacement (" << m_placement << ")" << endl;
    return;
}

//------------------------------------------------------------------------------
ostream& quetzal::catagen::operator<<(ostream& os, const Staircase& staircase)
{
    staircase.print(os);
    return os;
}
