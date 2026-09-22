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
    // The nearest ancestor that is a zone, or a null entity. Entities nest, as coins in a pouch an actor carries do
    flecs::entity zone_of(flecs::entity e);

    //--------------------------------------------------------------------------
    // Sets an entity's parent and site together. Its children move with it and its id is kept, so references to it stay
    // valid
    void reparent(flecs::entity e, flecs::entity parent, const Site& site);

    //--------------------------------------------------------------------------
    // The zones and everything in them. Entities are in one flecs world, a zone's contents being its children; each zone's
    // layout is stored outside flecs
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

        flecs::entity create_zone(const std::string& name, std::unique_ptr<Layout> layout);

        flecs::entity zone(const std::string& name) const; // Null when there is no such zone
        size_t zone_count() const;

        Layout& layout(flecs::entity zone);
        const Layout& layout(flecs::entity zone) const;

        flecs::entity create_entity(flecs::entity zone, const placement_type& placement);

        // The layout's queries from the entity's zone and placement, each wall crossable as its attachments allow. In move,
        // every Blocking entity other than e blocks
        MoveResult move(flecs::entity e, const vector_type& direction, value_type distance) const;
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
