//------------------------------------------------------------------------------
// stratum_view
// ascii.cpp
//------------------------------------------------------------------------------

#include "ascii.hpp"

namespace sim = quetzal::stratum_sim;

//------------------------------------------------------------------------------
char quetzal::stratum_view::Glyphs::operator()(Kind kind) const
{
    switch (kind)
    {
        case Kind::Floor: return floor;
        case Kind::Actor: return actor;
        case Kind::Rock: break;
    }

    return rock;
}

//------------------------------------------------------------------------------
quetzal::stratum_view::Console quetzal::stratum_view::render(const Cells& cells, const Glyphs& glyphs)
{
    Console console(cells.width(), cells.height(), glyphs.rock);

    for (size_t j = 0; j < cells.height(); ++j)
    {
        for (size_t i = 0; i < cells.width(); ++i)
        {
            console.set(i, j, glyphs(cells.at(i, j)));
        }
    }

    return console;
}

//------------------------------------------------------------------------------
quetzal::stratum_view::Console quetzal::stratum_view::render(const sim::World& world, flecs::entity zone, const Glyphs& glyphs)
{
    return render(cells(world, zone), glyphs);
}
