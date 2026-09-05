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
#include "load_layout.hpp"
#include "Scheduler.hpp"
#include "World.hpp"
#include <filesystem>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // A loaded world and what is needed to run it: its zones, the turn order over them, the
    // clock advancing both, and the entity a host controls
    // This is the simulation being played rather than a view of it, so a host is left with
    // input mapping and drawing and nothing else
    class Session
    {
    public:

        Session();
        Session(const Session&) = delete;
        ~Session() = default;

        Session& operator=(const Session&) = delete;

        // Reads a level into a new zone and places the controlled entity in the lowest open
        // cell, facing east
        // The driver comes in here rather than from a host, so that turn-based and real time
        // are a property of what was loaded. It will come from the level's own data once
        // levels carry any
        // Throws std::runtime_error when the file cannot be read or holds no open cell. The
        // session is left as it was rather than half loaded, so a failed load leaves the
        // previous level standing
        void load(const std::filesystem::path& filepath, Driver driver = Driver::TurnBased,
            double units_per_second = Clock::units_per_second_default);

        bool loaded() const;

        // Which format the loaded level was written in. Both load into the same structure, so
        // this is for a caller with something else to decide by it. Block until one is loaded
        Format format() const;

        World& world();
        const World& world() const;

        // The loaded zone's layout. Must be loaded. A caller needing more than the interface
        // says reaches for the concrete class with layout_as
        Layout& layout();
        const Layout& layout() const;

        Scheduler& scheduler();
        const Scheduler& scheduler() const;

        flecs::entity zone() const;
        flecs::entity player() const; // The entity the host controls

        // What the controlled entity is trying to do. Movement and facing are held on the
        // entity and last until something changes them
        void set_intent(const Intent& intent);

        // A one-shot action for the controlled entity, taken by the next advance and cleared.
        // Interacting is an edge rather than a level, so it is asked for rather than held
        void submit(const Command& command);

        // Gives time out and runs, returning the controlled entity when it needs a decision
        // that was not available
        // The elapsed time is what a real-time host measured since it last called; a
        // turn-based one has no use for it and need not pass any
        // nullid means no actor will ever be ready, which is not the same as none being ready
        // now: passing time is what the grant already tried
        id_type advance(EventQueue& events, Clock::duration_type elapsed = Clock::duration_type::zero());

    private:

        World m_world;
        Scheduler m_scheduler;
        Clock m_clock;
        Command m_command_pending;
        flecs::entity m_zone;
        flecs::entity m_player;
        Format m_format = Format::Block;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SESSION_HPP
