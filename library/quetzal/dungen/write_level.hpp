#if !defined(QUETZAL_DUNGEN_WRITE_LEVEL_HPP)
#define QUETZAL_DUNGEN_WRITE_LEVEL_HPP
//------------------------------------------------------------------------------
// dungen
// write_level.hpp
//------------------------------------------------------------------------------

#include "Level.hpp"
#include "Parameters.hpp"
#include <iosfwd>

namespace quetzal::dungen
{

    // A discrete layout file in the block notation, the generator's parameters written as the tokens its options take so
    // that the file reproduces its own zone. Only the chosen algorithm's group is written
    // Throws std::bad_optional_access when no seed was assigned, rather than write a file that cannot reproduce its zone
    void write_level(std::ostream& os, const Level& level, const Parameters& parameters);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_WRITE_LEVEL_HPP
