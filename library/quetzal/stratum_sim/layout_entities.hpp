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
    // Returns the entity of the layout part that the key refers to, or a null entity if the part has none. The entity is
    // a child of its zone, except that a flat's override for a room or a cell is a child of the flat's entity
    template<typename T>
    flecs::entity layout_entity(flecs::entity zone, const T& key);

    template<typename T>
    flecs::entity ensure_layout_entity(flecs::entity zone, const T& key);

    // Removes an entity that is being deleted from the EntityIndex of its zone or of its parent
    template<typename T>
    void erase_layout_entity(flecs::entity zone, flecs::entity e, const T& key);

    //--------------------------------------------------------------------------
    flecs::entity room_entity(flecs::entity zone, id_type room_id);
    flecs::entity wall_entity(flecs::entity zone, id_type wall_id);
    flecs::entity flat_entity(flecs::entity zone, id_type flat_id);
    flecs::entity flat_room_entity(flecs::entity zone, id_type flat_id, id_type room_id);
    flecs::entity flat_cell_entity(flecs::entity zone, id_type flat_id, const discrete_point2_type& cell_position);

    flecs::entity ensure_room_entity(flecs::entity zone, id_type room_id);
    flecs::entity ensure_wall_entity(flecs::entity zone, id_type wall_id);
    flecs::entity ensure_flat_entity(flecs::entity zone, id_type flat_id);
    flecs::entity ensure_flat_room_entity(flecs::entity zone, id_type flat_id, id_type room_id);
    flecs::entity ensure_flat_cell_entity(flecs::entity zone, id_type flat_id, const discrete_point2_type& cell_position);

    namespace detail
    {
        // The parent that ensure_layout_entity assigns to a new entity
        template<typename T>
        flecs::entity layout_parent(flecs::entity zone, const T& key);

        flecs::entity layout_parent(flecs::entity zone, const FlatCellPosition& key);

        // The container is the entity whose EntityIndex lists the layout entity: its zone, or a flat for a room override
        template<typename T>
        flecs::entity indexed_entity(flecs::entity container, const T& key);

        template<typename T>
        flecs::entity create_layout_entity(flecs::entity container, flecs::entity parent, const T& key);

        template<typename T>
        void erase_indexed_entity(flecs::entity container, flecs::entity e, const T& key);
    }

} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
template<typename T>
flecs::entity quetzal::stratum_sim::layout_entity(flecs::entity zone, const T& key)
{
    assert(zone && zone.has<Zone>());
    return detail::indexed_entity(zone, key);
}

//------------------------------------------------------------------------------
template<typename T>
flecs::entity quetzal::stratum_sim::ensure_layout_entity(flecs::entity zone, const T& key)
{
    if (flecs::entity e = layout_entity(zone, key))
    {
        return e;
    }

    return detail::create_layout_entity(zone, detail::layout_parent(zone, key), key);
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::stratum_sim::erase_layout_entity(flecs::entity zone, flecs::entity e, const T& key)
{
    assert(zone && zone.has<Zone>());

    detail::erase_indexed_entity(zone, e, key);

    if (flecs::entity parent = e.parent(); parent && parent != zone)
    {
        detail::erase_indexed_entity(parent, e, key);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename T>
flecs::entity quetzal::stratum_sim::detail::layout_parent(flecs::entity zone, const T&)
{
    return zone;
}

//------------------------------------------------------------------------------
template<typename T>
flecs::entity quetzal::stratum_sim::detail::indexed_entity(flecs::entity container, const T& key)
{
    const auto* index = container.try_get<EntityIndex<T>>();
    if (index == nullptr)
    {
        return flecs::entity();
    }

    auto i = index->entities.find(key);
    return i != index->entities.end() ? i->second : flecs::entity();
}

//------------------------------------------------------------------------------
template<typename T>
flecs::entity quetzal::stratum_sim::detail::create_layout_entity(flecs::entity container, flecs::entity parent, const T& key)
{
    flecs::entity e = container.world().entity().add<LayoutEntity>().set<T>(key);
    set_parent(e, parent);
    container.ensure<EntityIndex<T>>().entities.emplace(key, world_handle(e));

    return e;
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::stratum_sim::detail::erase_indexed_entity(flecs::entity container, flecs::entity e, const T& key)
{
    auto* index = container.try_get_mut<EntityIndex<T>>();
    if (index == nullptr)
    {
        return;
    }

    if (auto i = index->entities.find(key); i != index->entities.end() && i->second == e)
    {
        index->entities.erase(i);
    }

    return;
}

#endif // QUETZAL_STRATUM_SIM_LAYOUT_ENTITIES_HPP
