//------------------------------------------------------------------------------
// stratum_sim
// seeds.cpp
//------------------------------------------------------------------------------

#include "seeds.hpp"
#include "positions.hpp"
#include <array>
#include <cstdint>
#include <random>
#include <vector>

//------------------------------------------------------------------------------
quetzal::stratum_sim::RandomStream::seed_type quetzal::stratum_sim::derived_seed(RandomStream::seed_type seed,
    std::span<const coord_type> coordinates)
{
    std::vector<std::uint32_t> words {static_cast<std::uint32_t>(seed), static_cast<std::uint32_t>(seed >> 32)};
    for (coord_type coordinate : coordinates)
    {
        words.push_back(static_cast<std::uint32_t>(coordinate));
    }

    std::seed_seq sequence(words.begin(), words.end());
    std::array<std::uint32_t, 2> derived;
    sequence.generate(derived.begin(), derived.end());

    return (static_cast<RandomStream::seed_type>(derived[1]) << 32) | derived[0];
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::RandomStream::seed_type quetzal::stratum_sim::region_seed(RandomStream::seed_type world_seed, const RegionSpec& region_spec)
{
    if (std::optional<RandomStream::seed_type> seed = region_spec.value<RandomStream::seed_type>(seed_key))
    {
        return *seed;
    }

    const RegionPosition position = region_position(region_spec);
    const std::array<coord_type, 2> coordinates {position.x, position.y};
    return derived_seed(world_seed, coordinates);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::RandomStream::seed_type quetzal::stratum_sim::zone_seed(RandomStream::seed_type region_seed, const ZoneSpec& zone_spec)
{
    if (std::optional<RandomStream::seed_type> seed = zone_spec.value<RandomStream::seed_type>(seed_key))
    {
        return *seed;
    }

    const ZonePosition position = zone_position(zone_spec);
    const std::array<coord_type, 3> coordinates {position.x, position.y, position.z};
    return derived_seed(region_seed, coordinates);
}
