#if !defined(QUETZAL_STRATUM_VIEW_CELLS_HPP)
#define QUETZAL_STRATUM_VIEW_CELLS_HPP
//------------------------------------------------------------------------------
// stratum_view
// cells.hpp
//------------------------------------------------------------------------------

#include "Console.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include <cstdint>

namespace quetzal::stratum_view
{

    //--------------------------------------------------------------------------
    // What occupies a cell, which is as far as the simulation is reduced before a view has
    // any say. Every view consumes these; what each turns them into is its own business
    enum class Kind : uint8_t
    {
        Rock,
        Floor,
        Actor
    };

    using Cells = Buffer<Kind>;

    // The whole zone reduced to what occupies each cell, shaped by the layout's grid
    Cells cells(const stratum_sim::World& world, flecs::entity zone);

} // namespace quetzal::stratum_view

#endif // QUETZAL_STRATUM_VIEW_CELLS_HPP
