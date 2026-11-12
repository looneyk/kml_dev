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

    using range_type = quetzal::math::Interval<coord_type>;

    constexpr std::string_view arrangement_key = "arrangement";
    constexpr std::string_view boxes_key = "boxes";
    constexpr std::string_view positions_key = "positions";
    constexpr std::string_view ranges_arrangement = "ranges";
    constexpr std::string_view positions_arrangement = "positions";

    constexpr std::array<std::string_view, 3> extent_keys {arrangement_key, boxes_key, positions_key};
    constexpr std::array<std::string_view, 3> box_keys {"x", "y", "z"};

    //--------------------------------------------------------------------------
    // An integer for one position, or [lower, upper]
    range_type read_range(const raw_values_type& v, const std::string& path)
    {
        if (!v.is_array())
        {
            const coord_type position = coord_value(v, path);
            return range_type(position, position);
        }

        const raw_values_type& bounds = fixed_array(v, path, 2);
        const coord_type lower = coord_value(bounds[0], element_path(path, 0));
        const coord_type upper = coord_value(bounds[1], element_path(path, 1));
        if (lower > upper)
        {
            fail(path, "a range whose lower bound is above its upper bound");
        }

        return range_type(lower, upper);
    }

    //--------------------------------------------------------------------------
    RegionExtent::Box read_box(const raw_values_type& v, const std::string& path)
    {
        object(v, path);
        check_keys(v, path, box_keys, "box");

        RegionExtent::Box box;
        std::array<std::optional<range_type>*, 3> axes {&box.x_extent, &box.y_extent, &box.z_extent};
        for (size_t i = 0; i < box_keys.size(); ++i)
        {
            if (const raw_values_type* axis = member(v, box_keys[i]); axis != nullptr)
            {
                *axes[i] = read_range(*axis, member_path(path, box_keys[i]));
            }
        }

        return box;
    }

    //--------------------------------------------------------------------------
    void reject_member(const raw_values_type& v, const std::string& path, std::string_view key, std::string_view arrangement)
    {
        if (member(v, key) != nullptr)
        {
            fail(member_path(path, key), std::format("not a member of a {} extent", arrangement));
        }

        return;
    }

    //--------------------------------------------------------------------------
    std::string range_text(const range_type& range)
    {
        return range.lower() == range.upper() ? std::format("{}", range.lower()) : std::format("[{}, {}]", range.lower(), range.upper());
    }

    //--------------------------------------------------------------------------
    std::string box_text(const RegionExtent::Box& box)
    {
        std::vector<std::string> members;
        const std::array<const std::optional<range_type>*, 3> axes {&box.x_extent, &box.y_extent, &box.z_extent};
        for (size_t i = 0; i < box_keys.size(); ++i)
        {
            if (axes[i]->has_value())
            {
                members.push_back(std::format("{}: {}", string_text(box_keys[i]), range_text(axes[i]->value())));
            }
        }

        return object_text(members);
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_format::RegionExtent quetzal::stratum_format::detail::read_region_extent(const raw_values_type& v, const std::string& path)
{
    object(v, path);
    check_keys(v, path, extent_keys, "region extent");

    const std::string arrangement_path = member_path(path, arrangement_key);
    const std::string arrangement = string_value(required_member(v, path, arrangement_key), arrangement_path);

    RegionExtent extent;
    if (arrangement == ranges_arrangement)
    {
        reject_member(v, path, positions_key, ranges_arrangement);

        const std::string boxes_path = member_path(path, boxes_key);
        const raw_values_type& boxes = array(required_member(v, path, boxes_key), boxes_path);
        for (size_t i = 0; i < boxes.size(); ++i)
        {
            extent.boxes.push_back(read_box(boxes[i], element_path(boxes_path, i)));
        }
    }
    else if (arrangement == positions_arrangement)
    {
        reject_member(v, path, boxes_key, positions_arrangement);
        extent.arrangement = RegionExtent::Arrangement::Positions;

        const std::string positions_path = member_path(path, positions_key);
        const raw_values_type& positions = array(required_member(v, path, positions_key), positions_path);
        for (size_t i = 0; i < positions.size(); ++i)
        {
            const std::string position_path = element_path(positions_path, i);
            const raw_values_type& position = fixed_array(positions[i], position_path, 3);
            extent.positions.push_back(zone_position_type {coord_value(position[0], element_path(position_path, 0)),
                coord_value(position[1], element_path(position_path, 1)), coord_value(position[2], element_path(position_path, 2))});
        }
    }
    else
    {
        fail(arrangement_path, std::format("{} or {} was expected", ranges_arrangement, positions_arrangement));
    }

    return extent;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::region_extent_text(const RegionExtent& extent)
{
    std::vector<std::string> elements;

    if (extent.arrangement == RegionExtent::Arrangement::Ranges)
    {
        for (const auto& box : extent.boxes)
        {
            elements.push_back(box_text(box));
        }

        const std::string boxes = elements.empty() ? "[]" : std::format("[ {} ]", joined(elements));
        return object_text({std::format("{}: {}", string_text(arrangement_key), string_text(ranges_arrangement)),
            std::format("{}: {}", string_text(boxes_key), boxes)});
    }

    for (const auto& position : extent.positions)
    {
        elements.push_back(std::format("[{}, {}, {}]", position.x(), position.y(), position.z()));
    }

    return object_text({std::format("{}: {}", string_text(arrangement_key), string_text(positions_arrangement)),
        std::format("{}: [{}]", string_text(positions_key), joined(elements))});
}
