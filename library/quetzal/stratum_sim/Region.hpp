#if !defined(QUETZAL_STRATUM_SIM_REGION_HPP)
#define QUETZAL_STRATUM_SIM_REGION_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Region.hpp
//------------------------------------------------------------------------------

#include "PersistentId.hpp"
#include "RegionSpec.hpp"
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace quetzal::stratum_sim
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
        RegionSpec region_spec;
        std::vector<PersistentId> zone_ids; // The zones that make up this region
        std::map<std::string, PersistentId> named_zone_ids; // <name, zone_id>
        std::size_t name_counter = 0; // The last suffix given to a name of its zones that was taken
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_REGION_HPP
