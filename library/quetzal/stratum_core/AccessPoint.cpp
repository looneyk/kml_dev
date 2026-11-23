//------------------------------------------------------------------------------
// stratum_core
// AccessPoint.cpp
//------------------------------------------------------------------------------

#include "AccessPoint.hpp"
#include <cassert>

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_core::AccessPoint::destination_type> quetzal::stratum_core::parse_access_point_destination(std::string_view text)
{
    if (valid_segment_name(text))
    {
        return RegionSpecName{std::string(text)};
    }

    if (std::optional<ZoneExtendedName> zone_extended_name = parse_zone_extended_name(text); zone_extended_name.has_value())
    {
        return *zone_extended_name;
    }

    return std::nullopt;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_core::format_access_point_destination(const AccessPoint::destination_type& destination)
{
    assert(!std::holds_alternative<PersistentId>(destination));

    const auto* region_spec_name = std::get_if<RegionSpecName>(&destination);
    return region_spec_name != nullptr ? region_spec_name->value : std::get<ZoneExtendedName>(destination).value;
}
