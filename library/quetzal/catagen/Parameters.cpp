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

    if (parameters.scale_min < 1)
    {
        throw invalid_argument(format("scale minimum {} is below one cell", parameters.scale_min));
    }

    if (parameters.scale_max < parameters.scale_min)
    {
        throw invalid_argument(format("scale maximum {} is below the minimum {}", parameters.scale_max, parameters.scale_min));
    }

    if (parameters.spacing_min < 0)
    {
        throw invalid_argument(format("spacing minimum {} is negative", parameters.spacing_min));
    }

    if (parameters.spacing_max < parameters.spacing_min)
    {
        throw invalid_argument(format("spacing maximum {} is below the minimum {}", parameters.spacing_max, parameters.spacing_min));
    }

    if (parameters.density <= 0.0 || parameters.density > 1.0)
    {
        throw invalid_argument(format("density {} is outside (0, 1]", parameters.density));
    }

    if (parameters.loop < 0.0 || parameters.loop > 1.0)
    {
        throw invalid_argument(format("loop {} is outside [0, 1]", parameters.loop));
    }

    if (!parameters.seed)
    {
        throw invalid_argument("no seed was assigned");
    }

    return;
}
