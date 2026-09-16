//------------------------------------------------------------------------------
// dungen
// Cell.cpp
//------------------------------------------------------------------------------

#include "Cell.hpp"
#include "CellType.hpp"
#include "CellTypeProperties.hpp"
#include <iostream>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::dungen::Cell::Cell(CellType type, id_type room_id) :
    m_type(type),
    m_room_id(room_id),
    m_walls()
{
}

//------------------------------------------------------------------------------
quetzal::dungen::CellType quetzal::dungen::Cell::type() const
{
    return m_type;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Cell::set_type(CellType type)
{
    m_type = type;
    return;
}

//------------------------------------------------------------------------------
const quetzal::dungen::CellTypeProperties& quetzal::dungen::Cell::properties() const
{
    return cell_types[m_type];
}

//------------------------------------------------------------------------------
quetzal::dungen::CellTypeProperties& quetzal::dungen::Cell::properties()
{
    return cell_types[m_type];
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::dungen::Cell::room_id() const
{
    assert(m_type == CellType::Room || m_type == CellType::Stair);
    return m_room_id;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Cell::set_room_id(id_type room_id)
{
    m_room_id = room_id;
    return;
}

//------------------------------------------------------------------------------
const quetzal::dungen::Wall& quetzal::dungen::Cell::wall(size_t i) const
{
    assert(i >= 0 && i < 4);
    return m_walls[i];
}

//------------------------------------------------------------------------------
quetzal::dungen::Wall& quetzal::dungen::Cell::wall(size_t i)
{
    assert(i >= 0 && i < 4);
    return m_walls[i];
}

//------------------------------------------------------------------------------
void quetzal::dungen::Cell::set_wall([[maybe_unused]] const Wall& wall)
{
    // ...
    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Cell::render(ostream& os) const
{
    os << properties().symbol();
}

//------------------------------------------------------------------------------
void quetzal::dungen::Cell::print(ostream& os) const
{
    os << properties().symbol();
}

//------------------------------------------------------------------------------
ostream& quetzal::dungen::operator<<(ostream& os, const Cell& cell)
{
    cell.print(os);
    return os;
}
