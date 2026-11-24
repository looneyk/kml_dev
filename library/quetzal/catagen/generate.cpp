//------------------------------------------------------------------------------
// catagen
// generate.cpp
//------------------------------------------------------------------------------

#include "generate.hpp"
#include "quetzal/common/id.hpp"
#include <algorithm>
#include <map>
#include <numeric>
#include <optional>
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

    using rectangles_type = vector<discrete_box2_type>;

    // An area is floor that walls separate and doors join: a room, or one run of corridor
    using cell_areas_type = vector<id_type>; // The area id of each cell, in slot order
    using area_pair_type = pair<id_type, id_type>; // Two areas, lower id first
    using site_type = pair<discrete_point2_type, CardinalDirection>; // A cell edge, addressed from one of its two sides
    using sites_type = map<area_pair_type, vector<site_type>>;

    //--------------------------------------------------------------------------
    coord_type cell_count(const discrete_box2_type& rectangle)
    {
        return rectangle.volume();
    }

    //--------------------------------------------------------------------------
    discrete_point2_type center(const discrete_box2_type& rectangle)
    {
        return rectangle.centroid();
    }

    //--------------------------------------------------------------------------
    bool contains(const discrete_vector2_type& extent, const discrete_point2_type& position)
    {
        return position.x() >= 0 && position.y() >= 0 && position.x() < extent.x() && position.y() < extent.y();
    }

    //--------------------------------------------------------------------------
    size_t cell_slot(const discrete_vector2_type& extent, const discrete_point2_type& position)
    {
        return static_cast<size_t>(position.y()) * extent.x() + position.x();
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
    void partition(const discrete_box2_type& rectangle, const Parameters& parameters, random_type& random, rectangles_type& rectangles)
    {
        coord_type nx = rectangle.extent().x();
        coord_type ny = rectangle.extent().y();

        // Solid cells along the cut, so the gap between two rooms is the spacing drawn. Two rectangles that touch do so
        // across the cut of their lowest common ancestor alone, so spacing every cut spaces every pair
        // Drawn only when the range has more than one value, so the default keeps the random sequence unchanged
        coord_type spacing = parameters.spacing.lower();
        if (parameters.spacing.proper())
        {
            spacing += random.random(parameters.spacing.length() + 1);
        }

        coord_type span = 2 * parameters.scale.lower() + spacing; // The shortest side that can be cut

        bool split_x = nx > parameters.scale.upper() && nx >= span;
        bool split_y = ny > parameters.scale.upper() && ny >= span;

        if (!split_x && !split_y)
        {
            rectangles.push_back(rectangle);
            return;
        }

        bool vertical = split_x && (!split_y || nx > ny || (nx == ny && random.random(2) == 0));

        if (vertical)
        {
            coord_type cut = parameters.scale.lower() + random.random(nx - spacing - 2 * parameters.scale.lower() + 1);
            partition({rectangle.position(), discrete_vector2_type{cut, ny}}, parameters, random, rectangles);
            partition({discrete_point2_type{rectangle.position().x() + cut + spacing, rectangle.position().y()}, discrete_vector2_type{nx - cut - spacing, ny}}, parameters, random, rectangles);
        }
        else
        {
            coord_type cut = parameters.scale.lower() + random.random(ny - spacing - 2 * parameters.scale.lower() + 1);
            partition({rectangle.position(), discrete_vector2_type{nx, cut}}, parameters, random, rectangles);
            partition({discrete_point2_type{rectangle.position().x(), rectangle.position().y() + cut + spacing}, discrete_vector2_type{nx, ny - cut - spacing}}, parameters, random, rectangles);
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
        for (const discrete_box2_type& rectangle : rectangles)
        {
            total_cell_count += cell_count(rectangle);
        }

        coord_type target_cell_count = static_cast<coord_type>(density * total_cell_count);

        vector<size_t> order(rectangles.size());
        iota(order.begin(), order.end(), size_t(0));
        random.shuffle(order);

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
    cell_areas_type cell_areas(const discrete_vector2_type& extent, const rectangles_type& rooms)
    {
        cell_areas_type areas(static_cast<size_t>(extent.x()) * extent.y(), nullid);

        for (id_type id = 0; id < rooms.size(); ++id)
        {
            const discrete_box2_type& room = rooms[id];

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
    vector<discrete_point2_type> path_cells(const discrete_point2_type& from, const discrete_point2_type& to, bool horizontal_first)
    {
        vector<discrete_point2_type> positions;
        discrete_point2_type corner = horizontal_first ? discrete_point2_type{to.x(), from.y()} : discrete_point2_type{from.x(), to.y()};

        auto run = [&positions](const discrete_point2_type& a, const discrete_point2_type& b)
        {
            coord_type di = b.x() > a.x() ? 1 : (b.x() < a.x() ? -1 : 0);
            coord_type dj = b.y() > a.y() ? 1 : (b.y() < a.y() ? -1 : 0);

            for (discrete_point2_type c = a; c != b; c.x() += di, c.y() += dj)
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
    void carve(const discrete_vector2_type& extent, const vector<discrete_point2_type>& positions, cell_areas_type& areas, id_type& area_count)
    {
        id_type area = nullid;

        for (const discrete_point2_type& position : positions)
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
    sites_type door_sites(const discrete_vector2_type& extent, const cell_areas_type& areas)
    {
        sites_type sites;

        for (coord_type j = 0; j < extent.y(); ++j)
        {
            for (coord_type i = 0; i < extent.x(); ++i)
            {
                id_type area = areas[cell_slot(extent, {i, j})];
                if (area == nullid)
                {
                    continue;
                }

                for (CardinalDirection direction : {CardinalDirection::East, CardinalDirection::North})
                {
                    discrete_point2_type other = neighbor({i, j}, direction);
                    if (!contains(extent, other))
                    {
                        continue;
                    }

                    id_type area_other = areas[cell_slot(extent, other)];
                    if (area_other == nullid || area_other == area)
                    {
                        continue;
                    }

                    area_pair_type key = area < area_other ? area_pair_type{area, area_other} : area_pair_type{area_other, area};
                    sites[key].push_back({{i, j}, direction});
                }
            }
        }

        return sites;
    }

    //--------------------------------------------------------------------------
    // Joins the rooms selection left separated, one corridor at a time between the closest unconnected pair. Components
    // are recounted after each, so a corridor joining more than two is not followed by a redundant one
    void connect_components(const discrete_vector2_type& extent, const rectangles_type& rooms, cell_areas_type& areas,
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

                    discrete_point2_type center_a = center(rooms[a]);
                    discrete_point2_type center_b = center(rooms[b]);
                    coord_type distance = abs(center_a.x() - center_b.x()) + abs(center_a.y() - center_b.y());

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

            carve(extent, path_cells(center(rooms[room_a]), center(rooms[room_b]), random.random(2) == 0),
                areas, area_count);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A cell is floor where it belongs to an area, and an edge is open only between two cells of the same area. An edge
    // with floor on neither side has no wall, the edges beyond the zone included
    void realize(Zone& zone, const cell_areas_type& areas)
    {
        const discrete_vector2_type& extent = zone.extent();

        auto area_of = [&](const discrete_point2_type& position)
        {
            return contains(extent, position) ? areas[cell_slot(extent, position)] : nullid;
        };

        for (coord_type j = 0; j < extent.y(); ++j)
        {
            for (coord_type i = 0; i < extent.x(); ++i)
            {
                id_type area = area_of({i, j});
                zone.set_cell({i, j}, area == nullid ? CellType::Solid : CellType::Open);

                for (CardinalDirection direction : {CardinalDirection::East, CardinalDirection::North, CardinalDirection::West, CardinalDirection::South})
                {
                    id_type other = area_of(neighbor({i, j}, direction));

                    bool joined = area != nullid && area == other;
                    bool faced = area != nullid || other != nullid;

                    zone.set_wall({i, j}, direction, !joined && faced ? WallType::Solid : WallType::Open);
                }
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    void punch(Zone& zone, const vector<site_type>& sites, random_type& random)
    {
        const site_type& site = random.choose(sites);
        zone.set_wall(site.first, site.second, WallType::Door);
        return;
    }

    //--------------------------------------------------------------------------
    // Adds a door, in random order, for each adjacency that joins two areas not yet joined, then a door for each remaining
    // adjacency with probability loop
    void place_doors(Zone& zone, const cell_areas_type& areas, id_type area_count, double loop, random_type& random)
    {
        sites_type sites = door_sites(zone.extent(), areas);

        vector<area_pair_type> keys;
        keys.reserve(sites.size());

        for (const auto& [key, _] : sites)
        {
            keys.push_back(key);
        }

        random.shuffle(keys);

        Components components(area_count);
        vector<area_pair_type> spare;

        for (const area_pair_type& key : keys)
        {
            if (components.merge(key.first, key.second))
            {
                punch(zone, sites[key], random);
            }
            else
            {
                spare.push_back(key);
            }
        }

        for (const area_pair_type& key : spare)
        {
            if (random.random() < loop)
            {
                punch(zone, sites[key], random);
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A range of one value draws nothing, so a fixed count leaves every later draw unchanged
    size_t draw_count(const quetzal::math::Interval<size_t>& count, random_type& random)
    {
        return count.lower() == count.upper() ? count.lower() : random.random_closed(count.lower(), count.upper());
    }

    //--------------------------------------------------------------------------
    // Returns true if the side leads onto an open cell across an open wall or a door, since a door is passable once opened
    bool passable_side(const Zone& zone, const discrete_point2_type& cell, CardinalDirection direction)
    {
        discrete_point2_type next = neighbor(cell, direction);
        return zone.wall(cell, direction) != WallType::Solid && zone.contains(next) && zone.cell(next) == CellType::Open;
    }

    //--------------------------------------------------------------------------
    bool staircase_faces(const Zone& zone, const discrete_point2_type& cell)
    {
        return ranges::any_of(zone.transit_endpoints(), [&](const stratum_core::TransitEndpoint& endpoint)
        {
            return endpoint.cell_position + endpoint.egress_direction == cell;
        });
    }

    //--------------------------------------------------------------------------
    // Returns true if a staircase's block can go at an open cell. The block needs a passable side to face, and the cells
    // across the block's passable sides must still reach one another around it
    bool block_allowed(const Zone& zone, const discrete_point2_type& cell)
    {
        assert(zone.cell(cell) == CellType::Open);

        if (staircase_faces(zone, cell))
        {
            return false;
        }

        vector<discrete_point2_type> sides;

        for (size_t index = 0; index < cardinal_directions.size(); ++index)
        {
            CardinalDirection direction = static_cast<CardinalDirection>(index);
            if (passable_side(zone, cell, direction))
            {
                sides.push_back(neighbor(cell, direction));
            }
        }

        if (sides.empty())
        {
            return false;
        }

        const discrete_vector2_type& extent = zone.extent();
        vector<bool> reached(static_cast<size_t>(extent.x()) * extent.y(), false);
        reached[cell_slot(extent, sides.front())] = true;

        vector<discrete_point2_type> pending{sides.front()};
        size_t sides_unreached = sides.size() - 1;

        while (!pending.empty() && sides_unreached > 0)
        {
            discrete_point2_type position = pending.back();
            pending.pop_back();

            for (size_t index = 0; index < cardinal_directions.size(); ++index)
            {
                CardinalDirection direction = static_cast<CardinalDirection>(index);
                discrete_point2_type next = neighbor(position, direction);

                if (next == cell || !passable_side(zone, position, direction) || reached[cell_slot(extent, next)])
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
    // Returns true if a door on any side of the cell opens onto it
    bool blocks_doorway(const Zone& zone, const discrete_point2_type& cell)
    {
        for (size_t index = 0; index < cardinal_directions.size(); ++index)
        {
            if (zone.wall(cell, static_cast<CardinalDirection>(index)) == WallType::Door)
            {
                return true;
            }
        }

        return false;
    }

    //--------------------------------------------------------------------------
    optional<discrete_point2_type> reposition_staircase(const Zone& zone, const discrete_point2_type& cell, random_type& random)
    {
        vector<discrete_point2_type> available;

        for (size_t index = 0; index < cardinal_directions.size(); ++index)
        {
            CardinalDirection direction = static_cast<CardinalDirection>(index);
            if (zone.wall(cell, direction) != WallType::Open)
            {
                continue;
            }

            discrete_point2_type next = neighbor(cell, direction);
            if (zone.cell(next) == CellType::Open && !blocks_doorway(zone, next) && block_allowed(zone, next))
            {
                available.push_back(next);
            }
        }

        return available.empty() ? nullopt : optional<discrete_point2_type>(random.choose(available));
    }

    //--------------------------------------------------------------------------
    CardinalDirection draw_facing_direction(const Zone& zone, const discrete_point2_type& cell, random_type& random)
    {
        vector<CardinalDirection> open_sides;
        vector<CardinalDirection> door_sides;

        for (size_t index = 0; index < cardinal_directions.size(); ++index)
        {
            CardinalDirection direction = static_cast<CardinalDirection>(index);
            WallType wall = zone.wall(cell, direction);

            if (wall == WallType::Open && zone.cell(neighbor(cell, direction)) == CellType::Open)
            {
                open_sides.push_back(direction);
            }
            else if (wall == WallType::Door)
            {
                door_sides.push_back(direction);
            }
        }

        const vector<CardinalDirection>& sides = open_sides.empty() ? door_sides : open_sides;
        assert(!sides.empty());
        return random.choose(sides);
    }

    //--------------------------------------------------------------------------
    optional<discrete_directed_point2_type> staircase_placement(const Zone& zone, discrete_point2_type cell, random_type& random)
    {
        if (!block_allowed(zone, cell))
        {
            return nullopt;
        }

        if (blocks_doorway(zone, cell))
        {
            cell = reposition_staircase(zone, cell, random).value_or(cell);
        }

        return discrete_directed_point2_type(cell, cardinal_direction(draw_facing_direction(zone, cell, random)));
    }

    //--------------------------------------------------------------------------
    // Places staircases at the centers of rooms, one per room, those leading up first. A room is passed over if a staircase
    // at its center would cut a passage or have no side to face. Running out of rooms places fewer staircases, with those
    // leading down falling short first
    void place_staircases(Zone& zone, const rectangles_type& rooms, const Parameters& parameters, random_type& random)
    {
        size_t down_count = draw_count(parameters.staircase_down_count, random);
        size_t up_count = draw_count(parameters.staircase_up_count, random);

        // Every staircase is an access point, and a zone requires one
        if (rooms.empty())
        {
            throw runtime_error("the zone has no room a staircase can take, and so no entry");
        }

        vector<size_t> available(rooms.size());
        iota(available.begin(), available.end(), size_t(0));

        while (zone.transit_endpoints().size() < down_count + up_count && !available.empty())
        {
            size_t slot = random.random(available.size());
            discrete_point2_type cell = center(rooms[available[slot]]);
            available.erase(available.begin() + static_cast<ptrdiff_t>(slot));

            optional<discrete_directed_point2_type> placement = staircase_placement(zone, cell, random);
            if (placement)
            {
                zone.create_staircase(*placement, zone.transit_endpoints().size() < up_count ? stratum_core::Directionality::Up
                    : stratum_core::Directionality::Down);
            }
        }

        if (zone.transit_endpoints().empty())
        {
            throw runtime_error("the zone has no room a staircase can take without closing off a passage, and so no entry");
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Places staircases at open cells in a random order, those leading up first. A cell is passed over if a staircase there
    // would cut a passage or have no side to face
    void place_open_staircases(Zone& zone, const Parameters& parameters, random_type& random)
    {
        const size_t down_count = draw_count(parameters.staircase_down_count, random);
        const size_t up_count = draw_count(parameters.staircase_up_count, random);
        const discrete_vector2_type& extent = zone.extent();

        vector<discrete_point2_type> candidates;
        for (coord_type j = 0; j < extent.y(); ++j)
        {
            for (coord_type i = 0; i < extent.x(); ++i)
            {
                candidates.push_back({i, j});
            }
        }

        random.shuffle(candidates);

        for (size_t slot = 0; slot < candidates.size() && zone.transit_endpoints().size() < down_count + up_count; ++slot)
        {
            if (zone.cell(candidates[slot]) != CellType::Open)
            {
                continue;
            }

            optional<discrete_directed_point2_type> placement = staircase_placement(zone, candidates[slot], random);
            if (placement)
            {
                zone.create_staircase(*placement, zone.transit_endpoints().size() < up_count ? stratum_core::Directionality::Up
                    : stratum_core::Directionality::Down);
            }
        }

        if (down_count + up_count > 0 && zone.transit_endpoints().empty())
        {
            throw runtime_error("the zone has no cell a staircase can take without closing off a passage");
        }

        return;
    }

    //--------------------------------------------------------------------------
    Zone generate_partition1(const string& name, const Parameters& parameters, random_type& random)
    {
        rectangles_type rectangles;
        partition({discrete_point2_type{0, 0}, parameters.extent}, parameters, random, rectangles);

        rectangles_type rooms = select(rectangles, parameters.density, random);
        cell_areas_type areas = cell_areas(parameters.extent, rooms);
        id_type area_count = rooms.size();

        connect_components(parameters.extent, rooms, areas, area_count, random);

        Zone zone(name, parameters.extent);
        realize(zone, areas);
        place_doors(zone, areas, area_count, parameters.loop, random);
        place_staircases(zone, rooms, parameters, random);

        return zone;
    }

    //--------------------------------------------------------------------------
    // Every cell one area, so the zone is open floor walled only at its border
    Zone generate_surface1(const string& name, const Parameters& parameters, random_type& random)
    {
        Zone zone(name, parameters.extent);
        realize(zone, cell_areas_type(static_cast<size_t>(parameters.extent.x()) * parameters.extent.y(), 0));
        place_open_staircases(zone, parameters, random);

        return zone;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::catagen::Zone quetzal::catagen::generate(const string& name, const Parameters& parameters)
{
    validate(parameters);

    random_type random(*parameters.seed); // validate checked it is set

    switch (parameters.algorithm)
    {
        case Algorithm::Partition1:
            return generate_partition1(name, parameters, random);

        case Algorithm::Surface1:
            return generate_surface1(name, parameters, random);
    }

    assert(false);
    return generate_partition1(name, parameters, random);
}
