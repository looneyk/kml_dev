#if !defined(QUETZAL_STRATUM_FORMAT_WORLDDOCUMENT_HPP)
#define QUETZAL_STRATUM_FORMAT_WORLDDOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// WorldDocument.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/stratum_core/Worldspace.hpp"
#include "quetzal/stratum_core/Zone.hpp"
#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    struct WorldDocument
    {
        using zones_type = std::map<stratum_core::PersistentId, stratum_core::Zone>; // <zone_id, zone>

        std::string version;
        std::vector<stratum_core::Worldspace> worldspaces;
        zones_type zones;
        std::map<stored_entity_id_type, stored_entity_id_type> entity_zones; // <entity_id, zone_id>
    };

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_WORLDDOCUMENT_HPP
