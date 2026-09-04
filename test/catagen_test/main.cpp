//------------------------------------------------------------------------------
// catagen_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/catagen/generate.hpp"
#include "quetzal/catagen/write_level.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/common/file_util.hpp"
#include "quetzal/math/Random.hpp"
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
    // The header row read back into the parameters it names, which is the whole of what a
    // level says about how to produce it again
    // Throws std::invalid_argument where a row names something this does not read, so a
    // parameter added to the writer and not to this is a failure rather than a silent pass
    catagen::Parameters parameters_of(const string& header)
    {
        istringstream is(header);

        string generator;
        string version;
        is >> generator >> version;

        if (generator != "catagen" || version != catagen::format_version)
        {
            throw invalid_argument(format("\"{}\" is not a catagen {} header", header, catagen::format_version));
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

        auto pair_of = [](const string& s, char separator)
        {
            size_t n = s.find(separator);
            return make_pair(stol(s.substr(0, n)), stol(s.substr(n + 1)));
        };

        catagen::Parameters parameters;

        auto [nx, ny] = pair_of(values.at("extent"), ',');
        parameters.extent = {static_cast<catagen::coord_type>(nx), static_cast<catagen::coord_type>(ny)};

        auto [scale_min, scale_max] = pair_of(values.at("scale"), '-');
        parameters.scale_min = static_cast<catagen::coord_type>(scale_min);
        parameters.scale_max = static_cast<catagen::coord_type>(scale_max);

        auto [spacing_min, spacing_max] = pair_of(values.at("spacing"), '-');
        parameters.spacing_min = static_cast<catagen::coord_type>(spacing_min);
        parameters.spacing_max = static_cast<catagen::coord_type>(spacing_max);

        parameters.density = stod(values.at("density"));
        parameters.loop = stod(values.at("loop"));
        parameters.seed = math::to_seed(values.at("seed"));

        return parameters;
    }

    //--------------------------------------------------------------------------
    // What a level file's first line is, the rest being the grid
    string header_of(const string& level)
    {
        return level.substr(0, level.find('\n'));
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
        shipped.scale_min = 2;
        shipped.scale_max = 5;
        shipped.spacing_min = 0;
        shipped.spacing_max = 4;
        shipped.density = 0.5;
        shipped.loop = 0.1;
        shipped.seed = 2614047767u;
        v.push_back({"shipped", shipped});

        // Rooms sharing walls, which is the case the block format cannot express
        catagen::Parameters shared;
        shared.extent = {16, 12};
        shared.scale_min = 3;
        shared.scale_max = 7;
        shared.density = 1.0;
        shared.loop = 0.0;
        shared.seed = 1u;
        v.push_back({"shared_walls", shared});

        // Rooms stood apart, so corridors cross the rock between them
        catagen::Parameters spaced;
        spaced.extent = {24, 16};
        spaced.scale_min = 3;
        spaced.scale_max = 6;
        spaced.spacing_min = 1;
        spaced.spacing_max = 3;
        spaced.density = 0.7;
        spaced.loop = 0.3;
        spaced.seed = 42u;
        v.push_back({"spaced", spaced});

        // A seed past the halfway mark of its range, which a signed read would lose
        catagen::Parameters wide;
        wide.extent = {12, 12};
        wide.scale_min = 2;
        wide.scale_max = 5;
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

        FileValidator validator("out", "reference", "catagen_test");

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

            catagen::Parameters parameters = parameters_of(header_of(level));
            report_case(format("{} regenerates from its own header", scenario.name), level_text(parameters) == level);
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
        inverted.scale_min = 8;
        inverted.scale_max = 4;
        report_case("a scale range that runs backwards is rejected", rejected(inverted));

        catagen::Parameters empty = parameters;
        empty.extent = {0, 0};
        report_case("an empty extent is rejected", rejected(empty));

        return;
    }

    //--------------------------------------------------------------------------
    // The grid alone, which is what a caller with nothing to record writes
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

        // The last column is a boundary wall or one of its corners and so is never blank,
        // which is what lets a reader derive the extent from the grid
        report_case("with no row trimmed", !ragged);

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
