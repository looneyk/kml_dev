//------------------------------------------------------------------------------
// stratum_format
// AccessPoint.cpp
//------------------------------------------------------------------------------

#include "AccessPoint.hpp"
#include <algorithm>
#include <cassert>
#include <cctype>

namespace
{

    //--------------------------------------------------------------------------
    // A letter followed by letters, digits, and underscores
    bool is_name(std::string_view text)
    {
        return !text.empty() && std::isalpha(static_cast<unsigned char>(text.front()))
            && std::ranges::all_of(text, [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; });
    }

} // namespace

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_format::AccessPoint::destination_type> quetzal::stratum_format::parse_access_point_destination(std::string_view text)
{
    if (is_name(text))
    {
        return RegionSpecName{std::string(text)};
    }

    if (relative_zone_level(text).has_value())
    {
        return ZoneExtendedName{std::string(text)};
    }

    return std::nullopt;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::format_access_point_destination(const AccessPoint::destination_type& destination)
{
    assert(!std::holds_alternative<std::uint64_t>(destination));

    const auto* region_spec_name = std::get_if<RegionSpecName>(&destination);
    return region_spec_name != nullptr ? region_spec_name->value : std::get<ZoneExtendedName>(destination).value;
}
