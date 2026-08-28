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

    // A region's footprint is horizontal, so containment is tested in two dimensions.
    // Testing the three dimensional outline instead would require the point to lie on its
    // plane, which a point at floor height does not
    using footprint_traits = quetzal::stratum_sim::traits_type::resized_traits<2>;
    using footprint_point_type = quetzal::geometry::Point<footprint_traits>;
    using footprint_type = quetzal::geometry::Polygon<footprint_traits>;

    //--------------------------------------------------------------------------
    bool coincident(const quetzal::stratum_sim::point_type& a, const quetzal::stratum_sim::point_type& b)
    {
        auto [ax, ay, az] = a.rep();
        auto [bx, by, bz] = b.rep();

        return quetzal::math::float_eq(ax, bx)
            && quetzal::math::float_eq(ay, by)
            && quetzal::math::float_eq(az, bz);
    }

    //--------------------------------------------------------------------------
    // Whether the directed segment p0 to p1 lies along the directed segment a to b. A
    // facing spans a whole polygon edge, which may cover several cell edges, so a cell
    // edge is a subsegment of its facing rather than equal to it
    bool covers(const quetzal::stratum_sim::point_type& a, const quetzal::stratum_sim::point_type& b,
        const quetzal::stratum_sim::point_type& p0, const quetzal::stratum_sim::point_type& p1)
    {
        using value_type = quetzal::stratum_sim::value_type;

        auto [dx, dy, dz] = (b - a).rep(); // dx, dy, _ ...
        auto [ex, ey, ez] = (p1 - p0).rep(); // ex, ey, _ ...

        value_type length2 = dx * dx + dy * dy;
        if (length2 <= value_type(0))
        {
            return false;
        }

        // Collinear and running the same way, not merely along the same line
        if (!quetzal::math::float_eq0(dx * ey - dy * ex, length2) || dx * ex + dy * ey <= value_type(0))
        {
            return false;
        }

        auto [ax, ay, az] = a.rep();

        auto offset = [&](const quetzal::stratum_sim::point_type& p)
        {
            return (p.x() - ax) * dy - (p.y() - ay) * dx;
        };

        if (!quetzal::math::float_eq0(offset(p0), length2) || !quetzal::math::float_eq0(offset(p1), length2))
        {
            return false;
        }

        auto parameter = [&](const quetzal::stratum_sim::point_type& p)
        {
            return ((p.x() - ax) * dx + (p.y() - ay) * dy) / length2;
        };

        value_type t0 = parameter(p0);
        value_type t1 = parameter(p1);

        return quetzal::math::float_ge(t0, value_type(0)) && quetzal::math::float_le(t1, value_type(1)) && t0 < t1;
    }

} // namespace

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Wall& quetzal::stratum_sim::Layout::wall(id_type id) const
{
    assert(id < m_wall_store.size());
    return m_wall_store[id];
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Wall& quetzal::stratum_sim::Layout::wall(id_type id)
{
    assert(id < m_wall_store.size());
    return m_wall_store[id];
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
const quetzal::stratum_sim::Region& quetzal::stratum_sim::Layout::region(id_type id) const
{
    assert(id < m_region_store.size());
    return m_region_store[id];
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Region& quetzal::stratum_sim::Layout::region(id_type id)
{
    assert(id < m_region_store.size());
    return m_region_store[id];
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::Layout::region_store_count() const
{
    return m_region_store.size();
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Layout::regions_type& quetzal::stratum_sim::Layout::region_store() const
{
    return m_region_store;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Flat& quetzal::stratum_sim::Layout::flat(id_type id) const
{
    assert(id < m_flat_store.size());
    return m_flat_store[id];
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Flat& quetzal::stratum_sim::Layout::flat(id_type id)
{
    assert(id < m_flat_store.size());
    return m_flat_store[id];
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
quetzal::id_type quetzal::stratum_sim::Layout::create_region(const polygon_type& outline, value_type height_floor, value_type height_ceiling)
{
    return create_region(outline, {}, height_floor, height_ceiling);
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::Layout::create_region(const polygon_type& outline, const std::vector<polygon_type>& holes, value_type height_floor, value_type height_ceiling)
{
    id_type region_id = m_region_store.size();

    auto create_ring = [&](const polygon_type& ring)
    {
        size_t n = ring.vertex_count();
        assert(n >= 3);

        id_type id0 = m_wall_store.size();

        for (size_t i = 0; i < n; ++i)
        {
            Wall wall;
            wall.next = id0 + (i + 1) % n;
            wall.prev = id0 + (i + n - 1) % n;
            wall.region = region_id;
            wall.position = ring.vertex(i);
            m_wall_store.push_back(wall);
        }

        return id0;
    };

    id_type wall_id0 = create_ring(outline);

    std::vector<id_type> hole_ids;
    hole_ids.reserve(holes.size());
    for (const polygon_type& hole : holes)
    {
        hole_ids.push_back(create_ring(hole));
    }

    id_type floor_id = m_flat_store.size();
    m_flat_store.push_back({region_id, height_floor, false});

    id_type ceiling_id = m_flat_store.size();
    m_flat_store.push_back({region_id, height_ceiling, true});

    m_region_store.push_back({wall_id0, hole_ids, floor_id, ceiling_id});

    return region_id;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Layout::ids_type quetzal::stratum_sim::Layout::region_walls(id_type region_id) const
{
    return ring_walls(region(region_id).wall);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Layout::ids_type quetzal::stratum_sim::Layout::region_hole_walls(id_type region_id, size_t hole) const
{
    const Region& r = region(region_id);
    assert(hole < r.holes.size());
    return ring_walls(r.holes[hole]);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Layout::ids_type quetzal::stratum_sim::Layout::region_all_walls(id_type region_id) const
{
    ids_type ids = region_walls(region_id);

    for (size_t hole = 0; hole < region(region_id).holes.size(); ++hole)
    {
        ids_type hole_ids = region_hole_walls(region_id, hole);
        ids.insert(ids.end(), hole_ids.begin(), hole_ids.end());
    }

    return ids;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Layout::ids_type quetzal::stratum_sim::Layout::ring_walls(id_type wall_id0) const
{
    ids_type ids;

    id_type id = wall_id0;

    while (id != nullid)
    {
        ids.push_back(id);
        id = wall(id).next;

        if (id == wall_id0)
        {
            break;
        }
    }

    return ids;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::polygon_type quetzal::stratum_sim::Layout::region_outline(id_type region_id) const
{
    return ring_outline(region(region_id).wall);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::polygon_type quetzal::stratum_sim::Layout::region_hole_outline(id_type region_id, size_t hole) const
{
    const Region& r = region(region_id);
    assert(hole < r.holes.size());
    return ring_outline(r.holes[hole]);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::polygon_type quetzal::stratum_sim::Layout::ring_outline(id_type wall_id0) const
{
    points_type points;

    for (id_type id : ring_walls(wall_id0))
    {
        points.push_back(wall(id).position);
    }

    return polygon_type(points.begin(), points.end());
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::Layout::region_at(const point_type& point) const
{
    footprint_point_type p(point.x(), point.y());

    auto within = [&p](const polygon_type& outline)
    {
        std::vector<footprint_point_type> points;
        points.reserve(outline.vertex_count());

        for (size_t i = 0; i < outline.vertex_count(); ++i)
        {
            auto [vx, vy, vz] = outline.vertex(i).rep();
            points.push_back(footprint_point_type(vx, vy));
        }

        return footprint_type(points.begin(), points.end()).contains(p);
    };

    for (id_type region_id = 0; region_id < m_region_store.size(); ++region_id)
    {
        if (!within(region_outline(region_id)))
        {
            continue;
        }

        bool in_hole = false;
        for (size_t hole = 0; hole < region(region_id).holes.size() && !in_hole; ++hole)
        {
            in_hole = within(region_hole_outline(region_id, hole));
        }

        if (!in_hole)
        {
            return region_id;
        }
    }

    return nullid;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::Layout::region_at(const Cell& cell) const
{
    const Grid& g = grid();

    if (cell.i < 0 || cell.j < 0 || cell.i >= g.extent.i || cell.j >= g.extent.j)
    {
        return nullid;
    }

    return m_cell_regions[static_cast<size_t>(cell.j) * g.extent.i + cell.i];
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Layout::set_cell_region(const Cell& cell, id_type region_id)
{
    const Grid& g = grid();
    assert(cell.i >= 0 && cell.j >= 0 && cell.i < g.extent.i && cell.j < g.extent.j);

    m_cell_regions[static_cast<size_t>(cell.j) * g.extent.i + cell.i] = region_id;

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

    if (a.next == nullid || b.next == nullid)
    {
        return false;
    }

    return coincident(a.position, wall(b.next).position) && coincident(b.position, wall(a.next).position);
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Layout::connect(id_type wall_id_a, id_type wall_id_b)
{
    assert(opposed(wall_id_a, wall_id_b));

    wall(wall_id_a).partner = wall_id_b;
    wall(wall_id_b).partner = wall_id_a;

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Layout::disconnect(id_type wall_id)
{
    id_type partner_id = wall(wall_id).partner;
    wall(wall_id).partner = nullid;

    if (partner_id != nullid)
    {
        wall(partner_id).partner = nullid;
    }

    return;
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::Layout::connect_regions(id_type region_id_a, id_type region_id_b)
{
    ids_type walls_a = region_all_walls(region_id_a);
    ids_type walls_b = region_all_walls(region_id_b);

    size_t count = 0;

    for (id_type a : walls_a)
    {
        if (wall(a).partner != nullid)
        {
            continue;
        }

        for (id_type b : walls_b)
        {
            if (wall(b).partner == nullid && opposed(a, b))
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
size_t quetzal::stratum_sim::Layout::connect_folds(id_type region_id)
{
    return connect_regions(region_id, region_id);
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Layout::set_passable(id_type wall_id, bool passable)
{
    wall(wall_id).passable = passable;
    return;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Layout::discrete() const
{
    return m_grid.has_value();
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Layout::set_grid(const point_type& origin, value_type spacing, const Cell& extent)
{
    assert(spacing > value_type(0));
    assert(extent.i >= 0 && extent.j >= 0);

    m_grid = Grid {origin, spacing, extent};
    m_cell_regions.assign(static_cast<size_t>(extent.i) * extent.j, nullid);

    return;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Grid& quetzal::stratum_sim::Layout::grid() const
{
    assert(m_grid.has_value());
    return *m_grid;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Cell quetzal::stratum_sim::Layout::cell_at(const point_type& point) const
{
    const Grid& g = grid();

    auto [px, py, pz] = point.rep();
    auto [ox, oy, oz] = g.origin.rep();

    value_type i = std::floor((px - ox) / g.spacing);
    value_type j = std::floor((py - oy) / g.spacing);

    return {static_cast<int>(i), static_cast<int>(j)};
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::point_type quetzal::stratum_sim::Layout::cell_position(const Cell& cell) const
{
    const Grid& g = grid();

    auto [ox, oy, oz] = g.origin.rep();

    value_type x = ox + (static_cast<value_type>(cell.i) + value_type(0.5)) * g.spacing;
    value_type y = oy + (static_cast<value_type>(cell.j) + value_type(0.5)) * g.spacing;

    return point_type(x, y, oz);
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::Layout::facing_between(const Cell& cell_a, const Cell& cell_b) const
{
    const Grid& g = grid();

    int di = cell_b.i - cell_a.i;
    int dj = cell_b.j - cell_a.j;
    assert(std::abs(di) + std::abs(dj) == 1); // Orthogonally adjacent

    auto [ox, oy, oz] = g.origin.rep();

    value_type sp = g.spacing;
    value_type x0 = ox + static_cast<value_type>(cell_a.i) * sp;
    value_type y0 = oy + static_cast<value_type>(cell_a.j) * sp;
    value_type x1 = x0 + sp;
    value_type y1 = y0 + sp;
    value_type z = oz;

    // Wound so that cell_a lies to the left, which is the side the facing belonging to
    // cell_a's region is traversed from
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

    for (id_type id = 0; id < m_wall_store.size(); ++id)
    {
        const Wall& w = m_wall_store[id];
        if (w.next != nullid && covers(w.position, wall(w.next).position, p0, p1))
        {
            return id;
        }
    }

    // No facing on that edge means both cells lie within one region
    return nullid;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Layout::blocked(const Cell& cell_a, const Cell& cell_b) const
{
    id_type id = facing_between(cell_a, cell_b);
    return id != nullid && !wall(id).passable;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::MoveResult quetzal::stratum_sim::Layout::move(const placement_type& placement, const vector_type& direction, value_type distance) const
{
    assert(discrete()); // A continuous layout integrates instead; that is the second implementation

    MoveResult result {placement, nullid};

    // Snap to whichever of the layout's own directions the caller most nearly asked for
    auto [dx, dy, dz] = direction.rep();

    if (math::float_eq0(dx) && math::float_eq0(dy))
    {
        return result;
    }

    Cell step = std::abs(dx) >= std::abs(dy)
        ? Cell {dx > value_type(0) ? 1 : -1, 0}
        : Cell {0, dy > value_type(0) ? 1 : -1};

    const Grid& g = grid();
    int steps = static_cast<int>(std::lround(distance / g.spacing));

    Cell cell = cell_at(placement.position());

    for (int n = 0; n < steps; ++n)
    {
        Cell next {cell.i + step.i, cell.j + step.j};
        id_type facing = facing_between(cell, next);

        if (facing != nullid && !wall(facing).passable)
        {
            result.obstruction = facing;
            break;
        }

        cell = next;
    }

    point_type p = cell_position(cell);
    result.placement.set_position(point_type(p.x(), p.y(), placement.position().z()));

    return result;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::placement_type quetzal::stratum_sim::Layout::turn(const placement_type& placement, Turn turn) const
{
    placement_type result = placement;

    auto [x, y, z] = placement.direction().rep();

    switch (turn)
    {
        case Turn::Left:
            result.set_direction(vector_type(-y, x, z));
            break;

        case Turn::Right:
            result.set_direction(vector_type(y, -x, z));
            break;

        case Turn::Around:
            result.set_direction(vector_type(-x, -y, z));
            break;
    }

    return result;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Layout::same_location(const placement_type& a, const placement_type& b) const
{
    return discrete()
        ? cell_at(a.position()) == cell_at(b.position())
        : coincident(a.position(), b.position());
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Layout::placements_type quetzal::stratum_sim::Layout::neighbors(const placement_type& placement) const
{
    assert(discrete());

    placements_type result;
    Cell cell = cell_at(placement.position());

    const Cell steps[4] {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    for (const Cell& step : steps)
    {
        Cell next {cell.i + step.i, cell.j + step.j};
        if (blocked(cell, next))
        {
            continue;
        }

        point_type p = cell_position(next);
        result.push_back(placement_type(point_type(p.x(), p.y(), placement.position().z()), placement.direction()));
    }

    return result;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Layout::visible(const placement_type& from, const placement_type& to) const
{
    assert(discrete());

    Cell cell = cell_at(from.position());
    Cell target = cell_at(to.position());

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
    size_t limit = static_cast<size_t>(g.extent.i + g.extent.j) + 2;

    for (size_t n = 0; n < limit; ++n)
    {
        Cell next = t_i < t_j ? Cell {cell.i + step_i, cell.j} : Cell {cell.i, cell.j + step_j};

        if (blocked(cell, next))
        {
            return false;
        }

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
    m_region_store.clear();
    m_flat_store.clear();
    m_grid.reset();
    m_cell_regions.clear();
    return;
}
