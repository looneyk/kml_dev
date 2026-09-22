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
    // What an actor is trying to do, persisting until changed. Directions are absolute, in world space, and the layout
    // snaps them to its own
    struct Intent
    {
        vector_type direction; // Zero for no movement
        value_type distance = value_type(1);
        vector_type facing; // Zero to leave the facing direction unchanged
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_INTENT_HPP
