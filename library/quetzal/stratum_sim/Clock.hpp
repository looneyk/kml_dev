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
    // How a clock gives time out, which is the whole of what makes a world turn-based or real
    // time. Chosen when a world is loaded rather than by whatever is hosting it, so a host
    // implements neither and does not know which it is running
    enum class Driver
    {
        TurnBased,
        RealTime
    };

    //--------------------------------------------------------------------------
    // Advances a World through its Scheduler: grants time, then runs whoever that made ready.
    // Which of the two grants a caller uses is the whole of what makes it turn-based or real
    // time; everything after the grant is the same either way
    // This is the simulation running, not a view of it, so a host supplies only the decision
    // it alone can make and none of the ordering
    class Clock
    {
    public:

        using duration_type = std::chrono::duration<double>; // Seconds, as Timestamp measures them

        // Units earned per second of wall clock under RealTime, and ignored under TurnBased.
        // One unit is one action for an actor of speed action_cost, so this is how many actions
        // a second such an actor gets; finer pacing is more units against lower speeds
        static constexpr double units_per_second_default = 1.0;

        Clock(World& world, Scheduler& scheduler);
        Clock(const Clock&) = delete;
        ~Clock() = default;

        Clock& operator=(const Clock&) = delete;

        Driver driver() const;
        double units_per_second() const;
        void set_driver(Driver driver, double units_per_second = units_per_second_default);

        // Enough for the soonest actor to become ready, skipping the wait. Turn-based, and
        // what a real-time caller must not do
        void grant_turn();

        // What these units earn at each actor's speed, the caller having converted from
        // whatever it measures. Actors may well not be ready afterwards
        void grant(long long units);

        // Runs each ready actor on whatever its Intent asks for, charging what the action
        // cost, until the controlled actor is up with nothing to do. Returns that actor, or
        // nullid when none is ready at all
        // Facing is taken before movement, so a turn and a step asked for together are one
        // action each rather than both at once
        // Turn-based drops what it took, one action to an intent. Real time keeps it, so a
        // direction held goes on producing steps as they are earned; what it does drop is an
        // intent whose action achieved nothing, which is what bounds the pass
        // An actor other than the controlled one that is asking for nothing waits, so that
        // deciding nothing does not stop the turn order
        id_type run(EventQueue& events, id_type actor_controlled);

        // Gives time out the way the driver does and runs whoever that made ready, returning
        // the controlled actor when it needs a decision it did not have
        // Turn-based skips ahead to the moment the next actor can act and goes again, so it
        // returns only when a decision is wanted or nobody will ever act. Real time grants what
        // the elapsed time earned and returns whether or not that made anyone ready, skipping
        // the wait being exactly what it must not do
        // A turn-based driver has no use for the elapsed time and ignores it
        id_type advance(EventQueue& events, id_type actor_controlled, duration_type elapsed = duration_type::zero());

    private:

        // Whole units earned by the elapsed time. The remainder is kept, so that a run of
        // frames too short to earn one still advances rather than each rounding away to nothing
        long long units_of(duration_type elapsed);

        World& m_world;
        Scheduler& m_scheduler;
        Driver m_driver = Driver::TurnBased;
        double m_units_per_second = units_per_second_default;
        double m_units_remainder = 0.0;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_CLOCK_HPP
