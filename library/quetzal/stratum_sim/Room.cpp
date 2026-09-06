//------------------------------------------------------------------------------
// stratum_sim
// Room.cpp
//------------------------------------------------------------------------------

#include "Room.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include <algorithm>
#include <cassert>

namespace
{

    // A room's outline is horizontal, so containment is tested in two dimensions. The
    // outline vertices have a height that plays no part in it
    using projection_type = quetzal::stratum_sim::point_projection_type;
    using footprint_type = quetzal::geometry::Polygon<projection_type::projected_traits>;

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::Room::Room(const outline_type& outline) :
    m_outline(outline)
{
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Room::Room(const outline_type& outline, const cells_type& cells) :
    m_outline(outline),
    m_cells(cells)
{
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::Room::zone_id() const
{
    return m_zone_id;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Room::set_zone_id(id_type zone_id)
{
    m_zone_id = zone_id;
    return;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::Room::entity_id() const
{
    return m_entity_id;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Room::set_entity_id(id_type entity_id)
{
    m_entity_id = entity_id;
    return;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Room::outline_type& quetzal::stratum_sim::Room::outline() const
{
    return m_outline;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Room::outline_type& quetzal::stratum_sim::Room::outline()
{
    return m_outline;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Room::set_outline(const outline_type& outline)
{
    m_outline = outline;
    return;
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::Room::vertex_count() const
{
    return m_outline.size();
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Room::Vertex& quetzal::stratum_sim::Room::vertex(size_t n) const
{
    assert(n < m_outline.size());
    return m_outline[n];
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Room::cells_type& quetzal::stratum_sim::Room::cells() const
{
    return m_cells;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Room::cells_type& quetzal::stratum_sim::Room::cells()
{
    return m_cells;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Room::set_cells(const cells_type& cells)
{
    m_cells = cells;
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Room::add_cell(const GridPosition& cell)
{
    m_cells.push_back(cell);
    return;
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::Room::cell_count() const
{
    return m_cells.size();
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Room::contains(const GridPosition& cell) const
{
    return std::find(m_cells.begin(), m_cells.end(), cell) != m_cells.end();
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Room::contains(const point_type& position) const
{
    if (m_outline.size() < 3)
    {
        return false;
    }

    static const projection_type project;

    std::vector<projection_type::projected_type> points;
    points.reserve(m_outline.size());

    for (const Vertex& v : m_outline)
    {
        points.push_back(project(v.position));
    }

    return footprint_type(points.begin(), points.end()).contains(project(position));
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Room::clear()
{
    m_zone_id = nullid;
    m_entity_id = nullid;
    m_outline.clear();
    m_cells.clear();

    return;
}
