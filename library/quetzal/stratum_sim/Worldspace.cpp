//------------------------------------------------------------------------------
// stratum_sim
// Worldspace.cpp
//------------------------------------------------------------------------------

#include "Worldspace.hpp"
#include "Generation.hpp"
#include "specs.hpp"
#include "quetzal/log/Log.hpp"
#include "quetzal/math/Interval.hpp"
#include <format>
#include <stdexcept>
#include <utility>

//------------------------------------------------------------------------------
quetzal::stratum_sim::Worldspace quetzal::stratum_sim::Worldspace::create(PersistentId worldspace_id, std::string_view name, const RegionSpec& region_spec, Region root_region)
{
    const KeyValueStore worldspace_spec = select_prefixed(region_spec.keys, worldspace_prefix);

    auto extent_interval = [&](std::string_view key)
    {
        const std::optional<math::Interval<coord_type>> interval = worldspace_spec.value<math::Interval<coord_type>>(key);

        if (!interval.has_value() || interval->lower() > interval->upper())
        {
            const std::string message = !interval.has_value()
                ? std::format("the worldspace {} has no {}{}", name, worldspace_prefix, key)
                : std::format("the worldspace {} has an extent whose lower bound is above its upper bound", name);
            log::logger().error("stratum_sim", "{}", message);
            throw std::runtime_error(message);
        }

        return interval.value();
    };

    const math::Interval<coord_type> x = extent_interval(extent_x_key);
    const math::Interval<coord_type> y = extent_interval(extent_y_key);
    const math::Interval<coord_type> z = extent_interval(extent_z_key);
    const extent_type extent(extent_type::point_type(x.lower(), y.lower(), z.lower()), extent_type::point_type(x.upper() + 1, y.upper() + 1,
        z.upper() + 1));

    Worldspace worldspace {worldspace_id, std::string(name), extent, {}, {}, RegionTree(std::move(root_region))};

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

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Worldspace::contains(const ZonePosition& position) const
{
    return extent.contains(extent_type::point_type(position.x, position.y, position.z));
}
