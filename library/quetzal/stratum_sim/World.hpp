#if !defined(QUETZAL_STRATUM_SIM_WORLD_HPP)
#define QUETZAL_STRATUM_SIM_WORLD_HPP
//------------------------------------------------------------------------------
// stratum_sim
// World.hpp
//------------------------------------------------------------------------------

#include "Generator.hpp"
#include "IdAllocator.hpp"
#include "Layout.hpp"
#include "PersistentId.hpp"
#include "Site.hpp"
#include "Zone.hpp"
#include "ZoneEntryIndex.hpp"
#include "types.hpp"
#include <flecs.h>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Tag, an entity another cannot move into.
    struct Blocking
    {
    };

    //--------------------------------------------------------------------------
    // The rate an actor gains energy at in the turn order
    struct Speed
    {
        int value = 0;
    };

    //--------------------------------------------------------------------------
    // The nearest ancestor that is a zone, or a null entity. Entities nest, as coins in a pouch an actor carries do
    flecs::entity zone_of(flecs::entity e);

    //--------------------------------------------------------------------------
    // Sets an entity's parent and site together. Its children move with it and its id is kept, so references to it stay
    // valid
    void reparent(flecs::entity e, flecs::entity parent, const Site& site);

    //--------------------------------------------------------------------------
    // The zones and everything in them. Entities are in one flecs world, a zone's contents being its children and its
    // layout a component on it
    class World
    {
    public:

        using entities_type = std::vector<flecs::entity>;

        World(IdAllocator& id_allocator, size_t zone_count_max);
        World(const World&) = delete;
        ~World() = default;

        World& operator=(const World&) = delete;

        flecs::world& entities();
        const flecs::world& entities() const;

        // In id order. A zone whose unload has begun is not among them, though its deletion may be deferred
        entities_type zones() const;
        size_t zone_count() const;

        // A zone with nothing in it, for a generator or a loader to fill
        flecs::entity create_zone(PersistentId zone_id);

        // A zone with the layout and nothing else
        flecs::entity create_zone(std::unique_ptr<Layout> layout);

        // A generated zone, populated.
        flecs::entity create_zone(const Generator& generator);

        // A zone read from a file, populated, with the file's persistent id where it has one.
        // Throws std::runtime_error when the reader rejects the file, an access point's type is not registered, or the file's
        // zone is already loaded, before anything is built
        flecs::entity load_zone(const std::filesystem::path& filepath);

        void unload_zone(flecs::entity zone);

        // Places the zone's denizens
        void populate(flecs::entity zone);

        // Relocation retains the entity's id, so references to it remain valid
        void relocate(flecs::entity e, flecs::entity zone, const placement_type& placement);

        // Makes zone_entered the zone entered most recently, then unloads the zones entered longest ago while the world has
        // more than zone_count_max, never zone_entered
        void trim_zones(flecs::entity zone_entered);

        size_t zone_count_max() const;
        void set_zone_count_max(size_t zone_count_max);

        // The layout's queries from the entity's zone and placement, each wall crossable as its attachments allow. In move,
        // every Blocking entity other than e blocks
        MoveResult move(flecs::entity e, const vector_type& direction, value_type distance) const;
        Layout::placements_type neighbors(flecs::entity e) const;
        bool visible(flecs::entity e, const placement_type& to) const;

        Layout& layout(flecs::entity zone);
        const Layout& layout(flecs::entity zone) const;

        entities_type entities_at(flecs::entity zone, const placement_type& placement) const;
        entities_type entities_at(flecs::entity zone, const discrete_point_type& cell_position) const;
        entities_type entities_within(flecs::entity zone, const placement_type& placement, value_type range) const;

        flecs::entity create_entity(flecs::entity zone, const placement_type& placement);

        // The loaded entity with that id, null where it is not loaded
        flecs::entity entity(PersistentId persistent_id) const;

        // The zone an entity in the world index is in, loaded or not
        PersistentId zone_of(PersistentId persistent_id) const;

        // A reference to the entity, which adds it to the world index
        EntityReference make_reference(flecs::entity target);

        // The referred-to entity, or null where it is not loaded. When the entity stored in the reference is no longer alive,
        // looks it up again by persistent id and stores the result
        flecs::entity resolve(EntityReference& reference) const;

        PersistentId next_id();

    private:

        IdAllocator& m_id_allocator;

        // All entities in the world with a PersistentId
        std::unordered_map<PersistentId, flecs::entity> m_loaded_entities;

        // <entity_id, zone_id>
        std::unordered_map<PersistentId, PersistentId> m_entity_zones;

        // Zones where unloading is in progress, their entities still in m_entity_zones
        std::unordered_set<PersistentId> m_zones_unloading;

        ZoneEntryIndex m_zone_entry_index;

        flecs::world m_entities; // Last, as its teardown runs the observers that update the members above
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_WORLD_HPP
