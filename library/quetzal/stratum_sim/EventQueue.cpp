//------------------------------------------------------------------------------
// stratum_sim
// EventQueue.cpp
//------------------------------------------------------------------------------

#include "EventQueue.hpp"
#include <cassert>

//------------------------------------------------------------------------------
quetzal::stratum_sim::EventQueue::EventQueue() :
    m_events()
{
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Event& quetzal::stratum_sim::EventQueue::front() const
{
    assert(!m_events.empty());
    return m_events.front();
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::EventQueue::pop()
{
    assert(!m_events.empty());
    m_events.pop_front();
    return;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::EventQueue::empty() const
{
    return m_events.empty();
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::EventQueue::size() const
{
    return m_events.size();
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::EventQueue::clear()
{
    m_events.clear();
    return;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::EventQueue::events_type& quetzal::stratum_sim::EventQueue::events() const
{
    return m_events;
}
