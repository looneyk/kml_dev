//------------------------------------------------------------------------------
// stratum_sim
// Layout.cpp
//------------------------------------------------------------------------------

#include "Layout.hpp"
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
bool quetzal::stratum_sim::Wall::deleted() const
{
    return m_deleted;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Wall::set_deleted(bool b) const
{
    m_deleted = b;
    return;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Grid::contains(const GridPosition& cell) const
{
    return cell.i >= 0 && cell.j >= 0 && cell.i < extent.x() && cell.j < extent.y();
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Wall& quetzal::stratum_sim::Layout::wall(id_type wall_id) const
{
    assert(wall_id < m_wall_store.size());
    return m_wall_store[wall_id];
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Wall& quetzal::stratum_sim::Layout::wall(id_type wall_id)
{
    assert(wall_id < m_wall_store.size());
    return m_wall_store[wall_id];
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::Layout::wall_store_count() const
{
    return m_wall_store.size();
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Layout::walls_type& quetzal::stratum_sim::Layout::wall_store() const
{
    return m_wall_store;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Room& quetzal::stratum_sim::Layout::room(id_type room_id) const
{
    assert(room_id < m_room_store.size());
    return m_room_store[room_id];
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Room& quetzal::stratum_sim::Layout::room(id_type room_id)
{
    assert(room_id < m_room_store.size());
    return m_room_store[room_id];
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::Layout::room_store_count() const
{
    return m_room_store.size();
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Layout::rooms_type& quetzal::stratum_sim::Layout::room_store() const
{
    return m_room_store;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Flat& quetzal::stratum_sim::Layout::flat(id_type flat_id) const
{
    assert(flat_id < m_flat_store.size());
    return m_flat_store[flat_id];
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Flat& quetzal::stratum_sim::Layout::flat(id_type flat_id)
{
    assert(flat_id < m_flat_store.size());
    return m_flat_store[flat_id];
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::Layout::flat_store_count() const
{
    return m_flat_store.size();
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Layout::flats_type& quetzal::stratum_sim::Layout::flat_store() const
{
    return m_flat_store;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::Layout::floor_id() const
{
    return m_floor_id;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::Layout::ceiling_id() const
{
    return m_ceiling_id;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Layout::set_flats(value_type height_floor, value_type height_ceiling)
{
    assert(m_floor_id == nullid); // The two bound the layout, so they are set once

    m_floor_id = m_flat_store.size();
    m_flat_store.push_back({height_floor, false});

    m_ceiling_id = m_flat_store.size();
    m_flat_store.push_back({height_ceiling, true});

    return;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Layout::ids_type quetzal::stratum_sim::Layout::create_walls(const std::vector<polygon_type>& outlines)
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
quetzal::id_type quetzal::stratum_sim::Layout::create_room(const Room& room)
{
    id_type room_id = m_room_store.size();
    m_room_store.push_back(room);

    for (const GridPosition& cell : room.cells())
    {
        set_cell_room(cell, room_id);
    }

    return room_id;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::Layout::room_at(const point_type& point) const
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
quetzal::id_type quetzal::stratum_sim::Layout::room_at(const GridPosition& cell) const
{
    const Grid& g = grid();
    assert(g.contains(cell));

    return m_cell_rooms[static_cast<size_t>(cell.j) * g.extent.x() + cell.i];
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Layout::set_cell_room(const GridPosition& cell, id_type room_id)
{
    const Grid& g = grid();
    assert(g.contains(cell));

    m_cell_rooms[static_cast<size_t>(cell.j) * g.extent.x() + cell.i] = room_id;

    return;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Layout::opposed(id_type wall_id_a, id_type wall_id_b) const
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
void quetzal::stratum_sim::Layout::connect(id_type wall_id_a, id_type wall_id_b)
{
    assert(opposed(wall_id_a, wall_id_b));

    wall(wall_id_a).partner_id = wall_id_b;
    wall(wall_id_b).partner_id = wall_id_a;

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Layout::disconnect(id_type wall_id)
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
size_t quetzal::stratum_sim::Layout::connect_walls(const ids_type& wall_ids_a, const ids_type& wall_ids_b)
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
size_t quetzal::stratum_sim::Layout::connect_folds(const ids_type& wall_ids)
{
    return connect_walls(wall_ids, wall_ids);
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Layout::set_passable(id_type wall_id, bool passable)
{
    wall(wall_id).passable = passable;
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Layout::set_grid(const point_type& origin, value_type spacing, const DiscreteVector& extent)
{
    assert(spacing > value_type(0));
    assert(extent.x() >= 0 && extent.y() >= 0);

    m_grid = Grid {origin, spacing, extent};
    m_cell_open.assign(static_cast<size_t>(extent.x()) * extent.y(), false);
    m_cell_rooms.assign(static_cast<size_t>(extent.x()) * extent.y(), nullid);

    return;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Layout::open(const GridPosition& cell) const
{
    const Grid& g = grid();
    assert(g.contains(cell));

    return m_cell_open[static_cast<size_t>(cell.j) * g.extent.x() + cell.i];
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Layout::set_cell_open(const GridPosition& cell, bool open)
{
    const Grid& g = grid();
    assert(g.contains(cell));

    m_cell_open[static_cast<size_t>(cell.j) * g.extent.x() + cell.i] = open;

    return;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Grid& quetzal::stratum_sim::Layout::grid() const
{
    assert(m_grid.has_value());
    return *m_grid;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::value_type quetzal::stratum_sim::Layout::step_distance() const
{
    return grid().spacing;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::GridPosition quetzal::stratum_sim::Layout::cell_at(const point_type& point) const
{
    const Grid& g = grid();

    auto [px, py, pz] = point.rep();
    auto [ox, oy, oz] = g.origin.rep();

    value_type i = std::floor((px - ox) / g.spacing);
    value_type j = std::floor((py - oy) / g.spacing);

    return {static_cast<int>(i), static_cast<int>(j)};
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::point_type quetzal::stratum_sim::Layout::cell_position(const GridPosition& cell) const
{
    const Grid& g = grid();

    auto [ox, oy, oz] = g.origin.rep();

    value_type x = ox + (static_cast<value_type>(cell.i) + value_type(0.5)) * g.spacing;
    value_type y = oy + (static_cast<value_type>(cell.j) + value_type(0.5)) * g.spacing;

    return point_type(x, y, oz);
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::Layout::wall_toward(const GridPosition& cell, const DiscreteVector& direction) const
{
    const Grid& g = grid();
    assert(g.contains(cell));

    long di = direction.x();
    long dj = direction.y();
    assert(std::abs(di) + std::abs(dj) == 1); // One step along an axis

    auto [ox, oy, oz] = g.origin.rep();

    value_type sp = g.spacing;
    value_type x0 = ox + static_cast<value_type>(cell.i) * sp;
    value_type y0 = oy + static_cast<value_type>(cell.j) * sp;
    value_type x1 = x0 + sp;
    value_type y1 = y0 + sp;
    value_type z = oz;

    // Wound so that the cell lies to the left, which is the side its own wall is traversed
    // from
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

    // No wall on that edge means nothing stands that way
    return nullid;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Layout::blocked(const GridPosition& cell, const DiscreteVector& direction) const
{
    id_type id = wall_toward(cell, direction);
    return id != nullid && !wall(id).passable;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::MoveResult quetzal::stratum_sim::Layout::move(const placement_type& placement, const vector_type& direction, value_type distance,
    const obstructed_type& obstructed) const
{
    MoveResult result {placement, nullid};

    // Snap to whichever of the layout's own directions the caller most nearly asked for
    DiscreteVector step = grid_step(direction);

    if (step.zero())
    {
        return result;
    }

    const Grid& g = grid();
    int steps = static_cast<int>(std::lround(distance / g.spacing));

    GridPosition cell = cell_at(placement.position());

    for (int n = 0; n < steps; ++n)
    {
        id_type wall_id = wall_toward(cell, step);

        if (wall_id != nullid && !wall(wall_id).passable)
        {
            result.obstruction_wall_id = wall_id;
            break;
        }

        // Every boundary cell carries an impassable wall outward, so passing the test above
        // is what keeps the mover on the grid
        GridPosition next = cell + step;
        assert(g.contains(next));

        if (obstructed)
        {
            point_type p = cell_position(next);
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

    point_type p = cell_position(cell);
    result.placement.set_position(point_type(p.x(), p.y(), placement.position().z()));

    return result;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::placement_type quetzal::stratum_sim::Layout::turn(const placement_type& placement, Rotation rotation) const
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
quetzal::stratum_sim::placement_type quetzal::stratum_sim::Layout::face(const placement_type& placement, const vector_type& direction) const
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
bool quetzal::stratum_sim::Layout::same_location(const placement_type& a, const placement_type& b) const
{
    return cell_at(a.position()) == cell_at(b.position());
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Layout::placements_type quetzal::stratum_sim::Layout::neighbors(const placement_type& placement) const
{
    placements_type result;
    GridPosition cell = cell_at(placement.position());

    const Grid& g = grid();
    const DiscreteVector steps[4] {Direction::East, Direction::West, Direction::North, Direction::South};

    for (const DiscreteVector& step : steps)
    {
        if (blocked(cell, step))
        {
            continue;
        }

        // The boundary blocks outward, so an unblocked step is one onto the grid
        GridPosition next = cell + step;
        assert(g.contains(next));

        point_type p = cell_position(next);
        result.push_back(placement_type(point_type(p.x(), p.y(), placement.position().z()), placement.direction()));
    }

    return result;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Layout::visible(const placement_type& from, const placement_type& to) const
{
    GridPosition cell = cell_at(from.position());
    GridPosition target = cell_at(to.position());

    if (cell == target)
    {
        return true;
    }

    const Grid& g = grid();

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

        value_type edge = origin + static_cast<value_type>(step > 0 ? index + 1 : index) * g.spacing;
        return (edge - position) / d;
    };

    value_type t_i = boundary(cell.i, step_i, ox, fx, dx);
    value_type t_j = boundary(cell.j, step_j, oy, fy, dy);

    value_type delta_i = step_i == 0 ? far : g.spacing / std::abs(dx);
    value_type delta_j = step_j == 0 ? far : g.spacing / std::abs(dy);

    // One step per cell edge crossed, so the walk cannot exceed the grid
    size_t limit = static_cast<size_t>(g.extent.x() + g.extent.y()) + 2;

    for (size_t n = 0; n < limit; ++n)
    {
        DiscreteVector step = t_i < t_j ? DiscreteVector(step_i, 0) : DiscreteVector(0, step_j);

        if (blocked(cell, step))
        {
            return false;
        }

        // The boundary blocks outward, so the ray is stopped before it can leave the grid
        GridPosition next = cell + step;
        assert(g.contains(next));

        if (t_i < t_j)
        {
            t_i += delta_i;
        }
        else
        {
            t_j += delta_j;
        }

        cell = next;

        if (cell == target)
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Layout::clear()
{
    m_wall_store.clear();
    m_room_store.clear();
    m_flat_store.clear();
    m_floor_id = nullid;
    m_ceiling_id = nullid;
    m_grid.reset();
    m_cell_open.clear();
    m_cell_rooms.clear();
    return;
}
