#if !defined(QUETZAL_STRATUM_SIM_SPECS_HPP)
#define QUETZAL_STRATUM_SIM_SPECS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// specs.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/KeyValueStore.hpp"
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace quetzal::stratum_sim
{

    // Returns the spec's keys that are not among known_keys, in key order
    std::vector<std::string> find_unknown_keys(const KeyValueStore& spec, std::span<const std::string_view> known_keys);

    // Returns the spec's entries whose keys begin with the prefix, with the prefix removed from each key
    KeyValueStore select_prefixed(const KeyValueStore& spec, std::string_view prefix);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SPECS_HPP
