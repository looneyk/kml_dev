//------------------------------------------------------------------------------
// stratum_sim
// load_layout.cpp
//------------------------------------------------------------------------------

#include "load_layout.hpp"
#include "attachments.hpp"
#include "DiscreteLayout.hpp"
#include "Zone.hpp"
#include "quetzal/stratum_format/grid.hpp"
#include "quetzal/stratum_format/read.hpp"
#include "quetzal/stratum_format/references.hpp"
#include <algorithm>
#include <format>
#include <stdexcept>
#include <string>
#include <cassert>
#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <vector>

//------------------------------------------------------------------------------
void quetzal::stratum_sim::load_layout(flecs::entity zone, const std::filesystem::path& filepath)
{
    assert(zone && zone.has<Zone>() && !zone.has<ZoneLayout>());

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

    door_walls_type doors;

    for (const std::vector<stratum_format::WallAddress>& addresses : stratum_format::door_walls(cells))
    {
        std::vector<id_type> wall_ids;

        for (const stratum_format::WallAddress& address : addresses)
        {
            wall_ids.push_back(layout->wall_toward(address.cell_position, -address.normal));
        }

        doors.push_back(wall_ids);
    }

    struct AccessPointData
    {
        flecs::entity access_type;
        Site site;
        std::uint64_t order = 0;
        placement_type entry;
    };

    std::vector<AccessPointData> access_points;

    // The reader has checked each is an entity sited on a surface, with a relocator stating its type, order and entry
    for (const stratum_format::values_type& reference : document.components.at("access_points"))
    {
        stratum_format::stored_entity_id_type id = *stratum_format::reference_id(reference);
        auto entity = std::find_if(document.entities.begin(), document.entities.end(), [&](const auto& e) { return e.id == id; });
        const stratum_format::values_type& relocator = entity->components.at("relocator");

        std::string type_name = relocator.at("type").get<std::string>();
        flecs::entity type = access_type(zone.world(), type_name);
        if (!type)
        {
            throw std::runtime_error(std::format("{}: access point type '{}' is not registered", filepath.string(), type_name));
        }

        Site site {Relation::On, {}, {}};
        if (const auto* wall = std::get_if<stratum_format::WallAddress>(&*entity->site->address))
        {
            site.address = WallAddress {layout->wall_toward(wall->cell_position, -wall->normal)};
        }
        else
        {
            const auto& flat = std::get<stratum_format::FlatCell>(*entity->site->address);
            site.address = FlatAddress {flat.role == stratum_format::FlatRole::Floor ? layout->floor_id() : layout->ceiling_id(), flat.cell_position};
        }

        const stratum_format::values_type& entry = relocator.at("entry");
        discrete_point_type entry_cell {entry.at("cell_position").at(0).get<coord_type>(), entry.at("cell_position").at(1).get<coord_type>()};
        vector_type orientation(entry.at("orientation").at(0).get<value_type>(), entry.at("orientation").at(1).get<value_type>(), value_type(0));

        access_points.push_back({type, site, relocator.at("order").get<std::uint64_t>(), placement_type(layout->position(entry_cell), orientation)});
    }

    zone.set<ZoneLayout>(ZoneLayout {std::move(layout)});
    zone.set<ZoneNotation>(ZoneNotation {cells.notation});
    create_doors(zone, doors);

    for (const AccessPointData& access_point : access_points)
    {
        create_access_point(zone, access_point.access_type, access_point.site, access_point.order, access_point.entry);
    }

    return;
}
