#if !defined(QUETZAL_STRATUM_SIM_ATTACHMENTS_HPP)
#define QUETZAL_STRATUM_SIM_ATTACHMENTS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// attachments.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <flecs.h>
#include <optional>
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
    // The state of an entity such as a door. It is on a door rather than a door facing, so both sides agree
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
    struct ActorDetection
    {
        PersistentId actor;
        DetectionState state = DetectionState::Visible;
    };

    //--------------------------------------------------------------------------
    struct Detection
    {
        DetectionState state = DetectionState::Visible;
        std::vector<ActorDetection> actor_states;
    };

    void register_attachment_components(flecs::world& entities);

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
    flecs::entity find_access_attachment(flecs::entity zone, id_type wall_id);

    // Returns true if an entity sited on the wall lets the actor through: its access is open, and the actor does not find
    // it concealed. A wall with no such entity cannot be crossed
    bool crossable(flecs::entity zone, id_type wall_id, flecs::entity actor);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_ATTACHMENTS_HPP
