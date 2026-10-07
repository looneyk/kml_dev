//------------------------------------------------------------------------------
// dungen
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/dungen/generate.hpp"
#include "quetzal/dungen/write_zone.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/log/Log.hpp"
#include "quetzal/log/StreamLogSink.hpp"
#include "quetzal/math/Random.hpp"
#include "quetzal/stratum_format/write_zone.hpp"
#include <algorithm>
#include <charconv>
#include <exception>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

using namespace std;
using namespace quetzal;

namespace
{

    //--------------------------------------------------------------------------
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
    dungen::vector_type to_vector(const string& s, const string& separator)
    {
        dungen::vector_type v;
        auto result = math::from_chars(s.data(), s.data() + s.size(), v, separator);

        if (result.ec != errc {} || result.ptr != s.data() + s.size())
        {
            throw invalid_argument(format("\"{}\" is not {} values separated by '{}'",
                s, dungen::vector_type::dimension, separator));
        }

        return v;
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
    log::LogLevel to_level(const string& s)
    {
        optional<log::LogLevel> level = log::to_log_level(s);
        if (!level)
        {
            throw invalid_argument(format("\"{}\" is not a log level", s));
        }

        return *level;
    }

    //--------------------------------------------------------------------------
    void usage(ostream& os)
    {
        os << "Usage: dungen [zone] [--algorithm maze1|grid1|cave1] [--extent nx,ny] [--name text]"
            << " [--stairs-down min-max] [--stairs-up min-max] [--room-density d] [--door-probability p] [--level n] [--seed n]"
            << " [--log-level trace|debug|info|warning|error|critical|off]" << endl;
        os << "  maze1: [--room-dimension-base n] [--room-dimension-variation n] [--room-merge]"
            << " [--corridor-straightness s] [--probability-dead-end-removal p]"
            << " [--probability-loop-removal p]" << endl;
        os << "  grid1: [--divisions nx,ny] [--room-dimension-min n] [--room-margin n] [--loop l]" << endl;
        os << "  cave1: [--fill f] [--smoothing-passes n] [--birth-threshold n]"
            << " [--survival-threshold n]" << endl;
        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    string filepath = "dungen.zone";
    string name;
    dungen::Parameters parameters;
    log::logger().add_sink(log::StreamLogSink(clog));

    try
    {
        for (int i = 1; i < argc; ++i)
        {
            string arg = argv[i];

            if (arg == "--algorithm" && i + 1 < argc)
            {
                parameters.algorithm = dungen::to_algorithm(argv[++i]);
            }
            else if (arg == "--extent" && i + 1 < argc)
            {
                parameters.extent = to_vector(argv[++i], ",");
            }
            else if (arg == "--name" && i + 1 < argc)
            {
                name = argv[++i];
            }
            else if (arg == "--stairs-down" && i + 1 < argc)
            {
                parameters.staircase_down_count = to_count_interval(argv[++i]);
            }
            else if (arg == "--stairs-up" && i + 1 < argc)
            {
                parameters.staircase_up_count = to_count_interval(argv[++i]);
            }
            else if (arg == "--room-density" && i + 1 < argc)
            {
                double room_density = to_real(argv[++i]);
                parameters.maze1.room_density = room_density;
                parameters.grid1.room_density = room_density;
            }
            else if (arg == "--door-probability" && i + 1 < argc)
            {
                double door_probability = to_real(argv[++i]);
                parameters.maze1.door_probability = door_probability;
                parameters.grid1.door_probability = door_probability;
            }
            else if (arg == "--room-dimension-base" && i + 1 < argc)
            {
                parameters.maze1.room_dimension_base = to_integer(argv[++i]);
            }
            else if (arg == "--room-dimension-variation" && i + 1 < argc)
            {
                parameters.maze1.room_dimension_variation = to_integer(argv[++i]);
            }
            else if (arg == "--room-merge")
            {
                parameters.maze1.room_merge = true;
            }
            else if (arg == "--corridor-straightness" && i + 1 < argc)
            {
                parameters.maze1.corridor_straightness = to_real(argv[++i]);
            }
            else if (arg == "--probability-dead-end-removal" && i + 1 < argc)
            {
                parameters.maze1.probability_dead_end_removal = to_real(argv[++i]);
            }
            else if (arg == "--probability-loop-removal" && i + 1 < argc)
            {
                parameters.maze1.probability_loop_removal = to_real(argv[++i]);
            }
            else if (arg == "--divisions" && i + 1 < argc)
            {
                parameters.grid1.divisions = to_vector(argv[++i], ",");
            }
            else if (arg == "--room-dimension-min" && i + 1 < argc)
            {
                parameters.grid1.room_dimension_min = to_integer(argv[++i]);
            }
            else if (arg == "--room-margin" && i + 1 < argc)
            {
                parameters.grid1.room_margin = to_integer(argv[++i]);
            }
            else if (arg == "--loop" && i + 1 < argc)
            {
                parameters.grid1.loop = to_real(argv[++i]);
            }
            else if (arg == "--fill" && i + 1 < argc)
            {
                parameters.cave1.fill = to_real(argv[++i]);
            }
            else if (arg == "--smoothing-passes" && i + 1 < argc)
            {
                parameters.cave1.smoothing_passes = to_count(argv[++i]);
            }
            else if (arg == "--birth-threshold" && i + 1 < argc)
            {
                parameters.cave1.birth_threshold = to_count(argv[++i]);
            }
            else if (arg == "--survival-threshold" && i + 1 < argc)
            {
                parameters.cave1.survival_threshold = to_count(argv[++i]);
            }
            else if (arg == "--level" && i + 1 < argc)
            {
                parameters.level = to_integer(argv[++i]);
            }
            else if (arg == "--seed" && i + 1 < argc)
            {
                parameters.seed = math::to_seed(argv[++i]);
            }
            else if (arg == "--log-level" && i + 1 < argc)
            {
                log::logger().set_level(to_level(argv[++i]));
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

        // A random seed when none is given, written to the header so the zone can be reproduced
        if (!parameters.seed)
        {
            parameters.seed = dungen::random_seed();
        }

        dungen::Zone zone = dungen::generate(0, name, parameters);

        ofstream os(filepath, ios_base::binary);
        if (!os)
        {
            throw runtime_error(format("unable to open zone file {}", filepath));
        }

        stratum_format::Document document = dungen::make_document(zone, parameters);
        document.is_unpopulated = true;

        stratum_format::write(os, document);
        stratum_format::write(cout, document);
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
