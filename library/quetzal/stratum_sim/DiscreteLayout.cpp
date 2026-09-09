//------------------------------------------------------------------------------
// stratum_sim
// DiscreteLayout.cpp
//------------------------------------------------------------------------------

#include "DiscreteLayout.hpp"
#include "quetzal/math/floating_point.hpp"
#include <cassert>
#include <cmath>
#include <limits>

namespace
{

    //--------------------------------------------------------------------------
    bool coincident(const quetzal::stratum_sim::point_type& a, const quetzal::stratum_sim::point_type& b)
    {
        auto [ax, ay, az] = a.rep();
        auto [bx, by, bz] = b.rep();

        return quetzal::math::float_eq(ax, bx)
            && quetzal::math::float_eq(ay, by)
            && quetzal::math::float_eq(az, bz);
    }

} // namespace

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Wall& quetzal::stratum_sim::DiscreteLayout::wall(id_type wall_id) const
{
    assert(wall_id < m_wall_store.size());
    return m_wall_store[wall_id];
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Wall& quetzal::stratum_sim::DiscreteLayout::wall(id_type wall_id)
{
    assert(wall_id < m_wall_store.size());
    return m_wall_store[wall_id];
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::DiscreteLayout::wall_store_count() const
{
    return m_wall_store.size();
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::DiscreteLayout::walls_type& quetzal::stratum_sim::DiscreteLayout::wall_store() const
{
    return m_wall_store;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Room& quetzal::stratum_sim::DiscreteLayout::room(id_type room_id) const
{
    assert(room_id < m_room_store.size());
    return m_room_store[room_id];
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Room& quetzal::stratum_sim::DiscreteLayout::room(id_type room_id)
{
    assert(room_id < m_room_store.size());
    return m_room_store[room_id];
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::DiscreteLayout::room_store_count() const
{
    return m_room_store.size();
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::DiscreteLayout::rooms_type& quetzal::stratum_sim::DiscreteLayout::room_store() const
{
    return m_room_store;
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
    assert(m_floor_id == nullid); // The two bound the layout, so they are set once

    m_floor_id = m_flat_store.size();
    m_flat_store.emplace_back(height_floor, false);

    m_ceiling_id = m_flat_store.size();
    m_flat_store.emplace_back(height_ceiling, true);

    return;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::DiscreteLayout::ids_type quetzal::stratum_sim::DiscreteLayout::create_walls(const std::vector<polygon_type>& outlines)
{
    ids_type starts;
    starts.reserve(outlines.size());

    for (const polygon_type& outline : outlines)
    {
        size_t n = outline.vertex_count();
        assert(n >= 3);

        id_type id0 = m_wall_store.size();

        for (size_t i = 0; i < n; ++i)
        {
            Wall wall;
            wall.next_id = id0 + (i + 1) % n;
            wall.prev_id = id0 + (i + n - 1) % n;
            wall.position = outline.vertex(i);
            m_wall_store.push_back(wall);
        }

        starts.push_back(id0);
    }

    return starts;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::DiscreteLayout::create_room(const Room& room)
{
    id_type room_id = m_room_store.size();
    m_room_store.push_back(room);

    for (const discrete_point_type& cell : room.cells())
    {
        set_cell_room(cell, room_id);
    }

    return room_id;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::DiscreteLayout::room_at(const point_type& point) const
{
    for (id_type room_id = 0; room_id < m_room_store.size(); ++room_id)
    {
        if (m_room_store[room_id].contains(point))
        {
            return room_id;
        }
    }

    return nullid;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::DiscreteLayout::room_at(const discrete_point_type& grid_position) const
{
    return cell(grid_position).room_id;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::DiscreteLayout::set_cell_room(const discrete_point_type& grid_position, id_type room_id)
{
    cell(grid_position).room_id = room_id;
    return;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::DiscreteLayout::opposed(id_type wall_id_a, id_type wall_id_b) const
{
    if (wall_id_a == nullid || wall_id_b == nullid || wall_id_a == wall_id_b)
    {
        return false;
    }

    const Wall& a = wall(wall_id_a);
    const Wall& b = wall(wall_id_b);

    if (a.next_id == nullid || b.next_id == nullid)
    {
        return false;
    }

    return coincident(a.position, wall(b.next_id).position) && coincident(b.position, wall(a.next_id).position);
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::DiscreteLayout::connect(id_type wall_id_a, id_type wall_id_b)
{
    assert(opposed(wall_id_a, wall_id_b));

    wall(wall_id_a).partner_id = wall_id_b;
    wall(wall_id_b).partner_id = wall_id_a;

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::DiscreteLayout::disconnect(id_type wall_id)
{
    id_type partner_id = wall(wall_id).partner_id;
    wall(wall_id).partner_id = nullid;

    if (partner_id != nullid)
    {
        wall(partner_id).partner_id = nullid;
    }

    return;
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::DiscreteLayout::connect_walls(const ids_type& wall_ids_a, const ids_type& wall_ids_b)
{
    size_t count = 0;

    for (id_type a : wall_ids_a)
    {
        if (wall(a).partner_id != nullid)
        {
            continue;
        }

        for (id_type b : wall_ids_b)
        {
            if (wall(b).partner_id == nullid && opposed(a, b))
            {
                connect(a, b);
                ++count;
                break;
            }
        }
    }

    return count;
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::DiscreteLayout::connect_folds(const ids_type& wall_ids)
{
    return connect_walls(wall_ids, wall_ids);
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::DiscreteLayout::set_grid(const point_type& origin, value_type scale, const discrete_vector_type& extent)
{
    assert(scale > value_type(0));
    assert(extent.x() >= 0 && extent.y() >= 0);

    m_grid = GridMapping {origin, scale, extent};
    m_cells.assign(static_cast<size_t>(extent.x()) * extent.y(), Cell {});

    return;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::DiscreteLayout::open(const discrete_point_type& grid_position) const
{
    return cell(grid_position).open;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::DiscreteLayout::set_cell_open(const discrete_point_type& grid_position, bool open)
{
    cell(grid_position).open = open;
    return;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Cell& quetzal::stratum_sim::DiscreteLayout::cell(const discrete_point_type& grid_position) const
{
    const GridMapping& g = grid();
    assert(g.contains(grid_position));

    return m_cells[static_cast<size_t>(grid_position.j()) * g.extent.x() + grid_position.i()];
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Cell& quetzal::stratum_sim::DiscreteLayout::cell(const discrete_point_type& grid_position)
{
    const GridMapping& g = grid();
    assert(g.contains(grid_position));

    return m_cells[static_cast<size_t>(grid_position.j()) * g.extent.x() + grid_position.i()];
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::GridMapping& quetzal::stratum_sim::DiscreteLayout::grid() const
{
    assert(m_grid.has_value());
    return *m_grid;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::value_type quetzal::stratum_sim::DiscreteLayout::step_distance() const
{
    return grid().scale;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::discrete_point_type quetzal::stratum_sim::DiscreteLayout::grid_position_at(const point_type& point) const
{
    return grid().grid_position_at(point);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::point_type quetzal::stratum_sim::DiscreteLayout::position(const discrete_point_type& grid_position) const
{
    point_type result = grid().position(grid_position);

    // The floor is what fixes where the ground is. A layout with no floor falls back to the
    // grid's own plane, which is what one under construction has; so does a cell the floor is
    // absent under, that being a hole rather than a floor to walk on
    if (m_floor_id != nullid)
    {
        std::optional<value_type> height = flat(m_floor_id).height(result);
        if (height)
        {
            result.set_z(*height);
        }
    }

    return result;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::DiscreteLayout::wall_toward(const discrete_point_type& grid_position, const discrete_vector_type& direction) const
{
    const GridMapping& g = grid();
    assert(g.contains(grid_position));

    long di = direction.x();
    long dj = direction.y();
    assert(std::abs(di) + std::abs(dj) == 1); // One step along an axis

    auto [ox, oy, oz] = g.origin.rep();

    value_type sp = g.scale;
    value_type x0 = ox + static_cast<value_type>(grid_position.i()) * sp;
    value_type y0 = oy + static_cast<value_type>(grid_position.j()) * sp;
    value_type x1 = x0 + sp;
    value_type y1 = y0 + sp;
    value_type z = oz;

    // Wound so that the cell lies to the left
    // Change to wall normals ...
    point_type p0;
    point_type p1;

    if (di == 1)
    {
        p0 = point_type(x1, y0, z);
        p1 = point_type(x1, y1, z);
    }
    else if (di == -1)
    {
        p0 = point_type(x0, y1, z);
        p1 = point_type(x0, y0, z);
    }
    else if (dj == 1)
    {
        p0 = point_type(x1, y1, z);
        p1 = point_type(x0, y1, z);
    }
    else
    {
        p0 = point_type(x0, y0, z);
        p1 = point_type(x1, y0, z);
    }

    // One wall lies on one cell edge, so the two run end for end
    for (id_type id = 0; id < m_wall_store.size(); ++id)
    {
        const Wall& w = m_wall_store[id];
        if (w.next_id != nullid && coincident(w.position, p0) && coincident(wall(w.next_id).position, p1))
        {
            return id;
        }
    }

    return nullid;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::DiscreteLayout::blocked(const discrete_point_type& cell, const discrete_vector_type& direction) const
{
    if (!grid().contains(cell + direction))
    {
        return true;
    }

    id_type id = wall_toward(cell, direction);
    return id != nullid && !crossable(id);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::MoveResult quetzal::stratum_sim::DiscreteLayout::move(const placement_type& placement, const vector_type& direction, value_type distance,
    const obstructed_type& obstructed) const
{
    MoveResult result {placement, nullid};

    // Snap to whichever of the layout's own directions the caller most nearly asked for
    discrete_vector_type step = grid_step(direction);

    if (step.zero())
    {
        return result;
    }

    const GridMapping& g = grid();
    int steps = static_cast<int>(std::lround(distance / g.scale));

    discrete_point_type cell = grid_position_at(placement.position());

    for (int n = 0; n < steps; ++n)
    {
        id_type wall_id = wall_toward(cell, step);

        discrete_point_type next = cell + step;

        if (blocked(cell, step))
        {
            // Named when a wall is what stopped it, which leaves the grid edge unnamed
            result.obstruction_wall_id = wall_id != nullid && !crossable(wall_id) ? wall_id : nullid;
            break;
        }

        // Nothing open lies off the grid, so the test above is what keeps the mover on it
        assert(g.contains(next));

        if (obstructed)
        {
            point_type p = position(next);
            placement_type candidate(point_type(p.x(), p.y(), placement.position().z()), placement.direction());

            id_type obstruction = obstructed(candidate);
            if (obstruction != nullid)
            {
                result.obstruction_entity_id = obstruction;
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
    return grid_position_at(a.position()) == grid_position_at(b.position());
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::DiscreteLayout::placements_type quetzal::stratum_sim::DiscreteLayout::neighbors(const placement_type& placement) const
{
    placements_type result;
    discrete_point_type cell = grid_position_at(placement.position());

    const GridMapping& g = grid();
    const discrete_vector_type steps[4] {Direction::East, Direction::West, Direction::North, Direction::South};

    for (const discrete_vector_type& step : steps)
    {
        if (blocked(cell, step))
        {
            continue;
        }

        // The boundary blocks outward, so an unblocked step is one onto the grid
        discrete_point_type next = cell + step;
        assert(g.contains(next));

        point_type p = position(next);
        result.push_back(placement_type(point_type(p.x(), p.y(), placement.position().z()), placement.direction()));
    }

    return result;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::DiscreteLayout::visible(const placement_type& from, const placement_type& to) const
{
    discrete_point_type grid_position = grid_position_at(from.position());
    discrete_point_type target = grid_position_at(to.position());

    if (grid_position == target)
    {
        return true;
    }

    const GridMapping& g = grid();

    auto [fx, fy, fz] = from.position().rep();
    auto [ox, oy, oz] = g.origin.rep();
    auto [dx, dy, dz] = (to.position() - from.position()).rep();

    int step_i = dx > value_type(0) ? 1 : (dx < value_type(0) ? -1 : 0);
    int step_j = dy > value_type(0) ? 1 : (dy < value_type(0) ? -1 : 0);

    // Distance along the ray to the next cell boundary on each axis, and between
    // successive boundaries. An axis with no motion never becomes the smaller of the two
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

    value_type t_i = boundary(grid_position.i(), step_i, ox, fx, dx);
    value_type t_j = boundary(grid_position.j(), step_j, oy, fy, dy);

    value_type delta_i = step_i == 0 ? far : g.scale / std::abs(dx);
    value_type delta_j = step_j == 0 ? far : g.scale / std::abs(dy);

    // One step per cell edge crossed, so the walk cannot exceed the grid
    size_t limit = static_cast<size_t>(g.extent.x() + g.extent.y()) + 2;

    for (size_t n = 0; n < limit; ++n)
    {
        discrete_vector_type step = t_i < t_j ? discrete_vector_type(step_i, 0) : discrete_vector_type(0, step_j);

        if (blocked(grid_position, step))
        {
            return false;
        }

        // The boundary blocks outward, so the ray is stopped before it can leave the grid
        discrete_point_type next = grid_position + step;
        assert(g.contains(next));

        if (t_i < t_j)
        {
            t_i += delta_i;
        }
        else
        {
            t_j += delta_j;
        }

        grid_position = next;

        if (grid_position == target)
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
        return nullid; // Facing no direction the grid has, so facing nothing
    }

    return wall_toward(grid_position_at(placement.position()), step);
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::DiscreteLayout::clear()
{
    m_wall_store.clear();
    m_room_store.clear();
    m_flat_store.clear();
    m_floor_id = nullid;
    m_ceiling_id = nullid;
    m_grid.reset();
    m_cells.clear();
    return;
}
