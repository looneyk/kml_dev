#if !defined(QUETZAL_STRATUM_SIM_ATTACHMENTS_HPP)
#define QUETZAL_STRATUM_SIM_ATTACHMENTS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// attachments.hpp
//------------------------------------------------------------------------------

#include "PersistentId.hpp"
#include "Site.hpp"
#include "types.hpp"
#include "quetzal/common/id.hpp"
#include "quetzal/stratum_format/RelocatorTarget.hpp"
#include <flecs.h>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace quetzal::stratum_sim
{

    // Entities attached to a wall or a flat: an alcove, a basin, a door, a button, a torch

    //--------------------------------------------------------------------------
    enum class AccessState
    {
        Open,
        Closed,
        Locked
    };

    //--------------------------------------------------------------------------
    // The state of an entity that is a way through, such as a door. A door off its hinges is open, and one smashed into
    // rubble that blocks the doorway is closed. It is on the door rather than a door facing, so both sides
    // agree
    struct Access
    {
        AccessState state = AccessState::Closed;
    };

    //--------------------------------------------------------------------------
    enum class DetectionState
    {
        Visible,
        Concealed,
        Revealed
    };

    //--------------------------------------------------------------------------
    // One actor's own detection state for an entity
    struct ActorDetection
    {
        PersistentId actor;
        DetectionState state = DetectionState::Visible;
    };

    //--------------------------------------------------------------------------
    // How an entity that not every actor sees, such as one side of a secret door, is detected: its general state, and the
    // state for each actor whose own state differs, at most one entry per actor
    struct Detection
    {
        DetectionState state = DetectionState::Visible;
        std::vector<ActorDetection> actor_states;
    };

    void register_attachment_components(flecs::world& entities);

    //--------------------------------------------------------------------------
    // Relationship from an access point to its type, which is an entity registered once in the world: staircase, door,
    // portal, waypoint, or a type that a mod adds
    struct AccessType
    {
    };

    //--------------------------------------------------------------------------
    // The access point type at which an actor arrives through an access point of this type
    struct Arrival
    {
        flecs::entity access_type;
    };

    //--------------------------------------------------------------------------
    // An entity at which a zone is entered or left, sited on a wall, the floor, or the ceiling. The order counts from 0
    // among the zone's access points of the same type. The target is where going through it moves an actor: unbound, or
    // the access point it is bound to. An access point with no target is arrival only
    struct AccessPoint
    {
        using target_type = std::variant<stratum_format::RelocatorTarget, EntityReference>;

        std::uint64_t order = 0;
        std::optional<target_type> target;
    };

    //--------------------------------------------------------------------------
    // Registers an access point type. Its Arrival is set once the type it arrives at is registered
    flecs::entity create_access_type(flecs::world& entities, std::string_view name);

    // Returns the registered access point type, or a null entity if no type has that name
    flecs::entity access_type(const flecs::world& entities, std::string_view name);

    // Returns the name of the entity's access point type, or an empty string if the entity is not an access point
    std::string access_type_name(flecs::entity e);

    //--------------------------------------------------------------------------
    // Returns true if the entity's access point target is unbound and is the level offset levels from the entity's zone,
    // whether the target is written relative or absolute. An absolute target matches only in a zone that has a level
    bool targets_level(flecs::entity e, level_type offset);

    // Returns true if an actor coming from the level offset levels from the entity's zone can arrive at the entity's access
    // point: it has no target, or its target leads back to that level
    bool accepts_arrival(flecs::entity e, level_type offset);

    //--------------------------------------------------------------------------
    // Makes an entity sited on a surface an access point of the type
    void set_access_point(flecs::entity e, flecs::entity access_type, std::uint64_t order,
        std::optional<AccessPoint::target_type> target = std::nullopt);

    // Creates an entity in the zone at the site and makes it an access point
    flecs::entity create_access_point(flecs::entity zone, flecs::entity access_type, const Site& site, std::uint64_t order,
        std::optional<AccessPoint::target_type> target = std::nullopt);

    // Returns the placement of an actor arriving at the access point, computed from its site
    placement_type arrival_placement(flecs::entity access_point);

    // Returns the zone's lowest-ordered access point of the type that accepts an arrival from the level offset levels from
    // the zone, or a null entity if the zone has none. An actor arriving from another level passes the offset that leads
    // back
    flecs::entity entry_point(flecs::entity zone, flecs::entity access_type, level_type offset);

    //--------------------------------------------------------------------------
    // Returns the entity whose Access applies to e: e itself, or its nearest ancestor that has an Access. Returns a null
    // entity if none has one
    flecs::entity access_entity(flecs::entity e);

    std::optional<AccessState> access(flecs::entity e);

    // Returns how the actor detects e, from the Detection on e or on its nearest ancestor that has one: the actor's own state
    // if it has one, and otherwise the general state. Returns Visible if none has a Detection. A null actor gets the general
    // state
    DetectionState detection(flecs::entity e, flecs::entity actor);

    //--------------------------------------------------------------------------
    // Returns an access point sited on the wall or the flat cell, or a null entity if there is none
    flecs::entity find_access_point(flecs::entity zone, const WallAddress& address);
    flecs::entity find_access_point(flecs::entity zone, const FlatCellAddress& address);

    // Returns true if the actor can go through the access point: it has a target, its access, if it has any, is open, and
    // the actor does not find it concealed
    bool can_enter(flecs::entity access_point, flecs::entity actor);

    //--------------------------------------------------------------------------
    // Returns an entity sited on the wall that has access, such as a door facing, or a null entity if there is none
    flecs::entity way_through(flecs::entity zone, id_type wall_id);

    // Returns true if an entity sited on the wall lets the actor through: its access is open, and the actor does not find
    // it concealed. A wall with no such entity cannot be crossed
    bool crossable(flecs::entity zone, id_type wall_id, flecs::entity actor);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_ATTACHMENTS_HPP
