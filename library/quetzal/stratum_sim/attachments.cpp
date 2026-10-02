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
#include <variant>
#include <cassert>

namespace
{

    //--------------------------------------------------------------------------
    // Access point types are named entities in their own scope, separate from everything else in the world
    std::string access_type_path(std::string_view name)
    {
        return std::format("access_types::{}", name);
    }

    //--------------------------------------------------------------------------
    template<typename Address>
    flecs::entity find_attached_relocator(flecs::entity zone, const Address& address)
    {
        flecs::entity sited_on = quetzal::stratum_sim::layout_entity(zone, address);
        const auto* attached = sited_on ? sited_on.try_get<quetzal::stratum_sim::AttachedEntities>() : nullptr;

        if (attached == nullptr)
        {
            return flecs::entity();
        }

        auto i = std::find_if(attached->entities.begin(), attached->entities.end(),
            [](flecs::entity e) { return e.has<quetzal::stratum_sim::Relocator>(); });
        return i != attached->entities.end() ? *i : flecs::entity();
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
bool quetzal::stratum_sim::targets_relocator_level(flecs::entity e, level_type offset)
{
    const auto* relocator = e.try_get<Relocator>();
    const auto* target = relocator != nullptr ? std::get_if<stratum_format::RelocatorTarget>(&relocator->target) : nullptr;

    if (target == nullptr)
    {
        return false;
    }

    if (const auto* relative = std::get_if<stratum_format::RelativeZone>(&target->zone))
    {
        return relative->offset == offset;
    }

    const auto* absolute = std::get_if<stratum_format::AbsoluteZone>(&target->zone);
    const auto* zone_level = absolute != nullptr ? zone_of(e).try_get<ZoneLevel>() : nullptr;

    return zone_level != nullptr && absolute->level == zone_level->level + offset;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_sim::access_type_name(flecs::entity e)
{
    flecs::entity type = e.target<AccessType>();
    return type ? std::string(type.name().c_str()) : std::string();
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::set_access_point(flecs::entity e, flecs::entity access_type, std::uint64_t order, const placement_type& placement)
{
    assert(access_type);
    assert(e.has<Site>() && e.get<Site>().relation == Relation::On && e.get<Site>().address.has_value());

    e.add<AccessType>(access_type).set<AccessPoint>(AccessPoint {order, placement});
    return;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::create_access_point(flecs::entity zone, flecs::entity access_type, const Site& site,
    std::uint64_t order, const placement_type& placement)
{
    assert(zone && zone.has<Zone>());

    flecs::entity e = zone.world().entity();
    reparent(e, zone, site);
    set_access_point(e, access_type, order, placement);

    return e;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::entry_point(flecs::entity zone, flecs::entity access_type, level_type offset)
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
            if (targets_relocator_level(e, offset) && (!found || access_point.order < order_found))
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
                auto i = std::ranges::find(detection->actor_states, *actor_id, &ActorDetection::actor);
                if (i != detection->actor_states.end())
                {
                    return i->state;
                }
            }

            return detection->state;
        }
    }

    return DetectionState::Visible;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::find_relocator(flecs::entity zone, const WallAddress& address)
{
    return find_attached_relocator(zone, address);
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::find_relocator(flecs::entity zone, const FlatCellAddress& address)
{
    return find_attached_relocator(zone, address);
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::can_enter(flecs::entity e, flecs::entity actor)
{
    std::optional<AccessState> state = access(e);
    return (!state.has_value() || *state == AccessState::Open) && detection(e, actor) != DetectionState::Concealed;
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
