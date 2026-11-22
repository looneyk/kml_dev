//------------------------------------------------------------------------------
// stratum_format
// ZoneExtendedName.cpp
//------------------------------------------------------------------------------

#include "ZoneExtendedName.hpp"
#include "quetzal/common/string_util.hpp"
#include <format>

namespace
{

    constexpr std::string_view relative_zone_prefix = "../[";
    constexpr std::string_view relative_zone_suffix = "]";

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_format::ZoneExtendedName quetzal::stratum_format::relative_zone_extended_name(std::int32_t k)
{
    return ZoneExtendedName{std::format("{}{:+}{}", relative_zone_prefix, k, relative_zone_suffix)};
}

//------------------------------------------------------------------------------
std::optional<std::int32_t> quetzal::stratum_format::relative_zone_level(std::string_view zone_extended_name)
{
    if (!zone_extended_name.starts_with(relative_zone_prefix) || !zone_extended_name.ends_with(relative_zone_suffix))
    {
        return std::nullopt;
    }

    const std::string_view k = zone_extended_name.substr(relative_zone_prefix.size(),
        zone_extended_name.size() - relative_zone_prefix.size() - relative_zone_suffix.size());

    if (!k.starts_with('-') && !(k.starts_with('+') && !k.substr(1).starts_with('-')))
    {
        return std::nullopt;
    }

    return quetzal::try_type<std::int32_t>(k.starts_with('+') ? k.substr(1) : k);
}
