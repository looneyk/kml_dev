//------------------------------------------------------------------------------
// dungen_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/file_util.hpp"
#include "quetzal/dungen/generate.hpp"
#include "quetzal/dungen/write_level.hpp"
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
    string level_text(const dungen::Parameters& parameters)
    {
        dungen::Level level = dungen::generate(0, "level", parameters);

        ostringstream os;
        dungen::write_level(os, level, parameters);

        return os.str();
    }

    //--------------------------------------------------------------------------
    // A level as level_text writes it, or empty with the generator's error where it could not be generated
    string level_text_or_error(const dungen::Parameters& parameters, string& error)
    {
        try
        {
            return level_text(parameters);
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
    // The whole string or nothing, so that trailing rubbish in a header value is an error
    // rather than something quietly ignored
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
    // The parameters hold their counts as size_t, which a negative is not
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
    // The level as the reader decodes it. Throws std::runtime_error where the reader rejects it
    stratum_format::Document document_of(const string& level)
    {
        istringstream is(level);
        return stratum_format::read(is, "level");
    }

    //--------------------------------------------------------------------------
    // The generator a level names, read back into the parameters it specifies, which is the whole of what a level says
    // about how to produce it again. Only the chosen algorithm's group is written, so only that one is read
    // Throws std::invalid_argument where the level specifies something this does not read, so a parameter added to the
    // writer and not to this is a failure rather than a silent pass
    dungen::Parameters parameters_of(const stratum_format::values_type& generator)
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

        dungen::Parameters parameters;

        parameters.algorithm = dungen::to_algorithm(take("algorithm"));
        parameters.staircase_down_count = to_count_interval(take("stairs_down"));
        parameters.staircase_up_count = to_count_interval(take("stairs_up"));
        parameters.seed = math::to_seed(take("seed"));

        string size = take("size");
        size_t n = size.find(',');
        parameters.size = {to_integer(size.substr(0, n)), to_integer(size.substr(n + 1))};

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
            throw invalid_argument(format("the level specifies {}, which this does not read", values.begin()->first));
        }

        return parameters;
    }

    //--------------------------------------------------------------------------
    // The runs the reference levels are written from. Every algorithm at a fixed seed, so
    // that each is reproducible from what is checked in
    struct Scenario
    {
        string name;
        dungen::Parameters parameters;
    };

    vector<Scenario> scenarios()
    {
        vector<Scenario> v;

        dungen::Parameters maze1;
        maze1.algorithm = dungen::Algorithm::Maze1;
        maze1.size = {31, 21};
        maze1.seed = 1u;
        v.push_back({"maze1", maze1});

        dungen::Parameters grid1;
        grid1.algorithm = dungen::Algorithm::Grid1;
        grid1.size = {33, 25};
        grid1.seed = 42u;
        v.push_back({"grid1", grid1});

        dungen::Parameters cave1;
        cave1.algorithm = dungen::Algorithm::Cave1;
        cave1.size = {40, 30};
        cave1.seed = 12345u;
        v.push_back({"cave1", cave1});

        // No staircase leading down, which the default counts do not cover
        dungen::Parameters up_only;
        up_only.algorithm = dungen::Algorithm::Maze1;
        up_only.size = {21, 15};
        up_only.staircase_down_count = {0, 0};
        up_only.seed = 7u;
        v.push_back({"up_only", up_only});

        // A seed past the halfway mark of its range, which a signed read would lose
        dungen::Parameters wide;
        wide.algorithm = dungen::Algorithm::Grid1;
        wide.size = {25, 19};
        wide.seed = 4294967295u;
        v.push_back({"wide_seed", wide});

        // Doorways left open, which barriers close off from the rooms they lead into
        dungen::Parameters grid1_open = grid1;
        grid1_open.grid1.door_probability = 0.5;
        v.push_back({"grid1_open_doorways", grid1_open});

        dungen::Parameters maze1_open = maze1;
        maze1_open.maze1.door_probability = 0.5;
        v.push_back({"maze1_open_doorways", maze1_open});

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
                dungen::Parameters parameters = parameters_of(document_of(reference).generator);
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
    void test_determinism()
    {
        report_group("determinism");

        dungen::Parameters parameters;
        parameters.size = {21, 15};
        parameters.seed = 7u;

        report_case("one seed gives one level", level_text(parameters) == level_text(parameters));

        dungen::Parameters other = parameters;
        other.seed = 8u;
        report_case("and another seed gives another", level_text(other) != level_text(parameters));

        // The id and name are held rather than read, so neither reaches the grid
        dungen::Level a = dungen::generate(0, "one", parameters);
        dungen::Level b = dungen::generate(7, "another", parameters);

        ostringstream os_a;
        ostringstream os_b;
        dungen::write_level(os_a, a, parameters);
        dungen::write_level(os_b, b, parameters);
        report_case("the id and name a level holds do not reach it", os_a.str() == os_b.str());

        report_case("a random seed is not the same one every time",
            dungen::random_seed() != dungen::random_seed());

        return;
    }

    //--------------------------------------------------------------------------
    void test_validate()
    {
        report_group("parameter validation");

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
        parameters.size = {21, 15};
        parameters.seed = 1u;
        report_case("a sound set of parameters passes", !rejected(parameters));

        dungen::Parameters seedless = parameters;
        seedless.seed.reset();
        report_case("no seed is rejected rather than quietly fixed", rejected(seedless));

        dungen::Parameters empty = parameters;
        empty.size = {0, 0};
        report_case("an empty size is rejected", rejected(empty));

        // Only the chosen algorithm's group is read, so another group holding nonsense is not
        // a reason to reject a run that never looks at it
        dungen::Parameters unread = parameters;
        unread.algorithm = dungen::Algorithm::Maze1;
        unread.cave1.fill = -1.0;
        report_case("a group the algorithm does not read is not judged", !rejected(unread));

        dungen::Parameters pinched = parameters;
        pinched.algorithm = dungen::Algorithm::Grid1;
        pinched.grid1.room_dimension_min = 1;
        report_case("a room of a single cell is allowed", !rejected(pinched));

        return;
    }

    //--------------------------------------------------------------------------
    // Every algorithm produces a level the reader accepts, of the size asked for
    void test_grid_shape()
    {
        report_group("grid shape");

        for (const Scenario& scenario : scenarios())
        {
            vector<string> rows;

            try
            {
                rows = document_of(level_text(scenario.parameters)).cells.rows;
                report_case(format("{} is a level the reader accepts", scenario.name), true);
            }
            catch (const runtime_error& e)
            {
                report_case(format("{} is a level the reader accepts", scenario.name), false, e.what());
                continue;
            }

            auto [nx, ny] = scenario.parameters.size.rep();

            report_case_equal(format("{} has a row per cell", scenario.name), rows.size(), static_cast<size_t>(ny));
            report_case(format("{} has a column per cell", scenario.name),
                !rows.empty() && rows.front().size() == static_cast<size_t>(nx));
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A doorway is itself a cell of wall, so two side by side are one doorway two cells wide
    // rather than a break in the line
    bool doorways_in_wall_lines(const string& level, string& detail)
    {
        vector<string> rows;

        try
        {
            rows = document_of(level).cells.rows;
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
    // Swept over seeds as well as the checked in levels, a placement fault turning on where a
    // room happens to land
    void test_door_placement()
    {
        report_group("doorway placement");

        constexpr dungen::Parameters::seed_type seed_count = 64;

        for (const Scenario& scenario : scenarios())
        {
            string detail;
            report_case(format("{} places every doorway in a wall line", scenario.name),
                doorways_in_wall_lines(level_text(scenario.parameters), detail), detail);
        }

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

            for (dungen::Parameters::seed_type seed = 1; seed <= seed_count; ++seed)
            {
                dungen::Parameters parameters = scenario.parameters;
                parameters.seed = seed;

                string seed_detail;
                string level = level_text_or_error(parameters, seed_detail);
                if (level.empty() || !doorways_in_wall_lines(level, seed_detail) || !doors_have_entities(level, seed_detail))
                {
                    if (failures == 0)
                    {
                        detail = format("seed {}, {}", seed, seed_detail);
                    }

                    ++failures;
                }
            }

            report_case(format("{} holds over {} seeds", scenario.name, seed_count), failures == 0,
                format("{} seeds failed, first at {}", failures, detail));
        }

        // A density below one leaves sectors holding a single cell instead of a room, and the
        // smallest dimension allows rooms one cell across. Both reach corridor shapes the
        // defaults never do, and the second is what placement has to draw again to avoid
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

            for (dungen::Parameters::seed_type seed = 1; seed <= seed_count; ++seed)
            {
                dungen::Parameters parameters;
                parameters.algorithm = dungen::Algorithm::Grid1;
                parameters.size = {33, 25};
                parameters.seed = seed;
                parameters.grid1.room_density = variant.room_density;
                parameters.grid1.room_dimension_min = variant.room_dimension_min;

                string seed_detail;
                string level = level_text_or_error(parameters, seed_detail);
                if (level.empty() || !doorways_in_wall_lines(level, seed_detail) || !doors_have_entities(level, seed_detail))
                {
                    if (failures == 0)
                    {
                        detail = format("seed {}, {}", seed, seed_detail);
                    }

                    ++failures;
                }
            }

            report_case(format("grid1 holds with {}", variant.name), failures == 0,
                format("{} seeds failed, first at {}", failures, detail));
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Whether the rooms the reader finds cover exactly the cells generation laid out as each room, which fails wherever a
    // barrier is missing and a room's fill leaks out through an open doorway
    bool rooms_read_back(const dungen::Parameters& parameters, string& detail)
    {
        optional<dungen::Level> generated;

        try
        {
            generated.emplace(dungen::generate(0, "level", parameters));
        }
        catch (const exception& e)
        {
            detail = e.what();
            return false;
        }

        const dungen::Level& level = *generated;

        vector<set<stratum_format::discrete_point_type>> cells_expected(level.rooms().size());
        auto [nx, ny] = level.size().rep();

        for (dungen::coord_type y = 0; y < ny; ++y)
        {
            for (dungen::coord_type x = 0; x < nx; ++x)
            {
                // A stair cell is a block, and in no room
                const dungen::Cell& cell = level.cell({x, y});
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
        dungen::write_level(os, level, parameters);

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
    void test_barriers()
    {
        report_group("barriers");

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
                parameters.size = {33, 25};
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

            report_case(format("{} reads back its rooms over {} seeds", variant.name, seed_count), failures == 0,
                format("{} seeds failed, first at {}", failures, detail));
        }

        dungen::Parameters doorless;
        doorless.algorithm = dungen::Algorithm::Grid1;
        doorless.size = {33, 25};
        doorless.seed = 42u;
        doorless.grid1.door_probability = 0.0;

        const vector<string> rows = document_of(level_text(doorless)).cells.rows;
        report_case("a door probability of zero leaves no door",
            none_of(rows.begin(), rows.end(), [](const string& row) { return row.find('+') != string::npos; }));

        return;
    }

    //--------------------------------------------------------------------------
    // Read from the written level, and a stair cell's space, which a block in no room leaves the file without, from the level
    // generated again: as many staircases each way as the counts allow, no floor cut off by a stair block, no staircase
    // blocking a doorway while a cell beside it would not, every staircase with an entity whose facing is a side with floor beyond, or with a door where no side
    // has floor beyond, and the entry listing every staircase cell
    bool staircases_placed_and_faced(const string& level, const dungen::Parameters& parameters, string& detail)
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

        dungen::Level generated = dungen::generate(0, "level", parameters);

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

                            string target = glyph(cell_position) == '>' ? "level_down" : "level_up";
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
                bool open_side = any_of(directions.begin(), directions.end(), [&](const discrete_vector_type& direction) { return glyph(cell_position + direction) == '.'; });
                bool step = find(directions.begin(), directions.end(), facing) != directions.end();
                char faced = glyph(cell_position + facing);

                if (!step || !(faced == '.' || (faced == '+' && !open_side)))
                {
                    detail = format("staircase at {}, {} faces {}, {}", i, j, facing.x(), facing.y());
                    return false;
                }
            }
        }

        // Running out of candidates places fewer, and those leading up are placed first, so a count below its minimum is
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

        constexpr dungen::Parameters::seed_type seed_count = 64;

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

            for (dungen::Parameters::seed_type seed = 1; seed <= seed_count; ++seed)
            {
                dungen::Parameters parameters = scenario.parameters;
                parameters.seed = seed;
                parameters.staircase_down_count = {0, 2};
                parameters.staircase_up_count = {1, 3};

                string seed_detail;
                string level = level_text_or_error(parameters, seed_detail);
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

        dungen::Parameters reversed;
        reversed.size = {21, 15};
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

        report_case("a staircase count whose maximum is below its minimum is rejected", rejected);

        dungen::Parameters entryless;
        entryless.size = {21, 15};
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

        report_case("staircase counts that may both be zero, leaving no entry, are rejected", rejected);

        dungen::Parameters down_only;
        down_only.size = {21, 15};
        down_only.seed = 1u;
        down_only.staircase_down_count = {1, 1};
        down_only.staircase_up_count = {0, 0};

        bool accepted = true;
        try
        {
            dungen::validate(down_only);
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
        test_determinism();
        test_validate();
        test_grid_shape();
        test_door_placement();
        test_barriers();
        test_staircases();
    }
    catch (const exception& e)
    {
        cout << format(escape_rendition_pattern, 196) << "Error: " << escape_reset_string << e.what() << endl;
        return 1;
    }

    return report_results();
}
