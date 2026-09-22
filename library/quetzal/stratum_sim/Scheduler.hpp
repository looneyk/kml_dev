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
    // Orders actors in abstract units. Each accumulates energy at its own speed and acts once it has action_cost to spend
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

        // Grants every actor the energy its speed earns over these units
        void advance(long long units);

        // The actor that can act now, the one with the most energy first and the lowest id among ties, or nullid when
        // none can yet
        id_type ready() const;

        // Advances to the moment the soonest actor can act and returns it. Turn-based only
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
