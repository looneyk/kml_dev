#if !defined(QUETZAL_STRATUM_SIM_CHARACTER_HPP)
#define QUETZAL_STRATUM_SIM_CHARACTER_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Character.hpp
//------------------------------------------------------------------------------

#include "CharacterSpec.hpp"
#include "Command.hpp"
#include "Intent.hpp"
#include "World.hpp"
#include <flecs.h>
#include <filesystem>
#include <string_view>
#include <utility>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Where a character file places the character: the persistent id of its zone, and its cell and orientation there
    struct CharacterPlacement
    {
        PersistentId zone_id;
        discrete_point_type cell_position;
        discrete_vector_type orientation;
    };

    //--------------------------------------------------------------------------
    class Character
    {
    public:

        explicit Character(World& world);
        Character(const Character&) = delete;
        Character(Character&&) = delete;
        ~Character() = default;

        Character& operator=(const Character&) = delete;
        Character& operator=(Character&&) = delete;

        // Character management

        void clear();

        // Makes the character's entity, with a new persistent id and the spec's name, in no zone
        void create(const CharacterSpec& character_spec);
        // Makes the character's entity from character.json in the game directory, in no zone, and returns its placement
        // Throws std::runtime_error if the file cannot be read or the world cannot build the character
        CharacterPlacement open(const std::filesystem::path& game_directory);

        // Writes character.json into the game directory. Requires a character in a zone
        void save(const std::filesystem::path& game_directory) const;

        flecs::entity entity() const;

        // Sets the movement and facing direction the character is trying to take, which persist until changed
        void set_intent(const Intent& intent);

        // Submits a one-shot action to be performed the next time it is ready
        template<typename C>
        void submit(C action);

    private:

        World& m_world;
        flecs::entity m_entity; // Null until create, and after clear
    };

} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
template<typename C>
void quetzal::stratum_sim::Character::submit(C action)
{
    m_entity.set<PendingCommand>(PendingCommand{Command(static_cast<id_type>(m_entity.id()), std::move(action))});
    return;
}

#endif // QUETZAL_STRATUM_SIM_CHARACTER_HPP
