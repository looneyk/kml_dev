//------------------------------------------------------------------------------
// dungen
// Level.cpp
//------------------------------------------------------------------------------

#include "Level.hpp"
#include <iostream>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::dungen::Level::Level(id_type id, const string& name, vector_type size, CellType cell_type) :
    m_id(id),
    m_name(name),
    m_cells(size, {cell_type}, 0),
    m_rooms(),
    m_doors(),
    m_staircases()
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
quetzal::dungen::vector_type quetzal::dungen::Level::size() const
{
    return m_cells.size();
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
const quetzal::dungen::Cell& quetzal::dungen::Level::cell(point_type position) const
{
    return m_cells.cell(position);
}

//------------------------------------------------------------------------------
quetzal::dungen::Cell& quetzal::dungen::Level::cell(point_type position)
{
    return m_cells.cell(position);
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
void quetzal::dungen::Level::create_room(const box_type& rectangle)
{
    id_type room_id = m_rooms.size();
    m_rooms.emplace_back(room_id, to_string(room_id), rectangle);
    expand_room(room_id, rectangle);
    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Level::expand_room(id_type room_id, const box_type& rectangle)
{
    auto [x0, y0] = rectangle.position();
    auto [nx, ny] = rectangle.extent();

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
void quetzal::dungen::Level::create_door(point_type position)
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
const quetzal::dungen::Level::staircases_type& quetzal::dungen::Level::staircases() const
{
    return m_staircases;
}

//------------------------------------------------------------------------------
quetzal::dungen::Level::staircases_type& quetzal::dungen::Level::staircases()
{
    return m_staircases;
}

//------------------------------------------------------------------------------
const quetzal::dungen::Staircase& quetzal::dungen::Level::staircase(id_type id) const
{
    assert(id < m_staircases.size());
    return m_staircases[id];
}

//------------------------------------------------------------------------------
quetzal::dungen::Staircase& quetzal::dungen::Level::staircase(id_type id)
{
    assert(id < m_staircases.size());
    return m_staircases[id];
}

//------------------------------------------------------------------------------
void quetzal::dungen::Level::create_staircase(point_type position, StaircaseDirection direction)
{
    Cell& c = cell(position);
    c.set_type(CellType::Stair);

    id_type staircase_id = m_staircases.size();
    m_staircases.emplace_back(staircase_id, to_string(staircase_id), position, direction);

    return;
}
//------------------------------------------------------------------------------
void quetzal::dungen::Level::render(ostream& os) const
{
    auto [nx, ny] = size();
    for (coord_type y = ny - 1; y >= 0; --y)
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
    os << "Level " << m_id << "\t" << m_name << "\tsize (" << size()[0] << ", " << size()[1] << ")" << endl;

    for (const auto& room : m_rooms)
    {
        os << room;
    }

    for (const auto& door : m_doors)
    {
        os << door;
    }

    for (const auto& staircase : m_staircases)
    {
        os << staircase;
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
