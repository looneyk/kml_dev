#if !defined(QUETZAL_STRATUM_SIM_SEEDS_HPP)
#define QUETZAL_STRATUM_SIM_SEEDS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// seeds.hpp
//------------------------------------------------------------------------------

#include "Generation.hpp"
#include "RandomStream.hpp"
#include "types.hpp"
#include <span>

namespace quetzal::stratum_sim
{

    // Returns a seed derived from the seed and the coordinates
    RandomStream::seed_type derived_seed(RandomStream::seed_type seed, std::span<const coord_type> coordinates);

    // Returns the seed from the region spec, or if none then the seed derived from the world seed and the position in the regionspec
    // Throws std::runtime_error if the seed or a coordinate is not an integer
    RandomStream::seed_type region_seed(RandomStream::seed_type world_seed, const RegionSpec& region_spec);

    // Returns the seed from the zone spec, or if none then the seed derived from the world seed and the position in the zone spec
    // Throws std::runtime_error if the seed or a coordinate is not an integer
    RandomStream::seed_type zone_seed(RandomStream::seed_type region_seed, const ZoneSpec& zone_spec);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SEEDS_HPP
