//------------------------------------------------------------------------------
// stratum_sim
// positions.cpp
//------------------------------------------------------------------------------

#include "positions.hpp"
#include "World.hpp"
#include <algorithm>
#include <string>

//------------------------------------------------------------------------------
quetzal::stratum_sim::RegionPosition quetzal::stratum_sim::region_position(const RegionSpec& region_spec)
{
    return {region_spec.value_or<coord_type>(position_x_key, 0), region_spec.value_or<coord_type>(position_y_key, 0)};
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::ZonePosition quetzal::stratum_sim::zone_position(const ZoneSpec& zone_spec)
{
    return {zone_spec.value_or<coord_type>(position_x_key, 0), zone_spec.value_or<coord_type>(position_y_key, 0),
        zone_spec.value_or<coord_type>(position_z_key, 0)};
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::set_region_position(RegionSpec& region_spec, const RegionPosition& position)
{
    region_spec.insert(std::string(position_x_key), std::to_string(position.x));
    region_spec.insert(std::string(position_y_key), std::to_string(position.y));
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::set_zone_position(ZoneSpec& zone_spec, const ZonePosition& position)
{
    zone_spec.insert(std::string(position_x_key), std::to_string(position.x));
    zone_spec.insert(std::string(position_y_key), std::to_string(position.y));
    zone_spec.insert(std::string(position_z_key), std::to_string(position.z));
    return;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::RegionPosition quetzal::stratum_sim::new_region_position(const World& world)
{
    bool any_region = false;
    coord_type x_max = 0;

    world.entities().each([&](flecs::entity, const Region&, const RegionPosition& position)
    {
        x_max = any_region ? std::max(x_max, position.x) : position.x;
        any_region = true;
    });

    return {any_region ? x_max + 1 : 0, 0};
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::ZonePosition quetzal::stratum_sim::new_zone_position(const World&, flecs::entity, level_type level)
{
    return {0, 0, level};
}
