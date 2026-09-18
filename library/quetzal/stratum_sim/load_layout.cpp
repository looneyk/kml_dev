//------------------------------------------------------------------------------
// stratum_sim
// load_layout.cpp
//------------------------------------------------------------------------------

#include "load_layout.hpp"
#include "quetzal/stratum_format/grid.hpp"
#include "quetzal/stratum_format/read.hpp"
#include <set>
#include <utility>

//------------------------------------------------------------------------------
std::unique_ptr<quetzal::stratum_sim::DiscreteLayout> quetzal::stratum_sim::load_layout(const std::filesystem::path& filepath, const door_type& door,
    stratum_format::Notation* notation)
{
    stratum_format::Document document = stratum_format::read(filepath);
    const stratum_format::Cells& cells = document.cells;

    auto layout = std::make_unique<DiscreteLayout>(cells.origin, cells.scale, cells.extent);

    for (coord_type j = 0; j < cells.extent.y(); ++j)
    {
        for (coord_type i = 0; i < cells.extent.x(); ++i)
        {
            discrete_point_type cell_position {i, j};
            if (!stratum_format::floor_door_cell(cells, cell_position))
            {
                continue;
            }

            DirectionMask walls = DirectionMask::None;

            for (size_t index = 0; index < cardinal_directions.size(); ++index)
            {
                if (stratum_format::has_wall(cells, cell_position, cardinal_directions[index]))
                {
                    walls |= direction_mask(static_cast<CardinalDirection>(index));
                }
            }

            layout->set_cell_open(cell_position, true);
            layout->set_cell_walls(cell_position, walls);
        }
    }

    std::vector<std::set<discrete_point_type>> rooms = stratum_format::room_cells(document);

    for (id_type room_id = 0; room_id < rooms.size(); ++room_id)
    {
        for (const discrete_point_type& cell_position : rooms[room_id])
        {
            layout->set_cell_room(cell_position, room_id);
        }
    }

    layout->set_room_count(rooms.size());

    const stratum_format::Flat* floor = nullptr;
    const stratum_format::Flat* ceiling = nullptr;

    for (const stratum_format::Flat& flat : document.flats)
    {
        (flat.role == stratum_format::FlatRole::Floor ? floor : ceiling) = &flat;
    }

    layout->set_flats(floor != nullptr ? floor->properties.height.value_or(value_type(0)) : value_type(0),
        ceiling != nullptr ? ceiling->properties.height.value_or(value_type(1)) : value_type(1));

    for (const auto& [flat, flat_id] : {std::pair {floor, layout->floor_id()}, std::pair {ceiling, layout->ceiling_id()}})
    {
        if (flat == nullptr)
        {
            continue;
        }

        for (const auto& [room_id, properties] : flat->rooms)
        {
            if (properties.height.has_value())
            {
                layout->flat(flat_id).set_room_height(room_id, *properties.height);
            }
        }

        for (const auto& [cell_position, properties] : flat->cells)
        {
            if (properties.height.has_value())
            {
                layout->flat(flat_id).set_cell_height(cell_position, *properties.height);
            }
        }
    }

    if (door)
    {
        for (const std::vector<stratum_format::WallAddress>& addresses : stratum_format::door_walls(cells))
        {
            std::vector<id_type> wall_ids;

            for (const stratum_format::WallAddress& address : addresses)
            {
                wall_ids.push_back(layout->wall_toward(address.cell_position, -address.normal));
            }

            door(wall_ids);
        }
    }

    if (notation != nullptr)
    {
        *notation = cells.notation;
    }

    return layout;
}
