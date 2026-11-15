//------------------------------------------------------------------------------
// stratum_sim
// ZoneSpec.cpp
//------------------------------------------------------------------------------

#include "ZoneSpec.hpp"
#include "Generation.hpp"
#include "Zone.hpp"
#include <string>

//------------------------------------------------------------------------------
quetzal::stratum_sim::Position quetzal::stratum_sim::ZoneSpec::position() const
{
    return {store.value_or<coord_type>(position_x_key, 0), store.value_or<coord_type>(position_y_key, 0), store.value_or<coord_type>(position_z_key, 0)};
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::ZoneSpec::set_position(const Position& position)
{
    store.insert(std::string(position_x_key), std::to_string(position.x));
    store.insert(std::string(position_y_key), std::to_string(position.y));
    store.insert(std::string(position_z_key), std::to_string(position.z));
    return;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::ZoneSpec quetzal::stratum_sim::resolve_zone_spec(const ZoneSpec& zone_spec, PatternResolver& resolver)
{
    return ZoneSpec {resolver(zone_spec.store)};
}
