//------------------------------------------------------------------------------
// stratum_sim
// AccessPoint.cpp
//------------------------------------------------------------------------------

#include "AccessPoint.hpp"
#include "attachments.hpp"
#include "DiscreteLayout.hpp"
#include "layout_entities.hpp"
#include "World.hpp"
#include <algorithm>
#include <format>
#include <string>
#include <utility>
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
    template<typename T>
    flecs::entity find_attached_access_point(flecs::entity zone, const T& key)
    {
        flecs::entity sited_on = quetzal::stratum_sim::layout_entity(zone, key);
        const auto* attached = sited_on ? sited_on.try_get<quetzal::stratum_sim::AttachedEntities>() : nullptr;

        if (attached == nullptr)
        {
            return flecs::entity();
        }

        auto i = std::find_if(attached->entities.begin(), attached->entities.end(),
            [](flecs::entity e) { return e.has<quetzal::stratum_sim::AccessPoint>(); });
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
std::string quetzal::stratum_sim::access_type_name(flecs::entity e)
{
    flecs::entity type = e.target<AccessType>();
    return type ? std::string(type.name().c_str()) : std::string();
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::targets_level(flecs::entity e, level_type offset)
{
    const auto* access_point = e.try_get<AccessPoint>();
    const auto* target = access_point != nullptr && access_point->target.has_value()
        ? std::get_if<stratum_format::AccessPointDestination>(&*access_point->target) : nullptr;

    if (target == nullptr)
    {
        return false;
    }

    return target->zone_extended_name.has_value() && stratum_format::relative_zone_level(*target->zone_extended_name) == offset;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::accepts_arrival(flecs::entity e, level_type offset)
{
    const auto* access_point = e.try_get<AccessPoint>();
    return access_point != nullptr && (!access_point->target.has_value() || targets_level(e, offset));
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::set_access_point(flecs::entity e, flecs::entity access_type, std::uint64_t order,
    std::optional<AccessPoint::target_type> target)
{
    assert(access_type);
    assert(e.has<Site>() && e.get<Site>().relation == Relation::On && e.get<Site>().address.has_value());

    e.add<AccessType>(access_type).set<AccessPoint>(AccessPoint{order, std::move(target)});
    return;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::create_access_point(flecs::entity zone, flecs::entity access_type, const Site& site, std::uint64_t order,
    std::optional<AccessPoint::target_type> target)
{
    assert(zone && zone.has<Zone>());

    flecs::entity e = zone.world().entity();
    reparent(e, zone, site);
    set_access_point(e, access_type, order, std::move(target));

    return e;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::placement_type quetzal::stratum_sim::arrival_placement(flecs::entity access_point)
{
    assert(access_point.has<AccessPoint>());
    return layout_as<DiscreteLayout>(*zone_of(access_point).get<Layout>().layout).arrival_placement(access_point.get<Site>());
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
            if (accepts_arrival(e, offset) && (!found || access_point.order < order_found))
            {
                found = e;
                order_found = access_point.order;
            }
        });

    return found;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::find_access_point(flecs::entity zone, const WallId& wall)
{
    return find_attached_access_point(zone, wall);
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::find_access_point(flecs::entity zone, const FlatCellPosition& flat_cell)
{
    return find_attached_access_point(zone, flat_cell);
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::can_enter(flecs::entity access_point, flecs::entity actor)
{
    std::optional<AccessState> state = access(access_point);
    return access_point.get<AccessPoint>().target.has_value() && (!state.has_value() || *state == AccessState::Open)
        && detection(access_point, actor) != DetectionState::Concealed;
}
