//------------------------------------------------------------------------------
// stratum_view
// cells.cpp
//------------------------------------------------------------------------------

#include "cells.hpp"
#include <cassert>

namespace sim = quetzal::stratum_sim;

//------------------------------------------------------------------------------
quetzal::stratum_view::Cells quetzal::stratum_view::cells(const sim::World& world, flecs::entity zone)
{
    const sim::Layout& layout = world.layout(zone);

    const sim::Grid& grid = layout.grid();

    Cells cells(static_cast<size_t>(grid.extent.x()), static_cast<size_t>(grid.extent.y()), Kind::Rock);

    for (long j = 0; j < grid.extent.y(); ++j)
    {
        for (long i = 0; i < grid.extent.x(); ++i)
        {
            if (layout.open(sim::GridPosition {i, j}))
            {
                cells.set(static_cast<size_t>(i), static_cast<size_t>(j), Kind::Floor);
            }
        }
    }

    // Entities last, so they take the cell they stand in
    zone.world().query_builder<const sim::placement_type>()
        .with(flecs::ChildOf, zone)
        .build()
        .each([&](const sim::placement_type& placement)
        {
            sim::GridPosition cell = layout.cell_at(placement.position());
            if (grid.contains(cell))
            {
                cells.set(static_cast<size_t>(cell.i), static_cast<size_t>(cell.j), Kind::Actor);
            }
        });

    return cells;
}
