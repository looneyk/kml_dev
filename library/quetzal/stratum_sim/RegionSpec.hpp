#if !defined(QUETZAL_STRATUM_SIM_REGION_SPEC_HPP)
#define QUETZAL_STRATUM_SIM_REGION_SPEC_HPP
//------------------------------------------------------------------------------
// stratum_sim
// RegionSpec.hpp
//------------------------------------------------------------------------------

#include "PatternResolver.hpp"
#include "quetzal/stratum_core/RegionSpec.hpp"
#include <filesystem>
#include <map>
#include <string>

namespace quetzal::stratum_sim
{

    using stratum_core::RegionSpec;

    //--------------------------------------------------------------------------
    // Returns the region spec composed of the base region spec, if any, with the region spec's data fields and store applied over them
    // Throws std::runtime_error if the base region spec is not found
    RegionSpec composed_region_spec(const RegionSpec& region_spec, const std::map<std::string, RegionSpec>& region_specs);

    // Returns the region spec with the pattern values in its name fields and store entries resolved
    // Zone store entries are excluded and pass through unchanged
    // Throws std::runtime_error for a pattern that does not compile, fails, or has a table or a function as its value
    RegionSpec resolve_region_spec(const RegionSpec& region_spec, PatternResolver& resolver);

    // Returns the region specs by name from the region spec file in the assets directory
    // Throws std::runtime_error if the file cannot be read or the reader rejects it
    std::map<std::string, RegionSpec> load_region_specs(const std::filesystem::path& assets_directory);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_REGION_SPEC_HPP
