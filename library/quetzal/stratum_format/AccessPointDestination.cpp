//------------------------------------------------------------------------------
// stratum_format
// AccessPointDestination.cpp
//------------------------------------------------------------------------------

#include "AccessPointDestination.hpp"
#include "quetzal/common/string_util.hpp"
#include <algorithm>
#include <format>
#include <cctype>

namespace
{

    constexpr std::string_view relative_zone_prefix = "../[";
    constexpr std::string_view relative_zone_suffix = "]";

    //--------------------------------------------------------------------------
    // A letter followed by letters, digits, and underscores
    bool is_name(std::string_view text)
    {
        return !text.empty() && std::isalpha(static_cast<unsigned char>(text.front()))
            && std::ranges::all_of(text, [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; });
    }

} // namespace

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_format::AccessPointDestination> quetzal::stratum_format::parse_access_point_destination(std::string_view text)
{
    AccessPointDestination target;

    if (!text.empty() && std::isalpha(static_cast<unsigned char>(text.front())))
    {
        const size_t slash = text.find('/');
        const std::string_view region_spec_name = text.substr(0, slash);
        if (!is_name(region_spec_name))
        {
            return std::nullopt;
        }

        target.region_spec_name = std::string(region_spec_name);
        if (slash == std::string_view::npos)
        {
            return target;
        }

        text = text.substr(slash + 1);
    }

    if (!relative_zone_level(text).has_value())
    {
        return std::nullopt;
    }

    target.zone_extended_name = std::string(text);
    return target;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::format_access_point_destination(const AccessPointDestination& target)
{
    if (!target.zone_extended_name.has_value())
    {
        return target.region_spec_name.value();
    }

    return target.region_spec_name.has_value() ? std::format("{}/{}", *target.region_spec_name, *target.zone_extended_name)
        : *target.zone_extended_name;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::relative_zone_extended_name(std::int32_t k)
{
    return std::format("{}{:+}{}", relative_zone_prefix, k, relative_zone_suffix);
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
