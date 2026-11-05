//------------------------------------------------------------------------------
// stratum_sim
// RegionSpec.cpp
//------------------------------------------------------------------------------

#include "RegionSpec.hpp"
#include "Generation.hpp"
#include <string>

//------------------------------------------------------------------------------
quetzal::stratum_sim::RegionPosition quetzal::stratum_sim::RegionSpec::position() const
{
    return {keys.value_or<coord_type>(position_x_key, 0), keys.value_or<coord_type>(position_y_key, 0)};
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::RegionSpec::set_position(const RegionPosition& position)
{
    keys.insert(std::string(position_x_key), std::to_string(position.x));
    keys.insert(std::string(position_y_key), std::to_string(position.y));
    return;
}
