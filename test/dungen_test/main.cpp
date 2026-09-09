//------------------------------------------------------------------------------
// dungen_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/FileValidator.hpp"
#include "quetzal/common/file_util.hpp"
#include "quetzal/dungen/generate.hpp"
#include "quetzal/dungen/write_level.hpp"
#include "quetzal/math/Random.hpp"
#include <charconv>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using namespace quetzal;

namespace
{

    static constexpr auto escape_reset_string = "\x1b[0m";
    static constexpr auto escape_rendition_pattern = "\x1b[38;5;{}m";

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
    long to_integer(const string& s)
    {
        long value = 0;
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
        long value = to_integer(s);

        if (value < 0)
        {
            throw invalid_argument(format("\"{}\" is not a count", s));
        }

        return static_cast<size_t>(value);
    }

    //--------------------------------------------------------------------------
    // The header row read back into the parameters it specifies, which is the whole of what a
    // level says about how to produce it again. Only the chosen algorithm's group is written,
    // so only that one is read
    // Throws std::invalid_argument where a row specifies something this does not read, so a
    // parameter added to the writer and not to this is a failure rather than a silent pass
    dungen::Parameters parameters_of(const string& header)
    {
        istringstream is(header);

        string generator;
        string version;
        is >> generator >> version;

        if (generator != "dungen" || version != dungen::format_version)
        {
            throw invalid_argument(format("\"{}\" is not a dungen {} header", header, dungen::format_version));
        }

        map<string, string> values;
        string token;

        while (is >> token)
        {
            size_t n = token.find('=');
            if (n == string::npos)
            {
                throw invalid_argument(format("\"{}\" is not a name=value pair", token));
            }

            values[token.substr(0, n)] = token.substr(n + 1);
        }

        dungen::Parameters parameters;

        parameters.algorithm = dungen::to_algorithm(values.at("algorithm"));
        parameters.stair_placements = dungen::to_stair_placements(values.at("stair_placements"));
        parameters.seed = math::to_seed(values.at("seed"));

        const string& size = values.at("size");
        size_t n = size.find(',');
        parameters.size = {static_cast<dungen::coord_type>(to_integer(size.substr(0, n))),
            static_cast<dungen::coord_type>(to_integer(size.substr(n + 1)))};

        if (parameters.algorithm == dungen::Algorithm::Maze1)
        {
            dungen::Maze1Parameters& maze1 = parameters.maze1;
            maze1.room_dimension_base = static_cast<dungen::coord_type>(to_integer(values.at("room_dimension_base")));
            maze1.room_dimension_variation = static_cast<dungen::coord_type>(to_integer(values.at("room_dimension_variation")));
            maze1.room_density = to_real(values.at("room_density"));
            maze1.room_merge = values.at("room_merge") == "true";
            maze1.corridor_straightness = to_real(values.at("corridor_straightness"));
            maze1.probability_dead_end_removal = to_real(values.at("probability_dead_end_removal"));
            maze1.probability_loop_removal = to_real(values.at("probability_loop_removal"));
        }
        else if (parameters.algorithm == dungen::Algorithm::Grid1)
        {
            dungen::Grid1Parameters& grid1 = parameters.grid1;

            const string& divisions = values.at("divisions");
            size_t d = divisions.find(',');
            grid1.divisions = {static_cast<dungen::coord_type>(to_integer(divisions.substr(0, d))),
                static_cast<dungen::coord_type>(to_integer(divisions.substr(d + 1)))};

            grid1.room_density = to_real(values.at("room_density"));
            grid1.room_dimension_min = static_cast<dungen::coord_type>(to_integer(values.at("room_dimension_min")));
            grid1.room_margin = static_cast<dungen::coord_type>(to_integer(values.at("room_margin")));
            grid1.loop = to_real(values.at("loop"));
        }
        else
        {
            dungen::Cave1Parameters& cave1 = parameters.cave1;
            cave1.fill = to_real(values.at("fill"));
            cave1.smoothing_passes = to_count(values.at("smoothing_passes"));
            cave1.birth_threshold = to_count(values.at("birth_threshold"));
            cave1.survival_threshold = to_count(values.at("survival_threshold"));
        }

        return parameters;
    }

    //--------------------------------------------------------------------------
    // What a level file's first line is, the rest being the grid
    string header_of(const string& level)
    {
        return level.substr(0, level.find('\n'));
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

        // No stairs at all, which is the placement the other three do not cover
        dungen::Parameters unstaired;
        unstaired.algorithm = dungen::Algorithm::Maze1;
        unstaired.size = {21, 15};
        unstaired.stair_placements = dungen::StairPlacements::None;
        unstaired.seed = 7u;
        v.push_back({"no_stairs", unstaired});

        // A seed past the halfway mark of its range, which a signed read would lose
        dungen::Parameters wide;
        wide.algorithm = dungen::Algorithm::Grid1;
        wide.size = {25, 19};
        wide.seed = 4294967295u;
        v.push_back({"wide_seed", wide});

        return v;
    }

    //--------------------------------------------------------------------------
    // Levels written now against levels checked in, gated byte for byte. A generator change
    // that alters what any of these produces reports here rather than months later
    void test_reference_levels()
    {
        report_group("reference levels");

        FileValidator validator("out", "reference", "dungen_test");

        for (const Scenario& scenario : scenarios())
        {
            write_file(format("out/{}.level", scenario.name), level_text(scenario.parameters));
        }

        validator.report_results();
        report_case("every level matches its reference", validator.result() <= FileValidator::ResultStatus::Unchanged);

        return;
    }

    //--------------------------------------------------------------------------
    // The claim the format rests on: a header names everything a run needs, so a level can be
    // produced again from the file alone
    void test_header_reproduces()
    {
        report_group("header reproduces its level");

        for (const Scenario& scenario : scenarios())
        {
            string level = read_file(format("reference/{}.level", scenario.name));
            report_case(format("{} is checked in", scenario.name), !level.empty());

            if (level.empty())
            {
                continue;
            }

            dungen::Parameters parameters = parameters_of(header_of(level));
            report_case(format("{} regenerates from its own header", scenario.name), level_text(parameters) == level);
        }

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

        return;
    }

    //--------------------------------------------------------------------------
    // Every algorithm produces a grid of the size asked for, in the symbols the format defines
    void test_grid_shape()
    {
        report_group("grid shape");

        for (const Scenario& scenario : scenarios())
        {
            string level = level_text(scenario.parameters);

            vector<string> rows;
            string row;
            istringstream is(level);
            getline(is, row); // The header

            while (getline(is, row))
            {
                rows.push_back(row);
            }

            auto [nx, ny] = scenario.parameters.size.rep();

            report_case_equal(format("{} has a row per cell", scenario.name), rows.size(), static_cast<size_t>(ny));
            report_case(format("{} has a column per cell", scenario.name),
                !rows.empty() && rows.front().size() == static_cast<size_t>(nx));

            bool known = true;
            for (const string& r : rows)
            {
                for (char c : r)
                {
                    known = known && (c == '#' || c == '0' || c == ' ' || c == '.' || c == '|' || c == '>');
                }
            }

            report_case(format("{} writes only symbols the format names", scenario.name), known);
        }

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main()
{
    try
    {
        test_reference_levels();
        test_header_reproduces();
        test_determinism();
        test_validate();
        test_grid_shape();
    }
    catch (const exception& e)
    {
        cout << format(escape_rendition_pattern, 196) << "Error: " << escape_reset_string << e.what() << endl;
        return 1;
    }

    return report_results();
}
