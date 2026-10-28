//------------------------------------------------------------------------------
// dungen
// Parameters.cpp
//------------------------------------------------------------------------------

#include "Parameters.hpp"
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <cassert>

using namespace std;

namespace
{

    using quetzal::dungen::coord_type;
    using quetzal::dungen::Parameters;

    // A vector or position separates its values with ',', and a range its bounds with '-'
    const quetzal::FormatSpec token_format_spec {.separator = ",", .series_delimiter = "-"};

    //--------------------------------------------------------------------------
    template<typename T>
    T token_value(const string& key, const string& text)
    {
        optional<T> value = quetzal::try_type<T>(text, token_format_spec);
        if (!value)
        {
            throw invalid_argument(format("\"{}\" is not a valid {}", text, key));
        }

        return *value;
    }

    //--------------------------------------------------------------------------
    void validate_probability(double value, const char* name)
    {
        if (value < 0.0 || value > 1.0)
        {
            throw invalid_argument(format("{} {} is outside [0, 1]", name, value));
        }

        return;
    }

    //--------------------------------------------------------------------------
    void validate_count(const quetzal::math::Interval<size_t>& count, const char* name)
    {
        if (count.upper() < count.lower())
        {
            throw invalid_argument(format("{} maximum {} is below the minimum {}", name, count.upper(), count.lower()));
        }

        return;
    }

    //--------------------------------------------------------------------------
    void validate_maze1(const Parameters& parameters)
    {
        const auto& maze1 = parameters.maze1;

        // The maze runs along the odd cells, so a zone under three cells across has none once rounded down to odd
        auto [nx, ny] = parameters.extent;
        if (nx < 3 || ny < 3)
        {
            throw invalid_argument(format("extent {},{} is below the three cells maze1 needs in each direction", nx, ny));
        }

        if (maze1.room_dimension_base < 1)
        {
            throw invalid_argument(format("room dimension base {} is below one cell", maze1.room_dimension_base));
        }

        if (maze1.room_dimension_variation < 0)
        {
            throw invalid_argument(format("room dimension variation {} is negative", maze1.room_dimension_variation));
        }

        // The zone must be wider than the largest room to leave a position for it
        coord_type room_dimension_max = maze1.room_dimension_base + maze1.room_dimension_variation;
        if (room_dimension_max >= nx || room_dimension_max >= ny)
        {
            throw invalid_argument(format("room dimension base {} plus variation {} does not fit within extent {},{}",
                maze1.room_dimension_base, maze1.room_dimension_variation, nx, ny));
        }

        validate_probability(maze1.room_density, "room density");
        validate_probability(maze1.corridor_straightness, "corridor straightness");
        validate_probability(maze1.probability_dead_end_removal, "dead end removal probability");
        validate_probability(maze1.probability_loop_removal, "loop removal probability");
        validate_probability(maze1.door_probability, "door probability");
        return;
    }

    //--------------------------------------------------------------------------
    void validate_grid1(const Parameters& parameters)
    {
        const auto& grid1 = parameters.grid1;

        auto [divisions_x, divisions_y] = grid1.divisions;
        if (divisions_x < 1 || divisions_y < 1)
        {
            throw invalid_argument(format("divisions {},{} leave no sector", divisions_x, divisions_y));
        }

        if (grid1.room_dimension_min < 1)
        {
            throw invalid_argument(format("room dimension minimum {} is below one cell", grid1.room_dimension_min));
        }

        if (grid1.room_margin < 1)
        {
            throw invalid_argument(format("room margin {} leaves no rock for a corridor to run through", grid1.room_margin));
        }

        // A sector must fit its room plus the margin on both sides
        auto [nx, ny] = parameters.extent;
        coord_type sector_x = nx / divisions_x;
        coord_type sector_y = ny / divisions_y;
        coord_type sector_min = grid1.room_dimension_min + 2 * grid1.room_margin;

        if (sector_x < sector_min || sector_y < sector_min)
        {
            throw invalid_argument(format("extent {},{} over divisions {},{} leaves sectors of {},{}, below the {} a room and its margins need",
                nx, ny, divisions_x, divisions_y, sector_x, sector_y, sector_min));
        }

        validate_probability(grid1.room_density, "room density");
        validate_probability(grid1.loop, "loop probability");
        validate_probability(grid1.door_probability, "door probability");
        return;
    }

    //--------------------------------------------------------------------------
    void validate_cave1(const Parameters& parameters)
    {
        const auto& cave1 = parameters.cave1;

        auto [nx, ny] = parameters.extent;
        if (nx < 3 || ny < 3)
        {
            throw invalid_argument(format("extent {},{} leaves no cell off the solid border", nx, ny));
        }

        validate_probability(cave1.fill, "fill");

        if (cave1.birth_threshold > 8)
        {
            throw invalid_argument(format("birth threshold {} is above the eight neighbors a cell has", cave1.birth_threshold));
        }

        if (cave1.survival_threshold > 8)
        {
            throw invalid_argument(format("survival threshold {} is above the eight neighbors a cell has", cave1.survival_threshold));
        }

        return;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::dungen::Parameters::seed_type quetzal::dungen::random_seed()
{
    return math::random_seed();
}

//------------------------------------------------------------------------------
void quetzal::dungen::set_parameters(Parameters& parameters, const KeyValueStore& tokens)
{
    for (const auto& [key, text] : tokens)
    {
        if (key == "algorithm")
        {
            parameters.algorithm = to_algorithm(text);
        }
        else if (key == "extent")
        {
            parameters.extent = token_value<vector_type>(key, text);
        }
        else if (key == "stairs_down")
        {
            parameters.staircase_down_count = token_value<math::Interval<size_t>>(key, text);
        }
        else if (key == "stairs_up")
        {
            parameters.staircase_up_count = token_value<math::Interval<size_t>>(key, text);
        }
        else if (key == "room_density")
        {
            double room_density = token_value<double>(key, text);
            parameters.maze1.room_density = room_density;
            parameters.grid1.room_density = room_density;
        }
        else if (key == "door_probability")
        {
            double door_probability = token_value<double>(key, text);
            parameters.maze1.door_probability = door_probability;
            parameters.grid1.door_probability = door_probability;
        }
        else if (key == "room_dimension_base")
        {
            parameters.maze1.room_dimension_base = token_value<coord_type>(key, text);
        }
        else if (key == "room_dimension_variation")
        {
            parameters.maze1.room_dimension_variation = token_value<coord_type>(key, text);
        }
        else if (key == "room_merge")
        {
            parameters.maze1.room_merge = token_value<bool>(key, text);
        }
        else if (key == "corridor_straightness")
        {
            parameters.maze1.corridor_straightness = token_value<double>(key, text);
        }
        else if (key == "probability_dead_end_removal")
        {
            parameters.maze1.probability_dead_end_removal = token_value<double>(key, text);
        }
        else if (key == "probability_loop_removal")
        {
            parameters.maze1.probability_loop_removal = token_value<double>(key, text);
        }
        else if (key == "divisions")
        {
            parameters.grid1.divisions = token_value<vector_type>(key, text);
        }
        else if (key == "room_dimension_min")
        {
            parameters.grid1.room_dimension_min = token_value<coord_type>(key, text);
        }
        else if (key == "room_margin")
        {
            parameters.grid1.room_margin = token_value<coord_type>(key, text);
        }
        else if (key == "loop")
        {
            parameters.grid1.loop = token_value<double>(key, text);
        }
        else if (key == "fill")
        {
            parameters.cave1.fill = token_value<double>(key, text);
        }
        else if (key == "smoothing_passes")
        {
            parameters.cave1.smoothing_passes = token_value<size_t>(key, text);
        }
        else if (key == "birth_threshold")
        {
            parameters.cave1.birth_threshold = token_value<size_t>(key, text);
        }
        else if (key == "survival_threshold")
        {
            parameters.cave1.survival_threshold = token_value<size_t>(key, text);
        }
        else if (key == "position")
        {
            parameters.position = token_value<zone_position_type>(key, text);
        }
        else if (key == "seed")
        {
            parameters.seed = token_value<Parameters::seed_type>(key, text);
        }
        else
        {
            throw invalid_argument(format("{} is not a dungen parameter", key));
        }
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::validate(const Parameters& parameters)
{
    auto [nx, ny] = parameters.extent;
    if (nx <= 0 || ny <= 0)
    {
        throw invalid_argument(format("extent {},{} is empty", nx, ny));
    }

    if (!parameters.seed)
    {
        throw invalid_argument("no seed was assigned");
    }

    validate_count(parameters.staircase_down_count, "staircase down count");
    validate_count(parameters.staircase_up_count, "staircase up count");

    // A dungeon zone is entered at, and left by, a staircase leading up
    if (parameters.staircase_up_count.lower() == 0)
    {
        throw invalid_argument("a staircase up count minimum of zero may leave the zone no staircase leading up");
    }

    switch (parameters.algorithm)
    {
        case Algorithm::Maze1:
            validate_maze1(parameters);
            break;

        case Algorithm::Grid1:
            validate_grid1(parameters);
            break;

        case Algorithm::Cave1:
            validate_cave1(parameters);
            break;

        default:
            assert(false);
    }

    return;
}
