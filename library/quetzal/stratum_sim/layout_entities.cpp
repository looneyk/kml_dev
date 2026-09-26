//------------------------------------------------------------------------------
// stratum_sim
// layout_entities.cpp
//------------------------------------------------------------------------------

#include "layout_entities.hpp"
#include "DiscreteLayout.hpp"
#include <type_traits>

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::is_layout_entity(flecs::entity e)
{
    bool found = false;
    for_each_layout_address([&]<typename Address>(std::type_identity<Address>) { found = found || e.has<Address>(); });

    return found;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::room_entity(flecs::entity zone, id_type room_id)
{
    return layout_entity(zone, RoomAddress {room_id});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::wall_entity(flecs::entity zone, id_type wall_id)
{
    return layout_entity(zone, WallAddress {wall_id});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::flat_entity(flecs::entity zone, id_type flat_id)
{
    return layout_entity(zone, FlatAddress {flat_id});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::flat_room_entity(flecs::entity zone, id_type flat_id, id_type room_id)
{
    return layout_entity(zone, FlatRoomAddress {flat_id, room_id});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::flat_cell_entity(flecs::entity zone, id_type flat_id, const discrete_point_type& cell_position)
{
    return layout_entity(zone, FlatCellAddress {flat_id, cell_position});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::ensure_room_entity(flecs::entity zone, id_type room_id)
{
    assert(room_id < layout_as<DiscreteLayout>(*zone.get<ZoneLayout>().layout).room_count());
    return ensure_layout_entity(zone, RoomAddress {room_id});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::ensure_wall_entity(flecs::entity zone, id_type wall_id)
{
    assert(wall_id != nullid);
    return ensure_layout_entity(zone, WallAddress {wall_id});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::ensure_flat_entity(flecs::entity zone, id_type flat_id)
{
    assert(flat_id != nullid);
    return ensure_layout_entity(zone, FlatAddress {flat_id});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::ensure_flat_room_entity(flecs::entity zone, id_type flat_id, id_type room_id)
{
    assert(flat_id != nullid);
    assert(room_id < layout_as<DiscreteLayout>(*zone.get<ZoneLayout>().layout).room_count());
    return ensure_layout_entity(zone, FlatRoomAddress {flat_id, room_id});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::ensure_flat_cell_entity(flecs::entity zone, id_type flat_id, const discrete_point_type& cell_position)
{
    assert(flat_id != nullid);
    return ensure_layout_entity(zone, FlatCellAddress {flat_id, cell_position});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::detail::layout_parent(flecs::entity zone, const FlatRoomAddress& address)
{
    return ensure_layout_entity(zone, FlatAddress {address.flat_id});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::detail::layout_parent(flecs::entity zone, const FlatCellAddress& address)
{
    return ensure_layout_entity(zone, FlatAddress {address.flat_id});
}
