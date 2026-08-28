//------------------------------------------------------------------------------
// stratum_sim
// commands.cpp
//------------------------------------------------------------------------------

#include "commands.hpp"
#include "Scheduler.hpp"
#include "quetzal/math/floating_point.hpp"

namespace
{

    //--------------------------------------------------------------------------
    // The cell step a direction most nearly asks for, zero when it asks for nothing
    quetzal::stratum_sim::Cell step_of(const quetzal::stratum_sim::vector_type& direction)
    {
        using quetzal::stratum_sim::value_type;

        auto [dx, dy, dz] = direction.rep();

        if (quetzal::math::float_eq0(dx) && quetzal::math::float_eq0(dy))
        {
            return {0, 0};
        }

        return std::abs(dx) >= std::abs(dy)
            ? quetzal::stratum_sim::Cell {dx > value_type(0) ? 1 : -1, 0}
            : quetzal::stratum_sim::Cell {0, dy > value_type(0) ? 1 : -1};
    }

} // namespace

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

    Layout& layout = world.layout(zone);
    placement_type placement = *pplacement;

    if (const Step* pstep = command.try_get<Step>())
    {
        // A zero direction means step along the actor's own facing
        vector_type direction = step_of(pstep->direction) == Cell {0, 0} ? placement.direction() : pstep->direction;

        MoveResult result = layout.move(placement, direction, pstep->distance);
        bool moved = !layout.same_location(result.placement, placement);

        if (moved)
        {
            e.set<placement_type>(result.placement);
            events.push(Moved {command.entity_id(), result.placement});
        }

        if (result.obstruction != nullid)
        {
            events.push(Blocked {command.entity_id(), result.obstruction});
        }

        return moved ? Scheduler::action_cost : 0;
    }

    if (const TurnTo* pturn = command.try_get<TurnTo>())
    {
        placement_type turned = layout.turn(placement, pturn->turn);
        e.set<placement_type>(turned);
        events.push(Turned {command.entity_id(), turned});

        return Scheduler::action_cost;
    }

    if (command.is<Interact>())
    {
        Cell step = step_of(placement.direction());
        if (step == Cell {0, 0})
        {
            return 0;
        }

        Cell cell = layout.cell_at(placement.position());
        id_type wall_id = layout.facing_between(cell, Cell {cell.i + step.i, cell.j + step.j});

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
