//------------------------------------------------------------------------------
// dungen
// Level.cpp
//------------------------------------------------------------------------------

#include "Level.hpp"
#include <iostream>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::dungen::Level::Level(id_type id, const string& name, dimensions_type size, CellType cell_type) :
    m_id(id),
    m_name(name),
    m_size{size},
    m_cells(size, {cell_type}, 0),
    m_rooms(),
    m_doors(),
    m_shafts()
{
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::dungen::Level::id() const
{
    return m_id;
}

//------------------------------------------------------------------------------
string quetzal::dungen::Level::name() const
{
    return m_name;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Level::set_name(const string& name)
{
    m_name = name;
    return;
}

//------------------------------------------------------------------------------
quetzal::dungen::dimensions_type quetzal::dungen::Level::size() const
{
    return m_size;
}

//------------------------------------------------------------------------------
const quetzal::dungen::Cells& quetzal::dungen::Level::cells() const
{
    return m_cells;
}

//------------------------------------------------------------------------------
quetzal::dungen::Cells& quetzal::dungen::Level::cells()
{
    return m_cells;
}

//------------------------------------------------------------------------------
const quetzal::dungen::Cell& quetzal::dungen::Level::cell(position_type position) const
{
    return m_cells.cell(position);
}

//------------------------------------------------------------------------------
quetzal::dungen::Cell& quetzal::dungen::Level::cell(position_type position)
{
    return m_cells.cell(position);
}

//------------------------------------------------------------------------------
const quetzal::dungen::Cell& quetzal::dungen::Level::cell(coord_type x, coord_type y) const
{
    return m_cells.cell({x, y});
}

//------------------------------------------------------------------------------
quetzal::dungen::Cell& quetzal::dungen::Level::cell(coord_type x, coord_type y)
{
    return m_cells.cell({x, y});
}

//------------------------------------------------------------------------------
const quetzal::dungen::Level::rooms_type& quetzal::dungen::Level::rooms() const
{
    return m_rooms;
}

//------------------------------------------------------------------------------
quetzal::dungen::Level::rooms_type& quetzal::dungen::Level::rooms()
{
    return m_rooms;
}

//------------------------------------------------------------------------------
const quetzal::dungen::Room& quetzal::dungen::Level::room(id_type id) const
{
    assert(id < m_rooms.size());
    return m_rooms[id];
}

//------------------------------------------------------------------------------
quetzal::dungen::Room& quetzal::dungen::Level::room(id_type id)
{
    assert(id < m_rooms.size());
    return m_rooms[id];
}

//------------------------------------------------------------------------------
void quetzal::dungen::Level::create_room(position_type position, dimensions_type size)
{
    id_type room_id = m_rooms.size();
    m_rooms.emplace_back(room_id, to_string(room_id), position, size);
    expand_room(room_id, position, size);
    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Level::expand_room(id_type room_id, position_type position, dimensions_type size)
{
    auto [x0, y0] = position;
    auto [nx, ny] = size;

    for (coord_type x = x0; x < x0 + nx; ++x)
    {
        for (coord_type y = y0; y < y0 + ny; ++y)
        {
            Cell& c = cell({x, y});
            c.set_type(CellType::Room);
            c.set_room_id(room_id);
        }
    }

    return;
}

//------------------------------------------------------------------------------
const quetzal::dungen::Level::doors_type& quetzal::dungen::Level::doors() const
{
    return m_doors;
}

//------------------------------------------------------------------------------
quetzal::dungen::Level::doors_type& quetzal::dungen::Level::doors()
{
    return m_doors;
}

//------------------------------------------------------------------------------
const quetzal::dungen::Door& quetzal::dungen::Level::door(id_type id) const
{
    assert(id < m_doors.size());
    return m_doors[id];
}

//------------------------------------------------------------------------------
quetzal::dungen::Door& quetzal::dungen::Level::door(id_type id)
{
    assert(id < m_doors.size());
    return m_doors[id];
}

//------------------------------------------------------------------------------
void quetzal::dungen::Level::create_door(position_type position)
{
    Cell& c = cell(position);
    c.set_type(CellType::Door);

    id_type door_id = m_doors.size();
    m_doors.emplace_back(door_id, to_string(door_id), position);

    // Add door to room(s)
    // Add room(s) to door

    return;
}

//------------------------------------------------------------------------------
const quetzal::dungen::Level::shafts_type& quetzal::dungen::Level::shafts() const
{
    return m_shafts;
}

//------------------------------------------------------------------------------
quetzal::dungen::Level::shafts_type& quetzal::dungen::Level::shafts()
{
    return m_shafts;
}

//------------------------------------------------------------------------------
const quetzal::dungen::Shaft& quetzal::dungen::Level::shaft(id_type id) const
{
    assert(id < m_shafts.size());
    return m_shafts[id];
}

//------------------------------------------------------------------------------
quetzal::dungen::Shaft& quetzal::dungen::Level::shaft(id_type id)
{
    assert(id < m_shafts.size());
    return m_shafts[id];
}

//------------------------------------------------------------------------------
void quetzal::dungen::Level::create_shaft(position_type position)
{
    Cell& c = cell(position);
    c.set_type(CellType::Shaft);

    id_type shaft_id = m_shafts.size();
    m_shafts.emplace_back(shaft_id, to_string(shaft_id), position);

    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Level::render(ostream& os) const
{
    auto [nx, ny] = size();
    for (coord_type y = 0; y < ny; ++y)
    {
        for (coord_type x = 0; x < nx; ++x)
        {
            cell({x, y}).render(os);
        }

        os << endl;
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Level::print(ostream& os) const
{
    os << "Level " << m_id << "\t" << m_name << "\tsize (" << m_size[0] << ", " << m_size[1] << ")" << endl;

    for (const auto& room : m_rooms)
    {
        os << room;
    }

    for (const auto& door : m_doors)
    {
        os << door;
    }

    for (const auto& shaft : m_shafts)
    {
        os << shaft;
    }

    render(os);
    return;
}

//------------------------------------------------------------------------------
ostream& quetzal::dungen::operator<<(ostream& os, const Level& level)
{
    level.print(os);
    return os;
}
