//------------------------------------------------------------------------------
// stratum_sim
// layout_entities.cpp
//------------------------------------------------------------------------------

#include "layout_entities.hpp"
#include "DiscreteLayout.hpp"

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::room_entity(flecs::entity zone, id_type room_id)
{
    return layout_entity(zone, RoomId{room_id});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::wall_entity(flecs::entity zone, id_type wall_id)
{
    return layout_entity(zone, WallId{wall_id});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::flat_entity(flecs::entity zone, id_type flat_id)
{
    return layout_entity(zone, FlatId{flat_id});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::flat_room_entity(flecs::entity zone, id_type flat_id, id_type room_id)
{
    flecs::entity flat = flat_entity(zone, flat_id);
    return flat ? detail::indexed_entity(flat, RoomId{room_id}) : flecs::entity();
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::flat_cell_entity(flecs::entity zone, id_type flat_id, const discrete_point_type& cell_position)
{
    return layout_entity(zone, FlatCellPosition{flat_id, cell_position});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::ensure_room_entity(flecs::entity zone, id_type room_id)
{
    assert(room_id < layout_as<DiscreteLayout>(*zone.get<Layout>().layout).room_count());
    return ensure_layout_entity(zone, RoomId{room_id});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::ensure_wall_entity(flecs::entity zone, id_type wall_id)
{
    assert(wall_id != nullid);
    return ensure_layout_entity(zone, WallId{wall_id});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::ensure_flat_entity(flecs::entity zone, id_type flat_id)
{
    assert(flat_id != nullid);
    return ensure_layout_entity(zone, FlatId{flat_id});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::ensure_flat_room_entity(flecs::entity zone, id_type flat_id, id_type room_id)
{
    assert(flat_id != nullid);
    assert(room_id < layout_as<DiscreteLayout>(*zone.get<Layout>().layout).room_count());

    flecs::entity flat = ensure_flat_entity(zone, flat_id);
    if (flecs::entity e = detail::indexed_entity(flat, RoomId{room_id}))
    {
        return e;
    }

    return detail::create_layout_entity(flat, flat, RoomId{room_id});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::ensure_flat_cell_entity(flecs::entity zone, id_type flat_id, const discrete_point_type& cell_position)
{
    assert(flat_id != nullid);
    return ensure_layout_entity(zone, FlatCellPosition{flat_id, cell_position});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::detail::layout_parent(flecs::entity zone, const FlatCellPosition& flat_cell)
{
    return ensure_layout_entity(zone, FlatId{flat_cell.flat_id});
}
