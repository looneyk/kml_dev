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

    constexpr char pattern_prefix = '&';

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::PatternResolver::PatternResolver(expression::SymbolTable symbol_table, random_type random) :
    m_parser(),
    m_symbol_table(std::move(symbol_table)),
    m_random(std::move(random))
{
}

//------------------------------------------------------------------------------
std::optional<std::string> quetzal::stratum_sim::PatternResolver::operator()(std::string_view key, std::string_view value)
{
    if (!value.starts_with(pattern_prefix))
    {
        return std::string(value);
    }

    if (value.substr(1).starts_with(pattern_prefix))
    {
        return std::string(value.substr(1));
    }

    const expression::Parser::result_type pattern = m_parser.parse(value.substr(1), m_symbol_table);
    if (!pattern.has_value())
    {
        const std::string message = std::format("the pattern '{}' of {} is not an expression given its symbols: {}", value, key, pattern.error().message);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    switch (pattern->node().type)
    {
        case expression::DataType::Null:
            return std::nullopt;

        case expression::DataType::Number:
            return std::format("{}", pattern->evaluate<double>(m_symbol_table));

        case expression::DataType::String:
            return pattern->evaluate<std::string>(m_symbol_table);
    }

    return std::nullopt;
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
