//------------------------------------------------------------------------------
// stratum_sim
// make_document.cpp
//------------------------------------------------------------------------------

#include "make_document.hpp"
#include "AccessPoint.hpp"
#include "DiscreteLayout.hpp"
#include "Name.hpp"
#include "file_components.hpp"
#include "layout_entities.hpp"
#include "PersistentId.hpp"
#include "properties.hpp"
#include "Site.hpp"
#include "Zone.hpp"
#include "quetzal/stratum_format/ZoneDocument.hpp"
#include "quetzal/stratum_format/CharacterDocument.hpp"
#include "quetzal/stratum_format/saved_game_format.hpp"
#include "quetzal/stratum_format/references.hpp"
#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <cassert>

namespace quetzal::stratum_sim
{
namespace
{

    using file_components_type = std::vector<std::pair<std::string, flecs::entity_t>>;

    //--------------------------------------------------------------------------
    // The value at ptr of a reflected type, as a zone file writes it, the reverse of load_layout's decoding. An unset
    // optional has no value, so the member that holds it is left out
    std::optional<stratum_format::raw_values_type> encoded_value(const flecs::world& world, flecs::entity_t type, const void* ptr)
    {
        const ecs_world_t* w = world.c_ptr();

        if (const auto* optional_field = static_cast<const OptionalField*>(ecs_get_id(w, type, world.id<OptionalField>())))
        {
            const void* value = optional_field->value(ptr);
            return value != nullptr ? encoded_value(world, optional_field->type, value) : std::nullopt;
        }

        if (type == world.id<EntityReference>())
        {
            return stratum_format::make_reference(static_cast<const EntityReference*>(ptr)->persistent_id.id);
        }

        if (const EcsStruct* struct_type = ecs_get(w, type, EcsStruct))
        {
            stratum_format::raw_values_type object = stratum_format::raw_values_type::object();

            for (const ecs_member_t& member : std::span(ecs_vec_first_t(&struct_type->members, ecs_member_t),
                static_cast<size_t>(ecs_vec_count(&struct_type->members))))
            {
                const auto* member_ptr = static_cast<const std::byte*>(ptr) + member.offset;

                if (std::optional<stratum_format::raw_values_type> value = encoded_value(world, member.type, member_ptr))
                {
                    object[member.name] = std::move(*value);
                }
            }

            return object;
        }

        char* json = ecs_ptr_to_json(w, type, ptr);
        stratum_format::raw_values_type value = stratum_format::raw_values_type::parse(json);
        ecs_os_free(json);

        return value;
    }

    //--------------------------------------------------------------------------
    // The entity's own file components, by name. A tag is an empty object, and a component it reads from its type through
    // inheritance is left out
    stratum_format::raw_values_type encoded_components(flecs::entity e, const file_components_type& file_components)
    {
        flecs::world world = e.world();
        stratum_format::raw_values_type components = stratum_format::raw_values_type::object();

        for (const auto& [name, component] : file_components)
        {
            if (!ecs_owns_id(world.c_ptr(), e, component))
            {
                continue;
            }

            components[name] = is_tag(world, component) ? stratum_format::raw_values_type::object()
                : *encoded_value(world, component, ecs_get_id(world.c_ptr(), e, component));
        }

        return components;
    }

    //--------------------------------------------------------------------------
    // The entity's file components if it exists and has any, and otherwise none
    stratum_format::raw_values_type encoded_layout_components(flecs::entity e, const file_components_type& file_components)
    {
        return e ? encoded_components(e, file_components) : stratum_format::raw_values_type::object();
    }

    //--------------------------------------------------------------------------
    stratum_format::FlatRole flat_role(const DiscreteLayout& layout, id_type flat_id)
    {
        return flat_id == layout.floor_id() ? stratum_format::FlatRole::Floor : stratum_format::FlatRole::Ceiling;
    }

    //--------------------------------------------------------------------------
    // The wall as a zone file places it: its cell and the normal pointing out of the wall, away from the cell
    stratum_format::WallPlacement encoded_wall_placement(const DiscreteLayout& layout, id_type wall_id)
    {
        return stratum_format::WallPlacement{layout.wall_cell_position(wall_id), -layout.wall_direction(wall_id)};
    }

    //--------------------------------------------------------------------------
    // Sets the position and orientation of a site relative to its address or its parent, each only if the placement has one
    void set_relative_placement(stratum_format::Site& site, const placement_type& placement)
    {
        const point_type& position = placement.position();
        if (position.x() != value_type(0) || position.y() != value_type(0))
        {
            site.position = stratum_format::surface_point_type(position.x(), position.y());
        }

        if (placement.direction() != vector_type())
        {
            site.orientation = grid_step(placement.direction());
        }

        return;
    }

    //--------------------------------------------------------------------------
    // The entity's site as a zone file writes it. An entity in the zone with no address is in the cell its placement is in
    stratum_format::Site encoded_site(const Site& site, const DiscreteLayout& layout, bool in_zone)
    {
        stratum_format::Site result;
        result.relation = site.relation == Relation::On ? stratum_format::Relation::On : stratum_format::Relation::In;

        if (site.address.has_value())
        {
            if (const auto* wall = std::get_if<WallId>(&*site.address))
            {
                result.address = encoded_wall_placement(layout, wall->wall_id);
            }
            else
            {
                const auto& flat_cell = std::get<FlatCellPosition>(*site.address);
                result.address = stratum_format::FlatCell{flat_role(layout, flat_cell.flat_id), flat_cell.cell_position};
            }

            set_relative_placement(result, site.placement);
        }
        else if (in_zone)
        {
            result.address = stratum_format::ZoneCell{layout.cell_position_at(site.placement.position())};
            result.orientation = grid_step(site.placement.direction());
        }
        else
        {
            set_relative_placement(result, site.placement);
        }

        return result;
    }

    //--------------------------------------------------------------------------
    // The access point as a zone file writes it: a bound target is the persistent id of the access point it is bound to
    stratum_format::AccessPoint encoded_access_point(flecs::entity e, const AccessPoint& access_point)
    {
        stratum_format::AccessPoint result{access_type_name(e), access_point.order, std::nullopt};

        if (!access_point.target.has_value())
        {
            return result;
        }

        if (const auto* unbound = std::get_if<stratum_format::AccessPointDestination>(&*access_point.target))
        {
            result.target = *unbound;
        }
        else
        {
            result.target = std::get<EntityReference>(*access_point.target).persistent_id.id;
        }

        return result;
    }

    //--------------------------------------------------------------------------
    // Appends each entity below the parent, depth first, leaving out the layout entities, whose components the layout's
    // sections have
    void collect_entities(flecs::entity parent, std::vector<flecs::entity>& entities)
    {
        parent.children([&](flecs::entity child)
        {
            if (child.has<LayoutEntity>())
            {
                return;
            }

            entities.push_back(child);
            collect_entities(child, entities);
        });

        return;
    }

    //--------------------------------------------------------------------------
    // Each entity below the root, in persistent id order
    std::vector<flecs::entity> subtree_by_persistent_id(flecs::entity root)
    {
        std::vector<flecs::entity> entities;
        collect_entities(root, entities);
        std::ranges::sort(entities, {}, [](flecs::entity e) { return e.get<PersistentId>(); });

        return entities;
    }

    //--------------------------------------------------------------------------
    // The entity as a file writes it. A child of the zone has no parent in the file, and is in the zone
    stratum_format::Entity encoded_entity(flecs::entity e, flecs::entity zone, const DiscreteLayout& layout, const file_components_type& file_components)
    {
        stratum_format::Entity entity;
        entity.id = e.get<PersistentId>().id;

        if (flecs::entity type = e.target(flecs::IsA))
        {
            entity.type = type.name().c_str();
        }

        flecs::entity parent = e.parent();
        if (parent != zone)
        {
            entity.parent_id = parent.get<PersistentId>().id;
        }

        if (const Site* site = e.try_get<Site>())
        {
            entity.site = encoded_site(*site, layout, parent == zone);
        }

        if (const AccessPoint* access_point = e.try_get<AccessPoint>())
        {
            entity.access_point = encoded_access_point(e, *access_point);
        }

        entity.components = encoded_components(e, file_components);
        return entity;
    }

    //--------------------------------------------------------------------------
    // Sets the components of each flat override entry from its entity, adds an entry for an override entity with components
    // that has none, and removes an entry left with neither a height nor components
    template<typename Key>
    void update_flat_entries(std::map<Key, stratum_format::FlatProperties>& entries, const file_components_type& file_components,
        auto override_entity, auto for_each_override)
    {
        for (auto& [key, properties] : entries)
        {
            properties.components = encoded_layout_components(override_entity(key), file_components);
        }

        for_each_override([&](const Key& key, flecs::entity e)
        {
            if (stratum_format::raw_values_type components = encoded_components(e, file_components); !components.empty())
            {
                entries[key].components = std::move(components);
            }

            return;
        });

        std::erase_if(entries, [](const auto& entry) { return !entry.second.height.has_value() && entry.second.components.empty(); });
        return;
    }

} // namespace
} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
quetzal::stratum_format::ZoneDocument quetzal::stratum_sim::make_document(flecs::entity zone)
{
    assert(zone && zone.has<Zone>() && zone.has<Document>());

    flecs::world world = zone.world();
    file_components_type file_components = stratum_sim::file_components(world);

    stratum_format::ZoneDocument document = *zone.get<Document>().document;
    document.version = std::string(stratum_format::format_version);
    document.zone_id = zone.get<PersistentId>().id;
    if (const auto* position = zone.try_get<Position>())
    {
        document.position = stratum_format::zone_position_type{position->x, position->y, position->z};
    }
    document.components = encoded_components(zone, file_components);

    if (!zone.has<Layout>())
    {
        return document;
    }

    const DiscreteLayout& layout = layout_as<DiscreteLayout>(*zone.get<Layout>().layout);

    for (size_t room_id = 0; room_id < document.rooms.size(); ++room_id)
    {
        document.rooms[room_id].components = encoded_layout_components(room_entity(zone, room_id), file_components);
    }

    // The walls the document lists keep their order, and a wall given components during play follows them
    std::vector<stratum_format::Wall> walls;

    for (const stratum_format::Wall& wall : document.walls)
    {
        stratum_format::raw_values_type components = encoded_layout_components(wall_entity(zone, layout.wall_toward(wall.placement.cell_position,
            -wall.placement.normal)), file_components);

        if (!components.empty())
        {
            walls.push_back(stratum_format::Wall{wall.placement, std::move(components)});
        }
    }

    if (const auto* index = zone.try_get<EntityIndex<WallId>>())
    {
        for (const auto& [wall, e] : index->entities)
        {
            stratum_format::WallPlacement file_placement = encoded_wall_placement(layout, wall.wall_id);
            stratum_format::raw_values_type components = encoded_components(e, file_components);

            if (!components.empty() && std::ranges::find(walls, file_placement, &stratum_format::Wall::placement) == walls.end())
            {
                walls.push_back(stratum_format::Wall{file_placement, std::move(components)});
            }
        }
    }

    document.walls = std::move(walls);

    for (stratum_format::Flat& flat : document.flats)
    {
        id_type flat_id = flat.role == stratum_format::FlatRole::Floor ? layout.floor_id() : layout.ceiling_id();
        flat.properties.components = encoded_layout_components(flat_entity(zone, flat_id), file_components);

        update_flat_entries(flat.rooms, file_components, [&](id_type room_id) { return flat_room_entity(zone, flat_id, room_id); },
            [&](auto f)
            {
                if (const auto* index = flat_entity(zone, flat_id).try_get<EntityIndex<RoomId>>())
                {
                    for (const auto& [room, e] : index->entities)
                    {
                        f(room.room_id, e);
                    }
                }

                return;
            });

        update_flat_entries(flat.cells, file_components,
            [&](const discrete_point_type& cell_position) { return flat_cell_entity(zone, flat_id, cell_position); },
            [&](auto f)
            {
                if (const auto* index = zone.try_get<EntityIndex<FlatCellPosition>>())
                {
                    for (const auto& [flat_cell, e] : index->entities)
                    {
                        if (flat_cell.flat_id == flat_id)
                        {
                            f(flat_cell.cell_position, e);
                        }
                    }
                }

                return;
            });
    }

    document.access_points.clear();
    document.entities.clear();

    for (flecs::entity e : subtree_by_persistent_id(zone))
    {
        if (e.has<AccessPoint>())
        {
            document.access_points.push_back(e.get<PersistentId>().id);
        }

        document.entities.push_back(encoded_entity(e, zone, layout, file_components));
    }

    return document;
}

//------------------------------------------------------------------------------
quetzal::stratum_format::CharacterDocument quetzal::stratum_sim::make_character_document(flecs::entity character)
{
    flecs::entity zone = zone_of(character);
    assert(zone && zone.has<Layout>() && character.has<Name>());

    flecs::world world = character.world();
    const DiscreteLayout& layout = layout_as<DiscreteLayout>(*zone.get<Layout>().layout);
    file_components_type file_components = stratum_sim::file_components(world);
    const placement_type& placement = character.get<Site>().placement;

    stratum_format::CharacterDocument document;
    document.version = std::string(stratum_format::saved_game_format_version);
    document.name = character.get<Name>().name;
    document.id = character.get<PersistentId>().id;
    document.components = encoded_components(character, file_components);
    document.zone_id = zone.get<PersistentId>().id;
    document.placement = stratum_format::discrete_directed_point_type(layout.cell_position_at(placement.position()), grid_step(placement.direction()));

    for (flecs::entity e : subtree_by_persistent_id(character))
    {
        document.inventory.push_back(encoded_entity(e, zone, layout, file_components));
    }

    return document;
}

//------------------------------------------------------------------------------
quetzal::stratum_format::ZoneDocument quetzal::stratum_sim::rebuild_document(flecs::entity zone)
{
    zone; // rebuild the cells, rooms, barriers, walls and flats from the layout, then the rest as make_document does ...

    return {};
}
