#if !defined(QUETZAL_STRATUM_SIM_LAYOUT_BASE_HPP)
#define QUETZAL_STRATUM_SIM_LAYOUT_BASE_HPP
//------------------------------------------------------------------------------
// stratum_sim
// LayoutBase.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <cassert>

namespace quetzal::stratum_sim
{

    class ProximityIndexBase;

    //--------------------------------------------------------------------------
    enum class Rotation
    {
        CCW,
        CW,
        Half
    };

    //--------------------------------------------------------------------------
    // The placement where a move ended, possibly short of the distance requested, and the wall or entity that blocked it
    struct MoveResult
    {
        placement_type placement;
        id_type blocking_wall_id = nullid;
        id_type blocking_entity_id = nullid;
    };

    //--------------------------------------------------------------------------
    // A zone's geometry and topology. It determines where a move ends, which wall is ahead, which placements are adjacent,
    // and which are visible
    class LayoutBase
    {
    public:

        using placements_type = std::vector<placement_type>;

        // Returns the entity blocking a placement, or nullid
        using blocker_function_type = std::function<id_type(const placement_type&)>;

        // Returns true if the wall with the given id may be crossed
        using crossable_function_type = std::function<bool(id_type)>;

        virtual ~LayoutBase() = default;

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

        // An empty index of the entities placed in the zone, which must not outlive the layout
        virtual std::unique_ptr<ProximityIndexBase> create_proximity_index() const = 0;

    protected:

        LayoutBase() = default;
        LayoutBase(const LayoutBase&) = default;
        LayoutBase(LayoutBase&&) noexcept = default;

        LayoutBase& operator=(const LayoutBase&) = default;
        LayoutBase& operator=(LayoutBase&&) noexcept = default;
    };

    //--------------------------------------------------------------------------
    inline MoveResult LayoutBase::move(const placement_type& placement, const vector_type& direction, value_type distance) const
    {
        return move(placement, direction, distance, crossable_function_type(), blocker_function_type());
    }

    //--------------------------------------------------------------------------
    // The layout as its concrete class, which must be L
    template<typename L>
    L& layout_as(LayoutBase& layout)
    {
        L* p = dynamic_cast<L*>(&layout);
        assert(p != nullptr);
        return *p;
    }

    //--------------------------------------------------------------------------
    template<typename L>
    const L& layout_as(const LayoutBase& layout)
    {
        const L* p = dynamic_cast<const L*>(&layout);
        assert(p != nullptr);
        return *p;
    }

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_LAYOUT_BASE_HPP
