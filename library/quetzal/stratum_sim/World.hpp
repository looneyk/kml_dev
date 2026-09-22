#if !defined(QUETZAL_STRATUM_SIM_WORLD_HPP)
#define QUETZAL_STRATUM_SIM_WORLD_HPP
//------------------------------------------------------------------------------
// stratum_sim
// World.hpp
//------------------------------------------------------------------------------

#include "Layout.hpp"
#include "Site.hpp"
#include "types.hpp"
#include <flecs.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    struct Zone // Tag, an independently loadable area
    {
    };

    //--------------------------------------------------------------------------
    // Tag, an entity another cannot move into.
    struct Blocking
    {
    };

    //--------------------------------------------------------------------------
    // The zone an entity is in, found along its parents, and a null entity where it is in none.
    // Depth is unbounded -- coins in a pouch an actor carries -- so the zone is not its parent
    flecs::entity zone_of(flecs::entity e);

    //--------------------------------------------------------------------------
    // What an entity belongs to and where it is, set together: taking it, putting it down and
    // moving it between zones are all this operation, at different parents. Its own children
    // travel with it, being its subtree, and its id is retained, so references to it stay valid
    void reparent(flecs::entity e, flecs::entity parent, const Site& site);

    //--------------------------------------------------------------------------
    // The set of Zones and everything in them. One flecs world holds the entities, each
    // Zone an entity whose contents are its children; the layouts are held alongside
    // rather than in it, being static topology rather than game objects
    class World
    {
    public:

        using entities_type = std::vector<flecs::entity>;

        World() = default;
        World(const World&) = delete;
        World(World&&) noexcept = default;
        ~World() = default;

        World& operator=(const World&) = delete;
        World& operator=(World&&) noexcept = default;

        flecs::world& entities();
        const flecs::world& entities() const;

        // Create a zone from a saved layout
        flecs::entity create_zone(const std::string& name, std::unique_ptr<Layout> layout);

        flecs::entity zone(const std::string& name) const; // Null when there is no such zone
        size_t zone_count() const;

        // The Layout interface for a zone entity
        Layout& layout(flecs::entity zone);
        const Layout& layout(flecs::entity zone) const;

        flecs::entity create_entity(flecs::entity zone, const placement_type& placement);

        // Where the mover ends up, topology and occupancy together. A layout resolves its
        // own geometry and is independent of entities; this owns both, so it is where the two
        // meet rather than either of them reaching into the other
        // Blocking entities stop a mover, itself excepted. Rules ask this rather than the
        // layout underneath
        MoveResult move(flecs::entity e, const vector_type& direction, value_type distance) const;

        // The layout's queries that depend on what is attached to a wall, asked of the entity's own
        // zone and placement. Rules ask these rather than the layout, which has no way to reach an
        // entity and so would take every wall as solid
        Layout::placements_type neighbors(flecs::entity e) const;
        bool visible(flecs::entity e, const placement_type& to) const;

        entities_type entities_at(flecs::entity zone, const placement_type& placement) const;
        entities_type entities_at(flecs::entity zone, const discrete_point_type& cell_position) const;
        entities_type entities_within(flecs::entity zone, const placement_type& placement, value_type range) const;

        // Relocation retains the entity's id, so references to it remain valid
        void relocate(flecs::entity e, flecs::entity zone, const placement_type& placement);

        void unload_zone(flecs::entity zone);

    private:

        flecs::world m_world;
        std::map<flecs::entity_t, std::unique_ptr<Layout>> m_layouts;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_WORLD_HPP
