//------------------------------------------------------------------------------
// catagen
// generate.cpp
//------------------------------------------------------------------------------

#include "generate.hpp"
#include "quetzal/common/id.hpp"
#include "quetzal/math/Random.hpp"
#include <algorithm>
#include <map>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>
#include <cassert>
#include <cstdlib>

using namespace std;
using namespace quetzal;
using namespace quetzal::catagen;

namespace
{

    using random_type = math::Random<>;
    using rectangles_type = vector<box_type>;

    // An area is floor that walls separate and doors join: a room, or one run of corridor
    using cell_areas_type = vector<id_type>; // The area id of each cell, in slot order
    using area_pair_type = pair<id_type, id_type>; // Two areas, lower id first
    using site_type = pair<point_type, CardinalDirection>; // A cell edge, addressed from one of its two sides
    using sites_type = map<area_pair_type, vector<site_type>>;

    //--------------------------------------------------------------------------
    coord_type cell_count(const box_type& rectangle)
    {
        return rectangle.volume();
    }

    //--------------------------------------------------------------------------
    point_type center(const box_type& rectangle)
    {
        return rectangle.centroid();
    }

    //--------------------------------------------------------------------------
    bool contains(const vector_type& extent, const point_type& position)
    {
        return position.i() >= 0 && position.j() >= 0 && position.i() < extent.i() && position.j() < extent.j();
    }

    //--------------------------------------------------------------------------
    size_t cell_slot(const vector_type& extent, const point_type& position)
    {
        return static_cast<size_t>(position.j()) * extent.i() + position.i();
    }

    //--------------------------------------------------------------------------
    // Disjoint sets over area ids
    class Components
    {
    public:

        explicit Components(size_t n) :
            m_parents(n)
        {
            iota(m_parents.begin(), m_parents.end(), size_t(0));
        }

        size_t root(size_t i)
        {
            while (m_parents[i] != i)
            {
                m_parents[i] = m_parents[m_parents[i]];
                i = m_parents[i];
            }

            return i;
        }

        bool merge(size_t a, size_t b)
        {
            size_t root_a = root(a);
            size_t root_b = root(b);

            if (root_a == root_b)
            {
                return false;
            }

            m_parents[root_b] = root_a;
            return true;
        }

    private:

        vector<size_t> m_parents;
    };

    //--------------------------------------------------------------------------
    // Splits along the longer side while a side exceeds the scale maximum and both halves would still reach the minimum. A
    // range too narrow to split into yields rectangles above the maximum
    void partition(const box_type& rectangle, const Parameters& parameters, random_type& random, rectangles_type& rectangles)
    {
        coord_type nx = rectangle.extent().i();
        coord_type ny = rectangle.extent().j();

        // Solid cells along the cut, so the gap between two rooms is the spacing drawn. Two rectangles that touch do so
        // across the cut of their lowest common ancestor alone, so spacing every cut spaces every pair
        // Drawn only when the range has more than one value, so the default keeps the random sequence unchanged
        coord_type spacing = parameters.spacing.lower();
        if (parameters.spacing.proper())
        {
            spacing += random.gen(parameters.spacing.length() + 1);
        }

        coord_type span = 2 * parameters.scale.lower() + spacing; // The shortest side that can be cut

        bool split_x = nx > parameters.scale.upper() && nx >= span;
        bool split_y = ny > parameters.scale.upper() && ny >= span;

        if (!split_x && !split_y)
        {
            rectangles.push_back(rectangle);
            return;
        }

        bool vertical = split_x && (!split_y || nx > ny || (nx == ny && random.gen(2) == 0));

        if (vertical)
        {
            coord_type cut = parameters.scale.lower() + random.gen(nx - spacing - 2 * parameters.scale.lower() + 1);
            partition({rectangle.position(), vector_type {cut, ny}}, parameters, random, rectangles);
            partition({point_type {rectangle.position().i() + cut + spacing, rectangle.position().j()}, vector_type {nx - cut - spacing, ny}}, parameters, random, rectangles);
        }
        else
        {
            coord_type cut = parameters.scale.lower() + random.gen(ny - spacing - 2 * parameters.scale.lower() + 1);
            partition({rectangle.position(), vector_type {nx, cut}}, parameters, random, rectangles);
            partition({point_type {rectangle.position().i(), rectangle.position().j() + cut + spacing}, vector_type {nx, ny - cut - spacing}}, parameters, random, rectangles);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // The rooms: rectangles kept in a random order until their cells reach the density. The rest become solid
    rectangles_type select(const rectangles_type& rectangles, double density, random_type& random)
    {
        if (density >= 1.0)
        {
            return rectangles;
        }

        coord_type total_cell_count = 0;
        for (const box_type& rectangle : rectangles)
        {
            total_cell_count += cell_count(rectangle);
        }

        coord_type target_cell_count = static_cast<coord_type>(density * total_cell_count);

        vector<size_t> order(rectangles.size());
        iota(order.begin(), order.end(), size_t(0));
        shuffle(order.begin(), order.end(), random.engine());

        rectangles_type rooms;
        coord_type kept_cell_count = 0;

        for (size_t slot : order)
        {
            if (!rooms.empty() && kept_cell_count >= target_cell_count)
            {
                break;
            }

            rooms.push_back(rectangles[slot]);
            kept_cell_count += cell_count(rectangles[slot]);
        }

        return rooms;
    }

    //--------------------------------------------------------------------------
    // Makes each room an area of its own, whose area id is its position in the room list
    cell_areas_type cell_areas(const vector_type& extent, const rectangles_type& rooms)
    {
        cell_areas_type areas(static_cast<size_t>(extent.i()) * extent.j(), nullid);

        for (id_type id = 0; id < rooms.size(); ++id)
        {
            const box_type& room = rooms[id];

            for (coord_type j = room.lower(1); j < room.upper(1); ++j)
            {
                for (coord_type i = room.lower(0); i < room.upper(0); ++i)
                {
                    areas[cell_slot(extent, {i, j})] = id;
                }
            }
        }

        return areas;
    }

    //--------------------------------------------------------------------------
    // The cells an L shaped path passes through, turning once
    vector<point_type> path_cells(const point_type& from, const point_type& to, bool horizontal_first)
    {
        vector<point_type> positions;
        point_type corner = horizontal_first ? point_type {to.i(), from.j()} : point_type {from.i(), to.j()};

        auto run = [&positions](const point_type& a, const point_type& b)
        {
            coord_type di = b.i() > a.i() ? 1 : (b.i() < a.i() ? -1 : 0);
            coord_type dj = b.j() > a.j() ? 1 : (b.j() < a.j() ? -1 : 0);

            for (point_type c = a; c != b; c.i() += di, c.j() += dj)
            {
                positions.push_back(c);
            }
        };

        run(from, corner);
        run(corner, to);
        positions.push_back(to);

        return positions;
    }

    //--------------------------------------------------------------------------
    // Turns the solid cells a path crosses into corridor, each unbroken run an area of its own, so a path crossing a room
    // does not split one area into disconnected pieces
    void carve(const vector_type& extent, const vector<point_type>& positions, cell_areas_type& areas, id_type& area_count)
    {
        id_type area = nullid;

        for (const point_type& position : positions)
        {
            size_t slot = cell_slot(extent, position);

            if (areas[slot] != nullid)
            {
                area = nullid;
                continue;
            }

            if (area == nullid)
            {
                area = area_count++;
            }

            areas[slot] = area;
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Every cell edge with a different area on each side, grouped by the pair of areas. Each is a possible door site
    sites_type door_sites(const vector_type& extent, const cell_areas_type& areas)
    {
        sites_type sites;

        for (coord_type j = 0; j < extent.j(); ++j)
        {
            for (coord_type i = 0; i < extent.i(); ++i)
            {
                id_type area = areas[cell_slot(extent, {i, j})];
                if (area == nullid)
                {
                    continue;
                }

                for (CardinalDirection direction : {CardinalDirection::East, CardinalDirection::North})
                {
                    point_type other = neighbor({i, j}, direction);
                    if (!contains(extent, other))
                    {
                        continue;
                    }

                    id_type area_other = areas[cell_slot(extent, other)];
                    if (area_other == nullid || area_other == area)
                    {
                        continue;
                    }

                    area_pair_type key = area < area_other ? area_pair_type {area, area_other} : area_pair_type {area_other, area};
                    sites[key].push_back({{i, j}, direction});
                }
            }
        }

        return sites;
    }

    //--------------------------------------------------------------------------
    // Joins the rooms selection left separated, one corridor at a time between the closest unconnected pair. Components
    // are recounted after each, so a corridor joining more than two is not followed by a redundant one
    void connect_components(const vector_type& extent, const rectangles_type& rooms, cell_areas_type& areas,
        id_type& area_count, random_type& random)
    {
        for (size_t pass = 0; pass < rooms.size(); ++pass)
        {
            Components components(area_count);
            for (const auto& [key, _] : door_sites(extent, areas))
            {
                components.merge(key.first, key.second);
            }

            id_type room_a = nullid;
            id_type room_b = nullid;
            coord_type separation = 0;

            for (id_type a = 0; a < rooms.size(); ++a)
            {
                for (id_type b = a + 1; b < rooms.size(); ++b)
                {
                    if (components.root(a) == components.root(b))
                    {
                        continue;
                    }

                    point_type center_a = center(rooms[a]);
                    point_type center_b = center(rooms[b]);
                    coord_type distance = abs(center_a.i() - center_b.i()) + abs(center_a.j() - center_b.j());

                    if (room_a == nullid || distance < separation)
                    {
                        room_a = a;
                        room_b = b;
                        separation = distance;
                    }
                }
            }

            if (room_a == nullid)
            {
                return; // Every room reaches every other
            }

            carve(extent, path_cells(center(rooms[room_a]), center(rooms[room_b]), random.gen(2) == 0),
                areas, area_count);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A cell is floor where it belongs to an area, and an edge is open only between two cells of the same area. An edge
    // with floor on neither side has no wall, the edges beyond the zone included
    void realize(Level& level, const cell_areas_type& areas)
    {
        const vector_type& extent = level.extent();

        auto area_of = [&](const point_type& position)
        {
            return contains(extent, position) ? areas[cell_slot(extent, position)] : nullid;
        };

        for (coord_type j = 0; j < extent.j(); ++j)
        {
            for (coord_type i = 0; i < extent.i(); ++i)
            {
                id_type area = area_of({i, j});
                level.set_cell({i, j}, area == nullid ? CellType::Solid : CellType::Open);

                for (CardinalDirection direction : {CardinalDirection::East, CardinalDirection::North, CardinalDirection::West, CardinalDirection::South})
                {
                    id_type other = area_of(neighbor({i, j}, direction));

                    bool joined = area != nullid && area == other;
                    bool faced = area != nullid || other != nullid;

                    level.set_wall({i, j}, direction, !joined && faced ? WallType::Solid : WallType::Open);
                }
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    void punch(Level& level, const vector<site_type>& sites, random_type& random)
    {
        const site_type& site = sites[random.gen(sites.size())];
        level.set_wall(site.first, site.second, WallType::Door);
        return;
    }

    //--------------------------------------------------------------------------
    // Adds a door, in random order, for each adjacency that joins two areas not yet joined, then a door for each remaining
    // adjacency with probability loop
    void place_doors(Level& level, const cell_areas_type& areas, id_type area_count, double loop, random_type& random)
    {
        sites_type sites = door_sites(level.extent(), areas);

        vector<area_pair_type> keys;
        keys.reserve(sites.size());

        for (const auto& [key, _] : sites)
        {
            keys.push_back(key);
        }

        shuffle(keys.begin(), keys.end(), random.engine());

        Components components(area_count);
        vector<area_pair_type> spare;

        for (const area_pair_type& key : keys)
        {
            if (components.merge(key.first, key.second))
            {
                punch(level, sites[key], random);
            }
            else
            {
                spare.push_back(key);
            }
        }

        for (const area_pair_type& key : spare)
        {
            if (random.gen() < loop)
            {
                punch(level, sites[key], random);
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A range of one value draws nothing, so a fixed count leaves every later draw unchanged
    size_t draw_count(const quetzal::math::Interval<size_t>& count, random_type& random)
    {
        return count.lower() == count.upper() ? count.lower() : random.gen(count.lower(), count.upper());
    }

    //--------------------------------------------------------------------------
    bool staircase(CellType type)
    {
        return type == CellType::StairsDown || type == CellType::StairsUp;
    }

    //--------------------------------------------------------------------------
    // Returns true if the side leads onto an open cell across an open wall or a door, since a door is passable once opened
    bool passable_side(const Level& level, const point_type& cell, CardinalDirection direction)
    {
        point_type next = neighbor(cell, direction);
        return level.wall(cell, direction) != WallType::Solid && level.contains(next) && level.cell(next) == CellType::Open;
    }

    //--------------------------------------------------------------------------
    // Returns true if a staircase's block can go at an open cell. The block needs a passable side to face, each staircase
    // next to it must keep a passable side, and the cells across the block's passable sides must still reach one another around it
    bool block_allowed(const Level& level, const point_type& cell)
    {
        assert(level.cell(cell) == CellType::Open);

        auto has_open_side = [&](const point_type& from)
        {
            for (size_t index = 0; index < cardinal_directions.size(); ++index)
            {
                CardinalDirection direction = static_cast<CardinalDirection>(index);
                if (neighbor(from, direction) != cell && passable_side(level, from, direction))
                {
                    return true;
                }
            }

            return false;
        };

        vector<point_type> sides;

        for (size_t index = 0; index < cardinal_directions.size(); ++index)
        {
            CardinalDirection direction = static_cast<CardinalDirection>(index);
            point_type next = neighbor(cell, direction);

            if (passable_side(level, cell, direction))
            {
                sides.push_back(next);
            }
            else if (level.contains(next) && staircase(level.cell(next)) && !has_open_side(next))
            {
                return false;
            }
        }

        if (sides.empty())
        {
            return false;
        }

        const vector_type& extent = level.extent();
        vector<bool> reached(static_cast<size_t>(extent.i()) * extent.j(), false);
        reached[cell_slot(extent, sides.front())] = true;

        vector<point_type> pending {sides.front()};
        size_t sides_unreached = sides.size() - 1;

        while (!pending.empty() && sides_unreached > 0)
        {
            point_type position = pending.back();
            pending.pop_back();

            for (size_t index = 0; index < cardinal_directions.size(); ++index)
            {
                CardinalDirection direction = static_cast<CardinalDirection>(index);
                point_type next = neighbor(position, direction);

                if (next == cell || !passable_side(level, position, direction) || reached[cell_slot(extent, next)])
                {
                    continue;
                }

                reached[cell_slot(extent, next)] = true;
                pending.push_back(next);

                if (find(sides.begin(), sides.end(), next) != sides.end())
                {
                    --sides_unreached;
                }
            }
        }

        return sides_unreached == 0;
    }

    //--------------------------------------------------------------------------
    // Places staircases at the centers of rooms, one per room, those leading up first. A room is passed over if a staircase
    // at its center would cut a passage or have no side to face. Running out of rooms places fewer staircases, with those
    // leading down falling short first
    void place_staircases(Level& level, const rectangles_type& rooms, const Parameters& parameters, random_type& random)
    {
        size_t down_count = draw_count(parameters.staircase_down_count, random);
        size_t up_count = draw_count(parameters.staircase_up_count, random);

        // Every staircase is an access point, and a zone requires one
        if (rooms.empty())
        {
            throw runtime_error("the level has no room a staircase can take, and so no entry");
        }

        vector<size_t> available(rooms.size());
        iota(available.begin(), available.end(), size_t(0));

        size_t placed = 0;

        while (placed < down_count + up_count && !available.empty())
        {
            size_t slot = random.gen(available.size());
            point_type cell = center(rooms[available[slot]]);
            available.erase(available.begin() + static_cast<ptrdiff_t>(slot));

            if (block_allowed(level, cell))
            {
                level.set_cell(cell, placed < up_count ? CellType::StairsUp : CellType::StairsDown);
                ++placed;
            }
        }

        if (placed == 0)
        {
            throw runtime_error("the level has no room a staircase can take without closing off a passage, and so no entry");
        }

        return;
    }

    //--------------------------------------------------------------------------
    vector<point_type> staircase_cells(const Level& level)
    {
        vector<point_type> cells;

        for (coord_type j = 0; j < level.extent().j(); ++j)
        {
            for (coord_type i = 0; i < level.extent().i(); ++i)
            {
                CellType type = level.cell({i, j});
                if (type == CellType::StairsDown || type == CellType::StairsUp)
                {
                    cells.push_back({i, j});
                }
            }
        }

        return cells;
    }

    //--------------------------------------------------------------------------
    // Returns true if a door on any side of the cell opens onto it
    bool blocks_doorway(const Level& level, const point_type& cell)
    {
        for (size_t index = 0; index < cardinal_directions.size(); ++index)
        {
            if (level.wall(cell, static_cast<CardinalDirection>(index)) == WallType::Door)
            {
                return true;
            }
        }

        return false;
    }

    //--------------------------------------------------------------------------
    // Moves each staircase that blocks a doorway one cell in a random direction, across an open wall onto floor where it
    // blocks no doorway and its block is allowed. A staircase with no such direction stays where it is. An open wall joins
    // two cells of one area, so the staircase stays in its own area
    void move_staircases_off_doorways(Level& level, random_type& random)
    {
        for (const point_type& cell : staircase_cells(level))
        {
            if (!blocks_doorway(level, cell))
            {
                continue;
            }

            vector<point_type> available;
            CellType type = level.cell(cell);

            // Floor again while its destination is chosen, so the block there is judged without this one
            level.set_cell(cell, CellType::Open);

            for (size_t index = 0; index < cardinal_directions.size(); ++index)
            {
                CardinalDirection direction = static_cast<CardinalDirection>(index);
                if (level.wall(cell, direction) != WallType::Open)
                {
                    continue;
                }

                point_type next = neighbor(cell, direction);
                if (level.cell(next) == CellType::Open && !blocks_doorway(level, next) && block_allowed(level, next))
                {
                    available.push_back(next);
                }
            }

            if (available.empty())
            {
                level.set_cell(cell, type);
                continue;
            }

            point_type destination = available[random.gen(available.size())];
            level.set_cell(destination, type);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Gives each staircase a random facing direction among the sides of its cell that have no wall and floor beyond, or, if
    // there are none, among the sides with a door
    void face_staircases(Level& level, random_type& random)
    {
        for (const point_type& cell : staircase_cells(level))
        {
            vector<CardinalDirection> open;
            vector<CardinalDirection> doors;

            for (size_t index = 0; index < cardinal_directions.size(); ++index)
            {
                CardinalDirection direction = static_cast<CardinalDirection>(index);
                WallType wall = level.wall(cell, direction);

                if (wall == WallType::Open && level.cell(neighbor(cell, direction)) == CellType::Open)
                {
                    open.push_back(direction);
                }
                else if (wall == WallType::Door)
                {
                    doors.push_back(direction);
                }
            }

            const vector<CardinalDirection>& sides = open.empty() ? doors : open;
            if (!sides.empty())
            {
                level.set_staircase_facing(cell, sides[random.gen(sides.size())]);
            }
        }

        return;
    }
} // namespace

//------------------------------------------------------------------------------
quetzal::catagen::Level quetzal::catagen::generate(const Parameters& parameters)
{
    validate(parameters);

    random_type random(*parameters.seed); // validate checked it is set

    rectangles_type rectangles;
    partition({point_type {0, 0}, parameters.extent}, parameters, random, rectangles);

    rectangles_type rooms = select(rectangles, parameters.density, random);
    cell_areas_type areas = cell_areas(parameters.extent, rooms);
    id_type area_count = rooms.size();

    connect_components(parameters.extent, rooms, areas, area_count, random);

    Level level(parameters.extent);
    realize(level, areas);
    place_doors(level, areas, area_count, parameters.loop, random);
    place_staircases(level, rooms, parameters, random);
    move_staircases_off_doorways(level, random);
    face_staircases(level, random);

    return level;
}
