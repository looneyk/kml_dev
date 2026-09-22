#if !defined(QUETZAL_STRATUM_SIM_ATTACHMENTS_HPP)
#define QUETZAL_STRATUM_SIM_ATTACHMENTS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// attachments.hpp
//------------------------------------------------------------------------------

#include "Site.hpp"
#include "World.hpp"
#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <flecs.h>
#include <utility>
#include <vector>

namespace quetzal::stratum_sim
{

    // Entities attached to a wall or a flat: an alcove, a basin, a door, a button, a torch

    //--------------------------------------------------------------------------
    // Its DoorFacing children are sited on its walls
    struct Door
    {
    };

    //--------------------------------------------------------------------------
    // One side of a door, and a child of it
    struct DoorFacing
    {
    };

    //--------------------------------------------------------------------------
    // On the door, so both of its sides are open or shut together
    struct Openable
    {
        bool open = false;
    };

    //--------------------------------------------------------------------------
    // Crossed without being opened, as a smashed door is
    struct Crossable
    {
    };

    //--------------------------------------------------------------------------
    // Hidden from this side until revealed. On a door facing, so each side of a door is concealed separately
    struct Concealed
    {
        bool revealed = false;
    };

    //--------------------------------------------------------------------------
    // A door with a door facing on each of its walls: usually two, and one where a doorway leads into a dead end
    flecs::entity create_door(World& world, flecs::entity zone, const std::vector<id_type>& wall_ids);

    //--------------------------------------------------------------------------
    // The wall ids of each door in a zone
    using door_walls_type = std::vector<std::vector<id_type>>;

    // Each door is created closed. Returns the count
    size_t create_doors(World& world, flecs::entity zone, const door_walls_type& doors);

    //--------------------------------------------------------------------------
    // The entity sited on a wall of the zone, or a null entity
    flecs::entity facing_on(flecs::entity zone, id_type wall_id);

    flecs::entity door_of(flecs::entity facing);

    flecs::entity other_facing(flecs::entity facing);

    //--------------------------------------------------------------------------
    // A wall with nothing sited on it, or with a concealed door facing not yet revealed, is solid. Otherwise an Openable
    // door is crossable when open, and any other when it has Crossable
    bool crossable(flecs::entity zone, id_type wall_id);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_ATTACHMENTS_HPP
