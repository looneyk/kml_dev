//------------------------------------------------------------------------------
// stratum_sim
// World.cpp
//------------------------------------------------------------------------------

#include "World.hpp"
#include <algorithm>
#include <cassert>

namespace
{

    // Every placed entity of a zone that satisfies the predicate, in id order
    template<typename F>
    quetzal::stratum_sim::World::entities_type placed_in(flecs::entity zone, F accept)
    {
        quetzal::stratum_sim::World::entities_type entities;

        zone.world().query_builder<const quetzal::stratum_sim::placement_type>()
            .with(flecs::ChildOf, zone)
            .build()
            .each([&](flecs::entity e, const quetzal::stratum_sim::placement_type& placement)
            {
                if (accept(placement))
                {
                    entities.push_back(e);
                }
            });

        std::sort(entities.begin(), entities.end(), [](flecs::entity a, flecs::entity b) { return a.id() < b.id(); });

        return entities;
    }

} // namespace

//------------------------------------------------------------------------------
flecs::world& quetzal::stratum_sim::World::entities()
{
    return m_world;
}

//------------------------------------------------------------------------------
const flecs::world& quetzal::stratum_sim::World::entities() const
{
    return m_world;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create_zone(const std::string& name)
{
    assert(!zone(name)); // Not is_alive: flecs asserts rather than answering for a null entity

    flecs::entity e = m_world.entity(name.c_str()).add<Zone>();
    m_layouts.emplace(e.id(), Layout());

    return e;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::zone(const std::string& name) const
{
    return m_world.lookup(name.c_str());
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::World::zone_count() const
{
    return m_layouts.size();
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Layout& quetzal::stratum_sim::World::layout(flecs::entity zone)
{
    auto i = m_layouts.find(zone.id());
    assert(i != m_layouts.end());
    return i->second;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Layout& quetzal::stratum_sim::World::layout(flecs::entity zone) const
{
    auto i = m_layouts.find(zone.id());
    assert(i != m_layouts.end());
    return i->second;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create_entity(flecs::entity zone, const placement_type& placement)
{
    assert(zone && zone.has<Zone>());
    return m_world.entity().child_of(zone).set<placement_type>(placement);
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::zone_of(flecs::entity e) const
{
    flecs::entity parent = e.parent();
    return parent && parent.has<Zone>() ? parent : flecs::entity();
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::MoveResult quetzal::stratum_sim::World::move(flecs::entity e, const vector_type& direction, value_type distance) const
{
    flecs::entity zone = zone_of(e);
    assert(zone);

    const Layout& zone_layout = layout(zone);

    // The mover is standing where it starts, so it is not something it can be stopped by
    Layout::obstructed_type obstructed = [&](const placement_type& candidate)
    {
        for (flecs::entity other : entities_at(zone, zone_layout.cell_at(candidate.position())))
        {
            if (other != e && other.has<Blocking>())
            {
                return static_cast<id_type>(other.id());
            }
        }

        return nullid;
    };

    return zone_layout.move(e.get<placement_type>(), direction, distance, obstructed);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::World::entities_type quetzal::stratum_sim::World::entities_at(flecs::entity zone, const GridPosition& cell) const
{
    const Layout& zone_layout = layout(zone);

    return placed_in(zone, [&](const placement_type& placement)
    {
        return zone_layout.cell_at(placement.position()) == cell;
    });
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::World::entities_type quetzal::stratum_sim::World::entities_within(flecs::entity zone, const placement_type& placement, value_type range) const
{
    auto [x, y, z] = placement.position().rep();
    value_type range2 = range * range;

    return placed_in(zone, [&](const placement_type& other)
    {
        auto [ox, oy, oz] = other.position().rep();
        value_type dx = ox - x;
        value_type dy = oy - y;
        value_type dz = oz - z;

        return dx * dx + dy * dy + dz * dz <= range2;
    });
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::relocate(flecs::entity e, flecs::entity zone, const placement_type& placement)
{
    assert(zone && zone.has<Zone>());

    e.child_of(zone); // ChildOf is exclusive, so this removes the old parent
    e.set<placement_type>(placement);

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::unload_zone(flecs::entity zone)
{
    assert(zone && zone.has<Zone>());

    m_layouts.erase(zone.id());
    zone.destruct(); // Takes its children with it

    return;
}
