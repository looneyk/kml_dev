#if !defined(QUETZAL_STRATUM_SIM_ZONE_HPP)
#define QUETZAL_STRATUM_SIM_ZONE_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Zone.hpp
//------------------------------------------------------------------------------

#include "Generation.hpp"
#include "Layout.hpp"
#include "Site.hpp"
#include "types.hpp"
#include "quetzal/common/id.hpp"
#include "quetzal/stratum_format/Notation.hpp"
#include <flecs.h>
#include <compare>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace quetzal::stratum_format
{
    struct Document;
}

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Tag for an independently loadable area
    struct Zone
    {
    };

    //--------------------------------------------------------------------------
    // A group of contiguous and/or interconnected zones
    struct Region
    {
        RegionSpec region_spec;
        std::size_t zone_count = 0; // The number of zones that have been created in this region, loaded or stored
    };

    //--------------------------------------------------------------------------
    // The zone's geometry and topology
    struct ZoneLayout
    {
        std::unique_ptr<Layout> layout;
    };

    //--------------------------------------------------------------------------
    // The document the zone was built from, without its entities. A saved zone is written with its geometry, which does not
    // change during play. Shared, so that nlohmann reaches only the files that read or write a document
    struct ZoneDocument
    {
        std::shared_ptr<const stratum_format::Document> document;
    };

    //--------------------------------------------------------------------------
    // The notation the zone's layout is written in, which a host uses to choose a view
    struct ZoneNotation
    {
        stratum_format::Notation notation = stratum_format::Notation::Block;
    };

    //--------------------------------------------------------------------------
    // The zone's level, a label used when making connections between zones; nothing is looked up by it
    struct ZoneLevel
    {
        level_type level = 0;
    };

    //--------------------------------------------------------------------------
    // Exclusive relationship for a top level entity that is not otherwise part of a zone
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
    // Tag for the entity of a part of a layout: a room, a wall, a flat, a flat's override for a room, or a flat cell. The
    // entity has its address as a component, by which it is found, and has no persistent id
    struct LayoutEntity
    {
    };

    //--------------------------------------------------------------------------
    // The zone's layout entities of one address type
    template<typename Address>
    struct AddressIndex
    {
        std::map<Address, flecs::entity> entities; // <address, entity>
    };

    //--------------------------------------------------------------------------
    // Calls f with std::type_identity<Address> for each address type of the parts of a layout that can have an entity of their own
    template<typename F>
    void for_each_layout_address(F&& f);

    namespace detail
    {
        template<typename... Types, typename F>
        void for_each_type(F&& f);
    }

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
    detail::for_each_type<RoomAddress, WallAddress, FlatAddress, FlatRoomAddress, FlatCellAddress>(std::forward<F>(f));
    return;
}

//------------------------------------------------------------------------------
template<typename... Types, typename F>
void quetzal::stratum_sim::detail::for_each_type(F&& f)
{
    (f(std::type_identity<Types>()), ...);
    return;
}

#endif // QUETZAL_STRATUM_SIM_ZONE_HPP
