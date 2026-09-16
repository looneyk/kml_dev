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
#include "Scheduler.hpp"
#include "World.hpp"
#include "attachments.hpp"
#include "load_layout.hpp"
#include <filesystem>

namespace quetzal::stratum_sim
{

    // The creation, loading, and population here are a mess. we need to define better exactly what the responsibilities are and restructure this to follow that ...

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

        // A zone constructed whole, which is how one normally arrives: the generator builds
        // the geometry and every content of it follows
        // The driver comes in here rather than from a host, so that turn-based and real time
        // are a property of the zone. It will come from the zone's own data once zones have
        // any
        // Throws std::runtime_error when the zone has no open cell. The session is left as
        // it was rather than half built, so a failure leaves the previous zone intact
        void create(const Generator& generator, Driver driver = Driver::TurnBased,
            double units_per_second = Clock::units_per_second_default);

        // Load a zone file.
        // We need two versions of this, one for pre-generated zones which have geometry and entities, but need to be populated,
        // and another for saved zones which include the last population placement.
        // Split this or otherwise support conditional populating ...
        void load(const std::filesystem::path& filepath, Driver driver = Driver::TurnBased,
            double units_per_second = Clock::units_per_second_default);

        // Geometry already built, populated as a constructed zone is. Not how a zone normally
        // arrives: this is the entry for a caller with geometry from somewhere else, and
        // what the other two are written in terms of
        // Why are doors not added in the create step? Zone geometry belongs there ...
        // create also calls populate, so that contradicts the above ...
        // populate should not be creating player, that belongs in session itself ...
        void populate(std::unique_ptr<Layout> layout, const door_walls_type& doors, stratum_format::Notation notation,
            Driver driver = Driver::TurnBased,
            double units_per_second = Clock::units_per_second_default);

        bool loaded() const;

        // Which of the two geometry models the zone is, whether it was read or constructed.
        // Both build the same structure, so this is for a caller with something else to
        // decide by it. Block until a zone exists
        stratum_format::Notation notation() const;

        World& world();
        const World& world() const;

        // The loaded zone's layout. Must be loaded. A caller needing more than the interface
        // exposes uses layout_as to get the concrete class
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
        stratum_format::Notation m_notation = stratum_format::Notation::Block;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SESSION_HPP
