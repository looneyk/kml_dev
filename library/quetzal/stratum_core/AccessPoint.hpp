#if !defined(QUETZAL_STRATUM_CORE_ACCESSPOINT_HPP)
#define QUETZAL_STRATUM_CORE_ACCESSPOINT_HPP
//------------------------------------------------------------------------------
// stratum_core
// AccessPoint.hpp
//------------------------------------------------------------------------------

#include "PersistentId.hpp"
#include "types.hpp"
#include "ZoneExtendedName.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace quetzal::stratum_core
{

    //--------------------------------------------------------------------------
    struct RegionSpecName
    {
        std::string value;

        friend bool operator==(const RegionSpecName&, const RegionSpecName&) = default;
    };

    //--------------------------------------------------------------------------
    struct AccessPoint
    {
        using destination_type = std::variant<PersistentId, RegionSpecName, ZoneExtendedName>;

        std::uint64_t order = 0; // count from 0 among the zone's access points of the same type // probably deprecated ...
        bool automatic_entry = false; // The acces point is activated by an actor's presence in its cell rather than by explicit interaction
        discrete_vector_type egress; // The offset from this access point position, when used as a destination, that an actor is placed at following transit
        std::optional<destination_type> destination; // nullopt indicates that the access point is not traversable
    };

    //--------------------------------------------------------------------------
    // Returns nullopt if the text is neither a RegionSpecName nor a ZoneExtendedName
    std::optional<AccessPoint::destination_type> parse_access_point_destination(std::string_view text);

    std::string format_access_point_destination(const AccessPoint::destination_type& destination);

} // namespace quetzal::stratum_core

#endif // QUETZAL_STRATUM_CORE_ACCESSPOINT_HPP
