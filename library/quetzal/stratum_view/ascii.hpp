#if !defined(QUETZAL_STRATUM_VIEW_ASCII_HPP)
#define QUETZAL_STRATUM_VIEW_ASCII_HPP
//------------------------------------------------------------------------------
// stratum_view
// ascii.hpp
//------------------------------------------------------------------------------

#include "cells.hpp"
#include <optional>
#include <vector>

namespace quetzal::stratum_view
{

    using Console = Buffer<char>;

    //--------------------------------------------------------------------------
    // One line of a glyph set: what it matches and what it draws
    // type always counts. state names the flags a place must have, and naming none matches
    // whatever state it is in. mask counts only where it is given. actor asks for something
    // standing on the place, and a line not asking matches whether or not one does
    struct Glyph
    {
        PlaceType type = PlaceType::None;
        State state = State::None;
        std::optional<Mask> mask;
        bool actor = false;
        char c = ' ';
    };

    //--------------------------------------------------------------------------
    // A mapping from what is at a position to the character shown for it. A set's expressive
    // range is a property of the set: one naming an open door draws it apart from a shut one,
    // and one naming neither draws both alike, without the renderer being aware which it has
    class Glyphs
    {
    public:

        Glyphs() = default;
        explicit Glyphs(std::vector<Glyph> glyphs);

        // The most specific line matching, counting a state flag given, a mask given and an
        // actor asked for each as one; the first of them where two are equally specific, and
        // a space where none matches at all
        char operator()(const Place& place) const;

        // Every line, in the order they were given
        const std::vector<Glyph>& glyphs() const;

    private:

        std::vector<Glyph> m_glyphs;
    };

    //--------------------------------------------------------------------------
    // The set a cell per glyph is drawn with: the three types a cell can be, a door shut and
    // open, and an actor over each type it can stand on. No position here has an edge
    const Glyphs& cell_glyphs();

    // The set the lattice is drawn with, which is what the wall format writes: floor is a
    // space rather than a dot, so a zone rendered onto the lattice reproduces the file it
    // was loaded from. Sixteen lines for a wall, ASCII having three glyphs for the joints
    // the mask tells apart
    const Glyphs& lattice_glyphs();

    // Cells as characters. Producing the grid is the whole of it, printing it is the host's
    // business. The second overload composes the reduction and the mapping for a caller that
    // needs characters from a world directly
    Console render(const Cells& cells, const Glyphs& glyphs = cell_glyphs());
    Console render(const stratum_sim::World& world, flecs::entity zone, const Glyphs& glyphs = cell_glyphs());

    // The lattice as characters, on the (2W+1) by (2H+1) grid the wall format writes. A name
    // of its own rather than an overload: Cells and Lattice are the same type, differing in
    // what they are indexed by rather than in what they contain
    // One lookup per position and no rule of its own: what a position is, what state it is
    // in and what reaches it are all in the reduction, so cells, edges and corners resolve
    // through the same table
    Console render_lattice(const Lattice& lattice, const Glyphs& glyphs = lattice_glyphs());
    Console render_lattice(const stratum_sim::World& world, flecs::entity zone, const Glyphs& glyphs = lattice_glyphs());

} // namespace quetzal::stratum_view

#endif // QUETZAL_STRATUM_VIEW_ASCII_HPP
