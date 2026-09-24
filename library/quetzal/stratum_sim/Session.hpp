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

    // The creation, loading, and population here are a mess. we need to define better exactly what the responsibilities are and restructure this to follow that
    // Here is the way it should work:

    // Create and load do no zone management.
    // create does the layout, and then populates it.
    // A loaded zone may already be populated if it was previously saved, or generated (a fixed layout, for example) with pre-placed denizens.
    // The saved file will have a flag indicating whether it is pre-populated.
    // The bigger issue here is that the bodies of the creat and load operations should not live in Session, but in World and this would call into that.
    // See my "// actual session state ..." in the cpp file for what should be session's responsibility.
    // We need to better define the division of responsibilities and interactions of the StratumGame components.

    //--------------------------------------------------------------------------
    // The zone being played in a world, its turn order, the clock that advances it, and the player
    class Session
    {
    public:

        Session(World& world, Player& player);
        Session(const Session&) = delete;
        ~Session() = default;

        Session& operator=(const Session&) = delete;

        // Has the world generate a zone, and enters it at the access point type
        // Throws std::runtime_error when the zone has no access point of the type, the player staying where it was
        void create(const Generator& generator, flecs::entity access_type);

        // Load a zone file.
        // We need two versions of this, one for pre-generated zones which have geometry and entities, but need to be populated,
        // and another for saved zones which include the last population placement.
        // Split this or otherwise support conditional populating ...
        void load(const std::filesystem::path& filepath, flecs::entity access_type);

        // Moves the player to the entry of the zone's entry point of the access point type, and makes it the zone being
        // played, its actors the turn order. The world then trims its zones
        // Throws std::runtime_error when the zone has no access point of the type, the player staying where it was
        void enter(flecs::entity zone, flecs::entity access_type);

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

        World& m_world;
        Player& m_player;
        Scheduler m_scheduler;
        Clock m_clock;
        Command m_command_pending;
        flecs::entity m_zone;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SESSION_HPP
