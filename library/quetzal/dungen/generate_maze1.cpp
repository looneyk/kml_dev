//------------------------------------------------------------------------------
// dungen
// generate_maze1.cpp
//------------------------------------------------------------------------------

#include "generate_maze1.hpp"
#include "Pattern.hpp"
#include "place_stairs.hpp"
#include "quetzal/math/math_util.hpp"
#include <algorithm>
#include <array>
#include <functional>
#include <set>
#include <tuple>
#include <vector>
#include <cmath>
#include <cassert>

using namespace std;

namespace
{

    using namespace quetzal::dungen;
    using quetzal::id_type;

    using connection_type = array<id_type, 2>;
    using connections_type = set<connection_type>;
    using directions_type = array<vector_type, 4>;

    const CellType N = CellType::Null;
    const CellType S = CellType::Solid;
    const CellType C = CellType::Corridor;

    const Cells pattern_end({3, 3},
    {
        S, S, S,
        S, C, S,
        S, C, S
    });
    const Pattern matcher_end = {pattern_end, {1, 1}};

    const Cells replacement_end({3, 3},
    {
        N, N, N,
        N, S, N,
        N, S, N
    });
    const Pattern replacer_end = {replacement_end, {1, 1}, {0, 2}};

    const Cells pattern_loop({5, 5},
    {
        S, S, S, S, S,
        S, C, C, C, S,
        S, C, S, C, S,
        N, N, N, N, N,
        N, N, N, N, N,
    });
    const Pattern matcher_loop = {pattern_loop, {2, 2}};

    const Cells replacement_loop({3, 3},
    {
        S, S, S,
        S, N, S,
        N, C, N,
    });
    const Pattern replacer_loop = {replacement_loop, {1, 1}, {0, 2}};

    //--------------------------------------------------------------------------
    bool contains(const Level& level, point_type position)
    {
        return level.cells().contains(position);
    }

    //--------------------------------------------------------------------------
    size_t room_placement_attempts(const Level& level, double density, coord_type room_dimension_max)
    {
        auto [nx, ny] = level.size();
        coord_type area_level = nx * ny;
        coord_type area_room_max = room_dimension_max * room_dimension_max;
        double density_factor = 16 * density;
        return static_cast<size_t>(density_factor * area_level / area_room_max);
    }

    //--------------------------------------------------------------------------
    box_type room_dimensions(const Level& level, function<coord_type()> dimensionalizer, random_type& random)
    {
        auto [nx, ny] = level.size();

        coord_type dx = dimensionalizer() | 1;
        coord_type dy = dimensionalizer() | 1;
        vector_type size = {dx, dy};

        coord_type x = random.gen(nx - dx) | 1;
        coord_type y = random.gen(ny - dy) | 1;
        point_type position = {x, y};

        return {position, size};
    }

    //--------------------------------------------------------------------------
    set<size_t> room_intersections(const Level& level, const box_type& rectangle)
    {
        set<size_t> intersections;

        for (coord_type y = rectangle.lower(1); y < rectangle.upper(1); ++y)
        {
            for (coord_type x = rectangle.lower(0); x < rectangle.upper(0); ++x)
            {
                Cell cell = level.cell({x, y});
                if (cell.type() == CellType::Room)
                {
                    intersections.insert(cell.room_id());
                }
            }
        }

        return intersections;
    }

    //--------------------------------------------------------------------------
    bool attempt_room_placement(Level& level, function<coord_type()> dimensionalizer, bool room_merge, random_type& random)
    {
        box_type rectangle = room_dimensions(level, dimensionalizer, random);
        auto intersections = room_intersections(level, rectangle);
        if (intersections.size() > 1)
        {
            return false;
        }
        else if (intersections.size() == 1)
        {
            if (room_merge) // and probability?
            {
                level.expand_room(*intersections.begin(), rectangle);
                return true;
            }

            return false;
        }

        level.create_room(rectangle);
        return true;
    }

    //--------------------------------------------------------------------------
    void place_rooms(Level& level, double density, function<coord_type()> dimensionalizer, coord_type room_dimension_max, bool room_merge, random_type& random)
    {
        size_t n = room_placement_attempts(level, density, room_dimension_max);
        for (size_t i = 0; i < n; ++i)
        {
            attempt_room_placement(level, dimensionalizer, room_merge, random);
        }

        return;
    }

    //--------------------------------------------------------------------------
    void pack_rooms(Level& level, function<coord_type()> dimensionalizer)
    {
        level;
        dimensionalizer;

        // ...

        return;
    }

    //--------------------------------------------------------------------------
    // Given a door position, determine if it connects two rooms
    tuple<bool, connection_type> room_connection(const Level& level, point_type position)
    {
        Cell cell_left = level.cell({position.x() - 1, position.y()});
        Cell cell_right = level.cell({position.x() + 1, position.y()});
        if (cell_left.type() == CellType::Room && cell_right.type() == CellType::Room)
        {
            return {true, {cell_left.room_id(), cell_right.room_id()}};
        }

        Cell cell_below = level.cell({position.x(), position.y() - 1});
        Cell cell_above = level.cell({position.x(), position.y() + 1});
        if (cell_below.type() == CellType::Room && cell_above.type() == CellType::Room)
        {
            return {true, {cell_below.room_id(), cell_above.room_id()}};
        }

        return {false, {0, 0}};
    }

    //--------------------------------------------------------------------------
    tuple<vector<point_type>, size_t> door_candidates(const Level& level, const Room& room)
    {
        vector<point_type> candidates;

        point_type position0 = room.rectangle().lower();
        point_type position1 = room.rectangle().upper();

        size_t n_existing = 0;

        for (coord_type x = position0.x(); x < position1.x(); x += 2)
        {
            if (position0.y() > 2)
            {
                coord_type y = position0.y() - 1;
                if (level.cell({x, y}).type() == CellType::Solid)
                {
                    candidates.push_back({x, y});
                }
                else if (level.cell({x, y}).type() == CellType::Door)
                {
                    ++n_existing;
                }
            }

            if (position1.y() < level.size().y() - 2)
            {
                coord_type y = position1.y();
                if (level.cell({x, y}).type() == CellType::Solid)
                {
                    candidates.push_back({x, y});
                }
                else if (level.cell({x, y}).type() == CellType::Door)
                {
                    ++n_existing;
                }
            }
        }

        for (coord_type y = position0.y(); y < position1.y(); y += 2)
        {
            if (position0.x() > 2)
            {
                coord_type x = position0.x() - 1;
                if (level.cell({x, y}).type() == CellType::Solid)
                {
                    candidates.push_back({x, y});
                }
                else if (level.cell({x, y}).type() == CellType::Door)
                {
                    ++n_existing;
                }
            }

            if (position1.x() < level.size().x() - 2)
            {
                coord_type x = position1.x();
                if (level.cell({x, y}).type() == CellType::Solid)
                {
                    candidates.push_back({x, y});
                }
                else if (level.cell({x, y}).type() == CellType::Door)
                {
                    ++n_existing;
                }
            }
        }

        return {candidates, n_existing};
    }

    //--------------------------------------------------------------------------
    size_t door_placements(const Room& room, random_type& random)
    {
        auto [dx, dy] = room.rectangle().extent();

        size_t sx = (dx - 1) / 2 + 1;
        size_t sy = (dy - 1) / 2 + 1;
        size_t sz = static_cast<size_t>(sqrt(sx * sy));

        return sz + random.gen(sz);
    }

    //--------------------------------------------------------------------------
    void place_doors(Level& level, Room& room, connections_type& connections, random_type& random)
    {
        auto [candidates, n_existing] = door_candidates(level, room);
        shuffle(candidates.begin(), candidates.end(), random.engine());

        size_t n_doors = door_placements(room, random);
        if (n_doors >= n_existing)
        {
//            n_doors -= n_existing;
        }

        n_doors = min(n_doors, candidates.size());

        for (size_t i = 0; i < n_doors; ++i)
        {
            if (level.cell(candidates[i]).type() != CellType::Door)
            {
                // Limit to a single door between any two rooms
                auto [connected, connection] = room_connection(level, candidates[i]);
                if (connected)
                {
                    sort(connection.begin(), connection.end());
                    if (!connections.contains(connection))
                    {
                        level.create_door(candidates[i]);
                        connections.insert(connection);
                    }
                }
                else
                {
                    level.create_door(candidates[i]);
                }
            }
        }

        return;
    }

    //------------------------------------------------------------------------------
    bool available(const Level& level, point_type position)
    {
        if (!level.cells().contains(position))
        {
            return false;
        }

        return level.cell(position).type() == CellType::Solid;
    }

    //--------------------------------------------------------------------------
    void excavate(Level& level, point_type position_from, point_type position_to)
    {
        level.cell(position_from + (position_to - position_from) / 2).set_type(CellType::Corridor);
        level.cell(position_to).set_type(CellType::Corridor);
        return;
    }

    //--------------------------------------------------------------------------
    bool attempt_excavate(Level& level, point_type position_from, point_type position_to)
    {
        if (available(level, position_to))
        {
            excavate(level, position_from, position_to);
            return true;
        }

        return false;
    }

    //--------------------------------------------------------------------------
    void excavate_cell(Level& level, point_type position_from, directions_type directions, double corridor_straightness, random_type& random)
    {
        auto first = directions.begin();
        if (random.gen() < corridor_straightness)
        {
            ++first;
        }
        shuffle(first, directions.end(), random.engine());

        for (auto direction : directions)
        {
            point_type position_to = position_from + direction;
            if (attempt_excavate(level, position_from, position_to))
            {
                for (size_t i = 1; i < 4; ++i)
                {
                    if (directions[i] == direction) {
                        swap(directions[i], directions[0]);
                        break;
                    }
                }

                level.cell(position_from).set_type(CellType::Corridor);
                excavate_cell(level, position_to, directions, corridor_straightness, random);
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    void excavate(Level& level, double corridor_straightness, random_type& random)
    {
        // A maze cell every other cell, so a step crosses the wall between two of them
        directions_type directions =
        {{
            2 * cardinal_direction(CardinalDirection::East),
            2 * cardinal_direction(CardinalDirection::North),
            2 * cardinal_direction(CardinalDirection::West),
            2 * cardinal_direction(CardinalDirection::South),
        }};
        shuffle(directions.begin(), directions.end(), random.engine());

        auto [nx, ny] = level.size();
        for (coord_type y = 1; y < ny; y += 2)
        {
            for (coord_type x = 1; x < nx; x += 2)
            {
                if (level.cell({x, y}).type() == CellType::Solid)
                {
                    excavate_cell(level, {x, y}, directions, corridor_straightness, random);
                }
            }
        }

        return;
    }

    //------------------------------------------------------------------------------
    // A stairway goes where a corridor ends, so the dead ends left before they are pruned are
    // the cells a stairway may take
    vector<point_type> stair_candidates(const Level& level)
    {
        vector<point_type> candidates;

        auto [nx, ny] = level.size();
        for (coord_type y = 1; y < ny; y += 2)
        {
            for (coord_type x = 1; x < nx; x += 2)
            {
                point_type position = {x, y};
                if (match(level.cells(), position, matcher_end))
                {
                    candidates.push_back(position);
                }
            }
        }

        return candidates;
    }

    //--------------------------------------------------------------------------
    void shorten_dead_ends(Level& level, double probability, random_type& random)
    {
        if (probability == 0.0)
        {
            return;
        }

//        double improbability = 1.0 - probability;
//        probability = 1.0 - improbability * improbability;

        auto [nx, ny] = level.size();
        for (coord_type x = 1; x < nx; x += 2)
        {
            for (coord_type y = 1; y < ny; y += 2)
            {
                point_type position = {x, y};
                bool again = true;
                while (again && contains(level, position) && level.cell(position).type() == CellType::Corridor)
                {
                    if (random.gen() >= probability)
                    {
                        break;
                    }

                    auto [matched, shift] = match_replace(level.cells(), position, matcher_end, replacer_end);
                    again = matched;
                    position += shift;
                }
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    void shorten_loops(Level& level, double probability, random_type& random)
    {
        if (probability == 0.0)
        {
            return;
        }

//        double improbability = 1.0 - probability;
//        probability = 1.0 - improbability * improbability;

        auto [nx, ny] = level.size();
        for (coord_type x = 1; x < nx; x += 2)
        {
            for (coord_type y = 1; y < ny; y += 2)
            {
                point_type position = {x, y};
                bool again = true;
                while (again && contains(level, position) && level.cell(position).type() == CellType::Solid)
                {
                    if (random.gen() < probability)
                    {
                        break;
                    }

                    auto [matched, shift] = match_replace(level.cells(), position, matcher_loop, replacer_loop);
                    again = matched;
                    position += shift;
                }
            }
        }

        return;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::dungen::Level quetzal::dungen::generate_maze1(id_type id, const string& name, const Parameters& parameters)
{
    assert(parameters.algorithm == Algorithm::Maze1);
    validate(parameters);

    const Maze1Parameters& maze1 = parameters.maze1;
    random_type random(parameters.seed.value());

    coord_type room_dimension_base = maze1.room_dimension_base;
    coord_type room_dimension_variation = maze1.room_dimension_variation;

    // Linear probability density. A variation of zero asks for no draw at all, which is not a
    // range to draw from
    auto room_dimensionalizer = [&random, room_dimension_base, room_dimension_variation]() -> coord_type
    {
        return room_dimension_base + (room_dimension_variation > 0 ? random.gen(room_dimension_variation) : 0);
    };

    // Both dimensions rounded down to odd, the rooms and the maze running along the odd cells
    auto [nx, ny] = parameters.size;
    if (!math::odd(nx))
    {
        --nx;
    }

    if (!math::odd(ny))
    {
        --ny;
    }

    Level level(id, name, {nx, ny}, CellType::Solid);

    // Rooms
    if (maze1.room_density < 1.0)
    {
        place_rooms(level, maze1.room_density, room_dimensionalizer, room_dimension_base + room_dimension_variation, maze1.room_merge, random);
    }
    else
    {
        pack_rooms(level, room_dimensionalizer);
    }

    // Doors
    connections_type connections;
    for (auto& room : level.rooms())
    {
        place_doors(level, room, connections, random);
    }

    // Corridors
    excavate(level, maze1.corridor_straightness, random);

    // Stairways, before the dead ends they occupy are pruned
    place_stairs(level, stair_candidates(level), parameters.stair_placements, random);

    // Dead end and loop removal
    shorten_dead_ends(level, maze1.probability_dead_end_removal, random);
    shorten_loops(level, maze1.probability_loop_removal, random);

    return level;
}
