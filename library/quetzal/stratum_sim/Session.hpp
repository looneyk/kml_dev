#if !defined(QUETZAL_STRATUM_SIM_SESSION_HPP)
#define QUETZAL_STRATUM_SIM_SESSION_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Session.hpp
//------------------------------------------------------------------------------

#include "Clock.hpp"
#include "Command.hpp"
#include "EventQueue.hpp"
#include "Layout.hpp"
#include "Character.hpp"
#include "Scheduler.hpp"
#include "World.hpp"
#include "quetzal/stratum_format/Notation.hpp"

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    class Session
    {
    public:

        Session(World& world, Character& character);
        Session(const Session&) = delete;
        Session(Session&&) = delete;
        ~Session() = default;

        Session& operator=(const Session&) = delete;
        Session& operator=(Session&&) = delete;

        // Session management

        void clear();

        // Moves the character to the access point's placement, and makes the access point's zone the one being played
        void start(flecs::entity access_point);

        void pause();
        void resume();

        // Moves the character to the placement of the zone's entry_point for the access point type and level offset, and makes
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

        Character& character();
        const Character& character() const;

        // Advances the clock and runs every ready actor, returning the character when it needs a command
        // elapsed is the real time since the last call; a turn-based host omits it
        id_type advance(EventQueue& events, Clock::duration_type elapsed = Clock::duration_type::zero());

    private:

        // Makes the zone the character is in the one being played: the world trims its zones, the character's pending command is
        // dropped, and the turn order is rebuilt from the zone's actors
        void play(flecs::entity zone);

        World& m_world;
        Character& m_character;
        Scheduler m_scheduler;
        Clock m_clock;
        flecs::entity m_zone;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SESSION_HPP
