//------------------------------------------------------------------------------
// stratum_sim
// Clock.cpp
//------------------------------------------------------------------------------

#include "Clock.hpp"
#include "actors.hpp"
#include "quetzal/math/floating_point.hpp"
#include <cmath>
#include <cassert>

namespace
{

    //--------------------------------------------------------------------------
    // Returns true if the direction is nonzero in the horizontal plane
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
void quetzal::stratum_sim::Clock::clear()
{
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
        if (actor_id == nullid || controlled_dead(actor_controlled))
        {
            return nullid;
        }

        flecs::entity e = m_world.entities().entity(actor_id);

        if (const auto* pending = e.try_get<PendingCommand>())
        {
            Command command = pending->command;
            e.remove<PendingCommand>();

            if (int cost = apply(m_world, command, events); cost > 0)
            {
                m_scheduler.spend(actor_id, cost);
            }

            continue;
        }

        const Intent* pintent = e.try_get<Intent>();

        Command command;
        bool turning = false;

        // Turning toward the direction of movement is part of the step; turning another way is a separate action
        if (pintent != nullptr && asked(pintent->facing) && asked(pintent->direction)
            && grid_step(pintent->facing) == grid_step(pintent->direction))
        {
            command = Command(actor_id, Step{pintent->direction, pintent->distance, true});
        }
        else if (pintent != nullptr && asked(pintent->facing))
        {
            command = Command(actor_id, TurnTo{pintent->facing});
            turning = true;
        }
        else if (pintent != nullptr && asked(pintent->direction))
        {
            command = Command(actor_id, Step{pintent->direction, pintent->distance});
        }

        if (command.empty())
        {
            if (actor_id == actor_controlled)
            {
                return actor_id;
            }

            m_scheduler.spend(actor_id);
            continue;
        }

        int cost = apply(m_world, command, events);

        // Clearing an intent whose action achieved nothing keeps an actor against a wall from retrying without end
        if (m_driver == Driver::TurnBased || cost == 0)
        {
            Intent remaining = *pintent;
            if (turning)
            {
                remaining.facing = vector_type();
            }
            else
            {
                // A step that turned toward its direction used up the facing direction too; a different facing direction remains
                remaining.direction = vector_type();

                if (asked(remaining.facing) && grid_step(remaining.facing) == grid_step(pintent->direction))
                {
                    remaining.facing = vector_type();
                }
            }

            e.set<Intent>(remaining);
        }

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

    // Turn-based skips ahead to the next actor's turn
    for (;;)
    {
        id_type actor_id = run(events, actor_controlled);
        if (actor_id != nullid || controlled_dead(actor_controlled))
        {
            return actor_id;
        }

        grant_turn();
        if (m_scheduler.ready() == nullid)
        {
            return nullid; // No actor will ever act
        }
    }
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Clock::controlled_dead(id_type actor_controlled) const
{
    return m_world.entities().entity(actor_controlled).has<Dead>();
}

//------------------------------------------------------------------------------
long long quetzal::stratum_sim::Clock::units_of(duration_type elapsed)
{
    m_units_remainder += elapsed.count() * m_units_per_second;

    double whole = std::floor(m_units_remainder);
    m_units_remainder -= whole;

    return static_cast<long long>(whole);
}
