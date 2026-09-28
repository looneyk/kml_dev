//------------------------------------------------------------------------------
// stratum_sim
// Player.cpp
//------------------------------------------------------------------------------

#include "Player.hpp"

namespace
{

    static constexpr int player_speed = 100;

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::Player::Player(World& world) :
    m_entity(world.entities().entity().add<Blocking>().add<ControlledActor>().set<Speed>(Speed {player_speed}).set<PersistentId>(world.next_id()))
{
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::Player::entity() const
{
    return m_entity;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Player::set_intent(const Intent& intent)
{
    m_entity.set<Intent>(intent);
    return;
}
