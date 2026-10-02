//------------------------------------------------------------------------------
// stratum_sim
// Game.cpp
//------------------------------------------------------------------------------

#include "Game.hpp"
#include "attachments.hpp"
#include "DisplayName.hpp"
#include "quetzal/stratum_format/GameDocument.hpp"
#include "quetzal/stratum_format/saved_game_format.hpp"
#include "quetzal/stratum_format/write_game.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <random>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <cassert>

namespace
{

    constexpr std::string_view game_filename = "game.json";

    // Windows' reserved characters, so a name that one platform accepts every platform accepts
    constexpr std::string_view unnamable_characters = "<>:\"/\\|?*";

    //--------------------------------------------------------------------------
    // The time in ISO 8601 basic format, local to the player
    std::string save_time(std::chrono::sys_seconds time)
    {
        return std::format("{:%Y%m%dT%H%M%S}", std::chrono::zoned_time(std::chrono::current_zone(), time));
    }

    //--------------------------------------------------------------------------
    std::string_view save_suffix(quetzal::stratum_sim::SaveType save_type)
    {
        switch (save_type)
        {
        case quetzal::stratum_sim::SaveType::Explicit:
            return "";
        case quetzal::stratum_sim::SaveType::Auto:
            return "_autosave";
        case quetzal::stratum_sim::SaveType::Quick:
            return "_quicksave";
        }

        assert(false);
        return "";
    }

    //--------------------------------------------------------------------------
    // The name with each character that cannot be in a directory name, reserved or a control character, replaced by '_'
    std::string make_directory_name(std::string_view name)
    {
        std::string directory_name(name);
        std::ranges::replace_if(directory_name, [](char c)
        {
            return unnamable_characters.find(c) != std::string_view::npos || static_cast<unsigned char>(c) < 0x20;
        }, '_');

        return directory_name;
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::stratum_sim::register_game_types(World& world)
{
    flecs::world& entities = world.entities();

    create_entity_type(entities, "door").set<Access>(Access {AccessState::Closed});

    return;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Game::Game(const Config& config) :
    m_config(config),
    m_settings(),
    m_counting_id_allocator(PersistentId {}),
    m_random(),
    m_generation(),
    m_world(m_counting_id_allocator, m_random, m_generation, m_config.zone_count_max),
    m_character(m_world),
    m_session(m_world, m_character),
    m_game_directory(),
    m_seed(0)
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
    m_game_directory.reset();
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Game::create(std::string_view name, const RegionSpec& region_spec, const CharacterSpec& character_spec)
{
    clear();

    m_counting_id_allocator = CountingIdAllocator(PersistentId {});
    m_seed = std::random_device {}();
    m_random.engine().seed(m_seed);

    std::filesystem::path game_directory = m_config.working_directory / name;

    if (!std::filesystem::create_directories(game_directory))
    {
        throw std::runtime_error(std::format("a game named '{}' already exists in {}", name, m_config.working_directory.string()));
    }

    try
    {
        m_character.create(character_spec);
        m_session.start(m_world.create(region_spec, game_directory));
    }
    catch (...)
    {
        clear();
        std::filesystem::remove_all(game_directory);
        throw;
    }

    m_game_directory = game_directory;

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Game::open(std::string_view directory)
{
    clear();
    m_world.open(directory);
    m_character.open(directory);
    return;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_sim::Game::save(SaveType save_type)
{
    assert(m_game_directory.has_value() && m_session.loaded() && !m_session.over());

    const auto* display_name = m_character.entity().try_get<DisplayName>();
    assert(display_name != nullptr && !display_name->name.empty());
    std::string character_name = make_directory_name(display_name->name);

    std::ostringstream engine_state;
    engine_state << m_random.engine();

    stratum_format::GameDocument document;
    document.version = std::string(stratum_format::saved_game_format_version);
    document.next_id = m_counting_id_allocator.next().id;
    document.seed = std::to_string(m_seed);
    document.engines["gameplay"] = engine_state.str();

    stratum_format::write(*m_game_directory / game_filename, document);
    m_character.save(*m_game_directory);
    m_world.save();

    std::string save_name;

    for (;;)
    {
        std::chrono::sys_seconds now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
        save_name = std::format("{}_{}{}", character_name, save_time(now), save_suffix(save_type));

        if (!std::filesystem::exists(m_config.saves_directory / save_name))
        {
            break;
        }

        std::this_thread::sleep_until(now + std::chrono::seconds(1));
    }

    std::filesystem::create_directories(m_config.saves_directory);
    std::filesystem::copy(*m_game_directory, m_config.saves_directory / save_name, std::filesystem::copy_options::recursive);

    return save_name;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Game::remove(std::string_view directory)
{
    // assert that this is a legitimate save directory first for safety ...
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
const quetzal::stratum_sim::Config& quetzal::stratum_sim::Game::config() const
{
    return m_config;
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
