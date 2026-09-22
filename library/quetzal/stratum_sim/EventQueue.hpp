#if !defined(QUETZAL_STRATUM_SIM_EVENTQUEUE_HPP)
#define QUETZAL_STRATUM_SIM_EVENTQUEUE_HPP
//------------------------------------------------------------------------------
// stratum_sim
// EventQueue.hpp
//------------------------------------------------------------------------------

#include "Event.hpp"
#include <deque>
#include <utility>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Events produced by the simulation, in the order in which they occurred
    class EventQueue
    {
    public:

        using events_type = std::deque<Event>;

        EventQueue();
        EventQueue(const EventQueue&) = default;
        EventQueue(EventQueue&&) noexcept = default;
        ~EventQueue() = default;

        EventQueue& operator=(const EventQueue&) = default;
        EventQueue& operator=(EventQueue&&) noexcept = default;

        template<typename E>
        void push(E event);

        // Must not be empty
        const Event& front() const;
        void pop();

        bool empty() const;
        size_t size() const;
        void clear();

        const events_type& events() const; // In occurrence order

    private:

        events_type m_events;
    };

} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
template<typename E>
void quetzal::stratum_sim::EventQueue::push(E event)
{
    m_events.emplace_back(std::move(event));
    return;
}

#endif // QUETZAL_STRATUM_SIM_EVENTQUEUE_HPP
