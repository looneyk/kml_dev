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

        // Clears the old game and creates a new one with a region/zone and character created from the specs.
        void create(const RegionSpec& region_spec, const CharacterSpec& character_spec,
            std::optional<RandomStream::seed_type> seed = std::nullopt); // starting specs, maybe read from settings instead ...

        // Clears the old game and starts a new one initialized from the saved game identified by save_name
        void open(std::string_view save_name);

        // Saves the game running in the current session
        std::string save(SaveType save_type);

        // Deletes a saved game
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

        // Reads game.json: the id allocator's count, the world seed, and the gameplay random engine's state, restored, or
        // reseeded as the settings choose, the world seed kept either way
        void restore_game_state(const std::filesystem::path& filepath);

        // Sets the world seed, keeps it for saving, seeds the gameplay random engine with it, and logs it
        void seed_world(RandomStream::seed_type seed);

        // Some of these properties, especially Config, are only of importance to the host ...
        Config m_config; // Immutable properties like file locations, physical hardware configuration, ...

        // In construction order, each referring to those above it
        CountingIdAllocator m_counting_id_allocator;
        Generation m_generation;
        World m_world;
        Character m_character;
        Session m_session;

        // The game in progress's directory in the working directory, and the world seed
        std::optional<std::filesystem::path> m_game_directory;
        RandomStream::seed_type m_seed;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_GAME_HPP
