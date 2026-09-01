//------------------------------------------------------------------------------
// stratum_sim
// Scheduler.cpp
//------------------------------------------------------------------------------

#include "Scheduler.hpp"
#include <cassert>

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Scheduler::add(id_type actor_id, int speed)
{
    assert(actor_id != nullid);

    m_speeds[actor_id] = speed;
    m_energies[actor_id] = 0;

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Scheduler::remove(id_type actor_id)
{
    m_speeds.erase(actor_id);
    m_energies.erase(actor_id);

    return;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Scheduler::contains(id_type actor_id) const
{
    return m_speeds.contains(actor_id);
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::Scheduler::count() const
{
    return m_speeds.size();
}

//------------------------------------------------------------------------------
int quetzal::stratum_sim::Scheduler::speed(id_type actor_id) const
{
    auto i = m_speeds.find(actor_id);
    assert(i != m_speeds.end());
    return i->second;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Scheduler::set_speed(id_type actor_id, int speed)
{
    assert(contains(actor_id));
    m_speeds[actor_id] = speed;

    return;
}

//------------------------------------------------------------------------------
int quetzal::stratum_sim::Scheduler::energy(id_type actor_id) const
{
    auto i = m_energies.find(actor_id);
    assert(i != m_energies.end());
    return i->second;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Scheduler::advance(long long units)
{
    if (units <= 0)
    {
        return;
    }

    for (auto& [actor_id, energy] : m_energies)
    {
        energy += static_cast<int>(units * m_speeds[actor_id]);
    }

    m_elapsed += units;

    return;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::Scheduler::ready() const
{
    id_type actor_id_ready = nullid;
    int most = 0;

    for (const auto& [actor_id, energy] : m_energies)
    {
        if (energy >= action_cost && energy > most)
        {
            actor_id_ready = actor_id;
            most = energy;
        }
    }

    return actor_id_ready;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::Scheduler::next()
{
    // Straight to the unit at which the soonest actor is ready rather than one at a time
    long long wait = -1;

    for (const auto& [actor_id, speed] : m_speeds)
    {
        if (speed <= 0)
        {
            continue;
        }

        int shortfall = action_cost - m_energies.at(actor_id);
        long long units = shortfall <= 0 ? 0 : (shortfall + speed - 1) / speed;

        if (wait < 0 || units < wait)
        {
            wait = units;
        }
    }

    if (wait < 0)
    {
        return nullid; // No actor accumulates anything
    }

    advance(wait);

    return ready();
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Scheduler::spend(id_type actor_id, int cost)
{
    assert(contains(actor_id));
    m_energies[actor_id] -= cost;

    return;
}

//------------------------------------------------------------------------------
long long quetzal::stratum_sim::Scheduler::elapsed() const
{
    return m_elapsed;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Scheduler::clear()
{
    m_speeds.clear();
    m_energies.clear();
    m_elapsed = 0;

    return;
}
