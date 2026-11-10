//------------------------------------------------------------------------------
// stratum_sim
// PatternResolver.cpp
//------------------------------------------------------------------------------

#include "PatternResolver.hpp"
#include <utility>

namespace
{

    constexpr char pattern_prefix = '&';

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::PatternResolver::PatternResolver(const KeyValueStore& symbols, random_type random, std::uint64_t world_seed, const ScriptLimits& limits) :
    m_sandbox(symbols, std::move(random), world_seed, limits)
{
}

//------------------------------------------------------------------------------
std::optional<std::string> quetzal::stratum_sim::PatternResolver::operator()(std::string_view key, std::string_view value)
{
    if (!value.starts_with(pattern_prefix))
    {
        return std::string(value);
    }

    if (value.substr(1).starts_with(pattern_prefix)) // Begins with &&
    {
        return std::string(value.substr(1));
    }

    return m_sandbox.evaluate(key, value.substr(1));
}

//------------------------------------------------------------------------------
quetzal::KeyValueStore quetzal::stratum_sim::PatternResolver::operator()(const KeyValueStore& store)
{
    KeyValueStore resolved;
    for (const auto& [key, value] : store)
    {
        if (std::optional<std::string> resolved_value = (*this)(key, value); resolved_value.has_value())
        {
            resolved.insert(key, std::move(resolved_value.value()));
        }
    }

    return resolved;
}
