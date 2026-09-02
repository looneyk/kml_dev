#if !defined(QUETZAL_STRATUM_SIM_LAYOUT_HPP)
#define QUETZAL_STRATUM_SIM_LAYOUT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Layout.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <functional>
#include <vector>
#include <cassert>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // How far around, in the sense the layout's own axes are wound
    enum class Rotation
    {
        CCW,
        CW,
        Half
    };

    //--------------------------------------------------------------------------
    // Where the mover ended up, which may be short of what was asked or unmoved, and what
    // stopped it. Both nullid when nothing did, and never both set
    struct MoveResult
    {
        placement_type placement;
        id_type obstruction_wall_id = nullid; // The wall it was stopped by
        id_type obstruction_entity_id = nullid; // What was standing in the way, whatever the caller counts as one
    };

    //--------------------------------------------------------------------------
    // How one Zone is arranged and what can be asked of it: where a mover ends up, what it
    // faces, what is adjacent, what is visible, and how far one action of movement covers
    // Nothing here names a cell or a wall, and nothing here implies that either exists. A
    // layout that quantizes movement and one that integrates it answer all of this alike,
    // rounding being the layout's business rather than the caller's
    // Geometry travels outward through this toward the view and never inward: a caller asks
    // where a move ends up rather than computing a destination and handing it back
    class Layout
    {
    public:

        using placements_type = std::vector<placement_type>;

        // Whatever stands at a placement and stops a mover entering it, nullid when nothing
        // does. What counts as one is the caller's business; the layout only asks, so it
        // stays ignorant of entities while still stopping for them
        using obstructed_type = std::function<id_type(const placement_type&)>;

        virtual ~Layout() = default;

        // Where the mover ends up, stopping at the first thing that blocks it, and what that
        // was. The result may be short of the distance asked for, or unmoved
        // Topology and occupancy are answered in one sweep rather than one after the other,
        // a blocker partway along a path being able to change where the rest of it goes
        virtual MoveResult move(const placement_type& placement, const vector_type& direction, value_type distance,
            const obstructed_type& obstructed) const = 0;

        // The layout's own topology alone, nothing counting as an obstruction
        MoveResult move(const placement_type& placement, const vector_type& direction, value_type distance) const;

        virtual placement_type turn(const placement_type& placement, Rotation rotation) const = 0;

        // The placement facing the direction most nearly asked for, snapped to whichever
        // directions the layout owns, as move snaps the direction it travels in. Unchanged
        // when the direction is zero, there being nothing asked for
        virtual placement_type face(const placement_type& placement, const vector_type& direction) const = 0;

        // Whether the two stand in the same place, in whatever sense this layout resolves one
        virtual bool same_location(const placement_type& a, const placement_type& b) const = 0;

        // Adjacency for AI, the placements reachable in one step
        virtual placements_type neighbors(const placement_type& placement) const = 0;

        virtual bool visible(const placement_type& from, const placement_type& to) const = 0;

        // How far one action of movement covers, which is what a move's cost is measured
        // against
        virtual value_type step_distance() const = 0;

    protected:

        // Copied and moved as the derived layout it is, never as this. Protected rather than
        // deleted, so a concrete layout stays a value the way it was before it had a base
        Layout() = default;
        Layout(const Layout&) = default;
        Layout(Layout&&) noexcept = default;

        Layout& operator=(const Layout&) = default;
        Layout& operator=(Layout&&) noexcept = default;
    };

    //--------------------------------------------------------------------------
    inline MoveResult Layout::move(const placement_type& placement, const vector_type& direction, value_type distance) const
    {
        return move(placement, direction, distance, obstructed_type());
    }

    //--------------------------------------------------------------------------
    // The layout as the kind of layout it is, for a caller that needs more than the interface
    // says. Asking a zone for cells it has none of is the caller's error rather than a
    // question with an answer, so this asserts rather than answering with nothing
    template<typename L>
    L& layout_as(Layout& layout)
    {
        L* p = dynamic_cast<L*>(&layout);
        assert(p != nullptr);
        return *p;
    }

    //--------------------------------------------------------------------------
    template<typename L>
    const L& layout_as(const Layout& layout)
    {
        const L* p = dynamic_cast<const L*>(&layout);
        assert(p != nullptr);
        return *p;
    }

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_LAYOUT_HPP
