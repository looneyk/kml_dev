#if !defined(QUETZAL_STRATUM_VIEW_READ_GLYPHS_HPP)
#define QUETZAL_STRATUM_VIEW_READ_GLYPHS_HPP
//------------------------------------------------------------------------------
// stratum_view
// read_glyphs.hpp
//------------------------------------------------------------------------------

#include "ascii.hpp"
#include <filesystem>
#include <istream>
#include <string>

namespace quetzal::stratum_view
{

    // Reads a glyph set from a file with one line per Glyph. Of two equally specific lines, the earlier one matches
    //
    //     glyphs 0.1
    //     floor actor '@'
    //     solid '#'
    //     none ' '
    //     door open '\'
    //     wall mask=EW '-'
    //
    // The first line states the format and its version. A glyph line is a feature type, then the access state (open, closed,
    // or locked) and the detection state (visible, concealed, or revealed) it requires in any order, then actor if it
    // requires an actor on the place, then mask= and the directions it requires, then the character in single quotes, so a
    // space can be a glyph. \\ and \' are the escapes
    // Omitting a state, actor, or mask= matches any. mask= with no directions requires that no wall line reach the position
    // A line beginning with # is a comment
    //
    // Throws std::runtime_error, reporting the line, when the file cannot be opened, has an unsupported format or version,
    // or has a line it cannot read
    Glyphs read_glyphs(const std::filesystem::path& filepath);

    // name identifies the stream in error messages
    Glyphs read_glyphs(std::istream& is, const std::string& name = {});

} // namespace quetzal::stratum_view

#endif // QUETZAL_STRATUM_VIEW_READ_GLYPHS_HPP
