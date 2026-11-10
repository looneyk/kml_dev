#if !defined(QUETZAL_STRATUM_SIM_ZONE_SPEC_HPP)
#define QUETZAL_STRATUM_SIM_ZONE_SPEC_HPP
//------------------------------------------------------------------------------
// stratum_sim
// ZoneSpec.hpp
//------------------------------------------------------------------------------

#include "PatternResolver.hpp"
#include "quetzal/common/KeyValueStore.hpp"

namespace quetzal::stratum_sim
{

    struct ZonePosition;

    //--------------------------------------------------------------------------
    struct ZoneSpec
    {
        // From the position keys
        // Throws std::runtime_error if a coordinate is not an integer
        ZonePosition position() const;

        void set_position(const ZonePosition& position);

        KeyValueStore store;
    };

    // Returns the zone spec with the pattern values in its store entries resolved
    // Throws std::runtime_error for a pattern that does not compile, fails, or has a table or a function as its value
    ZoneSpec resolve_zone_spec(const ZoneSpec& zone_spec, PatternResolver& resolver);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_ZONE_SPEC_HPP
