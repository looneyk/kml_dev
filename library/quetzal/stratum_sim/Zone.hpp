#if !defined(QUETZAL_STRATUM_SIM_ZONE_HPP)
#define QUETZAL_STRATUM_SIM_ZONE_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Zone.hpp
//------------------------------------------------------------------------------

#include "Layout.hpp"
#include "Site.hpp"
#include "quetzal/common/id.hpp"
#include "quetzal/stratum_format/Notation.hpp"
#include <flecs.h>
#include <compare>
#include <map>
#include <memory>
#include <tuple>
#include <type_traits>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    struct Zone // Tag, an independently loadable area
    {
    };

    //--------------------------------------------------------------------------
    // The zone's geometry and topology
    struct ZoneLayout
    {
        std::unique_ptr<Layout> layout;
    };

    //--------------------------------------------------------------------------
    // The notation the zone's layout is written in, which a host uses to choose a view
    struct ZoneNotation
    {
        stratum_format::Notation notation = stratum_format::Notation::Block;
    };

    //--------------------------------------------------------------------------
    // Exclusive relationship from an entity owned by the game, which has no parent, to the zone it is in. Flecs removes it
    // when the zone is deleted
    struct InZone
    {
    };

    // Adds terms to the builder that match the zone's children and the game's entities that are in the zone
    template<typename Builder>
    decltype(auto) with_in_zone(Builder&& builder, flecs::entity zone);

    //--------------------------------------------------------------------------
    // A room of the zone's layout
    struct RoomAddress
    {
        id_type room_id = nullid;

        auto operator<=>(const RoomAddress&) const = default;
    };

    //--------------------------------------------------------------------------
    // The address of a flat, whose entity has the components that apply across the whole flat
    struct FlatAddress
    {
        id_type flat_id = nullid;

        auto operator<=>(const FlatAddress&) const = default;
    };

    //--------------------------------------------------------------------------
    // The address of a flat's override for one room, as a flat cell address is the flat's override for one cell
    struct FlatRoomAddress
    {
        id_type flat_id = nullid;
        id_type room_id = nullid;

        auto operator<=>(const FlatRoomAddress&) const = default;
    };

    //--------------------------------------------------------------------------
    // The address types of the parts of a layout that can have an entity of their own. The entity has its address as a
    // component, by which it is found, and has no persistent id
    using layout_addresses_type = std::tuple<RoomAddress, WallAddress, FlatAddress, FlatRoomAddress, FlatCellAddress>;

    // Calls f with std::type_identity<Address> for each Address of layout_addresses_type
    template<typename F>
    void for_each_layout_address(F&& f);

    namespace detail
    {
        template<typename Addresses>
        struct address_entities;

        template<typename... Addresses>
        struct address_entities<std::tuple<Addresses...>>
        {
            using type = std::tuple<std::map<Addresses, flecs::entity>...>;
        };
    }

    //--------------------------------------------------------------------------
    // The zone's layout entities, by address
    struct ZoneAddressIndex
    {
        detail::address_entities<layout_addresses_type>::type entities; // <address, entity>, one map for each address type
    };

} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
template<typename Builder>
decltype(auto) quetzal::stratum_sim::with_in_zone(Builder&& builder, flecs::entity zone)
{
    return builder.with(flecs::ChildOf, zone).or_().template with<InZone>(zone);
}

//------------------------------------------------------------------------------
template<typename F>
void quetzal::stratum_sim::for_each_layout_address(F&& f)
{
    [&]<typename... Addresses>(std::type_identity<std::tuple<Addresses...>>)
    {
        (f(std::type_identity<Addresses>()), ...);
    }(std::type_identity<layout_addresses_type>());

    return;
}

#endif // QUETZAL_STRATUM_SIM_ZONE_HPP
