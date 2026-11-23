#if !defined(QUETZAL_STRATUM_SIM_ZONE_HPP)
#define QUETZAL_STRATUM_SIM_ZONE_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Zone.hpp
//------------------------------------------------------------------------------

#include "LayoutBase.hpp"
#include "ProximityIndexBase.hpp"
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
    struct ZoneDocument;
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
    struct Layout
    {
        std::unique_ptr<LayoutBase> layout;
    };

    //--------------------------------------------------------------------------
    // The entities in the zone whose sites have no address, by location, in an index that the zone's layout creates
    struct ProximityIndex
    {
        std::unique_ptr<ProximityIndexBase> proximity_index;
    };

    //--------------------------------------------------------------------------
    // The document the zone was built from, without its entities. A saved zone is written with its geometry, which does not
    // change during play. Shared, so that nlohmann reaches only the files that read or write a document
    struct Document
    {
        std::shared_ptr<const stratum_format::ZoneDocument> document;
    };

    //--------------------------------------------------------------------------
    // Exclusive relationship for a top level entity that is not otherwise part of a zone
    struct InZone
    {
    };

    //--------------------------------------------------------------------------
    // The worldspace that a zone is in
    struct WorldspaceId
    {
        PersistentId worldspace_id;
    };

    //--------------------------------------------------------------------------
    // The region that a zone is in
    struct RegionId
    {
        PersistentId region_id;
    };

    // Adds terms to the builder that match the zone's children and the game's entities that are in the zone
    template<typename Builder>
    decltype(auto) with_in_zone(Builder&& builder, flecs::entity zone);

    //--------------------------------------------------------------------------
    // A room of the zone's layout
    struct RoomId
    {
        id_type room_id = nullid;

        auto operator<=>(const RoomId&) const = default;
    };

    //--------------------------------------------------------------------------
    // The id of a flat, whose entity has the components that apply across the whole flat
    struct FlatId
    {
        id_type flat_id = nullid;

        auto operator<=>(const FlatId&) const = default;
    };

    //--------------------------------------------------------------------------
    // Tag for the entity of a part of a layout: a room, a wall, a flat, a flat's override for a room, or a flat cell. The
    // entity has its key as a component, by which it is found, and has no persistent id
    struct LayoutEntity
    {
    };

    //--------------------------------------------------------------------------
    // The zone's layout entities of one key type
    template<typename T>
    struct EntityIndex
    {
        std::map<T, flecs::entity> entities; // <key, entity>
    };

    //--------------------------------------------------------------------------
    // Calls f with std::type_identity<T> for each key type of the parts of a layout that can have an entity of their own
    template<typename F>
    void for_each_layout_entity(F&& f);

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
void quetzal::stratum_sim::for_each_layout_entity(F&& f)
{
    detail::for_each_type<RoomId, WallId, FlatId, FlatCellPosition>(std::forward<F>(f));
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
