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

    // The version of the file format, recorded in the header so that a reader knows what the
    // grid below it means. A leading zero says pre-release: the glyphs and the shape are still
    // free to change, and every level regenerates when they do
    constexpr auto format_version = "0.2";

    // A header row naming the generator, the format version and the parameters of the run,
    // then the grid. A grid row never begins with a letter, so a reader can tell them apart
    // The header carries the shared parameters and the chosen algorithm's group, the other
    // groups going unwritten
    // Throws std::bad_optional_access when no seed was assigned, rather than write a header
    // that cannot reproduce its own level
    void write_level(std::ostream& os, const Level& level, const Parameters& parameters);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_WRITE_LEVEL_HPP
