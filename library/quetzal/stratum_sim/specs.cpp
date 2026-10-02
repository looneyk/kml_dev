//------------------------------------------------------------------------------
// stratum_sim
// specs.cpp
//------------------------------------------------------------------------------

#include "specs.hpp"
#include <algorithm>

//------------------------------------------------------------------------------
std::vector<std::string> quetzal::stratum_sim::find_unknown_keys(const KeyValueStore& spec, std::span<const std::string_view> known_keys)
{
    std::vector<std::string> unknown_keys;

    for (const auto& [key, value] : spec)
    {
        if (std::ranges::find(known_keys, key) == known_keys.end())
        {
            unknown_keys.push_back(key);
        }
    }

    return unknown_keys;
}

//------------------------------------------------------------------------------
quetzal::KeyValueStore quetzal::stratum_sim::select_prefixed(const KeyValueStore& spec, std::string_view prefix)
{
    KeyValueStore selected;

    for (const auto& [key, value] : spec)
    {
        if (key.starts_with(prefix))
        {
            selected.insert(key.substr(prefix.size()), value);
        }
    }

    return selected;
}
