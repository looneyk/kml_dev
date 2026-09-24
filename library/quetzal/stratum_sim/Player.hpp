#if !defined(QUETZAL_STRATUM_SIM_PLAYER_HPP)
#define QUETZAL_STRATUM_SIM_PLAYER_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Player.hpp
//------------------------------------------------------------------------------

#include "World.hpp"
#include <flecs.h>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The entity a host controls
    class Player
    {
    public:

        // Creates the entity, in no zone until one is entered
        explicit Player(World& world);
        Player(const Player&) = delete;
        ~Player() = default;

        Player& operator=(const Player&) = delete;

        flecs::entity entity() const;

    private:

        flecs::entity m_entity;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_PLAYER_HPP
