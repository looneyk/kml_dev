//------------------------------------------------------------------------------
// stratum_sim
// CommandQueue.cpp
//------------------------------------------------------------------------------

#include "CommandQueue.hpp"
#include <cassert>

//------------------------------------------------------------------------------
quetzal::stratum_sim::CommandQueue::CommandQueue() :
    m_commands()
{
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::CommandQueue::push(Command command)
{
    m_commands.push_back(std::move(command));
    return;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Command& quetzal::stratum_sim::CommandQueue::front() const
{
    assert(!m_commands.empty());
    return m_commands.front();
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::CommandQueue::pop()
{
    assert(!m_commands.empty());
    m_commands.pop_front();
    return;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::CommandQueue::empty() const
{
    return m_commands.empty();
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::CommandQueue::size() const
{
    return m_commands.size();
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::CommandQueue::clear()
{
    m_commands.clear();
    return;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::CommandQueue::commands_type& quetzal::stratum_sim::CommandQueue::commands() const
{
    return m_commands;
}
