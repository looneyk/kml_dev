#if !defined(QUETZAL_STRATUM_SIM_CONTINUOUS_LAYOUT_HPP)
#define QUETZAL_STRATUM_SIM_CONTINUOUS_LAYOUT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// ContinuousLayout.hpp
//------------------------------------------------------------------------------

#include "Layout.hpp"

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Movement that integrates rather than quantizing, through geometry that is ordinary 3D
    // models rather than anything the grid implies
    // Nothing here is written. It is declared so that the case is stated rather than inferred
    // from DiscreteLayout, whose name does not mention what it leaves out. Every member is
    // pure, so nothing can construct one and nothing has to be defined
    //
    // What the discrete layout cannot stand in for:
    // - A mover is anywhere in the zone rather than in one cell or the next, so a position is
    //   not regenerated from a cell on every move and error accumulates unless something else
    //   stops it
    // - Collision is a swept path against wall geometry, ending partway or slid along a wall.
    //   Topology and occupancy solve together rather than one after the other: a blocker
    //   partway along changes where the slide happens, so a sequence gives answers that are
    //   wrong rather than approximate
    // - Walls are arbitrary polygons, so one area's single edge meets a neighbour's several
    //   and no exact match pairs them. Nothing in partner, next and prev carries over
    // - Proximity is answered by a spatial index rather than by a cell lookup, the movement
    //   grid not existing here. See the three grids in sim_zone_model.md
    // - value_type wants settling before any of it: every vertex is grid derived today and so
    //   exactly representable, which arbitrary angled walls end. See work_plan.md
    class ContinuousLayout : public Layout
    {
    public:

        // Integrates along the direction rather than stepping cell by cell, and answers with
        // wherever the collision left the mover
        MoveResult move(const placement_type& placement, const vector_type& direction, value_type distance,
            const obstructed_type& obstructed) const override = 0;

        // Rotates by whatever the layout owns rather than by a quarter turn
        placement_type turn(const placement_type& placement, Rotation rotation) const override = 0;

        // Takes the direction as it was given, there being nothing to snap it to
        placement_type face(const placement_type& placement, const vector_type& direction) const override = 0;

        // Near enough rather than rounding alike, so this carries a tolerance where the
        // discrete answer is exact
        bool same_location(const placement_type& a, const placement_type& b) const override = 0;

        // Navmesh nodes rather than cells. Both models answer adjacency with a discrete graph,
        // which is why this is on the interface at all
        placements_type neighbors(const placement_type& placement) const override = 0;

        // A ray against the wall polygons rather than a walk over the cells it crosses
        bool visible(const placement_type& from, const placement_type& to) const override = 0;

        // What the facing ray first meets, out to whatever reach the zone gives an actor's
        // arm. There is no cell edge to name, so the answer comes from the geometry and the
        // reach is the layout's rather than implied by a cell being one step wide
        id_type ahead(const placement_type& placement) const override = 0;

        // A property of the zone rather than the grid's spacing
        value_type step_distance() const override = 0;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_CONTINUOUS_LAYOUT_HPP
