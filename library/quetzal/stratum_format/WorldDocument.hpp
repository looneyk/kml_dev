#if !defined(QUETZAL_STRATUM_FORMAT_WORLDDOCUMENT_HPP)
#define QUETZAL_STRATUM_FORMAT_WORLDDOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// WorldDocument.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    // The data structure analog of a world file. name_counter is the last suffix given to a worldspace's name that was
    // taken, and entity_zones the world index: the zone of each entity that something refers to
    struct WorldDocument
    {

        struct Zone
        {
            stored_entity_id_type id = 0;
            std::string file;
            std::optional<zone_position_type> position;
            std::optional<std::string> name;
        };

        // spec is the region spec the region was created from, and name_counter the last suffix given to a name of its
        // zones that was taken
        struct Region
        {
            stored_entity_id_type id = 0;
            std::string name;
            std::size_t name_counter = 0;
            raw_values_type components;
            KeyValueStore spec;
            std::vector<Zone> zones;
        };

        struct Worldspace
        {
            stored_entity_id_type id = 0;
            std::string name;
            Region region;
        };

        std::string version;
        std::size_t name_counter = 0;
        std::vector<Worldspace> worldspaces;
        std::map<stored_entity_id_type, stored_entity_id_type> entity_zones; // <entity_id, zone_id>
    };

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_WORLDDOCUMENT_HPP
