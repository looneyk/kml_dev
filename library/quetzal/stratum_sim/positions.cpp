//------------------------------------------------------------------------------
// stratum_sim
// positions.cpp
//------------------------------------------------------------------------------

#include "positions.hpp"
#include "specs.hpp"
#include "World.hpp"
#include "quetzal/log/Log.hpp"
#include <algorithm>
#include <format>
#include <optional>
#include <stdexcept>
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

    world.entities().each([&](flecs::entity, const Region& region)
    {
        const coord_type x = region_position(region.region_spec).x;
        x_max = any_region ? std::max(x_max, x) : x;
        any_region = true;
    });

    return {any_region ? x_max + 1 : 0, 0};
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::ZonePosition quetzal::stratum_sim::new_zone_position(const World&, flecs::entity, level_type level)
{
    return {0, 0, level};
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::contains(const ZoneBox& box, const ZonePosition& position)
{
    auto axis_contains = [](const std::optional<math::Interval<coord_type>>& interval, coord_type coordinate)
    {
        return !interval.has_value() || interval->contains(coordinate);
    };

    return axis_contains(box.x, position.x) && axis_contains(box.y, position.y) && axis_contains(box.z, position.z);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Worldspace quetzal::stratum_sim::make_worldspace(std::string_view name, const RegionSpec& region_spec)
{
    const KeyValueStore worldspace_spec = select_prefixed(region_spec, worldspace_prefix);

    Worldspace worldspace;
    worldspace.name = std::string(name);
    worldspace.extent = ZoneBox {worldspace_spec.value<math::Interval<coord_type>>(extent_x_key),
        worldspace_spec.value<math::Interval<coord_type>>(extent_y_key), worldspace_spec.value<math::Interval<coord_type>>(extent_z_key)};

    for (const auto& interval : {worldspace.extent.x, worldspace.extent.y, worldspace.extent.z})
    {
        if (interval.has_value() && interval->lower() > interval->upper())
        {
            const std::string message = std::format("the worldspace {} has an extent whose lower bound is above its upper bound", name);
            log::logger().error("stratum_sim", "{}", message);
            throw std::runtime_error(message);
        }
    }

    std::optional<coord_type> zone_size_x = worldspace_spec.value<coord_type>(zone_size_x_key);
    std::optional<coord_type> zone_size_y = worldspace_spec.value<coord_type>(zone_size_y_key);

    if (zone_size_x.has_value() != zone_size_y.has_value())
    {
        const std::string message = std::format("the worldspace {} has a zone size with one coordinate and not the other", name);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    if (zone_size_x.has_value())
    {
        worldspace.zone_size = discrete_vector_type(*zone_size_x, *zone_size_y);
    }

    return worldspace;
}
