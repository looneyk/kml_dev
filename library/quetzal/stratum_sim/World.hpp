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
#include "Region.hpp"
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
    class World
    {
    public:

        using entities_type = std::vector<flecs::entity>;
        using worldspaces_type = std::vector<Worldspace>;

        World(IdAllocator& id_allocator, Generation& generation, std::size_t zone_count_max);
        World(const World&) = delete;
        World(World&&) = delete;
        ~World() = default;

        World& operator=(const World&) = delete;
        World& operator=(World&&) = delete;

        void clear();

        // The seed region and zone seeds derive from
        RandomStream::seed_type world_seed() const;
        void set_world_seed(RandomStream::seed_type seed);

        // Creates a new game's starting worldspace from the world definition's starting region
        // Returns the starting zone
        // Throws std::runtime_error if the starting region is not a tree's root, its spec is based on a region spec that
        // is not registered, start_zone is neither, or the worldspace has no zone there, and passes on what the zone
        // generator throws
        flecs::entity create(std::string_view start_zone, const std::filesystem::path& game_directory);

        // Read world.json from the game directory, which zones are saved into until clear: each worldspace, each region, and its
        // zones, and the world index. Loads no zone. Requires a cleared world
        // Throws std::runtime_error if the file cannot be read
        void open(const std::filesystem::path& game_directory);

        // Write every loaded zone, then world.json, with each worldspace, its regions, their zones, and the world index into the game directory
        void save();

        void save_zone(flecs::entity zone);

        flecs::world& entities();
        const flecs::world& entities() const;

        // Loaded zones in id order
        entities_type zones() const;
        size_t zone_count() const;

        const worldspaces_type& worldspaces() const;

        Worldspace& worldspace(PersistentId worldspace_id);
        const Worldspace& worldspace(PersistentId worldspace_id) const;

        Region& region(PersistentId region_id);
        const Region& region(PersistentId region_id) const;

        // Creates a worldspace with the coordinate system from the region spec, its top-level region from the spec, and an
        // empty zone at each position of its extent, in worldspace position order
        // Throws std::runtime_error if the worldspace keys are not valid
        PersistentId create_worldspace(std::string_view name, const RegionSpec& region_spec);

        // Unloads the worldspace's zones, discarding them without saving, and removes the worldspace and its regions
        void unload_worldspace(PersistentId worldspace_id);

        // Create an empty zone in the region
        flecs::entity create_zone(PersistentId region_id, PersistentId zone_id);

        // Create a zone in the region with the given layout
        flecs::entity create_zone(PersistentId region_id, std::unique_ptr<Layout> layout);

        // Read a zone into the region from a file and optionally populate it.
        // Throws std::runtime_error, before anything is built, if the reader rejects the file, check_buildable finds a
        // reason the world cannot build it, or the file is a saved zone that is already loaded
        flecs::entity load_zone(PersistentId region_id, const std::filesystem::path& filepath);

        // Fills the empty zone at the position in the worldspace from a zone template file and optionally populates it. The
        // zone keeps its persistent id and its name. zone_spec is the spec the template is instantiated with
        // Throws std::runtime_error if the reader rejects the file, the file is a saved zone, the worldspace has no zone at
        // the position, that zone cannot be read back or is not empty, or check_buildable finds a reason the world cannot
        // build it
        flecs::entity load_template(PersistentId worldspace_id, const ZonePosition& position, const std::filesystem::path& filepath,
            const ZoneSpec& zone_spec);

        // Returns the zone at the position in the worldspace, read back if it has unloaded, with its content generated if it
        // is empty. Throws std::runtime_error if the worldspace has no zone at the position or that zone cannot be read back,
        // and passes on what the zone generator throws
        flecs::entity load_zone_at(PersistentId worldspace_id, const ZonePosition& position);

        // Read a zone that has been unvisited or unloaded from the game directory
        // Throws std::runtime_error if the world has no game directory, no region lists the zone, or as load_zone does
        flecs::entity load_saved_zone(PersistentId zone_id);

        // Discards the zone, its contents and its layout, without saving
        void unload_zone(flecs::entity zone);

        // Places the zone's denizens
        void populate(flecs::entity zone);

        // Relocation keeps the entity's id, so references to it remain valid. An entity with no parent is owned by the game
        // and stays that way, located in the zone through InZone. Any other entity becomes a child of the zone
        void relocate(flecs::entity e, flecs::entity zone, const placement_type& placement);

        // Returns the access point at which an actor arrives through the access point, whose unbound target is a relative or
        // absolute level with no position. It is the entry point of the arrival type that leads back, in the zone at the
        // target level's position, read back if it has unloaded and with its content generated if it is empty. That zone is in
        // the access point's worldspace if the region is omitted, in the worldspace of the region's name if the access point
        // has a region spec, created from that spec if there is none, and otherwise in a new worldspace created from the region
        // spec of that name. Throws std::runtime_error if the target has a named zone or a position, if that region spec is
        // not registered, if the worldspace has no zone at the position, if that zone cannot be read back, or if it has no
        // such entry point, after discarding a new worldspace
        flecs::entity resolve_target(flecs::entity access_point);

        // Moves the actor through the access point to the arrival placement of the access point its target refers to, and
        // returns that access point. An unbound target is resolved by resolve_target, and the access point gone through is
        // then bound to the one arrived at, which is bound back to it unless it is arrival only, so going back arrives at
        // the access point gone through. A bound target's access point in a
        // zone that has unloaded is read back with its zone. Throws std::runtime_error, and moves nothing, if that zone
        // cannot be read back because the world has no game directory, or as resolve_target and load_saved_zone do
        flecs::entity relocate_through(flecs::entity actor, flecs::entity access_point);

        // Marks zone_entered as the most recently entered zone, then unloads the zones entered longest ago until the world
        // has no more than zone_count_max zones that are not empty, saving each first if the world has a game directory.
        // Never unloads zone_entered, and does nothing if zone_count_max is 0
        void trim_zones(flecs::entity zone_entered);

        std::size_t zone_count_max() const;
        void set_zone_count_max(std::size_t zone_count_max);

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

        // Returns the spec of the zone at the position in the region: the region spec's zone. keys, with a default name, the
        // zone's position, and its seed, its patterns resolved with the zone's symbols. The zone counter counts the zones
        // from 0 in the order in which they are created
        // Throws std::runtime_error for a pattern that is not a valid expression given the zone's symbols
        ZoneSpec resolved_zone_spec(PersistentId region_id, const ZonePosition& position, std::size_t zone_counter) const;

        // Creates an empty zone at the position in the region, with its resolved spec
        // Throws std::runtime_error if the position is outside the worldspace's extent or is another zone's position in it
        flecs::entity create_zone(PersistentId region_id, const ZonePosition& position, std::size_t zone_counter);

        // Returns the zone at the position in the worldspace, read back if it has unloaded
        // Throws std::runtime_error if the worldspace has no zone at the position or that zone cannot be read back
        flecs::entity zone_at(PersistentId worldspace_id, const ZonePosition& position);

        // Fills the empty zone from the document and optionally populates it. The zone keeps its position and its name
        // Throws std::runtime_error, reporting the source before anything is built, if check_buildable finds a reason the world
        // cannot build it
        void fill_zone(flecs::entity zone, const stratum_format::Document& document, std::string_view source);

        // Creates, for each of the zone's access points that has a region spec, the worldspace its target's region refers to,
        // from that region spec, if the world has no worldspace of that name
        // Throws std::runtime_error if such an access point's target is not unbound with a region, or the region spec is not
        // registered
        void create_target_worldspaces(flecs::entity zone);

        // Returns the region of the access point's unbound target
        // Throws std::runtime_error if the target is not unbound with a region
        std::string target_worldspace_name(flecs::entity access_point) const;

        // Returns the id of the worldspace of that name, created from the registered region spec if the world has none
        // Throws std::runtime_error if it is created and the region spec is not registered
        PersistentId target_worldspace(std::string_view name, std::string_view region_spec_name);

        // Returns the Document that the game's zone generator creates for the resolved spec, unpopulated, and passes on what
        // the zone generator throws
        stratum_format::Document generate_zone(const ZoneSpec& zone_spec);

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

        // Returns the entry point of the zone, in a worldspace made for the access point, of the arrival type and leading back
        // offset levels. Throws std::runtime_error, after discarding the worldspace, if it has none
        flecs::entity require_entry_point(flecs::entity access_point, flecs::entity zone, flecs::entity arrival_type, level_type offset);

        // Discards the worldspace of the zone made for the access point, which has never been entered. The world index no
        // longer has the zone's entities
        void discard_made_zone(flecs::entity zone);

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

    // Returns true if the zone's content has not been generated or loaded
    bool empty_zone(flecs::entity zone);

    // The start setting value for the center of the starting worldspace's extent or of the starting zone's layout
    inline constexpr std::string_view center_start = "center";

    // Returns the placement of an actor starting in the zone at start_cell, facing -y: center_start for the cell nearest
    // the center of the zone's layout that an actor can occupy, the first in row order of those as near, or a cell
    // position written i,j
    // Throws std::runtime_error if start_cell is neither, or the zone has no such cell an actor can occupy
    placement_type start_placement(flecs::entity zone, std::string_view start_cell);

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
