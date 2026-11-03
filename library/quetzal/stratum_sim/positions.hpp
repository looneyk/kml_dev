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
#include <string_view>

namespace quetzal::stratum_sim
{

    class World;

    // Returns the position the spec has under the position keys, each coordinate it does not have 0
    // Throws std::runtime_error if a coordinate is not an integer
    RegionPosition region_position(const RegionSpec& region_spec);
    ZonePosition zone_position(const ZoneSpec& zone_spec);

    // Sets the spec's position keys
    void set_region_position(RegionSpec& region_spec, const RegionPosition& position);
    void set_zone_position(ZoneSpec& zone_spec, const ZonePosition& position);

    // The position of a new region's seed. A placeholder until seeds are drawn hierarchically: x is one past the largest x
    // of the world's region specs, or 0 if it has none, and y is 0
    RegionPosition new_region_position(const World& world);

    // The position of a new zone in the worldspace, at the given level. A placeholder until the world's layout is designed:
    // x and y are 0, and z is the level
    ZonePosition new_zone_position(const World& world, flecs::entity worldspace, level_type level);

    bool contains(const ZoneBox& box, const ZonePosition& position);

    // Returns the worldspace that a top-level region spec's worldspace keys state, with no zones. Throws std::runtime_error
    // if an extent is not an interval of integers, or the zone size has one coordinate without the other
    Worldspace make_worldspace(std::string_view name, const RegionSpec& region_spec);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_POSITIONS_HPP
