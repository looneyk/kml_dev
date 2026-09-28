//------------------------------------------------------------------------------
// stratum_sim
// Game.cpp
//------------------------------------------------------------------------------

#include "Game.hpp"
#include "attachments.hpp"

namespace
{

    // Temporary: until the game's config file supplies it
    static constexpr size_t zone_count_max = 8;

} // namespace

//------------------------------------------------------------------------------
void quetzal::stratum_sim::register_game_types(World& world)
{
    flecs::world& entities = world.entities();

    create_entity_type(entities, "door").set<Access>(Access {AccessState::Closed});

    return;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Game::Game() :
    m_counting_id_allocator(PersistentId {}),
    m_random(),
    m_world(m_counting_id_allocator, m_random, zone_count_max),
    m_player(m_world),
    m_session(m_world, m_player),
    m_starting_region(m_world.create_region())
{
    register_game_types(m_world);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::World& quetzal::stratum_sim::Game::world()
{
    return m_world;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::World& quetzal::stratum_sim::Game::world() const
{
    return m_world;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Session& quetzal::stratum_sim::Game::session()
{
    return m_session;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Session& quetzal::stratum_sim::Game::session() const
{
    return m_session;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Player& quetzal::stratum_sim::Game::player()
{
    return m_player;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Player& quetzal::stratum_sim::Game::player() const
{
    return m_player;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::Game::starting_region() const
{
    return m_starting_region;
}
