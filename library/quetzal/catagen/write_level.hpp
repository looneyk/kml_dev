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

    // The version of the file format, recorded in the header so that a reader knows what the
    // grid below it means. A leading zero says pre-release: the glyphs and the shape are still
    // free to change, and every level regenerates when they do
    constexpr auto format_version = "0.2";

    // A header row naming the generator, the format version and the parameters of the run,
    // then the grid. A grid row never begins with a letter, so a reader can tell them apart
    // Throws std::bad_optional_access when no seed was assigned, rather than write a header
    // that cannot reproduce its own level
    void write_level(std::ostream& os, const Level& level, const Parameters& parameters);

    // The grid alone, for a caller with nothing to record -- printing to a terminal
    // Walls fall between the cells, so the grid is two characters across per cell plus one:
    // cell (i, j) is at (2i + 1, 2j + 1), the wall east of it at (2i + 2, 2j + 1), the wall
    // north of it at (2i + 1, 2j + 2), and their corners at the even positions in both.
    // The first line written is the highest row, the origin being the lower left cell
    void write_grid(std::ostream& os, const Level& level);

} // namespace quetzal::catagen

#endif // QUETZAL_CATAGEN_WRITE_LEVEL_HPP
