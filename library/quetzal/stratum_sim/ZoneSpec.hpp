#if !defined(QUETZAL_STRATUM_SIM_ZONE_SPEC_HPP)
#define QUETZAL_STRATUM_SIM_ZONE_SPEC_HPP
//------------------------------------------------------------------------------
// stratum_sim
// ZoneSpec.hpp
//------------------------------------------------------------------------------

#include "PatternResolver.hpp"
#include "quetzal/stratum_core/ZoneSpec.hpp"

namespace quetzal::stratum_sim
{

    using stratum_core::ZoneSpec;

    // Returns the zone spec with the pattern values in its store entries resolved
    // Throws std::runtime_error for a pattern that does not compile, fails, or has a table or a function as its value
    ZoneSpec resolve_zone_spec(const ZoneSpec& zone_spec, PatternResolver& resolver);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_ZONE_SPEC_HPP
