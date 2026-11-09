//------------------------------------------------------------------------------
// stratum_sim
// PatternResolver.cpp
//------------------------------------------------------------------------------

#include "PatternResolver.hpp"
#include "quetzal/expression/Expression.hpp"
#include "quetzal/log/Log.hpp"
#include <format>
#include <optional>
#include <stdexcept>
#include <utility>

namespace
{

    constexpr char pattern_marker = '&';

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::PatternResolver::PatternResolver(expression::SymbolTable symbol_table, random_type random) :
    m_parser(),
    m_symbol_table(std::move(symbol_table)),
    m_random(std::move(random))
{
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_sim::PatternResolver::resolve(std::string_view key_name, std::string_view value)
{
    if (!value.starts_with(pattern_marker))
    {
        return std::string(value);
    }

    if (value.substr(1).starts_with(pattern_marker))
    {
        return std::string(value.substr(1));
    }

    const std::optional<expression::Expression> pattern = m_parser.parse(value.substr(1), m_symbol_table);
    if (!pattern.has_value())
    {
        const std::string message = std::format("the pattern '{}' of {} is not an expression of its symbols", value, key_name);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    if (pattern->node().type == expression::DataType::String)
    {
        return pattern->evaluate<std::string>(m_symbol_table);
    }

    return std::format("{}", pattern->evaluate<double>(m_symbol_table));
}

//------------------------------------------------------------------------------
quetzal::KeyValueStore quetzal::stratum_sim::PatternResolver::resolve(const KeyValueStore& store)
{
    KeyValueStore resolved;
    for (const auto& [key, value] : store)
    {
        resolved.insert(key, resolve(key, value));
    }

    return resolved;
}
