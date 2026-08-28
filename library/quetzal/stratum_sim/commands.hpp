#if !defined(QUETZAL_STRATUM_SIM_COMMANDS_HPP)
#define QUETZAL_STRATUM_SIM_COMMANDS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// commands.hpp
//------------------------------------------------------------------------------

#include "CommandQueue.hpp"
#include "EventQueue.hpp"
#include "Layout.hpp"
#include "World.hpp"
#include "types.hpp"

namespace quetzal::stratum_sim
{

    // The movement vocabulary every view maps its input onto. A first person view sends
    // Step and Turn relative to facing; a top down view sends Step with a direction it
    // chose, which leaves facing alone

    //--------------------------------------------------------------------------
    struct Step
    {
        vector_type direction; // Zero to step along the actor's own facing
        value_type distance = value_type(1);
    };

    //--------------------------------------------------------------------------
    struct TurnTo
    {
        Turn turn = Turn::Left;
    };

    //--------------------------------------------------------------------------
    // Acts on whatever the actor faces: a door, a lever, a basin
    struct Interact
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
    struct Blocked
    {
        id_type entity_id = nullid;
        id_type wall_id = nullid; // What stopped it
    };

    //--------------------------------------------------------------------------
    struct Turned
    {
        id_type entity_id = nullid;
        placement_type placement;
    };

    //--------------------------------------------------------------------------
    struct Interacted
    {
        id_type entity_id = nullid;
        id_type wall_id = nullid;
    };

    // Applies one command, appending whatever it produced. Returns the energy it cost,
    // zero when the command did nothing
    int apply(World& world, const Command& command, EventQueue& events);

    // Drains the queue, applying each in submission order
    void apply(World& world, CommandQueue& commands, EventQueue& events);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_COMMANDS_HPP
