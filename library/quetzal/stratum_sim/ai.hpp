#if !defined(QUETZAL_STRATUM_SIM_AI_HPP)
#define QUETZAL_STRATUM_SIM_AI_HPP
//------------------------------------------------------------------------------
// stratum_sim
// ai.hpp
//------------------------------------------------------------------------------

#include "Intent.hpp"
#include "World.hpp"
#include "types.hpp"

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Goes toward a target while it can be seen, toward where it was last seen when it cannot,
    // and gives that up on arriving there having found nothing
    // Where it was last seen is what makes this more than a reaction, and it is held on the
    // entity rather than beside it, so it unloads, migrates and saves with everything else the
    // entity has. A decision asked for per action would have nowhere to keep it
    struct Pursue
    {
        id_type target_id = nullid;
        point_type position_last_seen;
        bool seen = false; // Whether position_last_seen means anything yet
    };

    // Writes an Intent for everything that decides for itself, which is everything but the
    // entity a host controls
    // An actor with no behavior is left alone, and one that asks for nothing waits rather
    // than stopping the turn order
    void think(World& world, id_type actor_controlled);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_AI_HPP
