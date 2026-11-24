//------------------------------------------------------------------------------
// stratum_format
// RegionExtent.cpp
//------------------------------------------------------------------------------

#include "RegionExtent.hpp"
#include "checked_values.hpp"
#include "json_util.hpp"
#include <array>
#include <format>
#include <string_view>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    using quetzal::stratum_core::RegionExtent;

    using interval_type = quetzal::math::Interval<coord_type>;

    constexpr std::string_view boxes_key = "boxes";
    constexpr std::string_view positions_key = "positions";

    constexpr std::array<std::string_view, 2> extent_keys{boxes_key, positions_key};
    constexpr std::array<std::string_view, 3> box_keys{"x", "y", "z"};

    //--------------------------------------------------------------------------
    // An integer for one position, or [lower, upper]
    interval_type read_interval(const raw_values_type& v, const std::string& path)
    {
        if (!v.is_array())
        {
            const coord_type position = coord_value(v, path);
            return interval_type(position, position);
        }

        const raw_values_type& bounds = fixed_array(v, path, 2);
        const coord_type lower = coord_value(bounds[0], element_path(path, 0));
        const coord_type upper = coord_value(bounds[1], element_path(path, 1));
        if (lower > upper)
        {
            fail(path, "an interval whose lower bound is above its upper bound");
        }

        return interval_type(lower, upper);
    }

    //--------------------------------------------------------------------------
    RegionExtent::Box read_box(const raw_values_type& v, const std::string& path)
    {
        object(v, path);
        check_keys(v, path, box_keys, "box");

        RegionExtent::Box box;
        std::array<std::optional<interval_type>*, 3> axes{&box.x_extent, &box.y_extent, &box.z_extent};
        for (size_t i = 0; i < box_keys.size(); ++i)
        {
            if (const raw_values_type* axis = member(v, box_keys[i]); axis != nullptr)
            {
                *axes[i] = read_interval(*axis, member_path(path, box_keys[i]));
            }
        }

        return box;
    }

    //--------------------------------------------------------------------------
    std::string interval_text(const interval_type& interval)
    {
        return interval.lower() == interval.upper() ? std::format("{}", interval.lower()) : std::format("[{}, {}]", interval.lower(), interval.upper());
    }

    //--------------------------------------------------------------------------
    std::string box_text(const RegionExtent::Box& box)
    {
        std::vector<std::string> members;
        const std::array<const std::optional<interval_type>*, 3> axes{&box.x_extent, &box.y_extent, &box.z_extent};
        for (size_t i = 0; i < box_keys.size(); ++i)
        {
            if (axes[i]->has_value())
            {
                members.push_back(std::format("{}: {}", string_text(box_keys[i]), interval_text(axes[i]->value())));
            }
        }

        return object_text(members);
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_core::RegionExtent quetzal::stratum_format::detail::read_region_extent(const raw_values_type& v, const std::string& path)
{
    object(v, path);
    check_keys(v, path, extent_keys, "region extent");

    const raw_values_type* boxes = member(v, boxes_key);
    const raw_values_type* positions = member(v, positions_key);
    if ((boxes == nullptr) == (positions == nullptr))
    {
        fail(path, std::format("one of {} and {} was expected", boxes_key, positions_key));
    }

    RegionExtent extent;
    if (boxes != nullptr)
    {
        const std::string boxes_path = member_path(path, boxes_key);
        const raw_values_type& box_values = array(*boxes, boxes_path);
        for (size_t i = 0; i < box_values.size(); ++i)
        {
            extent.boxes.push_back(read_box(box_values[i], element_path(boxes_path, i)));
        }
    }
    else
    {
        extent.arrangement = RegionExtent::Arrangement::Positions;

        const std::string positions_path = member_path(path, positions_key);
        const raw_values_type& position_values = array(*positions, positions_path);
        for (size_t i = 0; i < position_values.size(); ++i)
        {
            const std::string position_path = element_path(positions_path, i);
            const raw_values_type& position = fixed_array(position_values[i], position_path, 3);
            extent.positions.push_back(discrete_point3_type{coord_value(position[0], element_path(position_path, 0)),
                coord_value(position[1], element_path(position_path, 1)), coord_value(position[2], element_path(position_path, 2))});
        }
    }

    return extent;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::region_extent_text(const RegionExtent& extent)
{
    std::vector<std::string> elements;

    if (extent.arrangement == RegionExtent::Arrangement::Boxes)
    {
        for (const auto& box : extent.boxes)
        {
            elements.push_back(box_text(box));
        }

        const std::string boxes = elements.empty() ? "[]" : std::format("[ {} ]", joined(elements));
        return object_text({std::format("{}: {}", string_text(boxes_key), boxes)});
    }

    for (const auto& position : extent.positions)
    {
        elements.push_back(std::format("[{}, {}, {}]", position.x(), position.y(), position.z()));
    }

    return object_text({std::format("{}: [{}]", string_text(positions_key), joined(elements))});
}
