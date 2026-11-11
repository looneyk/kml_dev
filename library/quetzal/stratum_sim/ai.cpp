//------------------------------------------------------------------------------
// stratum_sim
// ai.cpp
//------------------------------------------------------------------------------

#include "ai.hpp"
#include "file_components.hpp"
#include "properties.hpp"
#include "Site.hpp"
#include <vector>

//------------------------------------------------------------------------------
void quetzal::stratum_sim::register_ai_components(flecs::world& entities)
{
    static_assert(sizeof(point_type) == 3 * sizeof(value_type));
    entities.component<point_type>().array<value_type>(3);
    register_optional<EntityReference>(entities);
    entities.component<Pursue>()
        .member(&Pursue::target, "target")
        .member(&Pursue::position_last_seen, "position_last_seen")
        .member(&Pursue::seen, "seen");

    register_file_component<Pursue>(entities, "pursue");

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::think(World& world, id_type actor_controlled)
{
    // Gathered first, as setting a component during iteration can move its entity to another table
    std::vector<flecs::entity> thinkers;

    world.entities().query_builder<const Pursue>()
        .build()
        .each([&](flecs::entity e, const Pursue&)
        {
            thinkers.push_back(e);
        });

    for (flecs::entity e : thinkers)
    {
        if (!e.is_alive() || static_cast<id_type>(e.id()) == actor_controlled)
        {
            continue;
        }

        flecs::entity zone = zone_of(e);
        const Site* psite = e.try_get<Site>();

        if (!zone || psite == nullptr)
        {
            continue;
        }

        const placement_type* pplacement = &psite->placement;

        const Layout& layout = world.layout(zone);
        Pursue pursue = e.get<Pursue>();

        flecs::entity target = pursue.target.has_value() ? world.resolve(*pursue.target) : world.entities().entity(actor_controlled);
        const Site* ptarget_site = target.is_alive() && zone_of(target) == zone ? target.try_get<Site>() : nullptr;
        const placement_type* ptarget = ptarget_site != nullptr ? &ptarget_site->placement : nullptr;

        if (ptarget != nullptr && world.visible(e, *ptarget))
        {
            pursue.position_last_seen = ptarget->position();
            pursue.seen = true;
        }

        if (pursue.seen && layout.same_location(*pplacement, placement_type(pursue.position_last_seen, pplacement->direction())))
        {
            pursue.seen = false;
        }

        e.set<Pursue>(pursue);

        if (!pursue.seen)
        {
            continue;
        }

        // The reachable neighbor nearest the goal. Neighbors are in a fixed order, so ties break the same way every run
        bool found = false;
        vector_type direction;
        value_type nearest = (pursue.position_last_seen - pplacement->position()).norm_squared();

        for (const placement_type& neighbor : world.neighbors(e))
        {
            value_type d = (pursue.position_last_seen - neighbor.position()).norm_squared();
            if (d < nearest)
            {
                nearest = d;
                direction = neighbor.position() - pplacement->position();
                found = true;
            }
        }

        if (found)
        {
            e.set<Intent>(Intent {direction, layout.step_distance(), vector_type()});
        }
    }

    return;
}
