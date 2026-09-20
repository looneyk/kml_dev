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
    // The destination of a move which may not be the full distance requested
    // Holds the id of the object it was blocked by or nullid
    struct MoveResult
    {
        placement_type placement;
        id_type blocking_wall_id = nullid; // The wall it was stopped by
        id_type blocking_entity_id = nullid; // What was occupying the cell ahead, whatever the caller counts as one
    };

    //--------------------------------------------------------------------------
    // The arrangement of a Zone and what can be asked of it: where a mover ends up, what it
    // faces, what is adjacent, what is visible, and how far one action of movement travels
    // Geometry travels outward through this toward the view and never inward: a caller asks
    // where a move ends up rather than computing a destination and handing it back
    class Layout
    {
    public:

        using placements_type = std::vector<placement_type>;

        // A callback function type to allow callers to define what qualifies as a blocker
        using blocker_function_type = std::function<id_type(const placement_type&)>;

        // A callback function type to allow callers to define what qualifies as crossssable
        using crossable_function_type = std::function<bool(id_type)>;

        virtual ~Layout() = default;

        // Returns the end point of a move and the blocker if it did not fully move as requested
        virtual MoveResult move(const placement_type& placement, const vector_type& direction, value_type distance,
            const crossable_function_type& is_wall_crossable, const blocker_function_type& blocker_at) const = 0;

        // The layout's own topology alone, no wall crossed and nothing counting as a blocker
        MoveResult move(const placement_type& placement, const vector_type& direction, value_type distance) const;

        virtual placement_type turn(const placement_type& placement, Rotation rotation) const = 0;

        // The resulting placement starting at the given position and most closely aligned with the requested direction
        virtual placement_type face(const placement_type& placement, const vector_type& direction) const = 0;

        virtual bool same_location(const placement_type& a, const placement_type& b) const = 0;

        // Adjacency for AI, the placements reachable in one step
        virtual placements_type neighbors(const placement_type& placement, const crossable_function_type& is_wall_crossable) const = 0;

        virtual bool visible(const placement_type& from, const placement_type& to, const crossable_function_type& is_wall_crossable) const = 0;

        // The wall directly in front of a placement if any. The layout determines what is there and
        // at what granularity, snapping the facing the way move snaps the direction it travels in
        virtual id_type ahead(const placement_type& placement) const = 0;

        // How far one action of movement travels, which is what a move's cost is measured against
        virtual value_type step_distance() const = 0;

    protected:

        Layout() = default;
        Layout(const Layout&) = default;
        Layout(Layout&&) noexcept = default;

        Layout& operator=(const Layout&) = default;
        Layout& operator=(Layout&&) noexcept = default;
    };

    //--------------------------------------------------------------------------
    inline MoveResult Layout::move(const placement_type& placement, const vector_type& direction, value_type distance) const
    {
        return move(placement, direction, distance, crossable_function_type(), blocker_function_type());
    }

    //--------------------------------------------------------------------------
    // The layout as the kind of layout it is, for a caller that needs more than the interface
    // exposes. Asking a zone for cells it has none of is the caller's error rather than a
    // question with an answer, so this asserts rather than returning nothing
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
