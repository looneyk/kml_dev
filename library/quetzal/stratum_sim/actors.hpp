#if !defined(QUETZAL_STRATUM_SIM_ACTORS_HPP)
#define QUETZAL_STRATUM_SIM_ACTORS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// actors.hpp
//------------------------------------------------------------------------------

#include <flecs.h>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Tag for an entity that others cannot move into
    struct Blocking
    {
    };

    //--------------------------------------------------------------------------
    // Tag for the actor that a host controls
    struct ControlledActor
    {
    };

    //--------------------------------------------------------------------------
    // Tag for an actor that has died. Once the controlled actor has it, no actor runs and the game is over
    struct Dead
    {
    };

    //--------------------------------------------------------------------------
    // The rate at which an actor gains energy in the turn order
    struct Speed
    {
        int value = 0;
    };

    void register_actor_components(flecs::world& entities);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_ACTORS_HPP
