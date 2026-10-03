//------------------------------------------------------------------------------
// stratum_sim
// World.cpp
//------------------------------------------------------------------------------

#include "World.hpp"
#include "ai.hpp"
#include "attachments.hpp"
#include "DiscreteLayout.hpp"
#include "DisplayName.hpp"
#include "file_components.hpp"
#include "layout_entities.hpp"
#include "load_layout.hpp"
#include "make_document.hpp"
#include "properties.hpp"
#include "Site.hpp"
#include "specs.hpp"
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
    // Returns the entities in the zone that have no site address and whose placement satisfies accept, in id order. A
    // placement with an address is relative to that address
    template<typename F>
    quetzal::stratum_sim::World::entities_type placed_in(flecs::entity zone, F accept)
    {
        quetzal::stratum_sim::World::entities_type entities;

        quetzal::stratum_sim::with_in_zone(zone.world().query_builder<const quetzal::stratum_sim::Site>(), zone)
            .build()
            .each([&](flecs::entity e, const quetzal::stratum_sim::Site& site)
            {
                if (!site.address.has_value() && accept(site.placement))
                {
                    entities.push_back(e);
                }
            });

        std::sort(entities.begin(), entities.end(), [](flecs::entity a, flecs::entity b) { return a.id() < b.id(); });

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
flecs::entity quetzal::stratum_sim::region_of(flecs::entity zone)
{
    assert(zone && zone.has<Zone>());
    assert(zone.parent() && zone.parent().has<Region>());

    return zone.parent();
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

//------------------------------------------------------------------------------
quetzal::stratum_sim::World::World(IdAllocator& id_allocator, random_type& random, Generation& generation, size_t zone_count_max) :
    m_id_allocator(id_allocator),
    m_random(random),
    m_generation(generation),
    m_loaded_entities(),
    m_entity_zones(),
    m_zones_unloading(),
    m_zone_entry_index(zone_count_max),
    m_game_directory(),
    m_entities()
{
    // Registered before the observers, whose callbacks must not register a component mid-operation
    m_entities.component<Zone>();
    m_entities.component<Region>();
    m_entities.component<ZoneLevel>().member(&ZoneLevel::level, "level");
    m_entities.component<ZoneDocument>();
    m_entities.component<InZone>().add(flecs::Exclusive);
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

    register_display_name(m_entities);
    register_file_component<DisplayName>(m_entities, display_name_component);

    // The state of an entity that a saved zone writes back, which a zone file may also state
    m_entities.component<AccessState>();
    m_entities.component<Access>().member(&Access::state, "state");
    m_entities.component<DetectionState>();
    m_entities.component<ActorDetection>().member(&ActorDetection::actor, "actor").member(&ActorDetection::state, "state");
    register_vector<ActorDetection>(m_entities);
    m_entities.component<Detection>().member(&Detection::state, "state").member(&Detection::actor_states, "actor_states");
    m_entities.component<Speed>().member(&Speed::value, "value");
    m_entities.component<Blocking>();
    m_entities.component<Dead>();
    static_assert(sizeof(point_type) == 3 * sizeof(value_type));
    m_entities.component<point_type>().array<value_type>(3);
    register_optional<EntityReference>(m_entities);
    m_entities.component<Pursue>()
        .member(&Pursue::target, "target")
        .member(&Pursue::position_last_seen, "position_last_seen")
        .member(&Pursue::seen, "seen");

    register_file_component<Access>(m_entities, "access");
    register_file_component<Detection>(m_entities, "detection");
    register_file_component<Speed>(m_entities, "speed");
    register_file_component<Blocking>(m_entities, "blocking");
    register_file_component<Dead>(m_entities, "dead");
    register_file_component<Pursue>(m_entities, "pursue");

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
    entities_type regions;
    m_entities.query_builder<const Region>().build().each([&](flecs::entity region, const Region&) { regions.push_back(region); });

    for (flecs::entity region : regions)
    {
        unload_region(region);
    }

    m_entity_zones.clear();
    m_zone_entry_index.clear();
    m_game_directory.reset();

    return;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create(const RegionSpec& region_spec, const std::filesystem::path& game_directory)
{
    m_game_directory = game_directory;

    std::string entry_access = region_spec.value_or(entry_access_key, std::string());
    size_t comma = entry_access.find(',');
    flecs::entity entry_type = comma != std::string::npos ? access_type(m_entities, entry_access.substr(0, comma)) : flecs::entity();
    std::optional<level_type> entry_offset = comma != std::string::npos ? try_type<level_type>(entry_access.substr(comma + 1)) : std::nullopt;

    if (!entry_type || !entry_offset.has_value())
    {
        throw std::runtime_error(std::format("the starting region spec's {} is not a registered access point type, a comma, and a level offset: '{}'",
            entry_access_key, entry_access));
    }

    flecs::entity zone = create_region_zone(region_spec, select_prefixed(region_spec, zone_prefix));
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

    for (const stratum_format::WorldRegion& world_region : document.regions)
    {
        RegionSpec region_spec;
        for (const auto& [key, value] : world_region.spec.items())
        {
            region_spec.insert(key, value.get<std::string>());
        }

        flecs::entity region = create_region(region_spec, PersistentId {world_region.id});
        std::vector<PersistentId>& zone_ids = region.get_mut<Region>().zone_ids;

        for (const stratum_format::ZoneReference& zone : world_region.zones)
        {
            zone_ids.push_back(PersistentId {zone.id});
        }
    }

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

    entities_type regions;
    m_entities.query_builder<const Region>().build().each([&](flecs::entity region, const Region&) { regions.push_back(region); });
    std::ranges::sort(regions, {}, [](flecs::entity region) { return region.get<PersistentId>(); });

    stratum_format::WorldDocument document;
    document.version = std::string(stratum_format::saved_game_format_version);

    for (flecs::entity region : regions)
    {
        const Region& data = region.get<Region>();
        stratum_format::WorldRegion world_region;
        world_region.id = region.get<PersistentId>().id;
        world_region.spec = stratum_format::raw_values_type::object();

        for (const auto& [key, value] : data.region_spec)
        {
            world_region.spec[key] = value;
        }

        for (PersistentId zone_id : data.zone_ids)
        {
            world_region.zones.push_back(stratum_format::ZoneReference {zone_id.id, saved_zone_relative_path(zone_id).generic_string()});
        }

        document.regions.push_back(std::move(world_region));
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
flecs::entity quetzal::stratum_sim::World::create_region(const RegionSpec& region_spec)
{
    return create_region(region_spec, next_id());
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create_region(const RegionSpec& region_spec, PersistentId region_id)
{
    flecs::entity region = m_entities.entity().add<Region>().set<PersistentId>(region_id);
    region.get_mut<Region>().region_spec = region_spec;
    return region;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create_region_zone(const RegionSpec& region_spec, const ZoneSpec& zone_spec)
{
    return create_zone(create_region(region_spec), zone_spec);
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::unload_region(flecs::entity region)
{
    assert(region && region.has<Region>());

    // Each zone's unload finishes in the PersistentId observer, as unload_zone's does
    region.children([&](flecs::entity child)
    {
        if (child.has<Zone>())
        {
            m_zones_unloading.insert(child.get<PersistentId>());
        }
    });

    region.destruct();

    return;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create_zone(flecs::entity region, PersistentId zone_id)
{
    assert(region && region.has<Region>());
    return m_entities.entity().add<Zone>().child_of(region).set<PersistentId>(zone_id);
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create_zone(flecs::entity region, std::unique_ptr<Layout> layout)
{
    assert(layout != nullptr);

    flecs::entity zone = create_zone(region, next_id());
    zone.set<ZoneLayout>(ZoneLayout {std::move(layout)});
    region.get_mut<Region>().zone_ids.push_back(zone.get<PersistentId>());

    return zone;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create_zone(flecs::entity region, const ZoneSpec& zone_spec)
{
    assert(region && region.has<Region>());

    const RegionSpec& region_spec = region.get<Region>().region_spec;
    std::optional<std::string> selector_name = region_spec.value(generator_selector_key);

    if (!selector_name.has_value())
    {
        throw std::runtime_error(std::format("region {} has no {}", region.get<PersistentId>().id, generator_selector_key));
    }

    const Generation::generator_selector_type& selector = registered(m_generation.m_generator_selectors, "generator selector", *selector_name);
    Generation::GeneratorSelection selection = selector(*this, region, zone_spec);

    assert(selection.generator_name.has_value() + selection.zone_template.has_value() + selection.region_spec_name.has_value() == 1);

    if (selection.region_spec_name.has_value())
    {
        return create_region_zone(registered(m_generation.m_region_specs, "region spec", *selection.region_spec_name), zone_spec);
    }

    if (selection.zone_template.has_value())
    {
        throw std::runtime_error(std::format("generator selector {} chose the zone template {}, and no zone is created from a template",
            *selector_name, *selection.zone_template));
    }

    const Generation::generator_type& generator = registered(m_generation.m_generators, "generator", *selection.generator_name);
    flecs::entity zone = build_zone(region, generator(selection.zone_spec), std::format("generator {}", *selection.generator_name));

    if (std::optional<level_type> level = selection.zone_spec.value<level_type>(level_key))
    {
        zone.set<ZoneLevel>(ZoneLevel {*level});
    }

    return zone;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::load_zone(flecs::entity region, const std::filesystem::path& filepath)
{
    return build_zone(region, stratum_format::read_zone(filepath), filepath.string());
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::load_saved_zone(PersistentId zone_id)
{
    assert(m_game_directory.has_value());

    flecs::entity region;
    m_entities.each([&](flecs::entity candidate, const Region& data)
    {
        if (std::ranges::find(data.zone_ids, zone_id) != data.zone_ids.end())
        {
            region = candidate;
        }
    });

    if (!region)
    {
        throw std::runtime_error(std::format("zone {} is in no region", zone_id.id));
    }

    return load_zone(region, saved_zone_path(zone_id));
}

//------------------------------------------------------------------------------
std::filesystem::path quetzal::stratum_sim::World::saved_zone_path(PersistentId zone_id) const
{
    assert(m_game_directory.has_value());
    return *m_game_directory / saved_zone_relative_path(zone_id);
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::require_entry_point(flecs::entity relocator, flecs::entity zone, flecs::entity arrival_type,
    level_type offset)
{
    flecs::entity entry = entry_point(zone, arrival_type, offset);

    if (!entry)
    {
        PersistentId zone_id = zone.get<PersistentId>();
        discard_made_zone(relocator, zone);
        throw std::runtime_error(std::format("zone {}, made for relocator {}, has no access point of the type '{}' whose target is {:+}",
            zone_id.id, relocator.get<PersistentId>().id, arrival_type.name().c_str(), offset));
    }

    return entry;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::discard_made_zone(flecs::entity relocator, flecs::entity zone)
{
    assert(zone && zone.has<Zone>());

    PersistentId zone_id = zone.get<PersistentId>();
    flecs::entity region = region_of(zone);

    if (region != region_of(stratum_sim::zone_of(relocator)))
    {
        unload_region(region);
    }
    else
    {
        std::erase(region.get_mut<Region>().zone_ids, zone_id);
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
flecs::entity quetzal::stratum_sim::World::resolve_relocator_target(flecs::entity relocator)
{
    assert(relocator.has<Relocator>() && std::holds_alternative<stratum_format::RelocatorTarget>(relocator.get<Relocator>().target));
    assert(relocator.has<AccessType>(flecs::Wildcard));

    const auto& target = std::get<stratum_format::RelocatorTarget>(relocator.get<Relocator>().target);

    if (std::holds_alternative<std::string>(target.zone) || target.position.has_value())
    {
        throw std::runtime_error(std::format("relocator {} has a target with a named zone or a position, which is not resolved",
            relocator.get<PersistentId>().id));
    }

    flecs::entity zone_from = stratum_sim::zone_of(relocator);
    assert(zone_from.has<ZoneLevel>());

    level_type level_from = zone_from.get<ZoneLevel>().level;
    const auto* relative = std::get_if<stratum_format::RelativeZone>(&target.zone);
    level_type level = relative != nullptr ? level_from + relative->offset : std::get<stratum_format::AbsoluteZone>(target.zone).level;
    level_type offset = level - level_from;

    flecs::entity arrival_type = relocator.target<AccessType>().get<Arrival>().access_type;
    ZoneSpec zone_spec {{std::string(level_key), std::to_string(level)}};

    // A target with a region always creates a new region, so no loaded zone is searched
    if (target.region.has_value())
    {
        return require_entry_point(relocator, create_region_zone(registered(m_generation.m_region_specs, "region spec", *target.region),
            zone_spec), arrival_type, -offset);
    }

    flecs::entity region = region_of(zone_from);
    flecs::entity found;
    std::pair<PersistentId, std::uint64_t> found_zone_id_and_order;

    m_entities.query_builder<const AccessPoint>()
        .with<AccessType>(arrival_type)
        .build()
        .each([&](flecs::entity e, const AccessPoint& access_point)
        {
            flecs::entity zone = stratum_sim::zone_of(e);

            if (zone == zone_from || m_zones_unloading.contains(zone.get<PersistentId>()) || !zone.has<ZoneLevel>()
                || zone.get<ZoneLevel>().level != level || region_of(zone) != region || !targets_relocator_level(e, -offset))
            {
                return;
            }

            std::pair zone_id_and_order {zone.get<PersistentId>(), access_point.order};

            if (!found || zone_id_and_order < found_zone_id_and_order)
            {
                found = e;
                found_zone_id_and_order = zone_id_and_order;
            }
        });

    if (found)
    {
        return found;
    }

    return require_entry_point(relocator, create_zone(region, zone_spec), arrival_type, -offset);
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::relocate_through(flecs::entity actor, flecs::entity relocator)
{
    assert(relocator.has<Relocator>());

    flecs::entity arrival;

    if (auto* reference = std::get_if<EntityReference>(&relocator.get_mut<Relocator>().target))
    {
        arrival = resolve(*reference);

        // The relocator it is bound to is in a zone that has unloaded, which is read back. Loading can move the relocator's
        // component, so the reference is found again
        if (!arrival)
        {
            if (!m_game_directory.has_value())
            {
                throw std::runtime_error(std::format("relocator {} is bound to relocator {}, whose zone is not loaded and cannot be read "
                    "back without a game directory", relocator.get<PersistentId>().id, reference->persistent_id.id));
            }

            load_saved_zone(zone_of(reference->persistent_id));
            arrival = resolve(std::get<EntityReference>(relocator.get_mut<Relocator>().target));
            assert(arrival);
        }
    }
    else
    {
        arrival = resolve_relocator_target(relocator);
        relocator.set<Relocator>(Relocator {make_reference(arrival)});
        arrival.set<Relocator>(Relocator {make_reference(relocator)});
    }

    assert(arrival.has<AccessPoint>());
    relocate(actor, stratum_sim::zone_of(arrival), arrival.get<AccessPoint>().placement);

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
quetzal::stratum_sim::World::entities_type quetzal::stratum_sim::World::entities_at(flecs::entity zone, const placement_type& placement) const
{
    const Layout& zone_layout = layout(zone);

    return placed_in(zone, [&](const placement_type& other)
    {
        return zone_layout.same_location(other, placement);
    });
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::World::entities_type quetzal::stratum_sim::World::entities_at(flecs::entity zone, const discrete_point_type& cell_position) const
{
    const DiscreteLayout& zone_layout = layout_as<DiscreteLayout>(layout(zone));

    return placed_in(zone, [&](const placement_type& placement)
    {
        return zone_layout.cell_position_at(placement.position()) == cell_position;
    });
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::World::entities_type quetzal::stratum_sim::World::entities_within(flecs::entity zone, const placement_type& placement, value_type range) const
{
    auto [x, y, z] = placement.position().rep();
    value_type range2 = range * range;

    return placed_in(zone, [&](const placement_type& other)
    {
        auto [ox, oy, oz] = other.position().rep();
        value_type dx = ox - x;
        value_type dy = oy - y;
        value_type dz = oz - z;

        return dx * dx + dy * dy + dz * dz <= range2;
    });
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
flecs::entity quetzal::stratum_sim::World::build_zone(flecs::entity region, const stratum_format::Document& document, std::string_view source)
{
    if (std::optional<std::string> error = check_buildable(m_entities, document))
    {
        throw std::runtime_error(std::format("{}: {}", source, *error));
    }

    if (document.zone_id.has_value() && entity(PersistentId {*document.zone_id}))
    {
        throw std::runtime_error(std::format("{}: zone {} is already loaded", source, *document.zone_id));
    }

    flecs::entity zone = create_zone(region, document.zone_id.has_value() ? PersistentId {*document.zone_id} : next_id());

    // A saved zone was listed when it was created
    if (!document.zone_id.has_value())
    {
        region.get_mut<Region>().zone_ids.push_back(zone.get<PersistentId>());
    }

    for (flecs::entity target : load_layout(zone, document))
    {
        make_reference(target);
    }

    // populate only if this is a a zone template, and maybe not even then if it has been pre-populated; look at the file flag for this ...
    if (!document.zone_id.has_value())
    {
        populate(zone);
    }

    return zone;
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
