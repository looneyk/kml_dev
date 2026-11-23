//------------------------------------------------------------------------------
// stratum_sim
// access_points.cpp
//------------------------------------------------------------------------------

#include "access_points.hpp"
#include "attachments.hpp"
#include "DiscreteLayout.hpp"
#include "file_components.hpp"
#include "layout_entities.hpp"
#include "World.hpp"
#include <algorithm>
#include <utility>
#include <variant>
#include <cassert>

namespace
{

    //--------------------------------------------------------------------------
    template<typename T>
    flecs::entity find_attached_access_point(flecs::entity zone, const T& key)
    {
        flecs::entity e = quetzal::stratum_sim::layout_entity(zone, key);
        const auto* attached = e ? e.try_get<quetzal::stratum_sim::AttachedEntities>() : nullptr;

        if (attached == nullptr)
        {
            return flecs::entity();
        }

        auto i = std::find_if(attached->entities.begin(), attached->entities.end(),
            [](flecs::entity e) { return e.has<quetzal::stratum_core::AccessPoint>(); });
        return i != attached->entities.end() ? *i : flecs::entity();
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::stratum_sim::register_access_point_components(flecs::world& entities)
{
    entities.component<stratum_core::Directionality>();
    register_file_component<stratum_core::Directionality>(entities, "directionality");

    return;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::targets_level(flecs::entity e, level_type offset)
{
    const auto* access_point = e.try_get<stratum_core::AccessPoint>();
    const auto* zone_name = access_point != nullptr && access_point->destination.has_value()
        ? std::get_if<stratum_core::ZoneExtendedName>(&*access_point->destination) : nullptr;

    return zone_name != nullptr && stratum_core::relative_zone_offset(zone_name->value) == offset;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::accepts_arrival(flecs::entity e, level_type offset)
{
    const auto* access_point = e.try_get<stratum_core::AccessPoint>();
    return access_point != nullptr && (!access_point->destination.has_value() || targets_level(e, offset));
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::set_access_point(flecs::entity e, const stratum_core::AccessPoint& access_point)
{
    assert(e.target(flecs::IsA));
    assert(e.has<Site>() && e.get<Site>().relation == Relation::On && e.get<Site>().address.has_value());

    e.set<stratum_core::AccessPoint>(access_point);
    return;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::create_access_point(flecs::entity zone, flecs::entity type, const Site& site,
    const stratum_core::AccessPoint& access_point)
{
    assert(zone && zone.has<Zone>());
    assert(type);

    flecs::entity e = zone.world().entity().is_a(type);
    reparent(e, zone, site);
    set_access_point(e, access_point);

    return e;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::placement_type quetzal::stratum_sim::destination_placement(flecs::entity access_point)
{
    assert(access_point.has<stratum_core::AccessPoint>());
    return layout_as<DiscreteLayout>(*zone_of(access_point).get<Layout>().layout).destination_placement(access_point.get<Site>(),
        access_point.get<stratum_core::AccessPoint>().egress);
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::entry_point(flecs::entity zone, flecs::entity type, level_type offset)
{
    assert(zone && zone.has<Zone>());
    assert(type);

    flecs::entity found;
    std::uint64_t order_found = 0;

    zone.world().query_builder<const stratum_core::AccessPoint>()
        .with(flecs::ChildOf, zone)
        .with(flecs::IsA, type)
        .build()
        .each([&](flecs::entity e, const stratum_core::AccessPoint& access_point)
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
flecs::entity quetzal::stratum_sim::find_entered_access_point(flecs::entity zone, const WallId& wall)
{
    if (flecs::entity on_wall = find_access_point(zone, wall))
    {
        return on_wall;
    }

    const DiscreteLayout& layout = layout_as<DiscreteLayout>(*zone.get<Layout>().layout);
    const discrete_vector_type& toward_wall = layout.wall_direction(wall.wall_id);
    const discrete_point_type beyond = layout.wall_cell_position(wall.wall_id) + toward_wall;

    if (!layout.grid().contains(beyond))
    {
        return flecs::entity();
    }

    flecs::entity beyond_wall = find_access_point(zone, FlatCellPosition{layout.floor_id(), beyond});
    return beyond_wall && beyond_wall.get<stratum_core::AccessPoint>().egress == -toward_wall ? beyond_wall : flecs::entity();
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::can_enter(flecs::entity access_point, flecs::entity actor)
{
    std::optional<AccessState> state = access(access_point);
    return access_point.get<stratum_core::AccessPoint>().destination.has_value() && (!state.has_value() || *state == AccessState::Open)
        && detection(access_point, actor) != DetectionState::Concealed;
}
