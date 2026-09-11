//------------------------------------------------------------------------------
// catagen
// Level.cpp
//------------------------------------------------------------------------------

#include "Level.hpp"
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::catagen::Level::Level(const vector_type& extent) :
    m_bounds(point_type {0, 0}, extent),
    m_cells(static_cast<size_t>(extent.i()) * extent.j(), CellType::Solid),
    m_walls_vertical(static_cast<size_t>(extent.i() + 1) * extent.j(), WallType::Solid),
    m_walls_horizontal(static_cast<size_t>(extent.i()) * (extent.j() + 1), WallType::Solid)
{
    assert(extent.i() > 0 && extent.j() > 0);
}

//------------------------------------------------------------------------------
const quetzal::catagen::vector_type& quetzal::catagen::Level::extent() const
{
    return m_bounds.extent();
}

//------------------------------------------------------------------------------
bool quetzal::catagen::Level::contains(const point_type& position) const
{
    return m_bounds.contains(position);
}

//------------------------------------------------------------------------------
quetzal::catagen::CellType quetzal::catagen::Level::cell(const point_type& position) const
{
    return m_cells[cell_slot(position)];
}

//------------------------------------------------------------------------------
void quetzal::catagen::Level::set_cell(const point_type& position, CellType type)
{
    m_cells[cell_slot(position)] = type;
    return;
}

//------------------------------------------------------------------------------
quetzal::catagen::WallType quetzal::catagen::Level::wall_vertical(coord_type i, coord_type j) const
{
    return m_walls_vertical[wall_vertical_slot(i, j)];
}

//------------------------------------------------------------------------------
void quetzal::catagen::Level::set_wall_vertical(coord_type i, coord_type j, WallType type)
{
    m_walls_vertical[wall_vertical_slot(i, j)] = type;
    return;
}

//------------------------------------------------------------------------------
quetzal::catagen::WallType quetzal::catagen::Level::wall_horizontal(coord_type i, coord_type j) const
{
    return m_walls_horizontal[wall_horizontal_slot(i, j)];
}

//------------------------------------------------------------------------------
void quetzal::catagen::Level::set_wall_horizontal(coord_type i, coord_type j, WallType type)
{
    m_walls_horizontal[wall_horizontal_slot(i, j)] = type;
    return;
}

//------------------------------------------------------------------------------
quetzal::catagen::WallType quetzal::catagen::Level::wall(const point_type& position, CardinalDirection direction) const
{
    switch (direction)
    {
        case CardinalDirection::East: return wall_vertical(position.i() + 1, position.j());
        case CardinalDirection::West: return wall_vertical(position.i(), position.j());
        case CardinalDirection::North: return wall_horizontal(position.i(), position.j() + 1);
        case CardinalDirection::South: return wall_horizontal(position.i(), position.j());
    }

    assert(false);
    return WallType::Solid;
}

//------------------------------------------------------------------------------
void quetzal::catagen::Level::set_wall(const point_type& position, CardinalDirection direction, WallType type)
{
    switch (direction)
    {
        case CardinalDirection::East: set_wall_vertical(position.i() + 1, position.j(), type); return;
        case CardinalDirection::West: set_wall_vertical(position.i(), position.j(), type); return;
        case CardinalDirection::North: set_wall_horizontal(position.i(), position.j() + 1, type); return;
        case CardinalDirection::South: set_wall_horizontal(position.i(), position.j(), type); return;
    }

    assert(false);
    return;
}

//------------------------------------------------------------------------------
void quetzal::catagen::Level::clear()
{
    m_bounds.clear();
    m_cells.clear();
    m_walls_vertical.clear();
    m_walls_horizontal.clear();
    return;
}

//------------------------------------------------------------------------------
size_t quetzal::catagen::Level::cell_slot(const point_type& position) const
{
    assert(contains(position));
    return static_cast<size_t>(position.j()) * m_bounds.extent().i() + position.i();
}

//------------------------------------------------------------------------------
size_t quetzal::catagen::Level::wall_vertical_slot(coord_type i, coord_type j) const
{
    assert(i >= 0 && i <= m_bounds.extent().i() && j >= 0 && j < m_bounds.extent().j());
    return static_cast<size_t>(j) * (m_bounds.extent().i() + 1) + i;
}

//------------------------------------------------------------------------------
size_t quetzal::catagen::Level::wall_horizontal_slot(coord_type i, coord_type j) const
{
    assert(i >= 0 && i < m_bounds.extent().i() && j >= 0 && j <= m_bounds.extent().j());
    return static_cast<size_t>(j) * m_bounds.extent().i() + i;
}

//------------------------------------------------------------------------------
quetzal::catagen::point_type quetzal::catagen::neighbor(const point_type& position, CardinalDirection direction)
{
    return position + cardinal_direction(direction);
}
