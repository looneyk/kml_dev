//------------------------------------------------------------------------------
// stratum_sim
// World.cpp
//------------------------------------------------------------------------------

#include "World.hpp"
#include "ai.hpp"
#include "attachments.hpp"
#include "DiscreteLayout.hpp"
#include "layout_entities.hpp"
#include "load_layout.hpp"
#include "Site.hpp"
#include "quetzal/stratum_format/read.hpp"
#include <algorithm>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <cassert>

namespace
{

    static constexpr int denizen_speed = 100;

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
    // Returns true if a layout entity has any component other than its address, its parent, and the list of entities sited on it
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

            if (!parent && !address && id != world.id<quetzal::stratum_sim::AttachedEntities>())
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
void quetzal::stratum_sim::reparent(flecs::entity e, flecs::entity parent, const Site& site)
{
    assert(e && parent && e != parent);

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
quetzal::stratum_sim::World::World(IdAllocator& id_allocator, size_t zone_count_max) :
    m_id_allocator(id_allocator),
    m_loaded_entities(),
    m_entity_zones(),
    m_zones_unloading(),
    m_zone_entry_index(zone_count_max),
    m_entities()
{
    // Registered before the observers, whose callbacks must not register a component mid-operation
    m_entities.component<Zone>();
    m_entities.component<InZone>().add(flecs::Exclusive);
    m_entities.component<PersistentId>().member(&PersistentId::id, "id");
    m_entities.component<EntityReference>()
        .member(&EntityReference::persistent_id, "persistent_id")
        .member(&EntityReference::entity, "entity");
    m_entities.component<AttachedEntities>();
    m_entities.component<ZoneAddressIndex>();
    m_entities.component<Site>();
    for_each_layout_address([&]<typename Address>(std::type_identity<Address>) { m_entities.component<Address>(); });

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
        if (is_layout_entity(e))
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

    // The default access point types; a mod may register more
    flecs::entity up = create_access_type(m_entities, "up");
    flecs::entity down = create_access_type(m_entities, "down");
    flecs::entity waypoint = create_access_type(m_entities, "waypoint");
    flecs::entity portal = create_access_type(m_entities, "portal");

    up.set<Arrival>(Arrival {down});
    down.set<Arrival>(Arrival {up});
    waypoint.set<Arrival>(Arrival {waypoint});
    portal.set<Arrival>(Arrival {portal});
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
flecs::entity quetzal::stratum_sim::World::create_zone(PersistentId zone_id)
{
    return m_entities.entity().add<Zone>().set<PersistentId>(zone_id);
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create_zone(std::unique_ptr<Layout> layout)
{
    assert(layout != nullptr);

    flecs::entity zone = create_zone(next_id());
    zone.set<ZoneLayout>(ZoneLayout {std::move(layout)});

    return zone;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::create_zone(const Generator& generator)
{
    flecs::entity zone = create_zone(next_id());

    try
    {
        generator.generate(zone);
    }
    catch (...)
    {
        unload_zone(zone);
        throw;
    }

    populate(zone);

    return zone;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::World::load_zone(const std::filesystem::path& filepath)
{
    stratum_format::Document document = stratum_format::read(filepath);

    if (std::optional<std::string> error = build_error(m_entities, document))
    {
        throw std::runtime_error(std::format("{}: {}", filepath.string(), *error));
    }

    if (document.zone_id.has_value() && entity(PersistentId {*document.zone_id}))
    {
        throw std::runtime_error(std::format("{}: zone {} is already loaded", filepath.string(), *document.zone_id));
    }

    flecs::entity zone = create_zone(document.zone_id.has_value() ? PersistentId {*document.zone_id} : next_id());

    for (flecs::entity target : load_layout(zone, document))
    {
        make_reference(target);
    }

    populate(zone);

    return zone;
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

    e.remove<Site>(); // First, so it leaves what it was sited on while still in that zone
    e.add<InZone>(zone);
    e.set<Site>(site);

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::trim_zones(flecs::entity zone_entered)
{
    assert(zone_entered && zone_entered.has<Zone>());

    for (flecs::entity zone : m_zone_entry_index.select_unloads(zones(), zone_entered))
    {
        assert(zone != zone_entered);
        unload_zone(zone);
    }

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
