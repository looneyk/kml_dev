//------------------------------------------------------------------------------
// dungen
// Zone.cpp
//------------------------------------------------------------------------------

#include "Zone.hpp"
#include "quetzal/common/string_util.hpp"
#include <algorithm>
#include <iostream>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::dungen::Zone::Zone(id_type id, const string& name, vector_type extent, CellType cell_type) :
    m_id(id),
    m_name(name),
    m_cells(extent, {cell_type}, 0),
    m_rooms(),
    m_doors(),
    m_transit_endpoints()
{
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::dungen::Zone::id() const
{
    return m_id;
}

//------------------------------------------------------------------------------
string quetzal::dungen::Zone::name() const
{
    return m_name;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Zone::set_name(const string& name)
{
    m_name = name;
    return;
}

//------------------------------------------------------------------------------
quetzal::dungen::vector_type quetzal::dungen::Zone::extent() const
{
    return m_cells.size();
}

//------------------------------------------------------------------------------
const quetzal::dungen::Cells& quetzal::dungen::Zone::cells() const
{
    return m_cells;
}

//------------------------------------------------------------------------------
quetzal::dungen::Cells& quetzal::dungen::Zone::cells()
{
    return m_cells;
}

//------------------------------------------------------------------------------
const quetzal::dungen::Cell& quetzal::dungen::Zone::cell(point_type position) const
{
    return m_cells.cell(position);
}

//------------------------------------------------------------------------------
quetzal::dungen::Cell& quetzal::dungen::Zone::cell(point_type position)
{
    return m_cells.cell(position);
}

//------------------------------------------------------------------------------
const quetzal::dungen::Zone::rooms_type& quetzal::dungen::Zone::rooms() const
{
    return m_rooms;
}

//------------------------------------------------------------------------------
quetzal::dungen::Zone::rooms_type& quetzal::dungen::Zone::rooms()
{
    return m_rooms;
}

//------------------------------------------------------------------------------
const quetzal::dungen::Room& quetzal::dungen::Zone::room(id_type id) const
{
    assert(id < m_rooms.size());
    return m_rooms[id];
}

//------------------------------------------------------------------------------
quetzal::dungen::Room& quetzal::dungen::Zone::room(id_type id)
{
    assert(id < m_rooms.size());
    return m_rooms[id];
}

//------------------------------------------------------------------------------
void quetzal::dungen::Zone::create_room(const box_type& rectangle)
{
    id_type room_id = m_rooms.size();
    m_rooms.emplace_back(room_id, std::to_string(room_id), rectangle);
    expand_room(room_id, rectangle);
    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Zone::create_room(const std::vector<point_type>& cell_positions)
{
    assert(!cell_positions.empty());

    point_type lower = cell_positions.front();
    point_type upper = cell_positions.front();

    for (const auto& position : cell_positions)
    {
        lower = {std::min(lower.x(), position.x()), std::min(lower.y(), position.y())};
        upper = {std::max(upper.x(), position.x()), std::max(upper.y(), position.y())};
    }

    id_type room_id = m_rooms.size();
    m_rooms.emplace_back(room_id, std::to_string(room_id), box_type(lower, upper + vector_type{1, 1}));

    for (const auto& position : cell_positions)
    {
        Cell& c = cell(position);
        c.set_type(CellType::Room);
        c.set_room_id(room_id);
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Zone::expand_room(id_type room_id, const box_type& rectangle)
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
const quetzal::dungen::Zone::doors_type& quetzal::dungen::Zone::doors() const
{
    return m_doors;
}

//------------------------------------------------------------------------------
quetzal::dungen::Zone::doors_type& quetzal::dungen::Zone::doors()
{
    return m_doors;
}

//------------------------------------------------------------------------------
const quetzal::dungen::Door& quetzal::dungen::Zone::door(id_type id) const
{
    assert(id < m_doors.size());
    return m_doors[id];
}

//------------------------------------------------------------------------------
quetzal::dungen::Door& quetzal::dungen::Zone::door(id_type id)
{
    assert(id < m_doors.size());
    return m_doors[id];
}

//------------------------------------------------------------------------------
void quetzal::dungen::Zone::create_door(point_type position)
{
    Cell& c = cell(position);
    c.set_type(CellType::Door);

    id_type door_id = m_doors.size();
    m_doors.emplace_back(door_id, std::to_string(door_id), position);

    // Add door to room(s)
    // Add room(s) to door

    return;
}

//------------------------------------------------------------------------------
const quetzal::dungen::Zone::transit_endpoints_type& quetzal::dungen::Zone::transit_endpoints() const
{
    return m_transit_endpoints;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Zone::create_staircase(const directed_point_type& placement, stratum_core::Directionality directionality)
{
    cell(placement.position()).set_type(CellType::Staircase);

    const vector_type egress_direction = placement.direction();
    m_transit_endpoints.push_back({.id = m_transit_endpoints.size(), .type = "staircase", .cell_position = placement.position(),
        .orientation = -egress_direction, .egress_direction = egress_direction, .directionality = directionality});

    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Zone::render(ostream& os) const
{
    auto [nx, ny] = extent();
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
void quetzal::dungen::Zone::print(ostream& os) const
{
    os << "Zone " << m_id << "\t" << m_name << "\textent (" << extent()[0] << ", " << extent()[1] << ")" << endl;

    for (const auto& room : m_rooms)
    {
        os << room;
    }

    for (const auto& door : m_doors)
    {
        os << door;
    }

    for (const auto& endpoint : m_transit_endpoints)
    {
        os << endpoint;
    }

    render(os);
    return;
}

//------------------------------------------------------------------------------
ostream& quetzal::dungen::operator<<(ostream& os, const Zone& zone)
{
    zone.print(os);
    return os;
}
