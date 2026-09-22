#if !defined(QUETZAL_STRATUM_FORMAT_NOTATION_HPP)
#define QUETZAL_STRATUM_FORMAT_NOTATION_HPP
//------------------------------------------------------------------------------
// stratum_format
// Notation.hpp
//------------------------------------------------------------------------------

#include <optional>
#include <string_view>

namespace quetzal::stratum_format
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

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_NOTATION_HPP
