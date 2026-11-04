#if !defined(QUETZAL_STRATUM_SIM_AI_HPP)
#define QUETZAL_STRATUM_SIM_AI_HPP
//------------------------------------------------------------------------------
// stratum_sim
// ai.hpp
//------------------------------------------------------------------------------

#include "Intent.hpp"
#include "World.hpp"
#include "types.hpp"
#include <optional>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Moves toward a target while it is visible, then toward where it was last seen, stopping there if the target is not in sight
    struct Pursue
    {
        std::optional<EntityReference> target; // The controlled actor if absent
        point_type position_last_seen;
        bool seen = false; // True if position_last_seen is set
    };

    void register_ai_components(flecs::world& entities);

    // Sets an Intent for every actor with a behavior except the controlled one
    void think(World& world, id_type actor_controlled);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_AI_HPP
