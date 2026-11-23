#if !defined(QUETZAL_STRATUM_CORE_ZONEEXTENDEDNAME_HPP)
#define QUETZAL_STRATUM_CORE_ZONEEXTENDEDNAME_HPP
//------------------------------------------------------------------------------
// stratum_core
// ZoneExtendedName.hpp
//------------------------------------------------------------------------------

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace quetzal::stratum_core
{

    //--------------------------------------------------------------------------
    struct ZoneExtendedName
    {
        std::string value;

        friend bool operator==(const ZoneExtendedName&, const ZoneExtendedName&) = default;
    };

    //--------------------------------------------------------------------------
    // Returns nullopt if the text is not a valid extended name
    std::optional<ZoneExtendedName> parse_zone_extended_name(std::string_view text);

    // Returns ../[k], the extended name of the zone k zones from the current zone in the same region
    ZoneExtendedName relative_zone_extended_name(std::int32_t k);

    // Extracts the zone offset from a zone extended name (../[k])
    std::optional<std::int32_t> relative_zone_offset(std::string_view extended_name);

    // Segment names follow C++ identifier syntax
    bool valid_segment_name(std::string_view text);

} // namespace quetzal::stratum_core

#endif // QUETZAL_STRATUM_CORE_ZONEEXTENDEDNAME_HPP
