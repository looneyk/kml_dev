#if !defined(QUETZAL_STRATUM_SIM_SEEDS_HPP)
#define QUETZAL_STRATUM_SIM_SEEDS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// seeds.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <cstdint>
#include <span>

namespace quetzal::stratum_sim
{

    // Returns a seed derived from the seed and the values
    seed_type derived_seed(seed_type seed, std::span<const std::uint64_t> values);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SEEDS_HPP
