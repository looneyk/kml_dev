//------------------------------------------------------------------------------
// catagen
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/catagen/generate.hpp"
#include "quetzal/catagen/write_level.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/math/Interval.hpp"
#include "quetzal/math/Random.hpp"
#include <algorithm>
#include <charconv>
#include <exception>
#include <format>
#include <fstream>
#include <iostream>
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
    void usage(ostream& os)
    {
        os << "Usage: catagen [level] [--extent nx,ny] [--scale min-max] [--spacing min-max]"
            << " [--density d] [--loop l] [--seed n] [--quiet]" << endl;
        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    string filepath = "catagen.level";
    catagen::Parameters parameters;
    bool quiet = false;

    try
    {
        for (int i = 1; i < argc; ++i)
        {
            string arg = argv[i];

            if (arg == "--extent" && i + 1 < argc)
            {
                parameters.extent = to_vector(argv[++i], ",");
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
            else if (arg == "--seed" && i + 1 < argc)
            {
                parameters.seed = math::to_seed(argv[++i]);
            }
            else if (arg == "--quiet")
            {
                quiet = true;
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
            parameters.seed = catagen::random_seed();
        }

        catagen::Level level = catagen::generate(parameters);

        ofstream os(filepath, ios_base::binary);
        if (!os)
        {
            throw runtime_error(format("unable to open level file {}", filepath));
        }

        catagen::write_level(os, level, parameters);

        // The same header the file holds, so that a run names the seed that reproduces it
        if (!quiet)
        {
            catagen::write_level(cout, level, parameters);
        }
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
