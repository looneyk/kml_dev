#if !defined(QUETZAL_STRATUM_SIM_ACCESS_POINTS_HPP)
#define QUETZAL_STRATUM_SIM_ACCESS_POINTS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// access_points.hpp
//------------------------------------------------------------------------------

#include "Site.hpp"
#include "types.hpp"
#include "quetzal/stratum_core/AccessPoint.hpp"
#include "quetzal/stratum_core/Directionality.hpp"
#include <flecs.h>
#include <cstdint>
#include <optional>
#include <variant>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Registers the file components of the entity an access point is on, such as its directionality
    void register_access_point_components(flecs::world& entities);

    //--------------------------------------------------------------------------
    // Returns true if the entity's access point destination is unbound and is the level offset levels from the entity's zone,
    // whether the destination is written relative or absolute. An absolute destination matches only in a zone that has a level
    bool targets_level(flecs::entity e, level_type offset);

    // Returns true if an actor coming from the level offset levels from the entity's zone can arrive at the entity's access
    // point: it has no destination, or its destination leads back to that level
    bool accepts_arrival(flecs::entity e, level_type offset);

    //--------------------------------------------------------------------------
    // Makes an entity an access point
    void set_access_point(flecs::entity e, const stratum_core::AccessPoint& access_point);

    // Creates an entity of the type in the zone at the site and makes it an access point
    flecs::entity create_access_point(flecs::entity zone, flecs::entity type, const Site& site, const stratum_core::AccessPoint& access_point);

    // The placement of an actor when it has arrived at an access point's destination
    placement_type destination_placement(flecs::entity access_point);

    // Returns the lowest-ordered unbound access point of the type in the zone that has a valid destination if any
    flecs::entity entry_point(flecs::entity zone, flecs::entity type, level_type offset);

    //--------------------------------------------------------------------------
    // Returns an access point sited on the wall or the flat cell, or a null entity if there is none
    flecs::entity find_access_point(flecs::entity zone, const WallId& wall);
    flecs::entity find_access_point(flecs::entity zone, const FlatCellPosition& flat_cell);

    // Returns the access point that an actor facing the wall enters: one sited on the wall, or one on the floor of the cell
    // beyond the wall whose egress leads back through it, or a null entity if there is neither
    flecs::entity find_entered_access_point(flecs::entity zone, const WallId& wall);

    // Returns true if the actor can go through the access point: it has a destination, its access, if it has any, is open, and
    // the actor does not find it concealed
    bool can_enter(flecs::entity access_point, flecs::entity actor);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_ACCESS_POINTS_HPP
