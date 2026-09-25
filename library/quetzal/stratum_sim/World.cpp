//------------------------------------------------------------------------------
// stratum_sim
// World.cpp
//------------------------------------------------------------------------------

#include "World.hpp"
#include "ai.hpp"
#include "attachments.hpp"
#include "DiscreteLayout.hpp"
#include "load_layout.hpp"
#include "Site.hpp"
#include "quetzal/stratum_format/read.hpp"
#include <algorithm>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
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
    // The zone's children with no site address whose placement satisfies accept, in id order. A placement with an
    // address is relative to that address
    template<typename F>
    quetzal::stratum_sim::World::entities_type placed_in(flecs::entity zone, F accept)
    {
        quetzal::stratum_sim::World::entities_type entities;

        zone.world().query_builder<const quetzal::stratum_sim::Site>()
            .with(flecs::ChildOf, zone)
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
    auto is_crossable(flecs::entity zone)
    {
        return [zone](quetzal::id_type wall_id) { return quetzal::stratum_sim::crossable(zone, wall_id); };
    }

    //--------------------------------------------------------------------------
    // Moves the entries of e and everything below it to the zone
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

} // namespace

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::zone_of(flecs::entity e)
{
    for (flecs::entity parent = e.parent(); parent; parent = parent.parent())
    {
        if (parent.has<Zone>())
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

    e.child_of(parent); // ChildOf is exclusive, so this removes the old parent
    e.set<Site>(site);

    return;
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
    m_entities.component<PersistentId>();

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

            if (!zone || !m_zones_unloading.contains(zone.get<PersistentId>()))
            {
                m_entity_zones.erase(persistent_id);
            }

            // Its contents were deleted before it, so a zone's unload is complete here
            if (is_zone)
            {
                m_zones_unloading.erase(persistent_id);
            }
        });

    // An entity placed in a zone gets its persistent id here, whatever placed it, and a new parent may be in another zone
    m_entities.observer()
        .with(flecs::ChildOf, flecs::Wildcard)
        .event(flecs::OnAdd)
        .each([this](flecs::entity e)
        {
            if (flecs::entity zone = stratum_sim::zone_of(e))
            {
                if (!e.has<PersistentId>())
                {
                    e.set<PersistentId>(next_id());
                }

                set_entity_zones(m_entity_zones, e, zone.get<PersistentId>());
            }
        });

    // The access point types registered by default, a mod registering more
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

    if (std::optional<std::string> type_name = unregistered_access_type(m_entities, document))
    {
        throw std::runtime_error(std::format("{}: access point type '{}' is not registered", filepath.string(), *type_name));
    }

    if (document.zone_id.has_value() && entity(PersistentId {*document.zone_id}))
    {
        throw std::runtime_error(std::format("{}: zone {} is already loaded", filepath.string(), *document.zone_id));
    }

    flecs::entity zone = create_zone(document.zone_id.has_value() ? PersistentId {*document.zone_id} : next_id());
    load_layout(zone, document);
    populate(zone);

    return zone;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::unload_zone(flecs::entity zone)
{
    assert(zone && zone.has<Zone>());

    // The unload finishes in the PersistentId observer, when the deletion runs, which is later where flecs is deferring
    m_zones_unloading.insert(zone.get<PersistentId>());
    zone.destruct(); // Takes its children with it, deleting them before the zone

    return;
}

//------------------------------------------------------------------------------
// One pursuer at the last open cell (temporary ...)
void quetzal::stratum_sim::World::populate(flecs::entity zone)
{
    const DiscreteLayout& discrete = layout_as<DiscreteLayout>(layout(zone));

    std::optional<discrete_point_type> start = last_open_cell(discrete);

    if (!start)
    {
        return;
    }

    create_entity(zone, placement_type(discrete.position(*start), vector_type(value_type(-1), value_type(0), value_type(0))))
        .add<Blocking>()
        .set<Speed>(Speed {denizen_speed})
        .set<Pursue>(Pursue {});

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::World::relocate(flecs::entity e, flecs::entity zone, const placement_type& placement)
{
    assert(zone && zone.has<Zone>());

    reparent(e, zone, Site {Relation::In, {}, placement});
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

    return zone_layout.move(placement_of(e), direction, distance, is_crossable(zone), blocker_at);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Layout::placements_type quetzal::stratum_sim::World::neighbors(flecs::entity e) const
{
    flecs::entity zone = stratum_sim::zone_of(e);
    assert(zone);

    return layout(zone).neighbors(placement_of(e), is_crossable(zone));
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::World::visible(flecs::entity e, const placement_type& to) const
{
    flecs::entity zone = stratum_sim::zone_of(e);
    assert(zone);

    return layout(zone).visible(placement_of(e), to, is_crossable(zone));
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
