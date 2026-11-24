#if !defined(QUETZAL_STRATUM_CORE_ZONE_SPEC_HPP)
#define QUETZAL_STRATUM_CORE_ZONE_SPEC_HPP
//------------------------------------------------------------------------------
// stratum_core
// ZoneSpec.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/common/KeyValueStore.hpp"

namespace quetzal::stratum_core
{

    //--------------------------------------------------------------------------
    struct ZoneSpec
    {
        // From the position keys
        // Throws std::runtime_error if a coordinate is not an integer
        discrete_point3_type position() const;

        void set_position(const discrete_point3_type& position);

        KeyValueStore store;
    };

} // namespace quetzal::stratum_core

#endif // QUETZAL_STRATUM_CORE_ZONE_SPEC_HPP
