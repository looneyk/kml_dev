#if !defined(QUETZAL_STRATUM_SIM_ZONE_HPP)
#define QUETZAL_STRATUM_SIM_ZONE_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Zone.hpp
//------------------------------------------------------------------------------

#include "Layout.hpp"
#include "PersistentId.hpp"
#include "PlacementIndex.hpp"
#include "Site.hpp"
#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <flecs.h>
#include <compare>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

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
    // The zone's geometry and topology
    struct ZoneLayout
    {
        std::unique_ptr<Layout> layout;
    };

    //--------------------------------------------------------------------------
    // The entities in the zone whose sites have no address, by location, in an index that the zone's layout creates
    struct ZonePlacementIndex
    {
        std::unique_ptr<PlacementIndex> placement_index;
    };

    //--------------------------------------------------------------------------
    // The document the zone was built from, without its entities. A saved zone is written with its geometry, which does not
    // change during play. Shared, so that nlohmann reaches only the files that read or write a document
    struct ZoneDocument
    {
        std::shared_ptr<const stratum_format::Document> document;
    };

    //--------------------------------------------------------------------------
    // A zone's position in its worldspace, each coordinate signed
    struct ZonePosition
    {
        coord_type x = 0;
        coord_type y = 0;
        coord_type z = 0;

        auto operator<=>(const ZonePosition&) const = default;
    };

    //--------------------------------------------------------------------------
    // Exclusive relationship for a top level entity that is not otherwise part of a zone
    struct InZone
    {
    };

    //--------------------------------------------------------------------------
    // The worldspace that a zone is in
    struct ZoneWorldspace
    {
        PersistentId worldspace_id;
    };

    //--------------------------------------------------------------------------
    // The region that a zone is in
    struct ZoneRegion
    {
        PersistentId region_id;
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
    // The address of a flat's override for one room
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
