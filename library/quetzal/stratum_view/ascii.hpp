#if !defined(QUETZAL_STRATUM_VIEW_ASCII_HPP)
#define QUETZAL_STRATUM_VIEW_ASCII_HPP
//------------------------------------------------------------------------------
// stratum_view
// ascii.hpp
//------------------------------------------------------------------------------

#include "cells.hpp"

namespace quetzal::stratum_view
{

    //--------------------------------------------------------------------------
    // The terminal's mapping from a Kind to how it shows it. A fixed set while the simulation
    // has no asset ids to key on; it becomes a table when content arrives
    struct Glyphs
    {
        char rock = '#';
        char floor = '.';
        char actor = '@';

        char operator()(Kind kind) const;
    };

    // Cells as characters. Producing the grid is the whole of it, printing it is the host's
    // business. The second overload composes the reduction and the mapping for a caller that
    // wants characters from a world directly
    Console render(const Cells& cells, const Glyphs& glyphs = {});
    Console render(const stratum_sim::World& world, flecs::entity zone, const Glyphs& glyphs = {});

} // namespace quetzal::stratum_view

#endif // QUETZAL_STRATUM_VIEW_ASCII_HPP
