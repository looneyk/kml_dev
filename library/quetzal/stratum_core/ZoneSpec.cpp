//------------------------------------------------------------------------------
// stratum_core
// ZoneSpec.cpp
//------------------------------------------------------------------------------

#include "ZoneSpec.hpp"
#include "spec_keys.hpp"
#include <string>

//------------------------------------------------------------------------------
quetzal::stratum_core::discrete_point3_type quetzal::stratum_core::ZoneSpec::position() const
{
    return {store.value_or<coord_type>(position_x_key, 0), store.value_or<coord_type>(position_y_key, 0),
        store.value_or<coord_type>(position_z_key, 0)};
}

//------------------------------------------------------------------------------
void quetzal::stratum_core::ZoneSpec::set_position(const discrete_point3_type& position)
{
    store.insert(std::string(position_x_key), std::to_string(position.x()));
    store.insert(std::string(position_y_key), std::to_string(position.y()));
    store.insert(std::string(position_z_key), std::to_string(position.z()));
    return;
}
