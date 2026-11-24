//------------------------------------------------------------------------------
// stratum_core
// Worldspace.cpp
//------------------------------------------------------------------------------

#include "Worldspace.hpp"
#include "quetzal/log/Log.hpp"
#include "quetzal/math/Interval.hpp"
#include <format>
#include <stdexcept>
#include <utility>

//------------------------------------------------------------------------------
quetzal::stratum_core::Worldspace quetzal::stratum_core::Worldspace::create(PersistentId worldspace_id, std::string_view name, seed_type seed,
    const RegionSpec& region_spec, Region root_region)
{
    auto fail = [&name](std::string_view problem)
    {
        const std::string message = std::format("the worldspace {} has {}", name, problem);
        log::logger().error(log_category, "{}", message);
        throw std::runtime_error(message);
    };

    if (!region_spec.extent.has_value())
    {
        fail("no extent");
    }

    const RegionExtent& region_extent = *region_spec.extent;
    if (region_extent.arrangement != RegionExtent::Arrangement::Boxes || region_extent.boxes.size() != 1)
    {
        fail("an extent that is not a single box");
    }

    const RegionExtent::Box& box = region_extent.boxes.front();
    if (!box.x_extent.has_value() || !box.y_extent.has_value() || !box.z_extent.has_value())
    {
        fail("an extent without a range on every axis");
    }

    const math::Interval<coord_type>& x = *box.x_extent;
    const math::Interval<coord_type>& y = *box.y_extent;
    const math::Interval<coord_type>& z = *box.z_extent;
    const extent_type extent(extent_type::point_type(x.lower(), y.lower(), z.lower()), extent_type::point_type(x.upper() + 1, y.upper() + 1, z.upper() + 1));

    return Worldspace{worldspace_id, std::string(name), seed, extent, region_spec.zone_size, RegionTree(std::move(root_region))};
}

//------------------------------------------------------------------------------
bool quetzal::stratum_core::Worldspace::contains(const discrete_point3_type& position) const
{
    return extent.contains(position);
}
