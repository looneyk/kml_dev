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

    // A glyph set as a file, one line per Glyph, in the order they are matched where two are
    // equally specific
    //
    //     glyphs 0.1
    //     floor actor '@'
    //     solid '#'
    //     none ' '
    //     door open '\'
    //     wall mask=EW '-'
    //
    // The first row names the format and its version. A line is a place type, then the state names
    // it asks for in any order, actor where it asks for one standing there, then mask= and the
    // directions it asks for, then the character in single quotes -- quoted so that a space is
    // a glyph like any other. \\ and \' are the two escapes, there being nothing else a single
    // character needs
    // Naming no state matches whatever state a place is in, leaving out actor matches whether
    // or not one stands there, and leaving out mask= matches whatever reaches it. mask= with
    // no directions asks for nothing reaching it at all
    // A line beginning with # is a comment, a glyph never beginning one
    //
    // Throws std::runtime_error when the file cannot be opened, names a format or version
    // this does not read, or has a line it cannot read -- reporting the line, since a set is
    // written by hand. Nothing is built until every line has passed
    Glyphs read_glyphs(const std::filesystem::path& filepath);

    // The same from anything a set can be read out of, the name being what a failure calls it
    Glyphs read_glyphs(std::istream& is, const std::string& name = {});

} // namespace quetzal::stratum_view

#endif // QUETZAL_STRATUM_VIEW_READ_GLYPHS_HPP
