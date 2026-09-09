//------------------------------------------------------------------------------
// dungen
// generate_grid1.cpp
//------------------------------------------------------------------------------

#include "generate_grid1.hpp"
#include "place_stairs.hpp"
#include <algorithm>
#include <numeric>
#include <utility>
#include <vector>
#include <cassert>

using namespace std;

namespace
{

    using namespace quetzal::dungen;

    //--------------------------------------------------------------------------
    // What one sector of the grid holds: a room, or the single cell a sector without one
    // still offers its neighbors to join through
    struct Sector
    {
        box_type rectangle;
        bool room = false;
    };

    using sectors_type = vector<Sector>;

    //--------------------------------------------------------------------------
    // Two sectors that adjoin, and which way round they do. A single column of sectors puts
    // its neighbors one slot apart just as a row does, so the orientation is stored here
    // rather than read back out of the slots
    struct Adjacency
    {
        size_t slot_a = 0;
        size_t slot_b = 0;
        bool horizontal = false;
    };

    //--------------------------------------------------------------------------
    // Disjoint sets over sector slots, for asking whether two sectors are already reachable
    // from one another
    class Components
    {
    public:

        explicit Components(size_t n);

        size_t root(size_t slot);

        // Whether the two were separate, joining them where they were
        bool join(size_t slot_a, size_t slot_b);

    private:

        vector<size_t> m_parents;
    };

    //--------------------------------------------------------------------------
    Components::Components(size_t n) :
        m_parents(n)
    {
        iota(m_parents.begin(), m_parents.end(), size_t(0));
    }

    //--------------------------------------------------------------------------
    size_t Components::root(size_t slot)
    {
        while (m_parents[slot] != slot)
        {
            m_parents[slot] = m_parents[m_parents[slot]]; // Halving the path, so repeated queries stay cheap
            slot = m_parents[slot];
        }

        return slot;
    }

    //--------------------------------------------------------------------------
    bool Components::join(size_t slot_a, size_t slot_b)
    {
        size_t a = root(slot_a);
        size_t b = root(slot_b);

        if (a == b)
        {
            return false;
        }

        m_parents[b] = a;
        return true;
    }

    //--------------------------------------------------------------------------
    // The half open cell range sector (i, j) covers, an uneven division spreading its
    // remainder across the sectors rather than leaving it all at one end
    box_type sector_bounds(vector_type size, vector_type divisions, coord_type i, coord_type j)
    {
        auto [nx, ny] = size;
        auto [divisions_x, divisions_y] = divisions;

        coord_type x0 = i * nx / divisions_x;
        coord_type x1 = (i + 1) * nx / divisions_x;
        coord_type y0 = j * ny / divisions_y;
        coord_type y1 = (j + 1) * ny / divisions_y;

        return {point_type {x0, y0}, point_type {x1, y1}};
    }

    //--------------------------------------------------------------------------
    // A room somewhere inside the sector, or the one cell that substitutes for a sector holding
    // none, the margin left clear on every side so that a corridor has rock to run through
    // and no two rooms share a wall
    Sector place_sector(Level& level, const box_type& bounds, const Grid1Parameters& grid1, bool room, random_type& random)
    {
        point_type origin = bounds.position() + vector_type {grid1.room_margin, grid1.room_margin};
        vector_type span = bounds.extent() - vector_type {2 * grid1.room_margin, 2 * grid1.room_margin};

        Sector sector;
        sector.room = room;
        sector.rectangle.set_extent(room
            ? vector_type {random.gen(grid1.room_dimension_min, span.x()), random.gen(grid1.room_dimension_min, span.y())}
            : vector_type {1, 1});
        sector.rectangle.set_position(
        {
            origin.x() + random.gen(span.x() - sector.rectangle.extent().x() + 1),
            origin.y() + random.gen(span.y() - sector.rectangle.extent().y() + 1)
        });

        if (room)
        {
            level.create_room(sector.rectangle);
        }
        else
        {
            level.cell(sector.rectangle.position()).set_type(CellType::Corridor);
        }

        return sector;
    }

    //--------------------------------------------------------------------------
    // Every orthogonal adjacency in the grid, each listed once
    vector<Adjacency> sector_adjacencies(vector_type divisions)
    {
        auto [divisions_x, divisions_y] = divisions;

        vector<Adjacency> adjacencies;

        for (coord_type j = 0; j < divisions_y; ++j)
        {
            for (coord_type i = 0; i < divisions_x; ++i)
            {
                size_t slot = static_cast<size_t>(j * divisions_x + i);

                if (i + 1 < divisions_x)
                {
                    adjacencies.push_back({slot, slot + 1, true});
                }

                if (j + 1 < divisions_y)
                {
                    adjacencies.push_back({slot, slot + static_cast<size_t>(divisions_x), false});
                }
            }
        }

        return adjacencies;
    }

    //--------------------------------------------------------------------------
    // A random spanning tree over the sectors, so that every one of them is reachable, and
    // the adjacencies it had no need of kept with probability loop
    vector<Adjacency> sector_connections(vector<Adjacency> adjacencies, size_t n_sectors, double loop, random_type& random)
    {
        shuffle(adjacencies.begin(), adjacencies.end(), random.engine());

        Components components(n_sectors);
        vector<Adjacency> connections;

        for (const Adjacency& adjacency : adjacencies)
        {
            if (components.join(adjacency.slot_a, adjacency.slot_b) || random.gen() < loop)
            {
                connections.push_back(adjacency);
            }
        }

        return connections;
    }

    //--------------------------------------------------------------------------
    // Rock along a horizontal or vertical line, its ends included, turned to corridor. What
    // is already open is left as it is, so a corridor crossing another one joins it
    void carve(Level& level, point_type from, point_type to)
    {
        coord_type dx = to.x() - from.x();
        coord_type dy = to.y() - from.y();
        assert(dx == 0 || dy == 0);

        coord_type steps = dx != 0 ? (dx > 0 ? dx : -dx) : (dy > 0 ? dy : -dy);
        vector_type step {dx > 0 ? 1 : (dx < 0 ? -1 : 0), dy > 0 ? 1 : (dy < 0 ? -1 : 0)};

        point_type position = from;
        for (coord_type i = 0; i <= steps; ++i)
        {
            if (level.cell(position).type() == CellType::Solid)
            {
                level.cell(position).set_type(CellType::Corridor);
            }

            position += step;
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Where a corridor meets a room it becomes a door in the wall it entered by; where it
    // meets a sector holding no room it stays corridor
    void place_door(Level& level, point_type position, bool room)
    {
        if (room && level.cell(position).type() != CellType::Door)
        {
            level.create_door(position);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A corridor from one sector to the other, leaving each through the wall it faces the
    // other across and turning once in the rock between them
    // The turn is taken on a line that no room reaches, the rooms being held off the edges of
    // their own sectors, so the corridor never crosses one on its way
    void connect(Level& level, const Sector& a, const Sector& b, bool horizontal, random_type& random)
    {
        if (horizontal)
        {
            coord_type ya = a.rectangle.position().y() + random.gen(a.rectangle.extent().y());
            coord_type yb = b.rectangle.position().y() + random.gen(b.rectangle.extent().y());
            coord_type xa = a.rectangle.upper().x(); // The wall cell east of a
            coord_type xb = b.rectangle.position().x() - 1; // The wall cell west of b
            coord_type xm = xa + random.gen(xb - xa + 1); // Where it turns

            carve(level, {xa, ya}, {xm, ya});
            carve(level, {xm, ya}, {xm, yb});
            carve(level, {xm, yb}, {xb, yb});

            place_door(level, {xa, ya}, a.room);
            place_door(level, {xb, yb}, b.room);
        }
        else
        {
            coord_type xa = a.rectangle.position().x() + random.gen(a.rectangle.extent().x());
            coord_type xb = b.rectangle.position().x() + random.gen(b.rectangle.extent().x());
            coord_type ya = a.rectangle.upper().y(); // The wall cell north of a
            coord_type yb = b.rectangle.position().y() - 1; // The wall cell south of b
            coord_type ym = ya + random.gen(yb - ya + 1); // Where it turns

            carve(level, {xa, ya}, {xa, ym});
            carve(level, {xa, ym}, {xb, ym});
            carve(level, {xb, ym}, {xb, yb});

            place_door(level, {xa, ya}, a.room);
            place_door(level, {xb, yb}, b.room);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A stairway goes in a room, or in a corridor where the density left no rooms at all
    vector<point_type> stair_candidates(const Level& level)
    {
        vector<point_type> rooms;
        vector<point_type> corridors;

        auto [nx, ny] = level.size();
        for (coord_type y = 0; y < ny; ++y)
        {
            for (coord_type x = 0; x < nx; ++x)
            {
                CellType type = level.cell(x, y).type();
                if (type == CellType::Room)
                {
                    rooms.push_back({x, y});
                }
                else if (type == CellType::Corridor)
                {
                    corridors.push_back({x, y});
                }
            }
        }

        return rooms.empty() ? corridors : rooms;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::dungen::Level quetzal::dungen::generate_grid1(id_type id, const string& name, const Parameters& parameters)
{
    assert(parameters.algorithm == Algorithm::Grid1);
    validate(parameters);

    const Grid1Parameters& grid1 = parameters.grid1;
    random_type random(parameters.seed.value());

    Level level(id, name, parameters.size, CellType::Solid);

    auto [divisions_x, divisions_y] = grid1.divisions;
    size_t n_sectors = static_cast<size_t>(divisions_x * divisions_y);

    // Which sectors hold a room, the density being how many of them do rather than the
    // chance of any one of them doing so, so that a level is not left to come out empty
    vector<size_t> slots(n_sectors);
    iota(slots.begin(), slots.end(), size_t(0));
    shuffle(slots.begin(), slots.end(), random.engine());

    size_t n_rooms = static_cast<size_t>(grid1.room_density * n_sectors + 0.5);

    vector<bool> room(n_sectors, false);
    for (size_t i = 0; i < n_rooms; ++i)
    {
        room[slots[i]] = true;
    }

    sectors_type sectors(n_sectors);
    for (coord_type j = 0; j < divisions_y; ++j)
    {
        for (coord_type i = 0; i < divisions_x; ++i)
        {
            size_t slot = static_cast<size_t>(j * divisions_x + i);
            sectors[slot] = place_sector(level, sector_bounds(parameters.size, grid1.divisions, i, j), grid1, room[slot], random);
        }
    }

    for (const Adjacency& connection : sector_connections(sector_adjacencies(grid1.divisions), n_sectors, grid1.loop, random))
    {
        connect(level, sectors[connection.slot_a], sectors[connection.slot_b], connection.horizontal, random);
    }

    place_stairs(level, stair_candidates(level), parameters.stair_placements, random);

    return level;
}
