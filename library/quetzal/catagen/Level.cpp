//------------------------------------------------------------------------------
// catagen
// Level.cpp
//------------------------------------------------------------------------------

#include "Level.hpp"
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::catagen::Level::Level(const Extent& extent) :
    m_extent(extent),
    m_cells(static_cast<size_t>(extent.nx) * extent.ny, CellType::Solid),
    m_walls_vertical(static_cast<size_t>(extent.nx + 1) * extent.ny, WallType::Solid),
    m_walls_horizontal(static_cast<size_t>(extent.nx) * (extent.ny + 1), WallType::Solid)
{
    assert(extent.nx > 0 && extent.ny > 0);
}

//------------------------------------------------------------------------------
const quetzal::catagen::Extent& quetzal::catagen::Level::extent() const
{
    return m_extent;
}

//------------------------------------------------------------------------------
bool quetzal::catagen::Level::contains(const CellPosition& position) const
{
    return position.i >= 0 && position.j >= 0 && position.i < m_extent.nx && position.j < m_extent.ny;
}

//------------------------------------------------------------------------------
quetzal::catagen::CellType quetzal::catagen::Level::cell(const CellPosition& position) const
{
    return m_cells[cell_slot(position)];
}

//------------------------------------------------------------------------------
void quetzal::catagen::Level::set_cell(const CellPosition& position, CellType type)
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
quetzal::catagen::WallType quetzal::catagen::Level::wall(const CellPosition& position, Direction direction) const
{
    switch (direction)
    {
        case Direction::East: return wall_vertical(position.i + 1, position.j);
        case Direction::West: return wall_vertical(position.i, position.j);
        case Direction::North: return wall_horizontal(position.i, position.j + 1);
        case Direction::South: return wall_horizontal(position.i, position.j);
    }

    assert(false);
    return WallType::Solid;
}

//------------------------------------------------------------------------------
void quetzal::catagen::Level::set_wall(const CellPosition& position, Direction direction, WallType type)
{
    switch (direction)
    {
        case Direction::East: set_wall_vertical(position.i + 1, position.j, type); return;
        case Direction::West: set_wall_vertical(position.i, position.j, type); return;
        case Direction::North: set_wall_horizontal(position.i, position.j + 1, type); return;
        case Direction::South: set_wall_horizontal(position.i, position.j, type); return;
    }

    assert(false);
    return;
}

//------------------------------------------------------------------------------
void quetzal::catagen::Level::clear()
{
    m_extent = {};
    m_cells.clear();
    m_walls_vertical.clear();
    m_walls_horizontal.clear();
    return;
}

//------------------------------------------------------------------------------
size_t quetzal::catagen::Level::cell_slot(const CellPosition& position) const
{
    assert(contains(position));
    return static_cast<size_t>(position.j) * m_extent.nx + position.i;
}

//------------------------------------------------------------------------------
size_t quetzal::catagen::Level::wall_vertical_slot(coord_type i, coord_type j) const
{
    assert(i >= 0 && i <= m_extent.nx && j >= 0 && j < m_extent.ny);
    return static_cast<size_t>(j) * (m_extent.nx + 1) + i;
}

//------------------------------------------------------------------------------
size_t quetzal::catagen::Level::wall_horizontal_slot(coord_type i, coord_type j) const
{
    assert(i >= 0 && i < m_extent.nx && j >= 0 && j <= m_extent.ny);
    return static_cast<size_t>(j) * m_extent.nx + i;
}

//------------------------------------------------------------------------------
quetzal::catagen::CellPosition quetzal::catagen::neighbor(const CellPosition& position, Direction direction)
{
    switch (direction)
    {
        case Direction::East: return {position.i + 1, position.j};
        case Direction::North: return {position.i, position.j + 1};
        case Direction::West: return {position.i - 1, position.j};
        case Direction::South: return {position.i, position.j - 1};
    }

    assert(false);
    return position;
}

//------------------------------------------------------------------------------
quetzal::catagen::Direction quetzal::catagen::opposite(Direction direction)
{
    switch (direction)
    {
        case Direction::East: return Direction::West;
        case Direction::North: return Direction::South;
        case Direction::West: return Direction::East;
        case Direction::South: return Direction::North;
    }

    assert(false);
    return direction;
}
