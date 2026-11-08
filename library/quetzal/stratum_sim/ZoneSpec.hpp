#if !defined(QUETZAL_STRATUM_SIM_ZONE_SPEC_HPP)
#define QUETZAL_STRATUM_SIM_ZONE_SPEC_HPP
//------------------------------------------------------------------------------
// stratum_sim
// ZoneSpec.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/KeyValueStore.hpp"

namespace quetzal::stratum_sim
{

    struct ZonePosition;

    //--------------------------------------------------------------------------
    // The description of a zone that World creates the zone from
    struct ZoneSpec
    {
        // The position given by the position keys, 0 where a key is not found
        // Throws std::runtime_error if a coordinate is not an integer
        ZonePosition position() const;

        void set_position(const ZonePosition& position);

        KeyValueStore store;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_ZONE_SPEC_HPP
