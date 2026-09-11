//------------------------------------------------------------------------------
// stratum_sim
// load_layout_dungen.cpp
//------------------------------------------------------------------------------

#include "load_layout_dungen.hpp"
#include <algorithm>
#include <cassert>

namespace
{

    using quetzal::stratum_sim::detail::Map;

    //--------------------------------------------------------------------------
    enum class CellType
    {
        Solid,
        Room,
        Corridor,
        Doorway,
        Stairway
    };

    //--------------------------------------------------------------------------
    // dungen's symbols, from dungen::cell_types. '0' is its Null type, marking a cell the
    // generator never assigned
    CellType cell_type(char c)
    {
        switch (c)
        {
            case '#': return CellType::Solid;
            case '0': return CellType::Solid;
            case ' ': return CellType::Room;
            case '.': return CellType::Corridor;
            case '|': return CellType::Doorway;
            case '>': return CellType::Stairway;
        }

        assert(false); // Unrecognized cell code
        return CellType::Solid;
    }

    //--------------------------------------------------------------------------
    // Room and corridor are both open floor and are laid out alike. They differ only in
    // what the level calls them, which is what makes the rooms nameable
    bool open(CellType type)
    {
        return type != CellType::Solid;
    }

    //--------------------------------------------------------------------------
    // A doorway or stairway is a cell of wall a mover may stand in rather than part of the
    // area around it. The boundary of each neighboring area runs along its face, wound about
    // that area like the rest of that area's wall, so a door hangs on the face and a mover
    // inside meets only backs -- which are no walls at all to a query, and so let it out
    bool block(CellType type)
    {
        return type == CellType::Doorway || type == CellType::Stairway;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::detail::Map quetzal::stratum_sim::detail::read_block(const std::vector<std::string>& lines)
{
    Map map;
    coord_type width = 0;

    for (const std::string& s : lines)
    {
        width = std::max(width, static_cast<coord_type>(s.size()));
    }

    map.extent = discrete_vector_type(width, static_cast<coord_type>(lines.size()));

    // Trailing spaces may have been trimmed, and a short line is room floor
    std::vector<CellType> cell_types(map.cell_count(), CellType::Room);

    for (coord_type j = 0; j < map.extent.y(); ++j)
    {
        const std::string& line = lines[static_cast<size_t>(map.extent.y() - 1 - j)]; // The first line of the file is the highest row
        for (coord_type i = 0; i < map.extent.x(); ++i)
        {
            cell_types[map.slot({i, j})] = cell_type(i < static_cast<coord_type>(line.size()) ? line[static_cast<size_t>(i)] : ' ');
        }
    }

    // Off the grid is nowhere a mover may be, so it reads as closed and the outer boundary
    // needs no case of its own
    auto at = [&](const discrete_point_type& grid_position)
    {
        return grid_position.i() < 0 || grid_position.j() < 0
            || grid_position.i() >= map.extent.x() || grid_position.j() >= map.extent.y()
            ? CellType::Solid
            : cell_types[map.slot(grid_position)];
    };

    // A block is part of the area it lies in, so the area's boundary wraps around the solid cells
    // either side of it and its own sides are walled by that boundary rather than by nothing
    find_components(map,
        [&](coord_type i, coord_type j) { return open(at({i, j})); },
        [&](const discrete_point_type&, const discrete_point_type&) { return true; });

    map.cell_open.assign(map.cell_count(), false);

    for (coord_type j = 0; j < map.extent.y(); ++j)
    {
        for (coord_type i = 0; i < map.extent.x(); ++i)
        {
            map.cell_open[map.slot({i, j})] = open(at({i, j}));
        }
    }

    // Each block and the open cells its faces adjoin, which is where a door hangs
    for (coord_type j = 0; j < map.extent.y(); ++j)
    {
        for (coord_type i = 0; i < map.extent.x(); ++i)
        {
            const discrete_point_type grid_position {i, j};

            if (!block(at(grid_position)))
            {
                continue;
            }

            Block b;
            b.grid_position = grid_position;

            for (const auto& direction : cardinal_directions)
            {
                discrete_point_type neighbor = grid_position + direction;

                if (open(at(neighbor)) && !block(at(neighbor)))
                {
                    b.neighbor_grid_positions.push_back(neighbor);
                }
            }

            map.blocks.push_back(b);
        }
    }

    // The one property either format encodes about rooms: dungen writes room floor and corridor
    // floor as different symbols
    map.cell_room.assign(map.cell_count(), false);

    for (size_t slot = 0; slot < cell_types.size(); ++slot)
    {
        map.cell_room[slot] = cell_types[slot] == CellType::Room;
    }

    // No passages: nothing separates two open cells in this format except a block, and a
    // block is not somewhere two areas meet -- it is a cell of wall between them

    return map;
}
