//------------------------------------------------------------------------------
// stratum_sim
// attachments.cpp
//------------------------------------------------------------------------------

#include "attachments.hpp"
#include "DiscreteLayout.hpp"
#include "World.hpp"
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
flecs::entity quetzal::stratum_sim::create_access_point(flecs::entity zone, flecs::entity access_type, const Site& site,
    std::uint64_t order, const placement_type& entry)
{
    assert(zone && zone.has<Zone>());
    assert(access_type);
    assert(site.relation == Relation::On && site.address.has_value());

    return zone.world().entity()
        .child_of(zone)
        .set<Site>(site)
        .add<AccessType>(access_type)
        .set<AccessPoint>(AccessPoint {order, entry});
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
flecs::entity quetzal::stratum_sim::create_door(flecs::entity zone, const std::vector<id_type>& wall_ids)
{
    assert(zone && zone.has<Zone>());
    assert(!wall_ids.empty());

    flecs::entity door = zone.world().entity()
        .child_of(zone)
        .add<Door>()
        .set<Openable>(Openable {false});

    for (id_type wall_id : wall_ids)
    {
        assert(wall_id != nullid);

        zone.world().entity()
            .child_of(door)
            .add<DoorFacing>()
            .set<Site>(Site {Relation::On, WallAddress {wall_id}, {}});
    }

    return door;
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::create_doors(flecs::entity zone, const door_walls_type& doors)
{
    for (const std::vector<id_type>& wall_ids : doors)
    {
        create_door(zone, wall_ids);
    }

    return doors.size();
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::facing_on(flecs::entity zone, id_type wall_id)
{
    assert(zone && zone.has<Zone>());

    flecs::entity found;

    // Wall ids are unique only within a zone
    zone.world().query_builder<const Site>()
        .with<DoorFacing>()
        .build()
        .each([&](flecs::entity e, const Site& site)
        {
            if (wall_id_of(site) == wall_id && zone_of(e) == zone)
            {
                found = e;
            }
        });

    return found;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::door_of(flecs::entity facing)
{
    assert(facing && facing.has<DoorFacing>());
    return facing.parent();
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::other_facing(flecs::entity facing)
{
    assert(facing);

    flecs::entity other;

    door_of(facing).children([&](flecs::entity e)
    {
        if (e != facing && e.has<DoorFacing>())
        {
            other = e;
        }
    });

    return other;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::crossable(flecs::entity zone, id_type wall_id)
{
    flecs::entity facing = facing_on(zone, wall_id);

    if (!facing)
    {
        return false;
    }

    if (facing.has<Concealed>() && !facing.get<Concealed>().revealed)
    {
        return false;
    }

    flecs::entity door = door_of(facing);
    if (door.has<Openable>())
    {
        return door.get<Openable>().open;
    }

    return door.has<Crossable>();
}
