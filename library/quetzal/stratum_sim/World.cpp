//------------------------------------------------------------------------------
// stratum_sim
// World.cpp
//------------------------------------------------------------------------------

#include "World.hpp"
#include "actors.hpp"
#include "ai.hpp"
#include "attachments.hpp"
#include "DiscreteLayout.hpp"
#include "DiscretePlacementIndex.hpp"
#include "Name.hpp"
#include "file_components.hpp"
#include "layout_entities.hpp"
#include "load_layout.hpp"
#include "make_document.hpp"
#include "properties.hpp"
#include "seeds.hpp"
#include "Site.hpp"
#include "specs.hpp"
#include "quetzal/log/Log.hpp"
#include "quetzal/stratum_format/Document.hpp"
#include "quetzal/stratum_format/read_world.hpp"
#include "quetzal/stratum_format/read_zone.hpp"
#include "quetzal/stratum_format/saved_game_format.hpp"
#include "quetzal/stratum_format/WorldDocument.hpp"
#include "quetzal/stratum_format/write_world.hpp"
#include "quetzal/stratum_format/write_zone.hpp"
#include <algorithm>
#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <cassert>

namespace
{

    static constexpr int denizen_speed = 100;

    //--------------------------------------------------------------------------
    // Returns the registered value, or throws std::runtime_error, describing it as the registration type, if none is
    // registered under the name
    template<typename T>
    const T& registered(const std::map<std::string, T>& registry, std::string_view registration_type, const std::string& name)
    {
        auto i = registry.find(name);

        if (i == registry.end())
        {
            throw std::runtime_error(std::format("no {} is registered as {}", registration_type, name));
        }

        return i->second;
    }

    //--------------------------------------------------------------------------
    // The name pattern of a zone of the region when its specs state none: the region's name and the zone's coordinates,
    // x and y in a grid worldspace and all three otherwise
    std::string default_zone_name_pattern(const quetzal::stratum_sim::Worldspace& worldspace, const quetzal::stratum_sim::Region& region)
    {
        return region.name + (worldspace.zone_size.has_value() ? "_{x}_{y}" : "_{x}_{y}_{z}");
    }

    //--------------------------------------------------------------------------
    // Temporary placeholder placement function, real placement will not work like this ...
    std::optional<quetzal::stratum_sim::discrete_point_type> last_open_cell(const quetzal::stratum_sim::DiscreteLayout& layout)
    {
        const quetzal::stratum_sim::GridMapping& grid = layout.grid();

        for (quetzal::stratum_sim::coord_type j = grid.extent.y(); j-- > 0; )
        {
            for (quetzal::stratum_sim::coord_type i = grid.extent.x(); i-- > 0; )
            {
                if (layout.open(quetzal::stratum_sim::discrete_point_type {i, j}))
                {
                    return quetzal::stratum_sim::discrete_point_type {i, j};
                }
            }
        }

        return {};
    }

    //--------------------------------------------------------------------------
    // Returns the zone that the entity is a child of, or is in through InZone, or a null entity if it is in no zone or is in
    // one through another entity
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
    quetzal::stratum_sim::World::entities_type entities_of(flecs::entity zone, const quetzal::stratum_sim::PlacementIndex::entity_ids_type& entity_ids)
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
    void append_layout_levels(quetzal::stratum_sim::World::entities_type&, flecs::entity, const quetzal::stratum_sim::DiscreteLayout*,
        const quetzal::stratum_sim::RoomAddress&)
    {
        return;
    }

    void append_layout_levels(quetzal::stratum_sim::World::entities_type& chain, flecs::entity zone,
        const quetzal::stratum_sim::DiscreteLayout* zone_layout, const quetzal::stratum_sim::WallAddress& address)
    {
        assert(zone_layout != nullptr);

        if (quetzal::id_type room_id = zone_layout->room_at(zone_layout->wall_cell_position(address.wall_id)); room_id != quetzal::nullid)
        {
            append_if(chain, quetzal::stratum_sim::room_entity(zone, room_id));
        }

        return;
    }

    void append_layout_levels(quetzal::stratum_sim::World::entities_type&, flecs::entity, const quetzal::stratum_sim::DiscreteLayout*,
        const quetzal::stratum_sim::FlatAddress&)
    {
        return;
    }

    void append_layout_levels(quetzal::stratum_sim::World::entities_type& chain, flecs::entity zone, const quetzal::stratum_sim::DiscreteLayout*,
        const quetzal::stratum_sim::FlatRoomAddress& address)
    {
        append_if(chain, quetzal::stratum_sim::room_entity(zone, address.room_id));
        append_if(chain, quetzal::stratum_sim::flat_entity(zone, address.flat_id));
        return;
    }

    void append_layout_levels(quetzal::stratum_sim::World::entities_type& chain, flecs::entity zone,
        const quetzal::stratum_sim::DiscreteLayout* zone_layout, const quetzal::stratum_sim::FlatCellAddress& address)
    {
        assert(zone_layout != nullptr);

        if (quetzal::id_type room_id = zone_layout->room_at(address.cell_position); room_id != quetzal::nullid)
        {
            append_if(chain, quetzal::stratum_sim::flat_room_entity(zone, address.flat_id, room_id));
            append_if(chain, quetzal::stratum_sim::room_entity(zone, room_id));
        }

        append_if(chain, quetzal::stratum_sim::flat_entity(zone, address.flat_id));
        return;
    }

    //--------------------------------------------------------------------------
    // Returns true if a layout entity has any component other than its tag, its address, its parent, and the list of entities sited on it
    bool has_own_components(flecs::entity layout_entity)
    {
        flecs::world world = layout_entity.world();
        bool found = false;

        layout_entity.each([&](flecs::id id)
        {
            bool parent = id == world.id<flecs::Parent>() || (id.is_pair() && (id.first() == flecs::ChildOf || id.first() == flecs::ParentDepth));

            bool address = false;
            quetzal::stratum_sim::for_each_layout_address([&]<typename Address>(std::type_identity<Address>)
            {
                address = address || id == world.id<Address>();
            });

            if (!parent && !address && id != world.id<quetzal::stratum_sim::LayoutEntity>()
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
                if (const auto* wall = std::get_if<quetzal::stratum_sim::WallAddress>(&*site->address))
                {
                    return layout.room_at(layout.wall_cell_position(wall->wall_id));
                }

                return layout.room_at(std::get<quetzal::stratum_sim::FlatCellAddress>(*site->address).cell_position);
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
quetzal::stratum_sim::World::World(IdAllocator& id_allocator, Generation& generation, size_t zone_count_max) :
    m_id_allocator(id_allocator),
    m_generation(generation),
    m_loaded_entities(),
    m_entity_zones(),
    m_zones_unloading(),
    m_zone_entry_index(zone_count_max),
    m_game_directory(),
    m_world_seed(0),
    m_worldspace_name_counter(0),
    m_entities()
{
    // Registered before the observers, whose callbacks must not register a component mid-operation
    m_entities.component<Zone>();
    m_entities.component<ZonePosition>().member(&ZonePosition::x, "x").member(&ZonePosition::y, "y").member(&ZonePosition::z, "z");
    m_entities.component<ZoneDocument>();
    m_entities.component<ZonePlacementIndex>();
    m_entities.component<InZone>().add(flecs::Exclusive);
    m_entities.component<ZoneWorldspace>();
    m_entities.component<ZoneRegion>();
    m_entities.component<PersistentId>().member(&PersistentId::id, "id");
    m_entities.component<EntityReference>()
        .member(&EntityReference::persistent_id, "persistent_id")
        .member(&EntityReference::entity, "entity");
    m_entities.component<AttachedEntities>();
    m_entities.component<LayoutEntity>();
    m_entities.component<Site>();
    for_each_layout_address([&]<typename Address>(std::type_identity<Address>)
    {
        m_entities.component<Address>();
        m_entities.component<AddressIndex<Address>>();
    });

    m_entities.observer<const PersistentId>()
        .event(flecs::OnSet)
        .each([this](flecs::entity e, const PersistentId& persistent_id)
        {
            [[maybe_unused]] auto [i, inserted] = m_loaded_entities.emplace(persistent_id, e);
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
    m_entities.observer<const ZonePosition>()
        .event(flecs::OnSet)
        .each([this](flecs::entity zone, const ZonePosition& position)
        {
            std::map<ZonePosition, PersistentId>& zone_ids = worldspace(worldspace_of(zone)).zone_ids;
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

            std::visit([&](const auto& address)
            {
                std::vector<flecs::entity>& attached = ensure_layout_entity(zone, address).template ensure<AttachedEntities>().entities;
                if (std::find(attached.begin(), attached.end(), e) == attached.end())
                {
                    attached.push_back(e);
                }
            }, *site.address);
        });

    // An entity in a zone whose site has no address is in the zone's placement index, at its placement
    m_entities.observer<const Site>()
        .event(flecs::OnSet)
        .each([](flecs::entity e, const Site& site)
        {
            flecs::entity zone = zone_containing(e);
            if (!zone)
            {
                return;
            }

            PlacementIndex& placement_index = *zone.get<ZonePlacementIndex>().placement_index;

            if (!site.address.has_value())
            {
                placement_index.place(e.id(), site.placement);
            }
            else if (placement_index.contains(e.id()))
            {
                placement_index.remove(e.id());
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

            zone.get<ZonePlacementIndex>().placement_index->remove(e.id());
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
    for_each_layout_address([&]<typename Address>(std::type_identity<Address>)
    {
        m_entities.observer<const Address>()
            .event(flecs::OnRemove)
            .each([this](flecs::entity layout_entity, const Address& address)
            {
                flecs::entity zone = stratum_sim::zone_of(layout_entity);
                if (!zone || deleting_with(zone))
                {
                    return;
                }

                erase_layout_entity(zone, address);

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
quetzal::stratum_sim::RandomStream::seed_type quetzal::stratum_sim::World::world_seed() const
{
    return m_world_seed;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::set_world_seed(RandomStream::seed_type seed)
{
    m_world_seed = seed;
    return;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create(std::string_view region_spec_name, const std::filesystem::path& game_directory)
{
    m_game_directory = game_directory;

    const RegionSpec& region_spec = registered(m_generation.m_region_specs, "region spec", std::string(region_spec_name));
    std::string entry_access = region_spec.keys.value_or(entry_access_key, std::string());
    size_t comma = entry_access.find(',');
    flecs::entity entry_type = comma != std::string::npos ? access_type(m_entities, entry_access.substr(0, comma)) : flecs::entity();
    std::optional<level_type> entry_offset = comma != std::string::npos ? try_type<level_type>(entry_access.substr(comma + 1)) : std::nullopt;

    if (!entry_type || !entry_offset.has_value())
    {
        throw std::runtime_error(std::format("the starting region spec's {} is not a registered access point type, a comma, and a level offset: '{}'",
            entry_access_key, entry_access));
    }

    flecs::entity zone = create_worldspace_zone(region_spec_name, region_spec, ZoneSpec {select_prefixed(region_spec.keys, zone_prefix)});
    flecs::entity entry = entry_point(zone, entry_type, *entry_offset);

    if (!entry)
    {
        throw std::runtime_error(std::format("the starting region's first zone has no access point for its {}: '{}'", entry_access_key, entry_access));
    }

    return entry;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::open(const std::filesystem::path& game_directory)
{
    stratum_format::WorldDocument document = stratum_format::read_world(game_directory / stratum_format::world_filename);

    m_game_directory = game_directory;

    for (const stratum_format::WorldDocument::Worldspace& document_worldspace : document.worldspaces)
    {
        const stratum_format::WorldDocument::Region& document_region = document_worldspace.region;
        Worldspace& worldspace = create_worldspace(document_worldspace.name, RegionSpec {document_region.spec},
            PersistentId {document_worldspace.id}, PersistentId {document_region.id});
        std::map<ZonePosition, PersistentId>& positioned_zone_ids = worldspace.zone_ids;
        Region& region = worldspace.regions.root();
        region.name = document_region.name;
        region.name_counter = document_region.name_counter;

        for (const stratum_format::WorldDocument::Zone& document_zone : document_region.zones)
        {
            region.zone_ids.push_back(PersistentId {document_zone.id});

            if (document_zone.position.has_value())
            {
                positioned_zone_ids.emplace(ZonePosition {document_zone.position->x(), document_zone.position->y(), document_zone.position->z()},
                    PersistentId {document_zone.id});
            }

            if (document_zone.name.has_value())
            {
                region.named_zone_ids.emplace(*document_zone.name, PersistentId {document_zone.id});
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
        assert(worldspace.regions.regions().size() == 1);

        std::map<PersistentId, ZonePosition> zone_positions; // <zone_id, position>
        for (const auto& [position, zone_id] : worldspace.zone_ids)
        {
            zone_positions.emplace(zone_id, position);
        }

        const Region& region_data = worldspace.regions.root();
        std::map<PersistentId, std::string> zone_names; // <zone_id, name>
        for (const auto& [name, zone_id] : region_data.named_zone_ids)
        {
            zone_names.emplace(zone_id, name);
        }

        stratum_format::WorldDocument::Region document_region;
        document_region.id = region_data.region_id.id;
        document_region.name = region_data.name;
        document_region.name_counter = region_data.name_counter;
        document_region.spec = region_data.region_spec.keys;

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

        document.worldspaces.push_back(stratum_format::WorldDocument::Worldspace {worldspace.worldspace_id.id, worldspace.name,
            std::move(document_region)});
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
size_t quetzal::stratum_sim::World::zone_count() const
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
quetzal::stratum_sim::PersistentId quetzal::stratum_sim::World::create_worldspace(std::string_view name, const RegionSpec& region_spec)
{
    RegionSpec positioned_spec = region_spec;
    if (!region_spec.keys.contains(position_x_key) && !region_spec.keys.contains(position_y_key))
    {
        positioned_spec.set_position(new_region_position());
    }

    const std::string untaken = untaken_name(std::string(name), m_worldspace_name_counter, [this](const std::string& candidate)
    {
        return std::ranges::contains(m_worldspaces, candidate, &Worldspace::name);
    });

    const PersistentId worldspace_id = next_id();
    const PersistentId region_id = next_id();
    return create_worldspace(untaken, positioned_spec, worldspace_id, region_id).worldspace_id;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Worldspace& quetzal::stratum_sim::World::create_worldspace(std::string_view name, const RegionSpec& region_spec,
    PersistentId worldspace_id, PersistentId region_id)
{
    Region root_region;
    root_region.region_id = region_id;
    root_region.name = std::string(name);
    root_region.region_spec = region_spec;

    m_worldspaces.push_back(Worldspace::create(worldspace_id, name, region_spec, std::move(root_region)));
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
quetzal::stratum_sim::RegionPosition quetzal::stratum_sim::World::new_region_position() const
{
    bool any_region = false;
    coord_type x_max = 0;

    for (const auto& worldspace : m_worldspaces)
    {
        for (const auto& region : worldspace.regions.regions())
        {
            const coord_type x = region.region_spec.position().x;
            x_max = any_region ? std::max(x_max, x) : x;
            any_region = true;
        }
    }

    return {any_region ? x_max + 1 : 0, 0};
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::ZonePosition quetzal::stratum_sim::World::new_zone_position(PersistentId, level_type level) const
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
    else if (const auto* position = zone.try_get<ZonePosition>())
    {
        name = zone_name(default_zone_name_pattern(worldspace(worldspace_of(zone)), region_data), *position);
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
flecs::entity quetzal::stratum_sim::World::create_worldspace_zone(std::string_view name, const RegionSpec& region_spec, const ZoneSpec& zone_spec)
{
    return create_zone(worldspace(create_worldspace(name, region_spec)).regions.root().region_id, zone_spec);
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::unload_worldspace(PersistentId worldspace_id)
{
    entities_type worldspace_zones;
    m_entities.each([&](flecs::entity zone, const ZoneWorldspace& zone_worldspace)
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
        .set<ZoneWorldspace>(ZoneWorldspace {region_worldspace(region_id).worldspace_id})
        .set<ZoneRegion>(ZoneRegion {region_id})
        .set<PersistentId>(zone_id);
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create_zone(PersistentId region_id, std::unique_ptr<Layout> layout)
{
    assert(layout != nullptr);

    flecs::entity zone = create_zone(region_id, next_id());
    set_layout(zone, std::move(layout));
    region(region_id).zone_ids.push_back(zone.get<PersistentId>());
    name_zone(zone);

    return zone;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create_zone(PersistentId region_id, const ZoneSpec& zone_spec)
{
    const stratum_format::Document document = generate_zone(region_id, resolve_zone_spec(region_id, zone_spec));
    return build_zone(region_id, next_id(), document, "the game's zone generator");
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::load_zone(PersistentId region_id, const std::filesystem::path& filepath)
{
    const stratum_format::Document document = stratum_format::read_zone(filepath);

    if (!document.cells.has_value())
    {
        return build_zone(region_id, PersistentId {document.zone_id.value()},
            generate_zone(region_id, resolve_zone_spec(region_id, ZoneSpec {document.zone_spec.value()})),
            std::format("{}, the game's zone generator", filepath.string()));
    }

    return build_zone(region_id, document.zone_id.has_value() ? PersistentId {*document.zone_id} : next_id(), document, filepath.string());
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
        throw std::runtime_error(std::format("zone {} is in no region", zone_id.id));
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
        PersistentId zone_id = zone.get<PersistentId>();
        discard_made_zone(access_point, zone);
        throw std::runtime_error(std::format("zone {}, made for access point {}, has no access point of the type '{}' whose target is {:+}",
            zone_id.id, access_point.get<PersistentId>().id, arrival_type.name().c_str(), offset));
    }

    return entry;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::discard_made_zone(flecs::entity access_point, flecs::entity zone)
{
    assert(zone && zone.has<Zone>());

    PersistentId zone_id = zone.get<PersistentId>();
    const PersistentId worldspace_id = worldspace_of(zone);

    if (worldspace_id != worldspace_of(stratum_sim::zone_of(access_point)))
    {
        unload_worldspace(worldspace_id);
    }
    else
    {
        Region& region_data = region(region_of(zone));
        std::erase(region_data.zone_ids, zone_id);
        std::erase_if(region_data.named_zone_ids, [zone_id](const auto& named_zone) { return named_zone.second == zone_id; });
        std::erase_if(worldspace(worldspace_id).zone_ids, [zone_id](const auto& positioned_zone) { return positioned_zone.second == zone_id; });
        unload_zone(zone);
    }

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
// One pursuer at the last open cell (temporary ...)
void quetzal::stratum_sim::World::populate(flecs::entity zone)
{
    const DiscreteLayout& zone_layout = layout_as<DiscreteLayout>(layout(zone));

    std::optional<discrete_point_type> start = last_open_cell(zone_layout);

    if (!start)
    {
        return;
    }

    create_entity(zone, placement_type(zone_layout.position(*start), vector_type(value_type(-1), value_type(0), value_type(0))))
        .add<Blocking>()
        .set<Speed>(Speed {denizen_speed})
        .set<Pursue>(Pursue {});

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
        && std::holds_alternative<stratum_format::RelocatorTarget>(*access_point.get<AccessPoint>().target));

    const auto& target = std::get<stratum_format::RelocatorTarget>(*access_point.get<AccessPoint>().target);

    if (std::holds_alternative<std::string>(target.zone) || target.position.has_value())
    {
        throw std::runtime_error(std::format("access point {} has a target with a named zone or a position, which is not resolved",
            access_point.get<PersistentId>().id));
    }

    flecs::entity zone_from = stratum_sim::zone_of(access_point);
    assert(zone_from.has<ZonePosition>());

    level_type level_from = zone_from.get<ZonePosition>().z;
    const auto* relative = std::get_if<stratum_format::RelativeZone>(&target.zone);
    level_type level = relative != nullptr ? level_from + relative->offset : std::get<stratum_format::AbsoluteZone>(target.zone).level;
    level_type offset = level - level_from;

    flecs::entity arrival_type = access_point.target<AccessType>().get<Arrival>().access_type;
    const PersistentId worldspace_id = worldspace_of(zone_from);
    const ZonePosition position = new_zone_position(worldspace_id, level);
    ZoneSpec zone_spec;
    zone_spec.set_position(position);

    if (target.region.has_value())
    {
        return require_entry_point(access_point, create_worldspace_zone(*target.region, registered(m_generation.m_region_specs, "region spec",
            *target.region), zone_spec), arrival_type, -offset);
    }

    const std::map<ZonePosition, PersistentId>& zone_ids = worldspace(worldspace_id).zone_ids;
    auto i = zone_ids.find(position);

    if (i == zone_ids.end())
    {
        return require_entry_point(access_point, create_zone(region_of(zone_from), zone_spec), arrival_type, -offset);
    }

    const PersistentId zone_id = i->second;
    flecs::entity zone = entity(zone_id);
    if (!zone)
    {
        zone = load_saved_zone(zone_id);
    }

    flecs::entity entry = entry_point(zone, arrival_type, -offset);

    if (!entry)
    {
        const std::string message = std::format("zone {}, at the target of access point {}, has no access point of the type '{}' whose target is {:+}",
            zone_id.id, access_point.get<PersistentId>().id, arrival_type.name().c_str(), -offset);
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
                throw std::runtime_error(std::format("access point {} is bound to access point {}, whose zone is not loaded and cannot be "
                    "read back without a game directory", access_point.get<PersistentId>().id, reference->persistent_id.id));
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

    for (flecs::entity zone : m_zone_entry_index.select_unloads(zones(), zone_entered))
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
    assert(zone && zone.has<Zone>() && zone.has<ZoneDocument>());

    std::filesystem::path filepath = saved_zone_path(zone.get<PersistentId>());
    std::filesystem::create_directories(filepath.parent_path());
    stratum_format::write(filepath, make_document(zone));

    return;
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::World::zone_count_max() const
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

    const Layout& zone_layout = layout(zone);

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
quetzal::stratum_sim::Layout::placements_type quetzal::stratum_sim::World::neighbors(flecs::entity e) const
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
quetzal::stratum_sim::Layout& quetzal::stratum_sim::World::layout(flecs::entity zone)
{
    assert(zone && zone.has<Zone>());
    return *zone.get<ZoneLayout>().layout;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Layout& quetzal::stratum_sim::World::layout(flecs::entity zone) const
{
    assert(zone && zone.has<Zone>());
    return *zone.get<ZoneLayout>().layout;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::PlacementIndex& quetzal::stratum_sim::World::placement_index(flecs::entity zone) const
{
    assert(zone && zone.has<Zone>());
    return *zone.get<ZonePlacementIndex>().placement_index;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::World::entities_type quetzal::stratum_sim::World::entities_at(flecs::entity zone, const placement_type& placement) const
{
    return entities_of(zone, placement_index(zone).entities_at(placement));
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::World::entities_type quetzal::stratum_sim::World::entities_at(flecs::entity zone, const discrete_point_type& cell_position) const
{
    return entities_of(zone, dynamic_cast<const DiscretePlacementIndex&>(placement_index(zone)).entities_at(cell_position));
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::World::entities_type quetzal::stratum_sim::World::entities_within(flecs::entity zone, const placement_type& placement, value_type range) const
{
    return entities_of(zone, placement_index(zone).entities_within(placement, range));
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
flecs::entity quetzal::stratum_sim::World::build_zone(PersistentId region_id, PersistentId zone_id, const stratum_format::Document& document,
    std::string_view source)
{
    assert(!document.zone_id.has_value() || document.zone_id.value() == zone_id.id);

    if (std::optional<std::string> error = check_buildable(m_entities, document))
    {
        throw std::runtime_error(std::format("{}: {}", source, *error));
    }

    if (document.zone_id.has_value() && entity(PersistentId {*document.zone_id}))
    {
        throw std::runtime_error(std::format("{}: zone {} is already loaded", source, *document.zone_id));
    }

    if (document.position.has_value())
    {
        const ZonePosition position {document.position->x(), document.position->y(), document.position->z()};
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
quetzal::stratum_sim::ZoneSpec quetzal::stratum_sim::World::resolve_zone_spec(PersistentId region_id, const ZoneSpec& zone_spec) const
{
    const RegionSpec& region_spec = region(region_id).region_spec;
    ZoneSpec resolved {select_prefixed(region_spec.keys, zone_prefix) | zone_spec.keys};
    resolved.keys.insert(std::string(seed_key), std::to_string(zone_seed(region_seed(m_world_seed, region_spec), resolved)));
    return resolved;
}

//------------------------------------------------------------------------------
quetzal::stratum_format::Document quetzal::stratum_sim::World::generate_zone(PersistentId region_id, const ZoneSpec& zone_spec)
{
    assert(m_generation.m_zone_generator);
    stratum_format::Document document = m_generation.m_zone_generator(zone_spec);
    assert(!document.zone_id.has_value());
    document.is_unpopulated = true;
    const ZonePosition position = zone_spec.position();
    document.position = stratum_format::zone_position_type {position.x, position.y, position.z};

    const std::string name_pattern = zone_spec.keys.value_or(name_key, default_zone_name_pattern(region_worldspace(region_id), region(region_id)));
    document.components[std::string(name_component)] = zone_name(name_pattern, position);

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
    for_each_layout_address([&]<typename Address>(std::type_identity<Address>)
    {
        if (const auto* address = e.try_get<Address>())
        {
            append_layout_levels(chain, zone, zone_layout, *address);
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
std::string quetzal::stratum_sim::World::extended_name(flecs::entity zone) const
{
    assert(zone && zone.has<Zone>() && zone.has<Name>());
    return std::format("{}/{}", worldspace(worldspace_of(zone)).name, zone.get<Name>().name);
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
    return zone.get<ZoneWorldspace>().worldspace_id;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::PersistentId quetzal::stratum_sim::region_of(flecs::entity zone)
{
    assert(zone && zone.has<Zone>());
    return zone.get<ZoneRegion>().region_id;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::set_layout(flecs::entity zone, std::unique_ptr<Layout> layout)
{
    assert(zone && zone.has<Zone>() && layout != nullptr);

    std::unique_ptr<PlacementIndex> placement_index = layout->create_placement_index();
    zone.set<ZoneLayout>(ZoneLayout {std::move(layout)});
    zone.set<ZonePlacementIndex>(ZonePlacementIndex {std::move(placement_index)});

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
