#if !defined(QUETZAL_STRATUM_SIM_WORLD_HPP)
#define QUETZAL_STRATUM_SIM_WORLD_HPP
//------------------------------------------------------------------------------
// stratum_sim
// World.hpp
//------------------------------------------------------------------------------

#include "Layout.hpp"
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
    // Tag, an entity another cannot move into. Presence rather than identity, so a dropped
    // sword has a placement and no more, while a monster and a pushable boulder stop a mover
    // the same way
    struct Blocking
    {
    };

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

        // The zone takes the layout it was built with. Which class that is comes from what
        // was loaded rather than from here, so nothing about a Zone privileges one of them
        flecs::entity create_zone(const std::string& name, std::unique_ptr<Layout> layout);

        flecs::entity zone(const std::string& name) const; // Null when there is no such zone
        size_t zone_count() const;

        // The interface, whatever kind of layout the zone has. A caller needing more than
        // it exposes uses layout_as to get the concrete class
        Layout& layout(flecs::entity zone);
        const Layout& layout(flecs::entity zone) const;

        // Placed in the Zone, which owns it until it moves elsewhere
        flecs::entity create_entity(flecs::entity zone, const placement_type& placement);

        flecs::entity zone_of(flecs::entity e) const;

        // Where the mover ends up, topology and occupancy together. A layout resolves its
        // own geometry and is independent of entities; this owns both, so it is where the two
        // meet rather than either of them reaching into the other
        // Blocking entities stop a mover, itself excepted. Rules ask this rather than the
        // layout underneath
        MoveResult move(flecs::entity e, const vector_type& direction, value_type distance) const;

        // Occupancy. Ordered by entity id, so a run produces the same result every time
        // What counts as the same place is the layout's to decide, which is what keeps the
        // placement form model-neutral; the cell form is for a caller that genuinely counts
        // cells and whose layout has them
        entities_type entities_at(flecs::entity zone, const placement_type& placement) const;
        entities_type entities_at(flecs::entity zone, const GridPosition& cell) const;
        entities_type entities_within(flecs::entity zone, const placement_type& placement, value_type range) const;

        // Migration keeps the entity's id, so references to it survive
        void relocate(flecs::entity e, flecs::entity zone, const placement_type& placement);

        // Discards a Zone, its contents, and its layout. Writing that state out first is
        // the persistence boundary's job, not this one's
        void unload_zone(flecs::entity zone);

    private:

        flecs::world m_world;
        std::map<flecs::entity_t, std::unique_ptr<Layout>> m_layouts;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_WORLD_HPP
