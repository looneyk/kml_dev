//------------------------------------------------------------------------------
// stratum_sim
// commands.cpp
//------------------------------------------------------------------------------

#include "commands.hpp"
#include "DiscreteLayout.hpp" // Interact, which still does cell arithmetic of its own
#include "Scheduler.hpp"
#include <cmath>

//------------------------------------------------------------------------------
int quetzal::stratum_sim::apply(World& world, const Command& command, EventQueue& events)
{
    flecs::entity e = world.entities().entity(command.entity_id());
    if (!e || !e.is_alive())
    {
        return 0;
    }

    flecs::entity zone = world.zone_of(e);
    const placement_type* pplacement = e.try_get<placement_type>();

    if (!zone || pplacement == nullptr)
    {
        return 0;
    }

    const Layout& layout = world.layout(zone);
    placement_type placement = *pplacement;

    if (const Step* pstep = command.try_get<Step>())
    {
        // A zero direction means step along the actor's own facing
        vector_type direction = grid_step(pstep->direction).zero() ? placement.direction() : pstep->direction;

        // Asked of the world rather than the layout, so that a wall and something standing in
        // the way stop the mover in one answer
        MoveResult result = world.move(e, direction, pstep->distance);
        bool moved = !layout.same_location(result.placement, placement);

        if (moved)
        {
            e.set<placement_type>(result.placement);
            events.push(Moved {command.entity_id(), result.placement});
        }

        if (result.obstruction_wall_id != nullid || result.obstruction_entity_id != nullid)
        {
            events.push(Blocked {command.entity_id(), result.obstruction_wall_id, result.obstruction_entity_id});
        }

        if (!moved)
        {
            return 0;
        }

        // What it cost is what it covered rather than a flat action, so that crossing four
        // cells costs four turns and a fraction of a step costs a fraction of one. Being
        // stopped short is therefore charged for the distance actually reached
        value_type covered = (result.placement.position() - placement.position()).norm();

        return static_cast<int>(std::lround(Scheduler::action_cost * covered / layout.step_distance()));
    }

    if (const TurnTo* pturn = command.try_get<TurnTo>())
    {
        placement_type turned = layout.face(placement, pturn->direction);

        // Already facing that way, so nothing was asked of the turn
        if (turned.direction() == placement.direction())
        {
            return 0;
        }

        e.set<placement_type>(turned);
        events.push(Turned {command.entity_id(), turned});

        return Scheduler::action_cost;
    }

    if (command.is<Interact>())
    {
        discrete_vector_type step = grid_step(placement.direction());
        if (step.zero())
        {
            return 0;
        }

        // Cell arithmetic in the rules layer, which is the one thing here the interface does
        // not cover. It goes when the layout is asked what is in front instead
        const DiscreteLayout& cells = layout_as<DiscreteLayout>(layout);

        GridPosition cell = cells.cell_at(placement.position());
        id_type wall_id = cells.wall_toward(cell, step);

        if (wall_id == nullid)
        {
            return 0; // Nothing in front to act on
        }

        events.push(Interacted {command.entity_id(), wall_id});
        return Scheduler::action_cost;
    }

    return 0;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::apply(World& world, CommandQueue& commands, EventQueue& events)
{
    while (!commands.empty())
    {
        apply(world, commands.front(), events);
        commands.pop();
    }

    return;
}
