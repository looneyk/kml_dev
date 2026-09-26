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
    // On a zone: its geometry and topology
    struct ZoneLayout
    {
        std::unique_ptr<Layout> layout;
    };

    //--------------------------------------------------------------------------
    // On a zone: the notation its layout is written in, from which a host chooses a view
    struct ZoneNotation
    {
        stratum_format::Notation notation = stratum_format::Notation::Block;
    };

    //--------------------------------------------------------------------------
    // A room of the zone's layout
    struct RoomAddress
    {
        id_type room_id = nullid;

        auto operator<=>(const RoomAddress&) const = default;
    };

    //--------------------------------------------------------------------------
    // A flat, where what applies across it is
    struct FlatAddress
    {
        id_type flat_id = nullid;

        auto operator<=>(const FlatAddress&) const = default;
    };

    //--------------------------------------------------------------------------
    // What a flat states for one room, as a flat cell address is what it states for one cell
    struct FlatRoomAddress
    {
        id_type flat_id = nullid;
        id_type room_id = nullid;

        auto operator<=>(const FlatRoomAddress&) const = default;
    };

    //--------------------------------------------------------------------------
    // What in a zone's layout can have an entity of its own. The entity has its address as a component, which is how it is
    // found, and no persistent id
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
    // On a zone: the entities of what in its layout has one
    struct ZoneAddressIndex
    {
        detail::address_entities<layout_addresses_type>::type entities; // <address, entity>, one map for each address type
    };

} // namespace quetzal::stratum_sim

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
