//------------------------------------------------------------------------------
// stratum_sim
// Worldspace.cpp
//------------------------------------------------------------------------------

#include "Worldspace.hpp"
#include "Generation.hpp"
#include "specs.hpp"
#include "quetzal/log/Log.hpp"
#include <format>
#include <stdexcept>
#include <utility>

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::WorldspaceExtent::contains(const ZonePosition& position) const
{
    auto axis_contains = [](const std::optional<math::Interval<coord_type>>& interval, coord_type coordinate)
    {
        return !interval.has_value() || interval->contains(coordinate);
    };

    return axis_contains(x, position.x) && axis_contains(y, position.y) && axis_contains(z, position.z);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Worldspace quetzal::stratum_sim::Worldspace::create(PersistentId worldspace_id, std::string_view name, const RegionSpec& region_spec, Region root_region)
{
    const KeyValueStore worldspace_spec = select_prefixed(region_spec.keys, worldspace_prefix);

    Worldspace worldspace {worldspace_id, std::string(name), WorldspaceExtent {worldspace_spec.value<math::Interval<coord_type>>(extent_x_key),
        worldspace_spec.value<math::Interval<coord_type>>(extent_y_key), worldspace_spec.value<math::Interval<coord_type>>(extent_z_key)},
        {}, {}, RegionTree(std::move(root_region))};

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
