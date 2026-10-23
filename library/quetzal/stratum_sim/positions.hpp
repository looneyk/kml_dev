#if !defined(QUETZAL_STRATUM_SIM_POSITIONS_HPP)
#define QUETZAL_STRATUM_SIM_POSITIONS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// positions.hpp
//------------------------------------------------------------------------------

#include "Generation.hpp"
#include "Zone.hpp"
#include "types.hpp"
#include <flecs.h>

namespace quetzal::stratum_sim
{

    class World;

    // Returns the position the spec has under the position keys, each coordinate it does not have 0
    // Throws std::runtime_error if a coordinate is not an integer
    ZonePosition zone_position(const ZoneSpec& zone_spec);
    RegionPosition region_position(const RegionSpec& region_spec);

    // Sets the spec's position keys
    void set_zone_position(ZoneSpec& zone_spec, const ZonePosition& position);
    void set_region_position(RegionSpec& region_spec, const RegionPosition& position);

    // The position of a new zone in the region, reached at the level. A placeholder until the world's layout is designed:
    // zone_position_at_level
    ZonePosition new_zone_position(const World& world, flecs::entity region, level_type level);

    // The position of a zone at the level. A placeholder until the world's layout is designed: x and y are 0, and z is the
    // level
    ZonePosition zone_position_at_level(level_type level);

    // The position of a new region in the world. A placeholder until the world's layout is designed: x is one past the
    // largest x of the world's regions, or 0 if it has none, and y is 0
    RegionPosition new_region_position(const World& world);

    // The level of a zone at the position. A placeholder until the world's layout is designed: the level is z
    level_type zone_level(const ZonePosition& position);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_POSITIONS_HPP
