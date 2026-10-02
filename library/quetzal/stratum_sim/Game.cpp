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
    m_generation(),
    m_world(m_counting_id_allocator, m_random, m_generation, zone_count_max),
    m_character(m_world),
    m_session(m_world, m_character),
    m_starting_region(m_world.create_region(RegionSpec {}))
{
    register_game_types(m_world);
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Game::menu_open()
{
    m_session.pause();
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Game::menu_close()
{
    m_session.resume();
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Game::clear()
{
    m_session.clear();
    m_character.clear();
    m_world.clear();
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Game::create(const RegionSpec& region_spec, const CharacterSpec& character_spec)
{
    clear();
    m_character.create(character_spec);
    m_world.create(region_spec);
    m_session.start();
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Game::open(std::string_view directory)
{
    clear();
    m_world.open(directory);
    m_character.open(directory);
    m_session.start();
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Game::save(std::string_view directory)
{
    m_character.save(directory);
    m_world.save(directory);
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Game::remove(std::string_view directory)
{
    directory; // ...

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Game::quit()
{
    clear();
    return;
}

//------------------------------------------------------------------------------
std::vector<quetzal::stratum_sim::SavedGameDetails> quetzal::stratum_sim::Game::saved_games()
{
    // From the saved game directory in config, return all the subdirecties ...

    return {};
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Generation& quetzal::stratum_sim::Game::generation()
{
    return m_generation;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Generation& quetzal::stratum_sim::Game::generation() const
{
    return m_generation;
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
quetzal::stratum_sim::Character& quetzal::stratum_sim::Game::character()
{
    return m_character;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Character& quetzal::stratum_sim::Game::character() const
{
    return m_character;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::Game::starting_region() const
{
    return m_starting_region;
}
