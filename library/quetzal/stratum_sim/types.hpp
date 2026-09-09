#if !defined(QUETZAL_STRATUM_SIM_TYPES_HPP)
#define QUETZAL_STRATUM_SIM_TYPES_HPP
//------------------------------------------------------------------------------
// stratum_sim
// types.hpp
//------------------------------------------------------------------------------

#include "quetzal/geometry/DirectedPoint.hpp"
#include "quetzal/geometry/DiscretePoint.hpp"
#include "quetzal/geometry/MinProjection.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/DiscreteVector.hpp"
#include <vector>

namespace quetzal::stratum_sim
{

    using value_type = float;
    using traits_type = math::CartesianTraits<value_type, 3>;
    using point_type = geometry::Point<traits_type>;
    using vector_type = point_type::vector_type;
    using points_type = std::vector<point_type>;
    using polygon_type = geometry::Polygon<traits_type>;

    // Mover position and facing direction
    using placement_type = geometry::DirectedPoint<traits_type>; // rename ...

    // Simple projection along the z-axis onto the xy_plane 
    using point_projection_type = geometry::MinProjection<point_type>;
    using vector_projection_type = geometry::MinProjection<vector_type>;

    using discrete_vector_type = math::DiscreteVector<long, 2>;
    using discrete_point_type = geometry::DiscretePoint<long, 2>;

    //--------------------------------------------------------------------------
    struct Direction
    {
        inline static const discrete_vector_type East = discrete_vector_type(1, 0);
        inline static const discrete_vector_type North = discrete_vector_type(0, 1);
        inline static const discrete_vector_type West = discrete_vector_type(-1, 0);
        inline static const discrete_vector_type South = discrete_vector_type(0, -1);
    };

    // The cell step a direction through space most nearly asks for, height disregarded
    inline discrete_vector_type grid_step(const vector_type& direction)
    {
        static const vector_projection_type project;
        return math::direction<discrete_vector_type>(project(direction));
    }

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_TYPES_HPP
