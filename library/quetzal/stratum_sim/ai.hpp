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
    // Moves toward a target while it is visible, then toward where it was last seen, stopping there if it finds nothing
    struct Pursue
    {
        std::optional<id_type> target_id; // The controlled actor where absent
        point_type position_last_seen;
        bool seen = false; // Whether position_last_seen is set
    };

    // Sets an Intent for every actor with a behavior except the controlled one
    void think(World& world, id_type actor_controlled);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_AI_HPP
