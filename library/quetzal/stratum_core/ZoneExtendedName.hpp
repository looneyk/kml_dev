#if !defined(QUETZAL_STRATUM_CORE_ZONEEXTENDEDNAME_HPP)
#define QUETZAL_STRATUM_CORE_ZONEEXTENDEDNAME_HPP
//------------------------------------------------------------------------------
// stratum_core
// ZoneExtendedName.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
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

    // Extracts the offset from an extended name (../[k]) (partial support for vector rather than just z coord offsets)
    std::optional<discrete_vector3_type> zone_offset(std::string_view extended_name);

    // Segment names follow C++ identifier syntax
    bool valid_segment_name(std::string_view text);

} // namespace quetzal::stratum_core

#endif // QUETZAL_STRATUM_CORE_ZONEEXTENDEDNAME_HPP
