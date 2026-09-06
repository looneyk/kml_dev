#if !defined(QUETZAL_STRATUM_SIM_INTENT_HPP)
#define QUETZAL_STRATUM_SIM_INTENT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Intent.hpp
//------------------------------------------------------------------------------

#include "types.hpp"

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // What an actor is trying to do, held on the actor rather than asked for when it comes
    // to act. A Command is an edge and this is a level: "north was pressed" against "north
    // is where I am trying to go"
    // Both parts are directions in world space, absolute rather than relative, which the
    // layout snaps to its own
    // Being a component it belongs to the entity: it survives unload and migration with
    // everything else the entity has, which is what an actor pursuing a goal over many
    // turns needs and what a decision asked for per action has nowhere to put
    struct Intent
    {
        vector_type direction; // Movement asked for, zero when none is
        value_type distance = value_type(1); // How far, which a real-time driver sets per tick
        vector_type facing; // Facing asked for, zero when the facing is to be left alone
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_INTENT_HPP
