//------------------------------------------------------------------------------
// stratum_view
// cells.cpp
//------------------------------------------------------------------------------

#include "cells.hpp"
#include "quetzal/stratum_sim/AccessPoint.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp"
#include "quetzal/stratum_sim/Site.hpp"
#include <variant>
#include <cassert>

namespace sim = quetzal::stratum_sim;

namespace
{

    using quetzal::stratum_view::coord_type;
    using quetzal::stratum_view::point_type;
    using quetzal::stratum_view::vector_type;

    //--------------------------------------------------------------------------
    // Cell (i, j) is at (2i + 1, 2j + 1), and the wall on its side in a direction is one position further that way
    point_type lattice_position(const point_type& cell_position)
    {
        return 2 * cell_position + vector_type {1, 1};
    }

    //--------------------------------------------------------------------------
    const auto& directions = sim::cardinal_directions;

    //--------------------------------------------------------------------------
    // Returns the way through on the wall as the viewer perceives it: a null entity if there is none or the viewer finds it
    // concealed
    flecs::entity visible_access_attachment(flecs::entity zone, quetzal::id_type wall_id, flecs::entity viewer)
    {
        flecs::entity way = sim::find_access_attachment(zone, wall_id);
        return way && sim::detection(way, viewer) != sim::DetectionState::Concealed ? way : flecs::entity();
    }

    //--------------------------------------------------------------------------
    // Converts a way through's components to the states that a view draws differently
    quetzal::stratum_view::State state_of(flecs::entity way)
    {
        using quetzal::stratum_view::State;

        State state = State::None;

        if (sim::access(way) == sim::AccessState::Open)
        {
            state |= State::Open;
        }

        if (sim::detection(way, flecs::entity()) == sim::DetectionState::Concealed)
        {
            state |= State::Concealed;
        }

        return state;
    }

    //--------------------------------------------------------------------------
    // Returns true if a door occupies the cell beyond rather than lying on the edge between the two. A
    // door in a cell has a wall on one side only
    bool door_in_cell(const sim::DiscreteLayout& layout, const sim::discrete_point_type& beyond,
        const sim::discrete_vector_type& direction)
    {
        return layout.open(beyond) && layout.wall_toward(beyond, -direction) == quetzal::nullid;
    }

    //--------------------------------------------------------------------------
    // Calls f for every door that occupies a cell, with its state. A door on an edge is skipped
    template<typename F>
    void each_door_cell(const sim::DiscreteLayout& layout, flecs::entity zone, flecs::entity viewer, F f)
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
                    flecs::entity way = wall_id != quetzal::nullid ? visible_access_attachment(zone, wall_id, viewer) : flecs::entity();

                    sim::discrete_point_type beyond = cell_position + direction;

                    if (way && door_in_cell(layout, beyond, direction))
                    {
                        f(beyond, state_of(way));
                    }
                }
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Returns true if the entity's access point is known to lead below its zone. Stairs whose direction is not known are
    // drawn leading up, the entrance glyph
    bool leads_down(flecs::entity e)
    {
        const sim::AccessPoint& access_point = e.get<sim::AccessPoint>();
        const auto* zone_position = sim::zone_of(e).try_get<sim::ZonePosition>();

        if (!access_point.target.has_value())
        {
            return false;
        }

        if (const auto* target = std::get_if<quetzal::stratum_format::AccessPointTarget>(&*access_point.target))
        {
            if (const auto* relative = std::get_if<quetzal::stratum_format::RelativeZone>(&target->zone))
            {
                return relative->offset < 0;
            }

            const auto* absolute = std::get_if<quetzal::stratum_format::AbsoluteZone>(&target->zone);
            return absolute != nullptr && zone_position != nullptr && absolute->level < zone_position->z;
        }

        flecs::entity bound = std::get<sim::EntityReference>(*access_point.target).entity;
        const auto* bound_position = bound && bound.is_alive() ? sim::zone_of(bound).try_get<sim::ZonePosition>() : nullptr;

        return bound_position != nullptr && zone_position != nullptr && bound_position->z < zone_position->z;
    }

    //--------------------------------------------------------------------------
    // Calls f for every cell drawn as stairs, with the place type that draws it: the cell of an access point on the floor,
    // and the closed cell beyond an access point on a wall, which is a stair block
    template<typename F>
    void each_stairs_cell(const sim::DiscreteLayout& layout, flecs::entity zone, F f)
    {
        sim::with_in_zone(zone.world().query_builder<const sim::Site>(), zone)
            .with<sim::AccessPoint>()
            .build()
            .each([&](flecs::entity e, const sim::Site& site)
            {
                if (!site.address.has_value())
                {
                    return;
                }

                quetzal::stratum_view::PlaceType type = leads_down(e) ? quetzal::stratum_view::PlaceType::StairsDown
                    : quetzal::stratum_view::PlaceType::StairsUp;

                if (const auto* flat_cell = std::get_if<sim::FlatCellAddress>(&*site.address))
                {
                    if (flat_cell->flat_id == layout.floor_id())
                    {
                        f(flat_cell->cell_position, type);
                    }

                    return;
                }

                quetzal::id_type wall_id = std::get<sim::WallAddress>(*site.address).wall_id;
                sim::discrete_point_type beyond = layout.wall_cell_position(wall_id) + layout.wall_direction(wall_id);

                if (layout.grid().contains(beyond) && !layout.open(beyond))
                {
                    f(beyond, type);
                }
            });

        return;
    }

    //--------------------------------------------------------------------------
    // The cell each entity in the zone occupies, skipping those off the grid
    template<typename F>
    void each_actor_cell(const sim::DiscreteLayout& layout, flecs::entity zone, F f)
    {
        const sim::GridMapping& grid = layout.grid();

        sim::with_in_zone(zone.world().query_builder<const sim::Site>(), zone)
            .build()
            .each([&](const sim::Site& site)
            {
                if (site.address.has_value())
                {
                    return; // Placed relative to its address
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
quetzal::stratum_view::Cells quetzal::stratum_view::cells(const sim::World& world, flecs::entity zone, flecs::entity viewer)
{
    const sim::DiscreteLayout& layout = sim::layout_as<sim::DiscreteLayout>(world.layout(zone));

    const sim::GridMapping& grid = layout.grid();

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

    each_door_cell(layout, zone, viewer, [&](const sim::discrete_point_type& cell_position, State state)
    {
        cells.set(cell_position, Place {PlaceType::Door, state});
    });

    each_stairs_cell(layout, zone, [&](const sim::discrete_point_type& cell_position, PlaceType type)
    {
        cells.set(cell_position, Place {type});
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
quetzal::stratum_view::Lattice quetzal::stratum_view::lattice(const sim::World& world, flecs::entity zone, flecs::entity viewer)
{
    const sim::DiscreteLayout& layout = sim::layout_as<sim::DiscreteLayout>(world.layout(zone));

    const sim::GridMapping& grid = layout.grid();

    auto w = grid.extent.x();
    auto h = grid.extent.y();

    Lattice lattice(vector_type {2 * w + 1, 2 * h + 1}, Place {});

    // Every wall faces an open cell, so visiting the open cells reaches every wall
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

                flecs::entity way = visible_access_attachment(zone, wall_id, viewer);

                if (!way)
                {
                    lattice.set(lattice_position(cell_position) + direction, Place {PlaceType::Wall});
                    continue;
                }

                Place place {PlaceType::Door, state_of(way)};

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

    // Corners, from the edges already placed
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

    // Returns true if a position is part of a wall line. A cell never is, so a door occupying one is a gap in the line
    auto line = [&](const point_type& position)
    {
        if (!lattice.contains(position) || (position.i() % 2 == 1 && position.j() % 2 == 1))
        {
            return false;
        }

        PlaceType type = lattice.at(position).type;
        return type == PlaceType::Wall || type == PlaceType::Door;
    };

    // Returns true if every cell a position touches is closed: a cell touches itself, an edge the two it lies between and a corner
    // the four meeting at it. Positions beyond the grid count as closed
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
                assert(place.type == PlaceType::None); // A wall faces open floor, so no wall is at a solid position
                place.type = PlaceType::Solid;
            }

            lattice.set(position, place);
        }
    }

    each_stairs_cell(layout, zone, [&](const sim::discrete_point_type& cell_position, PlaceType type)
    {
        Place place = lattice.at(lattice_position(cell_position));
        place.type = type;
        lattice.set(lattice_position(cell_position), place);
    });

    each_actor_cell(layout, zone, [&](const sim::discrete_point_type& cell_position)
    {
        point_type position = lattice_position(cell_position);

        Place place = lattice.at(position);
        place.actor = true;
        lattice.set(position, place);
    });

    return lattice;
}
