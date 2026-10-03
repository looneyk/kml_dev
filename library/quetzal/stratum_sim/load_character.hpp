#if !defined(QUETZAL_STRATUM_SIM_LOAD_CHARACTER_HPP)
#define QUETZAL_STRATUM_SIM_LOAD_CHARACTER_HPP
//------------------------------------------------------------------------------
// stratum_sim
// load_character.hpp
//------------------------------------------------------------------------------

#include <flecs.h>
#include <optional>
#include <string>

namespace quetzal::stratum_format
{
    struct CharacterDocument;
}

namespace quetzal::stratum_sim
{

    // Checks, before anything is built, that the world can build the character from the document. Returns the first reason
    // it cannot, or none: a component object of the character or of an entity of its inventory that has no component
    // registered under its name or does not fit it
    std::optional<std::string> check_buildable(const flecs::world& world, const stratum_format::CharacterDocument& document);

    // Fills the character's entity from a document that check_buildable accepts: its persistent id and its component objects
    // decoded, and each entity of its inventory, with its persistent id, its type, its parent, its site on that parent, and
    // its component objects. The character is in no zone
    void load_character(flecs::entity character, const stratum_format::CharacterDocument& document);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_LOAD_CHARACTER_HPP
