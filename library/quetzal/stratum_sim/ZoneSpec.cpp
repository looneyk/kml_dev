//------------------------------------------------------------------------------
// stratum_sim
// ZoneSpec.cpp
//------------------------------------------------------------------------------

#include "ZoneSpec.hpp"

//------------------------------------------------------------------------------
quetzal::stratum_core::ZoneSpec quetzal::stratum_sim::resolve_zone_spec(const ZoneSpec& zone_spec, PatternResolver& resolver)
{
    return ZoneSpec{resolver(zone_spec.store)};
}
