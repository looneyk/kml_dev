#if !defined(QUETZAL_STRATUM_SIM_COMMANDQUEUE_HPP)
#define QUETZAL_STRATUM_SIM_COMMANDQUEUE_HPP
//------------------------------------------------------------------------------
// stratum_sim
// CommandQueue.hpp
//------------------------------------------------------------------------------

#include "Command.hpp"
#include "quetzal/common/id.hpp"
#include <deque>
#include <utility>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Commands awaiting execution, consumed in submission order
    class CommandQueue
    {
    public:

        using commands_type = std::deque<Command>;

        CommandQueue();
        CommandQueue(const CommandQueue&) = default;
        CommandQueue(CommandQueue&&) noexcept = default;
        ~CommandQueue() = default;

        CommandQueue& operator=(const CommandQueue&) = default;
        CommandQueue& operator=(CommandQueue&&) noexcept = default;

        template<typename C>
        void push(id_type entity_id, C command);

        void push(Command command);

        // Must not be empty
        const Command& front() const;
        void pop();

        bool empty() const;
        size_t size() const;
        void clear();

        const commands_type& commands() const; // In submission order

    private:

        commands_type m_commands;
    };

} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
template<typename C>
void quetzal::stratum_sim::CommandQueue::push(id_type entity_id, C command)
{
    m_commands.emplace_back(entity_id, std::move(command));
    return;
}

#endif // QUETZAL_STRATUM_SIM_COMMANDQUEUE_HPP
