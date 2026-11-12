#if !defined(QUETZAL_STRATUM_FORMAT_WORLDDOCUMENT_HPP)
#define QUETZAL_STRATUM_FORMAT_WORLDDOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// WorldDocument.hpp
//------------------------------------------------------------------------------

#include "RegionSpec.hpp"
#include "types.hpp"
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    struct WorldDocument
    {

        struct Zone
        {
            stored_entity_id_type id = 0;
            std::string file;
            std::optional<zone_position_type> position;
            std::optional<std::string> name;
        };

        struct Region
        {
            stored_entity_id_type id = 0;
            std::optional<stored_entity_id_type> parent_id;
            std::string name;
            std::optional<std::string> display_name;
            std::size_t name_counter = 0;
            raw_values_type components;
            RegionSpec spec;
            std::vector<Zone> zones;
        };

        struct Worldspace
        {
            stored_entity_id_type id = 0;
            std::string name;
            std::vector<Region> regions; // The top-level region first, and each region before its subregions
        };

        std::string version;
        std::size_t name_counter = 0;
        std::vector<Worldspace> worldspaces;
        std::map<stored_entity_id_type, stored_entity_id_type> entity_zones; // <entity_id, zone_id>
    };

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_WORLDDOCUMENT_HPP
