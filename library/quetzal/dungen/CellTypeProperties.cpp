//------------------------------------------------------------------------------
// dungen
// CellTypeProperties.cpp
//------------------------------------------------------------------------------

#include "CellTypeProperties.hpp"
#include <iostream>

using namespace std;

//------------------------------------------------------------------------------
quetzal::dungen::CellTypeProperties::CellTypeProperties(CellType type, const string& name, const string& symbol) :
    m_type(type),
    m_name(name),
    m_symbol(symbol)
{
}

//------------------------------------------------------------------------------
quetzal::dungen::CellType quetzal::dungen::CellTypeProperties::type() const
{
    return m_type;
}

//------------------------------------------------------------------------------
string quetzal::dungen::CellTypeProperties::name() const
{
    return m_name;
}

//------------------------------------------------------------------------------
string quetzal::dungen::CellTypeProperties::symbol() const
{
    return m_symbol;
}

//------------------------------------------------------------------------------
void quetzal::dungen::CellTypeProperties::print(ostream& os) const
{
    os << type() << "\t" << name() << "\t" << symbol();
    return;
}

//------------------------------------------------------------------------------
ostream& quetzal::dungen::operator<<(ostream& os, const CellTypeProperties& properties)
{
    properties.print(os);
    return os;
}

//------------------------------------------------------------------------------
unordered_map<quetzal::dungen::CellType, quetzal::dungen::CellTypeProperties> quetzal::dungen::cell_types
{
    {CellType::Null, {CellType::Null, "Null", "0"}},
    {CellType::Solid, {CellType::Solid, "Solid", "#"}},
    {CellType::Room, {CellType::Room, "Room", " "}},
    {CellType::Corridor, {CellType::Corridor, "Corridor", "."}},
    {CellType::Door, {CellType::Door, "Door", "|"}},
    {CellType::Staircase, {CellType::Staircase, "Staircase", ">"}},
};
