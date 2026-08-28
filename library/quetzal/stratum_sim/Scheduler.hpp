#if !defined(QUETZAL_STRATUM_SIM_SCHEDULER_HPP)
#define QUETZAL_STRATUM_SIM_SCHEDULER_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Scheduler.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/id.hpp"
#include <map>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Orders actors in abstract units. Each accumulates energy at its own speed and acts
    // once it has action_cost to spend
    // Whether units advance on player command or on the wall clock is the driver's
    // business, not this one's
    class Scheduler
    {
    public:

        using speeds_type = std::map<id_type, int>; // Ordered, so ties break the same way every run

        static constexpr int action_cost = 100; // One ordinary action

        Scheduler() = default;
        Scheduler(const Scheduler&) = default;
        Scheduler(Scheduler&&) noexcept = default;
        ~Scheduler() = default;

        Scheduler& operator=(const Scheduler&) = default;
        Scheduler& operator=(Scheduler&&) noexcept = default;

        void add(id_type actor_id, int speed);
        void remove(id_type actor_id);
        bool contains(id_type actor_id) const;
        size_t count() const;

        int speed(id_type actor_id) const;
        void set_speed(id_type actor_id, int speed);
        int energy(id_type actor_id) const;

        // Advances until an actor can act and returns it, the readiest first and the
        // lowest id of those tied. nullid when no actor can ever act
        id_type next();

        void spend(id_type actor_id, int cost = action_cost);

        long long elapsed() const;
        void clear();

    private:

        speeds_type m_speeds;
        speeds_type m_energies;
        long long m_elapsed = 0;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SCHEDULER_HPP
