#if !defined(QUETZAL_STRATUM_SIM_CHARACTER_HPP)
#define QUETZAL_STRATUM_SIM_CHARACTER_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Character.hpp
//------------------------------------------------------------------------------

#include "Command.hpp"
#include "Intent.hpp"
#include "World.hpp"
#include <flecs.h>
#include <utility>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The entity a host controls
    class Character
    {
    public:

        // Creates the entity, in no zone until one is entered
        explicit Character(World& world);
        Character(const Character&) = delete;
        ~Character() = default;

        Character& operator=(const Character&) = delete;

        flecs::entity entity() const;

        // Sets the movement and facing direction the character is trying to take, which persist until changed
        void set_intent(const Intent& intent);

        // Submits a one-shot action, which the character takes the next time it is ready
        template<typename C>
        void submit(C action);

    private:

        flecs::entity m_entity;
    };

} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
template<typename C>
void quetzal::stratum_sim::Character::submit(C action)
{
    m_entity.set<PendingCommand>(PendingCommand {Command(static_cast<id_type>(m_entity.id()), std::move(action))});
    return;
}

#endif // QUETZAL_STRATUM_SIM_CHARACTER_HPP
