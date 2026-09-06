//------------------------------------------------------------------------------
// stratum_sim
// Clock.cpp
//------------------------------------------------------------------------------

#include "Clock.hpp"
#include "quetzal/math/floating_point.hpp"
#include <cmath>
#include <cassert>

namespace
{

    //--------------------------------------------------------------------------
    // Whether a direction asks for anything. Horizontal, as the layout's own directions are
    bool asked(const quetzal::stratum_sim::vector_type& direction)
    {
        auto [dx, dy, dz] = direction.rep();
        return !quetzal::math::float_eq0(dx) || !quetzal::math::float_eq0(dy);
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::Clock::Clock(World& world, Scheduler& scheduler) :
    m_world(world),
    m_scheduler(scheduler)
{
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Driver quetzal::stratum_sim::Clock::driver() const
{
    return m_driver;
}

//------------------------------------------------------------------------------
double quetzal::stratum_sim::Clock::units_per_second() const
{
    return m_units_per_second;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Clock::set_driver(Driver driver, double units_per_second)
{
    assert(units_per_second > 0.0);

    m_driver = driver;
    m_units_per_second = units_per_second;
    m_units_remainder = 0.0;

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Clock::grant_turn()
{
    m_scheduler.next();
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Clock::grant(long long units)
{
    m_scheduler.advance(units);
    return;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::Clock::run(EventQueue& events, id_type actor_controlled)
{
    for (;;)
    {
        id_type actor_id = m_scheduler.ready();
        if (actor_id == nullid)
        {
            return nullid;
        }

        flecs::entity e = m_world.entities().entity(actor_id);
        const Intent* pintent = e.try_get<Intent>();

        Command command;
        bool turning = false;

        // Asked to face the way it is going, an actor cannot do one without the other, so the
        // two are a single faced step. Asked to face elsewhere while moving, they are two
        // actions and the turn is an action of its own
        if (pintent != nullptr && asked(pintent->facing) && asked(pintent->direction)
            && grid_step(pintent->facing) == grid_step(pintent->direction))
        {
            command = Command(actor_id, Step {pintent->direction, pintent->distance, true});
        }
        else if (pintent != nullptr && asked(pintent->facing))
        {
            command = Command(actor_id, TurnTo {pintent->facing});
            turning = true;
        }
        else if (pintent != nullptr && asked(pintent->direction))
        {
            command = Command(actor_id, Step {pintent->direction, pintent->distance});
        }

        if (command.empty())
        {
            // The controlled actor hands control out rather than waiting, which is the whole
            // of what tells it apart from everything else
            if (actor_id == actor_controlled)
            {
                return actor_id;
            }

            m_scheduler.spend(actor_id);
            continue;
        }

        int cost = apply(m_world, command, events);

        // Turn-based takes one action's worth of what was asked for and is done with it. Real
        // time keeps it, so that a direction held goes on producing steps as the actor earns
        // them without being asked again every frame
        // Either way it is dropped when the action achieved nothing, which is what stops a
        // actor held against a wall from trying the same step without end and is why the pass
        // below always terminates
        if (m_driver == Driver::TurnBased || cost == 0)
        {
            Intent remaining = *pintent;
            if (turning)
            {
                remaining.facing = vector_type();
            }
            else
            {
                // A faced step consumed the facing along with the direction, and where the
                // two differ the facing is still owed
                remaining.direction = vector_type();

                if (asked(remaining.facing) && grid_step(remaining.facing) == grid_step(pintent->direction))
                {
                    remaining.facing = vector_type();
                }
            }

            e.set<Intent>(remaining);
        }

        // An action that achieved nothing costs no time, so the actor is still owed a turn.
        // What it asked for is dropped in that case, so the next pass finds nothing asked for
        // and hands control back or waits rather than trying the same action again
        if (cost > 0)
        {
            m_scheduler.spend(actor_id, cost);
        }
    }
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::Clock::advance(EventQueue& events, id_type actor_controlled, duration_type elapsed)
{
    if (m_driver == Driver::RealTime)
    {
        m_scheduler.advance(units_of(elapsed));
        return run(events, actor_controlled);
    }

    // Skipping ahead to the moment the next actor can act, which is the whole of turn-based
    for (;;)
    {
        id_type actor_id = run(events, actor_controlled);
        if (actor_id != nullid)
        {
            return actor_id;
        }

        grant_turn();
        if (m_scheduler.ready() == nullid)
        {
            return nullid; // Nobody can act, and passing time did not change that
        }
    }
}

//------------------------------------------------------------------------------
long long quetzal::stratum_sim::Clock::units_of(duration_type elapsed)
{
    m_units_remainder += elapsed.count() * m_units_per_second;

    double whole = std::floor(m_units_remainder);
    m_units_remainder -= whole;

    return static_cast<long long>(whole);
}
