//------------------------------------------------------------------------------
// stratum_sim
// load_layout.cpp
//------------------------------------------------------------------------------

#include "load_layout.hpp"
#include "attachments.hpp"
#include "decode_components.hpp"
#include "DiscreteLayout.hpp"
#include "file_components.hpp"
#include "PersistentId.hpp"
#include "layout_entities.hpp"
#include "properties.hpp"
#include "World.hpp"
#include "positions.hpp"
#include "Zone.hpp"
#include "quetzal/stratum_format/Document.hpp"
#include "quetzal/stratum_format/grid.hpp"
#include "quetzal/stratum_format/references.hpp"
#include <algorithm>
#include <format>
#include <functional>
#include <string>
#include <cassert>
#include <cstdint>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace
{

    //--------------------------------------------------------------------------
    // Returns the access point's entity. The reader has checked that it is sited on a surface and has a relocator with a
    // type, order, and placement
    const quetzal::stratum_format::Entity& access_point_entity(const quetzal::stratum_format::Document& document,
        quetzal::stratum_format::stored_entity_id_type id)
    {
        return *std::find_if(document.entities.begin(), document.entities.end(), [&](const auto& e) { return e.id == id; });
    }

    //--------------------------------------------------------------------------
    // Converts the file's site to the layout's: the address to layout ids, and the position and orientation to a placement
    // relative to the address
    quetzal::stratum_sim::Site site_of(const quetzal::stratum_format::Site& site, const quetzal::stratum_sim::DiscreteLayout& layout)
    {
        using namespace quetzal;
        using namespace quetzal::stratum_sim;

        Site result {site.relation == stratum_format::Relation::On ? Relation::On : Relation::In, {}, {}};

        // An entity in a cell of the zone has no address: its placement is the cell and its orientation
        if (const auto* zone_cell = site.address.has_value() ? std::get_if<stratum_format::ZoneCell>(&*site.address) : nullptr)
        {
            result.placement = placement_type(layout.position(zone_cell->cell_position), vector_type(static_cast<value_type>(site.orientation->x()),
                static_cast<value_type>(site.orientation->y()), value_type(0)));
            return result;
        }

        if (site.address.has_value())
        {
            if (const auto* wall = std::get_if<stratum_format::WallAddress>(&*site.address))
            {
                result.address = WallAddress {layout.wall_toward(wall->cell_position, -wall->normal)};
            }
            else
            {
                const auto& flat_cell = std::get<stratum_format::FlatCell>(*site.address);
                result.address = FlatCellAddress {flat_cell.role == stratum_format::FlatRole::Floor ? layout.floor_id() : layout.ceiling_id(),
                    flat_cell.cell_position};
            }
        }

        if (site.position.has_value())
        {
            result.placement.position() = point_type(site.position->x(), site.position->y(), value_type(0));
        }

        if (site.orientation.has_value())
        {
            result.placement.direction() = vector_type(static_cast<value_type>(site.orientation->x()),
                static_cast<value_type>(site.orientation->y()), value_type(0));
        }

        return result;
    }

    //--------------------------------------------------------------------------
    // A bound target, which only a saved zone has, is the persistent id of a relocator in this zone or another
    std::variant<quetzal::stratum_format::RelocatorTarget, quetzal::stratum_sim::EntityReference> relocator_target(
        const std::variant<quetzal::stratum_format::RelocatorTarget, quetzal::stratum_format::stored_entity_id_type>& target,
        const std::unordered_map<quetzal::stratum_format::stored_entity_id_type, flecs::entity>& entities)
    {
        using namespace quetzal::stratum_sim;

        if (const auto* unbound = std::get_if<quetzal::stratum_format::RelocatorTarget>(&target))
        {
            return *unbound;
        }

        quetzal::stratum_format::stored_entity_id_type id = std::get<quetzal::stratum_format::stored_entity_id_type>(target);

        auto i = entities.find(id);
        return EntityReference {PersistentId {id}, i != entities.end() ? i->second : flecs::entity()};
    }

    //--------------------------------------------------------------------------
    // The same paths the reader writes in its errors
    std::string flat_room_path(const std::string& flat_path, quetzal::id_type room_id)
    {
        return std::format("{}.rooms[room {}]", flat_path, room_id);
    }

    std::string flat_cell_path(const std::string& flat_path, const quetzal::stratum_format::discrete_point_type& cell_position)
    {
        return std::format("{}.cells[cell [{}, {}]]", flat_path, cell_position.i(), cell_position.j());
    }

    //--------------------------------------------------------------------------
    // Returns the components of the layout and of each room, wall, flat, and flat override for a room or a cell, with the
    // path to each
    std::vector<std::pair<const quetzal::stratum_format::raw_values_type*, std::string>> layout_component_objects(
        const quetzal::stratum_format::Document& document)
    {
        std::vector<std::pair<const quetzal::stratum_format::raw_values_type*, std::string>> objects {{&document.components, "layout"}};

        for (size_t i = 0; i < document.rooms.size(); ++i)
        {
            objects.emplace_back(&document.rooms[i].components, std::format("rooms[{}]", i));
        }

        for (size_t i = 0; i < document.walls.size(); ++i)
        {
            objects.emplace_back(&document.walls[i].components, std::format("walls[{}]", i));
        }

        for (size_t i = 0; i < document.flats.size(); ++i)
        {
            std::string flat_path = std::format("flats[{}]", i);
            objects.emplace_back(&document.flats[i].properties.components, flat_path);

            for (const auto& [room_id, properties] : document.flats[i].rooms)
            {
                objects.emplace_back(&properties.components, flat_room_path(flat_path, room_id));
            }

            for (const auto& [cell_position, properties] : document.flats[i].cells)
            {
                objects.emplace_back(&properties.components, flat_cell_path(flat_path, cell_position));
            }
        }

        return objects;
    }

} // namespace

//------------------------------------------------------------------------------
std::optional<std::string> quetzal::stratum_sim::check_buildable(const flecs::world& world, const stratum_format::Document& document)
{
    for (stratum_format::stored_entity_id_type id : document.access_points)
    {
        const std::string& type_name = *access_point_entity(document, id).relocator->type;
        if (!access_type(world, type_name))
        {
            return std::format("access point type '{}' is not registered", type_name);
        }
    }

    // Flecs' error logging is silenced, since the returned error is the report
    int log_level = ecs_log_set_level(-4); // Errors log at -3
    std::optional<std::string> error;

    for (const auto& [components, path] : layout_component_objects(document))
    {
        if (!error.has_value())
        {
            error = check_decodable(world, *components, path);
        }
    }

    for (size_t i = 0; i < document.entities.size() && !error.has_value(); ++i)
    {
        error = check_decodable(world, document.entities[i].components, std::format("entities[{}]", i));
    }

    ecs_log_set_level(log_level);
    return error;
}

//------------------------------------------------------------------------------
std::vector<flecs::entity> quetzal::stratum_sim::load_layout(flecs::entity zone, const stratum_format::Document& document)
{
    assert(zone && zone.has<Zone>() && !zone.has<ZoneLayout>());

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

    const DiscreteLayout& zone_layout = *layout;
    set_layout(zone, std::move(layout));
    zone.set<ZoneNotation>(ZoneNotation {cells.notation});

    auto zone_document = std::make_shared<stratum_format::Document>(document);
    zone_document->entities.clear();
    zone_document->is_unpopulated = false;
    zone.set<ZoneDocument>(ZoneDocument {std::move(zone_document)});

    if (document.level.has_value() && !zone.has<ZonePosition>())
    {
        zone.set<ZonePosition>(zone_position_at_level(*document.level));
    }

    flecs::world world = zone.world();
    std::unordered_map<stratum_format::stored_entity_id_type, flecs::entity> entities; // <file_id, entity>

    // Every entity is created in the zone first, in the file's order, so each has its persistent id before a parent or a
    // site refers to it
    for (const stratum_format::Entity& entity : document.entities)
    {
        flecs::entity e = world.entity();

        if (document.zone_id.has_value())
        {
            e.set<PersistentId>(PersistentId {entity.id});
        }

        // A type that is not loaded is made with no components, so the entity keeps its type
        if (!entity.type.empty())
        {
            flecs::entity type = entity_type(world, entity.type);
            e.is_a(type ? type : create_entity_type(world, entity.type));
        }

        entities.emplace(entity.id, e.child_of(zone));
    }

    for (const stratum_format::Entity& entity : document.entities)
    {
        flecs::entity e = entities.at(entity.id);

        if (entity.parent_id.has_value())
        {
            set_parent(e, entities.at(*entity.parent_id));
        }

        if (entity.site.has_value())
        {
            e.set<Site>(site_of(*entity.site, zone_layout));
        }

        if (entity.relocator.has_value())
        {
            e.set<Relocator>(Relocator {relocator_target(entity.relocator->target, entities)});
        }
    }

    for (stratum_format::stored_entity_id_type id : document.access_points)
    {
        const stratum_format::Entity& entity = access_point_entity(document, id);
        const stratum_format::Relocator& relocator = *entity.relocator;
        const stratum_format::discrete_directed_point_type& placement = *relocator.placement;

        vector_type orientation(static_cast<value_type>(placement.direction().x()), static_cast<value_type>(placement.direction().y()), value_type(0));
        set_access_point(entities.at(entity.id), access_type(world, *relocator.type), *relocator.order,
            placement_type(zone_layout.position(placement.position()), orientation));
    }

    // A template's references are to its own entities, and a saved zone's references are already persistent ids
    std::vector<flecs::entity> referred_to;
    auto resolve = [&](stratum_format::stored_entity_id_type id)
    {
        if (document.zone_id.has_value())
        {
            auto i = entities.find(id);
            return EntityReference {PersistentId {id}, i != entities.end() ? i->second : flecs::entity()};
        }

        flecs::entity e = entities.at(id);
        referred_to.push_back(e);
        return EntityReference {e.get<PersistentId>(), e};
    };

    decode_components(zone, document.components, "layout", resolve);

    // Every room and both flats have an entity. A wall, or a flat's override for a room or a cell, has one only if the
    // file has components for it
    for (id_type room_id = 0; room_id < zone_layout.room_count(); ++room_id)
    {
        ensure_room_entity(zone, room_id);
    }

    ensure_flat_entity(zone, zone_layout.floor_id());
    ensure_flat_entity(zone, zone_layout.ceiling_id());

    auto decode_layout_components = [&](const auto& address, const stratum_format::raw_values_type& components, const std::string& path)
    {
        if (!component_objects(components).empty())
        {
            decode_components(ensure_layout_entity(zone, address), components, path, resolve);
        }
    };

    for (size_t i = 0; i < document.rooms.size(); ++i)
    {
        decode_layout_components(RoomAddress {i}, document.rooms[i].components, std::format("rooms[{}]", i));
    }

    for (size_t i = 0; i < document.walls.size(); ++i)
    {
        const stratum_format::WallAddress& wall = document.walls[i].address;
        decode_layout_components(WallAddress {zone_layout.wall_toward(wall.cell_position, -wall.normal)}, document.walls[i].components, std::format("walls[{}]", i));
    }

    for (size_t i = 0; i < document.flats.size(); ++i)
    {
        const stratum_format::Flat& flat = document.flats[i];
        id_type flat_id = flat.role == stratum_format::FlatRole::Floor ? zone_layout.floor_id() : zone_layout.ceiling_id();
        std::string flat_path = std::format("flats[{}]", i);

        decode_layout_components(FlatAddress {flat_id}, flat.properties.components, flat_path);

        for (const auto& [room_id, properties] : flat.rooms)
        {
            decode_layout_components(FlatRoomAddress {flat_id, room_id}, properties.components, flat_room_path(flat_path, room_id));
        }

        for (const auto& [cell_position, properties] : flat.cells)
        {
            decode_layout_components(FlatCellAddress {flat_id, cell_position}, properties.components, flat_cell_path(flat_path, cell_position));
        }
    }

    for (size_t i = 0; i < document.entities.size(); ++i)
    {
        decode_components(entities.at(document.entities[i].id), document.entities[i].components, std::format("entities[{}]", i), resolve);
    }

    return referred_to;
}
