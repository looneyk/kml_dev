#if !defined(QUETZAL_STRATUM_SIM_WORLD_HPP)
#define QUETZAL_STRATUM_SIM_WORLD_HPP
//------------------------------------------------------------------------------
// stratum_sim
// World.hpp
//------------------------------------------------------------------------------

#include "Generation.hpp"
#include "IdAllocator.hpp"
#include "LayoutBase.hpp"
#include "EntityReference.hpp"
#include "properties.hpp"
#include "RandomStream.hpp"
#include "ScriptLimits.hpp"
#include "Site.hpp"
#include "Zone.hpp"
#include "ZoneEntryIndex.hpp"
#include "WorldDefinition.hpp"
#include "types.hpp"
#include "quetzal/stratum_core/Region.hpp"
#include "quetzal/stratum_core/RegionTree.hpp"
#include "quetzal/stratum_core/Worldspace.hpp"
#include <flecs.h>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace quetzal::stratum_sim
{

    using stratum_core::Region;
    using stratum_core::RegionTree;
    using stratum_core::Worldspace;

    //--------------------------------------------------------------------------
    class World
    {
    public:

        using entities_type = std::vector<flecs::entity>;
        using worldspaces_type = std::vector<Worldspace>;
        using zone_ids_type = std::map<zone_position_type, PersistentId>; // <position, zone_id>

        World(IdAllocator& id_allocator, Generation& generation, std::size_t zone_count_max, const ScriptLimits& script_limits);
        World(const World&) = delete;
        World(World&&) = delete;
        ~World() = default;

        World& operator=(const World&) = delete;
        World& operator=(World&&) = delete;

        void clear();

        // The seed region and zone seeds derive from
        seed_type world_seed() const;
        void set_world_seed(seed_type seed);

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

        // Returns the ids of the worldspace's zones that have a position
        const zone_ids_type& zone_ids(PersistentId worldspace_id) const;

        // Creates a worldspace from the region spec tree with the extent from its root spec, an empty zone at each
        // position of its extent, in worldspace position order, a region for each region spec, and each zone's zone spec
        // A random engine seeded from the seed draws each zone's seed in worldspace position order, and then each region
        // spec's seed in tree order
        // Throws std::runtime_error, creating nothing, if the worldspace keys are not valid, a spec is based on a region spec
        // that is not registered, a pattern fails, a subregion spec has no extent, or a subregion's extent reaches outside the
        // worldspace, is outside its parent's extent, or overlaps a sibling's
        PersistentId create_worldspace(const WorldDefinition::region_spec_tree_type& tree, seed_type seed);

        // Unloads the worldspace's zones, discarding them without saving, and removes the worldspace and its regions
        void unload_worldspace(PersistentId worldspace_id);

        // Create an empty zone in the region
        flecs::entity create_zone(PersistentId region_id, PersistentId zone_id);

        // Create a zone in the region with the given layout
        flecs::entity create_zone(PersistentId region_id, std::unique_ptr<LayoutBase> layout);

        // Read a zone into the region from a file and optionally populate it.
        // Throws std::runtime_error, before anything is built, if the reader rejects the file, check_buildable finds a
        // reason the world cannot build it, or the file is a saved zone that is already loaded
        flecs::entity load_zone(PersistentId region_id, const std::filesystem::path& filepath);

        // Fills the empty zone at the position in the worldspace from a zone template file and optionally populates it. The
        // zone keeps its persistent id and its name. zone_spec is the spec the template is instantiated with
        // Throws std::runtime_error if the reader rejects the file, the file is a saved zone, the worldspace has no zone at
        // the position, that zone cannot be read back or is not empty, or check_buildable finds a reason the world cannot
        // build it
        flecs::entity load_template(PersistentId worldspace_id, const zone_position_type& position, const std::filesystem::path& filepath,
            const ZoneSpec& zone_spec);

        // Returns the zone at the position in the worldspace, read back if it has unloaded, with its content generated if it
        // is empty. Throws std::runtime_error if the worldspace has no zone at the position or that zone cannot be read back,
        // and passes on what the zone generator throws
        flecs::entity load_zone_at(PersistentId worldspace_id, const zone_position_type& position);

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

        // Returns the access point at the terminal end of an access point transit
        // Throws std::runtime_error if the region spec is not registered, as those functions do, or if the zone has no such entry point
        flecs::entity resolve_destination(flecs::entity access_point);

        // Moves the actor through the access point to the destination placement of the access point its destination refers to,
        // and returns that access point. An unbound destination is resolved by resolve_destination, and the access point gone through is
        // then bound to the one arrived at, which is bound back to it unless it is arrival only, so going back arrives at
        // the access point gone through. A bound destination's access point in a
        // zone that has unloaded is read back with its zone. Throws std::runtime_error, and moves nothing, if that zone
        // cannot be read back because the world has no game directory, or as resolve_destination and load_saved_zone do
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
        LayoutBase::placements_type neighbors(flecs::entity e) const;
        bool visible(flecs::entity e, const placement_type& to) const;

        LayoutBase& layout(flecs::entity zone);
        const LayoutBase& layout(flecs::entity zone) const;

        // The entities in the zone whose sites have no address, by location
        const ProximityIndexBase& proximity_index(flecs::entity zone) const;

        // Each in ascending id order, from the zone's proximity index
        entities_type entities_at(flecs::entity zone, const placement_type& placement) const;
        entities_type entities_at(flecs::entity zone, const discrete_point_type& cell_position) const;
        entities_type entities_within(flecs::entity zone, const placement_type& placement, value_type distance) const;

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

        // Creates a zone from the document and optionally populates it
        // Throws std::runtime_error, reporting the source before anything is built, if check_buildable finds a reason the world
        // cannot build it, it is a saved zone that is already loaded, or its position is outside its worldspace's extent or is
        // another zone's position in its worldspace
        flecs::entity build_zone(PersistentId region_id, PersistentId zone_id, const stratum_format::ZoneDocument& document, std::string_view source);

        // Returns the spec of a zone in the region: the region spec's zone. keys, with a default name, the position of the
        // empty zone's spec, and its seed unless the zone. keys have one, its patterns resolved with the zone's symbols. The
        // zone counter counts the zones of the worldspace from 0 in worldspace position order
        // Throws std::runtime_error for a pattern that does not compile, fails, or has a table or a function as its value
        ZoneSpec resolved_zone_spec(PersistentId region_id, const ZoneSpec& empty_zone_spec, std::size_t zone_counter) const;

        // Create an empty zone with only a position, seed, and a placeholder name
        // Throws std::runtime_error if the position is outside the worldspace's extent or is another zone's position in it
        flecs::entity create_zone(PersistentId region_id, const zone_position_type& position, std::size_t zone_counter, seed_type seed);

        // Create a region tree from the region spec tree
        // Throws std::runtime_error if a subregion spec has no extent, or a subregion's extent reaches outside the worldspace,
        // is outside its parent's extent, or overlaps a sibling's
        void create_regions(PersistentId worldspace_id, const WorldDefinition::region_spec_tree_type& resolved_tree);

        // Replace the empty zone's spec and its placeholder name with those resolved from its region's spec, and add it to its region
        void set_zone_spec(flecs::entity zone, std::size_t zone_counter);

        // Returns the zone at the position in the worldspace, read back if it has unloaded
        // Throws std::runtime_error if the worldspace has no zone at the position or that zone cannot be read back
        flecs::entity zone_at(PersistentId worldspace_id, const zone_position_type& position);

        // Fills the empty zone from the document and optionally populates it. The zone keeps its position and its name
        // Throws std::runtime_error, reporting the source before anything is built, if check_buildable finds a reason the world
        // cannot build it
        void fill_zone(flecs::entity zone, const stratum_format::ZoneDocument& document, std::string_view source);

        // Creates the destination worldspace of each of the zone's access points whose unbound destination has a region spec name
        // Throws std::runtime_error if a region spec is not registered
        void create_destination_worldspaces(flecs::entity zone);

        // Returns the id of the worldspace named for the region spec of the access point's unbound destination, created from
        // the registered region spec if the world has none
        // Throws std::runtime_error if it is created and the region spec is not registered
        PersistentId destination_worldspace(flecs::entity access_point);

        // Returns the worldspace's entry zone, from the worldspace's root region spec
        // Throws std::runtime_error if the root region spec has no entry_zone or the worldspace has no zone of that name
        flecs::entity load_entry_zone(PersistentId worldspace_id);

        // Returns the zone of an access point's destination
        // Throws std::runtime_error if the destination is not a relative zone level
        flecs::entity load_relative_zone(flecs::entity access_point);

        // Temporary, until a zone's seed pool is drawn: returns the seed of a worldspace created for the access point,
        // derived from the world seed and the access point's persistent id
        seed_type destination_seed(flecs::entity access_point) const;

        // Returns the ZoneDocument that the game's zone generator creates for the resolved spec, unpopulated, and passes on what
        // the zone generator throws
        stratum_format::ZoneDocument generate_zone(const ZoneSpec& zone_spec);

        // The file of a saved zone in the game directory
        std::filesystem::path saved_zone_path(PersistentId zone_id) const;

        Worldspace& create_worldspace(std::string_view name, seed_type seed, const RegionSpec& region_spec, PersistentId worldspace_id,
            PersistentId region_id);

        // Returns the worldspace whose region tree has the region. Requires a region with that id in one of the worldspaces
        Worldspace& region_worldspace(PersistentId region_id);
        const Worldspace& region_worldspace(PersistentId region_id) const;

        // The position of a new zone in the worldspace, at the given level. A placeholder until the world's layout is designed:
        // x and y are 0, and z is the level
        zone_position_type new_zone_position(PersistentId worldspace_id, level_type level) const;

        // Sets the Name of a zone that has none to the region's name with the zone's coordinates, or to the region's name for
        // a zone without a position
        void name_zone(flecs::entity zone);

        // Replaces the root region spec's entry_zone with entry_zone_id, the id of the zone of that name
        // Throws std::runtime_error unless exactly one of the zones has the name
        void set_entry_zone_id(Worldspace& worldspace, const entities_type& zones);

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
        ScriptLimits m_script_limits;
        std::optional<std::filesystem::path> m_game_directory;
        seed_type m_world_seed;
        std::size_t m_worldspace_count;
        worldspaces_type m_worldspaces;
        std::unordered_map<PersistentId, zone_ids_type> m_worldspace_zone_ids; // <worldspace_id, zone_ids>

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

    // Sets the zone's layout, and the empty proximity index the layout creates
    void set_layout(flecs::entity zone, std::unique_ptr<LayoutBase> layout);

    //--------------------------------------------------------------------------
    // Sets an entity's parent and site together
    void reparent(flecs::entity e, flecs::entity parent, const Site& site);

    // Sets only the parent for an entity that has no site
    void set_parent(flecs::entity e, flecs::entity parent);

    // An observer is given handles that use its stage, so a handle that outlasts the observer must be converted to one that uses the world
    // Returns that handle
    flecs::entity world_handle(flecs::entity e);

    //--------------------------------------------------------------------------
    // Creates an entity type, which is a prefab. An entity IsA its type, and the type's components are the entity's defaults
    // The caller sets the type's components
    flecs::entity create_entity_type(flecs::world& entities, std::string_view name);

    // Returns a null entity if no type with that name is loaded
    flecs::entity entity_type(const flecs::world& entities, std::string_view name);

    std::string entity_type_name(flecs::entity e);

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
