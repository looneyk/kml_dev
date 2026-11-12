//------------------------------------------------------------------------------
// stratum_sim
// seeds.cpp
//------------------------------------------------------------------------------

#include "seeds.hpp"
#include <array>
#include <random>
#include <vector>

//------------------------------------------------------------------------------
quetzal::stratum_sim::seed_type quetzal::stratum_sim::derived_seed(seed_type seed, std::span<const std::uint64_t> values)
{
    std::vector<std::uint32_t> words {static_cast<std::uint32_t>(seed), static_cast<std::uint32_t>(seed >> 32)};
    for (std::uint64_t value : values)
    {
        words.push_back(static_cast<std::uint32_t>(value));
        words.push_back(static_cast<std::uint32_t>(value >> 32));
    }

    std::seed_seq sequence(words.begin(), words.end());
    std::array<std::uint32_t, 2> derived;
    sequence.generate(derived.begin(), derived.end());

    return (static_cast<seed_type>(derived[1]) << 32) | derived[0];
}
