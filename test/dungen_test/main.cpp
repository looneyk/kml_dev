//------------------------------------------------------------------------------
// dungen_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/file_util.hpp"
#include "quetzal/dungen/generate.hpp"
#include "quetzal/dungen/place_staircases.hpp"
#include "quetzal/dungen/write_zone.hpp"
#include "quetzal/log/Log.hpp"
#include "quetzal/math/Random.hpp"
#include "quetzal/stratum_format/grid.hpp"
#include "quetzal/stratum_format/read_zone.hpp"
#include "quetzal/stratum_format/references.hpp"
#include "quetzal/stratum_format/RelocatorTarget.hpp"
#include "quetzal/stratum_format/write_zone.hpp"
#include "quetzal/testing/named_checks.hpp"
#include "quetzal/testing/reference_files.hpp"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

using namespace std;
using namespace quetzal;

namespace
{

    // Compares regardless of key order
    using map_type = nlohmann::json;

    //--------------------------------------------------------------------------
    // Returns the wall an entity is sited on, or null if it is sited on a flat cell or on its parent
    const stratum_format::WallAddress* wall_address(const stratum_format::Entity& entity)
    {
        if (!entity.site.has_value() || !entity.site->address.has_value())
        {
            return nullptr;
        }

        return get_if<stratum_format::WallAddress>(&*entity.site->address);
    }

    //--------------------------------------------------------------------------
    // The error place_staircases throws if a count drawn above zero has none placed because the zone has too few candidates
    const string unplaced_rise_error = "at least one of each requested staircase rise could not be placed";

    //--------------------------------------------------------------------------
    // A zone as the tool writes it, header and grid
    string zone_text(const dungen::Parameters& parameters, const string& name = {})
    {
        dungen::Zone zone = dungen::generate(0, name, parameters);

        ostringstream os;
        dungen::write_zone(os, zone, parameters);

        return os.str();
    }

    //--------------------------------------------------------------------------
    // Returns a zone as zone_text writes it, or an empty string with the generator's error if it could not be generated
    string zone_text_or_error(const dungen::Parameters& parameters, string& error)
    {
        try
        {
            return zone_text(parameters);
        }
        catch (const exception& e)
        {
            error = e.what();
            return {};
        }
    }

    //--------------------------------------------------------------------------
    void write_file(const string& filepath, const string& s)
    {
        ofstream os(filepath, ios_base::binary);
        os << s;
        return;
    }

    //--------------------------------------------------------------------------
    string read_file(const string& filepath)
    {
        ifstream is(filepath, ios_base::binary);
        string s;
        string line;

        while (read_line(is, line))
        {
            s += line;
            s += '\n';
        }

        return s;
    }

    //--------------------------------------------------------------------------
    // Returns nothing if the two zone files are equivalent as parsed JSON, or the JSON patch from the reference to the
    // file written
    optional<string> compare_equivalent(const filesystem::path& out_filepath, const filesystem::path& reference_filepath)
    {
        try
        {
            map_type map = map_type::parse(read_file(out_filepath.string()));
            map_type map_expected = map_type::parse(read_file(reference_filepath.string()));

            if (map == map_expected)
            {
                return nullopt;
            }

            return map_type::diff(map_expected, map).dump();
        }
        catch (const map_type::parse_error& e)
        {
            return e.what();
        }
    }

    //--------------------------------------------------------------------------
    // The whole string or nothing; trailing characters are an error
    dungen::coord_type to_integer(const string& s)
    {
        dungen::coord_type value = 0;
        auto result = from_chars(s.data(), s.data() + s.size(), value);

        if (result.ec != errc {} || result.ptr != s.data() + s.size())
        {
            throw invalid_argument(format("\"{}\" is not a whole number", s));
        }

        return value;
    }

    //--------------------------------------------------------------------------
    double to_real(const string& s)
    {
        double value = 0.0;
        auto result = from_chars(s.data(), s.data() + s.size(), value);

        if (result.ec != errc {} || result.ptr != s.data() + s.size())
        {
            throw invalid_argument(format("\"{}\" is not a number", s));
        }

        return value;
    }

    //--------------------------------------------------------------------------
    // Rejects a negative count, as counts are size_t
    size_t to_count(const string& s)
    {
        dungen::coord_type value = to_integer(s);

        if (value < 0)
        {
            throw invalid_argument(format("\"{}\" is not a count", s));
        }

        return static_cast<size_t>(value);
    }

    //--------------------------------------------------------------------------
    // A closed range of counts, written low to high with a hyphen between
    math::Interval<size_t> to_count_interval(const string& s)
    {
        size_t n = s.find('-');

        if (n == string::npos)
        {
            throw invalid_argument(format("\"{}\" is not two counts separated by '-'", s));
        }

        return {to_count(s.substr(0, n)), to_count(s.substr(n + 1))};
    }

    //--------------------------------------------------------------------------
    // Returns the zone as the reader decodes it. Throws std::runtime_error if the reader rejects it
    stratum_format::Document document_of(const string& zone)
    {
        istringstream is(zone);
        return stratum_format::read_zone(is, "zone");
    }

    //--------------------------------------------------------------------------
    // The parameters in a zone file's generator section, where only the chosen algorithm's group is written
    // Throws std::invalid_argument for a value this does not read, so a parameter added to the writer alone fails
    dungen::Parameters parameters_of(const stratum_format::raw_values_type& generator)
    {
        if (!generator.is_object() || generator.value("name", "") != "dungen")
        {
            throw invalid_argument(format("\"{}\" is not a dungen generator", generator.dump()));
        }

        map<string, string> values;

        for (const auto& [key, value] : generator.at("parameters").items())
        {
            values[key] = value.get<string>();
        }

        // Each value read is removed, so the values left at the end are the ones this does not read
        auto take = [&values](const string& key)
        {
            auto i = values.find(key);
            if (i == values.end())
            {
                throw invalid_argument(format("the zone file specifies no {}", key));
            }

            string value = std::move(i->second);
            values.erase(i);
            return value;
        };

        dungen::Parameters parameters;

        parameters.algorithm = dungen::to_algorithm(take("algorithm"));
        parameters.staircase_down_count = to_count_interval(take("stairs_down"));
        parameters.staircase_up_count = to_count_interval(take("stairs_up"));
        parameters.seed = math::to_seed(take("seed"));

        if (values.contains("position"))
        {
            string position = take("position");
            dungen::zone_position_type::vector_type v;
            math::from_chars(position.data(), position.data() + position.size(), v, ",");
            parameters.position = dungen::zone_position_type(v);
        }

        string extent = take("extent");
        size_t n = extent.find(',');
        parameters.extent = {to_integer(extent.substr(0, n)), to_integer(extent.substr(n + 1))};

        if (parameters.algorithm == dungen::Algorithm::Maze1)
        {
            dungen::Maze1Parameters& maze1 = parameters.maze1;
            maze1.room_dimension_base = to_integer(take("room_dimension_base"));
            maze1.room_dimension_variation = to_integer(take("room_dimension_variation"));
            maze1.room_density = to_real(take("room_density"));
            maze1.room_merge = take("room_merge") == "true";
            maze1.corridor_straightness = to_real(take("corridor_straightness"));
            maze1.probability_dead_end_removal = to_real(take("probability_dead_end_removal"));
            maze1.probability_loop_removal = to_real(take("probability_loop_removal"));
            maze1.door_probability = to_real(take("door_probability"));
        }
        else if (parameters.algorithm == dungen::Algorithm::Grid1)
        {
            dungen::Grid1Parameters& grid1 = parameters.grid1;

            string divisions = take("divisions");
            size_t d = divisions.find(',');
            grid1.divisions = {to_integer(divisions.substr(0, d)), to_integer(divisions.substr(d + 1))};

            grid1.room_density = to_real(take("room_density"));
            grid1.room_dimension_min = to_integer(take("room_dimension_min"));
            grid1.room_margin = to_integer(take("room_margin"));
            grid1.loop = to_real(take("loop"));
            grid1.door_probability = to_real(take("door_probability"));
        }
        else
        {
            dungen::Cave1Parameters& cave1 = parameters.cave1;
            cave1.fill = to_real(take("fill"));
            cave1.smoothing_passes = to_count(take("smoothing_passes"));
            cave1.birth_threshold = to_count(take("birth_threshold"));
            cave1.survival_threshold = to_count(take("survival_threshold"));
        }

        if (!values.empty())
        {
            throw invalid_argument(format("the zone file specifies {}, which this does not read", values.begin()->first));
        }

        return parameters;
    }

    //--------------------------------------------------------------------------
    // The runs the reference zone files are written from, each algorithm at a fixed seed. Some zones are named and some are
    // left without a name
    struct Scenario
    {
        string name;
        string zone_name;
        dungen::Parameters parameters;
    };

    vector<Scenario> scenarios()
    {
        vector<Scenario> v;

        dungen::Parameters maze1;
        maze1.algorithm = dungen::Algorithm::Maze1;
        maze1.extent = {31, 21};
        maze1.seed = 1u;
        v.push_back({"maze1", "Cistern Level 1", maze1});

        dungen::Parameters grid1;
        grid1.algorithm = dungen::Algorithm::Grid1;
        grid1.extent = {33, 25};
        grid1.seed = 42u;
        v.push_back({"grid1", "Barracks Level 2", grid1});

        dungen::Parameters cave1;
        cave1.algorithm = dungen::Algorithm::Cave1;
        cave1.extent = {40, 30};
        cave1.seed = 12345u;
        v.push_back({"cave1", "Caverns Level 3", cave1});

        // No staircase leading down, which the default counts do not cover
        dungen::Parameters up_only;
        up_only.algorithm = dungen::Algorithm::Maze1;
        up_only.extent = {21, 15};
        up_only.staircase_down_count = {0, 0};
        up_only.seed = 7u;
        v.push_back({"up_only", "", up_only});

        // A seed past the halfway mark of its range, which a signed read would lose
        dungen::Parameters wide;
        wide.algorithm = dungen::Algorithm::Grid1;
        wide.extent = {25, 19};
        wide.seed = 4294967295u;
        v.push_back({"wide_seed", "", wide});

        // Doorways left open, which barriers close off from the rooms they lead into
        dungen::Parameters grid1_open = grid1;
        grid1_open.grid1.door_probability = 0.5;
        v.push_back({"grid1_open_doorways", "Open Barracks", grid1_open});

        dungen::Parameters maze1_open = maze1;
        maze1_open.maze1.door_probability = 0.5;
        v.push_back({"maze1_open_doorways", "", maze1_open});

        return v;
    }

    //--------------------------------------------------------------------------
    // Zone files written now against those checked in, which must be equivalent
    TEST_CASE("reference zones")
    {
        filesystem::create_directories(testing::out_directory);

        for (const Scenario& scenario : scenarios())
        {
            SUBCASE(format("{} is equivalent to its reference", scenario.name))
            {
                const string filename = scenario.name + ".zone";
                write_file((testing::out_directory / filename).string(), zone_text(scenario.parameters, scenario.zone_name));
                testing::check_reference(filename, compare_equivalent);
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A zone file has every parameter of its run, so it can be reproduced from the file alone
    TEST_CASE("parameters reproduce their zone")
    {
        for (const Scenario& scenario : scenarios())
        {
            string reference = read_file(format("reference/{}.zone", scenario.name));
            CHECK_CASE(format("{} is checked in", scenario.name), !reference.empty());

            if (reference.empty())
            {
                continue;
            }

            try
            {
                // The name is the zone's, not a parameter of its run, so it is copied as the file wrote it
                stratum_format::Document document = document_of(reference);
                string name = document.components.is_object() ? document.components.value("name", string()) : string();

                dungen::Parameters parameters = parameters_of(document.generator);
                CHECK_CASE(format("{} regenerates from its own parameters", scenario.name),
                    map_type::parse(zone_text(parameters, name)) == map_type::parse(reference));
            }
            catch (const exception& e)
            {
                CHECK_CASE(format("{} regenerates from its own parameters", scenario.name), false, e.what());
            }
        }

        stratum_format::raw_values_type generator = document_of(zone_text(scenarios().front().parameters)).generator;
        generator["parameters"]["unread"] = "1";

        bool rejected = false;
        try
        {
            parameters_of(generator);
        }
        catch (const invalid_argument&)
        {
            rejected = true;
        }

        CHECK_CASE("a parameter this does not read is rejected", rejected);

        // A position, when given, is written into the layout and the parameters both
        dungen::Parameters positioned = scenarios().front().parameters;
        positioned.position = dungen::zone_position_type {3, -4, -2};
        stratum_format::Document positioned_document = document_of(zone_text(positioned));

        CHECK_CASE("a zone given a position has it in its layout", positioned_document.position == positioned.position);
        CHECK_CASE("and in its parameters, from which it regenerates", parameters_of(positioned_document.generator).position
            == positioned.position && !document_of(zone_text(scenarios().front().parameters)).position.has_value());

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("determinism")
    {
        dungen::Parameters parameters;
        parameters.extent = {21, 15};
        parameters.seed = 7u;

        CHECK_CASE("one seed produces one zone", zone_text(parameters) == zone_text(parameters));

        dungen::Parameters other = parameters;
        other.seed = 8u;
        CHECK_CASE("and another seed produces another", zone_text(other) != zone_text(parameters));

        auto write = [&](id_type id, const string& name)
        {
            ostringstream os;
            dungen::write_zone(os, dungen::generate(id, name, parameters), parameters);
            return os.str();
        };

        CHECK_CASE("a Zone's id is not written to its file", write(0, "one") == write(7, "one"));

        stratum_format::Document named = document_of(write(0, "Cistern Level 1"));
        stratum_format::Document unnamed = document_of(write(0, ""));
        CHECK_CASE("a Zone's name is written as the layout's name",
            named.components.is_object() && named.components.value("name", string()) == "Cistern Level 1");
        CHECK_CASE("an empty name is not written", !unnamed.components.is_object() || !unnamed.components.contains("name"));
        CHECK_CASE("and the name does not affect the grid", named.cells.rows == unnamed.cells.rows);

        CHECK_CASE("a random seed is not the same one every time",
            dungen::random_seed() != dungen::random_seed());

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("parameter validation")
    {
        auto rejected = [](const dungen::Parameters& parameters)
        {
            try
            {
                dungen::validate(parameters);
            }
            catch (const invalid_argument&)
            {
                return true;
            }

            return false;
        };

        dungen::Parameters parameters;
        parameters.extent = {21, 15};
        parameters.seed = 1u;
        CHECK_CASE("a sound set of parameters passes", !rejected(parameters));

        dungen::Parameters seedless = parameters;
        seedless.seed.reset();
        CHECK_CASE("no seed is rejected rather than quietly fixed", rejected(seedless));

        dungen::Parameters empty = parameters;
        empty.extent = {0, 0};
        CHECK_CASE("an empty size is rejected", rejected(empty));

        // Only the chosen algorithm's group is read, so another group's invalid values are not rejected
        dungen::Parameters unread = parameters;
        unread.algorithm = dungen::Algorithm::Maze1;
        unread.cave1.fill = -1.0;
        CHECK_CASE("a group the algorithm does not read is not judged", !rejected(unread));

        dungen::Parameters pinched = parameters;
        pinched.algorithm = dungen::Algorithm::Grid1;
        pinched.grid1.room_dimension_min = 1;
        CHECK_CASE("a room of a single cell is allowed", !rejected(pinched));

        return;
    }

    //--------------------------------------------------------------------------
    // Every algorithm produces a zone the reader accepts, of the requested size
    TEST_CASE("grid dimensions")
    {
        for (const Scenario& scenario : scenarios())
        {
            vector<string> rows;

            try
            {
                string text = zone_text(scenario.parameters, scenario.zone_name);
                stratum_format::Document document = document_of(text);
                rows = document.cells.rows;
                CHECK_CASE(format("{} is a zone the reader accepts", scenario.name), true);

                ostringstream os;
                stratum_format::write(os, document);
                CHECK_CASE(format("{} reads back as the document built in memory", scenario.name), os.str() == text);
            }
            catch (const runtime_error& e)
            {
                CHECK_CASE(format("{} is a zone the reader accepts", scenario.name), false, e.what());
                continue;
            }

            auto [nx, ny] = scenario.parameters.extent.rep();

            CHECK_CASE_EQUAL(format("{} has a row per cell", scenario.name), rows.size(), static_cast<size_t>(ny));
            CHECK_CASE(format("{} has a column per cell", scenario.name),
                !rows.empty() && rows.front().size() == static_cast<size_t>(nx));
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A doorway is one cell of a wall line, so two side by side are one doorway two cells wide
    bool doorways_in_wall_lines(const string& zone, string& detail)
    {
        vector<string> rows;

        try
        {
            rows = document_of(zone).cells.rows;
        }
        catch (const runtime_error& e)
        {
            detail = e.what();
            return false;
        }

        auto off_grid = '#';

        auto at = [&rows, off_grid](dungen::coord_type x, dungen::coord_type y)
        {
            if (y < 0 || static_cast<size_t>(y) >= rows.size())
            {
                return off_grid;
            }

            const string& r = rows[static_cast<size_t>(y)];
            return x >= 0 && static_cast<size_t>(x) < r.size() ? r[static_cast<size_t>(x)] : off_grid;
        };

        auto wall = [](char c) { return c == '#' || c == '+'; };

        for (dungen::coord_type y = 0; y < static_cast<dungen::coord_type>(rows.size()); ++y)
        {
            for (dungen::coord_type x = 0; x < static_cast<dungen::coord_type>(rows[static_cast<size_t>(y)].size()); ++x)
            {
                if (at(x, y) != '+')
                {
                    continue;
                }

                if (!(wall(at(x, y - 1)) && wall(at(x, y + 1)))
                    && !(wall(at(x - 1, y)) && wall(at(x + 1, y))))
                {
                    detail = format("doorway at column {}, row {} has wall on neither axis", x, y);
                    return false;
                }
            }
        }

        return true;
    }

    //--------------------------------------------------------------------------
    // Read from the written zone alone: one door entity per door glyph, its door facings on the walls of that doorway
    // in the order door_walls returns them, and no door facing anywhere else
    bool doors_have_entities(const string& zone, string& detail)
    {
        stratum_format::Document document;

        try
        {
            document = document_of(zone);
        }
        catch (const runtime_error& e)
        {
            detail = e.what();
            return false;
        }

        vector<vector<stratum_format::WallAddress>> doors = stratum_format::door_walls(document.cells);
        map<stratum_format::stored_entity_id_type, vector<stratum_format::WallAddress>> walls_by_door;
        size_t door_count = 0;

        for (const stratum_format::Entity& entity : document.entities)
        {
            if (entity.type == "door")
            {
                ++door_count;
                continue;
            }

            if (entity.type != "door_facing")
            {
                continue;
            }

            const auto* address = wall_address(entity);
            if (address == nullptr || !entity.parent_id.has_value())
            {
                detail = format("a door facing with id {} that is not a child attached to a wall", entity.id);
                return false;
            }

            walls_by_door[*entity.parent_id].push_back(*address);
        }

        if (door_count != doors.size() || walls_by_door.size() != doors.size())
        {
            detail = format("{} door glyphs, {} door entities and {} of them with door facings", doors.size(), door_count, walls_by_door.size());
            return false;
        }

        for (const vector<stratum_format::WallAddress>& walls : doors)
        {
            if (none_of(walls_by_door.begin(), walls_by_door.end(), [&walls](const auto& entry) { return entry.second == walls; }))
            {
                const stratum_format::WallAddress& wall = walls.front();
                detail = format("no door with a door facing on each wall of the doorway at [{}, {}] normal [{}, {}]",
                    wall.cell_position.i(), wall.cell_position.j(), wall.normal.x(), wall.normal.y());
                return false;
            }
        }

        return true;
    }

    //--------------------------------------------------------------------------
    // Swept over seeds as well as the checked in zone files, as a placement fault depends on where the rooms are
    TEST_CASE("doorway placement")
    {
        constexpr dungen::Parameters::seed_type seed_count = 64;

        for (const Scenario& scenario : scenarios())
        {
            string detail;
            CHECK_CASE(format("{} places every doorway in a wall line", scenario.name),
                doorways_in_wall_lines(zone_text(scenario.parameters), detail), detail);
        }

        for (const Scenario& scenario : scenarios())
        {
            string detail;
            CHECK_CASE(format("{} writes a door entity at each door", scenario.name), doors_have_entities(zone_text(scenario.parameters), detail),
                detail);
        }

        for (const Scenario& scenario : scenarios())
        {
            string detail;
            size_t failures = 0;
            size_t unplaced = 0;

            for (dungen::Parameters::seed_type seed = 1; seed <= seed_count; ++seed)
            {
                dungen::Parameters parameters = scenario.parameters;
                parameters.seed = seed;

                string seed_detail;
                string zone = zone_text_or_error(parameters, seed_detail);
                if (zone.empty() && seed_detail == unplaced_rise_error)
                {
                    ++unplaced;
                }
                else if (zone.empty() || !doorways_in_wall_lines(zone, seed_detail) || !doors_have_entities(zone, seed_detail))
                {
                    if (failures == 0)
                    {
                        detail = format("seed {}, {}", seed, seed_detail);
                    }

                    ++failures;
                }
            }

            CHECK_CASE(format("{} holds over {} seeds", scenario.name, seed_count), failures == 0 && unplaced < seed_count,
                format("{} seeds failed, first at {}; {} with too few candidates for a staircase", failures, detail, unplaced));
        }

        // A density below one leaves sectors with a single cell instead of a room, and the smallest dimension allows rooms
        // one cell across; both produce corridor layouts the defaults do not
        struct Variant
        {
            string name;
            double room_density;
            dungen::coord_type room_dimension_min;
        };

        const Variant variants[] {{"sparse rooms", 0.4, 3}, {"sparsest rooms", 0.2, 3},
            {"rooms of a single cell", 1.0, 1}, {"sparse rooms of a single cell", 0.4, 1}};

        for (const Variant& variant : variants)
        {
            string detail;
            size_t failures = 0;
            size_t unplaced = 0;

            for (dungen::Parameters::seed_type seed = 1; seed <= seed_count; ++seed)
            {
                dungen::Parameters parameters;
                parameters.algorithm = dungen::Algorithm::Grid1;
                parameters.extent = {33, 25};
                parameters.seed = seed;
                parameters.grid1.room_density = variant.room_density;
                parameters.grid1.room_dimension_min = variant.room_dimension_min;

                string seed_detail;
                string zone = zone_text_or_error(parameters, seed_detail);
                if (zone.empty() && seed_detail == unplaced_rise_error)
                {
                    ++unplaced;
                }
                else if (zone.empty() || !doorways_in_wall_lines(zone, seed_detail) || !doors_have_entities(zone, seed_detail))
                {
                    if (failures == 0)
                    {
                        detail = format("seed {}, {}", seed, seed_detail);
                    }

                    ++failures;
                }
            }

            CHECK_CASE(format("grid1 holds with {}", variant.name), failures == 0 && unplaced < seed_count,
                format("{} seeds failed, first at {}; {} with too few candidates for a staircase", failures, detail, unplaced));
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Returns true if the rooms the reader finds contain exactly the cells generation laid out as each room. It fails if a
    // barrier is missing and a room's fill leaks out through an open doorway
    bool rooms_read_back(const dungen::Parameters& parameters, string& detail)
    {
        optional<dungen::Zone> generated;

        try
        {
            generated.emplace(dungen::generate(0, "zone", parameters));
        }
        catch (const exception& e)
        {
            detail = e.what();
            return false;
        }

        const dungen::Zone& zone = *generated;

        vector<set<stratum_format::discrete_point_type>> cells_expected(zone.rooms().size());
        auto [nx, ny] = zone.extent().rep();

        for (dungen::coord_type y = 0; y < ny; ++y)
        {
            for (dungen::coord_type x = 0; x < nx; ++x)
            {
                // A stair cell is a block, and in no room
                const dungen::Cell& cell = zone.cell({x, y});
                if (cell.type() == dungen::CellType::Room)
                {
                    if (cell.room_id() != nullid)
                    {
                        cells_expected[cell.room_id()].insert({x, y});
                    }
                }
            }
        }

        ostringstream os;
        dungen::write_zone(os, zone, parameters);

        vector<set<stratum_format::discrete_point_type>> cells;

        try
        {
            cells = stratum_format::room_cells(document_of(os.str()));
        }
        catch (const runtime_error& e)
        {
            detail = e.what();
            return false;
        }

        if (cells.size() != cells_expected.size())
        {
            detail = format("{} rooms read back, {} generated", cells.size(), cells_expected.size());
            return false;
        }

        for (size_t room_id = 0; room_id < cells.size(); ++room_id)
        {
            if (cells[room_id] != cells_expected[room_id])
            {
                detail = format("room {} reads back {} cells, {} generated", room_id, cells[room_id].size(), cells_expected[room_id].size());
                return false;
            }
        }

        return true;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("barriers")
    {
        constexpr dungen::Parameters::seed_type seed_count = 64;

        struct Variant
        {
            string name;
            dungen::Algorithm algorithm;
            double door_probability;
        };

        const Variant variants[] {{"grid1 with every door", dungen::Algorithm::Grid1, 1.0},
            {"grid1 with half its doors", dungen::Algorithm::Grid1, 0.5}, {"grid1 with no doors", dungen::Algorithm::Grid1, 0.0},
            {"maze1 with every door", dungen::Algorithm::Maze1, 1.0}, {"maze1 with half its doors", dungen::Algorithm::Maze1, 0.5},
            {"maze1 with no doors", dungen::Algorithm::Maze1, 0.0}};

        for (const Variant& variant : variants)
        {
            string detail;
            size_t failures = 0;

            for (dungen::Parameters::seed_type seed = 1; seed <= seed_count; ++seed)
            {
                dungen::Parameters parameters;
                parameters.algorithm = variant.algorithm;
                parameters.extent = {33, 25};
                parameters.seed = seed;
                parameters.grid1.door_probability = variant.door_probability;
                parameters.maze1.door_probability = variant.door_probability;

                string seed_detail;
                if (!rooms_read_back(parameters, seed_detail))
                {
                    if (failures == 0)
                    {
                        detail = format("seed {}, {}", seed, seed_detail);
                    }

                    ++failures;
                }
            }

            CHECK_CASE(format("{} reads back its rooms over {} seeds", variant.name, seed_count), failures == 0,
                format("{} seeds failed, first at {}", failures, detail));
        }

        dungen::Parameters doorless;
        doorless.algorithm = dungen::Algorithm::Grid1;
        doorless.extent = {33, 25};
        doorless.seed = 42u;
        doorless.grid1.door_probability = 0.0;

        const vector<string> rows = document_of(zone_text(doorless)).cells.rows;
        CHECK_CASE("a door probability of zero leaves no door",
            none_of(rows.begin(), rows.end(), [](const string& row) { return row.find('+') != string::npos; }));

        return;
    }

    //--------------------------------------------------------------------------
    // Checks the written zone, and checks each stair cell's surroundings, which the file does not record, in the zone
    // generated again. There must be as many staircases each way as the counts allow, and no floor cut off by a stair
    // block. No staircase may block a doorway if a cell beside it would not. Every staircase must be sited on the wall
    // on an open side with floor beyond, or with a door if no side has floor beyond, and the access points must refer to
    // every staircase
    bool staircases_placed_and_faced(const string& zone, const dungen::Parameters& parameters, string& detail)
    {
        using stratum_format::coord_type;
        using stratum_format::discrete_point_type;
        using stratum_format::discrete_vector_type;

        stratum_format::Document document;

        try
        {
            document = document_of(zone);
        }
        catch (const runtime_error& e)
        {
            detail = e.what();
            return false;
        }

        const stratum_format::Cells& cells = document.cells;
        const auto& directions = math::cardinal_directions<coord_type, 2>;

        map<discrete_point_type, size_t> room_of;
        vector<set<discrete_point_type>> rooms = stratum_format::room_cells(document);

        for (size_t room_id = 0; room_id < rooms.size(); ++room_id)
        {
            for (const discrete_point_type& cell_position : rooms[room_id])
            {
                room_of[cell_position] = room_id;
            }
        }

        auto glyph = [&cells](const discrete_point_type& cell_position)
        {
            return stratum_format::on_grid(cells, cell_position)
                ? cells.rows[cells.rows.size() - 1 - static_cast<size_t>(cell_position.j())][static_cast<size_t>(cell_position.i())] : '#';
        };

        auto floor_stair = [](char c) { return c == '.' || c == '>' || c == '<'; };

        dungen::Zone generated = dungen::generate(0, "zone", parameters);

        auto space = [&](const discrete_point_type& cell_position)
        {
            if (glyph(cell_position) == '>' || glyph(cell_position) == '<')
            {
                id_type room_id = generated.cell(cell_position).room_id();
                return room_id == nullid ? optional<size_t>() : optional<size_t>(room_id);
            }

            auto i = room_of.find(cell_position);
            return i == room_of.end() ? optional<size_t>() : optional<size_t>(i->second);
        };

        auto doorway = [&](const discrete_point_type& cell_position)
        {
            if (glyph(cell_position) == '+')
            {
                return true;
            }

            if (glyph(cell_position) != '.' || space(cell_position).has_value())
            {
                return false;
            }

            return any_of(directions.begin(), directions.end(), [&](const discrete_vector_type& direction)
                { return floor_stair(glyph(cell_position + direction)) && space(cell_position + direction).has_value(); });
        };

        auto blocks_doorway = [&](const discrete_point_type& cell_position)
        {
            return any_of(directions.begin(), directions.end(), [&](const discrete_vector_type& direction) { return doorway(cell_position + direction); });
        };

        // With every stair block in place, each floor and doorway cell is reachable from every other, a door being
        // passable once opened
        auto passable = [&](const discrete_point_type& cell_position) { return glyph(cell_position) == '.' || glyph(cell_position) == '+'; };
        set<discrete_point_type> passable_cells;

        for (coord_type j = 0; j < cells.extent.y(); ++j)
        {
            for (coord_type i = 0; i < cells.extent.x(); ++i)
            {
                if (passable({i, j}))
                {
                    passable_cells.insert({i, j});
                }
            }
        }

        set<discrete_point_type> reached;
        vector<discrete_point_type> pending;

        if (!passable_cells.empty())
        {
            reached.insert(*passable_cells.begin());
            pending.push_back(*passable_cells.begin());
        }

        while (!pending.empty())
        {
            discrete_point_type cell_position = pending.back();
            pending.pop_back();

            for (const discrete_vector_type& direction : directions)
            {
                if (passable(cell_position + direction) && reached.insert(cell_position + direction).second)
                {
                    pending.push_back(cell_position + direction);
                }
            }
        }

        // maze1 can leave corridor that no door reaches, so is not held to this yet
        for (const discrete_point_type& cell_position : passable_cells)
        {
            if (parameters.algorithm != dungen::Algorithm::Maze1 && !reached.contains(cell_position))
            {
                detail = format("floor at {}, {} is cut off", cell_position.i(), cell_position.j());
                return false;
            }
        }

        size_t down_count = 0;
        size_t up_count = 0;
        set<discrete_point_type> staircase_cells;

        for (coord_type j = 0; j < cells.extent.y(); ++j)
        {
            for (coord_type i = 0; i < cells.extent.x(); ++i)
            {
                discrete_point_type cell_position {i, j};
                if (glyph(cell_position) != '>' && glyph(cell_position) != '<')
                {
                    continue;
                }

                ++(glyph(cell_position) == '>' ? down_count : up_count);
                staircase_cells.insert(cell_position);

                if (blocks_doorway(cell_position) && any_of(directions.begin(), directions.end(), [&](const discrete_vector_type& direction)
                    {
                        discrete_point_type neighbor = cell_position + direction;
                        return glyph(neighbor) == '.' && space(neighbor) == space(cell_position) && !doorway(neighbor) && !blocks_doorway(neighbor);
                    }))
                {
                    detail = format("staircase at {}, {} blocks a doorway with a cell free beside it", i, j);
                    return false;
                }

                vector<discrete_vector_type> open_sides;

                for (const stratum_format::Entity& entity : document.entities)
                {
                    // Whatever its type, the staircase's entity is the one with a relocator on a wall that a neighbor presents to
                    // this cell
                    const auto* address = wall_address(entity);
                    if (address == nullptr || !entity.relocator.has_value() || address->cell_position - address->normal != cell_position)
                    {
                        continue;
                    }

                    open_sides.push_back(address->normal);

                    stratum_format::RelocatorTarget target {nullopt, stratum_format::RelativeZone {glyph(cell_position) == '>' ? -1 : 1}, nullopt};
                    if (const auto* unbound = get_if<stratum_format::RelocatorTarget>(&entity.relocator->target); unbound == nullptr || *unbound != target)
                    {
                        detail = format("staircase at {}, {} does not have the target {}", i, j, stratum_format::format_relocator_target(target));
                        return false;
                    }
                }

                if (open_sides.size() != 1)
                {
                    detail = format("staircase at {}, {} is attached to {} of the walls around it", i, j, open_sides.size());
                    return false;
                }

                const discrete_vector_type& open_side = open_sides.front();
                bool floor_beside = any_of(directions.begin(), directions.end(), [&](const discrete_vector_type& direction) { return glyph(cell_position + direction) == '.'; });
                char opened_onto = glyph(cell_position + open_side);

                if (!(opened_onto == '.' || (opened_onto == '+' && !floor_beside)))
                {
                    detail = format("staircase at {}, {} opens onto {}, {}", i, j, open_side.x(), open_side.y());
                    return false;
                }
            }
        }

        // Running out of candidates places fewer than were drawn, which is a warning, but a count drawn above zero places at
        // least one, and a range whose minimum is above zero draws above zero
        const math::Interval<size_t>& down = parameters.staircase_down_count;
        const math::Interval<size_t>& up = parameters.staircase_up_count;

        if (down_count > down.upper() || up_count > up.upper() || (down.lower() > 0 && down_count == 0) || (up.lower() > 0 && up_count == 0))
        {
            detail = format("{} staircases down for {}-{} and {} up for {}-{}", down_count, down.lower(), down.upper(), up_count, up.lower(), up.upper());
            return false;
        }

        set<discrete_point_type> access_point_cells;

        for (stratum_format::stored_entity_id_type id : document.access_points)
        {
            auto entity = find_if(document.entities.begin(), document.entities.end(), [id](const stratum_format::Entity& e) { return e.id == id; });
            const auto* address = entity != document.entities.end() ? wall_address(*entity) : nullptr;

            if (address != nullptr)
            {
                access_point_cells.insert(address->cell_position - address->normal);
            }
        }

        if (document.access_points.size() != access_point_cells.size() || access_point_cells != staircase_cells)
        {
            detail = format("the {} access points are not one on every staircase", document.access_points.size());
            return false;
        }

        return true;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("staircases")
    {
        constexpr dungen::Parameters::seed_type seed_count = 64;

        for (const Scenario& scenario : scenarios())
        {
            string detail;
            CHECK_CASE(format("{} places and faces its staircases", scenario.name),
                staircases_placed_and_faced(zone_text(scenario.parameters), scenario.parameters, detail), detail);
        }

        for (const Scenario& scenario : scenarios())
        {
            string detail;
            size_t failures = 0;
            size_t unplaced = 0;

            for (dungen::Parameters::seed_type seed = 1; seed <= seed_count; ++seed)
            {
                dungen::Parameters parameters = scenario.parameters;
                parameters.seed = seed;
                parameters.staircase_down_count = {0, 2};
                parameters.staircase_up_count = {1, 3};

                string seed_detail;
                string zone = zone_text_or_error(parameters, seed_detail);
                if (zone.empty() && seed_detail == unplaced_rise_error)
                {
                    ++unplaced;
                }
                else if (zone.empty() || !staircases_placed_and_faced(zone, parameters, seed_detail))
                {
                    if (failures == 0)
                    {
                        detail = format("seed {}, {}", seed, seed_detail);
                    }

                    ++failures;
                }
            }

            CHECK_CASE(format("{} places and faces ranges of staircases over {} seeds", scenario.name, seed_count),
                failures == 0 && unplaced < seed_count,
                format("{} seeds failed, first at {}; {} with too few candidates for a staircase", failures, detail, unplaced));
        }

        // A corridor along row 1 from column 1, in solid
        auto corridor = [](dungen::coord_type length)
        {
            dungen::Zone zone(0, "corridor", {length + 2, 3}, dungen::CellType::Solid);
            for (dungen::coord_type x = 1; x <= length; ++x)
            {
                zone.cell({x, 1}).set_type(dungen::CellType::Corridor);
            }

            return zone;
        };

        // Returns the error place_staircases throws, or an empty string if it places what it can. Adds the messages of the
        // warnings it writes to the process-wide log under dungen's category to warnings
        auto placing = [](dungen::Zone& zone, const vector<dungen::point_type>& candidates, math::Interval<size_t> up, math::Interval<size_t> down,
            vector<string>& warnings)
        {
            dungen::random_type random(1u);
            log::logger().add_sink([&warnings](const log::LogEntry& entry)
            {
                if (entry.level == log::LogLevel::Warning && entry.category == dungen::log_category)
                {
                    warnings.push_back(entry.message);
                }
            });

            string error;
            try
            {
                dungen::place_staircases(zone, candidates, up, down, random);
            }
            catch (const runtime_error& e)
            {
                error = e.what();
            }

            log::logger().clear_sinks();
            return error;
        };

        {
            dungen::Zone zone = corridor(2);
            vector<string> warnings;

            CHECK_CASE("a staircase is placed on a candidate", placing(zone, {{1, 1}}, {1, 1}, {0, 0}, warnings).empty()
                && zone.staircases().size() == 1 && zone.cell({1, 1}).type() == dungen::CellType::Staircase);
            CHECK_CASE("facing the floor beside it", !zone.staircases().empty() && zone.staircases()[0].direction() == dungen::vector_type {1, 0});
            CHECK_CASE("the floor a staircase faces gets no second staircase", placing(zone, {{2, 1}}, {1, 1}, {0, 0}, warnings) == unplaced_rise_error);
        }

        {
            dungen::Zone zone = corridor(3);
            vector<string> warnings;

            CHECK_CASE("a count drawn above zero with none placed is an error", placing(zone, {{1, 1}}, {1, 1}, {1, 1}, warnings) == unplaced_rise_error);
        }

        {
            dungen::Zone zone = corridor(3);
            vector<string> warnings;

            CHECK_CASE("a candidate that would cut a passage in two is skipped", placing(zone, {{2, 1}}, {1, 1}, {0, 0}, warnings) == unplaced_rise_error);
        }

        {
            dungen::Zone zone = corridor(3);
            zone.cell({3, 1}).set_type(dungen::CellType::Door);
            vector<string> warnings;

            CHECK_CASE("a candidate beside a door is skipped", placing(zone, {{2, 1}}, {1, 1}, {0, 0}, warnings) == unplaced_rise_error);
        }

        {
            dungen::Zone zone = corridor(5);
            vector<string> warnings;

            bool placed = placing(zone, {{1, 1}, {5, 1}}, {1, 1}, {1, 1}, warnings).empty();
            size_t up_count = static_cast<size_t>(count_if(zone.staircases().begin(), zone.staircases().end(),
                [](const dungen::Staircase& staircase) { return staircase.rise() == dungen::Rise::Up; }));

            CHECK_CASE("staircases leading up and down alternate", placed && zone.staircases().size() == 2 && up_count == 1 && warnings.empty());
        }

        {
            dungen::Zone zone = corridor(5);
            vector<string> warnings;

            bool placed = placing(zone, {{1, 1}, {5, 1}}, {3, 3}, {0, 0}, warnings).empty();
            CHECK_CASE("fewer placed than were drawn is a warning", placed && zone.staircases().size() == 2
                && warnings == vector<string> {"2 of 3 staircases leading up and 0 of 0 leading down placed, the candidates running out"});
        }

        dungen::Parameters reversed;
        reversed.extent = {21, 15};
        reversed.seed = 1u;
        reversed.staircase_down_count = {3, 2};

        bool rejected = false;
        try
        {
            dungen::validate(reversed);
        }
        catch (const invalid_argument&)
        {
            rejected = true;
        }

        CHECK_CASE("a staircase count whose maximum is below its minimum is rejected", rejected);

        dungen::Parameters entryless;
        entryless.extent = {21, 15};
        entryless.seed = 1u;
        entryless.staircase_down_count = {0, 1};
        entryless.staircase_up_count = {0, 1};

        rejected = false;
        try
        {
            dungen::validate(entryless);
        }
        catch (const invalid_argument&)
        {
            rejected = true;
        }

        CHECK_CASE("staircase counts that may both be zero, leaving no entry, are rejected", rejected);

        dungen::Parameters down_only;
        down_only.extent = {21, 15};
        down_only.seed = 1u;
        down_only.staircase_down_count = {1, 1};
        down_only.staircase_up_count = {0, 0};

        rejected = false;
        try
        {
            dungen::validate(down_only);
        }
        catch (const invalid_argument&)
        {
            rejected = true;
        }

        CHECK_CASE("a staircase down alone is rejected, a dungeon zone needing one leading up", rejected);

        return;
    }

} // namespace
