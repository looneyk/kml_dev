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
    return {store.value_or<coord_type>(position_x_key, 0), store.value_or<coord_type>(position_y_key, 0), store.value_or<coord_type>(position_z_key, 0)};
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::ZoneSpec::set_position(const ZonePosition& position)
{
    store.insert(std::string(position_x_key), std::to_string(position.x));
    store.insert(std::string(position_y_key), std::to_string(position.y));
    store.insert(std::string(position_z_key), std::to_string(position.z));
    return;
}
