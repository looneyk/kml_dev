#if !defined(QUETZAL_STRATUM_SIM_STRATUMGAME_HPP)
#define QUETZAL_STRATUM_SIM_STRATUMGAME_HPP
//------------------------------------------------------------------------------
// stratum_sim
// StratumGame.hpp
//------------------------------------------------------------------------------

#include "IdAllocator.hpp"
#include "Player.hpp"
#include "Session.hpp"
#include "World.hpp"

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Registers the game's entity types, such as a door that is closed until opened. Temporary, until the game's types
    // load from data
    void register_game_types(World& world);

    //--------------------------------------------------------------------------
    // The one owner of a game's world, session and player
    class StratumGame
    {
    public:

        StratumGame(); // A new game, counting persistent ids from zero
        StratumGame(const StratumGame&) = delete;
        ~StratumGame() = default;

        StratumGame& operator=(const StratumGame&) = delete;

        World& world();
        const World& world() const;

        Session& session();
        const Session& session() const;

        Player& player();
        const Player& player() const;

        // The region a new game's zones are made in. Temporary, until a new game chooses its region
        flecs::entity starting_region() const;

    private:

        // In construction order, each referring to those above it
        CountingIdAllocator m_counting_id_allocator;
        World m_world;
        Player m_player;
        Session m_session;
        flecs::entity m_starting_region;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_STRATUMGAME_HPP
