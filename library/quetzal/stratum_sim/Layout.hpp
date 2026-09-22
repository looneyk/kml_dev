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
    enum class Rotation
    {
        CCW,
        CW,
        Half
    };

    //--------------------------------------------------------------------------
    // Where a move ended, possibly short of the distance requested, and what blocked it
    struct MoveResult
    {
        placement_type placement;
        id_type blocking_wall_id = nullid;
        id_type blocking_entity_id = nullid;
    };

    //--------------------------------------------------------------------------
    // A zone's geometry and topology: where a move ends, what is ahead, what is adjacent, and what is visible
    class Layout
    {
    public:

        using placements_type = std::vector<placement_type>;

        // Returns the entity blocking a placement, or nullid
        using blocker_function_type = std::function<id_type(const placement_type&)>;

        // Whether the wall with the given id may be crossed
        using crossable_function_type = std::function<bool(id_type)>;

        virtual ~Layout() = default;

        virtual MoveResult move(const placement_type& placement, const vector_type& direction, value_type distance,
            const crossable_function_type& is_wall_crossable, const blocker_function_type& blocker_at) const = 0;

        // No wall is crossable and no entity blocks
        MoveResult move(const placement_type& placement, const vector_type& direction, value_type distance) const;

        virtual placement_type turn(const placement_type& placement, Rotation rotation) const = 0;

        // The placement turned to the layout direction nearest the requested one
        virtual placement_type face(const placement_type& placement, const vector_type& direction) const = 0;

        virtual bool same_location(const placement_type& a, const placement_type& b) const = 0;

        // The placements reachable in one move
        virtual placements_type neighbors(const placement_type& placement, const crossable_function_type& is_wall_crossable) const = 0;

        virtual bool visible(const placement_type& from, const placement_type& to, const crossable_function_type& is_wall_crossable) const = 0;

        // The wall directly ahead of a placement, or nullid
        virtual id_type ahead(const placement_type& placement) const = 0;

        // The distance one action of movement travels
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
    // The layout as its concrete class, which must be L
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
