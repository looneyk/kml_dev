//------------------------------------------------------------------------------
// catagen_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/catagen/generate.hpp"
#include "quetzal/catagen/write_zone.hpp"
#include "quetzal/common/file_util.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/math/Interval.hpp"
#include "quetzal/math/Random.hpp"
#include "quetzal/stratum_format/grid.hpp"
#include "quetzal/stratum_format/read_zone.hpp"
#include "quetzal/stratum_format/references.hpp"
#include "quetzal/stratum_format/RelocatorTarget.hpp"
#include "quetzal/stratum_format/write_zone.hpp"
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
#include <utility>
#include <variant>
#include <vector>

using namespace std;
using namespace quetzal;

namespace
{

    // Compares regardless of key order
    using map_type = nlohmann::json;

    constexpr catagen::Parameters::seed_type seed_count = 64;

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
    string zone_text(const catagen::Parameters& parameters, const string& name = {})
    {
        catagen::Zone zone = catagen::generate(name, parameters);

        ostringstream os;
        catagen::write_zone(os, zone, parameters);
        return os.str();
    }

    //--------------------------------------------------------------------------
    void write_file(const filesystem::path& filepath, const string& s)
    {
        ofstream os(filepath, ios_base::binary);
        os << s;
        return;
    }

    //--------------------------------------------------------------------------
    string read_file(const filesystem::path& filepath)
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
    // Returns nothing if the two zone files are equivalent as parsed JSON, or the JSON patch from the reference to the file written
    optional<string> compare_equivalent(const filesystem::path& out_filepath, const filesystem::path& reference_filepath)
    {
        try
        {
            map_type map = map_type::parse(read_file(out_filepath));
            map_type map_expected = map_type::parse(read_file(reference_filepath));

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
    catagen::coord_type to_integer(const string& s)
    {
        catagen::coord_type value = 0;
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
    // Returns the zone as the reader decodes it. Throws std::runtime_error if the reader rejects it
    stratum_format::Document document_of(const string& zone)
    {
        istringstream is(zone);
        return stratum_format::read_zone(is, "zone");
    }

    //--------------------------------------------------------------------------
    // The parameters in a zone file's generator section
    // Throws std::invalid_argument for a value this does not read, so a parameter added to the writer alone fails
    catagen::Parameters parameters_of(const stratum_format::raw_values_type& generator)
    {
        if (!generator.is_object() || generator.value("name", "") != "catagen")
        {
            throw invalid_argument(format("\"{}\" is not a catagen generator", generator.dump()));
        }

        map<string, string> values;

        for (const auto& [key, value] : generator.at("parameters").items())
        {
            values[key] = value.get<string>();
        }

        auto interval_of = [](const string& s, char separator)
        {
            size_t n = s.find(separator);
            return math::Interval<catagen::coord_type> {
                to_integer(s.substr(0, n)), to_integer(s.substr(n + 1))};
        };

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

        catagen::Parameters parameters;

        string extent = take("extent");
        math::from_chars(extent.data(), extent.data() + extent.size(), parameters.extent, ",");

        parameters.scale = interval_of(take("scale"), '-');
        parameters.spacing = interval_of(take("spacing"), '-');

        parameters.density = to_real(take("density"));
        parameters.loop = to_real(take("loop"));

        for (auto [key, count] : {pair {"stairs_down", &parameters.staircase_down_count}, pair {"stairs_up", &parameters.staircase_up_count}})
        {
            math::Interval<catagen::coord_type> interval = interval_of(take(key), '-');
            if (interval.lower() < 0 || interval.upper() < 0)
            {
                throw invalid_argument(format("{} is not two counts", key));
            }

            *count = {static_cast<size_t>(interval.lower()), static_cast<size_t>(interval.upper())};
        }

        parameters.seed = math::to_seed(take("seed"));

        if (values.contains("level"))
        {
            parameters.level = to_integer(take("level"));
        }

        if (!values.empty())
        {
            throw invalid_argument(format("the zone file specifies {}, which this does not read", values.begin()->first));
        }

        return parameters;
    }

    //--------------------------------------------------------------------------
    // The runs the reference zone files are written from, with fixed parameters and seeds. Some zones are named and some are
    // left without a name
    struct Scenario
    {
        string name;
        string zone_name;
        catagen::Parameters parameters;
    };

    vector<Scenario> scenarios()
    {
        vector<Scenario> v;

        // The shipped zone's parameters, so the test case checks the run that produced the asset
        catagen::Parameters shipped;
        shipped.extent = {20, 10};
        shipped.scale = {2, 5};
        shipped.spacing = {0, 4};
        shipped.density = 0.5;
        shipped.loop = 0.1;
        shipped.seed = 2614047767u;
        v.push_back({"shipped", "", shipped});

        // Rooms sharing walls, which the block notation cannot express
        catagen::Parameters shared;
        shared.extent = {16, 12};
        shared.scale = {3, 7};
        shared.density = 1.0;
        shared.loop = 0.0;
        shared.seed = 1u;
        v.push_back({"shared_walls", "Catacombs Level 1", shared});

        // Rooms spaced apart, so corridors cross the solid cells between them
        catagen::Parameters spaced;
        spaced.extent = {24, 16};
        spaced.scale = {3, 6};
        spaced.spacing = {1, 3};
        spaced.density = 0.7;
        spaced.loop = 0.3;
        spaced.seed = 42u;
        v.push_back({"spaced", "Ossuary Level 2", spaced});

        // A seed past the halfway mark of its range, which a signed read would lose
        catagen::Parameters wide;
        wide.extent = {12, 12};
        wide.scale = {2, 5};
        wide.seed = 4294967295u;
        v.push_back({"wide_seed", "", wide});
        return v;
    }

    //--------------------------------------------------------------------------
    // Read from the written zone alone: one door entity per door glyph, its door facings on the walls of that doorway
    // in the order door_walls returns them
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
    // Checks the written zone, and checks which sides of each stair cell were open, which its written walls hide, in the
    // zone generated again. There must be as many staircases each way as the counts allow, no floor cut off by a stair
    // block, and every stair cell walled toward floor. No staircase may block a doorway if a cell beside it would not. Every
    // staircase must be sited on the wall on an open side with floor beyond, or with a door if there is no such side,
    // and the access points must refer to every staircase
    bool staircases_placed_and_faced(const string& zone, const catagen::Parameters& parameters, string& detail)
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

        // In the written grid, a cell is at its odd position and the edge on each side one position from it
        auto glyph_at = [&cells](coord_type x, coord_type y) { return cells.rows[cells.rows.size() - 1 - static_cast<size_t>(y)][static_cast<size_t>(x)]; };

        auto cell_glyph = [&](const discrete_point_type& cell_position)
        {
            return stratum_format::on_grid(cells, cell_position) ? glyph_at(2 * cell_position.i() + 1, 2 * cell_position.j() + 1) : ' ';
        };

        auto edge_glyph = [&](const discrete_point_type& cell_position, const discrete_vector_type& direction)
        {
            return glyph_at(2 * cell_position.i() + 1 + direction.x(), 2 * cell_position.j() + 1 + direction.y());
        };

        auto blocks_doorway = [&](const discrete_point_type& cell_position)
        {
            return any_of(directions.begin(), directions.end(), [&](const discrete_vector_type& direction) { return edge_glyph(cell_position, direction) == '+'; });
        };

        // With every stair block in place, each floor cell is reachable from every other, a door being passable once opened
        set<discrete_point_type> floor_cells;

        for (coord_type j = 0; j < cells.extent.y(); ++j)
        {
            for (coord_type i = 0; i < cells.extent.x(); ++i)
            {
                if (cell_glyph({i, j}) == '.')
                {
                    floor_cells.insert({i, j});
                }
            }
        }

        set<discrete_point_type> reached;
        vector<discrete_point_type> pending;

        if (!floor_cells.empty())
        {
            reached.insert(*floor_cells.begin());
            pending.push_back(*floor_cells.begin());
        }

        while (!pending.empty())
        {
            discrete_point_type cell_position = pending.back();
            pending.pop_back();

            for (const discrete_vector_type& direction : directions)
            {
                discrete_point_type neighbor = cell_position + direction;
                char edge = edge_glyph(cell_position, direction);

                if (cell_glyph(neighbor) == '.' && (edge == ' ' || edge == '+') && reached.insert(neighbor).second)
                {
                    pending.push_back(neighbor);
                }
            }
        }

        for (const discrete_point_type& cell_position : floor_cells)
        {
            if (!reached.contains(cell_position))
            {
                detail = format("floor at {}, {} is cut off", cell_position.i(), cell_position.j());
                return false;
            }
        }

        catagen::Zone generated = catagen::generate({}, parameters);

        // Open in the zone generated, before the stair cell's walls were written, and with floor beyond
        auto open_to_floor = [&](const discrete_point_type& cell_position, const discrete_vector_type& direction)
        {
            size_t index = static_cast<size_t>(find(directions.begin(), directions.end(), direction) - directions.begin());
            return generated.wall(cell_position, static_cast<math::CardinalDirection>(index)) == catagen::WallType::Open
                && cell_glyph(cell_position + direction) == '.';
        };

        size_t down_count = 0;
        size_t up_count = 0;
        set<discrete_point_type> staircase_cells;

        for (coord_type j = 0; j < cells.extent.y(); ++j)
        {
            for (coord_type i = 0; i < cells.extent.x(); ++i)
            {
                discrete_point_type cell_position {i, j};
                if (cell_glyph(cell_position) != '>' && cell_glyph(cell_position) != '<')
                {
                    continue;
                }

                ++(cell_glyph(cell_position) == '>' ? down_count : up_count);
                staircase_cells.insert(cell_position);

                for (const discrete_vector_type& direction : directions)
                {
                    if (cell_glyph(cell_position + direction) == '.' && edge_glyph(cell_position, direction) == ' ')
                    {
                        detail = format("staircase at {}, {} has no wall on its side {}, {}, toward floor", i, j, direction.x(), direction.y());
                        return false;
                    }
                }

                if (blocks_doorway(cell_position) && any_of(directions.begin(), directions.end(), [&](const discrete_vector_type& direction)
                    {
                        return open_to_floor(cell_position, direction) && !blocks_doorway(cell_position + direction);
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

                    stratum_format::RelocatorTarget target {nullopt, stratum_format::RelativeZone {cell_glyph(cell_position) == '>' ? -1 : 1}, nullopt};
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
                bool floor_beside = any_of(directions.begin(), directions.end(), [&](const discrete_vector_type& direction) { return open_to_floor(cell_position, direction); });

                if (!(open_to_floor(cell_position, open_side) || (edge_glyph(cell_position, open_side) == '+' && !floor_beside)))
                {
                    detail = format("staircase at {}, {} opens onto {}, {}", i, j, open_side.x(), open_side.y());
                    return false;
                }
            }
        }

        // Running out of rooms places fewer staircases, and those leading up are placed first, so a count below its minimum
        // is allowed only if no room was left for the staircases leading down
        const math::Interval<size_t>& down = parameters.staircase_down_count;
        const math::Interval<size_t>& up = parameters.staircase_up_count;

        if (down_count > down.upper() || up_count > up.upper() || (up_count < up.lower() && down_count > 0))
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
    catagen::Parameters sound_parameters()
    {
        catagen::Parameters parameters;
        parameters.extent = {16, 12};
        parameters.seed = 1u;
        return parameters;
    }

    //--------------------------------------------------------------------------
    // The grid alone, as write_grid emits it
    vector<string> grid_rows()
    {
        catagen::Parameters parameters;
        parameters.extent = {8, 6};
        parameters.seed = 3u;

        catagen::Zone zone = catagen::generate({}, parameters);

        ostringstream os;
        catagen::write_grid(os, zone);

        vector<string> rows;
        string row;
        istringstream is(os.str());
        while (getline(is, row))
        {
            rows.push_back(row);
        }

        return rows;
    }

} // namespace

//------------------------------------------------------------------------------
// Zone files written now against those checked in, which must be equivalent
TEST_SUITE("reference zones")
{
    TEST_CASE("each scenario's zone is equivalent to its reference")
    {
        filesystem::create_directories(testing::out_directory);

        for (const Scenario& scenario : scenarios())
        {
            SUBCASE(format("{} is equivalent to its reference", scenario.name))
            {
                const string filename = scenario.name + ".zone";
                write_file(testing::out_directory / filename, zone_text(scenario.parameters, scenario.zone_name));
                testing::check_reference(filename, compare_equivalent);
            }
        }

        return;
    }
}

//------------------------------------------------------------------------------
// A zone file has every parameter of its run, so it can be reproduced from the file alone
TEST_SUITE("parameters reproduce their zone")
{
    TEST_CASE("each scenario regenerates from its own parameters")
    {
        for (const Scenario& scenario : scenarios())
        {
            SUBCASE(format("{} regenerates from its own parameters", scenario.name))
            {
                string reference = read_file(testing::reference_directory / (scenario.name + ".zone"));
                REQUIRE_MESSAGE(!reference.empty(), format("{} is checked in", scenario.name));

                // The name is the zone's, not a parameter of its run, so it is copied as the file wrote it
                stratum_format::Document document = document_of(reference);
                string name = document.components.is_object() ? document.components.value("name", string()) : string();

                map_type difference = map_type::diff(map_type::parse(reference), map_type::parse(zone_text(parameters_of(document.generator), name)));
                CHECK_MESSAGE(difference.empty(), difference.dump());
            }
        }

        return;
    }

    TEST_CASE("a parameter this does not read is rejected")
    {
        stratum_format::raw_values_type generator = document_of(zone_text(scenarios().front().parameters)).generator;
        generator["parameters"]["unread"] = "1";

        CHECK_THROWS_AS(parameters_of(generator), invalid_argument);
        return;
    }

    TEST_CASE("a zone given a level has it in its layout and its parameters, from which it regenerates")
    {
        catagen::Parameters leveled = scenarios().front().parameters;
        leveled.level = 1;
        stratum_format::Document leveled_document = document_of(zone_text(leveled));

        CHECK_MESSAGE((leveled_document.level == 1), "a zone given a level has it in its layout");
        CHECK_MESSAGE((parameters_of(leveled_document.generator).level == 1), "and in its parameters, from which it regenerates");
        CHECK_MESSAGE(!document_of(zone_text(scenarios().front().parameters)).level.has_value(), "a zone given no level has none");
        return;
    }
}

//------------------------------------------------------------------------------
TEST_SUITE("zones the reader accepts")
{
    TEST_CASE("each scenario's zone is accepted and reads back as the document built in memory")
    {
        for (const Scenario& scenario : scenarios())
        {
            SUBCASE(format("{} is a zone the reader accepts", scenario.name))
            {
                string text = zone_text(scenario.parameters, scenario.zone_name);
                stratum_format::Document document;
                REQUIRE_NOTHROW(document = document_of(text));

                ostringstream os;
                stratum_format::write(os, document);
                CHECK_MESSAGE(os.str() == text, format("{} reads back as the document built in memory", scenario.name));
            }
        }

        return;
    }
}

//------------------------------------------------------------------------------
TEST_SUITE("determinism")
{
    TEST_CASE("one seed produces one zone")
    {
        catagen::Parameters parameters = sound_parameters();
        parameters.seed = 7u;

        CHECK(zone_text(parameters) == zone_text(parameters));
        return;
    }

    TEST_CASE("another seed produces another zone")
    {
        catagen::Parameters parameters = sound_parameters();
        parameters.seed = 7u;
        catagen::Parameters other = parameters;
        other.seed = 8u;

        CHECK(zone_text(other) != zone_text(parameters));
        return;
    }

    TEST_CASE("a Zone's name is written as the layout's name")
    {
        stratum_format::Document named = document_of(zone_text(sound_parameters(), "Catacombs Level 1"));

        CHECK(named.components.is_object());
        CHECK(named.components.value("name", string()) == "Catacombs Level 1");
        return;
    }

    TEST_CASE("an empty name is not written")
    {
        stratum_format::Document unnamed = document_of(zone_text(sound_parameters()));

        CHECK((!unnamed.components.is_object() || !unnamed.components.contains("name")));
        return;
    }

    TEST_CASE("the name does not affect the grid")
    {
        stratum_format::Document named = document_of(zone_text(sound_parameters(), "Catacombs Level 1"));
        stratum_format::Document unnamed = document_of(zone_text(sound_parameters()));

        CHECK(named.cells.rows == unnamed.cells.rows);
        return;
    }

    // Drawn from entropy
    TEST_CASE("a random seed is not the same one every time")
    {
        CHECK(catagen::random_seed() != catagen::random_seed());
        return;
    }
}

//------------------------------------------------------------------------------
TEST_SUITE("parameter validation")
{
    TEST_CASE("a sound set of parameters passes")
    {
        CHECK_NOTHROW(catagen::validate(sound_parameters()));
        return;
    }

    TEST_CASE("no seed is rejected rather than quietly fixed")
    {
        catagen::Parameters seedless = sound_parameters();
        seedless.seed.reset();

        CHECK_THROWS_AS(catagen::validate(seedless), invalid_argument);
        return;
    }

    TEST_CASE("a scale range that runs backwards is rejected")
    {
        catagen::Parameters inverted = sound_parameters();
        inverted.scale = {8, 4};

        CHECK_THROWS_AS(catagen::validate(inverted), invalid_argument);
        return;
    }

    TEST_CASE("an empty extent is rejected")
    {
        catagen::Parameters empty = sound_parameters();
        empty.extent = {0, 0};

        CHECK_THROWS_AS(catagen::validate(empty), invalid_argument);
        return;
    }
}

//------------------------------------------------------------------------------
// Walls fall between the cells, so the grid is two characters across per cell plus one
TEST_SUITE("grid dimensions")
{
    TEST_CASE("the grid has a row per lattice position")
    {
        CHECK(grid_rows().size() == size_t(2 * 6 + 1));
        return;
    }

    TEST_CASE("the grid has a column per lattice position")
    {
        vector<string> rows = grid_rows();

        REQUIRE(!rows.empty());
        CHECK(rows.front().size() == size_t(2 * 8 + 1));
        return;
    }

    TEST_CASE("no row of the grid is trimmed")
    {
        vector<string> rows = grid_rows();

        CHECK(all_of(rows.begin(), rows.end(), [&rows](const string& row) { return row.size() == rows.front().size(); }));
        return;
    }
}

//------------------------------------------------------------------------------
TEST_SUITE("doors")
{
    TEST_CASE("each scenario writes a door entity at each door")
    {
        for (const Scenario& scenario : scenarios())
        {
            SUBCASE(format("{} writes a door entity at each door", scenario.name))
            {
                string detail;
                bool written = doors_have_entities(zone_text(scenario.parameters), detail);
                CHECK_MESSAGE(written, detail);
            }
        }

        return;
    }

    TEST_CASE("each scenario writes a door entity at each door over a sweep of seeds")
    {
        for (const Scenario& scenario : scenarios())
        {
            SUBCASE(format("{} writes a door entity at each door over {} seeds", scenario.name, seed_count))
            {
                for (catagen::Parameters::seed_type seed = 1; seed <= seed_count; ++seed)
                {
                    CAPTURE(seed);
                    catagen::Parameters parameters = scenario.parameters;
                    parameters.seed = seed;

                    string zone;
                    CHECK_NOTHROW(zone = zone_text(parameters));
                    if (zone.empty())
                    {
                        continue;
                    }

                    string detail;
                    bool written = doors_have_entities(zone, detail);
                    CHECK_MESSAGE(written, detail);
                }
            }
        }

        return;
    }
}

//------------------------------------------------------------------------------
TEST_SUITE("staircases")
{
    TEST_CASE("each scenario places and faces its staircases")
    {
        for (const Scenario& scenario : scenarios())
        {
            SUBCASE(format("{} places and faces its staircases", scenario.name))
            {
                string detail;
                bool placed = staircases_placed_and_faced(zone_text(scenario.parameters), scenario.parameters, detail);
                CHECK_MESSAGE(placed, detail);
            }
        }

        return;
    }

    TEST_CASE("each scenario places and faces ranges of staircases over a sweep of seeds")
    {
        for (const Scenario& scenario : scenarios())
        {
            SUBCASE(format("{} places and faces ranges of staircases over {} seeds", scenario.name, seed_count))
            {
                for (catagen::Parameters::seed_type seed = 1; seed <= seed_count; ++seed)
                {
                    CAPTURE(seed);
                    catagen::Parameters parameters = scenario.parameters;
                    parameters.seed = seed;
                    parameters.staircase_down_count = {0, 2};
                    parameters.staircase_up_count = {1, 3};

                    string zone;
                    CHECK_NOTHROW(zone = zone_text(parameters));
                    if (zone.empty())
                    {
                        continue;
                    }

                    string detail;
                    bool placed = staircases_placed_and_faced(zone, parameters, detail);
                    CHECK_MESSAGE(placed, detail);
                }
            }
        }

        return;
    }

    TEST_CASE("a staircase count whose maximum is below its minimum is rejected")
    {
        catagen::Parameters reversed;
        reversed.seed = 1u;
        reversed.staircase_up_count = {2, 1};

        CHECK_THROWS_AS(catagen::validate(reversed), invalid_argument);
        return;
    }

    TEST_CASE("staircase counts that may both be zero, leaving no entry, are rejected")
    {
        catagen::Parameters entryless;
        entryless.seed = 1u;
        entryless.staircase_down_count = {0, 1};
        entryless.staircase_up_count = {0, 1};

        CHECK_THROWS_AS(catagen::validate(entryless), invalid_argument);
        return;
    }

    TEST_CASE("a staircase down alone is rejected, a dungeon zone needing one leading up")
    {
        catagen::Parameters down_only;
        down_only.seed = 1u;
        down_only.staircase_down_count = {1, 1};
        down_only.staircase_up_count = {0, 0};

        CHECK_THROWS_AS(catagen::validate(down_only), invalid_argument);
        return;
    }
}
