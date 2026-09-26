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
#include <map>
#include <tuple>
#include <cassert>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The entity of what in the zone's layout the address refers to, null where it has none. It is a child of its zone, and
    // a flat's override for a room or a cell is a child of the flat's entity
    template<typename Address>
    flecs::entity layout_entity(flecs::entity zone, const Address& address);

    template<typename Address>
    flecs::entity ensure_layout_entity(flecs::entity zone, const Address& address);

    // Takes an entity being deleted out of its zone's ZoneAddressIndex
    template<typename Address>
    void erase_layout_entity(flecs::entity zone, const Address& address);

    // Whether it has an address of layout_addresses_type
    bool is_layout_entity(flecs::entity e);

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
        // The parent ensure_layout_entity gives an entity
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

    const auto* index = zone.try_get<ZoneAddressIndex>();
    if (index == nullptr)
    {
        return flecs::entity();
    }

    const auto& entities = std::get<std::map<Address, flecs::entity>>(index->entities);
    auto i = entities.find(address);
    return i != entities.end() ? i->second : flecs::entity();
}

//------------------------------------------------------------------------------
template<typename Address>
flecs::entity quetzal::stratum_sim::ensure_layout_entity(flecs::entity zone, const Address& address)
{
    if (flecs::entity e = layout_entity(zone, address))
    {
        return e;
    }

    flecs::entity e = zone.world().entity().set<Address>(address);
    set_parent(e, detail::layout_parent(zone, address));
    std::get<std::map<Address, flecs::entity>>(zone.ensure<ZoneAddressIndex>().entities).emplace(address, e);

    return e;
}

//------------------------------------------------------------------------------
template<typename Address>
void quetzal::stratum_sim::erase_layout_entity(flecs::entity zone, const Address& address)
{
    assert(zone && zone.has<Zone>());

    std::get<std::map<Address, flecs::entity>>(zone.ensure<ZoneAddressIndex>().entities).erase(address);
    return;
}

//------------------------------------------------------------------------------
template<typename Address>
flecs::entity quetzal::stratum_sim::detail::layout_parent(flecs::entity zone, const Address&)
{
    return zone;
}

#endif // QUETZAL_STRATUM_SIM_LAYOUT_ENTITIES_HPP
