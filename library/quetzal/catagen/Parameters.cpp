//------------------------------------------------------------------------------
// catagen
// Parameters.cpp
//------------------------------------------------------------------------------

#include "Parameters.hpp"
#include <format>
#include <stdexcept>

using namespace std;

//------------------------------------------------------------------------------
quetzal::catagen::Parameters::seed_type quetzal::catagen::random_seed()
{
    random_device rd;
    return static_cast<Parameters::seed_type>(rd());
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
