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
    // A loaded world with its zones, their turn order, the clock that advances them, and the entity a host controls
    class Session
    {
    public:

        Session();
        Session(const Session&) = delete;
        ~Session() = default;

        Session& operator=(const Session&) = delete;

        // Generates a zone and populates it
        // Throws std::runtime_error when the zone has no open cell, leaving the previous zone intact
        void create(const Generator& generator, Driver driver = Driver::TurnBased,
            double units_per_second = Clock::units_per_second_default);

        // Load a zone file.
        // We need two versions of this, one for pre-generated zones which have geometry and entities, but need to be populated,
        // and another for saved zones which include the last population placement.
        // Split this or otherwise support conditional populating ...
        void load(const std::filesystem::path& filepath, Driver driver = Driver::TurnBased,
            double units_per_second = Clock::units_per_second_default);

        // Populates a layout built elsewhere
        // Why are doors not added in the create step? Zone geometry belongs there ...
        // create also calls populate, so that contradicts the above ...
        // populate should not be creating player, that belongs in session itself ...
        void populate(std::unique_ptr<Layout> layout, const door_walls_type& doors, stratum_format::Notation notation,
            Driver driver = Driver::TurnBased,
            double units_per_second = Clock::units_per_second_default);

        bool loaded() const;

        // Block until a zone exists
        stratum_format::Notation notation() const;

        World& world();
        const World& world() const;

        // Requires a loaded zone
        Layout& layout();
        const Layout& layout() const;

        Scheduler& scheduler();
        const Scheduler& scheduler() const;

        flecs::entity zone() const;
        flecs::entity player() const; // The entity the host controls

        // The player's movement and facing direction, which persist until changed
        void set_intent(const Intent& intent);

        // A one-shot action for the player, taken by the next advance
        void submit(const Command& command);

        // Advances the clock and runs every ready actor, returning the player when it needs a command, or nullid when no
        // actor will ever be ready
        // elapsed is the real time since the last call; a turn-based host omits it
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
