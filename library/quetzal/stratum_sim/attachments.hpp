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
    // On an entity that is a way through, such as a door: whether it lets anything through. A door off its hinges is open,
    // and one smashed into rubble that blocks it is closed. On the door rather than a door facing, so both sides agree
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
    // On an entity that not every actor sees, such as one side of a secret door: its state, and that of each actor whose own
    // differs
    struct Detection
    {
        DetectionState state = DetectionState::Visible;
        std::unordered_map<PersistentId, DetectionState> actor_states; // <actor_id, state>
    };

    //--------------------------------------------------------------------------
    // Relationship from an access point to its type, an entity registered once in the world: up, down, waypoint, portal,
    // or one a mod adds
    struct AccessType
    {
    };

    //--------------------------------------------------------------------------
    // On an access point type: the type an actor arrives at through a relocator of this type
    struct Arrival
    {
        flecs::entity access_type;
    };

    //--------------------------------------------------------------------------
    // An entity a zone is entered at, sited on a surface: a wall, the floor or the ceiling. Its order counts from 0 among
    // the zone's access points of its type, and its entry is where an arriving actor is placed
    struct AccessPoint
    {
        std::uint64_t order = 0;
        placement_type entry;
    };

    //--------------------------------------------------------------------------
    // Where an entity moves an actor that enters it: a destination, or the relocator it is bound to
    struct Relocator
    {
        std::variant<std::string, EntityReference> target;
    };

    //--------------------------------------------------------------------------
    // Registers an access point type, whose arrival is set on it once the type it arrives at is registered
    flecs::entity create_access_type(flecs::world& entities, std::string_view name);

    // The registered access point type, or a null entity where none has that name
    flecs::entity access_type(const flecs::world& entities, std::string_view name);

    //--------------------------------------------------------------------------
    // Makes an entity sited on a surface an access point of the type
    void set_access_point(flecs::entity e, flecs::entity access_type, std::uint64_t order, const placement_type& entry);

    // A new entity in the zone, sited there and made an access point
    flecs::entity create_access_point(flecs::entity zone, flecs::entity access_type, const Site& site, std::uint64_t order,
        const placement_type& entry);

    // The zone's access point of the type with the lowest order available, or a null entity where it has none
    flecs::entity entry_point(flecs::entity zone, flecs::entity access_type);

    //--------------------------------------------------------------------------
    // The entity whose Access applies to e: e, or the nearest of its parents with one. Null where none has one
    flecs::entity access_entity(flecs::entity e);

    std::optional<AccessState> access(flecs::entity e);

    // How the actor detects e: from e's Detection or the nearest of its parents', the actor's own state where it has one,
    // and otherwise the Detection's state. Visible where none has one. A null actor takes the Detection's state
    DetectionState detection(flecs::entity e, flecs::entity actor);

    //--------------------------------------------------------------------------
    // An entity sited on the wall that has access, such as a door facing, or a null entity
    flecs::entity way_through(flecs::entity zone, id_type wall_id);

    // Whether an entity sited on the wall lets the actor through: its access is open and the actor does not find it
    // concealed. A wall with nothing on it with access cannot be crossed
    bool crossable(flecs::entity zone, id_type wall_id, flecs::entity actor);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_ATTACHMENTS_HPP
