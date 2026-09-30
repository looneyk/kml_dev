#if !defined(QUETZAL_STRATUM_SIM_GAME_HPP)
#define QUETZAL_STRATUM_SIM_GAME_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Game.hpp
//------------------------------------------------------------------------------

#include "IdAllocator.hpp"
#include "Character.hpp"
#include "Config.hpp"
#include "SavedGameDetails.hpp"
#include "Session.hpp"
#include "Settings.hpp"
#include "World.hpp"

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

        Game();
        Game(const Game&) = delete;
        Game(Game&&) = delete;
        ~Game() = default;

        Game& operator=(const Game&) = delete;
        Game& operator=(Game&&) = delete;

        // Game state control and menu operations

        void menu_open();
        void menu_close();
        void clear();
        void create(const RegionSpec& region_spec, const CharacterSpec& character_spec); // starting specs, maybe read from settings instead ...
        void open(std::string_view directory);
        void save(std::string_view directory);
        void remove(std::string_view directory);
        void quit();

        std::vector<SavedGameDetails> saved_games();

        // Accessors

        const Config& config() const;

        Settings& settings();
        const Settings& settings() const;

        World& world();
        const World& world() const;

        Session& session();
        const Session& session() const;

        Character& character();
        const Character& character() const;

        // The region a new game's zones are made in. Temporary, until a new game chooses its region
        flecs::entity starting_region() const;

    private:

        // Some of these properties, especially Config, are only of importance to the host ...
        Config m_config; // Immutable properties like file locations, physical hardware configuration, ...
        Settings m_settings; // User settings, graphics, audio, gameplay, control mapping, ...

        // In construction order, each referring to those above it
        CountingIdAllocator m_counting_id_allocator;
        random_type m_random; // Seeded from std::random_device
        World m_world;
        Character m_character;
        Session m_session;
        flecs::entity m_starting_region;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_GAME_HPP
