//------------------------------------------------------------------------------
// stratum_view
// cells.cpp
//------------------------------------------------------------------------------

#include "cells.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp"
#include "quetzal/stratum_sim/Site.hpp"
#include <cassert>

namespace sim = quetzal::stratum_sim;

namespace
{

    using quetzal::stratum_view::coord_type;
    using quetzal::stratum_view::point_type;
    using quetzal::stratum_view::vector_type;

    //--------------------------------------------------------------------------
    // Where a cell lies on the lattice, cell (i, j) being at (2i + 1, 2j + 1). A wall on one
    // of its edges is that position stepped once in the direction it faces
    point_type lattice_position(const point_type& cell_position)
    {
        return 2 * cell_position + vector_type {1, 1};
    }

    //--------------------------------------------------------------------------
    // The four a cell can be asked about, in the order a wall lies on its edges
    const auto& directions = sim::cardinal_directions;

    //--------------------------------------------------------------------------
    // The components an attachment has, as the states a view draws apart. This is the one
    // place either side is aware of the other: nothing downstream sees a component, and nothing
    // in the simulation depends on a view distinguishing these
    quetzal::stratum_view::State state_of(flecs::entity attachment)
    {
        using quetzal::stratum_view::State;

        State state = State::None;

        if (attachment.has<sim::Openable>() && attachment.get<sim::Openable>().open)
        {
            state |= State::Open;
        }

        if (attachment.has<sim::Crossable>())
        {
            state |= State::Crossable;
        }

        // The component records the positive, so the state is what it has not been found to be
        if (attachment.has<sim::Concealed>() && !attachment.get<sim::Concealed>().revealed)
        {
            state |= State::Concealed;
        }

        return state;
    }

    //--------------------------------------------------------------------------
    // Whether a door reached from one side occupies the cell beyond it rather than lying on the
    // edge between the two. A door seen from one side only hangs on a doorway block's ring,
    // the far side of it being the bare back of that ring; a door both cells see is an edge
    bool door_in_cell(const sim::DiscreteLayout& layout, const sim::discrete_point_type& beyond,
        const sim::discrete_vector_type& direction)
    {
        return layout.open(beyond) && layout.wall_toward(beyond, -direction) == quetzal::nullid;
    }

    //--------------------------------------------------------------------------
    // Every door occupying a cell, as the cell it occupies and the state it is in. A door
    // on an edge has no cell of its own and is not reported
    template<typename F>
    void each_door_cell(const sim::DiscreteLayout& layout, flecs::entity zone, F f)
    {
        const sim::GridMapping& grid = layout.grid();

        for (coord_type j = 0; j < grid.extent.y(); ++j)
        {
            for (coord_type i = 0; i < grid.extent.x(); ++i)
            {
                sim::discrete_point_type cell_position {i, j};

                if (!layout.open(cell_position))
                {
                    continue;
                }

                for (const sim::discrete_vector_type& direction : directions)
                {
                    quetzal::id_type wall_id = layout.wall_toward(cell_position, direction);
                    flecs::entity facing = wall_id != quetzal::nullid
                        ? sim::facing_on(zone, wall_id) : flecs::entity();

                    sim::discrete_point_type beyond = cell_position + direction;

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
    // The cell each entity in the zone occupies, off-grid ones passed over. Both reductions
    // ask this last and set a flag rather than a value, so what an actor stands on is still
    // in the place beneath it
    template<typename F>
    void each_actor_cell(const sim::DiscreteLayout& layout, flecs::entity zone, F f)
    {
        const sim::GridMapping& grid = layout.grid();

        zone.world().query_builder<const sim::Site>()
            .with(flecs::ChildOf, zone)
            .build()
            .each([&](const sim::Site& site)
            {
                if (site.address.has_value())
                {
                    return; // Placed in its address's frame rather than the zone's
                }

                sim::discrete_point_type cell_position = layout.cell_position_at(site.placement.position());
                if (grid.contains(cell_position))
                {
                    f(cell_position);
                }
            });

        return;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_view::Cells quetzal::stratum_view::cells(const sim::World& world, flecs::entity zone)
{
    // One cell per grid position is the shape of this reduction, so it asks for a layout
    // that has cells. What lies on the edges between them is not in it and is not asked for
    const sim::DiscreteLayout& layout = sim::layout_as<sim::DiscreteLayout>(world.layout(zone));

    const sim::GridMapping& grid = layout.grid();

    // Every grid position is one or the other, so there is no nothing here: closed until an
    // open cell overrides it
    Cells cells(grid.extent, Place {PlaceType::Solid});

    for (coord_type j = 0; j < grid.extent.y(); ++j)
    {
        for (coord_type i = 0; i < grid.extent.x(); ++i)
        {
            if (layout.open(sim::discrete_point_type {i, j}))
            {
                cells.set({i, j}, Place {PlaceType::Floor});
            }
        }
    }

    // A door occupying a cell has a position here; one on an edge between two cells does
    // not, so the wall format's doors are what this reduction cannot show
    each_door_cell(layout, zone, [&](const sim::discrete_point_type& cell_position, State state)
    {
        cells.set(cell_position, Place {PlaceType::Door, state});
    });

    each_actor_cell(layout, zone, [&](const sim::discrete_point_type& cell_position)
    {
        Place place = cells.at(cell_position);
        place.actor = true;
        cells.set(cell_position, place);
    });

    return cells;
}

//------------------------------------------------------------------------------
quetzal::stratum_view::Lattice quetzal::stratum_view::lattice(const sim::World& world, flecs::entity zone)
{
    const sim::DiscreteLayout& layout = sim::layout_as<sim::DiscreteLayout>(world.layout(zone));

    const sim::GridMapping& grid = layout.grid();

    auto w = grid.extent.x();
    auto h = grid.extent.y();

    Lattice lattice(vector_type {2 * w + 1, 2 * h + 1}, Place {});

    // Driven from the open cells, which are the only places a wall can be asked about: a wall
    // exists on the side facing open floor, so a closed cell has nothing to report and is
    // never asked.
    // Every wall faces an open cell, so every wall is reached
    for (coord_type j = 0; j < h; ++j)
    {
        for (coord_type i = 0; i < w; ++i)
        {
            sim::discrete_point_type cell_position {i, j};

            if (!layout.open(cell_position))
            {
                continue;
            }

            lattice.set(lattice_position(cell_position), Place {PlaceType::Floor});

            for (const sim::discrete_vector_type& direction : directions)
            {
                id_type wall_id = layout.wall_toward(cell_position, direction);

                if (wall_id == nullid)
                {
                    continue;
                }

                // What is attached to the wall is what makes it a door, and the state it
                // is in is read from the components the door has
                flecs::entity facing = sim::facing_on(zone, wall_id);

                if (!facing)
                {
                    lattice.set(lattice_position(cell_position) + direction, Place {PlaceType::Wall});
                    continue;
                }

                Place place {PlaceType::Door, state_of(sim::door_of(facing))};

                // Placed once however many ways into the cell there are
                sim::discrete_point_type beyond = cell_position + direction;

                if (door_in_cell(layout, beyond, direction))
                {
                    lattice.set(lattice_position(beyond), place);
                    continue;
                }

                lattice.set(lattice_position(cell_position) + direction, place);
            }
        }
    }

    // Corners, from the edges already placed. A joint is where a view puts something to cover
    // it, so the reduction marks one there and leaves what to make of it to the view
    for (coord_type j = 0; j <= h; ++j)
    {
        for (coord_type i = 0; i <= w; ++i)
        {
            bool joined = (j < h && lattice.at({2 * i, 2 * j + 1}).type != PlaceType::None)
                || (j > 0 && lattice.at({2 * i, 2 * j - 1}).type != PlaceType::None)
                || (i < w && lattice.at({2 * i + 1, 2 * j}).type != PlaceType::None)
                || (i > 0 && lattice.at({2 * i - 1, 2 * j}).type != PlaceType::None);

            if (joined)
            {
                lattice.set({2 * i, 2 * j}, Place {PlaceType::Wall});
            }
        }
    }

    // What reaches each position, once everything lying on one is placed. A cell is never
    // part of a wall line, so a door occupying one is a gap in the line rather than a
    // continuation of it, and an edge is left with only the corners at its own two ends
    auto line = [&](const point_type& position)
    {
        if (!lattice.contains(position) || (position.i() % 2 == 1 && position.j() % 2 == 1))
        {
            return false;
        }

        PlaceType type = lattice.at(position).type;
        return type == PlaceType::Wall || type == PlaceType::Door;
    };

    // Whether every cell a position touches is closed, which is what tells solid from an edge
    // with no wall on it. A cell touches itself, an edge the two it lies between and a corner
    // the four meeting at it, so this is one rule over the three rather than a case for each
    // A position off the grid touches no open cell, so the outer boundary is solid without
    // anything beyond the grid being called a cell
    auto solid = [&](const point_type& position)
    {
        coord_type x = position.i();
        coord_type y = position.j();

        coord_type i0 = x % 2 == 1 ? (x - 1) / 2 : x / 2 - 1;
        coord_type j0 = y % 2 == 1 ? (y - 1) / 2 : y / 2 - 1;

        for (coord_type j = j0; j <= j0 + (y % 2 == 1 ? 0 : 1); ++j)
        {
            for (coord_type i = i0; i <= i0 + (x % 2 == 1 ? 0 : 1); ++i)
            {
                if (i >= 0 && j >= 0 && i < w && j < h && layout.open(sim::discrete_point_type {i, j}))
                {
                    return false;
                }
            }
        }

        return true;
    };

    for (coord_type y = 0; y <= 2 * h; ++y)
    {
        for (coord_type x = 0; x <= 2 * w; ++x)
        {
            point_type position {x, y};
            Place place = lattice.at(position);

            place.mask = DirectionMask::None;
            if (line(position + sim::cardinal_direction(sim::CardinalDirection::East))) { place.mask |= DirectionMask::East; }
            if (line(position + sim::cardinal_direction(sim::CardinalDirection::North))) { place.mask |= DirectionMask::North; }
            if (line(position + sim::cardinal_direction(sim::CardinalDirection::West))) { place.mask |= DirectionMask::West; }
            if (line(position + sim::cardinal_direction(sim::CardinalDirection::South))) { place.mask |= DirectionMask::South; }

            if (solid(position))
            {
                assert(place.type == PlaceType::None); // A wall faces open floor, so none is here
                place.type = PlaceType::Solid;
            }

            lattice.set(position, place);
        }
    }

    each_actor_cell(layout, zone, [&](const sim::discrete_point_type& cell_position)
    {
        point_type position = lattice_position(cell_position);

        Place place = lattice.at(position);
        place.actor = true;
        lattice.set(position, place);
    });

    return lattice;
}
