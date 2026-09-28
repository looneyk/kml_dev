#if !defined(QUETZAL_STRATUM_SIM_SESSION_HPP)
#define QUETZAL_STRATUM_SIM_SESSION_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Session.hpp
//------------------------------------------------------------------------------

#include "Clock.hpp"
#include "Command.hpp"
#include "Generator.hpp"
#include "EventQueue.hpp"
#include "Layout.hpp"
#include "Player.hpp"
#include "Scheduler.hpp"
#include "World.hpp"
#include "quetzal/stratum_format/Notation.hpp"
#include <filesystem>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    class Session
    {
    public:

        Session(World& world, Player& player);
        Session(const Session&) = delete;
        ~Session() = default;

        Session& operator=(const Session&) = delete;

        // Generate a zone in the region
        void create(flecs::entity region, const Generator& generator, flecs::entity access_type, level_type offset);

        void load(flecs::entity region, const std::filesystem::path& filepath, flecs::entity access_type, level_type offset);

        // Moves the player to the placement of the zone's entry_point for the access point type and level offset, and makes
        // it the zone being played. Throws std::runtime_error when the zone has no such entry point
        void enter(flecs::entity zone, flecs::entity access_type, level_type offset);

        void set_driver(Driver driver, double units_per_second = Clock::units_per_second_default);

        bool loaded() const;

        // Requires a loaded zone
        stratum_format::Notation notation() const;

        World& world();
        const World& world() const;

        // Requires a loaded zone
        Layout& layout();
        const Layout& layout() const;

        Scheduler& scheduler();
        const Scheduler& scheduler() const;

        flecs::entity zone() const;

        Player& player();
        const Player& player() const;

        // Advances the clock and runs every ready actor, returning the player when it needs a command
        // elapsed is the real time since the last call; a turn-based host omits it
        id_type advance(EventQueue& events, Clock::duration_type elapsed = Clock::duration_type::zero());

    private:

        // Makes the zone the player is in the one being played: the world trims its zones, the player's pending command is
        // dropped, and the turn order is rebuilt from the zone's actors
        void play(flecs::entity zone);

        World& m_world;
        Player& m_player;
        Scheduler m_scheduler;
        Clock m_clock;
        flecs::entity m_zone;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SESSION_HPP
