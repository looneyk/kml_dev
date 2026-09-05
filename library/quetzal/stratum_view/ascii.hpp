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
    // kind always counts. state names the flags a place must have, and naming none matches
    // whatever state it is in. mask counts only where it is given
    struct Glyph
    {
        Kind kind = Kind::Rock;
        State state = State::None;
        std::optional<Mask> mask;
        char c = ' ';
    };

    //--------------------------------------------------------------------------
    // A mapping from what stands at a position to the character shown for it. What a set can
    // say is a property of the set: one naming an open door draws it apart from a shut one,
    // and one naming neither draws both alike, without the renderer knowing which it has
    class Glyphs
    {
    public:

        Glyphs() = default;
        explicit Glyphs(std::vector<Glyph> glyphs);

        // The most specific line matching, counting a named state flag and a named mask
        // each as one; the first of them where two are equally specific, and a space where
        // none matches at all
        char operator()(const Place& place) const;

        // Every line, in the order they were given
        const std::vector<Glyph>& glyphs() const;

    private:

        std::vector<Glyph> m_glyphs;
    };

    //--------------------------------------------------------------------------
    // The set a cell per glyph is drawn with, one line per kind, no position having an edge
    // or a state to say anything about
    const Glyphs& cell_glyphs();

    // The set the lattice is drawn with, which is what the wall format writes: floor is a
    // space rather than a dot, so a level rendered onto the lattice reproduces the file it
    // was loaded from. Sixteen lines for a wall, ASCII having three glyphs for the joints
    // the mask tells apart
    const Glyphs& lattice_glyphs();

    // Cells as characters. Producing the grid is the whole of it, printing it is the host's
    // business. The second overload composes the reduction and the mapping for a caller that
    // wants characters from a world directly
    Console render(const Cells& cells, const Glyphs& glyphs = cell_glyphs());
    Console render(const stratum_sim::World& world, flecs::entity zone, const Glyphs& glyphs = cell_glyphs());

    // The lattice as characters, on the (2W+1) by (2H+1) grid the wall format writes. A name
    // of its own rather than an overload: Cells and Lattice are the same type, differing in
    // what they are indexed by rather than in what they hold
    // One lookup per position and no rule of its own: what a position is, what state it is
    // in and what reaches it are all in the reduction, so cells, edges and corners resolve
    // through the same table
    Console render_lattice(const Lattice& lattice, const Glyphs& glyphs = lattice_glyphs());
    Console render_lattice(const stratum_sim::World& world, flecs::entity zone, const Glyphs& glyphs = lattice_glyphs());

} // namespace quetzal::stratum_view

#endif // QUETZAL_STRATUM_VIEW_ASCII_HPP
