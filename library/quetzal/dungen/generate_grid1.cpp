//------------------------------------------------------------------------------
// dungen
// generate_grid1.cpp
//------------------------------------------------------------------------------

#include "generate_grid1.hpp"
#include "place_staircases.hpp"
#include "remove_doors.hpp"
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
    // A sector's room, or the single cell through which a sector without one joins its neighbors
    struct Sector
    {
        box_type rectangle;
        bool room = false;
    };

    using sectors_type = vector<Sector>;

    //--------------------------------------------------------------------------
    // Two adjacent sectors and their orientation, which the slots alone leave ambiguous for a single column of sectors
    struct Adjacency
    {
        size_t slot_a = 0;
        size_t slot_b = 0;
        bool horizontal = false;
    };

    //--------------------------------------------------------------------------
    // Disjoint sets over sector slots
    class Components
    {
    public:

        explicit Components(size_t n);

        size_t root(size_t slot);

        // Joins the two sets and returns whether they were separate
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
            m_parents[slot] = m_parents[m_parents[slot]]; // Path halving
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
    // The half open cell range of sector (i, j), spreading the remainder of an uneven division across the sectors
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
    // A room inside the sector, or a single cell when room is false, with the margin clear on every side so corridors run
    // through solid cells and no two rooms share a wall
    Sector choose_sector(const box_type& bounds, const Grid1Parameters& grid1, bool room, random_type& random)
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

        return sector;
    }

    //--------------------------------------------------------------------------
    void write_sector(Zone& zone, const Sector& sector)
    {
        if (sector.room)
        {
            zone.create_room(sector.rectangle);
        }
        else
        {
            zone.cell(sector.rectangle.position()).set_type(CellType::Corridor);
        }

        return;
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
    // A random spanning tree over the sectors, plus each remaining adjacency with probability loop
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
    // Turns the solid cells along a horizontal or vertical line, ends included, into corridor
    void carve(Zone& zone, point_type from, point_type to)
    {
        coord_type dx = to.x() - from.x();
        coord_type dy = to.y() - from.y();
        assert(dx == 0 || dy == 0);

        coord_type steps = dx != 0 ? (dx > 0 ? dx : -dx) : (dy > 0 ? dy : -dy);
        vector_type step {dx > 0 ? 1 : (dx < 0 ? -1 : 0), dy > 0 ? 1 : (dy < 0 ? -1 : 0)};

        // I don't believe that this will create diagonal corridors correctly, it does not add an additional block when both dimensions increment at the same time ...
        // And, it looks like this will always create diagonal corridors no matter what the aspect ratio of the from, to difference is ...

        point_type position = from;
        for (coord_type i = 0; i <= steps; ++i)
        {
            if (zone.cell(position).type() == CellType::Solid)
            {
                zone.cell(position).set_type(CellType::Corridor);
            }

            position += step;
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Makes a door where a corridor enters a room, and leaves corridor where it enters a sector without one
    void place_door(Zone& zone, point_type position, bool room)
    {
        if (room && zone.cell(position).type() != CellType::Door)
        {
            zone.create_door(position);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Returns the wall of from that faces to, and the rows of to along which a corridor leaving by
    // that wall may run. The range is empty if to is one cell tall and lies too near that wall for
    // its one row to be clear of the doorway
    void around_rows(const Sector& from, const Sector& to, coord_type& wall_y, coord_type& y0, coord_type& y1)
    {
        bool above = to.rectangle.position().y() >= from.rectangle.upper().y();
        wall_y = above ? from.rectangle.upper().y() : from.rectangle.position().y() - 1;
        y0 = above ? max(to.rectangle.position().y(), wall_y + 1) : to.rectangle.position().y();
        y1 = above ? to.rectangle.upper().y() - 1 : min(to.rectangle.upper().y() - 1, wall_y - 1);
        return;
    }

    //--------------------------------------------------------------------------
    void around_columns(const Sector& from, const Sector& to, coord_type& wall_x, coord_type& x0, coord_type& x1)
    {
        bool right = to.rectangle.position().x() >= from.rectangle.upper().x();
        wall_x = right ? from.rectangle.upper().x() : from.rectangle.position().x() - 1;
        x0 = right ? max(to.rectangle.position().x(), wall_x + 1) : to.rectangle.position().x();
        x1 = right ? to.rectangle.upper().x() - 1 : min(to.rectangle.upper().x() - 1, wall_x - 1);
        return;
    }

    //--------------------------------------------------------------------------
    // Digs a corridor that leaves through the wall of from that faces to, turns alongside from, and
    // enters across to's near wall. Fails if to leaves no row for it
    bool connect_around_rows(Zone& zone, const Sector& from, const Sector& to, coord_type to_wall_x,
        random_type& random)
    {
        coord_type y = 0;
        coord_type y0 = 0;
        coord_type y1 = 0;
        around_rows(from, to, y, y0, y1);

        if (y0 > y1)
        {
            return false;
        }

        coord_type x = from.rectangle.position().x() + random.gen(from.rectangle.extent().x());
        coord_type ym = random.gen(y0, y1);

        carve(zone, {x, y}, {x, ym});
        carve(zone, {x, ym}, {to_wall_x, ym});

        place_door(zone, {x, y}, from.room);
        place_door(zone, {to_wall_x, ym}, to.room);
        return true;
    }

    //--------------------------------------------------------------------------
    bool connect_around_columns(Zone& zone, const Sector& from, const Sector& to, coord_type to_wall_y,
        random_type& random)
    {
        coord_type x = 0;
        coord_type x0 = 0;
        coord_type x1 = 0;
        around_columns(from, to, x, x0, x1);

        if (x0 > x1)
        {
            return false;
        }

        coord_type y = from.rectangle.position().y() + random.gen(from.rectangle.extent().y());
        coord_type xm = random.gen(x0, x1);

        carve(zone, {x, y}, {xm, y});
        carve(zone, {xm, y}, {xm, to_wall_y});

        place_door(zone, {x, y}, from.room);
        place_door(zone, {xm, to_wall_y}, to.room);
        return true;
    }

    //--------------------------------------------------------------------------
    // Returns true if a corridor can join the two with no doorway at a corner. The only pair that cannot be joined is two
    // rooms that are both one cell across and too near each other for a corridor to leave either by a side wall. A sector
    // without a room has no doorway
    bool joinable(const Sector& a, const Sector& b, bool horizontal)
    {
        if (!a.room || !b.room)
        {
            return true;
        }

        coord_type wall = 0;
        coord_type first = 0;
        coord_type last = 0;

        if (horizontal)
        {
            if (b.rectangle.position().x() - 1 - a.rectangle.upper().x() >= 2)
            {
                return true;
            }

            if (max(a.rectangle.position().y(), b.rectangle.position().y())
                <= min(a.rectangle.upper().y(), b.rectangle.upper().y()) - 1)
            {
                return true;
            }

            around_rows(a, b, wall, first, last);
            if (first <= last)
            {
                return true;
            }

            around_rows(b, a, wall, first, last);
            return first <= last;
        }

        if (b.rectangle.position().y() - 1 - a.rectangle.upper().y() >= 2)
        {
            return true;
        }

        if (max(a.rectangle.position().x(), b.rectangle.position().x())
            <= min(a.rectangle.upper().x(), b.rectangle.upper().x()) - 1)
        {
            return true;
        }

        around_columns(a, b, wall, first, last);
        if (first <= last)
        {
            return true;
        }

        around_columns(b, a, wall, first, last);
        return first <= last;
    }

    //--------------------------------------------------------------------------
    // Digs a corridor from one sector to the other, keeping its turn off both wall lines, since a doorway needs solid cells
    // on either side
    void connect(Zone& zone, const Sector& a, const Sector& b, bool horizontal, random_type& random)
    {
        if (horizontal)
        {
            coord_type xa = a.rectangle.upper().x(); // The wall cell east of a
            coord_type xb = b.rectangle.position().x() - 1; // The wall cell west of b

            if (xb - xa >= 2)
            {
                coord_type ya = a.rectangle.position().y() + random.gen(a.rectangle.extent().y());
                coord_type yb = b.rectangle.position().y() + random.gen(b.rectangle.extent().y());
                coord_type xm = random.gen(xa + 1, xb - 1); // Where it turns

                carve(zone, {xa, ya}, {xm, ya});
                carve(zone, {xm, ya}, {xm, yb});
                carve(zone, {xm, yb}, {xb, yb});

                place_door(zone, {xa, ya}, a.room);
                place_door(zone, {xb, yb}, b.room);
                return;
            }

            coord_type y0 = max(a.rectangle.position().y(), b.rectangle.position().y());
            coord_type y1 = min(a.rectangle.upper().y(), b.rectangle.upper().y()) - 1;

            if (y0 <= y1)
            {
                coord_type y = random.gen(y0, y1);

                carve(zone, {xa, y}, {xb, y});

                place_door(zone, {xa, y}, a.room);
                place_door(zone, {xb, y}, b.room);
                return;
            }

            if (connect_around_rows(zone, a, b, xb, random) || connect_around_rows(zone, b, a, xa, random))
            {
                return;
            }

            // The turn falls on a wall line here, making that end a corner, so it goes on the side without a room
            coord_type ya = a.rectangle.position().y() + random.gen(a.rectangle.extent().y());
            coord_type yb = b.rectangle.position().y() + random.gen(b.rectangle.extent().y());

            if (a.room)
            {
                carve(zone, {xa, ya}, {xb, ya});
                carve(zone, {xb, ya}, {xb, yb});
            }
            else
            {
                carve(zone, {xa, ya}, {xa, yb});
                carve(zone, {xa, yb}, {xb, yb});
            }

            place_door(zone, {xa, ya}, a.room);
            place_door(zone, {xb, yb}, b.room);
        }
        else
        {
            coord_type ya = a.rectangle.upper().y(); // The wall cell north of a
            coord_type yb = b.rectangle.position().y() - 1; // The wall cell south of b

            if (yb - ya >= 2)
            {
                coord_type xa = a.rectangle.position().x() + random.gen(a.rectangle.extent().x());
                coord_type xb = b.rectangle.position().x() + random.gen(b.rectangle.extent().x());
                coord_type ym = random.gen(ya + 1, yb - 1); // Where it turns

                carve(zone, {xa, ya}, {xa, ym});
                carve(zone, {xa, ym}, {xb, ym});
                carve(zone, {xb, ym}, {xb, yb});

                place_door(zone, {xa, ya}, a.room);
                place_door(zone, {xb, yb}, b.room);
                return;
            }

            coord_type x0 = max(a.rectangle.position().x(), b.rectangle.position().x());
            coord_type x1 = min(a.rectangle.upper().x(), b.rectangle.upper().x()) - 1;

            if (x0 <= x1)
            {
                coord_type x = random.gen(x0, x1);

                carve(zone, {x, ya}, {x, yb});

                place_door(zone, {x, ya}, a.room);
                place_door(zone, {x, yb}, b.room);
                return;
            }

            if (connect_around_columns(zone, a, b, yb, random) || connect_around_columns(zone, b, a, ya, random))
            {
                return;
            }

            // The turn falls on a wall line here, making that end a corner, so it goes on the side without a room
            coord_type xa = a.rectangle.position().x() + random.gen(a.rectangle.extent().x());
            coord_type xb = b.rectangle.position().x() + random.gen(b.rectangle.extent().x());

            if (a.room)
            {
                carve(zone, {xa, ya}, {xa, yb});
                carve(zone, {xa, yb}, {xb, yb});
            }
            else
            {
                carve(zone, {xa, ya}, {xb, ya});
                carve(zone, {xb, ya}, {xb, yb});
            }

            place_door(zone, {xa, ya}, a.room);
            place_door(zone, {xb, yb}, b.room);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A staircase can be placed in an open cell, room or corridor
    vector<point_type> staircase_candidates(const Zone& zone)
    {
        vector<point_type> rooms;
        vector<point_type> corridors;

        auto [nx, ny] = zone.extent();
        for (coord_type y = 0; y < ny; ++y)
        {
            for (coord_type x = 0; x < nx; ++x)
            {
                CellType type = zone.cell({x, y}).type();
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
quetzal::dungen::Zone quetzal::dungen::generate_grid1(id_type id, const string& name, const Parameters& parameters)
{
    assert(parameters.algorithm == Algorithm::Grid1);
    validate(parameters);

    const Grid1Parameters& grid1 = parameters.grid1;
    random_type random(parameters.seed.value());

    Zone zone(id, name, parameters.extent, CellType::Solid);

    auto [divisions_x, divisions_y] = grid1.divisions;
    size_t n_sectors = static_cast<size_t>(divisions_x * divisions_y);

    // Which sectors have a room. The density is a fraction of the sectors, not a per-sector probability, so a zone is never
    // left empty
    vector<size_t> slots(n_sectors);
    iota(slots.begin(), slots.end(), size_t(0));
    shuffle(slots.begin(), slots.end(), random.engine());

    size_t n_rooms = static_cast<size_t>(grid1.room_density * n_sectors + 0.5);

    vector<bool> room(n_sectors, false);
    for (size_t i = 0; i < n_rooms; ++i)
    {
        room[slots[i]] = true;
    }

    // A placement whose neighbor is reachable only through a corner doorway is drawn again. The attempts are bounded, as a
    // sector with no space to vary the draw would never settle
    constexpr size_t placement_attempts = 16;

    sectors_type sectors(n_sectors);
    for (coord_type j = 0; j < divisions_y; ++j)
    {
        for (coord_type i = 0; i < divisions_x; ++i)
        {
            size_t slot = static_cast<size_t>(j * divisions_x + i);
            box_type bounds = sector_bounds(parameters.extent, grid1.divisions, i, j);
            Sector sector;

            for (size_t attempt = 0; attempt < placement_attempts; ++attempt)
            {
                sector = choose_sector(bounds, grid1, room[slot], random);

                if ((i == 0 || joinable(sectors[slot - 1], sector, true))
                    && (j == 0 || joinable(sectors[slot - static_cast<size_t>(divisions_x)], sector, false)))
                {
                    break;
                }
            }

            sectors[slot] = sector;
            write_sector(zone, sector);
        }
    }

    for (const Adjacency& connection : sector_connections(sector_adjacencies(grid1.divisions), n_sectors, grid1.loop, random))
    {
        connect(zone, sectors[connection.slot_a], sectors[connection.slot_b], connection.horizontal, random);
    }

    place_staircases(zone, staircase_candidates(zone), parameters.staircase_up_count, parameters.staircase_down_count, random);
    remove_doors(zone, grid1.door_probability, random);

    return zone;
}
