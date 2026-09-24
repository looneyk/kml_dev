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
    m_entity(world.entities().entity().add<Blocking>().set<Speed>(Speed {player_speed}).set<PersistentId>(world.next_id()))
{
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::Player::entity() const
{
    return m_entity;
}
