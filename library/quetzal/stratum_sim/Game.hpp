#if !defined(QUETZAL_STRATUM_SIM_GAME_HPP)
#define QUETZAL_STRATUM_SIM_GAME_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Game.hpp
//------------------------------------------------------------------------------

#include "IdAllocator.hpp"
#include "Character.hpp"
#include "Config.hpp"
#include "Generation.hpp"
#include "SavedGameDetails.hpp"
#include "Session.hpp"
#include "Settings.hpp"
#include "World.hpp"
#include <filesystem>
#include <optional>
#include <random>
#include <string>
#include <string_view>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Registers the game's entity types, such as a door that is closed until opened. Temporary, until the game's types load from data
    void register_game_types(World& world);

    //--------------------------------------------------------------------------
    // What made a save: the player's choice, or an auto or quick save, each of which is a track of its own
    enum class SaveType
    {
        Explicit,
        Auto,
        Quick
    };

    //--------------------------------------------------------------------------
    // The one owner of a game's world, session and character
    class Game
    {
    public:

        explicit Game(const Config& config);
        Game(const Game&) = delete;
        Game(Game&&) = delete;
        ~Game() = default;

        Game& operator=(const Game&) = delete;
        Game& operator=(Game&&) = delete;

        // Game state control and menu operations

        void menu_open();
        void menu_close();
        void clear();
        // Clears, then creates a game whose directory in the working directory is named name, and starts play at its first
        // zone's entry point. Throws std::runtime_error if a game of that name exists in the working directory, and passes
        // on what creating the world throws, leaving no game in progress
        void create(std::string_view name, const RegionSpec& region_spec, const CharacterSpec& character_spec); // starting specs, maybe read from settings instead ...
        void open(std::string_view directory);

        // Writes game.json, the character's and the world's files, and every loaded zone into the game directory, then copies
        // it to a new save directory, <character name>_<save time>, with _autosave or _quicksave appended for those save
        // types, waiting for the next second if one of that name exists. A character in the name that cannot be in a
        // directory name is replaced by '_'. Returns the save's name. Requires a game in progress whose character is alive
        // and named. Throws std::runtime_error if a file cannot be written
        std::string save(SaveType save_type);
        void remove(std::string_view directory);
        void quit();

        std::vector<SavedGameDetails> saved_games();

        // Accessors

        const Config& config() const;

        Settings& settings();
        const Settings& settings() const;

        Generation& generation();
        const Generation& generation() const;

        World& world();
        const World& world() const;

        Session& session();
        const Session& session() const;

        Character& character();
        const Character& character() const;

    private:

        // Some of these properties, especially Config, are only of importance to the host ...
        Config m_config; // Immutable properties like file locations, physical hardware configuration, ...
        Settings m_settings; // User settings, graphics, audio, gameplay, control mapping, ...

        // In construction order, each referring to those above it
        CountingIdAllocator m_counting_id_allocator;
        random_type m_random; // Seeded from std::random_device
        Generation m_generation;
        World m_world;
        Character m_character;
        Session m_session;

        // The game in progress's directory in the working directory, and the seed its engine was seeded from
        std::optional<std::filesystem::path> m_game_directory;
        std::random_device::result_type m_seed;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_GAME_HPP
