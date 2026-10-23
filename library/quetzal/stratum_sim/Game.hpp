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
#include "RandomStream.hpp"
#include "SavedGameDetails.hpp"
#include "Session.hpp"
#include "World.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Registers the game's entity types, such as a door that is closed until opened. Temporary, until the game's types load from data
    void register_game_types(World& world);

    //--------------------------------------------------------------------------
    // The one owner of a game's world, session and character
    class Game
    {
    public:

        // A boolean setting: true if opening a save reseeds the random engine, false if it restores the state saved
        // We need a better general centralized settings configuration for game ...
        static constexpr std::string_view reseed_on_open_key = "gameplay.reseed_on_open";

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
        // Clears, empties the working directory, and creates a game there, its game directory named by the character, then
        // starts play at its first zone's entry point. Seeds the gameplay random engine with the seed, or with one drawn from
        // std::random_device if none is passed, and logs it. Requires a named character. Passes on what creating the world
        // throws, leaving no game in progress
        void create(const RegionSpec& region_spec, const CharacterSpec& character_spec,
            std::optional<RandomStream::seed_type> seed = std::nullopt); // starting specs, maybe read from settings instead ...

        // Clears, empties the working directory, and copies the save there as the game directory, named by the character,
        // then reads it: the id allocator's count, the random engine's state, restored or reseeded as the settings choose,
        // the world, and the character, and starts play in the character's zone at its placement. Throws
        // std::runtime_error if there is no such save or a file in it cannot be read, leaving no game in progress
        void open(std::string_view save_name);

        // Writes game.json, the character's and the world's files, and every loaded zone into the game directory, then copies
        // it to a new save directory, <character name>_<save time>, with _autosave or _quicksave appended for those save
        // types, waiting for the next second if one of that name exists. A character in the name that cannot be in a
        // directory name is replaced by '_'. Returns the save's name. Requires a game in progress whose character is alive
        // and named. Throws std::runtime_error if a file cannot be written
        std::string save(SaveType save_type);

        // Deletes the save, by the name saved_games returns, from the saves directory. Throws std::runtime_error, deleting nothing,
        // unless the name is of a directory directly in the saves directory that has a character file
        void remove(std::string_view save_name);

        void quit();

        // Each save in the saves directory, newest first, with its character's name, the time it was saved, and its type, read from its name.
        // Throws std::runtime_error if a save's character file cannot be read or has no name
        std::vector<SavedGameDetails> saved_games() const;

        // Accessors

        const Config& config() const;

        Generation& generation();
        const Generation& generation() const;

        World& world();
        const World& world() const;

        Session& session();
        const Session& session() const;

        Character& character();
        const Character& character() const;

    private:

        // Reads game.json: the id allocator's count, the seed, and the random engine's state, restored or reseeded as the settings dictate
        void restore_game_state(const std::filesystem::path& filepath);

        // Seeds the gameplay random engine, keeps the seed for saving, and logs it
        void seed_gameplay_random(RandomStream::seed_type seed);

        // Some of these properties, especially Config, are only of importance to the host ...
        Config m_config; // Immutable properties like file locations, physical hardware configuration, ...

        // In construction order, each referring to those above it
        CountingIdAllocator m_counting_id_allocator;
        Generation m_generation;
        World m_world;
        Character m_character;
        Session m_session;

        // The game in progress's directory in the working directory, and the seed the gameplay random engine was seeded from
        std::optional<std::filesystem::path> m_game_directory;
        RandomStream::seed_type m_seed;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_GAME_HPP
