//------------------------------------------------------------------------------
// stratum_sim
// World.cpp
//------------------------------------------------------------------------------

#include "World.hpp"
#include "AccessPoint.hpp"
#include "actors.hpp"
#include "ai.hpp"
#include "attachments.hpp"
#include "DiscreteLayout.hpp"
#include "DiscreteProximityIndex.hpp"
#include "Name.hpp"
#include "file_components.hpp"
#include "layout_entities.hpp"
#include "load_layout.hpp"
#include "make_document.hpp"
#include "PatternResolver.hpp"
#include "properties.hpp"
#include "seeds.hpp"
#include "Site.hpp"
#include "specs.hpp"
#include "stratum_sim_util.hpp"
#include "quetzal/log/Log.hpp"
#include "quetzal/stratum_format/ZoneDocument.hpp"
#include "quetzal/stratum_format/read_world.hpp"
#include "quetzal/stratum_format/read_zone.hpp"
#include "quetzal/stratum_format/saved_game_format.hpp"
#include "quetzal/stratum_format/WorldDocument.hpp"
#include "quetzal/stratum_format/write_world.hpp"
#include "quetzal/stratum_format/write_zone.hpp"
#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <format>
#include <map>
#include <numeric>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
#include <variant>
#include <cassert>

namespace
{

    static constexpr int denizen_speed = 100;

    constexpr std::string_view default_zone_name = "&region_name .. '_' .. zone_counter";

    const quetzal::FormatSpec comma_separated {.separator = ","};


    //--------------------------------------------------------------------------
    // The name of a zone composed of the region's name and the zone's coordinates, 2D or 3D depending on context
    std::string positional_zone_name(const quetzal::stratum_sim::Worldspace& worldspace, std::string_view region_name,
        const quetzal::stratum_sim::Position& position)
    {
        return worldspace.zone_size.has_value() ? std::format("{}_{}_{}", region_name, position.x, position.y)
            : std::format("{}_{}_{}_{}", region_name, position.x, position.y, position.z);
    }

    //--------------------------------------------------------------------------
    // A zone's symbol table
    quetzal::KeyValueStore zone_symbols(const quetzal::stratum_sim::Region& region, const quetzal::stratum_sim::Position& position,
        std::size_t zone_counter)
    {
        quetzal::KeyValueStore symbols {{"x", std::to_string(position.x)}, {"y", std::to_string(position.y)}, {"z", std::to_string(position.z)},
            {"zone_counter", std::to_string(zone_counter)}, {"region_name", region.name}};

        for (const auto& [key, value] : region.region_spec.store)
        {
            if (!key.starts_with(quetzal::stratum_sim::zone_prefix))
            {
                symbols.insert(std::format("region.{}", key), value);
            }
        }

        return symbols;
    }

    //--------------------------------------------------------------------------
    // The half-open extent along an axis, or the parent's extent on that axis if not given
    std::pair<quetzal::stratum_sim::coord_type, quetzal::stratum_sim::coord_type> axis_extent(
        const std::optional<quetzal::math::Interval<quetzal::stratum_sim::coord_type>>& axis, const quetzal::stratum_sim::Worldspace::extent_type& parent_box,
        std::size_t i)
    {
        return axis.has_value() ? std::pair {axis->lower(), axis->upper() + 1} : std::pair {parent_box.lower(i), parent_box.upper(i)};
    }

    //--------------------------------------------------------------------------
    // Calls the function with each position of the extent
    template<std::invocable<const quetzal::stratum_sim::Position&> Function>
    void for_each_position(const quetzal::stratum_format::RegionExtent& extent, const quetzal::stratum_sim::Worldspace::extent_type& parent_box, Function function)
    {
        using quetzal::stratum_sim::coord_type;

        for (const auto& position : extent.positions)
        {
            function(quetzal::stratum_sim::Position {position.x(), position.y(), position.z()});
        }

        for (const auto& box : extent.boxes)
        {
            const auto [x_lower, x_upper] = axis_extent(box.x_extent, parent_box, 0);
            const auto [y_lower, y_upper] = axis_extent(box.y_extent, parent_box, 1);
            const auto [z_lower, z_upper] = axis_extent(box.z_extent, parent_box, 2);

            for (coord_type z = z_lower; z < z_upper; ++z)
            {
                for (coord_type y = y_lower; y < y_upper; ++y)
                {
                    for (coord_type x = x_lower; x < x_upper; ++x)
                    {
                        function(quetzal::stratum_sim::Position {x, y, z});
                    }
                }
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Returns the smallest box that contains every position of the extent, or an empty box at the parent's lower corner for an extent with none
    quetzal::stratum_sim::Worldspace::extent_type bounding_box(const quetzal::stratum_format::RegionExtent& extent, const quetzal::stratum_sim::Worldspace::extent_type& parent_box)
    {
        using box_type = quetzal::stratum_sim::Worldspace::extent_type;

        std::optional<std::array<std::pair<quetzal::stratum_sim::coord_type, quetzal::stratum_sim::coord_type>, 3>> bounds;
        auto include = [&bounds](const std::array<std::pair<quetzal::stratum_sim::coord_type, quetzal::stratum_sim::coord_type>, 3>& ranges)
        {
            if (!bounds.has_value())
            {
                bounds = ranges;
                return;
            }

            for (std::size_t i = 0; i < 3; ++i)
            {
                (*bounds)[i] = {std::min((*bounds)[i].first, ranges[i].first), std::max((*bounds)[i].second, ranges[i].second)};
            }

            return;
        };

        for (const auto& position : extent.positions)
        {
            include({std::pair {position.x(), position.x() + 1}, std::pair {position.y(), position.y() + 1}, std::pair {position.z(), position.z() + 1}});
        }

        for (const auto& box : extent.boxes)
        {
            include({axis_extent(box.x_extent, parent_box, 0), axis_extent(box.y_extent, parent_box, 1), axis_extent(box.z_extent, parent_box, 2)});
        }

        if (!bounds.has_value())
        {
            return box_type(parent_box.lower(), parent_box.lower());
        }

        return box_type(box_type::point_type((*bounds)[0].first, (*bounds)[1].first, (*bounds)[2].first),
            box_type::point_type((*bounds)[0].second, (*bounds)[1].second, (*bounds)[2].second));
    }

    //--------------------------------------------------------------------------
    // Temporary placeholder placement function, real placement will not work like this ...
    template<std::predicate<const quetzal::stratum_sim::discrete_point_type&> Free>
    std::optional<quetzal::stratum_sim::discrete_point_type> last_open_cell(const quetzal::stratum_sim::DiscreteLayout& layout, Free free)
    {
        const quetzal::stratum_sim::GridMapping& grid = layout.grid();

        for (quetzal::stratum_sim::coord_type j = grid.extent.y(); j-- > 0; )
        {
            for (quetzal::stratum_sim::coord_type i = grid.extent.x(); i-- > 0; )
            {
                if (layout.open(quetzal::stratum_sim::discrete_point_type {i, j}) && free(quetzal::stratum_sim::discrete_point_type {i, j}))
                {
                    return quetzal::stratum_sim::discrete_point_type {i, j};
                }
            }
        }

        return {};
    }

    //--------------------------------------------------------------------------
    // Returns the zone that the entity is a child of, or is in through InZone, or null if it is not in a zone or is in a zone through another entity
    flecs::entity zone_containing(flecs::entity e)
    {
        if (flecs::entity zone = e.target<quetzal::stratum_sim::InZone>())
        {
            return zone;
        }

        flecs::entity parent = e.parent();
        return parent && parent.has<quetzal::stratum_sim::Zone>() ? parent : flecs::entity();
    }

    //--------------------------------------------------------------------------
    quetzal::stratum_sim::World::entities_type entities_of(flecs::entity zone, const quetzal::stratum_sim::ProximityIndexBase::entity_ids_type& entity_ids)
    {
        quetzal::stratum_sim::World::entities_type entities;
        entities.reserve(entity_ids.size());

        for (quetzal::id_type entity_id : entity_ids)
        {
            entities.push_back(flecs::entity(zone.world(), static_cast<flecs::entity_t>(entity_id)));
        }

        return entities;
    }

    //--------------------------------------------------------------------------
    // Returns a function that tests whether the actor can cross a wall, as the actor perceives it
    auto is_crossable(flecs::entity zone, flecs::entity actor)
    {
        return [zone, actor](quetzal::id_type wall_id) { return quetzal::stratum_sim::crossable(zone, wall_id, actor); };
    }

    //--------------------------------------------------------------------------
    // Sets the zone of e and of everything below it in the world index
    void set_entity_zones(std::unordered_map<quetzal::stratum_sim::PersistentId, quetzal::stratum_sim::PersistentId>& entity_zones,
        flecs::entity e, quetzal::stratum_sim::PersistentId zone_id)
    {
        if (const auto* persistent_id = e.try_get<quetzal::stratum_sim::PersistentId>())
        {
            if (auto i = entity_zones.find(*persistent_id); i != entity_zones.end())
            {
                i->second = zone_id;
            }
        }

        e.children([&](flecs::entity child) { set_entity_zones(entity_zones, child, zone_id); });
        return;
    }

    //--------------------------------------------------------------------------
    // Returns true if any descendant of e is sited on a wall or a flat cell, which are only addresses in e's zone
    bool has_descendant_sited_on_layout(flecs::entity e)
    {
        bool found = false;

        e.children([&](flecs::entity child)
        {
            const auto* site = child.try_get<quetzal::stratum_sim::Site>();
            found = found || (site != nullptr && site->address.has_value()) || has_descendant_sited_on_layout(child);
        });

        return found;
    }

    //--------------------------------------------------------------------------
    void append_if(quetzal::stratum_sim::World::entities_type& chain, flecs::entity level)
    {
        if (level)
        {
            chain.push_back(level);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Appends the levels of a layout entity's property chain that lie between the entity and its zone. At the room level and
    // at the layout level, a flat's components come before the room's or the layout's
    void append_layout_levels(quetzal::stratum_sim::World::entities_type& chain, flecs::entity zone, flecs::entity layout_entity,
        const quetzal::stratum_sim::DiscreteLayout*, const quetzal::stratum_sim::RoomId& room)
    {
        if (flecs::entity parent = layout_entity.parent(); parent != zone)
        {
            append_if(chain, quetzal::stratum_sim::room_entity(zone, room.room_id));
            append_if(chain, parent);
        }

        return;
    }

    void append_layout_levels(quetzal::stratum_sim::World::entities_type& chain, flecs::entity zone, flecs::entity,
        const quetzal::stratum_sim::DiscreteLayout* zone_layout, const quetzal::stratum_sim::WallId& wall)
    {
        assert(zone_layout != nullptr);

        if (quetzal::id_type room_id = zone_layout->room_at(zone_layout->wall_cell_position(wall.wall_id)); room_id != quetzal::nullid)
        {
            append_if(chain, quetzal::stratum_sim::room_entity(zone, room_id));
        }

        return;
    }

    void append_layout_levels(quetzal::stratum_sim::World::entities_type&, flecs::entity, flecs::entity,
        const quetzal::stratum_sim::DiscreteLayout*, const quetzal::stratum_sim::FlatId&)
    {
        return;
    }

    void append_layout_levels(quetzal::stratum_sim::World::entities_type& chain, flecs::entity zone, flecs::entity,
        const quetzal::stratum_sim::DiscreteLayout* zone_layout, const quetzal::stratum_sim::FlatCellPosition& flat_cell)
    {
        assert(zone_layout != nullptr);

        if (quetzal::id_type room_id = zone_layout->room_at(flat_cell.cell_position); room_id != quetzal::nullid)
        {
            append_if(chain, quetzal::stratum_sim::flat_room_entity(zone, flat_cell.flat_id, room_id));
            append_if(chain, quetzal::stratum_sim::room_entity(zone, room_id));
        }

        append_if(chain, quetzal::stratum_sim::flat_entity(zone, flat_cell.flat_id));
        return;
    }

    //--------------------------------------------------------------------------
    // Returns true if a layout entity has any component other than its tag, its key, its parent, and the list of entities sited on it
    bool has_own_components(flecs::entity layout_entity)
    {
        flecs::world world = layout_entity.world();
        bool found = false;

        layout_entity.each([&](flecs::id id)
        {
            bool parent = id == world.id<flecs::Parent>() || (id.is_pair() && (id.first() == flecs::ChildOf || id.first() == flecs::ParentDepth));

            bool key = false;
            quetzal::stratum_sim::for_each_layout_entity([&]<typename T>(std::type_identity<T>)
            {
                key = key || id == world.id<T>();
            });

            if (!parent && !key && id != world.id<quetzal::stratum_sim::LayoutEntity>()
                && id != world.id<quetzal::stratum_sim::AttachedEntities>())
            {
                found = true;
            }
        });

        return found;
    }

    //--------------------------------------------------------------------------
    // Types are named entities in their own scope, separate from everything else in the world
    std::string type_path(std::string_view name)
    {
        return std::format("types::{}", name);
    }

    //--------------------------------------------------------------------------
    // The file of a saved zone, relative to the game directory
    std::filesystem::path saved_zone_relative_path(quetzal::stratum_sim::PersistentId zone_id)
    {
        return std::filesystem::path("zones") / std::format("{}.zone", zone_id.id);
    }

    //--------------------------------------------------------------------------
    // Returns the room that contains the entity, found from its site, or from its parent's site if it is sited on its parent
    quetzal::id_type room_id_of(flecs::entity e, const quetzal::stratum_sim::DiscreteLayout& layout)
    {
        for (flecs::entity sited = e; sited && !sited.has<quetzal::stratum_sim::Zone>(); sited = sited.parent())
        {
            const auto* site = sited.try_get<quetzal::stratum_sim::Site>();
            if (site == nullptr)
            {
                return quetzal::nullid;
            }

            if (site->address.has_value())
            {
                if (const auto* wall = std::get_if<quetzal::stratum_sim::WallId>(&*site->address))
                {
                    return layout.room_at(layout.wall_cell_position(wall->wall_id));
                }

                return layout.room_at(std::get<quetzal::stratum_sim::FlatCellPosition>(*site->address).cell_position);
            }

            if (sited.parent().has<quetzal::stratum_sim::Zone>())
            {
                return layout.room_at(layout.cell_position_at(site->placement.position()));
            }
        }

        return quetzal::nullid;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::World::World(IdAllocator& id_allocator, Generation& generation, std::size_t zone_count_max, const ScriptLimits& script_limits) :
    m_id_allocator(id_allocator),
    m_generation(generation),
    m_loaded_entities(),
    m_entity_zones(),
    m_zones_unloading(),
    m_zone_entry_index(zone_count_max),
    m_script_limits(script_limits),
    m_game_directory(),
    m_world_seed(0),
    m_worldspace_name_counter(0),
    m_entities()
{
    // Registered before the observers, whose callbacks must not register a component mid-operation
    m_entities.component<Zone>();
    m_entities.component<Position>().member(&Position::x, "x").member(&Position::y, "y").member(&Position::z, "z");
    m_entities.component<Document>();
    m_entities.component<ProximityIndex>();
    m_entities.component<InZone>().add(flecs::Exclusive);
    m_entities.component<WorldspaceId>();
    m_entities.component<RegionId>();
    m_entities.component<PersistentId>().member(&PersistentId::id, "id");
    m_entities.component<EntityReference>()
        .member(&EntityReference::persistent_id, "persistent_id")
        .member(&EntityReference::entity, "entity");
    m_entities.component<AttachedEntities>();
    m_entities.component<LayoutEntity>();
    m_entities.component<Site>();
    for_each_layout_entity([&]<typename T>(std::type_identity<T>)
    {
        m_entities.component<T>();
        m_entities.component<EntityIndex<T>>();
    });

    m_entities.observer<const PersistentId>()
        .event(flecs::OnSet)
        .each([this](flecs::entity e, const PersistentId& persistent_id)
        {
            [[maybe_unused]] auto [i, inserted] = m_loaded_entities.emplace(persistent_id, world_handle(e));
            assert(inserted || i->second == e);
        });

    m_entities.observer<const PersistentId>()
        .event(flecs::OnRemove)
        .each([this](flecs::entity e, const PersistentId& persistent_id)
        {
            m_loaded_entities.erase(persistent_id);

            bool is_zone = e.has<Zone>();
            flecs::entity zone = is_zone ? e : stratum_sim::zone_of(e);

            if (!zone || !deleting_with(zone))
            {
                m_entity_zones.erase(persistent_id);
            }

            // A zone's contents are deleted before the zone, so its unload is complete here
            if (is_zone)
            {
                m_zones_unloading.erase(persistent_id);
            }
        });

    // A zone with a position is listed in its worldspace at that position, and only there
    m_entities.observer<const Position>()
        .event(flecs::OnSet)
        .each([this](flecs::entity zone, const Position& position)
        {
            std::map<Position, PersistentId>& zone_ids = worldspace(worldspace_of(zone)).zone_ids;
            const PersistentId zone_id = zone.get<PersistentId>();

            std::erase_if(zone_ids, [&](const auto& positioned_zone) { return positioned_zone.second == zone_id && positioned_zone.first != position; });
            [[maybe_unused]] auto [i, inserted] = zone_ids.emplace(position, zone_id);
            assert(inserted || i->second == zone_id);
        });

    // An entity placed in a zone gets its persistent id here, whatever placed it. A new parent or InZone may put it in another
    // zone
    auto parented = [this](flecs::entity e)
    {
        if (e.has<LayoutEntity>())
        {
            return;
        }

        if (flecs::entity zone = stratum_sim::zone_of(e))
        {
            if (!e.has<PersistentId>())
            {
                e.set<PersistentId>(next_id());
            }

            set_entity_zones(m_entity_zones, e, zone.get<PersistentId>());
        }
    };

    m_entities.observer()
        .with(flecs::ChildOf, flecs::Wildcard)
        .event(flecs::OnAdd)
        .each(parented);

    m_entities.observer<const flecs::Parent>()
        .event(flecs::OnSet)
        .each([parented](flecs::entity e, const flecs::Parent&) { parented(e); });

    m_entities.observer()
        .with<InZone>(flecs::Wildcard)
        .event(flecs::OnAdd)
        .each(parented);

    // An entity sited on a wall or a flat cell is listed on the wall's or cell's entity, which is created when the first entity
    // is sited there
    m_entities.observer<const Site>()
        .event(flecs::OnSet)
        .each([this](flecs::entity e, const Site& site)
        {
            if (!site.address.has_value())
            {
                return;
            }

            flecs::entity zone = stratum_sim::zone_of(e);
            assert(zone);

            std::visit([&](const auto& key)
            {
                std::vector<flecs::entity>& attached = ensure_layout_entity(zone, key).template ensure<AttachedEntities>().entities;
                if (std::find(attached.begin(), attached.end(), e) == attached.end())
                {
                    attached.push_back(world_handle(e));
                }
            }, *site.address);
        });

    // An entity in a zone whose site has no address is in the zone's proximity index, at its placement
    m_entities.observer<const Site>()
        .event(flecs::OnSet)
        .each([](flecs::entity e, const Site& site)
        {
            flecs::entity zone = zone_containing(e);
            if (!zone)
            {
                return;
            }

            ProximityIndexBase& proximity_index = *zone.get<ProximityIndex>().proximity_index;

            if (!site.address.has_value())
            {
                proximity_index.place(e.id(), site.placement);
            }
            else if (proximity_index.contains(e.id()))
            {
                proximity_index.remove(e.id());
            }
        });

    m_entities.observer<const Site>()
        .event(flecs::OnRemove)
        .each([this](flecs::entity e, const Site& site)
        {
            if (site.address.has_value())
            {
                return;
            }

            flecs::entity zone = zone_containing(e);
            if (!zone || deleting_with(zone))
            {
                return;
            }

            zone.get<ProximityIndex>().proximity_index->remove(e.id());
        });

    // A wall's or flat cell's entity is deleted when nothing is sited on it and it has no components of its own
    m_entities.observer<const Site>()
        .event(flecs::OnRemove)
        .each([this](flecs::entity e, const Site& site)
        {
            if (!site.address.has_value())
            {
                return;
            }

            flecs::entity zone = stratum_sim::zone_of(e);
            if (!zone || deleting_with(zone))
            {
                return;
            }

            // There is none if the entity it was sited on is being deleted, as that entity leaves the zone's index first
            flecs::entity sited_on = std::visit([&](const auto& address) { return layout_entity(zone, address); }, *site.address);
            if (!sited_on)
            {
                return;
            }

            std::vector<flecs::entity>& attached = sited_on.ensure<AttachedEntities>().entities;
            std::erase(attached, e);

            if (attached.empty() && !has_own_components(sited_on))
            {
                sited_on.destruct();
            }
        });

    // A layout entity that is being deleted is removed from its zone's index, and the entities sited on it lose their sites
    for_each_layout_entity([&]<typename T>(std::type_identity<T>)
    {
        m_entities.observer<const T>()
            .event(flecs::OnRemove)
            .each([this](flecs::entity layout_entity, const T& key)
            {
                flecs::entity zone = stratum_sim::zone_of(layout_entity);
                if (!zone || deleting_with(zone))
                {
                    return;
                }

                erase_layout_entity(zone, layout_entity, key);

                if (const auto* attached = layout_entity.try_get<AttachedEntities>())
                {
                    for (flecs::entity e : attached->entities)
                    {
                        e.remove<Site>();
                    }
                }
            });
    });

    register_names(m_entities);
    register_file_component<Name>(m_entities, name_component);
    register_file_component<DisplayName>(m_entities, display_name_component);

    // The state of an entity that a saved zone writes back, which a zone file may also state
    register_actor_components(m_entities);
    register_attachment_components(m_entities);
    register_ai_components(m_entities);

    // The default access point types, each arriving at its own type; a mod may register more
    for (std::string_view name : {"staircase", "door", "portal", "waypoint"})
    {
        flecs::entity access_type = create_access_type(m_entities, name);
        access_type.set<Arrival>(Arrival {access_type});
    }
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::clear()
{
    while (!m_worldspaces.empty())
    {
        unload_worldspace(m_worldspaces.back().worldspace_id);
    }

    m_entity_zones.clear();
    m_zone_entry_index.clear();
    m_game_directory.reset();
    m_worldspace_name_counter = 0;

    return;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::seed_type quetzal::stratum_sim::World::world_seed() const
{
    return m_world_seed;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::set_world_seed(seed_type seed)
{
    m_world_seed = seed;
    return;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create(std::string_view start_zone, const std::filesystem::path& game_directory)
{
    m_game_directory = game_directory;

    const WorldDefinition& definition = m_generation.m_world_definition;
    PatternResolver::random_type world_random(m_world_seed);
    const PersistentId worldspace_id = create_worldspace(definition.starting_region,
        value_at(definition.region_spec_trees, definition.starting_region, "region spec tree"), world_random.seed());

    Position position;
    if (start_zone == center_start)
    {
        const Worldspace::extent_type& extent = worldspace(worldspace_id).extent;
        position = {std::midpoint(extent.lower(0), extent.upper(0) - 1), std::midpoint(extent.lower(1), extent.upper(1) - 1), std::midpoint(extent.lower(2), extent.upper(2) - 1)};
    }
    else if (std::optional<geometry::DiscretePoint<coord_type, 3>> written = try_type<geometry::DiscretePoint<coord_type, 3>>(start_zone, comma_separated); written.has_value())
    {
        position = {written->x(), written->y(), written->z()};
    }
    else
    {
        const std::string message = std::format("the start zone '{}' is not {} or a zone position x,y,z", start_zone, center_start);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    return load_zone_at(worldspace_id, position);
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::open(const std::filesystem::path& game_directory)
{
    stratum_format::WorldDocument document = stratum_format::read_world(game_directory / stratum_format::world_filename);

    m_game_directory = game_directory;

    for (const stratum_format::WorldDocument::Worldspace& document_worldspace : document.worldspaces)
    {
        const stratum_format::WorldDocument::Region& document_root = document_worldspace.regions.front();
        Worldspace& worldspace = create_worldspace(document_worldspace.name, document_worldspace.seed, document_root.spec,
            PersistentId {document_worldspace.id}, PersistentId {document_root.id});

        for (const stratum_format::WorldDocument::Region& document_region : document_worldspace.regions)
        {
            Region subregion;
            subregion.region_id = PersistentId {document_region.id};
            Region& region = document_region.parent_id.has_value()
                ? worldspace.regions.add_subregion(PersistentId {document_region.parent_id.value()}, std::move(subregion))
                : worldspace.regions.root();
            region.name = document_region.name;
            region.display_name = document_region.display_name;
            region.name_counter = document_region.name_counter;
            region.region_spec = document_region.spec;

            for (const stratum_format::WorldDocument::Zone& document_zone : document_region.zones)
            {
                region.zone_ids.push_back(PersistentId {document_zone.id});

                if (document_zone.position.has_value())
                {
                    worldspace.zone_ids.emplace(Position {document_zone.position->x(), document_zone.position->y(), document_zone.position->z()},
                        PersistentId {document_zone.id});
                }

                if (document_zone.name.has_value())
                {
                    region.named_zone_ids.emplace(*document_zone.name, PersistentId {document_zone.id});
                }
            }
        }
    }

    m_worldspace_name_counter = document.name_counter;

    for (const auto& [entity_id, zone_id] : document.entity_zones)
    {
        m_entity_zones.emplace(PersistentId {entity_id}, PersistentId {zone_id});
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::save()
{
    assert(m_game_directory.has_value());

    stratum_format::WorldDocument document;
    document.version = std::string(stratum_format::saved_game_format_version);
    document.name_counter = m_worldspace_name_counter;

    // Worldspaces are created in id order
    for (const Worldspace& worldspace : m_worldspaces)
    {
        std::map<PersistentId, Position> zone_positions; // <zone_id, position>
        for (const auto& [position, zone_id] : worldspace.zone_ids)
        {
            zone_positions.emplace(zone_id, position);
        }

        stratum_format::WorldDocument::Worldspace document_worldspace {worldspace.worldspace_id.id, worldspace.name, worldspace.seed, {}};

        for (const Region& region_data : worldspace.regions.regions())
        {
            std::map<PersistentId, std::string> zone_names; // <zone_id, name>
            for (const auto& [name, zone_id] : region_data.named_zone_ids)
            {
                zone_names.emplace(zone_id, name);
            }

            stratum_format::WorldDocument::Region document_region;
            document_region.id = region_data.region_id.id;
            if (region_data.parent_id.has_value())
            {
                document_region.parent_id = region_data.parent_id.value().id;
            }

            document_region.name = region_data.name;
            document_region.display_name = region_data.display_name;
            document_region.name_counter = region_data.name_counter;
            document_region.spec = region_data.region_spec;

            for (PersistentId zone_id : region_data.zone_ids)
            {
                stratum_format::WorldDocument::Zone document_zone {zone_id.id, saved_zone_relative_path(zone_id).generic_string(), {}, {}};

                if (auto i = zone_positions.find(zone_id); i != zone_positions.end())
                {
                    document_zone.position = stratum_format::zone_position_type {i->second.x, i->second.y, i->second.z};
                }

                if (auto i = zone_names.find(zone_id); i != zone_names.end())
                {
                    document_zone.name = i->second;
                }

                document_region.zones.push_back(std::move(document_zone));
            }

            document_worldspace.regions.push_back(std::move(document_region));
        }

        document.worldspaces.push_back(std::move(document_worldspace));
    }

    for (const auto& [entity_id, zone_id] : m_entity_zones)
    {
        document.entity_zones.emplace(entity_id.id, zone_id.id);
    }

    for (flecs::entity zone : zones())
    {
        save_zone(zone);
    }

    stratum_format::write(*m_game_directory / stratum_format::world_filename, document);

    return;
}

//------------------------------------------------------------------------------
flecs::world& quetzal::stratum_sim::World::entities()
{
    return m_entities;
}

//------------------------------------------------------------------------------
const flecs::world& quetzal::stratum_sim::World::entities() const
{
    return m_entities;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::World::entities_type quetzal::stratum_sim::World::zones() const
{
    entities_type zones;

    m_entities.query_builder<const PersistentId>()
        .with<Zone>()
        .build()
        .each([&](flecs::entity e, const PersistentId& persistent_id)
        {
            if (!m_zones_unloading.contains(persistent_id))
            {
                zones.push_back(e);
            }
        });

    std::sort(zones.begin(), zones.end(), [](flecs::entity a, flecs::entity b) { return a.id() < b.id(); });

    return zones;
}

//------------------------------------------------------------------------------
std::size_t quetzal::stratum_sim::World::zone_count() const
{
    return zones().size();
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::World::worldspaces_type& quetzal::stratum_sim::World::worldspaces() const
{
    return m_worldspaces;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Worldspace& quetzal::stratum_sim::World::worldspace(PersistentId worldspace_id)
{
    auto i = std::ranges::find(m_worldspaces, worldspace_id, &Worldspace::worldspace_id);
    assert(i != m_worldspaces.end());
    return *i;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Worldspace& quetzal::stratum_sim::World::worldspace(PersistentId worldspace_id) const
{
    auto i = std::ranges::find(m_worldspaces, worldspace_id, &Worldspace::worldspace_id);
    assert(i != m_worldspaces.end());
    return *i;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Region& quetzal::stratum_sim::World::region(PersistentId region_id)
{
    return region_worldspace(region_id).regions.region(region_id);
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Region& quetzal::stratum_sim::World::region(PersistentId region_id) const
{
    return region_worldspace(region_id).regions.region(region_id);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::PersistentId quetzal::stratum_sim::World::create_worldspace(std::string_view name,
    const WorldDefinition::region_spec_tree_type& tree, seed_type seed)
{
    assert(!tree.empty());

    const RegionSpec root_spec = composed_region_spec(tree.front().region_spec, m_generation.m_region_specs);
    const std::string untaken = untaken_name(std::string(name), m_worldspace_name_counter, [this](const std::string& candidate)
    {
        return std::ranges::contains(m_worldspaces, candidate, &Worldspace::name);
    });

    const PersistentId worldspace_id = next_id();
    const PersistentId region_id = next_id();
    const Worldspace::extent_type extent = create_worldspace(untaken, seed, root_spec, worldspace_id, region_id).extent;

    try
    {
        PatternResolver::random_type random(seed);
        entities_type zones;
        for (coord_type z = extent.lower(2); z < extent.upper(2); ++z)
        {
            for (coord_type y = extent.lower(1); y < extent.upper(1); ++y)
            {
                for (coord_type x = extent.lower(0); x < extent.upper(0); ++x)
                {
                    zones.push_back(create_zone(region_id, Position {x, y, z}, zones.size(), random.seed()));
                }
            }
        }

        create_regions(worldspace_id, resolve_region_spec_tree(tree, m_generation.m_region_specs, random, m_world_seed, m_script_limits));

        // Each zone is listed again, under its own name, in the region it is in
        Region& root = worldspace(worldspace_id).regions.root();
        root.zone_ids.clear();
        root.named_zone_ids.clear();

        for (std::size_t zone_counter = 0; zone_counter < zones.size(); ++zone_counter)
        {
            set_zone_spec(zones[zone_counter], zone_counter);
        }
    }
    catch (...)
    {
        unload_worldspace(worldspace_id);
        throw;
    }

    return worldspace_id;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Worldspace& quetzal::stratum_sim::World::create_worldspace(std::string_view name, seed_type seed,
    const RegionSpec& region_spec, PersistentId worldspace_id, PersistentId region_id)
{
    Region root_region;
    root_region.region_id = region_id;
    root_region.name = std::string(name);
    root_region.region_spec = region_spec;

    m_worldspaces.push_back(Worldspace::create(worldspace_id, name, seed, region_spec, std::move(root_region)));
    return m_worldspaces.back();
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Worldspace& quetzal::stratum_sim::World::region_worldspace(PersistentId region_id)
{
    auto i = std::ranges::find_if(m_worldspaces, [region_id](const Worldspace& candidate) { return candidate.regions.contains(region_id); });
    assert(i != m_worldspaces.end());
    return *i;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Worldspace& quetzal::stratum_sim::World::region_worldspace(PersistentId region_id) const
{
    auto i = std::ranges::find_if(m_worldspaces, [region_id](const Worldspace& candidate) { return candidate.regions.contains(region_id); });
    assert(i != m_worldspaces.end());
    return *i;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Position quetzal::stratum_sim::World::new_zone_position(PersistentId, level_type level) const
{
    return {0, 0, level};
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::name_zone(flecs::entity zone)
{
    Region& region_data = region(region_of(zone));
    const PersistentId zone_id = zone.get<PersistentId>();
    std::string name;

    if (const auto* zone_name_component = zone.try_get<Name>())
    {
        name = zone_name_component->name;
    }
    else if (const auto* position = zone.try_get<Position>())
    {
        name = positional_zone_name(worldspace(worldspace_of(zone)), region_data.name, *position);
    }
    else
    {
        name = region_data.name;
    }

    std::string untaken = untaken_name(name, region_data.name_counter, [&](const std::string& candidate)
    {
        auto i = region_data.named_zone_ids.find(candidate);
        return i != region_data.named_zone_ids.end() && i->second != zone_id;
    });

    region_data.named_zone_ids.emplace(untaken, zone_id);
    zone.set<Name>(Name {std::move(untaken)});

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::unload_worldspace(PersistentId worldspace_id)
{
    entities_type worldspace_zones;
    m_entities.each([&](flecs::entity zone, const WorldspaceId& zone_worldspace)
    {
        if (zone_worldspace.worldspace_id == worldspace_id && !m_zones_unloading.contains(zone.get<PersistentId>()))
        {
            worldspace_zones.push_back(zone);
        }
    });

    for (flecs::entity zone : worldspace_zones)
    {
        unload_zone(zone);
    }

    [[maybe_unused]] std::size_t erased_count = std::erase_if(m_worldspaces, [worldspace_id](const Worldspace& worldspace)
    {
        return worldspace.worldspace_id == worldspace_id;
    });
    assert(erased_count == 1);

    return;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create_zone(PersistentId region_id, PersistentId zone_id)
{
    return m_entities.entity()
        .add<Zone>()
        .set<WorldspaceId>(WorldspaceId {region_worldspace(region_id).worldspace_id})
        .set<RegionId>(RegionId {region_id})
        .set<PersistentId>(zone_id);
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create_zone(PersistentId region_id, std::unique_ptr<LayoutBase> layout)
{
    assert(layout != nullptr);

    flecs::entity zone = create_zone(region_id, next_id());
    set_layout(zone, std::move(layout));
    region(region_id).zone_ids.push_back(zone.get<PersistentId>());
    name_zone(zone);

    return zone;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::load_zone(PersistentId region_id, const std::filesystem::path& filepath)
{
    const stratum_format::ZoneDocument document = stratum_format::read_zone(filepath);
    return build_zone(region_id, document.zone_id.has_value() ? PersistentId {*document.zone_id} : next_id(), document, filepath.string());
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::load_template(PersistentId worldspace_id, const Position& position,
    const std::filesystem::path& filepath, [[maybe_unused]] const ZoneSpec& zone_spec)
{
    const stratum_format::ZoneDocument document = stratum_format::read_zone(filepath);

    if (document.zone_id.has_value())
    {
        const std::string message = std::format("{}: a saved zone is not a template", filepath.string());
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    flecs::entity zone = zone_at(worldspace_id, position);

    if (!empty_zone(zone))
    {
        const std::string message = std::format("{}: zone {} at ({}, {}, {}) of the worldspace {} is not empty", filepath.string(),
            zone.get<PersistentId>().id, position.x, position.y, position.z, worldspace(worldspace_id).name);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    fill_zone(zone, document, filepath.string());
    return zone;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::load_zone_at(PersistentId worldspace_id, const Position& position)
{
    flecs::entity zone = zone_at(worldspace_id, position);

    if (empty_zone(zone))
    {
        fill_zone(zone, generate_zone(ZoneSpec {zone.get<Document>().document->zone_spec.value()}), "the game's zone generator");
    }

    return zone;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::load_saved_zone(PersistentId zone_id)
{
    if (!m_game_directory.has_value())
    {
        const std::string message = std::format("zone {} is not loaded and cannot be read back without a game directory", zone_id.id);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    std::optional<PersistentId> region_id;
    for (const Worldspace& worldspace : m_worldspaces)
    {
        for (const Region& candidate : worldspace.regions.regions())
        {
            if (std::ranges::contains(candidate.zone_ids, zone_id))
            {
                region_id = candidate.region_id;
            }
        }
    }

    if (!region_id.has_value())
    {
        const std::string message = std::format("zone {} is in no region", zone_id.id);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    return load_zone(region_id.value(), saved_zone_path(zone_id));
}

//------------------------------------------------------------------------------
std::filesystem::path quetzal::stratum_sim::World::saved_zone_path(PersistentId zone_id) const
{
    assert(m_game_directory.has_value());
    return *m_game_directory / saved_zone_relative_path(zone_id);
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::require_entry_point(flecs::entity access_point, flecs::entity zone, flecs::entity arrival_type,
    level_type offset)
{
    flecs::entity entry = entry_point(zone, arrival_type, offset);

    if (!entry)
    {
        const std::string message = std::format("zone {}, made for access point {}, has no access point of the type '{}' whose target is {:+}",
            zone.get<PersistentId>().id, access_point.get<PersistentId>().id, arrival_type.name().c_str(), offset);
        discard_made_zone(zone);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    return entry;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::discard_made_zone(flecs::entity zone)
{
    assert(zone && zone.has<Zone>());

    PersistentId zone_id = zone.get<PersistentId>();
    unload_worldspace(worldspace_of(zone));
    std::erase_if(m_entity_zones, [zone_id](const auto& entity_zone) { return entity_zone.second == zone_id; });

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::unload_zone(flecs::entity zone)
{
    assert(zone && zone.has<Zone>());

    // The unload finishes in the PersistentId observer when the deletion runs, which is later if flecs is deferring
    m_zones_unloading.insert(zone.get<PersistentId>());
    zone.destruct(); // Deletes the zone's children before the zone

    return;
}

//------------------------------------------------------------------------------
// One denizen at the last open cell (temporary ...)
void quetzal::stratum_sim::World::populate(flecs::entity zone)
{
    const DiscreteLayout& zone_layout = layout_as<DiscreteLayout>(layout(zone));

    std::optional<discrete_point_type> start = last_open_cell(zone_layout, [&](const discrete_point_type& cell_position)
    {
        return !find_access_point(zone, FlatCellPosition {zone_layout.floor_id(), cell_position});
    });

    if (!start)
    {
        return;
    }

    create_entity(zone, placement_type(zone_layout.position(*start), vector_type(value_type(-1), value_type(0), value_type(0))))
        .add<Blocking>()
        .set<Speed>(Speed {denizen_speed});

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::relocate(flecs::entity e, flecs::entity zone, const placement_type& placement)
{
    assert(zone && zone.has<Zone>());

    Site site {Relation::In, {}, placement};

    if (e.parent())
    {
        reparent(e, zone, site);
        return;
    }

    [[maybe_unused]] flecs::entity zone_from = stratum_sim::zone_of(e);
    assert(!zone_from || zone_from == zone || !has_descendant_sited_on_layout(e));

    e.remove<Site>(); // First, so it leaves what it was sited on while still in that zone
    e.add<InZone>(zone);
    e.set<Site>(site);

    return;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::resolve_target(flecs::entity access_point)
{
    assert(access_point.has<AccessPoint>() && access_point.get<AccessPoint>().target.has_value()
        && std::holds_alternative<stratum_format::AccessPointTarget>(*access_point.get<AccessPoint>().target));

    const auto& target = std::get<stratum_format::AccessPointTarget>(*access_point.get<AccessPoint>().target);

    if (std::holds_alternative<std::string>(target.zone) || target.position.has_value())
    {
        const std::string message = std::format("access point {} has a target with a named zone or a position, which is not resolved",
            access_point.get<PersistentId>().id);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    flecs::entity zone_from = stratum_sim::zone_of(access_point);
    assert(zone_from.has<Position>());

    level_type level_from = zone_from.get<Position>().z;
    const auto* relative = std::get_if<stratum_format::RelativeZone>(&target.zone);
    level_type level = relative != nullptr ? level_from + relative->offset : std::get<stratum_format::AbsoluteZone>(target.zone).level;
    level_type offset = level - level_from;

    flecs::entity arrival_type = access_point.target<AccessType>().get<Arrival>().access_type;
    const std::optional<std::string>& region_spec_name = access_point.get<AccessPoint>().region_spec;
    PersistentId worldspace_id = worldspace_of(zone_from);

    if (target.region.has_value() && !region_spec_name.has_value())
    {
        const PersistentId made = create_worldspace(*target.region,
            {RegionSpecNode {.region_spec = value_at(m_generation.m_region_specs, *target.region, "region spec")}}, target_seed(access_point));
        return require_entry_point(access_point, load_zone_at(made, new_zone_position(made, level)), arrival_type, -offset);
    }

    if (target.region.has_value())
    {
        worldspace_id = target_worldspace(*target.region, region_spec_name.value(), target_seed(access_point));
    }

    flecs::entity zone = load_zone_at(worldspace_id, new_zone_position(worldspace_id, level));
    flecs::entity entry = entry_point(zone, arrival_type, -offset);

    if (!entry)
    {
        const std::string message = std::format("zone {}, at the target of access point {}, has no access point of the type '{}' whose target is {:+}",
            zone.get<PersistentId>().id, access_point.get<PersistentId>().id, arrival_type.name().c_str(), -offset);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    return entry;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::relocate_through(flecs::entity actor, flecs::entity access_point)
{
    assert(access_point.has<AccessPoint>() && access_point.get<AccessPoint>().target.has_value());

    flecs::entity arrival;

    if (auto* reference = std::get_if<EntityReference>(&*access_point.get_mut<AccessPoint>().target))
    {
        arrival = resolve(*reference);

        // The access point it is bound to is in a zone that has unloaded, which is read back. Loading can move the access
        // point's component, so the reference is found again
        if (!arrival)
        {
            if (!m_game_directory.has_value())
            {
                const std::string message = std::format("access point {} is bound to access point {}, whose zone is not loaded and cannot be "
                    "read back without a game directory", access_point.get<PersistentId>().id, reference->persistent_id.id);
                log::logger().error("stratum_sim", "{}", message);
                throw std::runtime_error(message);
            }

            load_saved_zone(zone_of(reference->persistent_id));
            arrival = resolve(std::get<EntityReference>(*access_point.get_mut<AccessPoint>().target));
            assert(arrival);
        }
    }
    else
    {
        arrival = resolve_target(access_point);
        access_point.get_mut<AccessPoint>().target = make_reference(arrival);

        if (arrival.get<AccessPoint>().target.has_value())
        {
            arrival.get_mut<AccessPoint>().target = make_reference(access_point);
        }
    }

    assert(arrival.has<AccessPoint>());
    relocate(actor, stratum_sim::zone_of(arrival), arrival_placement(arrival));

    return arrival;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::trim_zones(flecs::entity zone_entered)
{
    assert(zone_entered && zone_entered.has<Zone>());

    if (zone_count_max() == 0)
    {
        return;
    }

    entities_type zones_with_content = zones();
    std::erase_if(zones_with_content, empty_zone);

    for (flecs::entity zone : m_zone_entry_index.select_unloads(zones_with_content, zone_entered))
    {
        assert(zone != zone_entered);

        if (m_game_directory.has_value())
        {
            save_zone(zone);
        }

        unload_zone(zone);
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::save_zone(flecs::entity zone)
{
    assert(m_game_directory.has_value());
    assert(zone && zone.has<Zone>() && zone.has<Document>());

    std::filesystem::path filepath = saved_zone_path(zone.get<PersistentId>());
    std::filesystem::create_directories(filepath.parent_path());
    stratum_format::write(filepath, make_document(zone));

    return;
}

//------------------------------------------------------------------------------
std::size_t quetzal::stratum_sim::World::zone_count_max() const
{
    return m_zone_entry_index.zone_count_max();
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::set_zone_count_max(size_t zone_count_max)
{
    m_zone_entry_index.set_zone_count_max(zone_count_max);
    return;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::MoveResult quetzal::stratum_sim::World::move(flecs::entity e, const vector_type& direction, value_type distance) const
{
    flecs::entity zone = stratum_sim::zone_of(e);
    assert(zone);

    const LayoutBase& zone_layout = layout(zone);

    auto blocker_at = [&](const placement_type& candidate_placement)
    {
        for (flecs::entity other : entities_at(zone, candidate_placement))
        {
            if (other != e && other.has<Blocking>())
            {
                return static_cast<id_type>(other.id());
            }
        }

        return nullid;
    };

    return zone_layout.move(placement_of(e), direction, distance, is_crossable(zone, e), blocker_at);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::LayoutBase::placements_type quetzal::stratum_sim::World::neighbors(flecs::entity e) const
{
    flecs::entity zone = stratum_sim::zone_of(e);
    assert(zone);

    return layout(zone).neighbors(placement_of(e), is_crossable(zone, e));
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::World::visible(flecs::entity e, const placement_type& to) const
{
    flecs::entity zone = stratum_sim::zone_of(e);
    assert(zone);

    return layout(zone).visible(placement_of(e), to, is_crossable(zone, e));
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::LayoutBase& quetzal::stratum_sim::World::layout(flecs::entity zone)
{
    assert(zone && zone.has<Zone>());
    return *zone.get<Layout>().layout;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::LayoutBase& quetzal::stratum_sim::World::layout(flecs::entity zone) const
{
    assert(zone && zone.has<Zone>());
    return *zone.get<Layout>().layout;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::ProximityIndexBase& quetzal::stratum_sim::World::proximity_index(flecs::entity zone) const
{
    assert(zone && zone.has<Zone>());
    return *zone.get<ProximityIndex>().proximity_index;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::World::entities_type quetzal::stratum_sim::World::entities_at(flecs::entity zone, const placement_type& placement) const
{
    return entities_of(zone, proximity_index(zone).entities_at(placement));
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::World::entities_type quetzal::stratum_sim::World::entities_at(flecs::entity zone, const discrete_point_type& cell_position) const
{
    return entities_of(zone, dynamic_cast<const DiscreteProximityIndex&>(proximity_index(zone)).entities_at(cell_position));
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::World::entities_type quetzal::stratum_sim::World::entities_within(flecs::entity zone, const placement_type& placement, value_type range) const
{
    return entities_of(zone, proximity_index(zone).entities_within(placement, range));
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create_entity(flecs::entity zone, const placement_type& placement)
{
    assert(zone && zone.has<Zone>());
    return m_entities.entity()
        .child_of(zone)
        .set<Site>(Site {Relation::In, {}, placement});
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::build_zone(PersistentId region_id, PersistentId zone_id, const stratum_format::ZoneDocument& document,
    std::string_view source)
{
    assert(!document.zone_id.has_value() || document.zone_id.value() == zone_id.id);

    if (std::optional<std::string> error = check_buildable(m_entities, document))
    {
        const std::string message = std::format("{}: {}", source, *error);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    if (document.zone_id.has_value() && entity(PersistentId {*document.zone_id}))
    {
        const std::string message = std::format("{}: zone {} is already loaded", source, *document.zone_id);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    if (document.position.has_value())
    {
        const Position position {document.position->x(), document.position->y(), document.position->z()};
        const Worldspace& zone_worldspace = region_worldspace(region_id);

        if (!zone_worldspace.contains(position))
        {
            const std::string message = std::format("{}: the zone's position ({}, {}, {}) is outside the extent of the worldspace {}", source,
                position.x, position.y, position.z, zone_worldspace.name);
            log::logger().error("stratum_sim", "{}", message);
            throw std::runtime_error(message);
        }

        auto i = zone_worldspace.zone_ids.find(position);
        if (i != zone_worldspace.zone_ids.end() && i->second != zone_id)
        {
            const std::string message = std::format("{}: zone {} of the worldspace {} is at the zone's position ({}, {}, {})", source,
                i->second.id, zone_worldspace.name, position.x, position.y, position.z);
            log::logger().error("stratum_sim", "{}", message);
            throw std::runtime_error(message);
        }
    }

    flecs::entity zone = create_zone(region_id, zone_id);

    // A saved zone was listed when it was created
    if (!document.zone_id.has_value())
    {
        region(region_id).zone_ids.push_back(zone.get<PersistentId>());
    }

    for (flecs::entity target : load_layout(zone, document))
    {
        make_reference(target);
    }

    name_zone(zone);

    if (document.is_unpopulated)
    {
        populate(zone);
    }

    return zone;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::ZoneSpec quetzal::stratum_sim::World::resolved_zone_spec(PersistentId region_id, const ZoneSpec& empty_zone_spec,
    std::size_t zone_counter) const
{
    const Region& zone_region = region(region_id);

    ZoneSpec zone_spec {select_prefixed(zone_region.region_spec.store, zone_prefix)};
    if (!zone_spec.store.contains(name_key))
    {
        zone_spec.store.insert(std::string(name_key), std::string(default_zone_name));
    }

    const Position position = empty_zone_spec.position();
    zone_spec.set_position(position);
    if (!zone_spec.store.contains(seed_key))
    {
        zone_spec.store.insert(std::string(seed_key), empty_zone_spec.store.value(seed_key).value());
    }

    const seed_type seed = zone_spec.store.value<seed_type>(seed_key).value();
    PatternResolver resolver(zone_symbols(zone_region, position, zone_counter), PatternResolver::random_type(seed), m_world_seed, m_script_limits);
    return resolve_zone_spec(zone_spec, resolver);
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create_zone(PersistentId region_id, const Position& position, std::size_t zone_counter,
    seed_type seed)
{
    const Region& zone_region = region(region_id);

    ZoneSpec zone_spec;
    zone_spec.set_position(position);
    zone_spec.store.insert(std::string(seed_key), std::to_string(seed));

    stratum_format::ZoneDocument document;
    document.position = stratum_format::zone_position_type {position.x, position.y, position.z};
    document.zone_spec = zone_spec.store;
    document.components[std::string(name_component)] = std::format("{}_{}", zone_region.name, zone_counter);

    return build_zone(region_id, next_id(), document, std::format("the empty zone at ({}, {}, {})", position.x, position.y, position.z));
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::create_regions(PersistentId worldspace_id, const WorldDefinition::region_spec_tree_type& resolved_tree)
{
    Worldspace& created = worldspace(worldspace_id);
    RegionTree& regions = created.regions;
    Region& root = regions.root();
    root.display_name = resolved_tree.front().region_spec.display_name;
    root.region_spec = resolved_tree.front().region_spec;

    auto fail = [](const std::string& message)
    {
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    };

    // The bounding box of each spec's extent, by its slot in the tree, for the absent axes of its subregions' boxes
    std::vector<Worldspace::extent_type> bounding_boxes {created.extent};
    std::vector<PersistentId> region_ids {root.region_id};
    for (std::size_t i = 1; i < resolved_tree.size(); ++i)
    {
        const RegionSpec& region_spec = resolved_tree[i].region_spec;
        if (!region_spec.extent.has_value())
        {
            fail(std::format("the subregion spec {} has no extent", region_spec.name.value()));
        }

        const Worldspace::extent_type parent_box = bounding_boxes[resolved_tree[i].parent_id];
        const Worldspace::extent_type box = bounding_box(region_spec.extent.value(), parent_box);
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
            if (box.lower(axis) < box.upper(axis) && (box.lower(axis) < created.extent.lower(axis) || box.upper(axis) > created.extent.upper(axis)))
            {
                fail(std::format("the extent of the subregion spec {} reaches outside the worldspace {}", region_spec.name.value(), created.name));
            }
        }

        Region subregion;
        subregion.region_id = next_id();
        subregion.name = region_spec.name.value();
        subregion.display_name = region_spec.display_name;
        subregion.region_spec = region_spec;

        const PersistentId subregion_id = subregion.region_id;
        const PersistentId parent_id = region_ids[resolved_tree[i].parent_id];
        region_ids.push_back(subregion_id);
        regions.add_subregion(parent_id, std::move(subregion));

        for_each_position(region_spec.extent.value(), parent_box, [&](const Position& position)
        {
            flecs::entity zone = entity(created.zone_ids.at(position));
            const PersistentId zone_region_id = zone.get<RegionId>().region_id;
            if (zone_region_id != parent_id)
            {
                // The zone is in a sibling or one of its subregions if an ancestor of its region is a child of the parent
                PersistentId ancestor_id = zone_region_id;
                while (regions.region(ancestor_id).parent_id.has_value() && regions.region(ancestor_id).parent_id != parent_id)
                {
                    ancestor_id = regions.region(ancestor_id).parent_id.value();
                }

                const std::string problem = regions.region(ancestor_id).parent_id == parent_id
                    ? std::format("overlaps the extent of its sibling {}", regions.region(ancestor_id).name)
                    : std::string("is outside its parent's extent");
                fail(std::format("the extent of the region {} {} at ({}, {}, {})", regions.extended_name(subregion_id), problem, position.x, position.y,
                    position.z));
            }

            zone.set<RegionId>(RegionId {subregion_id});
        });

        bounding_boxes.push_back(box);
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::set_zone_spec(flecs::entity zone, std::size_t zone_counter)
{
    const PersistentId region_id = region_of(zone);
    const Document& zone_document = zone.get<Document>();
    const ZoneSpec zone_spec = resolved_zone_spec(region_id, ZoneSpec {zone_document.document->zone_spec.value()}, zone_counter);

    auto document = std::make_shared<stratum_format::ZoneDocument>(*zone_document.document);
    document->zone_spec = zone_spec.store;
    zone.set<Document>(Document {std::move(document)});
    region(region_id).zone_ids.push_back(zone.get<PersistentId>());

    if (const std::optional<std::string> name = zone_spec.store.value(name_key); name.has_value())
    {
        zone.set<Name>(Name {name.value()});
    }
    else
    {
        zone.remove<Name>();
    }

    name_zone(zone);
    return;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::zone_at(PersistentId worldspace_id, const Position& position)
{
    const Worldspace& zone_worldspace = worldspace(worldspace_id);
    auto i = zone_worldspace.zone_ids.find(position);

    if (i == zone_worldspace.zone_ids.end())
    {
        const std::string message = std::format("the worldspace {} has no zone at ({}, {}, {})", zone_worldspace.name, position.x, position.y,
            position.z);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    const PersistentId zone_id = i->second;
    flecs::entity zone = entity(zone_id);
    return zone ? zone : load_saved_zone(zone_id);
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::fill_zone(flecs::entity zone, const stratum_format::ZoneDocument& document, std::string_view source)
{
    assert(empty_zone(zone) && zone.has<Position>());

    stratum_format::ZoneDocument zone_document = document;
    const Position& position = zone.get<Position>();
    zone_document.position = stratum_format::zone_position_type {position.x, position.y, position.z};
    zone_document.components.erase(std::string(name_component));

    if (std::optional<std::string> error = check_buildable(m_entities, zone_document))
    {
        const std::string message = std::format("{}: {}", source, *error);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    for (flecs::entity target : load_layout(zone, zone_document))
    {
        make_reference(target);
    }

    create_target_worldspaces(zone);

    if (zone_document.is_unpopulated)
    {
        populate(zone);
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::create_target_worldspaces(flecs::entity zone)
{
    // <worldspace name, region spec name, seed>
    std::vector<std::tuple<std::string, std::string, seed_type>> target_worldspaces;

    m_entities.query_builder<const AccessPoint>()
        .with(flecs::ChildOf, zone)
        .build()
        .each([&](flecs::entity e, const AccessPoint& access_point)
        {
            if (access_point.region_spec.has_value())
            {
                target_worldspaces.emplace_back(target_worldspace_name(e), access_point.region_spec.value(), target_seed(e));
            }
        });

    for (const auto& [name, region_spec_name, seed] : target_worldspaces)
    {
        target_worldspace(name, region_spec_name, seed);
    }

    return;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_sim::World::target_worldspace_name(flecs::entity access_point) const
{
    const AccessPoint& point = access_point.get<AccessPoint>();
    const auto* unbound = point.target.has_value() ? std::get_if<stratum_format::AccessPointTarget>(&*point.target) : nullptr;

    if (unbound == nullptr || !unbound->region.has_value())
    {
        const std::string message = std::format("access point {} has a region spec, and its target is not unbound with a region",
            access_point.get<PersistentId>().id);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    return unbound->region.value();
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::PersistentId quetzal::stratum_sim::World::target_worldspace(std::string_view name, std::string_view region_spec_name,
    seed_type seed)
{
    auto named = std::ranges::find(m_worldspaces, name, &Worldspace::name);
    if (named != m_worldspaces.end())
    {
        return named->worldspace_id;
    }

    return create_worldspace(name, {RegionSpecNode {.region_spec = value_at(m_generation.m_region_specs, std::string(region_spec_name), "region spec")}},
        seed);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::seed_type quetzal::stratum_sim::World::target_seed(flecs::entity access_point) const
{
    const std::array<std::uint64_t, 1> values {access_point.get<PersistentId>().id};
    return derived_seed(m_world_seed, values);
}

//------------------------------------------------------------------------------
quetzal::stratum_format::ZoneDocument quetzal::stratum_sim::World::generate_zone(const ZoneSpec& zone_spec)
{
    assert(m_generation.m_zone_generator);
    stratum_format::ZoneDocument document = m_generation.m_zone_generator(zone_spec);
    assert(!document.zone_id.has_value());
    document.is_unpopulated = true;

    return document;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::World::deleting_with(flecs::entity zone) const
{
    // World fini deletes the zones after setting should_quit and before setting its own fini flag
    return m_entities.should_quit() || m_zones_unloading.contains(zone.get<PersistentId>());
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::World::entities_type quetzal::stratum_sim::World::property_chain(flecs::entity e) const
{
    entities_type chain {e};

    if (e.has<Zone>())
    {
        return chain;
    }

    flecs::entity zone = stratum_sim::zone_of(e);
    assert(zone);

    const auto* zone_layout = dynamic_cast<const DiscreteLayout*>(&layout(zone));

    bool is_layout = false;
    for_each_layout_entity([&]<typename T>(std::type_identity<T>)
    {
        if (const auto* key = e.try_get<T>())
        {
            append_layout_levels(chain, zone, e, zone_layout, *key);
            is_layout = true;
        }
    });

    if (!is_layout)
    {
        for (flecs::entity type = e.target(flecs::IsA); type; type = type.target(flecs::IsA))
        {
            chain.push_back(type);
        }

        if (zone_layout != nullptr)
        {
            if (id_type room_id = room_id_of(e, *zone_layout); room_id != nullid)
            {
                append_if(chain, room_entity(zone, room_id));
            }
        }
    }

    chain.push_back(zone);

    return chain;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::entity(PersistentId persistent_id) const
{
    auto i = m_loaded_entities.find(persistent_id);
    return i != m_loaded_entities.end() ? i->second : flecs::entity();
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::PersistentId quetzal::stratum_sim::World::zone_of(PersistentId persistent_id) const
{
    auto i = m_entity_zones.find(persistent_id);
    assert(i != m_entity_zones.end());
    return i->second;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::EntityReference quetzal::stratum_sim::World::make_reference(flecs::entity target)
{
    flecs::entity zone = stratum_sim::zone_of(target);
    assert(zone);

    PersistentId persistent_id = target.get<PersistentId>();
    m_entity_zones.try_emplace(persistent_id, zone.get<PersistentId>());

    return EntityReference {persistent_id, target};
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::resolve(EntityReference& reference) const
{
    if (!reference.entity || !reference.entity.is_alive())
    {
        reference.entity = entity(reference.persistent_id);
    }

    return reference.entity;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::PersistentId quetzal::stratum_sim::World::next_id()
{
    return m_id_allocator.next();
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::zone_of(flecs::entity e)
{
    for (flecs::entity holder = e; holder; holder = holder.parent())
    {
        if (flecs::entity zone = holder.target<InZone>())
        {
            return zone;
        }

        if (flecs::entity parent = holder.parent(); parent && parent.has<Zone>())
        {
            return parent;
        }
    }

    return flecs::entity();
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::PersistentId quetzal::stratum_sim::worldspace_of(flecs::entity zone)
{
    assert(zone && zone.has<Zone>());
    return zone.get<WorldspaceId>().worldspace_id;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::PersistentId quetzal::stratum_sim::region_of(flecs::entity zone)
{
    assert(zone && zone.has<Zone>());
    return zone.get<RegionId>().region_id;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::empty_zone(flecs::entity zone)
{
    assert(zone && zone.has<Zone>());
    return !zone.has<Layout>();
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::placement_type quetzal::stratum_sim::start_placement(flecs::entity zone, std::string_view start_cell)
{
    assert(zone && !empty_zone(zone));

    const DiscreteLayout& zone_layout = layout_as<DiscreteLayout>(*zone.get<Layout>().layout);
    const discrete_vector_type grid_extent = zone_layout.grid().extent;
    std::optional<discrete_point_type> cell_position;

    if (start_cell == center_start)
    {
        // Twice each coordinate, so the center of an even extent is between cells and the distances stay integers
        const discrete_point_type doubled_center {grid_extent.x() - 1, grid_extent.y() - 1};
        std::int64_t nearest = 0;

        for (coord_type j = 0; j < grid_extent.y(); ++j)
        {
            for (coord_type i = 0; i < grid_extent.x(); ++i)
            {
                const std::int64_t dx = 2 * i - doubled_center.x();
                const std::int64_t dy = 2 * j - doubled_center.y();
                if (zone_layout.open({i, j}) && (!cell_position.has_value() || dx * dx + dy * dy < nearest))
                {
                    cell_position = discrete_point_type {i, j};
                    nearest = dx * dx + dy * dy;
                }
            }
        }
    }
    else if (std::optional<discrete_point_type> written = try_type<discrete_point_type>(start_cell, comma_separated); written.has_value())
    {
        const discrete_point_type& position = *written;
        if (position.x() >= 0 && position.y() >= 0 && position.x() < grid_extent.x() && position.y() < grid_extent.y() && zone_layout.open(position))
        {
            cell_position = position;
        }
    }

    if (!cell_position.has_value())
    {
        const std::string message = std::format("the start cell '{}' is not {}, or a cell position i,j an actor can occupy in zone {}", start_cell,
            center_start, zone.get<PersistentId>().id);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    // cell_position was just checked
    return placement_type(zone_layout.position(*cell_position), vector_type(value_type(0), value_type(-1), value_type(0)));
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::set_layout(flecs::entity zone, std::unique_ptr<LayoutBase> layout)
{
    assert(zone && zone.has<Zone>() && layout != nullptr);

    std::unique_ptr<ProximityIndexBase> proximity_index = layout->create_proximity_index();
    zone.set<Layout>(Layout {std::move(layout)});
    zone.set<ProximityIndex>(ProximityIndex {std::move(proximity_index)});

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::reparent(flecs::entity e, flecs::entity parent, const Site& site)
{
    assert(e && parent && e != parent);

    [[maybe_unused]] flecs::entity zone_from = zone_of(e);
    [[maybe_unused]] flecs::entity zone_to = parent.has<Zone>() ? parent : zone_of(parent);
    assert(!zone_from || zone_from == zone_to || !has_descendant_sited_on_layout(e));

    e.remove<Site>(); // First, so it leaves what it was sited on while still in that zone
    set_parent(e, parent);
    e.set<Site>(site);

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::set_parent(flecs::entity e, flecs::entity parent)
{
    assert(e && parent && e != parent);

    if (parent.has<Zone>())
    {
        e.child_of(parent); // Removes a Parent
    }
    else
    {
        e.remove(flecs::ChildOf, flecs::Wildcard);
        e.set<flecs::Parent>(flecs::Parent {parent});
    }

    return;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::world_handle(flecs::entity e)
{
    return flecs::entity(e.world().get_world().c_ptr(), e.id());
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::create_entity_type(flecs::world& entities, std::string_view name)
{
    assert(!entity_type(entities, name));
    return entities.prefab(type_path(name).c_str());
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::entity_type(const flecs::world& entities, std::string_view name)
{
    return entities.lookup(type_path(name).c_str());
}
