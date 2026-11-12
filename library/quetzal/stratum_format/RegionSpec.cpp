//------------------------------------------------------------------------------
// stratum_format
// RegionSpec.cpp
//------------------------------------------------------------------------------

#include "RegionSpec.hpp"
#include "checked_values.hpp"
#include "json_util.hpp"
#include <algorithm>
#include <array>
#include <format>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    constexpr std::string_view name_key = "name";
    constexpr std::string_view display_name_key = "display_name";
    constexpr std::string_view parent_key = "parent";
    constexpr std::string_view region_spec_key = "region_spec";
    constexpr std::string_view seed_key = "seed";
    constexpr std::string_view extent_key = "extent";
    constexpr std::string_view zone_size_key = "zone_size";

    //--------------------------------------------------------------------------
    void add_text_member(std::vector<std::string>& members, std::string_view key, const std::optional<std::string>& value)
    {
        if (value.has_value())
        {
            members.push_back(std::format("{}: {}", string_text(key), string_text(value.value())));
        }

        return;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_format::RegionSpec quetzal::stratum_format::detail::read_region_spec(const raw_values_type& v, const std::string& path)
{
    RegionSpec region_spec;

    const std::array<std::pair<std::string_view, std::optional<std::string>*>, 4> text_members
    {{
        {name_key, &region_spec.name},
        {display_name_key, &region_spec.display_name},
        {parent_key, &region_spec.parent},
        {region_spec_key, &region_spec.region_spec}
    }};

    for (const auto& [key, value] : object(v, path).items())
    {
        const std::string value_path = member_path(path, key);
        auto text_member = std::ranges::find_if(text_members, [&key](const auto& entry) { return entry.first == key; });

        if (text_member != text_members.end())
        {
            *text_member->second = string_value(value, value_path);
        }
        else if (key == seed_key)
        {
            region_spec.seed = seed_value(value, value_path);
        }
        else if (key == extent_key)
        {
            region_spec.extent = read_region_extent(value, value_path);
        }
        else if (key == zone_size_key)
        {
            const raw_values_type& size = fixed_array(value, value_path, 2);
            region_spec.zone_size = discrete_vector_type(coord_value(size[0], element_path(value_path, 0)),
                coord_value(size[1], element_path(value_path, 1)));
        }
        else if (key.starts_with(std::format("{}.", extent_key)) || key.starts_with(std::format("{}.", zone_size_key)))
        {
            fail(value_path, std::format("a part of {}, written in its value", key.substr(0, key.find('.'))));
        }
        else
        {
            region_spec.store.insert(key, string_value(value, value_path));
        }
    }

    return region_spec;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::region_spec_text(const RegionSpec& region_spec)
{
    std::vector<std::string> members;
    add_text_member(members, name_key, region_spec.name);
    add_text_member(members, display_name_key, region_spec.display_name);
    add_text_member(members, parent_key, region_spec.parent);
    add_text_member(members, region_spec_key, region_spec.region_spec);

    if (region_spec.seed.has_value())
    {
        members.push_back(std::format("{}: {}", string_text(seed_key), string_text(std::to_string(region_spec.seed.value()))));
    }

    if (region_spec.extent.has_value())
    {
        members.push_back(std::format("{}: {}", string_text(extent_key), region_extent_text(region_spec.extent.value())));
    }

    if (region_spec.zone_size.has_value())
    {
        const discrete_vector_type& zone_size = region_spec.zone_size.value();
        members.push_back(std::format("{}: [{}, {}]", string_text(zone_size_key), zone_size.x(), zone_size.y()));
    }

    for (const auto& [key, value] : region_spec.store)
    {
        members.push_back(std::format("{}: {}", string_text(key), string_text(value)));
    }

    return object_text(members);
}
