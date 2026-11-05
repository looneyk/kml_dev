#if !defined(QUETZAL_STRATUM_SIM_WORLD_HPP)
#define QUETZAL_STRATUM_SIM_WORLD_HPP
//------------------------------------------------------------------------------
// stratum_sim
// World.hpp
//------------------------------------------------------------------------------

#include "Generation.hpp"
#include "IdAllocator.hpp"
#include "Layout.hpp"
#include "PersistentId.hpp"
#include "properties.hpp"
#include "RandomStream.hpp"
#include "Site.hpp"
#include "Zone.hpp"
#include "ZoneEntryIndex.hpp"
#include "Worldspace.hpp"
#include "types.hpp"
#include <flecs.h>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The zones and everything in them. All entities are in one flecs world; a zone's contents are its children, and its layout is a component
    class World
    {
    public:

        using entities_type = std::vector<flecs::entity>;
        using worldspaces_type = std::vector<Worldspace>;

        World(IdAllocator& id_allocator, Generation& generation, size_t zone_count_max);
        World(const World&) = delete;
        World(World&&) = delete;
        ~World() = default;

        World& operator=(const World&) = delete;
        World& operator=(World&&) = delete;

        // World management

        void clear();

        // The seed region and zone seeds derive from
        RandomStream::seed_type world_seed() const;
        void set_world_seed(RandomStream::seed_type seed);

        // Creates a new game's starting worldspace from the registered region spec of that name, and its first zone from the
        // spec's zone keys, and returns the zone's entry point for the spec's entry_access. The game directory is the one
        // zones are saved into until clear. Throws std::runtime_error if the region spec is not registered, entry_access
        // is not a registered access point type and a level offset, or the zone has no such entry point
        flecs::entity create(std::string_view region_spec_name, const std::filesystem::path& game_directory);

        // Reads world.json from the game directory, which zones are saved into until clear: each worldspace, with its
        // persistent id, its name and its top-level region, each region with its persistent id, its region spec and its
        // zones, and the world index. Loads no zone. Requires a cleared world. Throws std::runtime_error if the file cannot
        // be read
        void open(const std::filesystem::path& game_directory);

        // Writes every loaded zone as a saved zone, then world.json, with each worldspace, its region and their zones and
        // the world index, into the game directory. Requires a game directory
        void save();

        // Writes the zone as a saved zone into the game directory's zones, named by its persistent id. Requires a game
        // directory, which a world has only while a game is in progress, and a zone built from a document
        void save_zone(flecs::entity zone);

        flecs::world& entities();
        const flecs::world& entities() const;

        // In id order. A zone that has begun unloading is excluded, even if flecs has deferred its deletion
        entities_type zones() const;
        size_t zone_count() const;

        // In the order they were created. A reference to a worldspace or a region is valid until a worldspace is created or
        // unloaded
        const worldspaces_type& worldspaces() const;

        // Requires a worldspace with that id
        Worldspace& worldspace(PersistentId worldspace_id);
        const Worldspace& worldspace(PersistentId worldspace_id) const;

        // Requires a region with that id in one of the worldspaces
        Region& region(PersistentId region_id);
        const Region& region(PersistentId region_id) const;

        // Creates a worldspace, with the coordinate system that the region spec's worldspace keys state, and its top-level
        // region from the spec, and returns the worldspace's id. The worldspace and its region have the name, with a suffix
        // if another worldspace has it. Throws std::runtime_error if the worldspace keys are not valid
        PersistentId create_worldspace(std::string_view name, const RegionSpec& region_spec);

        // Creates a worldspace and the first zone of its top-level region, returns the zone
        flecs::entity create_worldspace_zone(std::string_view name, const RegionSpec& region_spec, const ZoneSpec& zone_spec);

        // Unloads the worldspace's zones, discarding them without saving, and removes the worldspace and its regions
        void unload_worldspace(PersistentId worldspace_id);

        // Creates an empty zone in the region, for a generator or the loader to fill. A zone given a ZonePosition is listed in
        // its worldspace at that position
        flecs::entity create_zone(PersistentId region_id, PersistentId zone_id);

        // Creates a zone in the region that has the layout
        flecs::entity create_zone(PersistentId region_id, std::unique_ptr<Layout> layout);

        // Creates a zone in the region as the region's generator selector chooses, at the spec's position: built from the
        // Document a generator returns, and populated if the document is marked unpopulated, or the first zone of a new
        // worldspace. The zone's name is the selection's ZoneSpec's name pattern, or else the region spec's zone.name, or
        // else the region's default, with the zone's coordinates. Throws std::runtime_error, before anything is built, if a name the selection depends on is not
        // registered, or the document cannot be built, and passes on what the generator throws
        flecs::entity create_zone(PersistentId region_id, const ZoneSpec& zone_spec);

        // Reads a zone into the region from a file, and populates it if the file is marked unpopulated. The zone is given the persistent
        // id of the file if it has one. Throws std::runtime_error, before anything is built, if the reader rejects the file, check_buildable finds a
        // reason the world cannot build it, or the file is a saved zone that is already loaded
        flecs::entity load_zone(PersistentId region_id, const std::filesystem::path& filepath);

        // Reads a zone of the game that has unloaded back from the game directory, into the region that lists it, as load_zone
        // does. Throws std::runtime_error if the world has no game directory, no region lists the zone, or as load_zone
        // does
        flecs::entity load_saved_zone(PersistentId zone_id);

        // Discards the zone, its contents and its layout, without saving
        void unload_zone(flecs::entity zone);

        // Places the zone's denizens
        void populate(flecs::entity zone);

        // Relocation keeps the entity's id, so references to it remain valid. An entity with no parent is owned by the game
        // and stays that way, located in the zone through InZone. Any other entity becomes a child of the zone
        void relocate(flecs::entity e, flecs::entity zone, const placement_type& placement);

        // Returns the access point at which an actor arrives through the access point, whose unbound target is a relative or
        // absolute level with no position. With the region omitted, it is the entry point of the arrival type that leads
        // back, in the zone of the access point's worldspace at the target level's position, read back if it has unloaded,
        // or else in a zone made there. With a region, it is the entry point of the first zone of a new worldspace created
        // from the region spec of that name. Throws std::runtime_error if the target has a named zone or a position, if that
        // region spec is not registered, if the zone at the position has no such entry point or cannot be read back, or,
        // after discarding the zone made, if it has no such entry point
        flecs::entity resolve_target(flecs::entity access_point);

        // Moves the actor through the access point to the arrival placement of the access point its target refers to, and
        // returns that access point. An unbound target is resolved by resolve_target, and the access point gone through is
        // then bound to the one arrived at, which is bound back to it unless it is arrival only, so going back arrives at
        // the access point gone through. A bound target's access point in a
        // zone that has unloaded is read back with its zone. Throws std::runtime_error, and moves nothing, if that zone
        // cannot be read back because the world has no game directory, or as resolve_target and load_saved_zone do
        flecs::entity relocate_through(flecs::entity actor, flecs::entity access_point);

        // Marks zone_entered as the most recently entered zone, then unloads the zones entered longest ago until the world
        // has no more than zone_count_max, saving each first if the world has a game directory. Never unloads
        // zone_entered
        void trim_zones(flecs::entity zone_entered);

        size_t zone_count_max() const;
        void set_zone_count_max(size_t zone_count_max);

        // The layout's queries, run from the entity's zone and placement. A wall is crossable or not as the entity
        // perceives what is on it. In move, every Blocking entity other than e blocks the way
        MoveResult move(flecs::entity e, const vector_type& direction, value_type distance) const;
        Layout::placements_type neighbors(flecs::entity e) const;
        bool visible(flecs::entity e, const placement_type& to) const;

        Layout& layout(flecs::entity zone);
        const Layout& layout(flecs::entity zone) const;

        // The entities in the zone whose sites have no address, by location
        const PlacementIndex& placement_index(flecs::entity zone) const;

        // Each in ascending id order, from the zone's placement index
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

        // Returns the zone's worldspace name and its name, joined by '/'
        std::string extended_name(flecs::entity zone) const;

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

        // Creates a zone from the document and optionally populates it
        // Throws std::runtime_error, reporting the source before anything is built, if check_buildable finds a reason the world
        // cannot build it, it is a saved zone that is already loaded, or its position is outside its worldspace's extent or is
        // another zone's position in its worldspace
        flecs::entity build_zone(PersistentId region_id, PersistentId zone_id, const stratum_format::Document& document, std::string_view source);

        // Builds the zone from the Document that a generator creates
        // Throws std::runtime_error if a name the selection depends on is not registered, or the document cannot be built, and passes on what the  generator throws
        flecs::entity generate_zone(PersistentId region_id, PersistentId zone_id, const ZoneSpec& zone_spec);

        // The file of a saved zone in the game directory
        std::filesystem::path saved_zone_path(PersistentId zone_id) const;

        Worldspace& create_worldspace(std::string_view name, const RegionSpec& region_spec, PersistentId worldspace_id, PersistentId region_id);

        // Returns the worldspace whose region tree has the region. Requires a region with that id in one of the worldspaces
        Worldspace& region_worldspace(PersistentId region_id);
        const Worldspace& region_worldspace(PersistentId region_id) const;

        // The position of a new region's seed. A placeholder until seeds are drawn hierarchically: x is one past the largest x
        // of the world's region specs, or 0 if it has none, and y is 0
        RegionPosition new_region_position() const;

        // The position of a new zone in the worldspace, at the given level. A placeholder until the world's layout is designed:
        // x and y are 0, and z is the level
        ZonePosition new_zone_position(PersistentId worldspace_id, level_type level) const;

        // Sets the zone's Name to one unique in its region, and lists it in its region by that name. The name is the zone's
        // Name, or else the region's default with the zone's coordinates, or else the region's name, with a suffix from
        // the region's counter if another zone of the region has it
        void name_zone(flecs::entity zone);

        // Returns the entry point of the zone made for the access point, of the arrival type and leading back offset levels.
        // Throws std::runtime_error, after discarding the zone, if it has none
        flecs::entity require_entry_point(flecs::entity access_point, flecs::entity zone, flecs::entity arrival_type, level_type offset);

        // Discards the zone made for the access point, which has never been entered, with its worldspace if that was made for
        // it too. Otherwise the access point's region and worldspace no longer list it. The world index no longer has its
        // entities
        void discard_made_zone(flecs::entity access_point, flecs::entity zone);

        // The entities that property reads, nearest first
        entities_type property_chain(flecs::entity e) const;

        // Returns true if the zone or the whole world is being deleted, in which case the entities deleted with it need no unlinking
        bool deleting_with(flecs::entity zone) const;

        IdAllocator& m_id_allocator;
        Generation& m_generation;

        // All entities in the world with a PersistentId
        std::unordered_map<PersistentId, flecs::entity> m_loaded_entities;

        // The zone that contains an entity
        std::unordered_map<PersistentId, PersistentId> m_entity_zones; // <entity_id, zone_id>

        // Zones that are being unloaded, whose entities remain in m_entity_zones
        std::unordered_set<PersistentId> m_zones_unloading;

        ZoneEntryIndex m_zone_entry_index;
        std::optional<std::filesystem::path> m_game_directory;
        RandomStream::seed_type m_world_seed;
        std::size_t m_worldspace_name_counter; // The last suffix given to a worldspace's name that was taken
        worldspaces_type m_worldspaces;

        flecs::world m_entities; // Last, as its teardown runs the observers that update the members above
    };

    //--------------------------------------------------------------------------
    // Returns the zone that the entity is in, directly or through its ancestors, or null
    flecs::entity zone_of(flecs::entity e);

    // Returns the id of the worldspace that the zone is in
    PersistentId worldspace_of(flecs::entity zone);

    // Returns the id of the region that the zone is in
    PersistentId region_of(flecs::entity zone);

    // Sets the zone's layout, and the empty placement index the layout creates
    void set_layout(flecs::entity zone, std::unique_ptr<Layout> layout);

    //--------------------------------------------------------------------------
    // Sets an entity's parent and site together
    void reparent(flecs::entity e, flecs::entity parent, const Site& site);

    // Sets only the parent for an entity that has no site
    void set_parent(flecs::entity e, flecs::entity parent);

    //--------------------------------------------------------------------------
    // Creates an entity type, which is a prefab. An entity IsA its type, and the type's components are the entity's defaults
    // The caller sets the type's components
    flecs::entity create_entity_type(flecs::world& entities, std::string_view name);

    // Returns a null entity if no type with that name is loaded
    flecs::entity entity_type(const flecs::world& entities, std::string_view name);

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
