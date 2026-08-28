#if !defined(QUETZAL_STRATUM_VIEW_ASCII_HPP)
#define QUETZAL_STRATUM_VIEW_ASCII_HPP
//------------------------------------------------------------------------------
// stratum_view
// ascii.hpp
//------------------------------------------------------------------------------

#include "Console.hpp"
#include "quetzal/stratum_sim/World.hpp"

namespace quetzal::stratum_view
{

    //--------------------------------------------------------------------------
    // What each thing looks like. A fixed set while the simulation has no asset ids to
    // key on; it becomes a table when content arrives
    struct Glyphs
    {
        char rock = '#';
        char floor = '.';
        char actor = '@';
    };

    // The whole Zone as characters, one per cell. Produces the grid; printing it is the
    // host's business
    Console render(const stratum_sim::World& world, flecs::entity zone, const Glyphs& glyphs = {});

} // namespace quetzal::stratum_view

#endif // QUETZAL_STRATUM_VIEW_ASCII_HPP
