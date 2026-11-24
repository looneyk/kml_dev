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
#include "quetzal/stratum_core/Zone.hpp"
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
        using zone_ids_type = std::map<discrete_point3_type, PersistentId>; // <position, zone_id>

        World(IdAllocator& id_allocator, Generation& generation, std::size_t zone_count_max, const ScriptLimits& script_limits);
        World(const World&) = delete;
        World(World&&) = delete;
        ~World() = default;

        World& operator=(const World&) = delete;
        World& operator=(World&&) = delete;

        void clear();

        // The seed that region and zone seeds are derived from
        seed_type world_seed() const;
        void set_world_seed(seed_type seed);

        // Creates the starting worldspace and returns the zone at start_zone, which is center_start or a zone position x,y,z
        // Throws std::runtime_error if the starting region spec tree is not registered, start_zone is neither, or the worldspace
        // has no zone there
        flecs::entity create(std::string_view start_zone, const std::filesystem::path& game_directory);

        // Requires an empty world
        // Throws std::runtime_error if the file cannot be read
        void open(const std::filesystem::path& game_directory);

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

        // Creates a worldspace with a region for each region spec and an empty zone at each position of the root spec's extent
        // Throws std::runtime_error, creating nothing, if a spec or a subregion's extent is not valid
        PersistentId create_worldspace(const WorldDefinition::region_spec_tree_type& tree, seed_type seed);

        // Removes the worldspace, its regions, and its zones, without saving
        void unload_worldspace(PersistentId worldspace_id);

        // Creates an empty zone
        flecs::entity create_zone(PersistentId region_id, PersistentId zone_id);

        flecs::entity create_zone(PersistentId region_id, std::unique_ptr<LayoutBase> layout);

        // Throws std::runtime_error, building nothing, if the file cannot be read or built, or is a saved zone that is already loaded
        flecs::entity load_zone(PersistentId region_id, const std::filesystem::path& filepath);

        // Fills the empty zone at the position from a zone template file instantiated with zone_spec. The zone keeps its id and name
        // Throws std::runtime_error if the file cannot be read or built or is a saved zone, or the position has no empty zone
        flecs::entity load_template(PersistentId worldspace_id, const discrete_point3_type& position, const std::filesystem::path& filepath,
            const ZoneSpec& zone_spec);

        // Returns the zone at the position, read back if it has unloaded and generated if it is empty
        // Throws std::runtime_error if the worldspace has no zone at the position or the zone cannot be read back
        flecs::entity load_zone_at(PersistentId worldspace_id, const discrete_point3_type& position);

        // Reads the zone back from the game directory
        // Throws std::runtime_error if the world has no game directory, the zone is in no region, or the file cannot be read or built
        flecs::entity load_saved_zone(PersistentId zone_id);

        // Discards the zone and its contents without saving
        void unload_zone(flecs::entity zone);

        // Places the zone's denizens
        void populate(flecs::entity zone);

        // An entity with no parent stays without one, in the zone through InZone, and any other entity becomes a child of the zone
        void relocate(flecs::entity e, flecs::entity zone, const placement_type& placement);

        // Returns the access point that the access point's unbound destination leads to
        // Throws std::runtime_error if the destination's region spec is not registered or its zone has no matching access point
        flecs::entity resolve_destination(flecs::entity access_point);

        // Moves the actor through the access point and returns the access point arrived at. An unbound destination becomes
        // bound, and the access point arrived at is bound back unless it is arrival only
        // Throws std::runtime_error, moving nothing, if the destination cannot be resolved or its zone cannot be read back
        flecs::entity relocate_through(flecs::entity actor, flecs::entity access_point);

        // Marks zone_entered as the most recently entered zone, then unloads the zones entered longest ago, saving each if the
        // world has a game directory, until no more than zone_count_max are not empty. A zone_count_max of 0 disables it
        void trim_zones(flecs::entity zone_entered);

        std::size_t zone_count_max() const;
        void set_zone_count_max(std::size_t zone_count_max);

        // The layout's queries from the entity's zone and placement, with walls as the entity perceives them. Every Blocking
        // entity other than e blocks a move
        MoveResult move(flecs::entity e, const vector_type& direction, value_type distance) const;
        LayoutBase::placements_type neighbors(flecs::entity e) const;
        bool visible(flecs::entity e, const placement_type& to) const;

        LayoutBase& layout(flecs::entity zone);
        const LayoutBase& layout(flecs::entity zone) const;

        // The entities in the zone whose sites have no address, by location
        const ProximityIndexBase& proximity_index(flecs::entity zone) const;

        // The entities in ascending id order
        entities_type entities_at(flecs::entity zone, const placement_type& placement) const;
        entities_type entities_at(flecs::entity zone, const discrete_point2_type& cell_position) const;
        entities_type entities_within(flecs::entity zone, const placement_type& placement, value_type distance) const;

        flecs::entity create_entity(flecs::entity zone, const placement_type& placement);

        // Returns the component merged along the chain of the entity, its type, its room, and the layout, with each filling
        // the optional fields that nearer ones left unset. A layout entity's chain has no type
        template<typename T>
        std::optional<T> property(flecs::entity e) const;

        // Returns a null entity if the entity is not loaded
        flecs::entity entity(PersistentId persistent_id) const;

        // Returns the entity's zone whether or not the entity is loaded
        PersistentId zone_of(PersistentId persistent_id) const;

        // Adds the entity to the world index
        EntityReference make_reference(flecs::entity target);

        // Returns a null entity if the entity is not loaded, and updates the reference's handle if the entity has been reloaded
        flecs::entity resolve(EntityReference& reference) const;

        PersistentId next_id();

    private:

        // Throws std::runtime_error, building nothing, if the world cannot build the document, it is a saved zone that is
        // already loaded, or its position is outside the worldspace or another zone's
        flecs::entity build_zone(PersistentId region_id, PersistentId zone_id, const stratum_format::ZoneDocument& document, std::string_view source);

        // Returns the region spec's zone. keys with the empty zone's position and seed and the patterns resolved. zone_counter
        // is the zone's ordinal in the worldspace, in position order
        // Throws std::runtime_error if a pattern fails
        ZoneSpec resolved_zone_spec(PersistentId region_id, const ZoneSpec& empty_zone_spec, std::size_t zone_counter) const;

        // Creates an empty zone with a placeholder name
        // Throws std::runtime_error if the position is outside the worldspace or another zone's
        flecs::entity create_zone(PersistentId region_id, const discrete_point3_type& position, std::size_t zone_counter, seed_type seed);

        // Throws std::runtime_error if a subregion's extent is missing, outside its parent's, or overlaps a sibling's
        void create_regions(PersistentId worldspace_id, const WorldDefinition::region_spec_tree_type& resolved_tree);

        // Replaces the empty zone's spec and placeholder name with those resolved from its region's spec, and adds the zone
        void set_zone_spec(flecs::entity zone, std::size_t zone_counter);

        // Returns the zone at the position, read back if it has unloaded
        // Throws std::runtime_error if the worldspace has no zone at the position or the zone cannot be read back
        flecs::entity zone_at(PersistentId worldspace_id, const discrete_point3_type& position);

        // The zone keeps its position and name
        // Throws std::runtime_error, building nothing, if the world cannot build the document
        void fill_zone(flecs::entity zone, const stratum_format::ZoneDocument& document, std::string_view source);

        // Creates the worldspace for each of the zone's access points whose destination is a region spec name
        // Throws std::runtime_error if a region spec is not registered
        void create_destination_worldspaces(flecs::entity zone);

        // Returns the worldspace named for the access point's destination region spec, creating it if the world has none
        // Throws std::runtime_error if the region spec is not registered
        PersistentId destination_worldspace(flecs::entity access_point);

        // Throws std::runtime_error if the root region spec has no entry zone
        flecs::entity load_entry_zone(PersistentId worldspace_id);

        // Throws std::runtime_error if the access point's destination is not a zone offset
        flecs::entity load_relative_zone(flecs::entity access_point);

        // Temporary until a zone's seed pool is drawn
        seed_type destination_seed(flecs::entity access_point) const;

        // Returns the zone generator's document for the spec, marked unpopulated
        stratum_format::ZoneDocument generate_zone(const ZoneSpec& zone_spec);

        std::filesystem::path saved_zone_path(PersistentId zone_id) const;

        Worldspace& create_worldspace(std::string_view name, seed_type seed, const RegionSpec& region_spec, PersistentId worldspace_id, PersistentId region_id);

        Worldspace& region_worldspace(PersistentId region_id);
        const Worldspace& region_worldspace(PersistentId region_id) const;

        // Sets a missing Name to the region's name, followed by the zone's coordinates if it has a position
        void name_zone(flecs::entity zone);

        // Adds or replaces the zone in the world's zones, and adds its id to the region's zone_ids
        void add_zone(PersistentId region_id, stratum_core::Zone zone);

        // Replaces the root region spec's entry_zone name with the id of the zone that has the name
        // Throws std::runtime_error unless exactly one zone has the name
        void set_entry_zone_id(Worldspace& worldspace, const entities_type& zones);

        entities_type property_chain(flecs::entity e) const;

        // Returns true if the zone or the world is being deleted
        bool deleting_with(flecs::entity zone) const;

        IdAllocator& m_id_allocator;
        Generation& m_generation;

        std::unordered_map<PersistentId, flecs::entity> m_loaded_entities;
        std::unordered_map<PersistentId, PersistentId> m_entity_zones; // <entity_id, zone_id>

        std::unordered_set<PersistentId> m_zones_unloading;

        ZoneEntryIndex m_zone_entry_index;
        ScriptLimits m_script_limits;
        std::optional<std::filesystem::path> m_game_directory;
        seed_type m_world_seed;
        std::size_t m_worldspace_count;
        worldspaces_type m_worldspaces;
        std::map<PersistentId, stratum_core::Zone> m_zones; // <zone_id, zone>
        std::unordered_map<PersistentId, zone_ids_type> m_worldspace_zone_ids; // <worldspace_id, zone_ids>

        flecs::world m_entities; // Last, as its teardown runs the observers that update the members above
    };

    //--------------------------------------------------------------------------
    // Returns the zone that the entity or an ancestor is in, or a null entity
    flecs::entity zone_of(flecs::entity e);

    PersistentId worldspace_of(flecs::entity zone);
    PersistentId region_of(flecs::entity zone);

    // Returns true if the zone's content has not been generated or loaded
    bool empty_zone(flecs::entity zone);

    // The start setting value for the center of the starting worldspace's extent or of the starting zone's layout
    inline constexpr std::string_view center_start = "center";

    // Returns the placement of an actor facing -y at start_cell, which is a cell position i,j or center_start for the
    // occupiable cell nearest the center of the zone's layout
    // Throws std::runtime_error if start_cell is neither or an actor cannot occupy the cell
    placement_type start_placement(flecs::entity zone, std::string_view start_cell);

    // Sets the zone's layout and the layout's empty proximity index
    void set_layout(flecs::entity zone, std::unique_ptr<LayoutBase> layout);

    //--------------------------------------------------------------------------
    void reparent(flecs::entity e, flecs::entity parent, const Site& site);

    // Sets the parent of an entity that has no site
    void set_parent(flecs::entity e, flecs::entity parent);

    // Returns a handle to e that uses the world, for an e from an observer whose stage the handle would otherwise outlive
    flecs::entity world_handle(flecs::entity e);

    //--------------------------------------------------------------------------
    // Creates an entity type, a prefab whose components are the defaults of the entities that are IsA it
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
