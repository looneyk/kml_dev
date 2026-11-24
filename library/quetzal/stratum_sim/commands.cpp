//------------------------------------------------------------------------------
// stratum_sim
// commands.cpp
//------------------------------------------------------------------------------

#include "commands.hpp"
#include "access_points.hpp"
#include "actors.hpp"
#include "attachments.hpp"
#include "DiscreteLayout.hpp"
#include "Scheduler.hpp"
#include "Site.hpp"
#include <cmath>

namespace
{

    // True if the controlled actor enters an access point on a wall by moving into the wall, as well as by Interact. Off,
    // so that a held movement key never takes the stairs by accident
    constexpr bool enter_access_point_by_moving = false;

    //--------------------------------------------------------------------------
    int enter_access_point(quetzal::stratum_sim::World& world, flecs::entity actor, flecs::entity access_point,
        quetzal::stratum_sim::EventQueue& events)
    {
        using namespace quetzal::stratum_sim;

        flecs::entity destination = world.relocate_through(actor, access_point);

        events.push(Relocated{static_cast<quetzal::id_type>(actor.id()), static_cast<quetzal::id_type>(access_point.id()),
            static_cast<quetzal::id_type>(destination.id())});

        return Scheduler::action_cost;
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

    flecs::entity zone = zone_of(e);
    const Site* psite = e.try_get<Site>();

    if (!zone || psite == nullptr)
    {
        return 0;
    }

    const LayoutBase& layout = world.layout(zone);
    placement_type placement = psite->placement;

    if (const Step* pstep = command.try_get<Step>())
    {
        vector_type direction = grid_step(pstep->direction).zero() ? placement.direction() : pstep->direction;

        if (pstep->face)
        {
            placement_type faced = layout.face(placement, direction);

            if (faced.direction() != placement.direction())
            {
                placement = faced;
                set_placement(e, placement);
                events.push(Turned{command.entity_id(), placement});
            }
        }

        MoveResult result = world.move(e, direction, pstep->distance);
        bool moved = !layout.same_location(result.placement, placement);

        if (moved)
        {
            set_placement(e, result.placement);
            events.push(Moved{command.entity_id(), result.placement});
        }

        if constexpr (enter_access_point_by_moving)
        {
            flecs::entity access_point = result.blocking_wall_id != nullid && e.has<ControlledActor>()
                ? find_access_point_ahead(zone, WallId{result.blocking_wall_id}) : flecs::entity();

            if (access_point && can_enter(access_point, e))
            {
                return enter_access_point(world, e, access_point, events);
            }
        }

        if (result.blocking_wall_id != nullid || result.blocking_entity_id != nullid)
        {
            events.push(Blocked{command.entity_id(), result.blocking_wall_id, result.blocking_entity_id});
        }

        if (!moved)
        {
            return 0;
        }

        // The cost is proportional to the distance traveled
        value_type traveled = (result.placement.position() - placement.position()).norm();

        return static_cast<int>(std::lround(Scheduler::action_cost * traveled / layout.step_distance()));
    }

    if (const TurnTo* pturn = command.try_get<TurnTo>())
    {
        placement_type turned = layout.face(placement, pturn->direction);

        if (turned.direction() == placement.direction())
        {
            return 0;
        }

        set_placement(e, turned);
        events.push(Turned{command.entity_id(), turned});

        return Scheduler::action_cost;
    }

    if (command.is<Interact>())
    {
        id_type wall_id = layout.ahead(placement);

        if (wall_id == nullid)
        {
            return 0;
        }

        flecs::entity access_point = find_access_point_ahead(zone, WallId{wall_id});
        if (access_point && e.has<ControlledActor>() && can_enter(access_point, e))
        {
            return enter_access_point(world, e, access_point, events);
        }

        Interaction interaction = Interaction::Nothing;
        flecs::entity way = find_access_attachment(zone, wall_id);
        if (!way && access_point && access_entity(access_point))
        {
            way = access_point;
        }

        // A locked way through stays as it is
        if (flecs::entity holder = access_entity(way); holder && holder.get<Access>().state != AccessState::Locked)
        {
            bool open = holder.get<Access>().state == AccessState::Closed;
            holder.set<Access>(Access{open ? AccessState::Open : AccessState::Closed});
            interaction = open ? Interaction::Opened : Interaction::Closed;
        }

        events.push(Interacted{command.entity_id(), wall_id, interaction, way ? static_cast<id_type>(way.id()) : nullid});
        return Scheduler::action_cost;
    }

    if (command.is<Enter>())
    {
        // Only a discrete layout has flat cells
        const auto* discrete_layout = dynamic_cast<const DiscreteLayout*>(&layout);
        flecs::entity access_point = discrete_layout != nullptr && e.has<ControlledActor>() ? find_access_point(zone, FlatCellPosition{discrete_layout->floor_id(),
            discrete_layout->cell_position_at(placement.position())}) : flecs::entity();

        return access_point && can_enter(access_point, e) ? enter_access_point(world, e, access_point, events) : 0;
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
