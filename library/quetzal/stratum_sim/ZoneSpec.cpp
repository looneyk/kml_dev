//------------------------------------------------------------------------------
// stratum_sim
// ZoneSpec.cpp
//------------------------------------------------------------------------------

#include "ZoneSpec.hpp"
#include "Generation.hpp"
#include "Zone.hpp"
#include <string>

//------------------------------------------------------------------------------
quetzal::stratum_sim::ZonePosition quetzal::stratum_sim::ZoneSpec::position() const
{
    return {keys.value_or<coord_type>(position_x_key, 0), keys.value_or<coord_type>(position_y_key, 0), keys.value_or<coord_type>(position_z_key, 0)};
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::ZoneSpec::set_position(const ZonePosition& position)
{
    keys.insert(std::string(position_x_key), std::to_string(position.x));
    keys.insert(std::string(position_y_key), std::to_string(position.y));
    keys.insert(std::string(position_z_key), std::to_string(position.z));
    return;
}
