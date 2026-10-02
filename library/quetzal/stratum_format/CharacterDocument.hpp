#if !defined(QUETZAL_STRATUM_FORMAT_CHARACTERDOCUMENT_HPP)
#define QUETZAL_STRATUM_FORMAT_CHARACTERDOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// CharacterDocument.hpp
//------------------------------------------------------------------------------

#include "Document.hpp"
#include "types.hpp"
#include <string>
#include <vector>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    // The data structure analog of a character file. The inventory's entities are in the form of a zone's entities
    struct CharacterDocument
    {
        std::string version;
        stored_entity_id_type id = 0;
        raw_values_type components;
        stored_entity_id_type zone_id = 0;
        discrete_directed_point_type placement;
        std::vector<Entity> inventory;
    };

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_CHARACTERDOCUMENT_HPP
