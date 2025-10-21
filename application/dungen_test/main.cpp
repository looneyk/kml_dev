//------------------------------------------------------------------------------
// dungen
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/dungen/types.hpp"
#include "quetzal/dungen/Cells.hpp"
#include "quetzal/dungen/Level.hpp"
#include "quetzal/dungen/Pattern.hpp"
#include "quetzal/math/Random.hpp"
#include "quetzal/math/math_util.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <tuple>
#include <cassert>

using namespace std;
using namespace quetzal;
using namespace quetzal::dungen;

namespace
{

    using connection_type = array<id_type, 2>;
    using connections_type = set<connection_type>;
    using direction_type = array<coord_type, 2>;
    using directions_type = array<direction_type, 4>;

    enum class ShaftPlacements
    {
        None,
        Paired,
        Multiple
    };

    CellType N = CellType::Null;
    CellType S = CellType::Solid;
    CellType C = CellType::Corridor;

    Cells patternEnd({3, 3},
    {
        S, S, S,
        S, C, S,
        S, C, S
    });
    Pattern matcherEnd = {patternEnd, {1, 1}};

    Cells replacementEnd({3, 3},
    {
        N, N, N,
        N, S, N,
        N, S, N
    });
    Pattern replacerEnd = {replacementEnd, {1, 1}, {0, 2}};

    Cells patternLoop({5, 5},
    {
        S, S, S, S, S,
        S, C, C, C, S,
        S, C, S, C, S,
        N, N, N, N, N,
        N, N, N, N, N,
    });
    Pattern matcherLoop = {patternLoop, {2, 2}};

    Cells replacementLoop({3, 3},
    {
        S, S, S,
        S, N, S,
        N, C, N,
    });
    Pattern replacerLoop = {replacementLoop, {1, 1}, {0, 2}};

    math::Random random;

    //--------------------------------------------------------------------------
    bool contains(const Level& level, position_type position)
    {
        return position.x() >= 0 && position.x() < level.cells().size().x() && position.y() >= 0 && position.y() < level.cells().size().y();
    }

    //--------------------------------------------------------------------------
    size_t room_placement_attempts(const Level& level, double density, coord_type roomDimensionMax)
    {
        auto [nx, ny] = level.size();
        coord_type areaLevel = nx * ny;
        coord_type areaRoomMax = roomDimensionMax * roomDimensionMax;
        double densityFactor = 16 * density;
        return static_cast<size_t>(densityFactor * areaLevel / areaRoomMax);
    }

    //--------------------------------------------------------------------------
    array<dimensions_type, 2> room_dimensions(const Level& level, function<coord_type()> dimensionalizer)
    {
        auto [nx, ny] = level.size();

        coord_type dx = dimensionalizer() | 1;
        coord_type dy = dimensionalizer() | 1;
        dimensions_type size = {dx, dy};

        coord_type x = random.gen(nx - dx) | 1;
        coord_type y = random.gen(ny - dy) | 1;
        position_type position = {x, y};

        return {position, size};
    }

    //--------------------------------------------------------------------------
    set<size_t> room_intersections(const Level& level, position_type position, dimensions_type size)
    {
        set<size_t> intersections;

        for (coord_type y = position[1]; y < position[1] + size[1]; ++y)
        {
            for (coord_type x = position[0]; x < position[0] + size[0]; ++x)
            {
                Cell cell = level.cell(x, y);
                if (cell.type() == CellType::Room)
                {
                    intersections.insert(cell.room_id());
                }
            }
        }

        return intersections;
    }

    //--------------------------------------------------------------------------
    bool attempt_room_placement(Level& level, function<coord_type()> dimensionalizer, bool roomMerge)
    {
        auto [position, size] = room_dimensions(level, dimensionalizer);
        auto intersections = room_intersections(level, position, size);
        if (intersections.size() > 1)
        {
            return false;
        }
        else if (intersections.size() == 1)
        {
            if (roomMerge) // and probability?
            {
                level.expand_room(*intersections.begin(), position, size);
                return true;
            }

            return false;
        }

        level.create_room(position, size);
        return true;
    }

    //--------------------------------------------------------------------------
    void place_rooms(Level& level, double density, function<coord_type()> dimensionalizer, coord_type roomDimensionMax, bool roomMerge)
    {
        size_t n = room_placement_attempts(level, density, roomDimensionMax);
        for (size_t i = 0; i < n; ++i)
        {
            attempt_room_placement(level, dimensionalizer, roomMerge);
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
    tuple<bool, connection_type> room_connection(const Level& level, position_type position)
    {
        Cell cellLeft = level.cell(position.x() - 1, position.y());
        Cell cellRight = level.cell(position.x() + 1, position.y());
        if (cellLeft.type() == CellType::Room && cellRight.type() == CellType::Room)
        {
            return {true, {cellLeft.room_id(), cellRight.room_id()}};
        }

        Cell cellBelow = level.cell(position.x(), position.y() - 1);
        Cell cellAbove = level.cell(position.x(), position.y() + 1);
        if (cellBelow.type() == CellType::Room && cellAbove.type() == CellType::Room)
        {
            return {true, {cellBelow.room_id(), cellAbove.room_id()}};
        }

        return {false, {0, 0}};
    }

    //--------------------------------------------------------------------------
    tuple<vector<position_type>, size_t> door_candidates(const Level& level, const Room& room)
    {
        vector<position_type> candidates;

        position_type position0 = room.position();
        position_type position1 = position0 + room.size();

        size_t nExisting = 0;

        for (coord_type x = position0.x(); x < position1.x(); x += 2)
        {
            if (position0.y() > 2)
            {
                coord_type y = position0.y() - 1;
                if (level.cell(x, y).type() == CellType::Solid)
                {
                    candidates.push_back({x, y});
                }
                else if (level.cell(x, y).type() == CellType::Door)
                {
                    ++nExisting;
                }
            }

            if (position1.y() < level.size().y() - 2)
            {
                coord_type y = position1.y();
                if (level.cell(x, y).type() == CellType::Solid)
                {
                    candidates.push_back({x, y});
                }
                else if (level.cell(x, y).type() == CellType::Door)
                {
                    ++nExisting;
                }
            }
        }

        for (coord_type y = position0.y(); y < position1.y(); y += 2)
        {
            if (position0.x() > 2)
            {
                coord_type x = position0.x() - 1;
                if (level.cell(x, y).type() == CellType::Solid)
                {
                    candidates.push_back({x, y});
                }
                else if (level.cell(x, y).type() == CellType::Door)
                {
                    ++nExisting;
                }
            }

            if (position1.x() < level.size().x() - 2)
            {
                coord_type x = position1.x();
                if (level.cell(x, y).type() == CellType::Solid)
                {
                    candidates.push_back({x, y});
                }
                else if (level.cell(x, y).type() == CellType::Door)
                {
                    ++nExisting;
                }
            }
        }

        return {candidates, nExisting};
    }

    //--------------------------------------------------------------------------
    size_t door_placements(const Room& room)
    {
        auto [dx, dy] = room.size();

        size_t sx = (dx - 1) / 2 + 1;
        size_t sy = (dy - 1) / 2 + 1;
        size_t sz = static_cast<size_t>(sqrt(sx * sy));

        return sz + random.gen(sz);
    }

    //--------------------------------------------------------------------------
    void place_doors(Level& level, Room& room, connections_type& connections)
    {
        auto [candidates, nExisting] = door_candidates(level, room);
        shuffle(candidates.begin(), candidates.end(), random.engine());

        size_t nDoors = door_placements(room);
//cout << "Room\t" << room.id() << "\tc\t" << c << "\tnDoors\t" << nDoors << endl;
//cout << "nExisting\t" << nExisting << endl;
        if (nDoors >= nExisting)
        {
//            nDoors -= nExisting;
        }

        nDoors = min(nDoors, candidates.size());

        for (size_t i = 0; i < nDoors; ++i)
        {
            if (level.cell(candidates[i]).type() != CellType::Door)
            {
                // Limit to a single door between any two rooms
                auto [bConnected, connection] = room_connection(level, candidates[i]);
                if (bConnected)
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
    bool available(const Level& level, position_type position)
    {
        auto [nx, ny] = level.size();
        if (position.x() < 0 || position.x() >= nx || position.y() < 0 || position.y() >= ny)
        {
            return false;
        }

        return level.cell(position).type() == CellType::Solid;
    }

    //--------------------------------------------------------------------------
    void excavate(Level& level, position_type positionFrom, position_type positionTo)
    {
        level.cell((positionFrom + positionTo) / 2).set_type(CellType::Corridor);
        level.cell(positionTo).set_type(CellType::Corridor);
        return;
    }

    //--------------------------------------------------------------------------
    bool attempt_excavate(Level& level, position_type positionFrom, position_type positionTo)
    {
        if (available(level, positionTo))
        {
            excavate(level, positionFrom, positionTo);
            return true;
        }

        return false;
    }

    //--------------------------------------------------------------------------
    void excavate_cell(Level& level, position_type positionFrom, directions_type directions, double corridorStraightness)
    {
        auto first = directions.begin();
        if (random.gen() < corridorStraightness)
        {
            ++first;
        }
        shuffle(first, directions.end(), random.engine());

        for (auto direction : directions)
        {
            position_type positionTo = positionFrom + direction;
            if (attempt_excavate(level, positionFrom, positionTo))
            {
                for (size_t i = 1; i < 4; ++i)
                {
                    if (directions[i] == direction) {
                        swap(directions[i], directions[0]);
                        break;
                    }
                }

                level.cell(positionFrom).set_type(CellType::Corridor);
                excavate_cell(level, positionTo, directions, corridorStraightness);
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    void excavate(Level& level, double corridorStraightness)
    {
        directions_type directions = 
        {{
            {2, 0},
            {0, 2},
            {-2, 0},
            {0, -2},
        }};
        shuffle(directions.begin(), directions.end(), random.engine());

        auto [nx, ny] = level.size();
        for (coord_type y = 1; y < ny; y += 2)
        {
            for (coord_type x = 1; x < nx; x += 2)
            {
                if (level.cell(x, y).type() == CellType::Solid)
                {
                    excavate_cell(level, {x, y}, directions, corridorStraightness);
                }
            }
        }

        return;
    }

    //------------------------------------------------------------------------------
    vector<position_type> shaft_candidates(const Level& level)
    {
        vector<position_type> candidates;

        auto [nx, ny] = level.size();
        for (coord_type y = 1; y < ny; y += 2)
        {
            for (coord_type x = 1; x < nx; x += 2)
            {
                position_type position = {x, y};
                if (match(level.cells(), position, matcherEnd))
                {
                    candidates.push_back(position);
                }
            }
        }

        return candidates;
    }

    //------------------------------------------------------------------------------
    size_t shaft_placements(const Level& level, ShaftPlacements shaftPlacements)
    {
        if (shaftPlacements == ShaftPlacements::Paired)
        {
            return 2;
        }

        auto [nx, ny] = level.size();
        return 3 + random.gen(nx * ny / 1000);
    }

    //------------------------------------------------------------------------------
    void place_shafts(Level& level, ShaftPlacements shaftPlacements)
    {
        if (shaftPlacements == ShaftPlacements::None)
        {
            return;
        }

        auto candidates = shaft_candidates(level);
        shuffle(candidates.begin(), candidates.end(), random.engine());

        size_t nShafts = shaft_placements(level, shaftPlacements);
        nShafts = min(nShafts, candidates.size());

        for (size_t i = 0; i < nShafts; ++i)
        {
            bool bDown = true;
            if (i == 1)
            {
                bDown = false;
            }
            else if (i > 1)
            {
                bDown = random.gen(2) == 0;
            }

            auto [x, y] = candidates[i];
            level.cell(x, y).set_type(CellType::Shaft);
//            level.cell(x, y) set property down/up
        }

        return;
    }

    //--------------------------------------------------------------------------
    void shorten_dead_ends(Level& level, double probability)
    {
        if (probability == 0.0)
        {
            return;
        }

//        double improbability = 1.0 - probability;
//        probability = 1.0 - improbability * improbability;

        auto [nx, ny] = level.size();
        for (long x = 1; x < nx; x += 2)
        {
            for (long y = 1; y < ny; y += 2)
            {
                position_type position = {x, y};
                bool bNext = true;
                while (bNext && contains(level, position) && level.cell(position).type() == CellType::Corridor)
                {
                    if (random.gen() >= probability)
                    {
                        break;
                    }

                    auto [bMatched, shift] = match_replace(level.cells(), position, matcherEnd, replacerEnd);
                    bNext = bMatched;
                    position += shift;
                }
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    void shorten_loops(Level& level, double probability)
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
                position_type position = {x, y};
                bool bNext = true;
                while (bNext && contains(level, position) && level.cell(position).type() == CellType::Solid)
                {
                    if (random.gen() < probability)
                    {
                        break;
                    }

                    auto [bMatched, shift] = match_replace(level.cells(), position, matcherLoop, replacerLoop);
                    bNext = bMatched;
                    position += shift;
                }
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    Level create_level(id_type id, const string& name, dimensions_type size)
    {
        // add generation parameters and calculate dimensions and geometry from them here ...

        coord_type roomDimensionBase = 3;
        coord_type roomDimensionVariation = 8;
//        double roomDimensionMean = 7.0;
//        double roomDimensionStddev = 1.5;
//        double roomDimensionHalfwidth = 4.0;
        auto room_dimensionalizer = [roomDimensionBase, roomDimensionVariation]() -> coord_type { return roomDimensionBase + random.gen(roomDimensionVariation); }; // linear probability density
//        auto room_dimensionalizer = [roomDimensionMean, roomDimensionStddev]() -> coord_type { return static_cast<coord_type>round(random.genn(roomDimensionMean, roomDimensionStddev)); }; // normal distribution
//        auto room_dimensionalizer = [roomDimensionMean, roomDimensionHalfwidth]() -> coord_type { return static_cast<coord_type>round(random.gent(roomDimensionMean, roomDimensionHalfwidth)); }; // triangular distribution
        double roomDensity = 0.25;
        bool roomMerge = false;
        double corridorStraightness = 0.5;
        ShaftPlacements shaftPlacements = ShaftPlacements::Paired;
        double probabilityDeadEndRemoval = 0.5;
        double probabilityLoopRemoval = 1.0;

        // Level

        auto [nx, ny] = size;
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
        if (roomDensity < 1.0)
        {
            place_rooms(level, roomDensity, room_dimensionalizer, roomDimensionBase + roomDimensionVariation, roomMerge);
        }
        else
        {
            pack_rooms(level, room_dimensionalizer);
        }

        // Doors
        connections_type connections;
        for (auto& room : level.rooms())
        {
            place_doors(level, room, connections);
        }

        // Corridors
        excavate(level, corridorStraightness);

        // ShaftPlacements
        place_shafts(level, shaftPlacements);

        // Dead end and loop removal
level.render(cout);
cout << endl;
        shorten_dead_ends(level, probabilityDeadEndRemoval);
level.render(cout);
cout << endl;
        shorten_loops(level, probabilityLoopRemoval);
        return level;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    Level level = create_level(0, "Level 0", {32, 32});

    ofstream os("Level_0.level", ios_base::binary);
    level.render(os);

    level.render(cout);
//    cout << level;

    return 0;
}
