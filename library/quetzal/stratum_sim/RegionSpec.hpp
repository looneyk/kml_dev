#if !defined(QUETZAL_STRATUM_SIM_REGION_SPEC_HPP)
#define QUETZAL_STRATUM_SIM_REGION_SPEC_HPP
//------------------------------------------------------------------------------
// stratum_sim
// RegionSpec.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include <filesystem>
#include <map>
#include <string>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The position a region's seed derives from, until seeds are drawn hierarchically
    struct RegionPosition
    {
        coord_type x = 0;
        coord_type y = 0;
    };

    //--------------------------------------------------------------------------
    // The description of a region that World creates the region from
    struct RegionSpec // type, terrain, competency level ...
    {
        // The position given by the position keys, 0 where a key is not found
        // Throws std::runtime_error if a coordinate is not an integer
        RegionPosition position() const;

        void set_position(const RegionPosition& position);

        KeyValueStore keys;
    };

    // Returns the region specs by name from the region spec file in the assets directory
    // Throws std::runtime_error if the file cannot be read or the reader rejects it
    std::map<std::string, RegionSpec> load_region_specs(const std::filesystem::path& assets_directory);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_REGION_SPEC_HPP
