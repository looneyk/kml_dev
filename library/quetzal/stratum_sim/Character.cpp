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
void quetzal::stratum_sim::Character::clear()
{
    // ...

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Character::create(const CharacterSpec& character_spec)
{
    character_spec; // create character entity ...

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Character::open(std::string_view directory)
{
    directory; // ...

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Character::save(std::string_view directory)
{
    directory; // ...

    return;
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
