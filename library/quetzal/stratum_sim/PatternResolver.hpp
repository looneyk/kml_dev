#if !defined(QUETZAL_STRATUM_SIM_PATTERN_RESOLVER_HPP)
#define QUETZAL_STRATUM_SIM_PATTERN_RESOLVER_HPP
//------------------------------------------------------------------------------
// stratum_sim
// PatternResolver.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/KeyValueStore.hpp"
#include "quetzal/expression/Parser.hpp"
#include "quetzal/expression/SymbolTable.hpp"
#include "quetzal/math/Random.hpp"
#include <optional>
#include <string>
#include <string_view>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Given key, value pairs, evaluate a pattern expression, substituting variables in the pattern expression with their values
    // Patterns are expressions prefixed with '&', and a value prefixed with '&&' is literal text that starts with '&'
    class PatternResolver
    {
    public:

        using random_type = math::Random<math::portable_random_engine_type>;

        PatternResolver(expression::SymbolTable symbol_table, random_type random);
        PatternResolver(const PatternResolver&) = delete;
        PatternResolver(PatternResolver&&) = default;
        ~PatternResolver() = default;

        PatternResolver& operator=(const PatternResolver&) = delete;
        PatternResolver& operator=(PatternResolver&&) = default;

        // Returns the value unchanged unless it is a pattern or starts with "&&"; key is for the error message
        // Returns std::nullopt for the pattern &null
        // Throws std::runtime_error for a pattern that is not a valid expression given the symbols
        std::optional<std::string> resolve(std::string_view key, std::string_view value);

        // Returns a new store with each entry in store with a pattern resolved or copied unchanged
        // An entry whose pattern is &null is omitted
        // Throws std::runtime_error for a pattern that is not a valid expression given the symbols
        KeyValueStore resolve(const KeyValueStore& store);

    private:

        expression::Parser m_parser;
        expression::SymbolTable m_symbol_table;
        random_type m_random;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_PATTERN_RESOLVER_HPP
