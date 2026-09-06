//------------------------------------------------------------------------------
// stratum_sim
// attachments.cpp
//------------------------------------------------------------------------------

#include "attachments.hpp"
#include "DiscreteLayout.hpp"
#include <cassert>

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::create_door(World& world, flecs::entity zone, const std::vector<id_type>& wall_ids)
{
    assert(zone && zone.has<Zone>());
    assert(!wall_ids.empty());

    flecs::entity door = world.entities().entity()
        .child_of(zone)
        .add<Door>()
        .set<Openable>(Openable {false});

    for (id_type wall_id : wall_ids)
    {
        assert(wall_id != nullid);

        world.entities().entity()
            .child_of(door)
            .add<DoorFacing>()
            .set<OnWall>(OnWall {wall_id})
            .set<Attachment>(Attachment {});
    }

    return door;
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::create_doors(World& world, flecs::entity zone, const door_walls_type& doors)
{
    for (const std::vector<id_type>& wall_ids : doors)
    {
        create_door(world, zone, wall_ids);
    }

    return doors.size();
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::facing_on(flecs::entity zone, id_type wall_id)
{
    assert(zone && zone.has<Zone>());

    flecs::entity found;

    // A facing is a child of its door and a door a child of its zone, so the zone two steps up
    // is what tells one zone's wall ids from another's
    zone.world().query_builder<const OnWall>()
        .build()
        .each([&](flecs::entity e, const OnWall& on_wall)
        {
            if (on_wall.wall_id == wall_id && e.parent().parent() == zone)
            {
                found = e;
            }
        });

    return found;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::door_of(flecs::entity facing)
{
    assert(facing);
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
        return false; // A wall with nothing attached is solid
    }

    if (facing.has<Concealed>() && !facing.get<Concealed>().found)
    {
        return false; // A secret door is a wall until it is found
    }

    flecs::entity barrier = door_of(facing);

    if (barrier.has<Openable>())
    {
        return barrier.get<Openable>().open;
    }

    return barrier.has<Passable>();
}
