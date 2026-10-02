#if !defined(QUETZAL_STRATUM_FORMAT_WORLDDOCUMENT_HPP)
#define QUETZAL_STRATUM_FORMAT_WORLDDOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// WorldDocument.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <string>
#include <vector>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    // A zone's persistent id and its file, relative to the directory of the world file
    struct ZoneReference
    {
        stored_entity_id_type id = 0;
        std::string file;
    };

    //--------------------------------------------------------------------------
    // A region as the world file has it: spec is the region spec it was created from
    struct WorldRegion
    {
        stored_entity_id_type id = 0;
        raw_values_type components;
        raw_values_type spec;
        std::vector<ZoneReference> zones;
    };

    //--------------------------------------------------------------------------
    // The data structure analog of a world file
    struct WorldDocument
    {
        std::string version;
        std::vector<WorldRegion> regions;
    };

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_WORLDDOCUMENT_HPP
