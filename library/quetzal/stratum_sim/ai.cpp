//------------------------------------------------------------------------------
// stratum_sim
// ai.cpp
//------------------------------------------------------------------------------

#include "ai.hpp"
#include "Site.hpp"
#include <vector>

namespace
{

    //--------------------------------------------------------------------------
    quetzal::stratum_sim::value_type distance2(const quetzal::stratum_sim::point_type& a, const quetzal::stratum_sim::point_type& b)
    {
        auto [dx, dy, dz] = (b - a).rep();
        return dx * dx + dy * dy + dz * dz;
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::stratum_sim::think(World& world, id_type actor_controlled)
{
    // Gathered before anything is written, a component set during iteration being able to move
    // the entity it belongs to
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

        flecs::entity target = world.entities().entity(pursue.target_id);
        const Site* ptarget_site = target.is_alive() ? target.try_get<Site>() : nullptr;
        const placement_type* ptarget = ptarget_site != nullptr ? &ptarget_site->placement : nullptr;

        // Seen now, so where it is replaces where it was
        if (ptarget != nullptr && world.visible(e, *ptarget))
        {
            pursue.position_last_seen = ptarget->position();
            pursue.seen = true;
        }

        // Arrived where it last saw it and found nothing there, so it stops going there
        if (pursue.seen && layout.same_location(*pplacement, placement_type(pursue.position_last_seen, pplacement->direction())))
        {
            pursue.seen = false;
        }

        e.set<Pursue>(pursue);

        if (!pursue.seen)
        {
            continue; // Nowhere to go, so it asks for nothing and waits
        }

        // The reachable step that closes the most distance, so a wall is gone around rather
        // than walked into. Neighbours come back in a fixed order, so ties break the same way
        // on every run
        bool found = false;
        vector_type direction;
        value_type nearest = distance2(pplacement->position(), pursue.position_last_seen);

        for (const placement_type& neighbor : world.neighbors(e))
        {
            value_type d = distance2(neighbor.position(), pursue.position_last_seen);
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
