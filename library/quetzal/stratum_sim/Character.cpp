//------------------------------------------------------------------------------
// stratum_sim
// Character.cpp
//------------------------------------------------------------------------------

#include "Character.hpp"

namespace
{

    static constexpr int character_speed = 100;

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::Character::Character(World& world) :
    m_entity(world.entities().entity().add<Blocking>().add<ControlledActor>().set<Speed>(Speed {character_speed}).set<PersistentId>(world.next_id()))
{
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::Character::entity() const
{
    return m_entity;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Character::set_intent(const Intent& intent)
{
    m_entity.set<Intent>(intent);
    return;
}
