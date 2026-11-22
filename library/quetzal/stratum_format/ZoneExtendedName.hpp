#if !defined(QUETZAL_STRATUM_FORMAT_ZONEEXTENDEDNAME_HPP)
#define QUETZAL_STRATUM_FORMAT_ZONEEXTENDEDNAME_HPP
//------------------------------------------------------------------------------
// stratum_format
// ZoneExtendedName.hpp
//------------------------------------------------------------------------------

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    struct ZoneExtendedName
    {
        std::string value;

        friend bool operator==(const ZoneExtendedName&, const ZoneExtendedName&) = default;
    };

    //--------------------------------------------------------------------------
    // Returns nothing if the text does not match the extended name grammar
    std::optional<ZoneExtendedName> parse_zone_extended_name(std::string_view text);

    // Returns ../[k], the extended name of the zone k zone levels from the current zone in the same region
    ZoneExtendedName relative_zone_extended_name(std::int32_t k);

    // Returns k if the zone extended name is ../[k], nullopt otherwise
    std::optional<std::int32_t> relative_zone_level(std::string_view zone_extended_name);

    // Returns true if the text is a letter or underscore followed by letters, digits, and underscores
    bool valid_segment_name(std::string_view text);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_ZONEEXTENDEDNAME_HPP
