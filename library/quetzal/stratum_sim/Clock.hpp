#if !defined(QUETZAL_STRATUM_SIM_CLOCK_HPP)
#define QUETZAL_STRATUM_SIM_CLOCK_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Clock.hpp
//------------------------------------------------------------------------------

#include "commands.hpp"
#include "EventQueue.hpp"
#include "Intent.hpp"
#include "Scheduler.hpp"
#include "World.hpp"
#include <chrono>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // How a clock grants time. Set when a world is loaded, not by the host
    enum class Driver
    {
        TurnBased,
        RealTime
    };

    //--------------------------------------------------------------------------
    // Advances a World through its Scheduler: grants time, then runs each actor that became ready
    class Clock
    {
    public:

        using duration_type = std::chrono::duration<double>; // Seconds

        // Units granted per second of real time under RealTime. One unit is one action for an actor of speed action_cost
        static constexpr double units_per_second_default = 1.0;

        Clock(World& world, Scheduler& scheduler);
        Clock(const Clock&) = delete;
        ~Clock() = default;

        Clock& operator=(const Clock&) = delete;

        Driver driver() const;
        double units_per_second() const;
        void set_driver(Driver driver, double units_per_second = units_per_second_default);

        // Grants enough time for the soonest actor to become ready. Turn-based only
        void grant_turn();

        // Grants units to each actor at its speed; no actor need be ready afterwards
        void grant(long long units);

        // Runs each ready actor on its PendingCommand, or failing that its Intent, charging the action's cost, until the
        // controlled actor is ready with neither. Returns that actor, or nullid when no actor is ready
        // A turn requested with a move in another direction is a separate action, taken first
        // Turn-based clears the Intent after one action; real time keeps it, so a held direction keeps the actor moving.
        // Both clear an Intent whose action achieved nothing, which ends the loop
        // Any other actor with no Intent waits
        id_type run(EventQueue& events, id_type actor_controlled);

        // Grants time as the driver does and runs the actors that became ready, returning the controlled actor when it
        // needs a command
        // Turn-based skips ahead to the next ready actor and repeats, returning only when the controlled actor needs a
        // command or no actor will ever act. Real time grants the units elapsed has earned and returns whether or not an actor
        // became ready. Turn-based ignores elapsed
        id_type advance(EventQueue& events, id_type actor_controlled, duration_type elapsed = duration_type::zero());

    private:

        // Whole units earned by elapsed; the fraction is kept for the next call
        long long units_of(duration_type elapsed);

        World& m_world;
        Scheduler& m_scheduler;
        Driver m_driver = Driver::TurnBased;
        double m_units_per_second = units_per_second_default;
        double m_units_remainder = 0.0;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_CLOCK_HPP
