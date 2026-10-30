//------------------------------------------------------------------------------
// stratum_sim
// Game.cpp
//------------------------------------------------------------------------------

#include "Game.hpp"
#include "attachments.hpp"
#include "DiscreteLayout.hpp"
#include "DisplayName.hpp"
#include "quetzal/log/Log.hpp"
#include "setting_definitions.hpp"
#include "quetzal/settings/Settings.hpp"
#include "quetzal/settings/SettingsSnapshot.hpp"
#include "quetzal/stratum_format/GameDocument.hpp"
#include "quetzal/stratum_format/read_character.hpp"
#include "quetzal/stratum_format/read_game.hpp"
#include "quetzal/stratum_format/saved_game_format.hpp"
#include "quetzal/stratum_format/write_game.hpp"
#include <algorithm>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <format>
#include <functional>
#include <random>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <cassert>

namespace
{

    constexpr std::string_view gameplay_random_engine_key = "gameplay";

    // Windows' reserved characters, so a name that one platform accepts every platform accepts
    constexpr std::string_view reserved_path_characters = "<>:\"/\\|?*";

    //--------------------------------------------------------------------------
    // The time in ISO 8601 basic format, local to the player
    std::string format_save_time(std::chrono::sys_seconds time)
    {
        return std::format("{:%Y%m%dT%H%M%S}", std::chrono::zoned_time(std::chrono::current_zone(), time));
    }

    //--------------------------------------------------------------------------
    std::string_view get_save_name_suffix(quetzal::stratum_sim::SaveType save_type)
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
    // Returns the type whose suffix ends the save's name
    quetzal::stratum_sim::SaveType parse_save_type(std::string_view save_name)
    {
        for (quetzal::stratum_sim::SaveType save_type : {quetzal::stratum_sim::SaveType::Auto, quetzal::stratum_sim::SaveType::Quick})
        {
            if (save_name.ends_with(get_save_name_suffix(save_type)))
            {
                return save_type;
            }
        }

        return quetzal::stratum_sim::SaveType::Explicit;
    }

    //--------------------------------------------------------------------------
    // The name with each character that cannot be in a directory name, reserved or a control character, replaced by '_'
    std::string replace_reserved_characters(std::string_view name)
    {
        std::string directory_name(name);
        std::ranges::replace_if(directory_name, [](char c)
        {
            return reserved_path_characters.find(c) != std::string_view::npos || static_cast<unsigned char>(c) < 0x20;
        }, '_');

        return directory_name;
    }

    //--------------------------------------------------------------------------
    // The working directory has only the game in progress
    void clear_working_directory(const quetzal::stratum_sim::Config& config)
    {
        std::filesystem::remove_all(config.working_directory);
        return;
    }

    //--------------------------------------------------------------------------
    // Creates the game directory in the working directory, named by the character, and returns it
    std::filesystem::path create_game_directory(const quetzal::stratum_sim::Config& config, std::string_view character_name)
    {
        std::filesystem::path game_directory = config.working_directory / replace_reserved_characters(character_name);
        std::filesystem::create_directories(game_directory);

        return game_directory;
    }

    //--------------------------------------------------------------------------
    // The character's name, as the player entered it, from the top of the save's character file
    std::string read_saved_character_name(const std::filesystem::path& save_directory)
    {
        return quetzal::stratum_format::read_character_name(save_directory / quetzal::stratum_format::character_filename);
    }

    //--------------------------------------------------------------------------
    // Sets each game setting that the manifest has a value for
    // Throws std::runtime_error if a key is not a game setting or a value is not a valid value of its setting
    void apply_manifest_settings(const quetzal::KeyValueStore& manifest_settings)
    {
        quetzal::settings::Settings& store = quetzal::settings::store();

        for (const auto& [key, value] : manifest_settings)
        {
            if (!store.contains(key) || store.setting(key).scope != quetzal::stratum_sim::game_setting_scope)
            {
                throw std::runtime_error(std::format("the manifest's setting '{}' is not a game setting", key));
            }
        }

        if (!store.initialize_values(manifest_settings, {}))
        {
            throw std::runtime_error("a value of the manifest's settings is outside its setting's range");
        }

        return;
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
quetzal::stratum_sim::Game::Game(const Config& config, const stratum_format::GameManifestDocument& manifest) :
    m_config(config),
    m_manifest(manifest),
    m_counting_id_allocator(PersistentId {}),
    m_generation(),
    m_world(m_counting_id_allocator, m_generation, m_config.zone_count_max),
    m_character(m_world),
    m_session(m_world, m_character),
    m_game_directory(),
    m_seed(0),
    m_notation(manifest.notation)
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
    settings::store().reset(game_setting_scope);
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Game::create(const RegionSpec& region_spec, const CharacterSpec& character_spec, stratum_format::Notation notation,
    std::optional<RandomStream::seed_type> seed)
{
    assert(!character_spec.name.empty());

    clear();
    apply_manifest_settings(m_manifest.settings);
    m_notation = notation;

    m_counting_id_allocator = CountingIdAllocator(PersistentId {});
    seed_world(seed ? *seed : math::random_seed());

    clear_working_directory(m_config);
    std::filesystem::path game_directory = create_game_directory(m_config, character_spec.name);

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
void quetzal::stratum_sim::Game::open(std::string_view save_name)
{
    clear();

    std::filesystem::path save_directory = m_config.saves_directory / save_name;
    if (!std::filesystem::is_directory(save_directory))
    {
        throw std::runtime_error(std::format("no save named '{}' is in {}", save_name, m_config.saves_directory.string()));
    }

    std::string character_name = read_saved_character_name(save_directory);
    clear_working_directory(m_config);
    std::filesystem::path game_directory = create_game_directory(m_config, character_name);
    std::filesystem::copy(save_directory, game_directory, std::filesystem::copy_options::recursive);

    try
    {
        restore_game_state(game_directory / stratum_format::game_filename);
        m_world.open(game_directory);

        CharacterPlacement character_placement = m_character.open(game_directory);
        flecs::entity zone = m_world.load_saved_zone(character_placement.zone_id);
        const DiscreteLayout& layout = layout_as<DiscreteLayout>(m_world.layout(zone));
        vector_type orientation(static_cast<value_type>(character_placement.orientation.x()), static_cast<value_type>(character_placement.orientation.y()),
            value_type(0));

        m_session.start(zone, placement_type(layout.position(character_placement.cell_position), orientation));
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
void quetzal::stratum_sim::Game::restore_game_state(const std::filesystem::path& filepath)
{
    stratum_format::GameDocument document = stratum_format::read_game(filepath);
    m_notation = document.notation;
    m_counting_id_allocator = CountingIdAllocator(PersistentId {document.next_id});

    const char* seed_end = document.seed.data() + document.seed.size();
    if (auto [end, ec] = std::from_chars(document.seed.data(), seed_end, m_seed); ec != std::errc() || end != seed_end)
    {
        throw std::runtime_error(std::format("{}: game.seed: '{}' is not a seed", filepath.string(), document.seed));
    }

    m_world.set_world_seed(m_seed);

    try
    {
        document.settings_snapshot.restore(game_setting_scope, settings::store());
    }
    catch (const std::runtime_error& e)
    {
        throw std::runtime_error(std::format("{}: {}", filepath.string(), e.what()));
    }

    if (settings::store().value<bool>(reseed_on_open_key))
    {
        const RandomStream::seed_type gameplay_seed = math::random_seed();
        gameplay_random().seed(gameplay_seed);
        log::logger().info("stratum_sim", "The gameplay random engine is reseeded with {}", gameplay_seed);
        return;
    }

    auto i = document.random_engines.find(std::string(gameplay_random_engine_key));
    if (i == document.random_engines.end())
    {
        throw std::runtime_error(std::format("{}: game.random_engines: no {} random engine", filepath.string(), gameplay_random_engine_key));
    }

    try
    {
        gameplay_random().set_state(i->second);
    }
    catch (const std::invalid_argument& e)
    {
        throw std::runtime_error(std::format("{}: game.random_engines.{}: {}", filepath.string(), gameplay_random_engine_key, e.what()));
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Game::seed_world(RandomStream::seed_type seed)
{
    m_seed = seed;
    m_world.set_world_seed(seed);
    gameplay_random().seed(seed);
    log::logger().info("stratum_sim", "The world seed is {}", seed);
    return;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_sim::Game::save(SaveType save_type)
{
    assert(m_game_directory.has_value() && m_session.loaded() && !m_session.over());

    const auto* display_name = m_character.entity().try_get<DisplayName>();
    assert(display_name != nullptr && !display_name->name.empty());
    std::string character_name = replace_reserved_characters(display_name->name);

    stratum_format::GameDocument document;
    document.version = std::string(stratum_format::saved_game_format_version);
    document.notation = m_notation;
    document.next_id = m_counting_id_allocator.next().id;
    document.seed = std::to_string(m_seed);
    document.random_engines[std::string(gameplay_random_engine_key)] = gameplay_random().state();
    document.settings_snapshot = settings::SettingsSnapshot::capture(game_setting_scope, settings::store());

    stratum_format::write(*m_game_directory / stratum_format::game_filename, document);
    m_character.save(*m_game_directory);
    m_world.save();

    std::string save_name;

    for (;;)
    {
        std::chrono::sys_seconds now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
        save_name = std::format("{}_{}{}", character_name, format_save_time(now), get_save_name_suffix(save_type));

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
void quetzal::stratum_sim::Game::remove(std::string_view save_name)
{
    // The name is checked before anything is deleted, so no name can reach outside the saves directory or delete a
    // directory that is not a save
    std::filesystem::path save_directory = m_config.saves_directory / save_name;
    bool is_save = !save_name.empty() && save_name.find_first_of("/\\") == std::string_view::npos && save_name != "." && save_name != ".."
        && std::filesystem::is_directory(save_directory) && std::filesystem::is_regular_file(save_directory / stratum_format::character_filename);

    if (!is_save)
    {
        throw std::runtime_error(std::format("'{}' is not a save in {}", save_name, m_config.saves_directory.string()));
    }

    std::filesystem::remove_all(save_directory);
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Game::quit()
{
    clear();
    return;
}

//------------------------------------------------------------------------------
std::vector<quetzal::stratum_sim::SavedGameDetails> quetzal::stratum_sim::Game::saved_games() const
{
    std::vector<SavedGameDetails> details;

    if (!std::filesystem::is_directory(m_config.saves_directory))
    {
        return details;
    }

    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(m_config.saves_directory))
    {
        if (entry.is_directory())
        {
            std::string save_name = entry.path().filename().string();
            details.emplace_back(save_name, read_saved_character_name(entry.path()), std::string_view(),
                std::filesystem::last_write_time(entry.path() / stratum_format::game_filename), parse_save_type(save_name));
        }
    }

    std::ranges::sort(details, std::ranges::greater(), &SavedGameDetails::time);
    return details;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Config& quetzal::stratum_sim::Game::config() const
{
    return m_config;
}

//------------------------------------------------------------------------------
const quetzal::stratum_format::GameManifestDocument& quetzal::stratum_sim::Game::manifest() const
{
    return m_manifest;
}

//------------------------------------------------------------------------------
quetzal::stratum_format::Notation quetzal::stratum_sim::Game::notation() const
{
    return m_notation;
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
