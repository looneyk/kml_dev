#if !defined(QUETZAL_STRATUM_CORE_SPEC_KEYS_HPP)
#define QUETZAL_STRATUM_CORE_SPEC_KEYS_HPP
//------------------------------------------------------------------------------
// stratum_core
// spec_keys.hpp
//------------------------------------------------------------------------------

#include <string_view>

namespace quetzal::stratum_core
{

    // The prefix of a RegionSpec's keys that are passed on to the ZoneSpec of each of its zones
    inline constexpr std::string_view zone_prefix = "zone.";

    // ZoneSpec keys
    inline constexpr std::string_view name_key = "name";
    inline constexpr std::string_view seed_key = "seed";

    // The ZoneSpec and RegionSpec keys of a position's coordinates.
    inline constexpr std::string_view position_x_key = "position.x";
    inline constexpr std::string_view position_y_key = "position.y";
    inline constexpr std::string_view position_z_key = "position.z";
    inline constexpr std::string_view parameters_prefix = "parameters.";

    // The RegionSpec key of the name of the zone at which a character will enter the worldspace
    inline constexpr std::string_view entry_zone_key = "entry_zone";

    // The RegionSpec key that replaces entry_zone_key when the worldspace is created, with the persistent id of that zone
    inline constexpr std::string_view entry_zone_id_key = "entry_zone_id";

} // namespace quetzal::stratum_core

#endif // QUETZAL_STRATUM_CORE_SPEC_KEYS_HPP
