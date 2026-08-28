#if !defined(QUETZAL_STRATUM_SIM_COMMAND_HPP)
#define QUETZAL_STRATUM_SIM_COMMAND_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Command.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/id.hpp"
#include <any>
#include <typeindex>
#include <utility>
#include <cassert>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Intent submitted to the simulation, produced by a view's input mapping or by an
    // acting entity's own logic
    // The command itself is any copyable type and is held type erased, so that a module
    // can introduce commands that the core has no knowledge of
    class Command
    {
    public:

        Command();

        template<typename C>
        Command(id_type entity_id, C command);

        Command(const Command&) = default;
        Command(Command&&) noexcept = default;
        ~Command() = default;

        Command& operator=(const Command&) = default;
        Command& operator=(Command&&) noexcept = default;

        id_type entity_id() const; // The entity the command acts for, nullid when none
        std::type_index type() const;
        bool empty() const;

        template<typename C>
        bool is() const;

        // Must be a command of this type
        template<typename C>
        const C& get() const;

        // Null when the command is not of this type
        template<typename C>
        const C* try_get() const;

    private:

        id_type m_entity_id;
        std::any m_command;
    };

} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
inline quetzal::stratum_sim::Command::Command() :
    m_entity_id(nullid),
    m_command()
{
}

//------------------------------------------------------------------------------
template<typename C>
quetzal::stratum_sim::Command::Command(id_type entity_id, C command) :
    m_entity_id(entity_id),
    m_command(std::move(command))
{
}

//------------------------------------------------------------------------------
inline quetzal::id_type quetzal::stratum_sim::Command::entity_id() const
{
    return m_entity_id;
}

//------------------------------------------------------------------------------
inline std::type_index quetzal::stratum_sim::Command::type() const
{
    return std::type_index(m_command.type());
}

//------------------------------------------------------------------------------
inline bool quetzal::stratum_sim::Command::empty() const
{
    return !m_command.has_value();
}

//------------------------------------------------------------------------------
template<typename C>
bool quetzal::stratum_sim::Command::is() const
{
    return m_command.type() == typeid(C);
}

//------------------------------------------------------------------------------
template<typename C>
const C& quetzal::stratum_sim::Command::get() const
{
    const C* pcommand = try_get<C>();
    assert(pcommand != nullptr);
    return *pcommand;
}

//------------------------------------------------------------------------------
template<typename C>
const C* quetzal::stratum_sim::Command::try_get() const
{
    return std::any_cast<C>(&m_command);
}

#endif // QUETZAL_STRATUM_SIM_COMMAND_HPP
