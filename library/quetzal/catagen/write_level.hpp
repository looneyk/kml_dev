#if !defined(QUETZAL_CATAGEN_WRITE_LEVEL_HPP)
#define QUETZAL_CATAGEN_WRITE_LEVEL_HPP
//------------------------------------------------------------------------------
// catagen
// write_level.hpp
//------------------------------------------------------------------------------

#include "Level.hpp"
#include "Parameters.hpp"
#include <iosfwd>

namespace quetzal::catagen
{

    // A discrete layout file in the wall notation, the generator's parameters written as the tokens its options take so
    // that the file reproduces its own zone
    // Throws std::bad_optional_access when no seed was assigned, rather than write a file that cannot reproduce its zone
    void write_level(std::ostream& os, const Level& level, const Parameters& parameters);

    // The grid rows alone, a row per line, for a caller with no parameters to write -- printing to a terminal
    void write_grid(std::ostream& os, const Level& level);

} // namespace quetzal::catagen

#endif // QUETZAL_CATAGEN_WRITE_LEVEL_HPP
