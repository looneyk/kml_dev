//------------------------------------------------------------------------------
// stratum_view
// cells.cpp
//------------------------------------------------------------------------------

#include "cells.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp"
#include <cassert>

namespace sim = quetzal::stratum_sim;

namespace
{

    //--------------------------------------------------------------------------
    // The four a cell can be asked about, in the order a wall lies on its edges
    const sim::discrete_vector_type directions[]
    {
        sim::Direction::East, sim::Direction::North, sim::Direction::West, sim::Direction::South
    };

    //--------------------------------------------------------------------------
    // The components an attachment carries, as the states a view draws apart. This is the one
    // place either side knows the other: nothing downstream sees a component, and nothing in
    // the simulation knows a view distinguishes these
    quetzal::stratum_view::State state_of(flecs::entity attachment)
    {
        using quetzal::stratum_view::State;

        State state = State::None;

        if (attachment.has<sim::Openable>() && attachment.get<sim::Openable>().open)
        {
            state |= State::Open;
        }

        if (attachment.has<sim::Passable>())
        {
            state |= State::Passable;
        }

        // The component holds the positive, so the state is what it has not been found to be
        if (attachment.has<sim::Concealed>() && !attachment.get<sim::Concealed>().found)
        {
            state |= State::Concealed;
        }

        return state;
    }

    //--------------------------------------------------------------------------
    // Whether a door reached from one side stands in the cell beyond it rather than on the
    // edge between the two. A door seen from one side only hangs on a doorway block's ring,
    // the far side of it being the bare back of that ring; a door both cells see is an edge
    bool door_in_cell(const sim::DiscreteLayout& layout, const sim::GridPosition& beyond,
        const sim::discrete_vector_type& direction)
    {
        return layout.open(beyond) && layout.wall_toward(beyond, -direction) == quetzal::nullid;
    }

    //--------------------------------------------------------------------------
    // Every door standing in a cell, as the cell it stands in and the state it is in. A door
    // on an edge has no cell of its own and is not answered
    template<typename F>
    void each_door_cell(const sim::DiscreteLayout& layout, flecs::entity zone, F f)
    {
        const sim::Grid& grid = layout.grid();

        for (long j = 0; j < grid.extent.y(); ++j)
        {
            for (long i = 0; i < grid.extent.x(); ++i)
            {
                sim::GridPosition cell {i, j};

                if (!layout.open(cell))
                {
                    continue;
                }

                for (const sim::discrete_vector_type& direction : directions)
                {
                    quetzal::id_type wall_id = layout.wall_toward(cell, direction);
                    flecs::entity facing = wall_id != quetzal::nullid
                        ? sim::facing_on(zone, wall_id) : flecs::entity();

                    sim::GridPosition beyond = cell + direction;

                    if (facing && door_in_cell(layout, beyond, direction))
                    {
                        f(beyond, state_of(sim::door_of(facing)));
                    }
                }
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // The cell each entity standing in the zone occupies, off-grid ones passed over. Both
    // reductions ask this last, so that an actor takes the cell it stands in rather than
    // being drawn over by the floor under it
    template<typename F>
    void each_actor_cell(const sim::DiscreteLayout& layout, flecs::entity zone, F f)
    {
        const sim::Grid& grid = layout.grid();

        zone.world().query_builder<const sim::placement_type>()
            .with(flecs::ChildOf, zone)
            .build()
            .each([&](const sim::placement_type& placement)
            {
                sim::GridPosition cell = layout.cell_at(placement.position());
                if (grid.contains(cell))
                {
                    f(cell);
                }
            });

        return;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_view::Cells quetzal::stratum_view::cells(const sim::World& world, flecs::entity zone)
{
    // One cell per grid position is the shape of this reduction, so it asks for a layout
    // that has cells. What stands on the edges between them is not in it and is not asked for
    const sim::DiscreteLayout& layout = sim::layout_as<sim::DiscreteLayout>(world.layout(zone));

    const sim::Grid& grid = layout.grid();

    Cells cells(static_cast<size_t>(grid.extent.x()), static_cast<size_t>(grid.extent.y()), Place {});

    for (long j = 0; j < grid.extent.y(); ++j)
    {
        for (long i = 0; i < grid.extent.x(); ++i)
        {
            if (layout.open(sim::GridPosition {i, j}))
            {
                cells.set(static_cast<size_t>(i), static_cast<size_t>(j), Place {Kind::Floor});
            }
        }
    }

    // A door standing in a cell has a position here; one on an edge between two cells does
    // not, so the wall format's doors are what this reduction cannot show
    each_door_cell(layout, zone, [&](const sim::GridPosition& cell, State state)
    {
        cells.set(static_cast<size_t>(cell.i), static_cast<size_t>(cell.j), Place {Kind::Door, state});
    });

    each_actor_cell(layout, zone, [&](const sim::GridPosition& cell)
    {
        cells.set(static_cast<size_t>(cell.i), static_cast<size_t>(cell.j), Place {Kind::Actor});
    });

    return cells;
}

//------------------------------------------------------------------------------
quetzal::stratum_view::Lattice quetzal::stratum_view::lattice(const sim::World& world, flecs::entity zone)
{
    const sim::DiscreteLayout& layout = sim::layout_as<sim::DiscreteLayout>(world.layout(zone));

    const sim::Grid& grid = layout.grid();

    long w = grid.extent.x();
    long h = grid.extent.y();

    Lattice lattice(static_cast<size_t>(2 * w + 1), static_cast<size_t>(2 * h + 1), Place {});

    auto set = [&](long i, long j, const Place& place)
    {
        lattice.set(static_cast<size_t>(i), static_cast<size_t>(j), place);
    };

    auto at = [&](long i, long j)
    {
        return lattice.at(static_cast<size_t>(i), static_cast<size_t>(j));
    };

    // Driven from the open cells, which are the only places a wall can be asked about: a wall
    // stands on the side facing open space, so rock has nothing to answer and is never asked.
    // Every wall faces an open cell, so every wall is reached
    for (long j = 0; j < h; ++j)
    {
        for (long i = 0; i < w; ++i)
        {
            sim::GridPosition cell {i, j};

            if (!layout.open(cell))
            {
                continue;
            }

            set(2 * i + 1, 2 * j + 1, Place {Kind::Floor});

            for (const sim::discrete_vector_type& direction : directions)
            {
                id_type wall_id = layout.wall_toward(cell, direction);

                if (wall_id == nullid)
                {
                    continue;
                }

                // What stands on the wall is what says whether it is a door, and the state it
                // is in is read from the components the door carries
                flecs::entity facing = sim::facing_on(zone, wall_id);

                if (!facing)
                {
                    set(2 * i + 1 + direction.x(), 2 * j + 1 + direction.y(), Place {Kind::Wall});
                    continue;
                }

                Place place {Kind::Door, state_of(sim::door_of(facing))};

                // Placed once however many ways into the cell there are
                sim::GridPosition beyond = cell + direction;

                if (door_in_cell(layout, beyond, direction))
                {
                    set(2 * beyond.i + 1, 2 * beyond.j + 1, place);
                    continue;
                }

                set(2 * i + 1 + direction.x(), 2 * j + 1 + direction.y(), place);
            }
        }
    }

    // Corners, from the edges already placed. A joint is where a view puts something to cover
    // it, so the reduction says one is there and leaves what to make of it to the view
    for (long j = 0; j <= h; ++j)
    {
        for (long i = 0; i <= w; ++i)
        {
            bool joined = (j < h && at(2 * i, 2 * j + 1).kind != Kind::Rock)
                || (j > 0 && at(2 * i, 2 * j - 1).kind != Kind::Rock)
                || (i < w && at(2 * i + 1, 2 * j).kind != Kind::Rock)
                || (i > 0 && at(2 * i - 1, 2 * j).kind != Kind::Rock);

            if (joined)
            {
                set(2 * i, 2 * j, Place {Kind::Wall});
            }
        }
    }

    // What reaches each position, once everything standing on one is placed. A cell is never
    // part of a wall line, so a door standing in one is a gap in the line rather than a
    // continuation of it, and an edge is left with only the corners at its own two ends
    auto line = [&](long x, long y)
    {
        if (x < 0 || y < 0 || x > 2 * w || y > 2 * h || (x % 2 == 1 && y % 2 == 1))
        {
            return false;
        }

        Kind kind = at(x, y).kind;
        return kind == Kind::Wall || kind == Kind::Door;
    };

    // Whether every cell a position touches is rock, which is what tells rock from an edge
    // with no wall on it. A cell touches itself, an edge the two it lies between and a corner
    // the four meeting at it, so this is one rule over the three rather than a case for each
    auto solid = [&](long x, long y)
    {
        long i0 = x % 2 == 1 ? (x - 1) / 2 : x / 2 - 1;
        long j0 = y % 2 == 1 ? (y - 1) / 2 : y / 2 - 1;

        for (long j = j0; j <= j0 + (y % 2 == 1 ? 0 : 1); ++j)
        {
            for (long i = i0; i <= i0 + (x % 2 == 1 ? 0 : 1); ++i)
            {
                if (i >= 0 && j >= 0 && i < w && j < h && layout.open(sim::GridPosition {i, j}))
                {
                    return false;
                }
            }
        }

        return true;
    };

    for (long y = 0; y <= 2 * h; ++y)
    {
        for (long x = 0; x <= 2 * w; ++x)
        {
            Place place = at(x, y);

            place.mask = Mask::None;
            if (line(x + 1, y)) { place.mask |= Mask::East; }
            if (line(x, y + 1)) { place.mask |= Mask::North; }
            if (line(x - 1, y)) { place.mask |= Mask::West; }
            if (line(x, y - 1)) { place.mask |= Mask::South; }

            if (solid(x, y))
            {
                assert(place.kind == Kind::Rock); // A wall faces open space, so none stands here
                place.state |= State::Solid;
            }

            set(x, y, place);
        }
    }

    each_actor_cell(layout, zone, [&](const sim::GridPosition& cell)
    {
        set(2 * cell.i + 1, 2 * cell.j + 1, Place {Kind::Actor});
    });

    return lattice;
}
