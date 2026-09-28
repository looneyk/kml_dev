#if !defined(QUETZAL_STRATUM_SIM_COMMANDS_HPP)
#define QUETZAL_STRATUM_SIM_COMMANDS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// commands.hpp
//------------------------------------------------------------------------------

#include "CommandQueue.hpp"
#include "EventQueue.hpp"
#include "World.hpp"
#include "types.hpp"

namespace quetzal::stratum_sim
{

    // Commands a view maps its input onto

    //--------------------------------------------------------------------------
    struct Step
    {
        vector_type direction; // Zero to move in the actor's facing direction
        value_type distance = value_type(1);

        // Turn to direction within the same action. The turn takes effect even when the move is blocked
        bool face = false;
    };

    //--------------------------------------------------------------------------
    // An absolute facing direction, which the layout snaps to one of its own
    struct TurnTo
    {
        vector_type direction;
    };

    //--------------------------------------------------------------------------
    // Acts on whatever the actor faces: a door, a lever, a basin. The controlled actor goes through a relocator it faces
    struct Interact
    {
    };

    //--------------------------------------------------------------------------
    // Enters the relocator on the floor of the actor's cell. A relocator on a wall is entered by moving into the wall
    struct Enter
    {
    };

    // Events

    //--------------------------------------------------------------------------
    struct Moved
    {
        id_type entity_id = nullid;
        placement_type placement;
    };

    //--------------------------------------------------------------------------
    // The wall or the entity that stopped the actor, never both
    struct Blocked
    {
        id_type entity_id = nullid;
        id_type blocking_wall_id = nullid;
        id_type blocking_entity_id = nullid;
    };

    //--------------------------------------------------------------------------
    struct Turned
    {
        id_type entity_id = nullid;
        placement_type placement;
    };

    //--------------------------------------------------------------------------
    // The effect of an Interact: Nothing for a bare wall or for anything it has no effect on, and NoDestination for a
    // relocator that leads nowhere the world can resolve
    enum class Interaction
    {
        Nothing,
        Opened,
        Closed,
        NoDestination
    };

    //--------------------------------------------------------------------------
    // The entity acted on is the way through or the relocator on the wall, or nullid if there is neither
    struct Interacted
    {
        id_type entity_id = nullid;
        id_type wall_id = nullid;
        Interaction interaction = Interaction::Nothing;
        id_type acted_on_id = nullid;
    };

    //--------------------------------------------------------------------------
    // The actor went through the relocator and arrived at the access point, which may be in another zone
    struct Relocated
    {
        id_type entity_id = nullid;
        id_type relocator_id = nullid;
        id_type arrival_id = nullid;
    };

    // Applies one command, appending its events. Returns its cost, zero when it did nothing
    int apply(World& world, const Command& command, EventQueue& events);

    // Drains the queue, applying each in submission order
    void apply(World& world, CommandQueue& commands, EventQueue& events);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_COMMANDS_HPP
