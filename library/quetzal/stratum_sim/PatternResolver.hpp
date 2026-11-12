#if !defined(QUETZAL_STRATUM_SIM_PATTERN_RESOLVER_HPP)
#define QUETZAL_STRATUM_SIM_PATTERN_RESOLVER_HPP
//------------------------------------------------------------------------------
// stratum_sim
// PatternResolver.hpp
//------------------------------------------------------------------------------

#include "ScriptLimits.hpp"
#include "ScriptSandbox.hpp"
#include "types.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Given key, value pairs, evaluate a pattern expression, substituting variables in the pattern expression with their symbol values
    // Patterns are expressions prefixed with '&', with a prefix of "&&" indicating literal text that starts with '&'
    class PatternResolver
    {
    public:

        using random_type = ScriptSandbox::random_type;

        // The symbols are the globals of the sandbox the patterns are evaluated in
        // Throws std::runtime_error if a symbol is an entry of another symbol that is not a table
        PatternResolver(const KeyValueStore& symbols, random_type random, seed_type world_seed, const ScriptLimits& limits);

        // Resolve a pattern value against the given symbols or return it unchanged if value is not a pattern
        // A prefix of "&&" represents a literal '&'
        // Returns std::nullopt for a pattern with a value of nil
        // key is for the error message
        // Throws std::runtime_error for a pattern that does not compile, fails, or has a table or a function as its value
        std::optional<std::string> operator()(std::string_view key, std::string_view value);

        // Returns a new store with each entry in store with a pattern resolved or copied unchanged
        // An entry whose pattern's value is nil is omitted
        // Throws std::runtime_error as a single value's resolution does
        KeyValueStore operator()(const KeyValueStore& store);

    private:

        ScriptSandbox m_sandbox;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_PATTERN_RESOLVER_HPP
