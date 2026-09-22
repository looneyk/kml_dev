//------------------------------------------------------------------------------
// stratum_sim
// commands.cpp
//------------------------------------------------------------------------------

#include "commands.hpp"
#include "attachments.hpp"
#include "Scheduler.hpp"
#include "Site.hpp"
#include <cmath>

//------------------------------------------------------------------------------
int quetzal::stratum_sim::apply(World& world, const Command& command, EventQueue& events)
{
    flecs::entity e = world.entities().entity(command.entity_id());
    if (!e || !e.is_alive())
    {
        return 0;
    }

    flecs::entity zone = zone_of(e);
    const Site* psite = e.try_get<Site>();

    if (!zone || psite == nullptr)
    {
        return 0;
    }

    const Layout& layout = world.layout(zone);
    placement_type placement = psite->placement;

    if (const Step* pstep = command.try_get<Step>())
    {
        // A zero direction means step along the actor's own facing
        vector_type direction = grid_step(pstep->direction).zero() ? placement.direction() : pstep->direction;

        if (pstep->face)
        {
            placement_type faced = layout.face(placement, direction);

            if (faced.direction() != placement.direction())
            {
                placement = faced;
                set_placement(e, placement);
                events.push(Turned {command.entity_id(), placement});
            }
        }

        // Asked of the world rather than the layout, so that a wall and something occupying
        // the cell ahead stop the actor in one query
        MoveResult result = world.move(e, direction, pstep->distance);
        bool moved = !layout.same_location(result.placement, placement);

        if (moved)
        {
            set_placement(e, result.placement);
            events.push(Moved {command.entity_id(), result.placement});
        }

        if (result.blocking_wall_id != nullid || result.blocking_entity_id != nullid)
        {
            events.push(Blocked{command.entity_id(), result.blocking_wall_id, result.blocking_entity_id});
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

        set_placement(e, turned);
        events.push(Turned {command.entity_id(), turned});

        return Scheduler::action_cost;
    }

    if (command.is<Interact>())
    {
        id_type wall_id = layout.ahead(placement);

        if (wall_id == nullid)
        {
            return 0; // Nothing in front to act on
        }

        // A door opens and shuts. Anything else in front is reported and does nothing, there
        // being nothing else yet that acting on has any effect upon
        Interaction interaction = Interaction::Nothing;
        flecs::entity facing = facing_on(zone, wall_id);

        if (facing)
        {
            flecs::entity door = door_of(facing);

            if (door.has<Openable>())
            {
                bool open = !door.get<Openable>().open;
                door.set<Openable>(Openable {open});
                interaction = open ? Interaction::Opened : Interaction::Closed;
            }
        }

        events.push(Interacted {command.entity_id(), wall_id, interaction});
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
