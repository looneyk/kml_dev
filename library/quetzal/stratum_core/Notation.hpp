#if !defined(QUETZAL_STRATUM_CORE_NOTATION_HPP)
#define QUETZAL_STRATUM_CORE_NOTATION_HPP
//------------------------------------------------------------------------------
// stratum_core
// Notation.hpp
//------------------------------------------------------------------------------

#include <optional>
#include <string_view>

namespace quetzal::stratum_core
{

    //--------------------------------------------------------------------------
    // Which glyph vocabulary the grid rows are written in
    enum class Notation
    {
        Block,
        Wall
    };

    std::string_view name(Notation notation);
    std::optional<Notation> to_notation(std::string_view name);

} // namespace quetzal::stratum_core

#endif // QUETZAL_STRATUM_CORE_NOTATION_HPP
