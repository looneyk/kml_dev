#if !defined(QUETZAL_STRATUM_SIM_MAKE_DOCUMENT_HPP)
#define QUETZAL_STRATUM_SIM_MAKE_DOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// make_document.hpp
//------------------------------------------------------------------------------

#include <flecs.h>

namespace quetzal::stratum_format
{
    struct CharacterDocument;
    struct Document;
}

namespace quetzal::stratum_sim
{

    // Returns a loaded zone as a saved zone's document, the reverse of load_layout. The geometry is the zone's ZoneDocument's,
    // and the rest is the zone as it is now: its persistent id and level, its access points, the component objects of the
    // layout and of each room, wall, flat and flat override, and every entity in the zone's subtree but the layout entities,
    // in persistent id order. The character, which the game owns, is in no zone's subtree
    stratum_format::Document make_document(flecs::entity zone);

    // Returns a loaded zone as a saved zone's document with its geometry rebuilt from the zone's layout, for a zone that has
    // no ZoneDocument
    stratum_format::Document rebuild_document(flecs::entity zone);

    // Returns the player's character as a character document: its name, its components, its zone, its cell
    // and orientation there, and every entity in its subtree, in persistent id order, as its inventory. Requires a named character
    stratum_format::CharacterDocument make_character_document(flecs::entity character);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_MAKE_DOCUMENT_HPP
