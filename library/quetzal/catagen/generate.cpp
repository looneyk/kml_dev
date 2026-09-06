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
#include <utility>
#include <vector>
#include <cstdlib>

using namespace std;
using namespace quetzal;
using namespace quetzal::catagen;

namespace
{

    using random_type = math::Random<>;
    using rectangles_type = vector<Rectangle>;
    using cell_region_mapping = vector<id_type>; // The region id for each cell in order
    using pair_type = pair<id_type, id_type>; // Two regions, lower id first
    using site_type = pair<CellPosition, Direction>; // A cell edge, addressed from one of its two sides
    using sites_type = map<pair_type, vector<site_type>>;

    //--------------------------------------------------------------------------
    coord_type area(const Rectangle& rectangle)
    {
        return rectangle.extent.nx * rectangle.extent.ny;
    }

    //--------------------------------------------------------------------------
    CellPosition center(const Rectangle& rectangle)
    {
        return {rectangle.position.i + rectangle.extent.nx / 2, rectangle.position.j + rectangle.extent.ny / 2};
    }

    //--------------------------------------------------------------------------
    bool contains(const Extent& extent, const CellPosition& position)
    {
        return position.i >= 0 && position.j >= 0 && position.i < extent.nx && position.j < extent.ny;
    }

    //--------------------------------------------------------------------------
    size_t cell_slot(const Extent& extent, const CellPosition& position)
    {
        return static_cast<size_t>(position.j) * extent.nx + position.i;
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
    void partition(const Rectangle& rectangle, const Parameters& parameters, random_type& random, rectangles_type& leaves)
    {
        coord_type nx = rectangle.extent.nx;
        coord_type ny = rectangle.extent.ny;

        // Rock left along the cut rather than taken out of either half, so the gap between
        // two rooms is what was asked for rather than the sum of two margins. Two leaves
        // that touch do so across the cut of their lowest common ancestor and no other, so
        // spacing every cut spaces every pair
        // Drawn only where the range offers a choice, so that a range of one -- no spacing,
        // the default -- leaves the random sequence where it was
        coord_type spacing = parameters.spacing_min;
        if (parameters.spacing_max > parameters.spacing_min)
        {
            spacing += random.gen(parameters.spacing_max - parameters.spacing_min + 1);
        }

        coord_type span = 2 * parameters.scale_min + spacing; // What a side must have to be worth cutting

        bool split_x = nx > parameters.scale_max && nx >= span;
        bool split_y = ny > parameters.scale_max && ny >= span;

        if (!split_x && !split_y)
        {
            leaves.push_back(rectangle);
            return;
        }

        bool vertical = split_x && (!split_y || nx > ny || (nx == ny && random.gen(2) == 0));

        if (vertical)
        {
            coord_type cut = parameters.scale_min + random.gen(nx - spacing - 2 * parameters.scale_min + 1);
            partition({rectangle.position, {cut, ny}}, parameters, random, leaves);
            partition({{rectangle.position.i + cut + spacing, rectangle.position.j}, {nx - cut - spacing, ny}}, parameters, random, leaves);
        }
        else
        {
            coord_type cut = parameters.scale_min + random.gen(ny - spacing - 2 * parameters.scale_min + 1);
            partition({rectangle.position, {nx, cut}}, parameters, random, leaves);
            partition({{rectangle.position.i, rectangle.position.j + cut + spacing}, {nx, ny - cut - spacing}}, parameters, random, leaves);
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
        for (const Rectangle& leaf : leaves)
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
    cell_region_mapping cell_regions(const Extent& extent, const rectangles_type& rooms)
    {
        cell_region_mapping regions(static_cast<size_t>(extent.nx) * extent.ny, nullid);

        for (id_type id = 0; id < rooms.size(); ++id)
        {
            const Rectangle& room = rooms[id];

            for (coord_type j = room.position.j; j < room.position.j + room.extent.ny; ++j)
            {
                for (coord_type i = room.position.i; i < room.position.i + room.extent.nx; ++i)
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
    vector<CellPosition> path_cells(const CellPosition& from, const CellPosition& to, bool horizontal_first)
    {
        vector<CellPosition> positions;
        CellPosition corner = horizontal_first ? CellPosition {to.i, from.j} : CellPosition {from.i, to.j};

        auto run = [&positions](const CellPosition& a, const CellPosition& b)
        {
            coord_type di = b.i > a.i ? 1 : (b.i < a.i ? -1 : 0);
            coord_type dj = b.j > a.j ? 1 : (b.j < a.j ? -1 : 0);

            for (CellPosition c = a; c != b; c.i += di, c.j += dj)
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
    void carve(const Extent& extent, const vector<CellPosition>& positions, cell_region_mapping& regions, id_type& region_count)
    {
        id_type region = nullid;

        for (const CellPosition& position : positions)
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
    sites_type door_sites(const Extent& extent, const cell_region_mapping& regions)
    {
        sites_type sites;

        for (coord_type j = 0; j < extent.ny; ++j)
        {
            for (coord_type i = 0; i < extent.nx; ++i)
            {
                id_type region = regions[cell_slot(extent, {i, j})];
                if (region == nullid)
                {
                    continue;
                }

                for (Direction direction : {Direction::East, Direction::North})
                {
                    CellPosition other = neighbor({i, j}, direction);
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
    void connect_components(const Extent& extent, const rectangles_type& rooms, cell_region_mapping& regions,
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

                    CellPosition center_a = center(rooms[a]);
                    CellPosition center_b = center(rooms[b]);
                    coord_type distance = abs(center_a.i - center_b.i) + abs(center_a.j - center_b.j);

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
        const Extent& extent = level.extent();

        auto region_of = [&](const CellPosition& position)
        {
            return contains(extent, position) ? regions[cell_slot(extent, position)] : nullid;
        };

        for (coord_type j = 0; j < extent.ny; ++j)
        {
            for (coord_type i = 0; i < extent.nx; ++i)
            {
                id_type region = region_of({i, j});
                level.set_cell({i, j}, region == nullid ? CellType::Solid : CellType::Open);

                for (Direction direction : {Direction::East, Direction::North, Direction::West, Direction::South})
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
    // Stairs down and up at the centres of two different rooms. Corridors are passed over,
    // being what joins rooms rather than somewhere to arrive
    void place_stairs(Level& level, const rectangles_type& rooms, random_type& random)
    {
        if (rooms.empty())
        {
            return;
        }

        size_t down = random.gen(rooms.size());
        level.set_cell(center(rooms[down]), CellType::StairsDown);

        if (rooms.size() < 2)
        {
            return;
        }

        size_t up = random.gen(rooms.size() - 1);
        if (up >= down)
        {
            ++up;
        }

        level.set_cell(center(rooms[up]), CellType::StairsUp);
        return;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::catagen::Level quetzal::catagen::generate(const Parameters& parameters)
{
    validate(parameters);

    random_type random(*parameters.seed); // Engaged, validate having just said so

    rectangles_type leaves;
    partition({{0, 0}, parameters.extent}, parameters, random, leaves);

    rectangles_type rooms = select(leaves, parameters.density, random);
    cell_region_mapping regions = cell_regions(parameters.extent, rooms);
    id_type region_count = rooms.size();

    connect_components(parameters.extent, rooms, regions, region_count, random);

    Level level(parameters.extent);
    realize(level, regions);
    place_doors(level, regions, region_count, parameters.loop, random);
    place_stairs(level, rooms, random);

    return level;
}
