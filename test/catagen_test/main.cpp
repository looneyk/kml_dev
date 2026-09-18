//------------------------------------------------------------------------------
// catagen_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/catagen/generate.hpp"
#include "quetzal/catagen/write_level.hpp"
#include "quetzal/common/file_util.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/math/Interval.hpp"
#include "quetzal/math/Random.hpp"
#include "quetzal/stratum_format/grid.hpp"
#include "quetzal/stratum_format/read.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
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

    static constexpr auto escape_reset_string = "\x1b[0m";
    static constexpr auto escape_rendition_pattern = "\x1b[38;5;{}m";

    // Key order plays no part in its comparison, which is what lets two level files compare as equivalent
    using map_type = nlohmann::json;

    size_t g_case_count = 0;
    size_t g_failure_count = 0;

    //--------------------------------------------------------------------------
    void report_group(const string& name)
    {
        cout << name << endl;
        return;
    }

    //--------------------------------------------------------------------------
    // Every case reports its own line so that a failure is identifiable by name
    void report_case(const string& name, bool b, const string& detail = {})
    {
        ++g_case_count;

        if (b)
        {
            cout << format(escape_rendition_pattern, 34) << "    ok   " << escape_reset_string << name << endl;
            return;
        }

        ++g_failure_count;
        cout << format(escape_rendition_pattern, 196) << "    FAIL " << escape_reset_string << name;
        if (!detail.empty())
        {
            cout << ": " << detail;
        }
        cout << endl;
        return;
    }

    //--------------------------------------------------------------------------
    void report_case_equal(const string& name, size_t value, size_t value_expected)
    {
        report_case(name, value == value_expected, format("expected {}, got {}", value_expected, value));
        return;
    }

    //--------------------------------------------------------------------------
    int report_results()
    {
        if (g_failure_count == 0)
        {
            cout << format(escape_rendition_pattern, 34) << format("OK: {} cases", g_case_count) << escape_reset_string << endl;
            return 0;
        }

        cout << format(escape_rendition_pattern, 196) << format("Error: {} of {} cases failed", g_failure_count, g_case_count) << escape_reset_string << endl;
        return 1;
    }

    //--------------------------------------------------------------------------
    // A level as the tool writes one, header and grid, so that what is compared is what a
    // level file holds rather than the Level in memory
    string level_text(const catagen::Parameters& parameters)
    {
        catagen::Level level = catagen::generate(parameters);

        ostringstream os;
        catagen::write_level(os, level, parameters);

        return os.str();
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
    // The whole string or nothing, so that trailing rubbish in a header value is an error
    // rather than something quietly ignored
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
    // The level as the reader decodes it. Throws std::runtime_error where the reader rejects it
    stratum_format::Document document_of(const string& level)
    {
        istringstream is(level);
        return stratum_format::read(is, "level");
    }

    //--------------------------------------------------------------------------
    // The generator a level names, read back into the parameters it specifies, which is the whole of what a level says
    // about how to produce it again
    // Throws std::invalid_argument where the level specifies something this does not read, so a parameter added to the
    // writer and not to this is a failure rather than a silent pass
    catagen::Parameters parameters_of(const stratum_format::values_type& generator)
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

        // Each value read is removed, so what remains at the end is what this does not read
        auto take = [&values](const string& key)
        {
            auto i = values.find(key);
            if (i == values.end())
            {
                throw invalid_argument(format("the level specifies no {}", key));
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

        if (!values.empty())
        {
            throw invalid_argument(format("the level specifies {}, which this does not read", values.begin()->first));
        }

        return parameters;
    }

    //--------------------------------------------------------------------------
    // The runs the reference levels are written from. Fixed parameters and fixed seeds, so
    // that every one of them is reproducible from what is checked in
    struct Scenario
    {
        string name;
        catagen::Parameters parameters;
    };

    vector<Scenario> scenarios()
    {
        vector<Scenario> v;

        // The shipped level's own parameters, so the asset and the suite gate the same run
        catagen::Parameters shipped;
        shipped.extent = {20, 10};
        shipped.scale = {2, 5};
        shipped.spacing = {0, 4};
        shipped.density = 0.5;
        shipped.loop = 0.1;
        shipped.seed = 2614047767u;
        v.push_back({"shipped", shipped});

        // Rooms sharing walls, which is the case the block format cannot express
        catagen::Parameters shared;
        shared.extent = {16, 12};
        shared.scale = {3, 7};
        shared.density = 1.0;
        shared.loop = 0.0;
        shared.seed = 1u;
        v.push_back({"shared_walls", shared});

        // Rooms stood apart, so corridors cross the rock between them
        catagen::Parameters spaced;
        spaced.extent = {24, 16};
        spaced.scale = {3, 6};
        spaced.spacing = {1, 3};
        spaced.density = 0.7;
        spaced.loop = 0.3;
        spaced.seed = 42u;
        v.push_back({"spaced", spaced});

        // A seed past the halfway mark of its range, which a signed read would lose
        catagen::Parameters wide;
        wide.extent = {12, 12};
        wide.scale = {2, 5};
        wide.seed = 4294967295u;
        v.push_back({"wide_seed", wide});

        return v;
    }

    //--------------------------------------------------------------------------
    // Levels written now against levels checked in, which are to be equivalent. A generator change that alters what any of these
    // produces reports here rather than months later
    void test_reference_levels()
    {
        report_group("reference levels");

        filesystem::create_directories("out");

        for (const Scenario& scenario : scenarios())
        {
            string level = level_text(scenario.parameters);
            write_file(format("out/{}.level", scenario.name), level);

            string reference = read_file(format("reference/{}.level", scenario.name));
            if (reference.empty())
            {
                report_case(format("{} is equivalent to its reference", scenario.name), false, "no reference is checked in");
                continue;
            }

            try
            {
                map_type map = map_type::parse(level);
                map_type map_expected = map_type::parse(reference);
                report_case(format("{} is equivalent to its reference", scenario.name), map == map_expected, map_type::diff(map_expected, map).dump());
            }
            catch (const map_type::parse_error& e)
            {
                report_case(format("{} is equivalent to its reference", scenario.name), false, e.what());
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // The claim the format rests on: a level names everything its run needs, so it can be produced again from the file alone
    void test_parameters_reproduce()
    {
        report_group("parameters reproduce their level");

        for (const Scenario& scenario : scenarios())
        {
            string reference = read_file(format("reference/{}.level", scenario.name));
            report_case(format("{} is checked in", scenario.name), !reference.empty());

            if (reference.empty())
            {
                continue;
            }

            try
            {
                catagen::Parameters parameters = parameters_of(document_of(reference).generator);
                report_case(format("{} regenerates from its own parameters", scenario.name),
                    map_type::parse(level_text(parameters)) == map_type::parse(reference));
            }
            catch (const exception& e)
            {
                report_case(format("{} regenerates from its own parameters", scenario.name), false, e.what());
            }
        }

        stratum_format::values_type generator = document_of(level_text(scenarios().front().parameters)).generator;
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

        report_case("a parameter this does not read is rejected", rejected);

        return;
    }

    //--------------------------------------------------------------------------
    void test_levels_read()
    {
        report_group("levels the reader accepts");

        for (const Scenario& scenario : scenarios())
        {
            try
            {
                document_of(level_text(scenario.parameters));
                report_case(format("{} is a level the reader accepts", scenario.name), true);
            }
            catch (const runtime_error& e)
            {
                report_case(format("{} is a level the reader accepts", scenario.name), false, e.what());
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Read from the written level alone: one door entity attached at the walls of each door glyph, and none elsewhere
    bool doors_have_entities(const string& level, string& detail)
    {
        stratum_format::Document document;

        try
        {
            document = document_of(level);
        }
        catch (const runtime_error& e)
        {
            detail = e.what();
            return false;
        }

        vector<vector<stratum_format::WallAddress>> doors = stratum_format::door_walls(document.cells);
        vector<bool> matched(doors.size(), false);

        for (const stratum_format::Attachment& attachment : document.attachments)
        {
            const auto* walls = get_if<vector<stratum_format::WallAddress>>(&attachment.attached_to);
            bool door = any_of(attachment.entities.begin(), attachment.entities.end(),
                [](const auto& entity) { return entity.is_object() && entity.value("type", "") == "door"; });

            if (walls == nullptr || !door)
            {
                continue;
            }

            auto i = find(doors.begin(), doors.end(), *walls);
            if (i == doors.end() || matched[static_cast<size_t>(i - doors.begin())])
            {
                const stratum_format::WallAddress& wall = walls->front();
                detail = format("a door entity at wall {}, {}, {}, {} that is {}", wall.cell_position.i(), wall.cell_position.j(), wall.normal.x(),
                    wall.normal.y(), i == doors.end() ? "not a door's" : "a door's second");
                return false;
            }

            matched[static_cast<size_t>(i - doors.begin())] = true;
        }

        size_t missing_count = static_cast<size_t>(count(matched.begin(), matched.end(), false));
        if (missing_count > 0)
        {
            detail = format("{} of {} doors have no door entity", missing_count, doors.size());
            return false;
        }

        return true;
    }

    //--------------------------------------------------------------------------
    void test_doors()
    {
        report_group("doors");

        constexpr catagen::Parameters::seed_type seed_count = 64;

        for (const Scenario& scenario : scenarios())
        {
            string detail;
            report_case(format("{} writes a door entity at each door", scenario.name), doors_have_entities(level_text(scenario.parameters), detail),
                detail);
        }

        for (const Scenario& scenario : scenarios())
        {
            string detail;
            size_t failures = 0;

            for (catagen::Parameters::seed_type seed = 1; seed <= seed_count; ++seed)
            {
                catagen::Parameters parameters = scenario.parameters;
                parameters.seed = seed;

                string seed_detail;
                string level;

                try
                {
                    level = level_text(parameters);
                }
                catch (const exception& e)
                {
                    seed_detail = e.what();
                }

                if (level.empty() || !doors_have_entities(level, seed_detail))
                {
                    if (failures == 0)
                    {
                        detail = format("seed {}, {}", seed, seed_detail);
                    }

                    ++failures;
                }
            }

            report_case(format("{} writes a door entity at each door over {} seeds", scenario.name, seed_count), failures == 0,
                format("{} seeds failed, first at {}", failures, detail));
        }

        return;
    }

    //--------------------------------------------------------------------------
    void test_determinism()
    {
        report_group("determinism");

        catagen::Parameters parameters;
        parameters.extent = {16, 12};
        parameters.seed = 7u;

        report_case("one seed gives one level", level_text(parameters) == level_text(parameters));

        catagen::Parameters other = parameters;
        other.seed = 8u;
        report_case("and another seed gives another", level_text(other) != level_text(parameters));

        // Drawn from entropy rather than fixed, so a caller who wants a different level each
        // run gets one
        report_case("a random seed is not the same one every time",
            catagen::random_seed() != catagen::random_seed());

        return;
    }

    //--------------------------------------------------------------------------
    void test_validate()
    {
        report_group("parameter validation");

        auto rejected = [](const catagen::Parameters& parameters)
        {
            try
            {
                catagen::validate(parameters);
            }
            catch (const invalid_argument&)
            {
                return true;
            }

            return false;
        };

        catagen::Parameters parameters;
        parameters.extent = {16, 12};
        parameters.seed = 1u;
        report_case("a sound set of parameters passes", !rejected(parameters));

        catagen::Parameters seedless = parameters;
        seedless.seed.reset();
        report_case("no seed is rejected rather than quietly fixed", rejected(seedless));

        catagen::Parameters inverted = parameters;
        inverted.scale = {8, 4};
        report_case("a scale range that runs backwards is rejected", rejected(inverted));

        catagen::Parameters empty = parameters;
        empty.extent = {0, 0};
        report_case("an empty extent is rejected", rejected(empty));

        return;
    }

    //--------------------------------------------------------------------------
    // The grid alone, which is what a caller with no parameters to write emits
    void test_grid_shape()
    {
        report_group("grid shape");

        catagen::Parameters parameters;
        parameters.extent = {8, 6};
        parameters.seed = 3u;

        catagen::Level level = catagen::generate(parameters);

        ostringstream os;
        catagen::write_grid(os, level);

        vector<string> rows;
        string row;
        istringstream is(os.str());
        while (getline(is, row))
        {
            rows.push_back(row);
        }

        // Walls fall between the cells, so the grid is two characters across per cell plus one
        report_case_equal("a row per lattice position", rows.size(), 2 * 6 + 1);
        report_case("and a column per lattice position",
            !rows.empty() && rows.front().size() == 2 * 8 + 1);

        bool ragged = false;
        for (const string& r : rows)
        {
            ragged = ragged || r.size() != rows.front().size();
        }

        report_case("with no row trimmed", !ragged);

        return;
    }

    //--------------------------------------------------------------------------
    // Read from the written level, and which sides of a stair cell were open, which its written walls hide, from the level
    // generated again: as many staircases each way as the counts allow, no floor cut off by a stair block, every stair cell
    // walled toward floor, no staircase blocking a doorway while a cell beside it would not, every staircase with an entity whose facing is an open side with
    // floor beyond, or with a door where there is no such side, and the entry listing every staircase cell
    bool staircases_placed_and_faced(const string& level, const catagen::Parameters& parameters, string& detail)
    {
        using stratum_format::coord_type;
        using stratum_format::discrete_point_type;
        using stratum_format::discrete_vector_type;

        stratum_format::Document document;

        try
        {
            document = document_of(level);
        }
        catch (const runtime_error& e)
        {
            detail = e.what();
            return false;
        }

        const stratum_format::Cells& cells = document.cells;
        const auto& directions = math::cardinal_directions<coord_type, 2>;

        // In the written grid, a cell at its odd position and the edge on its side one step from there
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

        catagen::Level generated = catagen::generate(parameters);

        // Open in the level generated, before the stair cell's walls were written, and with floor beyond
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

                vector<discrete_vector_type> facings;

                for (const stratum_format::Attachment& attachment : document.attachments)
                {
                    const auto* flat_cell = get_if<stratum_format::FlatCell>(&attachment.attached_to);
                    if (flat_cell == nullptr || flat_cell->role != stratum_format::FlatRole::Floor || flat_cell->cell_position != cell_position)
                    {
                        continue;
                    }

                    for (const auto& entity : attachment.entities)
                    {
                        // Whatever the entity's type, it is the one with a relocator
                        if (entity.contains("components") && entity["components"].contains("relocator"))
                        {
                            const auto& components = entity["components"];
                            if (!components.contains("facing"))
                            {
                                detail = format("staircase at {}, {} has no facing", i, j);
                                return false;
                            }

                            const auto& facing = components["facing"];
                            facings.push_back({facing[0].get<coord_type>(), facing[1].get<coord_type>()});

                            string target = cell_glyph(cell_position) == '>' ? "level_down" : "level_up";
                            if (components["relocator"].value("target", "") != target)
                            {
                                detail = format("staircase at {}, {} does not lead to {}", i, j, target);
                                return false;
                            }
                        }
                    }
                }

                if (facings.size() != 1)
                {
                    detail = format("staircase at {}, {} has {} facings", i, j, facings.size());
                    return false;
                }

                const discrete_vector_type& facing = facings.front();
                bool open_side = any_of(directions.begin(), directions.end(), [&](const discrete_vector_type& direction) { return open_to_floor(cell_position, direction); });
                bool step = find(directions.begin(), directions.end(), facing) != directions.end();

                if (!step || !(open_to_floor(cell_position, facing) || (edge_glyph(cell_position, facing) == '+' && !open_side)))
                {
                    detail = format("staircase at {}, {} faces {}, {}", i, j, facing.x(), facing.y());
                    return false;
                }
            }
        }

        // Running out of rooms places fewer, and those leading up are placed first, so a count below its minimum is
        // allowed only where nothing was left for the staircases leading down
        const math::Interval<size_t>& down = parameters.staircase_down_count;
        const math::Interval<size_t>& up = parameters.staircase_up_count;

        if (down_count > down.upper() || up_count > up.upper() || (up_count < up.lower() && down_count > 0))
        {
            detail = format("{} staircases down for {}-{} and {} up for {}-{}", down_count, down.lower(), down.upper(), up_count, up.lower(), up.upper());
            return false;
        }

        const auto* entry = document.components.contains("entry") ? &document.components["entry"] : nullptr;
        set<discrete_point_type> entry_cells;

        if (entry != nullptr && entry->is_array())
        {
            for (const auto& cell : *entry)
            {
                entry_cells.insert({cell[0].get<coord_type>(), cell[1].get<coord_type>()});
            }
        }

        if (entry == nullptr || !entry->is_array() || entry->size() != entry_cells.size() || entry_cells != staircase_cells)
        {
            detail = format("the entry {} is not every staircase cell", entry == nullptr ? "is absent and" : entry->dump());
            return false;
        }

        return true;
    }

    //--------------------------------------------------------------------------
    void test_staircases()
    {
        report_group("staircases");

        constexpr catagen::Parameters::seed_type seed_count = 64;

        for (const Scenario& scenario : scenarios())
        {
            string detail;
            report_case(format("{} places and faces its staircases", scenario.name),
                staircases_placed_and_faced(level_text(scenario.parameters), scenario.parameters, detail), detail);
        }

        for (const Scenario& scenario : scenarios())
        {
            string detail;
            size_t failures = 0;

            for (catagen::Parameters::seed_type seed = 1; seed <= seed_count; ++seed)
            {
                catagen::Parameters parameters = scenario.parameters;
                parameters.seed = seed;
                parameters.staircase_down_count = {0, 2};
                parameters.staircase_up_count = {1, 3};

                string seed_detail;
                string level;

                try
                {
                    level = level_text(parameters);
                }
                catch (const exception& e)
                {
                    seed_detail = e.what();
                }

                if (level.empty() || !staircases_placed_and_faced(level, parameters, seed_detail))
                {
                    if (failures == 0)
                    {
                        detail = format("seed {}, {}", seed, seed_detail);
                    }

                    ++failures;
                }
            }

            report_case(format("{} places and faces ranges of staircases over {} seeds", scenario.name, seed_count), failures == 0,
                format("{} seeds failed, first at {}", failures, detail));
        }

        catagen::Parameters reversed;
        reversed.seed = 1u;
        reversed.staircase_up_count = {2, 1};

        bool rejected = false;
        try
        {
            catagen::validate(reversed);
        }
        catch (const invalid_argument&)
        {
            rejected = true;
        }

        report_case("a staircase count whose maximum is below its minimum is rejected", rejected);

        catagen::Parameters entryless;
        entryless.seed = 1u;
        entryless.staircase_down_count = {0, 1};
        entryless.staircase_up_count = {0, 1};

        rejected = false;
        try
        {
            catagen::validate(entryless);
        }
        catch (const invalid_argument&)
        {
            rejected = true;
        }

        report_case("staircase counts that may both be zero, leaving no entry, are rejected", rejected);

        catagen::Parameters down_only;
        down_only.seed = 1u;
        down_only.staircase_down_count = {1, 1};
        down_only.staircase_up_count = {0, 0};

        bool accepted = true;
        try
        {
            catagen::validate(down_only);
        }
        catch (const invalid_argument&)
        {
            accepted = false;
        }

        report_case("a staircase down alone, as a tower entered from below has, is accepted as the entry", accepted);

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main()
{
    try
    {
        test_reference_levels();
        test_parameters_reproduce();
        test_levels_read();
        test_determinism();
        test_validate();
        test_grid_shape();
        test_doors();
        test_staircases();
    }
    catch (const exception& e)
    {
        cout << format(escape_rendition_pattern, 196) << "Error: " << escape_reset_string << e.what() << endl;
        return 1;
    }

    return report_results();
}
