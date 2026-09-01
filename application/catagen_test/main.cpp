//------------------------------------------------------------------------------
// catagen_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/catagen/generate.hpp"
#include "quetzal/catagen/write_level.hpp"
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
        os << "Usage: catagen_test [level] [--extent nx,ny] [--scale min-max] [--spacing min-max]"
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
                auto [nx, ny] = to_pair(argv[++i], ',');
                parameters.extent = {static_cast<catagen::coord_type>(nx), static_cast<catagen::coord_type>(ny)};
            }
            else if (arg == "--scale" && i + 1 < argc)
            {
                auto [scale_min, scale_max] = to_pair(argv[++i], '-');
                parameters.scale_min = static_cast<catagen::coord_type>(scale_min);
                parameters.scale_max = static_cast<catagen::coord_type>(scale_max);
            }
            else if (arg == "--spacing" && i + 1 < argc)
            {
                auto [spacing_min, spacing_max] = to_pair(argv[++i], '-');
                parameters.spacing_min = static_cast<catagen::coord_type>(spacing_min);
                parameters.spacing_max = static_cast<catagen::coord_type>(spacing_max);
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
                parameters.seed = static_cast<catagen::Parameters::seed_type>(to_integer(argv[++i]));
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

        // The same header the file carries, so that a run names the seed that reproduces it
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
