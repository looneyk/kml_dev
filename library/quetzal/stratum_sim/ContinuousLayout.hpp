#if !defined(QUETZAL_STRATUM_SIM_CONTINUOUS_LAYOUT_HPP)
#define QUETZAL_STRATUM_SIM_CONTINUOUS_LAYOUT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// ContinuousLayout.hpp
//------------------------------------------------------------------------------

#include "LayoutBase.hpp"

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Movement that integrates rather than quantizing, through geometry that is ordinary 3D
    // models rather than anything a grid could imply.
    // No current implementation
    //
    // - Continuous, unrestricted movement
    // - Arbitrary geometry with no fixed relation between any two sections
    // - Proximity is resolved by a spatial index
    // - Collision is a swept path against wall geometry, ending partway or sliding along a wall.
    //   Topology and occupancy solve together rather than one after the other: a blocker
    //   partway along changes where the slide happens, so a sequence gives results that are
    //   wrong rather than approximate
    class ContinuousLayout : public LayoutBase
    {
    public:

        // Integrates along the direction rather than stepping cell by cell, and returns
        // wherever the collision left the mover
        MoveResult move(const placement_type& placement, const vector_type& direction, value_type distance,
            const crossable_function_type& is_wall_crossable, const blocker_function_type& blocker_at) const override = 0;

        placement_type turn(const placement_type& placement, Rotation rotation) const override = 0;

        placement_type face(const placement_type& placement, const vector_type& direction) const override = 0;

        // Near enough within a tolerance
        bool same_location(const placement_type& a, const placement_type& b) const override = 0;

        // Navmesh nodes rather than cells. Both models express adjacency as a discrete graph,
        // which is why this is on the interface at all
        placements_type neighbors(const placement_type& placement, const crossable_function_type& is_wall_crossable) const override = 0;

        // A ray against the wall polygons
        bool visible(const placement_type& from, const placement_type& to, const crossable_function_type& is_wall_crossable) const override = 0;

        // What the facing ray first meets, out to whatever reach the zone allows an actor's
        // arm.
        id_type ahead(const placement_type& placement) const override = 0;

        value_type step_distance() const override = 0;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_CONTINUOUS_LAYOUT_HPP
