//------------------------------------------------------------------------------
// stratum_sim
// attachments.cpp
//------------------------------------------------------------------------------

#include "attachments.hpp"
#include "DiscreteLayout.hpp"
#include "layout_entities.hpp"
#include "World.hpp"
#include <algorithm>
#include <format>
#include <string>
#include <cassert>

namespace
{

    //--------------------------------------------------------------------------
    // The types are named entities in one scope, apart from everything else in the world
    std::string access_type_path(std::string_view name)
    {
        return std::format("access_types::{}", name);
    }

} // namespace

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::create_access_type(flecs::world& entities, std::string_view name)
{
    assert(!access_type(entities, name));
    return entities.entity(access_type_path(name).c_str());
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::access_type(const flecs::world& entities, std::string_view name)
{
    return entities.lookup(access_type_path(name).c_str());
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::set_access_point(flecs::entity e, flecs::entity access_type, std::uint64_t order, const placement_type& entry)
{
    assert(access_type);
    assert(e.has<Site>() && e.get<Site>().relation == Relation::On && e.get<Site>().address.has_value());

    e.add<AccessType>(access_type).set<AccessPoint>(AccessPoint {order, entry});
    return;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::create_access_point(flecs::entity zone, flecs::entity access_type, const Site& site,
    std::uint64_t order, const placement_type& entry)
{
    assert(zone && zone.has<Zone>());

    flecs::entity e = zone.world().entity();
    reparent(e, zone, site);
    set_access_point(e, access_type, order, entry);

    return e;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::entry_point(flecs::entity zone, flecs::entity access_type)
{
    assert(zone && zone.has<Zone>());
    assert(access_type);

    flecs::entity found;
    std::uint64_t order_found = 0;

    zone.world().query_builder<const AccessPoint>()
        .with(flecs::ChildOf, zone)
        .with<AccessType>(access_type)
        .build()
        .each([&](flecs::entity e, const AccessPoint& access_point)
        {
            if (!found || access_point.order < order_found)
            {
                found = e;
                order_found = access_point.order;
            }
        });

    return found;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::access_entity(flecs::entity e)
{
    for (flecs::entity holder = e; holder && !holder.has<Zone>(); holder = holder.parent())
    {
        if (holder.has<Access>())
        {
            return holder;
        }
    }

    return flecs::entity();
}

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_sim::AccessState> quetzal::stratum_sim::access(flecs::entity e)
{
    flecs::entity holder = access_entity(e);
    return holder ? std::optional<AccessState>(holder.get<Access>().state) : std::nullopt;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::DetectionState quetzal::stratum_sim::detection(flecs::entity e, flecs::entity actor)
{
    for (flecs::entity holder = e; holder && !holder.has<Zone>(); holder = holder.parent())
    {
        if (const auto* detection = holder.try_get<Detection>())
        {
            if (const auto* actor_id = actor ? actor.try_get<PersistentId>() : nullptr)
            {
                if (auto i = detection->actor_states.find(*actor_id); i != detection->actor_states.end())
                {
                    return i->second;
                }
            }

            return detection->state;
        }
    }

    return DetectionState::Visible;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::way_through(flecs::entity zone, id_type wall_id)
{
    flecs::entity wall = wall_entity(zone, wall_id);
    const auto* attached = wall ? wall.try_get<AttachedEntities>() : nullptr;

    if (attached == nullptr)
    {
        return flecs::entity();
    }

    auto i = std::find_if(attached->entities.begin(), attached->entities.end(), [](flecs::entity e) { return static_cast<bool>(access_entity(e)); });
    return i != attached->entities.end() ? *i : flecs::entity();
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::crossable(flecs::entity zone, id_type wall_id, flecs::entity actor)
{
    flecs::entity wall = wall_entity(zone, wall_id);
    const auto* attached = wall ? wall.try_get<AttachedEntities>() : nullptr;

    if (attached == nullptr)
    {
        return false;
    }

    return std::any_of(attached->entities.begin(), attached->entities.end(), [&](flecs::entity e)
    {
        return access(e) == AccessState::Open && detection(e, actor) != DetectionState::Concealed;
    });
}
