#if !defined(QUETZAL_STRATUM_SIM_ATTACHMENTS_HPP)
#define QUETZAL_STRATUM_SIM_ATTACHMENTS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// attachments.hpp
//------------------------------------------------------------------------------

#include "World.hpp"
#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <flecs.h>
#include <utility>
#include <vector>

namespace quetzal::stratum_sim
{

    // What is attached to a wall or a flat: an alcove, a basin, a door, a button, a torch. One
    // mechanism covers all of them, which is what keeps Wall a small dense record
    // See attachment_plan.md for the shape and sim_zone_model.md for why it is that shape

    //--------------------------------------------------------------------------
    // Where an attachment sits on the surface it hangs on, along it from its start and up
    // from the floor. A door facing fills its wall, so both are zero until something places
    // an attachment that does not
    struct Attachment
    {
        value_type along = value_type(0);
        value_type up = value_type(0);
    };

    //--------------------------------------------------------------------------
    // The wall an attachment hangs on. An id rather than a relationship because a wall is a
    // record and not an entity; the reverse direction is a query filtered by this
    struct OnWall
    {
        id_type wall_id = nullid;
    };

    //--------------------------------------------------------------------------
    // Attached to a wall and allows passage. Door is the variety that has Openable
    struct Door
    {
    };

    //--------------------------------------------------------------------------
    // One side of a door. The door it belongs to is its parent rather than a field, and the
    // other facing is its sibling
    struct DoorFacing
    {
    };

    //--------------------------------------------------------------------------
    // Shared by both facings: one door cannot be open from one side and shut from the other
    struct Openable
    {
        bool open = false;
    };

    //--------------------------------------------------------------------------
    // Crossed without being opened, which is what a door smashed to a hole leaves. Blocking
    // debris is the same attachment without this
    struct Passable
    {
    };

    //--------------------------------------------------------------------------
    // Secret from this side until it is found, so per facing rather than shared: the classic
    // secret door is concealed on one facing and an ordinary door on the other
    struct Concealed
    {
        bool found = false;
    };

    //--------------------------------------------------------------------------
    // A door on the walls it presents: one entity for what its sides share, and a facing on
    // each wall. Two in the ordinary case -- the two sides of one edge in the wall format,
    // the two faces a doorway block presents in the block format -- and one where a doorway
    // leads into a dead end.
    flecs::entity create_door(World& world, flecs::entity zone, const std::vector<id_type>& wall_ids);

    //--------------------------------------------------------------------------
    // The walls of each door a level file describes, as load_layout reports them
    using door_walls_type = std::vector<std::vector<id_type>>;

    // A door on each, built closed. Returns count
    size_t create_doors(World& world, flecs::entity zone, const door_walls_type& doors);

    //--------------------------------------------------------------------------
    // What is attached to a wall, and a null entity where nothing is. A wall id identifies a wall in
    // one zone's layout, so the zone is what the search is over
    flecs::entity facing_on(flecs::entity zone, id_type wall_id);

    // What the facing belongs to, which is its parent
    flecs::entity door_of(flecs::entity facing);

    // The other side of the same door, which is its sibling
    flecs::entity other_facing(flecs::entity facing);

    //--------------------------------------------------------------------------
    // Whether a mover may cross this wall: what is attached to it, and what that permits. A wall
    // with nothing attached is solid, an openable attachment reports whether it is open, and
    // anything else whether it may simply be walked through
    // A facing concealed and not yet found refuses, a secret door being a wall until it is
    // found. See attachment_plan.md for why this takes no actor yet
    bool crossable(flecs::entity zone, id_type wall_id);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_ATTACHMENTS_HPP
