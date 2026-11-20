#if !defined(QUETZAL_STRATUM_FORMAT_ACCESSPOINTDESTINATION_HPP)
#define QUETZAL_STRATUM_FORMAT_ACCESSPOINTDESTINATION_HPP
//------------------------------------------------------------------------------
// stratum_format
// AccessPointDestination.hpp
//------------------------------------------------------------------------------

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    struct AccessPointDestination
    {
        std::optional<std::string> region_spec_name;
        std::optional<std::string> zone_extended_name;
        // work in progress ...
        // add string conversions/format ...
        // put the extended name functions elsewhere ...

        friend bool operator==(const AccessPointDestination&, const AccessPointDestination&) = default;
    };

    //--------------------------------------------------------------------------
    // Returns nothing if the text is not <region spec name>, <region spec name>/<zone extended name>, or <zone extended name>,
    // where a zone extended name is ../[k]
    std::optional<AccessPointDestination> parse_access_point_destination(std::string_view text);

    // Returns the target's text
    std::string format_access_point_destination(const AccessPointDestination& target);

    // Returns ../[k], the extended name of the zone k zone levels from the current zone in the same region
    std::string relative_zone_extended_name(std::int32_t k);

    // Returns k if the zone extended name is ../[k], nullopt otherwise
    std::optional<std::int32_t> relative_zone_level(std::string_view zone_extended_name);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_ACCESSPOINTDESTINATION_HPP
