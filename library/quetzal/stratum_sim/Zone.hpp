#if !defined(QUETZAL_STRATUM_SIM_ZONE_HPP)
#define QUETZAL_STRATUM_SIM_ZONE_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Zone.hpp
//------------------------------------------------------------------------------

#include "Generation.hpp"
#include "Layout.hpp"
#include "PersistentId.hpp"
#include "PlacementIndex.hpp"
#include "Site.hpp"
#include "types.hpp"
#include "quetzal/common/id.hpp"
#include "quetzal/math/Interval.hpp"
#include "quetzal/stratum_format/Notation.hpp"
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
    // A group of contiguous and/or interconnected zones of one worldspace
    struct Region
    {
        std::string name;
        RegionSpec region_spec;
        std::vector<PersistentId> zone_ids; // The zones created in this region, loaded or saved, in the order they were created
        std::map<std::string, PersistentId> named_zone_ids; // <name, zone_id>, loaded or saved
        std::size_t name_counter = 0; // The last suffix given to a name of its zones that was taken
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
    // The notation the zone's layout is written in, which a host uses to choose a view
    struct ZoneNotation
    {
        stratum_format::Notation notation = stratum_format::Notation::Block;
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
    // A box of zone positions, each interval inclusive, and unbounded on an axis with no interval
    struct ZoneBox
    {
        std::optional<math::Interval<coord_type>> x;
        std::optional<math::Interval<coord_type>> y;
        std::optional<math::Interval<coord_type>> z;
    };

    //--------------------------------------------------------------------------
    // The coordinate system of the zones that are its children. Its top-level region is a child too
    struct Worldspace
    {
        std::string name;
        ZoneBox extent;
        std::optional<discrete_vector_type> zone_size; // A grid worldspace's
        std::map<ZonePosition, PersistentId> zone_ids; // <position, zone_id>, loaded or saved
    };

    //--------------------------------------------------------------------------
    // The position a region's seed derives from, until seeds are drawn hierarchically
    struct RegionPosition
    {
        coord_type x = 0;
        coord_type y = 0;
    };

    //--------------------------------------------------------------------------
    // Exclusive relationship for a top level entity that is not otherwise part of a zone
    struct InZone
    {
    };

    //--------------------------------------------------------------------------
    // Exclusive relationship from a zone to its region
    struct InRegion
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
