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
#include <cstdlib>

using namespace std;
using namespace quetzal;
using namespace quetzal::catagen;

namespace
{

    using random_type = math::Random<>;
    using rectangles_type = vector<box_type>;
    using cell_region_mapping = vector<id_type>; // The region id for each cell in order
    using pair_type = pair<id_type, id_type>; // Two regions, lower id first
    using site_type = pair<point_type, CardinalDirection>; // A cell edge, addressed from one of its two sides
    using sites_type = map<pair_type, vector<site_type>>;

    //--------------------------------------------------------------------------
    coord_type area(const box_type& rectangle)
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
    // Disjoint sets over region ids, for asking whether two regions are reachable from one
    // another and for taking adjacencies that join two that are not yet
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
    // Splits along the longer side while a side exceeds the scale maximum and both halves
    // would still reach the minimum, so the leaves tile the rectangle exactly and no cell is
    // spent on the cut. A range too narrow to split into leaves rooms above the maximum
    void partition(const box_type& rectangle, const Parameters& parameters, random_type& random, rectangles_type& leaves)
    {
        coord_type nx = rectangle.extent().i();
        coord_type ny = rectangle.extent().j();

        // Rock left along the cut rather than taken out of either half, so the gap between
        // two rooms is what was asked for rather than the sum of two margins. Two leaves
        // that touch do so across the cut of their lowest common ancestor and no other, so
        // spacing every cut spaces every pair
        // Drawn only where the range offers a choice, so that a range of one -- no spacing,
        // the default -- leaves the random sequence where it was
        coord_type spacing = parameters.spacing.lower();
        if (parameters.spacing.proper())
        {
            spacing += random.gen(parameters.spacing.length() + 1);
        }

        coord_type span = 2 * parameters.scale.lower() + spacing; // What a side must have to be worth cutting

        bool split_x = nx > parameters.scale.upper() && nx >= span;
        bool split_y = ny > parameters.scale.upper() && ny >= span;

        if (!split_x && !split_y)
        {
            leaves.push_back(rectangle);
            return;
        }

        bool vertical = split_x && (!split_y || nx > ny || (nx == ny && random.gen(2) == 0));

        if (vertical)
        {
            coord_type cut = parameters.scale.lower() + random.gen(nx - spacing - 2 * parameters.scale.lower() + 1);
            partition({rectangle.position(), vector_type {cut, ny}}, parameters, random, leaves);
            partition({point_type {rectangle.position().i() + cut + spacing, rectangle.position().j()}, vector_type {nx - cut - spacing, ny}}, parameters, random, leaves);
        }
        else
        {
            coord_type cut = parameters.scale.lower() + random.gen(ny - spacing - 2 * parameters.scale.lower() + 1);
            partition({rectangle.position(), vector_type {nx, cut}}, parameters, random, leaves);
            partition({point_type {rectangle.position().i(), rectangle.position().j() + cut + spacing}, vector_type {nx, ny - cut - spacing}}, parameters, random, leaves);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Keeps leaves in a random order until the area asked for is covered. What is dropped
    // becomes rock, and the kept rooms are scattered rather than clustered, so joining them
    // is left to the corridors
    rectangles_type select(const rectangles_type& leaves, double density, random_type& random)
    {
        if (density >= 1.0)
        {
            return leaves;
        }

        coord_type area_total = 0;
        for (const box_type& leaf : leaves)
        {
            area_total += area(leaf);
        }

        coord_type area_target = static_cast<coord_type>(density * area_total);

        vector<size_t> order(leaves.size());
        iota(order.begin(), order.end(), size_t(0));
        shuffle(order.begin(), order.end(), random.engine());

        rectangles_type rooms;
        coord_type area_kept = 0;

        for (size_t slot : order)
        {
            if (!rooms.empty() && area_kept >= area_target)
            {
                break;
            }

            rooms.push_back(leaves[slot]);
            area_kept += area(leaves[slot]);
        }

        return rooms;
    }

    //--------------------------------------------------------------------------
    // A room's id is its position in the room list, so that a region id reaches back to the
    // rectangle it came from for as long as the corridors have not started numbering
    cell_region_mapping cell_regions(const vector_type& extent, const rectangles_type& rooms)
    {
        cell_region_mapping regions(static_cast<size_t>(extent.i()) * extent.j(), nullid);

        for (id_type id = 0; id < rooms.size(); ++id)
        {
            const box_type& room = rooms[id];

            for (coord_type j = room.lower(1); j < room.upper(1); ++j)
            {
                for (coord_type i = room.lower(0); i < room.upper(0); ++i)
                {
                    regions[cell_slot(extent, {i, j})] = id;
                }
            }
        }

        return regions;
    }

    //--------------------------------------------------------------------------
    // The cells an L shaped path passes through, turning once. Which leg comes first is the
    // caller's, so that corridors do not all bend the same way
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
    // Turns the rock a path crosses into corridor. Each unbroken run of rock becomes a
    // region of its own, so that a path crossing an existing room does not leave one region
    // in two disconnected pieces
    void carve(const vector_type& extent, const vector<point_type>& positions, cell_region_mapping& regions, id_type& region_count)
    {
        id_type region = nullid;

        for (const point_type& position : positions)
        {
            size_t slot = cell_slot(extent, position);

            if (regions[slot] != nullid)
            {
                region = nullid;
                continue;
            }

            if (region == nullid)
            {
                region = region_count++;
            }

            regions[slot] = region;
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Every cell edge with a different region on each side, gathered by the pair it
    // separates. Two regions are adjacent exactly when they have such an edge, and each one
    // is somewhere a door between them could go
    sites_type door_sites(const vector_type& extent, const cell_region_mapping& regions)
    {
        sites_type sites;

        for (coord_type j = 0; j < extent.j(); ++j)
        {
            for (coord_type i = 0; i < extent.i(); ++i)
            {
                id_type region = regions[cell_slot(extent, {i, j})];
                if (region == nullid)
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

                    id_type region_other = regions[cell_slot(extent, other)];
                    if (region_other == nullid || region_other == region)
                    {
                        continue;
                    }

                    pair_type key = region < region_other ? pair_type {region, region_other} : pair_type {region_other, region};
                    sites[key].push_back({{i, j}, direction});
                }
            }
        }

        return sites;
    }

    //--------------------------------------------------------------------------
    // Joins what selection left separated, one corridor at a time between the closest pair
    // of rooms still out of reach of each other. Components are recounted after each, so a
    // corridor that happens to join more than two is not followed by a redundant one
    void connect_components(const vector_type& extent, const rectangles_type& rooms, cell_region_mapping& regions,
        id_type& region_count, random_type& random)
    {
        for (size_t pass = 0; pass < rooms.size(); ++pass)
        {
            Components components(region_count);
            for (const auto& [key, sites] : door_sites(extent, regions))
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
                regions, region_count);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Cells and walls follow from the region map: a cell is floor where it belongs to a
    // region, and the edge between two cells is open only where both belong to the same one
    // A wall faces open floor on the side it belongs to, so an edge with floor on neither
    // side has none at all -- the outer boundary included, where the cell across is the
    // rock outside the level
    void realize(Level& level, const cell_region_mapping& regions)
    {
        const vector_type& extent = level.extent();

        auto region_of = [&](const point_type& position)
        {
            return contains(extent, position) ? regions[cell_slot(extent, position)] : nullid;
        };

        for (coord_type j = 0; j < extent.j(); ++j)
        {
            for (coord_type i = 0; i < extent.i(); ++i)
            {
                id_type region = region_of({i, j});
                level.set_cell({i, j}, region == nullid ? CellType::Solid : CellType::Open);

                for (CardinalDirection direction : {CardinalDirection::East, CardinalDirection::North, CardinalDirection::West, CardinalDirection::South})
                {
                    id_type other = region_of(neighbor({i, j}, direction));

                    bool joined = region != nullid && region == other;
                    bool faced = region != nullid || other != nullid;

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
    // A door on enough walls for every region to reach every other, by taking the
    // adjacencies in a random order and keeping the ones joining two regions not yet joined.
    // What that leaves over is where a loop can be added
    void place_doors(Level& level, const cell_region_mapping& regions, id_type region_count, double loop, random_type& random)
    {
        sites_type sites = door_sites(level.extent(), regions);

        vector<pair_type> keys;
        keys.reserve(sites.size());

        for (const auto& [key, edges] : sites)
        {
            keys.push_back(key);
        }

        shuffle(keys.begin(), keys.end(), random.engine());

        Components components(region_count);
        vector<pair_type> spare;

        for (const pair_type& key : keys)
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

        for (const pair_type& key : spare)
        {
            if (random.gen() < loop)
            {
                punch(level, sites[key], random);
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A range of one draws nothing, so a fixed count leaves every draw after it where it was
    size_t draw_count(const quetzal::math::Interval<size_t>& count, random_type& random)
    {
        return count.lower() == count.upper() ? count.lower() : random.gen(count.lower(), count.upper());
    }

    //--------------------------------------------------------------------------
    // Staircases at the centers of rooms, a room to each, those leading up first. Corridors are passed over, being what
    // joins rooms rather than somewhere to arrive, and running out of rooms places fewer, those leading down going short
    // before those leading up
    void place_staircases(Level& level, const rectangles_type& rooms, const Parameters& parameters, random_type& random)
    {
        size_t down_count = draw_count(parameters.staircase_down_count, random);
        size_t up_count = draw_count(parameters.staircase_up_count, random);

        // Every staircase is an entry, and a level needs one
        if (rooms.empty())
        {
            throw runtime_error("the level has no room a staircase can take, and so no entry");
        }

        vector<size_t> available(rooms.size());
        iota(available.begin(), available.end(), size_t(0));

        for (size_t i = 0; i < down_count + up_count && !available.empty(); ++i)
        {
            size_t slot = random.gen(available.size());
            level.set_cell(center(rooms[available[slot]]), i < up_count ? CellType::StairsUp : CellType::StairsDown);
            available.erase(available.begin() + static_cast<ptrdiff_t>(slot));
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
    // A mover coming through a door on any side of the cell steps straight onto it
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
    // Each staircase blocking a doorway moves one cell, in a random direction across an open wall onto floor that blocks no
    // doorway, and stays where there is no such direction. An open wall joins two cells of one region, so the staircase
    // stays in its own space
    void move_staircases_off_doorways(Level& level, random_type& random)
    {
        for (const point_type& cell : staircase_cells(level))
        {
            if (!blocks_doorway(level, cell))
            {
                continue;
            }

            vector<point_type> available;

            for (size_t index = 0; index < cardinal_directions.size(); ++index)
            {
                CardinalDirection direction = static_cast<CardinalDirection>(index);
                if (level.wall(cell, direction) != WallType::Open)
                {
                    continue;
                }

                point_type next = neighbor(cell, direction);
                if (level.cell(next) == CellType::Open && !blocks_doorway(level, next))
                {
                    available.push_back(next);
                }
            }

            if (available.empty())
            {
                continue;
            }

            point_type destination = available[random.gen(available.size())];
            level.set_cell(destination, level.cell(cell));
            level.set_cell(cell, CellType::Open);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A random facing for each staircase among the sides of its cell with no wall, or where there are none, with a door
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

                if (wall == WallType::Open)
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

    random_type random(*parameters.seed); // Engaged, validate having just said so

    rectangles_type leaves;
    partition({point_type {0, 0}, parameters.extent}, parameters, random, leaves);

    rectangles_type rooms = select(leaves, parameters.density, random);
    cell_region_mapping regions = cell_regions(parameters.extent, rooms);
    id_type region_count = rooms.size();

    connect_components(parameters.extent, rooms, regions, region_count, random);

    Level level(parameters.extent);
    realize(level, regions);
    place_doors(level, regions, region_count, parameters.loop, random);
    place_staircases(level, rooms, parameters, random);
    move_staircases_off_doorways(level, random);
    face_staircases(level, random);

    return level;
}
