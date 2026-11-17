//------------------------------------------------------------------------------
// stratum_sim
// DiscreteLayout.cpp
//------------------------------------------------------------------------------

#include "DiscreteLayout.hpp"
#include "DiscreteProximityIndex.hpp"
#include "quetzal/math/floating_point.hpp"
#include <cassert>
#include <cmath>
#include <limits>
#include <optional>
#include <variant>

namespace
{

    //--------------------------------------------------------------------------
    // The direction's position in cardinal_directions, which is its bit in a mask and its part of a wall id
    size_t direction_index(const quetzal::stratum_sim::discrete_vector_type& direction)
    {
        const auto& directions = quetzal::stratum_sim::cardinal_directions;

        for (size_t index = 0; index < directions.size(); ++index)
        {
            if (directions[index] == direction)
            {
                return index;
            }
        }

        assert(false); // Not a cardinal direction
        return 0;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::DiscreteLayout::DiscreteLayout(const point_type& origin, value_type scale, const discrete_vector_type& extent) :
    LayoutBase(),
    m_grid{origin, scale, extent},
    m_cells(static_cast<size_t>(extent.x()) * extent.y())
{
    assert(scale > value_type(0));
    assert(extent.x() >= 0 && extent.y() >= 0);
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Flat& quetzal::stratum_sim::DiscreteLayout::flat(id_type flat_id) const
{
    assert(flat_id < m_flat_store.size());
    return m_flat_store[flat_id];
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Flat& quetzal::stratum_sim::DiscreteLayout::flat(id_type flat_id)
{
    assert(flat_id < m_flat_store.size());
    return m_flat_store[flat_id];
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::DiscreteLayout::flat_store_count() const
{
    return m_flat_store.size();
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::DiscreteLayout::flats_type& quetzal::stratum_sim::DiscreteLayout::flat_store() const
{
    return m_flat_store;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::DiscreteLayout::floor_id() const
{
    return m_floor_id;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::DiscreteLayout::ceiling_id() const
{
    return m_ceiling_id;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::DiscreteLayout::set_flats(value_type height_floor, value_type height_ceiling)
{
    assert(m_floor_id == nullid); // Set once

    m_floor_id = m_flat_store.size();
    m_flat_store.emplace_back(height_floor, false);

    m_ceiling_id = m_flat_store.size();
    m_flat_store.emplace_back(height_ceiling, true);

    return;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::value_type quetzal::stratum_sim::DiscreteLayout::height(id_type flat_id, const discrete_point_type& cell_position) const
{
    const Flat& f = flat(flat_id);

    if (std::optional<value_type> h = f.cell_height(cell_position); h.has_value())
    {
        return *h;
    }

    if (id_type room_id = room_at(cell_position); room_id != nullid)
    {
        if (std::optional<value_type> h = f.room_height(room_id); h.has_value())
        {
            return *h;
        }
    }

    return f.height();
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::DiscreteLayout::room_count() const
{
    return m_room_count;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::DiscreteLayout::set_room_count(size_t room_count)
{
    m_room_count = room_count;
    return;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::DiscreteLayout::room_at(const discrete_point_type& cell_position) const
{
    return cell(cell_position).room_id;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::DiscreteLayout::set_cell_room(const discrete_point_type& cell_position, id_type room_id)
{
    cell(cell_position).room_id = room_id;
    return;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::DiscreteLayout::open(const discrete_point_type& cell_position) const
{
    return cell(cell_position).open;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::DiscreteLayout::set_cell_open(const discrete_point_type& cell_position, bool open)
{
    cell(cell_position).open = open;
    return;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::DirectionMask quetzal::stratum_sim::DiscreteLayout::walls(const discrete_point_type& cell_position) const
{
    return cell(cell_position).walls;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::DiscreteLayout::set_cell_walls(const discrete_point_type& cell_position, DirectionMask walls)
{
    cell(cell_position).walls = walls;
    return;
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::DiscreteLayout::slot(const discrete_point_type& cell_position) const
{
    const GridMapping& g = grid();
    assert(g.contains(cell_position));

    return static_cast<size_t>(cell_position.j()) * g.extent.x() + cell_position.i();
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Cell& quetzal::stratum_sim::DiscreteLayout::cell(const discrete_point_type& cell_position) const
{
    return m_cells[slot(cell_position)];
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Cell& quetzal::stratum_sim::DiscreteLayout::cell(const discrete_point_type& cell_position)
{
    return m_cells[slot(cell_position)];
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::GridMapping& quetzal::stratum_sim::DiscreteLayout::grid() const
{
    return m_grid;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::value_type quetzal::stratum_sim::DiscreteLayout::step_distance() const
{
    return grid().scale;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::discrete_point_type quetzal::stratum_sim::DiscreteLayout::cell_position_at(const point_type& point) const
{
    return grid().cell_position_at(point);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::point_type quetzal::stratum_sim::DiscreteLayout::position(const discrete_point_type& cell_position) const
{
    point_type result = grid().position(cell_position);

    // A layout under construction has no floor yet
    if (m_floor_id != nullid)
    {
        result.set_z(height(m_floor_id, cell_position));
    }

    return result;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::DiscreteLayout::wall_toward(const discrete_point_type& cell_position, const discrete_vector_type& direction) const
{
    size_t index = direction_index(direction);
    if (!any(cell(cell_position).walls & direction_mask(static_cast<CardinalDirection>(index))))
    {
        return nullid;
    }

    return slot(cell_position) * cardinal_directions.size() + index;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::discrete_point_type quetzal::stratum_sim::DiscreteLayout::wall_cell_position(id_type wall_id) const
{
    assert(wall_id != nullid);

    size_t cell_slot = wall_id / cardinal_directions.size();
    auto width = static_cast<size_t>(grid().extent.x());

    return discrete_point_type{static_cast<coord_type>(cell_slot % width), static_cast<coord_type>(cell_slot / width)};
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::discrete_vector_type& quetzal::stratum_sim::DiscreteLayout::wall_direction(id_type wall_id) const
{
    assert(wall_id != nullid);
    return cardinal_directions[wall_id % cardinal_directions.size()];
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::placement_type quetzal::stratum_sim::DiscreteLayout::arrival_placement(const Site& site) const
{
    assert(site.relation == Relation::On && site.address.has_value());

    if (const auto* wall = std::get_if<WallId>(&*site.address))
    {
        const discrete_vector_type& toward_wall = wall_direction(wall->wall_id);
        return placement_type(position(wall_cell_position(wall->wall_id)),
            vector_type(static_cast<value_type>(-toward_wall.x()), static_cast<value_type>(-toward_wall.y()), value_type(0)));
    }

    return placement_type(position(std::get<FlatCellPosition>(*site.address).cell_position), site.placement.direction());
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::DiscreteLayout::crossable(const discrete_point_type& cell_position, const discrete_vector_type& direction,
    const crossable_function_type& is_wall_crossable) const
{
    id_type wall_id = wall_toward(cell_position, direction);

    if (wall_id == nullid)
    {
        assert(grid().contains(cell_position + direction)); // Open floor is enclosed by walls
        return true;
    }

    return is_wall_crossable && is_wall_crossable(wall_id);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::MoveResult quetzal::stratum_sim::DiscreteLayout::move(const placement_type& placement, const vector_type& direction, value_type distance,
    const crossable_function_type& is_wall_crossable, const blocker_function_type& blocker_at) const
{
    MoveResult result{placement, nullid};

    discrete_vector_type step = grid_step(direction);
    if (step.zero())
    {
        return result;
    }

    const GridMapping& g = grid();
    int steps = static_cast<int>(std::lround(distance / g.scale));

    discrete_point_type cell = cell_position_at(placement.position());

    for (int n = 0; n < steps; ++n)
    {
        discrete_point_type next = cell + step;

        if (!crossable(cell, step, is_wall_crossable))
        {
            result.blocking_wall_id = wall_toward(cell, step);
            break;
        }

        assert(open(next));

        if (blocker_at)
        {
            point_type p = position(next);
            placement_type candidate_placement(point_type(p.x(), p.y(), placement.position().z()), placement.direction());

            id_type blocker = blocker_at(candidate_placement);
            if (blocker != nullid)
            {
                result.blocking_entity_id = blocker;
                break;
            }
        }

        cell = next;
    }

    point_type p = position(cell);
    result.placement.set_position(point_type(p.x(), p.y(), placement.position().z()));

    return result;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::placement_type quetzal::stratum_sim::DiscreteLayout::turn(const placement_type& placement, Rotation rotation) const
{
    placement_type result = placement;

    auto [x, y, z] = placement.direction().rep();

    switch (rotation)
    {
        case Rotation::CCW:
            result.set_direction(vector_type(-y, x, z));
            break;

        case Rotation::CW:
            result.set_direction(vector_type(y, -x, z));
            break;

        case Rotation::Half:
            result.set_direction(vector_type(-x, -y, z));
            break;
    }

    return result;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::placement_type quetzal::stratum_sim::DiscreteLayout::face(const placement_type& placement, const vector_type& direction) const
{
    // assert that the direction is not a zero vector ...

    placement_type result = placement;

    auto [dx, dy, dz] = direction.rep();

    if (math::float_eq0(dx) && math::float_eq0(dy))
    {
        return result;
    }

    result.set_direction(std::abs(dx) >= std::abs(dy)
        ? vector_type(dx > value_type(0) ? value_type(1) : value_type(-1), value_type(0), value_type(0))
        : vector_type(value_type(0), dy > value_type(0) ? value_type(1) : value_type(-1), value_type(0)));

    return result;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::DiscreteLayout::same_location(const placement_type& a, const placement_type& b) const
{
    return cell_position_at(a.position()) == cell_position_at(b.position());
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::DiscreteLayout::placements_type quetzal::stratum_sim::DiscreteLayout::neighbors(const placement_type& placement,
    const crossable_function_type& is_wall_crossable) const
{
    placements_type result;
    discrete_point_type cell_position = cell_position_at(placement.position());

    const auto& steps = cardinal_directions;

    for (const discrete_vector_type& step : steps)
    {
        if (!crossable(cell_position, step, is_wall_crossable))
        {
            continue;
        }

        discrete_point_type next = cell_position + step;
        assert(open(next));

        point_type p = position(next);
        result.push_back(placement_type(point_type(p.x(), p.y(), placement.position().z()), placement.direction()));
    }

    return result;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::DiscreteLayout::visible(const placement_type& from, const placement_type& to, const crossable_function_type& is_wall_crossable) const
{
    discrete_point_type cell_position = cell_position_at(from.position());
    discrete_point_type target = cell_position_at(to.position());

    if (cell_position == target)
    {
        return true;
    }

    const GridMapping& g = grid();

    auto [fx, fy, fz] = from.position().rep();
    auto [ox, oy, oz] = g.origin.rep();
    auto [dx, dy, dz] = (to.position() - from.position()).rep();

    int step_i = dx > value_type(0) ? 1 : (dx < value_type(0) ? -1 : 0);
    int step_j = dy > value_type(0) ? 1 : (dy < value_type(0) ? -1 : 0);

    // Distance along the ray to the next cell boundary on each axis, and between successive boundaries
    constexpr value_type far = std::numeric_limits<value_type>::max();

    auto boundary = [&](int index, int step, value_type origin, value_type position, value_type d)
    {
        if (step == 0)
        {
            return far;
        }

        value_type edge = origin + static_cast<value_type>(step > 0 ? index + 1 : index) * g.scale;
        return (edge - position) / d;
    };

    value_type t_i = boundary(cell_position.i(), step_i, ox, fx, dx);
    value_type t_j = boundary(cell_position.j(), step_j, oy, fy, dy);

    value_type delta_i = step_i == 0 ? far : g.scale / std::abs(dx);
    value_type delta_j = step_j == 0 ? far : g.scale / std::abs(dy);

    // One step per cell edge crossed, so the walk cannot exceed the grid
    size_t limit = static_cast<size_t>(g.extent.x() + g.extent.y()) + 2;

    for (size_t n = 0; n < limit; ++n)
    {
        discrete_vector_type step = t_i < t_j ? discrete_vector_type(step_i, 0) : discrete_vector_type(0, step_j);

        if (!crossable(cell_position, step, is_wall_crossable))
        {
            return false;
        }

        discrete_point_type next = cell_position + step;
        assert(open(next));

        if (t_i < t_j)
        {
            t_i += delta_i;
        }
        else
        {
            t_j += delta_j;
        }

        cell_position = next;

        if (cell_position == target)
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::DiscreteLayout::ahead(const placement_type& placement) const
{
    discrete_vector_type step = grid_step(placement.direction());
    if (step.zero())
    {
        return nullid;
    }

    return wall_toward(cell_position_at(placement.position()), step);
}

//------------------------------------------------------------------------------
std::unique_ptr<quetzal::stratum_sim::ProximityIndexBase> quetzal::stratum_sim::DiscreteLayout::create_proximity_index() const
{
    return std::make_unique<DiscreteProximityIndex>(*this);
}

