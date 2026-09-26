#if !defined(QUETZAL_STRATUM_SIM_PLAYER_HPP)
#define QUETZAL_STRATUM_SIM_PLAYER_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Player.hpp
//------------------------------------------------------------------------------

#include "Command.hpp"
#include "Intent.hpp"
#include "World.hpp"
#include <flecs.h>
#include <utility>

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

        // Sets the movement and facing direction the player is trying to take, which persist until changed
        void set_intent(const Intent& intent);

        // Submits a one-shot action, which the player takes the next time it is ready
        template<typename C>
        void submit(C action);

    private:

        flecs::entity m_entity;
    };

} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
template<typename C>
void quetzal::stratum_sim::Player::submit(C action)
{
    m_entity.set<PendingCommand>(PendingCommand {Command(static_cast<id_type>(m_entity.id()), std::move(action))});
    return;
}

#endif // QUETZAL_STRATUM_SIM_PLAYER_HPP
