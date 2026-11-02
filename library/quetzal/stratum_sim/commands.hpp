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
    // An absolute facing direction, which the layout snaps to one of the directions it supports
    struct TurnTo
    {
        vector_type direction;
    };

    //--------------------------------------------------------------------------
    // Acts on whatever the actor faces: a door, a lever, a basin. The controlled actor goes through an access point it faces
    struct Interact
    {
    };

    //--------------------------------------------------------------------------
    // Enters the access point on the floor of the actor's cell. An access point on a wall is entered by Interact
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
    // The effect of an Interact: Nothing for a bare wall or for anything it has no effect on
    enum class Interaction
    {
        Nothing,
        Opened,
        Closed
    };

    //--------------------------------------------------------------------------
    // The entity acted on is the way through on the wall, or nullid if there is none
    struct Interacted
    {
        id_type entity_id = nullid;
        id_type wall_id = nullid;
        Interaction interaction = Interaction::Nothing;
        id_type acted_on_id = nullid;
    };

    //--------------------------------------------------------------------------
    // The actor went through the departure access point and arrived at the arrival access point, which may be in another zone
    struct Relocated
    {
        id_type entity_id = nullid;
        id_type departure_id = nullid;
        id_type arrival_id = nullid;
    };

    //--------------------------------------------------------------------------
    // The actor has died
    struct Died
    {
        id_type entity_id = nullid;
    };

    // Applies one command, appending its events. Returns its cost, zero when it did nothing
    int apply(World& world, const Command& command, EventQueue& events);

    // Drains the queue, applying each in submission order
    void apply(World& world, CommandQueue& commands, EventQueue& events);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_COMMANDS_HPP
