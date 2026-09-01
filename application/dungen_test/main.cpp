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
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <cassert>

using namespace std;
using namespace quetzal;
using namespace quetzal::dungen;

namespace
{

    using connection_type = array<id_type, 2>;
    using connections_type = set<connection_type>;
    using directions_type = array<vector_type, 4>;

    enum class ShaftPlacements
    {
        None,
        Paired,
        Multiple
    };

    using seed_type = default_random_engine::result_type;

    //--------------------------------------------------------------------------
    // The generation parameters of one run, gathered so that the level file can record them
    // and the command line can set them
    struct Parameters
    {
        dimensions_type size {32, 32}; // Rounded down to odd in each direction by create_level
        coord_type room_dimension_base = 3;
        coord_type room_dimension_variation = 8;
        double room_density = 0.25;
        bool room_merge = false;
        double corridor_straightness = 0.5;
        ShaftPlacements shaft_placements = ShaftPlacements::Paired;
        double probability_dead_end_removal = 0.5;
        double probability_loop_removal = 1.0;

        // Empty until one is assigned, so that a run that never seeded is caught rather than
        // quietly handed the same level every time
        optional<seed_type> seed;
    };

    //--------------------------------------------------------------------------
    // A seed drawn from the platform's entropy source, for a run wanting a different level
    // each time. Generation takes the seed it is given, so that a recorded seed reproduces
    // a level exactly
    seed_type random_seed()
    {
        random_device rd;
        return static_cast<seed_type>(rd());
    }

    //--------------------------------------------------------------------------
    const char* shaft_placements_name(ShaftPlacements placements)
    {
        switch (placements)
        {
            case ShaftPlacements::None: return "none";
            case ShaftPlacements::Paired: return "paired";
            case ShaftPlacements::Multiple: return "multiple";
        }

        assert(false);
        return "";
    }

    //--------------------------------------------------------------------------
    ShaftPlacements to_shaft_placements(const string& s)
    {
        if (s == "none") return ShaftPlacements::None;
        if (s == "paired") return ShaftPlacements::Paired;
        if (s == "multiple") return ShaftPlacements::Multiple;

        throw invalid_argument(format("\"{}\" is not none, paired or multiple", s));
    }

    //--------------------------------------------------------------------------
    // The whole string or nothing, so that trailing rubbish is an error rather than
    // something quietly ignored
    long to_integer(const string& s)
    {
        size_t n = 0;
        long value = stol(s, &n);

        if (n != s.size())
        {
            throw invalid_argument(format("\"{}\" is not a whole number", s));
        }

        return value;
    }

    //--------------------------------------------------------------------------
    double to_real(const string& s)
    {
        size_t n = 0;
        double value = stod(s, &n);

        if (n != s.size())
        {
            throw invalid_argument(format("\"{}\" is not a number", s));
        }

        return value;
    }

    //--------------------------------------------------------------------------
    // Two numbers with a separator between them, written as the level file header writes
    // them so that a header can be read back as a command line
    pair<long, long> to_pair(const string& s, char separator)
    {
        size_t n = s.find(separator);

        if (n == string::npos)
        {
            throw invalid_argument(format("\"{}\" is not two values separated by '{}'", s, separator));
        }

        return {to_integer(s.substr(0, n)), to_integer(s.substr(n + 1))};
    }

    //--------------------------------------------------------------------------
    void usage(ostream& os)
    {
        os << "Usage: dungen_test [level] [--size nx,ny] [--room-dimension-base n]"
            << " [--room-dimension-variation n] [--room-density d] [--room-merge]"
            << " [--corridor-straightness s] [--shaft-placements none|paired|multiple]"
            << " [--probability-dead-end-removal p] [--probability-loop-removal p] [--seed n]" << endl;
        return;
    }

    //--------------------------------------------------------------------------
    // One row naming the generator, the file format version, and the parameters the run
    // used. A grid row never starts with a letter, so the first token tells a reader which
    // of the two it is holding
    void write_header(ostream& os, const Level& level, const Parameters& parameters)
    {
        auto [nx, ny] = level.size();

        os << "dungen 1"
            << " size=" << nx << "," << ny
            << " room_dimension_base=" << parameters.room_dimension_base
            << " room_dimension_variation=" << parameters.room_dimension_variation
            << " room_density=" << parameters.room_density
            << " room_merge=" << parameters.room_merge
            << " corridor_straightness=" << parameters.corridor_straightness
            << " shaft_placements=" << shaft_placements_name(parameters.shaft_placements)
            << " probability_dead_end_removal=" << parameters.probability_dead_end_removal
            << " probability_loop_removal=" << parameters.probability_loop_removal
            << " seed=" << parameters.seed.value() // Throws rather than write a header that cannot reproduce
            << endl;
        return;
    }

    CellType N = CellType::Null;
    CellType S = CellType::Solid;
    CellType C = CellType::Corridor;

    Cells pattern_end({3, 3},
    {
        S, S, S,
        S, C, S,
        S, C, S
    });
    Pattern matcher_end = {pattern_end, {1, 1}};

    Cells replacement_end({3, 3},
    {
        N, N, N,
        N, S, N,
        N, S, N
    });
    Pattern replacer_end = {replacement_end, {1, 1}, {0, 2}};

    Cells pattern_loop({5, 5},
    {
        S, S, S, S, S,
        S, C, C, C, S,
        S, C, S, C, S,
        N, N, N, N, N,
        N, N, N, N, N,
    });
    Pattern matcher_loop = {pattern_loop, {2, 2}};

    Cells replacement_loop({3, 3},
    {
        S, S, S,
        S, N, S,
        N, C, N,
    });
    Pattern replacer_loop = {replacement_loop, {1, 1}, {0, 2}};

    math::Random random;

    //--------------------------------------------------------------------------
    bool contains(const Level& level, position_type position)
    {
        return position.x() >= 0 && position.x() < level.cells().size().x() && position.y() >= 0 && position.y() < level.cells().size().y();
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
    bool attempt_room_placement(Level& level, function<coord_type()> dimensionalizer, bool room_merge)
    {
        auto [position, size] = room_dimensions(level, dimensionalizer);
        auto intersections = room_intersections(level, position, size);
        if (intersections.size() > 1)
        {
            return false;
        }
        else if (intersections.size() == 1)
        {
            if (room_merge) // and probability?
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
    void place_rooms(Level& level, double density, function<coord_type()> dimensionalizer, coord_type room_dimension_max, bool room_merge)
    {
        size_t n = room_placement_attempts(level, density, room_dimension_max);
        for (size_t i = 0; i < n; ++i)
        {
            attempt_room_placement(level, dimensionalizer, room_merge);
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
        Cell cell_left = level.cell(position.x() - 1, position.y());
        Cell cell_right = level.cell(position.x() + 1, position.y());
        if (cell_left.type() == CellType::Room && cell_right.type() == CellType::Room)
        {
            return {true, {cell_left.room_id(), cell_right.room_id()}};
        }

        Cell cell_below = level.cell(position.x(), position.y() - 1);
        Cell cell_above = level.cell(position.x(), position.y() + 1);
        if (cell_below.type() == CellType::Room && cell_above.type() == CellType::Room)
        {
            return {true, {cell_below.room_id(), cell_above.room_id()}};
        }

        return {false, {0, 0}};
    }

    //--------------------------------------------------------------------------
    tuple<vector<position_type>, size_t> door_candidates(const Level& level, const Room& room)
    {
        vector<position_type> candidates;

        position_type position0 = room.position();
        position_type position1 = position0 + room.size();

        size_t n_existing = 0;

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
                    ++n_existing;
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
                    ++n_existing;
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
                    ++n_existing;
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
                    ++n_existing;
                }
            }
        }

        return {candidates, n_existing};
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
        auto [candidates, n_existing] = door_candidates(level, room);
        shuffle(candidates.begin(), candidates.end(), random.engine());

        size_t n_doors = door_placements(room);
//cout << "Room\t" << room.id() << "\tc\t" << c << "\tn_doors\t" << n_doors << endl;
//cout << "n_existing\t" << n_existing << endl;
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
    void excavate(Level& level, position_type position_from, position_type position_to)
    {
        level.cell((position_from + position_to) / 2).set_type(CellType::Corridor);
        level.cell(position_to).set_type(CellType::Corridor);
        return;
    }

    //--------------------------------------------------------------------------
    bool attempt_excavate(Level& level, position_type position_from, position_type position_to)
    {
        if (available(level, position_to))
        {
            excavate(level, position_from, position_to);
            return true;
        }

        return false;
    }

    //--------------------------------------------------------------------------
    void excavate_cell(Level& level, position_type position_from, directions_type directions, double corridor_straightness)
    {
        auto first = directions.begin();
        if (random.gen() < corridor_straightness)
        {
            ++first;
        }
        shuffle(first, directions.end(), random.engine());

        for (auto direction : directions)
        {
            position_type position_to = position_from + direction;
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
                excavate_cell(level, position_to, directions, corridor_straightness);
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    void excavate(Level& level, double corridor_straightness)
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
                    excavate_cell(level, {x, y}, directions, corridor_straightness);
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
                if (match(level.cells(), position, matcher_end))
                {
                    candidates.push_back(position);
                }
            }
        }

        return candidates;
    }

    //------------------------------------------------------------------------------
    size_t shaft_placements(const Level& level, ShaftPlacements placements)
    {
        if (placements == ShaftPlacements::Paired)
        {
            return 2;
        }

        auto [nx, ny] = level.size();
        return 3 + random.gen(nx * ny / 1000);
    }

    //------------------------------------------------------------------------------
    void place_shafts(Level& level, ShaftPlacements placements)
    {
        if (placements == ShaftPlacements::None)
        {
            return;
        }

        auto candidates = shaft_candidates(level);
        shuffle(candidates.begin(), candidates.end(), random.engine());

        size_t n_shafts = shaft_placements(level, placements);
        n_shafts = min(n_shafts, candidates.size());

        for (size_t i = 0; i < n_shafts; ++i)
        {
            bool down = true;
            if (i == 1)
            {
                down = false;
            }
            else if (i > 1)
            {
                down = random.gen(2) == 0;
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

    //--------------------------------------------------------------------------
    Level create_level(id_type id, const string& name, const Parameters& parameters)
    {
        // add generation parameters and calculate dimensions and geometry from them here ...

        coord_type room_dimension_base = parameters.room_dimension_base;
        coord_type room_dimension_variation = parameters.room_dimension_variation;
//        double room_dimension_mean = 7.0;
//        double room_dimension_stddev = 1.5;
//        double room_dimension_halfwidth = 4.0;
        auto room_dimensionalizer = [room_dimension_base, room_dimension_variation]() -> coord_type { return room_dimension_base + random.gen(room_dimension_variation); }; // linear probability density
//        auto room_dimensionalizer = [room_dimension_mean, room_dimension_stddev]() -> coord_type { return static_cast<coord_type>round(random.genn(room_dimension_mean, room_dimension_stddev)); }; // normal distribution
//        auto room_dimensionalizer = [room_dimension_mean, room_dimension_halfwidth]() -> coord_type { return static_cast<coord_type>round(random.gent(room_dimension_mean, room_dimension_halfwidth)); }; // triangular distribution
        double room_density = parameters.room_density;
        bool room_merge = parameters.room_merge;
        double corridor_straightness = parameters.corridor_straightness;
        ShaftPlacements placements = parameters.shaft_placements;
        double probability_dead_end_removal = parameters.probability_dead_end_removal;
        double probability_loop_removal = parameters.probability_loop_removal;

        // Level

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
        if (room_density < 1.0)
        {
            place_rooms(level, room_density, room_dimensionalizer, room_dimension_base + room_dimension_variation, room_merge);
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
        excavate(level, corridor_straightness);

        // ShaftPlacements
        place_shafts(level, placements);

        // Dead end and loop removal
level.render(cout);
cout << endl;
        shorten_dead_ends(level, probability_dead_end_removal);
level.render(cout);
cout << endl;
        shorten_loops(level, probability_loop_removal);
        return level;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    string filepath = "Level_0.level";
    Parameters parameters;

    try
    {
        for (int i = 1; i < argc; ++i)
        {
            string arg = argv[i];

            if (arg == "--size" && i + 1 < argc)
            {
                auto [nx, ny] = to_pair(argv[++i], ',');
                parameters.size = {static_cast<coord_type>(nx), static_cast<coord_type>(ny)};
            }
            else if (arg == "--room-dimension-base" && i + 1 < argc)
            {
                parameters.room_dimension_base = static_cast<coord_type>(to_integer(argv[++i]));
            }
            else if (arg == "--room-dimension-variation" && i + 1 < argc)
            {
                parameters.room_dimension_variation = static_cast<coord_type>(to_integer(argv[++i]));
            }
            else if (arg == "--room-density" && i + 1 < argc)
            {
                parameters.room_density = to_real(argv[++i]);
            }
            else if (arg == "--room-merge")
            {
                parameters.room_merge = true;
            }
            else if (arg == "--corridor-straightness" && i + 1 < argc)
            {
                parameters.corridor_straightness = to_real(argv[++i]);
            }
            else if (arg == "--shaft-placements" && i + 1 < argc)
            {
                parameters.shaft_placements = to_shaft_placements(argv[++i]);
            }
            else if (arg == "--probability-dead-end-removal" && i + 1 < argc)
            {
                parameters.probability_dead_end_removal = to_real(argv[++i]);
            }
            else if (arg == "--probability-loop-removal" && i + 1 < argc)
            {
                parameters.probability_loop_removal = to_real(argv[++i]);
            }
            else if (arg == "--seed" && i + 1 < argc)
            {
                parameters.seed = static_cast<seed_type>(to_integer(argv[++i]));
            }
            else if (arg.starts_with("--"))
            {
                usage(cerr);
                return 1;
            }
            else
            {
                filepath = arg;
            }
        }

        // A run that asked for no seed gets one of its own, so that the header names a seed
        // either way and any level can be produced again
        if (!parameters.seed)
        {
            parameters.seed = random_seed();
        }

        random.engine().seed(parameters.seed.value());

        Level level = create_level(0, "Level 0", parameters);

        ofstream os(filepath, ios_base::binary);
        if (!os)
        {
            throw runtime_error(format("unable to open level file {}", filepath));
        }

        write_header(os, level, parameters);
        level.render(os);

        write_header(cout, level, parameters);
        level.render(cout);
//    cout << level;
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
