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
#include "properties.hpp"
#include "Site.hpp"
#include "Zone.hpp"
#include "ZoneEntryIndex.hpp"
#include "types.hpp"
#include <flecs.h>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Tag for an entity that others cannot move into
    struct Blocking
    {
    };

    //--------------------------------------------------------------------------
    // The rate at which an actor gains energy in the turn order
    struct Speed
    {
        int value = 0;
    };

    //--------------------------------------------------------------------------
    // Returns the zone that e is in, directly or through its ancestors, or a null entity if it is in none. Entities nest,
    // like coins in a pouch that an actor carries
    flecs::entity zone_of(flecs::entity e);

    //--------------------------------------------------------------------------
    // Sets an entity's parent and site together. Its children move with it and its id is kept, so references to it stay
    // valid
    void reparent(flecs::entity e, flecs::entity parent, const Site& site);

    // Sets only the parent, for an entity that has no site. A zone contains its entities through ChildOf, and an entity
    // contains its children through Parent
    void set_parent(flecs::entity e, flecs::entity parent);

    //--------------------------------------------------------------------------
    // Creates an entity type, which is a prefab. An entity IsA its type, and the type's components are the entity's
    // defaults. The caller sets the type's components
    flecs::entity create_entity_type(flecs::world& entities, std::string_view name);

    // Returns a null entity if no type with that name is loaded
    flecs::entity entity_type(const flecs::world& entities, std::string_view name);

    //--------------------------------------------------------------------------
    // The zones and everything in them. All entities are in one flecs world; a zone's contents are its children, and its
    // layout is a component on it
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

        // In id order. A zone that has begun unloading is excluded, even if flecs has deferred its deletion
        entities_type zones() const;
        size_t zone_count() const;

        // Creates an empty zone for a generator or the loader to fill
        flecs::entity create_zone(PersistentId zone_id);

        // Creates a zone that has the layout and nothing else
        flecs::entity create_zone(std::unique_ptr<Layout> layout);

        // Creates a zone, has the generator fill it, and populates it
        flecs::entity create_zone(const Generator& generator);

        // Reads a zone from a file and populates it. The zone has the file's persistent id if the file has one.
        // Throws std::runtime_error, before anything is built, if the reader rejects the file, build_error finds a reason the
        // world cannot build it, or the file's zone is already loaded
        flecs::entity load_zone(const std::filesystem::path& filepath);

        void unload_zone(flecs::entity zone);

        // Places the zone's denizens
        void populate(flecs::entity zone);

        // Relocation keeps the entity's id, so references to it remain valid. An entity with no parent is owned by the game
        // and stays that way, located in the zone through InZone. Any other entity becomes a child of the zone
        void relocate(flecs::entity e, flecs::entity zone, const placement_type& placement);

        // Marks zone_entered as the most recently entered zone, then unloads the zones entered longest ago until the world
        // has no more than zone_count_max. Never unloads zone_entered
        void trim_zones(flecs::entity zone_entered);

        size_t zone_count_max() const;
        void set_zone_count_max(size_t zone_count_max);

        // The layout's queries, run from the entity's zone and placement. Whether a wall is crossable depends on how the
        // entity perceives what is on it. In move, every Blocking entity other than e blocks the way
        MoveResult move(flecs::entity e, const vector_type& direction, value_type distance) const;
        Layout::placements_type neighbors(flecs::entity e) const;
        bool visible(flecs::entity e, const placement_type& to) const;

        Layout& layout(flecs::entity zone);
        const Layout& layout(flecs::entity zone) const;

        entities_type entities_at(flecs::entity zone, const placement_type& placement) const;
        entities_type entities_at(flecs::entity zone, const discrete_point_type& cell_position) const;
        entities_type entities_within(flecs::entity zone, const placement_type& placement, value_type range) const;

        flecs::entity create_entity(flecs::entity zone, const placement_type& placement);

        // Returns the component merged along the chain of the entity, its type, the room that contains it, and the layout.
        // Each one fills the optional fields that the ones before it left unset. Returns nothing if none of them has the
        // component. A layout entity's chain has no type: a wall's chain is the wall, its room, and the layout, and a flat
        // cell's chain is the cell, the flat's override for its room, the room, the flat, and the layout
        template<typename T>
        std::optional<T> property(flecs::entity e) const;

        // Returns the loaded entity with that id, or a null entity if it is not loaded
        flecs::entity entity(PersistentId persistent_id) const;

        // Returns the zone of an entity in the world index, whether or not the entity is loaded
        PersistentId zone_of(PersistentId persistent_id) const;

        // Returns a reference to the entity and adds the entity to the world index
        EntityReference make_reference(flecs::entity target);

        // Returns the entity the reference refers to, or a null entity if it is not loaded. If the entity stored in the
        // reference is no longer alive, looks it up again by persistent id and stores the result in the reference
        flecs::entity resolve(EntityReference& reference) const;

        PersistentId next_id();

    private:

        // The entities that property reads, nearest first
        entities_type property_chain(flecs::entity e) const;

        // Returns true if the zone or the whole world is being deleted, in which case the entities deleted with it need no
        // unlinking
        bool deleting_with(flecs::entity zone) const;

        IdAllocator& m_id_allocator;

        // All entities in the world with a PersistentId
        std::unordered_map<PersistentId, flecs::entity> m_loaded_entities;

        // <entity_id, zone_id>
        std::unordered_map<PersistentId, PersistentId> m_entity_zones;

        // Zones that are being unloaded, whose entities remain in m_entity_zones
        std::unordered_set<PersistentId> m_zones_unloading;

        ZoneEntryIndex m_zone_entry_index;

        flecs::world m_entities; // Last, as its teardown runs the observers that update the members above
    };

} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
template<typename T>
std::optional<T> quetzal::stratum_sim::World::property(flecs::entity e) const
{
    std::optional<T> result;

    for (flecs::entity level : property_chain(e))
    {
        if (!level.owns<T>())
        {
            continue;
        }

        if (!result.has_value())
        {
            result = level.get<T>();
        }
        else
        {
            merge_unset(level.world(), level.world().id<T>(), &*result, &level.get<T>());
        }
    }

    return result;
}

#endif // QUETZAL_STRATUM_SIM_WORLD_HPP
