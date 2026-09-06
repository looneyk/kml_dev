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

    // The movement vocabulary every view maps its input onto. A first person view sends
    // Step and Turn relative to facing; a top down view sends Step with a direction it
    // chose, which leaves facing alone; a view with no facing of its own to show sends Step
    // with face set, which slaves facing to movement

    //--------------------------------------------------------------------------
    struct Step
    {
        vector_type direction; // Zero to step along the actor's own facing
        value_type distance = value_type(1);

        // Turn to direction before moving, as part of the same action rather than at the
        // cost of one of its own. The turn takes effect even where the step that followed it
        // did not, so walking into a wall still leaves the actor facing it
        bool face = false;
    };

    //--------------------------------------------------------------------------
    // The facing asked for, absolute rather than relative, which the layout snaps to one of
    // its own. A view with a relative turn asks the layout's turn what lies that way and sends
    // the result, so the quantization stays with the layout
    struct TurnTo
    {
        vector_type direction;
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
    // What stopped the actor, one or the other and never both
    struct Blocked
    {
        id_type entity_id = nullid;
        id_type obstruction_wall_id = nullid; // The wall it was stopped by
        id_type obstruction_entity_id = nullid; // The entity that was occupying the cell ahead
    };

    //--------------------------------------------------------------------------
    struct Turned
    {
        id_type entity_id = nullid;
        placement_type placement;
    };

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    // What acting on something did, so that a view can report it. Nothing is the honest result
    // for a bare wall and for anything acting on it does not yet reach
    enum class Interaction
    {
        Nothing,
        Opened,
        Closed
    };

    //--------------------------------------------------------------------------
    struct Interacted
    {
        id_type entity_id = nullid;
        id_type wall_id = nullid;
        Interaction interaction = Interaction::Nothing;
    };

    // Applies one command, appending whatever it produced. Returns the energy it cost,
    // zero when the command did nothing
    int apply(World& world, const Command& command, EventQueue& events);

    // Drains the queue, applying each in submission order
    void apply(World& world, CommandQueue& commands, EventQueue& events);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_COMMANDS_HPP
