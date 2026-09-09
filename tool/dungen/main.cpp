//------------------------------------------------------------------------------
// dungen
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/dungen/generate.hpp"
#include "quetzal/dungen/write_level.hpp"
#include "quetzal/math/Random.hpp"
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
    // Two numbers with a separator between them
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
        os << "Usage: dungen [level] [--algorithm maze1|grid1|cave1] [--size nx,ny]"
            << " [--stair-placements none|paired|multiple] [--room-density d] [--seed n]" << endl;
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
    string filepath = "Level_0.level";
    dungen::Parameters parameters;

    try
    {
        for (int i = 1; i < argc; ++i)
        {
            string arg = argv[i];

            if (arg == "--algorithm" && i + 1 < argc)
            {
                parameters.algorithm = dungen::to_algorithm(argv[++i]);
            }
            else if (arg == "--size" && i + 1 < argc)
            {
                auto [nx, ny] = to_pair(argv[++i], ',');
                parameters.size = {static_cast<dungen::coord_type>(nx), static_cast<dungen::coord_type>(ny)};
            }
            else if (arg == "--stair-placements" && i + 1 < argc)
            {
                parameters.stair_placements = dungen::to_stair_placements(argv[++i]);
            }
            else if (arg == "--room-density" && i + 1 < argc)
            {
                // The one name two algorithms share, so it sets both and the chosen one reads it
                double room_density = to_real(argv[++i]);
                parameters.maze1.room_density = room_density;
                parameters.grid1.room_density = room_density;
            }
            else if (arg == "--room-dimension-base" && i + 1 < argc)
            {
                parameters.maze1.room_dimension_base = static_cast<dungen::coord_type>(to_integer(argv[++i]));
            }
            else if (arg == "--room-dimension-variation" && i + 1 < argc)
            {
                parameters.maze1.room_dimension_variation = static_cast<dungen::coord_type>(to_integer(argv[++i]));
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
                auto [nx, ny] = to_pair(argv[++i], ',');
                parameters.grid1.divisions = {static_cast<dungen::coord_type>(nx), static_cast<dungen::coord_type>(ny)};
            }
            else if (arg == "--room-dimension-min" && i + 1 < argc)
            {
                parameters.grid1.room_dimension_min = static_cast<dungen::coord_type>(to_integer(argv[++i]));
            }
            else if (arg == "--room-margin" && i + 1 < argc)
            {
                parameters.grid1.room_margin = static_cast<dungen::coord_type>(to_integer(argv[++i]));
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
            else if (arg == "--seed" && i + 1 < argc)
            {
                parameters.seed = math::to_seed(argv[++i]);
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
            parameters.seed = dungen::random_seed();
        }

        dungen::Level level = dungen::generate(0, "Level 0", parameters);

        ofstream os(filepath, ios_base::binary);
        if (!os)
        {
            throw runtime_error(format("unable to open level file {}", filepath));
        }

        dungen::write_level(os, level, parameters);

        // The same header the file holds, so that a run names the seed that reproduces it
        dungen::write_level(cout, level, parameters);
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
