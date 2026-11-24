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
    // One line of a glyph set: the places it matches and the character it draws
    // type must match, and access, detection, and mask must match if they are given. actor requires an actor
    // on the place; if actor is false, the line matches either way
    struct Glyph
    {
        FeatureType type = FeatureType::None;
        std::optional<stratum_sim::AccessState> access;
        std::optional<stratum_sim::DetectionState> detection;
        std::optional<DirectionMask> mask;
        bool actor = false;
        char c = ' ';
    };

    //--------------------------------------------------------------------------
    // Maps a Place to the character drawn for it
    class Glyphs
    {
    public:

        Glyphs() = default;
        explicit Glyphs(std::vector<Glyph> glyphs);

        // Returns the character of the most specific matching line, where an access, a detection, a mask, and an actor add one to
        // the specificity. Ties go to the earlier line, and a space is returned if no line matches
        char operator()(const Place& place) const;

        // In the order given
        const std::vector<Glyph>& glyphs() const;

    private:

        std::vector<Glyph> m_glyphs;
    };

    //--------------------------------------------------------------------------
    // The glyph set for one character per cell
    const Glyphs& cell_glyphs();

    // The glyph set for the lattice, in the wall notation's glyphs, so a zone rendered onto the lattice reproduces the file
    // it was loaded from
    const Glyphs& lattice_glyphs();

    // The second overload reduces the zone to cells first, as the viewer perceives it
    Console render(const Cells& cells, const Glyphs& glyphs = cell_glyphs());
    Console render(const stratum_sim::World& world, flecs::entity zone, flecs::entity viewer, const Glyphs& glyphs = cell_glyphs());

    // Named apart from render because Cells and Lattice are the same type
    Console render_lattice(const Lattice& lattice, const Glyphs& glyphs = lattice_glyphs());
    Console render_lattice(const stratum_sim::World& world, flecs::entity zone, flecs::entity viewer, const Glyphs& glyphs = lattice_glyphs());

} // namespace quetzal::stratum_view

#endif // QUETZAL_STRATUM_VIEW_ASCII_HPP
