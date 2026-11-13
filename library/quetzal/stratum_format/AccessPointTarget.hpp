#if !defined(QUETZAL_STRATUM_FORMAT_ACCESSPOINTTARGET_HPP)
#define QUETZAL_STRATUM_FORMAT_ACCESSPOINTTARGET_HPP
//------------------------------------------------------------------------------
// stratum_format
// AccessPointTarget.hpp
//------------------------------------------------------------------------------

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace quetzal::stratum_format
{

    // A zone's position relative to the surface: 0 at the surface, negative going down and positive going up
    using level_type = std::int32_t;

    //--------------------------------------------------------------------------
    // A zone given as a number of levels from the relocator's zone
    struct RelativeZone
    {
        level_type offset = 0;

        friend bool operator==(const RelativeZone&, const RelativeZone&) = default;
    };

    //--------------------------------------------------------------------------
    // A zone given as its level
    struct AbsoluteZone
    {
        level_type level = 0;

        friend bool operator==(const AbsoluteZone&, const AbsoluteZone&) = default;
    };

    //--------------------------------------------------------------------------
    // Deprecated: a target type not yet designed is to replace it.
    // An unbound relocator's target, written [region/]zone[@position]. The region, a zone name, and the position are kept
    // as written
    struct AccessPointTarget
    {
        std::optional<std::string> region;
        std::variant<RelativeZone, AbsoluteZone, std::string> zone;
        std::optional<std::string> position;

        friend bool operator==(const AccessPointTarget&, const AccessPointTarget&) = default;
    };

    //--------------------------------------------------------------------------
    // Returns nothing if the text does not match [region/]zone[@position]
    std::optional<AccessPointTarget> parse_access_point_target(std::string_view text);

    // Returns the target written [region/]zone[@position]
    std::string format_access_point_target(const AccessPointTarget& target);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_ACCESSPOINTTARGET_HPP
