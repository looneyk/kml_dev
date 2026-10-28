//------------------------------------------------------------------------------
// catagen
// Parameters.cpp
//------------------------------------------------------------------------------

#include "Parameters.hpp"
#include <format>
#include <optional>
#include <stdexcept>
#include <string>

using namespace std;

namespace
{

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

} // namespace

//------------------------------------------------------------------------------
quetzal::catagen::Parameters::seed_type quetzal::catagen::random_seed()
{
    return math::random_seed();
}

//------------------------------------------------------------------------------
void quetzal::catagen::set_parameters(Parameters& parameters, const KeyValueStore& tokens)
{
    for (const auto& [key, text] : tokens)
    {
        if (key == "extent")
        {
            parameters.extent = token_value<vector_type>(key, text);
        }
        else if (key == "scale")
        {
            parameters.scale = token_value<math::Interval<coord_type>>(key, text);
        }
        else if (key == "spacing")
        {
            parameters.spacing = token_value<math::Interval<coord_type>>(key, text);
        }
        else if (key == "density")
        {
            parameters.density = token_value<double>(key, text);
        }
        else if (key == "loop")
        {
            parameters.loop = token_value<double>(key, text);
        }
        else if (key == "stairs_down")
        {
            parameters.staircase_down_count = token_value<math::Interval<size_t>>(key, text);
        }
        else if (key == "stairs_up")
        {
            parameters.staircase_up_count = token_value<math::Interval<size_t>>(key, text);
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
            throw invalid_argument(format("{} is not a catagen parameter", key));
        }
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::catagen::validate(const Parameters& parameters)
{
    if (parameters.extent.i() <= 0 || parameters.extent.j() <= 0)
    {
        throw invalid_argument(format("extent {},{} is empty", parameters.extent.i(), parameters.extent.j()));
    }

    if (parameters.scale.lower() < 1)
    {
        throw invalid_argument(format("scale minimum {} is below one cell", parameters.scale.lower()));
    }

    if (parameters.scale.upper() < parameters.scale.lower())
    {
        throw invalid_argument(format("scale maximum {} is below the minimum {}", parameters.scale.upper(), parameters.scale.lower()));
    }

    if (parameters.spacing.lower() < 0)
    {
        throw invalid_argument(format("spacing minimum {} is negative", parameters.spacing.lower()));
    }

    if (parameters.spacing.upper() < parameters.spacing.lower())
    {
        throw invalid_argument(format("spacing maximum {} is below the minimum {}", parameters.spacing.upper(), parameters.spacing.lower()));
    }

    if (parameters.density <= 0.0 || parameters.density > 1.0)
    {
        throw invalid_argument(format("density {} is outside (0, 1]", parameters.density));
    }

    if (parameters.loop < 0.0 || parameters.loop > 1.0)
    {
        throw invalid_argument(format("loop {} is outside [0, 1]", parameters.loop));
    }

    if (parameters.staircase_down_count.upper() < parameters.staircase_down_count.lower())
    {
        throw invalid_argument(format("staircase down count maximum {} is below the minimum {}",
            parameters.staircase_down_count.upper(), parameters.staircase_down_count.lower()));
    }

    if (parameters.staircase_up_count.upper() < parameters.staircase_up_count.lower())
    {
        throw invalid_argument(format("staircase up count maximum {} is below the minimum {}",
            parameters.staircase_up_count.upper(), parameters.staircase_up_count.lower()));
    }

    // A dungeon zone is entered at, and left by, a staircase leading up
    if (parameters.staircase_up_count.lower() == 0)
    {
        throw invalid_argument("a staircase up count minimum of zero may leave the zone no staircase leading up");
    }

    if (!parameters.seed)
    {
        throw invalid_argument("no seed was assigned");
    }

    return;
}
