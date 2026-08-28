//------------------------------------------------------------------------------
// stratum_view
// ascii.cpp
//------------------------------------------------------------------------------

#include "ascii.hpp"

//------------------------------------------------------------------------------
quetzal::stratum_view::Console quetzal::stratum_view::render(const stratum_sim::World& world, flecs::entity zone, const Glyphs& glyphs)
{
    const stratum_sim::Layout& layout = world.layout(zone);
    const stratum_sim::Grid& grid = layout.grid();

    Console console(static_cast<size_t>(grid.extent.i), static_cast<size_t>(grid.extent.j), glyphs.rock);

    for (int j = 0; j < grid.extent.j; ++j)
    {
        for (int i = 0; i < grid.extent.i; ++i)
        {
            if (layout.region_at(stratum_sim::Cell {i, j}) != nullid)
            {
                console.set(static_cast<size_t>(i), static_cast<size_t>(j), glyphs.floor);
            }
        }
    }

    // Entities last, so they draw over the floor they stand on
    zone.world().query_builder<const stratum_sim::placement_type>()
        .with(flecs::ChildOf, zone)
        .build()
        .each([&](const stratum_sim::placement_type& placement)
        {
            stratum_sim::Cell cell = layout.cell_at(placement.position());
            if (cell.i >= 0 && cell.j >= 0 && cell.i < grid.extent.i && cell.j < grid.extent.j)
            {
                console.set(static_cast<size_t>(cell.i), static_cast<size_t>(cell.j), glyphs.actor);
            }
        });

    return console;
}
