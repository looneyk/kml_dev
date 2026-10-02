//------------------------------------------------------------------------------
// stratum_sim
// Character.cpp
//------------------------------------------------------------------------------

#include "Character.hpp"
#include "DisplayName.hpp"
#include <cassert>

namespace
{

    static constexpr int character_speed = 100;

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::Character::Character(World& world) :
    m_world(world),
    m_entity()
{
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Character::clear()
{
    // There is no character before the first game is created
    if (m_entity)
    {
        m_entity.destruct();
        m_entity = flecs::entity();
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Character::create(const CharacterSpec& character_spec)
{
    assert(!m_entity);

    m_entity = m_world.entities().entity().add<Blocking>().add<ControlledActor>().set<Speed>(Speed {character_speed})
        .set<PersistentId>(m_world.next_id());

    if (!character_spec.name.empty())
    {
        m_entity.set<DisplayName>(DisplayName {character_spec.name});
    }

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
