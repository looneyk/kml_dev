#if !defined(QUETZAL_STRATUM_SIM_EVENT_HPP)
#define QUETZAL_STRATUM_SIM_EVENT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Event.hpp
//------------------------------------------------------------------------------

#include <any>
#include <concepts>
#include <typeindex>
#include <type_traits>
#include <utility>
#include <cassert>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Something that has happened in the simulation. Any copyable type, stored type erased so a module can add events
    // Plain data, never a reference into the simulation, so it stays valid as the simulation advances
    class Event
    {
    public:

        Event();

        // Constrained to keep it from taking precedence over the copy constructor
        // Fully qualified so the constraint matches the out of line definition token for token
        template<typename E>
            requires (!std::same_as<std::remove_cvref_t<E>, quetzal::stratum_sim::Event>)
        Event(E event);

        Event(const Event&) = default;
        Event(Event&&) noexcept = default;
        ~Event() = default;

        Event& operator=(const Event&) = default;
        Event& operator=(Event&&) noexcept = default;

        std::type_index type() const;
        bool empty() const;

        template<typename E>
        bool is() const;

        // Must be an event of this type
        template<typename E>
        const E& get() const;

        // Null when the event is not of this type
        template<typename E>
        const E* try_get() const;

    private:

        std::any m_event;
    };

} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
inline quetzal::stratum_sim::Event::Event() :
    m_event()
{
}

//------------------------------------------------------------------------------
template<typename E>
    requires (!std::same_as<std::remove_cvref_t<E>, quetzal::stratum_sim::Event>)
quetzal::stratum_sim::Event::Event(E event) :
    m_event(std::move(event))
{
}

//------------------------------------------------------------------------------
inline std::type_index quetzal::stratum_sim::Event::type() const
{
    return std::type_index(m_event.type());
}

//------------------------------------------------------------------------------
inline bool quetzal::stratum_sim::Event::empty() const
{
    return !m_event.has_value();
}

//------------------------------------------------------------------------------
template<typename E>
bool quetzal::stratum_sim::Event::is() const
{
    return m_event.type() == typeid(E);
}

//------------------------------------------------------------------------------
template<typename E>
const E& quetzal::stratum_sim::Event::get() const
{
    const E* pevent = try_get<E>();
    assert(pevent != nullptr);
    return *pevent;
}

//------------------------------------------------------------------------------
template<typename E>
const E* quetzal::stratum_sim::Event::try_get() const
{
    return std::any_cast<E>(&m_event);
}

#endif // QUETZAL_STRATUM_SIM_EVENT_HPP
