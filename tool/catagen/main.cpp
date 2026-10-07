//------------------------------------------------------------------------------
// catagen
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/catagen/generate.hpp"
#include "quetzal/catagen/write_zone.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/log/Log.hpp"
#include "quetzal/log/StreamLogSink.hpp"
#include "quetzal/math/Interval.hpp"
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
    catagen::vector_type to_vector(const string& s, const string& separator)
    {
        catagen::vector_type v;
        auto result = math::from_chars(s.data(), s.data() + s.size(), v, separator);

        if (result.ec != errc {} || result.ptr != s.data() + s.size())
        {
            throw invalid_argument(format("\"{}\" is not {} values separated by '{}'",
                s, catagen::vector_type::dimension, separator));
        }

        return v;
    }

    //--------------------------------------------------------------------------
    // A closed range, written low to high with the separator between
    math::Interval<catagen::coord_type> to_interval(const string& s, char separator)
    {
        size_t n = s.find(separator);

        if (n == string::npos)
        {
            throw invalid_argument(format("\"{}\" is not two values separated by '{}'", s, separator));
        }

        return {to_integer(s.substr(0, n)), to_integer(s.substr(n + 1))};
    }

    //--------------------------------------------------------------------------
    // A closed range of counts, written low to high with a hyphen between
    math::Interval<size_t> to_count_interval(const string& s)
    {
        math::Interval<catagen::coord_type> interval = to_interval(s, '-');

        if (interval.lower() < 0 || interval.upper() < 0)
        {
            throw invalid_argument(format("\"{}\" is not two counts", s));
        }

        return {static_cast<size_t>(interval.lower()), static_cast<size_t>(interval.upper())};
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
        os << "Usage: catagen [zone] [--extent nx,ny] [--name text] [--scale min-max] [--spacing min-max]"
            << " [--density d] [--loop l] [--stairs-down min-max] [--stairs-up min-max] [--level n] [--seed n] [--quiet]"
            << " [--log-level trace|debug|info|warning|error|critical|off]" << endl;
        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    string filepath = "catagen.zone";
    string name;
    catagen::Parameters parameters;
    bool quiet = false;
    log::logger().add_sink(log::StreamLogSink(clog));

    try
    {
        for (int i = 1; i < argc; ++i)
        {
            string arg = argv[i];

            if (arg == "--extent" && i + 1 < argc)
            {
                parameters.extent = to_vector(argv[++i], ",");
            }
            else if (arg == "--name" && i + 1 < argc)
            {
                name = argv[++i];
            }
            else if (arg == "--scale" && i + 1 < argc)
            {
                parameters.scale = to_interval(argv[++i], '-');
            }
            else if (arg == "--spacing" && i + 1 < argc)
            {
                parameters.spacing = to_interval(argv[++i], '-');
            }
            else if (arg == "--density" && i + 1 < argc)
            {
                parameters.density = to_real(argv[++i]);
            }
            else if (arg == "--loop" && i + 1 < argc)
            {
                parameters.loop = to_real(argv[++i]);
            }
            else if (arg == "--stairs-down" && i + 1 < argc)
            {
                parameters.staircase_down_count = to_count_interval(argv[++i]);
            }
            else if (arg == "--stairs-up" && i + 1 < argc)
            {
                parameters.staircase_up_count = to_count_interval(argv[++i]);
            }
            else if (arg == "--level" && i + 1 < argc)
            {
                parameters.level = to_integer(argv[++i]);
            }
            else if (arg == "--seed" && i + 1 < argc)
            {
                parameters.seed = math::to_seed(argv[++i]);
            }
            else if (arg == "--quiet")
            {
                quiet = true;
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
            parameters.seed = catagen::random_seed();
        }

        catagen::Zone zone = catagen::generate(name, parameters);

        ofstream os(filepath, ios_base::binary);
        if (!os)
        {
            throw runtime_error(format("unable to open zone file {}", filepath));
        }

        stratum_format::Document document = catagen::make_document(zone, parameters);
        document.is_unpopulated = true;

        stratum_format::write(os, document);

        if (!quiet)
        {
            stratum_format::write(cout, document);
        }
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
