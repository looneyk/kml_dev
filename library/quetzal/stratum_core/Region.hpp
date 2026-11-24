#if !defined(QUETZAL_STRATUM_CORE_REGION_HPP)
#define QUETZAL_STRATUM_CORE_REGION_HPP
//------------------------------------------------------------------------------
// stratum_core
// Region.hpp
//------------------------------------------------------------------------------

#include "PersistentId.hpp"
#include "RegionSpec.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace quetzal::stratum_core
{

    //--------------------------------------------------------------------------
    // A group of contiguous and/or interconnected zones of one worldspace
    struct Region
    {
        PersistentId region_id;
        std::optional<PersistentId> parent_id;
        std::vector<PersistentId> subregion_ids;
        std::string name;
        std::optional<std::string> display_name;
        KeyValueStore components;
        RegionSpec region_spec;
        std::vector<PersistentId> zone_ids;
    };

} // namespace quetzal::stratum_core

#endif // QUETZAL_STRATUM_CORE_REGION_HPP
