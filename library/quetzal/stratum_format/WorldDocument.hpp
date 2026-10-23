#if !defined(QUETZAL_STRATUM_FORMAT_WORLDDOCUMENT_HPP)
#define QUETZAL_STRATUM_FORMAT_WORLDDOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// WorldDocument.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <map>
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
    // A region in the world file: spec is the region spec it was created from
    struct WorldRegion
    {
        stored_entity_id_type id = 0;
        raw_values_type components;
        raw_values_type spec;
        std::vector<ZoneReference> zones;
    };

    //--------------------------------------------------------------------------
    // The data structure analog of a world file. entity_zones is the world index: the zone of each entity that something
    // refers to
    struct WorldDocument
    {
        std::string version;
        std::vector<WorldRegion> regions;
        std::map<stored_entity_id_type, stored_entity_id_type> entity_zones; // <entity_id, zone_id>
    };

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_WORLDDOCUMENT_HPP
