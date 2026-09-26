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
#include <flecs.h>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>

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
    // How an entity that not every actor sees, such as one side of a secret door, is detected: its general state, and the
    // state for each actor whose own state differs
    struct Detection
    {
        DetectionState state = DetectionState::Visible;
        std::unordered_map<PersistentId, DetectionState> actor_states; // <actor_id, state>
    };

    //--------------------------------------------------------------------------
    // Relationship from an access point to its type, which is an entity registered once in the world: up, down, waypoint,
    // portal, or a type that a mod adds
    struct AccessType
    {
    };

    //--------------------------------------------------------------------------
    // The access point type at which an actor arrives through a relocator of this type
    struct Arrival
    {
        flecs::entity access_type;
    };

    //--------------------------------------------------------------------------
    // An entity at which a zone is entered, sited on a wall, the floor, or the ceiling. The order counts from 0 among the
    // zone's access points of the same type, and the entry is where an arriving actor is placed
    struct AccessPoint
    {
        std::uint64_t order = 0;
        placement_type entry;
    };

    //--------------------------------------------------------------------------
    // Where the entity moves an actor that enters it: a destination, or the relocator that it is bound to
    struct Relocator
    {
        std::variant<std::string, EntityReference> target;
    };

    //--------------------------------------------------------------------------
    // Registers an access point type. Its Arrival is set once the type it arrives at is registered
    flecs::entity create_access_type(flecs::world& entities, std::string_view name);

    // Returns the registered access point type, or a null entity if no type has that name
    flecs::entity access_type(const flecs::world& entities, std::string_view name);

    //--------------------------------------------------------------------------
    // Makes an entity sited on a surface an access point of the type
    void set_access_point(flecs::entity e, flecs::entity access_type, std::uint64_t order, const placement_type& entry);

    // Creates an entity in the zone at the site and makes it an access point
    flecs::entity create_access_point(flecs::entity zone, flecs::entity access_type, const Site& site, std::uint64_t order,
        const placement_type& entry);

    // Returns the zone's access point of the type that has the lowest order, or a null entity if the zone has none
    flecs::entity entry_point(flecs::entity zone, flecs::entity access_type);

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
    // Returns an entity sited on the wall that has access, such as a door facing, or a null entity if there is none
    flecs::entity way_through(flecs::entity zone, id_type wall_id);

    // Returns true if an entity sited on the wall lets the actor through: its access is open, and the actor does not find
    // it concealed. A wall with no such entity cannot be crossed
    bool crossable(flecs::entity zone, id_type wall_id, flecs::entity actor);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_ATTACHMENTS_HPP
