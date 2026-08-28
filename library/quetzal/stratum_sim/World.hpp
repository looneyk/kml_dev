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
#include <string>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    struct Zone // Tag, an independently loadable area
    {
    };

    //--------------------------------------------------------------------------
    // The set of Zones and everything in them. One flecs world holds the entities, each
    // Zone an entity whose contents are its children; the Layouts are held alongside
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

        flecs::entity create_zone(const std::string& name);
        flecs::entity zone(const std::string& name) const; // Null when there is no such zone
        size_t zone_count() const;

        Layout& layout(flecs::entity zone);
        const Layout& layout(flecs::entity zone) const;

        // Placed in the Zone, which owns it until it moves elsewhere
        flecs::entity create_entity(flecs::entity zone, const placement_type& placement);

        flecs::entity zone_of(flecs::entity e) const;

        // Occupancy. Ordered by entity id, so a run gives the same answer every time
        entities_type entities_at(flecs::entity zone, const Cell& cell) const;
        entities_type entities_within(flecs::entity zone, const placement_type& placement, value_type range) const;

        // Migration keeps the entity's id, so references to it survive
        void relocate(flecs::entity e, flecs::entity zone, const placement_type& placement);

        // Discards a Zone, its contents, and its Layout. Writing that state out first is
        // the persistence boundary's job, not this one's
        void unload_zone(flecs::entity zone);

    private:

        flecs::world m_world;
        std::map<flecs::entity_t, Layout> m_layouts;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_WORLD_HPP
