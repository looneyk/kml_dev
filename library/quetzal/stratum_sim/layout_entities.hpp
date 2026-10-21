#if !defined(QUETZAL_STRATUM_SIM_LAYOUT_ENTITIES_HPP)
#define QUETZAL_STRATUM_SIM_LAYOUT_ENTITIES_HPP
//------------------------------------------------------------------------------
// stratum_sim
// layout_entities.hpp
//------------------------------------------------------------------------------

#include "World.hpp"
#include "Zone.hpp"
#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <flecs.h>
#include <cassert>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Returns the entity of the layout part that the address refers to, or a null entity if the part has none. The entity is
    // a child of its zone, except that a flat's override for a room or a cell is a child of the flat's entity
    template<typename Address>
    flecs::entity layout_entity(flecs::entity zone, const Address& address);

    template<typename Address>
    flecs::entity ensure_layout_entity(flecs::entity zone, const Address& address);

    // Removes an entity that is being deleted from its zone's AddressIndex
    template<typename Address>
    void erase_layout_entity(flecs::entity zone, const Address& address);

    //--------------------------------------------------------------------------
    flecs::entity room_entity(flecs::entity zone, id_type room_id);
    flecs::entity wall_entity(flecs::entity zone, id_type wall_id);
    flecs::entity flat_entity(flecs::entity zone, id_type flat_id);
    flecs::entity flat_room_entity(flecs::entity zone, id_type flat_id, id_type room_id);
    flecs::entity flat_cell_entity(flecs::entity zone, id_type flat_id, const discrete_point_type& cell_position);

    flecs::entity ensure_room_entity(flecs::entity zone, id_type room_id);
    flecs::entity ensure_wall_entity(flecs::entity zone, id_type wall_id);
    flecs::entity ensure_flat_entity(flecs::entity zone, id_type flat_id);
    flecs::entity ensure_flat_room_entity(flecs::entity zone, id_type flat_id, id_type room_id);
    flecs::entity ensure_flat_cell_entity(flecs::entity zone, id_type flat_id, const discrete_point_type& cell_position);

    namespace detail
    {
        // The parent that ensure_layout_entity assigns to a new entity
        template<typename Address>
        flecs::entity layout_parent(flecs::entity zone, const Address& address);

        flecs::entity layout_parent(flecs::entity zone, const FlatRoomAddress& address);
        flecs::entity layout_parent(flecs::entity zone, const FlatCellAddress& address);
    }

} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
template<typename Address>
flecs::entity quetzal::stratum_sim::layout_entity(flecs::entity zone, const Address& address)
{
    assert(zone && zone.has<Zone>());

    const auto* index = zone.try_get<AddressIndex<Address>>();
    if (index == nullptr)
    {
        return flecs::entity();
    }

    auto i = index->entities.find(address);
    return i != index->entities.end() ? i->second : flecs::entity();
}

//------------------------------------------------------------------------------
template<typename Address>
flecs::entity quetzal::stratum_sim::ensure_layout_entity(flecs::entity zone, const Address& address)
{
    if (flecs::entity e = layout_entity(zone, address))
    {
        return e;
    }

    flecs::entity e = zone.world().entity().add<LayoutEntity>().set<Address>(address);
    set_parent(e, detail::layout_parent(zone, address));
    zone.ensure<AddressIndex<Address>>().entities.emplace(address, e);

    return e;
}

//------------------------------------------------------------------------------
template<typename Address>
void quetzal::stratum_sim::erase_layout_entity(flecs::entity zone, const Address& address)
{
    assert(zone && zone.has<Zone>());

    zone.ensure<AddressIndex<Address>>().entities.erase(address);
    return;
}

//------------------------------------------------------------------------------
template<typename Address>
flecs::entity quetzal::stratum_sim::detail::layout_parent(flecs::entity zone, const Address&)
{
    return zone;
}

#endif // QUETZAL_STRATUM_SIM_LAYOUT_ENTITIES_HPP
