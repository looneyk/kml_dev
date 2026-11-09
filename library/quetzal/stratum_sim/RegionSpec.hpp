#if !defined(QUETZAL_STRATUM_SIM_REGION_SPEC_HPP)
#define QUETZAL_STRATUM_SIM_REGION_SPEC_HPP
//------------------------------------------------------------------------------
// stratum_sim
// RegionSpec.hpp
//------------------------------------------------------------------------------

#include "PatternResolver.hpp"
#include "types.hpp"
#include "quetzal/stratum_format/RegionSpec.hpp"
#include <filesystem>
#include <map>
#include <string>

namespace quetzal::stratum_sim
{

    using stratum_format::RegionSpec;

    //--------------------------------------------------------------------------
    // Deprecated: the position a region's seed derives from, until seeds are drawn hierarchically
    struct RegionPosition
    {
        coord_type x = 0;
        coord_type y = 0;
    };

    // Deprecated: returns the position given by the position keys, 0 where a key is not found
    // Throws std::runtime_error if a coordinate is not an integer
    RegionPosition region_position(const RegionSpec& region_spec);

    // Deprecated
    void set_region_position(RegionSpec& region_spec, const RegionPosition& position);

    // Returns the region spec with the pattern values in its name fields and store entries resolved
    // Zone store entries are excluded and pass through unchanged
    // Throws std::runtime_error for a pattern that is not a valid expression given the resolver's symbols
    RegionSpec resolve_region_spec(const RegionSpec& region_spec, PatternResolver& resolver);

    // Returns the region specs by name from the region spec file in the assets directory
    // Throws std::runtime_error if the file cannot be read or the reader rejects it
    std::map<std::string, RegionSpec> load_region_specs(const std::filesystem::path& assets_directory);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_REGION_SPEC_HPP
